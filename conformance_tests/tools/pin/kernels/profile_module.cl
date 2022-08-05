/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

float cnd(float d) {
	const float       A1 = 0.31938153f;
	const float       A2 = -0.356563782f;
	const float       A3 = 1.781477937f;
	const float       A4 = -1.821255978f;
	const float       A5 = 1.330274429f;
	const float RSQRT2PI = 0.39894228040143267793994605993438f;

	float K = 1.0f / (1.0f + 0.2316419f * fabs(d));

	float val = RSQRT2PI * exp(-0.5f * d * d) *
		(K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

	if (d > 0.f)
		val = 1.0f - val;

	return val;
}

kernel void blackscholes(const float riskfree, const float volatility, global const float* T, global const float* X, global const float* S, global float* Call, global float* Put) {
	const float c_half = 0.5f;

	int id = get_global_id(0);
	float sqrtT = sqrt(T[id]);
	float d1 = (log(S[id] / X[id]) + (riskfree + c_half * volatility * volatility) * T[id]) / (volatility * sqrtT);
	float d2 = d1 - volatility * sqrtT;
	float CNDD1 = cnd(d1);
	float CNDD2 = cnd(d2);
	float expRT = exp(-riskfree * T[id]);

	Call[id] = S[id] * CNDD1 - X[id] * expRT * CNDD2;
	Put[id] = Call[id] + expRT - S[id];
}
