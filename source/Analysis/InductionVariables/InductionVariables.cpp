#include "Analysis/InductionVariables/InductionVariables.h"
#include "Shared/NodesCollector.h"
#include "Shared/ParametricLinearExpressions.h"

//#include "Reprise/ServiceFunctions.h"


using namespace OPS::Analysis;
using namespace OPS::Analysis::ControlFlowGraphs::SSAControlFlowGraph;
using OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx;
using OPS::Analysis::ControlFlowGraphs::Stash;
using OPS::Shared::ParametricLinearExpression;
using namespace OPS::Analysis::ControlFlowGraphs::DominanceFrontierGraph;
using namespace OPS::Analysis::SSAForms;
using namespace OPS::Reprise;

ExpressionBase* TarjanNode::defaultLoop;

bool OPS::Analysis::LoopInductionAnalysis::isLoopHeader( ExpressionBase* loop )
{
	RepriseBase* parent = loop->getParent();
	if(parent)
	{
		if(parent->is_a<ForStatement>())		
		{
			ForStatement& loopstmt = parent->cast_to<ForStatement>();
			if(&loopstmt.getFinalExpression() == loop)
			{
				return true;
			}
		}
		else if(parent->is_a<WhileStatement>())
		{
			WhileStatement& loopstmt = parent->cast_to<WhileStatement>();
			if(&loopstmt.getCondition() == loop)
			{
				return true;
			}			
		}
	}
	return false;
}

void OPS::Analysis::LoopInductionAnalysis::VisitNodes( OPS::Reprise::ExpressionBase* loop )
{
	OPS_ASSERT(isLoopHeader(loop));

	//foreach phi after the header of the loop generator 
	//if generator is inside the loop and is unvisited, visit

	OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx& cfgex = mSSA->getCFGex();
	StatementVector* loopheader = cfgex.getBlock(Stash(loop));
	const SSAForm::PhiList phis = mSSA->getPhis(loopheader);
	for(SSAForm::PhiList::const_iterator phi=phis.begin(); phi!=phis.end(); phi++)
	{
		TarjanNode& current_node = mTarjanFactory->getNode(*phi);
		if(current_node.mStatus == TarjanNode::NS_NOTYET)
		{
			mTarjanWalker->visit(current_node);						
		}
	}
}

void OPS::Analysis::LoopInductionAnalysis::init()
{
	mNotFound = new InductionDescription(*mLoop);
	mIdealSequence = 0;
}

OPS::Analysis::LoopInductionAnalysis::LoopInductionAnalysis( OPS::Reprise::BlockStatement& blk, LoopHelper loop )
{
	mSSA.reset(new OPS::Analysis::SSAForms::SSAForm(blk));	
	mLoop = loop.getLoop();
	init();
	FindComponents(mLoop);
}

OPS::Analysis::LoopInductionAnalysis::LoopInductionAnalysis( ReprisePtr<OPS::Analysis::SSAForms::SSAForm> ssa, LoopHelper loop )
{
	mSSA = ssa;
	mLoop = loop.getLoop();
	init();
	FindComponents(mLoop);
}

OPS::Analysis::SSAForms::SSAForm& OPS::Analysis::LoopInductionAnalysis::getSSA()
{
	return *mSSA;
}

InductionDescription* OPS::Analysis::LoopInductionAnalysis::deductFromDeclaration( const OPS::Analysis::SSAForms::SSAGenerator* gen )
{	
	OPS_ASSERT(gen->getDeclaration() && gen->getDeclaration()->hasNonEmptyInitExpression() );
	const ExpressionBase* initExpression = &gen->getDeclaration()->getInitExpression();
	ExpressionBase* expr = const_cast<ExpressionBase*>(initExpression);
    if(!NodeVisitor::isInteger(expr->getResultType().get()))
	{
		return new InductionDescription(*mLoop);
	}

	std::unique_ptr<ParametricLinearExpression> linear (ParametricLinearExpression::createByAllVariables(expr));
	if(!linear.get()) return new InductionDescription(*mLoop);
	if(linear->getVariables().size() > 0)
		return new InductionDescription(*mLoop);
	if(!linear->isEvaluatable())
		return new InductionDescription(*mLoop);


	ParametricLinearExpression::Coefficient free_coeff = linear->getFreeCoefficient();
	if(!free_coeff->is_a<StrictLiteralExpression>() &&
        !NodeVisitor::isInteger(free_coeff->getResultType().get())
		) 
		return new InductionDescription(*mLoop);

	LinearIntegerInductionDescription* to_return = new LinearIntegerInductionDescription(*mLoop);
	to_return->setCoefficient(0);
	to_return->setConstantTerm(linear->getFreeCoefficientAsInteger());		
	return to_return;
}

const ExpressionBase* getLeftPart(const ExpressionBase* generator)
{
	if(generator->getParent() &&
		generator->getParent()->is_a<BasicCallExpression>())
	{
		const BasicCallExpression& bce = generator->getParent()->cast_to<BasicCallExpression>();
		if(bce.getKind() == BasicCallExpression::BCK_ASSIGN)
			return &bce.getArgument(1);
	}
	return 0;
}

