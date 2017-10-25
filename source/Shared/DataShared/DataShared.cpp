#include "OPS_Core/Compiler.h"

#if OPS_COMPILER_MSC_VER > 0
#  pragma warning(push)
#  pragma warning(disable:4127 4146)
#endif

//#include "llvm/ADT/APInt.h"
#include "Shared/DataShared.h"
#include "OPS_Core/Localization.h"

#if OPS_COMPILER_MSC_VER > 0
#  pragma warning(pop)
#endif

#include "DataSharedDeepWalkers.h"
#include "Shared/ExpressionHelpers.h"

using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Shared::ExpressionHelpers;
using namespace std;

namespace OPS
{
namespace Shared
{

void getArrayLimits(TypeBase* typeBase, std::vector<int>& limits)
{
	limits.clear();	
	while (typeBase->is_a<ArrayType>())
	{
		ArrayType& arrayType = typeBase->cast_to<ArrayType>();
		limits.insert(limits.begin(), arrayType.getElementCount());
		typeBase = &(arrayType.getBaseType());
	}
}

TypeBase* getArrayElementBasicType(TypeBase* pTypeBase)
{
	OPS_ASSERT(pTypeBase != NULL);

	TypeBase* pType = pTypeBase;
	while(pType->is_a<PtrType>() || pType->is_a<ArrayType>() || pType->is_a<VectorType>())
	{
		if(pType->is_a<PtrType>())
		{
			pType = &pType->cast_ptr<PtrType>()->getPointedType();
		}
		else if(pType->is_a<ArrayType>())
		{
			pType = &pType->cast_ptr<ArrayType>()->getBaseType();
		}
		else if(pType->is_a<VectorType>())
		{
			pType = &pType->cast_ptr<VectorType>()->getBaseType();
		}
	}

	return pType;
}


static set<Reprise::VariableDeclaration*> getDeclaredVariables(BaseVisitable<> *code, Declarations& decls)
{
    OPS_ASSERT(code != NULL);

	CollectVarDeclarationsDeepWalker::VariablesDeclarationsContainer sourceDeclarations;
    for (Declarations::VarIterator iter = decls.getFirstVar(); iter.isValid(); ++iter)
	{
		sourceDeclarations.insert(&*iter);
	}

	CollectVarDeclarationsDeepWalker deepWalker(sourceDeclarations);

    code->accept(deepWalker);

	return deepWalker.getVariableDecls();
}

set<Reprise::VariableDeclaration*> getDeclaredVariables(StatementBase *pStatement)
{
    return getDeclaredVariables(pStatement, pStatement->getParentBlock().getDeclarations());
}

set<Reprise::VariableDeclaration*> getDeclaredVariables(const ProgramFragment &fragment)
{
    return getDeclaredVariables(const_cast<ProgramFragment*>(&fragment),
                                const_cast<Declarations&>(fragment.getStatementsBlock().getDeclarations()));
}

list<Reprise::VariableDeclaration*> getAllVariableDeclarations(ExpressionBase* pExpressionToInvestigate)
{
	list<Reprise::VariableDeclaration*> res;
	
	CollectVariablesDeepWalker deepWalker;
	pExpressionToInvestigate->accept(deepWalker);
	CollectVariablesDeepWalker::VariablesDeclarationsContainer variablesDecls = deepWalker.getVariablesDecls();

	res.insert(res.begin(), variablesDecls.begin(), variablesDecls.end());

	return res;
}

namespace Literals
{

ExpressionBase* addWithEvaluating(const LiteralExpression* firstArg, const LiteralExpression* secondArg, bool& overflow)
{
	overflow = false;
	//1. Оба аргумента StrictLiteralExpression
	if (firstArg->is_a<StrictLiteralExpression>() && secondArg->is_a<StrictLiteralExpression>())
	{
		const StrictLiteralExpression* firstArgStrict = firstArg->cast_ptr<const StrictLiteralExpression>();
		const StrictLiteralExpression* secondArgStrict = secondArg->cast_ptr<const StrictLiteralExpression>();
		ReprisePtr<TypeBase> firstArgType(firstArgStrict->getResultType());
		ReprisePtr<TypeBase> secondArgType(secondArgStrict ->getResultType());
		TypeBase* resultType = 0;
		if (couldBeImplicitlyConverted(firstArgType.get(), secondArgType.get()))
			resultType = firstArgType.get();

		if (couldBeImplicitlyConverted(secondArgType.get(), firstArgType.get()))
			resultType = secondArgType.get();

		if (!resultType)
			return 0;

		if (resultType->is_a<BasicType>())
			// Здесь предполагается, что программа аргументы сложения имеют одинаковый тип и он совпадает с типом результата
			switch (resultType->cast_ptr<BasicType>()->getKind())
			{
			case BasicType::BT_BOOLEAN: 
				return StrictLiteralExpression::createBoolean(firstArgStrict->getBoolean() + secondArgStrict->getBoolean());
			case BasicType::BT_FLOAT32:
				return StrictLiteralExpression::createFloat32(firstArgStrict->getFloat32() + secondArgStrict->getFloat32());
			case BasicType::BT_FLOAT64:
				return StrictLiteralExpression::createFloat64(firstArgStrict->getFloat64() + secondArgStrict->getFloat64());
			case BasicType::BT_INT16:
				return StrictLiteralExpression::createInt16(firstArgStrict->getInt16() + secondArgStrict->getInt16());
			case BasicType::BT_INT32:
				return StrictLiteralExpression::createInt32(firstArgStrict->getInt32() + secondArgStrict->getInt32());
			case BasicType::BT_INT64:
				return StrictLiteralExpression::createInt64(firstArgStrict->getInt64() + secondArgStrict->getInt64());
			case BasicType::BT_INT8:
				return StrictLiteralExpression::createInt8(firstArgStrict->getInt8() + secondArgStrict->getInt8());
			case BasicType::BT_UINT16:
				return StrictLiteralExpression::createUInt16(firstArgStrict->getUInt16() + secondArgStrict->getUInt16());
			case BasicType::BT_UINT32:
				return StrictLiteralExpression::createUInt32(firstArgStrict->getUInt32() + secondArgStrict->getUInt32());
			case BasicType::BT_UINT64:
				return StrictLiteralExpression::createUInt64(firstArgStrict->getUInt64() + secondArgStrict->getUInt64());
			case BasicType::BT_UINT8:
				return StrictLiteralExpression::createUInt8(firstArgStrict->getUInt8() + secondArgStrict->getUInt8());
			case BasicType::BT_STRING:
				return StrictLiteralExpression::createString(firstArgStrict->getString() + secondArgStrict->getString());
			case BasicType::BT_WIDE_STRING:
				return StrictLiteralExpression::createWideString(firstArgStrict->getWideString() + secondArgStrict->getWideString());
			case BasicType::BT_WIDE_CHAR: //TODO: данные какого типа при сложении дают такой результат?
			case BasicType::BT_CHAR: //TODO: данные какого типа при сложении дают такой результат?
			default: // BasicType::BT_VOID, BasicType::BT_UNDEFINED - попадают в эту ветку 
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("addWithEvaluating(...)");
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Unexpected adding result type found!",""));
					return 0;
				}
				
			}
		// TODO: else приводится могут не только базовые типы
			
	}

