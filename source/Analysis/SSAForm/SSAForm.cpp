#include "Analysis/SSAForm/SSAForm.h"
#include "Reprise/Service/DeepWalker.h"


using namespace  OPS::Analysis::ControlFlowGraphs::SSAControlFlowGraph;
using namespace  OPS::Analysis::ControlFlowGraphs::DominanceFrontierGraph;

using namespace OPS;
using namespace OPS::Reprise;
using namespace OPS::Analysis::SSAForms;


class StmtVarFinder: public OPS::Reprise::Service::DeepWalker
{
private:
	VirtualRegistry* m_vreg;
	std::list<ReferenceExpression* > m_occurs;
	ReferenceExpression* m_generator;
	ExpressionBase* m_generator_container;
	VariableDeclaration* m_declaration;
public:	
	StmtVarFinder(VirtualRegistry& vreg)
	{
		m_vreg = &vreg;
		m_generator = 0;		
		m_generator_container = 0;
		m_declaration = 0;
		m_occurs.clear();
	};
	void collect(Stash* st)
	{
		m_generator = 0;		
		m_generator_container = 0;
		m_declaration = 0;
		m_occurs.clear();

		if(st->getAsExpression())
			st->getAsExpression()->accept(*this);
		else if(st->getAsStatement())
			st->getAsStatement()->accept(*this);
		else if (st->getAsDeclaration())
			st->getAsDeclaration()->accept(*this);
	}

	void visit(Reprise::VariableDeclaration& vd)
	{
		m_declaration = &vd;
		if(vd.hasNonEmptyInitExpression())
			vd.getInitExpression().accept(*this);
	}
	//TODO: множественные присваивания и разыменование
	void visit(Reprise::BasicCallExpression& bce )
	{
		if( (bce.getKind() == Reprise::BasicCallExpression::BCK_ASSIGN ) )
		{
			if(m_generator_container || m_generator || m_declaration)
			{
				throw OPS::Exception("Множественные присваивания :(");
			}
			m_generator_container = &(bce.getChild(0).cast_to<ExpressionBase>());
		}
		else if( (bce.getKind() == Reprise::BasicCallExpression::BCK_ARRAY_ACCESS))
		{
			if(m_generator_container == &bce)
			{
				m_generator_container = &(bce.getChild(0).cast_to<ExpressionBase>());
			}
		}
		DeepWalker::visit(bce);		
	}

	void visit(Reprise::ReferenceExpression& re)
	{	
		if(! m_vreg->isRegistred(re.getReference()) )
			return;
		if(m_generator_container == &re)
		{
			m_generator = &re;
		}
		else
		{
			m_occurs.push_back(&re);
		}
	}

	std::list<ReferenceExpression*>& getOccurs()
	{
		return m_occurs;
	}
	ReferenceExpression* getGenerator()
	{
		return m_generator;
	}
	VariableDeclaration* getDeclaration()
	{
		return m_declaration;
	}
};

void SSAVarMap::index(StatementVector* sv)
{
	StmtVarFinder svf(*vreg);
	std::set<const VariableDeclaration*, VarNameLess> exclude;
	for(StatementVector::iterator i = sv->begin(); i != sv->end(); i++)
	{
		svf.collect(&(*i));



		for(std::list<ReferenceExpression*>::iterator j = (svf.getOccurs().begin());
			svf.getOccurs().size() && (j != svf.getOccurs().end()) ; 
			j++)
		{
			//если используется переменная, которая, возможно, не определена
			if(defined[sv].find(& ((*j)->getReference())) == defined[sv].end())
			{
				if(exclude.find(& ((*j)->getReference())) == exclude.end())
				{
					if(unclear.find(sv) == unclear.end())
						unclear[sv];
					unclear[sv].insert(*j);
				}
			}
		}

		uses[&(*i)] = svf.getOccurs();

		gens[&(*i)] = svf.getGenerator();
		gens_declarations[&(*i)] = svf.getDeclaration();
		if(svf.getGenerator())
		{
			exclude.insert(& (svf.getGenerator()->getReference()));
		}
		if(svf.getDeclaration())
		{
			exclude.insert(svf.getDeclaration());
		}

	}
	for(StatementVectorList::iterator svli = (df->getDomTree())[sv]->begin();
		svli != (df->getDomTree())[sv]->end(); svli++)
	{
		for(std::set<const VariableDeclaration*, VarNameLess>::iterator setiter = exclude.begin();
			setiter != exclude.end();
			setiter++)
			defined[*svli][*setiter] = 1;
		for(std::map<const VariableDeclaration*, int, VarNameLess>::iterator mapiter = defined[sv].begin();
			mapiter != defined[sv].end();
			mapiter++)			
			(defined[*svli])[mapiter->first] = 1;

	}
	for(StatementVectorList::iterator svli = (cfgex->getSucc())[sv]->begin();
		svli != (cfgex->getSucc())[sv]->end(); svli++)
	{

		for(std::set<const VariableDeclaration*, VarNameLess>::iterator setiter = exclude.begin();
			setiter != exclude.end();
			setiter++)
			(part_defined[*svli][*setiter]).push_back(sv);
		for(std::map<const VariableDeclaration*, int, VarNameLess>::iterator mapiter = defined[sv].begin();
			mapiter != defined[sv].end();
			mapiter++)
			part_defined[*svli][mapiter->first].push_back(sv);



	}
	for( StatementVectorList::iterator svli = (df->getDomTree())[sv]->begin();
		svli != (df->getDomTree())[sv]->end(); svli++)
	{
		index(*svli);
	}


}


