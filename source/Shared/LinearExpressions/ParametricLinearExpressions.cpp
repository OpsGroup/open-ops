#include "Shared/ParametricLinearExpressions.h"
#include "Shared/LoopShared.h"
#include "Shared/DataShared.h"
#include "OPS_Core/Strings.h"
#include "OPS_Core/Localization.h"
#include "Shared/LinearExpressions.h"
#include "Shared/ExpressionHelpers.h"

#include <memory>
#include <algorithm>
#include <cstdio>
#include <iostream>

#include "OPS_Core/msc_leakcheck.h" //контроль утечек памяти должен находиться в конце всех include !!!

using namespace OPS::Reprise;
using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Shared::Literals;
using namespace std;

namespace OPS
{
namespace Shared
{

CanonicalLinearExpression::CanonicalLinearExpression()
	: m_freeSummand(0) 
{ 
}

CanonicalLinearExpression::CanonicalLinearExpression(Coefficient Summand)
	: m_freeSummand(Summand) 
{ 
}

CanonicalLinearExpression::CanonicalLinearExpression(OPS::Reprise::VariableDeclaration* Summand, Coefficient coeff)
	: m_freeSummand(0) 
{ 
	add(Summand, coeff);
}

void CanonicalLinearExpression::clear()
{
	m_summands.clear();
	m_freeSummand = 0;
}

/// Прибавляет к текущему каноническому линейному выражению число
void CanonicalLinearExpression::add(Coefficient Summand)
{ 
	m_freeSummand += Summand; 
}

/// Прибавляет к текущему каноническому линейному выражению переменную
void CanonicalLinearExpression::add(VariableDeclaration* Summand, Coefficient coeff)
{ 
	m_summands[Summand] += coeff;
} 


/// Прибавляет к текущему каноническому линейному выражению второе каноническое линейное выражение
void CanonicalLinearExpression::add(const CanonicalLinearExpression& Summand)
{
	SummandsMap::const_iterator iter = Summand.m_summands.begin();
	for(;iter != Summand.m_summands.end(); ++iter)
		m_summands[iter->first] += iter->second;

	m_freeSummand += Summand.getFreeSummand();
}

/// Умножает текущее каноническое линейное выражение на число
void CanonicalLinearExpression::multiply(Coefficient Multiplier)
{
	SummandsMap::iterator iter = m_summands.begin();
	for(;iter != m_summands.end(); ++iter)
		iter->second *= Multiplier;

	m_freeSummand *= Multiplier;
}

/// Умножает текущее каноническое линейное выражение на переменную
bool CanonicalLinearExpression::multiply(VariableDeclaration* Multiplier)
{
	if (m_summands.empty())
	{
		m_summands[Multiplier] = m_freeSummand;
		m_freeSummand = 0;
		return true;
	}
	else
		return false;
}

/// Умножает текущее каноническое линейное выражение на второе каноническое линейное выражение
bool CanonicalLinearExpression::multiply(const CanonicalLinearExpression& Multiplier)
{
	if (m_summands.empty())
	{
		SummandsMap::const_iterator iter = Multiplier.m_summands.begin();
		for(;iter != Multiplier.m_summands.end(); ++iter)
			m_summands[iter->first] = iter->second * m_freeSummand;

		m_freeSummand *= Multiplier.getFreeSummand();
	}
	else
		if (Multiplier.m_summands.empty())
			multiply(Multiplier.getFreeSummand());
		else
			return false;

	return true;
}

CanonicalLinearExpression::Coefficient CanonicalLinearExpression::getCoefficient(OPS::Reprise::VariableDeclaration *Summand) const
{
	SummandsMap::const_iterator it = m_summands.find(Summand);
	if (it != m_summands.end())
		return it->second;
	else
		return 0;
}

//Выдает список VariableDeclaration's переменных, коэффициенты при которых отличны от нуля
list<VariableDeclaration*> CanonicalLinearExpression::getVariables() const
{
    list<VariableDeclaration*> res;
	SummandsMap::const_iterator it;
	for (it=m_summands.begin(); it!=m_summands.end(); ++it)
		if (it->second!=0) res.push_back(it->first);

	return res;
}

CanonicalLinearExpression CanonicalLinearExpression::getOpposite() const
{
	CanonicalLinearExpression result(*this);
	result.multiply(-1);
	return result;
}

bool CanonicalLinearExpression::isConstant() const
{
	SummandsMap::const_iterator it;
	for(it = m_summands.begin(); it != m_summands.end(); ++it)
		if (it->second != 0)
			return false;

	return true;
}

ParametricLinearExpression::ParametricLinearExpression(const ParametricLinearExpression& arg)
{
	copyFrom(&arg);
}

ParametricLinearExpression& ParametricLinearExpression::operator=(const ParametricLinearExpression& rhs)
{
	if (this != &rhs)
	{
		m_evaluatable = rhs.isEvaluatable();

		// Очищаем список коэффициентов
		m_summands.clear();
		m_occurenceDescriptions.clear();

		// Необходимо, чтобы не уменьшить базу линеаризации
		ExpressionVariablesMap::const_iterator itOccur = rhs.m_occurenceDescriptions.begin();
		for (; itOccur!=rhs.m_occurenceDescriptions.end(); itOccur++)
			m_occurenceDescriptions.insert(make_pair(itOccur->first, itOccur->second));

		// Копируем слагаемые
		SummandsMap::const_iterator it = rhs.m_summands.begin();
		for (; it!=rhs.m_summands.end(); it++)
			setSummandsInfo(it->first, it->second.get());

		// Свободныый член
		m_freeSummand.reset(rhs.m_freeSummand->clone());
	}
	return *this;
}

ParametricLinearExpression::ParametricLinearExpression(VariablesDeclarationsVector someBaseVariables): m_evaluatable(true)
{
    m_freeSummand.reset(StrictLiteralExpression::createInt32(0));

	// Создаем структуру лин выражения
	VariablesDeclarationsVector::iterator iter = someBaseVariables.begin();
	for (; iter != someBaseVariables.end(); ++iter)
	{
		if (m_occurenceDescriptions.find(*iter) == m_occurenceDescriptions.end())
		{
			OccurenceDescriptionList listOccurences;
			m_occurenceDescriptions.insert(pair<DeclarationBase*, OccurenceDescriptionList>(*iter, listOccurences));
		}
	}
}

ParametricLinearExpression::ParametricLinearExpression(ParametricLinearExpression::VariablesDeclarationsVector someBaseVariables, const SymbolicDescription* oneSummand): m_evaluatable(true)
{
	IntegerHelper ih(BasicType::BT_INT32);

	// Создаем структуру лин выражения
	bool found = false;
	VariablesDeclarationsVector::iterator iter = someBaseVariables.begin();
	for (; iter != someBaseVariables.end(); ++iter)
	{
		if (m_occurenceDescriptions.find(*iter) == m_occurenceDescriptions.end())
		{
			OccurenceDescriptionList listOccurences;
			m_occurenceDescriptions.insert(pair<DeclarationBase*, OccurenceDescriptionList>(*iter, listOccurences));

			if (oneSummand->getDeclaration() == *iter)
			{
				ReprisePtr<SymbolicDescription> occurence(oneSummand->clone());
				m_occurenceDescriptions[*iter].push_back(occurence);
				m_summands.insert(pair<SymbolicDescription*, Coefficient>(m_occurenceDescriptions[*iter].back().get(), Coefficient(&ih(1))));
				found = true;
			}
		}
	}
	if (found)
		m_freeSummand.reset(&ih(0));
	else
		m_freeSummand.reset(oneSummand->convert2RepriseExpression());
}

ParametricLinearExpression::ParametricLinearExpression(VariablesDeclarationsVector someBaseVariables, const ReferenceExpression* oneSummand)
{
	m_evaluatable = false;
	IntegerHelper ih(BasicType::BT_INT32);

	// Создаем структуру лин выражения
	VariablesDeclarationsVector::iterator iter = someBaseVariables.begin();
	for (; iter != someBaseVariables.end(); ++iter)
	{
		if (m_occurenceDescriptions.find(*iter) == m_occurenceDescriptions.end())
		{
			OccurenceDescriptionList listOccurences;
			m_occurenceDescriptions.insert(pair<DeclarationBase*, OccurenceDescriptionList>(*iter, listOccurences));

			// Принадлежит ли переданная переменная к базе линеаризации
			if (&oneSummand->getReference() == (*iter))
			{
				m_evaluatable = true;
				m_freeSummand.reset(&ih(0));

				ReprisePtr<ExpressionBase> tempSummand(oneSummand->clone());
				SymbolicDescription* occur = SymbolicDescription::createSymbolicDescription(*tempSummand);
				if (!occur)
				{
					OPS::Console* const pConsole = &OPS::getOutputConsole("ParametricLinearExpression"); 
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Unexpected ReferenceExpression: "+oneSummand->dumpState()+".",""));
					clear();
					return;
				}
				m_occurenceDescriptions[*iter].push_back(ReprisePtr<SymbolicDescription>(occur));
				m_summands.insert(pair<SymbolicDescription*, Coefficient>(m_occurenceDescriptions[*iter].back().get(), Coefficient(&ih(1))));
			}
		}
	}

	// Если лин выражение осталось не вычислимо, значит мы не встретили oneSummandd базе линеаризации
	if (!m_evaluatable)
		m_freeSummand.reset(oneSummand->clone());
}

ParametricLinearExpression::ParametricLinearExpression(VariablesDeclarationsVector someBaseVariables, const LiteralExpression* oneSummand): m_evaluatable(true)
{
	IntegerHelper ih(BasicType::BT_INT32);

	// Создаем структуру лин выражения
	VariablesDeclarationsVector::iterator iter = someBaseVariables.begin();
	for (; iter != someBaseVariables.end(); ++iter)
		if (m_occurenceDescriptions.find(*iter) == m_occurenceDescriptions.end())
		{
			OccurenceDescriptionList listOccurences;
			m_occurenceDescriptions.insert(pair<DeclarationBase*, OccurenceDescriptionList>(*iter, listOccurences));
		}

	m_freeSummand.reset(oneSummand->clone());
}

void ParametricLinearExpression::clear()
{
	m_summands.clear();
	m_occurenceDescriptions.clear();
}

ParametricLinearExpression::~ParametricLinearExpression()
{
	clear();
}

long_long_t ParametricLinearExpression::getCoefficientAsInteger(const SymbolicDescription* BaseVariable) const
{
	SymbolicDescription* VarExactly = findEqualExpressionVariable(BaseVariable);
	if (VarExactly != 0)
	{
		Coefficient coef = getCoefficientExactly(VarExactly);
		if (coef->is_a<LiteralExpression>())
			return getLiteralAsInteger(coef->cast_ptr<LiteralExpression>());
	}

	return 0;
}

long_long_t ParametricLinearExpression::getCoefficientAsInteger(const VariableDeclaration* BaseVariable) const
{
	// TODO: после внесения изменений в Reprise эта функция и следующие должны измениться
	ExpressionVariablesMap::const_iterator it = m_occurenceDescriptions.find(BaseVariable);
	if (it == m_occurenceDescriptions.end())
		//throw RuntimeError("Impossible to get coefficient by VariableDeclaration that is not belongs to LinearizationBase!"); 
		return 0;

	if (it->second.empty())
		return 0;

	return getCoefficientAsInteger(it->second.front().get());
}

double ParametricLinearExpression::getCoefficientAsDouble(const SymbolicDescription* BaseVariable) const
{
	SymbolicDescription* VarExactly = findEqualExpressionVariable(BaseVariable);
	if (VarExactly != 0)
	{
		Coefficient coef = getCoefficientExactly(VarExactly);
		if (coef->is_a<LiteralExpression>())
			return getLiteralAsDouble(coef->cast_ptr<LiteralExpression>());
	}

	return 0;
}

double ParametricLinearExpression::getCoefficientAsDouble(const VariableDeclaration* BaseVariable) const
{
	ExpressionVariablesMap::const_iterator it = m_occurenceDescriptions.find(BaseVariable);
	if (it == m_occurenceDescriptions.end())
		//throw RuntimeError("Impossible to get coefficient by VariableDeclaration that is not belongs to LinearizationBase!"); 
		return 0;

	if (it->second.empty())
		return 0;

	return getCoefficientAsDouble(it->second.front().get());
}

long_long_t ParametricLinearExpression::getFreeCoefficientAsInteger() const
{
	if (!m_freeSummand->is_a<LiteralExpression>())
		throw RuntimeError("Impossible to get value from ExpressionBase differ then LiteralExpression!"); 

	return getLiteralAsInteger(m_freeSummand->cast_ptr<LiteralExpression>());
}

double ParametricLinearExpression::getFreeCoefficientAsDouble() const
{
	if (!m_freeSummand->is_a<LiteralExpression>())
		throw RuntimeError("Impossible to get value from ExpressionBase differ then LiteralExpression!"); 

	return getLiteralAsDouble(m_freeSummand->cast_ptr<LiteralExpression>());
}

long_long_t ParametricLinearExpression::getLiteralAsInteger(const LiteralExpression* Literal)
{
	if (Literal->is_a<BasicLiteralExpression>())
	{
		const BasicLiteralExpression* summand = Literal->cast_ptr<BasicLiteralExpression>();
		if (summand->getLiteralType() == BasicLiteralExpression::LT_INTEGER)
			return summand->getInteger();
	}

	if (Literal->is_a<StrictLiteralExpression>())
	{
		const StrictLiteralExpression* summand = Literal->cast_ptr<StrictLiteralExpression>();
		switch (summand->getLiteralType())
		{
		case BasicType::BT_INT8:
			return summand->getInt8();
		case BasicType::BT_INT16:
			return summand->getInt16();
		case BasicType::BT_INT32:
			return summand->getInt32();
        case BasicType::BT_UINT32:
            return summand->getUInt32();
		case BasicType::BT_INT64:
			return summand->getInt64();
		case BasicType::BT_UINT64:
			return summand->getInt64();
		default: ; // сработает исключение в конце функции
		}
	}

	// Если мы пришли сюда, то значение типа integer не было найдено
	throw RuntimeError("Impossible to get not integer value using getCoefficientAsInteger/getFreeCoefficientAsInteger function!"); 
}

double ParametricLinearExpression::getLiteralAsDouble(const LiteralExpression* Literal)
{
	if (Literal->is_a<BasicLiteralExpression>())
	{
		const BasicLiteralExpression* summand = Literal->cast_ptr<BasicLiteralExpression>();
		if (summand->getLiteralType() == BasicLiteralExpression::LT_FLOAT)
			return summand->getFloat();
	}

	if (Literal->is_a<StrictLiteralExpression>())
	{
		const StrictLiteralExpression* summand = Literal->cast_ptr<StrictLiteralExpression>();
		switch (summand->getLiteralType())
		{
		case BasicType::BT_FLOAT32:
			return summand->getFloat32();
		case BasicType::BT_FLOAT64:
			return summand->getFloat64();
		default: ; // сработает исключение в конце функции
		}
	}

	// Если мы пришли сюда, то значение типа integer не было найдено
	throw RuntimeError("Impossible to get not double value using getCoefficientAsDouble/getFreeCoefficientAsDouble function!"); 
}

//получает коэффициенты при всех переменных, не являющихся счетчиками циклов
CanonicalLinearExpression ParametricLinearExpression::getExternalParamCoefficients(
	const VariablesDeclarationsVector &loopCounters) const
{
	CanonicalLinearExpression::SummandsMap summands;
	VariablesDeclarationsVector::const_iterator itFind;
    VariablesDeclarationsVector varList = getVariables();
	VariablesDeclarationsVector::iterator it;
    for (it = varList.begin(); it!=varList.end(); ++it)
    {
        itFind = find(loopCounters.begin(),loopCounters.end(),*it);
        //если не нашли среди счетчиков - добавляем в строемое выражение
        if (itFind==loopCounters.end()) 
        {
            summands[*it] = (long)getCoefficientAsInteger(*it);
        }
    }
    CanonicalLinearExpression result(summands);
return result;
}

ParametricLinearExpression* ParametricLinearExpression::operator +(const ParametricLinearExpression& secondArg) const
{
	ParametricLinearExpression* res = new ParametricLinearExpression();

	// В начале надо проверить, что наборы переменных по которым линеаризуются выражения вложены. 
	if (!includesLinearizationBaseOf(secondArg))
        return res; // возвращаем "пустое" лин выр (пустая база линеаризации) означает что сумма не выполнена

	OccurenceDescriptionList emptyList;
	ExpressionVariablesMap::const_iterator occur_iter = m_occurenceDescriptions.begin();
	for(; occur_iter != m_occurenceDescriptions.end(); ++occur_iter)
    {
		std::pair <const DeclarationBase *, OccurenceDescriptionList> pair =
			std::make_pair(occur_iter->first, emptyList);
		res->m_occurenceDescriptions.insert(pair);
    }

	// Выполняем сумму аргументов, проверенных на корректность
	res->m_evaluatable = m_evaluatable && secondArg.m_evaluatable;
	ExpressionBase* coefExprBase;
	SummandsMap::const_iterator iter = m_summands.begin();
	for(; iter != m_summands.end(); ++iter)
	{
		if (secondArg.findEqualExpressionVariable(iter->first))
		{
			coefExprBase = 0;
			if (iter->second->is_a<LiteralExpression>() && secondArg[iter->first]->is_a<LiteralExpression>())
			{
				bool overflow;
				coefExprBase = addWithEvaluating(iter->second->cast_ptr<LiteralExpression>(), secondArg[iter->first]->cast_ptr<LiteralExpression>(), overflow);
				if (coefExprBase)
					if (!coefExprBase->is_a<LiteralExpression>())
						res->m_evaluatable = false;
			}

			if (!coefExprBase)
			{
				res->m_evaluatable = false;
				const Coefficient coef1  = iter->second;
				const Coefficient coef2  = secondArg[iter->first];
				if (isEqualToIntValue(coef1.get(), 0)) // упрощаем сложение с 0
					coefExprBase = coef2->clone();
				else
					if (isEqualToIntValue(coef2.get(), 0)) // упрощаем сложение с 0
						coefExprBase = coef1->clone();
					else
						coefExprBase = &(*(coef1->clone()) + *(coef2->clone()));
			}
		}
		else
			coefExprBase = iter->second->clone();

		SymbolicDescription* newVar = iter->first->clone();
		DeclarationBase* decl = newVar->getDeclaration();
		if (res->m_occurenceDescriptions.find(decl) == res->m_occurenceDescriptions.end())
		{
			list<ReprisePtr<SymbolicDescription> > listVariables;
			listVariables.push_back(ReprisePtr<SymbolicDescription>(newVar));
			res->m_occurenceDescriptions.insert(pair<DeclarationBase*, list<ReprisePtr<SymbolicDescription> > >(decl, listVariables));
		}
		else
			res->m_occurenceDescriptions[decl].push_back(ReprisePtr<SymbolicDescription>(newVar));
		res->m_summands.insert(pair<SymbolicDescription*, Coefficient>(res->m_occurenceDescriptions[decl].back().get(), Coefficient(coefExprBase)));

	}

	// Добавляем не учтенные слагаемые из второго аргумента
	iter = secondArg.m_summands.begin();
	for(; iter != secondArg.m_summands.end(); ++iter)
	{
		if (findEqualExpressionVariable(iter->first) == 0)
			res->setSummandsInfo(iter->first, iter->second.get());
	}

    // Складываем свободные члены линейных выражений
	coefExprBase = 0;
	if (m_freeSummand->is_a<LiteralExpression>() && secondArg.m_freeSummand->is_a<LiteralExpression>())
	{
		bool overflow;
		coefExprBase = addWithEvaluating(m_freeSummand->cast_ptr<LiteralExpression>(), secondArg.m_freeSummand->cast_ptr<LiteralExpression>(), overflow);
		if (coefExprBase)
			if (!coefExprBase->is_a<LiteralExpression>())
				res->m_evaluatable = false;
	}

	if (!coefExprBase)
	{
		res->m_evaluatable = false;
		if (isEqualToIntValue(m_freeSummand.get(), 0)) // упрощаем сложение с 0
			coefExprBase = secondArg.m_freeSummand->clone();
		else
			if (isEqualToIntValue(secondArg.m_freeSummand.get(), 0)) // упрощаем сложение с 0
				coefExprBase = m_freeSummand->clone();
			else
				coefExprBase = &(*(m_freeSummand->clone()) + *(secondArg.m_freeSummand->clone()));
	}
	res->m_freeSummand.reset(coefExprBase);

    return res;
}

ParametricLinearExpression* ParametricLinearExpression::operator -(const ParametricLinearExpression& secondArg) const
{
//	return ((*this) + secondArg.getOpposite());

	ParametricLinearExpression* res = new ParametricLinearExpression();
	SummandsMap::const_iterator iter;
	
	// В начале надо проверить, что наборы переменных по которым линеаризуются выражения вложены. 
	if (!includesLinearizationBaseOf(secondArg))
        return res; // возвращаем "пустое" лин выр (пустая база линеаризации) означает что сумма не выполнена

	OccurenceDescriptionList emptyList;
	ExpressionVariablesMap::const_iterator occur_iter = m_occurenceDescriptions.begin();
	for(; occur_iter != m_occurenceDescriptions.end(); ++occur_iter)
	{
		std::pair <const DeclarationBase *, OccurenceDescriptionList> pair =
			std::make_pair(occur_iter->first, emptyList);
		res->m_occurenceDescriptions.insert(pair);
	}
	// Выполняем разность аргументов, проверенных на корректность
	ExpressionBase* coefExprBase;
	res->m_evaluatable = m_evaluatable && secondArg.m_evaluatable;
	for(iter = m_summands.begin(); iter != m_summands.end(); ++iter)
	{
		if (secondArg.findEqualExpressionVariable(iter->first))
		{
			coefExprBase = 0;
			if (iter->second->is_a<LiteralExpression>() && secondArg[iter->first]->is_a<LiteralExpression>())
			{
				bool overflow;
				coefExprBase = subtractWithEvaluating(iter->second->cast_ptr<LiteralExpression>(), secondArg[iter->first]->cast_ptr<LiteralExpression>(), overflow);
				if (coefExprBase)
					if (!coefExprBase->is_a<LiteralExpression>())
						res->m_evaluatable = false;
			}

			if (!coefExprBase)
			{
				res->m_evaluatable = false;
				const Coefficient coef1  = iter->second;
				const Coefficient coef2  = secondArg[iter->first];
				if (isEqualToIntValue(coef1.get(), 0)) // упрощаем сложение с 0
					coefExprBase = &(- *(coef2->clone()));
				else
					if (isEqualToIntValue(coef2.get(), 0)) // упрощаем сложение с 0
						coefExprBase = coef1->clone();
					else
						coefExprBase = &(*(coef1->clone()) - *(coef2->clone()));
			}
		}
		else
			coefExprBase = iter->second->clone();

		SymbolicDescription* newVar = iter->first->clone();
		DeclarationBase* decl = newVar->getDeclaration();
		if (res->m_occurenceDescriptions.find(decl) == res->m_occurenceDescriptions.end())
		{
			list<ReprisePtr<SymbolicDescription> > listVariables;
			listVariables.push_back(ReprisePtr<SymbolicDescription>(newVar));
			res->m_occurenceDescriptions.insert(pair<DeclarationBase*, list<ReprisePtr<SymbolicDescription> > >(decl, listVariables));
		}
		else
			res->m_occurenceDescriptions[decl].push_back(ReprisePtr<SymbolicDescription>(newVar));
		res->m_summands.insert(pair<SymbolicDescription*, Coefficient>(newVar, Coefficient(coefExprBase)));
	}

	// Добавляем не учтенные слагаемые из второго аргумента
	iter = secondArg.m_summands.begin();
	for(; iter != secondArg.m_summands.end(); ++iter)
	{
		if (findEqualExpressionVariable(iter->first) == 0)
		{
			if (iter->second->is_a<LiteralExpression>())
            {
                ReprisePtr<ExpressionBase> opp(Literals::getOpposite(iter->second.get()->cast_ptr<LiteralExpression>()));
                res->setSummandsInfo(iter->first, opp.get());
            }
			else
            {
                ReprisePtr<ExpressionBase> opp(&(-(*iter->second.get())));
                res->setSummandsInfo(iter->first, opp.get());
            }
		}
	}

	// Вычитаем свободные члены лин выражения
	coefExprBase = 0;
	if (m_freeSummand->is_a<LiteralExpression>() && secondArg.m_freeSummand->is_a<LiteralExpression>())
	{
		bool overflow;
		coefExprBase = subtractWithEvaluating(m_freeSummand->cast_ptr<LiteralExpression>(), secondArg.m_freeSummand->cast_ptr<LiteralExpression>(), overflow);
		if (coefExprBase)
			if (!coefExprBase->is_a<LiteralExpression>())
				res->m_evaluatable = false;
	}

	if (!coefExprBase)
	{
		res->m_evaluatable = false;
		if (isEqualToIntValue(m_freeSummand.get(), 0)) // упрощаем сложение с 0
				coefExprBase = &(- *(secondArg.m_freeSummand->clone()));
			else
				if (isEqualToIntValue(secondArg.m_freeSummand.get(), 0)) // упрощаем сложение с 0
					coefExprBase = m_freeSummand->clone();
				else
					coefExprBase = &(*(m_freeSummand->clone()) - *(secondArg.m_freeSummand->clone()));
	}
	res->m_freeSummand.reset(coefExprBase);

    return res;
}

ParametricLinearExpression* ParametricLinearExpression::getOpposite() const
{
	ParametricLinearExpression* result = new ParametricLinearExpression(*this);

    SummandsMap::iterator iter = result->m_summands.begin();
	for(; iter != result->m_summands.end(); ++iter)
	{
		if (iter->second->is_a<LiteralExpression>())
            iter->second.reset(Literals::getOpposite(iter->second->cast_ptr<LiteralExpression>()));
		else
			iter->second.reset(&(- (*iter->second)));
	}

	if (result->m_freeSummand->is_a<LiteralExpression>())
        result->m_freeSummand.reset(Literals::getOpposite(m_freeSummand->cast_ptr<LiteralExpression>()));
	else
		result->m_freeSummand.reset(&(- (*result->m_freeSummand)));

    return result;
}

ParametricLinearExpression& ParametricLinearExpression::add(const LiteralExpression& summand)
{
	if (isEqualToIntValue(&summand, 0)) // если 2-ое слагаемое равно 0, то складывать не надо
		return *this;

	bool overflow;
	if (m_freeSummand->is_a<LiteralExpression>())
	{
		ExpressionBase* newFree = addWithEvaluating(m_freeSummand->cast_ptr<LiteralExpression>(), &summand, overflow);
		if (newFree != 0)
			m_freeSummand.reset(newFree);
		else
			m_freeSummand.reset(&(*m_freeSummand + (*summand.clone())));
	}
	else
		m_freeSummand.reset(&(*m_freeSummand + (*summand.clone())));

	if (!m_freeSummand->is_a<LiteralExpression>())
		m_evaluatable = false;

	return *this;
}

ParametricLinearExpression& ParametricLinearExpression::add(const ReferenceExpression& summand)
{
	IntegerHelper ih(BasicType::BT_INT32); 
	ReprisePtr<LiteralExpression> one(&ih(1));

	ReprisePtr<ExpressionBase> tempSummand(summand.clone());
	ReprisePtr<SymbolicDescription> temp(SymbolicDescription::createSymbolicDescription(*tempSummand)); 
	if (!temp.get())
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole("ParametricLinearExpression"); 
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Unexpected ReferenceExpression: "+summand.dumpState()+".",""));
		clear();
		return *this;
	}
	SymbolicDescription* ExprVar = findEqualExpressionVariable(temp.get());
	
