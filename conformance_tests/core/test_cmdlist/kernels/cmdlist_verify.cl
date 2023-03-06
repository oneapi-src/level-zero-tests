/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

/*
 * A kernel that takes long execution times, and produces easily verifiable results
 *
 * The inequality is difficul to evaluate but is always true. Since the compiler cannot use static
 * analysis to prove it, the "useless" work will not be optimized away
 */

#define X(IX) (-37.0f * cos(127.0f * sin(log(fabs(pown(asin(cos(convert_float(IX) + 0.3f)), 3))))))

#define X2(IX) (X(IX) * X(IX))
#define X3(IX) (X2(IX) * X(IX))
#define X4(IX) (X2(IX) * X2(IX))
#define X5(IX) (X3(IX) * X2(IX))
#define X6(IX) (X3(IX) * X3(IX))

/* Polynomial 997x6 - 3x5 + 991x4 - 5x3 + 983x2 - 2x + 2501 is always greater than zero */
#define RHS(IX) (997 * X6(IX) + 991 * X4(IX) + 983 * X2(IX) + 2501)
#define LHS(IX) (3 * X5(IX) + 5 * X3(IX) + 2 * X(IX))

#define INEQUALITY(IX) (RHS(IX) > LHS(IX))
#define NITERS 500

kernel void add_one(global int *vec) {
  int v = vec[get_global_id(0)];

  for (int i = 0; i < NITERS; i++) {
    if (INEQUALITY(v)) {
        v += 1; /* Change v to force re-evaluation of the inequality */
    } else {
        v -= 2;
    }
  }

  /* The loops cancel each other and this line is what's left */
  v += 1;

  for (int i = 0; i < NITERS; i++) {
    if (INEQUALITY(v)) {
        v -= 1; /* Undo changes */
    } else {
        v += 3;
    }
  }

  vec[get_global_id(0)] = v;
}
