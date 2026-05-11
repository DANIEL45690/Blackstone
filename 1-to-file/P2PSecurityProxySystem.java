package p2p.security.proxy;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.security.KeyStore;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Base64;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import javax.net.ssl.KeyManager;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;

public class P2PSecurityProxySystem {

    private static final int MAX_PACKET_SIZE = 65536;
    private static final int BUFFER_SIZE = 8192;
    private static final int PROXY_PORT = 8888;
    private static final int CONTROL_PORT = 9999;
    private static final int CONNECTION_TIMEOUT = 30000;
    private static final int THREAD_POOL_SIZE = 200;
    private static final long IP_BAN_DURATION = 3600000;
    private static final int MAX_CONNECTIONS_PER_IP = 50;
    private static final int RATE_LIMIT_REQUESTS = 100;
    private static final long RATE_LIMIT_WINDOW = 60000;

    private final Map<String, IpBanEntry> bannedIps = new ConcurrentHashMap<>();
    private final Map<String, ConnectionTracker> connectionTrackers = new ConcurrentHashMap<>();
    private final Map<String, RateLimiter> rateLimiters = new ConcurrentHashMap<>();
    private final Map<String, ProxySession> activeSessions = new ConcurrentHashMap<>();
    private final Map<String, byte[]> authTokens = new ConcurrentHashMap<>();
    private final BlockingQueue<PacketData> packetQueue = new LinkedBlockingQueue<>();
    private final ExecutorService workerPool;
    private final SSLContext sslContext;
    private final KeyManager[] keyManagers;
    private final TrustManager[] trustManagers;
    private final SecurityManager securityManager;
    private final ProxyProtocolManager protocolManager;
    private final FirewallManager firewallManager;
    private final TrafficEncryptor trafficEncryptor;
    private final DdosProtector ddosProtector;
    private final Anonymizer anonymizer;
    private volatile boolean running = true;
    private final AtomicLong totalPacketsProcessed = new AtomicLong(0);
    private final AtomicLong totalBytesTransferred = new AtomicLong(0);

    public P2PSecurityProxySystem() throws Exception {
        this.workerPool = Executors.newFixedThreadPool(THREAD_POOL_SIZE);
        this.securityManager = new SecurityManager();
        this.protocolManager = new ProxyProtocolManager();
        this.firewallManager = new FirewallManager();
        this.trafficEncryptor = new TrafficEncryptor();
        this.ddosProtector = new DdosProtector();
        this.anonymizer = new Anonymizer();

        KeyStore keyStore = KeyStore.getInstance("JKS");
        keyStore.load(null, null);
        KeyGenerator keyGen = KeyGenerator.getInstance("AES");
        keyGen.init(256);
        SecretKey secretKey = keyGen.generateKey();
        keyStore.setKeyEntry("p2p-key", secretKey, "password".toCharArray(), null);

        KeyManagerFactory kmf = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
        kmf.init(keyStore, "password".toCharArray());
        this.keyManagers = kmf.getKeyManagers();

        TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
        tmf.init(keyStore);
        this.trustManagers = tmf.getTrustManagers();

        this.sslContext = SSLContext.getInstance("TLSv1.3");
        this.sslContext.init(keyManagers, trustManagers, new SecureRandom());

        startProxyServer();
        startControlServer();
        startPacketProcessor();
        startCleanupScheduler();
    }

