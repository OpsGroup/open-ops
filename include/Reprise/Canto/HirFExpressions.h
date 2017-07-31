#ifndef FORTRAN2003_HIR_F_EXPRESSIONS_H
#define FORTRAN2003_HIR_F_EXPRESSIONS_H

#include "Reprise/Expressions.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{


class HirFAltResultExpression : public ExpressionBase
{
public:
	HirFAltResultExpression(int altResult);

	int getAlternativeResult() const;

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;	

//		ClonableMix implementation
	virtual HirFAltResultExpression* clone(void) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
		OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
private:
	int m_altResult;
};

//		used by HirFDimensionExpression
class HirFAsteriskExpression : public ExpressionBase
{
public:
//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;	

//		ClonableMix implementation
	virtual HirFAsteriskExpression* clone(void) const;
	
//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
};


//		used to declare array
class HirFDimensionExpression : public ExpressionBase, public IReplaceChildExpression
{
public:
	HirFDimensionExpression();
	HirFDimensionExpression(ExpressionBase *lowerBound, ExpressionBase *upperBound);
	
	ExpressionBase& getLowerBound();
	const ExpressionBase& getLowerBound() const;
	void setLowerBound(ExpressionBase *bound);
	
	ExpressionBase& getUpperBound();
	const ExpressionBase& getUpperBound() const;
	void setUpperBound(ExpressionBase *bound);

	
//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;	

//		ClonableMix implementation
	virtual HirFDimensionExpression* clone(void) const;
	
//		IReplaceChildExpression implementation
	virtual void replaceChildExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);


//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
private:
	ReprisePtr<ExpressionBase> m_lowerBound, m_upperBound;
};

class HirFArrayShapeExpression : public ExpressionBase, public IReplaceChildExpression
{
public:
	enum ShapeType
	{
		FST_EXPLICIT_SHAPE,
		FST_ASSUMED_SHAPE,
		FST_DEFERRED_SHAPE,
		FST_ASSUMED_SIZE
	};

	HirFArrayShapeExpression();

	static ShapeType getShapeType(HirFArrayShapeExpression& shape);
		
	void addDimension(HirFDimensionExpression* dimension)
	{	
		dimension->setParent(this);
		m_dimensionList.add(dimension);
	}

	HirFDimensionExpression& getDimension(int index)
	{
		return m_dimensionList[index];
	}

	const HirFDimensionExpression& getDimension(int index) const
	{
		return m_dimensionList[index];
	}

	int getRank() const 
	{
		return m_dimensionList.size();
	}

	//		IReplaceChildExpression implementation
	virtual void replaceChildExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

	//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& expression) const;

	//		ClonableMix implementation
	virtual HirFArrayShapeExpression* clone(void) const;

	//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()

private:
	typedef RepriseList<HirFDimensionExpression> TDimensionList;
	TDimensionList m_dimensionList;
};

class HirFArrayIndexRangeExpression : public CallExpressionBase
{
public:	
	HirFArrayIndexRangeExpression();

	//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

	//		ClonableMix implementation
	virtual HirFArrayIndexRangeExpression* clone(void) const;

	//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
};

/* (A,B,C, i*2, i = 1, 10, 2) */
class HirFImpliedDoExpression : public ExpressionBase, public IReplaceChildExpression
{
public:		
	HirFImpliedDoExpression(ExpressionBase* initExpression, 
			ExpressionBase* finalExpression, 
			ExpressionBase* stepExpression,
			ExpressionBase* bodyExpression);

	const ExpressionBase& getInitExpression(void) const;
	ExpressionBase& getInitExpression(void);
	void setInitExpression(ExpressionBase* initExpression);

	const ExpressionBase& getFinalExpression(void) const;
	ExpressionBase& getFinalExpression(void);
	void setFinalExpression(ExpressionBase* finalExpression);

	const ExpressionBase& getStepExpression(void) const;
	ExpressionBase& getStepExpression(void);
	void setStepExpression(ExpressionBase* stepExpression);

