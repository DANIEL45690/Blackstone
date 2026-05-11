const crypto = require('crypto');

class EncryptionService {
  constructor() {
    this.algorithm = 'aes-256-cbc';
    this.hashAlgorithm = 'sha256';
    this.encoding = 'hex';
    this.ivLength = 16;
  }

  getKey() {
    const key = process.env.AES_KEY;
    if (!key) throw new Error('AES_KEY not configured');
    return Buffer.from(key, 'hex');
  }

  encrypt(text) {
    const key = this.getKey();
    const iv = crypto.randomBytes(this.ivLength);
    const cipher = crypto.createCipheriv(this.algorithm, key, iv);
    let encrypted = cipher.update(text, 'utf8', this.encoding);
    encrypted += cipher.final(this.encoding);
    return Buffer.concat([iv, Buffer.from(encrypted, this.encoding)]);
  }

  decrypt(combinedBuffer) {
    const key = this.getKey();
    const iv = combinedBuffer.slice(0, this.ivLength);
    const encryptedText = combinedBuffer.slice(this.ivLength).toString(this.encoding);
    const decipher = crypto.createDecipheriv(this.algorithm, key, iv);
    let decrypted = decipher.update(encryptedText, this.encoding, 'utf8');
    decrypted += decipher.final('utf8');
    return decrypted;
  }

  generateHash(data) {
    return crypto.createHash(this.hashAlgorithm).update(data).digest(this.encoding);
  }

  generateRandomId() {
    return crypto.randomUUID();
  }

  generateRandomBytes(length = 32) {
    return crypto.randomBytes(length).toString(this.encoding);
  }

  hmacSign(data, secret) {
    return crypto.createHmac('sha256', secret).update(data).digest(this.encoding);
  }

  verifyHmac(data, signature, secret) {
    const expected = this.hmacSign(data, secret);
    return crypto.timingSafeEqual(Buffer.from(signature, this.encoding), Buffer.from(expected, this.encoding));
  }

  encryptWithPassword(text, password) {
    const salt = crypto.randomBytes(16);
    const key = crypto.pbkdf2Sync(password, salt, 100000, 32, 'sha256');
    const iv = crypto.randomBytes(this.ivLength);
    const cipher = crypto.createCipheriv(this.algorithm, key, iv);
    let encrypted = cipher.update(text, 'utf8', this.encoding);
    encrypted += cipher.final(this.encoding);
    const combined = Buffer.concat([salt, iv, Buffer.from(encrypted, this.encoding)]);
    return combined.toString('base64');
  }

  decryptWithPassword(combinedBase64, password) {
    const combined = Buffer.from(combinedBase64, 'base64');
    const salt = combined.slice(0, 16);
    const iv = combined.slice(16, 32);
    const encryptedText = combined.slice(32).toString(this.encoding);
    const key = crypto.pbkdf2Sync(password, salt, 100000, 32, 'sha256');
    const decipher = crypto.createDecipheriv(this.algorithm, key, iv);
    let decrypted = decipher.update(encryptedText, this.encoding, 'utf8');
    decrypted += decipher.final('utf8');
    return decrypted;
  }

  splitToChunks(data, chunkSize = 1024) {
    const chunks = [];
    for (let i = 0; i < data.length; i += chunkSize) {
      chunks.push(data.slice(i, i + chunkSize));
    }
    return chunks;
  }

  combineChunks(chunks) {
    return Buffer.concat(chunks);
  }

  async batchEncrypt(items, batchSize = 10) {
    const results = [];
    for (let i = 0; i < items.length; i += batchSize) {
      const batch = items.slice(i, i + batchSize);
      const batchResults = await Promise.all(batch.map(item => this.encrypt(item)));
      results.push(...batchResults);
    }
    return results;
  }

  async batchDecrypt(items) {
    return Promise.all(items.map(item => this.decrypt(item)));
  }
}

module.exports = new EncryptionService();
