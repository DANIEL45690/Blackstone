package p2p.security.proxy;

import p2p.security.proxy.config.ProxyConfig;
import p2p.security.proxy.network.ProxyServer;
import p2p.security.proxy.network.ControlServer;
import p2p.security.proxy.security.*;
import p2p.security.proxy.protocol.ProxyProtocolManager;
import p2p.security.proxy.metrics.MetricsCollector;
import p2p.security.proxy.handler.PacketProcessor;
import p2p.security.proxy.cache.SessionCache;
import p2p.security.proxy.filter.ContentFilter;

import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicLong;
import java.util.logging.*;

public class P2PSecurityProxySystem {

    private static final Logger LOGGER = Logger.getLogger(P2PSecurityProxySystem.class.getName());

    private final ProxyConfig config;
    private final ProxyServer proxyServer;
    private final ControlServer controlServer;
    private final FirewallManager firewallManager;
    private final TrafficEncryptor trafficEncryptor;
    private final DdosProtector ddosProtector;
    private final Anonymizer anonymizer;
    private final SecurityManager securityManager;
    private final ProxyProtocolManager protocolManager;
    private final MetricsCollector metricsCollector;
    private final SessionCache sessionCache;
    private final ContentFilter contentFilter;
    private final PacketProcessor packetProcessor;

    private final ExecutorService workerPool;
    private final ScheduledExecutorService scheduler;
    private volatile boolean running = true;
    private final AtomicLong totalPacketsProcessed = new AtomicLong(0);
    private final AtomicLong totalBytesTransferred = new AtomicLong(0);
    private final AtomicLong totalErrors = new AtomicLong(0);

    public P2PSecurityProxySystem() throws Exception {
        LOGGER.info("Initializing P2P Security Proxy System...");

        this.config = new ProxyConfig();
        this.firewallManager = new FirewallManager(config);
        this.trafficEncryptor = new TrafficEncryptor();
        this.ddosProtector = new DdosProtector(config);
        this.anonymizer = new Anonymizer();
        this.securityManager = new SecurityManager();
        this.protocolManager = new ProxyProtocolManager();
        this.metricsCollector = new MetricsCollector();
        this.sessionCache = new SessionCache(config);
        this.contentFilter = new ContentFilter(config);

        this.workerPool = Executors.newFixedThreadPool(config.getThreadPoolSize());
        this.scheduler = Executors.newScheduledThreadPool(5);
        this.packetProcessor = new PacketProcessor(this);

        this.proxyServer = new ProxyServer(config, this);
        this.controlServer = new ControlServer(config, this);

        initializeSecurity();
        startBackgroundTasks();

        LOGGER.info("P2P Security Proxy System initialized successfully");
    }

    private void initializeSecurity() {
        // Загрузка правил firewall
        firewallManager.loadRules();

        // Инициализация SSL контекста
        try {
            trafficEncryptor.initSSLContext();
        } catch (Exception e) {
            LOGGER.warning("SSL initialization failed: " + e.getMessage());
        }

        LOGGER.info("Security components initialized");
    }

    private void startBackgroundTasks() {
        // Очистка старых сессий каждые 5 минут
        scheduler.scheduleAtFixedRate(() -> {
            try {
                sessionCache.cleanup();
                metricsCollector.cleanup();
            } catch (Exception e) {
                LOGGER.warning("Background cleanup failed: " + e.getMessage());
            }
        }, 5, 5, TimeUnit.MINUTES);

        // Сбор метрик каждые 10 секунд
        scheduler.scheduleAtFixedRate(() -> {
            try {
                collectMetrics();
            } catch (Exception e) {
                LOGGER.warning("Metrics collection failed: " + e.getMessage());
            }
        }, 10, 10, TimeUnit.SECONDS);

        // Сохранение статистики каждые 5 минут
        scheduler.scheduleAtFixedRate(() -> {
            try {
                saveStatistics();
            } catch (Exception e) {
                LOGGER.warning("Statistics save failed: " + e.getMessage());
            }
        }, 5, 5, TimeUnit.MINUTES);
    }