InductionDescription* OPS::Analysis::LoopInductionAnalysis::deductFromExpression( const SSAGenerator* gen )
{
	OPS_ASSERT(gen->getExpression());
	const ExpressionBase* initExpression = gen->getExpression();	
	ExpressionBase* expr = const_cast<ExpressionBase*>(getLeftPart(initExpression));
    if(!expr || !NodeVisitor::isInteger(expr->getResultType().get()))
	{
		return new InductionDescription(*mLoop);
	}

	std::unique_ptr<ParametricLinearExpression> linear (ParametricLinearExpression::createByAllVariables(expr));
	if(!linear.get()) return new InductionDescription(*mLoop);

	if(!linear->isEvaluatable())
		return new InductionDescription(*mLoop);

	LinearIntegerInductionDescription to_return(*mLoop);
	ParametricLinearExpression::VariablesDeclarationsVector decl_list = linear->getVariables();


	for(ParametricLinearExpression::VariablesDeclarationsVector::iterator var = decl_list.begin();
		var != decl_list.end();
		++var)
	{
		ParametricLinearExpression::Coefficient var_coeff = linear->getCoefficient(*var);
		if(!var_coeff->is_a<StrictLiteralExpression>() ||
            !NodeVisitor::isInteger(var_coeff->getResultType().get())
			) 
			return new InductionDescription(*mLoop);
		const SSAGenerator* var_generator = deductGenerator(*var, expr);
		if(!var_generator)
			return new InductionDescription(*mLoop);
		
		if(!getInductionDescription(var_generator)->isLinearInteger())
			return new InductionDescription(*mLoop);
		
		LinearIntegerInductionDescription& deducted = *(getInductionDescription(var_generator)->getAsLinearInteger());
		long_long_t int_coeff = linear->getCoefficientAsInteger(*var);
		std::unique_ptr<InductionDescription> multiplied ( LinearIntegerInductionDescription::multiply(deducted,int_coeff) );
		if( ! multiplied->isLinearInteger())
			return new InductionDescription(*mLoop);
		std::unique_ptr<InductionDescription> added (LinearIntegerInductionDescription::sum(to_return, *multiplied->getAsLinearInteger()));
		if( !added->isLinearInteger() )
			return new InductionDescription(*mLoop);
		to_return = *added->getAsLinearInteger();			
	}

	ParametricLinearExpression::Coefficient free_coeff = linear->getFreeCoefficient();
	if( !free_coeff->is_a<StrictLiteralExpression>() &&
        !NodeVisitor::isInteger(free_coeff->getResultType().get())
		) 
		return new InductionDescription(*mLoop);

	to_return.setConstantTerm(to_return.getConstantTerm() + linear->getFreeCoefficientAsInteger());		
	return new LinearIntegerInductionDescription(to_return);
}

InductionDescription* OPS::Analysis::LoopInductionAnalysis::wrapInduction(InductionDescription* to_wrap, const SSASubscript* wrapping)
{
	OPS_ASSERT(to_wrap->getAsLinearInteger());
	OPS_ASSERT(wrapping);
	const SSAGenerator* wrapping_gen = mSSA->getSSAGenerator(wrapping);
	OPS_ASSERT(wrapping_gen);
	LinearIntegerInductionDescription* to_return = new LinearIntegerInductionDescription(*to_wrap->getAsLinearInteger());
	to_return->setWrapGenerator(*wrapping_gen);
	return to_return;
	
}

InductionDescription* OPS::Analysis::LoopInductionAnalysis::deductFromPhi( const SSAGenerator* gen )
{
	const PhiFunction* phi = gen->getPhi();
	OPS_ASSERT(phi);
	
	TarjanNode& node = mTarjanFactory->getNode(phi);
	if(node.getType() == TarjanNode::NT_PHI)
		return new InductionDescription(*mLoop);
	else if(node.getType() == TarjanNode::NT_MU)
	{
		if(phi->getUsesSubscript().size() == 2)
		{
			InductionDescription* main_description=0;
			SSASubscript* first_link = NodeVisitor::getOuterLink(*phi, &*mSSA);
			std::list<SSASubscript*> ssalinks = phi->getUsesSubscript();
			std::list<SSASubscript*>::iterator ssalink;			
			for(ssalink = ssalinks.begin();
				ssalink != ssalinks.end();
				++ssalink)
			{
				if(*ssalink != first_link)
				{
					main_description = getInductionDescription(*ssalink);
				}
			}
			OPS_ASSERT(main_description);
			if(main_description->isLinearInteger())
			{
				return wrapInduction(main_description, first_link);
			}
		}		
	}
	return new InductionDescription(*mLoop);
}


const ExpressionBase* getLoopHeader(const RepriseBase * loop)
{
	if(loop)
	{
		if(loop->is_a<ForStatement>()) 
			return &loop->cast_to<ForStatement>().getFinalExpression();
		if(loop->is_a<WhileStatement>()) 
			return &loop->cast_to<WhileStatement>().getCondition();
	}
	return 0;
}

const ExpressionBase* determineRepriseBaseLoop(const RepriseBase* base)
{
	if(!base) return 0;

	const RepriseBase* parent;

	if(base->is_a<VariableDeclaration>())
	{
		const VariableDeclaration& decl = base->cast_to<VariableDeclaration>();
		if(!decl.hasDefinedBlock()) 
			return 0;
		else 
			parent = &decl.getDefinedBlock();
	}

	parent = base->getParent();
	while (parent && 
		!( parent->is_a<ForStatement>() ||
		parent->is_a<WhileStatement>() )
		)
		parent = parent->getParent();	

	if(parent && parent->is_a<ForStatement>())
	{
		const ForStatement& stmt_for = parent->cast_to<ForStatement>();
		if(&stmt_for.getInitExpression() == base) //выражение для инициализации выполняется перед выполнением собственно цикла
			return determineRepriseBaseLoop(parent);
	}

	return getLoopHeader(parent);

}


const ExpressionBase* OPS::Analysis::TarjanNode::determineExprLoop()
{
	return determineRepriseBaseLoop(mExpr);
}