	ExpressionVariablesMap::iterator it = m_occurenceDescriptions.find(ExprVar->getDeclaration());
	
	bool overflow;
	if (it != m_occurenceDescriptions.end()) //в декларейшенах есть
	{
		if (ExprVar != 0) // и в декларейшенах есть и среди вхождений есть
		{
			bool evaluted = false;
			if (m_summands[ExprVar]->is_a<LiteralExpression>())
			{
				ExpressionBase* result = addWithEvaluating(m_summands[ExprVar]->cast_ptr<LiteralExpression>(), one.get(), overflow);
				if (result)
				{
					m_summands[ExprVar].reset(result);
					evaluted = true;
				}
			}

			if (!evaluted)
			{
				if (!isEqualToIntValue(m_summands[ExprVar].get(), 0)) // при сложении с 0, упрощаем
				{
					m_summands[ExprVar].reset(&(*m_summands[ExprVar] + *one));
					m_evaluatable = false;	
				}
				else
					m_summands[ExprVar].reset(one.get());
			}
		}
		else
		{	// декларейшн есть, а в слагаемых SymbolicDescription нет
			it->second.push_back(temp);
			m_summands.insert(pair<SymbolicDescription*, Coefficient>(temp.get(), one));
		}
	}
	else
	{
		m_evaluatable = false;
		if (!isEqualToIntValue(m_freeSummand.get(), 0)) // при сложении с 0, упрощаем
			m_freeSummand.reset(&(*m_freeSummand + *summand.clone()));
		else
			m_freeSummand.reset(summand.clone());
	}

