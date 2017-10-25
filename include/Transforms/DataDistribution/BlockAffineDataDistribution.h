#ifndef OPS_TRANSFORMATIONS_BLOCK_AFFINE_DATA_DISTRIBUTION_H
#define OPS_TRANSFORMATIONS_BLOCK_AFFINE_DATA_DISTRIBUTION_H

#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include "Transforms/ITransformation.h"

#include "Reprise/Reprise.h"
#include "BlockAffineDataDistributionParameters.h"

#include <list>

/*!
	Block-Affine Data Distribution.

--- Abstract:
	
	Parameters:
		1)	P is a number of processors
		3)	A[0..dim(0), 0..dim(1), 0..dim(2), ..., 0..dim(N-1)] is a multi-dimensional array
			Where	a) N is a count of dimensions.
					b) dim(k) is a size of k-dimension.
		2)	c(0), c(1), c(2), ..., c(N), d(0), d(1), d(2), ..., d(N-1) - 2*N+1 integer parameters,
		which specify block-affine distribution of array A by P processor units:

		Array element with N-dimensional index (i(0), i(1), i(2), ..., i(N-1)) will be local for
		processor with number:
			p = ( ]i(0)/d(0)[*c(0) + ]i(1)/d(1)[*c(1) + ... + ]i(N-1)/d(N01)[*c(N-1) + c(N) ) mod P
			
---- Technical details:		
	
---- Requirements:		
	
*/

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
	/*!
			BADD - Блочно-аффинное размещение данных (Block-Affine Data Distribution). Сокращено для удобства
			написания кода.		
	*/

	/*!
		\brief BADD - преобразование, которое переразмещает массивы и меняет индексы массива в данном блоке.
	*/
	class BADD : public OPS::TransformationsHub::TransformBase
	{
	public:
		class BADDException: public OPS::Exception
		{
		public:
			BADDException(std::string message): OPS::Exception(message) {};
		};

		BADD();

        virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params);
	};

	
	//		Класс, который меняет индексы массива в данном блоке.	
    class BADDIndexModifier
	{
    public:
		/*!
		Входные параметры:
			* distrArgs - параметры размещения. Массив 'a', т.к. он для каждого вхождения он свой.
			* arrayDeclaration - размещаемый массив X
			* changingIndexesBlock - блок в котором меняются вхождения массива X[i1,i2,...] на XX(F(i1,i2,...))
			* fDim - размерность относительно которой производится размещение
			* translationUnit - модуль в котором происходит размещение
		*/
		void setParameters(BADDParameters distrArgs, 
			OPS::Reprise::VariableDeclaration* arrayDeclaration,
			OPS::Reprise::StatementBase* changingIndexesBlock,
			int fDim, 
            OPS::Reprise::TranslationUnit& translationUnit,
            bool saveOldExprInNode = false);
			
		//	* changingIndexesBlock - список операторов в которых вхождения массива X[i1,i2,...] на XX(F(i1,i2,...))
		void setParameters2(BADDParameters& distrArgs, 
			OPS::Reprise::VariableDeclaration* arrayDeclaration,
			std::list<OPS::Reprise::StatementBase*> changingIndexesStmtList,
			int fDim, 
			OPS::Reprise::TranslationUnit& translationUnit);

		BADDParameters getDistrArguments() { return m_distrArgs; }
		
		//		функция isValid проверяет корректность входных данных		
		bool isValid();
 
		//		Получить все обращения к массивк X в блоке changingIndexesBlock				
		std::list<OPS::Reprise::BasicCallExpression*> getAllDistribingArrayReferences();		
		std::list<OPS::Reprise::BasicCallExpression*> getGeneratorsDistribingArrayReferences();		
		std::list<OPS::Reprise::BasicCallExpression*> getUsingsDistribingArrayReferences();

		//		Получить BADD-семейство для массива X в заданном блоке
		bool getBAADParametersFamily(BADDParametersFamily& result);

		/*!
				Получить уточненные параметры размещения для конкретного вхождения. Добавятся дополнительные 
				параметры ai, которые используются для реализации перекрытия областей.
				Пр.: Если подать на вход X[i+4][j-2], то a0 = -4, a1 = 2
		*/
		BADDParameters getDistrParametersForArrayReference(OPS::Reprise::BasicCallExpression* arrayAccesExpr);
		
		void changeIndexes();

		//		Функция, которая возвращает declaration созданного массива XX
		OPS::Reprise::VariableDeclaration* getNewArrayDeclaration();

		//		Функция, которая создает и возвращает оператор XX = malloc(<размер нового массива XX>)
		OPS::Reprise::StatementBase* createNewArrayAllocationStatement();
				


		OPS::Reprise::ExpressionBase* tryGetOptimizedLocalIndexExprByGlobalIndexExr(OPS::Reprise::RepriseList<OPS::Reprise::ExpressionBase>& globalIndexExprList,
			OPS::Transforms::DataDistribution::BADDParameters distrArg);
	
		/*!
			Основная функция, которая заменяет индексное выражение  вхождения массива X.
		*/
		OPS::Reprise::ExpressionBase* getLocalIndexExprByGlobalIndexExr(OPS::Reprise::RepriseList<OPS::Reprise::ExpressionBase>& globalIndexExprList,
			OPS::Transforms::DataDistribution::BADDParameters distrArg);
		OPS::Reprise::ExpressionBase* getLocalIndexExprByGlobalIndexExrForOneProcessor(OPS::Reprise::RepriseList<OPS::Reprise::ExpressionBase>& globalIndexExprList,
			OPS::Transforms::DataDistribution::BADDParameters distrArg);		
	private:

//
//			Private methods
//
		//		находит массив k. 
		//		k[i] - размер клетки в блоках по i-й размерности и лежит в диапозоне [1..P]. 		
		std::vector<int> internal_getK();

		//		находит номер процессора в котором находится переменная с данным мульти-индексом.
		int internal_getProcIdByGlobalIndex(std::vector<int> globalIndex);

		//		находит длину нового массива
		int internal_getNewArrayLength();
		
		//		создает массив XX
		OPS::Reprise::VariableDeclaration* internal_createNewArrayDeclaration(OPS::Reprise::VariableDeclaration* arrayDeclaration);

//
//			Private fields
//

		BADDParameters m_distrArgs;
		BADDParametersFamily m_distrParamsFamily;

		OPS::Reprise::VariableDeclaration* m_arrayDeclaration;

		std::list<OPS::Reprise::StatementBase*> m_changingIndexesStmtList;
		
		/*!
				fDim - измерение относительно которого будет происходить размещение данных.
				Этот параметр влияет на количество используемой памяти. Лучше то fDim, при котором k[fDim] - минимально.
		*/
		int m_fDim;
		OPS::Reprise::TranslationUnit* m_translationUnit;		

		//		Объявление нового массива XX
		OPS::Reprise::VariableDeclaration* m_newArrayDeclaration;

        bool m_saveOldExprInNode;
	};
}
}
}

#endif // OPS_TRANSFORMATIONS_BLOCK_AFFINE_DATA_DISTRIBUTION_H
