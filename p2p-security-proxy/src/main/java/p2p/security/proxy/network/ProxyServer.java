package p2p.security.proxy.network;

import p2p.security.proxy.config.ProxyConfig;
import p2p.security.proxy.P2PSecurityProxySystem;
import p2p.security.proxy.model.ProxySession;
import p2p.security.proxy.model.PacketData;
import p2p.security.proxy.handler.ConnectionHandler;

import java.io.*;
import java.net.*;
import java.nio.ByteBuffer;
import java.nio.channels.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

public class ProxyServer {

    private final ProxyConfig config;
    private final P2PSecurityProxySystem system;
    private final Map<String, ProxySession> activeSessions = new ConcurrentHashMap<>();
    private final BlockingQueue<PacketData> packetQueue = new LinkedBlockingQueue<>();
    private final AtomicInteger activeConnections = new AtomicInteger(0);

    private volatile boolean running = true;
    private Selector selector;
    private ServerSocketChannel serverChannel;
    private ConnectionHandler connectionHandler;

    public ProxyServer(ProxyConfig config, P2PSecurityProxySystem system) {
        this.config = config;
        this.system = system;
        this.connectionHandler = new ConnectionHandler(config, system);
    }

    public void start() {
        try {
            serverChannel = ServerSocketChannel.open();
            ServerSocket serverSocket = serverChannel.socket();
            serverSocket.setReuseAddress(true);
            serverSocket.bind(new InetSocketAddress(config.getBindAddress(), config.getProxyPort()), config.getMaxConnections());
            serverChannel.configureBlocking(false);

            selector = Selector.open();
            serverChannel.register(selector, SelectionKey.OP_ACCEPT);

            startAcceptLoop();
            startPacketProcessors();
            startHeartbeatChecker();

            System.out.println("Proxy server started on " + config.getBindAddress() + ":" + config.getProxyPort());

        } catch (Exception e) {
            System.err.println("Failed to start proxy server: " + e.getMessage());
            e.printStackTrace();
        }
    }

    private void startAcceptLoop() {
        for (int i = 0; i < config.getIoThreads(); i++) {
            system.getWorkerPool().submit(() -> {
                while (running && system.isRunning()) {
                    try {
                        selector.select(1000);
                        Iterator<SelectionKey> keys = selector.selectedKeys().iterator();
                        while (keys.hasNext()) {
                            SelectionKey key = keys.next();
                            keys.remove();

                            if (!key.isValid()) continue;

                            if (key.isAcceptable()) {
                                handleAccept();
                            } else if (key.isReadable()) {
                                handleRead(key);
                            } else if (key.isWritable()) {
                                handleWrite(key);
                            }
                        }
                    } catch (Exception e) {
                        if (running) {
                            System.err.println("Selector error: " + e.getMessage());
                        }
                    }
                }
            });
        }
    }

    private void handleAccept() throws Exception {
        SocketChannel clientChannel = serverChannel.accept();

        if (clientChannel == null) return;

        if (activeConnections.get() >= config.getMaxConnections()) {
            clientChannel.close();
            return;
        }

        clientChannel.configureBlocking(false);
        clientChannel.socket().setKeepAlive(true);
        clientChannel.socket().setTcpNoDelay(true);
        clientChannel.socket().setReceiveBufferSize(config.getReceiveBufferSize());
        clientChannel.socket().setSendBufferSize(config.getSendBufferSize());

        String clientIp = clientChannel.getRemoteAddress().toString();

        // Firewall check
        if (system.getFirewallManager().isBlocked(clientIp)) {
            clientChannel.close();
            return;
        }

        // DDoS protection
        if (config.isEnableDdosProtection() && system.getDdosProtector().isAttackDetected(clientIp)) {
            system.getFirewallManager().blockIp(clientIp, "DDoS attack detected");
            clientChannel.close();
            return;
        }

        // Create session
        ProxySession session = new ProxySession(clientChannel, clientIp, config);
        activeSessions.put(session.getSessionId(), session);
        activeConnections.incrementAndGet();

        clientChannel.register(selector, SelectionKey.OP_READ, session);

        system.getMetricsCollector().recordCounter("connections_total", 1);
    }

