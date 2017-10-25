#include "Analysis/LatticeGraph/LinearLib.h"
#include "Analysis/LatticeGraph/PIP.h"
#include "Analysis/LatticeGraph/ComplexCondition.h"
#include "Analysis/Montego/Occurrence.h"
#include "Shared/LoopShared.h"
#include "Analysis/DepGraph/DepGraph.h"
#include <fstream>
#include <cassert>
#include <sstream>

#include "OPS_Core/msc_leakcheck.h"//этот файл должен подключаться в конце всех include

#ifdef LATTICE_TABLES_DEBUG
//extern std::ofstream file;
//extern int deepOfTheSimplexSearch;
#endif

namespace OPS
{
namespace LatticeGraph
{

static OPS::Reprise::ReferenceExpression* findByName(OPS::Reprise::Declarations& ns, const std::string& name);


SimpleLinearExpression& SimpleLinearExpression::operator=(const SimpleLinearExpression& linearExpr)
{
	if((&linearExpr)!=this)
	{
		Clear();
		m_dim=linearExpr.m_dim;
		m_coefs=new int[m_dim];
		memcpy(m_coefs, linearExpr.m_coefs, m_dim*sizeof(int));
	}
	return *this;
}

SimpleLinearExpression::SimpleLinearExpression(const SimpleLinearExpression& linearExpr)
{
    m_dim=linearExpr.m_dim;
    m_coefs=new int[m_dim];
    memcpy(m_coefs,linearExpr.m_coefs,m_dim*sizeof(int));
}

void SimpleLinearExpression::allocateMem(int dim)
{
    OPS_ASSERT((dim>=0) && (dim<=1000000))
    Clear();
    m_dim=dim; 
    m_coefs = new int[m_dim]; 
    for (int i=0;i<m_dim;i++) m_coefs[i]=0;
}

void SimpleLinearExpression::Print(std::ostream& os,const char** paramNames,int paramNamesDim)
{
	os<<m_coefs[0];
	for(int i=1;i<m_dim;i++)
	{
		if(m_coefs[i]>=0)os<<'+';
		os<<m_coefs[i];
		if(paramNames)
		{
			os<<'*';
			if((i-1)<paramNamesDim)
				os<<paramNames[i-1];
			else
			{
				//char buf[8];
				//sprintf(buf,"_%d",i-paramNamesDim);
				//os<<buf;
				os<<'_'<<(i-paramNamesDim);
			}
		}
	}
}

std::string SimpleLinearExpression::toString(const std::vector<std::string>& paramNames)
{
    OPS_ASSERT((int)paramNames.size()>=m_dim-1);
    if ((int)paramNames.size()<m_dim-1) throw OPS::RuntimeError("Количество имен параметров, подаваемых в функцию SimpleLinearExpression::toString должно совпадать с SimpleLinearExpression.m_dim-1");
    std::ostringstream os;

	bool isFirstSummand = true;

	if (m_coefs[0] != 0)
	{
		os << m_coefs[0];
		isFirstSummand = false;
	}

    for(int i=1;i<m_dim;i++)
    {
		if (m_coefs[i] == 0) continue;

		bool coeffIsOne = false;
		if (m_coefs[i] > 0)
		{
			if (!isFirstSummand) os << '+';
			if (m_coefs[i] != 1)
				os << m_coefs[i];
			else
				coeffIsOne = true;
		}
		else
		{
			if (m_coefs[i] != -1)
				os << m_coefs[i];
			else
			{
				os << "-";
				coeffIsOne = true;
			}
		}

        if(paramNames.size()>0)
        {
			if (!coeffIsOne)
				os << '*';
            if((i-1)<(int)paramNames.size())
                os<<paramNames[i-1];
            else
            {
                //char buf[8];
                //sprintf(buf,"_%d",i-paramNamesDim);
                //os<<buf;
                os<<'_'<<(i-paramNames.size());
            }
			isFirstSummand = false;
        }
    }
    return os.str();
}

std::string SimpleLinearExpression::toString()
{
    std::vector<std::string> paramNames(m_dim-1);
    for (int i=0; i<m_dim-1; i++) paramNames[i]='x'+OPS::Strings::format("%d",i);
    return toString(paramNames);
}

void SimpleLinearExpression::Print(std::ostream& os,const std::string* paramNames,int paramNamesDim)
{
	os<<m_coefs[0];
	for(int i=1;i<m_dim;i++)
	{
		if(m_coefs[i]>=0)os<<'+';
		os<<m_coefs[i];
		if(paramNames)
		{
			os<<'*';
			if((i-1)<paramNamesDim)
				os<<paramNames[i-1];
			else
			{
				//char buf[8];
				//sprintf(buf,"_%d",i-paramNamesDim);
				//os<<buf;
				os<<'_'<<(i-paramNamesDim);
			}
		}
	}
}

//вычисляет линейное выражение
long SimpleLinearExpression::evaluate(std::vector<int>& paramValues)
{
    if (m_dim==0) return 0;
    long res=m_coefs[0];
    for (int i=1;i<m_dim;i++) res+=m_coefs[i]*paramValues[i-1];
    return res;
}

long SimpleLinearExpression::evaluate(const std::vector<int>& paramValues)
{
    return evaluate((std::vector<int>&)paramValues);
}


//Применяет трансформацию к линейному выражению
//в трансформационной матрице по строчкам записаны выражения старых переменных через новые
//Последний элемент каждой строки трансформационной матрицы - св. член. 
//если строчек меньше, чем переменных, то заменяет только те, которые заменить нужно согласно матрице (при этом 
//коэффициенты при не заменяемых параметрах могут поменяться, если они участвуют в замене, т.е. если dimj-1>dimi)
//свободный член замене не подлежит (это не значи что он не меняется вследствии изменения других параметров)
//т.е. если dimi>=m_dim, то строки матрицы трансформации начиная с m_dim-1 игнорируются
//ВНИМАНИЕ!!! ЕСЛИ dimi<m_dim-1, то СЧИТАЕТСЯ, ЧТО ОСТАВШИЕСЯ ПАРАМЕТРЫ В m_coefs (с dimi по m_dim)
//И НОВЫЕ ПАРАМЕТРЫ - ЭТО СОВЕРШЕННО РАЗНЫЕ ПАРАМЕТРЫ 
//ЕСЛИ У ВАС ПО СМЫСЛУ ЗАДАЧИ ОНИ ОДИНАКОВЫЕ, ТО БУДБТЕ ДОБРЫ ДОБАВЬТЕ ЕЩЕ СТРОЧЕК В matrix 
//C ЕДИНИЦАМИ НА ГЛАВНОЙ ДИАГОНАЛИ
void SimpleLinearExpression::transform(int** matrix,int dimi,int dimj)
{    
    OPS_ASSERT(dimi<=m_dim-1);
    int m_dimNew=m_dim+(dimj-1)-dimi;
	int* newData=new int[m_dimNew];
	for(int j=1;j<dimj;j++)	{
		newData[j]=0;
		for(int i=0;i<dimi;i++)	newData[j]+=m_coefs[i+1]*matrix[i][j-1];
	}
	//возьня со св. членом...
	newData[0]=m_coefs[0];
	for(int i=0;i<dimi;i++)	newData[0]+=m_coefs[i+1]*matrix[i][dimj-1];
    //переписываем неизменяемую часть
    for (int j=dimj;j<m_dimNew;j++) newData[j]=m_coefs[j-dimj+dimi+1];
	delete[] m_coefs;
	m_coefs=newData;
	m_dim=m_dimNew;
}

void SimpleLinearExpression::insertNZerosBeforeNewParamCoefs(int N, int varNum)//varNum - кол-во обычных переменных (не новых параметров)
{
    OPS_ASSERT(varNum <= m_dim-1);
    int* m_coefsNew = new int[m_dim+N];
    for (int i=0;i<varNum+1;i++) m_coefsNew[i]=m_coefs[i];
    for (int i=0;i<N;i++) m_coefsNew[1+varNum+i]=0;
    for (int i=0;i<m_dim-varNum-1;i++) m_coefsNew[i+varNum+N+1]=m_coefs[i+varNum+1];
    delete[] m_coefs;
    m_coefs=m_coefsNew;
    m_dim+=N;
}

void SimpleLinearExpression::deleteNZerosBeforeNewParamCoefs(int N, int varNum)
//varNum - кол-во обычных переменных (не новых параметров)
//удаляем коэффициенты с varNum-N+1 до varNum
{
    OPS_ASSERT(varNum <= m_dim-1);
    OPS_ASSERT(m_dim-N>0);
    int* m_coefsNew = new int[m_dim-N];
    for (int i = 0; i < varNum-N+1; i++) m_coefsNew[i] = m_coefs[i];
    for (int i = 0; i < m_dim - varNum - 1; i++) m_coefsNew[i+varNum-N+1] = m_coefs[i+varNum+1];
	delete[] m_coefs;
    m_coefs = m_coefsNew;
    m_dim -= N;
}

SimpleLinearExpression::SimpleLinearExpression()
{
    m_coefs=NULL; 
    m_dim=0;
}
SimpleLinearExpression::SimpleLinearExpression(int dim):m_dim(dim) 
{ 
    m_coefs = new int[m_dim]; 
    MakeZero(); 
}
SimpleLinearExpression::SimpleLinearExpression(const int* _data, int _dim):m_dim(_dim) 
{
    m_coefs = new int[m_dim]; 
    memcpy(m_coefs, _data, m_dim*sizeof(m_coefs[0]));
}
SimpleLinearExpression::~SimpleLinearExpression() 
{ 
    Clear(); 
}

void SimpleLinearExpression::Clear() 
{
	delete[] m_coefs;
    m_coefs = NULL; 
    m_dim = 0; 
}

void SimpleLinearExpression::MakeZero() 
{ 
    if(m_coefs) memset(m_coefs, 0, m_dim*sizeof(m_coefs[0])); 
}
int& SimpleLinearExpression::operator[](int index) 
{
    OPS_ASSERT((index >= 0) && (index < m_dim));
    return m_coefs[index];
}
int& SimpleLinearExpression::at(int index)
{ 
    OPS_ASSERT((index >= 0) && (index < m_dim));
    return m_coefs[index]; 
}
void SimpleLinearExpression::multiply(int p)//умножает линейное выражение на p
{
    for (int i=0; i<m_dim; i++) m_coefs[i]*=p;
}
void SimpleLinearExpression::substract(SimpleLinearExpression& subtrahend)
{
    OPS_ASSERT(m_dim==subtrahend.m_dim); 
    for (int i=0; i<m_dim; i++) 
        m_coefs[i]-=subtrahend.m_coefs[i];
}
void SimpleLinearExpression::add(SimpleLinearExpression& summand)
{
    OPS_ASSERT(m_dim==summand.m_dim); 
    for (int i=0;i<m_dim;i++) m_coefs[i]+=summand.m_coefs[i];
}

Inequality::Inequality()
{
    m_coefs=NULL; 
    m_dim=0;
}
Inequality::Inequality(int dim):SimpleLinearExpression(dim)
{
}
Inequality::Inequality(const int* data,int dim):SimpleLinearExpression(data,dim)
{
}
Inequality::Inequality(const SimpleLinearExpression& in):SimpleLinearExpression(in.m_coefs,in.m_dim)
{
}
Inequality::~Inequality()
{
    SimpleLinearExpression::Clear();
}

Inequality& Inequality::operator=(const Inequality& in)
{
    if((&in)!=this)
    {
        Clear();
        m_dim=in.m_dim;
        m_coefs=new int[m_dim];
        memcpy(m_coefs, in.m_coefs, m_dim*sizeof(int));
    }
    return *this;
}




void Inequality::Print(std::ostream& os,const char** paramNames,int paramNamesDim,int mode)
{
	switch(mode)
	{
	case 0:// a0+a1*x1+...an*xn>=0
		{
			os<<m_coefs[0];
			for(int i=1;i<m_dim;i++)
			{
				if(m_coefs[i]>=0)os<<'+';
				os<<m_coefs[i];
				if(paramNames)
				{
					os<<'*';
					if((i-1)<paramNamesDim)
						os<<paramNames[i-1];
					else
					{
						//char buf[8];
						//sprintf(buf,"_%d",i-paramNamesDim);
						//os<<buf;
						os<<'_'<<(i-paramNamesDim);
					}
				}
			}
			os<<">=0";//<<std::endl;
		}
		break;
	case 1:// a1*x1+...+an*xn<=a0
		{
			for(int i=1;i<m_dim;i++)
			{
				if(m_coefs[i]<=0)os<<'+';
				os<<-m_coefs[i];
				if(paramNames)
				{
					os<<'*';
					if((i-1)<paramNamesDim)
						os<<paramNames[i-1];
					else
					{
						//char buf[8];
						//sprintf(buf,"_%d",i-paramNamesDim);
						//os<<buf;
						os<<'_'<<(i-paramNamesDim);
					}
				}
			}
			os<<"<="<<m_coefs[0];//<<std::endl;
		}
		break;
	}

}

void Inequality::Print(std::ostream& os,const std::string* paramNames,int paramNamesDim,int mode)
{
	switch(mode)
	{
	case 0:// a0+a1*x1+...an*xn>=0
		{
			os << m_coefs[0];
			for(int i=1;i<m_dim;i++)
			{
				if(m_coefs[i]>=0)os<<'+';
				os<<m_coefs[i];
				if(paramNames)
				{
					os<<'*';
					if((i-1)<paramNamesDim)
						os<<paramNames[i-1];
					else
					{
						//char buf[8];
						//sprintf(buf,"_%d",i-paramNamesDim);
						//os<<buf;
						os<<'_'<<(i-paramNamesDim);
					}
				}
			}
			os<<">=0";//<<std::endl;
		}
		break;
	case 1:// a1*x1+...+an*xn<=a0
		{
			for(int i = 1; i < m_dim; ++i)
			{
				if (m_coefs[i] <= 0)
					os << '+';
				os << -m_coefs[i];
				if (paramNames)
				{
					os << '*';
					if ((i-1) < paramNamesDim)
						os << paramNames[i-1];
					else
					{
						//char buf[8];
						//sprintf(buf, "_%d", i-paramNamesDim);
						//os << buf;
						os<<'_'<<(i-paramNamesDim);
					}
				}
			}
			os << " <= " << m_coefs[0];//<<std::endl;
		}
		break;
	}

}

Polyhedron::Polyhedron(const Polyhedron& ct)
{
	InequalityList::const_iterator first=ct.m_ins.begin(),last=ct.m_ins.end();
	for(; first != last; ++first) {
		m_ins.push_back(new Inequality(**first));
	}

	feasibilityState = ct.GetFeasibilityState();
    m_externalParamsVector=ct.m_externalParamsVector;

}

//составляет опорный многогранник для вхождения. Порядок следования коэффициентов неравенств 
//определяется varOrder (там должны быть и счетчики циклов и внешние параметры)
//если varOrder=0, то он составляется автоматически
Polyhedron::Polyhedron(OPS::Montego::BasicOccurrence& o, 
           OPS::Reprise::RepriseBase* code, 
           int deepLimit, 
           std::vector<OPS::Reprise::VariableDeclaration*>* externalParamsVector)
{
    feasibilityState = FEASIBILITY_PROVEN;
    int loopCount = OPS::Shared::getEmbracedLoopsCount(o.getRefExpr(), code);
    if (loopCount == 0)  
        return;

    if (deepLimit==-1) deepLimit = loopCount;

	std::vector<DepGraph::LoopDesc> loopDesc = DepGraph::buildLoopDesc(o.getRefExpr(), code);

    for (int i = 0; i < deepLimit; i++)
    {
        //левая граница (объединять обе границы в один if нельзя!!!!)
        if ( loopDesc[i].lBoundUnk ) 
        {
            if (loopDesc[i].lBoundNotLinear) 
                //граница цикла неизвестна и не является линейным выражением
                return;
            else
            {
                //граница цикла неизвестна и является линейным выражением, но содержат оператор max,
                //тогда контекст построить пока не можем (нужен ККАФ-контекст (см. статью Гуды про ККАФ))
                if (loopDesc[i].loopBounds.m_lowerNumb>1) return;
            }
        } 
        //правая граница (объединять обе границы в один if нельзя!!!!)
        if ( loopDesc[i].rBoundUnk ) 
        {
            if (loopDesc[i].rBoundNotLinear) 
                //граница цикла неизвестна и не является линейным выражением
                return;
            else
            {//граница цикла неизвестна и является линейным выражением, но содержат оператор max,
                //тогда контекст построить пока не можем (нужен ККАФ-контекст (см. статью Гуды про ККАФ))
                if (loopDesc[i].loopBounds.m_upperNumb>1) return;
            }
        } 
    }
    if (externalParamsVector == 0)
    {
        //собираем все внешние параметры, от которых зависит Context (во всех границах всех циклов)
        std::set<OPS::Reprise::VariableDeclaration*> AllExternalParamsSet;
        std::set<OPS::Reprise::VariableDeclaration*> indexVariables;
        for(int i = 0; i < deepLimit; ++i)	
        {
            indexVariables.insert(loopDesc[i].counterIter);
            {
                //получаем список параметров левой границы
                std::map<OPS::Reprise::VariableDeclaration*, long>
				    M=loopDesc[i].loopBounds.m_lowerExternalParamCoefs.getMap();
                std::map<OPS::Reprise::VariableDeclaration*, long>::iterator j;
                for (j=M.begin(); j!=M.end(); ++j)
                    if (indexVariables.find(j->first) == indexVariables.end()) 
                        AllExternalParamsSet.insert(j->first);
            }
            {
                //получаем список параметров правой границы
                std::map<OPS::Reprise::VariableDeclaration*, long> 
				    M=loopDesc[i].loopBounds.m_upperExternalParamCoefs.getMap();
                std::map<OPS::Reprise::VariableDeclaration*, long>::iterator j;
                for (j=M.begin(); j!=M.end(); ++j)
                    if(indexVariables.find(j->first) == indexVariables.end()) AllExternalParamsSet.insert(j->first);
            }
        }
        int externalParamNum = (int)AllExternalParamsSet.size();
        //преобразуем set в list, чтобы зафиксировать порядок следования внешних параметров
        std::vector<OPS::Reprise::VariableDeclaration*> AllExternalParamsVector(externalParamNum);
        std::set<OPS::Reprise::VariableDeclaration*>::iterator it;
        int i=0;
        for (it=AllExternalParamsSet.begin(); it!=AllExternalParamsSet.end(); ++it,++i) AllExternalParamsVector[i]=*it;
        //запоминаем порядок следования и список внешних параметров в поле m_externalParamsVector контекста
        this->m_externalParamsVector=AllExternalParamsVector;
    }
    else 
        m_externalParamsVector = *externalParamsVector;
    //Inequality inq(1 + entry.loopNumb + externalParamNum);
    LatticeGraph::Inequality inq(1 + deepLimit + m_externalParamsVector.size());
    inq.MakeZero();

    //for(int i = 0; i < entry.loopNumb; ++i) 
    for(int i = 0; i < deepLimit; ++i)	{
        //НЕРАВЕНСТВО ДЛЯ ЛЕВОЙ ГРАНИЦЫ ЦИКЛА
        inq.m_coefs[i+1] = 1;
        for(int j=0;j<(i+1);j++)
        {
            //заносим свободный член и коэффициенты перед счетчиками циклов
            inq.m_coefs[j]=-loopDesc[i].loopBounds.m_lower[0][j];
        }
        //заносим коэффициенты перед внешними параметрами
        for (size_t j=0; j<m_externalParamsVector.size(); ++j) {
			std::map<OPS::Reprise::VariableDeclaration*, long> M = loopDesc[i].loopBounds.m_lowerExternalParamCoefs.getMap();
            std::map<OPS::Reprise::VariableDeclaration*, long>::iterator p = M.find(m_externalParamsVector[j]);
            if (p!=M.end()) inq.m_coefs[1 + deepLimit + j] = - p->second; //параметр найден
            else inq.m_coefs[1 + deepLimit + j] = 0; //не найден
        }
        this->AddInequality(inq);//добавили неравенство в контекст

        //НЕРАВЕНСТВО ДЛЯ ПРАВОЙ ГРАНИЦЫ ЦИКЛА
        //заносим свободный член и коэффициенты перед счетчиками циклов
        inq.m_coefs[i+1] = -1;
        memcpy(inq.m_coefs,loopDesc[i].loopBounds.m_upper[0].m_coefs,(i+1)*sizeof(int));
        //заносим коэффициенты перед внешними параметрами
        for (size_t j=0; j<m_externalParamsVector.size(); ++j) {
			std::map<OPS::Reprise::VariableDeclaration*, long> M = loopDesc[i].loopBounds.m_upperExternalParamCoefs.getMap();
            std::map<OPS::Reprise::VariableDeclaration*, long>::iterator p = M.find(m_externalParamsVector[j]);
            if (p!=M.end()) inq.m_coefs[1 + deepLimit + j] = p->second; //параметр найден
            else inq.m_coefs[1 + deepLimit + j] = 0; //не найден
        }
        this->AddInequality(inq);//добавили неравенство в контекст
    }
    feasibilityState = UNKNOWN_FEAS;
}

void Polyhedron::Clear()
{
	InequalityList::iterator first = m_ins.begin(), last = m_ins.end();
	for(; first != last; ++first) {
		delete *first;
	}
	m_ins.clear();
	feasibilityState = FEASIBILITY_PROVEN;
}

Inequality* Polyhedron::AddNewInequality(int dim)
{
	Inequality* res;
	m_ins.push_back(res=new Inequality(dim));
	feasibilityState = UNKNOWN_FEAS;
	return res;
}


void Polyhedron::AddInequality(const int* data,int dim)
{
	m_ins.push_back(new Inequality(data,dim));
    feasibilityState = UNKNOWN_FEAS;
}

//добавляет все неравенства из p к текущим
void Polyhedron::AddInequalities(Polyhedron& p)
{
    InequalityList::iterator it;
    for (it = p.m_ins.begin(); it != p.m_ins.end(); ++it)
    {
        AddInequality(*it);
    }
}

void Polyhedron::AddInequality(const Inequality& inq)
{
	m_ins.push_back(new Inequality(inq));
	feasibilityState = UNKNOWN_FEAS;
}

