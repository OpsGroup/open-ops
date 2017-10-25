#include "Analysis/Montego/OccurrenceCoeffs.h"
#include "Analysis/Montego/Occurrence.h"
#include "Shared/LoopShared.h"

namespace OPS
{
namespace Montego
{

//конструирует коэффициенты, распознавая линейные выражения
OccurrenceCoeffs::OccurrenceCoeffs(const BasicOccurrence &o)
{
    m_occurrence = &o;
    m_isCoeffsLinear = false;
    buildCoeffs();
}

OccurrenceCoeffs::~OccurrenceCoeffs()
{
    for (size_t i = 0; i < m_coeffs.size(); ++i)
        delete m_coeffs[i];
    m_coeffs.clear();
}

const BasicOccurrence* OccurrenceCoeffs::getOccurrence()
{
    return m_occurrence;
}

bool OccurrenceCoeffs::IsCoeffsLinear()
{
    return m_isCoeffsLinear;
}

void OccurrenceCoeffs::buildCoeffs()
{
    m_isCoeffsLinear = true;
    m_coeffs.clear();
    m_coeffs.resize(m_occurrence->getBracketCount());
    BasicOccurrenceName name = m_occurrence->getName();
    OPS::Reprise::ReferenceExpression* refExpr = m_occurrence->getRefExpr();
    //если есть скобки
    if (m_occurrence->getBracketCount() > 0)
    {
        m_coeffs.resize(m_occurrence->getBracketCount());
        int currentBracketNum = 0;
        //операция взятия скобок
        OPS::Reprise::RepriseBase* parent = refExpr->getParent();
        try
        {
            //цикл по полям структуры
            for (int i = -1; i < (int)name.m_fields.size(); ++i)
            {
                //количество скобок у поля структуры
                int last_j = (i==-1)? name.m_refExprBracketCount : name.m_fields[i].second;
                if (last_j > 0)
                {
                    OPS::Reprise::BasicCallExpression& arrayCall = parent->cast_to<OPS::Reprise::BasicCallExpression>();
                    OPS_ASSERT(arrayCall.getChildCount()-1 == last_j);
                    //цикл по скобкам у поля структуры
                    for (int j = 0; j < last_j; ++j)
                    {
                        m_coeffs[currentBracketNum] = 
                            OPS::Shared::ParametricLinearExpression::createByAllVariables(& (arrayCall.getChild(j+1).cast_to<OPS::Reprise::ExpressionBase>()));
                        if (m_coeffs[currentBracketNum] == 0)
                            throw OPS::RuntimeError("Нелинейное выражение");
                        else ++currentBracketNum;
                    }
                }
                if (last_j > 0) parent = parent->getParent(); //проходим обращение к массиву
                parent = parent->getParent();//переход к следующему полю
            }
        }
        catch (OPS::RuntimeError&)
        {
            //коэффициенты линейного выражения не являются целыми числами
            m_isCoeffsLinear = false;
        }
        if (!m_isCoeffsLinear)//если нелинейность - удаляем построенные коэффициенты
        {
            for (int i = 0; i < currentBracketNum; ++i)
                delete m_coeffs[i];
            m_coeffs.clear();
        }
    }//КОНЕЦ "если есть скобки"
}

/// Коэффиценты при счетчиках охватывающих циклов и свободный член.
/// счетчиками циклов считаются только счетчики циклов, принадлежащих program
/// счетчики внешних циклов считаются внешними переменными (0 - рассматриваем все окаймляющие вхождение циклы)
/// loopCounterCoefficients[i][0] - свободные члены
/// возвращает true - коэффициенты распознаны, false - нелинейность
bool OccurrenceCoeffs::getExternalParamAndLoopCounterCoefficients(
    std::vector<OPS::LatticeGraph::SimpleLinearExpression>& loopCounterCoefs, 
    std::vector<OPS::Shared::CanonicalLinearExpression>& externalParamCoefs, 
    OPS::Reprise::RepriseBase* program)
{
    int n = m_occurrence->getBracketCount();
    if (!m_isCoeffsLinear) 
        throw OPS::RuntimeError("Coeffs nonlinear but wanted! Use IsCoeffsLinear() to check.");
    
    externalParamCoefs.clear();
    externalParamCoefs.resize(n);
    loopCounterCoefs.clear();
    loopCounterCoefs.resize(n);

    std::vector<OPS::Reprise::VariableDeclaration*> loopCounters 
        = OPS::Shared::getIndexVariables(m_occurrence->getRefExpr(), program);

    try
    {
        for (int i = 0; i < n; ++i)
        {
			LatticeGraph::getExternalParamAndLoopCounterCoefficients
				(*m_coeffs[i], loopCounters, loopCounterCoefs[i], externalParamCoefs[i]);
            //std::cout << loopCounterCoefs[i].toString() << "\n";
        }
    }
    catch (OPS::RuntimeError&)
    {
        externalParamCoefs.clear();
        loopCounterCoefs.clear();
        return false;
    }
    return true;
}

/// Коэффиценты при переменных в указанном порядке и своб. член. Выбрасывает исключение OPS_ASSERT,
/// если указаны не все переменные, участвующие в выражении.
/// coeffs[i][0] - свободные члены
/// возвращает true - коэффициенты распознаны, false - нелинейность
bool OccurrenceCoeffs::getAllVarsCoeffsInOrder(std::vector<LatticeGraph::SimpleLinearExpression>& coeffs, 
                                               std::vector<OPS::Reprise::VariableDeclaration*> variableOrder)
{
    int n = m_occurrence->getBracketCount();
    if (!m_isCoeffsLinear) 
        throw OPS::RuntimeError("Coeffs nonlinear but wanted! Use IsCoeffsLinear() to check.");

    coeffs.clear();
    coeffs.resize(n);
    
    OPS::Shared::CanonicalLinearExpression externalParamCoefs;
    bool flagAllVars = true;
    try
    {
        for (int i = 0; i < n; ++i)
			LatticeGraph::getExternalParamAndLoopCounterCoefficients
			(*m_coeffs[i], variableOrder, coeffs[i], externalParamCoefs);
        if (!externalParamCoefs.isConstant()) flagAllVars = false;
    }
    catch (OPS::RuntimeError&)
    {
        coeffs.clear();
        return false;
    }
    OPS_ASSERT(flagAllVars = true); //в массиве variableOrder должны быть указаны ВСЕ входящие в выражение переменные

    return true;
}


//приводим индексные выражения к одному измерению (если известны границы массива)
void OccurrenceCoeffs::mapToOneDim()
{
    //if (!data) return;
    //if (!ub)   return;

    //if (d1)
    //    delete[] d1;

    //d1 = new int[loopNumb + 1];
    //for(int i = 0; i <= loopNumb; ++i)
    //    d1[i] = 0;

    //int mult = 1;
    //for(int j = dim-1; j >= 0; --j)
    //{
    //    for(int i = 0; i <= loopNumb; ++i)
    //        d1[i] += mult*data[j][i];
    //    d1[loopNumb] -= mult*0;		//ВЕЗДЕ ГДЕ "0" нужно "lb[j]"
    //    mult *= ub[j] - 0 + 1;				// -- // --
    //}
}

}//end of namespace
}//end of namespace