    private void collectMetrics() {
        metricsCollector.recordGauge("active_sessions", sessionCache.size());
        metricsCollector.recordGauge("total_packets", totalPacketsProcessed.get());
        metricsCollector.recordGauge("total_bytes", totalBytesTransferred.get());
        metricsCollector.recordGauge("banned_ips", firewallManager.getBlockedIps().size());
        metricsCollector.recordGauge("worker_pool_size", workerPool.getActiveCount());
    }

    private void saveStatistics() {
        Map<String, Object> stats = getStatistics();
        metricsCollector.saveStatistics(stats);
    }

    public void start() {
        LOGGER.info("Starting P2P Security Proxy System...");
        proxyServer.start();
        controlServer.start();

        String startupMessage = String.format(
            "P2P Security Proxy System started on ports %d (proxy) and %d (control)",
            config.getProxyPort(), config.getControlPort()
        );
        System.out.println("=" .repeat(60));
        System.out.println(startupMessage);
        System.out.println("Auth token: " + config.getAuthToken());
        System.out.println("=" .repeat(60));

        LOGGER.info(startupMessage);
    }

    public void shutdown() {
        LOGGER.info("Shutting down P2P Security Proxy System...");
        running = false;

        proxyServer.stop();
        controlServer.stop();
        packetProcessor.shutdown();

        workerPool.shutdown();
        scheduler.shutdown();

        try {
            if (!workerPool.awaitTermination(30, TimeUnit.SECONDS)) {
                workerPool.shutdownNow();
            }
            if (!scheduler.awaitTermination(10, TimeUnit.SECONDS)) {
                scheduler.shutdownNow();
            }
        } catch (InterruptedException e) {
            workerPool.shutdownNow();
            scheduler.shutdownNow();
            Thread.currentThread().interrupt();
        }

        firewallManager.saveRules();
        metricsCollector.saveStatistics(getStatistics());

        LOGGER.info("P2P Security Proxy System stopped");
        System.out.println("System shutdown complete");
    }

    public Map<String, Object> getStatistics() {
        Map<String, Object> stats = new HashMap<>();
        stats.put("packets_processed", totalPacketsProcessed.get());
        stats.put("bytes_transferred", totalBytesTransferred.get());
        stats.put("errors", totalErrors.get());
        stats.put("active_sessions", sessionCache.size());
        stats.put("banned_ips", firewallManager.getBlockedIps().size());
        stats.put("uptime", System.currentTimeMillis() - metricsCollector.getStartTime());
        stats.put("timestamp", System.currentTimeMillis());
        return stats;
    }

    // Getters
    public ProxyConfig getConfig() { return config; }
    public FirewallManager getFirewallManager() { return firewallManager; }
    public TrafficEncryptor getTrafficEncryptor() { return trafficEncryptor; }
    public DdosProtector getDdosProtector() { return ddosProtector; }
    public Anonymizer getAnonymizer() { return anonymizer; }
    public SecurityManager getSecurityManager() { return securityManager; }
    public ProxyProtocolManager getProtocolManager() { return protocolManager; }
    public MetricsCollector getMetricsCollector() { return metricsCollector; }
    public SessionCache getSessionCache() { return sessionCache; }
    public ContentFilter getContentFilter() { return contentFilter; }
    public PacketProcessor getPacketProcessor() { return packetProcessor; }
    public ExecutorService getWorkerPool() { return workerPool; }
    public AtomicLong getTotalPacketsProcessed() { return totalPacketsProcessed; }
    public AtomicLong getTotalBytesTransferred() { return totalBytesTransferred; }
    public AtomicLong getTotalErrors() { return totalErrors; }
    public boolean isRunning() { return running; }

    public static void main(String[] args) {
        try {
            // Настройка логирования
            System.setProperty("java.util.logging.SimpleFormatter.format",
                "[%1$tF %1$tT] [%4$-7s] %5$s %n");

            P2PSecurityProxySystem system = new P2PSecurityProxySystem();
            system.start();

            Runtime.getRuntime().addShutdownHook(new Thread(() -> {
                System.out.println("\nReceived shutdown signal...");
                system.shutdown();
            }));

        } catch (Exception e) {
            System.err.println("Failed to start P2P Security Proxy System: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }
}
