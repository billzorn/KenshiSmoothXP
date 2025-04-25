#pragma once

#include <cmath>

// the level multiplier in Kenshi:
// https://www.reddit.com/r/Kenshi/comments/1hisvzq/kenshi_fact_of_the_day_11/
//
// this code computes the value of the level multiplier
// in a numerically optimized way; some optimization help
// was provided by the Herbie tool:
// https://herbie.uwplse.org/
//
// In addition to the vanilla level multiplier:
//   ((d - x) / d)^2, d is 101
// options are provided to compute a different function:
//   (b * k) / (k + x), b and k computed from d
// This is intended to be used at high levels (>= d),
// retaining the original level curve for levels x<d,
// but smoothly transitioning (at level k) into a new
// function that never converges completely to zero.
// In this way experience gain can continue (very slowly)
// at high levels, and the other side of the quadratic
// vanilla lvlmult curve does not cause increasing xp gain.
//
// The functions offer several different ways of computing
// values of b and k so that the values and derivatives
// of the lvlmult curves match at the desired transition point,
// referred to as t.
// If t is known directly (for example, if you want to use
// vanilla lvlmult up to level t=95, and use the mod curve
// over level 95), it can be passed to lvlmult_mod().
// t can also be derived relative to d; lvlmult_dm1()
// assumes t=d-1, and lvlmult_ratio() takes a ratio argument a
// and computes t=d*a. lvlmult_ratio_precomp() is the same
// as lvlmult_ratio() but it takes an additional argument
// with a precomputed value for (1-a)^3.

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
