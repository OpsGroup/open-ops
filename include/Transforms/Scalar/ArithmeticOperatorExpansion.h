/*!	@file	ArithmeticOperatorExpansion.h
	@author Anton Khayrudinov
	@email	a.khayrudinov@gmail.com
*/

#ifndef _ARITHMETIC_OPERATOR_EXPANSION_
#define _ARITHMETIC_OPERATOR_EXPANSION_

#include "Reprise/Expressions.h"

namespace OPS
{

namespace Transforms
{

namespace Scalar
{

void expandArithmeticOperators(OPS::Reprise::StatementBase&);

/*! @brief Arithmetic Operators Transformations

	This is the common note to all transformations in this group.

	Typical transformation accepts an expression and introduces
	subroutines calls to algorithms that operate on bits. For instance,
	the plus "+" operator can be replaced with a call in this way:

		a + b

	is replaced with

		f(a, b)

	where the "f" routine extracts bits of its arguments, calls a 
	specific algorithm that adds two bit arrays and then collects
	the resulting bit array into a value and returns it.

	Any transformation is applied if the following conditions are met:
	
		• Operands are of known integer types. "Known types" are types
			that're recongnised by some internal routines of these
			transformations.

		• Subroutines that implement particular algorithms are defined: e.g.
			if "+" is transformed, then the "add" subroutine must be defined
			somewhere in the program (see below).

		• "get_bits" and "set_bits" subroutines must be defined: they're needed
			to get access to variables' bits (see below).

	Signature of a subroutine that implements the "+" operator:

		void add(
			bool* r_bits, int r_n,	// the result of addition a + b
			bool* a_bits, int a_n,	// bits of the first argument
			bool* b_bits, int b_n);	// bits of the second argument

	Signature of the "get_bits" subroutine:

		void get_bits(
			bool* bits,			// bits of the variable
			const void* var,	// a pointer to the variable
			int size);			// size in bits of the variable

	Signature of the "set_bits" subroutine:

		void set_bits(
			const bool* bits,	// bits of the variable
			void* var,			// a pointer to the variable
			int size);			// size in bits of the variable

	Normally, all these service subroutines are defined in
	a library written specially for this transformation. This means
	that you don't have to bother about these subroutines, but you have
	to known where to get the sources with them and to include
	suitable headers that conatins all these set_bits/get_bits stuff.	
*/

namespace ArithmeticOperatorExpansion
{

namespace Utils
{

enum TextId
{
	// service routines

	SR_GET_BITS,			// get_bits routine
	SR_SET_BITS,			// set_bits routine
	SR_CREATE,				// sr_create routine
	SR_INT_STRUCT,			// sr_int structure
	SR_BIT,					// "bit" type

	// arithmetic algorithms

	AA_CONV,				// converts an integer to another integer of different size

	AA_BIT_NOT,
	AA_BIT_AND,
	AA_BIT_OR,
	AA_BIT_XOR,

	AA_LOG_NOT,
	AA_LOG_AND,
	AA_LOG_OR,
	
	AA_SHL,
	AA_SHR,

	AA_ADD,
	AA_SUB,
	AA_NEG,
	AA_MUL,
	AA_DIV,
	AA_MOD,
	AA_INC,					// ++
	AA_DEC,					// --

	AA_EQ,					// ==
	AA_NOT_EQ,				// !=
	AA_GREATER,				// >
	AA_LESS,				// <
	AA_GREATER_EQ,			// >=
	AA_LESS_EQ,				// <=
};

const std::string& getText(TextId);
std::string intToStr(int a);

} // namespace Utils

/*!	Replaces integer multiplication with the shift operation.
	Example:

		(a + b)*64		// before
		(a + b) << 6	// after

	Replaces integer division with the shift operation:
	Example:

		(a + b)/64		// before
		(a + b) >> 6	// after

	• the expression has the form A*B or A/B
	• the first argument has an integer type
	• the last argument is a StrictLiteralExpression (e.g. 4, 8)
	• the last argument is a power of 2

	@see Utils
*/

namespace MulDivToShift
{

using OPS::Reprise::BasicCallExpression;

bool canApply(const BasicCallExpression&);
void apply(BasicCallExpression&);

} // namespace MulDivToShift

/*!	Replaces the integer divison on modulo with the bitwise "and" operation.
	Example:

		(a + b) % 8

	is replaced with

		(a + b) & 7

	• the expression has the form A % B
	• the first argument has an integer type
	• the last argument is a StrictLiteralExpression (e.g. 4, 8)
	• the last argument is a power of 2

	@see Utils
*/

namespace ModToAnd
{

using OPS::Reprise::BasicCallExpression;

bool canApply(const BasicCallExpression&);
void apply(BasicCallExpression&);

} // namespace ModToAnd

/*!	Replaces an arithmetic operation with a subroutine call
	that implements this operator. Example:

		~a + b

	assuming that a is int32 and b is uint16, this expression
	is replaced with

		i32_add_i32_u16(i32_not_i32(a), b)

	where the two subroutines are created automatically.

	@see Utils
*/

namespace AopToCall
{

using OPS::Reprise::BasicCallExpression;

bool canApply(const BasicCallExpression&);
void apply(BasicCallExpression&);

} // namespace AopToCall

/*!	@brief Subroutine Call Arguments Coercion

	Adds explicit types coercion to arguments of a subroutine call
	if the arguments' types mismatch types of parameters in the subroutine
	declaration. Example:

		int shift(int a, byte bits);
		
		<...>

		shift(a, 3);

	assuming that a has "int" type, the transfomation will be as following:

		shift(a, u8_conv_i32(3));

	@see Utils	
*/

namespace SCAC
{

using OPS::Reprise::SubroutineCallExpression;

bool canApply(const SubroutineCallExpression&);
void apply(SubroutineCallExpression&);

} // namespace SCAC

/*!	Adjusts assignment of one integer value to a variable of
	another integer type. Example:

		int a;
		byte b;

		a = b

	will be replaced with

		int a;
		byte b;

		a = i32_conv_u8(b);

	@see Utils
*/

namespace AssignmentCoercion
{

using OPS::Reprise::BasicCallExpression;

bool canApply(const BasicCallExpression&);
void apply(BasicCallExpression&);

} // namespace AssignmentCoercion

/*!	Replaces expressions like (a + 1) with a call to an increment algorithm.
	Example:

		(a - b) + 1;
		(c*d - e) - 1;

	will be replaced with

		inc(a - b);
		dec(c*d - e);

	@see Utils
*/

namespace AddToInc
{

using OPS::Reprise::BasicCallExpression;

bool canApply(const BasicCallExpression&);
void apply(BasicCallExpression&);

} // namespace AddToInc

} // namespace ArithmeticOperatorExpansion

} // namespace Scalar

} // namespace Transforms

} // namespace OPS

#endif // _ARITHMETIC_OPERATOR_EXPANSION_
