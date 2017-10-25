#include "Transforms/Scalar/ArithmeticOperatorExpansion.h"
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/ServiceFunctions.h"

using namespace OPS::Reprise;

namespace OPS
{

namespace Transforms
{

namespace Scalar
{

void expandArithmeticOperators(StatementBase& stmt)
{
	using namespace ArithmeticOperatorExpansion;

	struct Walker: Service::DeepWalker
	{
		void visit(BasicCallExpression& expr)
		{
			Service::DeepWalker::visit(expr);			

			if (MulDivToShift::canApply(expr))
				MulDivToShift::apply(expr);

			if (ModToAnd::canApply(expr))
				ModToAnd::apply(expr);

			if (AssignmentCoercion::canApply(expr))
				AssignmentCoercion::apply(expr);
			
			if (AddToInc::canApply(expr))
			{
				AddToInc::apply(expr);
				return;
			}			

			if (AopToCall::canApply(expr))
			{
				AopToCall::apply(expr);
				return;
			}
		}

		void visit(SubroutineCallExpression& call)
		{
			Service::DeepWalker::visit(call);

			if (SCAC::canApply(call))
				SCAC::apply(call);
		}
	};

	Walker visitor;

	//stmt.findProgramUnit()->accept(visitor);
	stmt.accept(visitor);
}

namespace ArithmeticOperatorExpansion
{

namespace Utils
{

/// Represents properties of a scalar type. 

struct TypeTraits
{
	enum Format { Integer, Float } format;
	bool sign;
	int bits;

	bool read(const TypeBase* type);
};

/// Represents properties of a function argument.

struct ArgInfo
{
	enum Format { SignedInt, UnsignedInt } format;
	int bits;			// number of bits
	int index;			// index in the list of subroutine arguments
	bool explicitArg;	// if set, the argument is not converted to bits
    ReprisePtr<TypeBase> type;

	ArgInfo();

	std::string formatName() const;	// i8, u16, etc.
	std::string name() const;		// arg1, arg3, etc.
	std::string bitsName() const;	// arg1_bits, arg3_bits, etc.
	std::string structName() const;	// arg2_int, arg0_int, etc.
};

/*!	@brief Handler description.

	Handler for an arithmetic operation may have the form:

		int i32_shl_i32_u8(int a, int b)

	this handler has the following properties:

		• internal algorithm "shl" that operates on two bit arrays
		• result type "signed 32-bit integer integer"
		• argument #1 of type "signed 32-bit integer"
		• argument #2 of type "unsigned 8-bit integer"

	The structure keeps this fields separately.
*/

struct HandlerInfo
{
	std::string algorithm;
	std::list<ArgInfo> arguments;
	ArgInfo result;	

