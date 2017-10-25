/*!	@file	aoe-sr.h
	@author Anton Khayrduinov
	@email	a.khayrudinov@gmail.com

	Provides an interface to the HDLA library in the form
	acceptable by the ArithmeticOperatorExpansion transformation.
	Defines a set of service routines required by the transformation.

	This library uses a simple background idea. An integer number is assumed
	to have infinite number of bits. Several LSBs (least significat bits)
	are stored in a bitarray, that usually has size 8, 16 or 32.
	All other bits depend on that whether the number is signed or unsigned:
	if it's unsigned, then those bits are zeros; if it's signed, then all those bits
	equal to the last bit in the bit array, that's MSB.

	For example let's consider a 5-bit unnsigned integer 6:

		• number of bits is 5
		• bit array has 5 bits: 01100
		• it's unsigned, so the number is equal to 0110 0000 0000 0000 <...>

	Now let's consider a signed 4-bit integer equals 11:

		• number of bits is 4
		• bit array is 1101
		• the number is signed, so it equals 1101 1111 1111 1111 <...>

	This defines bits with positive indexes. All bits with negative indexes
	are assumed to be zero.

	Therefore a number is represented with three properties:
		
		• number of bits
		• bit array
		• signed/unisgned flag

	Typical subroutine doing some operation on integers accepts a number of
	arguments. The result also has infinite number of bits, but the routine
	writes as many bits to the resulting buffer as it can contain.
*/

#ifndef _AOE_SR_
#define _AOE_SR_

typedef char bit;

#define in const		/// input parameter
#define out				/// output parameter

/// Represents an integer.

typedef struct _sr_int
{
	bit*	b;	/// bits
	int		n;	/// size in bits
	bit		s;	/// sign
}
sr_int;

///	Extracts bits from a variable.

void sr_get_bits(
	out	bit*	bits,	/// resulting bits of the variable
	in	void*	var,	/// a pointer to the variable	
	in	int		n);		/// variable size in bits

///	Collects bits to a variable.

void sr_set_bits(
	in	bit*	bits,	/// bits of the variable
	out	void*	var,	/// a pointer to the variable	
	in	int		n);		/// variable size in bits

/// Creates an integer descriptor.

sr_int sr_create(
	bit*	bits,
	int		n,
	bit		sign);

/// Fills an integer with a specified bit.

void sr_fill(
	out	sr_int	r,
	in	bit		f);

/// Converts an integer of one size to an integer of another size.

void sr_conv(
	out	sr_int	r,
	in	sr_int	a);	

/// r = a + b

void sr_add(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = a - b

void sr_sub(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = a + 1

void sr_inc(
	out	sr_int	r,
	in	sr_int	a);

/// r = a - 1

void sr_dec(
	out	sr_int	r,
	in	sr_int	a);

/// r = a & b

void sr_and(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = a | b

void sr_or(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = a ^ b

void sr_xor(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = ~(a & b)

void sr_nand(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = ~(a | b)

void sr_nor(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = ~(a ^ b)

void sr_nxor(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = -1 if a < b; r = +1 if a > b; r = 0 if a = b.

void sr_cmp(	
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = 1 if a equals b; r = 0 otherwise

void sr_eq(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = 0 if a equals b; r = 1 otherwise

void sr_neq(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = 1 if a < b; r = 0 otherwise

void sr_less(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = 1 if a > b; r = 0 otherwise

void sr_greater(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = 0 if a < b; r = 1 otherwise

void sr_nless(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = 0 if a > b; r = 1 otherwise

void sr_ngreater(
	out	sr_int	r,
	in	sr_int	a,
	in	sr_int	b);

/// r = a << m

void sr_shl(
	out	sr_int	r,
	in	sr_int	a,
	in	int		m);

/// r = a >> m

void sr_shr(
	out	sr_int	r,
	in	sr_int	a,
	in	int		m);

#endif // #ifndef _AOE_SR_