const ExpressionBase* determineStashLoop(Stash stash)
{
	if(stash.getAsExpression()) return determineRepriseBaseLoop(stash.getAsExpression());
	if(stash.getAsStatement()) return determineRepriseBaseLoop(stash.getAsStatement());
	if(stash.getAsDeclaration()) return determineRepriseBaseLoop(stash.getAsDeclaration());
	return 0;
}
const ExpressionBase* determinePhiFunctionLoop(const PhiFunction* phi)
{
	if(!phi) return 0;
	const StatementVector* defined_vector = phi->getDefinedBlock();
	OPS_ASSERT(defined_vector && defined_vector->size());
	Stash defined_begin = * (defined_vector->begin());
	return determineStashLoop(defined_begin);
}

const ExpressionBase* getParentLoop(const SSAGenerator* gen)
{
	if(gen->getPhi()) return determinePhiFunctionLoop(gen->getPhi());
	if(gen->getExpression()) return determineRepriseBaseLoop(gen->getExpression());
	if(gen->getDeclaration()) return determineRepriseBaseLoop(gen->getDeclaration());
	return 0;

}

InductionDescription* OPS::Analysis::LoopInductionAnalysis::deductInductionDescription( const OPS::Analysis::SSAForms::SSAGenerator* gen )
{
	if(getParentLoop(gen) != mLoop)
		return mNotFound;
	if(gen->getDeclaration())
		return deductFromDeclaration(gen);
	else if (gen->getExpression())
		return deductFromExpression(gen);
	else if (gen->getPhi())
		return deductFromPhi(gen);
	else
		throw OPS::Exception("Unknown ssa generator type");
}

InductionDescription* OPS::Analysis::LoopInductionAnalysis::getInductionDescription( const OPS::Analysis::SSAForms::SSAGenerator* gen )
{
	if(!mInductions.count(gen))
	{
		mInductions[gen] = deductInductionDescription(gen);		
	}
	return mInductions[gen];
}

InductionDescription* OPS::Analysis::LoopInductionAnalysis::getInductionDescription( const OPS::Analysis::SSAForms::SSASubscript* subscript )
{
	OPS_ASSERT(subscript);
	const SSAGenerator* gen = mSSA->getSSAGenerator(subscript);
	OPS_ASSERT(gen);
	return getInductionDescription(gen);
}

InductionDescription* OPS::Analysis::LoopInductionAnalysis::getInductionDescription( const OPS::Reprise::ExpressionBase* occurrence )
{
	if(mSSA->getSubscript(occurrence))
		return getInductionDescription(mSSA->getSubscript(occurrence));
	else return mNotFound;
}
void OPS::Analysis::LoopInductionAnalysis::FindComponents( OPS::Reprise::ExpressionBase* loop )
{
	mTarjanFactory.reset(new TarjanNodeFactory(loop, &*mSSA));
	mTarjanWalker.reset(new NodeVisitor(loop, &*mTarjanFactory, &*mSSA));

	VisitNodes(loop);
	mInductions = mTarjanWalker->mClassMap;
	mIdealSequence = mTarjanWalker->mIdealSequence;
}

struct GeneratorFinder:public OPS::Reprise::Service::DeepWalker
{

	SSAForm* mSSA;
	const SSAGenerator* found;
	const VariableDeclaration* mSearching;
	GeneratorFinder(SSAForm* ssa, const VariableDeclaration* searching): mSSA(ssa), mSearching(searching)
	{
		found = 0;
	}
	virtual void visit(ReferenceExpression& varRef)
	{
		if(&varRef.getReference() == mSearching)
		{
			const SSASubscript* subscript = mSSA->getSubscript(&varRef);
			if(subscript)
			{
				found = mSSA->getSSAGenerator(subscript);
			}
		}
	}
	
};


const OPS::Analysis::SSAForms::SSAGenerator* OPS::Analysis::LoopInductionAnalysis::deductGenerator( OPS::Reprise::VariableDeclaration* searching, OPS::Reprise::ExpressionBase* where_search)
{
	GeneratorFinder finder(&*mSSA, searching);
	where_search->accept(finder);
	return finder.found;
}







bool OPS::Analysis::LoopInductionAnalysis::hasIdealSequence()
{
	return mIdealSequence != 0;
}


OPS::Analysis::InductionDescription::InductionDescription(const ExpressionBase& loop): mLoop(&loop)
{	
	mWrapAround = false;
	mWrapGenerator = 0;
}

OPS::Analysis::InductionDescription::InductionDescription( const OPS::Reprise::ExpressionBase* loop ): mLoop(loop)
{
	mWrapAround = false;
	mWrapGenerator = 0;
}
LinearIntegerInductionDescription* OPS::Analysis::InductionDescription::getAsLinearInteger()
{
	return 0;
}

const LinearIntegerInductionDescription* OPS::Analysis::InductionDescription::getAsLinearInteger() const
{
	return 0;
}
bool OPS::Analysis::InductionDescription::isLinearInteger() const
{
	return getAsLinearInteger() != 0;
}

bool OPS::Analysis::InductionDescription::isWrapAround() const
{
	return mWrapAround;
}

const OPS::Analysis::SSAForms::SSAGenerator& OPS::Analysis::InductionDescription::getWrapGenerator() const 
{
	return *mWrapGenerator;
}

void OPS::Analysis::InductionDescription::setWrapGenerator(const  SSAGenerator& generator )
{
	mWrapAround = true;
	mWrapGenerator = &generator;
}

const OPS::Reprise::ExpressionBase* OPS::Analysis::InductionDescription::getWrapExpression() const 
{
	throw OPS::Exception("OPS::Analysis::InductionDescription::getWrapExpression not implemented yet");
	return 0;
}

const OPS::Reprise::ExpressionBase* OPS::Analysis::InductionDescription::getLoop() const 
{
	return mLoop;
}

bool OPS::Analysis::InductionDescription::hasWrapExpression() const
{
	return getWrapExpression() != 0;
}