	//2. Оба аргумента BasicLiteralExpression
	if (firstArg->is_a<BasicLiteralExpression>() && secondArg->is_a<BasicLiteralExpression>())
	{
		const BasicLiteralExpression* firstArgBasic = firstArg->cast_ptr<const BasicLiteralExpression>();
		const BasicLiteralExpression* secondArgBasic = secondArg->cast_ptr<const BasicLiteralExpression>();

		if (firstArgBasic->getLiteralType() == secondArgBasic->getLiteralType())
		{
			switch (firstArgBasic->getLiteralType())
			{
			case BasicLiteralExpression::LT_INTEGER:
				{
					long_long_t resultValue = firstArgBasic->getInteger() + secondArgBasic->getInteger();
					return BasicLiteralExpression::createInteger(resultValue);
				}
			case BasicLiteralExpression::LT_UNSIGNED_INTEGER:
				{
					unsigned_long_long_t resultValue = firstArgBasic->getUnsignedInteger() + secondArgBasic->getUnsignedInteger();
					return BasicLiteralExpression::createUnsignedInteger(resultValue);
				}
			case BasicLiteralExpression::LT_FLOAT:
				{
					double resultValue = firstArgBasic->getFloat() + secondArgBasic->getFloat();
					return BasicLiteralExpression::createFloat(resultValue);
				}
			case BasicLiteralExpression::LT_STRING:
					return BasicLiteralExpression::createString(firstArgBasic->getString() + secondArgBasic->getString());
			case BasicLiteralExpression::LT_WIDE_STRING:
					return BasicLiteralExpression::createWideString(firstArgBasic->getWideString() + secondArgBasic->getWideString());
			case BasicLiteralExpression::LT_CHAR:
					//return BasicLiteralExpression::createString(Strings::format("%c", firstArgBasic->getChar() + secondArgBasic->getChar()));
			case BasicLiteralExpression::LT_WIDE_CHAR:
					//return BasicLiteralExpression::createWideString(Strings::format(L"%C", firstArgBasic->getWideChar() + secondArgBasic->getWideChar()));
			default:
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("addWithEvaluating(...)");
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Unexpected subtraction result type found!",""));
					return 0;
				}
			}
		}
	}

	//3. Сложение составных типов не поддерживается
	if (firstArg->cast_ptr<CompoundLiteralExpression>() || secondArg->cast_ptr<CompoundLiteralExpression>())
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole("addWithEvaluating(...)");
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Incorrect semantics of adding operation found!",""));
		return 0;
	}

	//4. Если был не стандартный случай сложения литералов, то мы возвращаем просто ExpressionBase* в виде дерева выражения
	return &((*firstArg->clone()) + (*secondArg->clone()));
}

ExpressionBase* subtractWithEvaluating(const LiteralExpression* firstArg, const LiteralExpression* secondArg, bool& overflow)
{
	overflow = false;
	//1. Оба аргумента StrictLiteralExpression
	if (firstArg->is_a<StrictLiteralExpression>() && secondArg->is_a<StrictLiteralExpression>())
	{
		const StrictLiteralExpression* firstArgStrict = firstArg->cast_ptr<const StrictLiteralExpression>();
		const StrictLiteralExpression* secondArgStrict = secondArg->cast_ptr<const StrictLiteralExpression>();
		ReprisePtr<TypeBase> firstArgType(firstArgStrict->getResultType());
		ReprisePtr<TypeBase> secondArgType(secondArgStrict->getResultType());
		TypeBase* resultType = 0;

		if (couldBeImplicitlyConverted(firstArgType.get(), secondArgType.get()))
			resultType = firstArgType.get();

		if (couldBeImplicitlyConverted(secondArgType.get(), firstArgType.get()))
			resultType = secondArgType.get();

		if (resultType != 0)
		{	// типы приводимы
			if (resultType->is_a<BasicType>())
				// Здесь предполагается, что программа аргументы сложения имеют одинаковый тип и он совпадает с типом результата
				switch (resultType->cast_ptr<BasicType>()->getKind())
			{
				case BasicType::BT_BOOLEAN: 
					return StrictLiteralExpression::createBoolean(firstArgStrict->getBoolean() - secondArgStrict->getBoolean());
				case BasicType::BT_FLOAT32:
					return StrictLiteralExpression::createFloat32(firstArgStrict->getFloat32() - secondArgStrict->getFloat32());
				case BasicType::BT_FLOAT64:
					return StrictLiteralExpression::createFloat64(firstArgStrict->getFloat64() - secondArgStrict->getFloat64());
				case BasicType::BT_INT16:
					return StrictLiteralExpression::createInt16(firstArgStrict->getInt16() - secondArgStrict->getInt16());
				case BasicType::BT_INT32:
					return StrictLiteralExpression::createInt32(firstArgStrict->getInt32() - secondArgStrict->getInt32());
				case BasicType::BT_INT64:
					return StrictLiteralExpression::createInt64(firstArgStrict->getInt64() - secondArgStrict->getInt64());
				case BasicType::BT_INT8:
					return StrictLiteralExpression::createInt8(firstArgStrict->getInt8() - secondArgStrict->getInt8());
				case BasicType::BT_UINT16:
					return StrictLiteralExpression::createUInt16(firstArgStrict->getUInt16() - secondArgStrict->getUInt16());
				case BasicType::BT_UINT32:
					return StrictLiteralExpression::createUInt32(firstArgStrict->getUInt32() - secondArgStrict->getUInt32());
				case BasicType::BT_UINT64:
					return StrictLiteralExpression::createUInt64(firstArgStrict->getUInt64() - secondArgStrict->getUInt64());
				case BasicType::BT_UINT8:
					return StrictLiteralExpression::createUInt8(firstArgStrict->getUInt8() - secondArgStrict->getUInt8());
				case BasicType::BT_WIDE_CHAR: //TODO: данные какого типа при сложении дают такой результат?
				case BasicType::BT_CHAR: //TODO: данные какого типа при сложении дают такой результат?
				case BasicType::BT_STRING:
				case BasicType::BT_WIDE_STRING:
				default: // BasicType::BT_VOID, BasicType::BT_UNDEFINED - попадают в эту ветку 
					{
						OPS::Console* const pConsole = &OPS::getOutputConsole("subtractWithEvaluating(...)");
						pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Unexpected subtraction result type found!",""));
						return 0;
					}

			}
			// TODO: else приводится могут не только базовые типы
		}
	}

	//2. Оба аргумента BasicLiteralExpression
	if (firstArg->is_a<BasicLiteralExpression>() && secondArg->is_a<BasicLiteralExpression>())
	{
		const BasicLiteralExpression* firstArgBasic = firstArg->cast_ptr<const BasicLiteralExpression>();
		const BasicLiteralExpression* secondArgBasic = secondArg->cast_ptr<const BasicLiteralExpression>();

		if (firstArgBasic->getLiteralType() == secondArgBasic->getLiteralType())
		{
			switch (firstArgBasic->getLiteralType())
			{
			case BasicLiteralExpression::LT_INTEGER:
				{
					long_long_t resultValue = firstArgBasic->getInteger() - secondArgBasic->getInteger();
					return BasicLiteralExpression::createInteger(resultValue);
				}
			case BasicLiteralExpression::LT_UNSIGNED_INTEGER:
				{
					unsigned_long_long_t resultValue = firstArgBasic->getUnsignedInteger() - secondArgBasic->getUnsignedInteger();
					return BasicLiteralExpression::createUnsignedInteger(resultValue);
				}
			case BasicLiteralExpression::LT_FLOAT:
				{
					double resultValue = firstArgBasic->getFloat() - secondArgBasic->getFloat();
					return BasicLiteralExpression::createFloat(resultValue);
				}
			case BasicLiteralExpression::LT_CHAR: //TODO: данные какого типа при сложении дают такой результат?
			case BasicLiteralExpression::LT_STRING:
			case BasicLiteralExpression::LT_WIDE_CHAR: //TODO: данные какого типа при сложении дают такой результат?
			case BasicLiteralExpression::LT_WIDE_STRING:
			default:
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("subtractWithEvaluating(...)");
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Unexpected subtraction result type found!",""));
					return 0;
				}
			}
		}
	}

	//3. Сложение составных типов не поддерживается
	if (firstArg->cast_ptr<CompoundLiteralExpression>() || secondArg->cast_ptr<CompoundLiteralExpression>())
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole("addWithEvaluating(...)");
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Incorrect semantics of adding operation found!",""));
		return 0;
	}

	//4. Если был не стандартный случай сложения литералов, то мы возвращаем просто ExpressionBase* в виде дерева выражения
	return &((*firstArg->clone()) - (*secondArg->clone()));
}