	return *this;
}

ParametricLinearExpression& ParametricLinearExpression::multiply(const BasicLiteralExpression* multiplier)
{
	multiplyUnsafe(*multiplier);
	return *this;
}

ParametricLinearExpression& ParametricLinearExpression::multiply(const ReferenceExpression* multiplier)
{
	multiplyUnsafe(*multiplier);
	return *this;
}

bool ParametricLinearExpression::multiply(const ParametricLinearExpression* multiplier)
{
	bool correct = false;
	IntegerHelper ih(BasicType::BT_INT32);
	
	if (multiplier->isIndependent())
	{
		correct = true;
		ReprisePtr<CoefficientRef> c(multiplier->getFreeCoefficient()->clone());

		// Корректность: проверяем, что в свободном члене multiplier нет переменных из базы линеаризации this
		list<VariableDeclaration*> list = getAllVariableDeclarations(c.get());

		ExpressionVariablesMap::iterator iter = m_occurenceDescriptions.begin();
		for(; iter != m_occurenceDescriptions.end(); ++iter)
			if (iter->first->is_a<VariableDeclaration>())
				if (find(list.begin(), list.end(), iter->first->cast_ptr<VariableDeclaration>()) != list.end())
					return false;

		// Выполняем умножение
		multiplyUnsafe(*c);
	}
	else
	{
		if (isIndependent())
		{
			// копируем множитель, тк его функций multiply потом испортит
			std::unique_ptr<ParametricLinearExpression> pProduction(new ParametricLinearExpression(*multiplier));
			correct = pProduction->multiply(this);
			clear();
			copyFrom(pProduction.get());
			return correct;
		}
	}

	return correct;
}

void ParametricLinearExpression::multiplyUnsafe(const ExpressionBase& multiplier, BasicCallExpression::BuiltinCallKind OperationKind)
{
	// Проверка
	if ((OperationKind != BasicCallExpression::BCK_MULTIPLY) && (OperationKind != BasicCallExpression::BCK_DIVISION))
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole("ParametricLinearExpression");
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Not supported operation in multiplyUnsafe()!",""));
		return;
	}