	const ExpressionBase& getBodyExpression(void) const;
	ExpressionBase& getBodyExpression(void);
	void setBodyExpression(ExpressionBase* stepExpression);
	

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;	

//		ClonableMix implementation
	virtual HirFImpliedDoExpression* clone(void) const;
	
//		IReplaceChildExpression implementation
	virtual void replaceChildExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
private:
	ReprisePtr<ExpressionBase> m_initExpression;
	ReprisePtr<ExpressionBase> m_finalExpression;
	ReprisePtr<ExpressionBase> m_stepExpression;
	ReprisePtr<ExpressionBase> m_bodyExpression;
};



class HirFArgumentPairExpression : public ExpressionBase, public IReplaceChildExpression
{
public:
	HirFArgumentPairExpression(std::string name, ExpressionBase* expression);

	const std::string& getName() const;
	std::string& getName();
	void setName(std::string name);

	const ExpressionBase& getValue() const;
	ExpressionBase& getValue();
	void setValue(ExpressionBase* value);

	//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;	

	//		ClonableMix implementation
	virtual HirFArgumentPairExpression* clone(void) const;

	//		IReplaceChildExpression implementation
	virtual void replaceChildExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

	//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()

private:
	std::string m_name;
	ReprisePtr<ExpressionBase> m_expression;
};

/**	Fortran intrinsic call expression class.
	See Fortran ISO 3.2.3
*/
class HirFIntrinsicCallExpression : public CallExpressionBase
{
public:
	enum IntrinsicKind
	{
		IK__NONE = 0,
		IK__FIRST = 1,

		//	Operators (3.2.3)
		//	power-op
		IK_POW = IK__FIRST,

		//	mult-op
		IK_MUL,
		IK_DIV,

		//	add-op
		IK_PLUS,
		IK_MINUS,

		//	concat-op
		IK_CONCAT,

		//	rel-op
		IK_EQ,
		IK_NE,
		IK_LT,
		IK_LE,
		IK_GT,
		IK_GE,

		//	not-op
		IK_OP_NOT,

		//	and-op
		IK_AND,
		//	or-op
		IK_OR,
		//	equiv-op
		IK_EQV,
		IK_NEQV,

		//	6.2 Arrays
			// Array access expressions:
		IK_SECTION_TRIPLET, // 1:10:1 or 1:10
		IK_SECTION_VECTOR,  // / 1, 2, 3, 4 / 
		
		IK_ARRAY_CTOR,		

		//	6.3 Dynamic association
		IK_ALLOCATE,
		IK_NULLIFY,
		IK_DEALLOCATE,

		IK_EQUIVALENCE,

		// 8
		IK_STOP,

		//   9 Input/output statements
		IK_OPEN,
		IK_CLOSE,
		IK_PRINT,
		IK_READ,
		IK_WRITE,

		//	10.1.1 FORMAT statement
		IK_FORMAT,

		// 12 Procedures
		IK_RETURN,		

		//	Generic intrinsic functions (13.11)
		//	13.11.1 Argument presence inquiry function
		IK_PRESENT,			//	PRESENT (A) Argument presence

		//	13.11.2 Numeric functions
		IK_ABS,				//	ABS (A) Absolute value
		IK_CABS,
		IK_DABS,
		IK_IABS,
		IK_AIMAG,			//	AIMAG (Z) Imaginary part of a complex number
		IK_AINT,			//	AINT (A [, KIND]) Truncation to whole number
		IK_DINT,
		IK_ANINT,			//	ANINT (A [, KIND]) Nearest whole number
		IK_DNINT,
		IK_CEILING,			//	CEILING (A [, KIND]) Least integer greater than or equal to number
		IK_CMPLX,			//	CMPLX (X [, Y, KIND]) Conversion to complex type
		IK_CONJG,			//	CONJG (Z) Conjugate of a complex number
		IK_DBLE,			//	DBLE (A) Conversion to double precision real type
		IK_DIM,				//	DIM (X, Y) Positive difference
		IK_DDIM,
		IK_IDIM,
		IK_DPROD,			//	DPROD (X, Y) Double precision real product
		IK_FLOOR,			//	FLOOR (A [, KIND]) Greatest integer less than or equal to number
		IK_INT,				//	INT (A [, KIND]) Conversion to integer type
		IK_IFIX,
		IK_IDINT,
		IK_MAX,				//	MAX (A1, A2 [, A3,...]) Maximum value
		IK_AMAX0,
		IK_AMAX1,
		IK_DMAX1,
		IK_MAX0,
		IK_MAX1,
		IK_MIN,				//	MIN (A1, A2 [, A3,...]) Minimum value
		IK_AMIN0,
		IK_AMIN1,
		IK_DMIN1,
		IK_MIN0,
		IK_MIN1,
		IK_MOD,				//	MOD (A, P) Remainder function
		IK_AMOD,
		IK_DMOD,
		IK_MODULO,			//	MODULO (A, P) Modulo function
		IK_NINT,			//	NINT (A [, KIND]) Nearest integer
		IK_IDNINT,
		IK_REAL,			//	REAL (A [, KIND]) Conversion to real type
		IK_FLOAT,
		IK_SNGL,
		IK_SIGN,			//	SIGN (A, B) Transfer of sign
		IK_DSIGN,
		IK_ISIGN,