ReferenceExpression* SSAVarMap::getGen(Stash* stmt)
{
	if(gens.find(stmt) != gens.end())
	{
		return gens[stmt];
	}
	else 
	{
		throw OPS::Exception("Not analyzed Stash");
	}
};


VariableDeclaration* OPS::Analysis::SSAForms::SSAVarMap::getDeclaration( Stash* stmt )
{
	if(gens_declarations.find(stmt) != gens_declarations.end())
	{
		return gens_declarations[stmt];
	}
	else 
	{
		throw OPS::Exception("Not analyzed Stash");
	}
}



std::list<ReferenceExpression*>&  SSAVarMap::getUses(Stash* stmt)
{
	if(uses.find(stmt) != uses.end())
	{
		return uses[stmt];
	}
	else 
	{
		throw OPS::Exception("Not analyzed Stash");
	}

};
bool SSAVarMap::reindexUnclear( StatementVector* sv)
{
	bool result = false;
	std::set<const VariableDeclaration*, VarNameLess> exclude;
	if((cfgex->getPred())[sv]->size())
	{
		for( std::map<const VariableDeclaration*, std::list< StatementVector*>, VarNameLess>::iterator uncleiter = part_defined[sv].begin();
			uncleiter != part_defined[sv].end();
			uncleiter++)
			if(defined[sv].find(uncleiter->first) == defined[sv].end())
				if(uncleiter->second.size() == (cfgex->getPred())[sv]->size())
				{
					defined[sv][uncleiter->first] = 1;
					exclude.insert(uncleiter->first);   
					result = true;
				}
				if(unclear.find(sv) != unclear.end())
				{
					for(std::set<ReferenceExpression*>::iterator edatiter = unclear[sv].begin();
						edatiter != unclear[sv].end();
						edatiter++)
					{
						if(exclude.find(&(*edatiter)->getReference()) != exclude.end())
						{
							if(edatiter == unclear[sv].begin())
							{
								unclear[sv].erase(edatiter);
								edatiter = unclear[sv].begin();
							}
							else
							{
								std::set<ReferenceExpression*>::iterator edatiter2 = edatiter;
								edatiter--;
								unclear[sv].erase(edatiter2);
							}
							if(edatiter == unclear[sv].end())
							{
								if(unclear[sv].size() == 0) 
									unclear.erase(sv);
								break;
							}


						}


					}

				}


	}
	else
	{
		if(unclear.find(sv) != unclear.end()) throw OPS::Exception("Using of uninitialized variable");
	}


	for( StatementVectorList::iterator svli = (df->getDomTree())[sv]->begin();
		svli != (df->getDomTree())[sv]->end(); svli++)
	{
		for(std::set<const VariableDeclaration*, VarNameLess>::iterator setiter = exclude.begin();
			setiter != exclude.end();
			setiter++)
			defined[*svli][*setiter] = 1;
		for(std::map<const VariableDeclaration*, int, VarNameLess>::iterator mapiter = defined[sv].begin();
			mapiter != defined[sv].end();
			mapiter++)			
			(defined[*svli])[mapiter->first] = 1;

	}
	for( StatementVectorList::iterator svli = (cfgex->getSucc())[sv]->begin();
		svli != (cfgex->getSucc())[sv]->end(); svli++)
	{

		for(std::set<const VariableDeclaration*, VarNameLess>::iterator setiter = exclude.begin();
			setiter != exclude.end();
			setiter++)
			(part_defined[*svli][*setiter]).push_back(sv);
		for(std::map<const VariableDeclaration*, int, VarNameLess>::iterator mapiter = defined[sv].begin();
			mapiter != defined[sv].end();
			mapiter++)
			part_defined[*svli][mapiter->first].push_back(sv);				
	}
	for( StatementVectorList::iterator svli = (df->getDomTree())[sv]->begin();
		svli != (df->getDomTree())[sv]->end(); svli++)
	{
		result = result || reindexUnclear(*svli);
	}
	return result;



}



