#include "Analysis/DepGraph/DepGraph.h"
#include "Analysis/LatticeGraph/ElemLatticeGraph.h"
#include "Analysis/DepGraph/DepGraphEx/DepGraphEx.h"
#include "Analysis/DepGraph/SubProcAnalysis.h"
#include "Shared/LinearExpressionsUtils.h"
#include "Shared/LoopShared.h"
#include "Reprise/Service/DeepWalker.h"
#include <algorithm>

#include "OPS_Core/msc_leakcheck.h"
#include "OPS_Core/Localization.h"

namespace DepGraph
{
using namespace Id;

int getCommonUpperLoopsAmount(const OccurDesc& first, const OccurDesc& second)
{
	const int maxCommonLoopNumber = std::min(first.loopNumb, second.loopNumb);

	for(int i = 0; i < maxCommonLoopNumber; ++i)
	{
		if (first.loops[i].stmtFor != second.loops[i].stmtFor)
			return i;
	}

	return maxCommonLoopNumber;
}

using OPS::Console;

static inline void depGraphLog(Console::MessageLevel level, const std::string& message) 
{
	OPS::getOutputConsole("DepGraph").log(level, message); 
}

LoopDesc::LoopDesc(int lower, int upper, int step)
	:counterIter(0)
	,stmtFor(0)
	,l(lower)
	,u(upper)
	,s(step)
	,lBoundUnk(false)
	,rBoundUnk(false)
    ,lBoundNotLinear(false)
    ,rBoundNotLinear(false)
{}

LoopDesc::LoopDesc()
	:counterIter(0)
	,stmtFor(0)
	,l(0),u(0),s(0)
	,lBoundUnk(true)
	,rBoundUnk(true)
	,lBoundNotLinear(true)
	,rBoundNotLinear(true)
{}



OccurDesc::OccurDesc(int _occurNumb,const VariableDeclaration* _varDecl,StatementBase* _pStmt,ReferenceExpression* _pOccur,int _dim,int ln,bool _pris)
	:loopNumb(ln),dim(_dim),data(NULL)
	,m_varDecl(_varDecl),d1(NULL),pOccur(_pOccur),loops(0),m_isIndexesLinear(_pris),m_hasExtVarsOrNotLinear(false)
	,suppPolyhedron(NULL),pStmt(_pStmt),occurNumb(_occurNumb)
{
	if(dim) {
		//const VariableDeclaration* nameVar = dynamic_cast<const VariableDeclaration*>(m_varDecl);
		//OPS_ASSERT(nameVar, _TL("Named object is not variable.","Именованый объект не является переменной"));
		
		const ArrayType* arrayType = dynamic_cast<const ArrayType*>(&m_varDecl->getType());
		const Canto::HirFArrayType* fortranArrayType = dynamic_cast<const Canto::HirFArrayType*>(&m_varDecl->getType());
		if (arrayType == 0 && fortranArrayType == 0)
		{
			SetStatus(UNKNOWN_BOUNDS);
			OPS::Console* const pConsole = &OPS::getOutputConsole("DepGraph");
			pConsole->log(OPS::Console::LEVEL_WARNING, _TL("BuildVO: multidim variable is not static array: " + m_varDecl->getName(),
															"BuildVO: многомерная переменная не является статическим массивом."));
			//throw OPS::RuntimeError(_TL("Couldn't build occurence description: multidim variable is not static array.","Невозможно построить описание вхождения: многомерная переменная не является статическим массивом."));
		}
		//else
		{
			data = new int*[dim];
			for(int i = 0; i < dim; ++i)
			{
				data[i] = new int[loopNumb+1];
				memset(data[i], 0, sizeof(int)*(loopNumb+1));
			}
			ub.assign(dim ,0);
			lb.assign(dim, 0);

			if(arrayType)
			{
				const TypeBase* temp = arrayType;
				const ArrayType* tempArrType = arrayType;
				for(int i = 0; i < dim; ++i)
				{
					//data[i] = new int[loopNumb+1];
					
					//// Будем добывать границы массивов...
					//if (((*arrayType)[i].high<0) || ((*arrayType)[i].low<0))			============== NB! =================
					//	throw;//Такие массивы я пока в рассчет не беру...
	
					tempArrType = dynamic_cast<const ArrayType*>(temp);
					if (tempArrType != 0)
					{
						ub[i] = tempArrType->getElementCount() - 1;
						lb[i] = 0;
						temp = &tempArrType->getBaseType();
					}
					else
					{
																						//============== NB! =================
					}
				}
			}
			if(fortranArrayType)
			{
				dim = fortranArrayType->getShape().getRank();
				for(int i=0; i<dim; i++)
				{
					//data[i] = new int[loopNumb+1];
					const ExpressionBase* temp = &fortranArrayType->getShape().getDimension(i).getUpperBound();
					if(temp->is_a<BasicLiteralExpression>())
						ub[i] = temp->cast_to<BasicLiteralExpression>().getInteger();
					else if(temp->is_a<StrictLiteralExpression>())
						ub[i] = temp->cast_to<StrictLiteralExpression>().getInt32();
					else
					{
						//m_isIndexesLinear = false;
						SetStatus(UNKNOWN_BOUNDS);
						OPS::Console* const pConsole = &OPS::getOutputConsole("DepGraph");
						pConsole->log(OPS::Console::LEVEL_WARNING, _TL("BuildVO: upper bound of array is not const.",
							"BuildVO: Граница массива непостоянная величина."));
					}

					temp = &fortranArrayType->getShape().getDimension(i).getLowerBound();
					if(temp->is_a<BasicLiteralExpression>())
						lb[i] = temp->cast_to<BasicLiteralExpression>().getInteger();
					else if(temp->is_a<StrictLiteralExpression>())
						lb[i] = temp->cast_to<StrictLiteralExpression>().getInt32();
					else
					{
						//m_isIndexesLinear = false;
						SetStatus(UNKNOWN_BOUNDS);
						OPS::Console* const pConsole = &OPS::getOutputConsole("DepGraph");
						pConsole->log(OPS::Console::LEVEL_WARNING, _TL("BuildVO: lower bound of array is not const.",
							"BuildVO: Граница массива непостоянная величина."));
					}
				}

				if (isStatus(UNKNOWN_BOUNDS))
				{
					ub.clear();
					lb.clear();
				}
			}

		}
	}
	else {
		data = 0;
		d1 = 0;
		ub.clear();
		lb.clear();
	}

	if (loopNumb)
		loops = new LoopDesc[loopNumb];
	else 
		loops = 0;
}

OccurDesc::OccurDesc(const OccurDesc& indElem):Status(indElem),loopNumb(0),dim(0),data(NULL),m_varDecl(NULL),d1(NULL),pOccur(NULL),loops(NULL)
{
	Copy(indElem);
}

void OccurDesc::Copy(const OccurDesc& other)
{
    Clear();

	dim = other.dim;
	occurNumb = other.occurNumb;

	SaveStatus(other.GetStatusData());
	loopNumb = other.loopNumb;
	m_varDecl = other.m_varDecl;
	m_isIndexesLinear = other.m_isIndexesLinear;
	pStmt = other.pStmt;
	pOccur = other.pOccur;
    m_externalParamCoefs = other.m_externalParamCoefs;

	if (other.suppPolyhedron)
        suppPolyhedron = new OPS::LatticeGraph::Polyhedron(*other.suppPolyhedron);
	else
		suppPolyhedron = 0;

	if(dim) {
		data = new int*[dim];
		//ArrayType& arrType = m_varDecl->getType().cast_to<ArrayType>();

		for(int i = 0; i < dim; ++i) {
			data[i] = new int[loopNumb+1];

			if (other.data)
				memcpy(data[i], other.data[i], (loopNumb+1)*sizeof(int));
		}
		ub = other.ub;
		lb = other.lb;

		if (other.d1) {
			d1 = new int[other.loopNumb+1];
			memcpy(d1, other.d1, (other.loopNumb+1)*sizeof(int));
		}
		else {
			d1 = 0;
		}
	}
	else {
		data = 0;
		d1 = 0;
		ub.clear();
		lb.clear();
	}

	delete[] loops;

	if(loopNumb)
	{
		loops = new LoopDesc[loopNumb];

		if (other.loops) {
			for(int i = 0; i < loopNumb; ++i)
				loops[i] = other.loops[i];
		}
	}
	else
	{
		loops = 0;
	}
}

OccurDesc& OccurDesc::operator=(const OccurDesc& indElem)
{
	if (this == &indElem)
		return *this;

	Clear();
	Copy(indElem);
	return *this;
}

bool OccurDesc::IsIncludedByForBody(const ForStatement* stmtFor) const
{
	return GetLoopDeep(stmtFor) != -1;
}

OccurDesc::~OccurDesc()
{
	Clear();
}

void OccurDesc::ClearData()
{
	if (data)
	{
		for(int i=0;i<dim;i++)
			delete[] data[i];
		delete[] data;
		data = 0;
	}
}

void OccurDesc::Clear()
{
	ClearData();
    m_externalParamCoefs.clear();

	delete[] loops; loops = 0;
	delete[] d1; d1 = 0;
	ub.clear();
	lb.clear();
	delete suppPolyhedron; suppPolyhedron = 0;
}

int OccurDesc::GetLoopDeep(const ForStatement* stmtFor) const
{
	for(int i=0; i<loopNumb; i++)
		if(loops[i].stmtFor == stmtFor)
			return i;

	return -1;
}
//заполняет информацию о данном вхождении, в частности для массивов строит массив коэффициентов при счетчиках и при внешних параметрах 
OccurDesc* OccurList::AddNewElem(
					  int occurNumb, 
					  StatementBase* pStmt, 
					  LoopDesc* indLoops,
					  OccurrenceInfo& idOccur)
{
    // если индексы нормальные или скалярная переменная
    //IsIndexesLinear() всегда возвращает false в данном месте, т.к. ранее индексы на линейность не проверяются
	bool flagIsIndexesLinear = idOccur.IsIndexesLinear() || !idOccur.IsArray();
    bool flagHasExtVarsOrNotLinear = false;

	// создаем вхождение
	OccurDesc* res = new OccurDesc(occurNumb, idOccur.m_oDataObject, pStmt, idOccur.m_pED, idOccur.m_dim, idOccur.m_numcycles, flagIsIndexesLinear);
	if(res->IsArray())
	{
        //конструируем массив коэффициентов при счетчиках циклов
        //OPS::Shared::LinearExpressionMatrix le(idOccur.m_pED);
        //std::vector<OPS::Shared::ParametricLinearExpression*> allCoefs = le.detachCoefficients(); - не работает при внешних параметрах
        BasicCallExpression& arrayCall = idOccur.m_pED->getParent()->cast_to<BasicCallExpression>();
        int arrayDim = arrayCall.getChildCount()-1;
        res->m_externalParamCoefs.resize(arrayDim);
        std::vector<OPS::Reprise::VariableDeclaration*> loopCounters = OPS::Shared::getIndexVariables(idOccur.m_pED);
        int loopNumb = loopCounters.size();
        flagIsIndexesLinear = true;
        flagHasExtVarsOrNotLinear = false;

        OPS_ASSERT(arrayDim == idOccur.m_dim);
		res->ClearData();
        res->data = new int*[arrayDim];
		for(int i = 0; i < arrayDim; ++i)
			res->data[i] = new int[loopNumb+1];

        int i;
        try
        {
            for (i = 0; i < arrayDim; ++i)
            {
                OPS::Shared::ParametricLinearExpression* expr = 
                    OPS::Shared::ParametricLinearExpression::createByAllVariables(& (arrayCall.getChild(i+1).cast_to<ExpressionBase>()));
                if (expr!=0)
                {
                    OPS::LatticeGraph::SimpleLinearExpression loopCountersCoefs;
					getExternalParamAndLoopCounterCoefficients
						(*expr, loopCounters, loopCountersCoefs, res->m_externalParamCoefs[i]);
                    if (res->m_externalParamCoefs[i].getVariables().size()>0) flagHasExtVarsOrNotLinear = true;
					for (int j=0; j<loopNumb; ++j)
						res->data[i][j] = loopCountersCoefs[j+1];
                    res->data[i][loopNumb] = loopCountersCoefs[0]; //свободный член
                    delete expr;
                }
                else    
                {
                    flagIsIndexesLinear = false;
                    break;
                }
            }
        }
		catch (OPS::RuntimeError&)
        {
            //коэффициенты линейного выражения не являются целыми числами
            flagIsIndexesLinear = false;
        }
        if (!flagIsIndexesLinear)//если нелинейность - удаляем построенные коэффициенты
        {
            res->SetStatus(NONLINEAR);
            flagHasExtVarsOrNotLinear = true;
			res->ClearData();

        }
        res->m_isIndexesLinear = flagIsIndexesLinear;
        res->m_hasExtVarsOrNotLinear = flagHasExtVarsOrNotLinear;
	}
	if (indLoops) {
		for(int j = 0; j < idOccur.m_numcycles; ++j)
			res->loops[j] = indLoops[j];
        if ((res->suppPolyhedron = OPS::LatticeGraph::GetIterationSpace(*res)) == 0)
			depGraphLog(Console::LEVEL_ERROR, _TL("\nUnexpected Error: Couldn't fill suppPolyhedron!\n","Неожиданная ошибка: невозможно инициализировать пространство итераций!"));
	}
	
	m_occurrences.push_back(res);
	// запоминаем в индексе для быстрого поиска
	m_globalIndex[occurNumb] = res;

	if (res->hasExtVarsOrNotLinear())
	{
		res->SetStatus(NONLINEAR);
		//res->SetData(idOccur.m_data, indLoops);
	}
	res->MapToOneDim();

	if (idOccur.m_generator)
		res->SetStatus(IS_GENERATOR);
	return res;
}

void OccurList::Clear()
{
	m_globalIndex.clear();

	OccurIter	first = m_occurrences.begin(),
				last  = m_occurrences.end();

	for(; first != last; ++first) {
		delete (*first);
	}
	m_occurrences.clear();
}

OccurDesc* OccurList::operator[](int index)
{
	OccursIndex::iterator	it = m_globalIndex.find(index);

	if (it != m_globalIndex.end())
		return it->second;
	else
		return 0;
}

OccurIter OccurList::Erase(const OccurIter& it) 
{ 
	m_globalIndex.erase((*it)->GetNumber());
	return m_occurrences.erase(it);
}

OccurIter OccurList::Erase(const OccurIter& first, const OccurIter& last) 
{
	OccurIter	it = first;
	for(; it != last; ++it) {
		m_globalIndex.erase((*it)->GetNumber());
	}

	return m_occurrences.erase(first, last);
}

void OccurList::SetAllUnexamined()
{
	OccurIter first = m_occurrences.begin(),last=m_occurrences.end();
	for(; first != last; ++first) {
        (*first)->ClearStatus(EXAMINED);
	}
}

OccurDesc* OccurList::FindOccurDesc(const ReferenceExpression* occur)
{
	OccurIter first = m_occurrences.begin(), last = m_occurrences.end();
	for( ;first != last; ++first) {
		if ((*first)->pOccur == occur)
			return *first;
	}

	return 0;
}

OccurIter OccurList::GetNextUnexaminedVarNew(OccurIter searchFrom,const DeclarationBase* varDecl)
{
	for(; searchFrom != m_occurrences.end(); ++searchFrom)
	{
		if ((*searchFrom)->m_varDecl == varDecl)
			if (!(*searchFrom)->isStatus(EXAMINED))
				return searchFrom;
	}
	return searchFrom;
}

OccurIter OccurList::GetNextUnexaminedElemNew(OccurIter searchFrom)
{
	for(; searchFrom != m_occurrences.end(); ++searchFrom) {
		if (!(*searchFrom)->isStatus(EXAMINED))
			return searchFrom;		
	}
	return searchFrom;
}

OccurIter IndOccurContainer::GetNextUnexaminedVarGenNew(OccurIter searchFrom, const DeclarationBase* varDecl)
{
	return indGenList.GetNextUnexaminedVarNew(searchFrom, varDecl);
}

OccurIter IndOccurContainer::GetNextUnexaminedElemGenNew(OccurIter searchFrom)
{
	return indGenList.GetNextUnexaminedElemNew(searchFrom);
}

OccurIter IndOccurContainer::GetNextUnexaminedVarUtilNew(OccurIter searchFrom, const DeclarationBase* varDecl)
{
	return indUtilList.GetNextUnexaminedVarNew(searchFrom, varDecl);
}

OccurIter IndOccurContainer::GetNextUnexaminedElemUtilNew(OccurIter searchFrom)
{
	return indUtilList.GetNextUnexaminedElemNew(searchFrom);
}

int IndOccurContainer::GetSize() const
{
	return indGenList.GetSize() + indUtilList.GetSize();
}

void IndOccurContainer::Clear()
{
	globalIndex.clear();
	indGenList.Clear();
	indUtilList.Clear();
}

OccurDesc* IndOccurContainer::operator[](int index)
{
	if (index >= 0 && index <= int(globalIndex.size()))
		return globalIndex[index];
	else
		return 0;
}

void IndOccurContainer::CreateIndex()
{
	globalIndex.clear();

	if (GetSize() == 0)
		return;

	globalIndex.resize(GetSize(), 0);

	OccurIter first = indGenList.Begin(), last = indGenList.End();
	for(; first != last; ++first) {
		globalIndex[(*first)->GetNumber()] = (*first);
	}

	first = indUtilList.Begin(), last = indUtilList.End();
	for(; first != last; ++first) {
		globalIndex[(*first)->GetNumber()] = (*first);
	}
}

OccurDesc* IndOccurContainer::FindOccurDesc(const ReferenceExpression* occur)
{
	OccurDesc* res = indGenList.FindOccurDesc(occur);
	if (res)
		return res;

	res = indUtilList.FindOccurDesc(occur);
	return res;
}
									// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool IndOccurContainer::GetFirstAndLastBlockOccurNumb(const BlockStatement& block,int& firstOccurNumb,int& lastOccurNumb)
{
	id index(const_cast<BlockStatement&>(block));

	StatementBase * currNode = index.getThisOper(), *firstAssign = 0, *lastAssign=0;
	ExpressionStatement* tempExpr = 0;
	ForStatement* tempFor = 0;
	IfStatement* tempIf = 0; 
	while(currNode)
	{
		tempExpr = dynamic_cast<ExpressionStatement*>(currNode);
		if(tempExpr != 0)
		{
			BasicCallExpression* exprAssign = dynamic_cast<BasicCallExpression*>(&tempExpr->get());
			if ( exprAssign != 0 && exprAssign->getKind() == BasicCallExpression::BCK_ASSIGN)
			{
				//Найден оператор присваивания!!!
				if (firstAssign == 0)
				{
					firstAssign = currNode;
				}
				lastAssign = currNode;
			}
			else 
			{
				depGraphLog(Console::LEVEL_WARNING, _TL("Statement OT_STMT_EXPR has \"m_pHostedExpr\" of an unexpected  type! (IndOccurContainer::GetFirstAndLast...)\n","Неожиданный тип выражения."));
			}
		}
		else
		{
			tempFor = dynamic_cast<ForStatement*>(currNode);
			if(tempFor != 0)
			{
			}
			else
			{
				tempIf = dynamic_cast<IfStatement*>(currNode);
				if(tempFor != 0)
				{
				}
				else
					depGraphLog(Console::LEVEL_WARNING, _TL("Unexpected statement type encountered! (IndOccurContainer::GetFirstAndLast...)\n","Неожиданный тип оператора."));
			}
		}

		currNode = index.next();
	}			

	if (firstAssign == 0)
		return false;//Ошибка


	// Ищем номера вхождений...
	// Ищем генератор
	bool found = false;
	OccurIter first = indGenList.Begin(), last = indGenList.End();
	for(; first != last; ++first) {
		if ((*first)->GetStatement() == firstAssign)
		{
			firstOccurNumb = (*first)->GetNumber();
			found = true;
			break;
		}
	}

	if (found == false) //Не найден генератор
		return false;

	//Ищем последнее использование (оно первое, если смотреть в обратном порядке)
	first = indUtilList.Begin();
	last = indUtilList.End();
	

	if (first == last)//значит использований вообще нет.
	{
		//тогда будем искать последний генератор...
		first = indGenList.Begin();
		last = indGenList.End();
	}

	do
	{
		last--;
		if ((*last)->GetStatement() == lastAssign)
		{
			lastOccurNumb = (*last)->GetNumber();
			return true;
		}
	}
	while(first != last);

	// Если мы сюда попали, значит не найдено использование:генератор должен был найтись полюбому.
	// Следовательно, в последнем операторе нет использований. Будем искать последний генератор:
 	first = indGenList.Begin();
	last = indGenList.End();

	do
	{
		last--;
		if ((*last)->GetStatement() == lastAssign)
		{
			lastOccurNumb = (*last)->GetNumber();
			return true;
		}
	}
	while(first != last);

	return true;
}

LampArrow::LampArrow(
	const OccurDesc& srcOccur,
	EN_DepType deptype,
	int _commonLoopNumb,
	const OccurDesc& depOccur)
	:type(deptype)	
	,srcOccurNumb(srcOccur.GetNumber())
	,depOccurNumb(depOccur.GetNumber())
	,commonLoopNumb(_commonLoopNumb)
	,srcOccurDesc(&srcOccur)
	,depOccurDesc(&depOccur)
	,pSrcStmt(srcOccur.GetStatement())
	,pDepStmt(depOccur.GetStatement()) 
	,pSrcOccur(srcOccur.pOccur)
	,pDepOccur(depOccur.pOccur)
{
}
											//================= Исправить ===================
void LamportGraph::Build(const Id::id &index, EN_LamportGraphBuildParams buildParams, bool searchOuterLoops)
{
	Clear();
	BuildVO(index, m_occurContainer, searchOuterLoops ? OLBP_SEARCH_OUTERLOOPS : OLBP_DEFAULT);
	Build(buildParams);
}

void LamportGraph::Build(OPS::Reprise::BlockStatement &block, EN_LamportGraphBuildParams buildParams, bool searchOuterLoops)
{
	Clear();
	BuildVO(id(block), m_occurContainer, searchOuterLoops ? OLBP_SEARCH_OUTERLOOPS : OLBP_DEFAULT);
	Build(buildParams);
}

void LamportGraph::Build(EN_LamportGraphBuildParams buildParams)
{
	OPS_ASSERT(conList.empty());

	OccurList& indGenList = m_occurContainer.GetIndGenList();
	OccurList& indUtilList = m_occurContainer.GetIndUtilList();

	OccurIter evl = indGenList.Begin(),
		lastL = indGenList.End(), 
		lastR = indUtilList.End();

	m_occurContainer.SetAllUnexamined();

	for(; evl != lastL; evl = indGenList.GetNextUnexaminedElemNew(evl))
	{
		const VariableDeclaration* varName = (*evl)->m_varDecl;

		OccurIter evl2 = evl;
		//int commonLoopNumb = (*evl)->loopNumb;

		BuildSelfDep(**evl, OUTPUTDEP);

		//	Обрабатываем левые вхождения...
		evl2++;
		evl2 = indGenList.GetNextUnexaminedVarNew(evl2, varName); // т.е. поиск со следующего после evl
		for(; evl2 != lastL; evl2 = indGenList.GetNextUnexaminedVarNew(evl2, varName))
		{
			BuildOutDep(**evl, **evl2);
			evl2++;
		}

		//	Обрабатываем правые вхождения...

		evl2 = indUtilList.GetNextUnexaminedVarNew(indUtilList.Begin(), varName);
		
		for(; evl2 != lastR; evl2 = indUtilList.GetNextUnexaminedVarNew(evl2, varName))
		{
			BuildFlowAntiDeps(**evl, **evl2);
			(*evl2)->SetStatus(EXAMINED);
		}

		indUtilList.SetAllUnexamined();
		(*evl)->SetStatus(EXAMINED);
	}

	// Будем искать входные зависимости (INPUTDEP)...
	if (buildParams & LMP_CONSIDER_INPUTDEP)
	{
		indUtilList.SetAllUnexamined();
		evl = indUtilList.Begin();

		// Пройдемся по списку использований
		for(; evl != lastR; evl = indUtilList.GetNextUnexaminedElemNew(evl))
		{
			const VariableDeclaration* varName = (*evl)->m_varDecl;
			OccurIter evl2 = evl;

			// проверка случая входной самозависимости вхождения evl...
			BuildSelfDep(**evl, INPUTDEP);

			// Обрабатываем правые вхождения...
			evl2++;
			evl2 = indUtilList.GetNextUnexaminedVarNew(evl2, varName); // т.е. поиск со следующего после evl
			for(; evl2 != lastR; evl2 = indUtilList.GetNextUnexaminedVarNew(evl2,varName))
			{
				BuildInDep(**evl, **evl2);
				evl2++;
			}
			(*evl)->SetStatus(EXAMINED);
		}
	}
	RefineByDepGraphEx();
}

void LamportGraph::BuildFlowAntiDeps(const OccurDesc& first, const OccurDesc& second)
{
	const int commonLoopNumb = getCommonUpperLoopsAmount(first, second);
	const bool rel = PrecedesNonStrictly(second, first);

	if (!(!first.hasExtVarsOrNotLinear() && !second.hasExtVarsOrNotLinear())) // i.e. indexes are NOT linear form AND it's a array
	{
		if (commonLoopNumb)
		{
			AddRow(first, FLOWDEP, commonLoopNumb, second)->PushSupp(ANYSUPP);
			AddRow(second, ANTIDEP, commonLoopNumb, first)->PushSupp(ANYSUPP);
		}
		else //нет цикла, содержащего оба вхождения
		{
			//bool rel = PrecedesNonStrictly(second, first);

			if(rel) 
			{
				AddRow(second, ANTIDEP, commonLoopNumb, first)->PushSupp(LIDEP);
			}
			else
			{
				AddRow(first, FLOWDEP, commonLoopNumb, second)->PushSupp(LIDEP);//ОТКОММЕНТИРОВАТЬ, если нужен будет "такой" носитель вместо вообще отсутствия какого-либо носителя
			}
		}
	}
	else
	{
		if(second.IsArray())
		{
			if(commonLoopNumb)
			{
				// сначала идет оператор содержащий evl2, затем содержащий evl
				if(rel) {
					LampArrow* justAddedCon = 0;
					//int k = 0;
					for(int k = 0; (k = GetDepSupp(first,second,k,commonLoopNumb)) != NODEP; ++k)
					{
						if (k == LIDEP) {
							break;
						}
						else {
							if (justAddedCon == 0) {
								justAddedCon = AddRow(first, FLOWDEP, commonLoopNumb, second);
							}
							justAddedCon->PushSupp(k);
						}
					}
					// CONVERSELY   (наоборот)
					justAddedCon = 0;
					for(int k = 0; (k = GetDepSupp(second,first,k,commonLoopNumb))!=NODEP; ++k)
					{
						if (justAddedCon == 0)
						{
							justAddedCon = AddRow(second, ANTIDEP, commonLoopNumb, first);
						}
						justAddedCon->PushSupp(k);

						if (k == LIDEP)
							break;
					}
				}
				else//сначала идет оператор содержащий evl, затем содержащий evl2
				{
					LampArrow* justAddedCon = 0;
					for(int k = 0; (k = GetDepSupp(first, second, k, commonLoopNumb))!=NODEP; ++k)
					{
						if (justAddedCon == 0)
						{
							justAddedCon = AddRow(first,FLOWDEP,commonLoopNumb,second);
						}
						justAddedCon->PushSupp(k);

						if (k == LIDEP)break;
					}
					// CONVERSELY	
					justAddedCon = 0;
					for(int k = 0; (k=GetDepSupp(second,first,k,commonLoopNumb))!=NODEP; ++k)
					{		
						if (k == LIDEP)
						{
							break;
						}
						else 
						{
							if (justAddedCon == 0)
							{
								justAddedCon = AddRow(second, ANTIDEP, commonLoopNumb, first);
							}
							justAddedCon->PushSupp(k);
						}
					}						
				}
			}
			else//нет цикла, содержащего оба вхождения
			{
				//флаги, указывающие есть ли счетчики циклов в индексных выражениях вхождений evl и evl2
				bool evlHasLoopVarInSubscript = false, evl2HasLoopVarInSubscript = false,
						freeCoefsAreEqual = false;

				if (first.d1)
				{
					for(int i=0;i<first.loopNumb;i++)
						if(first.d1[i])
						{//т.е. во вхождении evl имеется счетчик цикла в индексном выражении
							evlHasLoopVarInSubscript = true;
							break;
						}
				}
				else
					evlHasLoopVarInSubscript = true;

				if (second.d1)
				{
					for(int i=0;i<second.loopNumb;i++)
						if (second.d1[i])
						{//т.е. во вхождении evl имеется счетчик цикла в индексном выражении
							evl2HasLoopVarInSubscript = true;
							break;
						}
				}
				else
					evl2HasLoopVarInSubscript = true;

				if (first.d1 && second.d1)
				{
					freeCoefsAreEqual = first.d1[first.loopNumb]==second.d1[second.loopNumb];
				}
				else
					freeCoefsAreEqual = true;

				if (evlHasLoopVarInSubscript || evl2HasLoopVarInSubscript
					|| freeCoefsAreEqual)
				{
					//т.е. если существует счетчик циклов в индексном выражении некоторого вхождения
					// либо свободные члены индексных выражений совпадают, то
					if (rel)
						AddRow(second, ANTIDEP, commonLoopNumb, first)->PushSupp(LIDEP);
					else
						AddRow(first, FLOWDEP, commonLoopNumb, second)->PushSupp(LIDEP);
				}
			}
		}
		else
		{
			if (rel) {
				// Откоментир. эту строку чтобы анти зависимость вида x=x+... отсутствовала в графе
				AddRow(second, ANTIDEP, commonLoopNumb, first)->PushSupp(LIDEP);
				// CONVERSELY...
				if(commonLoopNumb) {
					AddRow(first, FLOWDEP, commonLoopNumb, second)->PushSupp(ANYSUPP);
				}
			}
			else {
				AddRow(first, FLOWDEP, commonLoopNumb, second)->PushSupp(LIDEP);
				// CONVERSELY...
				if(commonLoopNumb) {
					AddRow(second, ANTIDEP, commonLoopNumb, first)->PushSupp(ANYSUPP);
				}
			}
		}
	}
}

void LamportGraph::BuildInDep(const OccurDesc& first, const OccurDesc& second)
{
	int commonLoopNumb = getCommonUpperLoopsAmount(first, second);

	if (first.IsArray() && second.IsArray())
	{
		if (!first.hasExtVarsOrNotLinear() && !second.hasExtVarsOrNotLinear()) // i.e. indexes're linear form
		{
			if (commonLoopNumb)
			{
				LampArrow* justAddedCon = 0;
				for(int k = 0; (k = GetDepSupp(first, second, k, commonLoopNumb)) != NODEP; ++k)
				{
					if (justAddedCon == 0)
					{
						justAddedCon = AddRow(first, INPUTDEP, commonLoopNumb, second);
					}
					justAddedCon->PushSupp(k);

					if (k == LIDEP)
						break;
				}

				// CONVERSELY	(наоборот)
				justAddedCon = 0;
				for(int k = 0; (k = GetDepSupp(second,first,k,commonLoopNumb)) != NODEP; ++k)
				{
					// Для LIDEP здесь должно быть также точно, как и в случае выше (здесь уже нет разници: оба вхождения использования)
					if (justAddedCon == 0) {
						justAddedCon = AddRow(second, INPUTDEP, commonLoopNumb, first);
					}
					justAddedCon->PushSupp(k);

					if (k == LIDEP)
						break;
				}
			}
			else //нет цикла, содержащего оба вхождения
			{
				// флаги, указывающие есть ли счетчики циклов в индексных выражениях вхождений evl и evl2

				bool evlHasLoopVarInSubscript = false, evl2HasLoopVarInSubscript = false,
						freeCoefsAreEqual = false;
				if (first.d1)
				{
					for(int i = 0; i < first.loopNumb; ++i) {
						if (first.d1[i]) {
							//т.е. во вхождении evl имеется счетчик цикла в индексном выражении
							evlHasLoopVarInSubscript = true;
							break;
						}
					}
				}
				else
					evlHasLoopVarInSubscript = true;

				if (second.d1)
				{
					for(int i = 0; i < second.loopNumb; ++i)
					{
						if (second.d1[i]) {
								//т.е. во вхождении evl имеется счетчик цикла в индексном выражении
								evl2HasLoopVarInSubscript = true;
								break;
						}
					}
				}
				else
					evl2HasLoopVarInSubscript = true;

				if (first.d1 && second.d1)
				{
					freeCoefsAreEqual = first.d1[first.loopNumb] == second.d1[second.loopNumb];
				}
				else
					freeCoefsAreEqual = true;


				if (evlHasLoopVarInSubscript || evl2HasLoopVarInSubscript
					|| freeCoefsAreEqual)
				{
					AddRow(first, INPUTDEP, commonLoopNumb, second)->PushSupp(LIDEP);
				}
			}
		}
		else
		{
			if (commonLoopNumb)
			{
				AddRow(first, INPUTDEP, commonLoopNumb, second)->PushSupp(ANYSUPP);
				// CONVERSELY	(наоборот)
				AddRow(second, INPUTDEP, commonLoopNumb, first)->PushSupp(ANYSUPP);
			}
			else 
			{ //нет цикла, содержащего оба вхождения
				AddRow(first, INPUTDEP, commonLoopNumb, second)->PushSupp(LIDEP);
			}
		}
	}	
	else 
	{	// Это скалярная переменная
		AddRow(first, INPUTDEP, commonLoopNumb, second)->PushSupp(LIDEP);
		// CONVERSELY
		if (commonLoopNumb) {
			AddRow(second, INPUTDEP, commonLoopNumb, first)->PushSupp(ANYSUPP);
		}
	}
}

void LamportGraph::BuildOutDep(const OccurDesc& first, const OccurDesc& second)
{
	int commonLoopNumb = getCommonUpperLoopsAmount(first, second);
	if (second.IsArray())	//	т.е. если это массив
	{
		if (!first.hasExtVarsOrNotLinear() && !second.hasExtVarsOrNotLinear()) // i.e. indexes're lenear form
		{
			// Если у вхождений есть общие циклы
			if (commonLoopNumb)
			{
				LampArrow* justAddedCon = 0;
				for(int k = 0; (k = GetDepSupp(first, second, k, commonLoopNumb)) != NODEP; ++k)
				{
					if (justAddedCon == 0) 
					{
						justAddedCon = AddRow(first, OUTPUTDEP, commonLoopNumb, second);
					}
					justAddedCon->PushSupp(k);

					if (k == LIDEP)
						break;
				}

				// CONVERSELY	(наоборот)
				justAddedCon = 0;
				for(int k = 0; (k = GetDepSupp(second, first, k, commonLoopNumb)) != NODEP; ++k)
				{
					if (k == LIDEP) 
					{
						break;
					}
					else
					{
						if (justAddedCon == 0)
						{
							justAddedCon = AddRow(second, OUTPUTDEP, commonLoopNumb, first);
						}
						justAddedCon->PushSupp(k);
					}
				}
			}
			else
			{	//нет цикла, содержащего оба вхождения
				// флаги, указывающие есть ли счетчики циклов в индексных выражениях вхождений evl и evl2
				bool evlHasLoopVarInSubscript = false, evl2HasLoopVarInSubscript = false,
					freeCoefsAreEqual = false;

				if (first.d1)
				{
					for(int i = 0; i < first.loopNumb; ++i)
						if (first.d1[i])
						{
							// т.е. во вхождении evl имеется счетчик цикла в индексном выражении
							evlHasLoopVarInSubscript = true;
							break;
						}
				}
				else
					evlHasLoopVarInSubscript = true;

				if (second.d1)
				{
					for(int i = 0; i < second.loopNumb; ++i)
						if (second.d1[i])
						{
							// т.е. во вхождении evl имеется счетчик цикла в индексном выражении
							evl2HasLoopVarInSubscript = true;
							break;
						}
				}
				else
					evl2HasLoopVarInSubscript = true;

				if (first.d1 && second.d1)
				{
					freeCoefsAreEqual = first.d1[first.loopNumb]==second.d1[second.loopNumb];
				}
				else
					freeCoefsAreEqual = true;

				if ((evlHasLoopVarInSubscript || evl2HasLoopVarInSubscript)
					|| freeCoefsAreEqual)
				{
					AddRow(first, OUTPUTDEP, commonLoopNumb, second)->PushSupp(LIDEP);
				}
			}
		}
		else
		{
			// если вхождения лежат в теле одного цикла
			if(commonLoopNumb)
			{
				AddRow(first, OUTPUTDEP, commonLoopNumb, second)->PushSupp(ANYSUPP);
				// CONVERSELY	(наоборот)
				AddRow(second, OUTPUTDEP, commonLoopNumb, first)->PushSupp(ANYSUPP);
			}
			else
			{ // нет цикла, содержащего оба вхождения
				AddRow(first, OUTPUTDEP, commonLoopNumb, second)->PushSupp(LIDEP);
			}
		}
	}
	else {	// Это скаляр
		AddRow(first, OUTPUTDEP, commonLoopNumb, second)->PushSupp(LIDEP);
		// CONVERSELY
		if (commonLoopNumb)
		{
			AddRow(second, OUTPUTDEP, commonLoopNumb, first)->PushSupp(ANYSUPP);
		}
	}
}

void LamportGraph::BuildSelfDep(const OccurDesc& desc, EN_DepType depType)
{
	const int commonLoopNumb = desc.loopNumb;

	// проверка случая входной/выходной самозависимости вхождения
	if (desc.IsArray()) //	если это массив
	{
		if (!desc.hasExtVarsOrNotLinear()) // i.e. indexes're lenear form
		{
			int k = 0;
			LampArrow* justAddedCon = 0;
			while((k = GetDepSupp(desc, desc, k, commonLoopNumb)) != NODEP)
			{
				if (k == LIDEP)
					break;

				if (justAddedCon == 0) {
					justAddedCon = AddRow(desc, depType, commonLoopNumb, desc);
				}
				justAddedCon->PushSupp(k);
				k++;
			}
		}
		else // indexes aren't lenear form
		{
			if (desc.loopNumb)//т.е. переменная находится в гнезде циклов
			{
				AddRow(desc, depType, commonLoopNumb, desc)->PushSupp(ANYSUPP);
			}
		}
	}
	else // т.е. скалярная переменная
	{
		if (desc.loopNumb)//т.е. переменная находится в гнезде циклов
		{
			AddRow(desc, depType, commonLoopNumb, desc)->PushSupp(ANYSUPP);
		}
	}
}

// Возвращает true, если массив располагается в памяти по строкам
// Возвращает false, если по столбцам
bool isCStyleArray(ReferenceExpression& expr)
{
	if (TranslationUnit* unit = expr.findTranslationUnit())
	{
		return unit->getSourceLanguage() == TranslationUnit::SL_C;
	}
	return true; // По-умолчанию все массивы сишные
}

void OccurDesc::MapToOneDim()
{
	if (!data) return;  // Матрица коэффициентов не определена

	// Для многомерных массивов (больше 1 размерности) необходимо знать верхние границы
	if (dim > 1 && ub.empty())   return;

	if (d1)
		delete[] d1;

	m_externalParamCoefsMapped.clear();

	d1 = new int[loopNumb + 1];
	for(int i = 0; i <= loopNumb; ++i)
		d1[i] = 0;

	int mult = 1;

	const bool isCArray = isCStyleArray(*pOccur);

	for(int idx = 0; idx < dim; ++idx)
	{
		const int j = isCArray ? dim - idx - 1 : idx;
		for(int i = 0; i <= loopNumb; ++i)
			d1[i] += mult*data[j][i];
		d1[loopNumb] -= mult*0;		//ВЕЗДЕ ГДЕ "0" нужно "lb[j]"
		m_externalParamCoefsMapped.add(m_externalParamCoefs[j] * mult);

		if (!ub.empty())
			mult *= ub[j] - 0 + 1;				// -- // --
	}
}

bool IsLoopsBoundsUnknown(const LoopDesc* loops, size_t loopsCount)
{
	for(size_t i = 0; i < loopsCount; ++i) 
	{
		if (loops[i].lBoundUnk || loops[i].rBoundUnk)
		{
			return true;
		}
	}
	return false;
}

bool GCDTestMapped(const OccurDesc& p, const OccurDesc& q, int supp)
{
	OPS_ASSERT(!p.isStatus(UNKNOWN_BOUNDS) && !q.isStatus(UNKNOWN_BOUNDS));

	int gcd = 0;

	for(int j = 0; j < supp; ++j)
        gcd = OPS::LatticeGraph::calculateGCD(gcd, p.d1[j] - q.d1[j]);

	for(int j = supp; j < p.loopNumb; ++j)
        gcd = OPS::LatticeGraph::calculateGCD(gcd, p.d1[j]);

	for(int j = supp; j < q.loopNumb; ++j)
        gcd = OPS::LatticeGraph::calculateGCD(gcd, q.d1[j]);

	if(gcd)
	{
		if ((p.d1[p.loopNumb] - q.d1[q.loopNumb]) % gcd)
			return false;
	}
	return true;
}

bool GCDTestUnmapped(const OccurDesc& p, const OccurDesc& q, int supp)
{
	if (p.dim != q.dim)
		return false;

	bool isDepended = true;
	int gcd = 0;

	for(int i=0; i<p.dim && isDepended; i++)
	{
		for(int j = 0; j < supp; ++j)
            gcd = OPS::LatticeGraph::calculateGCD(gcd, p.data[i][j] - q.data[i][j]);

		for(int j = supp; j < p.loopNumb; ++j)
            gcd = OPS::LatticeGraph::calculateGCD(gcd, p.data[i][j]);

		for(int j = supp; j < q.loopNumb; ++j)
            gcd = OPS::LatticeGraph::calculateGCD(gcd, q.data[i][j]);

		if (gcd)
		{
			if ((p.data[i][p.loopNumb] - q.data[i][q.loopNumb]) % gcd)
				isDepended = false;
		}
		//isDepended = true;
	}
	return isDepended;
}

// НОД-тест. Возвращает false, если зависимости нет.
bool GCDTest(const OccurDesc& p, const OccurDesc& q, int supp)
{
	if(!p.isStatus(UNKNOWN_BOUNDS) && !q.isStatus(UNKNOWN_BOUNDS) &&
		   p.d1 != 0 && q.d1 != 0)
	{
		return GCDTestMapped(p, q, supp);
	}
	else
	{
		return GCDTestUnmapped(p, q, supp);
	}
}

/// Релизация теста Банержи для определения циклически порожденных и независимых зависимостей.
/// Т.к. тесты очень похожи, то их реализации объединены в одну функцию.
/// Номер носителя меньше количества общих циклов, то выполняется тест на циклически
/// порожденную зависимость. Если же номер носителя равен количеству общих циклов, то
/// выполняется тест на циклически независимую зависимость.
/// Формулы тестов взяты из статьи Randy Allen и Ken Kennedy p491-allen.pdf
///   стр. 511 - циклически порожденные
///   стр. 512 - циклически независимые

bool BanerjeeTestMapped(const OccurDesc& p, const OccurDesc& q, int carrier, int commonLoopNumb)
{
	OPS_ASSERT(!p.isStatus(UNKNOWN_BOUNDS) && !q.isStatus(UNKNOWN_BOUNDS));

	int sl1 = 0, sl2 = 0, sl3 = 0,
		sr1 = 0, sr2 = 0, sr3 = 0,
		sa = 0, sb = 0;

	// Определяем какой тест выполнять.
	// lid - loop independent dependence
	const bool lidTest = carrier == commonLoopNumb;

	for(int i = 0; i < carrier; ++i)
	{
		sl1 += NEG(p.d1[i]-q.d1[i])*(p.loops[i].u-1);
		sr1 += POS(p.d1[i]-q.d1[i])*(p.loops[i].u-1);
	}

	for(int i = (lidTest ? carrier : carrier + 1); i < q.loopNumb; ++i)
	{
		sl3 += POS(q.d1[i])*(q.loops[i].u-1);
		sr3 += NEG(q.d1[i])*(q.loops[i].u-1);
	}

	for(int i = (lidTest ? carrier : carrier + 1); i < p.loopNumb; ++i)
	{
		sl2 += NEG(p.d1[i])*(p.loops[i].u-1);
		sr2 += POS(p.d1[i])*(p.loops[i].u-1);
	}

	for(int i = 0; i <= p.loopNumb; ++i)
		sa += p.d1[i];
	for(int i = 0; i <= q.loopNumb; ++i)
		sb += q.d1[i];

	int l = -sl1 - sl2 - sl3,
		r = sr1 + sr2 + sr3;

	if (lidTest)
	{
		// проверка на циклически независимую зависимость
		return l <= (sb-sa) && r >= (sb-sa);
	}
	else
	{
		// проверка на циклически порожденную зависимость
		return ((l-q.d1[carrier]-POS(NEG(p.d1[carrier])+q.d1[carrier])*(p.loops[carrier].u-2))<=(sb-sa) &&
			(r-q.d1[carrier]+POS(POS(p.d1[carrier])-q.d1[carrier])*(p.loops[carrier].u-2))>=(sb-sa));
	}
}

bool BanerjeeTestUnmapped(const OccurDesc& p, const OccurDesc& q, int carrier, int commonLoopNumb)
{
	if(p.dim != q.dim)
		return false;

	int sl1 = 0, sl2 = 0, sl3 = 0,
		sr1 = 0, sr2 = 0, sr3 = 0,
		sa = 0, sb = 0;

	bool isDepended = true;
	for (int j=0; j<p.dim && isDepended; j++)
	{
		// Определяем какой тест выполнять.
		// lid - loop independent dependence
		const bool lidTest = carrier == commonLoopNumb;

		for(int i = 0; i < carrier; ++i)
		{
			sl1 += NEG(p.data[j][i]-q.data[j][i])*(p.loops[i].u-1);
			sr1 += POS(p.data[j][i]-q.data[j][i])*(p.loops[i].u-1);
		}

		for(int i = (lidTest ? carrier : carrier + 1); i < q.loopNumb; ++i)
		{
			sl3 += POS(q.data[j][i])*(q.loops[i].u-1);
			sr3 += NEG(q.data[j][i])*(q.loops[i].u-1);
		}

		for(int i = (lidTest ? carrier : carrier + 1); i < p.loopNumb; ++i)
		{
			sl2 += NEG(p.data[j][i])*(p.loops[i].u-1);
			sr2 += POS(p.data[j][i])*(p.loops[i].u-1);
		}

		for(int i = 0; i <= p.loopNumb; ++i)
			sa += p.data[j][i];
		for(int i = 0; i <= q.loopNumb; ++i)
			sb += q.data[j][i];

		int l = -sl1 - sl2 - sl3,
			r = sr1 + sr2 + sr3;

		if (lidTest)
		{
			// проверка на циклически независимую зависимость
			isDepended = l <= (sb-sa) && r >= (sb-sa);
		}
		else
		{
			// проверка на циклически порожденную зависимость
			isDepended = ((l-q.data[j][carrier]-POS(NEG(p.data[j][carrier])+q.data[j][carrier])*(p.loops[carrier].u-2))<=(sb-sa) &&
				(r-q.data[j][carrier]+POS(POS(p.data[j][carrier])-q.data[j][carrier])*(p.loops[carrier].u-2))>=(sb-sa));
		}
	}
	return isDepended;
}

bool BanerjeeTest(const OccurDesc& p, const OccurDesc& q, int carrier, int commonLoopNumb)
{
	if (!p.isStatus(UNKNOWN_BOUNDS) && !q.isStatus(UNKNOWN_BOUNDS) &&
			p.d1 != 0 && q.d1 != 0)
	{
		return BanerjeeTestMapped(p, q, carrier, commonLoopNumb);
	}
	else
	{
		return BanerjeeTestUnmapped(p, q, carrier, commonLoopNumb);
	}
}

int LamportGraph::GetDepSupp(const OccurDesc& p, const OccurDesc& q, int start, int commonLoopNumb)
{
	// Вхождения одной переменной
	if (p.m_varDecl != q.m_varDecl)
		return NODEP;

	// Есть охватывающие циклы
	if (!p.loops || !q.loops)
		return NODEP;

	// Известны ли границы циклов
	bool loopBoundsUnknown =  IsLoopsBoundsUnknown(p.loops, commonLoopNumb) ||
							IsLoopsBoundsUnknown(q.loops, commonLoopNumb);

	for(int k = start; k <= commonLoopNumb; ++k)
	{
		if (!GCDTest(p, q, k))
			continue;

		if (loopBoundsUnknown)
		{
			PredicatePtr pred( banerjeeTestSymbolicUnmapped(p, q, k, commonLoopNumb) );
			if (isFalse(*pred))
				continue; // нет зависимости
			else
				return k == commonLoopNumb ? LIDEP : k;
		}
		else
		{
			if (BanerjeeTest(p, q, k, commonLoopNumb)) {
				return k == commonLoopNumb ? LIDEP : k;
			}
		}
	}
	return NODEP;
}

bool DepPreventsLoopInterchange(const OccurDesc& p, const OccurDesc& q, int k)
{
	if (p.m_varDecl != q.m_varDecl)
		return false;

	if (!p.d1 || !q.d1) return true;
	if (!p.loops || !q.loops)return false;
	// а вообще, странно, что для этих вхождений была вызвана эта ф-ция

	const int commonLoopNumb = getCommonUpperLoopsAmount(p, q);

	if (k >= (commonLoopNumb - 1))
		return true;
//	Banerjee...

	int sl1 = 0, sl2 = 0, sl3 = 0, 
		sr1 = 0, sr2 = 0, sr3 = 0,
		sa = 0 , sb = 0;
	
	for(int i = 0; i < k; ++i) {
		sl1 += NEG(p.d1[i]-q.d1[i]) * (p.loops[i].u-1);
		sr1 += POS(p.d1[i]-q.d1[i]) * (p.loops[i].u-1);
	}

  	for(int i = k + 2; i < q.loopNumb; ++i) {
		sl3 += POS(q.d1[i]) * (q.loops[i].u-1);
		sr3 += NEG(q.d1[i]) * (q.loops[i].u-1);
	}

	for(int i = k + 2; i < p.loopNumb; ++i) {
		sl2 += NEG(p.d1[i])*(p.loops[i].u-1);
		sr2 += POS(p.d1[i])*(p.loops[i].u-1);
	}

	for(int i = 0; i <= p.loopNumb; ++i)
		sa += p.d1[i];

	for(int i = 0; i <= q.loopNumb; ++i)
		sb += q.d1[i];
	int l = -sl1 - sl2 - sl3 + p.d1[k+1] - q.d1[k] - POS(NEG(p.d1[k])+q.d1[k])*(p.loops[k].u-2)-NEG(p.d1[k+1]-POS(q.d1[k+1]))*(p.loops[k+1].u-2),
		r =  sr1 + sr2 + sr3 + p.d1[k+1] - q.d1[k] + POS(POS(p.d1[k])-q.d1[k])*(p.loops[k].u-2)+POS(p.d1[k+1]+NEG(q.d1[k+1]))*(p.loops[k+1].u-2);

	return (l<=(sb-sa))&&((sb-sa)<=r);
}

bool DepPreventsLoopInterchangeEx(const OccurDesc& srcEntry, const OccurDesc& depEntry,int k)
{
	if (srcEntry.m_varDecl != depEntry.m_varDecl)
		return false;

	if ((!(!srcEntry.hasExtVarsOrNotLinear() && !depEntry.hasExtVarsOrNotLinear()) && (srcEntry.IsArray())))//if nonlinear indexis
		return true;

	if (srcEntry.suppPolyhedron == 0 ||
		depEntry.suppPolyhedron == 0) 
		return true;

	bool depPreventsLoopInterchange = false;
	int addedForTest = 2*k + 2;
	
	//General Init...
	int commonLoopsNumb = getCommonUpperLoopsAmount(srcEntry, depEntry);

	if (k >= commonLoopsNumb - 1) {
		//скорее всего не правильно поданы данные на вход
		return false;
	}

	int varNum = srcEntry.loopNumb;
	//int paramNum = depEntry.loopNumb, paramVarNum = depEntry.loopNumb;
	int* ubs = new int[varNum];
	int* lbs = new int[varNum];

	for(int i = 0; i < varNum; ++i) {
		ubs[i] = srcEntry.loops[i].u;
		lbs[i] = srcEntry.loops[i].l;
	}

	std::list<LatticeGraph::TreeNode*> solTabs;

	// Строим контекст, описывающий пространство итераций гнезда циклов
	LatticeGraph::Polyhedron* iterationSpaceForDep = depEntry.suppPolyhedron;

	// end of строим контекст
	//End of init

	int eqNum = commonLoopsNumb;

	if (!PrecedesStrictly(srcEntry, depEntry))
		--eqNum;


	for(;eqNum>=0;eqNum--)
	{
		int tabDimj = srcEntry.loopNumb + 1 + depEntry.loopNumb;
		int tabDimi = 2*varNum + srcEntry.dim*2 + eqNum*2;

		if (eqNum != commonLoopsNumb)
			++tabDimi;

		int currAdd = tabDimi;

		tabDimi += srcEntry.suppPolyhedron->GetSize();

		LatticeGraph::TreeNode* newRoot = new LatticeGraph::TreeNode();
		LatticeGraph::Tableau* tab = newRoot->tableau = new LatticeGraph::Tableau(tabDimi+addedForTest,tabDimj,varNum,0);
		newRoot->context = new LatticeGraph::Polyhedron(*iterationSpaceForDep);

		// Filling new starting Tableau (tab)

		// Неравенства, соотв. верхн и нижн границам счетч.

	/*
		for(int i=0;i<srcEntry.loopNumb;i++)
		{
			int s;
			// for m_lower bounds
			tab->data[i][i]=1;
			if(i)
			{
				s=-ubs[i]+ubExprForSrc[i][0];
				for(int j=0;j<i;j++)
				{
					s+=ubExprForSrc[i][j+1]*ubs[j];
					tab->data[currAdd][j]=-ubExprForSrc[i][j+1];
				}
			
				tab->data[currAdd][srcEntry.loopNumb]=s;
				tab->data[currAdd][i]=1;
				currAdd++;
			}

			// for m_upper bounds 
			tab->data[i+srcEntry.loopNumb][i]=-1;
			tab->data[i+srcEntry.loopNumb][srcEntry.loopNumb]=ubs[i]-lbExprForSrc[i][0];

			if(i)
			{
				s=ubs[i]-lbExprForSrc[i][0];
				for(int j=0;j<i;j++)
				{
					s-=-lbExprForSrc[i][j+1]*ubs[j];
					tab->data[currAdd][j]=lbExprForSrc[i][j+1];
				}
			
			
					tab->data[currAdd][i]=-1;
					tab->data[currAdd][srcEntry.loopNumb]=s;
					currAdd++;
			}

		}
	*/

		for(int i = 0; i < srcEntry.loopNumb; ++i) {
			tab->data[i][i] = 1;
			tab->data[i+srcEntry.loopNumb][i] = -1;
			tab->data[i+srcEntry.loopNumb][srcEntry.loopNumb] = ubs[i];
		}

        std::list<OPS::LatticeGraph::Inequality*>::iterator inqIter = srcEntry.suppPolyhedron->m_ins.begin();
		for(int i = 0; i < srcEntry.suppPolyhedron->GetSize(); ++i) {
            OPS::LatticeGraph::Inequality* tempInq = *inqIter;
			int s = 0;
			for(int j = 0; j < (tempInq->m_dim-1); ++j) {
				tab->data[currAdd][j] = -tempInq->m_coefs[j+1];
				s += ubs[j]*tempInq->m_coefs[j+1];
			}
			
			tab->data[currAdd][srcEntry.loopNumb] = s + tempInq->m_coefs[0];
			currAdd++;
			inqIter++;
		}

		// Неравенства, соотв. альтерн. многогрн.
		for(int i = 0; i < eqNum; ++i) {
			tab->data[2*i+srcEntry.loopNumb*2][i] = -1;
			tab->data[2*i+srcEntry.loopNumb*2][srcEntry.loopNumb+i+1] = -1;
			tab->data[2*i+srcEntry.loopNumb*2][srcEntry.loopNumb] = ubs[i];
			
			tab->data[2*i+srcEntry.loopNumb*2+1][i] = 1;
			tab->data[2*i+srcEntry.loopNumb*2+1][srcEntry.loopNumb+i+1] = 1;
			tab->data[2*i+srcEntry.loopNumb*2+1][srcEntry.loopNumb] = -ubs[i];
		}
		
		if (eqNum != commonLoopsNumb) {
			//то необх добавить еще одно неравенство
			tab->data[2*srcEntry.loopNumb+2*eqNum][eqNum] = 1;
			tab->data[2*srcEntry.loopNumb+2*eqNum][srcEntry.loopNumb+eqNum+1] = 1;
			tab->data[2*srcEntry.loopNumb+2*eqNum][srcEntry.loopNumb] = -ubs[eqNum] - 1;
		}

		int offset;

		if (eqNum == srcEntry.loopNumb) {
			offset = srcEntry.loopNumb*2+commonLoopsNumb*2;// WARNING !
		}
		else {
			if(commonLoopsNumb)
				offset = 2*srcEntry.loopNumb+2*eqNum+1;
			else
				offset = 2*srcEntry.loopNumb;
		}

		for(int i = 0; i < srcEntry.dim; ++i)
		{
			int s1 = 0;
			for(int j = 0; j < srcEntry.loopNumb; ++j)
			{
				tab->data[2*i+offset][j] = -srcEntry.data[i][j];
				s1 += srcEntry.data[i][j]*ubs[j];
				tab->data[2*i+offset][j+srcEntry.loopNumb+1] = -depEntry.data[i][j];
				
				tab->data[2*i+offset+1][j] = srcEntry.data[i][j];
				tab->data[2*i+offset+1][j+srcEntry.loopNumb+1] = depEntry.data[i][j];
			}
			tab->data[2*i+offset][srcEntry.loopNumb] = srcEntry.data[i][srcEntry.loopNumb]-depEntry.data[i][depEntry.loopNumb]+s1;
			tab->data[2*i+offset+1][srcEntry.loopNumb] = -srcEntry.data[i][srcEntry.loopNumb]+depEntry.data[i][depEntry.loopNumb]-s1;
		}

		//Дописываем строки, необходимые для теста возможности перестановки циклов...

		for(int i=0;i<k;i++)
		{
			tab->data[2*i+tabDimi][i]=-1;tab->data[2*i+tabDimi][srcEntry.loopNumb+i+1]=-1;tab->data[2*i+tabDimi][srcEntry.loopNumb]=ubs[i];
			tab->data[2*i+tabDimi+1][i]=1;tab->data[2*i+tabDimi+1][srcEntry.loopNumb+i+1]=1;tab->data[2*i+tabDimi+1][srcEntry.loopNumb]=-ubs[i];
		}

		tab->data[2*k+tabDimi][k]=1;tab->data[2*k+tabDimi][srcEntry.loopNumb+k+1]=1;tab->data[2*k+tabDimi][srcEntry.loopNumb]=-ubs[k]-1;//i'<i
		tab->data[2*k+tabDimi+1][k+1]=-1;tab->data[2*k+tabDimi+1][srcEntry.loopNumb+k+2]=-1;tab->data[2*k+tabDimi+1][srcEntry.loopNumb]=ubs[k+1]-1;//j'>j

		// конец дописывания строк для теста

		// End of filling
		
		tab->Simplify();
        OPS::LatticeGraph::CalcTableauSigns(tab,newRoot->context);
	/*
	#ifdef LATTICE_TABLES_DEBUG
		file<<"Starting table:\n"<<*tab;
	#endif
	*/
		newRoot->d=1;
		ApplySimplex(newRoot,solTabs);
		delete newRoot;

		if(solTabs.size())
		{
			depPreventsLoopInterchange = true;
			break;
		}
	}

	delete[] ubs;
	delete[] lbs;

    std::list<OPS::LatticeGraph::TreeNode*>::iterator first = solTabs.begin(), last = solTabs.end();
	for(; first != last; ++first) {
		delete *first;
	}

	return depPreventsLoopInterchange;
}

int LoopsAreInterchangable(LamportGraph& lg,int k)
{
	LampArrowIterator first=lg.Begin(),last=lg.End();
	
	while(first!=last)
	{
//		if((*first)->supp==k)
		if((*first)->TestSupp(k))
		{
			const OccurDesc* sE,*dE;

			sE=(*first)->srcOccurDesc;

			dE=(*first)->depOccurDesc;
			
			if((!sE)||(!dE))return 0;
			if(DepPreventsLoopInterchange(*sE,*dE,k))return 0;
		}
//		else
//			if((*first)->supp==ANYSUPP)return 0;
		first++;
	}
	return 1;
}

int LoopsAreInterchangableEx(LamportGraph& lg,int k)
{
	LampArrowIterator first=lg.Begin(),last=lg.End();
	
	while(first!=last)
	{
//		if((*first)->supp==k)
		{
			const OccurDesc* sE,*dE;

			sE=(*first)->srcOccurDesc;

			dE=(*first)->depOccurDesc;

			if((!sE)||(!dE))return 0;
			if(DepPreventsLoopInterchangeEx(*sE,*dE,k))return 0;
		}
		first++;
	}
	return 1;
}


class VariableListBuilder : public OPS::Reprise::Service::DeepWalker
{
	std::vector<ExpressionBase*>	m_loopItersList;
	std::vector<OccurrenceInfo>	m_varList;								//===============! Изменить !===================

