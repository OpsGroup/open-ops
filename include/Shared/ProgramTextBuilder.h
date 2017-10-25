#pragma once

#include "Reprise/Service/DeepWalker.h"
#include <list>

namespace OPS
{
namespace Shared
{

class IProgramOutputStream
{
public:
	virtual void write(const char* text, OPS::Reprise::RepriseBase& node) = 0;
	virtual void writeNewLine(const char* text, OPS::Reprise::RepriseBase& node) = 0;
};

class ProgramTextBuilder : public OPS::Reprise::Service::DeepWalker
{
	typedef OPS::Reprise::Service::DeepWalker baseClass;
public:

	struct Options
	{
		bool hideTypeCasts;
		bool hidePrototypes;
		bool hideVariableDeclarations;
		bool hideTypeDeclarations;
		bool hideEmptyElses;
		bool hideExternalDeclarations;
        bool emptyLineAfterSubroutine;

		Options();
		bool operator==(const Options& other) const;
	};

	explicit ProgramTextBuilder(IProgramOutputStream& stream, const Options& options = Options());

	Options& getOptions() { return m_options; }

	void visit(OPS::Reprise::ProgramUnit&);
	void visit(OPS::Reprise::VariableDeclaration&);
	void visit(OPS::Reprise::TypeDeclaration&);
	void visit(OPS::Reprise::SubroutineDeclaration&);

	void visit(OPS::Reprise::Canto::HirFBasicType&);
	void visit(OPS::Reprise::Canto::HirFArrayType&);
	void visit(OPS::Reprise::BasicType&);
	void visit(OPS::Reprise::PtrType&);
	void visit(OPS::Reprise::ArrayType&);
	void visit(OPS::Reprise::VectorType&);
	void visit(OPS::Reprise::StructType&);
	void visit(OPS::Reprise::SubroutineType&);
	void visit(OPS::Reprise::DeclaredType&);

	void visit(OPS::Reprise::BlockStatement&);
	void visit(OPS::Reprise::ForStatement&);
	void visit(OPS::Reprise::WhileStatement&);
	void visit(OPS::Reprise::PlainCaseLabel&);
	void visit(OPS::Reprise::PlainSwitchStatement&);
	void visit(OPS::Reprise::IfStatement&);
	void visit(OPS::Reprise::GotoStatement&);
	void visit(OPS::Reprise::ReturnStatement&);
	void visit(OPS::Reprise::ExpressionStatement&);
	void visit(OPS::Reprise::EmptyStatement&);

	void visit(OPS::Reprise::BasicLiteralExpression&);
	void visit(OPS::Reprise::StrictLiteralExpression&);
	void visit(OPS::Reprise::ReferenceExpression&);
	void visit(OPS::Reprise::SubroutineReferenceExpression&);
	void visit(OPS::Reprise::StructAccessExpression&);
	void visit(OPS::Reprise::EnumAccessExpression&);
	void visit(OPS::Reprise::TypeCastExpression&);
	void visit(OPS::Reprise::BasicCallExpression&);
	void visit(OPS::Reprise::SubroutineCallExpression&);
	void visit(OPS::Reprise::Canto::HirCCallExpression&);
    void visit(OPS::Reprise::Canto::HirBreakStatement&);
    void visit(OPS::Reprise::Canto::HirContinueStatement&);

	void visit(OPS::Reprise::Canto::HirFAltResultExpression&);
	void visit(OPS::Reprise::Canto::HirFAsteriskExpression&);
	void visit(OPS::Reprise::Canto::HirFDimensionExpression&);
	void visit(OPS::Reprise::Canto::HirFArrayShapeExpression&);
	void visit(OPS::Reprise::Canto::HirFArrayIndexRangeExpression&);
	void visit(OPS::Reprise::Canto::HirFImpliedDoExpression&);
	void visit(OPS::Reprise::Canto::HirFArgumentPairExpression&);
	void visit(OPS::Reprise::Canto::HirFIntrinsicCallExpression&);
/*	void visit(OPS::Reprise::Canto::HirFIntrinsicReferenceExpression&);
*/
	void increaseIndent();
	void decreaseIndent();

private:

	void outLabel(OPS::Reprise::StatementBase& stmt);
	void addBack(const char* text, OPS::Reprise::RepriseBase& node);
	void newLine(const char* text, OPS::Reprise::RepriseBase& node);

	typedef std::list<Reprise::PlainCaseLabel*> CasesList;
	typedef std::map<Reprise::StatementBase*, CasesList> StmtToCaseMap;

	Options		m_options;
	std::string	m_indention;
	IProgramOutputStream* m_stream;
	StmtToCaseMap m_stmtToCase;
};

}
}
