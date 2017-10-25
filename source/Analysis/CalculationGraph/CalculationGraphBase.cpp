#ifdef _MSC_VER
#pragma warning(disable : 880)	// Запрещаем сообщение о отсутствии int в определении
//#pragma warning(disable : 1011)	// Запрещаем сообщение о осутствует return в non-void функции
#pragma warning(disable : 4786)	// Запрещаем сообщение об обрубании имен в debug-версии
#pragma warning(disable : 4503)	// Запрещаем сообщение об обрубании имен в debug-версии
#pragma warning(disable : 4355)	// Запрещаем сообщение о this в конструкторе
#endif


#include <algorithm>

#include "Analysis/CalculationGraph/CalculationGraphBase.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"
#include "Shared/ParametricLinearExpressions.h"
#include "Shared/LinearExpressions.h"
#include "Shared/LoopShared.h"
//#include "PipelineStartDelays.h"
#include "Shared/Checks.h"
#include "OPS_Core/Localization.h"
#include "OPS_Core/msc_leakcheck.h" //контроль утечек памяти должен находиться в конце всех include !!!

using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::PlatformSettingsSpace;
using namespace OPS::Montego;
using namespace std;


namespace CalculationGraphSpace
{

const char* g_strGraphName = "CalculationGraph";


// ================================================
//          class CalculationGraphDirEdge
// ================================================

CalculationGraphDirEdge::CalculationGraphDirEdge(CalculationGraphNode* Begin, CalculationGraphNode* End, int DepDistence, EN_SignalType SignalType, int SendingDelay):
m_DirEdgeType(DET_NEUTRAL), m_SignalType(SignalType), m_DependenceDistance(DepDistence), m_BufferLatency(0), m_SendingDelay(SendingDelay)
{
	if (Begin)
		m_Begin = Begin;
	else
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole(g_strGraphName);
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("NULL pointer in CalculationGraphDirEdge constructor!",""));
		throw new OPS::ArgumentError("CalculationGraphDirEdge::CalculationGraphDirEdge");
	}

	if (End)
		m_End = End;
	else
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole(g_strGraphName);
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("NULL pointer in CalculationGraphDirEdge constructor!",""));
		throw new OPS::ArgumentError("CalculationGraphDirEdge::CalculationGraphDirEdge");
	}
	// По умолчанию дуга не обратная
	m_InitializationDirEdge = nullptr;
}

string CalculationGraphDirEdge::getStringRepresentation()
{
    return  OPS::Strings::format("%d %d %d %d %d",
                m_Begin->getNodeNumber(),
                m_End->getNodeNumber(),
                m_DependenceDistance,
                (int)m_DirEdgeType,
                m_BufferLatency);
}

pair<list<CalculationGraphDirEdge*>::iterator, list<CalculationGraphDirEdge*>::iterator> CalculationGraphDirEdge::deleteOccurrences()
{
    list<CalculationGraphDirEdge*>::iterator
        itIn = getEndUnsafe()->m_InDirEdges.erase(getEndUnsafe()->findInDirEdgeIterator(this)),
        itOut = getBeginUnsafe()->m_OutDirEdges.erase(getBeginUnsafe()->findOutDirEdgeIterator(this));
    return make_pair(itIn, itOut);
}




// ================================================
//          class CalculationGraphNode
// ================================================

CalculationGraphNode::CalculationGraphNode(OPS::Reprise::ExpressionBase* Data, int OpDelay, EN_NodeType Type):
m_NodeNumber(0), m_OperationDelay(OpDelay), m_DataType(DT_EXPRESSION), m_NodeType(Type)
{
	m_Data.push_back(Data);
}

CalculationGraphNode::CalculationGraphNode(OPS::Reprise::IfStatement* Data, int OpDelay):
m_NodeNumber(0), m_OperationDelay(OpDelay), m_DataType(DT_STATEMENT), m_NodeType(NT_CMP)
{
	m_Data.push_back(Data);
}

CalculationGraphDirEdge* CalculationGraphNode::addInDirEdge(CalculationGraphNode* VertexFrom, int DepDistance, CalculationGraphDirEdge::EN_SignalType SignalType)
{
	return VertexFrom->addOutDirEdge(this, DepDistance, SignalType);
}

CalculationGraphDirEdge* CalculationGraphNode::addOutDirEdge(CalculationGraphNode* VertexTo, int DepDistance, CalculationGraphDirEdge::EN_SignalType SignalType)
{
	// Создаем дугу
	CalculationGraphDirEdge* DirEdge = new CalculationGraphDirEdge(this, VertexTo, DepDistance, SignalType);

	// Добавляем в текущий список дуг
	m_OutDirEdges.push_back(DirEdge);

	// Добавляем в список дуг начала дуги
	VertexTo->m_InDirEdges.push_back(DirEdge);

	return DirEdge;
}