	std::stack<bool> m_inGeneratorStack;
	std::stack<bool> m_inSubscriptStack;

	bool isInsideGenerator() const { return m_inGeneratorStack.top(); }
	bool isInsideSubscript() const { return m_inSubscriptStack.top(); }

	void visit(ReferenceExpression&);
	void visit(Canto::HirCCallExpression&);
	void visit(BasicCallExpression&);
	void visit(SubroutineCallExpression&);
	void visit(Canto::HirFIntrinsicCallExpression&);

public:

	VariableListBuilder(const std::vector<LoopInfo>& loopsList);
	void ReleaseList(std::vector<OccurrenceInfo>& varList);
	void Build(ExpressionBase& expr, bool isGen = false);
};

VariableListBuilder::VariableListBuilder(const std::vector<LoopInfo>& loopsList)
{
	m_inGeneratorStack.push(false);
	m_inSubscriptStack.push(false);

	for(size_t i = 0; i < loopsList.size(); ++i)
		m_loopItersList.push_back(loopsList[i].pthis);
}

void VariableListBuilder::ReleaseList(std::vector<OccurrenceInfo>& varList)
{
	varList.swap(m_varList);
}

void VariableListBuilder::Build(ExpressionBase& expr, const bool isGen)
{
	m_inGeneratorStack.push(isGen);
	expr.accept(*this);
	m_inGeneratorStack.pop();
}

void VariableListBuilder::visit(ReferenceExpression& dataExpr)
{
	bool bIsLoopIndex = false;
	for (unsigned int i = 0; (i < m_loopItersList.size()) && !bIsLoopIndex ; ++i)
		bIsLoopIndex = ExprManagement::findSubExpr(&dataExpr, m_loopItersList[i]);

	if (!bIsLoopIndex || !isInsideSubscript())
		m_varList.push_back(OccurrenceInfo(&dataExpr.getReference(), isInsideGenerator(), (int)m_loopItersList.size(), 0, false, &dataExpr, bIsLoopIndex));
}

void VariableListBuilder::visit(Canto::HirCCallExpression& basicCallExpr)
{
	Canto::HirCCallExpression::HirCOperatorKind callKind = basicCallExpr.getKind();

	if (basicCallExpr.getArgumentCount() == 1)
	{
		if (callKind == Canto::HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS ||
			callKind == Canto::HirCCallExpression::HIRC_PREFIX_PLUS_PLUS ||
			callKind == Canto::HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS ||
			callKind == Canto::HirCCallExpression::HIRC_PREFIX_MINUS_MINUS)
		{
			// инкремент сначала считывает переменную
			Build(basicCallExpr.getArgument(0), false);
			m_inGeneratorStack.push(true);
		}
		Build(basicCallExpr.getArgument(0), isInsideGenerator());
	}

	if(callKind == Canto::HirCCallExpression::HIRC_ARRAY_ACCESS)
	{
		const bool isFIndexRange = basicCallExpr.getArgument(1).is_a<Canto::HirFArrayIndexRangeExpression>();

		const int dim = isFIndexRange
				? basicCallExpr.getArgument(1).cast_to<Canto::HirFArrayIndexRangeExpression>().getArgumentCount()
				: basicCallExpr.getArgumentCount() - 1;

		ReferenceExpression* arrayData = dynamic_cast<ReferenceExpression*>(&basicCallExpr.getArgument(0));

		m_inSubscriptStack.push(true);
		bool isLinear = true;
		for(int i = 0; i < dim; ++i)    // цикл по размерностям
		{
			// берем i-тую размерность
			ExpressionBase * pCurDim = isFIndexRange
					? &basicCallExpr.getArgument(1).cast_to<Canto::HirFArrayIndexRangeExpression>().getArgument(i)
					: &basicCallExpr.getArgument(i+1);

			// проверяем на линейность по счетчикам циклов
			if (!checkLinByBases(pCurDim, m_loopItersList)) {
				// нелинейность
				isLinear = false;
			};

			Build(*pCurDim, false);
		};
		m_inSubscriptStack.pop();

		if (arrayData)
			m_varList.push_back(OccurrenceInfo(&arrayData->getReference(), isInsideGenerator(),
			(int)m_loopItersList.size(), dim, isLinear, arrayData, false));
	}
	else if (callKind == Canto::HirCCallExpression::HIRC_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_PLUS_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_MINUS_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_MULTIPLY_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_DIVISION_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_MOD_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_LSHIFT_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_RSHIFT_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_BAND_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_BOR_ASSIGN ||
		callKind == Canto::HirCCallExpression::HIRC_BXOR_ASSIGN)
	{
		Build(basicCallExpr.getArgument(1), false);
		if (callKind == Canto::HirCCallExpression::HIRC_PLUS_ASSIGN ||
			callKind == Canto::HirCCallExpression::HIRC_MINUS_ASSIGN ||
			callKind == Canto::HirCCallExpression::HIRC_MULTIPLY_ASSIGN ||
			callKind == Canto::HirCCallExpression::HIRC_DIVISION_ASSIGN ||
			callKind == Canto::HirCCallExpression::HIRC_MOD_ASSIGN ||
			callKind == Canto::HirCCallExpression::HIRC_LSHIFT_ASSIGN ||
			callKind == Canto::HirCCallExpression::HIRC_RSHIFT_ASSIGN ||
			callKind == Canto::HirCCallExpression::HIRC_BAND_ASSIGN ||
			callKind == Canto::HirCCallExpression::HIRC_BOR_ASSIGN ||
			callKind == Canto::HirCCallExpression::HIRC_BXOR_ASSIGN)
		{
			// перед такими типами присваиваний, левую часть нужно сначала прочитать..
			Build(basicCallExpr.getArgument(0), false);
		}
		Build(basicCallExpr.getArgument(0), true);
	}
	else if (basicCallExpr.getArgumentCount() >= 2)
	{
		// я не знаю почему в обратном порядке. так было
		for(int i = basicCallExpr.getArgumentCount() - 1; i >= 0; --i)
		{
			Build(basicCallExpr.getArgument(i), false);
		}
	}
}

void VariableListBuilder::visit(BasicCallExpression& basicCallExpr)
{
	BasicCallExpression::BuiltinCallKind callKind = basicCallExpr.getKind();

	if(basicCallExpr.getArgumentCount() == 1 )
	{
		Build(basicCallExpr.getArgument(0), isInsideGenerator());
	}
	if(callKind == BasicCallExpression::BCK_ARRAY_ACCESS)
	{
		const bool isFIndexRange  = basicCallExpr.getArgument(1).is_a<Canto::HirFArrayIndexRangeExpression>();
		const int dim = isFIndexRange
				? basicCallExpr.getArgument(1).cast_to<Canto::HirFArrayIndexRangeExpression>().getArgumentCount()
				: basicCallExpr.getArgumentCount() - 1;

		ReferenceExpression* arrayData = dynamic_cast<ReferenceExpression*>(&basicCallExpr.getArgument(0));

		m_inSubscriptStack.push(true);
		bool isLinear = true;
		for(int i = 0; i < dim; ++i)    // цикл по размерностям
		{
			// берем i-тую размерность
			ExpressionBase * pCurDim = isFIndexRange
					? &basicCallExpr.getArgument(1).cast_to<Canto::HirFArrayIndexRangeExpression>().getArgument(i)
					: &basicCallExpr.getArgument(i+1);

			// проверяем на линейность по счетчикам циклов
			if (!checkLinByBases(pCurDim, m_loopItersList))
			{
				// нелинейность
				isLinear = false;
			};

			Build(*pCurDim, false);
		};
		m_inSubscriptStack.pop();

		if (arrayData)
			m_varList.push_back(OccurrenceInfo(&arrayData->getReference(), isInsideGenerator(),
			(int)m_loopItersList.size(), dim, isLinear, arrayData, false));

	}
	else if (callKind == BasicCallExpression::BCK_ASSIGN)
	{
		Build(basicCallExpr.getArgument(1), false);
		Build(basicCallExpr.getArgument(0), true);
	}
	else if (basicCallExpr.getArgumentCount() >= 2)
	{
		// я не знаю почему в обратном порядке. так было
		for(int i = basicCallExpr.getArgumentCount() - 1; i >= 0; --i)
		{
			Build(basicCallExpr.getArgument(i), false);
		}
	}
}

void VariableListBuilder::visit(SubroutineCallExpression& exprCall)
{
	// Сначала параметры передаются в функцию, т.е. читаются
	for(int i = 0; i < exprCall.getArgumentCount(); ++i) {
		Build(exprCall.getArgument(i));
	}

	// Во время работы функции параметры могут быть модифицированы.
	// Такие параметры будут генераторами.

	ExpressionBase* nameExpr = &exprCall.getCallExpression();
	SubroutineReferenceExpression* nameData = dynamic_cast<SubroutineReferenceExpression*>(nameExpr);

	if (nameData) {
		SubroutineDeclaration* func = dynamic_cast<SubroutineDeclaration*>(&nameData->getReference());

		if (func) {
			DepGraph::SubProcInfo procInfo = DepGraph::getSubProcInfo(*func);
			int paramCount = func->getType().getParameterCount();
			ParameterDescriptor* param;
			for (int i = 0; i < paramCount; i++)
			{
				param = &func->getType().getParameter(i);
				if (procInfo.m_paramAccessList[param] & DepGraph::SubProcInfo::AT_WRITE)
				{
					Build(exprCall.getArgument(i), true);
				}
			}
		}
	}
}

void VariableListBuilder::visit(Canto::HirFIntrinsicCallExpression& callExpr)
{
	Canto::HirFIntrinsicCallExpression::IntrinsicKind callKind = callExpr.getKind();

	const bool isGen = callKind == Canto::HirFIntrinsicCallExpression::IK_READ;

	for(int i = 0; i < callExpr.getArgumentCount(); ++i)
	{
		Build(callExpr.getArgument(i), isGen);		
	}
}