    private void handleRead(SelectionKey key) throws Exception {
        SocketChannel channel = (SocketChannel) key.channel();
        ProxySession session = (ProxySession) key.attachment();

        if (session == null || !channel.isOpen()) {
            return;
        }

        ByteBuffer buffer = ByteBuffer.allocate(config.getBufferSize());
        int bytesRead;

        try {
            bytesRead = channel.read(buffer);
        } catch (IOException e) {
            closeSession(session);
            return;
        }

        if (bytesRead == -1) {
            closeSession(session);
            return;
        }

        if (bytesRead == 0) {
            return;
        }

        buffer.flip();
        byte[] data = new byte[bytesRead];
        buffer.get(data);

        // Decrypt if needed
        if (session.isEncrypted() && session.getSessionKey() != null) {
            try {
                data = system.getTrafficEncryptor().decrypt(data, session.getSessionKey());
            } catch (Exception e) {
                system.getSecurityManager().logSuspiciousActivity(
                    session.getClientIp(),
                    "Decryption failed: " + e.getMessage(),
                    new byte[0]
                );
                closeSession(session);
                return;
            }
        }

        // Content filtering
        if (config.isEnableContentFilter() && system.getContentFilter().isBlocked(data)) {
            system.getSecurityManager().logSuspiciousActivity(
                session.getClientIp(),
                "Blocked content",
                data
            );
            return;
        }

        // Create packet and queue for processing
        PacketData packet = new PacketData(
            session.getSessionId(),
            session.getClientIp(),
            data,
            System.currentTimeMillis(),
            session.getSequenceNumber()
        );

        session.incrementSequenceNumber();
        packetQueue.offer(packet);

        system.getTotalBytesTransferred().addAndGet(bytesRead);
        session.addBytesReceived(bytesRead);
    }

    private void handleWrite(SelectionKey key) throws Exception {
        SocketChannel channel = (SocketChannel) key.channel();
        ByteBuffer buffer = (ByteBuffer) key.attachment();

        if (buffer != null && buffer.hasRemaining()) {
            int written = channel.write(buffer);
            if (written > 0) {
                ProxySession session = (ProxySession) key.attachment();
                if (session != null) {
                    session.addBytesSent(written);
                }
            }

            if (!buffer.hasRemaining()) {
                key.interestOps(SelectionKey.OP_READ);
            }
        }
    }

    private void startPacketProcessors() {
        int processors = Math.max(1, config.getWorkerThreads());
        for (int i = 0; i < processors; i++) {
            system.getWorkerPool().submit(() -> {
                while (running && system.isRunning()) {
                    try {
                        PacketData packet = packetQueue.poll(100, TimeUnit.MILLISECONDS);
                        if (packet != null) {
                            system.getPacketProcessor().process(packet);
                            system.getTotalPacketsProcessed().incrementAndGet();
                        }
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        break;
                    } catch (Exception e) {
                        system.getTotalErrors().incrementAndGet();
                        System.err.println("Packet processing error: " + e.getMessage());
                    }
                }
            });
        }
    }

    private void startHeartbeatChecker() {
        system.getScheduler().scheduleAtFixedRate(() -> {
            long now = System.currentTimeMillis();
            activeSessions.values().forEach(session -> {
                if (now - session.getLastHeartbeat() > config.getConnectionTimeout()) {
                    closeSession(session);
                }
            });
        }, 30, 30, TimeUnit.SECONDS);
    }

    public void sendToClient(ProxySession session, byte[] data) {
        if (session == null || !session.getChannel().isOpen()) {
            return;
        }

        try {
            // Compress if enabled
            if (session.isCompressionEnabled() && data.length > 1024) {
                data = session.compress(data);
                session.incrementCompressionCount();
            }

            // Encrypt if needed
            if (session.isEncrypted() && session.getSessionKey() != null) {
                data = system.getTrafficEncryptor().encrypt(data, session.getSessionKey());
            }

            ByteBuffer buffer = ByteBuffer.wrap(data);

            // Try to write immediately, if not all written, register for write
            session.getChannel().write(buffer);

            if (buffer.hasRemaining()) {
                SelectionKey key = session.getChannel().keyFor(selector);
                if (key != null) {
                    key.interestOps(SelectionKey.OP_WRITE);
                    key.attach(buffer);
                } else {
                    while (buffer.hasRemaining()) {
                        session.getChannel().write(buffer);
                    }
                }
            }

            session.incrementPacketsSent();
            session.addBytesSent(data.length);

        } catch (Exception e) {
            closeSession(session);
        }
    }

    public void closeSession(ProxySession session) {
        if (session == null) return;

        try {
            if (session.getChannel().isOpen()) {
                session.getChannel().close();
            }

            SocketChannel targetChannel = session.getTargetChannel();
            if (targetChannel != null && targetChannel.isOpen()) {
                targetChannel.close();
            }

        } catch (Exception e) {
            // Ignore close errors
        } finally {
            activeSessions.remove(session.getSessionId());
            activeConnections.decrementAndGet();
            system.getSessionCache().remove(session.getSessionId());

            system.getMetricsCollector().recordGauge("active_sessions", activeSessions.size());
        }
    }

    public void stop() {
        running = false;
        try {
            if (selector != null) {
                selector.wakeup();
                selector.close();
            }
            if (serverChannel != null) {
                serverChannel.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        // Close all sessions
        new ArrayList<>(activeSessions.values()).forEach(this::closeSession);

        System.out.println("Proxy server stopped");
    }

    public Map<String, ProxySession> getActiveSessions() {
        return Collections.unmodifiableMap(activeSessions);
    }

    public int getActiveConnectionCount() {
        return activeConnections.get();
    }
}