    SimpleLiterals multiplierSimplicity = specifySimplicityOfExpression(&multiplier);
    if (multiplierSimplicity == SL_POSITIVE_ONE) // упрощаем при умножении или делении на 1
		return; // результат совпадает с 1-ым аргументом

	const LiteralExpression* pMultiplier = 0;
	if (multiplier.is_a<LiteralExpression>())
		pMultiplier = multiplier.cast_ptr<LiteralExpression>();

	SummandsMap::iterator iter = m_summands.begin();
	for(; iter != m_summands.end(); ++iter)
    {
		bool overflow = false;
		iter->second.reset(multiplyWithLiteralSimplification(iter->second.get(), &multiplier, overflow));
		m_evaluatable = m_evaluatable && iter->second->is_a<StrictLiteralExpression>();
    }

	// Домножаем свободный член на множитель
	bool overflow = false;
	m_freeSummand.reset(multiplyWithLiteralSimplification(m_freeSummand.get(), &multiplier, overflow));
	m_evaluatable = m_evaluatable && m_freeSummand->is_a<StrictLiteralExpression>();
}


bool ParametricLinearExpression::divide(const ParametricLinearExpression* divider)
{
	// делить мы можем только, если делитель не зависит от базы линеаризации делимого
	bool correct = false;
	
	if (divider->isIndependent())
	{
		correct = true;
		ExpressionBase* c = divider->getFreeCoefficient()->clone();

		// Корректность: проверяем, что в свободном члене divider нет переменных из базы линеаризации this
		list<VariableDeclaration*> list = getAllVariableDeclarations(c);
		ExpressionVariablesMap::iterator iter = m_occurenceDescriptions.begin();
		for(; iter != m_occurenceDescriptions.end(); ++iter)
			if (iter->first->is_a<VariableDeclaration>())
				if (find(list.begin(), list.end(), iter->first->cast_ptr<VariableDeclaration>()) != list.end())
				{
					delete c;
					return false;
				}

		// Выполняем деление
		multiplyUnsafe(*c, BasicCallExpression::BCK_DIVISION);
	}
	
	return correct;
}