	/// Добавляет неравенство в контекст. После добавления считается, что объект *inq стал принадлежать данному контексту! (Контекст освобождает его память!)
void Polyhedron::AddInequality(Inequality*& inq)
{
	m_ins.push_back(inq);
	feasibilityState = UNKNOWN_FEAS;
	inq=NULL;
}

void Polyhedron::AddOppositeInequality(const int* data,int dim)
{
	Inequality* in=AddNewInequality(dim);
	for(int i=0;i<dim;i++)
		in->m_coefs[i]=-data[i];
	in->m_coefs[0]-=1;

	feasibilityState = UNKNOWN_FEAS;
}

void Polyhedron::AddInverseInequality(const Inequality& inq)
{
	Inequality* in=AddNewInequality(inq.m_dim);
	for(int i=0;i<inq.m_dim;i++)
		in->m_coefs[i]=-inq.m_coefs[i];

	feasibilityState = UNKNOWN_FEAS;
}

void Polyhedron::AddInverseInequality(const int* data,int dim)
{
	Inequality* in=AddNewInequality(dim);
	for(int i=0;i<dim;i++)
		in->m_coefs[i]=-data[i];

	feasibilityState = UNKNOWN_FEAS;
}

int Polyhedron::SatisfiedWith(const int* data,int dim,NewParamVector* paramList)
{
	InequalityList::iterator first=m_ins.begin(),last=m_ins.end();
	int* newVarArray=NULL;
	if(paramList)
	if(paramList->GetSize())
	{
		//вычисляем значения добавленных параметров, используя описание вектора на входе функции...
		newVarArray=new int[paramList->GetSize()];
		for(int i=0;i<paramList->GetSize();i++)
		{
//			newVarArray[i]=0;
			NewParamEquation* equation=paramList->operator[](i);
			newVarArray[i]=equation->GetFree();
			for(int j=0;j<dim;j++)
				newVarArray[i]+=equation->m_coefs[j]*data[j];

			for(int j=0;j<i;j++)
				newVarArray[i]+=equation->m_coefs[dim+j]*newVarArray[j];
		
			newVarArray[i]=lackDiv(newVarArray[i],equation->m_denom);
		}
	}
//	else newVarArray=NULL;

	while(first!=last)
	{	
		int* inqData=(*first)->m_coefs;
		int s=inqData[0];
		for(int i=1;(i<=dim)&&(i<(*first)->m_dim);i++)
			s+=inqData[i]*data[i-1];

		for(int i=(dim+1);i<(*first)->m_dim;i++)
			s+=inqData[i]*newVarArray[i-dim-1];


		if(s<0)
		{
			if(newVarArray)delete[] newVarArray;
			return 0;
		}
		first++;
	}

	if(newVarArray)delete[] newVarArray;
	return 1;
}

Polyhedron* Polyhedron::substituteParams(const int* data,int dim)
{
	InequalityList::iterator first=m_ins.begin(),last=m_ins.end();

	Polyhedron* resContext=new Polyhedron();
	int i;

	while(first!=last)
	{	
		int* inqData=(*first)->m_coefs;
		int s=inqData[0];
		for(i=1;(i<=dim)&&(i<(*first)->m_dim);i++)
			s+=inqData[i]*data[i-1];

		if(((*first)->m_dim-1)>dim)
		{
			Inequality* addInq=new Inequality((*first)->m_dim-dim);
			for(int j=(dim+1);j<(*first)->m_dim;j++)
				addInq->m_coefs[j-(dim+1)+1]=inqData[j];

			addInq->m_coefs[0]=s;

			resContext->AddInequality(addInq);
		}
		else
		{
			if(s<0)
			{
				//resContext->SetStatus(UNFEASIBLE);
				resContext->feasibilityState = UNFEASIBILITY_PROVEN;
				return resContext;
			}
		}
		first++;
	}
	return resContext;
}


Polyhedron* Polyhedron::substituteParams(std::map<OPS::Reprise::VariableDeclaration*,int>& externalParamValuesMap)
{
    InequalityList::iterator first=m_ins.begin(),last=m_ins.end();

    Polyhedron* resContext=new Polyhedron();
    int i;
    int extParamNum = (int)m_externalParamsVector.size();

    while(first!=last)
    {	
        int* inqData=(*first)->m_coefs;
        int loopCounterNum = (int)(*first)->m_dim-extParamNum-1;
        int s=inqData[0];
        for (i=0;i<extParamNum;i++) s+=inqData[loopCounterNum+i]*externalParamValuesMap[m_externalParamsVector[i]];
        if (loopCounterNum>0)   {
            Inequality* addInq=new Inequality(inqData,loopCounterNum+1);
            addInq->m_coefs[0]=s;
            resContext->AddInequality(addInq);
        }
        else        {
            if (s<0)    {
                resContext->feasibilityState = UNFEASIBILITY_PROVEN;
                return resContext;
            }
        }
        first++;
    }
    return resContext;
}





//!!!!!!!!!!!!!!!!!!!!!!!!!! TODO: можно сделать гораздо быстрее - нужно привести систему неравенств к 
//канонической форме (типа верхнетреугольной в методе Гаусса) - я видел это в книжках про неравенства
//в результате некоторые неравенства обнуляться и мы их вычеркнем
void Polyhedron::Simplify()
{
	if((m_ins.size() == 0) || (IsFeasible() == 0))
		return;//такие мы не упрощаем

	InequalityList::iterator first = m_ins.begin(), last = m_ins.end();

//	bool* redundunts = new bool[m_ins.size()];
//	int k=0;

	while(first != last)
	{
		//Строим контекст, в котором все те же неравенства, что и в данном, кроме first;
		Polyhedron tempCon;
		
		InequalityList::iterator firstTemp = m_ins.begin(), lastTemp = m_ins.end();

		while(firstTemp!=lastTemp)
		{
			if(firstTemp != first)
				tempCon.AddInequality(**firstTemp);

			firstTemp++;
		}

		//Вычитаем из получившегося контекста исходный...
		GenArea tempArea = tempCon;
		tempArea.DifferenceFrom(*this);
		if(tempArea.IsFeasible() == 0)
		{
			//значит, это неравенство избыточное
			delete *first;
			first = m_ins.erase(first);
		}
		else
			first++;
	}

//	delete[] redundunts;
}

int Polyhedron::IsFeasible(int nonnegativeVarNum) const
{
//	if(m_ins.size()<=1)return 1;
	if(feasibilityState == UNFEASIBILITY_PROVEN)
		return 0;

	if(feasibilityState == FEASIBILITY_PROVEN)
		return 1;

	if(m_ins.size()<=1)
	{//На easibilityState == UNFEASIBILITY_PROVEN уже было проверено ранее.
		feasibilityState = FEASIBILITY_PROVEN;
		return 1;
	}

	int maxIneqDim=0,res=0;
	InequalityList::const_iterator first=m_ins.begin(),last=m_ins.end();
	while(first!=last)
	{	
		if((*first)->m_dim>maxIneqDim)maxIneqDim=(*first)->m_dim;
		first++;
	}

/* подсчета уже не делается
if(maxIneqDim==2)
	{//В этом случае, нас полностью устраивает подсчет, происходящий при добавлении в контекст очередного неравенства
		feasibilityState = FEASIBILITY_PROVEN;
		return 1;
	}
    */

//	std::cout<<std::endl;
//	Print(std::cout,(const char**)NULL,0,1);
	//переменные, на которые отсутствуют ограничения по знаку, будем заменять на разность двух неотрицательных переменных (x=x1-x2)
	//(это будет реализовано так: просто добавим переменные, соответствующие -x2
	int varNum=maxIneqDim-1;//кол-во переменных в контексте
	int addedVarNum=maxIneqDim-1-nonnegativeVarNum;//кол-во переменных, которые нужно добавить (-x2)
	int freeCoeffIndex=varNum+addedVarNum;
	TreeNode* newRoot=new TreeNode();
	Tableau* tab=newRoot->tableau=new Tableau((int)m_ins.size()+varNum+addedVarNum,maxIneqDim+addedVarNum,varNum+addedVarNum,0);
	newRoot->context=new Polyhedron();
	newRoot->d=1;

	//Заполняем симплекс таблицу...
	for(int i=0;i<(varNum+addedVarNum);i++)
		tab->data[i][i]=1;

	first=m_ins.begin();
//	for(int i=0;i<m_ins.size();i++)
	for(int i=(varNum+addedVarNum);i<tab->dimi;i++)
	{
		int *src=(*first)->m_coefs,fd=(*first)->m_dim-1;
		for(int j=0;j<fd;j++)
			tab->data[i][j]=src[j+1];

		tab->data[i][freeCoeffIndex]=src[0];

		for(int j=0;j<addedVarNum;j++)
			tab->data[i][j+varNum]=-tab->data[i][j+nonnegativeVarNum];
//		memcpy(&(tab->m_coefs[i][0]),&((*first)->m_coefs[1]),(*first)->m_dim-1);
//		memcpy(&(tab->m_coefs[i][0]),src+1,fd-1);
//		tab->m_coefs[i][maxIneqDim-1]=(*first)->m_coefs[0];
//		tab->m_coefs[i][maxIneqDim-1]=src[0];
//		tab->m_coefs[i][0]=src[1];

		first++;
	}
	//конец заполнения

	tab->Simplify();
	CalcTableauSigns(tab,newRoot->context);

	std::list<TreeNode*> solTabs;
	NewParamVector* newParamList=new NewParamVector();

#ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
	file<<"\n !!! FEASIBILITY SOLVING PROCESS START !!!\n\n";
#endif


	ApplySimplexForFeasibilityProblem(newRoot,solTabs,newParamList);


#ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
	file<<"\n !!! FEASIBILITY SOLVING PROCESS FINISH!!!\n\n";
#endif

	delete newParamList;
	delete newRoot;

	if(solTabs.size())
	{
		res=1;
		std::list<TreeNode*>::iterator stFirst=solTabs.begin(),stLast=solTabs.end();
		while(stFirst!=stLast)
		{
			delete *stFirst;
			stFirst++;
		}
		solTabs.clear();

	}

	feasibilityState = (res==1 ? FEASIBILITY_PROVEN : UNFEASIBILITY_PROVEN);

	return res;
}

int Polyhedron::IsFeasibleEx(int veryLargeParamCol)
{
	InequalityList::iterator first=m_ins.begin(),last=m_ins.end();
	while(first!=last)
	{
		Inequality* inq=*first;
		if(veryLargeParamCol<inq->m_dim)
			if(inq->m_coefs[veryLargeParamCol]<0)
			{
				feasibilityState = UNFEASIBILITY_PROVEN;
				return 0;
			}
		first++;
	}

	feasibilityState = FEASIBILITY_PROVEN;
	return 1;
}

void Polyhedron::NormalizeBackInq()
{
	if(m_ins.size()==0)return;
	InequalityList::iterator last=m_ins.end();
	last--;
	Inequality* inq=*last;	
	if(inq->m_dim<2)return;
	int nod=abs(inq->m_coefs[1]);
	for(int i=2;i<inq->m_dim;i++)
	{
		nod=calculateGCD(nod,abs(inq->m_coefs[i]));
		if(nod==1)return;
	}

	if(nod)
	{
		// Normalizing...
		for(int i=1;i<inq->m_dim;i++)
			inq->m_coefs[i]/=nod;

		int fl=0;
		if(inq->m_coefs[0]<0)
			if(inq->m_coefs[0]%nod)fl=1;
			
		inq->m_coefs[0]/=nod;
		if(fl)inq->m_coefs[0]-=1;//Взяли с недостатком			

//		inq->data[0]=lackDiv(inq->data[0],nod);
	}
}

void Polyhedron::Print(std::ostream& os,const char** paramNames,int paramNamesDim,int mode)
{
	InequalityList::iterator first=m_ins.begin(),last=m_ins.end();
	while(first!=last)
	{	
		(*first)->Print(os,paramNames,paramNamesDim,mode);
		os<<std::endl;
		first++;
	}
}

void Polyhedron::Print(std::ostream& os,const std::string* paramNames,int paramNamesDim,int mode)
{
	InequalityList::iterator first=m_ins.begin(),last=m_ins.end();
	while(first!=last)
	{	
		(*first)->Print(os,paramNames,paramNamesDim,mode);
		os<<std::endl;
		first++;
	}
}

std::string Polyhedron::toString()
{
    std::string result;
    InequalityList::iterator first=m_ins.begin(), last=m_ins.end();
    while (first != last)
    {	
        result += (*first)->toString() + " >= 0\n";
        first++;
    }
    return result;
}


void Polyhedron::IntersectWith(const Polyhedron& con)
{
	//if(con.GetStatus(UNFEASIBLE))
	if(con.IsFeasible() == 0)
	{
		//SetStatus(UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
		return;
	}

	InequalityList::const_iterator first=con.m_ins.begin(),last=con.m_ins.end();
	//while((first!=last)&&(!GetStatus(UNFEASIBLE)))
	while((first!=last)&&(feasibilityState != UNFEASIBILITY_PROVEN))
	{	
		AddInequality(**first);
		first++;
	}
	IsFeasible();
}

Polyhedron& Polyhedron::operator=(const Polyhedron& con)
{
	if((&con)==this)return *this;
	Clear();
	InequalityList::const_iterator first=con.m_ins.begin(),last=con.m_ins.end();
	for(; first != last; ++first) {
		m_ins.push_back(new Inequality(**first));
	}
	feasibilityState = con.GetFeasibilityState();
    m_externalParamsVector=con.m_externalParamsVector;
	return *this;
}

void Polyhedron::Transform(int** matrix,int dimi,int dimj)
{
	/*
	InequalityList::iterator first=m_ins.begin(),last=m_ins.end();
	for(; first != last; ++first) {
		(*first)->transform(matrix,dimi,dimj);
	}
	feasibilityState = UNKNOWN_FEAS;
	*/

	if(IsFeasible() == 0)
		return;//контекст недопустим, с такими не работаем.

	InequalityList tempList;
	InequalityList::iterator first=m_ins.begin(),last=m_ins.end();
	//Просто копируем в темп
	while(first != last)
	{
		tempList.push_back(*first);
		first++;
	}
	
	//очищаем список без удаления неравенств
	m_ins.clear();

	//теперь трансформируем неравенства и подаем их контексту...
	first=tempList.begin(); last=tempList.end();
	while((first != last)&&(feasibilityState != UNFEASIBILITY_PROVEN))
	{
		(*first)->transform(matrix,dimi,dimj);
		this->AddInequality(*first);
		first++;
	}

	if(feasibilityState != UNFEASIBILITY_PROVEN)
		feasibilityState = UNKNOWN_FEAS;

}

GenArea::GenArea(const GenArea& ge)
	:Status(ge)
{

	//if(ge.GetStatus(GE_UNFEASIBLE))
	//if(ge.IsFeasible() == 0)
	if(ge.GetFeasibilityState() == UNFEASIBILITY_PROVEN)
	{
		//SetStatus(GE_UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
		return;
	}

	ContextList::const_iterator first=ge.data.begin(),last=ge.data.end();
	for(; first != last; ++first) {
		data.push_back(new Polyhedron(**first));
	}

	//SaveStatus(ge.GetStatusData());
	//feasibilityState = FEASIBILITY_PROVEN;
	feasibilityState = ge.GetFeasibilityState();
}

GenArea::GenArea(const Polyhedron& con)
{
	//if(con.GetStatus(UNFEASIBLE))
	//if(con.IsFeasible() == 0)
	if(con.GetFeasibilityState() == UNFEASIBILITY_PROVEN)
	{
		//SetStatus(GE_UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
		return;
	}

	data.push_back(new Polyhedron(con));
	//feasibilityState = FEASIBILITY_PROVEN;
	feasibilityState = con.GetFeasibilityState();
}

//по сложному условию конструируется объединение многогранников
GenArea::GenArea(ComplexCondition& cond)
{
    //алгоритм: находим ДНФ ComplexCondition
    //каждой коньюнкции соответствует Context из GenArea - вот и готово
	ComplexCondition::TDNF dnf = cond.buidDNF();
    for (int i=0;i<(int)dnf.size();i++)  {
        Polyhedron con((std::vector<Inequality> &) dnf[i]);
        UnionWith(con);
    }
}

GenArea& GenArea::operator=(const GenArea& ge)
{
	Clear();

	//if(ge.GetStatus(GE_UNFEASIBLE))
	//if(ge.IsFeasible() == 0)
	if(ge.GetFeasibilityState() == UNFEASIBILITY_PROVEN)
	{
		//SetStatus(GE_UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
		return *this;
	}

	ContextList::const_iterator first = ge.data.begin(), last = ge.data.end();
	for(; first != last; ++first) {
		data.push_back(new Polyhedron(**first));
	}

	//SaveStatus(ge.GetStatusData());
	//feasibilityState = FEASIBILITY_PROVEN;
	feasibilityState = ge.GetFeasibilityState();
	return *this;
}

void GenArea::Clear()
{
	ContextList::iterator first=data.begin(),last=data.end();
	while(first!=last)
	{	
		delete *first;
		first++;
	}
	data.clear();

	//SaveStatus(0);
	feasibilityState = FEASIBILITY_PROVEN;
}

void GenArea::Simplify()
{
	ContextList::iterator first=data.begin(),last=data.end();
	while(first!=last)
	{			
		(*first)->Simplify();
		first++;
	}
}

void GenArea::IntersectWith(const Polyhedron& con)
{
	//if(GetStatus(GE_UNFEASIBLE))
	if(this->IsFeasible() == 0)
	{
		return;
	}

	//if(con.GetStatus(UNFEASIBLE))
	if(con.IsFeasible() == 0)
	{
		//SaveStatus(GE_UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
		return;
	}

	//далее известно, что подаваемый на вход контекст допустим.
	if(data.size()==0)
	{
		if(con.GetSize()!=0)
		{
			data.push_back(new Polyhedron(con));
		}
		feasibilityState = FEASIBILITY_PROVEN;
		return;
	}

	ContextList::iterator first=data.begin(),last=data.end();
	while(first!=last)
	{	
		(*first)->IntersectWith(con);
		if(!(*first)->IsFeasible())
		{
			delete *first;
			first=data.erase(first);
		}
		else first++;
	}

	if(data.size()==0)
	{
		//SetStatus(GE_UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
	}
	else
		feasibilityState = FEASIBILITY_PROVEN;
}

void GenArea::IntersectWith(const GenArea& ge)
{
	//if(ge.GetStatus(GE_UNFEASIBLE))
	if(ge.IsFeasible() == 0)
	{
		//SetStatus(GE_UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
		return;
	}
	
	//if(GetStatus(GE_UNFEASIBLE))
	if(IsFeasible() == 0)
		return;

	if(data.size()==0)
	{//если в данном GenArea ничего нет (т.е. это все Rn), то просто присваиваем ему входной 'ge'
		(*this)=ge;
		feasibilityState = FEASIBILITY_PROVEN;//хотя в данном случае, это излишне
		return;
	}
    if (ge.GetSize()==0) //ge - это все пространство
        return;

	GenArea result;

	ContextList::const_iterator first=ge.data.begin(),last=ge.data.end();
	
	while(first!=last)
	{
		ContextList::iterator firstThis=Begin(),lastThis=End();
		while(firstThis!=lastThis)
		{
			Polyhedron con(**firstThis);
			con.IntersectWith(**first);
			result.UnionWith(con);//Допустимость контекста con проверяется внутри, причем недопустимый - не добавляется.
			firstThis++;
		}
		first++;
	}

	(*this)=result;

	if(data.size()==0)
	{//пересечение - пустое множество
		//SetStatus(GE_UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
	}
	else
		feasibilityState = FEASIBILITY_PROVEN;

	return;
}

void GenArea::DifferenceFrom(Polyhedron& con)
{
	//if(con.GetStatus(UNFEASIBLE))
	if(con.IsFeasible() == 0)
		return;

	//if(GetStatus(GE_UNFEASIBLE))
	if(IsFeasible() == 0)
		return;

	if(con.GetSize()==0)
	{
		//SetStatus(GE_UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
		return;
	}

	// checking for intersection...
	GenArea ge(*this);
	ge.IntersectWith(con);
	if(!ge.IsFeasible())
		return;
	// done

	ContextList::iterator first=data.begin(),last=data.end(),realLast,firstOfAdded,cur;

	if (first!=last) last--;

	for(int i=0;i<(int)con.m_ins.size();i++)  data.push_back(new Polyhedron());

	if (GetSize()==con.GetSize()) last=first=data.begin();
    else last++;

	realLast=data.end();
	cur=last;
	// Ищем Rn/con ...

	Polyhedron::InequalityList::const_iterator firstInq=con.m_ins.begin(),lastInq=con.m_ins.end();
	while(firstInq!=lastInq)
	{	
		firstOfAdded=cur;
		while(firstOfAdded!=realLast)
		{
			if(firstOfAdded==cur)
				(*firstOfAdded)->AddOppositeInequality(**firstInq);
			else
				(*firstOfAdded)->AddInequality(**firstInq);

			firstOfAdded++;
		}
		cur++;
		if(cur==realLast)break;
		firstInq++;
	}

	// нашли Rn/con

	if(GetSize()==con.GetSize())
	{//Выполнение этого условия означает, что данная область (this) до вызова этой функции равнялась всему Rn. Т.о. мы уже нашли, что искали
		feasibilityState = FEASIBILITY_PROVEN;//
		return;
	}

/*
	// Debugging message...
	ofstream file("zConts.txt");
	cur=last;
	while(cur!=realLast)
	{
		(*cur)->Print(file,NULL,1);
		cur++;
		file<<std::endl<<std::endl;
	}
	file.close();


	// end
*/
/*
	// Среди добавленных ищем недопустимые и удаляем...
 
	cur=last;
	while(cur!=realLast)
	{
		if((*cur)->IsFeasible()==0)
		{
			delete *cur;
			cur=data.erase(cur);
		}
		else cur++;
	}

	// ok
*/

	// Собственно пересечение
	int decFlag=1;
	while(first!=last)
	{	
		Polyhedron temp=**first,prev=temp;
/*
		// Debugging message
		cout<<"\n Context\n";
		temp.Print(cout,NULL,1);
		cout<<"\n Trying to intersect with:\n";
		// end
*/
		cur=last;
		while(cur!=realLast)
		{
/*			Context* z=*cur;
		
			// Debugging message
			z->Print(cout,NULL,1);
			cout<<std::endl;
			cout.flush();
			// end
*/
			temp.IntersectWith(**cur);
			if(temp.IsFeasible())
			{
				data.push_back(new Polyhedron(temp));
				if(decFlag)
				{
					realLast--;
					if(cur==realLast)break;
					decFlag=0;
				}
			}
			temp=prev;
			cur++;
		}
		delete *first;
		first=data.erase(first);
	}

//	if(GetSize()!=con.GetSize())
//	{
		cur=last;
		while(cur!=realLast)
		{
			delete *cur;
			cur=data.erase(cur);
		}
//	}

	if(data.size()==0)
	{
		//SetStatus(GE_UNFEASIBLE);
		feasibilityState = UNFEASIBILITY_PROVEN;
	}
	else
		feasibilityState = FEASIBILITY_PROVEN;
}

void GenArea::DifferenceFrom(GenArea& ge)
{
	ContextList::const_iterator first=ge.data.begin(),last=ge.data.end();
//	while((first!=last)&&(!GetStatus(GE_UNFEASIBLE)))
	while((first!=last)&&(feasibilityState != UNFEASIBILITY_PROVEN))
	{	
		DifferenceFrom(**first);
		first++;
	}
}

void GenArea::UnionWith(Polyhedron& con)
{
	//if(con.GetStatus(UNFEASIBLE))
	if(con.IsFeasible() == 0)
	{
		return;
	}
	//ClearStatus(GE_UNFEASIBLE);
	data.push_back(new Polyhedron(con));
	feasibilityState = FEASIBILITY_PROVEN;//т.к. объединили с непустым контекстом
}

void GenArea::UnionWith(GenArea& ge)
{
	//if(ge.GetStatus(GE_UNFEASIBLE))
	if(ge.IsFeasible() == 0)
	{
		return;
	}
	//ClearStatus(GE_UNFEASIBLE);
	ContextList::const_iterator first = ge.data.begin(), last = ge.data.end();
	for(; first != last; ++first) {
		data.push_back(new Polyhedron(**first));
	}
	feasibilityState = FEASIBILITY_PROVEN;//т.к. объединили с непустой областью
}

void GenArea::Print(std::ostream& os,const char** paramNames,int paramNamesDim,int mode)
{
	ContextList::iterator first = data.begin(), last = data.end();
	for(; first != last; ++first) {
		(*first)->Print(os, paramNames, paramNamesDim, mode);
		os << "\n\n";
	}
}

void GenArea::Print(std::ostream& os,std::string* paramNames,int paramNamesDim,int mode)
{
	ContextList::iterator first = data.begin(), last = data.end();
	for(; first != last; ++first) {
		(*first)->Print(os, paramNames, paramNamesDim, mode);
		os << std::endl << std::endl;
	}
}

bool GenArea::SatisfiedWith(const int* _data,int _dim,NewParamVector* paramList)
{
	ContextList::iterator first = data.begin(), last = data.end();
	for(; first != last; ++first) {
		if ((*first)->SatisfiedWith(_data,_dim,paramList))
			return true;
	}
	return false;
}
// Возвращает не 0 в случае, если область содержит хотя бы одну целочисленную точку. Иначе возвращает 0;
bool GenArea::IsFeasible() const
{
	if(feasibilityState == UNFEASIBILITY_PROVEN)
		return false;

	if(feasibilityState == FEASIBILITY_PROVEN)
		return true;

	//return !GetStatus(GE_UNFEASIBLE);

	//Нужно доказывать допустимость...
	ContextList::const_iterator first=data.begin(), last=data.end();
	while(first!=last)
	{
		if((*first)->IsFeasible() != 0)
		{//Нашли допустимый контекст, значит и вся область допустима. Устанавливаем флаги и выходим.
			feasibilityState = FEASIBILITY_PROVEN;	
			return true;
		}
	}

	//Если дошли до сюда, значит область недопустима.
	feasibilityState = UNFEASIBILITY_PROVEN;
	return false;
}


void GenArea::Transform(int** matrix,int dimi,int dimj)
{
	ContextList::iterator first = data.begin(), last = data.end();

	while(first!=last)
	{
		(*first)->Transform(matrix,dimi,dimj);
		first++;
	}
	feasibilityState = UNKNOWN_FEAS;
}

int calculateGCD(int a,int b)
{
	if(!a)return b;
	if(!b)return a;
	int v;
	while(0 != (v=a%b,a=b,b=v));
	return a;
}
											//==================Исправить!=======================!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
OPS::Reprise::ExpressionBase* GetExprNode(const SimpleLinearExpression& le,OPS::Reprise::Declarations& ns,const std::string* varNames)
{
	using namespace OPS::Reprise;
	if (le.m_dim == 0)
		return 0;

	if (le.m_dim == 1)
		return OPS::Reprise::BasicLiteralExpression::createInteger(le.m_coefs[0]);


	//Проверка, является ли линейное выражение тождественным нулем

	bool isZero = true;
	for(int i = 0; i < le.m_dim && !isZero; ++i)
		if (le.m_coefs[i])
			isZero = false;

	//проверили.

	if (isZero)
		return OPS::Reprise::BasicLiteralExpression::createInteger(0);


	// Далее считаем, что у нас выражение - не тождественный 0...


//	ExprData* tempData=ExprData::create(ns,varNames[0]);

	OPS::Reprise::ExpressionBase* temp, *res;

	if (le.m_coefs[0])
		res = OPS::Reprise::BasicLiteralExpression::createInteger(le.m_coefs[0]);
	else
		res = 0;


	if (le.m_coefs[1] == 0) {
		temp = NULL;
	}
	else if(le.m_coefs[1] > 0)
	{
		if(le.m_coefs[1] == 1) {
			if (res)
			{
				//res = ExprBinary::create(ExprOper::OT_PLUS, ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[0])), ExprData::create(ns,varNames[0]));
				BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
				p->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[0]));
				ReferenceExpression* pVar = findByName(ns, varNames[0]);
				if (pVar)
				{
					p->addArgument(pVar);
				}
				else
                                {
                                    assert(!"The following line will lead to crash. Have to be fixed.");
                                    // p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
                                }
				res = p;
			}
			else
			{
				//res = ExprData::create(ns, varNames[0]);
				ReferenceExpression* pVar = findByName(ns, varNames[0]);
				if (pVar)
				{
					res = pVar;
				}
				else
                                {
                                    assert(!"The following line will lead to crash. Have to be fixed.");
                                    //res = new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown"));
                                }
			}
		}
		else
			if(res) {
				//temp = ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[1])),ExprData::create(ns,varNames[0]));
				BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
				p->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[1]));
				ReferenceExpression* pVar = findByName(ns, varNames[0]);
				if (pVar)
				{
					p->addArgument(pVar);
				}
				else
				{
				    assert(!"The following line will lead to crash. Have to be fixed.");
				    //p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
				}
				temp = p;

