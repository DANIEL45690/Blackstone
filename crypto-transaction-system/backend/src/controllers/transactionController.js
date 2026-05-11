const Transaction = require('../models/Transaction');
const Chunk = require('../models/Chunk');
const crypto = require('crypto');
const EncryptionService = require('../services/encryptionService');
const BinarySplitter = require('../utils/binarySplitter');
const CacheService = require('../services/cacheService');
const AuditService = require('../services/auditService');

class TransactionController {
  async createTransaction(req, res) {
    const startTime = Date.now();
    try {
      const { data, metadata, tags, priority } = req.body;
      const encrypted = await EncryptionService.encrypt(data);
      const chunks = BinarySplitter.split(encrypted, 1024);
      const shuffledChunks = BinarySplitter.shuffle(chunks);
      const transaction = new Transaction({
        transactionId: crypto.randomUUID(),
        encryptedData: encrypted,
        chunks: shuffledChunks,
        originalHash: EncryptionService.generateHash(data),
        status: 'encrypted',
        metadata: { ...metadata, ipAddress: req.ip, userAgent: req.get('User-Agent') },
        tags: tags || [],
        priority: priority || 0
      });
      await transaction.save();
      for (let i = 0; i < shuffledChunks.length; i++) {
        const chunk = new Chunk({
          transactionId: transaction.transactionId,
          index: i,
          data: shuffledChunks[i],
          position: Math.floor(Math.random() * 1000000),
          hash: crypto.createHash('sha256').update(shuffledChunks[i]).digest('hex')
        });
        await chunk.save();
      }
      await CacheService.set(transaction.transactionId, { status: transaction.status, chunks: shuffledChunks.length });
      await AuditService.log(req.user?.id, 'CREATE', transaction.transactionId, { duration: Date.now() - startTime });
      res.status(201).json({ transactionId: transaction.transactionId, chunksCount: shuffledChunks.length, status: transaction.status });
    } catch (error) {
      await AuditService.log(req.user?.id, 'ERROR', null, { error: error.message });
      res.status(500).json({ error: error.message });
    }
  }

  async getTransaction(req, res) {
    try {
      const cached = await CacheService.get(req.params.id);
      if (cached) return res.json(cached);
      const transaction = await Transaction.findOne({ transactionId: req.params.id });
      if (!transaction) return res.status(404).json({ error: 'Not found' });
      await CacheService.set(req.params.id, transaction, 300);
      res.json(transaction);
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  }

  async decryptTransaction(req, res) {
    try {
      const transaction = await Transaction.findOne({ transactionId: req.params.id });
      if (!transaction) return res.status(404).json({ error: 'Not found' });
      const chunks = await Chunk.find({ transactionId: req.params.id }).sort('index');
      const combined = BinarySplitter.combine(chunks.map(c => c.data));
      const decrypted = await EncryptionService.decrypt(combined);
      transaction.status = 'decrypted';
      transaction.updatedAt = new Date();
      await transaction.save();
      await AuditService.log(req.user?.id, 'DECRYPT', req.params.id, { success: true });
      res.json({ data: decrypted, hash: transaction.originalHash });
    } catch (error) {
      await AuditService.log(req.user?.id, 'DECRYPT_ERROR', req.params.id, { error: error.message });
      res.status(500).json({ error: error.message });
    }
  }

  async getAllTransactions(req, res) {
    try {
      const { limit = 50, offset = 0, status, fromDate, toDate, search } = req.query;
      const query = {};
      if (status) query.status = status;
      if (fromDate || toDate) {
        query.timestamp = {};
        if (fromDate) query.timestamp.$gte = new Date(fromDate);
        if (toDate) query.timestamp.$lte = new Date(toDate);
      }
      if (search) {
        query.$or = [
          { transactionId: { $regex: search, $options: 'i' } },
          { 'metadata.source': { $regex: search, $options: 'i' } }
        ];
      }
      const transactions = await Transaction.find(query)
        .sort({ priority: -1, timestamp: -1 })
        .skip(parseInt(offset))
        .limit(parseInt(limit));
      const total = await Transaction.countDocuments(query);
      res.json({ data: transactions, pagination: { total, limit: parseInt(limit), offset: parseInt(offset) } });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  }

  async updateTransaction(req, res) {
    try {
      const transaction = await Transaction.findOne({ transactionId: req.params.id });
      if (!transaction) return res.status(404).json({ error: 'Not found' });
      if (req.body.status) transaction.status = req.body.status;
      if (req.body.metadata) transaction.metadata = { ...transaction.metadata, ...req.body.metadata };
      if (req.body.tags) transaction.tags = req.body.tags;
      if (req.body.priority !== undefined) transaction.priority = req.body.priority;
      transaction.updatedAt = new Date();
      await transaction.save();
      await CacheService.del(req.params.id);
      res.json(transaction);
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  }

  async deleteTransaction(req, res) {
    try {
      const transaction = await Transaction.findOneAndDelete({ transactionId: req.params.id });
      if (!transaction) return res.status(404).json({ error: 'Not found' });
      await Chunk.deleteMany({ transactionId: req.params.id });
      await CacheService.del(req.params.id);
      await AuditService.log(req.user?.id, 'DELETE', req.params.id, { success: true });
      res.json({ message: 'Deleted successfully', transactionId: req.params.id });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  }

  async getTransactionStats(req, res) {
    try {
      const total = await Transaction.countDocuments();
      const byStatus = await Transaction.aggregate([
        { $group: { _id: '$status', count: { $sum: 1 } } }
      ]);
      const byDate = await Transaction.aggregate([
        { $group: { _id: { $dateToString: { format: '%Y-%m-%d', date: '$timestamp' } }, count: { $sum: 1 } } },
        { $sort: { _id: -1 } },
        { $limit: 30 }
      ]);
      const avgChunks = await Chunk.aggregate([
        { $group: { _id: '$transactionId', count: { $sum: 1 } } },
        { $group: { _id: null, avg: { $avg: '$count' } } }
      ]);
      res.json({ total, byStatus, byDate, avgChunks: avgChunks[0]?.avg || 0 });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  }
}

module.exports = new TransactionController();