bool ParametricLinearExpression::isIndependent() const
{
	SummandsMap::const_iterator iter = m_summands.begin();
	for(;iter != m_summands.end(); ++iter)
	if (!isEqualToIntValue(iter->second.get(), 0))
		return false;
	
	return true;
}

bool ParametricLinearExpression::isZero() const 
{ 
    return (m_summands.empty() && (isEqualToIntValue(m_freeSummand.get(), 0)));
}

//Выдает список VariableDeclaration's переменных, коэффициенты при которых отличны от нуля
ParametricLinearExpression::VariablesDeclarationsVector ParametricLinearExpression::getVariables() const
{
	VariablesDeclarationsVector res;
	SummandsMap::const_iterator it;
	for (it=m_summands.begin(); it!=m_summands.end(); ++it)
		if (!isEqualToIntValue(it->second.get(), 0)) 
			if (it->first->getDeclaration()->is_a<VariableDeclaration>())
			{
				VariableDeclaration* decl = it->first->getDeclaration()->cast_ptr<VariableDeclaration>();
				if (find(res.begin(), res.end(), decl) == res.end())
					res.push_back(decl);
			}

	return res;
}

/// Выдает хеш-список переменных и коэффициэнтов при них
map<VariableDeclaration*, long_long_t> ParametricLinearExpression::getMap()
{
	map<VariableDeclaration*, long_long_t> res;
	if (m_evaluatable)
	{
		SummandsMap::iterator it;
		for (it=m_summands.begin(); it!=m_summands.end(); ++it)
			if (it->first->getDeclaration()->is_a<VariableDeclaration>())
			{
				VariableDeclaration* decl = it->first->getDeclaration()->cast_ptr<VariableDeclaration>();
				if (res.find(decl) == res.end())
					res[decl] = this->getCoefficientAsInteger(decl);
				else
				{	// корректно построить требуемый список не получается
					res.clear(); 
					return res;
				}
			}
	}
	//else throw RuntimeError("Parametric expression is not evaluatable!"); 
	return res;
}

