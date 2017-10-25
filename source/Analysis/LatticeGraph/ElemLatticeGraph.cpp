#include<memory>
#include<iostream>
#include "Analysis/LatticeGraph/LinDataForLatticeGraph.h"
#include "Analysis/LatticeGraph/LatticeGraph.h"
#include "Analysis/LatticeGraph/ExtendedQuast.h"
#include "Analysis/Montego/OccurrenceCoeffs.h"
#include<fstream>
#include <sstream>
#include<Analysis/DepGraph/Status.h>
#include "Shared/LoopShared.h"
#include "Analysis/LatticeGraph/LoopIndex.h"
#include "Shared/StatementsShared.h"
#include "Shared/LoopShared.h"
#include "Analysis/ControlFlowGraph.h"
#include "Shared/StatementsShared.h"

#include "OPS_Core/Localization.h"
#include "OPS_Core/msc_leakcheck.h"

//#define LATTICE_TABLES_DEBUG

#ifdef NDEBUG
#undef LATTICE_TABLES_DEBUG
#endif

#ifdef LATTICE_TABLES_DEBUG
std::ofstream file("latticeGraphDebug.txt");
int deepOfTheSimplexSearch;
#endif

using OPS::Console;

using namespace Id;

using namespace DepGraph;
using namespace OPS::Montego;

