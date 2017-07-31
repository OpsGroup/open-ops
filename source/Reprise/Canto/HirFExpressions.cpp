#include "Reprise/Canto/HirFExpressions.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{

using namespace OPS::Reprise;
using namespace std;

HirFAltResultExpression::HirFAltResultExpression(int altResult)
{
	m_altResult = altResult;
}

int HirFAltResultExpression::getAlternativeResult() const
{
	return m_altResult;
}

bool HirFAltResultExpression::isEqual(const ExpressionBase& exprNode) const		
{
	return (exprNode.is_a<HirFAltResultExpression>() &&
		exprNode.cast_to<HirFAltResultExpression>().getAlternativeResult() == getAlternativeResult());
}

HirFAltResultExpression* HirFAltResultExpression::clone(void) const
{
	return new HirFAltResultExpression(getAlternativeResult());
}

int HirFAltResultExpression::getChildCount(void) const
{
	return 0;
}
RepriseBase& HirFAltResultExpression::getChild(int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("HirFAltResultExpression::getChild");
}

std::string HirFAltResultExpression::dumpState(void) const
{
	return OPS::Strings::format("RETURN TO %d", getAlternativeResult());
}


bool HirFAsteriskExpression::isEqual(const ExpressionBase& exprNode) const		
{
	return exprNode.is_a<const HirFAsteriskExpression>();
}

HirFAsteriskExpression* HirFAsteriskExpression::clone(void) const
{
	return new HirFAsteriskExpression();
}

int HirFAsteriskExpression::getChildCount(void) const
{
	return 0;
}
RepriseBase& HirFAsteriskExpression::getChild(int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("HirFAsteriskExpression::getChild");
}

std::string HirFAsteriskExpression::dumpState(void) const
{
	return "*";
}

/**
 *
 *
 * HirFDimensionExpression
 *
 *
 */
HirFDimensionExpression::HirFDimensionExpression() 
	: m_lowerBound(new EmptyExpression()), m_upperBound(new EmptyExpression()) 
{
	m_lowerBound->setParent(this);
	m_upperBound->setParent(this);
}

HirFDimensionExpression::HirFDimensionExpression(ExpressionBase *lowerBound, ExpressionBase *upperBound)
{
	setLowerBound(lowerBound);
	setUpperBound(upperBound);
}

ExpressionBase& HirFDimensionExpression::getLowerBound()
{
	return *m_lowerBound;
}

const ExpressionBase&  HirFDimensionExpression::getLowerBound() const
{
	return *m_lowerBound;
}

void HirFDimensionExpression::setLowerBound(ExpressionBase *lowerBound)
{
	this->m_lowerBound.reset(lowerBound);
	this->m_lowerBound->setParent(this);
}	

ExpressionBase& HirFDimensionExpression::getUpperBound()
{
	return *m_upperBound;
}

const ExpressionBase&  HirFDimensionExpression::getUpperBound() const
{
	return *m_upperBound;
}

void HirFDimensionExpression::setUpperBound(ExpressionBase *upperBound)
{
	this->m_upperBound.reset(upperBound);
	this->m_upperBound->setParent(this);
}

// ExpressionBase implementation
bool HirFDimensionExpression::isEqual(const ExpressionBase& exprNode) const		
{
	if (const HirFDimensionExpression* expr = dynamic_cast<const HirFDimensionExpression*>(&exprNode))
		return m_lowerBound->isEqual(expr->getLowerBound()) && m_upperBound->isEqual(expr->getUpperBound());
	return false;
}

// ClonableMix implementation
HirFDimensionExpression* HirFDimensionExpression::clone(void) const
{
	HirFDimensionExpression *result = new HirFDimensionExpression(
		m_lowerBound->clone(), m_upperBound->clone());
	return result;
}

void HirFDimensionExpression::replaceChildExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (&source == m_lowerBound.get())
	{
		m_lowerBound = destination;
		m_lowerBound->setParent(this);
	}
	if (&source == m_upperBound.get())
	{
		m_upperBound = destination;
		m_upperBound->setParent(this);
	}
}

// RepriseBase implementation
int HirFDimensionExpression::getChildCount(void) const
{
	return 2;
}

RepriseBase& HirFDimensionExpression::getChild(int index)
{
	switch(index)
	{
	case 0: return *m_lowerBound;
	case 1: return *m_upperBound;
	OPS_DEFAULT_CASE_LABEL;
	}
	throw UnexpectedChildError("HirFDimensionExpression::getChild");
}

std::string HirFDimensionExpression::dumpState(void) const
{
	std::string state = "(" + m_lowerBound->dumpState() + ":" + m_upperBound->dumpState() + ")";	
	return state;	
}


/**
 *
 *
 * HirFArrayShapeExpression
 *
 *
 */
HirFArrayShapeExpression::HirFArrayShapeExpression()
{
}

HirFArrayShapeExpression::ShapeType HirFArrayShapeExpression::getShapeType(HirFArrayShapeExpression& shape)
{
	OPS_UNUSED(shape);
	throw RepriseError("HirFArrayShapeExpression::getShape is not supported yet.");
}

//		IReplaceChildExpression implementation
void HirFArrayShapeExpression::replaceChildExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (destination->is_a<HirFDimensionExpression>())
	{
		for (TDimensionList::Iterator it = m_dimensionList.begin(); it != m_dimensionList.end(); ++it)
		{
			if (*it == &source)
			{
				m_dimensionList.replace(it, destination.get()->cast_ptr<HirFDimensionExpression>());
				(*it)->setParent(this);
			}
		}
	}
}