bool OPS::Analysis::InductionDescription::equals( const InductionDescription* second )
{
	if(second->getLoop() != mLoop ) return false;
	if(!this->isLinearInteger()) return false;
	if(!second->isLinearInteger()) return false;
	if( this->isWrapAround() != second->isWrapAround() ) return false;
	if( this->isWrapAround() && &this->getWrapGenerator() != &second->getWrapGenerator() ) return false;

	const LinearIntegerInductionDescription* firstLinear = this->getAsLinearInteger();
	const LinearIntegerInductionDescription* secondLinear = second->getAsLinearInteger();
	
	if( (firstLinear->getCoefficient()) != (secondLinear->getCoefficient()) ) return false;
	if(firstLinear->getConstantTerm() != secondLinear->getConstantTerm() ) return false;
	return true;
}
void OPS::Analysis::LinearIntegerInductionDescription::setCoefficient( long_long_t Coefficient )
{
	mCoefficient = Coefficient;
}

void OPS::Analysis::LinearIntegerInductionDescription::setConstantTerm( long_long_t ConstantTerm )
{
	mConstantTerm = ConstantTerm;
}

LinearIntegerInductionDescription* OPS::Analysis::LinearIntegerInductionDescription::getAsLinearInteger()
{
	return this;
}

const LinearIntegerInductionDescription* OPS::Analysis::LinearIntegerInductionDescription::getAsLinearInteger() const
{
	return this;
}
OPS::Analysis::LinearIntegerInductionDescription::LinearIntegerInductionDescription(const ExpressionBase& loop): InductionDescription(loop)
{
	mCoefficient = mConstantTerm = 0;
}

long_long_t OPS::Analysis::LinearIntegerInductionDescription::getCoefficient() const
{
	return mCoefficient;
}

long_long_t OPS::Analysis::LinearIntegerInductionDescription::getConstantTerm() const 
{
	return mConstantTerm;
}

InductionDescription* OPS::Analysis::LinearIntegerInductionDescription::sum( LinearIntegerInductionDescription& first, LinearIntegerInductionDescription& second )
{		
	OPS_ASSERT(first.getLoop());
	OPS_ASSERT(first.getLoop() == second.getLoop());	
	const ExpressionBase& common_loop = *first.getLoop();
	LinearIntegerInductionDescription to_return(common_loop);

	if(first.isWrapAround() || second.isWrapAround())
	{
		return new InductionDescription(common_loop);
	}

	to_return.mCoefficient = first.mCoefficient + second.mCoefficient;
	to_return.mConstantTerm = first.mConstantTerm + second.mConstantTerm;

	return new LinearIntegerInductionDescription(to_return);
}

InductionDescription* OPS::Analysis::LinearIntegerInductionDescription::multiply( LinearIntegerInductionDescription& first, long_long_t& second )
{
	const ExpressionBase& loop = *first.getLoop();

	LinearIntegerInductionDescription to_return(loop);
	if(first.isWrapAround() )
	{
		return new InductionDescription(loop);
	}


	to_return.mCoefficient = first.mCoefficient * second;;
	to_return.mConstantTerm = first.mConstantTerm * second;

	return new LinearIntegerInductionDescription(to_return);
}
void OPS::Analysis::TarjanNode::init()
{
	mStatus = NS_NOTYET;
	mClass = NC_UNKNOWN;
}



const ExpressionBase* OPS::Analysis::TarjanNode::determinePhiLoop()
{	
	return determinePhiFunctionLoop(mPhi);
}

const ExpressionBase* OPS::Analysis::TarjanNode::determineSSALinkLoop()
{
	return determinePhiFunctionLoop(mSSALink.first);
}

OPS::Analysis::TarjanNode::TarjanNode( const ExpressionBase* Expr )
{
	mType = NT_EXPR;
	mExpr = Expr;
	init();
	mLoop = determineExprLoop();
}

bool isMuOfLoop(const PhiFunction* Phi, const ExpressionBase* loop, SSAForm* SSA)
{	
	ControlFlowGraphEx& cfgex = SSA->getCFGex();
	ExpressionBase* nonconst = const_cast<ExpressionBase*> (loop);
	StatementVector* loopheader = cfgex.getBlock(Stash(nonconst));
	const SSAForm::PhiList phis = SSA->getPhis(loopheader);
	return ( std::find(phis.begin(), phis.end(), Phi) != phis.end() );
}

OPS::Analysis::TarjanNode::TarjanNode( const PhiFunction* Phi, bool isMu )
{

	if(isMu)	
		mType = NT_MU;
	else 
		mType = NT_PHI;
	mPhi = Phi;
	init();
	mLoop = determinePhiLoop();
}

OPS::Analysis::TarjanNode::TarjanNode( const PhiFunction* Phi, const SSASubscript* Subscript )
{
	mType = NT_SSALINK;
	mSSALink = PhiSSALink(Phi, Subscript);
	init();
	mLoop = determineSSALinkLoop();
}

TarjanNode& OPS::Analysis::TarjanNodeFactory::getNode( const OPS::Reprise::ExpressionBase* Expr )
{
	if(!mExpressionMap.count(Expr))
	{
		mExpressionMap[Expr] = new TarjanNode(Expr);
	}

	return *mExpressionMap[Expr];
}

TarjanNode& OPS::Analysis::TarjanNodeFactory::getNode( const OPS::Analysis::SSAForms::PhiFunction* Phi )
{
	if(!mPhiMap.count(Phi))
	{
		mPhiMap[Phi] = new TarjanNode(Phi, isMuOfLoop(Phi, mLoop, mSSA));
	}

	return *mPhiMap[Phi];
}

