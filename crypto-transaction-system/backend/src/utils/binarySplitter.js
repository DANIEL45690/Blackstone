const crypto = require('crypto');

class BinarySplitter {
  split(data, chunkSize = 1024) {
    const chunks = [];
    for (let i = 0; i < data.length; i += chunkSize) {
      chunks.push(data.slice(i, i + chunkSize));
    }
    return chunks;
  }

  combine(chunks) {
    return Buffer.concat(chunks);
  }

  shuffle(chunks) {
    const shuffled = [...chunks];
    for (let i = shuffled.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [shuffled[i], shuffled[j]] = [shuffled[j], shuffled[i]];
    }
    return shuffled;
  }

  unshuffle(shuffledChunks, originalOrder) {
    const result = new Array(shuffledChunks.length);
    for (let i = 0; i < originalOrder.length; i++) {
      result[originalOrder[i]] = shuffledChunks[i];
    }
    return result;
  }

  generateOrderMap(length) {
    const order = Array.from({ length }, (_, i) => i);
    for (let i = order.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [order[i], order[j]] = [order[j], order[i]];
    }
    return order;
  }

  splitWithMetadata(data, chunkSize = 1024) {
    const chunks = this.split(data, chunkSize);
    const metadata = {
      totalChunks: chunks.length,
      chunkSize,
      originalSize: data.length,
      hash: crypto.createHash('sha256').update(data).digest('hex'),
      order: this.generateOrderMap(chunks.length)
    };
    return { chunks, metadata };
  }

  splitToBase64(data, chunkSize = 1024) {
    const chunks = this.split(data, chunkSize);
    return chunks.map(chunk => chunk.toString('base64'));
  }

  combineFromBase64(base64Chunks) {
    const buffers = base64Chunks.map(chunk => Buffer.from(chunk, 'base64'));
    return this.combine(buffers);
  }

  async streamSplit(readStream, chunkSize = 1024, onChunk) {
    let buffer = Buffer.alloc(0);
    for await (const data of readStream) {
      buffer = Buffer.concat([buffer, data]);
      while (buffer.length >= chunkSize) {
        const chunk = buffer.slice(0, chunkSize);
        if (onChunk) await onChunk(chunk);
        buffer = buffer.slice(chunkSize);
      }
    }
    if (buffer.length > 0 && onChunk) await onChunk(buffer);
  }

  async streamCombine(chunks, writeStream) {
    for (const chunk of chunks) {
      writeStream.write(chunk);
    }
    writeStream.end();
  }

  getChunkHash(chunk) {
    return crypto.createHash('sha256').update(chunk).digest('hex');
  }

  verifyChunks(chunks, expectedHashes) {
    for (let i = 0; i < chunks.length; i++) {
      const hash = this.getChunkHash(chunks[i]);
      if (expectedHashes[i] && hash !== expectedHashes[i]) {
        return { valid: false, failedIndex: i, expectedHash: expectedHashes[i], actualHash: hash };
      }
    }
    return { valid: true };
  }

  encryptChunks(chunks, key) {
    const crypto = require('crypto');
    const iv = crypto.randomBytes(16);
    const cipher = crypto.createCipheriv('aes-256-cbc', key, iv);
    const encryptedChunks = chunks.map(chunk => {
      let encrypted = cipher.update(chunk);
      return encrypted;
    });
    const final = cipher.final();
    return { chunks: encryptedChunks, iv, final };
  }
}

module.exports = new BinarySplitter();
