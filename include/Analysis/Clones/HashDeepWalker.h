#pragma once

#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include <OPS_Core/IO.h>
#include <Reprise/Service/DeepWalker.h>

#include<iostream>
#include<memory>

using namespace std;
using namespace OPS;
using namespace OPS::Reprise;

class HashDeepWalker : public Service::DeepWalker
{
public:
	struct SubTreeInfo {
		RepriseBase* node;
		size_t hashCode = 0;
		size_t subTreeSize=0;
		shared_ptr<SubTreeInfo> parent;
		vector<shared_ptr<SubTreeInfo>> children;
		SubTreeInfo(RepriseBase* n):node(n) {}
		SubTreeInfo(RepriseBase* n, int h, int s) : node(n), hashCode(h), subTreeSize(s),children() {}
	};

protected:
	size_t h = 0;
	int size = 0;

	shared_ptr<SubTreeInfo> currentNode;

	vector< shared_ptr<SubTreeInfo>> nodes;

	template<class Func>
	void processNode(Func f, RepriseBase* n)
	{
		size_t th = h;
		int ts = size;
		h = 0;
		size = 0;

		shared_ptr <SubTreeInfo> tp = currentNode;
		shared_ptr <SubTreeInfo> sti = make_shared<SubTreeInfo>(n);
		currentNode = sti;

		f();


		sti->hashCode = h;
		sti->subTreeSize = size;

		nodes.push_back(sti);

		if (tp != nullptr)
		{
			tp->children.push_back(sti);
		}

		size = ts + size;
		size++;
		size_t h2 = h << 1;
		h = th + h2;
		sti->parent = tp;
		currentNode = tp;
	}

public:
	void visit(ProgramUnit&);
	void visit(TranslationUnit&);

	//void visit(ProgramFragment&);

	void visit(Declarations&);
	void visit(VariableDeclaration&);
	void visit(TypeDeclaration&);
	void visit(SubroutineDeclaration&);

	void visit(BlockStatement&);
	void visit(ForStatement&);
	void visit(WhileStatement&);
	void visit(IfStatement&);
	void visit(PlainCaseLabel&);
	void visit(PlainSwitchStatement&);
	void visit(GotoStatement&);
	void visit(ReturnStatement&);
	void visit(ExpressionStatement&);
	void visit(ASMStatement&);
	void visit(EmptyStatement&);

	void visit(BasicType&);
	void visit(PtrType&);
	void visit(TypedefType&);
	void visit(ArrayType&);
	void visit(StructMemberDescriptor& structMember);
	void visit(StructType&);
	void visit(EnumMemberDescriptor&);
	void visit(EnumType&);
	void visit(ParameterDescriptor&);
	void visit(SubroutineType&);
	void visit(DeclaredType&);
	void visit(VectorType&);

	void visit(BasicLiteralExpression&);
	void visit(StrictLiteralExpression&);
	void visit(CompoundLiteralExpression&);
	void visit(ReferenceExpression&);
	void visit(SubroutineReferenceExpression&);
	void visit(StructAccessExpression&);
	void visit(EnumAccessExpression&);
	void visit(TypeCastExpression&);
	void visit(BasicCallExpression&);
	void visit(SubroutineCallExpression&);
	void visit(EmptyExpression&);

	map<size_t, vector<shared_ptr<HashDeepWalker::SubTreeInfo>>> getBuckets(int MassThreshold);

	int getSize() { return size; }
	int getHash() { return h; }
};