TarjanNode& OPS::Analysis::TarjanNodeFactory::getNode( const OPS::Analysis::SSAForms::PhiFunction* Phi, const SSASubscript* PhiArg )
{
	if(!mSSALinkMap.count(PhiSSALink(Phi, PhiArg)))
	{
		mSSALinkMap[PhiSSALink(Phi, PhiArg)] = new TarjanNode(Phi, PhiArg);
	}

	return *mSSALinkMap[PhiSSALink(Phi, PhiArg)];
}

OPS::Analysis::TarjanNodeFactory::TarjanNodeFactory( ExpressionBase* loop, SSAForm* SSA )
{
	mLoop = loop;
	mSSA = SSA;
	TarjanNode::defaultLoop = loop;
}

OPS::Analysis::NodeVisitor::NodeVisitor( ExpressionBase* loop, TarjanNodeFactory* TarjanFactory, SSAForm* SSA )
{
	mLoop = loop;
	mTarjanFactory = TarjanFactory;
	mSSA = SSA;
	mNumber = 0;
	mIdealSequence = 0;
}

int OPS::Analysis::NodeVisitor::min( int m1, int m2 )
{
	if(m1<m2) return m1;
	return m2;
}

const ExpressionBase* getParentLoop( const OPS::Reprise::ExpressionBase* inner_loop )
{
	RepriseBase* parent = inner_loop->getParent();
	if(!parent) return 0;
	else return determineRepriseBaseLoop(parent);
}

bool OPS::Analysis::NodeVisitor::contains( const OPS::Reprise::ExpressionBase* outer_loop, const OPS::Reprise::ExpressionBase* inner_loop )
{
	if(outer_loop == inner_loop) return false;
	if(!outer_loop) return true;
	if(!inner_loop) return false;
	
	while(inner_loop)
	{
		inner_loop = getParentLoop(inner_loop);
		if(inner_loop == outer_loop) 
			return true;
	}


	return false;
}

void OPS::Analysis::NodeVisitor::processDescendent( TarjanNode& child, int& mLow )
{
	if(contains(child.mLoop, mLoop)) 
		mLow = min(mLow, mNumber);
	else if (child.mStatus == TarjanNode::NS_ONSTACK)
	{
		mLow = min(mLow, child.mLowlink);
	}
	else if(child.mStatus == TarjanNode::NS_DONE)
	{
		mLow = min(mLow, mNumber);
	}
	else if (child.mStatus == TarjanNode::NS_NOTYET)
	{
		child.accept(*this);
		mLow = min(mLow, child.mLowlink);
	}
}

void OPS::Analysis::NodeVisitor::visitPhiDescendents( TarjanNode& visited_node, int& mLow )
{
	std::list<SSASubscript*> usesList = visited_node.mPhi->getUsesSubscript();
	std::list<SSASubscript*>::iterator use;				
	for(use = usesList.begin(); 
		use != usesList.end(); 
		++use)
	{
		TarjanNode& child = mTarjanFactory->getNode(visited_node.mPhi, *use);
		processDescendent(child, mLow);

	}
}

void OPS::Analysis::NodeVisitor::visitSSAGenerator( const SSAGenerator* ssalink, int& mLow )
{
	TarjanNode* child=0;
	if(ssalink->getType() == SSAGenerator::SSAGT_PHI)
	{
		const PhiFunction* philink = ssalink->getPhi();
		child = &mTarjanFactory->getNode(philink);
	}
	else if (ssalink->getType() == SSAGenerator::SSAGT_DECLARATION)
	{
		const ExpressionBase* decl_link = &ssalink->getDeclaration()->getInitExpression();
		child = & mTarjanFactory->getNode(decl_link);
	}
	else if (ssalink->getType() == SSAGenerator::SSAGT_GENERATOR)
	{
		if(ssalink->getExpression()->getParent()->is_a<BasicCallExpression>())
		{
			const BasicCallExpression* parent_generator = &ssalink->getExpression()->getParent()->cast_to<BasicCallExpression>();
			if(parent_generator->getKind() == BasicCallExpression::BCK_ASSIGN)
			{
				const ExpressionBase* gen_link = &parent_generator->getArgument(1);
				child = & mTarjanFactory->getNode(gen_link);
			}
		}						
	}
	if(child)
	{
		processDescendent( *child, mLow);
	}
}

void OPS::Analysis::NodeVisitor::visitExprDescendents( TarjanNode& visited_node, int& mLow )
{
	const ExpressionBase* expr = visited_node.mExpr;
	if(expr->is_a<ReferenceExpression>())
	{
		if(mSSA->getSubscript(expr))
		{
			const SSAGenerator* ssalink = mSSA->getSSAGenerator(expr);
			visitSSAGenerator( ssalink, mLow);
		}
	}
	for(int child_num = 0; child_num<expr->getChildCount(); child_num++)
	{
		const RepriseBase& child = (const_cast<ExpressionBase*> (expr) )->getChild(child_num);
		if(child.is_a<ExpressionBase>())
		{
			const ExpressionBase& child_expr = child.cast_to<ExpressionBase>();
			TarjanNode& child_node = mTarjanFactory->getNode(&child_expr);
			processDescendent( child_node, mLow);						
		}
	}
}

void OPS::Analysis::NodeVisitor::visitSSALinkDescendent( TarjanNode& visited_node, int& mLow )
{
	const SSASubscript* ssalink = visited_node.mSSALink.second;
	visitSSAGenerator( mSSA->getSSAGenerator(ssalink), mLow);
}