ParametricLinearExpression::Coefficient ParametricLinearExpression::operator[](SymbolicDescription* BaseVariable) const
{ 
    IntegerHelper ih(BasicType::BT_INT32);

	ExpressionVariablesMap::const_iterator map_iter = m_occurenceDescriptions.find(BaseVariable->getDeclaration());
	if (map_iter == m_occurenceDescriptions.end())
        return Coefficient(&ih(0));

	list<ReprisePtr<SymbolicDescription> >::const_iterator list_iter = map_iter->second.begin();
	for (; list_iter != map_iter->second.end(); ++list_iter)
		if (BaseVariable->isEqual(list_iter->get()))
		{
			SummandsMap::const_iterator it = m_summands.find(list_iter->get());
			if (it != m_summands.end())
				return it->second;
		}
	// декларейшн есть, но этого конкретного вхождения нет в базе линеаризации
	return Coefficient(&ih(0)); 
}

ParametricLinearExpression::Coefficient ParametricLinearExpression::operator[](unsigned BaseVariableNumber) const
{ 
	if ((int)m_occurenceDescriptions.size() > (int)BaseVariableNumber)
	{
		ExpressionVariablesMap::const_iterator iter;
		for(iter = m_occurenceDescriptions.begin(); ; ++iter,--BaseVariableNumber)
			if (BaseVariableNumber == 0)
			{
				if (iter->second.empty())
				{
					IntegerHelper ih(BasicType::BT_INT32); 
					return Coefficient(&ih(0));
				}
				else
					return getCoefficientExactly(iter->second.front().get()); 
			}
	}
	else
		if ((int)m_occurenceDescriptions.size() == (int)BaseVariableNumber)
			return m_freeSummand;
		else
			return ReprisePtr<ExpressionBase>();
}