ExpressionBase* multiplyWithEvaluating(const LiteralExpression* firstArg, const LiteralExpression* secondArg, bool& overflow)
{
	overflow = false;
	//1. Оба аргумента StrictLiteralExpression
	if (firstArg->is_a<StrictLiteralExpression>() && secondArg->is_a<StrictLiteralExpression>())
	{
		const StrictLiteralExpression* firstArgStrict = firstArg->cast_ptr<const StrictLiteralExpression>();
		const StrictLiteralExpression* secondArgStrict = secondArg->cast_ptr<const StrictLiteralExpression>();
		ReprisePtr<TypeBase> firstArgType(firstArgStrict->getResultType());
		ReprisePtr<TypeBase> secondArgType(secondArgStrict->getResultType());
		TypeBase* resultType = 0;
		if (couldBeImplicitlyConverted(firstArgType.get(), secondArgType.get()))
			resultType = firstArgType.get();

		if (couldBeImplicitlyConverted(secondArgType.get(), firstArgType.get()))
			resultType = secondArgType.get();

		if (!resultType)
			return 0;

		if (resultType->is_a<BasicType>())
			// Здесь предполагается, что программа аргументы сложения имеют одинаковый тип и он совпадает с типом результата
			switch (resultType->cast_ptr<BasicType>()->getKind())
			{
			case BasicType::BT_BOOLEAN:
				// Результат умножения не может быть bool, а только int см 6.5.5 стандарта С99. Но у нас пока нет норм приведения типов. // TODO: исправить
				return StrictLiteralExpression::createInt8(firstArgStrict->getBoolean() * secondArgStrict->getBoolean());
			case BasicType::BT_FLOAT32:
				return StrictLiteralExpression::createFloat32(firstArgStrict->getFloat32() * secondArgStrict->getFloat32());
			case BasicType::BT_FLOAT64:
				return StrictLiteralExpression::createFloat64(firstArgStrict->getFloat64() * secondArgStrict->getFloat64());
			case BasicType::BT_INT16:
				return StrictLiteralExpression::createInt16(firstArgStrict->getInt16() * secondArgStrict->getInt16());
			case BasicType::BT_INT32:
				return StrictLiteralExpression::createInt32(firstArgStrict->getInt32() * secondArgStrict->getInt32());
			case BasicType::BT_INT64:
				return StrictLiteralExpression::createInt64(firstArgStrict->getInt64() * secondArgStrict->getInt64());
			case BasicType::BT_INT8:
				return StrictLiteralExpression::createInt8(firstArgStrict->getInt8() * secondArgStrict->getInt8());
			case BasicType::BT_UINT16:
				return StrictLiteralExpression::createUInt16(firstArgStrict->getUInt16() * secondArgStrict->getUInt16());
			case BasicType::BT_UINT32:
				return StrictLiteralExpression::createUInt32(firstArgStrict->getUInt32() * secondArgStrict->getUInt32());
			case BasicType::BT_UINT64:
				return StrictLiteralExpression::createUInt64(firstArgStrict->getUInt64() * secondArgStrict->getUInt64());
			case BasicType::BT_UINT8:
				return StrictLiteralExpression::createUInt8(firstArgStrict->getUInt8() * secondArgStrict->getUInt8());
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("multiplyWithEvaluating(...)");
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Illegal use of binary '*' operator!",""));
					return 0;
				}
			case BasicType::BT_WIDE_CHAR: //TODO: данные какого типа при сложении дают такой результат?
			case BasicType::BT_CHAR: //TODO: данные какого типа при сложении дают такой результат?
			default: // BasicType::BT_VOID, BasicType::BT_UNDEFINED - попадают в эту ветку 
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("addWithEvaluating(...)");
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Unexpected adding result type found!",""));
					return 0;
				}
			}
		// TODO: else приводится могут не только базовые типы
	}

	//2. Оба аргумента BasicLiteralExpression
	if (firstArg->is_a<BasicLiteralExpression>() && secondArg->is_a<BasicLiteralExpression>())
	{
		const BasicLiteralExpression* firstArgBasic = firstArg->cast_ptr<const BasicLiteralExpression>();
		const BasicLiteralExpression* secondArgBasic = secondArg->cast_ptr<const BasicLiteralExpression>();

		if (firstArgBasic->getLiteralType() == secondArgBasic->getLiteralType())
		{
			switch (firstArgBasic->getLiteralType())
			{
			case BasicLiteralExpression::LT_INTEGER:
				{
					long_long_t resultValue = firstArgBasic->getInteger() * secondArgBasic->getInteger();
					return BasicLiteralExpression::createInteger(resultValue);
				}
			case BasicLiteralExpression::LT_UNSIGNED_INTEGER:
				{
					unsigned_long_long_t resultValue = firstArgBasic->getUnsignedInteger() * secondArgBasic->getUnsignedInteger();
					return BasicLiteralExpression::createUnsignedInteger(resultValue);
				}
			case BasicLiteralExpression::LT_FLOAT:
				{
					double resultValue = firstArgBasic->getFloat() * secondArgBasic->getFloat();
					return BasicLiteralExpression::createFloat(resultValue);
				}
			case BasicLiteralExpression::LT_CHAR:
				{
					// Произведение двух char-ов, это int (см 6.5.5 стандарта С99). Но при введении норм приведения типов здесь надо исправить // TODO: исправить
					return BasicLiteralExpression::createInteger(firstArgBasic->getChar() * secondArgBasic->getChar());				
				}
			case BasicLiteralExpression::LT_WIDE_CHAR:
				{
					// аналогично LT_CHAR
					return BasicLiteralExpression::createInteger(firstArgBasic->getWideChar() * secondArgBasic->getWideChar());				
				}
			case BasicLiteralExpression::LT_WIDE_STRING:
			case BasicLiteralExpression::LT_STRING:
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("multiplyWithEvaluating(...)");
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Illegal use of binary '*' operator (string arguments)!",""));
					return 0;
				}
			default:
				break;
			}
		}
	}

    //3. Умножение составных типов не поддерживается по стандарту if С99
	if (firstArg->cast_ptr<CompoundLiteralExpression>() || secondArg->cast_ptr<CompoundLiteralExpression>())
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole("multiplyWithEvaluating(...)");
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Incorrect semantics of adding operation found!",""));
		return 0;
	}

	//4. Если был не стандартный случай сложения литералов, то мы возвращаем просто ExpressionBase* в виде дерева выражения
	return &((*firstArg->clone()) * (*secondArg->clone()));
}

