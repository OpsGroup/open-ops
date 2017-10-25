#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Transforms/Scalar/SSASubstitutionForward/SSASubstitutionForward.h"
#include "Reprise/Service/DeepWalker.h"
namespace OPS
{
	namespace Transforms
	{
		namespace Scalar
		{
			using namespace Reprise;
			using OPS::Analysis::SSAForms::SSASubscript;

			typedef std::list<ReferenceExpression*> ReplaceList;

			class ReplaceFinder: public OPS::Reprise::Service::DeepWalker
			{
				SSAForm* mSSA;
				const SSASubscript* mSubscript;

			public:
				ReplaceList mList;

				ReplaceFinder(SSAForm& SSA, const SSASubscript* Subscript):mSSA(&SSA), mSubscript(Subscript)
				{

				}


				virtual void visit(ReferenceExpression& visited_node)
				{
					if( (mSSA->getSubscript(&visited_node)) && (mSSA->getSubscript(&visited_node)->equals(*mSubscript )))
					{
						mList.push_back(&visited_node);
					}
					DeepWalker::visit(visited_node);
				}
			};


			ReplaceList findUses(SSAForm& ssa, const SSASubscript* version, ExpressionBase& toWalk)
			{
				ReplaceFinder finder(ssa, version);
				toWalk.accept(finder);				
				return finder.mList;

			}

			class CanSubstituteWalker: public OPS::Reprise::Service::DeepWalker
			{
				SSAForm* mSSA;

			public:
				bool canSubstitute;

				CanSubstituteWalker(SSAForm& SSA):mSSA(&SSA)
				{
					canSubstitute = true;
				}


				virtual void visit(ReferenceExpression& visited_node)
				{
					if(! mSSA->getSubscript(&visited_node) )
					{
						canSubstitute = false;
					}					
				}
				virtual void visit(SubroutineCallExpression& visited_node)
				{
					canSubstitute = false;
				}
				virtual void visit(BasicCallExpression& visited_node)
				{
					if(visited_node.getKind() == BasicCallExpression::BCK_ASSIGN)
						canSubstitute = false;
				}
			};


			bool canSubstituteLeftPart(SSAForm& ssa, BasicCallExpression& generator)
			{
				OPS_ASSERT(generator.getKind() == BasicCallExpression::BCK_ASSIGN)
				CanSubstituteWalker walker(ssa);

				generator.getChild(1).accept(walker);
				return walker.canSubstitute;
			}




			


			VariableDeclaration* prepareVariable(VariableDeclaration& use, BasicCallExpression& anchor)
			{
				VariableDeclaration& var_ref = use;
				VariableDeclaration* new_var = &OPS::Reprise::Editing::createNewVariable(var_ref.getType(), var_ref.getDefinedBlock(), generateUniqueIndentifier(var_ref.getName()));;

				ExpressionBase* new_assignement = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, new ReferenceExpression(*new_var), new ReferenceExpression(var_ref) );
				ExpressionStatement* new_assign_statement = new ExpressionStatement(new_assignement);
				StatementBase* anchor_stmt= anchor.obtainParentStatement();
				OPS_ASSERT(anchor_stmt);
				BlockStatement* anchor_blk = &anchor_stmt->getParentBlock();
				anchor_blk->addBefore( anchor_blk->convertToIterator(anchor_stmt), new_assign_statement);

				return new_var;
			}

			bool cannotPrepare(BasicCallExpression& generator)
			{
				return !generator.getParent()->is_a<ExpressionStatement>();

			}

			typedef std::map<VariableDeclaration*, VariableDeclaration*> ReplaceMap;

			class ReplaceWalker: public OPS::Reprise::Service::DeepWalker
			{
				ReplaceMap* mMap;


			public:				

				ReplaceWalker(ReplaceMap& Map):mMap(&Map)
				{

				}


				virtual void visit(ReferenceExpression& visited_node)
				{
					if( mMap->find(&visited_node.getReference()) != mMap->end()) 
					{
						VariableDeclaration* new_decl = (*mMap)[ &visited_node.getReference()];
						visited_node.setReference(new_decl);
					}					
				}
			};