list<CalculationGraphDirEdge*>::iterator CalculationGraphNode::findInDirEdgeIterator(CalculationGraphDirEdge* DirEdge)
{
    return std::find(m_InDirEdges.begin(), m_InDirEdges.end(), DirEdge);
}

std::list<CalculationGraphDirEdge*>::iterator CalculationGraphNode::findOutDirEdgeIterator(CalculationGraphDirEdge* DirEdge)
{
	return std::find(m_OutDirEdges.begin(), m_OutDirEdges.end(), DirEdge);
}

list<ExpressionBase*> CalculationGraphNode::getIRNodes()
{
	list<ExpressionBase*> res;
	if (m_DataType == DT_EXPRESSION)
        for (RepriseBase* it: m_Data)
            res.push_back(it->cast_ptr<ExpressionBase>());
	return res;
}

const ExpressionBase* CalculationGraphNode::getIRNode()
{
    return (m_DataType == DT_EXPRESSION) ? m_Data.front()->cast_ptr<ExpressionBase>() : nullptr ;
}

string CalculationGraphNode::getStringRepresentation()
{
    if (m_DataType != DT_EXPRESSION)
		if (m_Data.front()->is_a<IfStatement>())
			return "IF"; // должно соответствовать NT_CMP
		else
			return "";

	// У всех ExpressionBase приклееных в CalculationGraphNode должен быть один и тот же тип
	// (чтение данного из одинаковой переменной). Это следует из определения in-in склейки.
	ExpressionBase* pData = m_Data.front()->cast_ptr<ExpressionBase>();

	// Проверим не пустое ли это выражение
	if (pData->is_a<EmptyExpression>())
		return "Transit"; // должно соответствовать NT_TRANSIT

	// Операция
	if (pData->is_a<BasicCallExpression>())
	{
		BasicCallExpression* pCall = pData->cast_ptr<BasicCallExpression>();
		if (pCall->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS) // Массив
			return pCall->dumpState();
		else // Арифметическая операция
			return BasicCallExpression::builtinCallKindToString(pCall->getKind());
	}

	// Константа или скалярная переменная
	if ((pData->is_a<LiteralExpression>()) || (pData->is_a<ReferenceExpression>()))
		return pData->dumpState();

	// Ошибка, тип неопределен
	OPS::Console* const pConsole = &OPS::getOutputConsole(g_strGraphName);
	pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Error during representing a vertex has occured!",""));
	return "error";
}

void CalculationGraphNode::clear()
{
    for (CalculationGraphDirEdge* itCurrentInDirEdge: m_InDirEdges)
    {
    	CalculationGraphNode* BeginOfTheDirEdge = itCurrentInDirEdge->getBeginUnsafe();
		list<CalculationGraphDirEdge*>::iterator itDirEdge = BeginOfTheDirEdge->findOutDirEdgeIterator(itCurrentInDirEdge);
		BeginOfTheDirEdge->m_OutDirEdges.erase(itDirEdge);

		delete itCurrentInDirEdge;
    }

    for (CalculationGraphDirEdge* itCurrentOutDirEdge: m_OutDirEdges)
    {
        CalculationGraphNode* EndOfTheDirEdge = itCurrentOutDirEdge->getEndUnsafe();
		list<CalculationGraphDirEdge*>::iterator itDirEdge = EndOfTheDirEdge->findInDirEdgeIterator(itCurrentOutDirEdge);
		EndOfTheDirEdge->m_InDirEdges.erase(itDirEdge);

		delete itCurrentOutDirEdge;
    }

	m_InDirEdges.clear();
	m_OutDirEdges.clear();
}




// ================================================
//              Auxiliary
// ================================================



bool isArrayAccess(const ExpressionBase* expression)
{
    bool bArrayAccess = false;
    if (expression->getParent() != 0)
        if (expression->is_a<BasicCallExpression>())
            bArrayAccess = expression->cast_to<BasicCallExpression>().getKind() == BasicCallExpression::BCK_ARRAY_ACCESS;
    return bArrayAccess;
}


