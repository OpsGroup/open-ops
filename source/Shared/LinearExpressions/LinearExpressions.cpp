#include "Shared/LinearExpressions.h"
#include "Reprise/Service/DeepWalker.h"
#include "Shared/LoopShared.h"
#include "Shared/DataShared.h"
#include "Reprise/ServiceFunctions.h"
#include "OPS_Core/Strings.h"
#include "OPS_Core/Localization.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/ParametricLinearExpressions.h"
#include <memory>

#include "OPS_Core/msc_leakcheck.h" //контроль утечек памяти должен находиться в конце всех include !!!

using namespace OPS::Reprise;
using namespace OPS::Shared::ExpressionHelpers;
using namespace std;

#define PARAMETRIC_LINEAR_EXPRESSION_ERROR(message)	\
			{ OPS::Console* const pConsole = &OPS::getOutputConsole("ParametricLinearExpression"); \
			pConsole->log(OPS::Console::LEVEL_ERROR, _TL(message,"")); \
			m_ItIsLinear = false; \
			return; }


namespace OPS
{
namespace Shared
{

/// Визитор позволяющий строить линейное выражение
class LEVisitor: public OPS::Reprise::Service::DeepWalker
{
public:
    LEVisitor(const std::vector<Reprise::VariableDeclaration*>& IndexVariables);
    ~LEVisitor();
    ParametricLinearExpression* returnVisitResult();

    void visit(OPS::Reprise::ReferenceExpression&);
	void visit(OPS::Reprise::StructAccessExpression&);
	void visit(OPS::Reprise::BasicCallExpression&);

    void visit(OPS::Reprise::BasicLiteralExpression&);
    void visit(OPS::Reprise::StrictLiteralExpression&);
    void visit(OPS::Reprise::CompoundLiteralExpression&);

    void visit(OPS::Reprise::EnumAccessExpression&);
    void visit(OPS::Reprise::EmptyExpression&);
    void visit(OPS::Reprise::TypeCastExpression&);
    void visit(OPS::Reprise::SubroutineReferenceExpression&);

    void visit(OPS::Reprise::SubroutineCallExpression&);
    void visit(OPS::Reprise::Canto::HirCCallExpression&);
    void visit(OPS::Reprise::Canto::HirFIntrinsicCallExpression&);

private:
    std::vector<OPS::Reprise::VariableDeclaration*> m_listOfBaseVariables;
    std::stack<ParametricLinearExpression*> m_calculationsStack; // стек для обхода дерева выражения

