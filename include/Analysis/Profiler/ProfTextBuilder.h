#ifndef OPS_PROFILER_TEXT_BUILDER_H_INCLUDED__
#define OPS_PROFILER_TEXT_BUILDER_H_INCLUDED__

#include "Reprise/Statements.h"
#include "Analysis/Profiler/Profiler.h"
#include <sstream>

namespace OPS
{
namespace Profiling
{

/// Отображаемый результат профилирования
enum ShowsProfilingResult
{
	/// Время
	PRF_TIME = 0,	
	/// Размер кода
	PRF_SIZE_CODE = 1,	
	/// Размер данных
	PRF_SIZE_DATA = 2	
};

/// Обработчик выражений
class BuildResultProfiling : public OPS::BaseVisitor,
		public OPS::Visitor<OPS::Reprise::ExpressionStatement>,
		public OPS::Visitor<OPS::Reprise::BlockStatement>,    
		public OPS::Visitor<OPS::Reprise::IfStatement>,
		public OPS::Visitor<OPS::Reprise::ForStatement>,
		public OPS::Visitor<OPS::Reprise::WhileStatement>,	
		public OPS::Visitor<OPS::Reprise::ReturnStatement>,
		public OPS::NonCopyableMix
{
public:
	BuildResultProfiling(OPS::Reprise::ProgramUnit* program, Profiler* profiler, ShowsProfilingResult spr, bool isOnlyForHeaders);

	std::string GetResult() { return m_ProfResult.str(); }
private:
	void Run(OPS::Reprise::RepriseBase* repriseBase);	
	long_long_t GetResultProfiling(OPS::Reprise::RepriseBase* repriseBase);

	void visit(OPS::Reprise::ExpressionStatement&);
	void visit(OPS::Reprise::BlockStatement&);
	void visit(OPS::Reprise::IfStatement&);
	void visit(OPS::Reprise::ForStatement&);
	void visit(OPS::Reprise::WhileStatement&);
	void visit(OPS::Reprise::ReturnStatement&);

private:
	Profiler* m_Profiler;
	OPS::Reprise::ProgramUnit* m_Program;
	std::stringstream m_ProfResult;
	std::string m_Offset;
	ShowsProfilingResult m_SPR;
	bool m_IsOnlyForHeaders;
};

/// Представление результата в текстовой форме
class ProfTextBuilder
{
public:
	ProfTextBuilder(OPS::Reprise::ProgramUnit* program, Profiler* profiler, ShowsProfilingResult spr, bool isOnlyForHeaders);
	~ProfTextBuilder(void);

	std::string GetResult();
private:
	BuildResultProfiling *m_BuildResultProfiling;
};

} // end namespace Profiling
} // end namespace OPS


#endif