		//	13.11.3 Mathematical functions
		IK_ACOS,			//	ACOS (X) Arccosine
		IK_DACOS,
		IK_ASIN,			//	ASIN (X) Arcsine
		IK_DASIN,
		IK_ATAN,			//	ATAN (X) Arctangent
		IK_DATAN,
		IK_ATAN2,			//	ATAN2 (Y, X) Arctangent
		IK_DATAN2,
		IK_COS,				//	COS (X) Cosine
		IK_CCOS,
		IK_DCOS,
		IK_COSH,			//	COSH (X) Hyperbolic cosine
		IK_DCOSH,
		IK_EXP,				//	EXP (X) Exponential
		IK_CEXP,
		IK_DEXP,
		IK_LOG,				//	LOG (X) Natural logarithm
		IK_ALOG,
		IK_CLOG,
		IK_DLOG,
		IK_LOG10,			//	LOG10 (X) Common logarithm (base 10)
		IK_ALOG10,
		IK_DLOG10,
		IK_SIN,				//	SIN (X) Sine
		IK_CSIN,
		IK_DSIN,
		IK_SINH,			//	SINH (X) Hyperbolic sine
		IK_DSINH,
		IK_SQRT,			//	SQRT (X) Square root
		IK_CSQRT,
		IK_DSQRT,
		IK_TAN,				//	TAN (X) Tangent
		IK_DTAN,
		IK_TANH,			//	TANH (X) Hyperbolic tangent
		IK_DTANH,

		//	13.11.4 Character functions
		IK_ACHAR,			//	ACHAR (I) Character in given position in ASCII collating sequence
		IK_ADJUSTL,			//	ADJUSTL (STRING) Adjust left
		IK_ADJUSTR,			//	ADJUSTR (STRING) Adjust right
		IK_CHAR,			//	CHAR (I [, KIND]) Character in given position in processor collating sequence
		IK_IACHAR,			//	IACHAR (C) Position of a character in ASCII collating sequence
		IK_ICHAR,			//	ICHAR (C) Position of a character in processor collating sequence
		IK_INDEX,			//	INDEX (STRING, SUBSTRING [, BACK]) Starting position of a substring
		IK_LEN_TRIM,		//	LEN_TRIM (STRING) Length without trailing blank characters
		IK_LGE,				//	LGE (STRING_A, STRING_B) Lexically greater than or equal
		IK_LGT,				//	LGT (STRING_A, STRING_B) Lexically greater than
		IK_LLE,				//	LLE (STRING_A, STRING_B) Lexically less than or equal
		IK_LLT,				//	LLT (STRING_A, STRING_B) Lexically less than
		IK_REPEAT,			//	REPEAT (STRING, NCOPIES) Repeated concatenation
		IK_SCAN,			//	SCAN (STRING, SET [, BACK]) Scan a string for a character in a set
		IK_TRIM,			//	TRIM (STRING) Remove trailing blank characters
		IK_VERIFY,			//	VERIFY (STRING, SET [, BACK]) Verify the set of characters in a string

		//	13.11.5 Character inquiry function
		IK_LEN,				//	LEN (STRING) Length of a character entity

		//	13.11.6 Kind functions
		IK_KIND,				//	KIND (X) Kind type parameter value
		IK_SELECTED_INT_KIND,	//	SELECTED_INT_KIND (R) Integer kind type parameter value, given range
		IK_SELECTED_REAL_KIND,	//	SELECTED_REAL_KIND ([P, R]) Real kind type parameter value, given precision and range