SSAVarMap::SSAVarMap(ControlFlowGraphEx& _cfgex, VirtualRegistry& _vr, DominanceFrontier& _df, BlockStatement* _bs)
:cfgex(&_cfgex), vreg(&_vr), df(&_df), block(_bs)
{
	std::map <const VariableDeclaration*, int, VarNameLess> init_defined;
	std::map<const VariableDeclaration*, std::list< StatementVector*>, VarNameLess> init_partdef;
	std::set<ReferenceExpression*> init_uncl;
	for( StatementVectorList::iterator i = cfgex->getSubblocks().begin(); i != cfgex->getSubblocks().end(); i++)
	{
		defined[*i] = init_defined;
		part_defined[*i] = init_partdef;
	}

	//добавить инициацию
	index(cfgex->getBlockByNum(0));

	while(unclear.size()&& reindexUnclear(cfgex->getBlockByNum(0)));

	if(unclear.size()) throw OPS::Exception("Using of uninitialized variable");



}

SSAVarMap::~SSAVarMap()
{

}

SSAGenerator::SSAGenerator( const ReferenceExpression* p_oper )
{
	m_phi = 0;
	m_decl = 0;
	OPS_ASSERT(p_oper);
	m_type = SSAGT_GENERATOR;
	m_oper = p_oper;
}

SSAGenerator::SSAGenerator( const PhiFunction* p_phi )
{
	m_oper = 0;
	m_decl = 0;
	OPS_ASSERT(p_phi);
	m_type = SSAGT_PHI;
	m_phi = p_phi;
}

PhiFunction::PhiFunction()
{
	m_genSubscript = 0;
}

PhiFunction::~PhiFunction()
{

}





bool OPS::Analysis::SSAForms::SSASubscript::isGenSubscript() const
{
	return (m_ssast == SSAST_GEN);
}

bool OPS::Analysis::SSAForms::SSASubscript::isUseSubscript() const
{
	return (m_ssast == SSAST_USE);
}

bool OPS::Analysis::SSAForms::SSASubscript::isComplexSubscript() const
{
	return (m_ssast == SSAST_COMPLEX);
}

int OPS::Analysis::SSAForms::SSASubscript::getUseSubscript() const
{
	return m_useSubscript;
}

int OPS::Analysis::SSAForms::SSASubscript::getGenSubscript() const
{
	return m_genSubscript;
}

OPS::Analysis::SSAForms::SSASubscript::SSASubscriptType OPS::Analysis::SSAForms::SSASubscript::getSSASubscriptType() const
{
	return m_ssast;
}

const VariableDeclaration& OPS::Analysis::SSAForms::SSASubscript::getVariable() const
{
	return *m_var;
}

OPS::Analysis::SSAForms::SSASubscript::SSASubscript( const VariableDeclaration& initvar, int subscript, SSASubscriptType ssast ) : m_var(&initvar)
{
	m_ssast = ssast;
	if(ssast == SSAST_USE) m_useSubscript = subscript;
	else if(ssast == SSAST_GEN) m_genSubscript = subscript;
}

OPS::Analysis::SSAForms::SSASubscript::SSASubscript( const VariableDeclaration& initvar, int genSubscript, int useSubscript ) : m_var(&initvar)
{
	m_ssast = SSAST_COMPLEX;
	m_genSubscript = genSubscript;
	m_useSubscript = useSubscript;
}

