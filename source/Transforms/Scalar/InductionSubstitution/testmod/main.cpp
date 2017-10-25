#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Analysis/SSAForm/SSAForm.h"
#include "Analysis/InductionVariables/InductionVariables.h"
#include "Transforms/Scalar/InductionSubstitution/InductionSubstitution.h"
#include "Backends/OutToC/OutToC.h"
#include "Reprise/Lifetime.h"
#ifdef _MSC_VER
#pragma warning(disable: 4099)
#endif

#include <iostream>
using namespace OPS::Reprise;
using namespace OPS::Frontend;
using namespace OPS::Analysis::SSAForms;
using namespace std;


class SSAToString: public OPS::Backends::OutToC
{
private:
	SSAForm* m_form;
	std::ostream* m_out_stream;
public:
	using OutToC::visit;
	SSAToString(std::ostream& outStream, SSAForm& form):OutToC(outStream), m_form(&form),m_out_stream(&outStream) {}
	void visit(ReferenceExpression& re)
	{
		OutToC::visit(re);
		if(m_form->getSubscript(&re))
		{
			(*m_out_stream)<<"/*_";
			if(m_form->getSubscript(&re)->isGenSubscript())
			{
				(*m_out_stream)<<m_form->getSubscript(&re)->getGenSubscript();

			}
			else
			{
				(*m_out_stream)<<m_form->getSubscript(&re)->getUseSubscript();
				
			}
			(*m_out_stream)<<" */";
		}
	}
	void visit(VariableDeclaration& vd)
	{
		OutToC::visit(vd);
		if(m_form->getSubscript(&vd))
		{
			(*m_out_stream)<<"/*_";
			if(m_form->getSubscript(&vd)->isGenSubscript())
			{
				(*m_out_stream)<<m_form->getSubscript(&vd)->getGenSubscript();

			}
			else
			{
				(*m_out_stream)<<m_form->getSubscript(&vd)->getUseSubscript();

			}
			(*m_out_stream)<<" */";
		}
	}
	void visit(ExpressionStatement& sb)
	{
		OPS::Analysis::ControlFlowGraphs::SSAControlFlowGraph::StatementVectorList::iterator curblock;
		for(curblock=m_form->getCFGex().getSubblocks().begin(); curblock!=m_form->getCFGex().getSubblocks().end(); ++curblock)
		{
			if( (*curblock)->size()  && ((*curblock)->begin()->getAsStatement() == &sb) )
			{
				SSAForm::PhiList tmplist=m_form->getPhis(*curblock);
				SSAForm::PhiList::const_iterator curr_phi;				
				(*m_out_stream)<<"/*";
				for( curr_phi=tmplist.begin(); curr_phi!=tmplist.end(); ++curr_phi)
				{
					
					(*m_out_stream)<<(*curr_phi)->getGenSubscript()->getVariable().getName()<<"_"<<(*curr_phi)->getGenSubscript()->getGenSubscript();
					(*m_out_stream)<<"=phi(...); ";

				}
				(*m_out_stream)<<"*/";
			}
		}
		OutToC::visit(sb);
	}

};


class InductionToString: public OPS::Backends::OutToC
{
private:
	SSAForm* m_form;
	std::ostream* m_out_stream;
	OPS::Analysis::LoopInductionAnalysis* mInduction;
public:
	using OutToC::visit;
	InductionToString(std::ostream& outStream, SSAForm& form, OPS::Analysis::LoopInductionAnalysis& ind):OutToC(outStream), m_form(&form), m_out_stream(&outStream), mInduction(&ind) {}
	void visit(ReferenceExpression& re)
	{
		OutToC::visit(re);
		if(m_form->getSubscript(&re))
		{
			OPS::Analysis::InductionDescription* descr = mInduction->getInductionDescription(m_form->getSubscript(&re));
			if(descr && descr->isLinearInteger())
			{

				OPS::Analysis::LinearIntegerInductionDescription* lindescr = descr->getAsLinearInteger();

				(*m_out_stream)<<"/* = ";
				if(lindescr->isWrapAround()) 
					(*m_out_stream)<<"/* WrapAround,  = ";

				(*m_out_stream)<<lindescr->getCoefficient()<<"t + "<<lindescr->getConstantTerm();


				(*m_out_stream)<<" */";
			}
		}
	}



};



int main()
{

	
	std::cout<<"Testmod for SSAFrom started...\n";

	try
	{
		Frontend frontend;		
		const CompileResult& result = frontend.compileSingleFile("Samples/Induction.cpp");

		if (result.errorCount() == 0)
		{
			cout << "Compiled successful.\n";
			
			OPS::Backends::OutToC writer(std::cout);
//			writer.visit(frontend.getProgramUnit().getUnit(0));

			Declarations& decls = frontend.getProgramUnit().getUnit(0).getGlobals();
			SubroutineDeclaration& subroutine = *decls.getFirstSubr();


			BlockStatement& bodyBlock = subroutine.getBodyBlock();
 			try
 			{
				ReprisePtr<SSAForm> ssa(new SSAForm(bodyBlock));
				SSAToString tostring(std::cout, *ssa);
				writer.visit(bodyBlock);
				//tostring.visit(bodyBlock);
				WhileStatement& loop = (++bodyBlock.getFirst())->cast_to<WhileStatement>();
				//ForStatement& ploop = (++bodyBlock.getFirst())->cast_to<ForStatement>();
				//ForStatement& loop = (ploop.getBody().getFirst())->cast_to<ForStatement>();

				OPS::Analysis::LoopInductionAnalysis indan(ssa, loop);
				InductionToString indwriter(std::cout, *ssa, indan);
				indwriter.visit(bodyBlock);

				cout<<endl<<endl<<"SubstituteAllInduction"<<endl<<endl;

				OPS::Transforms::Scalar::ReplaceMap undo_data = OPS::Transforms::Scalar::substituteAllInductionVariables(indan);

				writer.visit(bodyBlock);

				OPS::Transforms::Scalar::undoInductionSubstitution(undo_data);

				cout<<endl<<endl<<"Undo SubstituteAllInduction"<<endl<<endl;

				writer.visit(bodyBlock);			
		
 			}
 			catch(OPS::Exception e)
 			{
 				cout<<"OPS exception: "<<e.getMessage()<<endl;
 			}

			


		}
		
		cout << OPS::Reprise::Coordinator::instance().getStatistics();

	}
	catch(OPS::Reprise::RepriseError e)
	{
		std::cout<<"OPS Error! " + e.getMessage() + "\n";
	}
	catch(exception e)
	{
		cout<<"Some problems:\n\t"<<e.what();
	}

	std::cout<<"\nTestmod for SSAForm finised...\n";

	system("pause");
	
	return 0;
}
