#include "Analysis/DominanceFrontierGraph.h"

#include "OPS_Core/msc_leakcheck.h"
using namespace OPS::Analysis::ControlFlowGraphs;
using SSAControlFlowGraph::StatementGraph;
using SSAControlFlowGraph::CFGraph;
using SSAControlFlowGraph::StatementVector;
using SSAControlFlowGraph::StatementVectorList;
namespace OPS
{
	namespace Analysis
	{
	namespace ControlFlowGraphs 
	{
		
		namespace DominanceFrontierGraph
		{
			DominanceFrontier::DominanceFrontier(Reprise::BlockStatement &aBlock):m_cfgex(new ControlFlowGraphEx(aBlock) )
            {
				m_ownCFGex = true;
                buildDoms();
                buildIncDom();
                buildReverseDom();
                buildDomf();
                buildReverseDomF();

            }
            DominanceFrontier::DominanceFrontier(ControlFlowGraphEx &aCFGEx):m_cfgex(&aCFGEx)
            {
				m_ownCFGex = false;
                buildDoms();
                buildIncDom();
                buildReverseDom();
                buildDomf();
                buildReverseDomF();

            }
            DominanceFrontier::~DominanceFrontier()
            {

                for(StatementVectorList::const_iterator i = m_cfgex->getSubblocks().begin(); i != m_cfgex->getSubblocks().end();++i)
                {
                    if(doms.find(*i) != doms.end())
                    {
                        delete doms[*i]; 
                    }
                    if(domF.find(*i) != domF.end())
                    {
                        delete domF[*i]; 
                    }
                }
				if(m_ownCFGex) delete m_cfgex;
            }

            void DominanceFrontier::buildDoms()
            {
                doms.clear();
                StatementVectorList sorted(m_cfgex->getSubblocks());

                sorted.sort( std::less< StatementVector * >());

                for(StatementVectorList::iterator i = ++m_cfgex->getSubblocks().begin();i != m_cfgex->getSubblocks().end();++i)
                {
                    doms[*i] = new StatementVectorList(sorted);
                }   
                doms[*(m_cfgex->getSubblocks().begin())] = new StatementVectorList(m_cfgex->getSubblocks().begin(),++m_cfgex->getSubblocks().begin());
                bool changed = true;
                while(changed)
                {
                    changed = false;
                    for(StatementVectorList::const_iterator i = ++m_cfgex->getSubblocks().begin(); i != m_cfgex->getSubblocks().end();++i)
                    {

                        StatementVectorList newlist(sorted);
                        StatementGraph& blocksPred = (m_cfgex->getPred());	
                        for (StatementVectorList::iterator j = (blocksPred.find(*i))->second->begin();
                            j != blocksPred.find(*i)->second->end(); ++j)
                        {

                            StatementVectorList tmp(newlist);
                            newlist.erase(set_intersection ( doms[*j]->begin () , doms[*j]->end (), tmp.begin() , tmp.end (), newlist.begin(), std::less< StatementVector * >() ), newlist.end());


                        }
                        if(find(newlist.begin(),newlist.end(),*i) == newlist.end())
                            newlist.insert(lower_bound(newlist.begin(),newlist.end(),*i),*i);

                        StatementVectorList tmp(sorted);
                        tmp.erase(set_symmetric_difference( newlist.begin () , newlist.end (), doms[*i]->begin() , doms[*i]->end(), tmp.begin(), std::less< StatementVector * >() ), tmp.end());
                        if(!tmp.empty())
                        {
                            doms[*i]->clear();
                            doms[*i]->insert(doms[*i]->begin(),newlist.begin(),newlist.end());
                            changed = true;
                        }
                    }
                }
            }