void OPS::Analysis::NodeVisitor::visitDescendents( TarjanNode& visited_node, int& mLow )
{
	if( (visited_node.mType == TarjanNode::NT_PHI) || (visited_node.mType == TarjanNode::NT_MU))
	{
		visitPhiDescendents( visited_node, mLow);
	}
	else if( visited_node.mType == TarjanNode::NT_EXPR)
	{
		visitExprDescendents( visited_node, mLow);
	}
	else if (visited_node.mType == TarjanNode::NT_SSALINK)
	{
		visitSSALinkDescendent( visited_node, mLow);
	}
}

void OPS::Analysis::NodeVisitor::classifyTrivial( TarjanNode& classified )
{
	if(classified.mType == TarjanNode::NT_EXPR)
	{
		const ExpressionBase* expr = classified.mExpr;
		//todo: assert
		if(expr->is_a<LiteralExpression>())
		{
			classified.mClass = TarjanNode::NC_INV;
		} 
		else if(expr->is_a<ReferenceExpression>())
		{
			//инвариант
			//индуктивная переменная
			//черти-что
		}
	}
	else
	{
		classified.mClass = TarjanNode::NC_UNKNOWN;
	}
}

void OPS::Analysis::NodeVisitor::unclassifySequence( const SSAGenerator* classified )
{
	mClassMap[classified] = new InductionDescription(*mLoop);
}

bool OPS::Analysis::NodeVisitor::isInteger( const TypeBase* intType )
{
	if(!intType) return false;

	if(intType->is_a<BasicType>())
	{
		BasicType::BasicTypes typeKind = intType->cast_to<BasicType>().getKind();
		if( (typeKind >= BasicType::BT_INT8) &&
			(typeKind <= BasicType::BT_UINT64) )
			return true;

	}
	return false;
}

long_long_t OPS::Analysis::NodeVisitor::getLiteralAsInteger( const LiteralExpression& expr )
{
	if(expr.is_a<BasicLiteralExpression>())
	{
		const BasicLiteralExpression& basicLiteral = expr.cast_to<BasicLiteralExpression>();
		if(basicLiteral.getLiteralType() == BasicLiteralExpression::LT_INTEGER) 
		{
			return basicLiteral.getInteger();
		}
		else if (basicLiteral.getLiteralType() == BasicLiteralExpression::LT_UNSIGNED_INTEGER)
		{						
			return basicLiteral.getUnsignedInteger();
		}
	}
	else if(expr.is_a<StrictLiteralExpression>())
	{
		const StrictLiteralExpression& strictLiteral = expr.cast_to<StrictLiteralExpression>();
		if(strictLiteral.getLiteralType() == BasicType::BT_INT32) 
		{
			return strictLiteral.getInt32();
		}
		else 	if(strictLiteral.getLiteralType() == BasicType::BT_UINT32) 
		{
			return strictLiteral.getUInt32();
		}
		//todo: other
	}
	throw OPS::Exception("Not an integer");
}

SSASubscript* OPS::Analysis::NodeVisitor::getOuterLink( const PhiFunction& Phi, SSAForm* mSSA )
{
	DominanceFrontier& domf = mSSA->getDominanceFrontier();
	StatementVector* loopVector = const_cast<StatementVector*>(Phi.getDefinedBlock());	


	const PhiFunction::UsesSubscriptMap& ssalinks = Phi.getUsesMap();				
	for(PhiFunction::UsesSubscriptMap::const_iterator i= ssalinks.begin();
		i!= ssalinks.end();
		++i)
	{
		StatementVector* linkVector = i->second;

		if( !domf.getDoms().count(linkVector))
		{
			return 0;
		}
		StatementVectorList& PhiDominated = *domf.getDoms()[linkVector];


		if(std::find(PhiDominated.begin(), PhiDominated.end(), loopVector) == PhiDominated.end())
		{
			return i->first;
		}					
	}
	return 0;
}

bool OPS::Analysis::NodeVisitor::classifyMu( TarjanNode& first_elem, LinearIntegerInductionDescription& known_class )
{
	if(first_elem.mPhi->getUsesSubscript().size() == 2)
	{

		SSASubscript* first_link = getOuterLink(*first_elem.mPhi, &*mSSA);


		if(!isInteger(&first_link->getVariable().getType() ) )
		{						
			return false;
		}
		if( mSSA->getSSAGenerator(first_link)->getPhi() )
		{						
			return false;
		}
		else if( mSSA->getSSAGenerator(first_link)->getDeclaration() )
		{
			const VariableDeclaration* decl = mSSA->getSSAGenerator(first_link)->getDeclaration();
			if(decl->hasNonEmptyInitExpression() && decl->getInitExpression().is_a<StrictLiteralExpression>())
			{
				const StrictLiteralExpression& initExpression = decl->getInitExpression().cast_to<StrictLiteralExpression>();
                OPS_ASSERT(isInteger(initExpression.getResultType().get()));
				known_class.setConstantTerm(getLiteralAsInteger(initExpression));
				return true;

			}
			else return false;
		}
		else if( mSSA->getSSAGenerator(first_link)->getExpression())
		{
			const ReferenceExpression* expr = mSSA->getSSAGenerator(first_link)->getExpression();
			if(expr->getParent()->is_a<BasicCallExpression>())
			{
				const BasicCallExpression* assign = & (expr->getParent()->cast_to<BasicCallExpression>());
				if( (assign->getKind() == BasicCallExpression::BCK_ASSIGN) && 
					(assign->getArgument(1).is_a<StrictLiteralExpression>()) )		
				{
					const StrictLiteralExpression& initExpression = assign->getArgument(1).cast_to<StrictLiteralExpression>();
                    OPS_ASSERT(isInteger(initExpression.getResultType().get()));
					known_class.setConstantTerm(getLiteralAsInteger(initExpression));
					return true;
				}

			}
		}
	}
	return false;
}