bool OPS::Analysis::SSAForms::SSASubscript::operator == ( const SSASubscript& compared_to ) const
{
	if(compared_to.m_var != this->m_var) return false;
	if(compared_to.m_ssast != this->m_ssast) return false;
	if(compared_to.m_genSubscript != this->m_genSubscript) return false;
	if(compared_to.m_genSubscript != this->m_genSubscript) return false;
	return true;
}

bool OPS::Analysis::SSAForms::SSASubscript::equals(const SSASubscript& compared_to) const
{
	if(*this == compared_to) return true;
	if(compared_to.m_var != this->m_var) return false;
	if( (m_ssast == SSASubscript::SSAST_COMPLEX) || (compared_to.m_ssast == SSASubscript::SSAST_COMPLEX) ) return false;
	int subscript1, subscript2;
	if( m_ssast == SSASubscript::SSAST_USE ) subscript1 = m_useSubscript;
	else subscript1 = m_genSubscript;

	if( compared_to.m_ssast == SSASubscript::SSAST_USE ) subscript2 =  compared_to.m_useSubscript;
	else subscript2 =  compared_to.m_genSubscript;

	return subscript1 == subscript2;
	
	
}


bool OPS::Analysis::SSAForms::SSASubscript::operator<( const SSASubscript& compared_to ) const
{
	VarNameLess vnl;
	if(vnl(this->m_var, compared_to.m_var)) return true;
	if(vnl(compared_to.m_var, this->m_var)) return false;
	if(compared_to.m_ssast > this->m_ssast) return true;
	if(compared_to.m_ssast < this->m_ssast) return false;
	if(compared_to.m_genSubscript > this->m_genSubscript) return true;
	if(compared_to.m_genSubscript < this->m_genSubscript) return false;
	if(compared_to.m_genSubscript > this->m_genSubscript) return true;
	if(compared_to.m_genSubscript < this->m_genSubscript) return false;
	return false;	
}


OPS::Analysis::SSAForms::SSAForm::SSAForm(BlockStatement& bs ): m_bs(&bs)
{
	try
	{
		m_vreg.reset(new VirtualRegistry());
		m_vreg->fill(bs);

		m_df.reset(new DominanceFrontier(bs));

		m_cfgex = &(m_df->getCFGex());

		m_blocks= &(m_cfgex->getSubblocks());

		std::map<const VariableDeclaration*, PhiFunction, VarNameLess> initmap;

		for( StatementVectorList::iterator i = m_blocks->begin(); i != m_blocks->end(); ++i)
		{
			m_phisAnchors[*i] = initmap;
		}

		//заполняем карту генераторов в блоке
		fillGens();
		//по графу доминаторов ищем места, где следует вставлять фи-функции
		placePhis();


		//инициализируем структуры
		std::stack<int> stk;
		for(VirtualRegistry::VarSet::const_iterator i = m_vreg->getRegistredVars().begin();
			i != m_vreg->getRegistredVars().end(); 
			++i)
		{

			counters[*i] = 0;
			stacks[*i] = stk;
		}


		//расставить индексы у переменных
		indexVars(*(m_blocks->begin()));

	}
	catch(...)
	{
		clearPhis();
		clearSubscripts();
		clearSubscriptsToGens();
		throw;
	}
}

void OPS::Analysis::SSAForms::SSAForm::fillGens()
{
	m_varmap.reset(new SSAVarMap(*m_cfgex, *m_vreg, *m_df, &m_cfgex->rootBlock()));


	for( StatementVectorList::iterator i = m_blocks->begin(); i != m_blocks->end(); ++i)
	{
		for( StatementVector::iterator j = (*i)->begin(); j != (*i)->end(); j++)
			if(m_varmap->getGen(&(*j)))
			{
				if(gens.find(&(m_varmap->getGen(&*j))->getReference()) != gens.end()  && 
					find(gens[&m_varmap->getGen(&*j)->getReference()]->begin(), gens[&m_varmap->getGen(&*j)->getReference()]->end(),*i) == gens[&m_varmap->getGen(&*j)->getReference()]->end())
					gens[&m_varmap->getGen(&*j)->getReference()]->push_back(*i);
				else
				{
					gens[&m_varmap->getGen(&*j)->getReference()] = new  StatementVectorList();
					gens[&m_varmap->getGen(&*j)->getReference()]->push_back(*i);
				}

			}		
	}

}


//для каждой вершины берется ее граница доминаци
//и для каждой переменной, определенной в вершине, вставляет в границе
//якорь для будущей фи-функции


