// direct computation with d and k:
//diff = d - t
//bot = FMA(2, x, FMA(-3, t, d))
//left = (diff / d) * diff
//right = diff / (d * bot)
//lvlmult = left * right

// if k = d-1
//d_sqr = d * d
//bot = FMA(2, x - d, 3)
//lvlmult = 1 / (d_sqr * bot)

// if k = a*d
//_1_m_a = 1 - a
//x_d = x / d
//top = _1_m_a * _1_m_a * _1_m_a
//bot = FMA(-3, a, FMA(x_d, 2, 1))
//lvlmult = top / bot