static inline BasicCallExpression* cast2ArrayAccess(ExpressionBase* expr)
{
    BasicCallExpression* srcArrExpr = expr->cast_ptr<BasicCallExpression>();
    if (srcArrExpr != nullptr && srcArrExpr->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
        return srcArrExpr;
    else
        return nullptr;
}

int calculateDependenceDistance(ExpressionBase* source, ExpressionBase* depended)
{
	// вхождения могут быть скалярами, массивами, вызовами ф-ций, структурами и пр.
	if (source->is_a<ReferenceExpression>() && depended->is_a<ReferenceExpression>())
		return 0;

    BasicCallExpression* srcArrExpr = cast2ArrayAccess(source);
    BasicCallExpression* depArrExpr = cast2ArrayAccess(depended);

    if (srcArrExpr == nullptr || depArrExpr == nullptr)
        return -3;

    // Разберем индексные выражения обоих вхождений
    unique_ptr<OPS::Shared::LinearExpressionMatrix> sourceLEM(new OPS::Shared::LinearExpressionMatrix(source)),
                                                    dependedLEM(new OPS::Shared::LinearExpressionMatrix(depended));

	// вне рассмотрения // TODO: исследовать как это может быть
	if (sourceLEM->getArrayDimensionsCount() != dependedLEM ->getArrayDimensionsCount())
		return -3;

	// Переменная, которая в результате работы этой функции будет
	// содержать необходимое расстояние зависимости.
	int iDepDistance = -1; // признак того, что это значение еще не рассчитано

	// Признак зависимости внутри конвейера
	// ПОКА ОДНОКОНВЕЙЕРНАЯ СИТУАЦИЯ
	//int iDependenceIsInPipeline = 1;
	vector<VariableDeclaration*> LoopCounters = OPS::Shared::getIndexVariables(source);
	int iLoopsCount = sourceLEM->getLoopNestDepth();
	for(int i=0; i < sourceLEM->getArrayDimensionsCount(); ++i)
	{
		for(int j=0; j<iLoopsCount; ++j)
			if (sourceLEM->getCoefficientAsInteger(i, j) != dependedLEM->getCoefficientAsInteger(i, j))
				return -2; // Нерегулярная зависимость - склеивание внутри конвейера невозможно


		if (sourceLEM->getCoefficientAsInteger(i, iLoopsCount-1) == 0) // коэффициент при счетчике внутреннего цикла =0
		{
			if (sourceLEM->getCoefficientAsInteger(i, iLoopsCount) - dependedLEM->getCoefficientAsInteger(i, iLoopsCount) != 0)
			{
				// В двух регулярных выражениях из двух размерностей генератора и
				// использования, различные свободные члены.
				// Значит это либо не зависимые вхождения, либо межконвейерные зависимости.
				// Например, x[i+1][i+2] -> x[i][i] - не зависимы.
				// Например, x[i+j+1][i+2] -> x[i+j][i] - зависимы, но между конвейерами.
				return -1; // exit
			}
		}
		else // Коэффициент при счетчике самого внутреннего цикла не 0
		{
			if (iDepDistance < 0)
			{
				// Первый раз инициализируем iDepDistance
				//iDependenceIsInPipeline = 1;
				iDepDistance = sourceLEM->getCoefficientAsInteger(i, iLoopsCount) - dependedLEM->getCoefficientAsInteger(i, iLoopsCount);
			}

			if (iDepDistance != sourceLEM->getCoefficientAsInteger(i, iLoopsCount) - dependedLEM->getCoefficientAsInteger(i, iLoopsCount))
			{
				// В двух регулярных выражениях из двух размерностей генератора и
				// использования, различные свободные члены.
				// Значит это либо не зависимые вхождения, либо межконвейерные зависимости.
				// Например, x[i+j+1][j+2] -> x[i+j][j] - зависимы, но между конвейерами.
				return -1; // exit
			}
		}
	}

	//if (iDependenceIsInPipeline == 1)
	// если при запуске одного конвейера эти вхождения таки зависимы, то ...
	return iDepDistance;
}

bool isCyclicallyGeneratedDependence(const DependenceGraphAbstractArc* arc)
{
    // todo: just stub
    return false;
}


int calculateDependenceDistance(const DependenceGraphAbstractArc* DependenceArc)
{
	ExpressionBase* sourceOccurAsExpr = DependenceArc->getStartVertex().getSourceOccurrence()->getSourceExpression();
	ExpressionBase* dependedOccurAsExpr = DependenceArc->getEndVertex().getSourceOccurrence()->getSourceExpression();
    int dist = calculateDependenceDistance(sourceOccurAsExpr, dependedOccurAsExpr);
    if (dist == 0 && isCyclicallyGeneratedDependence(DependenceArc))
        dist = 1;
    return dist;
}

bool isPipelinableOneDimLoop(const ForStatement* Loop, DependenceGraph* DepGraph)
{
    using namespace OPS::Shared::Checks;

    //* 1. Проверяем нет циклов вложенных в данный
    CompositionCheckObjects typeList;
    typeList << CompositionCheckObjects::CCOT_BlockStatement << CompositionCheckObjects::CCOT_BreakStatement
        << CompositionCheckObjects::CCOT_ContinueStatement << CompositionCheckObjects::CCOT_EmptyStatement
        << CompositionCheckObjects::CCOT_ExpressionStatement << CompositionCheckObjects::CCOT_GotoStatement
        << CompositionCheckObjects::CCOT_IfStatement << CompositionCheckObjects::CCOT_ReturnStatement
        << CompositionCheckObjects::CCOT_SwitchStatement;

    if (!makeCompositionCheck<BlockStatement>(const_cast<BlockStatement&>(Loop->getBody()), typeList))
        return false;

    return true;
}


} // CalculationGraphSpace