ExpressionBase* multiplyWithLiteralSimplification(const ExpressionBase* firstArg, const ExpressionBase* secondArg, bool& overflow)
{
	IntegerHelper ih(BasicType::BT_INT32);
	SimpleLiterals firstArgSimplicity = specifySimplicityOfExpression(firstArg);
	SimpleLiterals secondArgSimplicity = specifySimplicityOfExpression(secondArg);
	if ((firstArgSimplicity == SL_ZERO) || (secondArgSimplicity == SL_ZERO))
		return &ih(0);

	if (firstArgSimplicity == SL_POSITIVE_ONE)
		return secondArg->clone();

	if (secondArgSimplicity == SL_POSITIVE_ONE)
		return firstArg->clone();

	if (firstArgSimplicity == SL_NEGATIVE_ONE)
	{
		if (secondArg->is_a<LiteralExpression>())
			return getOpposite(secondArg->cast_ptr<LiteralExpression>());
		else
			return &(-op(secondArg));
	}

	if (secondArgSimplicity == SL_NEGATIVE_ONE)
	{
		if (firstArg->is_a<LiteralExpression>())
			return getOpposite(firstArg->cast_ptr<LiteralExpression>());
		else
			return &(-op(firstArg));
	}

	if ((firstArg->is_a<LiteralExpression>()) && (secondArg->is_a<LiteralExpression>()))
		return multiplyWithEvaluating(firstArg->cast_ptr<LiteralExpression>(), secondArg->cast_ptr<LiteralExpression>(), overflow);
	else
		return &(op(firstArg) * op(secondArg));
}

ExpressionBase* divideWithEvaluating(const LiteralExpression* firstArg, const LiteralExpression* secondArg, bool& overflow)
{
	overflow = false;
	//1. Оба аргумента StrictLiteralExpression
	if (firstArg->is_a<StrictLiteralExpression>() && secondArg->is_a<StrictLiteralExpression>())
	{
		const StrictLiteralExpression* firstArgStrict = firstArg->cast_ptr<const StrictLiteralExpression>();
		const StrictLiteralExpression* secondArgStrict = secondArg->cast_ptr<const StrictLiteralExpression>();
		ReprisePtr<TypeBase> firstArgType(firstArgStrict->getResultType());
		ReprisePtr<TypeBase> secondArgType(secondArgStrict->getResultType());
		TypeBase* resultType = 0;
		if (couldBeImplicitlyConverted(firstArgType.get(), secondArgType.get()))
			resultType = firstArgType.get();

		if (couldBeImplicitlyConverted(secondArgType.get(), firstArgType.get()))
			resultType = secondArgType.get();

		if (!resultType)
			return 0;

		if (resultType->is_a<BasicType>())
		{
			// Здесь предполагается, что аргументы сложения имеют одинаковый тип и он совпадает с типом результата // TODO: усовершенствовать
			switch (resultType->cast_ptr<BasicType>()->getKind())
			{
			case BasicType::BT_BOOLEAN: 
				{
					// Результат деления не может быть bool, а только int см 6.5.5 стандарта С99. Но у нас пока нет норм приведения типов. // TODO: исправить
					if (secondArgStrict->getBoolean())
						return StrictLiteralExpression::createInt8(firstArgStrict->getBoolean() / secondArgStrict->getBoolean());
					break;
				}
			case BasicType::BT_FLOAT32:
				{
					if (secondArgStrict->getFloat32())
						return StrictLiteralExpression::createFloat32(firstArgStrict->getFloat32() / secondArgStrict->getFloat32());
					break;
				}
			case BasicType::BT_FLOAT64:
				{
					if (secondArgStrict->getFloat64())
						return StrictLiteralExpression::createFloat64(firstArgStrict->getFloat64() / secondArgStrict->getFloat64());
					break;
				}
			case BasicType::BT_INT16:
				{
					if (secondArgStrict->getInt16())
						return StrictLiteralExpression::createInt16(firstArgStrict->getInt16() / secondArgStrict->getInt16());
					break;
				}
			case BasicType::BT_INT32:
				{
					if (secondArgStrict->getInt32())
						return StrictLiteralExpression::createInt32(firstArgStrict->getInt32() / secondArgStrict->getInt32());
					break;
				}
			case BasicType::BT_INT64:
				{
					if (secondArgStrict->getInt64())
						return StrictLiteralExpression::createInt64(firstArgStrict->getInt64() / secondArgStrict->getInt64());
					break;
				}
			case BasicType::BT_INT8:
				{
					if (secondArgStrict->getInt8())
						return StrictLiteralExpression::createInt8(firstArgStrict->getInt8() / secondArgStrict->getInt8());
					break;
				}
			case BasicType::BT_UINT16:
				{
					if (secondArgStrict->getUInt16())
						return StrictLiteralExpression::createUInt16(firstArgStrict->getUInt16() / secondArgStrict->getUInt16());
					break;
				}
			case BasicType::BT_UINT32:
				{
					if (secondArgStrict->getUInt32())
						return StrictLiteralExpression::createUInt32(firstArgStrict->getUInt32() / secondArgStrict->getUInt32());
					break;
				}
			case BasicType::BT_UINT64:
				{
					if (secondArgStrict->getUInt64())
						return StrictLiteralExpression::createUInt64(firstArgStrict->getUInt64() / secondArgStrict->getUInt64());
					break;
				}
			case BasicType::BT_UINT8:
				{
					if (secondArgStrict->getUInt8())
						return StrictLiteralExpression::createUInt8(firstArgStrict->getUInt8() / secondArgStrict->getUInt8());
					break;
				}
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("divideWithEvaluating(...)");
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Illegal use of binary '/' operator (string arguments)!",""));
					return 0;
				}
			case BasicType::BT_WIDE_CHAR: //TODO: данные какого типа при сложении дают такой результат?
			case BasicType::BT_CHAR: //TODO: данные какого типа при сложении дают такой результат?
			default: // BasicType::BT_VOID, BasicType::BT_UNDEFINED - попадают в эту ветку 
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("addWithEvaluating(...)");
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Unexpected adding result type found!",""));
					return 0;
				}
				
			}
			// Возникло деление на 0, следовательно, мы должны выдать выражение, а не вычисленный результат
			return &((*firstArg->clone()) / (*secondArg->clone()));
		}
		// TODO: else приводится могут не только базовые типы
			
	}

	//2. Оба аргумента BasicLiteralExpression
	if (firstArg->is_a<BasicLiteralExpression>() && secondArg->is_a<BasicLiteralExpression>())
	{
		const BasicLiteralExpression* firstArgBasic = firstArg->cast_ptr<const BasicLiteralExpression>();
		const BasicLiteralExpression* secondArgBasic = secondArg->cast_ptr<const BasicLiteralExpression>();

		if (firstArgBasic->getLiteralType() == secondArgBasic->getLiteralType())
		{
			switch (firstArgBasic->getLiteralType())
			{
			case BasicLiteralExpression::LT_INTEGER:
				{
					if (secondArgBasic->getInteger())
						return BasicLiteralExpression::createInteger(firstArgBasic->getInteger() / secondArgBasic->getInteger());
					break;
				}
			case BasicLiteralExpression::LT_UNSIGNED_INTEGER:
				{
					if (secondArgBasic->getInteger())
						return BasicLiteralExpression::createUnsignedInteger(firstArgBasic->getUnsignedInteger() / secondArgBasic->getUnsignedInteger());
					break;
				}
			case BasicLiteralExpression::LT_FLOAT:
				{
					if (secondArgBasic->getFloat())
						return BasicLiteralExpression::createFloat(firstArgBasic->getFloat() / secondArgBasic->getFloat());
					break;
				}
			case BasicLiteralExpression::LT_CHAR:
				{
					// Деление двух char-ов, это int (см 6.5.5 стандарта С99). Но при введении норм приведения типов здесь надо исправить // TODO: исправить
					if (secondArgBasic->getChar())
						return BasicLiteralExpression::createInteger(firstArgBasic->getChar() / secondArgBasic->getChar());				
					break;
				}
			case BasicLiteralExpression::LT_WIDE_CHAR:
				{
					// см. LT_CHAR
					if (secondArgBasic->getWideChar())
						return BasicLiteralExpression::createInteger(firstArgBasic->getWideChar() / secondArgBasic->getWideChar());
					break;
				}
			case BasicLiteralExpression::LT_WIDE_STRING:
			case BasicLiteralExpression::LT_STRING:
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("multiplyWithEvaluating(...)");
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Illegal use of binary '*' operator!",""));
					return 0;
				}
			default:
				break;
			}
			// Возникло деление на 0, следовательно, мы должны выдать выражение, а не вычисленный результат,
			// либо вариант default означающий, что тип литерала не известен
			return &((*firstArg->clone()) / (*secondArg->clone()));
		}
	}

	//3. Деление составных типов не поддерживается по стандарту С99
	if (firstArg->cast_ptr<CompoundLiteralExpression>() || secondArg->cast_ptr<CompoundLiteralExpression>())
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole("addWithEvaluating(...)");
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Incorrect semantics of adding operation found!",""));
		return 0;
	}

	//4. Если был не стандартный случай сложения литералов, то мы возвращаем просто ExpressionBase* в виде дерева выражения
	return &((*firstArg->clone()) + (*secondArg->clone()));
}

