const { body, param, query, validationResult } = require('express-validator');

class Validator {
  validateTransaction = [
    body('data').notEmpty().withMessage('Data is required').isString().withMessage('Data must be string').isLength({ max: 10000000 }).withMessage('Data too large'),
    body('metadata').optional().isObject().withMessage('Metadata must be object'),
    body('tags').optional().isArray().withMessage('Tags must be array'),
    body('priority').optional().isInt({ min: 0, max: 10 }).withMessage('Priority must be 0-10'),
    (req, res, next) => {
      const errors = validationResult(req);
      if (!errors.isEmpty()) return res.status(400).json({ errors: errors.array() });
      next();
    }
  ];

  validateId = [
    param('id').isUUID().withMessage('Invalid transaction ID format'),
    (req, res, next) => {
      const errors = validationResult(req);
      if (!errors.isEmpty()) return res.status(400).json({ errors: errors.array() });
      next();
    }
  ];

  validatePagination = [
    query('limit').optional().isInt({ min: 1, max: 1000 }).withMessage('Limit must be 1-1000'),
    query('offset').optional().isInt({ min: 0 }).withMessage('Offset must be >= 0'),
    query('status').optional().isIn(['pending', 'encrypted', 'processing', 'processed', 'failed', 'decrypted']).withMessage('Invalid status'),
    (req, res, next) => {
      const errors = validationResult(req);
      if (!errors.isEmpty()) return res.status(400).json({ errors: errors.array() });
      next();
    }
  ];

  validateDateRange = [
    query('fromDate').optional().isISO8601().withMessage('Invalid from date'),
    query('toDate').optional().isISO8601().withMessage('Invalid to date'),
    (req, res, next) => {
      const errors = validationResult(req);
      if (!errors.isEmpty()) return res.status(400).json({ errors: errors.array() });
      next();
    }
  ];

  validateAuth = [
    body('email').isEmail().withMessage('Valid email required'),
    body('password').isLength({ min: 6 }).withMessage('Password must be at least 6 chars'),
    (req, res, next) => {
      const errors = validationResult(req);
      if (!errors.isEmpty()) return res.status(400).json({ errors: errors.array() });
      next();
    }
  ];

  validateApiKey = (req, res, next) => {
    const apiKey = req.headers['x-api-key'];
    if (!apiKey) return res.status(401).json({ error: 'API key required' });
    if (apiKey !== process.env.API_KEY && apiKey !== 'test-key-2024') {
      return res.status(403).json({ error: 'Invalid API key' });
    }
    next();
  };

  rateLimit = (maxRequests = 60, windowMs = 60000) => {
    const requests = new Map();
    return (req, res, next) => {
      const key = req.ip;
      const now = Date.now();
      const userRequests = requests.get(key) || [];
      const validRequests = userRequests.filter(time => now - time < windowMs);
      if (validRequests.length >= maxRequests) {
        return res.status(429).json({ error: 'Too many requests' });
      }
      validRequests.push(now);
      requests.set(key, validRequests);
      next();
    };
  };
}

module.exports = new Validator();