    bool m_variableVisited;
    bool m_integerVisited;
    bool m_ItIsLinear;
};


LinearExpressionMatrix::LinearExpressionMatrix(ExpressionBase* Node) 
{
	m_OriginalExpression = Node;

	m_LinearizationBase = getIndexVariables(Node);

	// Инициализация полей
	m_loopNestDepth = (int)m_LinearizationBase.size();
	bool AllowedType = false;

	// 1. Пользователь класса подал на вход константу
	if (Node->is_a<LiteralExpression>())
	{
		// Инициализация полей
		m_arrayDimensionsCount = 0;

		ParametricLinearExpression* representation = new ParametricLinearExpression(m_LinearizationBase);
		if (Node->is_a<BasicLiteralExpression>())
			representation->add(Node->cast_to<BasicLiteralExpression>());
		if(Node->is_a<StrictLiteralExpression>())
			representation->add(Node->cast_to<StrictLiteralExpression>());
		m_linearExpressions.push_back(representation);
		AllowedType = true;
	}
	 
	// 2. Пользователь передал ReferenceExpression, который может оказаться скалярной переменной или именем массива.
	if (Node->is_a<ReferenceExpression>())
	{
		// Проверка на случай подачи имени массива // TODO: найти другую проверку на то что это массив через Declaration
		bool isArrayName = false;
		RepriseBase* Parent = Node->getParent();
		if ((Parent != 0) && Parent->is_a<CallExpressionBase>())
		{
			if (Parent->is_a<BasicCallExpression>()) 
			{
				BasicCallExpression& _Parent = Parent->cast_to<BasicCallExpression>();
				if (_Parent.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
				{
					// Сведем к п.4
					Node = &_Parent;
					isArrayName = true;
				}
			}
		}

		// 2.1 Это точно скалярная переменная
		if (!isArrayName)
		{
			// Инициализация полей
			m_arrayDimensionsCount = 0;

			ParametricLinearExpression* representation = new ParametricLinearExpression(m_LinearizationBase);
			representation->add(Node->cast_to<ReferenceExpression>());
			m_linearExpressions.push_back(representation);
			AllowedType = true;
		}
	}
	
	// 3. Это м.б. составное выражение или массив в дереве Reprise.
	if (Node->is_a<BasicCallExpression>())
	{
		BasicCallExpression* _Node = &(Node->cast_to<BasicCallExpression>());

		// Инициализация полей
		// заполним коэффициентами массив m_linearExpressions
		switch(_Node->getKind())
		{
		case BasicCallExpression::BCK_ARRAY_ACCESS:
			{
				// для массива рассматриваем его индексные выражения
				m_arrayDimensionsCount = _Node->getArgumentCount() -1;
				for(int i=1; i<=m_arrayDimensionsCount; ++i)
				{
					
					m_linearExpressions.push_back(ParametricLinearExpression::createByListOfVariables(&_Node->getArgument(i), m_LinearizationBase));
				}
				break;
			}
		case BasicCallExpression::BCK_MULTIPLY:
		case BasicCallExpression::BCK_BINARY_PLUS:
		case BasicCallExpression::BCK_BINARY_MINUS:
			{
				m_arrayDimensionsCount = 0; 
				m_linearExpressions.push_back(ParametricLinearExpression::createByListOfVariables(_Node, m_LinearizationBase));
				break;
			}
			// TODO:  Расширить список операций с которых может начинаться дерево выражения Reprise
		default:
			{
				// ошибка входных данных
				throw RuntimeError("Incorrect kind of parameter constructor LinearExpressionMatrix(ExpressionBase* Node)"); //LinearExpressionsException
			}
		}
		AllowedType = true;
	}

	if (!AllowedType)
	{
		// ошибка входных данных
		throw RuntimeError("Parameters are not allowed in linear expression"); //LinearExpressionsException
	}
}

LinearExpressionMatrix::~LinearExpressionMatrix()
{
	for(size_t i = 0; i < m_linearExpressions.size(); ++i)
		delete m_linearExpressions[i];
}

LinearExpressionMatrix::Coefficients LinearExpressionMatrix::detachCoefficients()
{
	Coefficients tmp;
	m_linearExpressions.swap(tmp);
	return tmp;
}

ParametricLinearExpression& LinearExpressionMatrix::operator[](unsigned i)
{
	if ((int)m_linearExpressions.size() > (int)i)
	{
		return *m_linearExpressions[i];
	}
	else
	{
		throw RuntimeError("LinearExpressionMatrix::operator[] : unexpected linear expression index");
	}
}

long_long_t LinearExpressionMatrix::getCoefficientAsInteger(unsigned i, unsigned j)
{
	if (j > m_loopNestDepth)
	{
		std::string strLoopNestDepth = Strings::format("%d", m_loopNestDepth);
		std::string strDemendedCounter = Strings::format("%d", j);
		throw RuntimeError("Incorrect counter number. Occurence has " + strLoopNestDepth + " covering loops, but counter number " + strDemendedCounter + " was demended"); //LinearExpressionsException
	}
	// порверки на валидность i выполнит ParametricLinearExpression

	if (j == m_loopNestDepth)
		return m_linearExpressions[i]->getFreeCoefficientAsInteger();
	else
		return m_linearExpressions[i]->getCoefficientAsInteger(m_LinearizationBase[j]);
}

ReprisePtr<ExpressionBase> Occurrence::getCoefficient(VariableDeclaration* BaseVariable, int DimensionNumber)
{
	return m_indexExpressionsRepresentation[DimensionNumber][BaseVariable];//.getCoefficient(BaseVariable);
}

ReprisePtr<ExpressionBase> Occurrence::getFreeCoefficient(int DimensionNumber)
{ 
	if (m_indexExpressionsRepresentation.getArrayDimensionsCount() > 0)
	{
		if ((DimensionNumber < -1) || (DimensionNumber > m_indexExpressionsRepresentation.getArrayDimensionsCount() - 1))
		{
			std::string strDimCount = Strings::format("%d", m_indexExpressionsRepresentation.getArrayDimensionsCount());
			std::string strDimNumber = Strings::format("%d", DimensionNumber);
			throw RuntimeError("Incorrect array dimension number. Array occurence " + m_node->dumpState() + " has " + strDimCount + " dimensions, but " + strDimNumber + " dimension was demended"); //LinearExpressionsException
		}
	}
	else
		if (DimensionNumber != 0)
			throw RuntimeError("Incorrect array dimension number."); //LinearExpressionsException

	return m_indexExpressionsRepresentation[DimensionNumber].getFreeCoefficient();
}

ParametricLinearExpression* ParametricLinearExpression::createByListOfVariables(ExpressionBase *Node, const VariablesDeclarationsVector& vectorOfBaseVariables)
{
    LEVisitor visitor(vectorOfBaseVariables);
    Node->accept(visitor);
    return visitor.returnVisitResult();
}

// LEVisitor class
LEVisitor::LEVisitor(const vector<VariableDeclaration*>& IndexVariables)
{
	m_listOfBaseVariables = IndexVariables;
	m_ItIsLinear = true;
}

LEVisitor::~LEVisitor()
{
	while(!m_calculationsStack.empty())
	{
		delete m_calculationsStack.top();
		m_calculationsStack.pop();
	}
}

ParametricLinearExpression* LEVisitor::returnVisitResult()
{ 
	if (m_ItIsLinear && !m_calculationsStack.empty())
		return new ParametricLinearExpression(*m_calculationsStack.top()); 
	else
		return 0;
}

void LEVisitor::visit(ReferenceExpression& basic)
{
	if (!m_ItIsLinear)
		return;
	m_calculationsStack.push(new ParametricLinearExpression(m_listOfBaseVariables, &basic));
}

void LEVisitor::visit(OPS::Reprise::StructAccessExpression& basic)
{
	if (!m_ItIsLinear)
		return;
	ReprisePtr<SymbolicDescription> p(SymbolicDescription::createSymbolicDescription(basic));
	if (!p.get())
		PARAMETRIC_LINEAR_EXPRESSION_ERROR("Unexpected StructAccessExpression!")
	m_calculationsStack.push(new ParametricLinearExpression(m_listOfBaseVariables, p.get()));
}

void LEVisitor::visit(BasicLiteralExpression& basic)
{
	if (!m_ItIsLinear)
		return;
	switch (basic.getLiteralType())
	{
	case BasicLiteralExpression::LT_INTEGER:
	case BasicLiteralExpression::LT_UNSIGNED_INTEGER:
		{
			m_calculationsStack.push(new ParametricLinearExpression(m_listOfBaseVariables, &basic));
			break;
		}
	default:
		// ошибка входных данных
		// TODO: сделать выход
		;
	}
}

void LEVisitor::visit(OPS::Reprise::StrictLiteralExpression& basic)
{
	if (!m_ItIsLinear)
		return;
	switch (basic.getLiteralType())
	{
    case BasicType::BT_CHAR:
    case BasicType::BT_WIDE_CHAR:
	case BasicType::BT_INT8:
	case BasicType::BT_INT16:
	case BasicType::BT_INT32:
	case BasicType::BT_INT64:
	case BasicType::BT_UINT8:
	case BasicType::BT_UINT16:
	case BasicType::BT_UINT32:
	case BasicType::BT_UINT64:
		{
			m_calculationsStack.push(new ParametricLinearExpression(m_listOfBaseVariables, &basic));
			break;
		}
	default:
		// ошибка входных данных
		// TODO: сделать выход
		;
	}
}

void LEVisitor::visit(OPS::Reprise::EnumAccessExpression& enumAccess)
{
	if (!m_ItIsLinear)
		return;

	StrictLiteralExpression sle(BasicType::BT_INT32);
	sle.setInt32(enumAccess.getMember().getValue());
	m_calculationsStack.push(new ParametricLinearExpression(m_listOfBaseVariables, &sle));
}

void LEVisitor::visit(BasicCallExpression& basic)
{
	if (basic.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
	{
		if (!m_ItIsLinear)
			return;
		ExpressionBase& p = basic.getArgument(0);
		while (!p.is_a<ReferenceExpression>())
			if (!p.is_a<BasicCallExpression>())
				PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports such operation for arrays!")
			else
				p = p.cast_to<BasicCallExpression>().getArgument(0);

        ReprisePtr<SymbolicDescription> baseOcc(SymbolicDescription::createSymbolicDescription(basic));
		if (!baseOcc.get())
			PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports such operation for arrays!")
        m_calculationsStack.push(new ParametricLinearExpression(m_listOfBaseVariables, baseOcc.get()));
		return;
	}

	DeepWalker::visit(basic);

	if (!m_ItIsLinear)
		return;

	switch(basic.getKind())
	{
	case BasicCallExpression::BCK_MULTIPLY:
	case BasicCallExpression::BCK_DIVISION:
		{
			std::unique_ptr<ParametricLinearExpression> Multiplier1(m_calculationsStack.top());
			m_calculationsStack.pop();
			std::unique_ptr<ParametricLinearExpression> Multiplier2(m_calculationsStack.top());
			m_calculationsStack.pop();

			if (basic.getKind() == BasicCallExpression::BCK_MULTIPLY)
			{
				if (Multiplier1->isIndependent() || Multiplier2->isIndependent())
				{
					Multiplier1->multiply(Multiplier2.get());
					m_calculationsStack.push(Multiplier1.release());
				}
				else
				{
					m_ItIsLinear = false;
					return;
				}
			}
			if (basic.getKind() == BasicCallExpression::BCK_DIVISION)
			{
				if (Multiplier1->isIndependent() || Multiplier2->isIndependent())
				{
					Multiplier2->divide(Multiplier1.get());
					m_calculationsStack.push(Multiplier2.release());
				}
				else
				{
					m_ItIsLinear = false;
					return;
				}
			}
			break;
		}
	case BasicCallExpression::BCK_BINARY_PLUS:
	case BasicCallExpression::BCK_BINARY_MINUS:
		{
			std::unique_ptr<ParametricLinearExpression> Summand1(m_calculationsStack.top());
			m_calculationsStack.pop();
			std::unique_ptr<ParametricLinearExpression> Summand2(m_calculationsStack.top());
			m_calculationsStack.pop();

			if (basic.getKind() == BasicCallExpression::BCK_BINARY_PLUS)
                m_calculationsStack.push(*Summand1 + *Summand2);
			if (basic.getKind() == BasicCallExpression::BCK_BINARY_MINUS)
                m_calculationsStack.push(*Summand2 - *Summand1);
			break;
		}
	case BasicCallExpression::BCK_UNARY_MINUS:
		{
			std::unique_ptr<ParametricLinearExpression> Summand(m_calculationsStack.top());
			m_calculationsStack.pop();
            m_calculationsStack.push(Summand->getOpposite());
			// Унарный минус может быть перед выражением, константой или переменной
			break;
		}
	case BasicCallExpression::BCK_UNARY_PLUS:
		{
			break;
		}
	default: 
		PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports such an operation as " +
													BasicCallExpression::builtinCallKindToString(basic.getKind()))
	}
}

void LEVisitor::visit(OPS::Reprise::EmptyExpression& basic)
{
	PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports empty expressions")
}

void LEVisitor::visit(OPS::Reprise::TypeCastExpression& basic)
{
	PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports type cast expressions")
}

void LEVisitor::visit(OPS::Reprise::SubroutineReferenceExpression& basic)
{
	PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports subroutine reference expressions")
}

void LEVisitor::visit(OPS::Reprise::SubroutineCallExpression& basic)
{
	PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports subroutine call expressions")
}

void LEVisitor::visit(OPS::Reprise::Canto::HirCCallExpression& basic)
{
	PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports HirCCallExpression")
}

void LEVisitor::visit(OPS::Reprise::Canto::HirFIntrinsicCallExpression& basic)
{
	PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports HirFIntrinsicCallExpression")
}

void LEVisitor::visit(OPS::Reprise::CompoundLiteralExpression& basic)
{
	PARAMETRIC_LINEAR_EXPRESSION_ERROR("ParametricLinearExpression do not supports compound literal expressions")
}

/// Класс позволяющий определить линейно ли выражение относительно списка переменных
/// линейным относительно {i,j,k} является выражение вида: 2*i+4*j-k+5;  нелинейным: A*i+4*j-2*k+B
class LEPredicate : public OPS::Reprise::Service::DeepWalker
{
public:
    LEPredicate():m_isLinear(true)
    {}
    LEPredicate(std::vector<OPS::Reprise::VariableDeclaration*>& listOfVar, OPS::Reprise::ExpressionBase* Node);
    bool isLinear(const std::vector<OPS::Reprise::VariableDeclaration*>& listOfVar, OPS::Reprise::ExpressionBase* Node);
    void visit(OPS::Reprise::ReferenceExpression& basic);
    //void visit(OPS::Reprise::Canto::HirCCallExpression& basic);
    void visit(OPS::Reprise::BasicCallExpression& basic);
    bool isLinear(){return m_isLinear;}
private:
    bool m_isLinear;
    std::vector<OPS::Reprise::VariableDeclaration*> m_listOfVar;
};

bool isLinear(const std::vector<OPS::Reprise::VariableDeclaration*>& listOfVar, OPS::Reprise::ExpressionBase* Node)
{
    OPS::Shared::LEPredicate lp;
    return lp.isLinear(listOfVar, Node);
}


LEPredicate::LEPredicate(std::vector<OPS :: Reprise :: VariableDeclaration *>& listOfVar, OPS::Reprise::ExpressionBase *Node)
{
	m_isLinear = true;
	m_listOfVar = listOfVar;
    if (Node->is_a<BasicCallExpression>())
		visit(Node->cast_to<BasicCallExpression>());
	else if (Node->is_a<ReferenceExpression>())
		visit(Node->cast_to<ReferenceExpression>());
	else if (Node->is_a<BasicLiteralExpression>())
		m_isLinear = true;
	else m_isLinear = false;
}
bool LEPredicate::isLinear(const std::vector<OPS :: Reprise :: VariableDeclaration *>& listOfVar, OPS::Reprise::ExpressionBase *Node)
{
	m_isLinear = true;
	m_listOfVar = listOfVar;
    /*if (Node->is_a<Canto::HirCCallExpression>())
		visit(Node->cast_to<Canto::HirCCallExpression>());
    else*/ if (Node->is_a<BasicCallExpression>())
		visit(Node->cast_to<BasicCallExpression>());
	else if (Node->is_a<ReferenceExpression>())
		visit(Node->cast_to<ReferenceExpression>());
	else if (Node->is_a<LiteralExpression>())
		return true;
	else m_isLinear = false;
	return m_isLinear;
}
void LEPredicate::visit(OPS::Reprise::BasicCallExpression &basic)
{
	DeepWalker::visit(basic);
	switch(basic.getKind())
	{
	case BasicCallExpression::BCK_UNARY_MINUS:
	case BasicCallExpression::BCK_UNARY_PLUS:
	case BasicCallExpression::BCK_BINARY_PLUS:
	case BasicCallExpression::BCK_BINARY_MINUS:
		break;
	case BasicCallExpression::BCK_DIVISION:
	case BasicCallExpression::BCK_MULTIPLY:
		{
			ExpressionBase& lArgument = basic.getArgument(0);
			ExpressionBase& rArgument = basic.getArgument(1);
			if (rArgument.is_a<ReferenceExpression>())
			{
				if (!lArgument.is_a<LiteralExpression>())
					m_isLinear = false;
			}
			if (lArgument.is_a<ReferenceExpression>())
			{
				if (!rArgument.is_a<LiteralExpression>())
					m_isLinear = false;
			}
		}
		break;
	default:
		m_isLinear = false;
		break;
	}
}
void LEPredicate::visit(OPS::Reprise::ReferenceExpression &basic)
{
	VariableDeclaration& varDecl = basic.getReference();
	unsigned int i=0;
	while( i<m_listOfVar.size() && m_listOfVar[i]!= &varDecl )
		i++;
	if(i==m_listOfVar.size())
		m_isLinear = false;
}

/*
	Implementation of OccurenceInfo
*/
OccurenceInfo::OccurenceInfo(OPS::Reprise::ExpressionBase& expression, bool isCStyle): m_isCStyle(isCStyle)
{
	initData(expression);
}

OccurenceInfo::OccurenceInfo(const OccurenceInfo& occurenceInfo): m_isCStyle(occurenceInfo.m_isCStyle)
{
	initData(*occurenceInfo.m_pExpression);
}

OccurenceInfo& OccurenceInfo::operator=(const OccurenceInfo& occurenceInfo)
{
	uninitData();

	initData(*occurenceInfo.m_pExpression);

	return *this;
}

ExpressionBase& OccurenceInfo::getExpression()
{
	return *m_pExpression;
}

LinearExpressionMatrix& OccurenceInfo::getExpressionMatrix()
{
	return *m_pExpressionMatrix;
}

Occurrence& OccurenceInfo::getOccurence()
{
	return *m_pOccurence;
}

OccurenceInfo::LoopsNest& OccurenceInfo::getLoopsNest()
{
	return m_loopsNest;
}

void OccurenceInfo::initData( OPS::Reprise::ExpressionBase& expression )
{
	this->m_pExpression = &expression;

	m_pDeclaration = Shared::getArrayVariableDeclaration(expression);
	if(m_pDeclaration != NULL)
	{
		Shared::getArrayLimits(&m_pDeclaration->getType(), m_limits);
	}

	this->m_pExpressionMatrix = new LinearExpressionMatrix(&expression);
	this->m_pOccurence = new Occurrence(&expression);
	
	StatementBase* pParentStatement = expression.obtainParentStatement();

	if(pParentStatement != NULL)
	{
		m_loopsNest = getEmbracedLoopsNest(*pParentStatement);
		m_loopsNest.reverse();
	}

	for(LoopsNest::iterator it = m_loopsNest.begin(); it != m_loopsNest.end(); ++it)
	{
		VariableDeclaration& loopCounter = Editing::getBasicForCounter(*(*it)).getReference();

		IntegerHelper ih(BasicType::BT_INT32);
		ReprisePtr<ExpressionBase> rpCurrentCoefficient(&ih(0));
		for(int i = 0; i < m_pExpressionMatrix->getArrayDimensionsCount(); ++i)  // Это цикл по []
		{
			// Get N1 * N2 * N3
			ReprisePtr<ExpressionBase> rpLinearisationMultiplier(&ih(1));
			if(m_isCStyle)
			{
				for(size_t j = i + 1; j < m_limits.size(); ++j)
				{
					rpLinearisationMultiplier = ReprisePtr<ExpressionBase>(&(*rpLinearisationMultiplier * ih(m_limits[j])));
				}
			}
			else
			{
				for(int j = 0; j < i; ++j)
				{
					rpLinearisationMultiplier = ReprisePtr<ExpressionBase>(&(*rpLinearisationMultiplier * ih(m_limits[j])));
				}
			}

			ReprisePtr<ExpressionBase> rpInputCoefficient = (*m_pExpressionMatrix)[i][&loopCounter];

			if(rpInputCoefficient.get() != NULL)
			{
				rpCurrentCoefficient = ReprisePtr<ExpressionBase>(&(*rpCurrentCoefficient + *rpLinearisationMultiplier * *rpInputCoefficient));
			}
		}

		if(!rpCurrentCoefficient->is_a<StrictLiteralExpression>())
		{
			pair<VariableDeclaration*, ReprisePtr<ExpressionBase> > currentCounterWithCoefficient(&loopCounter, rpCurrentCoefficient);
			m_linearisationCoefficients.insert(currentCounterWithCoefficient);
		}
	}
}

void OccurenceInfo::uninitData()
{
	delete m_pExpressionMatrix;
	delete m_pOccurence;
}

OccurenceInfo::~OccurenceInfo()
{
	uninitData();
}

OPS::Reprise::VariableDeclaration* OccurenceInfo::getDeclaration()
{
	return m_pDeclaration;
}

OccurenceInfo::ArrayLimits OccurenceInfo::getArrayLimits()
{
	return m_limits;
}

OccurenceInfo::LinearisationCoefficients& OccurenceInfo::getLinearisationCoefficients()
{
	return m_linearisationCoefficients;
}
}
}