// ExpressionBase implementation
bool HirFArrayShapeExpression::isEqual(const ExpressionBase&  expression) const		
{	
	if (!expression.is_a<HirFArrayShapeExpression>())	
		return false;

	const HirFArrayShapeExpression& arrayShapeExpression = expression.cast_to<HirFArrayShapeExpression>();
	if (arrayShapeExpression.getRank() != this->getRank())
		return false;

	for (int i = 0; i < this->getRank(); i++)
		if (!this->getDimension(i).isEqual(arrayShapeExpression.getDimension(i))) return false;
	
	return true;
}

// ClonableMix implementation
HirFArrayShapeExpression* HirFArrayShapeExpression::clone(void) const
{
	HirFArrayShapeExpression* result = new HirFArrayShapeExpression();	
	for (int i = 0; i < this->getRank(); i++)
		result->addDimension(this->getDimension(i).clone());
	return result;
}

// RepriseBase implementation
int HirFArrayShapeExpression::getChildCount(void) const
{
	return this->getRank();
}

RepriseBase& HirFArrayShapeExpression::getChild(int index)
{
	return this->getDimension(index);
}

std::string HirFArrayShapeExpression::dumpState(void) const
{	
	std::string state;
	
	state += "[";	
	for (int i = 0; i < getRank(); i++)
	{
		state += getDimension(i).dumpState();
		if (i != getRank() - 1)
			state += ", ";
	}
	state += "]";	

	return state;	
}

/**
*
*
* HirFArrayIndexRangeExpression
*
*
*/
HirFArrayIndexRangeExpression::HirFArrayIndexRangeExpression()
{
}

// ExpressionBase implementation
bool HirFArrayIndexRangeExpression::isEqual(const ExpressionBase&  expression) const		
{	
	if (!expression.is_a<HirFArrayIndexRangeExpression>())	
		return false;

	const HirFArrayIndexRangeExpression& indexRangeExpression = expression.cast_to<HirFArrayIndexRangeExpression>();
	if (indexRangeExpression.getArgumentCount() != getArgumentCount())
		return false;

	for (int i = 0; i < this->getArgumentCount(); i++)
		if (!this->getArgument(i).isEqual(indexRangeExpression.getArgument(i))) return false;

	return true;
}

// ClonableMix implementation
HirFArrayIndexRangeExpression* HirFArrayIndexRangeExpression::clone(void) const
{
	HirFArrayIndexRangeExpression* result = new HirFArrayIndexRangeExpression();	
	for (int i = 0; i < this->getArgumentCount(); i++)
		result->addArgument(this->getArgument(i).clone());
	return result;
}

// RepriseBase implementation
int HirFArrayIndexRangeExpression::getChildCount(void) const
{
	return this->getArgumentCount();
}

RepriseBase& HirFArrayIndexRangeExpression::getChild(int index)
{
	return this->getArgument(index);
}

std::string HirFArrayIndexRangeExpression::dumpState(void) const
{	
	std::string state;

	state += "(";	
	for (int i = 0; i < getArgumentCount(); i++)
	{
		state += getArgument(i).dumpState();
		if (i != getArgumentCount() - 1)
			state += ", ";
	}
	state += ")";	

	return state;	
}


/**
 *
 *
 * HirFImpliedDoExpression
 *
 *
 */
HirFImpliedDoExpression::HirFImpliedDoExpression(ExpressionBase* initExpression, 
	ExpressionBase* finalExpression, 
	ExpressionBase* stepExpression,
	ExpressionBase* bodyExpression)	
{
	setInitExpression(initExpression);
	setFinalExpression(finalExpression);
	setStepExpression(stepExpression);	
	setBodyExpression(bodyExpression);
}

const ExpressionBase& HirFImpliedDoExpression::getInitExpression(void) const
{
	return *m_initExpression;
}

ExpressionBase& HirFImpliedDoExpression::getInitExpression(void)
{
	return *m_initExpression;
}

void HirFImpliedDoExpression::setInitExpression(ExpressionBase* initExpression)
{
	m_initExpression.reset(initExpression);
	m_initExpression->setParent(this);	
}

const ExpressionBase& HirFImpliedDoExpression::getFinalExpression(void) const
{
	return *m_finalExpression;
}

ExpressionBase& HirFImpliedDoExpression::getFinalExpression(void)
{
	return *m_finalExpression;
}

void HirFImpliedDoExpression::setFinalExpression(ExpressionBase* finalExpression)
{
	m_finalExpression.reset(finalExpression);
	m_finalExpression->setParent(this);
}


const ExpressionBase& HirFImpliedDoExpression::getStepExpression(void) const
{
	return *m_stepExpression;
}

ExpressionBase& HirFImpliedDoExpression::getStepExpression(void)
{
	return *m_stepExpression;
}

void HirFImpliedDoExpression::setStepExpression(ExpressionBase* stepExpression)
{
	m_stepExpression.reset(stepExpression);
	m_stepExpression->setParent(this);
}

const ExpressionBase& HirFImpliedDoExpression::getBodyExpression(void) const
{
	return *m_bodyExpression;
}

ExpressionBase& HirFImpliedDoExpression::getBodyExpression(void)
{
	return *m_bodyExpression;
}

void HirFImpliedDoExpression::setBodyExpression(ExpressionBase* bodyExpression)
{
	m_bodyExpression.reset(bodyExpression);
	m_bodyExpression->setParent(this);
}