											//	===============! Изменить !===================
void getAllVariables(ExpressionBase& node, const std::vector<LoopInfo>& loops, std::vector<OccurrenceInfo>& varsList)
{
	VariableListBuilder	builder(loops);
	builder.Build(node);
	builder.ReleaseList(varsList);
}
//вызывает функции построения информации о всех внешних циклах для данного выражения - 
//это неэкономично. Нужно 1 раз строить информацию о циклах а потом ее просто считывать и копировать по вхождениям
void IndOccurContainer::ParseExpression(ExpressionBase* expr, id& index, EN_OccurListBuildParams buildParams, int* numbE)
{
	std::vector<LoopInfo> loops;

	// Получить информацию о циклах
	index.getOuterLoopsList(loops, (buildParams & OLBP_SEARCH_OUTERLOOPS) != 0);

	// Получить все вхождения переменных
		std::vector<OccurrenceInfo>  varEnt;
	getAllVariables(*expr, loops, varEnt);

	if (varEnt.empty())
		return;

	LoopDesc* indLoops = GetIndLoopArray1(loops, index);
	for(size_t i = 0; i < varEnt.size(); ++i, ++(*numbE)) {

		OccurrenceInfo& temp = varEnt[i];
		OccurList& targetList = temp.m_generator ? indGenList : indUtilList;

		// создаем вхождение
		targetList.AddNewElem(*numbE, index.getThisOper(), indLoops, temp);
	}

	delete[] indLoops;
}
//простматривает все ExpressionStatment'ы в программе и для каждого вызывает функцию ParseExpression												//===============! Изменить !===================
void IndOccurContainer::Build(id index, EN_OccurListBuildParams buildParams /* = OLBP_SEARCH_OUTERLOOPS */)
{
	OPS::Console* const pConsole = &OPS::getOutputConsole("DepGraph");
	//	pConsole->log(OPS::Console::LEVEL_WARNING, "BuildVO: Test!");

	int numbE = 0;

	BlockStatement* forBlock = 0;//специально для целей предупреждения!

	for(StatementBase* currNode = index.getThisOper(); currNode; currNode = index.next())
	{
		GetTypeVisitor visitor;
		currNode->accept(visitor);
		switch(visitor.m_typeOfNode)
		{
		case GetTypeVisitor::NK_ExpressionStatement:
			{
				if (forBlock) {
					if (forBlock != &currNode->getParentBlock())
					{
						// Был вход в оператор For но потом резкий выход из него. 
						// В теле операторы не рассматривались...
						pConsole->log(OPS::Console::LEVEL_WARNING, _TL("BuildVO: No Statements are found in the For LoopBody!","BuildVO: в теле цикла не обнаружено операторов."));
					}
				}

				/*std::vector<LoopInfo> loops;

				// Получить информацию о циклах
				index.getOuterLoopsList(loops, (buildParams & OLBP_SEARCH_OUTERLOOPS) != 0);

				if (loops.empty() && 
					forBlock &&
					(buildParams & OLBP_SEARCH_OUTERLOOPS))
				{
					// Рассматриваемый оператор находится в гнезде циклов, НО описания гнезд циклов не найдено!
					pConsole->log(OPS::Console::LEVEL_ERROR, "BuildVO: No 'For' Statements found for a LoopBody Assign Statement!");
				}*/

				forBlock = 0; // Все, больше эта ссылка не нужна.

                ParseExpression(&currNode->cast_to<ExpressionStatement>().get(), index, buildParams, &numbE);
			}
			break;

		case GetTypeVisitor::NK_ForStatement:
			if(forBlock)
			{
				if (&(currNode->cast_to<ForStatement>().getParentBlock()) != forBlock) {
					// Значит, что в теле цикла не найдено операторов, иначе forBlock был бы обнулен.
					pConsole->log(OPS::Console::LEVEL_WARNING, _TL("BuildVO: No Statements are found in the For LoopBody!","BuildVO: в теле цикла не обнаружено операторов."));
				}
			}

			forBlock = &currNode->cast_to<ForStatement>().getBody();
			break;

		case GetTypeVisitor::NK_IfStatement:			
			ParseExpression(&currNode->cast_to<IfStatement>().getCondition(), index, buildParams, &numbE);
			break;
		case GetTypeVisitor::NK_ReturnStatement:
			ParseExpression(&currNode->cast_to<ReturnStatement>().getReturnExpression(), index, buildParams, &numbE);
			break;
		case GetTypeVisitor::NK_BlockStatement:
		case GetTypeVisitor::NK_EmptyStatement:
		case GetTypeVisitor::NK_GotoStatement:
			break;
		default:
			forBlock = NULL;
			pConsole->log(OPS::Console::LEVEL_WARNING, _TL("BuildVO: Unexpected statement type encountered!","BuildVO: неизвестный тип оператора."));
			break;
		}
	}

	if (forBlock) {
		// Если бы были операторы, то этот указатель был бы обнулен.
		pConsole->log(OPS::Console::LEVEL_WARNING, _TL("BuildVO: No Statements are found in the For LoopBody!","BuildVO: в теле цикла не обнаружено операторов."));
	}

	if (numbE) {
		CreateIndex();
	}
	else {
		// Не найдено (и не построено) ни одного вхождения. Странно...
		pConsole->log(OPS::Console::LEVEL_WARNING, _TL("BuildVO: No Occurrence Description was built!","BuildVO: не построено ни одного вхождения!"));
	}
}

