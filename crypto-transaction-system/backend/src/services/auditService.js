const mongoose = require('mongoose');

const auditLogSchema = new mongoose.Schema({
  userId: String,
  action: String,
  resourceId: String,
  details: mongoose.Schema.Types.Mixed,
  ipAddress: String,
  userAgent: String,
  timestamp: { type: Date, default: Date.now },
  duration: Number,
  success: { type: Boolean, default: true }
});

auditLogSchema.index({ timestamp: -1 });
auditLogSchema.index({ userId: 1 });
auditLogSchema.index({ action: 1 });

const AuditLog = mongoose.model('AuditLog', auditLogSchema);

class AuditService {
  async log(userId, action, resourceId, details = {}, req = null) {
    try {
      const log = new AuditLog({
        userId,
        action,
        resourceId,
        details,
        ipAddress: req?.ip,
        userAgent: req?.get('User-Agent'),
        timestamp: new Date(),
        duration: details.duration,
        success: details.success !== false
      });
      await log.save();
      return log;
    } catch (error) {
      console.error('Audit log error:', error);
    }
  }

  async getLogs(filters = {}, limit = 100, offset = 0) {
    const query = {};
    if (filters.userId) query.userId = filters.userId;
    if (filters.action) query.action = filters.action;
    if (filters.resourceId) query.resourceId = filters.resourceId;
    if (filters.fromDate) query.timestamp = { $gte: new Date(filters.fromDate) };
    if (filters.toDate) query.timestamp = { ...query.timestamp, $lte: new Date(filters.toDate) };

    const logs = await AuditLog.find(query)
      .sort({ timestamp: -1 })
      .skip(offset)
      .limit(limit);
    const total = await AuditLog.countDocuments(query);
    return { logs, total };
  }

  async getUserActivity(userId, days = 7) {
    const since = new Date();
    since.setDate(since.getDate() - days);
    return await AuditLog.find({ userId, timestamp: { $gte: since } }).sort({ timestamp: -1 });
  }

  async getStats() {
    const stats = await AuditLog.aggregate([
      { $group: { _id: '$action', count: { $sum: 1 } } }
    ]);
    const daily = await AuditLog.aggregate([
      { $group: { _id: { $dateToString: { format: '%Y-%m-%d', date: '$timestamp' } }, count: { $sum: 1 } } },
      { $sort: { _id: -1 } },
      { $limit: 30 }
    ]);
    return { byAction: stats, daily };
  }
}

module.exports = new AuditService();