void OPS::Analysis::SSAForms::SSAForm::placePhis()
{
	 StatementGraph& domF = m_df->getDomF();
	for(std::map<const VariableDeclaration*,  StatementVectorList*, VarNameLess>::iterator  i = gens.begin(); i != gens.end(); ++i)	
	{
		for( StatementVectorList::iterator j = i->second->begin(); j != i->second->end(); ++j)
		{
			if(domF.find(*j) != domF.end())
			{
				for( StatementVectorList::iterator k = domF.find(*j)->second->begin();
					k != domF.find(*j)->second->end();
					++k)
				{
					insertPhi(*k, *i->first);
				}
			}
		}
	}
}


const BlockStatement* obtainParentBlock(const RepriseBase* stmt)
{
	if(!stmt->getParent()) return 0;
	const RepriseBase* parent = stmt->getParent();
	if(parent->is_a<BlockStatement>()) return &parent->cast_to<const BlockStatement>();
	else return obtainParentBlock(parent);
}

class VisibilityAnalisys
{
public:
	static bool isVisible(const VariableDeclaration& var, const BlockStatement* spc)
	{	
		const BlockStatement* nmspc = &(var.getDefinedBlock());
		while(spc)
		{
			if(nmspc == spc)
				return true;
			else 
			{
				spc = obtainParentBlock(spc);
			}
		}
		return false;
	}
	
};



void OPS::Analysis::SSAForms::SSAForm::insertPhi(  StatementVector* ins_to, const VariableDeclaration& var )
{
	Stash* st = &(*(ins_to->begin()));
	BlockStatement* parent_block = 0;
	
	if(st->isExpression()) 
	{
		if(st->getAsExpression()->obtainParentDeclaration())
		{
			parent_block = &(st->getAsExpression()->obtainParentDeclaration()->getDefinedBlock());
		}
		else if (st->getAsExpression()->obtainParentStatement())
		{
			parent_block = &(st->getAsExpression()->obtainParentStatement()->getParentBlock());
		}
	}
	else
	{
		parent_block = &(st->getAsStatement()->getParentBlock());
	}

	if(VisibilityAnalisys::isVisible(var, parent_block))
	{
		if(m_phisAnchors[ins_to].find(&var) == m_phisAnchors[ins_to].end())
			m_phisAnchors[ins_to][&var];
	}

}

