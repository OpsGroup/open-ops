#include "Reprise/Reprise.h"
#include "Reprise/Lifetime.h"
#include "Frontend/Frontend.h"
#include "Analysis/SSAForm/SSAForm.h"
#include "Backends/OutToC/OutToC.h"
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
	SSAToString(std::ostream& outStream, SSAForm& form):OutToC(outStream), m_form(&form), m_out_stream(&outStream) {}
	virtual void visit(BasicCallExpression& bce)
	{
		if(bce.getParent() && 
			bce.getParent()->is_a<StatementBase>())
			printPhis(Stash(&bce));
		OutToC::visit(bce);
	}
	virtual void visit(ReferenceExpression& re)
	{
		if(re.getParent() && 
			re.getParent()->is_a<StatementBase>())
					printPhis(Stash(&re));
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
	virtual void visit(VariableDeclaration& vd)
	{
		printPhis(Stash(&vd));
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

	void printSubscript(SSASubscript& subs)
	{
		(*m_out_stream)<<subs.getVariable().getName()<<"_";

		if(subs.isGenSubscript())
		{
			(*m_out_stream)<<subs.getGenSubscript();			
		}
		else if(subs.isUseSubscript())
		{
			(*m_out_stream)<<subs.getUseSubscript();
		}
		else if(subs.isComplexSubscript())
		{
			(*m_out_stream)<<subs.getGenSubscript()<<"&"<<subs.getUseSubscript();
		}
	}

	void printPhi(const PhiFunction& phi)
	{
		printSubscript(*phi.getGenSubscript());
		(*m_out_stream)<<"=phi( ";
		std::list<SSASubscript*> tmplist = phi.getUsesSubscript();
		std::list<SSASubscript*>::iterator arg;
		for(arg = tmplist.begin();
			arg != tmplist.end();
			++arg)
		{
			printSubscript(**arg);
			std::list<SSASubscript*>::iterator next_arg = arg; ++next_arg;
			if(next_arg != tmplist.end())				
			{
				(*m_out_stream)<<", ";
			}
		}
		(*m_out_stream)<<" );";					
	}
	
	void printPhis(Stash stmt)
	{
		StatementVector* parent_block = m_form->getCFGex().getBlock(stmt);
		if( parent_block && 
			*(parent_block->begin()) == stmt 
			)
		{
			SSAForm::PhiList tmplist=m_form->getPhis(parent_block);
			if(!tmplist.size()) return;
			SSAForm::PhiList::const_iterator curr_phi;				
			(*m_out_stream)<<"/*";
			for( curr_phi=tmplist.begin(); curr_phi!=tmplist.end(); ++curr_phi)
			{
				printPhi(**curr_phi);
			}
			(*m_out_stream)<<"*/";
		}
	}

	virtual void visit(ExpressionStatement& sb)
	{

		printPhis(Stash(&sb));
		OutToC::visit(sb);
	}

};


int main()
{

	
	std::cout<<"Testmod for SSAFrom started...\n";

	try
	{
		Frontend frontend;
		const CompileResult& result = frontend.compileSingleFile("Samples/goto continue break.cpp");

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
				SSAForm ssa(bodyBlock);
				SSAToString tostring(std::cout, ssa);

				writer.visit(bodyBlock);
				tostring.visit(bodyBlock);



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