	void push(ArgInfo& arg);	// adds an argument and gives it an index
	std::string name() const;	// name of the handler function, like i8_add_u16_i32
};

ArgInfo::ArgInfo()
{
	format = SignedInt;
	bits = 0;
	index = 0;
	explicitArg = false;
}

std::string ArgInfo::formatName() const
{
	switch (format)
	{
	case SignedInt:
		return "i" + intToStr(bits);

	case UnsignedInt:
		return "u" + intToStr(bits);
	}

	OPS_ASSERT(false);
	throw OPS::RuntimeError("Unexpected format in ArgInfo::formatName()");
}

std::string ArgInfo::name() const
{
	OPS_ASSERT(index >= 0);

	if (index > 0)
		return "arg" + intToStr(index);

	return "res";
}

std::string ArgInfo::bitsName() const
{
	return name() + "_bits";
}

std::string ArgInfo::structName() const
{
	return name() + "_int";
}

void HandlerInfo::push(ArgInfo& arg)
{
	arg.index = arguments.size() + 1;
	arguments.push_back(arg);
}

std::string HandlerInfo::name() const
{
	std::string s = result.formatName() + "_" + algorithm;
	std::list<ArgInfo>::const_iterator i = arguments.begin();

	while (i != arguments.end())	
		s += "_" + i->formatName(),
		++i;
	
	return s;
}

/*!	Finds a basic type (int, char, float, etc.) associated with a 
	specified type instance. The accepted type can be a basic type,
	a typedef and so on.
*/

bool getBasicType(BasicType::BasicTypes& basicType, const TypeBase& type)
{
	if (type.is_a<BasicType>())
	{
		basicType = type.cast_to<BasicType>().getKind();
		return true;
	}

	if (type.is_a<TypedefType>())
		return getBasicType(
			basicType,
			type.cast_to<TypedefType>().getBaseType());

	if (type.is_a<DeclaredType>())
		return getBasicType(
			basicType,
			type.cast_to<DeclaredType>().getDeclaration().getType());

	return false;
}

bool TypeTraits::read(const TypeBase* type)
{
	OPS_ASSERT(type != NULL);

	BasicType::BasicTypes basicType;

	if (!getBasicType(basicType, *type))
		return false;

	switch (basicType)
	{
	case BasicType::BT_BOOLEAN:
	case BasicType::BT_INT8:
	case BasicType::BT_CHAR:
		bits = 8;
		format = Integer;
		sign = true;
		break;

	case BasicType::BT_INT16:
		bits = 16;
		format = Integer;
		sign = true;
		break;

	case BasicType::BT_INT32:
		bits = 32;
		format = Integer;
		sign = true;
		break;

	case BasicType::BT_INT64:
		bits = 64;
		format = Integer;
		sign = true;
		break;

    case BasicType::BT_INT128:
        bits = 128;
        format = Integer;
        sign = true;
        break;

	case BasicType::BT_UINT8:
		bits = 8;
		format = Integer;
		sign = false;
		break;

	case BasicType::BT_UINT16:
		bits = 16;
		format = Integer;
		sign = false;
		break;

	case BasicType::BT_UINT32:
		bits = 32;
		format = Integer;
		sign = false;
		break;

	case BasicType::BT_UINT64:
		bits = 64;
		format = Integer;
		sign = false;
		break;

    case BasicType::BT_UINT128:
        bits = 128;
        format = Integer;
        sign = false;
        break;

	default:
		return false;
	}
	
	return true;
}

const std::string& getText(TextId id)
{
	struct TextConstantStorage
	{
		TextConstantStorage()
		{
			set(SR_GET_BITS,	"sr_get_bits");
			set(SR_SET_BITS,	"sr_set_bits");
			set(SR_CREATE,		"sr_create");
			set(SR_INT_STRUCT,	"sr_int");
			set(SR_BIT,			"bit");

			set(AA_CONV,		"sr_conv");

			set(AA_BIT_NOT,		"sr_not");
			set(AA_BIT_AND,		"sr_and");
			set(AA_BIT_OR,		"sr_or");
			set(AA_BIT_XOR,		"sr_xor");

			set(AA_LOG_AND,		"sr_log_and");
			set(AA_LOG_OR,		"sr_log_or");
			set(AA_LOG_NOT,		"sr_log_not");

			set(AA_SHL,			"sr_shl");
			set(AA_SHR,			"sr_shr");

			set(AA_EQ,			"sr_eq");
			set(AA_NOT_EQ,		"sr_neq");
			set(AA_GREATER,		"sr_greater");
			set(AA_GREATER_EQ,	"sr_greater_eq");
			set(AA_LESS,		"sr_less");
			set(AA_LESS_EQ,		"sr_less_eq");

			set(AA_ADD,			"sr_add");				
			set(AA_SUB,			"sr_sub");
			set(AA_NEG,			"sr_neg");
			set(AA_MUL,			"sr_mul");
			set(AA_DIV,			"sr_div");
			set(AA_MOD,			"sr_mod");			
			set(AA_INC,			"sr_inc");
			set(AA_DEC,			"sr_dec");
		}

		const std::string& operator[] (TextId id)
		{
			OPS_ASSERT(m_storage.find(id) != m_storage.end());				
			return m_storage[id];
		}

	private:

		void set(TextId id, const std::string& s)
		{
			m_storage[id] = s;
		}

		std::map<TextId, std::string> m_storage;
	};

	static TextConstantStorage storage;

	return storage[id];
}

bool opName(std::string& name, BasicCallExpression::BuiltinCallKind kind)
{
	struct OpNameMap
	{
		OpNameMap()
		{
			// bitwise operators

			map[BasicCallExpression::BCK_BITWISE_NOT]	= AA_BIT_NOT;
			map[BasicCallExpression::BCK_BITWISE_AND]	= AA_BIT_AND;
			map[BasicCallExpression::BCK_BITWISE_OR]	= AA_BIT_OR;
			map[BasicCallExpression::BCK_BITWISE_XOR]	= AA_BIT_XOR;
			map[BasicCallExpression::BCK_LEFT_SHIFT]	= AA_SHL;
			map[BasicCallExpression::BCK_RIGHT_SHIFT]	= AA_SHR;

			// logical operators	

			map[BasicCallExpression::BCK_LOGICAL_AND]	= AA_LOG_AND;
			map[BasicCallExpression::BCK_LOGICAL_NOT]	= AA_LOG_NOT;
			map[BasicCallExpression::BCK_LOGICAL_OR]	= AA_LOG_OR;	

			// comparing operators

			map[BasicCallExpression::BCK_EQUAL]			= AA_EQ;
			map[BasicCallExpression::BCK_GREATER]		= AA_GREATER;
			map[BasicCallExpression::BCK_GREATER_EQUAL] = AA_GREATER_EQ;
			map[BasicCallExpression::BCK_LESS]			= AA_LESS;
			map[BasicCallExpression::BCK_LESS_EQUAL]	= AA_LESS_EQ;
			map[BasicCallExpression::BCK_NOT_EQUAL]		= AA_NOT_EQ;			

			// arithmetic operators

			map[BasicCallExpression::BCK_DIVISION]		= AA_DIV;
			map[BasicCallExpression::BCK_MULTIPLY]		= AA_MUL;
			map[BasicCallExpression::BCK_UNARY_MINUS]	= AA_NEG;	
			map[BasicCallExpression::BCK_BINARY_PLUS]	= AA_ADD;
			map[BasicCallExpression::BCK_BINARY_MINUS]	= AA_SUB;
			map[BasicCallExpression::BCK_INTEGER_MOD]	= AA_MOD;
		}	

		typedef std::map<BasicCallExpression::BuiltinCallKind, TextId> Container;
		Container map;
	};

	static OpNameMap ops;

	if (ops.map.find(kind) == ops.map.end())
		return false;

	name = getText(ops.map[kind]);
	return true;
}

/// Calculates log2(n)

template <typename IntType>
IntType log2(IntType n)
{
	if (n <= 0 || (n & (n - 1)) != 0)
		return (IntType)-1;

	IntType log2 = 0;

	while (n != 0)
		log2++,
		n >>= 1;
	
	return log2 - 1;
}

/// Calculates log2(a)

template <typename IntType>
IntType log2(const ExpressionBase& expr)
{
	if (!expr.is_a<StrictLiteralExpression>())
		return -1;

	const StrictLiteralExpression& a = expr.cast_to<StrictLiteralExpression>();

	switch (a.getLiteralType())
	{
	case BasicType::BT_INT8:
		return (IntType)log2(a.getInt8());

	case BasicType::BT_INT16:
		return (IntType)log2(a.getInt16());

	case BasicType::BT_INT32:
		return (IntType)log2(a.getInt32());

	case BasicType::BT_INT64:
		return (IntType)log2(a.getInt64());

	case BasicType::BT_UINT8:
		return (IntType)log2(a.getUInt8());

	case BasicType::BT_UINT16:
		return (IntType)log2(a.getUInt16());

	case BasicType::BT_UINT32:
		return (IntType)log2(a.getUInt32());

	case BasicType::BT_UINT64:
		return (IntType)log2(a.getUInt64());

	default:
		return -1;
	}	
}

/// Checks whether a number is a power of two and the power is positive.

template <typename IntType>
bool isPowerOfTwo(IntType a)
{
	return log2(a) >= 0;
}

/// Checks whether a number is a power of two.

bool isPowerOfTwo(const ExpressionBase& expr)
{
	return log2<int>(expr) >= 0;		
}

std::string intToStr(int a)
{
	return OPS::Strings::format("%d", a);	
}

bool typeInfo(ArgInfo& var, ReprisePtr<TypeBase> type)
{
	TypeTraits traits;

    if (!traits.read(type.get()) || traits.format != TypeTraits::Integer)
		return false;

	var.bits = traits.bits;
	var.format = traits.sign ? ArgInfo::SignedInt : ArgInfo::UnsignedInt;
	var.type = type;

	return true;
}

///	Checks whether a specified type is integer.

bool isInt(const TypeBase* type)
{	
	TypeTraits traits;

	return
		traits.read(type) &&
		traits.format == TypeTraits::Integer;
}

/// Checks whether a specified type is an unisgned integer.

bool isUnsignedInt(const TypeBase* type)
{
	TypeTraits traits;

	return
		traits.read(type) &&
		traits.format == TypeTraits::Integer &&
		!traits.sign;
}

/*!	Binary operation with its pair of operands corresponds
	to a routine that implements this operation. This function
	looks for such a routine and if it exists, returns its information.	
*/

bool handlerInfo(
	HandlerInfo& handler,
	const BasicCallExpression& expr,
	Declarations& globals)
{
	// get algorithm name

	if (!opName(handler.algorithm, expr.getKind()))
		return false;

	// get the algorithm subroutine

	SubroutineDeclaration* algFunc = globals.findSubroutine(handler.algorithm);

	if (algFunc == NULL)
		return false;	

	SubroutineType& algType = algFunc->getType();

	// get result type info

    if (!typeInfo(handler.result, expr.getResultType()))
		return false;

	handler.result.type = expr.getResultType();	

	// get arguments types info	

	TypeDeclaration* intStruct = globals.findType(getText(SR_INT_STRUCT));
	handler.arguments.clear();
	
	for (int i = 0; i < expr.getArgumentCount(); i++)
	{
		ArgInfo arg;

        if (!typeInfo(arg, expr.getArgument(i).getResultType()))
			return false;
		
		/*	If the i-th parameter of the algorithm subroutine is not
			an sr_int structure, then the argument must not be converted
			to bits. The following piece of code has the only intent:
			
				arg.explicitArg = type(param(i + 1)) != type(struct _sr_int)
				
			but since parameters type are DeclaredTypes and the structure
			is a TypedefTypes, the code appears trickly. */

		if (intStruct == NULL || i + 1 >= algType.getParameterCount())
			arg.explicitArg = true;
		else
		{
			TypeBase& structType = intStruct->getType();
			TypeBase& paramType = algType.getParameter(i + 1).getType();

			if (!paramType.is_a<DeclaredType>())
				arg.explicitArg = true;
			else
				arg.explicitArg = !structType.isEqual(
					paramType.cast_to<DeclaredType>().getDeclaration().getType());			
		}
				
		handler.push(arg);
	}	

	return true;
}

/*!	Contains a SubroutineDeclaration and helps
	constructing its parts.
*/

struct SubroutineManager
{
	SubroutineManager(SubroutineDeclaration* f)		
	{
		OPS_ASSERT(f != NULL);
		m_func = f;
	}