void OPS::Analysis::SSAForms::SSAForm::indexVars(  StatementVector* blk )
{
	std::map <const VariableDeclaration*, int, VarNameLess> pushed;



	//для всех фи-функций в блоке
	for(std::map<const VariableDeclaration*,PhiFunction,VarNameLess>::iterator 
		it = m_phisAnchors[blk].begin(); it != m_phisAnchors[blk].end(); ++it)
	{		
		int i = counters[it->first];
		stacks[it->first].push(i);
		//pushed обозначает нам количество элементов, которые надо будет вытолкнуть из стека
		//при завершении функции
		pushed[it->first] = 1;
		counters[it->first]++;
		SSASubscript* phi_gen = new SSASubscript(*(it->first), i, SSASubscript::SSAST_GEN);
		m_subscripts_to_gens[phi_gen] = new SSAGenerator(&(it->second));

		(it->second).setGen(phi_gen);		
		(it->second).mDefinedBlock = blk;

	}
	//следующий этап:
	/*
	For each instruction, v = x op y, in block
	Replace x with xi, where i == top(Stacks[x])
	Replace y with yi, where i == top(Stacks[y])
	i = Counters[v]
	Replace v by vi
	Push i onto Stacks[v]
	Counters[v] = i +1
	*/

	for( StatementVector::iterator itit = blk->begin(); 
		itit != blk->end();
		++itit)
	{


		/*			Replace x with xi, where i == top(Stacks[x])
		Replace y with yi, where i == top(Stacks[y])
		*/
		for(std::list<ReferenceExpression*>::iterator j = m_varmap->getUses(&*itit).begin(); j != m_varmap->getUses(&*itit).end(); ++j)
		{

			if(m_vreg->isRegistred((*j)->getReference()))
			{
				SSASubscript* ssause = new SSASubscript((*j)->getReference(),stacks[&(*j)->getReference()].top(),SSASubscript::SSAST_USE);
				m_subscripts[*j] = ssause;
			}
		}

		/*Replace v by vi*/
		if(m_varmap->getGen(&*itit))
		{
			VariableDeclaration& temp_vd = (m_varmap->getGen(&*itit)->getReference());

			if(m_vreg->isRegistred(temp_vd))
			{	

				if(m_subscripts.find(m_varmap->getGen(&*itit)) != m_subscripts.end())
				{


					SSASubscript* ssacompl = new SSASubscript(temp_vd, counters[&temp_vd], m_subscripts[m_varmap->getGen(&*itit)]->getUseSubscript());
					delete m_subscripts[m_varmap->getGen(&*itit)];
					m_subscripts[m_varmap->getGen(&*itit)] = ssacompl;
				}
				else
				{
					SSASubscript* ssagen = new SSASubscript(temp_vd, counters[&temp_vd], SSASubscript::SSAST_GEN);
					m_subscripts[m_varmap->getGen(&*itit)] = ssagen;
					m_subscripts_to_gens[ssagen] = new SSAGenerator(m_varmap->getGen(&*itit));

				}



				
				stacks[&temp_vd].push(counters[&temp_vd]);
				counters[&temp_vd]++;

				if(pushed.find(&temp_vd) != pushed.end())
					pushed[&temp_vd]++;
				else
					pushed[&temp_vd] = 1;

			}

		}
		/*Replace v by vi*/
		if(m_varmap->getDeclaration(&*itit))
		{
			VariableDeclaration& temp_vd = *(m_varmap->getDeclaration(&*itit));

			if(m_vreg->isRegistred(temp_vd))
			{	

				SSASubscript* ssagen = new SSASubscript(temp_vd, counters[&temp_vd], SSASubscript::SSAST_GEN);
				m_decl_subscripts[m_varmap->getDeclaration(&*itit)] = ssagen;
				m_subscripts_to_gens[ssagen] = new SSAGenerator(m_varmap->getDeclaration(&*itit));



				
				stacks[&temp_vd].push(counters[&temp_vd]);
				counters[&temp_vd]++;

				if(pushed.find(&temp_vd) != pushed.end())
					pushed[&temp_vd]++;
				else
					pushed[&temp_vd] = 1;

			}
		}

	}


	


	/*For each successor, s, of block
	For each phi-node, p, in s
	v <= jth operand of p
	Replace v with vi, where i = top(Stacks[v])*/

	for( StatementVectorList::iterator ititit = (m_cfgex->getSucc())[blk]->begin();
		ititit != (m_cfgex->getSucc())[blk]->end();
		ititit++)
	{
		std::list<const VariableDeclaration*> to_erase;
		for(std::map<const VariableDeclaration*, PhiFunction, VarNameLess>::iterator 
			it = m_phisAnchors[*ititit].begin(); it != m_phisAnchors[*ititit].end(); ++it)
		{
			if(VisibilityAnalisys::isVisible(*it->first, &((*(blk->begin())).getParentBlock())))
			{
				if(stacks[it->first].empty())
					to_erase.push_back(it->first);
				else
				{
					insertPhiArg(*ititit, *it->first, blk,stacks[it->first].top());
				}
			}
		}
		for(std::list<const VariableDeclaration*>::iterator 
			it = to_erase.begin(); it != to_erase.end(); it++)
		{
			m_phisAnchors[*ititit].erase(m_phisAnchors[*ititit].find(*it));

		}
	}

	for( StatementVectorList::iterator j = (m_df->getDomTree())[blk]->begin();
		j != (m_df->getDomTree())[blk]->end();
		j++)
	{
		indexVars(*j);
	}

	for(std::map<const VariableDeclaration*, int, VarNameLess>::iterator k = pushed.begin(); k != pushed.end(); ++k)
	{
		for(int i = 0; i<k->second; i++)
			stacks[k->first].pop();
	}

}

void OPS::Analysis::SSAForms::SSAForm::insertPhiArg(  StatementVector* ins_to,const VariableDeclaration& old,  StatementVector* from , int what )
{
	m_phisAnchors[ins_to][&old].addUse(new SSASubscript(old,what,SSASubscript::SSAST_USE),from);
}

OPS::Analysis::SSAForms::SSAForm::~SSAForm()
{
	clearSubscripts();
	clearSubscriptsToGens();
}