ParametricLinearExpression::Coefficient ParametricLinearExpression::operator[](VariableDeclaration* BaseVariable) const
{
	IntegerHelper ih(BasicType::BT_INT32);

	ExpressionVariablesMap::const_iterator it = m_occurenceDescriptions.find(BaseVariable);
	if (it == m_occurenceDescriptions.end() ||
		it->second.empty())
		return Coefficient( &ih(0) );

	SymbolicDescription* ExprVariable = it->second.front().get();
	return getCoefficientExactly(ExprVariable);
}

bool ParametricLinearExpression::includesLinearizationBaseOf(const ParametricLinearExpression& Second) const
{
	ExpressionVariablesMap::const_iterator map_iter = Second.m_occurenceDescriptions.begin();
	for(; map_iter != Second.m_occurenceDescriptions.end(); ++map_iter)
		if (m_occurenceDescriptions.find(map_iter->first) == m_occurenceDescriptions.end())
			return false;

	return true;
}

string ParametricLinearExpression::dumpState() const
{
	string res = "";
	SummandsMap::const_iterator it = m_summands.begin();
	for(; it != m_summands.end(); ++it)
		res += it->first->dumpState() + " * (" + it->second->dumpState() + ") + ";
	res += m_freeSummand->dumpState();

	return res;
}

ParametricLinearExpression::OccurenceDescriptionList ParametricLinearExpression::getOccurencesDescriptionsList(DeclarationBase* DeclarationSample) const
{
	ExpressionVariablesMap::const_iterator it = m_occurenceDescriptions.find(DeclarationSample);
	if (it != m_occurenceDescriptions.end())
		return it->second;
	else
	{
		OccurenceDescriptionList listDescriptions;
		return listDescriptions;
	}
}

