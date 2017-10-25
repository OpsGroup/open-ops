#include"Analysis/DepGraph/LoopStruct.h"
#include <iostream>
#include "OPS_Core/msc_leakcheck.h"
#include "OPS_Core/Localization.h"
#include "Analysis/LatticeGraph/LatticeGraph.h"
#include "Shared/LoopShared.h"

namespace DepGraph
{
using namespace OPS::LatticeGraph;

LoopBounds::LoopBounds(int _lowerNumb,int _upperNumb):m_lowerNumb(_lowerNumb),m_upperNumb(_upperNumb)
{
	if(m_lowerNumb)
    {
		m_lower=new SimpleLinearExpression[m_lowerNumb];
    }
	else
		m_lower=NULL;

	if(m_upperNumb)
    {
		m_upper=new SimpleLinearExpression[m_upperNumb];
    }
	else
		m_upper=NULL;
}


void LoopBounds::clear()
{
	if (m_lower!=NULL)
	{
		delete[] m_lower;
		m_lower=NULL; 
        m_lowerNumb=0;
	}

	if(m_upper!=NULL)
	{
		delete[] m_upper;
		m_upper=NULL;
        m_upperNumb=0;
	}

}

LoopBounds::LoopBounds(const LoopBounds& lb)
	:m_lower(NULL)
	,m_upper(NULL)
	,m_lowerExternalParamCoefs(lb.m_lowerExternalParamCoefs)
	,m_upperExternalParamCoefs(lb.m_upperExternalParamCoefs)
	,m_lowerNumb(0)
	,m_upperNumb(0)	
{
    clear();
	m_lowerNumb=lb.m_lowerNumb;
	if(m_lowerNumb)
	{
		m_lower=new SimpleLinearExpression[m_lowerNumb];
		for(int i=0;i<m_lowerNumb;i++)
        {
			m_lower[i]=lb.m_lower[i];
        }
	}
	else
    {
		m_lower=NULL;
    }

	m_upperNumb=lb.m_upperNumb;
	if(m_upperNumb)
	{
		m_upper=new SimpleLinearExpression[m_upperNumb];
		for(int i=0;i<m_upperNumb;i++)
        {
			m_upper[i]=lb.m_upper[i];
        }
	}
	else
    {
		m_upper=NULL;
    }
}

LoopBounds& LoopBounds::operator= (const LoopBounds& lb)
{
	if(this!=&lb)
	{
		clear();
        m_lowerNumb=lb.m_lowerNumb;
        if(m_lowerNumb)
        {
            m_lower=new SimpleLinearExpression[m_lowerNumb];
			m_lowerExternalParamCoefs = lb.m_lowerExternalParamCoefs;
            for(int i=0;i<m_lowerNumb;i++)
            {
                m_lower[i]=lb.m_lower[i];
            }
        }
        else
        {
			m_lower=NULL;
        }

        m_upperNumb=lb.m_upperNumb;
        if(m_upperNumb)
        {
            m_upper=new SimpleLinearExpression[m_upperNumb];
			m_upperExternalParamCoefs = lb.m_upperExternalParamCoefs;
            for(int i=0;i<m_upperNumb;i++)
            {
                m_upper[i]=lb.m_upper[i];
            }
        }
        else
        {
			m_upper=NULL;
        }
	}
return *this;
}

//!!!!!!!!!!!!!!!1TODO: надо еще выводить на печать коэффициенты при внешних переменных!!!!!!!!!!!!!!!!!!!!!!!11
void LoopBounds::print(std::ostream& os,std::string* paramNames,int paramNamesDim,int mode,int position)
{
	switch(mode)
	{
	case 0:
		{
//		os.width(30);//os.flags(ios::left);
		os<<_TL("lower bounds:","")<<"..........";
//		os.width(30);//os.flags(ios::right);
		os<<_TL("upper bounds:\n","");
	
		int n=m_lowerNumb>m_upperNumb?m_lowerNumb:m_upperNumb;

		for(int i=0;i<n;i++)
		{
//			os.width(30);os.flags(ios::left);
			if(i<m_lowerNumb)
				m_lower[i].Print(os,paramNames,paramNamesDim);
			else
				os<<' ';
			os<<"..........";
//			os.width(30);os.flags(ios::right);
			if(i<m_upperNumb)
				m_upper[i].Print(os,paramNames,paramNamesDim);
			else
				os<<' ';
			os<<std::endl;
		}
		break;
		}
	case 1:
		{
		os.width(5*position);
		if(m_lowerNumb>1)os<<"max(";
		for(int i=0;i<m_lowerNumb;i++)
		{
			m_lower[i].Print(os,paramNames,paramNamesDim);
			if(i!=(m_lowerNumb-1))os<<',';
		}

		if(m_lowerNumb>1)os<<')';
		os<<" ... ";
		if(m_upperNumb>1)os<<"min(";
		for(int i=0;i<m_upperNumb;i++)
		{
			m_upper[i].Print(os,paramNames,paramNamesDim);
			if(i!=(m_upperNumb-1))os<<',';
		}
		if(m_upperNumb>1)os<<')';

		os<<std::endl;
	
		break;
		}
	}
}

void LoopBoundsNode::Clear()
{
	delete loopBounds;
	loopBounds=NULL;
	if(pointers)
	{
		for(int i=0;i<pointerNumb;i++)
			delete pointers[i];
		delete[] pointers;
		pointers=NULL;
		pointerNumb=0;
	}
}

OPS::Shared::CanonicalLinearExpression LoopBounds::getLowerFreeExpression() const
{
	if (m_lowerNumb != 0 && m_lower != 0)
	{
		OPS::Shared::CanonicalLinearExpression res(m_lowerExternalParamCoefs);
		// Добавляем свободный член
		res.add(m_lower[0].m_coefs[0]);
		return res;
	}
	else
	{
		// Нет свободного члена
		return m_lowerExternalParamCoefs;
	}
}

OPS::Shared::CanonicalLinearExpression LoopBounds::getUpperFreeExpression() const
{
	if (m_upperNumb != 0 && m_upper != 0)
	{
		OPS::Shared::CanonicalLinearExpression res(m_upperExternalParamCoefs);
		// Добавляем свободный член
		res.add(m_upper[0].m_coefs[0]);
		return res;
	}
	else
	{
		// Нет свободного члена
		return m_upperExternalParamCoefs;
	}
}

LoopBoundsNode::LoopBoundsNode(LoopBounds* lb,int _pointerNumb):pointerNumb(_pointerNumb)
{
	loopBounds=lb;
	if(pointerNumb)
	{
		pointers=new LoopBoundsNode*[pointerNumb];
		for(int i=0;i<pointerNumb;i++)pointers[i]=NULL;
	}
	else
		pointers=NULL;
}