				//res = ExprBinary::create(ExprOper::OT_PLUS,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[0])),temp);
				BasicCallExpression* pp = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
				pp->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[0]));
				pp->addArgument(temp);
				res = pp;
			}
			else
			{
				//res = ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[1])),ExprData::create(ns,varNames[0]));
				BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
				p->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[1]));
				ReferenceExpression* pVar = findByName(ns, varNames[0]);
				if (pVar)
				{
					p->addArgument(pVar);
				}
				else
				{
				    assert(!"The following line will lead to crash. Have to be fixed.");
				    //p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
				}
				
				res = p;
			}
	}
	else if(le.m_coefs[1] == -1)
	{
		if(res)
		{
			//res=ExprBinary::create(ExprOper::OT_MINUS,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[0])),ExprData::create(ns,varNames[0]));
			BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
			p->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[0]));
			ReferenceExpression* pVar = findByName(ns, varNames[0]);
			if (pVar)
			{
				p->addArgument(pVar);
			}
			else
			{
			    assert(!"The following line will lead to crash. Have to be fixed.");
			    //p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
			}
			res = p;
		}
		else
		{
			//res=ExprUnary::create(ExprOper::OT_UNMINUS,ExprData::create(ns,varNames[0]));
			BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_UNARY_MINUS);
			ReferenceExpression* pVar = findByName(ns, varNames[0]);
			if (pVar)
			{
				p->addArgument(pVar);
			}
			else
			{
			    assert(!"The following line will lead to crash. Have to be fixed.");
			    //p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
			}
			res = p;
		}