ExpressionBase* ParametricLinearExpression::convert2RepriseExpression() const
{
    ExpressionBase* res = m_freeSummand->clone();
    SimpleLiterals resSimplicity = specifySimplicityOfExpression(res);
    ExpressionVariablesMap::const_iterator map_iter = m_occurenceDescriptions.begin();
    for (; map_iter != m_occurenceDescriptions.end(); ++map_iter)
    {
        list<ReprisePtr<SymbolicDescription> >::const_iterator list_iter = map_iter->second.begin();
        for (; list_iter != map_iter->second.end(); ++list_iter)
        {
            SummandsMap::const_iterator it = m_summands.find(list_iter->get());

            if (it != m_summands.end())
            {
                SimpleLiterals coefficientSimplicity = specifySimplicityOfExpression(it->second.get());

                if (coefficientSimplicity != Literals::SL_ZERO)
                {
                    ReprisePtr<ExpressionBase> tempSummand(it->first->convert2RepriseExpression());
                    if (coefficientSimplicity == Literals::SL_NOT_SIMPLE)
                        tempSummand.reset(&(op(it->second) * op(tempSummand)));

                    ExpressionBase* tempCalc;
                    if (resSimplicity != SL_ZERO)
                        tempCalc = (coefficientSimplicity == Literals::SL_NEGATIVE_ONE) ? &(op(res) - op(tempSummand)) : &(op(res) + op(tempSummand));
                    else
                        tempCalc = (coefficientSimplicity == Literals::SL_NEGATIVE_ONE) ? &(- op(tempSummand)) : tempSummand.release(); // tempSummand разрушиться сам
                    delete res;
                    res = tempCalc;
                }
            }
        }
    }

    return res;
}

SymbolicDescription* ParametricLinearExpression::findEqualExpressionVariable(const SymbolicDescription* Prototype) const
{
	ExpressionVariablesMap::const_iterator map_iter = m_occurenceDescriptions.find(Prototype->getDeclaration());
	if (map_iter == m_occurenceDescriptions.end())
		return 0;

	list<ReprisePtr<SymbolicDescription> >::const_iterator it = map_iter->second.begin();
	for(; it != map_iter->second.end(); ++it)
	{
		if ((*it)->isEqual(Prototype))
			return it->get();
	}
	return 0;
}

ParametricLinearExpression::Coefficient ParametricLinearExpression::getCoefficientExactly(SymbolicDescription* ExactlyVariable) const 
{
	ExpressionVariablesMap::const_iterator iter = m_occurenceDescriptions.find(ExactlyVariable->getDeclaration());
	if (iter == m_occurenceDescriptions.end())
		return Coefficient(); 

	SummandsMap::const_iterator it = m_summands.find(ExactlyVariable);
	if (it != m_summands.end())
		return it->second;
	else
	{
		IntegerHelper ih(BasicType::BT_INT32); 
		return Coefficient(&ih(0)); 
	}
}

void ParametricLinearExpression::setSummandsInfo(const SymbolicDescription* Var, const ExpressionBase* Coef)
{
	DeclarationBase* decl = Var->getDeclaration();
	ExpressionVariablesMap::iterator iter = m_occurenceDescriptions.find(decl);
	SymbolicDescription* VarExactly = Var->clone();
	if (iter == m_occurenceDescriptions.end())
	{ // не нашли объявление этой переменной
		OccurenceDescriptionList listVariables; 
		listVariables.push_back(ReprisePtr<SymbolicDescription>(VarExactly));
		m_occurenceDescriptions.insert(pair<DeclarationBase*, OccurenceDescriptionList>(decl, listVariables));
		VarExactly = listVariables.back().get();
	}
	else
	{ // объявление найдено
		bool found = false;
		list<ReprisePtr<SymbolicDescription> >::iterator list_iter = m_occurenceDescriptions[decl].begin();
		for(; list_iter != m_occurenceDescriptions[decl].end(); ++list_iter)
			if ((*list_iter)->isEqual(Var))
			{
				delete VarExactly;
				VarExactly = list_iter->get();
				found = true;
				break;
			}
		if (!found) // не нашли равного описания
		{
			m_occurenceDescriptions[decl].push_back(ReprisePtr<SymbolicDescription>(VarExactly));
			VarExactly = m_occurenceDescriptions[decl].back().get();
		}
	}
	
	if (m_summands.find(VarExactly) == m_summands.end())
		m_summands.insert(pair<SymbolicDescription*, Coefficient>(VarExactly, Coefficient(Coef->clone())));
	else
		m_summands[VarExactly].reset(Coef->clone());
}

void ParametricLinearExpression::copyFrom(const ParametricLinearExpression* Original)
{
	clear();

	// Копируем данные об объявлениях
	ExpressionVariablesMap::const_iterator itExprVar = Original->m_occurenceDescriptions.begin();
	for(; itExprVar != Original->m_occurenceDescriptions.end(); ++itExprVar)
		m_occurenceDescriptions.insert(make_pair(itExprVar->first, itExprVar->second));

	// Копируем слагаемые
	SummandsMap::const_iterator itSummand = Original->m_summands.begin();
	for (; itSummand != Original->m_summands.end(); itSummand++)
		setSummandsInfo(itSummand->first, itSummand->second.get());

	m_evaluatable = Original->m_evaluatable;
	m_freeSummand.reset(Original->m_freeSummand->clone());
}

ParametricLinearExpression* ParametricLinearExpression::createByAllVariables(Reprise::ExpressionBase* Node)
{
    list<Reprise::VariableDeclaration*> listOfBaseVariables = getAllVariableDeclarations(Node);
	VariablesDeclarationsVector vectorOfBaseVariables(listOfBaseVariables.begin(), listOfBaseVariables.end());
	return createByListOfVariables(Node, vectorOfBaseVariables);
}

ParametricLinearExpression* ParametricLinearExpression::createByIndexes(Reprise::ExpressionBase* Node)
{
	VariablesDeclarationsVector vectorOfBaseVariables = getIndexVariables(Node);
    return createByListOfVariables(Node, vectorOfBaseVariables);
}

Reprise::ReprisePtr<ExpressionBase> ParametricLinearExpression::simplify(ExpressionBase* Target)
{
	ReprisePtr<ExpressionBase> result(Target);
    ParametricLinearExpression* simplifier = ParametricLinearExpression::createByAllVariables(Target);
    if (!simplifier) // не упростилось
		return result;
	result.reset(simplifier->convert2RepriseExpression());
	delete simplifier;
	return result;
}


}
}
