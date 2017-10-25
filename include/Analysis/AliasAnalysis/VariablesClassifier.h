#ifndef OPS_ALIASANALYSIS_VARIABLESCLASSIFIER_H_INCLUDED__
#define OPS_ALIASANALYSIS_VARIABLESCLASSIFIER_H_INCLUDED__

#include "Analysis/AliasAnalysis/PointersTable.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace AliasAnalysis
{

typedef std::vector<ExprName> VariablesList;

/// Параметры доступа
enum ParamAccess
{
	VAR_GLOB_MODIFIED=1,
	VAR_GLOB_READONLY,
	VAR_LOCAL_MODIFIED,
	VAR_LOCAL_READONLY
};

/// Разделение переменных по способу обращения к памяти
typedef std::map<ExprName, ParamAccess> VarAccess;

struct ParamCall
{
	VarAccess varAccess;			/// Разделение переменных по способу обращения к памяти
	VariablesList ConvertParam;		/// Параметры конвертации convertParam
};

///	Классификатор переменных
class VariablesClassifier
{
private:
	/// Все переменные функций разделенные по способу обращения к памяти
	typedef std::map<ExprName, ParamCall> ClassifVar;
public:
	VariablesClassifier(TranslationUnit* aTranslationUnit);
	/// Проверка на указатель
	bool IsPointer(ReferenceExpression& referenceExpression);	
	/// Модифицируется ли глобальная переменная в процедуре
	bool IsModifiedVar(const ExprName& procName, const ExprName &exprName);
	/// Только читается переменная в процедуре
	bool IsReadOnlyVar(const ExprName& procName, const ExprName &exprName);
	/// Модифицируется ли локальная переменная в процедуре
	bool IsModifiedLoc(const ExprName& procName, size_t iNum);
	/// Только читается локальная переменная в процедуре
	bool IsReadOnlyLoc(const ExprName& procName, size_t iNum);
	/// Возвращает параметры конвертации
	VariablesList GetConvertParam(const ExprName& subProcName);	
private:
	///	Запуск классификатора
	void Build();
	/// Создает список глобальных массивов и указателей
	void SetGlobalPtr();
private:
	TranslationUnit* m_TranslationUnit;	/// Транслируемый модуль
	ClassifVar m_ClassifVar;			/// Map параметров доступа
	VariablesList m_GlobalPtr;			/// Список глобальных указателей
};

/// Класификатор
class ClassifierWalker: public OPS::Reprise::Service::DeepWalker
{
public:
	ClassifierWalker(ParamCall* pParamCall, VariablesList &aGlobalPtr): m_ParamCall(pParamCall), m_GlobalPtr(aGlobalPtr) {}
	void visit(OPS::Reprise::BlockStatement& blockStatement) { DeepWalker::visit(blockStatement); }
	void visit(ReferenceExpression& referenceExpression);
	void visit(BasicCallExpression& basicCallExpr);
private:
	void SetTypeMod(ReferenceExpression& referenceExpression, bool isReadOnly);
	bool IsGlobalPtr(const ExprName& exprName);
	bool IsPointer(ReferenceExpression& referenceExpression);
	ParamCall* m_ParamCall;
	VariablesList m_GlobalPtr;
};

} // end namespace AliasAnalysis
} // end namespace OPS


#endif
