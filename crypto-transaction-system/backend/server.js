const express = require('express');
const mongoose = require('mongoose');
const crypto = require('crypto');
const rateLimit = require('express-rate-limit');
const helmet = require('helmet');
const cors = require('cors');
const compression = require('compression');
const winston = require('winston');
const dotenv = require('dotenv');
const passport = require('passport');
const { createServer } = require('http');
const { Server } = require('socket.io');
const swaggerUi = require('swagger-ui-express');
const swaggerJsdoc = require('swagger-jsdoc');

dotenv.config();

const app = express();
const server = createServer(app);
const io = new Server(server);
const PORT = process.env.PORT || 3000;

const logger = winston.createLogger({
  level: 'info',
  format: winston.format.json(),
  transports: [
    new winston.transports.File({ filename: 'logs/error.log', level: 'error' }),
    new winston.transports.File({ filename: 'logs/combined.log' }),
    new winston.transports.Console({ format: winston.format.simple() })
  ]
});

app.use(helmet());
app.use(cors());
app.use(compression());
app.use(express.json());
app.use(express.raw({ type: 'application/octet-stream', limit: '100mb' }));
app.use(express.urlencoded({ extended: true }));

const limiter = rateLimit({
  windowMs: 15 * 60 * 1000,
  max: 100,
  message: 'Too many requests from this IP'
});
app.use('/api/', limiter);

const swaggerOptions = {
  definition: {
    openapi: '3.0.0',
    info: { title: 'Crypto Transaction API', version: '1.0.0' }
  },
  apis: ['./routes/*.js']
};
app.use('/api-docs', swaggerUi.serve, swaggerUi.setup(swaggerJsdoc(swaggerOptions)));

const transactionSchema = new mongoose.Schema({
  transactionId: { type: String, unique: true, required: true },
  encryptedData: Buffer,
  chunks: [Buffer],
  originalHash: String,
  status: { type: String, enum: ['pending', 'encrypted', 'processing', 'processed', 'failed', 'decrypted'], default: 'pending' },
  timestamp: { type: Date, default: Date.now },
  updatedAt: { type: Date, default: Date.now },
  retryCount: { type: Number, default: 0 },
  errorMessage: String,
  metadata: {
    source: String,
    destination: String,
    amount: Number,
    currency: String,
    ipAddress: String,
    userAgent: String
  },
  tags: [String],
  priority: { type: Number, default: 0 }
});

transactionSchema.index({ transactionId: 1 });
transactionSchema.index({ timestamp: -1 });
transactionSchema.index({ status: 1 });
transactionSchema.index({ priority: -1 });

const Transaction = mongoose.model('Transaction', transactionSchema);

const chunkSchema = new mongoose.Schema({
  transactionId: { type: String, required: true, index: true },
  index: { type: Number, required: true },
  data: Buffer,
  position: { type: Number, default: 0 },
  hash: String
});
chunkSchema.index({ transactionId: 1, index: 1 });
const Chunk = mongoose.model('Chunk', chunkSchema);

io.on('connection', (socket) => {
  logger.info('New client connected');
  socket.on('transaction:encrypt', async (data) => {
    try {
      const result = await encryptTransaction(data);
      socket.emit('transaction:encrypted', result);
    } catch (error) {
      socket.emit('transaction:error', error.message);
    }
  });
  socket.on('disconnect', () => logger.info('Client disconnected'));
});