//				std::cout<<"\nUNARY STATE:\n";
//				std::cout<<temp->dumpState();
//				std::cout.flush();

	}
	else
	{
		//temp=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[1])),ExprData::create(ns,varNames[0]));
		BasicCallExpression* pTemp = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
		pTemp->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[1]));
		ReferenceExpression* pVar = findByName(ns, varNames[0]);
		if (pVar)
		{
			pTemp->addArgument(pVar);
		}
		else
		{
		    assert(!"The following line will lead to crash. Have to be fixed.");
		    //pTemp->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
		}
		temp = pTemp;

		if(res)
		{
			//temp=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,-le.m_coefs[1])),ExprData::create(ns,varNames[0]));
			BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
			p->addArgument(BasicLiteralExpression::createInteger(-le.m_coefs[1]));
			ReferenceExpression* pVar = findByName(ns, varNames[0]);
			if (pVar)
			{
				p->addArgument(pVar);
			}
			else
			{
			    assert(!"The following line will lead to crash. Have to be fixed.");
			    //p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
			}
			temp = p;

			//res=ExprBinary::create(ExprOper::OT_MINUS,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[0])),temp);
			BasicCallExpression* pp = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
			pp->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[0]));
			pp->addArgument(temp);
			res = pp;
		}
		else
		{
			//res=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[1])),ExprData::create(ns,varNames[0]));
			BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
			p->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[1]));
			ReferenceExpression* pVar = findByName(ns, varNames[0]);
			if (pVar)
			{
				p->addArgument(pVar);
			}
			else
			{
			    assert(!"The following line will lead to crash. Have to be fixed.");
			    //p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
			}
			res = p;
		}		
	}

