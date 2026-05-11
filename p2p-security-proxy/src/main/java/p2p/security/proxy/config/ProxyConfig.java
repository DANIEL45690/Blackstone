package p2p.security.proxy.config;

import java.io.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

public class ProxyConfig {

    // Network Configuration
    private int proxyPort = 8888;
    private int controlPort = 9999;
    private int sslPort = 8443;
    private int connectionTimeout = 30000;
    private int maxConnections = 10000;
    private String bindAddress = "0.0.0.0";

    // Performance Configuration
    private int maxPacketSize = 65536;
    private int bufferSize = 8192;
    private int threadPoolSize = 200;
    private int ioThreads = 8;
    private int workerThreads = 16;
    private boolean useNativeTransport = true;
    private int sendBufferSize = 65536;
    private int receiveBufferSize = 65536;

    // Security Configuration
    private long ipBanDuration = 3600000;
    private int maxConnectionsPerIp = 50;
    private int rateLimitRequests = 100;
    private long rateLimitWindow = 60000;
    private boolean enableDdosProtection = true;
    private boolean enableEncryption = true;
    private boolean enableContentFilter = true;
    private boolean enableIpBlacklist = true;
    private boolean enableWhitelist = false;

    // Encryption Configuration
    private String encryptionAlgorithm = "AES/GCM/NoPadding";
    private int encryptionKeySize = 256;
    private String tlsVersion = "TLSv1.3";
    private String certificatePath = "certificates/server.jks";
    private String certificatePassword = "changeit";

    // P2P Configuration
    private boolean enableP2PRouting = true;
    private int maxHops = 5;
    private int cacheSize = 10000;
    private long messageTimeout = 60000;

    // Monitoring Configuration
    private boolean enableMetrics = true;
    private int metricsInterval = 60;
    private boolean enableAuditLog = true;
    private String logLevel = "INFO";
    private String logPath = "logs/p2p-proxy.log";

    // Database Configuration
    private boolean enablePersistence = false;
    private String dbUrl = "jdbc:sqlite:p2p-proxy.db";
    private String dbDriver = "org.sqlite.JDBC";

    // Authentication
    private String authToken;
    private boolean enableTokenAuth = true;
    private List<String> allowedTokens = new ArrayList<>();

    private final Map<String, String> customProperties = new ConcurrentHashMap<>();

    public ProxyConfig() {
        this.authToken = System.getProperty("p2p.auth.token", generateToken());
        loadFromFile();
        loadSystemProperties();
    }

    private String generateToken() {
        return UUID.randomUUID().toString().replace("-", "");
    }

    private void loadFromFile() {
        File configFile = new File("config/proxy.properties");
        if (!configFile.exists()) return;

        Properties props = new Properties();
        try (FileInputStream fis = new FileInputStream(configFile)) {
            props.load(fis);

            proxyPort = Integer.parseInt(props.getProperty("proxy.port", String.valueOf(proxyPort)));
            controlPort = Integer.parseInt(props.getProperty("control.port", String.valueOf(controlPort)));
            connectionTimeout = Integer.parseInt(props.getProperty("connection.timeout", String.valueOf(connectionTimeout)));
            threadPoolSize = Integer.parseInt(props.getProperty("thread.pool.size", String.valueOf(threadPoolSize)));
            maxConnectionsPerIp = Integer.parseInt(props.getProperty("max.connections.per.ip", String.valueOf(maxConnectionsPerIp)));

        } catch (Exception e) {
            System.err.println("Failed to load config: " + e.getMessage());
        }
    }

    private void loadSystemProperties() {
        proxyPort = Integer.getInteger("p2p.proxy.port", proxyPort);
        controlPort = Integer.getInteger("p2p.control.port", controlPort);
        logLevel = System.getProperty("p2p.log.level", logLevel);
    }

    public void saveToFile() {
        Properties props = new Properties();
        props.setProperty("proxy.port", String.valueOf(proxyPort));
        props.setProperty("control.port", String.valueOf(controlPort));
        props.setProperty("connection.timeout", String.valueOf(connectionTimeout));
        props.setProperty("thread.pool.size", String.valueOf(threadPoolSize));

        try (FileOutputStream fos = new FileOutputStream("config/proxy.properties")) {
            props.store(fos, "P2P Security Proxy Configuration");
        } catch (Exception e) {
            System.err.println("Failed to save config: " + e.getMessage());
        }
    }

    // Getters and Setters
    public int getProxyPort() { return proxyPort; }
    public void setProxyPort(int proxyPort) { this.proxyPort = proxyPort; }

    public int getControlPort() { return controlPort; }
    public void setControlPort(int controlPort) { this.controlPort = controlPort; }

    public int getSslPort() { return sslPort; }
    public void setSslPort(int sslPort) { this.sslPort = sslPort; }

    public int getConnectionTimeout() { return connectionTimeout; }
    public void setConnectionTimeout(int connectionTimeout) { this.connectionTimeout = connectionTimeout; }

    public int getMaxConnections() { return maxConnections; }
    public void setMaxConnections(int maxConnections) { this.maxConnections = maxConnections; }

    public String getBindAddress() { return bindAddress; }
    public void setBindAddress(String bindAddress) { this.bindAddress = bindAddress; }

    public int getMaxPacketSize() { return maxPacketSize; }
    public void setMaxPacketSize(int maxPacketSize) { this.maxPacketSize = maxPacketSize; }

    public int getBufferSize() { return bufferSize; }
    public void setBufferSize(int bufferSize) { this.bufferSize = bufferSize; }

    public int getThreadPoolSize() { return threadPoolSize; }
    public void setThreadPoolSize(int threadPoolSize) { this.threadPoolSize = threadPoolSize; }