													//===============! Изменить !===================
void BuildVO(const id& index,IndOccurContainer& indOccurList,EN_OccurListBuildParams buildParams)
{
    indOccurList.Build(index, buildParams);
}

void LamportGraph::Clear()
{
	LampArrowIterator first = conList.begin(), last = conList.end();
	for (; first != last; ++first) {
		delete (*first);
	}
	conList.clear();
	m_occurContainer.Clear();
}

LampArrowIterator LamportGraph::DeleteArrow(LampArrowIterator arrowIter)
{
	if (arrowIter == conList.end())
		return conList.end();

	if (*arrowIter)
		delete *arrowIter;

	return conList.erase(arrowIter);
}

LampArrow* LamportGraph::AddRow(const OccurDesc& srcOccur,EN_DepType deptype,int commonLoopNumb,const OccurDesc& depOccur)
{
	LampArrow* res = new LampArrow(srcOccur,deptype,commonLoopNumb,depOccur);
	conList.push_back(res);
	SetLampArrowBasedOnOccurStatus(res, srcOccur, depOccur);
	return res;
}


void LamportGraph::GetAllArrowsFromGeneratorsToThisOccur(const ReferenceExpression* pDepOccur,std::list<LampArrowIterator>& arrowList)
{
	LampArrowIterator first=conList.begin(),last=conList.end();
	for(; first != last; ++first)
	{
		if ((*first)->pDepOccur == pDepOccur)
			if (((*first)->type == FLOWDEP) || 
				((*first)->type == OUTPUTDEP))
			{
				//Это как раз и означает, что pSrcDep - генератор
				arrowList.push_back(first);
			}
	}
}

void LamportGraph::GetAllArrowsFromUtilitiesToThisOccur(const ReferenceExpression* pDepOccur,std::list<LampArrowIterator>& arrowList)
{
	LampArrowIterator first = conList.begin(), last = conList.end();
	for(;first != last; ++first)
	{
		if ((*first)->pDepOccur == pDepOccur)
			if (((*first)->type == ANTIDEP) ||
				((*first)->type == INPUTDEP)) 
			{
				//Это как раз и означает, что pSrcDep - использование
				arrowList.push_back(first);
			}				
	}
}

void LamportGraph::GetAllOccursInArrowsToThisOccur(const ReferenceExpression* pDepOccur,std::list<const OccurDesc*>& occurList, int srcOccurType)
{
	LampArrowIterator first = conList.begin(), last = conList.end();

	switch(srcOccurType)
	{
	case 0:
		//Отбираем только генераторы...
		for(;first != last; ++first)
		{
			if ((*first)->pDepOccur == pDepOccur)
				if (((*first)->type == FLOWDEP) ||
					((*first)->type == OUTPUTDEP)) 
				{
					//Это как раз и означает, что pSrcDep - генератор
					occurList.push_back((*first)->srcOccurDesc);
				}				
		}
		break;
	case 1:
		//Отбираем только использования...
		for(;first != last; ++first)
		{
			if ((*first)->pDepOccur == pDepOccur)
				if (((*first)->type == ANTIDEP) ||
					((*first)->type == INPUTDEP)) 
				{
					//Это как раз и означает, что pSrcDep - использование
					occurList.push_back((*first)->srcOccurDesc);
				}				
		}
		break;
	case 2:
		//Отбираем и генераторы и использования...
		for(;first != last; ++first)
		{
			if ((*first)->pDepOccur == pDepOccur)
			{
				occurList.push_back((*first)->srcOccurDesc);
			}				
		}
	};

}

int LamportGraph::Refine(EN_RefineMethod refineMethod)
{
	OPS::Console* const pConsole = &OPS::getOutputConsole("DepGraph");

	switch(refineMethod)
	{
	case RM_ELEM_LATTICE:
		{
			LampArrowIterator first = conList.begin(), last = conList.end();
			for(; first != last; ) {
				LampArrow* currArrow = *first;
				const OccurDesc* src;
				const OccurDesc* dep;
				src = currArrow->srcOccurDesc;
				dep = currArrow->depOccurDesc;
				if (src == 0 || dep == 0)
				{
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("LamportGraph::Refine: Couldn't find Occurrence Description in the container.","LamportGraph::Refine: невозможно найти описание вхождения в данном контейнере."));
					++first;
					continue;
				}
                LatticeGraph::ElemLatticeGraph lg(const_cast<OccurDesc&>(*src), const_cast<OccurDesc&>(*dep), currArrow->type, true);

				if (lg.isStatus(LatticeGraph::LG_ERROR_INIT))
				{
					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("LamportGraph::Refine: Couldn't build a lattice graph.","LamportGraph::Refine: невозможно построить решетчатый граф."));
					//lg.Clear();
					++first;
					continue;
				}

                if(lg.isStatus(LatticeGraph::LG_EMPTY))
				{
					// Полностью удалить дугу
					delete *first;
					first = conList.erase(first);
					continue;
				}

				//// А вот теперь прийдется повозиться...

				//currArrow->suppList.clear();

				//for(int i = 0; i < currArrow->commonLoopNumb; ++i)
				//{
				//	if (lg.TestLatticeBasedSupp(i))
				//		currArrow->suppList.push_back(i);
				//}

				//if (lg.TestLatticeBasedSupp(LIDEP))
				//	currArrow->suppList.push_back(LIDEP);
				//lg.Clear();
				++first;
			}

			return 0;
		}
		break;