/*
	if(le.m_coefs[1]==0)
		temp=NULL;
	else
		if(le.m_coefs[1]==1)
			temp=ExprData::create(ns,varNames[0]);
		else
			if(le.m_coefs[1]==-1)
			{
				temp=ExprUnary::create(ExprOper::OT_UNMINUS,ExprData::create(ns,varNames[0]));
//				std::cout<<"\nUNARY STATE:\n";
//				std::cout<<temp->dumpState();
//				std::cout.flush();

			}
			else
				temp=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[1])),ExprData::create(ns,varNames[0]));
*/
//	temp=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[1])),ExprData::create(ns,varNames[0]));
//	ExprBinary* res=ExprBinary::create(ExprOper::OT_PLUS,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[0])),temp);

/*
	if(le.m_coefs[0])
		if(temp)
			res=ExprBinary::create(ExprOper::OT_PLUS,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[0])),temp);
		else
			res=ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[0]));
	else
		if(temp)
			res=temp;
		else
			res=NULL;
*/
/*
	for(i=2;i<(le.m_dim-1);i++)
	{
		if(le.m_coefs[i]==0)continue;

//		ExprBinary* temp=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[i])),ExprData::create(ns,varNames[i-1]));
//		res=ExprBinary::create(ExprOper::OT_PLUS,res,temp);

		if(le.m_coefs[i]==1)
			temp=ExprData::create(ns,varNames[i-1]);
		else
			if(le.m_coefs[i]==-1)
				temp=ExprUnary::create(ExprOper::OT_UNMINUS,ExprData::create(ns,varNames[i-1]));
			else
				temp=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[i])),ExprData::create(ns,varNames[i-1]));

		if(res)
			res=ExprBinary::create(ExprOper::OT_PLUS,res,temp);
		else
			res=temp;
	}
*/

	for(int i=2;i<le.m_dim;i++)
	{
		if(le.m_coefs[i]==0)continue;

		if(le.m_coefs[i]>0)
		{
			if(le.m_coefs[i]==1)
			{
				if(res)
				{
					//res=ExprBinary::create(ExprOper::OT_PLUS,res,ExprData::create(ns,varNames[i-1]));
					BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
					p->addArgument(res);
					ReferenceExpression* pVar = findByName(ns, varNames[i-1]);
					if (pVar)
					{
						p->addArgument(pVar);
					}
					else
					{
					    assert(!"The following line will lead to crash. Have to be fixed.");
					    //p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
					}
					res = p;
				}
				else
				{
					//res=ExprData::create(ns,varNames[i-1]);
					ReferenceExpression* pVar = findByName(ns, varNames[i-1]);
					if (pVar)
					{
						res = pVar;
					}
					else
					{
						assert(!"The following line will lead to crash. Have to be fixed.");
						//res = new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown"));
					}
				}
			}
			else
				if(res)
				{
					//temp=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[i])),ExprData::create(ns,varNames[i-1]));
					BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
					p->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[i]));
					ReferenceExpression* pVar = findByName(ns, varNames[i-1]);
					if (pVar)
					{
						p->addArgument(pVar);
					}
					else
					{
						assert(!"The following line will lead to crash. Have to be fixed.");
						//p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
					}
					temp = p;

					//res=ExprBinary::create(ExprOper::OT_PLUS,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[i])),temp);
					BasicCallExpression* pp = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
					pp->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[i]));
					pp->addArgument(temp);
					res = pp;
					
				}
				else
				{
					//res=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[i])),ExprData::create(ns,varNames[i-1]));
					BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
					p->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[i]));
					ReferenceExpression* pVar = findByName(ns, varNames[i-1]);
					if (pVar)
					{
						p->addArgument(pVar);
					}
					else
					{
						assert(!"The following line will lead to crash. Have to be fixed.");
						//p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
					}
					res = p;
				}
		}
		else
			if(le.m_coefs[i]==-1)
			{
				if(res)
				{
					//res=ExprBinary::create(ExprOper::OT_MINUS,res,ExprData::create(ns,varNames[i-1]));
					BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
					p->addArgument(res);
					ReferenceExpression* pVar = findByName(ns, varNames[i-1]);
					if (pVar)
					{
						p->addArgument(pVar);
					}
					else
					{
						assert(!"The following line will lead to crash. Have to be fixed.");
						//p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
					}
					res = p;
				}
				else
				{
					//res=ExprUnary::create(ExprOper::OT_UNMINUS,ExprData::create(ns,varNames[i-1]));
					BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_UNARY_MINUS);
					ReferenceExpression* pVar = findByName(ns, varNames[i-1]);
					if (pVar)
					{
						p->addArgument(pVar);
					}
					else
					{
						assert(!"The following line will lead to crash. Have to be fixed.");
						//p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
					}
					res = p;
				}