namespace OPS
{
namespace LatticeGraph
{

const std::string nonApplicableLinearClass="Программа для которой вызвано построение решетчатого графа должна удовлетворять условиям:\n" 
											"1. Операторы программы - операторы циклов For и операторы присваивания. \n"
											"2. Границы циклов начинаются с 0, шаг циклов равен +1. \n"
											"3. Индексные выражения переменных и границы циклов есть аффинные формы относительно счетчиков охватывающих циклов, НЕ содержащие внешних переменных.";

const std::string nonApplicableStmts="Программа для которой вызвано построение решетчатого графа, может содержать только операторы цикла For и операторы присваивания.";

static inline void latticeGraphLog(Console::MessageLevel level, const std::string& message) 
{
	OPS::getOutputConsole("LatticeGraph").log(level, message); 
}


void ElemLatticeGraph::clear()
{
    m_srcEntryCounterNames.clear();
	//Clearing statuses...
	ClearStatus((status_t)~(LG_DONT_TRY_BUILD_BY_INVERTING));//Стираем все, кроме настроечных
    delete m_FeautrierSolution;
    delete m_FeautrierSolutionInInitialVars;
    delete m_suppPolyhedronSrcEntry;
    delete m_suppPolyhedronDepEntry;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//int ElemLatticeGraph::TestLatticeBasedSupp(int supp)
//{
//	VoevodinSolution::iterator firstS=m_voevodinSolution.begin(),lastS=m_voevodinSolution.end();
//	while(firstS!=lastS)
//	{
//		if(supp==GetValueBasedSupp(**firstS,m_commonLoopsNumb))
//			return 1;//Нашли
//		firstS++;
//	}
//
//	return 0;
//}

int min(int a, int b)
{
    return a > b ? b : a;
}

//проверяет составные имена структур начиная с имени структуры до min(bracketNum1,bracketNum2) + оставшиеся без скобок дескрипторы
//возвращает 0 - все нормально, но имена не совпадают
//1 - совпадают
//-1 - экстраординарный случай использования скобок
//-2 - имена variableDeclaration разные
int isNamesEqual(Montego::BasicOccurrenceName name1, Montego::BasicOccurrenceName name2)
{
    if (name1.m_varDecl != name2.m_varDecl) return -2;
    int minBracketNum = min(name1.getAllBracketCount(), name2.getAllBracketCount());
    int passedBrNum = name1.m_refExprBracketCount, structDescrNum = 0;
    //экстраординарные случаи отбрасываем
    if ((passedBrNum < minBracketNum) && (name1.m_refExprBracketCount != name2.m_refExprBracketCount)) 
        return -1;

    while (passedBrNum < minBracketNum)
    {
        if ( name1.m_fields[structDescrNum].first != name2.m_fields[structDescrNum].first ) 
            return 0;
        passedBrNum += min(name1.m_fields[structDescrNum].second, name2.m_fields[structDescrNum].second);
        //экстраординарные случаи отбрасываем
        if ((passedBrNum < minBracketNum) && (name1.m_fields[structDescrNum].second != name2.m_fields[structDescrNum].second)) 
            return -1;

        structDescrNum++;
    }
    if (passedBrNum == minBracketNum)
        if (((int)name1.m_fields.size()>=structDescrNum+1) && ((int)name2.m_fields.size()>=structDescrNum+1))
        {
            //пропускаем цепочку одинаковых имен с нулевым числом скобок
            while ((name1.m_fields[structDescrNum].second == 0) && (name2.m_fields[structDescrNum].second == 0) 
                && (name1.m_fields[structDescrNum].first == name2.m_fields[structDescrNum].first))
            {
                structDescrNum++;
                if (((int)name1.m_fields.size()<structDescrNum+1) || ((int)name2.m_fields.size()<structDescrNum+1)) 
                    break;
            }
            if (((int)name1.m_fields.size()>=structDescrNum+1) && ((int)name2.m_fields.size()>=structDescrNum+1))
            {
                if (name1.m_fields[structDescrNum].first != name2.m_fields[structDescrNum].first)
                    return 0;
            }
        }

    return 1;
}



ExtendedQuast* ElemLatticeGraph::getFeautrierSolution()
{
    return m_FeautrierSolution;
}

ExtendedQuast* ElemLatticeGraph::getFeautrierSolutionInInitialVars() 
{
    if (m_FeautrierSolutionInInitialVars==NULL)   buildFeautrierSolutionInInitialVars(); 
    return m_FeautrierSolutionInInitialVars;
}

///	Массив имен счетчиков для вхождения-источника
std::vector<std::string>& ElemLatticeGraph::getSrcEntryCounterNames()
{
    return m_srcEntryCounterNames;
}

///	Массив имен счетчиков для зависимого вхождения
std::vector<std::string>& ElemLatticeGraph::getDepEntryOnlyCounterNames()
{
    return m_depEntryCounterNames;
}

///	Массив имен внешних параметров
std::vector<std::string>& ElemLatticeGraph::getExternalParamNames() 
{
    return m_externalParamNames;
}
std::vector<OPS::Reprise::VariableDeclaration*>& ElemLatticeGraph::getExternalParamsVector()
{
    return m_externalParamsVector;
}

///получить количество внешних параметров
int ElemLatticeGraph::getExternalParamNum()
{
    return m_externalParamNum;
}

/// Получить указатели на описания вхождений. Для антизависимости src - это зависимое вхождение, dep - это источник зависимости
OccurDesc* ElemLatticeGraph::getSrcOccurDesc()
{
    return m_srcEntry;
}
OccurDesc* ElemLatticeGraph::getDepOccurDesc()
{
    return m_depEntry;
}
Montego::BasicOccurrence* ElemLatticeGraph::getSrcOccurrence()
{
    return m_newsrcEntry;
}
Montego::BasicOccurrence* ElemLatticeGraph::getDepOccurrence()
{
    return m_newdepEntry;
}


/// Получить кол-во общих циклов для зависимых вхождений
int ElemLatticeGraph::getCommonLoopsNumb()
{
    return m_commonLoopsNumb;
}



StatementBase* ElemLatticeGraph::getProgramFragment()
{
    return m_program;
}

SimpleLinearExpression makeLinearExpression(int dim, int CountersNum, SimpleLinearExpression& loopCounterLinExpr,\
                                            OPS::Shared::CanonicalLinearExpression* extParamLinExpr, \
                                            std::vector<OPS::Reprise::VariableDeclaration*>& externalParamsVector)
{
    //преобразует линейное выражение счетчиков циклов loopCounterLinExpr 
    //и внешних параметров extParamLinExpr в одно линейное выражение 
    //чтобы можно было применить Transform
    //dim - размерность (включая свободный член) 
    //если кол-во счетчиков в loopCounterLinExpr меньше CountersNum, заполняем недостающие коэффициенты нулями
    //доводя количество счетчиков до CountersNum
    
    OPS_ASSERT(CountersNum>=loopCounterLinExpr.m_dim-1);
    SimpleLinearExpression rab(dim);
    for (int j=0;j<loopCounterLinExpr.m_dim;++j) 
        rab[j]=loopCounterLinExpr[j]; //коэффициенты при счетчиках и свободный член
    for (int j=loopCounterLinExpr.m_dim;j<CountersNum+1;++j) 
        rab[j]=0;//добавляем нули на место не присутствующих счетчиков
    //коэффициенты при внешних переменных
    //коэффициенты при внешних переменных
	for (int j=0;j<(int)externalParamsVector.size();++j)
		rab[1+CountersNum+j] = extParamLinExpr->getCoefficient(externalParamsVector[j]);
    return rab;
}
SimpleLinearExpression makeLinearExpression(int dim, int CountersNum, int* loopCounterLinExpr, \
                                            OPS::Shared::CanonicalLinearExpression* extParamLinExpr,\
                                            std::vector<OPS::Reprise::VariableDeclaration*>& externalParamsVector)
{
    //преобразует коэффициенты при счетчиках циклов loopCounterLinExpr 
    //и внешних параметров extParamLinExpr в одно линейное выражение 
    //чтобы можно было применить Transform
    //dim - размерность (включая свободный член) 

    //переставляем свободный член на первое место
    SimpleLinearExpression rab(CountersNum+1);
    for (int i=0;i<CountersNum;++i) rab.m_coefs[i+1]=loopCounterLinExpr[i]; 
    rab.m_coefs[0]=loopCounterLinExpr[CountersNum];     
    return makeLinearExpression(dim,CountersNum,rab,extParamLinExpr,externalParamsVector);
}


void makeFromLinearExpression(SimpleLinearExpression& expr, int dim, int CountersNum, SimpleLinearExpression& loopCounterLinExpr,OPS::Shared::CanonicalLinearExpression& extParamLinExpr,std::vector<OPS::Reprise::VariableDeclaration*>& externalParamsVector)
{
    //делает обратное преобразование к makeLinearExpression
    int externalParamNum=(int)externalParamsVector.size();
    //записываем назад в левую границу цикла
    for (int j=0;j<loopCounterLinExpr.m_dim;++j) 
        loopCounterLinExpr[j]=expr[j]; //коэффициенты при счетчиках
    //коэффициенты при внешних переменных
    //передаем адрес - так мы сможем присваивать полю private напрямую
    std::map<OPS::Reprise::VariableDeclaration*, long> extParams=extParamLinExpr.getMap();
    for (int j=0;j<externalParamNum;++j) extParams[externalParamsVector[j]]=expr[loopCounterLinExpr.m_dim+j];
}

void makeFromLinearExpression(SimpleLinearExpression& expr, int dim, int CountersNum, int* loopCounterLinExpr, OPS::Shared::CanonicalLinearExpression& extParamLinExpr,std::vector<OPS::Reprise::VariableDeclaration*>& externalParamsVector)
{
    //делает обратное преобразование к makeLinearExpression
    SimpleLinearExpression rab(CountersNum+1);
    makeFromLinearExpression(expr,dim,CountersNum, rab,extParamLinExpr,externalParamsVector);
    //переставляем свободный член на последнее место
    for (int i=0;i<CountersNum;++i) loopCounterLinExpr[i]=rab.m_coefs[i+1]; 
    loopCounterLinExpr[CountersNum]=rab.m_coefs[0];     
}

void makeUnique(std::list<OPS::Reprise::VariableDeclaration*>& externalParamsList)//удаляем одинаковые внешние параметры
{
    std::list<OPS::Reprise::VariableDeclaration*>::iterator it1,it2;
    for (it1=externalParamsList.begin(); it1!=externalParamsList.end(); it1++)
        for (it2=it1, it2++; it2!=externalParamsList.end();) 
            if (*it1==*it2) it2=externalParamsList.erase(it2);
            else it2++;
}
//строит список внешних параметров m_externalParamsVector
void ElemLatticeGraph::makeExternalVariableVector()
{
    //Делает набор внешних параметров одним и тем же (если параметр отсутствует, будем добавлять ноль)

    //строим список внешних параметров - это объединение четырех списков: 
    //внешние параметры контекстов srcEntry и depEntry и внешние параметры в индексных выражениях srcEntry и depEntry
    //параметры не дублируются, здесь происходит исключение одинаковых
    std::list<OPS::Reprise::VariableDeclaration*> externalParamsList;
	if(m_srcEntry->suppPolyhedron)
        externalParamsList.insert(externalParamsList.end(), m_srcEntry->suppPolyhedron->m_externalParamsVector.begin(),m_srcEntry->suppPolyhedron->m_externalParamsVector.end());
    std::list<OPS::Reprise::VariableDeclaration*> suppPolyhDepEntryExtParams;
	if(m_depEntry->suppPolyhedron)
        suppPolyhDepEntryExtParams.insert(suppPolyhDepEntryExtParams.end(), m_depEntry->suppPolyhedron->m_externalParamsVector.begin(),m_depEntry->suppPolyhedron->m_externalParamsVector.end());
    
    //внешние параметры в индексных выражениях
    std::list<OPS::Reprise::VariableDeclaration*> srcVars;
    //проверяем правильно ли депграф заполнил линейные выражения
    OPS_ASSERT((int)m_srcEntry->m_externalParamCoefs.size()==m_srcEntry->dim);
    OPS_ASSERT((int)m_depEntry->m_externalParamCoefs.size()==m_depEntry->dim);
    for (int i=0;i<m_srcEntry->dim;i++)  {//собираем со всех квадратных скобок
        std::list<OPS::Reprise::VariableDeclaration*> temp = m_srcEntry->m_externalParamCoefs[i].getVariables();
        if (temp.size()>0) srcVars.insert(srcVars.end(),temp.begin(),temp.end());
    }
    std::list<OPS::Reprise::VariableDeclaration*> depVars;
    for (int i=0;i<m_depEntry->dim;i++)  {
        std::list<OPS::Reprise::VariableDeclaration*> temp = m_depEntry->m_externalParamCoefs[i].getVariables();
        if (temp.size()>0) depVars.insert(depVars.end(),temp.begin(),temp.end());
    }
    
    externalParamsList.insert(externalParamsList.end(),suppPolyhDepEntryExtParams.begin(),suppPolyhDepEntryExtParams.end());
    externalParamsList.insert(externalParamsList.end(),srcVars.begin(),srcVars.end());
    externalParamsList.insert(externalParamsList.end(),depVars.begin(),depVars.end());
    makeUnique(externalParamsList);//удаляем одинаковые
    m_externalParamsVector.insert(m_externalParamsVector.end(),externalParamsList.begin(),externalParamsList.end());
    m_externalParamNum=(int)m_externalParamsVector.size();
    //конец построения
    //переставляем параметры произвольного знака в конец списка
    std::vector<OPS::Reprise::VariableDeclaration*>::iterator it=m_arbitraryParamVector.begin();
    for (int i=0;i<(int)m_arbitraryParamVector.size();i++,it++)   {
        //проверяем наличие такого параметра в m_externalParamsVector
        int ind=-1;
        for (int j=m_externalParamNum-1;j>=0;j--) 
            if (m_arbitraryParamVector[i]==m_externalParamsVector[j]) {ind=j; break;}
            if ((ind>=0)&&(ind!=m_externalParamNum-1)) {
                //переставляем этот параметр в конец, сдвигая вторую часть массива влево
                m_externalParamsVector.push_back(m_arbitraryParamVector[i]);
                m_externalParamsVector.erase(it);
            }
    }
}

//строит список внешних параметров m_externalParamsVector
void ElemLatticeGraph::newmakeExternalVariableVector()
{
    //Делает набор внешних параметров одним и тем же (если параметр отсутствует, будем добавлять ноль)

    //строим список внешних параметров - это объединение четырех списков: 
    //внешние параметры контекстов srcEntry и depEntry и внешние параметры в индексных выражениях srcEntry и depEntry
    //параметры не дублируются, здесь происходит исключение одинаковых
    std::list<OPS::Reprise::VariableDeclaration*> externalParamsList;
    if (m_suppPolyhedronSrcEntry)
        externalParamsList.insert(externalParamsList.end(), 
            m_suppPolyhedronSrcEntry->m_externalParamsVector.begin(),
            m_suppPolyhedronSrcEntry->m_externalParamsVector.end());
    std::list<OPS::Reprise::VariableDeclaration*> suppPolyhDepEntryExtParams;
    if (m_suppPolyhedronDepEntry)
        suppPolyhDepEntryExtParams.insert(suppPolyhDepEntryExtParams.end(), 
            m_suppPolyhedronDepEntry->m_externalParamsVector.begin(), 
            m_suppPolyhedronDepEntry->m_externalParamsVector.end());

    //внешние параметры в индексных выражениях
    std::list<OPS::Reprise::VariableDeclaration*> srcVars;
    std::vector<SimpleLinearExpression> loopCounterCoeffs;
    OPS::Montego::OccurrenceCoeffs srcEntryCoefs(*m_newsrcEntry);
    OPS::Montego::OccurrenceCoeffs depEntryCoefs(*m_newdepEntry);
    std::vector<OPS::Shared::CanonicalLinearExpression> externalParamCoefsSrc;
    std::vector<OPS::Shared::CanonicalLinearExpression> externalParamCoefsDep;
    srcEntryCoefs.getExternalParamAndLoopCounterCoefficients(loopCounterCoeffs, externalParamCoefsSrc, m_program);
    depEntryCoefs.getExternalParamAndLoopCounterCoefficients(loopCounterCoeffs, externalParamCoefsDep, m_program);
    //проверяем правильно ли депграф заполнил линейные выражения
    OPS_ASSERT((int)externalParamCoefsSrc.size() == m_newsrcEntry->getBracketCount());
    OPS_ASSERT((int)externalParamCoefsDep.size() == m_newdepEntry->getBracketCount());
    for (int i=0; i<m_newsrcEntry->getBracketCount(); i++)  
    {
        //собираем со всех квадратных скобок
        std::list<OPS::Reprise::VariableDeclaration*> temp = externalParamCoefsSrc[i].getVariables();
        if (temp.size()>0) srcVars.insert(srcVars.end(),temp.begin(),temp.end());
    }
    std::list<OPS::Reprise::VariableDeclaration*> depVars;
    for (int i=0; i<m_newdepEntry->getBracketCount(); i++)  {
        std::list<OPS::Reprise::VariableDeclaration*> temp = externalParamCoefsDep[i].getVariables();
        if (temp.size()>0) depVars.insert(depVars.end(),temp.begin(),temp.end());
    }

    externalParamsList.insert(externalParamsList.end(),suppPolyhDepEntryExtParams.begin(),suppPolyhDepEntryExtParams.end());
    externalParamsList.insert(externalParamsList.end(),srcVars.begin(),srcVars.end());
    externalParamsList.insert(externalParamsList.end(),depVars.begin(),depVars.end());
    makeUnique(externalParamsList);//удаляем одинаковые
    m_externalParamsVector.insert(m_externalParamsVector.end(),externalParamsList.begin(),externalParamsList.end());
    m_externalParamNum=(int)m_externalParamsVector.size();
    //конец построения
    //переставляем параметры произвольного знака в конец списка
    std::vector<OPS::Reprise::VariableDeclaration*>::iterator it=m_arbitraryParamVector.begin();
    for (int i=0;i<(int)m_arbitraryParamVector.size();i++,it++)   {
        //проверяем наличие такого параметра в m_externalParamsVector
        int ind=-1;
        for (int j=m_externalParamNum-1;j>=0;j--) 
            if (m_arbitraryParamVector[i]==m_externalParamsVector[j]) {ind=j; break;}
            if ((ind>=0)&&(ind!=m_externalParamNum-1)) {
                //переставляем этот параметр в конец, сдвигая вторую часть массива влево
                m_externalParamsVector.push_back(m_arbitraryParamVector[i]);
                m_externalParamsVector.erase(it);
            }
    }
}


//делает внутреннее представление графа, т.е. заполняет структуру m_linData
void  LinData::makeLinData(ElemLatticeGraph& graph)
{
    OccurDesc* srcEntry=graph.getSrcOccurDesc();
    OccurDesc* depEntry=graph.getDepOccurDesc();
    m_externalParamNum=graph.getExternalParamNum();
    m_commonLoopsNumb=graph.getCommonLoopsNumb();
    clear();
    m_externalParamNum=graph.getExternalParamNum();
    m_commonLoopsNumb=graph.getCommonLoopsNumb();
    m_loopNumbDepEntryOnly=depEntry->loopNumb-m_commonLoopsNumb;
    m_loopNumbSrcEntry=srcEntry->loopNumb;
    OPS_ASSERT(srcEntry->dim==depEntry->dim);
    m_coefsNumSrcDepEntry=srcEntry->dim;
    m_dimiSrcEntryTransformMatrix=srcEntry->loopNumb+m_externalParamNum+1;  
    m_dimjSrcEntryTransformMatrix=m_dimiSrcEntryTransformMatrix;
    m_dimiDepEntryTransformMatrix=depEntry->loopNumb+m_externalParamNum+1;  
    m_dimjDepEntryTransformMatrix=m_dimiDepEntryTransformMatrix;
    //индексные выражения srcEntry
    m_dimiSrcEntryTransformMatrix=srcEntry->loopNumb+m_externalParamNum+1; 
    m_dimjSrcEntryTransformMatrix=m_dimiSrcEntryTransformMatrix;
    m_coefsSrcEntry.resize(srcEntry->dim);
	
    OPS_ASSERT((int)srcEntry->m_externalParamCoefs.size()==srcEntry->dim);
    for (int i=0;i<srcEntry->dim;++i)  {
        //преобразуем индексные выражения к SimpleLinearExpression
        m_coefsSrcEntry[i]=makeLinearExpression(m_dimiSrcEntryTransformMatrix,srcEntry->loopNumb,\
            srcEntry->data[i], &srcEntry->m_externalParamCoefs[i],graph.getExternalParamsVector());
    }
    //индексные выражения depEntry
    m_dimiDepEntryTransformMatrix=depEntry->loopNumb+m_externalParamNum+1; 
    m_dimjDepEntryTransformMatrix=m_dimiDepEntryTransformMatrix;
    m_coefsDepEntry.resize(depEntry->dim);
    for (int i=0;i<depEntry->dim;++i)  {
        //преобразуем индексные выражения к SimpleLinearExpression
        m_coefsDepEntry[i]=makeLinearExpression(m_dimiDepEntryTransformMatrix,depEntry->loopNumb,\
            depEntry->data[i], &depEntry->m_externalParamCoefs[i],graph.getExternalParamsVector());
    }
    //границы циклов depEntry начиная с первого не общего, до самого внутреннего
    m_loopNumbDepEntryOnly=depEntry->loopNumb-m_commonLoopsNumb;
    if (m_loopNumbDepEntryOnly)   {
        m_loopLowerBoundForDepEntry = new SimpleLinearExpression[m_loopNumbDepEntryOnly];
        m_loopUpperBoundForDepEntry = new SimpleLinearExpression[m_loopNumbDepEntryOnly];
        for (int i=0;i<m_loopNumbDepEntryOnly;++i)  {
            //левая граница
            m_loopLowerBoundForDepEntry[i]=
				makeLinearExpression(m_dimjDepEntryTransformMatrix,depEntry->loopNumb,
				depEntry->loops[m_commonLoopsNumb+i].loopBounds.m_lower[0],
				&depEntry->loops[m_commonLoopsNumb+i].loopBounds.m_lowerExternalParamCoefs,
                graph.getExternalParamsVector());
            //правая граница
            m_loopUpperBoundForDepEntry[i]=
				makeLinearExpression(m_dimjDepEntryTransformMatrix,depEntry->loopNumb,
				depEntry->loops[m_commonLoopsNumb+i].loopBounds.m_upper[0],
				&depEntry->loops[m_commonLoopsNumb+i].loopBounds.m_upperExternalParamCoefs,
                graph.getExternalParamsVector());
        }
    }
    //потом границы всех циклов srcEntry начиная с верхнего
    m_loopLowerBoundForSrcEntry = new SimpleLinearExpression[srcEntry->loopNumb];
    m_loopUpperBoundForSrcEntry = new SimpleLinearExpression[srcEntry->loopNumb];
    for (int i=0;i<srcEntry->loopNumb;++i)  {
        //левая граница
        m_loopLowerBoundForSrcEntry[i]=
            makeLinearExpression(m_dimjSrcEntryTransformMatrix,srcEntry->loopNumb,\
            srcEntry->loops[i].loopBounds.m_lower[0],\
			&srcEntry->loops[i].loopBounds.m_lowerExternalParamCoefs,\
            graph.getExternalParamsVector());
        //правая граница
        m_loopUpperBoundForSrcEntry[i]=
            makeLinearExpression(m_dimjSrcEntryTransformMatrix,srcEntry->loopNumb,\
            srcEntry->loops[i].loopBounds.m_upper[0],\
			&srcEntry->loops[i].loopBounds.m_upperExternalParamCoefs,\
            graph.getExternalParamsVector());
    }
}

//делает внутреннее представление графа, т.е. заполняет структуру m_linData
void  LinData::newmakeLinData(ElemLatticeGraph& graph)
{
    Montego::BasicOccurrence* srcEntry=graph.getSrcOccurrence();
    Montego::BasicOccurrence* depEntry=graph.getDepOccurrence();
    m_externalParamNum = graph.getExternalParamNum();
    m_commonLoopsNumb=graph.getCommonLoopsNumb();
    clear();
    m_externalParamNum=graph.getExternalParamNum();
    m_commonLoopsNumb=graph.getCommonLoopsNumb();
	m_loopDescSrcEntry = buildLoopDesc(srcEntry->getRefExpr(), graph.getProgramFragment());
	m_loopDescDepEntry = buildLoopDesc(depEntry->getRefExpr(), graph.getProgramFragment());
    m_loopNumbSrcEntry=m_loopDescSrcEntry.size();
    m_loopNumbDepEntryOnly=m_loopDescDepEntry.size() - m_commonLoopsNumb;
    m_coefsNumSrcDepEntry = min(srcEntry->getBracketCount(), depEntry->getBracketCount());
    m_dimiSrcEntryTransformMatrix=m_loopNumbSrcEntry+m_externalParamNum+1;  
    m_dimjSrcEntryTransformMatrix=m_dimiSrcEntryTransformMatrix;
    m_dimiDepEntryTransformMatrix=m_loopNumbDepEntryOnly+m_commonLoopsNumb+m_externalParamNum+1;  
    m_dimjDepEntryTransformMatrix=m_dimiDepEntryTransformMatrix;

    //список счетчиков и внешних параметров в нужном порядке для srcEntry
    std::vector<VariableDeclaration*> extParams = graph.getExternalParamsVector();
    std::vector<OPS::Reprise::VariableDeclaration*> allVarsSrc 
        = OPS::Shared::getIndexVariables(graph.getSrcOccurrence()->getRefExpr(), graph.getProgramFragment());
    allVarsSrc.insert(allVarsSrc.end(), extParams.begin(), extParams.end());

    //индексные выражения srcEntry
    m_dimiSrcEntryTransformMatrix=m_loopNumbSrcEntry+m_externalParamNum+1; 
    m_dimjSrcEntryTransformMatrix=m_dimiSrcEntryTransformMatrix;
    OPS::Montego::OccurrenceCoeffs srcEntryCoeffs(*srcEntry);
    srcEntryCoeffs.getAllVarsCoeffsInOrder(m_coefsSrcEntry, allVarsSrc);

    //список счетчиков и внешних параметров в нужном порядке для srcEntry
    std::vector<OPS::Reprise::VariableDeclaration*> allVarsDep 
        = OPS::Shared::getIndexVariables(graph.getDepOccurrence()->getRefExpr(), graph.getProgramFragment());
    allVarsDep.insert(allVarsDep.end(), extParams.begin(), extParams.end());

    //индексные выражения depEntry
    m_dimiDepEntryTransformMatrix=m_loopNumbDepEntryOnly+m_commonLoopsNumb+m_externalParamNum+1; 
    m_dimjDepEntryTransformMatrix=m_dimiDepEntryTransformMatrix;
    OPS::Montego::OccurrenceCoeffs depEntryCoeffs(*depEntry);
    depEntryCoeffs.getAllVarsCoeffsInOrder(m_coefsDepEntry, allVarsDep);

    //границы циклов depEntry начиная с первого не общего, до самого внутреннего
    if (m_loopNumbDepEntryOnly)   {
        m_loopLowerBoundForDepEntry = new SimpleLinearExpression[m_loopNumbDepEntryOnly];
        m_loopUpperBoundForDepEntry = new SimpleLinearExpression[m_loopNumbDepEntryOnly];
        for (int i=0; i<m_loopNumbDepEntryOnly; ++i)  {
            //левая граница
            m_loopLowerBoundForDepEntry[i]=
                makeLinearExpression(m_dimjDepEntryTransformMatrix, m_loopNumbDepEntryOnly+m_commonLoopsNumb,
                m_loopDescDepEntry[m_commonLoopsNumb+i].loopBounds.m_lower[0],
                &m_loopDescDepEntry[m_commonLoopsNumb+i].loopBounds.m_lowerExternalParamCoefs,
                graph.getExternalParamsVector());
            //правая граница
            m_loopUpperBoundForDepEntry[i]=
                makeLinearExpression(m_dimjDepEntryTransformMatrix,m_loopNumbDepEntryOnly+m_commonLoopsNumb,
                m_loopDescDepEntry[m_commonLoopsNumb+i].loopBounds.m_upper[0],
                &m_loopDescDepEntry[m_commonLoopsNumb+i].loopBounds.m_upperExternalParamCoefs,
                graph.getExternalParamsVector());
        }
    }
    //потом границы всех циклов srcEntry начиная с верхнего
    m_loopLowerBoundForSrcEntry = new SimpleLinearExpression[m_loopNumbSrcEntry];
    m_loopUpperBoundForSrcEntry = new SimpleLinearExpression[m_loopNumbSrcEntry];
    for (int i=0;i<m_loopNumbSrcEntry;++i)  {
        //левая граница
        m_loopLowerBoundForSrcEntry[i]=
            makeLinearExpression(m_dimjSrcEntryTransformMatrix,m_loopNumbSrcEntry,\
            m_loopDescSrcEntry[i].loopBounds.m_lower[0],\
            &m_loopDescSrcEntry[i].loopBounds.m_lowerExternalParamCoefs,\
            graph.getExternalParamsVector());
        //правая граница
        m_loopUpperBoundForSrcEntry[i]=
            makeLinearExpression(m_dimjSrcEntryTransformMatrix,m_loopNumbSrcEntry,\
            m_loopDescSrcEntry[i].loopBounds.m_upper[0],\
            &m_loopDescSrcEntry[i].loopBounds.m_upperExternalParamCoefs,\
            graph.getExternalParamsVector());
    }
}

//делает преобразованияе циклов к нулевым нижним границам, заполняет  m_linDataWithZeroLowerBounds 
//(только правые границы циклов!!!!!!!!!!!! левые не инициализируются!!!!!!!!!!!!!!!!!!!)
//придется еще делать замену в индексных выражениях: i=i'+a
//заполняем m_linDataWithZeroLowerBounds.m_transformMatrixSrcEntry и m_transformMatrixDepEntry
void LinDataWithZeroLowerBounds::makeZeroLowerBounds(LinData &linData)
{
    copyAllExceptTransform(linData);
    //Для srcEntry заполняем матрицу обратной замены переменных (новые через старые)
    m_invTransformMatrixSrcEntry=new int*[m_dimiSrcEntryTransformMatrix];
    for (int i=0;i<m_loopNumbSrcEntry;++i) {
        m_invTransformMatrixSrcEntry[i]=new int[m_dimjSrcEntryTransformMatrix];
        for (int j=0; j<m_dimjSrcEntryTransformMatrix-1; ++j) 
            m_invTransformMatrixSrcEntry[i][j]=-m_loopLowerBoundForSrcEntry[i][j+1];//-a в формуле i'_k=i_k-a
        m_invTransformMatrixSrcEntry[i][m_dimjSrcEntryTransformMatrix-1]=
            -m_loopLowerBoundForSrcEntry[i][0];//свободный член
        m_invTransformMatrixSrcEntry[i][i]=1; //i_k в формуле i'_k=i_k+a
    }
    //параметры не изменяем
    for (int i=m_loopNumbSrcEntry;i<m_dimiSrcEntryTransformMatrix;++i) {
        m_invTransformMatrixSrcEntry[i]=new int[m_dimjSrcEntryTransformMatrix];
        for (int j=0;j<m_dimjSrcEntryTransformMatrix;++j) 
            m_invTransformMatrixSrcEntry[i][j]=0;
        m_invTransformMatrixSrcEntry[i][i]=1; 
    }
    //Конец "Для srcEntry заполняем матрицу замены переменных (старые через новые)"
    
    //Для depEntry заполняем матрицу обратной замены переменных (новые через старые)
    m_invTransformMatrixDepEntry=new int*[m_dimiDepEntryTransformMatrix];
    //Общие циклы: строки i=0..m_commonLoopsNumb в матрице TransformMatrixDepEntry те же самые
    //только теперь количество столбцов другое!!!!!!!!!!!!!!!!!
    for (int i=0;i<m_commonLoopsNumb;++i) {
        m_invTransformMatrixDepEntry[i]=new int[m_dimjDepEntryTransformMatrix];
        for (int j=0; j<m_dimjDepEntryTransformMatrix; ++j)
            m_invTransformMatrixDepEntry[i][j]=0; //обнуляем
        for (int j=0; j<m_commonLoopsNumb; ++j)//переписываем коэффициенты при счетчиках общих циклов
            m_invTransformMatrixDepEntry[i][j]=m_invTransformMatrixSrcEntry[i][j];
        //свободный член
        m_invTransformMatrixDepEntry[i][m_dimjDepEntryTransformMatrix-1]=m_invTransformMatrixSrcEntry[i][m_dimjSrcEntryTransformMatrix-1];
        for (int j=0; j<m_externalParamNum; ++j)//переписываем коэффициенты при параметрах
            m_invTransformMatrixDepEntry[i][j+m_loopNumbDepEntryOnly+m_commonLoopsNumb]=m_invTransformMatrixSrcEntry[i][j+m_loopNumbSrcEntry];
    }
    //Циклы только лично depEntry
    for (int i=m_commonLoopsNumb;i<m_loopNumbDepEntryOnly+m_commonLoopsNumb;++i) {
        m_invTransformMatrixDepEntry[i]=new int[m_dimjDepEntryTransformMatrix];
        for (int j=0; j<m_dimjDepEntryTransformMatrix-1; ++j) 
            m_invTransformMatrixDepEntry[i][j]=-m_loopLowerBoundForDepEntry[i-m_commonLoopsNumb][j+1];//-a в формуле i'_k=i_k-a
        m_invTransformMatrixDepEntry[i][m_dimjDepEntryTransformMatrix-1]=-m_loopLowerBoundForDepEntry[i-m_commonLoopsNumb][0];//свободный член
        m_invTransformMatrixDepEntry[i][i]=1; //i_k в формуле i'_k=i_k-a
    }
    //параметры не изменяем
    for (int i=m_loopNumbDepEntryOnly+m_commonLoopsNumb;i<m_dimiDepEntryTransformMatrix;++i) {
        m_invTransformMatrixDepEntry[i]=new int[m_dimjDepEntryTransformMatrix];
        for (int j=0;j<m_dimjDepEntryTransformMatrix;++j) 
            m_invTransformMatrixDepEntry[i][j]=0;
        m_invTransformMatrixDepEntry[i][i]=1; 
    }
    //КОНЕЦ "Для depEntry заполняем матрицу замены переменных (старые через новые)"

    //строим обратные матрицы
    buildTransformMatrixDepEntry();
    buildTransformMatrixSrcEntry();
    
    //заменяем всё и вся согласно заполненным матрицам преобразований
    applyTransform();
}

void ElemLatticeGraph::makeIterationSpaces(LinearExprDataForLatticeGraph& linData)
{
    m_iterationSpaceForSrcEntry.m_externalParamsVector=m_externalParamsVector;
    m_iterationSpaceForDepEntry.m_externalParamsVector=m_externalParamsVector;
	//int externalParamNum = linData.m_externalParamNum;
    int loopNumbSrcEntry = linData.m_loopNumbSrcEntry;
    int commonLoopsNumb = linData.m_commonLoopsNumb;
    int loopNumbDepEntryOnly = linData.m_loopNumbDepEntryOnly;
    //общие неравенства для srcEntry и depEntry
    for (int i=0; i<loopNumbSrcEntry; ++i)   
    {
        SimpleLinearExpression* ex = new SimpleLinearExpression(linData.m_loopUpperBoundForSrcEntry[i]);
        ex->m_coefs[i+1] = -1;
        //ex->insertNZerosBeforeNewParamCoefs(loopNumbSrcEntry-i,i);//вставляем нули - коэффициенты при счетчиках нижних циклов
        m_iterationSpaceForSrcEntry.m_ins.push_back((Inequality*)ex);
        if ( i < m_commonLoopsNumb ) 
        {
            SimpleLinearExpression* ex2=new SimpleLinearExpression(linData.m_loopUpperBoundForSrcEntry[i]);
            ex2->m_coefs[i+1] = -1;
            //вставляем или удаляем лишние нули - коэффициенты при счетчиках внутренних циклов depEntry
            if (loopNumbSrcEntry < commonLoopsNumb + loopNumbDepEntryOnly)
            {
                int diff = commonLoopsNumb + loopNumbDepEntryOnly - loopNumbSrcEntry;
                ex2->insertNZerosBeforeNewParamCoefs(diff, loopNumbSrcEntry);
            }
            if (loopNumbSrcEntry > commonLoopsNumb + loopNumbDepEntryOnly) //надо удалить нули
            {
                int diff = loopNumbSrcEntry - commonLoopsNumb + loopNumbDepEntryOnly;
                ex2->deleteNZerosBeforeNewParamCoefs(diff, loopNumbSrcEntry);
            }
            m_iterationSpaceForDepEntry.m_ins.push_back((Inequality*)ex2);
        }
    }
    //для depEntry
    for (int i=0; i<linData.m_loopNumbDepEntryOnly; ++i)   
    {
        SimpleLinearExpression* ex=new SimpleLinearExpression(linData.m_loopUpperBoundForDepEntry[i]);
        ex->m_coefs[i+1+m_commonLoopsNumb]=-1;
        //ex->insertNZerosBeforeNewParamCoefs(commonLoopsNumb+loopNumbDepEntryOnly-(i+commonLoopsNumb),i+commonLoopsNumb);
        m_iterationSpaceForDepEntry.m_ins.push_back((Inequality*)ex);
    }
    //неравенства: счетчики >=0 не нужны, мы их потом добавим в симплекс таблицу по умолчанию
    
}

//делает преобразования i'=b-i для счетчиков циклов srcEntry заполняет  m_linDataForTrueDep
//придется делать замену в верхних границах всех! циклов и во всех индексных выражениях
//будем заполнять матрицы преобразований m_linDataForTrueDep.m_transformMatrixSrcEntry и m_transformMatrixDepEntry
void LinDataForTrueDep::makeTransformForTrueDep(LinDataWithZeroLowerBounds& linDataWithZeroLowerBounds)
{
    //Для тех циклов depEntry, которые НЕ являются общими с srcEntry: никакой замены не делаем!!!!
    
    copyAllExceptTransform(linDataWithZeroLowerBounds);
    //Для srcEntry заполняем матрицу обратной замены переменных (новые через старые)
    m_invTransformMatrixSrcEntry=new int*[m_dimiSrcEntryTransformMatrix];
    for (int i=0;i<m_loopNumbSrcEntry;++i) {
        m_invTransformMatrixSrcEntry[i]=new int[m_dimjSrcEntryTransformMatrix];
        for (int j=0; j<m_dimjSrcEntryTransformMatrix-1; ++j) 
            m_invTransformMatrixSrcEntry[i][j]=linDataWithZeroLowerBounds.m_loopUpperBoundForSrcEntry[i][j+1];//b в формуле i'_k=b-i_k
        m_invTransformMatrixSrcEntry[i][m_dimjSrcEntryTransformMatrix-1]=
            linDataWithZeroLowerBounds.m_loopUpperBoundForSrcEntry[i][0];//свободный член
        m_invTransformMatrixSrcEntry[i][i]=-1; //-i_k в формуле i'_k=b-i_k
    }
    //параметры не изменяем
    for (int i=m_loopNumbSrcEntry;i<m_dimiSrcEntryTransformMatrix;++i) {
        m_invTransformMatrixSrcEntry[i]=new int[m_dimjSrcEntryTransformMatrix];
        for (int j=0;j<m_dimjSrcEntryTransformMatrix;++j) 
            m_invTransformMatrixSrcEntry[i][j]=0;
        m_invTransformMatrixSrcEntry[i][i]=1; 
    }
    //Конец "Для srcEntry заполняем матрицу замены переменных (старые через новые)"
    
    //Для depEntry заполняем матрицу обратной замены переменных (новые через старые)
    m_invTransformMatrixDepEntry=new int*[m_dimiDepEntryTransformMatrix];
    //Общие циклы: строки i=0..m_commonLoopsNumb в матрице TransformMatrixDepEntry те же самые
    //только теперь количество столбцов другое!!!!!!!!!!!!!!!!!
    for (int i=0;i<m_commonLoopsNumb;++i) {
        m_invTransformMatrixDepEntry[i]=new int[m_dimjDepEntryTransformMatrix];
        for (int j=0; j<m_dimjDepEntryTransformMatrix; ++j)
            m_invTransformMatrixDepEntry[i][j]=0; //обнуляем
        for (int j=0; j<m_commonLoopsNumb; ++j)//переписываем коэффициенты при счетчиках общих циклов
            m_invTransformMatrixDepEntry[i][j]=m_invTransformMatrixSrcEntry[i][j];
        //свободный член
        m_invTransformMatrixDepEntry[i][m_dimjDepEntryTransformMatrix-1]=m_invTransformMatrixSrcEntry[i][m_dimjSrcEntryTransformMatrix-1];
        for (int j=0; j<m_externalParamNum; ++j)//переписываем коэффициенты при параметрах
            m_invTransformMatrixDepEntry[i][j+m_loopNumbDepEntryOnly+m_commonLoopsNumb]=m_invTransformMatrixSrcEntry[i][j+m_loopNumbSrcEntry];
    }
    //Циклы только лично depEntry
    for (int i=m_commonLoopsNumb;i<m_loopNumbDepEntryOnly+m_commonLoopsNumb;++i) {
        m_invTransformMatrixDepEntry[i]=new int[m_dimjDepEntryTransformMatrix];
        for (int j=0; j<m_dimjDepEntryTransformMatrix-1; ++j) 
            m_invTransformMatrixDepEntry[i][j]=linDataWithZeroLowerBounds.m_loopUpperBoundForDepEntry[i-m_commonLoopsNumb][j+1];//b в формуле i'_k=b-i_k
        m_invTransformMatrixDepEntry[i][m_dimjDepEntryTransformMatrix-1]=linDataWithZeroLowerBounds.m_loopUpperBoundForDepEntry[i-m_commonLoopsNumb][0];//свободный член
        m_invTransformMatrixDepEntry[i][i]=-1; //-i_k в формуле i'_k=b-i_k
    }
    //параметры не изменяем
    for (int i=m_loopNumbDepEntryOnly+m_commonLoopsNumb;i<m_dimiDepEntryTransformMatrix;++i) {
        m_invTransformMatrixDepEntry[i]=new int[m_dimjDepEntryTransformMatrix];
        for (int j=0;j<m_dimjDepEntryTransformMatrix;++j) 
            m_invTransformMatrixDepEntry[i][j]=0;
        m_invTransformMatrixDepEntry[i][i]=1; 
    }
    //КОНЕЦ "Для depEntry заполняем матрицу замены переменных (старые через новые)"

    //строим обратные матрицы
    buildTransformMatrixDepEntry();
    buildTransformMatrixSrcEntry();

    //заменяем всё и вся согласно заполненным матрицам преобразований
    applyTransform();
}
//заполняет матрицу m_transformMatrixDepEntry
void LinearExprDataForLatticeGraph::buildTransformMatrixDepEntry()
{
    int err = GetInvertedMatrix(m_invTransformMatrixDepEntry,m_dimiDepEntryTransformMatrix,m_dimjDepEntryTransformMatrix,m_transformMatrixDepEntry);
    if (err!=0) throw OPS::RuntimeError("LinDataForTrueDep::buildTransformMatrixDepEntry: Can't build inverted matrix to m_invTransformMatrixDepEntry");
}
//заполняет матрицу m_transformMatrixSrcEntry
void LinearExprDataForLatticeGraph::buildTransformMatrixSrcEntry()
{
    int err = GetInvertedMatrix(m_invTransformMatrixSrcEntry,m_dimiSrcEntryTransformMatrix,m_dimjSrcEntryTransformMatrix,m_transformMatrixSrcEntry);
    if (err!=0) throw OPS::RuntimeError("LinDataForTrueDep::buildTransformMatrixSrcEntry: Can't build inverted matrix to m_invTransformMatrixSrcEntry");
}

void ElemLatticeGraph::buildFeautrierSolutionInInitialVars()
{
    if (m_FeautrierSolutionInInitialVars==NULL)  {
        m_FeautrierSolutionInInitialVars = m_FeautrierSolution->clone();
        //вторая матрица обязательно прямая, а не обратная и обязательно srcEntry
        m_FeautrierSolutionInInitialVars->transform(m_linDataWithZeroLowerBounds.m_invTransformMatrixDepEntry,m_linDataWithZeroLowerBounds.m_transformMatrixSrcEntry,m_linDataWithZeroLowerBounds.m_dimjSrcEntryTransformMatrix);
    }
}

ElemLatticeGraph::~ElemLatticeGraph()
{
    clear();
}

ElemLatticeGraph::ElemLatticeGraph(OccurDesc& srcEntry, OccurDesc& depEntry, EN_DepType depType
                                   , bool flagIncludeDepAtSameIter
                                   , std::vector<OPS::Reprise::VariableDeclaration*>* arbitraryParamVector)
								   : m_FeautrierSolution(NULL), m_FeautrierSolutionInInitialVars(NULL)
								   , m_srcEntry(&srcEntry), m_depEntry(&depEntry)
								   , m_newsrcEntry(0), m_newdepEntry(0)
								   , m_program(0)
								   , m_aliasInterface(0)
								   , m_depType(depType)
								   , m_flagIncludeDepAtSameIter(flagIncludeDepAtSameIter)
                                   , m_suppPolyhedronSrcEntry(0)
                                   , m_suppPolyhedronDepEntry(0)

{
    if (arbitraryParamVector != 0) 
    {
        m_arbitraryParamVector = *arbitraryParamVector;
        build(false);
    }
    else
        build(true);
}

ElemLatticeGraph::ElemLatticeGraph(StatementBase& program, Montego::AliasInterface& ai
                                   , Montego::Occurrence& srcEntry, Montego::Occurrence& depEntry
                                   , Montego::DependenceGraphAbstractArc::DependenceType depType
                                   , bool flagIncludeDepAtSameIter
                                   , std::vector<OPS::Reprise::VariableDeclaration*>* arbitraryParamVector)
                                   : m_FeautrierSolution(NULL), m_FeautrierSolutionInInitialVars(NULL)
								   , m_srcEntry(0), m_depEntry(0)
								   , m_program(&program), m_aliasInterface(&ai)
								   , m_newdepType(depType)
                                   , m_flagIncludeDepAtSameIter(flagIncludeDepAtSameIter)
                                   , m_suppPolyhedronSrcEntry(0)
                                   , m_suppPolyhedronDepEntry(0)

{
    m_newsrcEntry = srcEntry.cast_ptr<Montego::BasicOccurrence>();
    m_newdepEntry = depEntry.cast_ptr<Montego::BasicOccurrence>();
    if ((m_newsrcEntry == 0) || (m_newdepEntry == 0)) 
    {
        SetStatus(LG_ERROR_INIT);
        return;
    }
    if (arbitraryParamVector != 0) 
    {
        m_arbitraryParamVector = *arbitraryParamVector;
        newbuild(false);
    }
    else
        newbuild(true);
}


void ElemLatticeGraph::build(bool flagAllExternalParamsAreArbitrary)
{
    if (TestApplicability(m_srcEntry,m_depEntry) == false)	
    {
		SetStatus(LG_ERROR_INIT);
		return;
	}
    if ((m_srcEntry->dim == 0) || (m_depEntry->dim == 0))
    {
        SetStatus(LG_ERROR_INIT);
        return;
    }

    m_commonLoopsNumb = getCommonUpperLoopsAmount(*m_srcEntry, *m_depEntry);

    //строит список внешних параметров m_externalParamsVector
    makeExternalVariableVector();
    if (flagAllExternalParamsAreArbitrary)    m_arbitraryParamVector = m_externalParamsVector;
//    makeExtParamPositive(m_arbitraryParamVector);//изменяет m_externalParamsVector  - НЕ НАДО. НАШ СИМПЛЕКС МЕТОД РАБОТАЕТ И ДЛЯ ОТРИЦАТЕЛЬНЫХ ПАРАМЕТРОВ
    m_linData.makeLinData(*this);//заполняет структуру m_linData - границы всех циклов и все индексные выражения в удобном виде SimpleLinearExpression
    m_linDataWithZeroLowerBounds.makeZeroLowerBounds(m_linData);//заполняем m_linDataWithZeroLowerBounds
    //делаем замену для графа истинной или выходной зависимости
    if ((m_depType==FLOWDEP)||(m_depType==OUTPUTDEP)) 
        m_linDataForTrueDep.makeTransformForTrueDep(m_linDataWithZeroLowerBounds);
    else m_linDataForTrueDep.copyAllFrom(m_linDataWithZeroLowerBounds);//для антизависимости - ничего не меняем
    makeIterationSpaces(m_linDataForTrueDep);//строим пространства итераций вхождений
    //заполняем массив имен переменных и параметров
    m_srcEntryCounterNames.resize(m_srcEntry->loopNumb);
    for(int i=0;i<m_srcEntry->loopNumb;i++)	{
        m_srcEntryCounterNames[i]=m_srcEntry->loops[i].counterIter->getName();
    }
    m_depEntryCounterNames.resize(m_depEntry->loopNumb);
    for(int i=0;i<m_depEntry->loopNumb;i++)	{
        m_depEntryCounterNames[i]=m_depEntry->loops[i].counterIter->getName();
    }
    //заполняем массив имен внешних параметров
    m_externalParamNames.resize(m_externalParamsVector.size());
    for (int i=0;i<m_externalParamNum;i++)	{
        m_externalParamNames[i]=m_externalParamsVector[i]->getName();
    }
    // end of заполнение имен переменных и параметров

    //выходим из подпрограммы, если граф заведомо пустой
    if ((m_depType==OUTPUTDEP) && ((!m_srcEntry->isStatus(IS_GENERATOR))||(!m_depEntry->isStatus(IS_GENERATOR))) )
    {
        SetStatus(LG_EMPTY);
        m_FeautrierSolution = new ExtendedQuast();
        return;//для выходной зависимости оба вхождения должны быть генераторами
    }
    //проблема: любой генератор потенциально является использованием, т.к. участвует в выражении и возвращает свое значение
    //например: y[i]= (x[i]=...) + (x[n-i]=...);
    //но если оба вхождения генераторы, то граф истинной и антизависимости между ними совпадают с двумя типами графов 
    //выходной зависимости. 
    if ( ((m_depType==FLOWDEP)||(m_depType==ANTIDEP)) //если зависимость истинная или анти и при этом
        && !m_srcEntry->isStatus(IS_GENERATOR) )
    {
        SetStatus(LG_EMPTY);
        m_FeautrierSolution = new ExtendedQuast();
        return;
    }



	int altPolyNumb;

    //количество равенств в системе, описывающей альтернативный многогранник
    //см. стр.70-71 диссертации Шульженко
	int eqNum;
    //выясняем, будут ли петли в решетчатом графе. Знаем: m_srcEntry - генератор, m_depEntry - использование
	if (PrecedesStrictly(*m_srcEntry,*m_depEntry)) 
    {
        if ((m_depType==FLOWDEP)||(m_depType==OUTPUTDEP)) m_loopsPresented=true;
        else /*ANTI_DEP*/  m_loopsPresented=false;
    }
    else 
    {
		if (PrecedesStrictly(*m_depEntry,*m_srcEntry))
        {
            if ((m_depType==FLOWDEP)||(m_depType==OUTPUTDEP)) m_loopsPresented=false;
            else /*ANTI_DEP*/  m_loopsPresented=true;
        }
        else //depEntry и srcEntry находятся в одном выражении - неизвестно, что будет выполнено раньше
        {
            if (m_depEntry != m_srcEntry)
                m_loopsPresented=true; 
            else
                m_loopsPresented=false; 
        }
	}
    //если петли не нужны - отключаем 
    if (!m_flagIncludeDepAtSameIter) m_loopsPresented=false;
    //если пространства итераций - разные, то петли на самом деле не петли!
    if ((m_commonLoopsNumb<m_srcEntry->loopNumb) || (m_commonLoopsNumb<m_depEntry->loopNumb) )  
        m_loopsPresented=true;
        

    if (m_loopsPresented) {altPolyNumb=-1; eqNum=m_commonLoopsNumb;}
    else {altPolyNumb=0; eqNum=m_commonLoopsNumb-1;}

    std::list<ExtendedQuast*> solutionsInAltPolytops;
    for(;eqNum>=0;eqNum--)     {
        //Цикл по альтернативным многогранникам. На каждой следующей итерации альт. многогранник
        //лексикографически меньше, чем на предыдущей. Мы останавляваемся, когда для всех значений параметров,
        //найдем хотя бы одну точку в альт. многограннике
	    int tabDimj=m_srcEntry->loopNumb+1+m_depEntry->loopNumb+m_externalParamNum;//число столбцов
	    int tabDimi;//количество неравенств

	    if(eqNum==m_commonLoopsNumb)
		    tabDimi=m_srcEntry->dim*2+eqNum*2+m_srcEntry->loopNumb*2; //одному равенству соответствуют 2 неравенства!
            //структуру таблицы см. ниже
	    else
		    tabDimi=m_srcEntry->dim*2+eqNum*2+1+m_srcEntry->loopNumb*2;//одному равенству соответствуют 2 неравенства!

	    TreeNode* newRoot=new TreeNode();
        //m_srcEntry->loopNumb - количество неизвестных в симплекс-методе, 
        //m_depEntry->loopNumb+m_externalParamNum-m_arbitraryParamVector.size() - количество неотрицательных параметров
	    Tableau* tab=newRoot->tableau=new Tableau(tabDimi,tabDimj,m_srcEntry->loopNumb,m_depEntry->loopNumb+m_externalParamNum-(int)m_arbitraryParamVector.size());
	    newRoot->context=new Polyhedron(m_iterationSpaceForDepEntry);
        //Структура таблицы:
        //столбцы: 0..tab->m_srcEntry->loopNumb=m_srcEntry->loopNumb-1 - коэффициенты при переменных - счетчиках циклов вхождения-источника
        //столбец: m_srcEntry->loopNumb=m_srcEntry->loopNumb - свободный член
        //ВНИМАНИЕ: в неравентсвах типа Inequality свободный член идет вначале, а в таблице - в середине
        //столбцы (m_depEntry->loopNumb штук): m_srcEntry->loopNumb+1 .. m_srcEntry->loopNumb+m_depEntry->loopNumb - коэффициенты при параметрах - счетчиках циклов зависимого вхождения 
        //(общие счетчики на самом деле не общие - одни - переменные(со штрихами), другие - параметры)
        //столбцы: m_srcEntry->loopNumb+m_depEntry->loopNumb+1 .. m_srcEntry->loopNumb+m_depEntry->loopNumb+m_externalParamNum=tabj-1 - коэффициенты при внешних параметрах

        //неравенства с 0 .. m_srcEntry->loopNumb-1 - левые границы циклов ( - это всегда неравенства вида: счетчики>=0)
        //неравенства с m_srcEntry->loopNumb .. 2*m_srcEntry->loopNumb-1 - правые границы циклов
        //неравенства с 2*m_srcEntry->loopNumb .. 2*m_srcEntry->loopNumb + 2*eqNum-1 - описывают альтернативный многогранник (см. стр.70-71 диссертации Шульженко)
        //неравенства с 2*m_srcEntry->loopNumb + 2*eqNum .. + 2*m_srcEntry->loopNumb + 2*eqNum + 2*m_srcEntry->dim - равенства индексных выражений

        //ВИНМАНИЕ: МЫ В КОНЦЕ БУДЕМ ПОЛЬЗОВАТЬСЯ ФУНКЦИЕЙ ApplySimplex, которая находит лексикографический МИНИМУМ,
        //для РГ истинной зависимости нужен максимум, поэтому выше, в функции TransformToCanonicalAndPositive,
        //была сделана замена:
        //i_k:=верхняя граница к-того цикла - i_k для счетчиков циклов m_srcEntry (со штрихами)
        //ЕЩЕ ВНИМАНИЕ: все переменные и параметры в функции ApplySimplex предполагаются положительными,
        //поэтому выше, в функции TransformToCanonicalAndPositive, была сделана замена, 
        //которая делает нижние границы счетчиков нулями.
        //это было сделано не только для счетчиков циклов m_srcEntry (со штрихами), но и для счетчиков m_depEntry!

	    // Неравенства, соответствующие нижним границам счетчиков циклов
	    for(int i=0;i<m_srcEntry->loopNumb;i++)
	    {
		    tab->data[i][i]=1;
            //условия положительности счетчиков циклов
            //с нулевой левой границей
        }
        int currentAdd;//текущая добавка к i
        // Неравенства, соответствующие верхним (изменяющимся) границам счетчиков циклов
	    std::list<Inequality*>::iterator inqIter=m_iterationSpaceForSrcEntry.m_ins.begin();
	    currentAdd=m_srcEntry->loopNumb;
        for(int i=0;i<m_srcEntry->loopNumb;i++)	    {
		    //коэффициенты при счетчиках
            for(int j=0; j < m_srcEntry->loopNumb; j++) tab->data[i+currentAdd][j]=(*inqIter)->m_coefs[j+1];
            //свободный член
            tab->data[i+currentAdd][m_srcEntry->loopNumb]=(*inqIter)->m_coefs[0];
            //коэффициенты при внешних параметрах
            for(int j=0; j < m_externalParamNum; j++) 
                tab->data[i+currentAdd][j+m_srcEntry->loopNumb+m_depEntry->loopNumb+1]=(*inqIter)->m_coefs[j+m_srcEntry->loopNumb+1];
		    ++inqIter;
	    }
	    //КОНЕЦ "Неравенства, соответствующие границам счетчиков циклов"

	    // неравенства, соответствующие альтернативным многогранникам
        currentAdd=2*m_srcEntry->loopNumb;
        //первые eqNum координат должны быть равны
	    for(int i=0;i<eqNum;i++)	    {
            //i-i'>=0
		    tab->data[2*i+currentAdd][i]=1;
            tab->data[2*i+currentAdd][m_srcEntry->loopNumb+i+1]=-1;
            //-i+i'>=0
		    tab->data[2*i+currentAdd+1][i]=-1;
            tab->data[2*i+currentAdd+1][m_srcEntry->loopNumb+i+1]=1;
	    }
        currentAdd=2*m_srcEntry->loopNumb+2*eqNum;
        //а следующая за ними коорданата - строго больше (после замены мы теперь всегда ищем лексик. МИНИМУМ) 
        //i>i' или i-i'-1>=0
	    if(eqNum!=m_commonLoopsNumb)    {
		    tab->data[currentAdd][eqNum]=1;
            tab->data[currentAdd][m_srcEntry->loopNumb+eqNum+1]=-1;
		    tab->data[currentAdd][m_srcEntry->loopNumb]=-1;
            currentAdd=2*m_srcEntry->loopNumb+2*eqNum+1;
	    }
        //заполняем коэффициенты таблицы, соответствующие равенствам индексных выражений m_srcEntry и m_depEntry
	    for(int i=0;i<m_srcEntry->dim;i++)    {
            //2i'-3j' = 4i+9j+2k  т.е. m_srcEntry->data = m_depEntry->data
		    for(int j=0;j<m_srcEntry->loopNumb;j++)    {
                //коэффициенты при счетчиках m_srcEntry
			    tab->data[2*i+currentAdd][j]=m_linDataForTrueDep.m_coefsSrcEntry[i][j+1];
			    tab->data[2*i+currentAdd+1][j]=-m_linDataForTrueDep.m_coefsSrcEntry[i][j+1];
		    }
		    for(int j=0;j<m_depEntry->loopNumb;j++)    {
                //коэффициенты при счетчиках m_depEntry
			    tab->data[2*i+currentAdd][j+m_srcEntry->loopNumb+1]=-m_linDataForTrueDep.m_coefsDepEntry[i][j+1];
			    tab->data[2*i+currentAdd+1][j+m_srcEntry->loopNumb+1]=m_linDataForTrueDep.m_coefsDepEntry[i][j+1];
		    }
            //свободный член
		    tab->data[2*i+currentAdd][m_srcEntry->loopNumb] =
                m_linDataForTrueDep.m_coefsSrcEntry[i][0] - m_linDataForTrueDep.m_coefsDepEntry[i][0];
		    tab->data[2*i+currentAdd+1][m_srcEntry->loopNumb] =
               -m_linDataForTrueDep.m_coefsSrcEntry[i][0] + m_linDataForTrueDep.m_coefsDepEntry[i][0];
            //внешние параметры
            for (int j=0;j<m_externalParamNum;++j) {
                tab->data[2*i+currentAdd][j+m_srcEntry->loopNumb+m_depEntry->loopNumb+1]=
                    m_linDataForTrueDep.m_coefsSrcEntry[i][j+m_srcEntry->loopNumb+1] - m_linDataForTrueDep.m_coefsDepEntry[i][j+m_depEntry->loopNumb+1];
                tab->data[2*i+currentAdd+1][j+m_srcEntry->loopNumb+m_depEntry->loopNumb+1]=
                   -m_linDataForTrueDep.m_coefsSrcEntry[i][j+m_srcEntry->loopNumb+1] + m_linDataForTrueDep.m_coefsDepEntry[i][j+m_depEntry->loopNumb+1];
            }

	    }
        //КОНЕЦ "заполняем коэффициенты таблицы, соответствующие равенствам индексных выражений m_srcEntry и m_depEntry"
	    // End of filling

	    tab->Simplify();
	    CalcTableauSigns(tab,newRoot->context);

	    newRoot->d=1;
	    std::list<TreeNode*> solTabs;
        //находит лексикографический МИНИМУМ. Добавляет найденный набор функций и их областей определения в solTabs.
        //деревой if-ов Фотрье получается в newRoot
	    ApplySimplex(newRoot,solTabs);
        //строим решение Фотрье по дереву newRoot
        ExtendedQuast* sol=new ExtendedQuast(m_depEntry->loopNumb+m_externalParamNum,m_srcEntry->loopNumb,newRoot);
        
        solutionsInAltPolytops.push_back(sol);
        delete newRoot;
        std::list<TreeNode*>::iterator it=solTabs.begin();
        for (;it!=solTabs.end();++it) delete *it;
	    altPolyNumb++;//переходим к следующему альтернативному многограннику
    }
    //делаем общее решение Фотрье
    if (m_FeautrierSolution!=NULL) delete m_FeautrierSolution;
    m_FeautrierSolution=ExtendedQuast::lexMinimum(solutionsInAltPolytops);
    //освобождаем память от частных решений
    std::list<ExtendedQuast*>::iterator it=solutionsInAltPolytops.begin();
    for (;it!=solutionsInAltPolytops.end();++it) delete *it;

    //упрощаем получившееся решение
    GenArea area(m_iterationSpaceForDepEntry);//контекст
    m_FeautrierSolution->combinePartsButRemainSameConditions();
    m_FeautrierSolution->removeExclusiveInequalities(area);

    //делаем в реш. Фотрье замену, обратную m_linDataForTrueDep.transform()
    //вторая матрица обязательно прямая, а не обратная и обязательно srcEntry
    if ((m_depType==FLOWDEP)||(m_depType==OUTPUTDEP))
        m_FeautrierSolution->transform(m_linDataForTrueDep.m_invTransformMatrixDepEntry,m_linDataForTrueDep.m_transformMatrixSrcEntry,m_linDataForTrueDep.m_dimjSrcEntryTransformMatrix);

    if(m_FeautrierSolution->getTypeOfNod()==ExtendedQuast::EMPTY)
		SetStatus(LG_EMPTY);

    //обратную makeZeroLowerBounds замену в ответе не делаем - вся работа с реш. графом должна происходить 
    //в терминах положительных переменных и параметров
}

/// Возвращает true, если оператор, содержащий вхождение p, предшествует 
/// по графу потока управления (построенного для тела общего цикла) оператору, содержащему вхождение q.
bool PrecedesStrictly(const Montego::BasicOccurrence& p, const Montego::BasicOccurrence& q)
{
    StatementBase* pstmt = p.getParentStatement();
    StatementBase* qstmt = q.getParentStatement();
    //std::cout << "PrecedesStrictly for " << p.toString() << " and " << q.toString() << "\n";
    if (pstmt != qstmt)
    {
        RepriseBase* par1, *par2;
        RepriseBase* parent = OPS::Shared::getFirstCommonParentEx(*pstmt, *qstmt, par1, par2);
        BlockStatement* block = parent->cast_ptr<BlockStatement>();
        if (block == 0) block = OPS::Shared::getSurroundingBlock(parent->cast_ptr<StatementBase>());
        if (block == 0) return true;
        ControlFlowGraph graph(*block);
        if (block != OPS::Shared::getOuterSimpleBlock(block)) return true;
        ControlFlowGraph::StatementVector sv = graph.getStatementVector();
        if ( (find(sv.begin(), sv.end(), pstmt) == sv.end()) || (find(sv.begin(), sv.end(), qstmt) == sv.end()) )
            return true;
        if (!graph.hasPath(p.getParentStatement(), q.getParentStatement())) return false;
        else return true;
    }
    else 
    {
        //запускаем определитель порядка для подвыражений
        if (p.getHeadNode() == q.getHeadNode()) return false;
        RepriseBase* par1, *par2;
        RepriseBase* parent = OPS::Shared::getFirstCommonParentEx(*p.getHeadNode(), *q.getHeadNode(), par1, par2);
        OPS_ASSERT(parent != 0);
        //std::cout << "parent = " << parent->dumpState() << "\n";
        //std::cout << "parent.parent = " << parent->getParent()->dumpState() << "\n";
        if (parent == q.getHeadNode()) return true;
        if (parent == p.getHeadNode()) return false;
        BasicCallExpression* bce = parent->cast_ptr<BasicCallExpression>();
        if (bce && bce->getKind() == BasicCallExpression::BCK_ASSIGN)
            if (&bce->getChild(0) == q.getHeadNode())
                return true;
        return false;
    }
}

void ElemLatticeGraph::newbuild(bool flagAllExternalParamsAreArbitrary)
{
    if ((m_newdepType == Montego::DependenceGraphAbstractArc::DT_ANTIDEPENDENCE) ||
        (m_newdepType == Montego::DependenceGraphAbstractArc::DT_ININ_DEPENDENCE))
    {
        Montego::BasicOccurrence* tmp = m_newdepEntry;
        m_newdepEntry = m_newsrcEntry;
        m_newsrcEntry = tmp;
    }
    if (!TestLinearCoefsAndLoops(m_newsrcEntry, m_newdepEntry))	
    {
        SetStatus(LG_ERROR_INIT);
        return;
    }
    if (m_newsrcEntry->getBracketCount() == 0)
    {
        SetStatus(LG_ERROR_INIT);
        return;
    }

    m_commonLoopsNumb = OPS::Shared::getCommonUpperLoopsAmount(m_newsrcEntry->getSourceExpression(), m_newdepEntry->getSourceExpression(), m_program);
    m_suppPolyhedronSrcEntry = new Polyhedron(*m_newsrcEntry, m_program);
    m_suppPolyhedronDepEntry = new Polyhedron(*m_newdepEntry, m_program);
    //строит список внешних параметров m_externalParamsVector (требуется для TestApplicability)
    newmakeExternalVariableVector();

    if (!TestExternalParamConst())	
    {
        SetStatus(LG_ERROR_INIT);
        return;
    }

    if (flagAllExternalParamsAreArbitrary)    m_arbitraryParamVector = m_externalParamsVector;
    //    makeExtParamPositive(m_arbitraryParamVector);//изменяет m_externalParamsVector  - НЕ НАДО. НАШ СИМПЛЕКС МЕТОД РАБОТАЕТ И ДЛЯ ОТРИЦАТЕЛЬНЫХ ПАРАМЕТРОВ
    m_linData.newmakeLinData(*this);//заполняет структуру m_linData - границы всех циклов и все индексные выражения в удобном виде SimpleLinearExpression
    int loopNumbSrcEntry = m_linData.m_loopNumbSrcEntry;
    int loopNumbDepEntryOnly = m_linData.m_loopNumbDepEntryOnly;
    m_linDataWithZeroLowerBounds.makeZeroLowerBounds(m_linData);//заполняем m_linDataWithZeroLowerBounds
    //делаем замену для графа истинной или выходной зависимости
    if ((m_newdepType==Montego::DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)||(m_newdepType==Montego::DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE)) 
        m_linDataForTrueDep.makeTransformForTrueDep(m_linDataWithZeroLowerBounds);
    else m_linDataForTrueDep.copyAllFrom(m_linDataWithZeroLowerBounds);//для антизависимости - ничего не меняем
    makeIterationSpaces(m_linDataForTrueDep);//строим пространства итераций вхождений
#ifdef LATTICE_TABLES_DEBUG
    /*
    for (int i=0;i<m_srcEntry->loopNumb;i++)  {
        file << i<<" Lower loop bound:\n";
        file << "Initial:\n";
        file << m_linData.m_loopLowerBoundForSrcEntry[i].toString() << "\n";
        file << "After zero lower bounds:\n";
        file << m_linDataWithZeroLowerBounds.m_loopLowerBoundForSrcEntry[i].toString() << "\n";
        file << "After true dep transform:\n";
        file << m_linDataForTrueDep.m_loopLowerBoundForSrcEntry[i].toString() << "\n\n";

        file << i<<" Upper loop bound:\n";
        file << "Initial:\n";
        file << m_linData.m_loopUpperBoundForSrcEntry[i].toString() << "\n";
        file << "After zero lower bounds:\n";
        file << m_linDataWithZeroLowerBounds.m_loopUpperBoundForSrcEntry[i].toString() << "\n";
        file << "After true dep transform:\n";
        file << m_linDataForTrueDep.m_loopUpperBoundForSrcEntry[i].toString() << "\n\n";
    }*/
#endif
    //заполняем массив имен переменных и параметров
    m_srcEntryCounterNames.resize(loopNumbSrcEntry);
    for(int i = 0; i < loopNumbSrcEntry; i++)	{
        m_srcEntryCounterNames[i] = m_linDataForTrueDep.m_loopDescSrcEntry[i].counterIter->getName();
    }
    m_depEntryCounterNames.resize(m_commonLoopsNumb + loopNumbDepEntryOnly);
    for(int i = 0; i < m_commonLoopsNumb + loopNumbDepEntryOnly; i++)	{
        m_depEntryCounterNames[i]=m_linDataForTrueDep.m_loopDescDepEntry[i].counterIter->getName();
    }
    //заполняем массив имен внешних параметров
    m_externalParamNames.resize(m_externalParamsVector.size());
    for (int i=0;i<m_externalParamNum;i++)	{
        m_externalParamNames[i]=m_externalParamsVector[i]->getName();
    }
    // end of заполнение имен переменных и параметров

    //выходим из подпрограммы, если граф заведомо пустой
    if ((m_newdepType == DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE) 
        && ((!m_newsrcEntry->isGenerator())||(!m_newdepEntry->isGenerator())) )
    {
        SetStatus(LG_EMPTY);
        m_FeautrierSolution = new ExtendedQuast();
        return;//для выходной зависимости оба вхождения должны быть генераторами
    }
    if ( ((m_newdepType == DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)
        || (m_newdepType == DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)) //если зависимость истинная или анти и при этом
        && !m_newsrcEntry->isGenerator() )
    {
        SetStatus(LG_EMPTY);
        m_FeautrierSolution = new ExtendedQuast();
        return;
    }



    int altPolyNumb;

    //количество равенств в системе, описывающей альтернативный многогранник
    //см. стр.70-71 диссертации Шульженко
    int eqNum;
    //выясняем, будут ли петли в решетчатом графе. Знаем: m_srcEntry - генератор, m_depEntry - использование
    if (m_flagIncludeDepAtSameIter)
    {
        bool precedesStrictly;
        if (m_newdepType == Montego::DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)
            precedesStrictly = PrecedesStrictly(*m_newdepEntry, *m_newsrcEntry);
        else
            precedesStrictly = PrecedesStrictly(*m_newsrcEntry,*m_newdepEntry);
        if (precedesStrictly)
            m_loopsPresented=true;
        else  
        {
            m_loopsPresented = false;
            //проверяем наоборот
            if (m_newdepType == Montego::DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)
                precedesStrictly = PrecedesStrictly(*m_newsrcEntry, *m_newdepEntry);
            else
                precedesStrictly = PrecedesStrictly(*m_newdepEntry,*m_newsrcEntry);
            if (!precedesStrictly && m_newdepEntry != m_newsrcEntry)  m_loopsPresented=true;
        }
    }
    else
        //если петли не нужны - отключаем 
        m_loopsPresented=false;

//теперь PrecedesStrictly работает хорошо, если между операторами нет пути в графе потока управления,
// значит зависимости на одной и той же итерации общего цикла быть не может!
//    //если пространства итераций - разные, то петли на самом деле не петли!
//    if ((m_commonLoopsNumb < loopNumbSrcEntry) || (m_commonLoopsNumb < m_commonLoopsNumb+loopNumbDepEntryOnly) )  
//        m_loopsPresented=true;


    if (m_loopsPresented) {altPolyNumb=-1; eqNum=m_commonLoopsNumb;}
    else {altPolyNumb=0; eqNum=m_commonLoopsNumb-1;}

#ifdef LATTICE_TABLES_DEBUG
    /*
    std::vector<std::string> argumentNames(m_commonLoopsNumb+loopNumbDepEntryOnly+m_externalParamNum);
    for (int i=0; i<m_commonLoopsNumb+loopNumbDepEntryOnly; i++) argumentNames[i]=m_depEntryCounterNames[i];
    for (int i=0; i<m_externalParamNum; i++) argumentNames[i+m_depEntry->loopNumb]=m_externalParamNames[i];
    std::vector<std::string> resultNames(m_srcEntry->loopNumb);
    for (int i=0; i<m_srcEntry->loopNumb; i++) {
        if (i<m_commonLoopsNumb) resultNames[i]=m_srcEntryCounterNames[i]+"'";
        else resultNames[i]=m_srcEntryCounterNames[i];
    }*/
#endif


    std::list<ExtendedQuast*> solutionsInAltPolytops;
    for(;eqNum>=0;eqNum--)     
    {
        //Цикл по альтернативным многогранникам. На каждой следующей итерации альт. многогранник
        //лексикографически меньше, чем на предыдущей. Мы останавляваемся, когда для всех значений параметров,
        //найдем хотя бы одну точку в альт. многограннике
        //число столбцов
        int tabDimj = loopNumbSrcEntry + 1 + loopNumbDepEntryOnly + m_commonLoopsNumb + m_externalParamNum;
        int tabDimi;//количество неравенств

        if (eqNum == m_commonLoopsNumb)
            tabDimi = m_linDataForTrueDep.m_coefsNumSrcDepEntry*2 + eqNum*2 
                + loopNumbSrcEntry*2; //одному равенству соответствуют 2 неравенства!
        //структуру таблицы см. ниже
        else
            tabDimi = m_linDataForTrueDep.m_coefsNumSrcDepEntry*2 + eqNum*2 + 1 
                + loopNumbSrcEntry*2;//одному равенству соответствуют 2 неравенства!

        TreeNode* newRoot=new TreeNode();
        //m_srcEntry->loopNumb - количество неизвестных в симплекс-методе, 
        //m_depEntry->loopNumb+m_externalParamNum-m_arbitraryParamVector.size() - количество неотрицательных параметров
        Tableau* tab = newRoot->tableau = 
            new Tableau(tabDimi, tabDimj, loopNumbSrcEntry, 
            loopNumbDepEntryOnly+m_commonLoopsNumb+m_externalParamNum-(int)m_arbitraryParamVector.size());
        newRoot->context=new Polyhedron(m_iterationSpaceForDepEntry);
        //Структура таблицы:
        //столбцы: 0..tab->m_srcEntry->loopNumb=m_srcEntry->loopNumb-1 - коэффициенты при переменных - счетчиках циклов вхождения-источника
        //столбец: m_srcEntry->loopNumb=m_srcEntry->loopNumb - свободный член
        //ВНИМАНИЕ: в неравентсвах типа Inequality свободный член идет вначале, а в таблице - в середине
        //столбцы (m_depEntry->loopNumb штук): m_srcEntry->loopNumb+1 .. m_srcEntry->loopNumb+m_depEntry->loopNumb - коэффициенты при параметрах - счетчиках циклов зависимого вхождения 
        //(общие счетчики на самом деле не общие - одни - переменные(со штрихами), другие - параметры)
        //столбцы: m_srcEntry->loopNumb+m_depEntry->loopNumb+1 .. m_srcEntry->loopNumb+m_depEntry->loopNumb+m_externalParamNum=tabj-1 - коэффициенты при внешних параметрах

        //неравенства с 0 .. m_srcEntry->loopNumb-1 - левые границы циклов ( - это всегда неравенства вида: счетчики>=0)
        //неравенства с m_srcEntry->loopNumb .. 2*m_srcEntry->loopNumb-1 - правые границы циклов
        //неравенства с 2*m_srcEntry->loopNumb .. 2*m_srcEntry->loopNumb + 2*eqNum-1 - описывают альтернативный многогранник (см. стр.70-71 диссертации Шульженко)
        //неравенства с 2*m_srcEntry->loopNumb + 2*eqNum .. + 2*m_srcEntry->loopNumb + 2*eqNum + 2*m_srcEntry->dim - равенства индексных выражений

        //ВИНМАНИЕ: МЫ В КОНЦЕ БУДЕМ ПОЛЬЗОВАТЬСЯ ФУНКЦИЕЙ ApplySimplex, которая находит лексикографический МИНИМУМ,
        //для РГ истинной зависимости нужен максимум, поэтому выше, в функции TransformToCanonicalAndPositive,
        //была сделана замена:
        //i_k:=верхняя граница к-того цикла - i_k для счетчиков циклов m_srcEntry (со штрихами)
        //ЕЩЕ ВНИМАНИЕ: все переменные и параметры в функции ApplySimplex предполагаются положительными,
        //поэтому выше, в функции TransformToCanonicalAndPositive, была сделана замена, 
        //которая делает нижние границы счетчиков нулями.
        //это было сделано не только для счетчиков циклов m_srcEntry (со штрихами), но и для счетчиков m_depEntry!

        // Неравенства, соответствующие нижним границам счетчиков циклов
        for(int i = 0; i < loopNumbSrcEntry; i++)
        {
            tab->data[i][i] = 1;
            //условия положительности счетчиков циклов
            //с нулевой левой границей
        }
        int currentAdd;//текущая добавка к i
        // Неравенства, соответствующие верхним (изменяющимся) границам счетчиков циклов
        std::list<Inequality*>::iterator inqIter = m_iterationSpaceForSrcEntry.m_ins.begin();
        currentAdd = loopNumbSrcEntry;
        for(int i = 0; i < loopNumbSrcEntry; i++)	    {
            //коэффициенты при счетчиках
            for(int j=0; j < loopNumbSrcEntry; j++) tab->data[i+currentAdd][j]=(*inqIter)->m_coefs[j+1];
            //свободный член
            tab->data[i+currentAdd][loopNumbSrcEntry]=(*inqIter)->m_coefs[0];
            //коэффициенты при внешних параметрах
            for(int j=0; j < m_externalParamNum; j++) 
                tab->data[i+currentAdd][j+loopNumbSrcEntry+loopNumbDepEntryOnly+m_commonLoopsNumb+1]
                    = (*inqIter)->m_coefs[j+loopNumbSrcEntry+1];

#ifdef LATTICE_TABLES_DEBUG
            //строим список имен параметров
            std::vector<std::string> argumentNames(loopNumbSrcEntry+m_externalParamNum);
            for (int i=0; i<loopNumbSrcEntry; i++) argumentNames[i]=m_srcEntryCounterNames[i];
            for (int i=0; i<m_externalParamNum; i++) argumentNames[i+loopNumbSrcEntry]=m_externalParamNames[i];
            file<<"Inequality of loop bound:"<<std::endl;
            file<<(*inqIter)->toString(argumentNames)<<"\n\n";
            file.flush();
#endif


            ++inqIter;
        }
        //КОНЕЦ "Неравенства, соответствующие границам счетчиков циклов"

        // неравенства, соответствующие альтернативным многогранникам
        currentAdd=2*loopNumbSrcEntry;
        //первые eqNum координат должны быть равны
        for(int i = 0; i < eqNum; i++)	    {
            //i-i'>=0
            tab->data[2*i+currentAdd][i]=1;
            tab->data[2*i+currentAdd][loopNumbSrcEntry+i+1]=-1;
            //-i+i'>=0
            tab->data[2*i+currentAdd+1][i]=-1;
            tab->data[2*i+currentAdd+1][loopNumbSrcEntry+i+1]=1;
        }
        currentAdd=2*loopNumbSrcEntry+2*eqNum;
        //а следующая за ними коорданата - строго больше (после замены мы теперь всегда ищем лексик. МИНИМУМ) 
        //i>i' или i-i'-1>=0
        if (eqNum != m_commonLoopsNumb)    
        {
            tab->data[currentAdd][eqNum]=1;
            tab->data[currentAdd][loopNumbSrcEntry+eqNum+1]=-1;
            tab->data[currentAdd][loopNumbSrcEntry]=-1;
            currentAdd=2*loopNumbSrcEntry+2*eqNum+1;
        }
        //заполняем коэффициенты таблицы, соответствующие равенствам индексных выражений m_srcEntry и m_depEntry
        for(int i = 0; i < m_linDataForTrueDep.m_coefsNumSrcDepEntry; i++)    
        {
            //2i'-3j' = 4i+9j+2k  т.е. m_srcEntry->data = m_depEntry->data
            for(int j = 0; j < loopNumbSrcEntry; j++)    
            {
                //коэффициенты при счетчиках m_srcEntry
                tab->data[2*i+currentAdd][j]=m_linDataForTrueDep.m_coefsSrcEntry[i][j+1];
                tab->data[2*i+currentAdd+1][j]=-m_linDataForTrueDep.m_coefsSrcEntry[i][j+1];
            }
            for(int j=0; j<loopNumbDepEntryOnly+m_commonLoopsNumb; j++)    
            {
                //коэффициенты при счетчиках m_depEntry
                tab->data[2*i+currentAdd][j+loopNumbSrcEntry+1]=-m_linDataForTrueDep.m_coefsDepEntry[i][j+1];
                tab->data[2*i+currentAdd+1][j+loopNumbSrcEntry+1]=m_linDataForTrueDep.m_coefsDepEntry[i][j+1];
            }
            //свободный член
            tab->data[2*i+currentAdd][loopNumbSrcEntry] =
                m_linDataForTrueDep.m_coefsSrcEntry[i][0] - m_linDataForTrueDep.m_coefsDepEntry[i][0];
            tab->data[2*i+currentAdd+1][loopNumbSrcEntry] =
                -m_linDataForTrueDep.m_coefsSrcEntry[i][0] + m_linDataForTrueDep.m_coefsDepEntry[i][0];
            //внешние параметры
            for (int j=0;j<m_externalParamNum;++j) {
                tab->data[2*i+currentAdd][j+loopNumbSrcEntry+loopNumbDepEntryOnly+m_commonLoopsNumb+1] =
                    m_linDataForTrueDep.m_coefsSrcEntry[i][j+loopNumbSrcEntry+1] - m_linDataForTrueDep.m_coefsDepEntry[i][j+loopNumbDepEntryOnly+m_commonLoopsNumb+1];
                tab->data[2*i+currentAdd+1][j+loopNumbSrcEntry+loopNumbDepEntryOnly+m_commonLoopsNumb+1]=
                    -m_linDataForTrueDep.m_coefsSrcEntry[i][j+loopNumbSrcEntry+1] + m_linDataForTrueDep.m_coefsDepEntry[i][j+loopNumbDepEntryOnly+m_commonLoopsNumb+1];
            }

        }
        //КОНЕЦ "заполняем коэффициенты таблицы, соответствующие равенствам индексных выражений m_srcEntry и m_depEntry"
        // End of filling

        tab->Simplify();
        CalcTableauSigns(tab,newRoot->context);

#ifdef LATTICE_TABLES_DEBUG
        file<<_TL("Starting table:\n","")<<*tab;
        deepOfTheSimplexSearch=0;
#endif

        newRoot->d=1;
        std::list<TreeNode*> solTabs;
        //находит лексикографический МИНИМУМ. Добавляет найденный набор функций и их областей определения в solTabs.
        //деревой if-ов Фотрье получается в newRoot
        ApplySimplex(newRoot,solTabs);
        //строим решение Фотрье по дереву newRoot
        ExtendedQuast* sol=new ExtendedQuast(loopNumbDepEntryOnly+m_commonLoopsNumb+m_externalParamNum,loopNumbSrcEntry, newRoot);

#ifdef LATTICE_TABLES_DEBUG
        //file<<"Solution:\n"<<sol->toString(&argumentNames,&resultNames);
#endif

        solutionsInAltPolytops.push_back(sol);
        delete newRoot;
        std::list<TreeNode*>::iterator it=solTabs.begin();
        for (;it!=solTabs.end();++it) delete *it;
        altPolyNumb++;//переходим к следующему альтернативному многограннику
    }
#ifdef LATTICE_TABLES_DEBUG
    //std::cout<<"Solution in the first alternate polytope:\n"<<solutionsInAltPolytops.front()->toString(&argumentNames,&resultNames);
#endif
    //делаем общее решение Фотрье
    if (m_FeautrierSolution!=NULL) delete m_FeautrierSolution;
    m_FeautrierSolution=ExtendedQuast::lexMinimum(solutionsInAltPolytops);
    //освобождаем память от частных решений
    std::list<ExtendedQuast*>::iterator it = solutionsInAltPolytops.begin();
    for (;it!=solutionsInAltPolytops.end();++it) delete *it;

#ifdef LATTICE_TABLES_DEBUG
    //std::cout<<"Lex Minimum:\n"<<m_FeautrierSolution->toString(&argumentNames,&resultNames);
    //std::cout<<m_linDataForTrueDep.m_transformMatrixSrcEntry[0][0]<<" "<<m_linDataForTrueDep.m_transformMatrixSrcEntry[0][1]<<" "<<m_linDataForTrueDep.m_transformMatrixSrcEntry[0][2]<<"\n";
    //std::cout<<m_linDataForTrueDep.m_transformMatrixSrcEntry[1][0]<<" "<<m_linDataForTrueDep.m_transformMatrixSrcEntry[1][1]<<" "<<m_linDataForTrueDep.m_transformMatrixSrcEntry[1][2]<<"\n";
#endif

    //упрощаем получившееся решение
    GenArea area(m_iterationSpaceForDepEntry);//контекст
    m_FeautrierSolution->combinePartsButRemainSameConditions();
    m_FeautrierSolution->removeExclusiveInequalities(area);

    //делаем в реш. Фотрье замену, обратную m_linDataForTrueDep.transform()
    //вторая матрица обязательно прямая, а не обратная и обязательно srcEntry
    if ((m_newdepType==Montego::DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)
        ||(m_newdepType==Montego::DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE))
        m_FeautrierSolution->transform(m_linDataForTrueDep.m_invTransformMatrixDepEntry, 
            m_linDataForTrueDep.m_transformMatrixSrcEntry, 
            m_linDataForTrueDep.m_dimjSrcEntryTransformMatrix);

    if(m_FeautrierSolution->getTypeOfNod()==ExtendedQuast::EMPTY)
        SetStatus(LG_EMPTY);

    //обратную makeZeroLowerBounds замену в ответе не делаем - вся работа с реш. графом должна происходить 
    //в терминах положительных переменных и параметров
}


std::string ElemLatticeGraph::toString()
{
    std::string s;
    if (isStatus(LG_ERROR_INIT))
    {
        s = "Lattice graph couldn't be built for such program\n";
    }
    else
    {
        if (isStatus(LG_EMPTY))
        {
            s = "Lattice graph is empty\n";
        }
        else
        {
            std::vector<std::string> argumentNames(m_linData.m_loopNumbDepEntryOnly + m_linData.m_commonLoopsNumb + m_externalParamNum);
            for (int i=0; i<m_linData.m_loopNumbDepEntryOnly + m_linData.m_commonLoopsNumb; i++) 
                argumentNames[i]=m_depEntryCounterNames[i];
            for (int i=0; i<m_externalParamNum; i++) 
                argumentNames[i+m_linData.m_loopNumbDepEntryOnly + m_linData.m_commonLoopsNumb] 
                    = m_externalParamNames[i];
            std::vector<std::string> resultNames(m_linData.m_loopNumbSrcEntry);
            for (int i=0; i<m_linData.m_loopNumbSrcEntry; i++) {
                if (i<m_commonLoopsNumb) resultNames[i]=m_srcEntryCounterNames[i]+"'";
                else resultNames[i]=m_srcEntryCounterNames[i];
            }
            std::string typeOfGraph;
            if (m_newdepType == Montego::DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE) typeOfGraph = "true";
            if (m_newdepType == Montego::DependenceGraphAbstractArc::DT_ANTIDEPENDENCE) typeOfGraph = "anti";
            if (m_newdepType == Montego::DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE) typeOfGraph = "output";
            s = "Piecewise quasi-affine function of " + typeOfGraph + "-dependence graph:\n";
            if (!m_FeautrierSolutionInInitialVars) buildFeautrierSolutionInInitialVars();
            s+=m_FeautrierSolutionInInitialVars->toString(&argumentNames,&resultNames);
        }
    }
    return s;
}
//делает вектор значений внешних параметров, в соостветствии со списком внешних параметров РГ
std::vector<int> buildExtParamsValuesVector(std::map<OPS::Reprise::VariableDeclaration*,int>& externalParamValuesMap, 
                                             std::vector<OPS::Reprise::VariableDeclaration*>& externalParamsVector)
{
    int n = (int)externalParamsVector.size();
    std::vector<int> externalParamValuesVector(n);
    for (int i=0;i<n;i++) externalParamValuesVector[i]=externalParamValuesMap[externalParamsVector[i]];    
    return externalParamValuesVector;
}

std::vector<int> combineLoopIndexDataAndExtParamValues(LoopIndex& sink, const std::vector<int>& externalParamValuesVector)
{
    std::vector<int> sinkData(sink.getSize()+externalParamValuesVector.size());
    for (int i=0; i<sink.getSize(); i++) sinkData[i]=sink[i];
    for (int i=0; i<(int)externalParamValuesVector.size(); i++) sinkData[i+sink.getSize()]=externalParamValuesVector[i];
    return sinkData;
}

//возвращает true, если граф построен правильно. (Выполнение может занять много времени - это перебор. Зато - 100% результат!)
bool ElemLatticeGraph::selfTest(std::map<OPS::Reprise::VariableDeclaration*,int>& externalParamValuesMap)
{
	if(isStatus(LG_ERROR_INIT))	{
		latticeGraphLog(OPS::Console::LEVEL_WARNING, _TL("\n An Error's occured during graph build.\n",""));
		return false;
	}
    if (!m_FeautrierSolutionInInitialVars) buildFeautrierSolutionInInitialVars();

    //выходим из подпрограммы, если граф заведомо пустой
    if ((m_newdepType == DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE) 
        && ((!m_newsrcEntry->isGenerator())||(!m_newdepEntry->isGenerator())) )
    {
        if ((!isStatus(LG_EMPTY))||(m_FeautrierSolutionInInitialVars->getTypeOfNod()!=ExtendedQuast::EMPTY)) 
        {
            std::cout << "Graph should by empty, but it does not!!!!";
            return false;
        }
        else return true;
    }
    if ( ((m_newdepType == DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)
        || (m_newdepType == DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)) //если зависимость истинная или анти и при этом
        && !m_newsrcEntry->isGenerator() )
    {
        if ((!isStatus(LG_EMPTY))||(m_FeautrierSolutionInInitialVars->getTypeOfNod()!=ExtendedQuast::EMPTY)) 
        {
            std::cout << "Graph should by empty, but it does not!!!!";
            return false;
        }
        else return true;
    }
    //проверяем подано ли на вход достаточное количество значений внешних параметров
    OPS_ASSERT(m_externalParamsVector.size()<=externalParamValuesMap.size());

    const std::vector<int>& externalParamValuesVector 
        = buildExtParamsValuesVector(externalParamValuesMap,m_externalParamsVector);
    LinData linDataWithoutParams; 
    linDataWithoutParams.copyAllExceptTransform(m_linData);
    linDataWithoutParams.substituteParams(externalParamValuesVector);
	LoopIndex source(linDataWithoutParams.m_loopNumbSrcEntry, linDataWithoutParams.m_loopLowerBoundForSrcEntry, linDataWithoutParams.m_loopUpperBoundForSrcEntry);
    
    //составляем массивы границ зависимого вхождения (использования - для TRUE и ANTI)
    int loopNumbDepEntry = linDataWithoutParams.m_commonLoopsNumb + linDataWithoutParams.m_loopNumbDepEntryOnly;
    SimpleLinearExpression* allLoopLowerBoundForDepEntry = new SimpleLinearExpression[loopNumbDepEntry];
    SimpleLinearExpression* allLoopUpperBoundForDepEntry = new SimpleLinearExpression[loopNumbDepEntry];
    for (int i = 0; i < m_commonLoopsNumb; i++) 
    {
        allLoopLowerBoundForDepEntry[i] = linDataWithoutParams.m_loopLowerBoundForSrcEntry[i];
        allLoopUpperBoundForDepEntry[i] = linDataWithoutParams.m_loopUpperBoundForSrcEntry[i];
    }
    for (int i = 0; i < linDataWithoutParams.m_loopNumbDepEntryOnly;i++) 
    {
        allLoopLowerBoundForDepEntry[m_commonLoopsNumb+i] = linDataWithoutParams.m_loopLowerBoundForDepEntry[i];
        allLoopUpperBoundForDepEntry[m_commonLoopsNumb+i] = linDataWithoutParams.m_loopUpperBoundForDepEntry[i];
    }
    LoopIndex sink(loopNumbDepEntry, allLoopLowerBoundForDepEntry, allLoopUpperBoundForDepEntry);
	LoopIndex supposedSrc(linDataWithoutParams.m_loopNumbSrcEntry, linDataWithoutParams.m_loopLowerBoundForSrcEntry,linDataWithoutParams.m_loopUpperBoundForSrcEntry);
	bool sourceExists, moveSrcIndex, okBuilt = true;

    //Если в решетчатом графе нет петель тогда надо подвинуть srcIndex на следующую позицию
    if (m_loopsPresented) moveSrcIndex = false;
    else moveSrcIndex = true;
    
    sink.SetToLowerBounds();
    long iterNum = 0;
	while(sink.isInBounds())	{
		//Находим действительный источник source для дуги, имея ее конец sink
        if ((m_newdepType==DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)||(m_newdepType==DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE)) 
            source.InitWithUpper(sink,m_commonLoopsNumb);
        else 
            source.InitWithLower(sink,m_commonLoopsNumb);
		if (moveSrcIndex) 
        {
            if ((m_newdepType==DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)||(m_newdepType==DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE)) 
                source--;
            else 
                source++;
        }
        //считаем вектор значений счетчиков циклов и внешних параметров
        const std::vector<int>& sinkData=combineLoopIndexDataAndExtParamValues(sink, externalParamValuesVector);
		sourceExists = false;
		while (source.isInBounds() && !sourceExists)		
        {
			bool foundEqual = true;
			for(int i = 0; i < m_linData.m_coefsNumSrcDepEntry; i++)
            {
                //считаем вектор значений счетчиков циклов и внешних параметров
                const std::vector<int>& sourceData = combineLoopIndexDataAndExtParamValues(source, externalParamValuesVector);
				if ( m_linData.m_coefsSrcEntry[i].evaluate(sourceData)
                  != m_linData.m_coefsDepEntry[i].evaluate(sinkData) ) 	
                {
                       foundEqual = false; 
                       break;
                }
			}
			if (foundEqual)   
            {
				sourceExists = true;
				break;
			}
			else 
            {
                if ((m_newdepType==DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)||(m_newdepType==DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE)) 
                    source--;
                else 
                    source++;
            }
		}
        std::vector<int>  supposedSrc = m_FeautrierSolutionInInitialVars->evaluate(sinkData);
        bool supposedSrcExists = (supposedSrc.size()>0);
		if ( ( (!supposedSrcExists) && sourceExists ) || ( supposedSrcExists && (!sourceExists) )	)   
        {
            std::stringstream str;
            if ((m_newdepType==DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)||(m_newdepType==DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE))
            {
                if (sourceExists) 
                {
                    latticeGraphLog(Console::LEVEL_ERROR, _TL("\n The Source Exists, But Lattice says it Does NOT!\n",""));
                    str << _TL("Source: ","") << source << _TL(" -->  Sink: ","")<< sink << std::endl;
                }
                else 
                {
                    latticeGraphLog(Console::LEVEL_ERROR, _TL("\n The Source doesn't exist, But Lattice says it does!\n",""));
                    str << _TL("Source: (","");
                    for (int i=0;i<(int)supposedSrc.size()-1;i++) str << supposedSrc[i] <<",";
                    str << supposedSrc[supposedSrc.size()-1] <<")";
                    str << _TL(" -->  Sink: ","")<< sink << std::endl;
                }
            }
            else
            {
                if (sourceExists) 
                {
                    latticeGraphLog(Console::LEVEL_ERROR, _TL("\n The Source Exists, But Lattice says it Does NOT!\n",""));
                    str << _TL("Source: ","") << sink << _TL(" -->  Sink: ","")<< source << std::endl;
                }
                else 
                {
                    latticeGraphLog(Console::LEVEL_ERROR, _TL("\n The Source doesn't exist, But Lattice says it does!\n",""));
                    str << _TL("Source: ","") << sink << _TL(" -->  Sink: (","");
                    for (int i=0;i<(int)supposedSrc.size()-1;i++) str << supposedSrc[i] <<",";
                    str << supposedSrc[supposedSrc.size()-1] <<")" << std::endl;
                }
            }
			latticeGraphLog(Console::LEVEL_DEBUG, str.str());
			okBuilt = false;
			break;
		}
        if ( supposedSrcExists && sourceExists )   {//проверяем на равенство
            bool equal=true;
            for(size_t i = 0; i < supposedSrc.size(); i++) 
                if (source[i]!=supposedSrc[i]) {equal = false; break;}
            if (! equal) 
            {
                std::stringstream str;
                latticeGraphLog(Console::LEVEL_ERROR, _TL("\n The real source doesn't equal to lattice graph source!\n",""));
                str << _TL("Real source: ","") << source << _TL(" -->  Sink: ","") << sink << std::endl;
                str << _TL("Feautrie solution source: (","");
                for (size_t i = 0; i < supposedSrc.size()-1; i++) str << supposedSrc[i] <<",";
                str << supposedSrc[supposedSrc.size()-1] <<")";
                latticeGraphLog(Console::LEVEL_DEBUG, str.str());
                okBuilt = false;
                break;
                }
        }
		sink++; iterNum++;
#ifdef LATTICE_TABLES_DEBUG
       // std::cout <<"Sink: " << sink << "\n";
#endif
	}
    if ((iterNum == 0)&&(m_FeautrierSolutionInInitialVars->getTypeOfNod()!=ExtendedQuast::EMPTY))
    {
        std::stringstream str;
        latticeGraphLog(Console::LEVEL_ERROR, _TL("\n The iteration space is empty, \
but lattice graph is not! It may be not an error. Because lattice graph may be \
not empty for other params\n",""));
    }
    delete[] allLoopLowerBoundForDepEntry;
    delete[] allLoopUpperBoundForDepEntry;
    return okBuilt;
}

bool ElemLatticeGraph::TestApplicability(OccurDesc* srcEntry, OccurDesc* depEntry)
{
    if(srcEntry->m_varDecl!=depEntry->m_varDecl)    {
        return false;
    }

	if ((srcEntry->IsIndexesLinear() == false) || (depEntry->IsIndexesLinear() == false))
	{
        //Нелинейные индексные выражения... не работаем в этом случае.
		//OPS::Console* const pConsole = &OPS::getOutputConsole("LatticeGraph");
		//pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment is not from Linear Class.", nonApplicableLinearClass));
		return false;
	}
    //проверяем на линейность границы циклов
    for (int i = 0; i < srcEntry->loopNumb; ++i)
        if ((srcEntry->loops[i].lBoundNotLinear)||(srcEntry->loops[i].rBoundNotLinear))
            return false;
    for (int i = 0; i < depEntry->loopNumb; ++i)
        if ((depEntry->loops[i].lBoundNotLinear)||(depEntry->loops[i].rBoundNotLinear))
            return false;

    int commonLoopsNumb = getCommonUpperLoopsAmount(*srcEntry, *depEntry);

	if(commonLoopsNumb > 0)
	{		
		if(srcEntry->loops[0].stmtFor == depEntry->loops[0].stmtFor)
		{//Значит, оба вхождения имеют общий цикл loops[0].stmtFor. проверим его на линейность...

			id stmtIndex(srcEntry->loops[0].stmtFor->getBody());			
			return ElemLatticeGraph::TestFragmentApplicability(stmtIndex);
		}
	}

	//Если дошли до сюда - это значит, что поданные вхождения не имеют общих циклов.

	//Проверим, лежит ли вхождение-источник в Допустимом гнезде циклов
	if(srcEntry->loopNumb > 0)
	{
		id stmtIndex(srcEntry->loops[0].stmtFor->getBody());			
		if( ElemLatticeGraph::TestFragmentApplicability(stmtIndex) == false)
			return false;
	}

	//теперь, то же для зависимого вхождения...
	if(depEntry->loopNumb > 0)
	{
		id stmtIndex(depEntry->loops[0].stmtFor->getBody());			
		if( ElemLatticeGraph::TestFragmentApplicability(stmtIndex) == false)
			return false;
	}

	//если циклов нет вообще, просто проверим последовательно операторы от одного вхождения до другого
	if(PrecedesNonStrictly(*srcEntry, *depEntry))
	{//srcEntry раньше по тексту, чем depEntry
		id stmtIndex(srcEntry->GetStatement(), depEntry->GetStatement());
		if( ElemLatticeGraph::TestFragmentApplicability(stmtIndex) == false)    return false;
	}
	else
	{
        //depEntry раньше по тексту, чем srcEntry
		id stmtIndex(depEntry->GetStatement(), srcEntry->GetStatement());
		if( ElemLatticeGraph::TestFragmentApplicability(stmtIndex) == false)	return false;
	}

	return true;
}

class CheckExpression : public OPS::Reprise::Service::DeepWalker
{
public:
    CheckExpression() : m_checkResult(true) {}
    bool m_checkResult; //=true - выражение содержит только допустимые элементы, = false - с выражением что-то не то

    void visit(BasicCallExpression& e)
    {
        switch (e.getKind())
        {
        case BasicCallExpression::BCK_BINARY_MINUS : 
        case BasicCallExpression::BCK_BINARY_PLUS :
        case BasicCallExpression::BCK_MULTIPLY :
        case BasicCallExpression::BCK_UNARY_MINUS :
        case BasicCallExpression::BCK_UNARY_PLUS : 
            break;
        default : {m_checkResult = false; break;}
        }
    }
    void visit(StructAccessExpression&){m_checkResult = false;}
    void visit(TypeCastExpression&){m_checkResult = false;}
    void visit(SubroutineCallExpression&){m_checkResult = false;}
};

bool checkOccurrence(Montego::BasicOccurrence* o)
{
    bool result = true;
    CheckExpression checkVisitor;
    Montego::BasicOccurrenceName name = o->getName();
    for (int i=0; i < o->getBracketCount(); i++)
    {
        checkVisitor.m_checkResult = true;
        name.m_bracketContent[i]->accept(checkVisitor);
        result = result && checkVisitor.m_checkResult;
    }
    return result;
}
/// Проверить возможность построения графа для данных вхождений.
/// Возвращает true, если граф постоить возможно, иначе false;
bool ElemLatticeGraph::TestLinearCoefsAndLoops(Montego::BasicOccurrence* srcEntry, Montego::BasicOccurrence* depEntry)
{
    if (isNamesEqual(srcEntry->getName(), depEntry->getName()) != 1)
    {
        //если имена вхождений различные, то
        return false;
    }
    //проверяем на допустимость индексные выражения
    if ((!checkOccurrence(srcEntry)) || (!checkOccurrence(depEntry))) return false;

    OPS::Montego::OccurrenceCoeffs srcEntryCoeffs(*srcEntry);
    OPS::Montego::OccurrenceCoeffs depEntryCoeffs(*depEntry);
    std::vector<OPS::Shared::CanonicalLinearExpression> externalParamCoefs;
    std::vector<SimpleLinearExpression> coeffs;
    if ( !srcEntryCoeffs.getExternalParamAndLoopCounterCoefficients(coeffs,externalParamCoefs,m_program)
        || !depEntryCoeffs.getExternalParamAndLoopCounterCoefficients(coeffs,externalParamCoefs,m_program))
        return false;

    //проверяем на линейность границы циклов
	std::vector<DepGraph::LoopDesc> loopDescSrc = buildLoopDesc(srcEntry->getRefExpr(), m_program);
    for (size_t i = 0; i < loopDescSrc.size(); ++i)
        if ((loopDescSrc[i].lBoundNotLinear)||(loopDescSrc[i].rBoundNotLinear))
            return false;
	std::vector<DepGraph::LoopDesc> loopDescDep = buildLoopDesc(depEntry->getRefExpr(), m_program);
    for (size_t i = 0; i < loopDescDep.size(); ++i)
        if ((loopDescDep[i].lBoundNotLinear)||(loopDescDep[i].rBoundNotLinear))
            return false;

    //проверяем родителей вхождений - должны быть только блоки или операторы for
    bool fragSrc = true, fragDep = true;
    if (loopDescSrc.size() > 0)
        fragSrc = ElemLatticeGraph::newTestFragmentApplicability(loopDescSrc[0].stmtFor->getBody(), *(srcEntry->getParentStatement()));
    if (loopDescDep.size() > 0)
        fragDep = ElemLatticeGraph::newTestFragmentApplicability(loopDescDep[0].stmtFor->getBody(), *(depEntry->getParentStatement()));
    if (!fragSrc || !fragDep) return false;
    return true;
}

bool ElemLatticeGraph::TestExternalParamConst()
{
	std::vector<DepGraph::LoopDesc> loopDescSrc = buildLoopDesc(m_newsrcEntry->getRefExpr(), m_program);
    //проверяем, изменяются ли внешние параметры в рассматриваемом гнезде циклов
    RepriseBase* code;
    int commonLoopsNumb = OPS::Shared::getCommonUpperLoopsAmount(m_newsrcEntry->getSourceExpression(), m_newdepEntry->getSourceExpression(), m_program);

    if (commonLoopsNumb > 0)
        code = loopDescSrc[0].stmtFor;
    else
    {
        //на самом деле это неправильно, нужно по графу потока управления собрать все операторы между
        //рассматриваемыми вхождениями
        code = OPS::Shared::getFirstCommonParent(*(m_newsrcEntry->getParentStatement()), *(m_newdepEntry->getParentStatement())); 
    }
    //для каждого внешнего параметра
    for (size_t i = 0; i < m_externalParamsVector.size(); ++i)
    {
        //получаем список его вхождений в рассматриваемом фрагменте программы
        std::list<Montego::BasicOccurrencePtr> param_aliases = 
            m_aliasInterface->getBasicOccurrencesAliasedWith(*(m_externalParamsVector[i]), code);
        //проверяем, есть ли среди них генераторы
        std::list<Montego::BasicOccurrencePtr>::iterator it;
        for (it = param_aliases.begin(); it != param_aliases.end(); ++it)
        {
            if ((*it)->isGenerator()) return false;
        }
    }   
    return true;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//					//============NB!===================
////Проверка, что Id указывает только на фрагмент допустимых операторов (true)
//bool ElemLatticeGraph::TestAcceptableStmts(id& stmtIndex)
//{
//	OPS::Console* const pConsole = &OPS::getOutputConsole("LatticeGraph");
//	GetTypeVisitor visitor;
//	for(StatementBase* currNode = stmtIndex.getThisOper(); currNode; currNode = stmtIndex.next())
//	{
//		currNode->accept(visitor);
//		switch(visitor.m_typeOfNode)
//		{
//		case GetTypeVisitor::NK_ExpressionStatement:
//			{
//				ExpressionStatement* stmtExpr = dynamic_cast<ExpressionStatement*>(currNode);
//				
//				if(stmtExpr)
//				{
//					stmtExpr->get().accept(visitor);
//					if(visitor.m_typeOfNode != GetTypeVisitor::NK_BasicCallExpression && visitor.m_typeOfNode != GetTypeVisitor::NK_HirCCallExpression)
//					{
//						pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
//						return false;//Такие операторы недопустимы !
//					}
//					else
//					{
//						if(stmtExpr->get().is_a<BasicCallExpression>())
//							if(stmtExpr->get().cast_to<BasicCallExpression>().getKind()!=BasicCallExpression::BCK_ASSIGN)
//							{
//								pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
//								return false;//Такие операторы недопустимы !
//							}
//						
//						if (stmtExpr->get().is_a<Canto::HirCCallExpression>())
//							if (stmtExpr->get().cast_to<Canto::HirCCallExpression>().getKind() != Canto::HirCCallExpression::HIRC_ASSIGN)
//							{
//								pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
//								return false;//Такие операторы недопустимы !
//							}
//					}
//				}
//				else
//				{
//					pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
//					return false;//неожиданный поворот
//				}
//
//			}
//			break;
//
//		case GetTypeVisitor::NK_ForStatement:
//
//			break;
//
//		default:
//
//			pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
//			return false; //нашли НЕОБРАБАТЫВАЕМЫЙ оператор -> фрагмент нельзя использовать.
//		}
//	}
//
//	return true;
//}

// Проверить применимость к фрагменту программы
bool ElemLatticeGraph::TestFragmentApplicability(id& stmtIndex)
{
	//OPS::Console* const pConsole = &OPS::getOutputConsole("LatticeGraph");

	//Проверим, состоит ли этот фрагмент только из операторов присваивания и циклов For...
	stmtIndex.reset();

	GetTypeVisitor visitor;
	for(StatementBase* currNode = stmtIndex.getThisOper(); currNode; currNode = stmtIndex.next())
	{
		currNode->accept(visitor);
		switch(visitor.m_typeOfNode)
		{
		case GetTypeVisitor::NK_ExpressionStatement:
			{
				ExpressionStatement* stmtExpr = dynamic_cast<ExpressionStatement*>(currNode);

				if(stmtExpr)
				{
					stmtExpr->get().accept(visitor);
					if(visitor.m_typeOfNode != GetTypeVisitor::NK_BasicCallExpression && visitor.m_typeOfNode != GetTypeVisitor::NK_HirCCallExpression)
					{
						//pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
						return false;//Такие операторы недопустимы !
					}
					else
					{
						if(stmtExpr->get().is_a<BasicCallExpression>())
                        {
                            if(stmtExpr->get().cast_to<BasicCallExpression>().getKind()!=BasicCallExpression::BCK_ASSIGN)
							{
								//pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
								return false;//Такие операторы недопустимы !
							}
                        }
						if (stmtExpr->get().is_a<Canto::HirCCallExpression>())
                        {
                            if (stmtExpr->get().cast_to<Canto::HirCCallExpression>().getKind() != Canto::HirCCallExpression::HIRC_ASSIGN)
							{
								//pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
								return false;//Такие операторы недопустимы !
							}
                        }
					}
				}
				else
				{
					//pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
					return false;//неожиданный поворот
				}
			}
			break;

		case GetTypeVisitor::NK_ForStatement:

			break;

		default:

			//pConsole->log(OPS::Console::LEVEL_WARNING, _TL("The program fragment must contain 'for' and assign statements only.",nonApplicableStmts));
			return false; //нашли НЕОБРАБАТЫВАЕМЫЙ оператор -> фрагмент нельзя использовать.
		}
	}
	return true;
}

// Проверяет множество прародителей оператора, содержащего вхождение
//оно должно включать только блоки, циклы for и операторы if. while, case - нельзя!!!!
// Возвращает true, если граф постоить возможно, иначе false.
//block - на каком прародителе нужно остановиться
bool ElemLatticeGraph::newTestFragmentApplicability(StatementBase& block, StatementBase& occurrenceStmt)
{
    RepriseBase* parent = occurrenceStmt.getParent();
    while ((parent != 0) && (parent != &block))
    {
        if ( (!parent->is_a<BlockStatement>()) &&
            (!parent->is_a<ForStatement>())  &&
            (!parent->is_a<IfStatement>()) )
            return false;
        parent = parent->getParent();
    }
    if (parent == 0)
        throw OPS::RuntimeError("Оператор находится вне рассматриваемого блока!!!");
    return true;
}

//TODO: надо сделать конструктором Context!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Polyhedron* GetIterationSpace(const OccurDesc& entry, int deepLimit)
{
	if (entry.loopNumb == 0)
		return 0;

	if(deepLimit==-1)
		deepLimit = entry.loopNumb;

	//for(int i=0;i<entry.loopNumb;i++)
	for(int i=0;i<deepLimit;i++)
    {
        //левая граница (объединять обе границы в один if нельзя!!!!)
		if ( entry.loops[i].lBoundUnk ) 
        {
            if (entry.loops[i].lBoundNotLinear) 
                //граница цикла неизвестна и не является линейным выражением
                return 0;
            else
            {//граница цикла неизвестна и является линейным выражением, но содержат оператор max,
             //тогда контекст построить пока не можем (нужен ККАФ-контекст (см. статью Гуды про ККАФ))
                if (entry.loops[i].loopBounds.m_lowerNumb>1) return 0;
            }
        } 
        //правая граница (объединять обе границы в один if нельзя!!!!)
        if ( entry.loops[i].rBoundUnk ) 
        {
            if (entry.loops[i].rBoundNotLinear) 
                //граница цикла неизвестна и не является линейным выражением
                return 0;
            else
            {//граница цикла неизвестна и является линейным выражением, но содержат оператор max,
                //тогда контекст построить пока не можем (нужен ККАФ-контекст (см. статью Гуды про ККАФ))
                if (entry.loops[i].loopBounds.m_upperNumb>1) return 0;
            }
        } 
    }
	Polyhedron* con=new Polyhedron();
    //собираем все внешние параметры, от которых зависит Context (во всех границах всех циклов)
    std::set<OPS::Reprise::VariableDeclaration*> AllExternalParamsSet;
	std::set<OPS::Reprise::VariableDeclaration*> indexVariables;
    for(int i = 0; i < deepLimit; ++i)	{
		indexVariables.insert(entry.loops[i].counterIter);
            {
                //получаем список параметров левой границы
                std::map<OPS::Reprise::VariableDeclaration*, long>
					M=entry.loops[i].loopBounds.m_lowerExternalParamCoefs.getMap();
                std::map<OPS::Reprise::VariableDeclaration*, long>::iterator j;
                for (j=M.begin(); j!=M.end(); ++j)
					if(indexVariables.find(j->first) == indexVariables.end()) AllExternalParamsSet.insert(j->first);
            }
            {
                //получаем список параметров правой границы
                std::map<OPS::Reprise::VariableDeclaration*, long> 
					M=entry.loops[i].loopBounds.m_upperExternalParamCoefs.getMap();
                std::map<OPS::Reprise::VariableDeclaration*, long>::iterator j;
                for (j=M.begin(); j!=M.end(); ++j)
					if(indexVariables.find(j->first) == indexVariables.end()) AllExternalParamsSet.insert(j->first);
            }
    }
    int externalParamNum = (int)AllExternalParamsSet.size();
    //преобразуем set в list, чтобы зафиксировать порядок следования внешних параметров
    std::vector<OPS::Reprise::VariableDeclaration*> AllExternalParamsVector(externalParamNum);
    std::set<OPS::Reprise::VariableDeclaration*>::iterator it;
    int i=0;
    for (it=AllExternalParamsSet.begin(); it!=AllExternalParamsSet.end(); ++it,++i) AllExternalParamsVector[i]=*it;
    //запоминаем порядок следования и список внешних параметров в поле m_externalParamsVector контекста
    con->m_externalParamsVector=AllExternalParamsVector;
	//Inequality inq(1 + entry.loopNumb + externalParamNum);
	Inequality inq(1 + deepLimit + externalParamNum);
	inq.MakeZero();

	//for(int i = 0; i < entry.loopNumb; ++i) 
	for(int i = 0; i < deepLimit; ++i)	{
        //НЕРАВЕНСТВО ДЛЯ ЛЕВОЙ ГРАНИЦЫ ЦИКЛА
		inq.m_coefs[i+1] = 1;
		for(int j=0;j<(i+1);j++)
        {
            //заносим свободный член и коэффициенты перед счетчиками циклов
			inq.m_coefs[j]=-entry.loops[i].loopBounds.m_lower[0][j];
        }
        //заносим коэффициенты перед внешними параметрами
        for (int j=0; j<externalParamNum; ++j) {
			std::map<OPS::Reprise::VariableDeclaration*, long> M = entry.loops[i].loopBounds.m_lowerExternalParamCoefs.getMap();
            std::map<OPS::Reprise::VariableDeclaration*, long>::iterator p = M.find(AllExternalParamsVector[j]);
            if (p!=M.end()) inq.m_coefs[1 + deepLimit + j] = - p->second; //параметр найден
            else inq.m_coefs[1 + deepLimit + j] = 0; //не найден
        }
		con->AddInequality(inq);//добавили неравенство в контекст

        //НЕРАВЕНСТВО ДЛЯ ПРАВОЙ ГРАНИЦЫ ЦИКЛА
        //заносим свободный член и коэффициенты перед счетчиками циклов
		inq.m_coefs[i+1] = -1;
		memcpy(inq.m_coefs,entry.loops[i].loopBounds.m_upper[0].m_coefs,(i+1)*sizeof(int));
        //заносим коэффициенты перед внешними параметрами
        for (int j=0; j<externalParamNum; ++j) {
			std::map<OPS::Reprise::VariableDeclaration*, long> M = entry.loops[i].loopBounds.m_upperExternalParamCoefs.getMap();
            std::map<OPS::Reprise::VariableDeclaration*, long>::iterator p = M.find(AllExternalParamsVector[j]);
            if (p!=M.end()) inq.m_coefs[1 + deepLimit + j] = p->second; //параметр найден
            else inq.m_coefs[1 + deepLimit + j] = 0; //не найден
        }
		con->AddInequality(inq);//добавили неравенство в контекст
	}
	return con;
}

//TODO: исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
//возвр. 1, если в графе существует дуга из области fromCon в область toCon
//предполагается, что указатели являются "valid" и указывают на НЕПУСТЫЕ (feasible) контексты
//И граф описывается ЛИНЕЙНЫМИ функциями
int ElemLatticeGraph::RowExists(Polyhedron* fromCon,Polyhedron* toCon)
{
//	VoevodinSolution::iterator firstFunc=elg.getAndMakeVoevodinSolutionInTransformedVars().begin(),lastFunc=elg.getAndMakeVoevodinSolutionInTransformedVars().end();
//
//		while(firstFunc!=lastFunc)
//		{
//			GenArea* area=&((*firstFunc)->areas);
//
//			std::list<Context*>::iterator firstCon=area->Begin(),lastCon=area->End();
////			Context tempToCon;
//			while(firstCon!=lastCon)
//			{
//				Context tempToCon(**firstCon);
////				tempToCon=**firstCon;
//				tempToCon.IntersectWith(*toCon);
//
//				if(tempToCon.IsFeasible())
//				{
//					Context tempFromCon(*fromCon);
//					tempFromCon.Transform((*firstFunc)->data,(*firstFunc)->dimi,(*firstFunc)->dimj);
//					tempFromCon.IntersectWith(tempToCon);
//					if(tempFromCon.IsFeasible())
//						return 1;
//				}
////				tempToCon.clear();
//				firstCon++;
//			}
//
//			firstFunc++;
//		}
//
	return 0;
}

LinData ElemLatticeGraph::getLinData()
{
    return m_linData;
}

bool ElemLatticeGraph::isLoopsPresented()
{
    return m_loopsPresented;
}


void LinearExprDataForLatticeGraph::clear()   {
    m_loopDescSrcEntry.clear();
    m_loopDescDepEntry.clear();
    if (m_loopLowerBoundForSrcEntry) delete[] m_loopLowerBoundForSrcEntry;
    if (m_loopUpperBoundForSrcEntry) delete[] m_loopUpperBoundForSrcEntry;
    if (m_loopLowerBoundForDepEntry) delete[] m_loopLowerBoundForDepEntry;
    if (m_loopUpperBoundForDepEntry) delete[] m_loopUpperBoundForDepEntry;
    m_coefsSrcEntry.clear();
    m_coefsDepEntry.clear();
    if (m_transformMatrixSrcEntry)    {
        for (int i=0;i<m_dimiSrcEntryTransformMatrix;++i) 
            if (m_transformMatrixSrcEntry[i]) delete[] m_transformMatrixSrcEntry[i];
        delete[] m_transformMatrixSrcEntry;
    }
    if (m_invTransformMatrixSrcEntry)    {
        for (int i=0;i<m_dimiSrcEntryTransformMatrix;++i) 
            if (m_invTransformMatrixSrcEntry[i]) delete[] m_invTransformMatrixSrcEntry[i];
        delete[] m_invTransformMatrixSrcEntry;
    }  
    if (m_transformMatrixDepEntry)    {
        for (int i=0;i<m_dimiDepEntryTransformMatrix;++i) 
            if (m_transformMatrixDepEntry[i]) delete[] m_transformMatrixDepEntry[i];
        delete[] m_transformMatrixDepEntry;
    }
    if (m_invTransformMatrixDepEntry)    {
        for (int i=0;i<m_dimiDepEntryTransformMatrix;++i) 
            if (m_invTransformMatrixDepEntry[i]) delete[] m_invTransformMatrixDepEntry[i];
        delete[] m_invTransformMatrixDepEntry;
    }  

    m_loopLowerBoundForSrcEntry=NULL;
    m_loopUpperBoundForSrcEntry=NULL;
    m_loopLowerBoundForDepEntry=NULL;
    m_loopUpperBoundForDepEntry=NULL;
    m_transformMatrixSrcEntry=NULL;
    m_invTransformMatrixSrcEntry=NULL;
    m_transformMatrixDepEntry=NULL;
    m_invTransformMatrixDepEntry=NULL;
    m_loopNumbDepEntryOnly=0;
    m_loopNumbSrcEntry=0;
    m_coefsNumSrcDepEntry=0;
    m_dimiSrcEntryTransformMatrix=0; m_dimjSrcEntryTransformMatrix=0;
    m_dimiDepEntryTransformMatrix=0; m_dimjDepEntryTransformMatrix=0;
    m_commonLoopsNumb=0;
    m_externalParamNum=0;

}

//делает замену всего, согласно матрицам преобразований
void LinearExprDataForLatticeGraph::applyTransform()
{
    //делаем преобразование индексных выражений srcEntry
    for (int i=0;i< m_coefsNumSrcDepEntry;++i)  {
        //m_dimiSrcEntryTransformMatrix-1 - так как свободный член не заменяем. На самом деле надо m_dimiSrcEntryTransformMatrix-1-m_externalParamNum
        m_coefsSrcEntry[i].transform(m_transformMatrixSrcEntry,m_dimiSrcEntryTransformMatrix-1,m_dimjSrcEntryTransformMatrix);
    }
    //делаем преобразование индексных выражений depEntry
    for (int i=0;i< m_coefsNumSrcDepEntry;++i)  {
        m_coefsDepEntry[i].transform(m_transformMatrixDepEntry,m_dimiDepEntryTransformMatrix-1,m_dimjDepEntryTransformMatrix);
    }
    //КОНЕЦ "меняем индексные выражениия вхождений"

    //Меняем границы циклов. Корректно работают только замены вида i'=i+a или i'=-i+a, где 'a' может зависеть от внешних параметров 
    //Делаем замену в
    //а)циклах depEntry 
    //б)потом во всех циклах srcEntry
    //шаг а)
    for (int i=0;i<m_loopNumbDepEntryOnly;++i)  {
        //левая граница
        m_loopLowerBoundForDepEntry[i].transform(m_transformMatrixDepEntry,m_dimiDepEntryTransformMatrix-1,m_dimjDepEntryTransformMatrix);
        //в неравенстве i>=a левая часть тоже поменялась, переносим добавленные слагаемые в правую часть
        //именно здесь предполагается, что замена зависит только от внешних параметров и коэффициент при i равен +1 или -1
        //коэффициенты при счетчиках верхних циклов
        for (int j=0; j<m_commonLoopsNumb+i; j++) 
            m_loopLowerBoundForDepEntry[i][j+1]-=m_transformMatrixDepEntry[i+m_commonLoopsNumb][j];
        //коэффициенты при внешних параметрах
        for (int j=0; j<m_externalParamNum; j++) 
            m_loopLowerBoundForDepEntry[i][j+m_loopNumbDepEntryOnly+m_commonLoopsNumb+1]-=m_transformMatrixDepEntry[i+m_commonLoopsNumb][j+m_loopNumbDepEntryOnly+m_commonLoopsNumb];
        m_loopLowerBoundForDepEntry[i][0]-=m_transformMatrixDepEntry[i+m_commonLoopsNumb][m_dimjDepEntryTransformMatrix-1];//свободный член

        //правая граница
        m_loopUpperBoundForDepEntry[i].transform(m_transformMatrixDepEntry,m_dimiDepEntryTransformMatrix-1,m_dimjDepEntryTransformMatrix);
        //в неравенстве i>=a левая часть тоже поменялась, переносим добавленные слагаемые в правую часть
        //именно здесь предполагается, что замена счетчика зависит только от предыдущх счетчика параметров и коэффициент при i равен +1 или -1
        //коэффициенты при счетчиках верхних циклов
        for (int j=0; j<m_commonLoopsNumb+i; j++) 
            m_loopUpperBoundForDepEntry[i][j+1]-=m_transformMatrixDepEntry[i+m_commonLoopsNumb][j];
        //коэффициенты при внешних параметрах
        for (int j=0; j<m_externalParamNum; j++) 
            m_loopUpperBoundForDepEntry[i][j+m_loopNumbDepEntryOnly+m_commonLoopsNumb+1]-=m_transformMatrixDepEntry[i+m_commonLoopsNumb][j+m_loopNumbDepEntryOnly+m_commonLoopsNumb];
        m_loopUpperBoundForDepEntry[i][0]-=m_transformMatrixDepEntry[i+m_commonLoopsNumb][m_dimjDepEntryTransformMatrix-1];//свободный член

        if (m_transformMatrixDepEntry[i][i]==-1) {//тогда надо поменять у границ знак и поменять их местами
            m_loopLowerBoundForDepEntry[i].multiply(-1);
            m_loopUpperBoundForDepEntry[i].multiply(-1);
            SimpleLinearExpression temp=m_loopLowerBoundForDepEntry[i];
            m_loopLowerBoundForDepEntry[i]=m_loopUpperBoundForDepEntry[i];
            m_loopUpperBoundForDepEntry[i]=temp;
        }
    }
    //шаг б)
    for (int i=0;i<m_loopNumbSrcEntry;++i)  {
        //левая граница
        m_loopLowerBoundForSrcEntry[i].transform(m_transformMatrixSrcEntry,m_dimiSrcEntryTransformMatrix-1,m_dimjSrcEntryTransformMatrix);
        //в неравенстве i>=a левая часть тоже поменялась, переносим добавленные слагаемые в правую часть
        //именно здесь предполагается, что замена зависит только от внешних параметров и коэффициент при i равен +1 или -1
        //коэффициенты при счетчиках верхних циклов
        for (int j=0; j<i; j++) 
            m_loopLowerBoundForSrcEntry[i][j+1]-=m_transformMatrixSrcEntry[i][j];
        //коэффициенты при внешних параметрах
        for (int j=0; j<m_externalParamNum; j++) 
            m_loopLowerBoundForSrcEntry[i][j+m_loopNumbSrcEntry+1]-=m_transformMatrixSrcEntry[i][j+m_loopNumbSrcEntry];
        m_loopLowerBoundForSrcEntry[i][0]-=m_transformMatrixSrcEntry[i][m_dimjSrcEntryTransformMatrix-1];//свободный член

        //правая граница
        m_loopUpperBoundForSrcEntry[i].transform(m_transformMatrixSrcEntry,m_dimiSrcEntryTransformMatrix-1,m_dimjSrcEntryTransformMatrix);
        //в неравенстве i>=a левая часть тоже поменялась, переносим добавленные слагаемые в правую часть
        //именно здесь предполагается, что замена зависит только от внешних параметров и коэффициент при i равен +1 или -1
        //коэффициенты при счетчиках верхних циклов
        for (int j=0; j<i; j++) 
            m_loopUpperBoundForSrcEntry[i][j+1]-=m_transformMatrixSrcEntry[i][j];
        //коэффициенты при внешних параметрах
        for (int j=0; j<m_externalParamNum; j++) 
            m_loopUpperBoundForSrcEntry[i][j+m_loopNumbSrcEntry+1]-=m_transformMatrixSrcEntry[i][j+m_loopNumbSrcEntry];
        m_loopUpperBoundForSrcEntry[i][0]-=m_transformMatrixSrcEntry[i][m_dimjSrcEntryTransformMatrix-1];//свободный член

        if (m_transformMatrixSrcEntry[i][i]==-1) {//тогда надо поменять у границ знак и поменять их местами
            m_loopLowerBoundForSrcEntry[i].multiply(-1);
            m_loopUpperBoundForSrcEntry[i].multiply(-1);
            SimpleLinearExpression temp=m_loopLowerBoundForSrcEntry[i];
            m_loopLowerBoundForSrcEntry[i]=m_loopUpperBoundForSrcEntry[i];
            m_loopUpperBoundForSrcEntry[i]=temp;
        }
    }
}

//копирует все поля из структуры other кроме матриц преобразований
//вызывает clear(), поэтому матрицы преобразований нужно заполнять после!!!!!!!!!!
void LinearExprDataForLatticeGraph::copyAllExceptTransform(const LinearExprDataForLatticeGraph& other)
{
    clear();
    m_loopDescSrcEntry = other.m_loopDescSrcEntry;
    m_loopDescDepEntry = other.m_loopDescDepEntry;
    m_loopNumbDepEntryOnly = other.m_loopNumbDepEntryOnly;
    m_loopNumbSrcEntry = other.m_loopNumbSrcEntry;
    m_coefsNumSrcDepEntry=other.m_coefsNumSrcDepEntry;
    m_dimiSrcEntryTransformMatrix=other.m_dimiSrcEntryTransformMatrix; m_dimjSrcEntryTransformMatrix=other.m_dimjSrcEntryTransformMatrix;
    m_dimiDepEntryTransformMatrix=other.m_dimiDepEntryTransformMatrix; m_dimjDepEntryTransformMatrix=other.m_dimjDepEntryTransformMatrix;
    m_dimiSrcEntryTransformMatrix=other.m_dimiSrcEntryTransformMatrix;  
    m_dimjSrcEntryTransformMatrix=other.m_dimjSrcEntryTransformMatrix;
    m_dimiDepEntryTransformMatrix=other.m_dimiDepEntryTransformMatrix;  
    m_dimjDepEntryTransformMatrix=other.m_dimjDepEntryTransformMatrix;
    m_commonLoopsNumb=other.m_commonLoopsNumb;
    m_externalParamNum=other.m_externalParamNum;
    if (other.m_loopLowerBoundForSrcEntry)  
    {
        m_loopLowerBoundForSrcEntry=new SimpleLinearExpression[m_loopNumbSrcEntry];
        for (int i=0; i<m_loopNumbSrcEntry; ++i) 
            m_loopLowerBoundForSrcEntry[i]=other.m_loopLowerBoundForSrcEntry[i];
    }
    if (other.m_loopUpperBoundForSrcEntry)  
    {
        m_loopUpperBoundForSrcEntry=new SimpleLinearExpression[m_loopNumbSrcEntry];
        for (int i=0; i<m_loopNumbSrcEntry; ++i) 
            m_loopUpperBoundForSrcEntry[i]=other.m_loopUpperBoundForSrcEntry[i];
    }
    if (other.m_loopLowerBoundForDepEntry)  
    {
        m_loopLowerBoundForDepEntry=new SimpleLinearExpression[m_loopNumbDepEntryOnly];
        for (int i=0; i<m_loopNumbDepEntryOnly; ++i) 
            m_loopLowerBoundForDepEntry[i]=other.m_loopLowerBoundForDepEntry[i];
    }
    if (other.m_loopUpperBoundForDepEntry)  
    {
        m_loopUpperBoundForDepEntry=new SimpleLinearExpression[m_loopNumbDepEntryOnly];
        for (int i=0; i<m_loopNumbDepEntryOnly; ++i) 
            m_loopUpperBoundForDepEntry[i]=other.m_loopUpperBoundForDepEntry[i];
    }
    m_coefsSrcEntry=other.m_coefsSrcEntry;
    m_coefsDepEntry=other.m_coefsDepEntry;
}

//копирует все поля из структуры other
void LinearExprDataForLatticeGraph::copyAllFrom(LinearExprDataForLatticeGraph& other)
{
    copyAllExceptTransform(other);//в частности делает clear
    if (other.m_transformMatrixSrcEntry!=NULL) 
    {
        m_transformMatrixSrcEntry=new int*[m_dimiSrcEntryTransformMatrix];
        for (int i=0;i<m_dimiSrcEntryTransformMatrix;i++)
            if (other.m_transformMatrixSrcEntry[i]!=NULL)
            {
                m_transformMatrixSrcEntry[i]=new int[m_dimjSrcEntryTransformMatrix];
                for (int j=0;j<m_dimjSrcEntryTransformMatrix;j++) 
                    m_transformMatrixSrcEntry[i][j]=other.m_transformMatrixSrcEntry[i][j];
            }
    }
    if (other.m_invTransformMatrixSrcEntry!=NULL) 
    {
        m_invTransformMatrixSrcEntry=new int*[m_dimiSrcEntryTransformMatrix];
        for (int i=0;i<m_dimiSrcEntryTransformMatrix;i++)
            if (other.m_invTransformMatrixSrcEntry[i]!=NULL)
            {
                m_invTransformMatrixSrcEntry[i]=new int[m_dimjSrcEntryTransformMatrix];
                for (int j=0;j<m_dimjSrcEntryTransformMatrix;j++) 
                    m_invTransformMatrixSrcEntry[i][j]=other.m_invTransformMatrixSrcEntry[i][j];
            }
    }
    if (other.m_transformMatrixDepEntry!=NULL) 
    {
        m_transformMatrixDepEntry=new int*[m_dimiDepEntryTransformMatrix];
        for (int i=0;i<m_dimiDepEntryTransformMatrix;i++)
            if (other.m_transformMatrixDepEntry[i]!=NULL)
            {
                m_transformMatrixDepEntry[i]=new int[m_dimjDepEntryTransformMatrix];
                for (int j=0;j<m_dimjDepEntryTransformMatrix;j++) 
                    m_transformMatrixDepEntry[i][j]=other.m_transformMatrixDepEntry[i][j];
            }
    }
    if (other.m_invTransformMatrixDepEntry!=NULL) 
    {
        m_invTransformMatrixDepEntry=new int*[m_dimiDepEntryTransformMatrix];
        for (int i=0;i<m_dimiDepEntryTransformMatrix;i++)
            if (other.m_invTransformMatrixDepEntry[i]!=NULL)
            {
                m_invTransformMatrixDepEntry[i]=new int[m_dimjDepEntryTransformMatrix];
                for (int j=0;j<m_dimjDepEntryTransformMatrix;j++) 
                    m_invTransformMatrixDepEntry[i][j]=other.m_invTransformMatrixDepEntry[i][j];
            }
    }
}

//подставляет значения внешних параметров. Размер линейных выражений не меняет, просто обнуляет коэффициенты при внешних параметрах.
//пока подставляет только в границы циклов, если нужно еще - связывайтесь с Гудой
void LinearExprDataForLatticeGraph::substituteParams(const std::vector<int>& externalParamValuesVector)
{
    //циклы srcEntry
    for (int i=0;i<m_loopNumbSrcEntry;i++)  {
        for (int j=0;j<m_externalParamNum;j++) {
            m_loopUpperBoundForSrcEntry[i][0]+=m_loopUpperBoundForSrcEntry[i][1+m_loopNumbSrcEntry+j]*externalParamValuesVector[j];
            m_loopLowerBoundForSrcEntry[i][0]+=m_loopLowerBoundForSrcEntry[i][1+m_loopNumbSrcEntry+j]*externalParamValuesVector[j];
        }
    }
    //циклы depEntry
    for (int i=0;i<m_loopNumbDepEntryOnly;i++)  {
        for (int j=0;j<m_externalParamNum;j++) {
            m_loopUpperBoundForDepEntry[i][0]+=m_loopUpperBoundForDepEntry[i][1+m_commonLoopsNumb+m_loopNumbDepEntryOnly+j]*externalParamValuesVector[j];
            m_loopLowerBoundForDepEntry[i][0]+=m_loopLowerBoundForDepEntry[i][1+m_commonLoopsNumb+m_loopNumbDepEntryOnly+j]*externalParamValuesVector[j];
        }
    }
}

LinearExprDataForLatticeGraph::LinearExprDataForLatticeGraph():
         m_loopLowerBoundForSrcEntry(NULL)
        ,m_loopUpperBoundForSrcEntry(NULL)
        ,m_loopLowerBoundForDepEntry(NULL)
        ,m_loopUpperBoundForDepEntry(NULL)
		,m_loopNumbDepEntryOnly(0)
		,m_loopNumbSrcEntry(0)
		,m_commonLoopsNumb(0)
		,m_externalParamNum(0)
		,m_coefsNumSrcDepEntry(0)
		,m_transformMatrixSrcEntry(NULL)
		,m_dimiSrcEntryTransformMatrix(0)
		,m_dimjSrcEntryTransformMatrix(0)
		,m_transformMatrixDepEntry(NULL)
		,m_dimiDepEntryTransformMatrix(0)
		,m_dimjDepEntryTransformMatrix(0)
        ,m_invTransformMatrixSrcEntry(NULL)
        ,m_invTransformMatrixDepEntry(NULL)
{
}

LinearExprDataForLatticeGraph::~LinearExprDataForLatticeGraph()
{
    clear();
}

LinearExprDataForLatticeGraph::LinearExprDataForLatticeGraph(const LinearExprDataForLatticeGraph& other):
	m_loopLowerBoundForSrcEntry(NULL)
   ,m_loopUpperBoundForSrcEntry(NULL)
   ,m_loopLowerBoundForDepEntry(NULL)
   ,m_loopUpperBoundForDepEntry(NULL)
   ,m_loopNumbDepEntryOnly(0)
   ,m_loopNumbSrcEntry(0)
   ,m_commonLoopsNumb(0)
   ,m_externalParamNum(0)
   ,m_coefsNumSrcDepEntry(0)
   ,m_transformMatrixSrcEntry(NULL)
   ,m_dimiSrcEntryTransformMatrix(0)
   ,m_dimjSrcEntryTransformMatrix(0)
   ,m_transformMatrixDepEntry(NULL)
   ,m_dimiDepEntryTransformMatrix(0)
   ,m_dimjDepEntryTransformMatrix(0)
   ,m_invTransformMatrixSrcEntry(NULL)
   ,m_invTransformMatrixDepEntry(NULL)
{
    copyAllFrom((LinearExprDataForLatticeGraph&)other);
}

LinearExprDataForLatticeGraph& LinearExprDataForLatticeGraph::operator=(const LinearExprDataForLatticeGraph& other)
{
    copyAllFrom((LinearExprDataForLatticeGraph&)other);
    return *this;
}


////заполняет m_loopCounterMinForSrcEntry и другие такие же - работает вроди бы правильно! Хотя пока нигде не требуется
//void LinearExprDataForLatticeGraph::findMinAndMaxCounterValues()
//{
//    //отчищаем (на всякий случай)
//    if (m_loopCounterMinForSrcEntry) delete[] m_loopCounterMinForSrcEntry;
//    if (m_loopCounterMaxForSrcEntry) delete[] m_loopCounterMaxForSrcEntry;
//    if (m_loopCounterMinForDepEntry) delete[] m_loopCounterMinForDepEntry;
//    if (m_loopCounterMaxForDepEntry) delete[] m_loopCounterMaxForDepEntry;
//
//    //Для srcEntry заполняем массив минимальных значений индексов 
//    m_loopCounterMinForSrcEntry = new SimpleLinearExpression[m_loopNumbSrcEntry];
//    m_loopCounterMinForSrcEntry[0] = m_loopLowerBoundForSrcEntry[0];
//    m_loopCounterMaxForSrcEntry = new SimpleLinearExpression[m_loopNumbSrcEntry];
//    m_loopCounterMaxForSrcEntry[0] = m_loopUpperBoundForSrcEntry[0];
//    for (int i=1; i<m_loopNumbSrcEntry; i++)  {
//        //минимальное значение
//        m_loopCounterMinForSrcEntry[i].allocateMem(m_dimjSrcEntryTransformMatrix);
//        int k;
//        for (k=1+m_loopNumbSrcEntry; k<m_dimjSrcEntryTransformMatrix; k++)  {
//            //вычисляем коэффициент при k-том параметре
//            m_loopCounterMinForSrcEntry[i][k]=m_loopLowerBoundForSrcEntry[i][k];
//            for (int j=0; j<i; j++)
//                if (m_loopLowerBoundForSrcEntry[i][j+1]>=0) 
//                    m_loopCounterMinForSrcEntry[i][k]+=m_loopLowerBoundForSrcEntry[i][j+1]*m_loopCounterMinForSrcEntry[j][k];
//                else 
//                    m_loopCounterMinForSrcEntry[i][k]+=m_loopLowerBoundForSrcEntry[i][j+1]*m_loopCounterMaxForSrcEntry[j][k];
//        }
//        k=0;
//        //вычисляем свободный член
//        m_loopCounterMinForSrcEntry[i][k]=m_loopLowerBoundForSrcEntry[i][k];
//        for (int j=0; j<i; j++)
//            if (m_loopLowerBoundForSrcEntry[i][j+1]>=0) 
//                m_loopCounterMinForSrcEntry[i][k]+=m_loopLowerBoundForSrcEntry[i][j+1]*m_loopCounterMinForSrcEntry[j][k];
//            else 
//                m_loopCounterMinForSrcEntry[i][k]+=m_loopLowerBoundForSrcEntry[i][j+1]*m_loopCounterMaxForSrcEntry[j][k];
//
//        //максимальное значение
//        m_loopCounterMaxForSrcEntry[i].allocateMem(m_dimjSrcEntryTransformMatrix);
//        for (k=1+m_loopNumbSrcEntry; k<m_dimjSrcEntryTransformMatrix; k++)  {
//            //вычисляем коэффициент при k-том параметре
//            m_loopCounterMaxForSrcEntry[i][k]=m_loopUpperBoundForSrcEntry[i][k];
//            for (int j=0; j<i; j++)
//                if (m_loopUpperBoundForSrcEntry[i][j+1]>=0) 
//                    m_loopCounterMaxForSrcEntry[i][k]+=m_loopUpperBoundForSrcEntry[i][j+1]*m_loopCounterMaxForSrcEntry[j][k];
//                else 
//                    m_loopCounterMaxForSrcEntry[i][k]+=m_loopUpperBoundForSrcEntry[i][j+1]*m_loopCounterMinForSrcEntry[j][k];
//        }
//        k=0;
//        //вычисляем свободный член
//        m_loopCounterMaxForSrcEntry[i][k]=m_loopUpperBoundForSrcEntry[i][k];
//        for (int j=0; j<i; j++)
//            if (m_loopUpperBoundForSrcEntry[i][j+1]>=0) 
//                m_loopCounterMaxForSrcEntry[i][k]+=m_loopUpperBoundForSrcEntry[i][j+1]*m_loopCounterMaxForSrcEntry[j][k];
//            else 
//                m_loopCounterMaxForSrcEntry[i][k]+=m_loopUpperBoundForSrcEntry[i][j+1]*m_loopCounterMinForSrcEntry[j][k];
//    }
//
//    //Для depEntry заполняем массив минимальных значений индексов 
//    if (m_loopNumbDepEntryOnly>0)  {
//        m_loopCounterMinForDepEntry = new SimpleLinearExpression[m_loopNumbDepEntryOnly];
//        m_loopCounterMaxForDepEntry = new SimpleLinearExpression[m_loopNumbDepEntryOnly];
//        for (int i=1; i<m_loopNumbDepEntryOnly; i++)  {
//            //минимальное значение
//            m_loopCounterMinForDepEntry[i].allocateMem(m_dimjDepEntryTransformMatrix);
//            int k;
//            for (k=1+m_commonLoopsNumb+m_loopNumbDepEntryOnly; k<m_dimjDepEntryTransformMatrix; k++)  {
//                //вычисляем коэффициент при k-том параметре
//                m_loopCounterMinForDepEntry[i][k]=m_loopLowerBoundForDepEntry[i][k];
//                for (int j=0; j<m_commonLoopsNumb; j++)
//                    if (m_loopLowerBoundForDepEntry[i][j+1]>=0) 
//                        m_loopCounterMinForDepEntry[i][k]+=m_loopLowerBoundForDepEntry[i][j+1]*m_loopCounterMinForSrcEntry[j][k];
//                    else 
//                        m_loopCounterMinForDepEntry[i][k]+=m_loopLowerBoundForDepEntry[i][j+1]*m_loopCounterMaxForSrcEntry[j][k];
//                for (int j=m_commonLoopsNumb; j<m_commonLoopsNumb+m_loopNumbDepEntryOnly; j++)
//                    if (m_loopLowerBoundForDepEntry[i][j+1]>=0) 
//                        m_loopCounterMinForDepEntry[i][k]+=m_loopLowerBoundForDepEntry[i][j+1]*m_loopCounterMinForDepEntry[j-m_commonLoopsNumb][k];
//                    else 
//                        m_loopCounterMinForDepEntry[i][k]+=m_loopLowerBoundForDepEntry[i][j+1]*m_loopCounterMaxForDepEntry[j-m_commonLoopsNumb][k];
//            }
//            k=0;
//            //вычисляем свободный член
//            m_loopCounterMinForDepEntry[i][k]=m_loopLowerBoundForDepEntry[i][k];
//            for (int j=0; j<m_commonLoopsNumb; j++)
//                if (m_loopLowerBoundForDepEntry[i][j+1]>=0) 
//                    m_loopCounterMinForDepEntry[i][k]+=m_loopLowerBoundForDepEntry[i][j+1]*m_loopCounterMinForSrcEntry[j][k];
//                else 
//                    m_loopCounterMinForDepEntry[i][k]+=m_loopLowerBoundForDepEntry[i][j+1]*m_loopCounterMaxForSrcEntry[j][k];
//            for (int j=m_commonLoopsNumb; j<m_commonLoopsNumb+m_loopNumbDepEntryOnly; j++)
//                if (m_loopLowerBoundForDepEntry[i][j+1]>=0) 
//                    m_loopCounterMinForDepEntry[i][k]+=m_loopLowerBoundForDepEntry[i][j+1]*m_loopCounterMinForDepEntry[j-m_commonLoopsNumb][k];
//                else 
//                    m_loopCounterMinForDepEntry[i][k]+=m_loopLowerBoundForDepEntry[i][j+1]*m_loopCounterMaxForDepEntry[j-m_commonLoopsNumb][k];
//
//            //максимальное значение
//            m_loopCounterMaxForDepEntry[i].allocateMem(m_dimjDepEntryTransformMatrix);
//            for (k=1+m_loopNumbDepEntryOnly; k<m_dimjDepEntryTransformMatrix; k++)  {
//                //вычисляем коэффициент при k-том параметре
//                m_loopCounterMaxForDepEntry[i][k]=m_loopUpperBoundForDepEntry[0][k];
//                for (int j=0; j<m_commonLoopsNumb; j++)
//                    if (m_loopUpperBoundForDepEntry[i][j+1]>=0) 
//                        m_loopCounterMaxForDepEntry[i][k]+=m_loopUpperBoundForDepEntry[i][j+1]*m_loopCounterMaxForSrcEntry[j][k];
//                    else 
//                        m_loopCounterMaxForDepEntry[i][k]+=m_loopUpperBoundForDepEntry[i][j+1]*m_loopCounterMinForSrcEntry[j][k];
//                for (int j=m_commonLoopsNumb; j<m_commonLoopsNumb+m_loopNumbDepEntryOnly; j++)
//                    if (m_loopUpperBoundForDepEntry[i][j+1]>=0) 
//                        m_loopCounterMaxForDepEntry[i][k]+=m_loopUpperBoundForDepEntry[i][j+1]*m_loopCounterMaxForDepEntry[j-m_commonLoopsNumb][k];
//                    else 
//                        m_loopCounterMaxForDepEntry[i][k]+=m_loopUpperBoundForDepEntry[i][j+1]*m_loopCounterMinForDepEntry[j-m_commonLoopsNumb][k];
//            }
//            k=0;
//            //вычисляем свободный член
//            m_loopCounterMaxForDepEntry[i][k]=m_loopUpperBoundForDepEntry[0][k];
//            for (int j=0; j<m_commonLoopsNumb; j++)
//                if (m_loopUpperBoundForDepEntry[i][j+1]>=0) 
//                    m_loopCounterMaxForDepEntry[i][k]+=m_loopUpperBoundForDepEntry[i][j+1]*m_loopCounterMaxForSrcEntry[j][k];
//                else 
//                    m_loopCounterMaxForDepEntry[i][k]+=m_loopUpperBoundForDepEntry[i][j+1]*m_loopCounterMinForSrcEntry[j][k];
//            for (int j=m_commonLoopsNumb; j<m_commonLoopsNumb+m_loopNumbDepEntryOnly; j++)
//                if (m_loopUpperBoundForDepEntry[i][j+1]>=0) 
//                    m_loopCounterMaxForDepEntry[i][k]+=m_loopUpperBoundForDepEntry[i][j+1]*m_loopCounterMaxForDepEntry[j-m_commonLoopsNumb][k];
//                else 
//                    m_loopCounterMaxForDepEntry[i][k]+=m_loopUpperBoundForDepEntry[i][j+1]*m_loopCounterMinForDepEntry[j-m_commonLoopsNumb][k];
//        }
//    }
//}

}//end of namespace
}//end of namespace
