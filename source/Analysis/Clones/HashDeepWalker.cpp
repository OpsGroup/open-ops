#include"Analysis/Clones/HashDeepWalker.h"

void HashDeepWalker::visit(ProgramUnit& pu)
{
	processNode([&]() {
		DeepWalker::visit(pu);
		}, &pu);
}
void HashDeepWalker::visit(TranslationUnit& tu)
{
	processNode([&]() {
		DeepWalker::visit(tu);
		}, &tu);
}

//void visit(ProgramFragment& pf) 
//{
//	//processNode([&]() {
//	//	cout << string(" ") << "It is subroutinedecl" << endl;
//	//	DeepWalker::visit(pf);
//	//	}, &pf);
//}

void HashDeepWalker::visit(Declarations& dec)
{
	processNode([&]() {
		h += hash<int>{}(dec.getChildCount()) << 1;
		DeepWalker::visit(dec);
		}, &dec);
}

void HashDeepWalker::visit(VariableDeclaration& vd)
{
	processNode([&]() {
		h += hash<int>{}(vd.getKind()) << 1;
		//vd.getName()
		// vd.getType(); implement hasher for TypeBase
		DeepWalker::visit(vd);
		}, &vd);
}

void HashDeepWalker::visit(TypeDeclaration& td)
{
	processNode([&]() {
		h += hash<int>{}(td.getKind()) << 1;
		td.getKind();
		DeepWalker::visit(td);
		}, &td);
}

void HashDeepWalker::visit(SubroutineDeclaration& sd)
{
	processNode([&]() {
		auto& type = sd.getType();
		//h += hash<string>{}(type.getReturnType()); implement hasher for TypeBase
		//h += hash<string>{}(sd.getName()) << 1;
		DeepWalker::visit(sd);
		}, &sd);
}

//Statements
void HashDeepWalker::visit(BlockStatement& bs) {
	processNode([&]() {
		h += hash<int>{}(bs.getChildCount()) << 1;
		DeepWalker::visit(bs);
		}, &bs);
}
void HashDeepWalker::visit(ForStatement& fs)
{
	processNode([&]() {
		h += hash<string>{}(typeid(fs).name()) << 1;
		DeepWalker::visit(fs);
		}, &fs);
}
void HashDeepWalker::visit(WhileStatement& ws)
{
	processNode([&]() {
		h += hash<string>{}(typeid(ws).name()) << 1;
		DeepWalker::visit(ws);
		}, &ws);
}
void HashDeepWalker::visit(IfStatement& ifs)
{
	processNode([&]() {
		h += hash<string>{}(typeid(ifs).name()) << 1;
		DeepWalker::visit(ifs);
		}, &ifs);
}
void HashDeepWalker::visit(PlainCaseLabel& pcl)
{
	processNode([&]() {
		h += hash<sqword>{}(pcl.getValue()) << 1;
		DeepWalker::visit(pcl);
		}, &pcl);
}
void HashDeepWalker::visit(PlainSwitchStatement& pss)
{
	processNode([&]() {
		h += hash<string>{}(typeid(pss).name()) << 1;
		DeepWalker::visit(pss);
		}, &pss);
}
void HashDeepWalker::visit(GotoStatement& gs)
{
	processNode([&]() {
		h += hash<string>{}(typeid(gs).name()) << 1;
		DeepWalker::visit(gs);
		}, &gs);
}
void HashDeepWalker::visit(ReturnStatement& rs)
{
	processNode([&]() {
		h += hash<string>{}(typeid(rs).name()) << 1;
		DeepWalker::visit(rs);
		}, &rs);
}
void HashDeepWalker::visit(ExpressionStatement& es)
{
	processNode([&]() {
		h += hash<int>{}(es.getChildCount()) << 1;
		DeepWalker::visit(es);
		}, &es);
}
void HashDeepWalker::visit(ASMStatement& as)
{
	processNode([&]() {
		h += hash<string>{}(as.getASMString()) << 1;
		DeepWalker::visit(as);
		}, &as);
}
void HashDeepWalker::visit(EmptyStatement& es)
{
	processNode([&]() {
		h += hash<string>{}(typeid(es).name()) << 1;
		DeepWalker::visit(es);
		}, &es);
}

