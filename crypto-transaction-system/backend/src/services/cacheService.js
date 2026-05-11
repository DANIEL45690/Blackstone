const redis = require('redis');

class CacheService {
  constructor() {
    this.client = null;
    this.isConnected = false;
    this.init();
  }

  async init() {
    try {
      this.client = redis.createClient({ url: process.env.REDIS_URL || 'redis://localhost:6379' });
      this.client.on('error', (err) => console.error('Redis error:', err));
      this.client.on('connect', () => {
        this.isConnected = true;
        console.log('Redis connected');
      });
      await this.client.connect();
    } catch (error) {
      console.error('Failed to connect to Redis:', error);
      this.isConnected = false;
    }
  }

  async get(key) {
    if (!this.isConnected) return null;
    try {
      const value = await this.client.get(key);
      return value ? JSON.parse(value) : null;
    } catch (error) {
      console.error('Cache get error:', error);
      return null;
    }
  }

  async set(key, value, ttlSeconds = 3600) {
    if (!this.isConnected) return false;
    try {
      await this.client.set(key, JSON.stringify(value), { EX: ttlSeconds });
      return true;
    } catch (error) {
      console.error('Cache set error:', error);
      return false;
    }
  }

  async del(key) {
    if (!this.isConnected) return false;
    try {
      await this.client.del(key);
      return true;
    } catch (error) {
      console.error('Cache del error:', error);
      return false;
    }
  }

  async delPattern(pattern) {
    if (!this.isConnected) return false;
    try {
      const keys = await this.client.keys(pattern);
      if (keys.length) await this.client.del(keys);
      return true;
    } catch (error) {
      console.error('Cache delPattern error:', error);
      return false;
    }
  }

  async exists(key) {
    if (!this.isConnected) return false;
    try {
      return await this.client.exists(key) === 1;
    } catch (error) {
      return false;
    }
  }

  async increment(key, by = 1) {
    if (!this.isConnected) return null;
    try {
      return await this.client.incrBy(key, by);
    } catch (error) {
      return null;
    }
  }

  async expire(key, seconds) {
    if (!this.isConnected) return false;
    try {
      await this.client.expire(key, seconds);
      return true;
    } catch (error) {
      return false;
    }
  }

  async flush() {
    if (!this.isConnected) return false;
    try {
      await this.client.flushAll();
      return true;
    } catch (error) {
      return false;
    }
  }
}

module.exports = new CacheService();
