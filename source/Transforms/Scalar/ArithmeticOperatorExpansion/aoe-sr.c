#include "aoe-sr.h"

typedef unsigned char byte;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

void sr_get_bits(
	out	bit*	bits,
	in	void*	var,
	in	int		n)
{
	const int m = 8*sizeof(byte);
	const byte* data = (const byte*)var;
	int i;

	for (i = 0; i < n; i++)
		bits[i] = data[i / m] & (1 << i % m) ? 1 : 0;
}

void sr_set_bits(
	in	bit*	bits,
	out	void*	var,
	in	int		n)
{
	const int m = 8*sizeof(byte);
	byte* data = (byte*)var;
	int i;

	for (i = 0; i < n; i++)
		if (bits[i])
			data[i / m] |= 1 << (i % m);
		else
			data[i / m] &= ~(1 << (i % m));
}

sr_int sr_create(
	bit*	bits,
	int		n,
	bit		sign)
{
	sr_int i;

	i.b = bits;
	i.n = n;
	i.s = sign;

	return i;
}

void sr_fill(
	out	sr_int	r,
	in	bit		f)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = f;
}

bit sr_bit_ex(
	in	sr_int	a,
	in	int		i,
	in	bit		f)
{
	if (i < 0)
		return 0;

	if (i < a.n)
		return a.b[i];
	
	return a.s & f;
}

bit sr_bit(
	in	sr_int	a,
	in	int		i)
{
	return sr_bit_ex(a, i, a.b[a.n - 1]);	
}

void sr_add(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b)
{
	int i;
	bit ci = 0;	

	for (i = 0; i < r.n; i++)
	{
		bit ai = sr_bit(a, i);
		bit bi = sr_bit(b, i);

		r.b[i] = ai ^ bi ^ ci;
		ci = ai & bi ^ (ai ^ bi) & ci;
	}
}

void sr_sub(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b)
{
	int i;
	bit ci = 0;

	for (i = 0; i < r.n; i++)
	{
		bit ai = sr_bit(a, i);
		bit bi = sr_bit(b, i);

		r.b[i] = ai ^ bi ^ ci;
		ci = (ai ^ 1) & (bi ^ ci) ^ bi & ci;
	}
}

void sr_inc(
	out	sr_int	r,
	in	sr_int	a)
{
	bit b[1] = {1};
	sr_add(r, a, sr_create(b, 1, 0));
}

void sr_dec(
	out	sr_int	r,
	in	sr_int	a)
{
	bit b[1] = {1};
	sr_sub(r, a, sr_create(b, 1, 0));
	// sr_add(r, a, sr_create(b, 1, 1)) is also correct
}

void sr_conv(
	out sr_int r,
	in	sr_int a)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = sr_bit(a, i);
}

void sr_not(
	out sr_int r,
	in	sr_int a)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = sr_bit(a, i) ^ 1;
}

void sr_and(
	out sr_int r,
	in	sr_int a,
	in	sr_int b)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = sr_bit(a, i) & sr_bit(b, i);
}

void sr_or(
	out sr_int r,
	in	sr_int a,
	in	sr_int b)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = sr_bit(a, i) | sr_bit(b, i);
}

void sr_xor(
	out sr_int r,
	in	sr_int a,
	in	sr_int b)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = sr_bit(a, i) ^ sr_bit(b, i);
}

void sr_nand(
	out sr_int r,
	in	sr_int a,
	in	sr_int b)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = (sr_bit(a, i) & sr_bit(b, i)) ^ 1;
}

void sr_nor(
	out sr_int r,
	in	sr_int a,
	in	sr_int b)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = (sr_bit(a, i) | sr_bit(b, i)) ^ 1;
}

void sr_nxor(
	out sr_int r,
	in	sr_int a,
	in	sr_int b)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = sr_bit(a, i) ^ sr_bit(b, i) ^ 1;
}

void sr_neq(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b)
{
	int i;
	bit c = 0;

	for (i = 0; i < max(a.n, b.n); i++)
		c |= sr_bit(a, i) ^ sr_bit(b, i);

	r.b[0] = c;

	for (i = 1; i < r.n; i++)
		r.b[i] = 0;	
}

void sr_eq(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b)
{
	sr_neq(r, a, b);
	r.b[0] ^= 1;
}

void sr_cmp(	
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = 0;

	for (i = max(a.n, b.n) - 1; i >= 0; i--)
		if (sr_bit(a, i) ^ sr_bit(b, i))
		{
			bit f = sr_bit(b, i);

			r.b[0] = 1;

			for (i = 1; i < r.n; i++)
				r.b[i] = f;

			break;
		}
}

void sr_less(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b)
{
	bit f[2];
	sr_cmp(sr_create(f, 2, 1), a, b);
	sr_fill(r, 0);
	r.b[0] = f[0] & f[1];
}

void sr_greater(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b)
{
	bit f[2];
	sr_cmp(sr_create(f, 2, 1), a, b);
	sr_fill(r, 0);
	r.b[0] = f[0] & (f[1] ^ 1);
}

void sr_nless(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b)
{
	sr_less(r, a, b);
	r.b[0] ^= 1;
}

void sr_ngreater(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b)
{
	sr_greater(r, a, b);
	r.b[0] ^= 1;
}

void sr_shl(
	out	sr_int	r,
	in	sr_int	a,
	in	int		m)
{
	bit f = sr_bit(a, a.n - m - 1);
	int i;	

	for (i = r.n - 1; i >= 0; i--)
		r.b[i] = sr_bit_ex(a, i - m, f);	
}

void sr_shr(
	out	sr_int	r,
	in	sr_int	a,
	in	int		m)
{
	int i;

	for (i = 0; i < r.n; i++)
		r.b[i] = sr_bit(a, i + m);
}