//				std::cout<<"\nUNARY STATE:\n";
//				std::cout<<temp->dumpState();
//				std::cout.flush();

			}
			else
			{
				//temp=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[i])),ExprData::create(ns,varNames[i-1]));
				BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
				p->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[i]));
				ReferenceExpression* pVar = findByName(ns, varNames[i-1]);
				if (pVar)
				{
					p->addArgument(pVar);
				}
				else
				{
					assert(!"The following line will lead to crash. Have to be fixed.");
					//p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
				}
				temp = p;

				if(res)
				{
					//temp=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,-le.m_coefs[i])),ExprData::create(ns,varNames[i-1]));
					BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
					p->addArgument(BasicLiteralExpression::createInteger(-le.m_coefs[i]));
					ReferenceExpression* pVar = findByName(ns, varNames[i-1]);
					if (pVar)
					{
						p->addArgument(pVar);
					}
					else
					{
						assert(!"The following line will lead to crash. Have to be fixed.");
						//p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
					}
					temp = p;

					//res=ExprBinary::create(ExprOper::OT_MINUS,res,temp);
					BasicCallExpression* pp = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
					pp->addArgument(res);
					pp->addArgument(temp);
					res = pp;
				}
				else
				{
					//res=ExprBinary::create(ExprOper::OT_MUL,ExprImm::create(ImmValue(ImmValue::IV_INT,le.m_coefs[i])),ExprData::create(ns,varNames[i-1]));
					BasicCallExpression* p = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
					p->addArgument(BasicLiteralExpression::createInteger(le.m_coefs[i]));
					ReferenceExpression* pVar = findByName(ns, varNames[i-1]);
					if (pVar)
					{
						p->addArgument(pVar);
					}
					else
					{
						assert(!"The following line will lead to crash. Have to be fixed.");
						//p->addArgument(new ReferenceExpression(VariableDeclaration(BasicType::voidType(),"unknown")));
					}
					res = p;
				}
			}
	}


	return res;
}

