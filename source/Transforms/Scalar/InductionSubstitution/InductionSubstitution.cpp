#include "Transforms/Scalar/InductionSubstitution/InductionSubstitution.h"
#include "Reprise/ServiceFunctions.h"
#include "Shared/ExpressionHelpers.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{
	
	using namespace OPS::Reprise;
	using OPS::Analysis::LoopInductionAnalysis;
	using OPS::Analysis::LinearIntegerInductionDescription;
	using OPS::Analysis::InductionDescription;
	using namespace OPS::Analysis::SSAForms;
	using namespace OPS::Shared::ExpressionHelpers;


	class InductionSubstitutor: public OPS::Reprise::Service::DeepWalker
	{
	private:
		SSAForm* m_form;
		OPS::Analysis::LoopInductionAnalysis* mInduction;
		ReprisePtr<ExpressionBase> idealExpression;
		LinearIntegerInductionDescription* ideal_description;
		VariableDeclaration* ideal_var;





	public:
		ReplaceMap mUndoData;
		using DeepWalker::visit;
		InductionSubstitutor(SSAForm& form, OPS::Analysis::LoopInductionAnalysis& ind): m_form(&form), mInduction(&ind) 
		{
			InductionDescription* descr = mInduction->getInductionDescription(mInduction->getIdealSequence());		
			OPS_ASSERT(descr->isLinearInteger());
			ideal_description = descr->getAsLinearInteger();
			IntegerHelper ih(BasicType::BT_INT64);
			const VariableDeclaration* const_ideal_var = &mInduction->getIdealSequence()->getPhi()->getGenSubscript()->getVariable();
			ideal_var = const_cast<VariableDeclaration*> (const_ideal_var);

			long_long_t coeff = ideal_description->getCoefficient();
			long_long_t term = ideal_description->getConstantTerm();
			OPS_ASSERT(coeff);
			ReferenceExpression& var_reference = *(new ReferenceExpression(*ideal_var));
			if( (coeff == 1) && (term == 0)	)		
			{
				idealExpression.reset(&var_reference);
			}
			else if (term == 0)
			{
				idealExpression.reset( &( var_reference / ih(coeff) ) );
			}
			else if (coeff == 1)
			{
				idealExpression.reset( & (var_reference - ih(term)) );
			}
			else 
			{
				idealExpression.reset ( &( (var_reference - ih(term)) / ih(coeff) ) );
			}
		};
		bool isLvalue(ExpressionBase& expr)
		{
			if(expr.getParent() &&
				expr.getParent()->is_a<BasicCallExpression>() )
			{
				BasicCallExpression& bce = expr.getParent()->cast_to<BasicCallExpression>();
				if(bce.getKind() == BasicCallExpression::BCK_ASSIGN)
				{
					if(&bce.getArgument(0) == &expr)
						return true;
				}
			}
			return false;
		}

		void visit(ReferenceExpression& re)
		{
			if(isLvalue(re)) return;
			if(m_form->getSubscript(&re))
			{
				const SSASubscript* use = m_form->getSubscript(&re);

				if(m_form->getSSAGenerator(use) == mInduction->getIdealSequence())
					return; //та же самая переменная, не надо заменять
				OPS::Analysis::InductionDescription* descr = mInduction->getInductionDescription(use);
				if(descr->isWrapAround()) return;
				if(descr && descr->isLinearInteger())
				{
					OPS::Analysis::LinearIntegerInductionDescription* lindescr = descr->getAsLinearInteger();

					IntegerHelper ih(BasicType::BT_INT64);
					long_long_t coeff = lindescr->getCoefficient();
					long_long_t term = lindescr->getConstantTerm();

					long_long_t ideal_coeff = ideal_description->getCoefficient();
					long_long_t ideal_term = ideal_description->getConstantTerm();




					ExpressionBase* to_replace;

					if(ideal_coeff == coeff)
					{
						ExpressionBase* ideal_var_ref = new ReferenceExpression(*ideal_var);
						long_long_t diff = term - ideal_term;
						if(diff == 0)
						{
							to_replace = ideal_var_ref;
						}
						else
						{
							to_replace = & (*ideal_var_ref + ih(diff));
						}
					} 
					else if (coeff == 0)
					{
						to_replace = &ih(term);
					}
					else
					{
						ExpressionBase* ideal_clone = idealExpression->clone();					
						if( (coeff == 1) && (term == 0)	)		
						{
							to_replace = ideal_clone;
						}
						else if (term == 0)
						{
							to_replace =  &( ih(coeff) * (*ideal_clone)  ) ;
						}
						else if (coeff == 1)
						{
							to_replace = &(*ideal_clone + ih(term)) ;
						}
						else 
						{
							to_replace = &( ih(coeff) * (*ideal_clone) + ih(term) );
						}
					}			
					mUndoData[to_replace] = OPS::Reprise::Editing::replaceExpression(re, ReprisePtr<ExpressionBase>(to_replace));
				}
			}
		}
	};


	ReplaceMap substituteAllInductionVariables( LoopInductionAnalysis& inductions )
	{
		if(!inductions.hasIdealSequence()) return ReplaceMap();
		else 
		{
			InductionSubstitutor substitutor(inductions.getSSA(), inductions);
			inductions.getLoop()->obtainParentStatement()->accept(substitutor);	
			return substitutor.mUndoData;
		}
	}

	void undoInductionSubstitution( ReplaceMap& undoData )
	{
		for(ReplaceMap::iterator replacement = undoData.begin();
			replacement != undoData.end();
			++replacement)
		{
			OPS::Reprise::Editing::replaceExpression(*replacement->first, replacement->second);
		}
	}

} // Scalar
} // Transforms
} // OPS