	SubroutineDeclaration& func() const
	{
		return *m_func;
	}

	Declarations& declarations() const
	{
		return func().getDeclarations();
	}

	BlockStatement& body() const
	{
		return func().getBodyBlock();
	}

	void addParameter(TypeBase* type, const std::string& name) const
	{
		func().getType().addParameter(
			new ParameterDescriptor(			
				name,
				type));			
	}

	void addDeclaration(TypeBase* type, const std::string& name, bool local) const
	{
		VariableDeclaration* var = new VariableDeclaration(type, name);

		declarations().addLast(var);

		if (local)
			var->setDefinedBlock(body());
	}

	VariableDeclaration& getDeclaration(const std::string& name) const
	{
		VariableDeclaration* var = declarations().findVariable(name);
		OPS_ASSERT(var != NULL);
		return *var;
	}

private:

	SubroutineDeclaration* m_func;
};

/*!	Creates a call of the form:

		func(arg.bitsName(), &var, arg.bits)
*/

SubroutineCallExpression* processArgument(
	SubroutineManager& man,
	SubroutineDeclaration& func,
	const ArgInfo& i)
{
	SubroutineCallExpression* call = new SubroutineCallExpression(
		new SubroutineReferenceExpression(
			func));	

	call->addArgument(
		new ReferenceExpression(
			man.getDeclaration(
				i.bitsName())));

	call->addArgument(
		new BasicCallExpression(
			BasicCallExpression::BCK_TAKE_ADDRESS,
			new ReferenceExpression(
				man.getDeclaration(
					i.name()))));

	call->addArgument(
		StrictLiteralExpression::createInt32(
			i.bits));					

	return call;
}

/*!	Constructs a function with a specified name and arguments
	corresponding to arguments of the expression.
	The body looks like this:

		int i32_add_i8_u32(char a, unsigned b)
		{
			bit a_bits[8];
			bit b_bits[32];
			bit r_bits[32];

			sr_int a_int;
			sr_int b_int;
			sr_int r_int;

			int r;

			sr_get_bits(a_bits, &a, 8);
			sr_get_bits(b_bits, &b, 32);

			a_int = sr_create(a_bits, 8, 1);
			b_int = sr_create(b_bits, 32, 0);
			r_int = sr_create(r_bits, 32, 1);

			sr_add(r_int, a_int, b_int);
			sr_set_bits(r_bits, &r, 32);
			return r;
		}
*/

SubroutineDeclaration* createFunction(
	const HandlerInfo& handler,
	SubroutineDeclaration* algorithm,	// add, sub, neg, ...
	SubroutineDeclaration* getBits,		// get_bits
	SubroutineDeclaration* setBits,		// set_bits
	SubroutineDeclaration* create,		// sr_create
	TypeDeclaration* intStruct,			// sr_int structure
	TypeDeclaration* bitType)			// "bit" type
{
	OPS_ASSERT(algorithm != NULL);
	OPS_ASSERT(getBits != NULL);
	OPS_ASSERT(setBits != NULL);
	OPS_ASSERT(create != NULL);
	OPS_ASSERT(intStruct != NULL);
	OPS_ASSERT(bitType != NULL);

	typedef std::list<ArgInfo>::const_iterator argument;
	const std::list<ArgInfo>& args = handler.arguments;

	// merge the result with arguments for further unified processing;
	// it's important that the result holds the first position, i.e.
	// the list looks like: result, arg1, arg2, arg3, ...

	std::list<ArgInfo> vars;

	vars.push_back(handler.result);
	
	for (argument i = args.begin(); i != args.end(); ++i)
		vars.push_back(*i);

	// construct the function	

	SubroutineManager man(
		new SubroutineDeclaration(
			new SubroutineType(
                handler.result.type->clone(),		// result type
				SubroutineType::CK_DEFAULT,	// calling convention
				false,						// vararg
				true),						// args known			
			handler.name()));

	man.func().setBodyBlock(
		ReprisePtr<BlockStatement>(
			new BlockStatement));

	man.func().setDeclarations(new Declarations);

	// declare arguments

	for (argument i = args.begin(); i != args.end(); ++i)
        man.addParameter(i->type->clone(), i->name());

	// declare arguments as local variables	- this is required
	
	for (argument i = args.begin(); i != args.end(); ++i)
        man.addDeclaration(i->type->clone(), i->name(), false);

	// declare local variables	

	for (argument i = vars.begin(); i != vars.end(); ++i)
		if (!i->explicitArg)
			man.addDeclaration(			
				new ArrayType(
					i->bits,
					new DeclaredType(*bitType)),
				i->bitsName(),
				true);	

	for (argument i = vars.begin(); i != vars.end(); ++i)
		if (!i->explicitArg)
			man.addDeclaration(
				new DeclaredType(*intStruct),			
				i->structName(),
				true);	

	man.addDeclaration(
        handler.result.type->clone(),
		handler.result.name(),
		true);

	// make calls to get_bits that extract bits from arguments
		
	for (argument i = args.begin(); i != args.end(); ++i)
		if (!i->explicitArg)
			man.body().addLast(
				new ExpressionStatement(
					processArgument(man, *getBits, *i)));

	// make calls to sr_create that fills in sr_int structures

	for (argument i = vars.begin(); i != vars.end(); ++i)
		if (!i->explicitArg)
		{
			SubroutineCallExpression* call = new SubroutineCallExpression(
				new SubroutineReferenceExpression(
					*create));

			call->addArgument(
				new ReferenceExpression(
					man.getDeclaration(
						i->bitsName())));

			call->addArgument(
				StrictLiteralExpression::createInt32(
					i->bits));

			call->addArgument(
				StrictLiteralExpression::createBoolean(
					i->format == ArgInfo::SignedInt));		

			man.body().addLast(
				new ExpressionStatement(
					new BasicCallExpression(
						BasicCallExpression::BCK_ASSIGN,
						new ReferenceExpression(
							man.getDeclaration(
								i->structName())),
						call)));
		}

	// make a call to the main algorithm that computes bits of the result

	SubroutineCallExpression* algorithmCall = new SubroutineCallExpression(
		new SubroutineReferenceExpression(
			*algorithm));

	for (argument i = vars.begin(); i != vars.end(); ++i)
		algorithmCall->addArgument(
			new ReferenceExpression(
				man.getDeclaration(
					i->explicitArg ? i->name() : i->structName())));

	man.body().addLast(
		new ExpressionStatement(
			algorithmCall));

	// make a call to set_bits that collects bits into the resulting variable

	man.body().addLast(
		new ExpressionStatement(
			processArgument(
				man,
				*setBits,
				handler.result)));

	// make the return statement

	man.body().addLast(
		new ReturnStatement(
			new ReferenceExpression(
				man.getDeclaration(
					handler.result.name()))));

	// return the created function

	return &man.func();
}

/*!	Creates a call to the handler.
	If neccessary, creates the handler subroutine.
*/

SubroutineCallExpression* createFunctionCall(
	const HandlerInfo& handler,
	Declarations& globals)
{
	SubroutineDeclaration* func = globals.findSubroutine(handler.name());

	if (func == NULL)
	{
		func = createFunction(
			handler,
			globals.findSubroutine(handler.algorithm),
			globals.findSubroutine(getText(SR_GET_BITS)),
			globals.findSubroutine(getText(SR_SET_BITS)),
			globals.findSubroutine(getText(SR_CREATE)),
			globals.findType(getText(SR_INT_STRUCT)),
			globals.findType(getText(SR_BIT)));

		globals.addSubroutine(func);
	}

	return
		new SubroutineCallExpression(
			new SubroutineReferenceExpression(
				*func));
}

/*!	Creates a call to a coercion subroutine that accepts a value
	of one type and returns a value of another type. If the coercion is not
	possible or useless (the two types are identical), the function
	returns NULL.
*/

SubroutineCallExpression* createCoercionCall(	
	TypeBase* srcType,
    TypeBase* resType,
	Declarations& globals)
{
	OPS_ASSERT(srcType != NULL);
	OPS_ASSERT(resType != NULL);	

	// determine the coercion algorithm

	const std::string algorithm = getText(AA_CONV);

	// check that the algorithm subroutine is declared

	if (globals.findSubroutine(algorithm) == NULL)
		return NULL;

	// construct a description of the conversion routine

	HandlerInfo handler;	
	ArgInfo handlerArg;
	
    if (!typeInfo(handler.result, ReprisePtr<TypeBase>(resType)) ||
        !typeInfo(handlerArg, ReprisePtr<TypeBase>(srcType)))
		return NULL;

	handlerArg.index = 1;
	handler.algorithm = algorithm;
	handler.result.index = 0;
	handler.arguments.push_back(handlerArg);

	// compare the actual argument type and the type required by the subroutine

	if (handler.result.format == handlerArg.format &&
		handler.result.bits == handlerArg.bits)
		return NULL;

	// create a call to the handler
	
	return createFunctionCall(handler, globals);
}

/// Checks whether all expression arguments are integer.

bool allArgsInteger(const CallExpressionBase& expr)
{
	for (int i = 0; i < expr.getArgumentCount(); i++)
        if (!isInt(expr.getArgument(i).getResultType().get()))
			return false;

	return true;
}

/// Returns globals corresponding to an expression.

Declarations& getGlobals(const RepriseBase& node)
{
	TranslationUnit* unit = node.findTranslationUnit();
	OPS_ASSERT(unit != NULL);
	return unit->getGlobals();
}

/// Checks whether service routines are accessible.

bool allSRAccessible(const RepriseBase& node)
{
	Declarations& globals = getGlobals(node);

	return
		globals.findSubroutine(getText(SR_GET_BITS)) != NULL &&
		globals.findSubroutine(getText(SR_SET_BITS)) != NULL &&
		globals.findSubroutine(getText(SR_CREATE)) != NULL &&
		globals.findType(getText(SR_INT_STRUCT)) != NULL &&
		globals.findType(getText(SR_BIT)) != NULL;
}

} // namespace Utils

namespace MulDivToShift
{

using Utils::log2;

bool canApply(const BasicCallExpression& expr)
{	
	if (!Utils::allArgsInteger(expr))
		return false;	

	if (expr.getKind() != BasicCallExpression::BCK_MULTIPLY &&
		expr.getKind() != BasicCallExpression::BCK_DIVISION)
		return false;

	if (expr.getArgumentCount() != 2)
		return false;

	const ExpressionBase& arg0 = expr.getArgument(0);
	const ExpressionBase& arg1 = expr.getArgument(1);

	return Utils::isPowerOfTwo(arg0) || Utils::isPowerOfTwo(arg1);
}

void apply(BasicCallExpression& expr)
{
	OPS_ASSERT(canApply(expr));

	// ensure that the last argument is a constant in form 2^n

	if (expr.getKind() == BasicCallExpression::BCK_MULTIPLY)
	{
		ReprisePtr<ExpressionBase> arg0(&expr.getArgument(0));
		ReprisePtr<ExpressionBase> arg1(&expr.getArgument(1));		

		if (!Utils::isPowerOfTwo(*arg1))
		{
			expr.setArgument(0, &*arg1);
			expr.setArgument(1, &*arg0);
		}
	}

	StrictLiteralExpression& a = expr.getArgument(1).cast_to<StrictLiteralExpression>();	

	switch (a.getLiteralType())
	{
	case BasicType::BT_INT8:
		a.setInt8(log2(a.getInt8()));
		break;

	case BasicType::BT_INT16:
		a.setInt16(log2(a.getInt16()));
		break;

	case BasicType::BT_INT32:
		a.setInt32(log2(a.getInt32()));
		break;

	case BasicType::BT_INT64:
		a.setInt64(log2(a.getInt64()));
		break;

	case BasicType::BT_UINT8:
		a.setUInt8(log2(a.getUInt8()));
		break;

	case BasicType::BT_UINT16:
		a.setUInt16(log2(a.getUInt16()));
		break;

	case BasicType::BT_UINT32:
		a.setUInt32(log2(a.getUInt32()));
		break;

	case BasicType::BT_UINT64:
		a.setUInt64(log2(a.getUInt64()));
		break;

	default:
		OPS_ASSERT(false);
	}

	switch (expr.getKind())
	{
	case BasicCallExpression::BCK_MULTIPLY:
		expr.setKind(BasicCallExpression::BCK_LEFT_SHIFT);
		break;

	case BasicCallExpression::BCK_DIVISION:
		expr.setKind(BasicCallExpression::BCK_RIGHT_SHIFT);
		break;

	default:
		OPS_ASSERT(false);
	}
}

} // namespace MulDivToShift

namespace ModToAnd
{

bool canApply(const BasicCallExpression& expr)
{
	return
		Utils::allArgsInteger(expr) &&
		expr.getKind() == BasicCallExpression::BCK_INTEGER_MOD &&
		expr.getArgumentCount() == 2 &&
        Utils::isUnsignedInt(expr.getArgument(0).getResultType().get()) &&
		Utils::isPowerOfTwo(expr.getArgument(1));
}

void apply(BasicCallExpression& expr)
{
	OPS_ASSERT(canApply(expr));

	StrictLiteralExpression& a = expr.getArgument(1).cast_to<StrictLiteralExpression>();	

	switch (a.getLiteralType())
	{
	case BasicType::BT_INT8:
		a.setInt8(a.getInt8() - 1);
		break;

	case BasicType::BT_INT16:
		a.setInt16(a.getInt16() - 1);
		break;

	case BasicType::BT_INT32:
		a.setInt32(a.getInt32() - 1);
		break;

	case BasicType::BT_INT64:
		a.setInt64(a.getInt64() - 1);
		break;

	case BasicType::BT_UINT8:
		a.setUInt8(a.getUInt8() - 1);
		break;

	case BasicType::BT_UINT16:
		a.setUInt16(a.getUInt16() - 1);
		break;

	case BasicType::BT_UINT32:
		a.setUInt32(a.getUInt32() - 1);
		break;

	case BasicType::BT_UINT64:
		a.setUInt64(a.getUInt64() - 1);
		break;

	default:
		OPS_ASSERT(false);
	}

	expr.setKind(BasicCallExpression::BCK_BITWISE_AND);
}

} // namespace ModToAnd

namespace AopToCall
{

using namespace Utils;

bool canApply(const BasicCallExpression& expr)
{
	return allArgsInteger(expr) && allSRAccessible(expr);	
}

void apply(BasicCallExpression& expr)
{
	OPS_ASSERT(canApply(expr));		

	Declarations& globals = getGlobals(expr);

	// get a handler function name

	HandlerInfo handler;
	
	if (!handlerInfo(handler, expr, globals))
		return;	

	// create a call to the handler

	ReprisePtr<SubroutineCallExpression> call(
		createFunctionCall(
			handler,
			globals));

	for (int i = 0; i < expr.getArgumentCount(); i++)
		call->addArgument(&expr.getArgument(i));	

	// replace the expression with the call
	
	Editing::replaceExpression(expr, call);
}

} // namespace AopToCall

namespace SCAC
{

using namespace Utils;

bool canApply(const SubroutineCallExpression& call)
{
	return allSRAccessible(call);
}

void apply(SubroutineCallExpression& call)
{
	OPS_ASSERT(canApply(call));	

	// get the subroutine declaration

	ExpressionBase& callExpr = call.getCallExpression();
	
	if (!callExpr.is_a<SubroutineReferenceExpression>())
		return;
	
	SubroutineType& callType =
		callExpr.cast_to<SubroutineReferenceExpression>().getReference().getType();

	// traverse arguments

	for (int i = 0; i < call.getArgumentCount() && i < callType.getParameterCount(); i++)
	{
		ExpressionBase& arg = call.getArgument(i);		
		
		SubroutineCallExpression* conv = createCoercionCall(
            arg.getResultType().get(),
            &callType.getParameter(i).getType(),
			getGlobals(call));

		if (conv != NULL)
		{
			conv->addArgument(&arg);
			call.setArgument(i, conv);
		}
	}
}

} // namespace SCAC

namespace AssignmentCoercion
{

using OPS::Reprise::BasicCallExpression;
using namespace Utils;

bool canApply(const BasicCallExpression& expr)
{
	return
		allArgsInteger(expr) &&
		allSRAccessible(expr) &&
		expr.getKind() == BasicCallExpression::BCK_ASSIGN;
}

void apply(BasicCallExpression& expr)
{
	OPS_ASSERT(canApply(expr));
	OPS_ASSERT(expr.getArgumentCount() == 2);	

	SubroutineCallExpression* conv = createCoercionCall(
        expr.getArgument(1).getResultType().get(),
        expr.getArgument(0).getResultType().get(),
		getGlobals(expr));

	if (conv != NULL)
	{
		conv->addArgument(&expr.getArgument(1));
		expr.setArgument(1, conv);
	}	
}

} // namespace AssignmentCoercion

namespace AddToInc
{

using namespace Utils;

bool canApply(const BasicCallExpression& expr)
{
	return
		allSRAccessible(expr) &&
		expr.getArgumentCount() == 2 &&
        isInt(expr.getArgument(0).getResultType().get()) &&
		log2<int>(expr.getArgument(1)) == 0 &&
		
		(expr.getKind() == BasicCallExpression::BCK_BINARY_PLUS ||
		expr.getKind() == BasicCallExpression::BCK_BINARY_MINUS);		
}

void apply(BasicCallExpression& expr)
{
	OPS_ASSERT(canApply(expr));
	OPS_ASSERT(expr.getArgumentCount() == 2);

	// describe the handler call

	HandlerInfo handler;
	ArgInfo arg;

	if (!typeInfo(arg, expr.getArgument(0).getResultType()) ||
		!typeInfo(handler.result, expr.getResultType()))
		return;
	
	handler.push(arg);

	if (expr.getKind() == BasicCallExpression::BCK_BINARY_PLUS)
		handler.algorithm = getText(AA_INC);
	else
		handler.algorithm = getText(AA_DEC);

	// check that the algorithm implementing the operation is declared

	if (getGlobals(expr).findSubroutine(handler.algorithm) == NULL)
		return;	

	// create a call to the handler

	ReprisePtr<SubroutineCallExpression> call(
		createFunctionCall(
			handler,
			getGlobals(expr)));

	call->addArgument(&expr.getArgument(0));

	// replace the expression with the call
	
	Editing::replaceExpression(expr, call);
}

} // namespace AddToInc

} // namespace ArithmeticOperatorExpansion

} // namespace Scalar

} // namespace Transforms

} // namespace OPS