		//	13.11.7 Logical function
		IK_LOGICAL,			//	LOGICAL (L [, KIND]) Convert between objects of type logical with different kind type parameters

		//	13.11.8 Numeric inquiry functions
		IK_DIGITS,			//	DIGITS (X) Number of significant digits of the model
		IK_EPSILON,			//	EPSILON (X) Number that is almost negligible compared to one
		IK_HUGE,			//	HUGE (X) Largest number of the model
		IK_MAXEXPONENT,		//	MAXEXPONENT (X) Maximum exponent of the model
		IK_MINEXPONENT,		//	MINEXPONENT (X) Minimum exponent of the model
		IK_PRECISION,		//	PRECISION (X) Decimal precision
		IK_RADIX,			//	RADIX (X) Base of the model
		IK_RANGE,			//	RANGE (X) Decimal exponent range
		IK_TINY,			//	TINY (X) Smallest positive number of the model

		//	13.11.9 Bit inquiry function
		IK_BIT_SIZE,		//	BIT_SIZE (I) Number of bits of the model

		//	13.11.10 Bit manipulation functions
		IK_BTEST,			//	BTEST (I, POS) Bit testing
		IK_IAND,			//	IAND (I, J) Logical AND
		IK_IBCLR,			//	IBCLR (I, POS) Clear bit
		IK_IBITS,			//	IBITS (I, POS, LEN) Bit extraction
		IK_IBSET,			//	IBSET (I, POS) Set bit
		IK_IEOR,			//	IEOR (I, J) Exclusive OR
		IK_IOR,				//	IOR (I, J) Inclusive OR
		IK_ISHFT,			//	ISHFT (I, SHIFT) Logical shift
		IK_ISHFTC,			//	ISHFTC (I, SHIFT [, SIZE]) Circular shift
		IK_NOT,				//	NOT (I) Logical complement

		//	13.11.11 Transfer function
		IK_TRANSFER,		//	TRANSFER (SOURCE, MOLD [, SIZE]) Treat first argument as if of type of second argument

		//	13.11.12 Floating-point manipulation functions
		IK_EXPONENT,		//	EXPONENT (X) Exponent part of a model number
		IK_FRACTION,		//	FRACTION (X) Fractional part of a number
		IK_NEAREST,			//	NEAREST (X, S) Nearest different processor number in given direction
		IK_RRSPACING,		//	RRSPACING (X) Reciprocal of the relative spacing of model numbers near given number
		IK_SCALE,			//	SCALE (X, I) Multiply a real by its base to an integer power
		IK_SET_EXPONENT,	//	SET_EXPONENT (X, I) Set exponent part of a number
		IK_SPACING,			//	SPACING (X) Absolute spacing of model numbers near given number

		//	13.11.13 Vector and matrix multiply functions
		IK_DOT_PRODUCT,		//	DOT_PRODUCT (VECTOR_A, VECTOR_B) Dot product of two rank-one arrays
		IK_MATMUL,			//	MATMUL (MATRIX_A, MATRIX_B) Matrix multiplication

		//	13.11.14 Array reduction functions
		IK_ALL,				//	ALL (MASK [, DIM]) True if all values are true
		IK_ANY,				//	ANY (MASK [, DIM]) True if any value is true
		IK_COUNT,			//	COUNT (MASK [, DIM]) Number of true elements in an array
		IK_MAXVAL_DIM,		//	MAXVAL (ARRAY, DIM [, MASK])
		IK_MAXVAL,			//	or MAXVAL (ARRAY [, MASK]) Maximum value in an array
		IK_MINVAL_DIM,		//	MINVAL (ARRAY, DIM [, MASK])
		IK_MINVAL,			//	or MINVAL (ARRAY [, MASK]) Minimum value in an array
		IK_PRODUCT_DIM,		//	PRODUCT (ARRAY, DIM [, MASK])
		IK_PRODUCT,			//	or PRODUCT (ARRAY [, MASK]) Product of array elements
		IK_SUM_DIM,			//	SUM (ARRAY, DIM [, MASK])
		IK_SUM,				//	or SUM (ARRAY [, MASK]) Sum of array elements

