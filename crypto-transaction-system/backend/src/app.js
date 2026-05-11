const express = require('express');
const mongoose = require('mongoose');
const crypto = require('crypto');
const transactionRoutes = require('./routes/transactionRoutes');
const authRoutes = require('./routes/authRoutes');
const adminRoutes = require('./routes/adminRoutes');
const webhookRoutes = require('./routes/webhookRoutes');
const dotenv = require('dotenv');
const passport = require('passport');
const { Strategy: JwtStrategy, ExtractJwt } = require('passport-jwt');

dotenv.config();

const app = express();

const jwtOptions = {
  jwtFromRequest: ExtractJwt.fromAuthHeaderAsBearerToken(),
  secretOrKey: process.env.JWT_SECRET
};

passport.use(new JwtStrategy(jwtOptions, async (payload, done) => {
  try {
    return done(null, { id: payload.sub, email: payload.email });
  } catch (error) {
    return done(error, null);
  }
}));

app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(passport.initialize());

app.use('/api/v1/auth', authRoutes);
app.use('/api/v1/transactions', transactionRoutes);
app.use('/api/v1/admin', passport.authenticate('jwt', { session: false }), adminRoutes);
app.use('/api/v1/webhooks', webhookRoutes);

app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date(), version: '1.0.0' });
});

app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).json({ error: 'Internal server error', message: err.message });
});

module.exports = app;
