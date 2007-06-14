/* $Id$ */

#ifndef MACROS_H
#define MACROS_H

/// Fetch n bits starting at bit s from x
#define GB(x, s, n) (((x) >> (s)) & ((1U << (n)) - 1))
/// Set n bits starting at bit s in x to d
#define SB(x, s, n, d) ((x) = ((x) & ~(((1U << (n)) - 1) << (s))) | ((d) << (s)))
/// Add i to the n bits starting at bit s in x
#define AB(x, s, n, i) ((x) = ((x) & ~(((1U << (n)) - 1) << (s))) | (((x) + ((i) << (s))) & (((1U << (n)) - 1) << (s))))

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

static inline int min(int a, int b) { if (a <= b) return a; return b; }
static inline int max(int a, int b) { if (a >= b) return a; return b; }

static inline uint minu(uint a, uint b) { if (a <= b) return a; return b; }
static inline uint maxu(uint a, uint b) { if (a >= b) return a; return b; }


static inline int clamp(int a, int min, int max)
{
	if (a <= min) return min;
	if (a >= max) return max;
	return a;
}

static inline uint clampu(uint a, uint min, uint max)
{
	if (a <= min) return min;
	if (a >= max) return max;
	return a;
}

#define HASBIT(x,y)    (((x) & (1 << (y))) != 0)
#define SETBIT(x,y)    ((x) |=  (1 << (y)))
#define CLRBIT(x,y)    ((x) &= ~(1 << (y)))
#define TOGGLEBIT(x,y) ((x) ^=  (1 << (y)))

#endif /* MACROS_H */