			void replaceVariables(ExpressionBase* replacingExpression, ReplaceMap& toReplace)
			{
				ReplaceWalker walker (toReplace);
				replacingExpression->accept(walker);
												
			}
			
			typedef std::map<VariableDeclaration*, int> SubscriptMap;
			class SubscriptWalker: public OPS::Reprise::Service::DeepWalker
			{
				SSAForm* mSSA;



			public:				
				SubscriptMap mMap;

				SubscriptWalker(SSAForm& SSA):mSSA(&SSA)
				{

				}


				virtual void visit(ReferenceExpression& visited_node)
				{
					if( mSSA->getSubscript(&visited_node) )
					{
						mMap[&visited_node.getReference()] = mSSA->getSubscript(&visited_node)->getUseSubscript();					
					}					
				}
			};

			typedef std::list<VariableDeclaration*> DeadDeclarationList;

			DeadDeclarationList findDeadDefinitions(SSAForm& ssa, BasicCallExpression& generator, ExpressionBase& expressionTo)
			{				
				ExpressionBase* test_expression = generator.getChild(1).cast_to<ExpressionBase>().clone();				
				ExpressionStatement* test_stmt = new ExpressionStatement(test_expression);
				BlockStatement& blk = expressionTo.obtainParentStatement()->getParentBlock();
				blk.addBefore( blk.convertToIterator(expressionTo.obtainParentStatement()), test_stmt);
				DeadDeclarationList difference;
				try {
				SSAForm test_ssa(ssa.getCFGex().rootBlock());

				SubscriptWalker subs_before(test_ssa);
				generator.getChild(1).accept(subs_before);

				SubscriptWalker subs_after(test_ssa);
				test_expression->accept(subs_after);

				


				for(SubscriptMap::iterator i = subs_before.mMap.begin(); i!= subs_before.mMap.end(); ++i)
				{
					int before_subscript = i->second;
					int after_subscript = subs_after.mMap[i->first];
					if(before_subscript != after_subscript) difference.push_back(i->first);
				}					
				

				
				}
				catch (...)
				{

					blk.erase(test_stmt);
					throw;
				}
				blk.erase(test_stmt);
				return difference;
				
			}

			ExpressionBase* prepareExpression(SSAForm& ssa, BasicCallExpression& generator, ExpressionBase& expressionTo)
			{
				OPS_ASSERT(generator.getKind() == BasicCallExpression::BCK_ASSIGN)


				DeadDeclarationList toPrepare = findDeadDefinitions(ssa, generator, expressionTo);


				if(toPrepare.size() && cannotPrepare(generator))
				{
					throw OPS::Exception("SSASubstitutionForward: cannot insert additional generators.");
				}
				
				ReplaceMap toReplace;

				for(DeadDeclarationList::iterator i = toPrepare.begin(); i!=toPrepare.end(); ++i)
				{
					if(toReplace.find( *i ) == toReplace.end())
						toReplace[*i] = prepareVariable(**i, generator);					
				}

				ExpressionBase* replacingExpression = generator.getChild(1).cast_to<ExpressionBase>().clone();

				replaceVariables(replacingExpression, toReplace);

				
				return replacingExpression;

			}

			void makeSSASubstitutionForward(SSAForm& ssa, BasicCallExpression& generator, ExpressionBase& expressionTo)
			{
				OPS_ASSERT(generator.getKind() == BasicCallExpression::BCK_ASSIGN); //это действительно генератор
				OPS_ASSERT(generator.getChild(0).is_a<ReferenceExpression>()); //скалярной переменной
				OPS_ASSERT(ssa.getSubscript(&generator.getChild(0).cast_to<ReferenceExpression>())); //положенной на SSA				
				ReplaceList to_replace = findUses(ssa, ssa.getSubscript(&generator.getChild(0).cast_to<ReferenceExpression>()), expressionTo);				
				if(!to_replace.size()) return;
				OPS_ASSERT(canSubstituteLeftPart(ssa, generator));						

				ReprisePtr<ExpressionBase> expression_what(prepareExpression(ssa, generator, expressionTo));

				for(ReplaceList::iterator i = to_replace.begin(); i!= to_replace.end(); ++i)
				{
					makeSubstitutionForward(**i, **i, expression_what);
				}			

			}
		}
	}
}