            void DominanceFrontier::buildDomf()
            {
                const StatementGraph& blocksPred = m_cfgex->getPred();
                for(StatementVectorList::const_iterator init = m_cfgex->getSubblocks().begin(); init != m_cfgex->getSubblocks().end();++init)
                {
                    StatementVectorList* newSVL = new StatementVectorList();
                    domF[*init] = newSVL;
                }

                for(StatementVectorList::const_iterator i = m_cfgex->getSubblocks().begin(); i != m_cfgex->getSubblocks().end();++i)
                {
                    if(blocksPred.find(*i)->second->size() >= 2)
                    {

                        StatementVector* idom = domTreeInverse[*i];

                        for(StatementVectorList::const_iterator j = blocksPred.find(*i)->second->begin();
                            j != blocksPred.find(*i)->second->end();++j)
                        {
                            StatementVector* runner = (*j);
                            while(runner != idom)
                            {
                                if(domF.find(runner) != domF.end())
                                {
                                    if(find(domF[runner]->begin(),domF[runner]->end(), *i) == domF[runner]->end())
                                        domF[runner]->push_back(*i);
                                    runner = domTreeInverse[runner];

                                }
                                else
                                {
                                    StatementVectorList* newSVL = new StatementVectorList();
                                    newSVL->push_back(*i);
                                    domF[runner] = newSVL;
                                    runner = domTreeInverse[runner];;
                                }

                            }


                        }
                    }
                }    	
            }


            void DominanceFrontier::buildIncDom()
            {
                incDom.clear();
                for(StatementVectorList::iterator i = m_cfgex->getSubblocks().begin(); i != m_cfgex->getSubblocks().end();++i)
                {
                    for (StatementVectorList::iterator j = m_cfgex->getSubblocks().begin(); j != m_cfgex->getSubblocks().end();++j)
                    {
                        incDom[*i][*j] = 0;                 
                    }	 
                }

                for(StatementVectorList::iterator i = m_cfgex->getSubblocks().begin(); i != m_cfgex->getSubblocks().end();++i)
                {
                    if(doms.find(*i) != doms.end())
                    {
                        for (StatementVectorList::iterator j = doms[*i]->begin(); j != doms[*i]->end();++j)
                        {
                            if(*j != 0)
                            {
                                incDom[*i][*j] = 1;                 
                            }
                        }	 
                    }
                }
            }

            void DominanceFrontier::buildReverseDom()
            {
                domTree.clear();
                StatementVectorList::iterator i;
                for(i = m_cfgex->getSubblocks().begin(); i != m_cfgex->getSubblocks().end();++i)
                {
                    StatementVectorList* newSVL = new StatementVectorList();
                    domTree[*i] = newSVL;
                }

                for(i = m_cfgex->getSubblocks().begin(); i != m_cfgex->getSubblocks().end();++i)
                {
                    for(StatementVectorList::iterator j = m_cfgex->getSubblocks().begin(); j != m_cfgex->getSubblocks().end();++j)
                    {	
                        if(incDom[*i][*j])
                        {  
                            if(doms[*i]->size() == doms[*j]->size()+1)
                                domTreeInverse[*i] = (*j);
                            if(domTree.find(*j) != domTree.end())
                            {

                                if(doms[*i]->size() == doms[*j]->size()+1)
                                    domTree[*j]->push_back(*i);

                            }
                            else
                            {
                                StatementVectorList* newSVL = new StatementVectorList();
                                newSVL->push_back(*i);
                                domTree[*i] = newSVL;					
                            }
                        }
                    }
                }
            }

            void DominanceFrontier::buildReverseDomF()
            {
                domFInverse.clear();

                StatementVectorList::iterator i;
                for(i = m_cfgex->getSubblocks().begin(); i != m_cfgex->getSubblocks().end();++i)
                {
                    StatementVectorList* newSVL = new StatementVectorList();
                    domFInverse[*i] = newSVL;
                }

                for(i = m_cfgex->getSubblocks().begin(); i != m_cfgex->getSubblocks().end();++i)
                {
                    for(StatementVectorList::iterator j = m_cfgex->getSubblocks().begin(); j != m_cfgex->getSubblocks().end();++j)
                    {	
                        if(domF[*i]->size()>0)
                        {  				
                            if(domFInverse[*j]->size()>0)
                            {
                                domFInverse[*j]->push_back(*i);
                            }
                            else
                            {
                                StatementVectorList* newSVL = new StatementVectorList();
                                newSVL->push_back(*i);
                                domFInverse[*i] = newSVL;					
                            }
                        }
                    }
                }
            }

        }
	}
}
}
