const app = require('./app');
const mongoose = require('mongoose');
const dotenv = require('dotenv');
const Redis = require('redis');
const { Worker } = require('bull');
const { createClient } = require('redis');

dotenv.config();

const PORT = process.env.PORT || 3000;

const redisClient = createClient({ url: process.env.REDIS_URL });
redisClient.connect().catch(console.error);

const encryptionQueue = new Worker('encryption', async (job) => {
  console.log('Processing encryption job:', job.data);
  return { result: 'encrypted' };
}, { connection: { url: process.env.REDIS_URL } });

mongoose.connect(process.env.MONGODB_URI, {
  useNewUrlParser: true,
  useUnifiedTopology: true,
  serverSelectionTimeoutMS: 5000
})
  .then(() => {
    console.log('Connected to MongoDB');
    app.listen(PORT, () => {
      console.log(`Server running on port ${PORT}`);
      console.log(`Environment: ${process.env.NODE_ENV}`);
    });
  })
  .catch(err => {
    console.error('MongoDB connection error:', err);
    process.exit(1);
  });

process.on('SIGTERM', () => {
  console.log('SIGTERM received, closing server...');
  process.exit(0);
});

process.on('SIGINT', () => {
  console.log('SIGINT received, closing server...');
  process.exit(0);
});