		//	13.11.15 Array inquiry functions
		IK_ALLOCATED,		//	ALLOCATED (ARRAY) Array allocation status
		IK_LBOUND,			//	LBOUND (ARRAY [, DIM]) Lower dimension bounds of an array
		IK_SHAPE,			//	SHAPE (SOURCE) Shape of an array or scalar
		IK_SIZE,			//	SIZE (ARRAY [, DIM]) Total number of elements in an array
		IK_UBOUND,			//	UBOUND (ARRAY [, DIM]) Upper dimension bounds of an array

		//	13.11.16 Array construction functions
		IK_MERGE,			//	MERGE (TSOURCE, FSOURCE, MASK) Merge under mask
		IK_PACK,			//	PACK (ARRAY, MASK [, VECTOR]) Pack an array into an array of rank one under a mask
		IK_SPREAD,			//	SPREAD (SOURCE, DIM, NCOPIES) Replicates array by adding a dimension
		IK_UNPACK,			//	UNPACK (VECTOR, MASK, FIELD) Unpack an array of rank one into an array under a mask

		//	13.11.17 Array reshape function
		IK_RESHAPE,			//	RESHAPE (SOURCE, SHAPE[, PAD, ORDER]) Reshape an array

		//	13.11.18 Array manipulation functions
		IK_CSHIFT,			//	CSHIFT (ARRAY, SHIFT [, DIM]) Circular shift
		IK_EOSHIFT,			//	EOSHIFT (ARRAY, SHIFT [, BOUNDARY, DIM]) End-off shift
		IK_TRANSPOSE,		//	TRANSPOSE (MATRIX) Transpose of an array of rank two

		//	13.11.19 Array location functions
		IK_MAXLOC_DIM,		//	MAXLOC (ARRAY, DIM [, MASK])
		IK_MAXLOC,			//	or MAXLOC (ARRAY [, MASK]) Location of a maximum value in an array
		IK_MINLOC_DIM,		//	MINLOC (ARRAY, DIM [, MASK])
		IK_MINLOC,			//	or MINLOC (ARRAY [, MASK]) Location of a minimum value in an array

		//	13.11.20 Pointer association status functions
		IK_ASSOCIATED,		//	ASSOCIATED (POINTER [, TARGET]) Association status inquiry or comparison
		IK_NULL,			//	NULL ([MOLD]) Returns disassociated pointer

		//	13.12 Intrinsic subroutines
		IK_CPU_TIME,		//	CPU_TIME (TIME) Obtain processor time
		IK_DATE_AND_TIME,	//	DATE_AND_TIME ([DATE, TIME, ZONE, VALUES]) Obtain date and time
		IK_MVBITS,			//	MVBITS (FROM, FROMPOS, LEN, TO, TOPOS) Copies bits from one integer to another
		IK_RANDOM_NUMBER,	//	RANDOM_NUMBER (HARVEST) Returns pseudorandom number
		IK_RANDOM_SEED,		//	RANDOM_SEED ([SIZE, PUT, GET]) Initializes or restarts the pseudorandom number generator
		IK_SYSTEM_CLOCK,	//	SYSTEM_CLOCK ([COUNT, COUNT_RATE, COUNT_MAX]) Obtain data from the system clock
		IK__LAST,
	};

//	Static methods
	static std::string intrinsicKindToString(IntrinsicKind kind);

	explicit HirFIntrinsicCallExpression(IntrinsicKind kind);
	HirFIntrinsicCallExpression(IntrinsicKind kind, ExpressionBase* argument);
	HirFIntrinsicCallExpression(IntrinsicKind kind, ExpressionBase* leftArg, ExpressionBase* rightArg);

	IntrinsicKind getKind(void) const;
	void setKind(IntrinsicKind kind);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		ClonableMix implementation
	virtual HirFIntrinsicCallExpression* clone(void) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
protected:
	static std::string dumpKindFormat(IntrinsicKind kind);

private:
	IntrinsicKind m_kind;
};


class HirFIntrinsicReferenceExpression : public ExpressionBase
{
public:
	explicit HirFIntrinsicReferenceExpression(HirFIntrinsicCallExpression::IntrinsicKind kind);

	HirFIntrinsicCallExpression::IntrinsicKind getKind(void) const;

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		ClonableMix implementation
	virtual HirFIntrinsicReferenceExpression* clone(void) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
private:
	HirFIntrinsicCallExpression::IntrinsicKind m_kind;
};

}
}
}

#endif