bool HirFImpliedDoExpression::isEqual(const ExpressionBase& exprNode) const
{
	OPS_UNUSED(exprNode)
	throw RepriseError("HirFImpliedDoExpression::isEqual is not supported yet.");

}

HirFImpliedDoExpression* HirFImpliedDoExpression::clone(void) const
{
	return new HirFImpliedDoExpression(m_initExpression->clone(),
										m_finalExpression->clone(),
										m_stepExpression->clone(),
										m_bodyExpression->clone());
}

//		IReplaceChildExpression implementation
void HirFImpliedDoExpression::replaceChildExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (&source == m_initExpression.get())
	{
		m_initExpression = destination;
		m_initExpression->setParent(this);
	} 
	else if (&source == m_finalExpression.get())
	{
		m_finalExpression = destination;
		m_finalExpression->setParent(this);
	} 
	else if (&source == m_stepExpression.get())
	{
		m_stepExpression = destination;
		m_stepExpression->setParent(this);
	} 
	else if (&source == m_bodyExpression.get())
	{
		m_bodyExpression = destination;
		m_bodyExpression->setParent(this);
	} 
}

int HirFImpliedDoExpression::getChildCount(void) const
{
	return 4;
}

RepriseBase& HirFImpliedDoExpression::getChild(int index)
{
	switch(index)
	{
	case 0:
		return *m_bodyExpression;
	case 1:
		return *m_initExpression;
	case 2:
		return *m_finalExpression;
	case 3:
		return *m_stepExpression;
		OPS_DEFAULT_CASE_LABEL
	}
	throw UnexpectedChildError("HirFImpliedDoExpression::getChild");
}

std::string HirFImpliedDoExpression::dumpState(void) const
{
	return Strings::format("(%s, %s, %s, %s)"
		,m_bodyExpression->dumpState().c_str()
		,m_initExpression->dumpState().c_str()
		,m_finalExpression->dumpState().c_str()
		,m_stepExpression->dumpState().c_str());
}

/**
 *
 * 
 * HirFArgumentPairExpression
 *
 *
 */

HirFArgumentPairExpression::HirFArgumentPairExpression(std::string name, ExpressionBase* expression)
	: m_name(name), m_expression(expression)
{
	expression->setParent(this);
}

const string& HirFArgumentPairExpression::getName() const
{
	return m_name;	
}

std::string& HirFArgumentPairExpression::getName()
{
	return m_name;
}

void HirFArgumentPairExpression::setName(std::string name)
{
	m_name = name;
}

const ExpressionBase& HirFArgumentPairExpression::getValue() const
{
	return *m_expression;
}

ExpressionBase& HirFArgumentPairExpression::getValue()
{
	return *m_expression;
}

void HirFArgumentPairExpression::setValue(ExpressionBase* value)
{
	m_expression.reset(value);
	m_expression->setParent(this);
}

bool HirFArgumentPairExpression::isEqual(const ExpressionBase& expression) const
{
	if (!expression.is_a<HirFArgumentPairExpression>())	
		return false;

	const HirFArgumentPairExpression& argPairExpression = expression.cast_to<HirFArgumentPairExpression>();	
	return argPairExpression.getName() != this->getName() || 
		!argPairExpression.getValue().isEqual(this->getValue());	
}

HirFArgumentPairExpression* HirFArgumentPairExpression::clone(void) const
{
	return new HirFArgumentPairExpression(m_name, m_expression->clone());
}

//		IReplaceChildExpression implementation
void HirFArgumentPairExpression::replaceChildExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (&source == m_expression.get())
	{
		m_expression = destination;
		m_expression->setParent(this);
	}
}

int HirFArgumentPairExpression::getChildCount(void) const
{
	return 1;
}

RepriseBase& HirFArgumentPairExpression::getChild(int index)
{
	if (index == 0)
		return *m_expression;
	else
		throw UnexpectedChildError("HirFArgumentPairExpression::getChild");
}

std::string HirFArgumentPairExpression::dumpState(void) const
{
	return Strings::format("%s = %s", getName().c_str(), getValue().dumpState().c_str());	
}


/**
 *
 * 
 * HirFIntrinsicCallExpression
 *
 *
 */
HirFIntrinsicCallExpression::HirFIntrinsicCallExpression(const IntrinsicKind kind) : m_kind(kind)
{
}

HirFIntrinsicCallExpression::HirFIntrinsicCallExpression(const IntrinsicKind kind, ExpressionBase* const argument) : m_kind(kind)
{
	addArgument(argument);
}

HirFIntrinsicCallExpression::HirFIntrinsicCallExpression(const IntrinsicKind kind, ExpressionBase* const leftArg, ExpressionBase* const rightArg) : m_kind(kind)
{
	addArgument(leftArg);
	addArgument(rightArg);
}

HirFIntrinsicCallExpression::IntrinsicKind HirFIntrinsicCallExpression::getKind(void) const
{
	return m_kind;
}

void HirFIntrinsicCallExpression::setKind(IntrinsicKind kind)
{
	m_kind = kind;
}

//		ExpressionBase implementation
bool HirFIntrinsicCallExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<HirFIntrinsicCallExpression>())
	{
		const HirFIntrinsicCallExpression& other = exprNode.cast_to<HirFIntrinsicCallExpression>();
		if (other.m_kind == m_kind && CallExpressionBase::isEqual(other))
		{
			return true;
		}
	}
	return false;
}

