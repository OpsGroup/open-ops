#include "Analysis/SSAForm/VirtualRegistry.h"
#include "Reprise/Service/DeepWalker.h"

namespace OPS 
{ 
namespace Analysis
{
namespace SSAForms
{
	class VirtualRegistryHelper: public OPS::Reprise::Service::DeepWalker
	{
	private:
		VirtualRegistry* m_helped;
	public:
		VirtualRegistryHelper(VirtualRegistry& helped): m_helped(&helped)
		{

		};
		virtual void visit(BlockStatement& bs)
		{
			bs.getDeclarations().accept(*this); 	
			DeepWalker::visit(bs);		
		};

		virtual void visit(Declarations& de)
		{
			m_helped->layDeclarations(de);
		};
		virtual void visit(BasicCallExpression& bce)
		{			
			if(bce.getKind() == BasicCallExpression::BCK_TAKE_ADDRESS)
			{
				m_helped->excludeDeclaration(bce.getChild(0).cast_to<ExpressionBase>())	;	
			}
			else
			{
				DeepWalker::visit(bce);
			}

		};
	};





	const VirtualRegistry::VarSet& VirtualRegistry::getRegistredVars() const
	{
		return m_registred;
	}

	bool VirtualRegistry::isRegistred( const VariableDeclaration& vd ) const
	{
		return (m_registred.find(&vd) != m_registred.end());
	}

	bool VirtualRegistry::isRegistred( const ExpressionBase& eb ) const
	{
		if(! (eb.is_a<ReferenceExpression>()) ) return false;
		ExpressionBase* nonconst = const_cast<ExpressionBase*> (&eb); //страшный костыль!

		if (nonconst->getChild(0).is_a<VariableDeclaration>())
			return isRegistred(nonconst->getChild(0).cast_to<VariableDeclaration>());
		return false;
	}

	bool VirtualRegistry::registerVar( const VariableDeclaration& variable )
	{
		if(m_registred.find(&variable) != m_registred.end()) return false;
		m_registred.insert(&variable);
		return true;
	}

	bool VirtualRegistry::isOriginallyRegistred( const VariableDeclaration& vd ) const
	{
		return (m_originallyRegistered.find(&vd) != m_originallyRegistered.end());	
	}

	VirtualRegistry::VirtualRegistry(  ) 
	{
	}

	void VirtualRegistry::fill(const BlockStatement& block)
	{
		m_block = (&block);

		VirtualRegistryHelper vh(*this);
		BlockStatement* nonconst = const_cast<BlockStatement*> (m_block);
		nonconst->accept(vh);

	}

	const BlockStatement& VirtualRegistry::getBlock() const
	{
		return *m_block;
	}

	void VirtualRegistry::layDeclarations( const Declarations& decs )
	{
		for (Declarations::ConstVarIterator varIter = decs.getFirstVar(); varIter.isValid(); ++varIter)
		{
			if ((varIter->getType().is_a<BasicType>()) || (varIter->getType().is_a<PtrType>()))
			{
				const VariableDeclaration& vd = *varIter;
				if(!vd.getDeclarators().isAsynchronous())
					layDeclaration(*varIter);
			}
		}
	}

	void VirtualRegistry::layDeclaration( const VariableDeclaration& vd )
	{
		m_registred.insert(&vd);
		m_originallyRegistered.insert(&vd);
	}

	void VirtualRegistry::excludeDeclaration( const VariableDeclaration& vd )
	{
		m_registred.erase(&vd);
		m_originallyRegistered.erase(&vd);
	}

	void VirtualRegistry::excludeDeclaration( const ExpressionBase& eb )
	{
		OPS_ASSERT(eb.is_a<ReferenceExpression>());
		if(! (eb.is_a<ReferenceExpression>()) ) return;
		const ReferenceExpression* re = &(eb.cast_to<const ReferenceExpression>());

		excludeDeclaration(re->getReference());

	}

	VirtualRegistry::~VirtualRegistry()
	{

	}
	bool VarNameLess::operator()( const OPS::Reprise::VariableDeclaration* p1, const OPS::Reprise::VariableDeclaration* p2 ) const
	{
		return (p1->getName())<(p2->getName());
	}
}
}
}