													//==================Исправить!=======================
OPS::Reprise::ExpressionBase* GetLeftExprNode(LoopBounds& lb,OPS::Reprise::Declarations& ns,std::string* varNames,OPS::Reprise::SubroutineDeclaration& maxFuncIter)
{
	using namespace OPS::Reprise;
	if(lb.m_lowerNumb==0)return NULL;

	if(lb.m_lowerNumb==1)return GetExprNode(lb.m_lower[0],ns,varNames);

	SubroutineCallExpression* res,*curr;
	
	//curr=res=ExprCall::create(maxFuncIter.getNameRef(),GetExprNode(lb.m_lower[0],ns,varNames));
	
	
	curr = res = new SubroutineCallExpression( new SubroutineReferenceExpression(maxFuncIter) );
	curr->addArgument(GetExprNode(lb.m_lower[0],ns,varNames));
	res->addArgument(GetExprNode(lb.m_lower[0],ns,varNames));

	res->addArgument(GetExprNode(lb.m_lower[1],ns,varNames));

	for(int i=2;i<(lb.m_lowerNumb-1);i++)
	{
//		ExprCall* temp;
//		curr->addArgument(temp=ExprCall::create(maxFuncIter,GetExprNode(lb.m_lower[i],ns,varNames)));
//		curr=temp;
		curr=res;
		//res=ExprCall::create(maxFuncIter.getNameRef(),GetExprNode(lb.m_lower[i],ns,varNames));
		res = new SubroutineCallExpression( new SubroutineReferenceExpression(maxFuncIter) );
		res->addArgument(GetExprNode(lb.m_lower[i],ns,varNames));
		res->addArgument(curr);
	}

//	curr->addArgument(ExprCall::create(maxFuncIter,GetExprNode(lb.m_lower[lb.m_lowerNumb-1],ns,varNames)));

	return res;
}
														//==================Исправить!=======================
OPS::Reprise::ExpressionBase* GetRightExprNode(LoopBounds& lb,OPS::Reprise::Declarations& ns,std::string* varNames,OPS::Reprise::SubroutineDeclaration& minFuncIter)
{
	using namespace OPS::Reprise;

	if(lb.m_upperNumb==0)return NULL;

	if(lb.m_upperNumb==1)return GetExprNode(lb.m_upper[0],ns,varNames);

	SubroutineCallExpression* res,*curr;
	
	//curr=res=ExprCall::create(minFuncIter.getNameRef(),GetExprNode(*(lb.m_upper[0]),ns,varNames));
	curr = res = new SubroutineCallExpression(new SubroutineReferenceExpression(minFuncIter));
	curr->addArgument(GetExprNode(lb.m_upper[0],ns,varNames));
	res->addArgument(GetExprNode(lb.m_upper[0],ns,varNames));

	res->addArgument(GetExprNode(lb.m_lower[1],ns,varNames));

	for(int i=2;i<(lb.m_upperNumb-1);i++)
	{
//		ExprCall* temp;
//		curr->addArgument(temp=ExprCall::create(minFuncIter,GetExprNode(*(lb.m_upper[i]),ns,varNames)));
//		curr=temp;
		curr=res;
		//res=ExprCall::create(minFuncIter.getNameRef(),GetExprNode(lb.m_lower[i],ns,varNames));
		res = new SubroutineCallExpression(new SubroutineReferenceExpression(minFuncIter));
		res->addArgument(GetExprNode(lb.m_lower[i],ns,varNames));
 		res->addArgument(curr);
	}

//	curr->addArgument(ExprCall::create(minFuncIter,GetExprNode(lb.m_upper[lb.m_upperNumb-1],ns,varNames)));

	return res;
}

//строит описание ОДНОЙ границы одного цикла
//возвращает 0 - все нормально, 1 - цикл неканонический или границы нелинейные
static void buildOneLinearBound(LatticeGraph::SimpleLinearExpression& resultCounterCoefs\
						 , OPS::Shared::CanonicalLinearExpression& resultExternalParamCoefs\
						 , bool& resultFlagNotLinear\
						 , bool& resultFlagUnknown\
						 , OPS::Reprise::ExpressionBase* boundExpression \
						 , std::vector<OPS::Reprise::VariableDeclaration*>& loopIndexDeclarations\
						 , bool flagIsBoundIsLeft)
{
	//проверяем, является ли выражение для левой границы каноническим
	bool decreaseBound = false;//если верхняя граница задана как < то ее надо уменьшить
	bool flagLoopIsCanonical = true;
	if (flagIsBoundIsLeft==true)
	{
		//для левой границы
		if ( boundExpression->is_a<OPS::Reprise::BasicCallExpression>() )
		{
			OPS::Reprise::BasicCallExpression* tempExpr = &boundExpression->cast_to<OPS::Reprise::BasicCallExpression>();
			if(tempExpr->getKind() != OPS::Reprise::BasicCallExpression::BCK_ASSIGN )
				flagLoopIsCanonical = false;
		}
		else
			flagLoopIsCanonical = false;
	}
	else
	{
		//для правой границы
		if ( boundExpression->is_a<OPS::Reprise::BasicCallExpression>() )
		{
			OPS::Reprise::BasicCallExpression* tempExpr = &boundExpression->cast_to<OPS::Reprise::BasicCallExpression>();
			if (( tempExpr->getKind() != OPS::Reprise::BasicCallExpression::BCK_LESS ) &&
				( tempExpr->getKind() != OPS::Reprise::BasicCallExpression::BCK_LESS_EQUAL ))
				flagLoopIsCanonical = false;
			if ( tempExpr->getKind() == OPS::Reprise::BasicCallExpression::BCK_LESS )
				decreaseBound = true;
		}
		else
			flagLoopIsCanonical = false;
	}
	if (!flagLoopIsCanonical)
	{
		resultFlagUnknown = true;
		resultFlagNotLinear = true;
		return;
	}
	//КОНЕЦ (проверяем, является ли выражение для левой границы каноническим)

	//проверяем, является ли граница линейным выражением и заполняем коэффициенты
	OPS::Reprise::ExpressionBase* bound = &boundExpression->cast_to<OPS::Reprise::CallExpressionBase>().getArgument(1);
	try
	{
		OPS::Shared::ParametricLinearExpression* parametricLinExprOfBound
			= OPS::Shared::ParametricLinearExpression::createByAllVariables(bound);
		if (parametricLinExprOfBound!=0) //выражение совсем нелинейное
		{
			//заполняем массивы коэффициентов при счетчиках циклов и внешних параметрах
			getExternalParamAndLoopCounterCoefficients
				(*parametricLinExprOfBound, loopIndexDeclarations, resultCounterCoefs, resultExternalParamCoefs);
			if (!flagIsBoundIsLeft && decreaseBound) resultCounterCoefs[0]--;
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
	//КОНЕЦ (проверяем, является ли граница линейным выражением и заполняем коэффициенты)
}

//находит минимальные и максимальные значения счетчиков
void findMinAndMaxCounterValues(std::vector<DepGraph::LoopDesc>& loopDesc)
{
	int loopNum = loopDesc.size();
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

/// строит описание гнезда циклов данного вхождения
/// рассматриваются, только циклы, содержащиеся внутри code
/// Если code = 0, возвращает все циклы
std::vector<DepGraph::LoopDesc> buildLoopDesc(OPS::Reprise::ExpressionBase *expr, OPS::Reprise::RepriseBase *code)
{
	std::vector<DepGraph::LoopDesc> loopDesc;//результат

	//конструируем вектор со счетчиками циклов
	std::vector<OPS::Reprise::VariableDeclaration*> loopIndexDeclarations
		= OPS::Shared::getIndexVariables(expr, code);
	std::list<OPS::Reprise::ForStatement*> loops = OPS::Shared::getEmbracedLoopsNest(*expr, code);
	OPS_ASSERT(loopIndexDeclarations.size() == loops.size());

	int loopCount = loops.size();
	if (loopCount == 0) return loopDesc;

	loopDesc.resize(loopCount);

	// выделяем память для хранения линейных выражений границ
	for(int i = 0; i < loopCount; ++i)
	{
		loopDesc[i].loopBounds.m_lower = new OPS::LatticeGraph::SimpleLinearExpression[1];
		loopDesc[i].loopBounds.m_lowerExternalParamCoefs.clear();
		loopDesc[i].loopBounds.m_lower[0] = OPS::LatticeGraph::SimpleLinearExpression(i+1);
		loopDesc[i].loopBounds.m_lowerNumb = 1;

		loopDesc[i].loopBounds.m_upper = new OPS::LatticeGraph::SimpleLinearExpression[1];
		loopDesc[i].loopBounds.m_upperExternalParamCoefs.clear();
		loopDesc[i].loopBounds.m_upper[0] = OPS::LatticeGraph::SimpleLinearExpression(i+1);
		loopDesc[i].loopBounds.m_upperNumb = 1;
	}
	std::list<OPS::Reprise::ForStatement*>::iterator it = loops.begin();
	//цикл по всем циклам loops
	for (int i = 0; i < loopCount; ++i, ++it)
	{
		//ссылка на счетчик
		loopDesc[i].counterIter=loopIndexDeclarations[i];

		//ссылка на цикл во внутреннем представлении
		loopDesc[i].stmtFor = *it;

		//левая граница
		buildOneLinearBound(loopDesc[i].loopBounds.m_lower[0], loopDesc[i].loopBounds.m_lowerExternalParamCoefs
		, loopDesc[i].lBoundNotLinear, loopDesc[i].lBoundUnk, &(*it)->getInitExpression(), loopIndexDeclarations, true);

		//правая граница
		buildOneLinearBound(loopDesc[i].loopBounds.m_upper[0], loopDesc[i].loopBounds.m_upperExternalParamCoefs
		, loopDesc[i].rBoundNotLinear, loopDesc[i].rBoundUnk, &(*it)->getFinalExpression(), loopIndexDeclarations, false);
	}
	//заполняем минимальные и максимальные значения счетчиков
	findMinAndMaxCounterValues(loopDesc);

	return loopDesc;
}
}