	case RM_BELOW_LATTICE: break;
//		{
//			indOccurContainer.CreateIndex();	//На всякий пожарный
//
//			//Сначала собирем все вхождения, в которые идут дуги в графе...
//			std::list<int> depOccurList;
//
//			LampArrowIterator first = conList.begin(), last = conList.end();
//			for(; first != last; ++first) 
//			{
//				depOccurList.push_back((*first)->depOccurNumb);
//			}
//
//			depOccurList.unique();
//			// собрали
//
//			std::list<int>::iterator firstDepOccur = depOccurList.begin(), lastDepOccur = depOccurList.end();
//			for(;firstDepOccur != lastDepOccur; )
//			{
//				std::list<LampArrowIterator> arrowList;
//
//				OccurDesc* tempOccurDesc = indOccurContainer[*firstDepOccur];
//				if (tempOccurDesc == 0)
//				{
//					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("LamportGraph::Refine: Couldn't find Occurrence Description in the container.","LamportGraph::Refine: невозможно найти описание вхождения в данном контейнере."));
//					++firstDepOccur;
//					continue;
//				}
//
//				GetAllArrowsFromGeneratorsToThisOccur(tempOccurDesc->pOccur, arrowList);
//				int errorCode = RefineArrowsWithBelowLattice(tempOccurDesc, arrowList, indOccurContainer);
//
//				switch(errorCode)
//				{
//				case 1:
//					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("LamportGraph::Refine: Couldn't find Occurrence Description in the container.","LamportGraph::Refine: невозможно найти описание вхождения в данном контейнере."));
//					break;
//
//				case 2:
//					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("LamportGraph::Refine: Couldn't build a non affine lattice graph (not implemented).",""));
//
//					break;
//
//				case 3:
//					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("LamportGraph::Refine: Couldn't build a lattice graph.","LamportGraph::Refine: невозможно построить решетчатый граф"));
//					break;
//				};
//
//				arrowList.clear();
//
//				GetAllArrowsFromUtilitiesToThisOccur(tempOccurDesc->pOccur, arrowList);
//				errorCode = RefineArrowsWithBelowLattice(tempOccurDesc, arrowList, indOccurContainer);
//
//				switch(errorCode)
//				{
//				case 1:
//					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("LamportGraph::Refine: Couldn't find Occurrence Description in the container.","LamportGraph::Refine: невозможно найти описание вхождения в данном контейнере."));
//					break;
//
//				case 2:
//					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("LamportGraph::Refine: Couldn't build a non affine lattice graph (not implemented).",""));
//
//					break;
//
//				case 3:
//					pConsole->log(OPS::Console::LEVEL_ERROR, _TL("LamportGraph::Refine: Couldn't build a lattice graph.","LamportGraph::Refine: невозможно построить решетчатый граф."));
//					break;
//				};
//
//				++firstDepOccur;
//			}
//		}
//		break;
//
	case RM_VALUE_LATTICE: break;
//		{
//			pConsole->log(OPS::Console::LEVEL_WARNING, _TL("LamportGraph::Refine: \"RM_VALUE_LATTICE\" refine method hasn't been implemented yet.",""));
//		}
//		break;
//
	case RM_NO_REFINEMENT:
		break;
	};
	return 0;
}