async function encryptTransaction(data) {
  const key = Buffer.from(process.env.AES_KEY, 'hex');
  const iv = crypto.randomBytes(16);
  const cipher = crypto.createCipheriv('aes-256-cbc', key, iv);
  let encrypted = cipher.update(data.text, 'utf8', 'hex');
  encrypted += cipher.final('hex');
  const combined = Buffer.concat([iv, Buffer.from(encrypted, 'hex')]);
  const chunks = [];
  for (let i = 0; i < combined.length; i += 1024) {
    chunks.push(combined.slice(i, i + 1024));
  }
  const transaction = new Transaction({
    transactionId: crypto.randomUUID(),
    encryptedData: combined,
    chunks: chunks,
    originalHash: crypto.createHash('sha256').update(data.text).digest('hex'),
    status: 'encrypted',
    metadata: data.metadata,
    tags: data.tags || []
  });
  await transaction.save();
  for (let i = 0; i < chunks.length; i++) {
    const chunk = new Chunk({
      transactionId: transaction.transactionId,
      index: i,
      data: chunks[i],
      position: Math.floor(Math.random() * 1000000),
      hash: crypto.createHash('sha256').update(chunks[i]).digest('hex')
    });
    await chunk.save();
  }
  return { transactionId: transaction.transactionId, chunksCount: chunks.length };
}

async function decryptTransaction(transactionId, keyBuffer) {
  const transaction = await Transaction.findOne({ transactionId });
  if (!transaction) throw new Error('Transaction not found');
  const key = keyBuffer || Buffer.from(process.env.AES_KEY, 'hex');
  const iv = transaction.encryptedData.slice(0, 16);
  const encryptedData = transaction.encryptedData.slice(16);
  const decipher = crypto.createDecipheriv('aes-256-cbc', key, iv);
  let decrypted = decipher.update(encryptedData, 'hex', 'utf8');
  decrypted += decipher.final('utf8');
  return { data: decrypted, hash: transaction.originalHash };
}

app.post('/api/v1/encrypt', async (req, res) => {
  try {
    const result = await encryptTransaction(req.body);
    res.status(201).json(result);
  } catch (error) {
    logger.error(error);
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/v1/decrypt/:id', async (req, res) => {
  try {
    const result = await decryptTransaction(req.params.id, req.body.key);
    res.json(result);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get('/api/v1/transactions', async (req, res) => {
  const { limit = 50, offset = 0, status } = req.query;
  const query = {};
  if (status) query.status = status;
  const transactions = await Transaction.find(query).sort({ timestamp: -1 }).skip(parseInt(offset)).limit(parseInt(limit));
  const total = await Transaction.countDocuments(query);
  res.json({ data: transactions, total, limit, offset });
});

app.get('/api/v1/transactions/:id', async (req, res) => {
  const transaction = await Transaction.findOne({ transactionId: req.params.id });
  if (!transaction) return res.status(404).json({ error: 'Not found' });
  res.json(transaction);
});

app.get('/api/v1/transactions/:id/chunks', async (req, res) => {
  const chunks = await Chunk.find({ transactionId: req.params.id }).sort('index');
  res.json({ chunks: chunks.map(c => ({ index: c.index, data: c.data.toString('hex'), position: c.position, hash: c.hash })) });
});

app.delete('/api/v1/transactions/:id', async (req, res) => {
  await Transaction.findOneAndDelete({ transactionId: req.params.id });
  await Chunk.deleteMany({ transactionId: req.params.id });
  res.json({ message: 'Deleted' });
});

app.get('/api/v1/stats', async (req, res) => {
  const total = await Transaction.countDocuments();
  const encrypted = await Transaction.countDocuments({ status: 'encrypted' });
  const processed = await Transaction.countDocuments({ status: 'processed' });
  const failed = await Transaction.countDocuments({ status: 'failed' });
  const totalChunks = await Chunk.countDocuments();
  res.json({ total, encrypted, processed, failed, totalChunks });
});

app.get('/health', (req, res) => {
  res.json({ status: 'healthy', timestamp: new Date(), uptime: process.uptime() });
});

mongoose.connect(process.env.MONGODB_URI, { useNewUrlParser: true, useUnifiedTopology: true })
  .then(() => {
    logger.info('MongoDB connected');
    server.listen(PORT, () => logger.info(`Server running on port ${PORT}`));
  })
  .catch(err => logger.error(err));

process.on('unhandledRejection', (err) => {
  logger.error('Unhandled Rejection:', err);
  process.exit(1);
});