Reprise::ExpressionBase* getOpposite(const Reprise::LiteralExpression* arg)
{
	Reprise::ExpressionBase* result = 0;
	// Константы умножаем на -1, преобразуя их в отрицательное число
	if (arg->is_a<BasicLiteralExpression>())
	{
		const BasicLiteralExpression& BasicLiteral = arg->cast_to<BasicLiteralExpression>();
		switch(BasicLiteral.getLiteralType())
		{
		case BasicLiteralExpression::LT_BOOLEAN:
		case BasicLiteralExpression::LT_CHAR:
		case BasicLiteralExpression::LT_FLOAT:
			break;
		case BasicLiteralExpression::LT_INTEGER:
			{
				result = BasicLiteralExpression::createInteger(-1*BasicLiteral.getInteger());
				break;
			}
		case BasicLiteralExpression::LT_UNSIGNED_INTEGER:
			// TODO: прописать
		case BasicLiteralExpression::LT_WIDE_CHAR:
			{
				break;
			}
		default:
			{
				OPS::Console* const pConsole = &OPS::getOutputConsole("ParametricLinearExpression");
				pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Incorrect multiplication operation!",""));
				throw RuntimeError("Incorrect multiplication operation!");
			}
		
		}
	}

	if (arg->is_a<StrictLiteralExpression>())
	{
		const StrictLiteralExpression& StrictLiteral = arg->cast_to<const StrictLiteralExpression>();
		switch(StrictLiteral.getLiteralType())
		{
		case BasicType::BT_BOOLEAN:
		case BasicType::BT_CHAR:
		case BasicType::BT_WIDE_CHAR:
		case BasicType::BT_FLOAT32:
		case BasicType::BT_FLOAT64:
			break;
		case BasicType::BT_INT16:
			{
				result = StrictLiteralExpression::createInt16(-1*StrictLiteral.getInt16());
				break;
			}
		case BasicType::BT_INT32:
			{
				result = StrictLiteralExpression::createInt32(-1*StrictLiteral.getInt32());
				break;
			}
		case BasicType::BT_INT64:
			{
				result = StrictLiteralExpression::createInt64(-1*StrictLiteral.getInt64());
				break;
			}
		case BasicType::BT_INT8:
			{
				result = StrictLiteralExpression::createInt8(-1*StrictLiteral.getInt8());
				break;
			}
		case BasicType::BT_UINT16:
		case BasicType::BT_UINT32:
		case BasicType::BT_UINT64:
		case BasicType::BT_UINT8:
		default:				
			{
				OPS::Console* const pConsole = &OPS::getOutputConsole("ParametricLinearExpression");
				pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Incorrect multiplication operation!",""));
				throw RuntimeError("Incorrect multiplication operation!");
			}
		}
		
	}

	if (result == 0)
		result = &(- (*arg->clone()));
		//result = new BasicCallExpression(BasicCallExpression::BCK_UNARY_MINUS, arg->clone());

	return result;
}

bool couldBeImplicitlyConverted(TypeBase* MainType, TypeBase* TypeToConvert)
{ 
	// TODO:  сделать грамотное описание проверки возможности преобразования типа TypeToConvert к типу MainType
	if (MainType->is_a<BasicType>() && TypeToConvert->is_a<BasicType>())
	{
		BasicType* MainTypeBasic = MainType->cast_ptr<BasicType>();
		BasicType* TypeToConvertBasic = TypeToConvert->cast_ptr<BasicType>();
		if ((MainTypeBasic->getKind() != BasicType::BT_CHAR) && (MainTypeBasic->getKind() != BasicType::BT_WIDE_CHAR)) // результат сложения char м.б. string или char, и w_char аналогично. // TODO: разобраться
		{
			if ((MainTypeBasic->getKind() == BasicType::BT_INT64) && (TypeToConvertBasic->getKind() == BasicType::BT_INT32))
				return true; // заплатка, чтобы вообще работали целочисленные вычисления в LinearExpressions //BasicType::BT_INT64; 
			return (MainTypeBasic->getKind() == TypeToConvertBasic->getKind());
		}
	}
	return false;
}

bool isEqualToIntValue(const ExpressionBase* ExpressionToInvestigate, int Value)
{
    if (ExpressionToInvestigate->is_a<BasicLiteralExpression>())
    {
        const BasicLiteralExpression* Expression = ExpressionToInvestigate->cast_ptr<const BasicLiteralExpression>();
        if (Expression->getLiteralType() == BasicLiteralExpression::LT_INTEGER)
            if (Expression->getInteger() == Value)
                return true;

        if (Expression->getLiteralType() == BasicLiteralExpression::LT_FLOAT)
            if (Expression->getFloat() == Value)
                return true;
    }
    if (ExpressionToInvestigate->is_a<StrictLiteralExpression>())
    {
        const StrictLiteralExpression* Expression = ExpressionToInvestigate->cast_ptr<const StrictLiteralExpression>();
        switch(Expression->getLiteralType())
        {
        case BasicType::BT_INT8:
            if (Expression->getInt8() == Value)
                return true;
            break;
        case BasicType::BT_INT16:
            if (Expression->getInt16() == Value)
                return true;
            break;
        case BasicType::BT_INT32:
            if (Expression->getInt32() == Value)
                return true;
            break;
        case BasicType::BT_INT64:
            if (Expression->getInt64() == Value)
                return true;
            break;
        case BasicType::BT_UINT8:
            if (Expression->getUInt8() == Value)
                return true;
            break;
        case BasicType::BT_UINT16:
            if (Expression->getUInt16() == Value)
                return true;
            break;
        case BasicType::BT_UINT32:
            if (Expression->getUInt32() == Value)
                return true;
            break;
        case BasicType::BT_UINT64:
            if (Expression->getUInt64() == Value)
                return true;
            break;

        case BasicType::BT_FLOAT32:
            if (Expression->getFloat32() == Value)
                return true;
            break;
        case BasicType::BT_FLOAT64:
            if (Expression->getFloat64() == Value)
                return true;
            break;
        default: ;
        }
    }
    return false;
}