bool OPS::Analysis::NodeVisitor::classifyExpression( TarjanNode& exprElem, long_long_t& addition )
{
	ExpressionBase* expr = const_cast<ExpressionBase*>(exprElem.mExpr);
	std::unique_ptr<ParametricLinearExpression> linear (ParametricLinearExpression::createByAllVariables(expr));
	if(!linear.get()) return false;
	if(linear->getVariables().size() != 1)
		return false;
	if(!linear->isEvaluatable())
		return false;

	VariableDeclaration* decl = * linear->getVariables().begin();
	ParametricLinearExpression::Coefficient var_coeff = linear->getCoefficient(decl);
	if(!var_coeff->is_a<StrictLiteralExpression>() ||
        !isInteger(var_coeff->getResultType().get()) ||
		(linear->getCoefficientAsInteger(decl) != 1)					   
		) 
		return false;


	ParametricLinearExpression::Coefficient free_coeff = linear->getFreeCoefficient();
	if(!free_coeff->is_a<StrictLiteralExpression>() &&
        !isInteger(free_coeff->getResultType().get())
		) 
		return false;

	addition += linear->getFreeCoefficientAsInteger();		
	return true;
}

void OPS::Analysis::NodeVisitor::visit( TarjanNode& visited_node )
{
	int mThis;
	int mLow;
	visited_node.mStatus = TarjanNode::NS_ONSTACK;
	mLow = mThis = mNumber = mNumber+1;
	visited_node.mLowlink = mLow;
	mSCCStack.push(&visited_node);
	visitDescendents(visited_node, mLow);
	visited_node.mLowlink = mLow;
	if(mLow != mThis)				
		return;
	if( (mSCCStack.top() == &visited_node) || (visited_node.mType != TarjanNode::NT_MU) )
	{
		mSCCStack.pop();
		visited_node.mStatus = TarjanNode::NS_DONE;
		classifyTrivial(visited_node);
	}
	else
	{
		SCC* current_component = new SCC();
		while( true )
		{
			OPS_ASSERT(!mSCCStack.empty());
			TarjanNode* current_node = mSCCStack.top(); mSCCStack.pop();
			current_node->mStatus = TarjanNode::NS_DONE;
			current_component->push_back(current_node);
			if(current_node == &visited_node) break;
		}

		mComponents.push_back(current_component);
		classifySequence(current_component);
	}
}

ReferenceExpression* getLvalue(ExpressionBase* expr)
{
	if(expr->getParent() &&
		expr->getParent()->is_a<BasicCallExpression>()
		)
	{
		BasicCallExpression& assign = expr->getParent()->cast_to<BasicCallExpression>();
		if(assign.getKind() == BasicCallExpression::BCK_ASSIGN)
		{
			ExpressionBase* lvalue = &assign.getArgument(0);
			if(lvalue->is_a<ReferenceExpression>())
			{
				return &lvalue->cast_to<ReferenceExpression>();
			}
		}
	}
	return 0;
};