    private void startProxyServer() {
        workerPool.submit(() -> {
            try (ServerSocketChannel serverChannel = ServerSocketChannel.open()) {
                serverChannel.bind(new InetSocketAddress(PROXY_PORT));
                serverChannel.configureBlocking(false);
                Selector selector = Selector.open();
                serverChannel.register(selector, SelectionKey.OP_ACCEPT);

                while (running) {
                    selector.select(1000);
                    Iterator<SelectionKey> keys = selector.selectedKeys().iterator();
                    while (keys.hasNext()) {
                        SelectionKey key = keys.next();
                        keys.remove();
                        if (key.isAcceptable()) {
                            handleAccept(serverChannel, selector);
                        } else if (key.isReadable()) {
                            handleRead(key);
                        } else if (key.isWritable()) {
                            handleWrite(key);
                        }
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
    }

    private void handleAccept(ServerSocketChannel serverChannel, Selector selector) throws Exception {
        SocketChannel clientChannel = serverChannel.accept();
        clientChannel.configureBlocking(false);
        String clientIp = clientChannel.getRemoteAddress().toString();

        if (firewallManager.isBlocked(clientIp)) {
            clientChannel.close();
            return;
        }

        ConnectionTracker tracker = connectionTrackers.computeIfAbsent(clientIp, k -> new ConnectionTracker());
        if (tracker.incrementAndCheckLimit()) {
            firewallManager.blockIp(clientIp, "Connection limit exceeded");
            clientChannel.close();
            return;
        }

        ProxySession session = new ProxySession(clientChannel, clientIp);
        activeSessions.put(session.getSessionId(), session);
        clientChannel.register(selector, SelectionKey.OP_READ, session);
    }

    private void handleRead(SelectionKey key) throws Exception {
        SocketChannel channel = (SocketChannel) key.channel();
        ProxySession session = (ProxySession) key.attachment();
        ByteBuffer buffer = ByteBuffer.allocate(BUFFER_SIZE);

        int bytesRead = channel.read(buffer);
        if (bytesRead == -1) {
            closeSession(session);
            return;
        }

        buffer.flip();
        byte[] data = new byte[bytesRead];
        buffer.get(data);

        if (session.isEncrypted()) {
            data = trafficEncryptor.decrypt(data, session.getSessionKey());
        }

        RateLimiter limiter = rateLimiters.computeIfAbsent(session.getClientIp(), k -> new RateLimiter());
        if (!limiter.allowRequest()) {
            firewallManager.blockIp(session.getClientIp(), "Rate limit exceeded");
            closeSession(session);
            return;
        }

        PacketData packet = new PacketData(session.getSessionId(), session.getClientIp(), data,
                System.currentTimeMillis());
        packetQueue.offer(packet);
        totalBytesTransferred.addAndGet(bytesRead);
    }

    private void handleWrite(SelectionKey key) throws Exception {
        SocketChannel channel = (SocketChannel) key.channel();
        ProxySession session = (ProxySession) key.attachment();
        ByteBuffer buffer = (ByteBuffer) key.attachment();

        if (buffer != null && buffer.hasRemaining()) {
            channel.write(buffer);
            if (!buffer.hasRemaining()) {
                key.interestOps(SelectionKey.OP_READ);
            }
        }
    }

    private void startControlServer() {
        workerPool.submit(() -> {
            try (ServerSocket controlSocket = new ServerSocket(CONTROL_PORT)) {
                while (running) {
                    Socket clientSocket = controlSocket.accept();
                    clientSocket.setSoTimeout(CONNECTION_TIMEOUT);
                    workerPool.submit(() -> handleControlConnection(clientSocket));
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
    }

    private void handleControlConnection(Socket clientSocket) {
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
                PrintWriter writer = new PrintWriter(clientSocket.getOutputStream(), true)) {

            String authLine = reader.readLine();
            if (!authenticateControl(authLine)) {
                writer.println("ERROR: Authentication failed");
                return;
            }

            writer.println("OK: Connected to P2P Security Proxy");
            String command;
            while ((command = reader.readLine()) != null && running) {
                String response = processControlCommand(command);
                writer.println(response);
                if (command.equalsIgnoreCase("SHUTDOWN")) {
                    break;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private boolean authenticateControl(String authLine) {
        return authLine != null && authLine.startsWith("AUTH ") &&
                authLine.substring(5).equals(System.getProperty("p2p.auth.token", "default_token_2024"));
    }

    private String processControlCommand(String command) {
        String[] parts = command.split(" ");
        switch (parts[0].toUpperCase()) {
            case "STATS":
                return String.format("STATS: Packets=%d, Bytes=%d, Sessions=%d, Banned=%d",
                        totalPacketsProcessed.get(), totalBytesTransferred.get(),
                        activeSessions.size(), bannedIps.size());
            case "BAN":
                if (parts.length > 1) {
                    firewallManager.blockIp(parts[1], "Manual ban");
                    return "OK: Banned " + parts[1];
                }
                return "ERROR: Missing IP";
            case "UNBAN":
                if (parts.length > 1) {
                    firewallManager.unblockIp(parts[1]);
                    return "OK: Unbanned " + parts[1];
                }
                return "ERROR: Missing IP";
            case "LIST_BANS":
                return "BANNED_IPS: " + String.join(", ", firewallManager.getBlockedIps());
            case "CLEAR_BANS":
                firewallManager.clearBans();
                return "OK: All bans cleared";
            case "SHUTDOWN":
                shutdown();
                return "OK: Shutting down";
            default:
                return "ERROR: Unknown command";
        }
    }

    private void startPacketProcessor() {
        for (int i = 0; i < THREAD_POOL_SIZE / 4; i++) {
            workerPool.submit(() -> {
                while (running) {
                    try {
                        PacketData packet = packetQueue.take();
                        processPacket(packet);
                        totalPacketsProcessed.incrementAndGet();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        break;
                    }
                }
            });
        }
    }

    private void processPacket(PacketData packet) {
        if (firewallManager.isBlocked(packet.sourceIp)) {
            return;
        }

        if (ddosProtector.isAttackDetected(packet.sourceIp)) {
            firewallManager.blockIp(packet.sourceIp, "DDoS attack detected");
            return;
        }

        ProxySession session = activeSessions.get(packet.sessionId);
        if (session == null) {
            return;
        }

        PacketType type = protocolManager.identifyPacket(packet.data);
        switch (type) {
            case HANDSHAKE:
                handleHandshake(session, packet);
                break;
            case DATA:
                handleDataPacket(session, packet);
                break;
            case CONTROL:
                handleControlPacket(session, packet);
                break;
            case P2P:
                handleP2PPacket(session, packet);
                break;
            case HEARTBEAT:
                updateHeartbeat(session);
                break;
            default:
                handleUnknownPacket(session, packet);
        }
    }

    private void handleHandshake(ProxySession session, PacketData packet) {
        try {
            byte[] decrypted = trafficEncryptor.decrypt(packet.data, session.getTempKey());
            HandshakeMessage handshake = HandshakeMessage.fromBytes(decrypted);

            if (handshake.validate()) {
                byte[] sessionKey = trafficEncryptor.generateSessionKey();
                session.setSessionKey(sessionKey);
                session.setEncrypted(true);

                byte[] response = HandshakeMessage.createResponse(sessionKey).toBytes();
                byte[] encryptedResponse = trafficEncryptor.encrypt(response, session.getTempKey());
                sendToClient(session, encryptedResponse);

                String anonymizedIp = anonymizer.anonymizeIp(session.getClientIp());
                session.setAnonymizedIp(anonymizedIp);
            } else {
                closeSession(session);
            }
        } catch (Exception e) {
            closeSession(session);
        }
    }

    private void handleDataPacket(ProxySession session, PacketData packet) {
        byte[] processed = protocolManager.processDataPacket(packet.data);
        if (processed != null) {
            String targetPeer = extractTargetPeer(processed);
            if (targetPeer != null) {
                forwardToPeer(targetPeer, processed, session);
            }
            sendToClient(session, protocolManager.createAcknowledge(packet.sequenceId));
        }
    }

    private void handleControlPacket(ProxySession session, PacketData packet) {
        try {
            ControlMessage message = ControlMessage.fromBytes(packet.data);
            String type = message.getType();
            if ("CONNECT".equals(type)) {
                handleConnectRequest(session, message);
            } else if ("DISCONNECT".equals(type)) {
                handleDisconnectRequest(session, message);
            } else if ("STATUS".equals(type)) {
                sendStatusResponse(session);
            } else if ("CONFIG".equals(type)) {
                updateSessionConfig(session, message);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void handleP2PPacket(ProxySession session, PacketData packet) {
        try {
            P2PMessage p2pMessage = P2PMessage.fromBytes(packet.data);
            String destination = p2pMessage.getDestination();

            if (destination.equals(session.getClientIp())) {
                processLocalP2PMessage(session, p2pMessage);
            } else {
                ProxySession targetSession = activeSessions.values().stream()
                        .filter(s -> s.getClientIp().equals(destination) || s.getAnonymizedIp().equals(destination))
                        .findFirst()
                        .orElse(null);

                if (targetSession != null) {
                    forwardP2PMessage(targetSession, p2pMessage, session);
                } else {
                    queueForDelivery(p2pMessage);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void processLocalP2PMessage(ProxySession session, P2PMessage message) {
        byte[] response = protocolManager.processMessage(message.getPayload(), message.getType());
        try {
            P2PMessage responseMsg = new P2PMessage(session.getAnonymizedIp(), message.getSource(), response,
                    message.getType());
            sendToClient(session, responseMsg.toBytes());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void forwardP2PMessage(ProxySession target, P2PMessage message, ProxySession source) {
        try {
            byte[] encrypted = trafficEncryptor.encrypt(message.toBytes(), target.getSessionKey());
            sendToClient(target, encrypted);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void queueForDelivery(P2PMessage message) {
        protocolManager.queueMessage(message);
    }

    private void handleConnectRequest(ProxySession session, ControlMessage message) {
        String targetIp = message.getParameter("target_ip");
        int targetPort = Integer.parseInt(message.getParameter("target_port"));

        try {
            SocketChannel targetChannel = SocketChannel.open();
            targetChannel.connect(new InetSocketAddress(targetIp, targetPort));
            targetChannel.configureBlocking(false);
            session.setTargetChannel(targetChannel);
            session.setConnected(true);

            ControlMessage response = ControlMessage.createResponse("CONNECT_OK",
                    "Connected to " + targetIp + ":" + targetPort);
            sendToClient(session, response.toBytes());
        } catch (Exception e) {
            try {
                ControlMessage response = ControlMessage.createResponse("CONNECT_FAIL", e.getMessage());
                sendToClient(session, response.toBytes());
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    private void handleDisconnectRequest(ProxySession session, ControlMessage message) {
        if (session.isConnected() && session.getTargetChannel() != null) {
            try {
                session.getTargetChannel().close();
            } catch (Exception e) {
            }
            session.setConnected(false);
        }
        try {
            ControlMessage response = ControlMessage.createResponse("DISCONNECT_OK", "Disconnected");
            sendToClient(session, response.toBytes());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void sendStatusResponse(ProxySession session) {
        String status = String.format("STATUS: connected=%b, packets=%d, bytes=%d, session_time=%d",
                session.isConnected(), session.getPacketsSent(), session.getBytesSent(),
                System.currentTimeMillis() - session.getCreationTime());
        sendToClient(session, status.getBytes());
    }

    private void updateSessionConfig(ProxySession session, ControlMessage message) {
        if (message.hasParameter("encryption")) {
            session.setEncrypted(Boolean.parseBoolean(message.getParameter("encryption")));
        }
        if (message.hasParameter("compression")) {
            session.setCompressionEnabled(Boolean.parseBoolean(message.getParameter("compression")));
        }
        if (message.hasParameter("anonymization")) {
            session.setAnonymizationEnabled(Boolean.parseBoolean(message.getParameter("anonymization")));
        }
    }

    private void handleUnknownPacket(ProxySession session, PacketData packet) {
        if (packet.data.length > 0) {
            byte[] hash = securityManager.calculateHash(packet.data);
            securityManager.logSuspiciousActivity(session.getClientIp(), "Unknown packet type", hash);
        }
    }

    private void updateHeartbeat(ProxySession session) {
        session.updateLastHeartbeat();
    }

    private void sendToClient(ProxySession session, byte[] data) {
        try {
            if (session.isCompressionEnabled()) {
                data = compressData(data);
            }

            if (session.isEncrypted() && session.getSessionKey() != null) {
                data = trafficEncryptor.encrypt(data, session.getSessionKey());
            }

            ByteBuffer buffer = ByteBuffer.wrap(data);
            while (buffer.hasRemaining()) {
                session.getChannel().write(buffer);
            }
            session.incrementPacketsSent();
            session.addBytesSent(data.length);
        } catch (Exception e) {
            closeSession(session);
        }
    }

    private byte[] compressData(byte[] data) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try (GZIPOutputStream gzipOut = new GZIPOutputStream(baos)) {
            gzipOut.write(data);
        }
        return baos.toByteArray();
    }

    private byte[] decompressData(byte[] data) throws IOException {
        ByteArrayInputStream bais = new ByteArrayInputStream(data);
        try (GZIPInputStream gzipIn = new GZIPInputStream(bais);
                ByteArrayOutputStream baos = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[BUFFER_SIZE];
            int len;
            while ((len = gzipIn.read(buffer)) != -1) {
                baos.write(buffer, 0, len);
            }
            return baos.toByteArray();
        }
    }

    private String extractTargetPeer(byte[] data) {
        Pattern pattern = Pattern.compile("PEER:([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})");
        Matcher matcher = pattern.matcher(new String(data));
        return matcher.find() ? matcher.group(1) : null;
    }

    private void forwardToPeer(String targetIp, byte[] data, ProxySession sourceSession) {
        ProxySession targetSession = activeSessions.values().stream()
                .filter(s -> s.getClientIp().equals(targetIp) || s.getAnonymizedIp().equals(targetIp))
                .findFirst()
                .orElse(null);

        if (targetSession != null) {
            sendToClient(targetSession, data);
        } else {
            try {
                queueForDelivery(new P2PMessage(sourceSession.getAnonymizedIp(), targetIp, data, "DATA"));
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    private void startCleanupScheduler() {
        ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor();
        scheduler.scheduleAtFixedRate(() -> {
            long now = System.currentTimeMillis();
            activeSessions.values().removeIf(session -> {
                if (now - session.getLastHeartbeat() > CONNECTION_TIMEOUT) {
                    closeSession(session);
                    return true;
                }
                return false;
            });

            bannedIps.entrySet().removeIf(entry -> now - entry.getValue().banTime > IP_BAN_DURATION);

            connectionTrackers.entrySet().removeIf(entry -> {
                entry.getValue().cleanup();
                return entry.getValue().isEmpty();
            });

            rateLimiters.entrySet().removeIf(entry -> entry.getValue().isExpired());
        }, 60, 60, TimeUnit.SECONDS);
    }

    private void closeSession(ProxySession session) {
        try {
            if (session.getChannel().isOpen()) {
                session.getChannel().close();
            }
            if (session.getTargetChannel() != null && session.getTargetChannel().isOpen()) {
                session.getTargetChannel().close();
            }
        } catch (Exception e) {
        } finally {
            activeSessions.remove(session.getSessionId());
            connectionTrackers.computeIfPresent(session.getClientIp(), (k, v) -> {
                v.decrement();
                return v;
            });
        }
    }

    private void shutdown() {
        running = false;
        activeSessions.values().forEach(this::closeSession);
        workerPool.shutdown();
        try {
            workerPool.awaitTermination(30, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        System.exit(0);
    }

    public static void main(String[] args) {
        try {
            System.setProperty("p2p.auth.token", UUID.randomUUID().toString());
            new P2PSecurityProxySystem();
            System.out.println("P2P Security Proxy System started on ports " + PROXY_PORT + " (proxy) and "
                    + CONTROL_PORT + " (control)");
            System.out.println("Auth token: " + System.getProperty("p2p.auth.token"));
        } catch (Exception e) {
            System.err.println("Failed to start P2P Security Proxy System: " + e.getMessage());
            e.printStackTrace();
        }
    }

    private enum PacketType {
        HANDSHAKE, DATA, CONTROL, P2P, HEARTBEAT, UNKNOWN
    }

    private static class ProxySession {
        private final String sessionId;
        private final SocketChannel channel;
        private final String clientIp;
        private final long creationTime;
        private String anonymizedIp;
        private byte[] sessionKey;
        private byte[] tempKey;
        private boolean encrypted;
        private boolean compressionEnabled;
        private boolean anonymizationEnabled;
        private SocketChannel targetChannel;
        private boolean connected;
        private long lastHeartbeat;
        private long packetsSent;
        private long bytesSent;

        public ProxySession(SocketChannel channel, String clientIp) {
            this.sessionId = UUID.randomUUID().toString();
            this.channel = channel;
            this.clientIp = clientIp;
            this.creationTime = System.currentTimeMillis();
            this.lastHeartbeat = creationTime;
            this.tempKey = generateTempKey();
            this.encrypted = true;
            this.compressionEnabled = true;
            this.anonymizationEnabled = true;
        }

        private byte[] generateTempKey() {
            byte[] key = new byte[32];
            new SecureRandom().nextBytes(key);
            return key;
        }

        public String getSessionId() {
            return sessionId;
        }

        public SocketChannel getChannel() {
            return channel;
        }

        public String getClientIp() {
            return clientIp;
        }

        public long getCreationTime() {
            return creationTime;
        }

        public String getAnonymizedIp() {
            return anonymizedIp;
        }

        public void setAnonymizedIp(String ip) {
            this.anonymizedIp = ip;
        }

        public byte[] getSessionKey() {
            return sessionKey;
        }

        public void setSessionKey(byte[] key) {
            this.sessionKey = key;
        }

        public byte[] getTempKey() {
            return tempKey;
        }

        public boolean isEncrypted() {
            return encrypted;
        }

        public void setEncrypted(boolean enc) {
            this.encrypted = enc;
        }

        public boolean isCompressionEnabled() {
            return compressionEnabled;
        }

        public void setCompressionEnabled(boolean comp) {
            this.compressionEnabled = comp;
        }

        public boolean isAnonymizationEnabled() {
            return anonymizationEnabled;
        }

        public void setAnonymizationEnabled(boolean anon) {
            this.anonymizationEnabled = anon;
        }

        public SocketChannel getTargetChannel() {
            return targetChannel;
        }

        public void setTargetChannel(SocketChannel ch) {
            this.targetChannel = ch;
        }

        public boolean isConnected() {
            return connected;
        }

        public void setConnected(boolean conn) {
            this.connected = conn;
        }

        public long getLastHeartbeat() {
            return lastHeartbeat;
        }

        public void updateLastHeartbeat() {
            this.lastHeartbeat = System.currentTimeMillis();
        }

        public long getPacketsSent() {
            return packetsSent;
        }

        public void incrementPacketsSent() {
            this.packetsSent++;
        }

        public long getBytesSent() {
            return bytesSent;
        }

        public void addBytesSent(long bytes) {
            this.bytesSent += bytes;
        }
    }

    private static class PacketData {
        final String sessionId;
        final String sourceIp;
        final byte[] data;
        final long timestamp;
        final long sequenceId;
        private static final AtomicLong seqCounter = new AtomicLong(0);

        PacketData(String sessionId, String sourceIp, byte[] data, long timestamp) {
            this.sessionId = sessionId;
            this.sourceIp = sourceIp;
            this.data = data;
            this.timestamp = timestamp;
            this.sequenceId = seqCounter.incrementAndGet();
        }
    }

    private static class HandshakeMessage {
        private final int version;
        private final String clientId;
        private final byte[] publicKey;
        private final long timestamp;

        HandshakeMessage(int version, String clientId, byte[] publicKey, long timestamp) {
            this.version = version;
            this.clientId = clientId;
            this.publicKey = publicKey;
            this.timestamp = timestamp;
        }

        boolean validate() {
            return version == 1 && clientId != null && publicKey != null && publicKey.length == 32 &&
                    Math.abs(System.currentTimeMillis() - timestamp) < 30000;
        }

        byte[] toBytes() throws IOException {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(baos);
            dos.writeInt(version);
            dos.writeUTF(clientId);
            dos.writeInt(publicKey.length);
            dos.write(publicKey);
            dos.writeLong(timestamp);
            return baos.toByteArray();
        }

        static HandshakeMessage fromBytes(byte[] data) throws IOException {
            DataInputStream dis = new DataInputStream(new ByteArrayInputStream(data));
            int version = dis.readInt();
            String clientId = dis.readUTF();
            int keyLen = dis.readInt();
            byte[] pubKey = new byte[keyLen];
            dis.readFully(pubKey);
            long timestamp = dis.readLong();
            return new HandshakeMessage(version, clientId, pubKey, timestamp);
        }

        static HandshakeMessage createResponse(byte[] sessionKey) {
            return new HandshakeMessage(1, "proxy-server", sessionKey, System.currentTimeMillis());
        }
    }

    private static class ControlMessage {
        private final String type;
        private final Map<String, String> parameters;

        ControlMessage(String type, Map<String, String> params) {
            this.type = type;
            this.parameters = params;
        }

        String getType() {
            return type;
        }

        String getParameter(String key) {
            return parameters.get(key);
        }

        boolean hasParameter(String key) {
            return parameters.containsKey(key);
        }

        byte[] toBytes() throws IOException {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            ObjectOutputStream oos = new ObjectOutputStream(baos);
            oos.writeUTF(type);
            oos.writeObject(parameters);
            return baos.toByteArray();
        }

        @SuppressWarnings("unchecked")
        static ControlMessage fromBytes(byte[] data) throws IOException, ClassNotFoundException {
            ObjectInputStream ois = new ObjectInputStream(new ByteArrayInputStream(data));
            String type = ois.readUTF();
            Map<String, String> params = (Map<String, String>) ois.readObject();
            return new ControlMessage(type, params);
        }

        static ControlMessage createResponse(String status, String message) {
            Map<String, String> params = new HashMap<>();
            params.put("status", status);
            params.put("message", message);
            return new ControlMessage("RESPONSE", params);
        }
    }

    private static class P2PMessage {
        private final String source;
        private final String destination;
        private final byte[] payload;
        private final String type;
        private final long messageId;

        P2PMessage(String source, String destination, byte[] payload, String type) {
            this.source = source;
            this.destination = destination;
            this.payload = payload;
            this.type = type;
            this.messageId = System.nanoTime();
        }

        String getSource() {
            return source;
        }

        String getDestination() {
            return destination;
        }

        byte[] getPayload() {
            return payload;
        }

        String getType() {
            return type;
        }

        byte[] toBytes() throws IOException {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(baos);
            dos.writeUTF(source);
            dos.writeUTF(destination);
            dos.writeInt(payload.length);
            dos.write(payload);
            dos.writeUTF(type);
            dos.writeLong(messageId);
            return baos.toByteArray();
        }

        static P2PMessage fromBytes(byte[] data) throws IOException {
            DataInputStream dis = new DataInputStream(new ByteArrayInputStream(data));
            String source = dis.readUTF();
            String destination = dis.readUTF();
            int payloadLen = dis.readInt();
            byte[] payload = new byte[payloadLen];
            dis.readFully(payload);
            String type = dis.readUTF();
            dis.readLong();
            return new P2PMessage(source, destination, payload, type);
        }
    }

    private static class IpBanEntry {
        final String ip;
        final String reason;
        final long banTime;

        IpBanEntry(String ip, String reason) {
            this.ip = ip;
            this.reason = reason;
            this.banTime = System.currentTimeMillis();
        }
    }

    private static class ConnectionTracker {
        private final AtomicInteger activeConnections = new AtomicInteger(0);
        private final AtomicInteger totalConnections = new AtomicInteger(0);
        private long lastReset = System.currentTimeMillis();

        boolean incrementAndCheckLimit() {
            long now = System.currentTimeMillis();
            if (now - lastReset > 60000) {
                totalConnections.set(0);
                lastReset = now;
            }
            int total = totalConnections.incrementAndGet();
            if (total > MAX_CONNECTIONS_PER_IP) {
                return true;
            }
            activeConnections.incrementAndGet();
            return false;
        }

        void decrement() {
            activeConnections.decrementAndGet();
        }

        void cleanup() {
            if (System.currentTimeMillis() - lastReset > 60000) {
                totalConnections.set(0);
                activeConnections.set(0);
            }
        }

        boolean isEmpty() {
            return activeConnections.get() == 0 && totalConnections.get() == 0;
        }
    }

    private static class RateLimiter {
        private final Queue<Long> requestTimes = new LinkedList<>();

        synchronized boolean allowRequest() {
            long now = System.currentTimeMillis();
            while (!requestTimes.isEmpty() && requestTimes.peek() < now - RATE_LIMIT_WINDOW) {
                requestTimes.poll();
            }
            if (requestTimes.size() < RATE_LIMIT_REQUESTS) {
                requestTimes.offer(now);
                return true;
            }
            return false;
        }

        boolean isExpired() {
            return requestTimes.isEmpty() ||
                    (System.currentTimeMillis() - requestTimes.peek() > RATE_LIMIT_WINDOW * 2);
        }
    }

    private static class ProxyProtocolManager {
        private final Map<String, Queue<P2PMessage>> messageQueues = new ConcurrentHashMap<>();

        PacketType identifyPacket(byte[] data) {
            if (data == null || data.length < 4)
                return PacketType.UNKNOWN;
            String magic = new String(data, 0, Math.min(4, data.length));
            switch (magic) {
                case "P2PH":
                    return PacketType.HANDSHAKE;
                case "P2PD":
                    return PacketType.DATA;
                case "P2PC":
                    return PacketType.CONTROL;
                case "P2PP":
                    return PacketType.P2P;
                case "P2HB":
                    return PacketType.HEARTBEAT;
                default:
                    return PacketType.UNKNOWN;
            }
        }

        byte[] processDataPacket(byte[] data) {
            if (data.length > 4 && new String(data, 0, 4).equals("P2PD")) {
                byte[] actualData = new byte[data.length - 4];
                System.arraycopy(data, 4, actualData, 0, actualData.length);
                return actualData;
            }
            return data;
        }

        byte[] createAcknowledge(long sequenceId) {
            try {
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                DataOutputStream dos = new DataOutputStream(baos);
                dos.writeBytes("ACK");
                dos.writeLong(sequenceId);
                return baos.toByteArray();
            } catch (Exception e) {
                return new byte[0];
            }
        }

        byte[] processMessage(byte[] payload, String type) {
            if ("ECHO".equals(type)) {
                return payload;
            } else if ("STATUS".equals(type)) {
                return "OK".getBytes();
            }
            return payload;
        }

        void queueMessage(P2PMessage message) {
            messageQueues.computeIfAbsent(message.getDestination(), k -> new ConcurrentLinkedQueue<>()).offer(message);
        }
    }

    private static class FirewallManager {
        private final Map<String, IpBanEntry> bannedIps = new ConcurrentHashMap<>();
        private final List<String> whitelist = new CopyOnWriteArrayList<>();
        private final List<String> blacklist = new CopyOnWriteArrayList<>();

        boolean isBlocked(String ip) {
            if (whitelist.contains(ip))
                return false;
            if (blacklist.contains(ip))
                return true;
            IpBanEntry entry = bannedIps.get(ip);
            if (entry != null) {
                if (System.currentTimeMillis() - entry.banTime > IP_BAN_DURATION) {
                    bannedIps.remove(ip);
                    return false;
                }
                return true;
            }
            return false;
        }

        void blockIp(String ip, String reason) {
            bannedIps.put(ip, new IpBanEntry(ip, reason));
            blacklist.add(ip);
        }

        void unblockIp(String ip) {
            bannedIps.remove(ip);
            blacklist.remove(ip);
        }

        void addToWhitelist(String ip) {
            whitelist.add(ip);
            bannedIps.remove(ip);
        }

        Set<String> getBlockedIps() {
            return new HashSet<>(bannedIps.keySet());
        }

        void clearBans() {
            bannedIps.clear();
            blacklist.clear();
        }
    }

    private static class TrafficEncryptor {
        private final SecureRandom random = new SecureRandom();

        byte[] encrypt(byte[] data, byte[] key) throws Exception {
            SecretKeySpec keySpec = new SecretKeySpec(key, "AES");
            Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
            byte[] iv = new byte[12];
            random.nextBytes(iv);
            GCMParameterSpec gcmSpec = new GCMParameterSpec(128, iv);
            cipher.init(Cipher.ENCRYPT_MODE, keySpec, gcmSpec);
            byte[] encrypted = cipher.doFinal(data);
            byte[] result = new byte[iv.length + encrypted.length];
            System.arraycopy(iv, 0, result, 0, iv.length);
            System.arraycopy(encrypted, 0, result, iv.length, encrypted.length);
            return result;
        }

        byte[] decrypt(byte[] data, byte[] key) throws Exception {
            byte[] iv = new byte[12];
            byte[] encrypted = new byte[data.length - 12];
            System.arraycopy(data, 0, iv, 0, 12);
            System.arraycopy(data, 12, encrypted, 0, encrypted.length);
            SecretKeySpec keySpec = new SecretKeySpec(key, "AES");
            Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
            GCMParameterSpec gcmSpec = new GCMParameterSpec(128, iv);
            cipher.init(Cipher.DECRYPT_MODE, keySpec, gcmSpec);
            return cipher.doFinal(encrypted);
        }

        byte[] generateSessionKey() {
            byte[] key = new byte[32];
            random.nextBytes(key);
            return key;
        }
    }

    private static class DdosProtector {
        private final Map<String, DdosMetrics> metrics = new ConcurrentHashMap<>();
        private final int THRESHOLD_PACKETS = 1000;
        private final int THRESHOLD_BYTES = 10 * 1024 * 1024;
        private final long WINDOW_MS = 5000;

        boolean isAttackDetected(String ip) {
            DdosMetrics m = metrics.computeIfAbsent(ip, k -> new DdosMetrics());
            long now = System.currentTimeMillis();
            synchronized (m) {
                if (now - m.windowStart > WINDOW_MS) {
                    m.packetCount = 0;
                    m.byteCount = 0;
                    m.windowStart = now;
                }
                return m.packetCount > THRESHOLD_PACKETS || m.byteCount > THRESHOLD_BYTES;
            }
        }

        void recordPacket(String ip, int size) {
            DdosMetrics m = metrics.get(ip);
            if (m != null) {
                synchronized (m) {
                    m.packetCount++;
                    m.byteCount += size;
                }
            }
        }

        private static class DdosMetrics {
            long packetCount = 0;
            long byteCount = 0;
            long windowStart = System.currentTimeMillis();
        }
    }

    private static class Anonymizer {
        private final Map<String, String> ipMapping = new ConcurrentHashMap<>();
        private final AtomicLong counter = new AtomicLong(1);

        String anonymizeIp(String realIp) {
            return ipMapping.computeIfAbsent(realIp, k -> "anon-" + counter.getAndIncrement() + ".p2p");
        }

        String deanonymizeIp(String anonymizedIp) {
            for (Map.Entry<String, String> entry : ipMapping.entrySet()) {
                if (entry.getValue().equals(anonymizedIp)) {
                    return entry.getKey();
                }
            }
            return null;
        }
    }

    private static class SecurityManager {
        private final MessageDigest digest;
        private final List<String> suspiciousActivities = new CopyOnWriteArrayList<>();

        SecurityManager() {
            try {
                this.digest = MessageDigest.getInstance("SHA-256");
            } catch (NoSuchAlgorithmException e) {
                throw new RuntimeException(e);
            }
        }

        byte[] calculateHash(byte[] data) {
            return digest.digest(data);
        }

        void logSuspiciousActivity(String ip, String activity, byte[] hash) {
            String entry = String.format("[%s] IP: %s, Activity: %s, Hash: %s",
                    LocalDateTime.now(), ip, activity, Base64.getEncoder().encodeToString(hash));
            suspiciousActivities.add(entry);
            if (suspiciousActivities.size() > 10000) {
                suspiciousActivities.remove(0);
            }
        }

        List<String> getSuspiciousActivities() {
            return new ArrayList<>(suspiciousActivities);
        }
    }
}