void Inequality::SwapItems(int i, int j)
{
	if ((i >= m_dim) || (j >= m_dim))
	{
		// Нужно увеличивать размер массива inq.m_coefs
		int newDim = i < j ? (j+1) : (i+1);
		int* newData=new int[newDim];
		memset(newData, 0, newDim*sizeof(int));
		memcpy(newData, m_coefs, m_dim*sizeof(int));

		delete[] m_coefs;
		m_coefs = newData;
		m_dim = newDim;
	}

	//просто меняем местами слагаемые...
	int x = m_coefs[i];
	m_coefs[i] = m_coefs[j];
	m_coefs[j] = x;
	return;
	
}

//меняет данное неравенство на противоположное
void Inequality::makeOposite()
{
    multiply(-1);
    m_coefs[0]-=1;
}

void Polyhedron::SwapItems(int i, int j)
{
	InequalityList::iterator first = m_ins.begin(), last = m_ins.end();
	for(; first != last; ++first) {
		(*first)->SwapItems(i, j);
	}
}

static OPS::Reprise::ReferenceExpression* findByName(OPS::Reprise::Declarations& ns, const std::string& name)
{
	for (OPS::Reprise::Declarations::VarIterator iter = ns.getFirstVar(); iter.isValid(); ++iter)
	{
		if (iter->getName() == name)
		{
			return new OPS::Reprise::ReferenceExpression(*iter);
		}
	}
	return 0;
}

void getExternalParamAndLoopCounterCoefficients(
		const OPS::Shared::ParametricLinearExpression& expr,
		const OPS::Shared::ParametricLinearExpression::VariablesDeclarationsVector &loopCounters,
		OPS::LatticeGraph::SimpleLinearExpression& loopCounterCoefficients,
		OPS::Shared::CanonicalLinearExpression& externalParamCoefficients)
{
	externalParamCoefficients = expr.getExternalParamCoefficients(loopCounters);

	loopCounterCoefficients.allocateMem(loopCounters.size()+1);
	loopCounterCoefficients.MakeZero();
	for (size_t i = 0; i < loopCounters.size(); ++i)
	{
		loopCounterCoefficients[i+1] = (int) expr.getCoefficientAsInteger(loopCounters[i]);
	}
	loopCounterCoefficients[0] = (int) expr.getFreeCoefficientAsInteger();
}


}
}