//Types
void HashDeepWalker::visit(BasicType& bt)
{
	processNode([&]() {
		h += hash<int>{}(bt.getKind()) << 1;
		h += hash<int>{}(bt.getSizeOf()) << 1;
		DeepWalker::visit(bt);
		}, &bt);
}
void HashDeepWalker::visit(PtrType& pt)
{
	processNode([&]() {
		h += hash<string>{}(typeid(pt).name()) << 1;
		DeepWalker::visit(pt);
		}, &pt);
}
void HashDeepWalker::visit(TypedefType& tdt)
{
	processNode([&]() {
		h += hash<string>{}(typeid(tdt).name()) << 1;
		DeepWalker::visit(tdt);
		}, &tdt);
}
void HashDeepWalker::visit(ArrayType& at)
{
	processNode([&]() {
		h += hash<string>{}(typeid(at).name()) << 1;
		h += hash<int>{}(at.getElementCount()) << 1;
		DeepWalker::visit(at);
		}, &at);
}
void HashDeepWalker::visit(StructMemberDescriptor& structMember)
{
	processNode([&]() {
		h += hash<string>{}(typeid(structMember).name()) << 1;
		h += hash<int>{}(structMember.getBitsLimit()) << 1;
		//h += hash<string>{}(structMember.getName()) << 1;
		DeepWalker::visit(structMember);
		}, &structMember);
}
void HashDeepWalker::visit(StructType& st)
{
	processNode([&]() {
		h += hash<string>{}(typeid(st).name()) << 1;
		h += hash<int>{}(st.getMemberCount()) << 1;
		DeepWalker::visit(st);
		}, &st);
}
void HashDeepWalker::visit(EnumMemberDescriptor& emdt)
{
	processNode([&]() {
		h += hash<string>{}(typeid(emdt).name()) << 1;
		h += hash<int>{}(emdt.getValue()) << 1;
		DeepWalker::visit(emdt);
		}, &emdt);
}
void HashDeepWalker::visit(EnumType& et)
{
	processNode([&]() {
		h += hash<string>{}(typeid(et).name()) << 1;
		h += hash<int>{}(et.getMemberCount()) << 1;
		DeepWalker::visit(et);
		}, &et);
}
void HashDeepWalker::visit(ParameterDescriptor& pd)
{
	processNode([&]() {
		h += hash<string>{}(typeid(pd).name()) << 1;
		h += hash<int>{}(pd.getTransitKind()) << 1;
		DeepWalker::visit(pd);
		}, &pd);
}
void HashDeepWalker::visit(SubroutineType& st)
{
	processNode([&]() {
		h += hash<string>{}(typeid(st).name()) << 1;
		h += hash<int>{}(st.getCallingKind()) << 1;
		h += hash<int>{}(st.getParameterCount()) << 1;
		DeepWalker::visit(st);
		}, &st);
}
void HashDeepWalker::visit(DeclaredType& dt)
{
	processNode([&]() {
		h += hash<string>{}(typeid(dt).name()) << 1;
		DeepWalker::visit(dt);
		}, &dt);
}
void HashDeepWalker::visit(VectorType& vt)
{
	processNode([&]() {
		h += hash<string>{}(typeid(vt).name()) << 1;
		h += hash<int>{}(vt.getElementCount()) << 1;
		DeepWalker::visit(vt);
		}, &vt);
}

//Expressions
void HashDeepWalker::visit(BasicLiteralExpression& ble)
{
	processNode([&]() {
		h += hash<int>{}(ble.getLiteralType()) << 1;
		h += hash<int>{}(ble.getChildCount()) << 1;
		DeepWalker::visit(ble);
		}, &ble);
}
void HashDeepWalker::visit(StrictLiteralExpression& sle
)
{
	processNode([&]() {
		h += hash<int>{}(sle.getLiteralType()) << 1;
		h += hash<int>{}(sle.getChildCount()) << 1;
		DeepWalker::visit(sle);
		}, &sle);
}
void HashDeepWalker::visit(CompoundLiteralExpression& cle)
{
	processNode([&]() {
		h += hash<int>{}(cle.getChildCount()) << 1;
		DeepWalker::visit(cle);
		}, &cle);
}
void HashDeepWalker::visit(ReferenceExpression& re)
{
	processNode([&]() {
		h += hash<string>{}(typeid(re).name()) << 1;
		h += hash<int>{}(re.getChildCount()) << 1;
		DeepWalker::visit(re);
		}, &re);
}
void HashDeepWalker::visit(SubroutineReferenceExpression& sre)
{
	processNode([&]() {
		h += hash<string>{}(typeid(sre).name()) << 1;
		h += hash<int>{}(sre.getChildCount()) << 1;
		DeepWalker::visit(sre);
		}, &sre);
}
void HashDeepWalker::visit(StructAccessExpression& sae)
{
	processNode([&]() {
		h += hash<string>{}(typeid(sae).name()) << 1;
		h += hash<int>{}(sae.getChildCount()) << 1;
		DeepWalker::visit(sae);
		}, &sae);
}
void HashDeepWalker::visit(EnumAccessExpression& eae)
{
	processNode([&]() {
		h += hash<string>{}(typeid(eae).name()) << 1;
		h += hash<int>{}(eae.getChildCount()) << 1;
		DeepWalker::visit(eae);
		}, &eae);
}
void HashDeepWalker::visit(TypeCastExpression& tce)
{
	processNode([&]() {
		h += hash<string>{}(typeid(tce).name()) << 1;
		h += hash<int>{}(tce.getChildCount()) << 1;
		DeepWalker::visit(tce);
		}, &tce);
}
void HashDeepWalker::visit(BasicCallExpression& bce)
{
	processNode([&]() {
		h += hash<string>{}(typeid(bce).name()) << 1;
		h += hash<int>{}(bce.getKind()) << 1;
		h += hash<int>{}(bce.getChildCount()) << 1;
		DeepWalker::visit(bce);
		}, &bce);
}
void HashDeepWalker::visit(SubroutineCallExpression& sce)
{
	processNode([&]() {
		h += hash<string>{}(typeid(sce).name()) << 1;
		h += hash<int>{}(sce.getArgumentCount()) << 1;
		DeepWalker::visit(sce);
		}, &sce);
}
void HashDeepWalker::visit(EmptyExpression& ee)
{
	processNode([&]() {
		DeepWalker::visit(ee);
		}, &ee);
}

map<size_t, vector<shared_ptr<HashDeepWalker::SubTreeInfo>>> HashDeepWalker::getBuckets(int MassThreshold)
{
	map<size_t, vector<shared_ptr<HashDeepWalker::SubTreeInfo>>> buckets = map<size_t, vector<shared_ptr<HashDeepWalker::SubTreeInfo>>>();

	for (int i = 0; i < nodes.size(); i++)
	{
		if (nodes[i]->subTreeSize > MassThreshold)
		{
			if (buckets.find(nodes[i]->hashCode) == buckets.end())
			{
				buckets[nodes[i]->hashCode] = vector< shared_ptr<HashDeepWalker::SubTreeInfo>>();
			}
			buckets[nodes[i]->hashCode].push_back(nodes[i]);
		}
	}

	return buckets;
}