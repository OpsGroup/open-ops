#ifndef OPS_SHARED_LOOPSHARED_H_INCLUDED__
#define OPS_SHARED_LOOPSHARED_H_INCLUDED__

#include "Reprise/Statements.h"

#include <list>

namespace OPS
{
namespace Shared
{
	
/// подсчет охватывающих e циклов for, принадлежащих заданному фрагменту программы code
/// возвращает -1, если e не принадлежит code. Если code=0, возвращает кол-во всех циклов
int getEmbracedLoopsCount(const OPS::Reprise::RepriseBase* e, const OPS::Reprise::RepriseBase* code = 0);

/// выдает занумерованный (от 1) набор счетчиков циклов, принадлежащих заданному фрагменту программы code и
/// окаймляющих данный выражение ExpressionBase e
/// или оператор StatementBase e. Если аргумент не является оператором или выражением - выбрасывает исключение
/// Если оператор-аргумент является оператором цикла, то его счетчик включается в возвращаемый массив
/// Если code == 0, возвращаются все счетчики
std::vector<OPS::Reprise::VariableDeclaration*> getIndexVariables(OPS::Reprise::RepriseBase* e, OPS::Reprise::RepriseBase* code = 0);

///// выдает занумерованный (от 1) набор счетчиков циклов окаймляющих данный оператор
//std::map<OPS::Reprise::VariableDeclaration*, int>& getIndexVariables(OPS::Reprise::StatementBase* Stmt);
//
/// Определеняет количество общих внешних циклов для двух операторов
int getCommonUpperLoopsAmount(OPS::Reprise::StatementBase* Stmt1, OPS::Reprise::StatementBase* Stmt2);

/// Определеняет количество общих внешних циклов для двух вхождений 
int getCommonUpperLoopsAmount(OPS::Reprise::ExpressionBase* Occurrence1, OPS::Reprise::ExpressionBase* Occurrence2);

// возвращает количество общих циклов for для двух узлов e1,e2. Считаются только циклы внутри заданного
// фрагмента программы
int getCommonUpperLoopsAmount(OPS::Reprise::RepriseBase* e1, OPS::Reprise::RepriseBase* e2, OPS::Reprise::RepriseBase* code);

/**
	\brief Определеяет является ли гнездо циклов тесным
	Тесным считается цикл вида:
	 for ...
	  for ...
	   for ...
	     Statements
	Каждый for содержит только один for и возможно пустые операторы, а Statements не содержит циклов for. 
	\param Loop - указатель на самый внешний цикл в гнезде
	\return 
		\retval true - гнездо циклов тесное
		\retval false - гнездо циклов не тесное
*/
bool isPerfectLoopNest(OPS::Reprise::ForStatement* Loop);

std::vector<OPS::Reprise::ForStatement*> getLoopsOfPerfectNest(OPS::Reprise::ForStatement& upperLoop);

/**
	\brief Проверяет, есть ли в блоке хоть один оператор типа CheckType
	Если в качестве CheckType указать не наследника StatementBase, то функция вернет true.
	\param BlockToCheck - блок в котором ищем операторы типа CheckType
	\return 
		\retval true - если найден оператор типа CheckType
		\retval false - если не найден оператор типа CheckType
*/
template <typename CheckType> bool isPresentInBlock(const OPS::Reprise::BlockStatement& BlockToCheck)
{
	using namespace OPS::Reprise;

	BlockStatement::ConstIterator itStmtIter = BlockToCheck.getFirst();
	while(itStmtIter.isValid() == 1)
	{
		if ((*itStmtIter).is_a<CheckType>())
			// Найден оператор типа CheckType
			return true; 

		if ((*itStmtIter).is_a<BlockStatement>())
			if (isPresentInBlock<CheckType>((*itStmtIter).cast_to<BlockStatement>()))
				return true; // иначе продолжаем поиск

		if ((*itStmtIter).is_a<ForStatement>())
			if (isPresentInBlock<CheckType>((*itStmtIter).cast_to<ForStatement>().getBody()))
				return true; // иначе продолжаем поиск

		if ((*itStmtIter).is_a<WhileStatement>())
			if (isPresentInBlock<CheckType>((*itStmtIter).cast_to<WhileStatement>().getBody()))
				return true; // иначе продолжаем поиск

		if ((*itStmtIter).is_a<PlainSwitchStatement>())
			if (isPresentInBlock<CheckType>((*itStmtIter).cast_ptr<PlainSwitchStatement>()->getBody()))
				return true; // иначе продолжаем поиск

		if ((*itStmtIter).is_a<IfStatement>())
		{
			const IfStatement* pIf = (*itStmtIter).cast_ptr<IfStatement>();
			if (isPresentInBlock<CheckType>(pIf->getThenBody()))
				return true; // иначе продолжаем поиск
			if (isPresentInBlock<CheckType>(pIf->getElseBody()))
				return true; // иначе продолжаем поиск
		}

		itStmtIter++;
	}

	return false;
}

/**
	\brief Check that all FOR loops in the nest are canonized
		\param pTopLoopInNest - the head of investigated nest
*/
bool isAllLoopsInNestAreCanonised(OPS::Reprise::ForStatement* pTopLoopInNest);

/**
	\brief Returns list of embraced loops inside code
		\param smth - expression or statement
        \param code - code fragment
*/
std::list<OPS::Reprise::ForStatement*> getEmbracedLoopsNest(OPS::Reprise::RepriseBase& smth, OPS::Reprise::RepriseBase* code = 0);
std::list<const OPS::Reprise::ForStatement*> getEmbracedLoopsNest(const OPS::Reprise::RepriseBase& smth, const OPS::Reprise::RepriseBase* code = 0);

/// Возвращает номер цикла в гнездe, начиная с самого внешнего.
/// Вернен -1 если цикл не принадлежит гнезду
int getEmbracedLoopsIndex(OPS::Reprise::RepriseBase& stmt, OPS::Reprise::ForStatement& loop, OPS::Reprise::RepriseBase* code = 0);

} // end namespace Shared
} // end namespace OPS

#endif //OPS_SHARED_LOOPSHARED_H_INCLUDED__
