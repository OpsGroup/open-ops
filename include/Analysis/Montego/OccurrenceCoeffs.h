#pragma once
#include <vector>
#include "Analysis/LatticeGraph/LinearLib.h"
/*
класс для работы с коэффициентами в скобках [] вхождения Occurrence
*/

namespace OPS
{
namespace Montego
{

class BasicOccurrence;

class OccurrenceCoeffs
{
public:
    //конструирует коэффициенты, распознавая линейные выражения
	explicit OccurrenceCoeffs(const BasicOccurrence &o);

    ~OccurrenceCoeffs();

    /// Коэффиценты при счетчиках охватывающих циклов и свободный член.
    /// счетчиками циклов считаются только счетчики циклов, принадлежащих program
    /// счетчики внешних циклов считаются внешними переменными (0 - рассматриваем все окаймляющие вхождение циклы)
    /// loopCounterCoefficients[i][0] - свободные члены
    /// возвращает true - коэффициенты распознаны, false - нелинейность
    bool getExternalParamAndLoopCounterCoefficients(std::vector<LatticeGraph::SimpleLinearExpression>& loopCounterCoeffs, std::vector<OPS::Shared::CanonicalLinearExpression>& externalParamCoefs, OPS::Reprise::RepriseBase* program = 0);

    /// Коэффиценты при переменных в указанном порядке и своб. член. Выбрасывает исключение OPS_ASSERT,
    /// если указаны не все переменные, участвующие в выражении.
    /// coeffs[i][0] - свободные члены
    /// возвращает true - коэффициенты распознаны, false - нелинейность
    bool getAllVarsCoeffsInOrder(std::vector<LatticeGraph::SimpleLinearExpression>& coeffs, std::vector<OPS::Reprise::VariableDeclaration*> variableOrder);

    ///	m_coeffs1D эквивалентно coeffs, НО аппроксимировано до одномерного массива, если границы массива были известны;
    std::vector<int> getCoeffs1D(); 

    //приводим индексные выражения к одному измерению (если известны границы массива)
    void mapToOneDim();

	const BasicOccurrence* getOccurrence();

    bool IsCoeffsLinear();

private:

    //заполняет m_coeffs
    void buildCoeffs();

	const BasicOccurrence* m_occurrence;

    /// Коэффиценты и ссылки на переменные, при которых они стоят
    std::vector<OPS::Shared::ParametricLinearExpression*> m_coeffs; 

    bool m_isCoeffsLinear;
};

}//end of namespace
}//end of namespace