bool OPS::Analysis::NodeVisitor::isIdealSequence(const PhiFunction* phi, const ExpressionBase* const_expr)
{
	//todo: expr in StepExpression
	ExpressionBase* expr = const_cast<ExpressionBase*> (const_expr);
	
	ReferenceExpression* lvalue = getLvalue(expr);
	if(lvalue )
	{
		if( &lvalue->getReference() != &phi->getGenSubscript()->getVariable() )
			return true;
		else
		{
			if(expr->obtainParentStatement() &&
				expr->obtainParentStatement()->is_a<ForStatement>() &&
				&expr->obtainParentStatement()->cast_to<ForStatement>().getFinalExpression() == mLoop)
			{
				const SSASubscript* final_subscript = mSSA->getSubscript(lvalue);
				if( final_subscript &&
					(&final_subscript->getVariable() == &phi->getGenSubscript()->getVariable()) &&
					(final_subscript->getGenSubscript() == phi->getGenSubscript()->getGenSubscript() + 1)
					)
				{
					return true;
				}
				else 
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

void OPS::Analysis::NodeVisitor::classifySequence( SCC* classified )
{
	LinearIntegerInductionDescription known_class(*mLoop);
	OPS_ASSERT(classified && (classified->size() >= 2) );
	OPS_ASSERT( (*classified->rbegin())->mType == TarjanNode::NT_MU);				
	TarjanNode& first_elem = **(classified->rbegin());
	const PhiFunction* phi = first_elem.mPhi;	
	const SSAGenerator* sequence_gen = mSSA->getSSAGenerator(phi->getGenSubscript());
	if(! classifyMu(first_elem, known_class) )
	{
		unclassifySequence(sequence_gen);
		return;
	}
	
	bool isIdeal = true;

	long_long_t addition = 0;
	const ExpressionBase* previousExpression = 0;

	for(SCC::reverse_iterator node = ++classified->rbegin();
		node != classified->rend();
		++node)
	{
		if( (*node)->mType == TarjanNode::NT_PHI || (*node)->mType == TarjanNode::NT_MU)
		{
			previousExpression = 0;
			unclassifySequence(sequence_gen);
			return;
		}
		else if((*node)->mType == TarjanNode::NT_EXPR)
		{
			if((*node)->mExpr->getParent() && 
				((*node)->mExpr->getParent() == previousExpression)
				)
			{
				;//do nothing						
			} 
			else if(classifyExpression(**node, addition))
			{
				isIdeal = isIdeal && isIdealSequence(phi, (*node)->mExpr);				
			}
			else 
			{
				unclassifySequence(sequence_gen);
				return;
			}
			previousExpression = (*node)->mExpr;
		}

	}
	known_class.setCoefficient(addition);
	mClassMap[sequence_gen] = new LinearIntegerInductionDescription(known_class);
	if(isIdeal) 
		mIdealSequence = sequence_gen;
}

bool OPS::Analysis::LoopInductionAnalysis::isInduction( const OPS::Analysis::SSAForms::SSAGenerator* gen )
{
	return ( getInductionDescription(gen) && getInductionDescription(gen)->isLinearInteger() );
}

bool OPS::Analysis::LoopInductionAnalysis::isInduction( const OPS::Reprise::ExpressionBase* occurrence )
{
	return ( getInductionDescription(occurrence) && getInductionDescription(occurrence)->isLinearInteger() );
}

bool OPS::Analysis::LoopInductionAnalysis::isInduction( const OPS::Analysis::SSAForms::SSASubscript* subscript )
{
	return ( getInductionDescription(subscript) && getInductionDescription(subscript)->isLinearInteger() );
}





OPS::Analysis::InductionAnalysis::InductionAnalysis( OPS::Reprise::BlockStatement& blk )
{
	mSSA.reset(new OPS::Analysis::SSAForms::SSAForm(blk));
	mNotFound.reset(new InductionDescription(0));
	collectLoops();
	buildEveryLoopInductionAnalysis();
}

OPS::Analysis::InductionAnalysis::InductionAnalysis( OPS::Reprise::ReprisePtr<OPS::Analysis::SSAForms::SSAForm> ssa )
{
	mSSA=ssa;
	mNotFound.reset(new InductionDescription(0));
	collectLoops();
	buildEveryLoopInductionAnalysis();
}
using OPS::Shared::collectNodes;

void OPS::Analysis::InductionAnalysis::collectLoops()
{
	std::vector<ForStatement*> for_nodes = collectNodes<ForStatement>(mSSA->getCFGex().rootBlock());
	std::vector<ForStatement*>::iterator for_iter = for_nodes.begin();
	for(; for_iter != for_nodes.end(); ++for_iter)
	{
		mLoops.push_back(& (*for_iter)->getFinalExpression());
	}

	std::vector<WhileStatement*> while_nodes = collectNodes<WhileStatement>(mSSA->getCFGex().rootBlock());
	std::vector<WhileStatement*>::iterator while_iter = while_nodes.begin();

	for(; while_iter != while_nodes.end(); ++while_iter)
	{
		mLoops.push_back(& (*while_iter)->getCondition());
	}
}

void OPS::Analysis::InductionAnalysis::buildEveryLoopInductionAnalysis()
{
	for(LoopList::iterator loop = mLoops.begin();
		loop != mLoops.end();
		++loop)
	{		
		mLoopAnalysis[*loop] = ReprisePtr<LoopInductionAnalysis> (new  LoopInductionAnalysis(mSSA, *loop));
	}
}

InductionDescription* OPS::Analysis::InductionAnalysis::getInductionDescription( const OPS::Reprise::ExpressionBase* occurrence )
{
	if(mSSA->getSubscript(occurrence))
		return getInductionDescription(mSSA->getSubscript(occurrence));
	else return mNotFound.get();
}

InductionDescription* OPS::Analysis::InductionAnalysis::getInductionDescription( const OPS::Analysis::SSAForms::SSAGenerator* gen )
{
	InductionDescription* to_return = mNotFound.get();
	for(LoopList::iterator loop = mLoops.begin();
		loop != mLoops.end();
		++loop)
	{		
		to_return = mLoopAnalysis[*loop]->getInductionDescription(gen);
		if(to_return && to_return->isLinearInteger()) break;
	}
	return to_return;
}

InductionDescription* OPS::Analysis::InductionAnalysis::getInductionDescription( const OPS::Analysis::SSAForms::SSASubscript* subscript )
{
	OPS_ASSERT(subscript);
	const SSAGenerator* gen = mSSA->getSSAGenerator(subscript);
	OPS_ASSERT(gen);
	return getInductionDescription(gen);
}

bool OPS::Analysis::InductionAnalysis::isInduction( const OPS::Analysis::SSAForms::SSAGenerator* gen )
{
	return ( getInductionDescription(gen) && getInductionDescription(gen)->isLinearInteger() );
}

bool OPS::Analysis::InductionAnalysis::isInduction( const OPS::Reprise::ExpressionBase* occurrence )
{
	return ( getInductionDescription(occurrence) && getInductionDescription(occurrence)->isLinearInteger() );
}

bool OPS::Analysis::InductionAnalysis::isInduction( const OPS::Analysis::SSAForms::SSASubscript* subscript )
{
	return ( getInductionDescription(subscript) && getInductionDescription(subscript)->isLinearInteger() );
}

bool OPS::Analysis::InductionAnalysis::hasIdealSequence( LoopHelper loop )
{
	return mLoopAnalysis[loop.getLoop()]->hasIdealSequence();
}

const OPS::Analysis::SSAForms::SSAGenerator* OPS::Analysis::InductionAnalysis::getIdealSequence( LoopHelper loop )
{
	return mLoopAnalysis[loop.getLoop()]->getIdealSequence();
}

OPS::Analysis::LoopHelper::LoopHelper( OPS::Reprise::ExpressionBase* loop )
{
	mLoop = loop;
}

OPS::Analysis::LoopHelper::LoopHelper( OPS::Reprise::ExpressionBase& loop )
{
	mLoop = &loop;
}

OPS::Analysis::LoopHelper::LoopHelper( OPS::Reprise::ForStatement& forStmt )
{
	mLoop = &forStmt.getFinalExpression();
}

OPS::Analysis::LoopHelper::LoopHelper( OPS::Reprise::WhileStatement& whileStmt )
{
	mLoop = &whileStmt.getCondition();
}

OPS::Reprise::ExpressionBase* OPS::Analysis::LoopHelper::getLoop()
{
	return mLoop;
}
