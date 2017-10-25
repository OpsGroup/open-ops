#ifndef OPS_BACKEND_REPRISEXML_REPRISE_XML_H__
#define OPS_BACKEND_REPRISEXML_REPRISE_XML_H__

#include "Reprise/Service/DeepWalker.h"
#include "OPS_Core/XmlBuilder.h"

namespace OPS
{
namespace Backends
{

class RepriseXml : public OPS::Reprise::Service::WalkerBase
{
public:
	struct Options
	{
		bool writeNCID;
		bool writeNCIDofParent;
		bool writeSourceCodeLocation;

		inline Options()
			:writeNCID(true)
			,writeNCIDofParent(false)
			,writeSourceCodeLocation(false)
		{
		}
	};

	explicit RepriseXml(OPS::XmlBuilder& builder, const Options& options = Options());

	void visit(OPS::Reprise::ProgramUnit&);
	void visit(OPS::Reprise::TranslationUnit&);


	void visit(OPS::Reprise::Declarations&);
	void visit(OPS::Reprise::VariableDeclaration&);
	void visit(OPS::Reprise::TypeDeclaration&);
	void visit(OPS::Reprise::SubroutineDeclaration&);


	void visit(OPS::Reprise::BasicType&);
	void visit(OPS::Reprise::PtrType&);
	void visit(OPS::Reprise::TypedefType&);
	void visit(OPS::Reprise::ArrayType&);
	void visit(OPS::Reprise::StructMemberDescriptor&);
	void visit(OPS::Reprise::StructType&);
	void visit(OPS::Reprise::EnumMemberDescriptor&);
	void visit(OPS::Reprise::EnumType&);
	void visit(OPS::Reprise::ParameterDescriptor&);
	void visit(OPS::Reprise::SubroutineType&);
	void visit(OPS::Reprise::DeclaredType&);
    void visit(OPS::Reprise::VectorType&);

	void visit(OPS::Reprise::Canto::HirCBasicType&);
	void visit(OPS::Reprise::Canto::HirFBasicType&);
	void visit(OPS::Reprise::Canto::HirFArrayType&);


	void visit(OPS::Reprise::BasicLiteralExpression&);
	void visit(OPS::Reprise::StrictLiteralExpression&);
	void visit(OPS::Reprise::CompoundLiteralExpression&);
	void visit(OPS::Reprise::ReferenceExpression&);
	void visit(OPS::Reprise::SubroutineReferenceExpression&);
	void visit(OPS::Reprise::StructAccessExpression&);
	void visit(OPS::Reprise::EnumAccessExpression&);
	void visit(OPS::Reprise::TypeCastExpression&);
	void visit(OPS::Reprise::BasicCallExpression&);
	void visit(OPS::Reprise::SubroutineCallExpression&);
	void visit(OPS::Reprise::EmptyExpression&);

	void visit(OPS::Reprise::Canto::HirCCallExpression&);
    void visit(OPS::Reprise::Canto::HirBreakStatement&);
    void visit(OPS::Reprise::Canto::HirContinueStatement&);
    void visit(OPS::Reprise::Canto::HirFAsteriskExpression&);
	void visit(OPS::Reprise::Canto::HirFDimensionExpression&);	
	void visit(OPS::Reprise::Canto::HirFArrayShapeExpression&);
	void visit(OPS::Reprise::Canto::HirFImpliedDoExpression&);
	void visit(OPS::Reprise::Canto::HirFArgumentPairExpression&);
	void visit(OPS::Reprise::Canto::HirFIntrinsicCallExpression&);


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
	void visit(OPS::Reprise::ASMStatement&);

private:
	typedef std::set<OPS::Reprise::TypeBase*> VisitedTypeSet;

	void writeHelper(OPS::Reprise::RepriseBase& repriseBase);
	void writeHelper(OPS::Reprise::TypeBase& typeBase);
	void writeHelper(OPS::Reprise::StatementBase& stmtBase);
	void writeCallArgumentsHelper(OPS::Reprise::CallExpressionBase& callExprBase);

	OPS::XmlBuilder* m_builder;
	Options m_options;
};


}
}

#endif		// OPS_BACKEND_REPRISEXML_REPRISE_XML_H__