int LamportGraph::RefineArrowsWithBelowLattice(OccurDesc* pDepOccurDesc, std::list<LampArrowIterator>& arrowList)
{
	OPS_UNUSED(pDepOccurDesc);
	OPS_UNUSED(arrowList);
//	std::list<LampArrowIterator>::iterator first = arrowList.begin(), last = arrowList.end();
//
//	// нужно собрать список всех вхождений в началах дуг, чтобы его потом 
//	// подать в функцию построения LatticeGraph-а
//	std::list<OccurDesc*> srcOccurDescs;
//	for(; first != last; ++first)
//	{
//		OccurDesc* tempOccurDesc = (**first)->srcOccurDesc;
//		if (tempOccurDesc == 0)
//			return 1;		
//		srcOccurDescs.push_back(tempOccurDesc);
//	}
//	// собрали
//
//	LatticeGraph lg;
//
//	lg.SetStatus(LG_DONT_TRY_BUILD_BY_INVERTING);
//	lg.ClearStatus(LG_COPY_OCCUR_DESC);
//
//	lg.Build(srcOccurDescs, *pDepOccurDesc);
//
//	if (lg.GetStatus(LG_NONLINEAR_SOLUTION)) {
//		//граф не может быть описан аффинными функциями; пока такой граф мы не можем построить
//		return 2;
//	}
//
//	if (lg.GetStatus(LG_ERROR_INIT))
//	{
//		//не удалось построить граф
//		return 3;
//	}
//
//	// Теперь, делаем refine дугам...
//	std::list<LampArrowIterator>::iterator 
//		firstArrowIt = arrowList.begin(),
//		lastArrowIt = arrowList.end();
//	
//	for(; firstArrowIt != lastArrowIt; ++firstArrowIt)
//	{
//		LampArrow* currLampArrow = **firstArrowIt;
//
//		lg.GetLatticeBasedSuppList(currLampArrow->pSrcOccur, currLampArrow->pDepOccur, currLampArrow->suppList);
//
//		if (currLampArrow->suppList.empty())
//		{
//			// значит дугу нужно удалить
//			conList.erase(*firstArrowIt);
//		}
//	}
	return 0;
}