    public int getIoThreads() { return ioThreads; }
    public void setIoThreads(int ioThreads) { this.ioThreads = ioThreads; }

    public int getWorkerThreads() { return workerThreads; }
    public void setWorkerThreads(int workerThreads) { this.workerThreads = workerThreads; }

    public boolean isUseNativeTransport() { return useNativeTransport; }
    public void setUseNativeTransport(boolean useNativeTransport) { this.useNativeTransport = useNativeTransport; }

    public int getSendBufferSize() { return sendBufferSize; }
    public void setSendBufferSize(int sendBufferSize) { this.sendBufferSize = sendBufferSize; }

    public int getReceiveBufferSize() { return receiveBufferSize; }
    public void setReceiveBufferSize(int receiveBufferSize) { this.receiveBufferSize = receiveBufferSize; }

    public long getIpBanDuration() { return ipBanDuration; }
    public void setIpBanDuration(long ipBanDuration) { this.ipBanDuration = ipBanDuration; }

    public int getMaxConnectionsPerIp() { return maxConnectionsPerIp; }
    public void setMaxConnectionsPerIp(int maxConnectionsPerIp) { this.maxConnectionsPerIp = maxConnectionsPerIp; }

    public int getRateLimitRequests() { return rateLimitRequests; }
    public void setRateLimitRequests(int rateLimitRequests) { this.rateLimitRequests = rateLimitRequests; }

    public long getRateLimitWindow() { return rateLimitWindow; }
    public void setRateLimitWindow(long rateLimitWindow) { this.rateLimitWindow = rateLimitWindow; }

    public boolean isEnableDdosProtection() { return enableDdosProtection; }
    public void setEnableDdosProtection(boolean enableDdosProtection) { this.enableDdosProtection = enableDdosProtection; }

    public boolean isEnableEncryption() { return enableEncryption; }
    public void setEnableEncryption(boolean enableEncryption) { this.enableEncryption = enableEncryption; }

    public boolean isEnableContentFilter() { return enableContentFilter; }
    public void setEnableContentFilter(boolean enableContentFilter) { this.enableContentFilter = enableContentFilter; }

    public boolean isEnableIpBlacklist() { return enableIpBlacklist; }
    public void setEnableIpBlacklist(boolean enableIpBlacklist) { this.enableIpBlacklist = enableIpBlacklist; }

    public boolean isEnableWhitelist() { return enableWhitelist; }
    public void setEnableWhitelist(boolean enableWhitelist) { this.enableWhitelist = enableWhitelist; }

    public String getEncryptionAlgorithm() { return encryptionAlgorithm; }
    public void setEncryptionAlgorithm(String encryptionAlgorithm) { this.encryptionAlgorithm = encryptionAlgorithm; }

    public int getEncryptionKeySize() { return encryptionKeySize; }
    public void setEncryptionKeySize(int encryptionKeySize) { this.encryptionKeySize = encryptionKeySize; }

    public String getTlsVersion() { return tlsVersion; }
    public void setTlsVersion(String tlsVersion) { this.tlsVersion = tlsVersion; }

    public String getCertificatePath() { return certificatePath; }
    public void setCertificatePath(String certificatePath) { this.certificatePath = certificatePath; }

    public String getCertificatePassword() { return certificatePassword; }
    public void setCertificatePassword(String certificatePassword) { this.certificatePassword = certificatePassword; }

    public boolean isEnableP2PRouting() { return enableP2PRouting; }
    public void setEnableP2PRouting(boolean enableP2PRouting) { this.enableP2PRouting = enableP2PRouting; }

    public int getMaxHops() { return maxHops; }
    public void setMaxHops(int maxHops) { this.maxHops = maxHops; }

    public int getCacheSize() { return cacheSize; }
    public void setCacheSize(int cacheSize) { this.cacheSize = cacheSize; }

    public long getMessageTimeout() { return messageTimeout; }
    public void setMessageTimeout(long messageTimeout) { this.messageTimeout = messageTimeout; }

    public boolean isEnableMetrics() { return enableMetrics; }
    public void setEnableMetrics(boolean enableMetrics) { this.enableMetrics = enableMetrics; }

    public int getMetricsInterval() { return metricsInterval; }
    public void setMetricsInterval(int metricsInterval) { this.metricsInterval = metricsInterval; }

    public boolean isEnableAuditLog() { return enableAuditLog; }
    public void setEnableAuditLog(boolean enableAuditLog) { this.enableAuditLog = enableAuditLog; }

    public String getLogLevel() { return logLevel; }
    public void setLogLevel(String logLevel) { this.logLevel = logLevel; }

    public String getLogPath() { return logPath; }
    public void setLogPath(String logPath) { this.logPath = logPath; }

    public boolean isEnablePersistence() { return enablePersistence; }
    public void setEnablePersistence(boolean enablePersistence) { this.enablePersistence = enablePersistence; }

    public String getDbUrl() { return dbUrl; }
    public void setDbUrl(String dbUrl) { this.dbUrl = dbUrl; }

    public String getDbDriver() { return dbDriver; }
    public void setDbDriver(String dbDriver) { this.dbDriver = dbDriver; }

    public String getAuthToken() { return authToken; }
    public void setAuthToken(String authToken) { this.authToken = authToken; }

    public boolean isEnableTokenAuth() { return enableTokenAuth; }
    public void setEnableTokenAuth(boolean enableTokenAuth) { this.enableTokenAuth = enableTokenAuth; }

    public List<String> getAllowedTokens() { return new ArrayList<>(allowedTokens); }
    public void addAllowedToken(String token) { this.allowedTokens.add(token); }

    public String getCustomProperty(String key) { return customProperties.get(key); }
    public void setCustomProperty(String key, String value) { customProperties.put(key, value); }
}