//		ClonableMix implementation
HirFIntrinsicCallExpression* HirFIntrinsicCallExpression::clone(void) const
{
	HirFIntrinsicCallExpression* callExpression = new HirFIntrinsicCallExpression(m_kind);
	for (ArgumentsType::const_iterator iter = m_arguments.begin(); iter != m_arguments.end(); ++iter)
	{
		callExpression->addArgument((*iter)->clone());
	}
	callExpression->acquireNotes(*this);
	return callExpression;
}

//		RepriseBase implementation
int HirFIntrinsicCallExpression::getChildCount(void) const
{
	return getArgumentCount();
}

RepriseBase& HirFIntrinsicCallExpression::getChild(int index)
{
	return getArgument(index);
}

std::string HirFIntrinsicCallExpression::dumpState(void) const
{
	std::string state = CallExpressionBase::dumpState();
	state += intrinsicKindToString(m_kind);
	state += "(";

	for (int i = 0; i < getArgumentCount(); i++)
	{
		if (i > 0) state += ", ";
		state += getArgument(i).dumpState();
	}

	state += ")";
	return state;
}

//	
std::string HirFIntrinsicCallExpression::intrinsicKindToString(const IntrinsicKind kind)
{
	switch (kind)
	{
	case HirFIntrinsicCallExpression::IK_POW:
		return "**";
	case HirFIntrinsicCallExpression::IK_MUL:
		return "MUL";
	case HirFIntrinsicCallExpression::IK_DIV:
		return "DIV";
	case HirFIntrinsicCallExpression::IK_PLUS:
		return "PLUS";
	case HirFIntrinsicCallExpression::IK_MINUS:
		return "MINUS";
	case HirFIntrinsicCallExpression::IK_CONCAT:
		return "//";
	case HirFIntrinsicCallExpression::IK_EQ:
		return "==";
	case HirFIntrinsicCallExpression::IK_NE:
		return "!=";
	case HirFIntrinsicCallExpression::IK_LT:
		return "<";
	case HirFIntrinsicCallExpression::IK_LE:
		return "<=";
	case HirFIntrinsicCallExpression::IK_GT:
		return ">";
	case HirFIntrinsicCallExpression::IK_GE:
		return ">=";

	case HirFIntrinsicCallExpression::IK_OP_NOT:
		return "NOT";
	case HirFIntrinsicCallExpression::IK_AND:
		return "AND";
	case HirFIntrinsicCallExpression::IK_OR:
		return "OR";
	case HirFIntrinsicCallExpression::IK_EQV:
		return "EQV"; 
	case HirFIntrinsicCallExpression::IK_NEQV:
		return "NEQV";
	case HirFIntrinsicCallExpression::IK_SECTION_TRIPLET:
		return "SECTION_TRIPLET";
	case HirFIntrinsicCallExpression::IK_SECTION_VECTOR:
		return "SECTION_VECTOR";
	case HirFIntrinsicCallExpression::IK_ARRAY_CTOR:
		return "ARRAY_CTOR";
	case HirFIntrinsicCallExpression::IK_ALLOCATE:
		return "ALLOCATE";
	case HirFIntrinsicCallExpression::IK_NULLIFY:
		return "NULLIFY";
	case HirFIntrinsicCallExpression::IK_DEALLOCATE:
		return "DEALLOCATE";
	case HirFIntrinsicCallExpression::IK_EQUIVALENCE:
		return "EQUIVALENCE";
	case HirFIntrinsicCallExpression::IK_STOP:
		return "STOP";
	case HirFIntrinsicCallExpression::IK_OPEN:
		return "OPEN";
	case HirFIntrinsicCallExpression::IK_CLOSE:
		return "CLOSE";
	case HirFIntrinsicCallExpression::IK_PRINT:
		return "PRINT";
	case HirFIntrinsicCallExpression::IK_READ:
		return "READ";
	case HirFIntrinsicCallExpression::IK_WRITE:
		return "WRITE";
	case HirFIntrinsicCallExpression::IK_FORMAT:
		return "FORMAT";

// 12 Procedures
	case HirFIntrinsicCallExpression::IK_RETURN:
		return "RETURN";

//	Generic intrinsic functions (13.11)
//	13.11.1 Argument presence inquiry function
	case HirFIntrinsicCallExpression::IK_PRESENT: //	PRESENT (A) Argument presence
		return "PRESENT";
//	13.11.2 Numeric functions
	case HirFIntrinsicCallExpression::IK_ABS:				//	ABS (A) Absolute value
		return "ABS";
	case IK_CABS: return "CABS";
	case IK_DABS: return "DABS";
	case IK_IABS: return "IABS";
	case HirFIntrinsicCallExpression::IK_AIMAG:			//	AIMAG (Z) Imaginary part of a complex number
		return "AIMAG";
	case HirFIntrinsicCallExpression::IK_ANINT:			//	ANINT (A [, KIND]) Nearest whole number
		return "ANINT";
	case IK_DINT: return "DINT";
	case IK_DNINT: return "DNINT";
	case HirFIntrinsicCallExpression::IK_CEILING:			//	CEILING (A [, KIND]) Least integer greater than or equal to number
		return "CEILING";
	case HirFIntrinsicCallExpression::IK_CMPLX:			//	CMPLX (X [, Y, KIND]) Conversion to complex type
		return "CMPLX";
	case HirFIntrinsicCallExpression::IK_CONJG:			//	CONJG (Z) Conjugate of a complex number
		return "CONJG";
	case HirFIntrinsicCallExpression::IK_DBLE:				//	DBLE (A) Conversion to double precision real type
		return "DBLE";
	case HirFIntrinsicCallExpression::IK_DIM:				//	DIM (X, Y) Positive difference
		return "DIM";
	case IK_DDIM: return "DDIM";
	case IK_IDIM: return "IDIM";
	case HirFIntrinsicCallExpression::IK_DPROD:			//	DPROD (X, Y) Double precision real product
		return "DPROD";
	case HirFIntrinsicCallExpression::IK_FLOOR:			//	FLOOR (A [, KIND]) Greatest integer less than or equal to number
		return "FLOOR";
	case HirFIntrinsicCallExpression::IK_INT:				//	INT (A [, KIND]) Conversion to integer type
		return "INT";
	case IK_IFIX: return "IFIX";
	case IK_IDINT: return "IDINT";
	case HirFIntrinsicCallExpression::IK_MAX:				//	MAX (A1, A2 [, A3,...]) Maximum value
		return "MAX";
	case IK_AMAX0: return "AMAX0";
	case IK_AMAX1: return "AMAX1";
	case IK_DMAX1: return "DMAX1";
	case IK_MAX0: return "MAX0";
	case IK_MAX1: return "MAX1";
	case HirFIntrinsicCallExpression::IK_MIN:				//	MIN (A1, A2 [, A3,...]) Minimum value
		return "MIN";
	case IK_AMIN0: return "AMIN0";
	case IK_AMIN1: return "AMIN1";
	case IK_DMIN1: return "DMIN1";
	case IK_MIN0: return "MIN0";
	case IK_MIN1: return "MIN1";
	case HirFIntrinsicCallExpression::IK_MOD:				//	MOD (A, P) Remainder function
		return "MOD";
	case IK_AMOD: return "AMOD";
	case IK_DMOD: return "DMOD";
	case HirFIntrinsicCallExpression::IK_MODULO:			//	MODULO (A, P) Modulo function
		return "MODULO";
	case HirFIntrinsicCallExpression::IK_NINT:				//	NINT (A [, KIND]) Nearest integer
		return "NINT";	
	case IK_IDNINT: return "IDNINT";
	case HirFIntrinsicCallExpression::IK_REAL:				//	REAL (A [, KIND]) Conversion to real type
		return "REAL";
	case IK_FLOAT: return "FLOAT";
	case IK_SNGL: return "SNGL";
	case HirFIntrinsicCallExpression::IK_SIGN:				//	SIGN (A, B) Transfer of sign
		return "SIGN";
	case IK_DSIGN: return "DSIGN";
	case IK_ISIGN: return "ISIGN";

//	13.11.3 Mathematical functions
	case HirFIntrinsicCallExpression::IK_ACOS:				//	ACOS (X) Arccosine
		return "ACOS";
	case IK_DACOS: return "DACOS";
	case HirFIntrinsicCallExpression::IK_ASIN:				//	ASIN (X) Arcsine
		return "ASIN";
	case IK_DASIN: return "DASIN";
	case HirFIntrinsicCallExpression::IK_ATAN:				//	ATAN (X) Arctangent
		return "ATAN";
	case IK_DATAN: return "DATAN";
	case HirFIntrinsicCallExpression::IK_ATAN2:			//	ATAN2 (Y, X) Arctangent
		return "ATAN2";
	case IK_DATAN2: return "DATAN2";
	case HirFIntrinsicCallExpression::IK_COS:				//	COS (X) Cosine
		return "COS";
	case IK_CCOS: return "CCOS";
	case IK_DCOS: return "DCOS";
	case HirFIntrinsicCallExpression::IK_COSH:				//	COSH (X) Hyperbolic cosine
		return "COSH";
	case IK_DCOSH: return "DCOSH";
	case HirFIntrinsicCallExpression::IK_EXP:				//	EXP (X) Exponential
		return "EXP";
	case IK_CEXP: return "CEXP";
	case IK_DEXP: return "DEXP";
	case HirFIntrinsicCallExpression::IK_LOG:				//	LOG (X) Natural logarithm
		return "LOG";
	case IK_ALOG: return "ALOG";
	case IK_CLOG: return "CLOG";
	case IK_DLOG: return "DLOG";
	case HirFIntrinsicCallExpression::IK_LOG10:			//	LOG10 (X) Common logarithm (base 10)
		return "LOG10";
	case IK_ALOG10: return "ALOG10";
	case IK_DLOG10: return "DLOG10";
	case HirFIntrinsicCallExpression::IK_SIN:				//	SIN (X) Sine
		return "SIN";
	case IK_CSIN: return "CSIN";
	case IK_DSIN: return "DSIN";

	case HirFIntrinsicCallExpression::IK_SQRT:				//	SQRT (X) Square root
		return "SQRT";
	case IK_CSQRT: return "CSQRT";
	case IK_DSQRT: return "DSQRT";
	case HirFIntrinsicCallExpression::IK_TAN:				//	TAN (X) Tangent
		return "TAN";
	case IK_DTAN: return "DTAN";
	case HirFIntrinsicCallExpression::IK_TANH:				//	TANH (X) Hyperbolic tangent
		return "TAHN";
	case IK_DTANH: return "DTANH";

//	13.11.4 Character functions
	case HirFIntrinsicCallExpression::IK_ACHAR:			//	ACHAR (I) Character in given position in ASCII collating sequence
		return "ACHAR";
	case HirFIntrinsicCallExpression::IK_ADJUSTL:			//	ADJUSTL (STRING) Adjust left
		return "ADJUSTL";
	case HirFIntrinsicCallExpression::IK_ADJUSTR:			//	ADJUSTR (STRING) Adjust right
		return "ADJUSTR";
	case HirFIntrinsicCallExpression::IK_CHAR:				//	CHAR (I [, KIND]) Character in given position in processor collating sequence
		return "CHAR";	
	case HirFIntrinsicCallExpression::IK_IACHAR:			//	IACHAR (C) Position of a character in ASCII collating sequence
		return "IACHAR";
	case HirFIntrinsicCallExpression::IK_ICHAR:			//	ICHAR (C) Position of a character in processor collating sequence
		return "ICHAR";	
	case HirFIntrinsicCallExpression::IK_INDEX:			//	INDEX (STRING, SUBSTRING [, BACK]) Starting position of a substring
		return "INDEX";
	case HirFIntrinsicCallExpression::IK_LEN_TRIM:			//	LEN_TRIM (STRING) Length without trailing blank characters
		return "LEN_TRIM";	
	case HirFIntrinsicCallExpression::IK_LGE:				//	LGE (STRING_A, STRING_B) Lexically greater than or equal
		return "LGE";
	case HirFIntrinsicCallExpression::IK_LGT:				//	LGT (STRING_A, STRING_B) Lexically greater than
		return "LGT";	
	case HirFIntrinsicCallExpression::IK_LLE:				//	LLE (STRING_A, STRING_B) Lexically less than or equal
		return "LLE";
	case HirFIntrinsicCallExpression::IK_LLT:				//	LLT (STRING_A, STRING_B) Lexically less than
		return "LLT";
	case HirFIntrinsicCallExpression::IK_REPEAT:			//	REPEAT (STRING, NCOPIES) Repeated concatenation
		return "REPEAT";	
	case HirFIntrinsicCallExpression::IK_SCAN:				//	SCAN (STRING, SET [, BACK]) Scan a string for a character in a set
		return "SCAN";
	case HirFIntrinsicCallExpression::IK_TRIM:				//	TRIM (STRING) Remove trailing blank characters
		return "TRIM";	
	case HirFIntrinsicCallExpression::IK_VERIFY:			//	VERIFY (STRING, SET [, BACK]) Verify the set of characters in a string
		return "VERIFY";

//	13.11.5 Character inquiry function
	case HirFIntrinsicCallExpression::IK_LEN:				//	LEN (STRING) Length of a character entity
		return "LEN";

//	13.11.6 Kind functions
	case HirFIntrinsicCallExpression::IK_KIND:				//	KIND (X) Kind type parameter value
		return "KIND";
	case HirFIntrinsicCallExpression::IK_SELECTED_INT_KIND:	//	SELECTED_INT_KIND (R) Integer kind type parameter value, given range
		return "SELECTED_INT_KIND";	
	case HirFIntrinsicCallExpression::IK_SELECTED_REAL_KIND:	//	SELECTED_REAL_KIND ([P, R]) Real kind type parameter value, given precision and range
		return "SELECTED_REAL_KIND";	

//	13.11.7 Logical function
	case HirFIntrinsicCallExpression::IK_LOGICAL:			//	LOGICAL (L [, KIND]) Convert between objects of type logical with different kind type parameters
		return "ACHAR";
//	13.11.8 Numeric inquiry functions
	case HirFIntrinsicCallExpression::IK_DIGITS:			//	DIGITS (X) Number of significant digits of the model
		return "DIGITS";
	case HirFIntrinsicCallExpression::IK_EPSILON:			//	EPSILON (X) Number that is almost negligible compared to one
		return "EPSILON";
	case HirFIntrinsicCallExpression::IK_HUGE:				//	HUGE (X) Largest number of the model
		return "HUGE";
	case HirFIntrinsicCallExpression::IK_MAXEXPONENT:		//	MAXEXPONENT (X) Maximum exponent of the model
		return "MAXEXPONENT";
	case HirFIntrinsicCallExpression::IK_MINEXPONENT:		//	MINEXPONENT (X) Minimum exponent of the model
		return "MINEXPONENT";
	case HirFIntrinsicCallExpression::IK_PRECISION:		//	PRECISION (X) Decimal precision
		return "PRECISION";
	case HirFIntrinsicCallExpression::IK_RADIX:			//	RADIX (X) Base of the model
		return "RADIX";
	case HirFIntrinsicCallExpression::IK_RANGE:			//	RANGE (X) Decimal exponent range
		return "RANGE";
	case HirFIntrinsicCallExpression::IK_TINY:				//	TINY (X) Smallest positive number of the model
		return "TINY";

//	13.11.9 Bit inquiry function
	case HirFIntrinsicCallExpression::IK_BIT_SIZE:			//	BIT_SIZE (I) Number of bits of the model
		return "BIT_SIZE";
//	13.11.10 Bit manipulation functions
	case HirFIntrinsicCallExpression::IK_BTEST:			//	BTEST (I, POS) Bit testing
		return "BTEST";
	case HirFIntrinsicCallExpression::IK_IAND:				//	IAND (I, J) Logical AND
		return "IAND";
	case HirFIntrinsicCallExpression::IK_IBCLR:			//	IBCLR (I, POS) Clear bit
		return "IBCLR";
	case HirFIntrinsicCallExpression::IK_IBITS:			//	IBITS (I, POS, LEN) Bit extraction
		return "IBITS";
	case HirFIntrinsicCallExpression::IK_IBSET:			//	IBSET (I, POS) Set bit
		return "IBSET";
	case HirFIntrinsicCallExpression::IK_IEOR:				//	IEOR (I, J) Exclusive OR
		return "IEOR";
	case HirFIntrinsicCallExpression::IK_IOR:				//	IOR (I, J) Inclusive OR
		return "IOR";
	case HirFIntrinsicCallExpression::IK_ISHFT:			//	ISHFT (I, SHIFT) Logical shift
		return "ISHFT";
	case HirFIntrinsicCallExpression::IK_ISHFTC:			//	ISHFTC (I, SHIFT [, SIZE]) Circular shift
		return "ISHFTC";
	case HirFIntrinsicCallExpression::IK_NOT:				//	NOT (I) Logical complement
		return "NOT";

//	13.11.11 Transfer function
	case HirFIntrinsicCallExpression::IK_TRANSFER:			//	TRANSFER (SOURCE, MOLD [, SIZE]) Treat first argument as if of type of second argument
		return "TRANSFER";

//	13.11.12 Floating-point manipulation functions
	case HirFIntrinsicCallExpression::IK_EXPONENT:			//	EXPONENT (X) Exponent part of a model number
		return "EXPONENT";
	case HirFIntrinsicCallExpression::IK_FRACTION:		//	FRACTION (X) Fractional part of a number
		return "FRACTION";
	case HirFIntrinsicCallExpression::IK_NEAREST:			//	NEAREST (X, S) Nearest different processor number in given direction
		return "NEAREST";
	case HirFIntrinsicCallExpression::IK_RRSPACING:		//	RRSPACING (X) Reciprocal of the relative spacing of model numbers near given number
		return "RRSPACING";
	case HirFIntrinsicCallExpression::IK_SCALE:			//	SCALE (X, I) Multiply a real by its base to an integer power
		return "SCALE";
	case HirFIntrinsicCallExpression::IK_SET_EXPONENT:	//	SET_EXPONENT (X, I) Set exponent part of a number
		return "SET_EXPONENT";
	case HirFIntrinsicCallExpression::IK_SPACING:			//	SPACING (X) Absolute spacing of model numbers near given number
		return "SPACING";

			//	13.11.13 Vector and matrix multiply functions
	case HirFIntrinsicCallExpression::IK_DOT_PRODUCT:		//	DOT_PRODUCT (VECTOR_A, VECTOR_B) Dot product of two rank-one arrays
		return "OT_PRODUCT";
	case HirFIntrinsicCallExpression::IK_MATMUL:			//	MATMUL (MATRIX_A, MATRIX_B) Matrix multiplication
		return "MATMUL";

			//	13.11.14 Array reduction functions
	case HirFIntrinsicCallExpression::IK_ALL:				//	ALL (MASK [, DIM]) True if all values are true
		return "ALL";
	case HirFIntrinsicCallExpression::IK_ANY:				//	ANY (MASK [, DIM]) True if any value is true
		return "ANY";
	case HirFIntrinsicCallExpression::IK_COUNT:			//	COUNT (MASK [, DIM]) Number of true elements in an array
		return "COUNT";
	case HirFIntrinsicCallExpression::IK_MAXVAL_DIM:		//	MAXVAL (ARRAY, DIM [, MASK])
		return "MAXVAL_DIM";
	case HirFIntrinsicCallExpression::IK_MAXVAL:			//	or MAXVAL (ARRAY [, MASK]) Maximum value in an array
		return "MAXVAL";
	case HirFIntrinsicCallExpression::IK_MINVAL_DIM:		//	MINVAL (ARRAY, DIM [, MASK])
		return "MINVAL_DIM";
	case HirFIntrinsicCallExpression::IK_MINVAL:			//	or MINVAL (ARRAY [, MASK]) Minimum value in an array
		return "MINVAL";
	case HirFIntrinsicCallExpression::IK_PRODUCT_DIM:		//	PRODUCT (ARRAY, DIM [, MASK])
		return "PRODUCT_DIM";
	case HirFIntrinsicCallExpression::IK_PRODUCT:			//	or PRODUCT (ARRAY [, MASK]) Product of array elements
		return "PRODUCT";
	case HirFIntrinsicCallExpression::IK_SUM_DIM:			//	SUM (ARRAY, DIM [, MASK])
		return "SUM_DIM";
	case HirFIntrinsicCallExpression::IK_SUM:				//	or SUM (ARRAY [, MASK]) Sum of array elements
		return "SUM";

			//	13.11.15 Array inquiry functions
	case HirFIntrinsicCallExpression::IK_ALLOCATED:		//	ALLOCATED (ARRAY) Array allocation status
		return "ALLOCATED";
	case HirFIntrinsicCallExpression::IK_LBOUND:			//	LBOUND (ARRAY [, DIM]) Lower dimension bounds of an array
		return "LBOUND";
	case HirFIntrinsicCallExpression::IK_SHAPE:			//	SHAPE (SOURCE) Shape of an array or scalar
		return "SHAPE";
	case HirFIntrinsicCallExpression::IK_SIZE:			//	SIZE (ARRAY [, DIM]) Total number of elements in an array
		return "SIZE";
	case HirFIntrinsicCallExpression::IK_UBOUND:			//	UBOUND (ARRAY [, DIM]) Upper dimension bounds of an array
		return "UBOUND";

			//	13.11.16 Array construction functions
	case HirFIntrinsicCallExpression::IK_MERGE:			//	MERGE (TSOURCE, FSOURCE, MASK) Merge under mask
		return "MERGE";
	case HirFIntrinsicCallExpression::IK_PACK:			//	PACK (ARRAY, MASK [, VECTOR]) Pack an array into an array of rank one under a mask
		return "PACK";
	case HirFIntrinsicCallExpression::IK_SPREAD:			//	SPREAD (SOURCE, DIM, NCOPIES) Replicates array by adding a dimension
		return "SPREAD";
	case HirFIntrinsicCallExpression::IK_UNPACK:			//	UNPACK (VECTOR, MASK, FIELD) Unpack an array of rank one into an array under a mask
		return "UNPACK";

			//	13.11.17 Array reshape function
	case HirFIntrinsicCallExpression::IK_RESHAPE:			//	RESHAPE (SOURCE, SHAPE[, PAD, ORDER]) Reshape an array
		return "RESHAPE";

			//	13.11.18 Array manipulation functions
	case HirFIntrinsicCallExpression::IK_CSHIFT:			//	CSHIFT (ARRAY, SHIFT [, DIM]) Circular shift
		return "CSHIFT";
	case HirFIntrinsicCallExpression::IK_EOSHIFT:			//	EOSHIFT (ARRAY, SHIFT [, BOUNDARY, DIM]) End-off shift
		return "EOSHIFT";

	case HirFIntrinsicCallExpression::IK_TRANSPOSE:		//	TRANSPOSE (MATRIX) Transpose of an array of rank two
		return "TRANSPOSE";

			//	13.11.19 Array location functions
	case HirFIntrinsicCallExpression::IK_MAXLOC_DIM:		//	MAXLOC (ARRAY, DIM [, MASK])
		return "MAXLOC_DIM";
	case HirFIntrinsicCallExpression::IK_MAXLOC:			//	or MAXLOC (ARRAY [, MASK]) Location of a maximum value in an array
		return "MAXLOC";
	case HirFIntrinsicCallExpression::IK_MINLOC_DIM:		//	MINLOC (ARRAY, DIM [, MASK])
		return "MINLOC_DIM";
	case HirFIntrinsicCallExpression::IK_MINLOC:			//	or MINLOC (ARRAY [, MASK]) Location of a minimum value in an array
		return "MINLOC";

			//	13.11.20 Pointer association status functions
	case HirFIntrinsicCallExpression::IK_ASSOCIATED:		//	ASSOCIATED (POINTER [, TARGET]) Association status inquiry or comparison
		return "ASSOCIATED";
	case HirFIntrinsicCallExpression::IK_NULL:			//	NULL ([MOLD]) Returns disassociated pointer
		return "NULL";

			//	13.12 Intrinsic subroutines
	case HirFIntrinsicCallExpression::IK_CPU_TIME:		//	CPU_TIME (TIME) Obtain processor time
		return "CPU_TIME";
	case HirFIntrinsicCallExpression::IK_DATE_AND_TIME:	//	DATE_AND_TIME ([DATE, TIME, ZONE, VALUES]) Obtain date and time
		return "DATE_AND_TIME";
	case HirFIntrinsicCallExpression::IK_MVBITS:			//	MVBITS (FROM, FROMPOS, LEN, TO, TOPOS) Copies bits from one integer to another
		return "MVBITS";
	case HirFIntrinsicCallExpression::IK_RANDOM_NUMBER:	//	RANDOM_NUMBER (HARVEST) Returns pseudorandom number
		return "RANDOM_NUMBER";
	case HirFIntrinsicCallExpression::IK_RANDOM_SEED:		//	RANDOM_SEED ([SIZE, PUT, GET]) Initializes or restarts the pseudorandom number generator
		return "RANDOM_SEED";
	case HirFIntrinsicCallExpression::IK_SYSTEM_CLOCK:	//	SYSTEM_CLOCK ([COUNT, COUNT_RATE, COUNT_MAX]) Obtain data from the system clock
		return "SYSTEM_CLOCK";
	default:
		return Strings::format("(unknown intrinsic kind %u)", kind);
	}
}