std::ostream& operator<<(std::ostream& os, const LamportGraph& lg)
{
	ConstLampArrowIterator first = lg.Begin(), last = lg.End();

	if (first == last)
	{
		os << _TL("Lamport Graph Is Empty!","Граф зависимостей пуст!");
		return os;
	}

	for(; first != last; ++first)
	{
		os << *(*first) << std::endl;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const LampArrow& arrow)
{
	os << "(" << arrow.srcOccurNumb << ',';
	switch(arrow.type)
	{
	case FLOWDEP:	os << "flow";  break;
	case ANTIDEP:	os << "anti";  break;
	case OUTPUTDEP:	os << "output";break;
	case INPUTDEP:	os << "input"; break;
	}
	os << ',';

	if (arrow.TestSupp(ANYSUPP))
	{
		os << "any";
	}
	else
	{
		LampArrow::SuppList::const_iterator	firstSupp = arrow.suppList.begin(),
											lastSupp = arrow.suppList.end();
		os << '[';
		for(; firstSupp != lastSupp; ++firstSupp)
		{
			if (*firstSupp == LIDEP)
				os << "lid";
			else
				os << *firstSupp;
			if (firstSupp != lastSupp)
				os << ',';
		}
		os << ']';
	}
	os << "," << arrow.depOccurNumb << ")";

	return os;
}

//строит описание ОДНОЙ границы одного цикла
void buildOneLinearBound(LatticeGraph::SimpleLinearExpression& resultCounterCoefs
						 , OPS::Shared::CanonicalLinearExpression& resultExternalParamCoefs
						 , bool& resultFlagNotLinear
						 , bool& resultFlagUnknown
						 , ExpressionBase* boundExpression
						 , std::vector<OPS::Reprise::VariableDeclaration*>& loopIndexDeclarations
                         , bool flagIsBoundIsLeft)
{
    //проверяем, является ли выражение для левой границы каноническим
    bool decreaseBound = false;//если верхняя граница задана как < то ее надо уменьшить
    GetTypeVisitor visitor;
    boundExpression->accept(visitor);
    if (visitor.m_typeOfNode != GetTypeVisitor::NK_BasicCallExpression && visitor.m_typeOfNode != GetTypeVisitor::NK_HirCCallExpression )
    {
        CallExpressionBase* tempCallExpr = &boundExpression->cast_to<CallExpressionBase>();
        if (tempCallExpr->getArgumentCount()!= 2)  throw OPS::RuntimeError("Unexpected node kind");
    }
    if (flagIsBoundIsLeft==true)
    {
        //для левой границы
        if (visitor.m_typeOfNode == GetTypeVisitor::NK_BasicCallExpression ) 
        {
            BasicCallExpression* tempExpr = &boundExpression->cast_to<BasicCallExpression>();
            if(tempExpr->getKind() != BasicCallExpression::BCK_ASSIGN ) throw OPS::RuntimeError("Unexpected node kind");
        }
    }
    else 
    {
        //для правой границы
        if (visitor.m_typeOfNode == GetTypeVisitor::NK_BasicCallExpression)
        {
            if ( boundExpression->cast_to<BasicCallExpression>().getKind() == BasicCallExpression::BCK_LESS)  
                decreaseBound = true;
        }
    }
    //КОНЕЦ (проверяем, является ли выражение для левой границы каноническим)

    //проверяем, является ли левая граница линейным выражением и заполняем коэффициенты
    ExpressionBase* bound = &boundExpression->cast_to<CallExpressionBase>().getArgument(1);
    try
    {
        OPS::Shared::ParametricLinearExpression* parametricLinExprOfBound 
            = OPS::Shared::ParametricLinearExpression::createByAllVariables(bound);
        if (parametricLinExprOfBound!=0) //выражение совсем нелинейное
        {
            //заполняем массивы коэффициентов при счетчиках циклов и внешних параметрах
			getExternalParamAndLoopCounterCoefficients
				(*parametricLinExprOfBound, loopIndexDeclarations, resultCounterCoefs, resultExternalParamCoefs);
            resultFlagNotLinear = false;
            //если есть внешние параметры - присваиваем флаг = граница неизвестна
            if (resultExternalParamCoefs.getVariables().size()>0)
                resultFlagUnknown = true;
            else 
                resultFlagUnknown = false;
            delete parametricLinExprOfBound;
        }
        else
        {
            resultFlagUnknown = true;
            resultFlagNotLinear = true;
        }
    }
    catch(OPS::Exception&)
    {//выражение линейное, но коэффициенты не являются целыми константами
        resultFlagUnknown = true;
        resultFlagNotLinear = true;
    }
    //КОНЕЦ (проверяем, является ли левая граница линейным выражением и заполняем коэффициенты)
}

//находит минимальные и максимальные значения счетчиков
void findMinAndMaxCounterValues(LoopDesc* loopDesc, int loopNum)
{
    // зануление минимальных и максимальных значений счетчиков
    for(int i = 0; i < loopNum; ++i) 
    {
        loopDesc[i].l = 0;
        loopDesc[i].u = 0;
    }
    //считаем
    for (int i = 0; i < loopNum; ++i)
    {
        //проверка: нормальные ли границы, нет ли зависимости от внешних параметров
        if ((loopDesc[i].lBoundUnk) || (loopDesc[i].rBoundUnk)) break;
        //минимальное значение
        loopDesc[i].l = loopDesc[i].loopBounds.m_lower[0][0];
        for (int j=0; j<i; j++)
        {
            if (loopDesc[i].loopBounds.m_lower[0][j+1]>0)
                loopDesc[i].l += loopDesc[i].loopBounds.m_lower[0][j+1] * loopDesc[j].l;
            else
                loopDesc[i].l += loopDesc[i].loopBounds.m_lower[0][j+1] * loopDesc[j].u;
        }
        //максимальное значение
        loopDesc[i].u = loopDesc[i].loopBounds.m_upper[0][0];
        for (int j=0; j<i; j++)
        {
            if (loopDesc[i].loopBounds.m_upper[0][j+1]>0)
                loopDesc[i].u += loopDesc[i].loopBounds.m_upper[0][j+1] * loopDesc[j].u;
            else
                loopDesc[i].u += loopDesc[i].loopBounds.m_upper[0][j+1] * loopDesc[j].l;
        }
    }
}

//строит описание гнезда циклов
LoopDesc* GetIndLoopArray1(std::vector<LoopInfo>& loops,id& a)
{
	OPS_UNUSED(a);

	// Только для простых циклов: без внешних переменных, без максимумов.
	if (loops.empty()) return 0;

	LoopDesc* res = new LoopDesc[loops.size()];
    
	// выделяем память для хранения линейных выражений границ
	for(int i = 0; i < (int)loops.size(); ++i)
	{
		res[i].loopBounds.m_lower = new LatticeGraph::SimpleLinearExpression[1];
		res[i].loopBounds.m_lowerExternalParamCoefs.clear();
		res[i].loopBounds.m_lower[0] = LatticeGraph::SimpleLinearExpression(i+1);
		res[i].loopBounds.m_lowerNumb = 1;

		res[i].loopBounds.m_upper = new LatticeGraph::SimpleLinearExpression[1];
		res[i].loopBounds.m_upperExternalParamCoefs.clear();
		res[i].loopBounds.m_upper[0] = LatticeGraph::SimpleLinearExpression(i+1);
		res[i].loopBounds.m_upperNumb = 1;
	}

    //конструируем вектор со счетчиками циклов
    std::vector<OPS::Reprise::VariableDeclaration*> loopIndexDeclarations(loops.size()); 
	for (size_t i = 0; i < loops.size(); ++i)  loopIndexDeclarations[i] = loops[i].m_oDataObject;

    //цикл по всем циклам loops
	for (size_t i = 0; i < loops.size(); ++i)
    {
        //ссылка на счетчик
        res[i].counterIter=loops[i].m_oDataObject;

        //ссылка на цикл во внутреннем представлении
        res[i].stmtFor = loops[i].pStmtFor;
        
        //левая граница
		buildOneLinearBound(res[i].loopBounds.m_lower[0], res[i].loopBounds.m_lowerExternalParamCoefs
            , res[i].lBoundNotLinear, res[i].lBoundUnk, loops[i].m_left, loopIndexDeclarations, true);

        //правая граница
		buildOneLinearBound(res[i].loopBounds.m_upper[0], res[i].loopBounds.m_upperExternalParamCoefs
            , res[i].rBoundNotLinear, res[i].rBoundUnk, loops[i].m_right, loopIndexDeclarations, false);
    }
    //заполняем минимальные и максимальные значения счетчиков
    findMinAndMaxCounterValues(res,loops.size());
	
    return res;
}


LampArrowIterator LamportGraph::GetDepIterEx(LampArrowIterator searchFrom, const ReferenceExpression* pSrcOccur, const ReferenceExpression* pDepOccur)
{
	for(; searchFrom != conList.end(); ++searchFrom)
	{
		if ((*searchFrom)->pSrcOccur == pSrcOccur &&
			(*searchFrom)->pDepOccur == pDepOccur)
			return searchFrom;
	}
	return conList.end();
}

LampArrowIterator LamportGraph::GetDepIterEx(LampArrowIterator searchFrom,const StatementBase* pSrcStmt,const StatementBase* pDepStmt)
{
	for(; searchFrom != conList.end(); ++searchFrom) {
		if ((*searchFrom)->pSrcStmt == pSrcStmt &&
			(*searchFrom)->pDepStmt == pDepStmt)
			return searchFrom;
	}
	return conList.end();
}

bool LamportGraph::TestDep(int occurNumb, int depOccurNumb)
{
	LampArrowIterator first = conList.begin();
	for(; first != conList.end(); ++first) {
		if ((*first)->srcOccurNumb == occurNumb &&
			(*first)->depOccurNumb == depOccurNumb)
			return true;
	}
	return false;
}

bool LamportGraph::TestDep(const StatementBase* pSrcStmt,const StatementBase* pDepStmt)
{
	return GetDepIter(pSrcStmt, pDepStmt) != conList.end();
}

bool LamportGraph::TestDep(const ReferenceExpression* pSrcOccur,const ReferenceExpression* pDepOccur)
{
	return GetDepIter(pSrcOccur, pDepOccur) != conList.end();
}

bool LamportGraph::TestDep(EN_DepType depType, int occurNumb, int depOccurNumb)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first)
	{
		if ((*first)->srcOccurNumb == occurNumb &&
			(*first)->depOccurNumb == depOccurNumb &&
			(*first)->type == depType)
			return true;
	}
	return false;
}

bool LamportGraph::TestDep(EN_DepType depType,const StatementBase* pSrcStmt,const StatementBase* pDepStmt)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first) 
	{
		if ((*first)->pSrcStmt == pSrcStmt &&
			(*first)->pDepStmt == pDepStmt &&
			(*first)->type == depType)
			return true;
	}
	return false;
}

using namespace OPS::Reprise;

bool LamportGraph::TestDep(EN_DepType depType, const OPS::Reprise::StatementBase* pSrcStmt1, 
			 const OPS::Reprise::StatementBase* pSrcStmt2, const OPS::Reprise::StatementBase* pDepStmt)
{
	OPS_ASSERT( pSrcStmt1 != NULL && pSrcStmt2 != NULL && pDepStmt != NULL );

	if ( !ProgramFragment::isFragment(*pSrcStmt1, *pSrcStmt2) )
		throw OPS::ArgumentError("Error: [pSrcStmt1, pSrcStmt2) not program fragment.");

	const BlockStatement* const parent = &pSrcStmt1->getParentBlock();
	BlockStatement::ConstIterator fragmentBegin = parent->convertToIterator(pSrcStmt1);
	BlockStatement::ConstIterator fragmentEnd = parent->convertToIterator(pSrcStmt2);
	for (BlockStatement::ConstIterator it = fragmentBegin; it != fragmentEnd; it.goNext())
	{
		if (TestDep(depType, &*it, pDepStmt))
			return true;
	}
	return false;
}

bool LamportGraph::TestDep(EN_DepType depType,const ReferenceExpression* pSrcOccur,const ReferenceExpression* pDepOccur)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first)
	{
		if ((*first)->pSrcOccur == pSrcOccur &&
			(*first)->pDepOccur == pDepOccur &&
			(*first)->type == depType)
			return true;
	}
	return false;
}

bool LamportGraph::TestSupp(int supp,int occurNumb,int depOccurNumb)
{
	LampArrowIterator first = conList.begin();
	for(; first != conList.end(); ++first) 
	{
		if ((*first)->srcOccurNumb == occurNumb &&
			(*first)->depOccurNumb == depOccurNumb &&
			(*first)->TestSupp(supp))
			return true;
	}
	return false;
}

bool LamportGraph::TestSupp(int supp, const StatementBase* pSrcStmt, const StatementBase* pDepStmt)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first)
	{
		if ((*first)->pSrcStmt == pSrcStmt && 
			(*first)->pDepStmt == pDepStmt && 
			(*first)->TestSupp(supp))
			return true;
	}
	return false;
}

bool LamportGraph::TestSupp(int supp,const ReferenceExpression* pSrcOccur,const ReferenceExpression* pDepOccur)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first)
	{
		if ((*first)->pSrcOccur == pSrcOccur &&
			(*first)->pDepOccur == pDepOccur &&
			(*first)->TestSupp(supp))
			return true;
	}
	return false;
}

bool LamportGraph::TestSupp(int supp)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first)
	{
		if ((*first)->TestSupp(supp))
			return true;
	}
	return false;
}

bool LamportGraph::TestDepAndSupp(EN_DepType depType,int supp,int occurNumb,int depOccurNumb)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first)
	{
		if ((*first)->srcOccurNumb == occurNumb &&
			(*first)->depOccurNumb == depOccurNumb &&
			(*first)->type == depType &&
			(*first)->TestSupp(supp))
			return true;
	}
	return false;
}

