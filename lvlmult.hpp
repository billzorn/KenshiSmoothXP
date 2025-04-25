#pragma once

#include <cmath>

template <typename fp_t>
fp_t lvlmult_vanilla(fp_t x, fp_t d) {
    // original function: (*valuePointer is x, factor2 is d)
    // const float invFactor2 = 1.0f / factor2;
    // float normalizedDifference = (factor2 - *valuePointer) * invFactor2;
    // val = normalizedDifference * normalizedDifference;
    // (val is lvlmult)

    // herbie has a better FMA one for this too actually
    //ratio = x / d
    //lvlmult = FMA(ratio - 2, ratio, 1)

    const fp_t ratio = x / d;
    return fma(ratio - static_cast<fp_t>(2.0), ratio, static_cast<fp_t>(1.0));
}

template <typename fp_t>
fp_t lvlmult_mod(fp_t x, fp_t d, fp_t t) {
    // first define crossover points:
    // k = (d-t) / 2 - t = (d-3t) / 2
    // b = (d-t)^3 / d^2 * (d-3t)
    // now lvlmult in terms of b and k is:
    // (b * k) / (k + x)

	//# direct computation, as advised by Herbie
    //diff = d - t
    //bot = FMA(2, x, FMA(-3, t, d))
    //left = (diff / d) * diff
    //right = diff / (d * bot)
    //lvlmult = left * right

    const fp_t diff = d - t;
    const fp_t bot = fma(static_cast<fp_t>(2.0), x,
                         fma(static_cast<fp_t>(-3.0), t, d));
    const fp_t left = (diff / d) * diff;
    const fp_t right = diff / (d * bot);
    return left * right;
}

template <typename fp_t>
fp_t lvlmult_dm1(fp_t x, fp_t d) {
    // special case where we assume t is d-1
    // now the crossover point is:
    // k = (1/2) - t
    // b = 1 / d^2 * (1 - 2*t)
    // as before, lvlmult is:
    // (b * k) / (k + x)

    //# direct computation with FMA
    //d_sqr = d * d
    //bot = FMA(2, x - d, 3)
    //lvlmult = 1 / (d_sqr * bot)

    const fp_t d_sqr = d * d;
    const fp_t bot = fma(static_cast<fp_t>(2.0), x - d, static_cast<fp_t>(3.0));
    return static_cast<fp_t>(1.0) / (d_sqr * bot);
}

template <typename fp_t>
fp_t lvlmult_ratio(fp_t x, fp_t d, fp_t a) {
    // special case where we assume t is a fraction of d:
    // a is the ratio (0 < a < 1; t = d * a)

    //# Herbie likes this one
    //_1_m_a = 1 - a
    //x_d = x / d
    //top = _1_m_a * _1_m_a * _1_m_a
    //bot = FMA(-3, a, FMA(x_d, 2, 1))
    //lvlmult = top / bot

    const fp_t _1_m_a = static_cast<fp_t>(1.0) - a;
    const fp_t x_d = x / d;
    const fp_t top = _1_m_a * _1_m_a * _1_m_a;
    const fp_t bot = fma(static_cast<fp_t>(-3.0), a,
                         fma(x_d, static_cast<fp_t>(2.0), static_cast<fp_t>(1.0)));
    return top / bot;
}

template <typename fp_t>
fp_t lvlmult_ratio_precomp(fp_t x, fp_t d, fp_t a, fp_t one_minus_a_cubed) {
    // we can actually precompute (1 - a)^3
    // because a is known at configuration time

    const fp_t x_d = x / d;
    const fp_t bot = fma(static_cast<fp_t>(-3.0), a,
                         fma(x_d, static_cast<fp_t>(2.0), static_cast<fp_t>(1.0)));
    return one_minus_a_cubed / bot;
}