std::string HirFIntrinsicCallExpression::dumpKindFormat(IntrinsicKind kind)
{
	OPS_UNUSED(kind)
	return "(unexpected)";
}

//	HirFIntrinsicReferenceExpression class implementation
HirFIntrinsicReferenceExpression::HirFIntrinsicReferenceExpression(HirFIntrinsicCallExpression::IntrinsicKind kind)
	: m_kind(kind)
{
}

HirFIntrinsicCallExpression::IntrinsicKind HirFIntrinsicReferenceExpression::getKind(void) const
{
	return m_kind;
}

//		ExpressionBase implementation
bool HirFIntrinsicReferenceExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<HirFIntrinsicReferenceExpression>())
	{
		const HirFIntrinsicReferenceExpression& other = exprNode.cast_to<HirFIntrinsicReferenceExpression>();
		if (other.m_kind == m_kind)
			return true;
	}
	return false;
}

//		ClonableMix implementation
HirFIntrinsicReferenceExpression* HirFIntrinsicReferenceExpression::clone(void) const
{
	HirFIntrinsicReferenceExpression* result = new HirFIntrinsicReferenceExpression(m_kind);
	result->acquireNotes(*this);
	return result;
}

//		HirFIntrinsicReferenceExpression - RepriseBase implementation
int HirFIntrinsicReferenceExpression::getChildCount(void) const
{
	return 0;
}

RepriseBase& HirFIntrinsicReferenceExpression::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("HirFIntrinsicReferenceExpression::getChild()");
}

std::string HirFIntrinsicReferenceExpression::dumpState(void) const
{
	std::string state = ExpressionBase::dumpState();
	state += Strings::format("%u", m_kind);
	return state;
}


}
}
}