bool LamportGraph::TestDepAndSupp(EN_DepType depType,int supp,const StatementBase* pSrcStmt,const StatementBase* pDepStmt)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first)
	{
		if ((*first)->pSrcStmt == pSrcStmt && 
			(*first)->pDepStmt == pDepStmt &&
			(*first)->type == depType && 
			(*first)->TestSupp(supp))
			return true;
	}
	return false;
}

bool LamportGraph::TestDepAndSupp(EN_DepType depType,int supp,const ReferenceExpression* pSrcOccur,const ReferenceExpression* pDepOccur)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first) {
		if ((*first)->pSrcOccur == pSrcOccur &&
			(*first)->pDepOccur == pDepOccur &&
			(*first)->type == depType && 
			(*first)->TestSupp(supp))
			return true;
	}
	return false;
}

bool LamportGraph::TestDepAndSupp(EN_DepType depType,int supp)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first)
	{
		if ((*first)->type == depType && 
			(*first)->TestSupp(supp))
			return true;
	}
	return false;
}

bool LamportGraph::HasLoopIndependentArrowsOnly()
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first)
	{
		if (!(*first)->IsLoopIndependentOnly()) 
			return false;
	}
	return true;
}

bool LamportGraph::HasSingleArrowToDepOccur(const ReferenceExpression* pSrcOccur,const ReferenceExpression* pDepOccur)
{
	for(LampArrowIterator first = conList.begin(); first != conList.end(); ++first) 
	{
		if ((*first)->pDepOccur == pDepOccur && 
			(*first)->pSrcOccur != pSrcOccur)
				return false; //существует дуга, входящая в pDepOccur и не исходящая из pSrcOccur
	}

	return true;
}
												//=============! Изменить !===============
void BuildLamportGraph(const id& index, LamportGraph& lamp)
{
	lamp.Clear();
	lamp.Build(index);
}

bool LampArrow::TestSupp(int _supp) const
{
	SuppList::const_iterator first = suppList.begin(), last = suppList.end();

	if (first != last && *first == ANYSUPP)
		return true;

	for(;first != last; ++first) 
	{
		if (*first == _supp)
			return true;
	}
	return false;
}

bool LampArrow::TestSupp(const OPS::Reprise::ForStatement &supp) const
{
	if (!suppList.empty() && suppList.front() == ANYSUPP)
		return true;

	SuppList::const_iterator it = suppList.begin();
	for(; it != suppList.end(); ++it)
	{
		if (*it >=0 && srcOccurDesc->loops[*it].stmtFor == &supp)
			return true;
	}
	return false;
}

bool LampArrow::IsLoopIndependent() const
{
	return TestSupp(LIDEP);
}

bool LampArrow::IsLoopIndependentOnly() const
{
	SuppList::const_iterator first=suppList.begin(),last=suppList.end();

	if (first != last && *first == ANYSUPP)
		return false;

	for(;first != last; ++first)
	{
		if (*first != LIDEP)
			return false;
	}
	return true;
}
													//=============! Изменить !===============			
void LamportGraph::RefineByDepGraphEx()
{
	using namespace DepGraph;

	StatementBase* pStmt = 0;

	{
		LampArrowIterator it = Begin();
		if (it == End())
			return;

		LampArrow* pArrow = *it;
		pStmt = pArrow->pDepStmt;
	}

	ProgramUnit* pProgram = pStmt->findProgramUnit();
	/*if (&pStmt->getParentBlock())
	{
		pStmt->getParentBlock().findProgramUnit(pProgram);
	}
	else
	{
		BlockStatement* pBlock = dynamic_cast<BlockStatement*>(pStmt);
		if (pBlock)
			pBlock->findProgramUnit(pProgram);
	}*/

	if (pProgram == 0)
		return;

	//std::string note = pProgram->getNote("GlobalConditionTable");
	/*if (!pImm)
		return;*/

	GlobalConditionTable* pTable = GlobalConditionTable::Get(*pProgram);		// NB! Debug!!!!!!!!!!!!!!!!!
	if (!pTable)
		return;

	LampArrowIterator it = Begin(), itEnd = End();

	for(; it != itEnd ;)
	{
		LampArrow* pArrow = *it;
		GlobalConditionTable::ConstIterator itCond = 
			pTable->Find(std::make_pair(pArrow->pSrcOccur, pArrow->pDepOccur));
		if (itCond != pTable->End() && !itCond->second.pred_value)
		{
			LampArrowIterator itToErase = it++;
			DeleteArrow(itToErase);
		}
		else
			it++;
	}
}

std::string LampArrow::GetTag(EN_PrintModes printMode)
{
	std::string res;	
	switch(type)
	{
		case FLOWDEP:
			switch(printMode)
			{
				case PM_DEVELOPER_RUSSIAN:
				case PM_POPULAR_RUSSIAN:
					res = "Потоковая зависимость\n"; break;

				case PM_DEVELOPER_ENGLISH:
					res = "FLOWDEP"; break;

				case PM_POPULAR_ENGLISH:
					res = "Flow dependence\n"; break;
			};
			break;

		case ANTIDEP:
			switch(printMode)
			{
				case PM_DEVELOPER_RUSSIAN:
				case PM_POPULAR_RUSSIAN:
					res = "Антизависимость\n"; break;

				case PM_DEVELOPER_ENGLISH:
					res = "ANTIDEP"; break;

				case PM_POPULAR_ENGLISH:
					res = "Antidependence\n"; break;
			};			
			break;

		case OUTPUTDEP:
			switch(printMode)
			{
				case PM_DEVELOPER_RUSSIAN:
				case PM_POPULAR_RUSSIAN:
					res = "Выходная зависимость\n"; break;

				case PM_DEVELOPER_ENGLISH:
					res = "OUTPUTDEP"; break;

				case PM_POPULAR_ENGLISH:
					res = "Output dependence\n"; break;
			};
			break;

		case INPUTDEP:
			switch(printMode)
			{
				case PM_DEVELOPER_RUSSIAN:
				case PM_POPULAR_RUSSIAN:
					res = "Входная зависимость\n"; break;

				case PM_DEVELOPER_ENGLISH:
					res = "INPUTDEP"; break;

				case PM_POPULAR_ENGLISH:
					res = "Input dependence\n"; break;
			};
	};

	SuppList::iterator first = suppList.begin(), last = suppList.end(), tempIt;

	switch(printMode)
	{
		case PM_DEVELOPER_RUSSIAN:
		case PM_POPULAR_RUSSIAN:
			res += "Носители (уровни): "; break;

		case PM_DEVELOPER_ENGLISH:
			res += ",["; break;

		case PM_POPULAR_ENGLISH:
			res += "Supports (levels): "; break;
	};

	if (first != last && *first == ANYSUPP) 
	{
		switch(printMode)
		{
			case PM_DEVELOPER_RUSSIAN:
			case PM_POPULAR_RUSSIAN:
				res += "все возможные\nЦиклически независимая";
			break;

			case PM_DEVELOPER_ENGLISH:
				res += "any]";
			break;

			case PM_POPULAR_ENGLISH:
				res += "all possible\nLoop independent";
			break;
		};
		goto end_flags;
	}

	{
		bool lid = false;
		bool added = false;
		while(first!=last)
		{
			if (*first == LIDEP)
			{
					lid = true;
			}
			else
			{
					res += OPS::Strings::format("%d", *first);
					added = true;
			}

			first++;

			if (first != last && added)
			{
				tempIt = first;
				tempIt++;
				if (*first != LIDEP)
				{
						res += ", ";
				}
				else
				{
						if (tempIt != last)
								res += ", ";
				}
				added = false;
			}
		};

		switch(printMode)
		{
			case PM_DEVELOPER_RUSSIAN:
			case PM_POPULAR_RUSSIAN:
					if ( suppList.empty() ||
							((suppList.size() == 1) && lid))
							res += "нет";

					if (lid)
							res += "\nЦиклически независимая";
					break;

			case PM_DEVELOPER_ENGLISH:
					res += lid ? ",lid]" : "]\n";

					res +="Occurs: #" + OPS::Strings::format("%d", srcOccurNumb) + " -> #" + OPS::Strings::format("%d", depOccurNumb);
					break;

			case PM_POPULAR_ENGLISH:
					if ( suppList.empty() ||
							((suppList.size() == 1) && lid))
							res += "no";

					if (lid)
							res += "\nLoop independent";
					break;
		};
	}

end_flags:
	if (GetStatusData() == 0)
		return res;

	switch(printMode)
	{
		case PM_DEVELOPER_RUSSIAN:
			res += "\nУстановленные флаги:";
			if (isStatus(LAS_CR_EXTERNAL_NONLINEAR_EXPR_IN_SUBSCRIPT))
				res += "\nCR_EXTERNAL_NONLINEAR_EXPR_IN_SUBSCRIPT";

			if (isStatus(LAS_CR_EXTERNAL_VAR_IN_SUBSCRIPT))
				res += "\nCR_EXTERNAL_VAR_IN_SUBSCRIPT";

			if (isStatus(LAS_CR_SOME_COMMON_LOOPS_WASNT_PARSERED_CAREFULLY))
				res += "\nCR_SOME_COMMON_LOOPS_WASNT_PARSERED_CAREFULLY";

			break;

		case PM_DEVELOPER_ENGLISH:
			res += "\nFlags:";
			if(isStatus(LAS_CR_EXTERNAL_NONLINEAR_EXPR_IN_SUBSCRIPT))
				res += "\nCR_EXTERNAL_NONLINEAR_EXPR_IN_SUBSCRIPT";

			if(isStatus(LAS_CR_EXTERNAL_VAR_IN_SUBSCRIPT))
				res += "\nCR_EXTERNAL_VAR_IN_SUBSCRIPT";

			if(isStatus(LAS_CR_SOME_COMMON_LOOPS_WASNT_PARSERED_CAREFULLY))
				res += "\nCR_SOME_COMMON_LOOPS_WASNT_PARSERED_CAREFULLY";
			break;
		default: break;
	};

	return res;
}
												//=============! Изменить !===============			
ReferenceExpression* CopyOccurrence(const ReferenceExpression& occur)
{
	if (occur.getParent())
	{
		BasicCallExpression* temp = &occur.getParent()->cast_to<BasicCallExpression>();
		if(temp)
			if (temp->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
			{
				// т.е. оригинальное вхождение - массив.
				BasicCallExpression* tempArray = temp->clone();
				tempArray->setParent(0);
				return &tempArray->getArgument(0).cast_to<ReferenceExpression>();
			}
	}

	// т.е. оригинальное вхождение - скалярная переменная.
	ReferenceExpression* tempOccur = (ReferenceExpression*)occur.clone();
	tempOccur->setParent(0);
	return tempOccur;
}

bool StringIsNumber(const char* buf, size_t n)
{
	size_t i = 0;
	while((i<n)&&(buf[i]==' ')&&buf[i])
		i++;

	if((i==n)||(buf[i]==0))return false;//т.к. в строке записаны только пробелы

	if(buf[i]=='-')
	{
		i++;
		if((i==n)||(buf[i]==0))return false;
	}

	while((i<n)&&buf[i])
	{
		if((buf[i]<'0')||(buf[i]>'9'))return false;
		i++;
	}
	return true;
}

bool StringIsNumber(const char* buf)
{
	size_t i = 0;
	while(buf[i] == ' ')
		i++;

	if (buf[i] == 0)
		return false; //т.к. в строке записаны только пробелы

	if (buf[i] == '-') {
		i++;
		if (buf[i] == 0)
			return false;
	}

	for(; buf[i]; ++i) {
		if ((buf[i]<'0') || (buf[i]>'9'))return false;
	}
	return true;
}

bool PrecedesNonStrictly(const OccurDesc& p, const OccurDesc& q)
{
	if (p.GetStatement() == q.GetStatement()) return true;
	if (p.GetNumber() < q.GetNumber()) return true;
	return false;
}

bool PrecedesStrictly(const OccurDesc& p, const OccurDesc& q)
{
	if (p.GetStatement() == q.GetStatement()) return false;
	if (p.GetNumber() < q.GetNumber()) return true;
	return false;
}

void SetLampArrowBasedOnOccurStatus(LampArrow* lampArrow,const OccurDesc& srcOccur,const OccurDesc& depOccur)
{
	if (srcOccur.isStatus(NONLINEAR) || depOccur.isStatus(NONLINEAR))
		lampArrow->SetStatus(LAS_CR_EXTERNAL_NONLINEAR_EXPR_IN_SUBSCRIPT);

	if (srcOccur.isStatus(FREE_IS_PARAMETER) || 
		srcOccur.isStatus(INDEX_COEF_IS_PARAMETER) || 
		depOccur.isStatus(FREE_IS_PARAMETER) || 
		depOccur.isStatus(INDEX_COEF_IS_PARAMETER)) 
	{
        lampArrow->SetStatus(LAS_CR_EXTERNAL_VAR_IN_SUBSCRIPT);
	}


	//Проверяем, получено ли описание границ циклов, и на сколько полно.
	int	n = lampArrow->commonLoopNumb;//Будем проверять только те описания циклов, которые использовались при построении дуги с помощью неравенств Банержи
	for(int i=0;i<n;i++)
	{
		if (srcOccur.loops[i].lBoundUnk || 
			srcOccur.loops[i].rBoundUnk ||
			depOccur.loops[i].lBoundUnk ||
			depOccur.loops[i].rBoundUnk)
		{
			lampArrow->SetStatus(LAS_CR_SOME_COMMON_LOOPS_WASNT_PARSERED_CAREFULLY);
			break;
		}
	}
}

class CarrierPred
{
public:
	CarrierPred(ForStatement& loop):m_loop(loop) {}
	bool operator()(LampArrow& arrow) const
	{
		return arrow.TestSupp(m_loop) &&
			   arrow.srcOccurDesc->IsIncludedByForBody(&m_loop) &&
			   arrow.depOccurDesc->IsIncludedByForBody(&m_loop);
	}
	ForStatement& m_loop;
};

LampArrowList getSubGraphByCarrier(LamportGraph& depGraph, ForStatement& loop)
{
	return getSubGraphByPred(depGraph, CarrierPred(loop));
}

class LoopBodyPred
{
public:
	LoopBodyPred(ForStatement& loop):m_loop(loop) {}
	bool operator()(LampArrow& arrow) const
	{
		return arrow.srcOccurDesc->IsIncludedByForBody(&m_loop) &&
			   arrow.depOccurDesc->IsIncludedByForBody(&m_loop);
	}
	ForStatement& m_loop;
};

LampArrowList getSubGraphByLoopBody(LamportGraph& depGraph, ForStatement& loop)
{
	return getSubGraphByPred(depGraph, LoopBodyPred(loop));
}

}