SimpleLiterals specifySimplicityOfExpression(const ExpressionBase* expression)
{
    if (isEqualToIntValue(expression, 0))
        return SL_ZERO;
    if (isEqualToIntValue(expression, 1))
        return SL_POSITIVE_ONE;
    if (isEqualToIntValue(expression, -1))
        return SL_NEGATIVE_ONE;

    return SL_NOT_SIMPLE;
}

} // namespace Literals

OPS::Reprise::VariableDeclaration* getArrayVariableDeclaration( ExpressionBase& expression )
{
	if(expression.is_a<ReferenceExpression>())
	{
		return &expression.cast_to<ReferenceExpression>().getReference();
	}

	BasicCallExpression* pCallExpression = expression.cast_ptr<BasicCallExpression>();
	if(pCallExpression != NULL && pCallExpression->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
	{
		return getArrayVariableDeclaration(pCallExpression->getArgument(0));
	}

	return NULL;
}

OPS::Reprise::TranslationUnit* getTranslationUnit(OPS::Reprise::RepriseBase* repriseBase)
{
	while (repriseBase && (!repriseBase->is_a<OPS::Reprise::TranslationUnit>()))
		repriseBase=repriseBase->getParent();
	if (repriseBase && (repriseBase->is_a<OPS::Reprise::TranslationUnit>()))
		return repriseBase->cast_ptr<OPS::Reprise::TranslationUnit>();
	return 0;
}

void CalculatorDeepWalker::visit(BasicCallExpression& Node)
{
	BasicType::BasicTypes ResultType = BasicType::BT_UNDEFINED;
	ReprisePtr<StrictLiteralExpression> Argument0, Argument1, Argument2;

	unsigned arity = getOperatorArity(Node.getKind());
	if (0<arity && arity<=3)
	{
		Node.getArgument(0).accept(*this);
		if (m_ResultStack.empty()) // вычисления невозможны
			return; 
		Argument0.reset(pop_last());
		// все операции на вход должны получать аргументы одного типа
		ResultType = Argument0->getLiteralType(); 
	}
	if (1<arity && arity<=3)
	{
		Node.getArgument(1).accept(*this);
		if (m_ResultStack.empty()) // вычисления невозможны
			return; 
		Argument1.reset(pop_last());
	}
	if (arity == 3)
	{
		Node.getArgument(2).accept(*this);
		if (m_ResultStack.empty()) // вычисления невозможны
			return; 
		Argument2.reset(pop_last());
		ResultType = Argument1->getLiteralType(); // для оператора ?:
	}
	if (arity<1 || arity>3)
	{
		m_ResultStack.clear();
		return;
	}

	switch(Node.getKind())
	{
		case BasicCallExpression::BCK_ARRAY_ACCESS:	
		case BasicCallExpression::BCK_ASSIGN: 
			m_ResultStack.clear();
			return;	
		case BasicCallExpression::BCK_BINARY_MINUS:	
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() - Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() - Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() - Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() - Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() - Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() - Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() - Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() - Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() - Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() - Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createFloat32
					(Argument0->getFloat32() - Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createFloat64
					(Argument0->getFloat64() - Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() - Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_BINARY_PLUS:
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() + Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() + Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() + Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() + Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() + Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() + Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() + Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() + Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() + Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() + Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createFloat32
					(Argument0->getFloat32() + Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createFloat64
					(Argument0->getFloat64() + Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() + Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
				push_result(StrictLiteralExpression::createString
					(Argument0->getString() + Argument1->getString()));
				return;
			case BasicType::BT_WIDE_STRING:
				push_result(StrictLiteralExpression::createWideString
					(Argument0->getWideString() + Argument1->getWideString()));
				return;
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_BITWISE_AND:		
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() & Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() & Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() & Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() & Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() & Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() & Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() & Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() & Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() & Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() & Argument1->getUInt64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() & Argument1->getBoolean()));
				return;
			case BasicType::BT_FLOAT32:
			case BasicType::BT_FLOAT64:
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_BITWISE_NOT:		
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar(~Argument0->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar(~Argument0->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8(~Argument0->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16(~Argument0->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32(~Argument0->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64(~Argument0->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8(~Argument0->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16(~Argument0->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32(~Argument0->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64(~Argument0->getUInt64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean(~Argument0->getBoolean()));
				return;
			case BasicType::BT_FLOAT32:
			case BasicType::BT_FLOAT64:
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_BITWISE_OR:		
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() | Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() | Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() | Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() | Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() | Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() | Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() | Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() | Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() | Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() | Argument1->getUInt64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() | Argument1->getBoolean()));
				return;
			case BasicType::BT_FLOAT32:
			case BasicType::BT_FLOAT64:
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_BITWISE_XOR:		
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() ^ Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() ^ Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() ^ Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() ^ Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() ^ Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() ^ Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() ^ Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() ^ Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() ^ Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() ^ Argument1->getUInt64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() ^ Argument1->getBoolean()));
				return;
			case BasicType::BT_FLOAT32:
			case BasicType::BT_FLOAT64:
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_CONDITIONAL:	
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getBoolean() ? Argument1->getChar() : Argument2->getChar() ));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getBoolean() ? Argument1->getWideChar() : Argument2->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getBoolean() ? Argument1->getInt8() : Argument2->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getBoolean() ? Argument1->getInt16() : Argument2->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getBoolean() ? Argument1->getInt32() : Argument2->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getBoolean() ? Argument1->getInt64() : Argument2->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getBoolean() ? Argument1->getUInt8() : Argument2->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getBoolean() ? Argument1->getUInt16() : Argument2->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getBoolean() ? Argument1->getUInt32() : Argument2->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getBoolean() ? Argument1->getUInt64() : Argument2->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createFloat32
					(Argument0->getBoolean() ? Argument1->getFloat32() : Argument2->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createFloat64
					(Argument0->getBoolean() ? Argument1->getFloat64() : Argument2->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() ? Argument1->getBoolean() : Argument2->getBoolean()));
				return;
			case BasicType::BT_STRING:
				push_result(StrictLiteralExpression::createString
					(Argument0->getBoolean() ? Argument1->getString() : Argument2->getString()));
				return;
			case BasicType::BT_WIDE_STRING:
				push_result(StrictLiteralExpression::createWideString
					(Argument0->getBoolean() ? Argument1->getWideString() : Argument2->getWideString()));
				return;
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		
		case BasicCallExpression::BCK_DIVISION:	
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() / Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() / Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() / Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() / Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() / Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() / Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() / Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() / Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() / Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() / Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createFloat32
					(Argument0->getFloat32() / Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createFloat64
					(Argument0->getFloat64() / Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() / Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_EQUAL:
			switch(ResultType) // здесь надо воспринимать ResultType как тип первого аргумента
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getChar() == Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideChar() == Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt8() == Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt16() == Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt32() == Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt64() == Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt8() == Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt16() == Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt32() == Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt64() == Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat32() == Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat64() == Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() == Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getString() == Argument1->getString()));
				return;
			case BasicType::BT_WIDE_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideString() == Argument1->getWideString()));
				return;
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_GREATER:
			switch(ResultType) // здесь надо воспринимать ResultType как тип первого аргумента
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getChar() > Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideChar() > Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt8() > Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt16() > Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt32() > Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt64() > Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt8() > Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt16() > Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt32() > Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt64() > Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat32() > Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat64() > Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() > Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getString() > Argument1->getString()));
				return;
			case BasicType::BT_WIDE_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideString() > Argument1->getWideString()));
				return;
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_GREATER_EQUAL:
			switch(ResultType) // здесь надо воспринимать ResultType как тип первого аргумента
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getChar() >= Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideChar() >= Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt8() >= Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt16() >= Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt32() >= Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt64() >= Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt8() >= Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt16() >= Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt32() >= Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt64() >= Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat32() >= Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat64() >= Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() >= Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getString() >= Argument1->getString()));
				return;
			case BasicType::BT_WIDE_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideString() >= Argument1->getWideString()));
				return;
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_INTEGER_DIVISION:	
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() / Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() / Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() / Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() / Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() / Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() / Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() / Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() / Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() / Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() / Argument1->getUInt64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() / Argument1->getBoolean()));
				return;
			case BasicType::BT_FLOAT32:
			case BasicType::BT_FLOAT64:
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_INTEGER_MOD:	
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() % Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() % Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() % Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() % Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() % Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() % Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() % Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() % Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() % Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() % Argument1->getUInt64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() % Argument1->getBoolean()));
				return;
			case BasicType::BT_FLOAT32:
			case BasicType::BT_FLOAT64:
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_LEFT_SHIFT:
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() << Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() << Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() << Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() << Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() << Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() << Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() << Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() << Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() << Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() << Argument1->getUInt64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() << Argument1->getBoolean()));
				return;
			case BasicType::BT_FLOAT32:
			case BasicType::BT_FLOAT64:
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_LESS:	
			switch(ResultType) // здесь надо воспринимать ResultType как тип первого аргумента
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getChar() < Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideChar() < Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt8() < Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt16() < Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt32() < Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt64() < Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt8() < Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt16() < Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt32() < Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt64() < Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat32() < Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat64() < Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() < Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getString() < Argument1->getString()));
				return;
			case BasicType::BT_WIDE_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideString() < Argument1->getWideString()));
				return;
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_LESS_EQUAL:
			switch(ResultType) // здесь надо воспринимать ResultType как тип первого аргумента
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getChar() <= Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideChar() <= Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt8() <= Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt16() <= Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt32() <= Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt64() <= Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt8() <= Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt16() <= Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt32() <= Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt64() <= Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat32() <= Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat64() <= Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() <= Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getString() <= Argument1->getString()));
				return;
			case BasicType::BT_WIDE_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideString() <= Argument1->getWideString()));
				return;
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_LOGICAL_AND:		
			switch(ResultType) // здесь надо воспринимать ResultType как тип первого аргумента
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getChar() && Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideChar() && Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt8() && Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt16() && Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt32() && Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt64() && Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt8() && Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt16() && Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt32() && Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt64() && Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat32() && Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat64() && Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() && Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_LOGICAL_NOT:
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getUInt64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean(!Argument0->getBoolean()));
				return;
			case BasicType::BT_FLOAT32:
			case BasicType::BT_FLOAT64:
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_LOGICAL_OR:		
			switch(ResultType) // здесь надо воспринимать ResultType как тип первого аргумента
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getChar() || Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideChar() || Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt8() || Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt16() || Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt32() || Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt64() || Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt8() || Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt16() || Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt32() || Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt64() || Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat32() || Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat64() || Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() || Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_MULTIPLY:
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() * Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() * Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() * Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() * Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() * Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() * Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() * Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() * Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() * Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() * Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createFloat32
					(Argument0->getFloat32() * Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createFloat64
					(Argument0->getFloat64() * Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() * Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_NOT_EQUAL:
			switch(ResultType) // здесь надо воспринимать ResultType как тип первого аргумента
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getChar() != Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideChar() != Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt8() != Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt16() != Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt32() != Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getInt64() != Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt8() != Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt16() != Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt32() != Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getUInt64() != Argument1->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat32() != Argument1->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getFloat64() != Argument1->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() != Argument1->getBoolean()));
				return;
			case BasicType::BT_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getString() != Argument1->getString()));
				return;
			case BasicType::BT_WIDE_STRING:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getWideString() != Argument1->getWideString()));
				return;
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_RIGHT_SHIFT:
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar
					(Argument0->getChar() >> Argument1->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar
					(Argument0->getWideChar() >> Argument1->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8
					(Argument0->getInt8() >> Argument1->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16
					(Argument0->getInt16() >> Argument1->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32
					(Argument0->getInt32() >> Argument1->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64
					(Argument0->getInt64() >> Argument1->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8
					(Argument0->getUInt8() >> Argument1->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16
					(Argument0->getUInt16() >> Argument1->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32
					(Argument0->getUInt32() >> Argument1->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64
					(Argument0->getUInt64() >> Argument1->getUInt64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean
					(Argument0->getBoolean() >> Argument1->getBoolean()));
				return;
			case BasicType::BT_FLOAT32:
			case BasicType::BT_FLOAT64:
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_UNARY_MINUS:
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar(-Argument0->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar(-Argument0->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8(-Argument0->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16(-Argument0->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32(-Argument0->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64(-Argument0->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8(-Argument0->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16(-Argument0->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32(-Argument0->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64(-Argument0->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createFloat32(-Argument0->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createFloat64(-Argument0->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean(-Argument0->getBoolean()));
				return;
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_UNARY_PLUS:	
			switch(ResultType)
			{
			case BasicType::BT_CHAR:
				push_result(StrictLiteralExpression::createChar(+Argument0->getChar()));
				return;
			case BasicType::BT_WIDE_CHAR:
				push_result(StrictLiteralExpression::createWideChar(+Argument0->getWideChar()));
				return;
			case BasicType::BT_INT8:
				push_result(StrictLiteralExpression::createInt8(+Argument0->getInt8()));
				return;
			case BasicType::BT_INT16:
				push_result(StrictLiteralExpression::createInt16(+Argument0->getInt16()));
				return;
			case BasicType::BT_INT32:
				push_result(StrictLiteralExpression::createInt32(+Argument0->getInt32()));
				return;
			case BasicType::BT_INT64:
				push_result(StrictLiteralExpression::createInt64(+Argument0->getInt64()));
				return;
			case BasicType::BT_UINT8:
				push_result(StrictLiteralExpression::createUInt8(+Argument0->getUInt8()));
				return;
			case BasicType::BT_UINT16:
				push_result(StrictLiteralExpression::createUInt16(+Argument0->getUInt16()));
				return;
			case BasicType::BT_UINT32:
				push_result(StrictLiteralExpression::createUInt32(+Argument0->getUInt32()));
				return;
			case BasicType::BT_UINT64:
				push_result(StrictLiteralExpression::createUInt64(+Argument0->getUInt64()));
				return;
			case BasicType::BT_FLOAT32:
				push_result(StrictLiteralExpression::createFloat32(+Argument0->getFloat32()));
				return;
			case BasicType::BT_FLOAT64:
				push_result(StrictLiteralExpression::createFloat64(+Argument0->getFloat64()));
				return;
			case BasicType::BT_BOOLEAN:
				push_result(StrictLiteralExpression::createBoolean(+Argument0->getBoolean()));
				return;
			case BasicType::BT_STRING:
			case BasicType::BT_WIDE_STRING:
			case BasicType::BT_UNDEFINED:
			case BasicType::BT_VOID:
			default:
				throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the operator!");
			}
		case BasicCallExpression::BCK_DE_REFERENCE:		
		case BasicCallExpression::BCK_SIZE_OF: // TODO: здесь видимо можно что-то сделать
		case BasicCallExpression::BCK_COMMA:
		case BasicCallExpression::BCK_TAKE_ADDRESS: 
			m_ResultStack.clear();
			return;	
		default: 
			return;

	}
}

void CalculatorDeepWalker::visit(TypeCastExpression& Node)
{
	if (!Node.getCastType().is_a<BasicType>())
	{
		m_ResultStack.clear();
		return;
	}

	// Вычисляем значение аргумента
	Node.getCastArgument().accept(*this);
	ReprisePtr<StrictLiteralExpression> Argument(pop_last());

	// Определяем тип к которому надо привести
	BasicType::BasicTypes ResultType = Node.getCastType().cast_to<BasicType>().getKind();
	switch(ResultType)
	{
	case BasicType::BT_CHAR:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createChar(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createChar(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createChar(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createChar(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createChar(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createChar(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createChar(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createChar(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createChar(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createChar(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createChar(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createChar(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createChar(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_WIDE_CHAR:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createWideChar(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createWideChar(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createWideChar(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createWideChar(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createWideChar(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createWideChar(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createWideChar(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createWideChar(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createWideChar(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createWideChar(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createWideChar(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createWideChar(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createWideChar(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_INT8:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createInt8(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createInt8(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createInt8(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createInt8(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createInt8(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createInt8(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createInt8(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createInt8(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createInt8(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createInt8(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createInt8(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createInt8(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createInt8(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_INT16:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createInt16(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createInt16(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createInt16(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createInt16(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createInt16(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createInt16(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createInt16(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createInt16(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createInt16(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createInt16(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createInt16(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createInt16(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createInt16(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_INT32:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createInt32(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createInt32(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createInt32(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createInt32(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createInt32(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createInt32(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createInt32(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createInt32(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createInt32(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createInt32(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createInt32(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createInt32(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createInt32(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_INT64:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createInt64(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createInt64(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createInt64(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createInt64(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createInt64(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createInt64(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createInt64(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createInt64(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createInt64(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createInt64(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createInt64(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createInt64(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createInt64(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_UINT8:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createUInt8(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createUInt8(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createUInt8(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createUInt8(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createUInt8(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createUInt8(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createUInt8(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createUInt8(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createUInt8(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createUInt8(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createUInt8(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createUInt8(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createUInt8(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_UINT16:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createUInt16(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createUInt16(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createUInt16(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createUInt16(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createUInt16(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createUInt16(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createUInt16(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createUInt16(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createUInt16(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createUInt16(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createUInt16(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createUInt16(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createUInt16(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_UINT32:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createUInt32(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createUInt32(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createUInt32(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createUInt32(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createUInt32(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createUInt32(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createUInt32(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createUInt32(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createUInt32(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createUInt32(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createUInt32(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createUInt32(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createUInt32(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_UINT64:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createUInt64(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createUInt64(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createUInt64(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createUInt64(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createUInt64(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createUInt64(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createUInt64(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createUInt64(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createUInt64(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createUInt64(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createUInt64(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createUInt64(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createUInt64(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_FLOAT32:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createFloat32(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createFloat32(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createFloat32(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createFloat32(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createFloat32(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createFloat32(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createFloat32(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createFloat32(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createFloat32(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createFloat32(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createFloat32(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createFloat32(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createFloat32(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_FLOAT64:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createFloat64(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createFloat64(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createFloat64(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createFloat64(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createFloat64(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createFloat64(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createFloat64(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createFloat64(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createFloat64(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createFloat64(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createFloat64(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createFloat64(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createFloat64(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_BOOLEAN:
		switch(Argument->getLiteralType())
		{
		case BasicType::BT_CHAR:
			push_result(StrictLiteralExpression::createBoolean(Argument->getChar()));
			return;
		case BasicType::BT_WIDE_CHAR:
			push_result(StrictLiteralExpression::createBoolean(Argument->getWideChar()));
			return;
		case BasicType::BT_INT8:
			push_result(StrictLiteralExpression::createBoolean(Argument->getInt8()));
			return;
		case BasicType::BT_INT16:
			push_result(StrictLiteralExpression::createBoolean(Argument->getInt16()));
			return;
		case BasicType::BT_INT32:
			push_result(StrictLiteralExpression::createBoolean(Argument->getInt32()));
			return;
		case BasicType::BT_INT64:
			push_result(StrictLiteralExpression::createBoolean(Argument->getInt64()));
			return;
		case BasicType::BT_UINT8:
			push_result(StrictLiteralExpression::createBoolean(Argument->getUInt8()));
			return;
		case BasicType::BT_UINT16:
			push_result(StrictLiteralExpression::createBoolean(Argument->getUInt16()));
			return;
		case BasicType::BT_UINT32:
			push_result(StrictLiteralExpression::createBoolean(Argument->getUInt32()));
			return;
		case BasicType::BT_UINT64:
			push_result(StrictLiteralExpression::createBoolean(Argument->getUInt64()));
			return;
		case BasicType::BT_FLOAT32:
			push_result(StrictLiteralExpression::createBoolean(Argument->getFloat32()));
			return;
		case BasicType::BT_FLOAT64:
			push_result(StrictLiteralExpression::createBoolean(Argument->getFloat64()));
			return;
		case BasicType::BT_BOOLEAN:
			push_result(StrictLiteralExpression::createBoolean(Argument->getBoolean()));
			return;
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
		case BasicType::BT_UNDEFINED:
		case BasicType::BT_VOID:
		default:
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
		}
	case BasicType::BT_STRING:
		if (Argument->getLiteralType() == BasicType::BT_STRING)
		{
			push_result(Argument.release());
			return;
		}
		else
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
	case BasicType::BT_WIDE_STRING:
		if (Argument->getLiteralType() == BasicType::BT_WIDE_STRING)
		{
			push_result(Argument.release());
			return;
		}
		else
			throw RepriseError("CalculatorDeepWalker: Argument type is not supported for the typecasting!");
	case BasicType::BT_UNDEFINED:
	case BasicType::BT_VOID:
	default:
		throw RepriseError("CalculatorDeepWalker: Casting type is not supported for the typecasting!");
	}

}


int getArrayAccessDepth( OPS::Reprise::ReferenceExpression& referenceExpression )
{
	RepriseBase* pParent = referenceExpression.getParent();

	int result = 0;

	while (pParent != NULL && pParent->is_a<BasicCallExpression>() && pParent->cast_ptr<BasicCallExpression>()->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
	{
		++result;
		pParent = pParent->cast_ptr<BasicCallExpression>()->getParent();
	}

	return result;
}

int getTypeSize(OPS::Reprise::TypeBase& typeBase )
{
	std::vector<int> limits;
	getArrayLimits(&typeBase, limits);

	int result = 1;

	for (std::vector<int>::size_type i = 0; i < limits.size(); ++i)
	{
		result = result * limits[i];
	}

	return result;
}

int getTypeDimension(TypeBase& typeBase )
{
	TypeBase* pTypeBase = &typeBase;
	int result = 0;

	while (pTypeBase->is_a<ArrayType>() || pTypeBase->is_a<PtrType>())
	{
		if (pTypeBase->is_a<ArrayType>())
		{
			ArrayType& arrayType = pTypeBase->cast_to<ArrayType>();
			pTypeBase = &(arrayType.getBaseType());
		}
		else
		{
			PtrType& ptrType = pTypeBase->cast_to<PtrType>();
			pTypeBase = &(ptrType.getPointedType());
		}
		result++;
	}

	return result;
}

Reprise::ReprisePtr<Reprise::ArrayType> getVectorizedArray(const Reprise::ArrayType* arrayToVectorize, int length)
{
	Reprise::ArrayType* result = NULL;
	Reprise::VectorType* resultBaseType = new Reprise::VectorType(length, arrayToVectorize->getBaseType().clone());
	if(arrayToVectorize->hasCountExpression())
	{
		Reprise::BasicCallExpression* divExpr = 
			new Reprise::BasicCallExpression(
				Reprise::BasicCallExpression::BCK_DIVISION, 
				arrayToVectorize->getCountExpression().clone(),
				Reprise::BasicLiteralExpression::createInteger(length)
			);
		result = new Reprise::ArrayType(divExpr, resultBaseType);	
	} else {
		result = new Reprise::ArrayType(arrayToVectorize->getElementCount()/length, resultBaseType);
	}
	return ReprisePtr<Reprise::ArrayType>(result);
}


}
}


