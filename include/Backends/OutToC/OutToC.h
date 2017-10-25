/*
	Backends/OutToC/OutToC.h - out to C backend

*/

//  Multiple include guard start
#ifndef OPS_BACKENDS_OUTTOC_OUTTOC_H__
#define OPS_BACKENDS_OUTTOC_OUTTOC_H__

//  Standard includes
#include <string>
#include <iostream>
#include <stack>
#include <list>

//  OPS includes
#include "Reprise/Service/WalkerBase.h"

//  Local includes

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Backends
{
//  Constants and enums

//  Global classes

struct OutToCSettings
{
	OutToCSettings();

	typedef std::map<Reprise::BasicType::BasicTypes, Reprise::Canto::HirCBasicType::HirCBasicKind> BasicTypeToHirCType;

	bool hideBuiltins;
	bool useIncludes;
	bool prettyExpressions;

	std::set<OPS::Reprise::DeclarationBase*>* usedDeclarations;

	BasicTypeToHirCType typeMap;
};

class IncludeManager;

/* Вывод внутреннего представления в С */
class OutToC : public OPS::Reprise::Service::WalkerBase
{
public:
	/** Конструктор OutToC
	\param outStream - поток, куда выводить текст программы
	\param indent - размер отступа начала строки в пробелах. 0 - нет отступов
	**/
	OutToC(std::ostream& outStream, unsigned int indention = 2);
	~OutToC();

	OutToCSettings& getSettings();
	const OutToCSettings& getSettings() const;

	static std::string expressionToString(const OPS::Reprise::ExpressionBase& expr);

	static int getExpressionPriority(OPS::Reprise::ExpressionBase& expr);

	void visit(Reprise::TranslationUnit&);

	void visit(Reprise::Declarations&);
	void visit(Reprise::VariableDeclaration&);
	void visit(Reprise::TypeDeclaration&);
	void visit(Reprise::SubroutineDeclaration&);

	void visit(Reprise::BlockStatement&);
	void visit(Reprise::ForStatement&);
	void visit(Reprise::WhileStatement&);
	void visit(Reprise::PlainCaseLabel&);
	void visit(Reprise::PlainSwitchStatement& switchStmt);
	void visit(Reprise::IfStatement&);
	void visit(Reprise::GotoStatement&);
	void visit(Reprise::ReturnStatement&);
	void visit(Reprise::ExpressionStatement&);
	void visit(Reprise::ASMStatement&);
	void visit(Reprise::EmptyStatement&);

	void visit(Reprise::Canto::HirCBasicType&);
	void visit(Reprise::BasicType&);
	void visit(Reprise::PtrType&);
	void visit(Reprise::TypedefType&);
	void visit(Reprise::ArrayType&);
	void visit(Reprise::StructMemberDescriptor&);
	void visit(Reprise::StructType&);
	void visit(Reprise::EnumMemberDescriptor&);
	void visit(Reprise::EnumType&);
	void visit(Reprise::SubroutineType&);
	void visit(Reprise::DeclaredType&);
    void visit(Reprise::VectorType&);

	void visit(Reprise::BasicLiteralExpression&);
	void visit(Reprise::StrictLiteralExpression&);
	void visit(Reprise::CompoundLiteralExpression&);
	void visit(Reprise::ReferenceExpression&);
	void visit(Reprise::SubroutineReferenceExpression&);
	void visit(Reprise::StructAccessExpression&);
	void visit(Reprise::EnumAccessExpression&);
	void visit(Reprise::TypeCastExpression&);
	void visit(Reprise::BasicCallExpression&);
	void visit(Reprise::SubroutineCallExpression&);
	void visit(Reprise::EmptyExpression&);
	void visit(Reprise::Canto::HirCCallExpression&);
    void visit(Reprise::Canto::HirBreakStatement&);
    void visit(Reprise::Canto::HirContinueStatement&);
    void visit(Reprise::Canto::HirCVariableInitStatement&);

	void increaseIndent();
	void decreaseIndent();

	void newLine(const std::string& text = "");

    void printTypeOnly(Reprise::TypeBase& t);

private:
	typedef std::set<Reprise::TypeBase*> VisitedTypeSet;
	typedef std::list<Reprise::PlainCaseLabel*> CasesList;
	typedef std::map<Reprise::StatementBase*, CasesList> StmtToCaseMap;
	typedef std::set<std::string> IncludedFiles;

	OPS::Reprise::Canto::HirCBasicType::HirCBasicKind basicTypeToCType(
			OPS::Reprise::BasicType::BasicTypes basicType);

	void outSubroutineParameters(Reprise::SubroutineType& subroutineType);
	void outConstVolatile(Reprise::TypeBase& typeBase);

	unsigned int const m_indentionSize;
	std::string		m_indent;
	std::stack<std::string> m_declStack;
	StmtToCaseMap m_stmtToCase;

protected:
	std::string composeDecl(const std::string& name, Reprise::TypeBase& itemType);
	void outLabel(Reprise::StatementBase& statement, bool needStmt = false);
	const std::string& getIndent() const;
    bool exprNeedBraces(Reprise::ExpressionBase& expr);
    bool isUsed(Reprise::DeclarationBase& decl);
	
	std::ostream&	m_outStream;
	OutToCSettings	m_settings;
	IncludeManager* m_includeManager;
    IncludedFiles m_includedFiles;
};

//  Global functions

std::string outToCOneUnitWithMain(Reprise::TranslationUnit& u);

//  Exit namespace
}
}

//  Multiple include guard end
#endif 						//	OPS_BACKENDS_OUTTOC_OUTTOC_H__
