#include "Analysis/LatticeGraph/ElemLatticeGraph.h"
#include "Analysis/LatticeGraph/LatticeGraph.h"

#include "OPS_Core/msc_leakcheck.h"


//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//void LatticeGraph::Clear()
//{
//	LatticeDataElemIterator firstS=arrowFuncList.begin(),lastS=arrowFuncList.end();
//	while(firstS!=lastS)
//	{		
//		delete *firstS;
//		firstS++;
//	}
//	arrowFuncList.clear();
//
//	if(occurDesc)
//	{
//		delete occurDesc;
//		occurDesc = NULL;
//	}
//
//	//Clearing statuses...
//	ClearStatus(~(LG_DONT_TRY_BUILD_BY_INVERTING));//Стираем все, кроме настроечных
//	//Cleared!
//
//}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//void LatticeGraph::DeleteFuncsWithSuppLessThan(int supp)
//{
//	LatticeDataElemIterator firstS=arrowFuncList.begin(),lastS=arrowFuncList.end();
//	while(firstS!=lastS)
//	{
//		if(GetValueBasedSupp((*firstS)->arrowFunc,(*firstS)->m_commonLoopsNumb)<supp)
//		{
//			delete *firstS;
//			firstS=arrowFuncList.erase(firstS);
//		}
//		else
//			firstS++;
//	}
//
//}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//bool LatticeGraph::HasSrcOccur(const ReferenceExpression* srcOccur)
//{
//	LatticeDataElemIterator firstS=arrowFuncList.begin(),lastS=arrowFuncList.end();
//	while(firstS!=lastS)
//	{
//		if((*firstS)->pOccur==srcOccur)
//			return true;
//		firstS++;
//	}
//	return false;
//}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//void LatticeGraph::GetLatticeBasedSuppList(const ReferenceExpression* pSrcOccur,const ReferenceExpression* pDepOccur, std::list<int>& latticeBasedSuppList)
//{
//	latticeBasedSuppList.clear();//очистим список
//
//	if(pDepOccur!=pOccur)
//		return;//странная ситуация
//
//	LatticeDataElemIterator firstS=arrowFuncList.begin(),lastS=arrowFuncList.end();
//
//	while(firstS!=lastS)
//	{
//		LatticeDataElem* tempElem=*firstS;
//		if(tempElem->pOccur==pSrcOccur)
//			latticeBasedSuppList.push_back(GetValueBasedSupp(tempElem->arrowFunc,tempElem->m_commonLoopsNumb));
//
//		firstS++;
//	}
//	
//	latticeBasedSuppList.unique();
//}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//void LatticeGraph::Build(const std::list<OccurDesc*>& srcEntries,OccurDesc& depEntry)
//{
//	Clear();
//	std::list<OccurDesc*>::const_iterator srcFirst=srcEntries.begin(),srcLast=srcEntries.end();
//
//	//occurDesc=&depEntry;
//	//копируем описание вхождения... - не копируем!!!!!!!!!!! (Гуда С.А.)
//	occurDesc = depEntry;
//
//	ElemLatticeGraph elg;
//	if(GetStatus(LG_DONT_TRY_BUILD_BY_INVERTING))
//		elg.SetStatus(LG_DONT_TRY_BUILD_BY_INVERTING);
//	while(srcFirst!=srcLast)
//	{
//		elg.Build(**srcFirst,depEntry);
//		if(elg.GetStatus(LG_ERROR_INIT))
//		{
//			SetStatus(LG_ERROR_INIT);
//			return;
//		}
//
//		if(!elg.GetStatus(LG_NOSOLUTION))
//		{//т.е. решение есть
//			if(elg.GetStatus(LG_NONLINEAR_SOLUTION))
//			{
//				SetStatus(LG_NONLINEAR_SOLUTION|LG_ERROR_INIT);
////				Clear();
//				return;// Пока я не буду обрабатывать такие ситуации (поэтому установлен флаг LG_ERROR_INIT)
//			}
//			else
//			{
//				if(arrowFuncList.size()==0)
//				{//просто добавляем
//					//Нужно оптимизировать - не копировать функцию, а копировать указатель на нее...
//					ElemLatticeGraph::VoevodinSolution::iterator firstS = elg.getAndMakeVoevodinSolutionInTransformedVars().begin(),lastS=elg.getAndMakeVoevodinSolutionInTransformedVars().end();
//					while(firstS!=lastS)
//					{
//						LatticeDataElem* newDataElem=new LatticeDataElem(**firstS,(*srcFirst)->GetStatement(),(*srcFirst)->pOccur,(elg.getSrcOccurDesc()),(*srcFirst)->GetNumber(), elg.GetCommonLoopsNumb(),elg.GetVarNames());
//						arrowFuncList.push_back(newDataElem);
//						firstS++;
//					}
//				}
//				else
//				{//а вот теперь, сложная обработка...
//
//					ElemLatticeGraph::VoevodinSolution::iterator firstS=elg.getAndMakeVoevodinSolutionInTransformedVars().begin(),lastS=elg.getAndMakeVoevodinSolutionInTransformedVars().end();
//					while(firstS!=lastS)
//					{
//						LatticeDataElem* newDataElem = new LatticeDataElem(**firstS,(*srcFirst)->GetStatement(),(*srcFirst)->pOccur,(elg.getSrcOccurDesc()),(*srcFirst)->GetNumber(),elg.GetCommonLoopsNumb(),elg.GetVarNames());
//
//						UnionWithArrowFunc(newDataElem);
//
//						firstS++;
//					}
//
//				}
//			}
//		}
//		elg.Clear();
//		srcFirst++;
//	}
//	
//	pStmt=depEntry.GetStatement();
//	pOccur=depEntry.pOccur;
//	occurNumb=depEntry.GetNumber();
//
//	if(arrowFuncList.size()==0)
//		SetStatus(LG_NOSOLUTION);
//}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//void LatticeGraph::UnionWithLatticeGraph(LatticeGraph& lg)
//{
//	LatticeDataElemIterator first=lg.Begin(),last=lg.End();
//
//	if(arrowFuncList.size()==0)
//	{//Просто копируем
//		while(first!=last)
//		{
//			arrowFuncList.push_back(*first);
//			*first = NULL;
//			first++;
//		}
//	}
//	else
//	{// Объединяем.
//		while(first!=last)
//		{
//			UnionWithArrowFunc(*first);
//			first++;
//		}
//	}
//	lg.Clear();
//}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//void LatticeGraph::UnionWithArrowFunc(LatticeDataElem*& lde)
//{
//	if(arrowFuncList.size()==0)
//	{//Граф пустой. В этом случае нужно просто добавить поданную функцию.
//		arrowFuncList.push_back(lde);
//		lde = NULL;
//		return;
//	}
//
//	LatticeDataElemIterator first=arrowFuncList.begin(),last=arrowFuncList.end();
//	
//	do
//	{
//
//		if(CombineRowFuncs(*first,lde)==0)
//		{
//
//			//if((*first)->arrowFunc.areas.GetStatus(GE_UNFEASIBLE))
//			if((*first)->arrowFunc.areas.IsFeasible() == 0)
//			{
//				delete *first;
//				first=arrowFuncList.erase(first);
//			}
//			else
//				first++;
//
//			//if(lde->arrowFunc.areas.GetStatus(GE_UNFEASIBLE))
//			if(lde->arrowFunc.areas.IsFeasible() == 0)
//				first=last;
//
//		}
//		else
//			if(CombineRowFuncs(*first,lde)==1)
//			{//Функции не изменились => нужно просто добавить новую функцию к решетчатому графу...
//				arrowFuncList.push_back(lde);
////				first++;
//				lde=NULL;
//				return;
//
//			}
//			else
//			{
//				latticeGraphLog(Console::LEVEL_WARNING, _TL("\n Some strange error while building Lattice Graph (LatticeGraph::Build)\n",""));
//				delete lde;
//				lde=NULL;
//				return;
//			}
//	
//
//	}
//	while(first!=last);
//
//	//if(!lde->arrowFunc.areas.GetStatus(GE_UNFEASIBLE))
//	if(lde->arrowFunc.areas.IsFeasible())
//		arrowFuncList.push_back(lde);
//
//	lde=NULL;
//}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//const LatticeDataElem* LatticeGraph::GetSource(LoopIndex& source,const LoopIndex& sink)
//{
//	LatticeDataElemIterator first=arrowFuncList.begin(),last=arrowFuncList.end();
//	while(first!=last)
//	{
//		ArrowFunc* r=&((*first)->arrowFunc);
//		if(r->areas.SatisfiedWith(sink.GetData(),sink.getSize(),r->newParamList))
//		{
//			//Имея конец sink дуги, находим ее начало, используя функцию r->...
//			r->GetSource(source,sink);
//			//нашли. 
//			return *first;
//		}
//
//		first++;
//	}
//
//	return NULL;
//}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//void LatticeGraph::Print(const char* fileName)
//{
//	std::ofstream file(fileName);
//	LatticeDataElemIterator first=arrowFuncList.begin(),last=arrowFuncList.end();
//	while(first!=last)
//	{
//		ArrowFunc* temp=&((*first)->arrowFunc);
//		int addedVarNum=temp->newParamList->getSize();
//		int m_srcEntry->loopNumb=temp->dimi;
//		int m_depEntry->loopNumb=temp->dimj-addedVarNum-1;
//		std::string* varNames=(*first)->varNames;
//
////		buf[0]=0;
////		my_PrintExpression((*first)->entryRef,buf);
//		file<<"\nSource #"<<(*first)->occurNumb<<": "/*<<buf*/<<std::endl<<std::endl;
//
//		for(int i=0;i<m_srcEntry->loopNumb;i++)
//		{
//			file<<varNames[i]<<"'=";
//			for(int j=0;j<(m_depEntry->loopNumb+addedVarNum);j++)
//			{
//				if(temp->data[i][j]>=0)file<<'+';
//				file<<temp->data[i][j]<<'*';//<<varNames[j+m_srcEntry->loopNumb];
//				if(j<m_depEntry->loopNumb)
//					file<<varNames[j+m_srcEntry->loopNumb];
//				else
//				{
//					//char buf[8];
//					//sprintf(buf,"_%d",j-m_depEntry->loopNumb+1);
//					//file<<buf;
//					file<<'_'<<(j-m_depEntry->loopNumb+1);
//				}
//			}
//			if(temp->data[i][m_depEntry->loopNumb+addedVarNum]>=0)file<<'+';
//			file<<temp->data[i][m_depEntry->loopNumb+addedVarNum]<<std::endl;
//		}
//
//		file<<"\nAdded Var(s)' Equation(s):\n\n";
//		temp->newParamList->Print(file,&(varNames[m_srcEntry->loopNumb]),m_depEntry->loopNumb);
//
//		file<<"\nContext(s):\n\n";
//
//		temp->areas.Print(file,&(varNames[m_srcEntry->loopNumb]),m_depEntry->loopNumb,1);
//
//		file<<"\n altPolyNumber="<<temp->altPolyNumb;
//		file<<"\n==========================================================\n\n";
//		first++;
//	}
//}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//int LatticeGraph::selfTest(std::list<OccurDesc*>& srcEntries,OccurDesc& depEntry)
//{//Алгоритм - прост и не оптимален, зато - относительно легко был реализован
//
//	int errorState=0;
////Сначала строим набот элементарных решетчатых графов и проверяем их правильность построения...
//	std::list<ElemLatticeGraph*> elemLGL;//LGL - Lattice Graphs List
//
//	std::list<OccurDesc*>::iterator first=srcEntries.begin(),last=srcEntries.end();
//
//	while(first!=last)
//	{
//		ElemLatticeGraph* elg=new ElemLatticeGraph();
//		elg->Build(**first,depEntry);
//		
//		if(elg->selfTest()==0)
//		{
//			latticeGraphLog(Console::LEVEL_WARNING, _TL("\nSome Elem. Lattice Graph is built incorrectly! (LatticeGraph::selfTest)",""));
//			std::list<ElemLatticeGraph*>::iterator firstLGL=elemLGL.begin(),lastLGL=elemLGL.end();
//			while(firstLGL!=lastLGL)
//			{
//				delete *firstLGL;
//				firstLGL++;
//			}
//			elemLGL.clear();
//			return 0;
//		}
//
//		if(elg->getAndMakeVoevodinSolutionInTransformedVars().size()==0)
//			delete elg;
//		else
//			elemLGL.push_back(elg);
//
//		first++;
//	}
//
//	std::list<ElemLatticeGraph*>::iterator firstLGL=elemLGL.begin(),lastLGL=elemLGL.end();
//	if(firstLGL==lastLGL)return 1;
//
//	LoopIndex sink((*firstLGL)->GetParamVarNum(),((*firstLGL)->GetSuppPolyhedronForDep()));
////	LoopIndex maxSrc,tempSrc;
//
//	sink.SetToLowerBounds();
//
//	while(sink.isInBounds())
//	{
////		cout<<sink<<std::endl;
//		//Ищем начало дуги по решетчатому графу...
//
//		LoopIndex* supposedSrc=NULL;
//		int supposedSrcFound=0;
//		LatticeDataElem* lattSrcDesc=NULL;
//		LatticeDataElemIterator firstLDE=arrowFuncList.begin(),lastLDE=arrowFuncList.end();
//		while(firstLDE!=lastLDE)
//		{
//			ArrowFunc* r=&((*firstLDE)->arrowFunc);
//			if(r->areas.SatisfiedWith(sink.GetData(),sink.getSize(),r->newParamList))
//			{
//				if(supposedSrcFound)
//				{//Ошибка: решетчатый граф имеет более одной дуги, входящей в одну вершину
//					errorState=1;
//					delete supposedSrc;
//					goto endOfSelfTest;
//				}
//
//				//Имея конец sink дуги, находим ее начало, используя функцию r->...
//				supposedSrc=new LoopIndex(r->dimi,(*firstLDE)->occurDesc->suppPolyhedron);
//				r->GetSource(*supposedSrc,sink);
//				//нашли. 
//				lattSrcDesc=*firstLDE;
//				supposedSrcFound=1;
//			}
//
//			firstLDE++;
//		}
//
//		//получили
//
//		//ищем максимальный вектор-начало дуги, по построенным элем. решетчатым графам...
//		int found=0;
//		OccurDesc* srcEntry;//Описание вхождения, которое задает начало дуги
////		int srcEntryNumb;
//		LoopIndex* maxSrc;
//		firstLGL=elemLGL.begin();
//		while(firstLGL!=lastLGL)
//		{
//			LoopIndex* tempSrc=new LoopIndex((*firstLGL)->GetVarNum(),NULL);
//
//			if((*firstLGL)->GetSource(*tempSrc,sink))
//			{
//				if(found==0)
//				{
//					maxSrc=tempSrc;
//					tempSrc=NULL;
//					srcEntry=((*firstLGL)->getSrcOccurDesc());
////					srcEntryNumb=(*firstLGL)->getSrcOccurDesc().occurNumb;
//					found=1;
//				}
//				else
//				{//Нужно сравнивать, что раньше выполниться...
////					int m_commonLoopsNumb=getCommonUpperLoopsAmount(srcEntry->pStmt,depEntry->pStmt);
//					int m_commonLoopsNumb=OPS::Shared::getCommonUpperLoopsAmount(srcEntry->GetStatement(),pStmt);
//
//					if(LexCmp(*tempSrc,*maxSrc,m_commonLoopsNumb)>0)
//					{//т.е. если tempSrc лексикографически больше maxSrc
//						delete maxSrc;
//						maxSrc=tempSrc;
//						tempSrc=NULL;
//						srcEntry=((*firstLGL)->getSrcOccurDesc());
////						srcEntryNumb=(*firstLGL)->getSrcOccurDesc().occurNumb;
//					}
//					else
//						if(LexCmp(*tempSrc,*maxSrc,m_commonLoopsNumb)==0)
//						{//тогда нужно проверить, какое вх.-источник находится раньше по тексту программы
//							if((*firstLGL)->getSrcOccurDesc()->GetNumber() > srcEntry->GetNumber()/*srcEntryNumb*/)
//							{
//								delete maxSrc;
//								maxSrc=tempSrc;
//								tempSrc=NULL;
//								srcEntry=((*firstLGL)->getSrcOccurDesc());
////								srcEntryNumb=(*firstLGL)->getSrcOccurDesc().occurNumb;
//							}
//						}
//				}
//
//			}
//
//
//			delete tempSrc;
//			firstLGL++;
//		}
//
//
//
//		if(found)
//		{//найдено начало дуги. Необходимо проверить, совпадает ли это с "показаниями" решетчатого графа
//			
//			if(supposedSrcFound)
//			{//Нужно проверить, совпадают, ли эти векторы и сответствующие вхождения источники
//				if(*supposedSrc!=*maxSrc)
//				{//Ошибка: координаты начал найденных дуг не совпадают!
//					errorState=2;
//					delete maxSrc;
//					delete supposedSrc;
//					goto endOfSelfTest;
//				}
//
////				if(srcEntry!=lattSrcDesc->entryRef)
//				if(/*srcEntryNumb*/srcEntry->GetNumber() != lattSrcDesc->occurNumb)
//				{//Ошибка: вхождения источники не совпадают!
//					errorState=4;
//					delete maxSrc;
//					delete supposedSrc;
//					goto endOfSelfTest;
//				}
//
//			}
//			else
//			{//Ошибка: В Решетчатом графе в вершину sink не входит дуга, НО существует элементарный решетчатый граф в котором существует дуга в вершину sink
//				errorState=8;
//				delete maxSrc;
//				goto endOfSelfTest;
//			}
//		}
//		else
//		{
//			if(supposedSrcFound)
//			{//Ошибка: В решетчатом графе существует дуга в вершину sink, но не существует элементарного решетчатого графа с такой дугой
//				errorState=8;
//				delete supposedSrc;
//				goto endOfSelfTest;
//			}
//		}
//
//		if(supposedSrcFound) delete supposedSrc;
//		if(found)delete maxSrc;
//
//		sink++;
//	}
//
//
//	
//endOfSelfTest:
//
//	firstLGL=elemLGL.begin();lastLGL=elemLGL.end();
//	while(firstLGL!=lastLGL)
//	{
//		delete *firstLGL;
//		firstLGL++;
//	}
//	elemLGL.clear();
//
//	if(errorState)return 0;
//
//	return 1;	
//}