const OPS::Analysis::SSAForms::SSAGenerator* OPS::Analysis::SSAForms::SSAForm::getSSAGenerator( const SSASubscript* subscript ) const
{
	for(SSAGraphMap::iterator gen = m_subscripts_to_gens.begin();
		gen != m_subscripts_to_gens.end();
		++gen)
	{
		if(gen->first->equals(*subscript))
			return gen->second;
	}
	return 0;
}

const OPS::Analysis::SSAForms::SSAGenerator* OPS::Analysis::SSAForms::SSAForm::getSSAGenerator( const ExpressionBase* occurrence ) const
{
	return getSSAGenerator(getSubscript(occurrence));
}


void OPS::Analysis::SSAForms::SSAForm::clearSubscriptsToGens()
{
	for(std::map<const SSASubscript*, SSAGenerator*>::iterator i = m_subscripts_to_gens.begin(); i != m_subscripts_to_gens.end(); ++i )	
	{
		delete i->second;
	}
}

void OPS::Analysis::SSAForms::SSAForm::clearPhis()
{
	m_phisAnchors.clear();
}

void OPS::Analysis::SSAForms::SSAForm::clearSubscripts()
{
	

	for(ExpressionsSubscriptsMap::iterator i=m_subscripts.begin(); i!=m_subscripts.end(); ++i)
	{
		delete i->second;
	}

	for(DeclarationsSubscriptsMap::iterator i=m_decl_subscripts.begin(); i!=m_decl_subscripts.end(); ++i)
	{
		delete i->second;
	}
}

const SSASubscript* OPS::Analysis::SSAForms::SSAForm::getSubscript( const ExpressionBase* occurrence ) const
{
	return m_subscripts[occurrence];
}

const SSASubscript* OPS::Analysis::SSAForms::SSAForm::getSubscript( const VariableDeclaration* decl ) const
{
	return m_decl_subscripts[decl];
}

ControlFlowGraphEx& OPS::Analysis::SSAForms::SSAForm::getCFGex()
{
	return *m_cfgex;
}

const OPS::Analysis::SSAForms::SSAForm::PhiList OPS::Analysis::SSAForms::SSAForm::getPhis( StatementVector* cfblock ) const
{
	PhiList to_return;
	for(PhisMap::iterator phiter = m_phisAnchors[cfblock].begin(); phiter != m_phisAnchors[cfblock].end(); ++phiter)
	{
		to_return.push_back(&phiter->second);
	}
	return to_return;
}

DominanceFrontier& OPS::Analysis::SSAForms::SSAForm::getDominanceFrontier()
{
	return *m_df;
}
OPS::Analysis::SSAForms::SSAGenerator::SSAGenerator( const VariableDeclaration* p_decl )
{
	m_oper = 0;
	m_phi = 0;
	m_decl = p_decl;
	OPS_ASSERT(p_decl);
	m_type = SSAGT_DECLARATION;
}

OPS::Analysis::SSAForms::SSAGenerator::SSAGenerator()
{
	m_phi = 0; m_oper = 0; m_decl = 0;
}

const VariableDeclaration& OPS::Analysis::SSAForms::SSAGenerator::getVariable()
{
	if(getDeclaration()) return *getDeclaration();
	if(getExpression()) return getExpression()->getReference();
	if(getPhi()) return getPhi()->getGenSubscript()->getVariable();
	throw OPS::Exception("Bad SSAGenerator");
}
SSASubscript* OPS::Analysis::SSAForms::PhiFunction::getGenSubscript() const
{
	return m_genSubscript;
}

const PhiFunction::UsesSubscriptMap& OPS::Analysis::SSAForms::PhiFunction::getUsesMap() const
{
	return m_usesSubscript;
}

void OPS::Analysis::SSAForms::PhiFunction::setGen( SSASubscript* gen )
{
	m_genSubscript = gen;
}

void OPS::Analysis::SSAForms::PhiFunction::addUse( SSASubscript* use, StatementVector* path )
{
	OPS_ASSERT(use);
	OPS_ASSERT(path);
	m_usesSubscript[use] = path;
}

std::list<SSASubscript*> OPS::Analysis::SSAForms::PhiFunction::getUsesSubscript() const
{
	std::list<SSASubscript*> to_return;
	for(UsesSubscriptMap::const_iterator use = m_usesSubscript.begin();
		use != m_usesSubscript.end();
		++use)
	{
		to_return.push_back(use->first);
	}
	return to_return;
}
