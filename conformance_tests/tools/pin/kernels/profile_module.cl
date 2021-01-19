/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

double cnd(double d) {
  const double A1 = 0.31938153;
  const double A2 = -0.356563782;
  const double A3 = 1.781477937;
  const double A4 = -1.821255978;
  const double A5 = 1.330274429;
  const double RSQRT2PI = 0.39894228040143267793994605993438;

  double K = 1.0 / (1.0 + 0.2316419 * fabs(d));

  double val = RSQRT2PI * exp(-0.5 * d * d) *
               (K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

  if (d > 0.)
    val = 1.0 - val;

  return val;
}

__kernel void blackscholes(const double riskfree, const double volatility,
                           global const double *T, global const double *X,
                           global const double *S, global double *Call,
                           global double *Put) {
  const double sqrt1_2 = 0.70710678118654752440084436210485;
  const double c_half = 0.5;

  int id = get_global_id(0);
  double sqrtT = sqrt(T[id]);
  double d1 = (log(S[id] / X[id]) +
               (riskfree + c_half * volatility * volatility) * T[id]) /
              (volatility * sqrtT);
  double d2 = d1 - volatility * sqrtT;
  double CNDD1 = cnd(d1);
  double CNDD2 = cnd(d2);
  double expRT = exp(-riskfree * T[id]);

  Call[id] = S[id] * CNDD1 - X[id] * expRT * CNDD2;
  Put[id] = Call[id] + expRT - S[id];
}
