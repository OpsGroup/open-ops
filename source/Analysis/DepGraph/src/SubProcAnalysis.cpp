#include "Analysis/DepGraph/SubProcAnalysis.h"
#include "Analysis/DepGraph/DepGraph.h"
#include "Reprise/Service/Marker.h"
#include "Shared/NodesCollector.h"

#include "OPS_Core/msc_leakcheck.h"

namespace DepGraph
{
	using namespace OPS::Reprise;
	using namespace OPS::Reprise::Service;
	using namespace Id;

	const std::string SUB_PROC_INFO_NOTE = "DepGraph::SubProcInfo";

	SubroutineMarker subProcInfoMarker;

	SubProcInfo::SubProcInfo(const SubroutineDeclaration& subProc)
		:m_sideEffects(SE_WRITE_PARAM | SE_GLOBAL_VAR | SE_CALL)
	{
		const SubroutineType& proto = subProc.getType();
		
		for (int i=0; i<proto.getParameterCount(); ++i)
		{
			const ParameterDescriptor* itParam = &proto.getParameter(i);
			OPS::Reprise::TypeBase* temp = const_cast<OPS::Reprise::TypeBase*>(&itParam->getType());
			GetTypeVisitor visitor;
			temp->accept(visitor);
			// Переменные передаваемые по значению могут модифицироваться
			// и читаться без побочных эффектов.
			if (visitor.m_typeOfNode != GetTypeVisitor::NK_BaseBasicType)
			{
				m_paramAccessList[itParam] = AT_READ | AT_WRITE;
			}
		}
	}
										// =============Изменить!===============
	SubProcInfo* getSubProcInfoNote(SubroutineDeclaration& subProc)
	{		
		if (subProcInfoMarker.hasMarks(subProc))
		{
			return subProcInfoMarker.getMark(subProc);
		}
		else 
			return 0;
		
	}

	// TODO: переместить в shared
	bool isLocalVariable(const VariableDeclaration& data, SubroutineDeclaration& subProc)
	{
		RepriseBase* parent = data.getParent();
		while(parent != 0)
		{
			if (parent == &subProc.getDeclarations())
				return true;

			parent = parent->getParent();
		}
		return false;
	}

	bool isGlobalVar(const VariableDeclaration& data, SubroutineDeclaration& subProc)
	{
		return !isLocalVariable(data, subProc);
	}

	bool hasGlobalVarUsage(SubroutineDeclaration& subProc, OccurList& occurs)
	{
		OccurIter	it = occurs.Begin(),
					itEnd = occurs.End();

		for(; it != itEnd; ++it) {
			if (isGlobalVar(*(*it)->m_varDecl, subProc))
				return true;
		}
		return false;
	}

	bool hasGlobalVarUsage(SubroutineDeclaration& subProc, IndOccurContainer&	occurs)
	{
		return hasGlobalVarUsage(subProc, occurs.GetIndGenList()) ||
			   hasGlobalVarUsage(subProc, occurs.GetIndUtilList());
	}

	/// Определяет, вызывает ли функция другие функции с побочными эффектами
	bool hasNonPureCalls(SubroutineDeclaration& subProc)
	{
		std::vector<SubroutineCallExpression*> calls = OPS::Shared::collectNodes<SubroutineCallExpression>(subProc.getBodyBlock());

		for(size_t i = 0; i < calls.size(); ++i)
		{
			if (!calls[i]->hasExplicitSubroutineDeclaration())
			{
				// нет явного определения функции - не можем точно сказать, вызывает или нет
				return true;
			}

			SubProcInfo procInfo = getSubProcInfo(calls[i]->getExplicitSubroutineDeclaration());
			if (procInfo.m_sideEffects != 0)
			{
				// есть побочные эффекты
				return true;
			}
		}
		return false;
	}

	SubProcInfo* createSubProcInfo(SubroutineDeclaration& subProc)
	{
		SubProcInfo* info = new SubProcInfo(subProc);

		subProcInfoMarker.addMark(&subProc,info);
										
		// Для прототипов оставляем все как есть - худший вариант
		if (!subProc.hasImplementation())
			return info;

		// Строим контейнер вхождений переменных
		BlockStatement& procBody = subProc.getBodyBlock();
		id	index(procBody);
		IndOccurContainer	occurs;
		BuildVO(index, occurs);

		bool hasParamGens = false;

		for (Declarations::VarIterator iter = procBody.getDeclarations().getFirstVar(); iter.isValid(); ++iter)
		{
			VariableDeclaration& itVar = *iter;
			if (!itVar.hasParameterReference())
			{
				continue;
			}
			const ParameterDescriptor& param = itVar.getParameterReference();
			if (occurs.GetNextUnexaminedVarGenNew(occurs.GetIndGenList().Begin(), &itVar) 
				== occurs.GetIndGenList().End()) {
				// Нет генераторов этого параметра - убираем флаг записи
				info->m_paramAccessList[&(param)] &= ~SubProcInfo::AT_WRITE;
			}
			else {
				hasParamGens = true;
			}

			// Ищем использования для itVar
			if (occurs.GetNextUnexaminedVarUtilNew(occurs.GetIndUtilList().Begin(), &itVar) 
				== occurs.GetIndUtilList().End()) {
				// Нет использований этого параметра - убираем флаг чтения
				info->m_paramAccessList[&(param)] &= ~SubProcInfo::AT_READ;
			}
		}

		// Проверяем присутствие глобальных переменных
		if (!hasGlobalVarUsage(subProc, occurs)) {
			info->m_sideEffects &= ~SubProcInfo::SE_GLOBAL_VAR;
		}

		// Если нет генераторов входных параметров, то сбрасываем флаг
		if (hasParamGens == false) {
			info->m_sideEffects &= ~SubProcInfo::SE_WRITE_PARAM;
		}

		if (!hasNonPureCalls(subProc)) {
			info->m_sideEffects &= ~SubProcInfo::SE_CALL;
		}

		return info;
	}

	SubProcInfo getSubProcInfo(SubroutineDeclaration& subProc, bool forceUpdate /* = false*/ )
	{
		SubProcInfo* info = getSubProcInfoNote(subProc);
		if (info == 0 || forceUpdate) {
			info = createSubProcInfo(subProc);
		}
		return *info;
	}
}
