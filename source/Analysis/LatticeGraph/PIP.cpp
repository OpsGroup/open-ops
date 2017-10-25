#include"Analysis/LatticeGraph/PIP.h"
#include"Analysis/LatticeGraph/ExtendedQuast.h"
#include<memory>
#include<iostream>
#include<fstream>

#include "OPS_Core/Localization.h"
#include "OPS_Core/msc_leakcheck.h"

//#define LATTICE_TABLES_DEBUG

#ifdef LATTICE_TABLES_DEBUG
extern std::ofstream file;
extern int deepOfTheSimplexSearch;
#endif


namespace OPS
{
namespace LatticeGraph
{
NewParamEquation::NewParamEquation(NewParamEquation& newParEq)
{
	if(newParEq.m_coefs)
	{
		m_dim=newParEq.m_dim;
		m_denom=newParEq.m_denom;
		m_coefs=new int[m_dim];
		memcpy(m_coefs,newParEq.m_coefs,m_dim*sizeof(int));
	}
	else
		m_coefs=NULL;
}

void NewParamEquation::operator=(NewParamEquation& newParEq)
{
	if(this!=&(newParEq))
	{
		Clear();
		if(newParEq.m_coefs)
		{
			m_dim=newParEq.m_dim;
			m_denom=newParEq.m_denom;
			m_coefs=new int[m_dim];
			memcpy(m_coefs,newParEq.m_coefs,m_dim*sizeof(int));
		}
		else
			m_coefs=NULL;
	}
}

//делает аффинную замену переменных x=t(y)
void NewParamEquation::transform(int** transformMatrix,int dimi,int dimj)
{
    //чуток медленнее из-за одного дополнительного выделения памяти, зато надежно и код не повторяется
    SimpleLinearExpression* linExpr=getLinExpr();
    linExpr->transform(transformMatrix,dimi,dimj);
	delete[] m_coefs;
    m_coefs=linExpr->m_coefs;
    m_dim=linExpr->m_dim;
    //перемещаем свободный член на последнее место
    int free=m_coefs[0];
    for (int i=0;i<m_dim-1;i++) m_coefs[i]=m_coefs[i+1];
    m_coefs[m_dim-1]=free;
    //удаляем linExpr
    linExpr->m_coefs=NULL;
    linExpr->m_dim=0;
    delete linExpr;
}

//возвращает линейное выражение в числителе (свободный член в SimpleLinearExpression стоит на первом месте!!!!!!!)
SimpleLinearExpression* NewParamEquation::getLinExpr()
{
    SimpleLinearExpression* result=new SimpleLinearExpression(m_dim);
    for (int i=0;i<m_dim-1;i++) 
        result->at(i+1)=m_coefs[i];
    result->at(0)=m_coefs[m_dim-1];
    return result;
}

void NewParamEquation::insertNZerosBeforeNewParamCoefs(int N, int varNum)//varNum - кол-во обычных переменных (не новых параметров)
{
    int* m_coefsNew = new int[m_dim+N];
    for (int i=0;i<varNum;i++) m_coefsNew[i]=m_coefs[i];
    for (int i=0;i<N;i++) m_coefsNew[varNum+i]=0;
    for (int i=0;i<m_dim-varNum;i++) m_coefsNew[i+varNum+N]=m_coefs[i+varNum];
	delete[] m_coefs;
    m_coefs=m_coefsNew;
    m_dim+=N;
}

//возвращает систему из двух неравенств, определяющих новый параметр
Polyhedron NewParamEquation::getInequalities()
{
    Polyhedron result;
    Inequality ineq(m_dim+1);
    for (int i=0;i<2;i++) result.AddInequality(getInequality(i));
    return result;
}


//возвращает number = 0 или 1 - левое и правое неравенства, определяющие новый параметр
Inequality NewParamEquation::getInequality(int number)
{
    Inequality ineq(m_dim+1);
    if (number==0)  {
        for (int i=0;i<m_dim;i++) ineq[i]=m_coefs[i];
        ineq[m_dim]=-m_denom;
        return ineq;
    }
    else  {
        for (int i=0;i<m_dim;i++) ineq[i]=-m_coefs[i];
        ineq[m_dim]=m_denom;
        ineq[0]+=m_denom-1;
        return ineq;
    }
}


void NewParamVector::Clear()
{
	std::vector<NewParamEquation*>::iterator first=paramEqVector.begin(),last=paramEqVector.end();
	while(first!=last)
	{	
		delete *first;
		first++;
	}
	paramEqVector.clear();
}

NewParamVector::NewParamVector(NewParamVector& newParamVector)
{
	std::vector<NewParamEquation*>::iterator first=newParamVector.Begin(),
	last=newParamVector.End();
	while(first!=last)
	{	
		paramEqVector.push_back(new NewParamEquation(**first));
		first++;
	}
}

void NewParamVector::operator=(NewParamVector& newParamVector)
{
	if(this!=&newParamVector)
	{
		Clear();
		std::vector<NewParamEquation*>::iterator first=newParamVector.Begin(),last=newParamVector.End();
		while(first!=last)
		{	
			paramEqVector.push_back(new NewParamEquation(**first));
			first++;
		}
	}
}

void NewParamVector::DeleteBack()
{
	if(paramEqVector.size())
	{
		NewParamEquation* p=Back();
		delete p;
		paramEqVector.pop_back();
	}
}

void NewParamVector::Print(std::ostream& os,const char** paramNames,int paramNamesDim)
{
	std::vector<NewParamEquation*>::iterator first=Begin(),
	last=End();
	int i=1,j;
	//char buf[8];
	while(first!=last)
	{	
		NewParamEquation* np=*first;
		//sprintf(buf,"_%d=[(",i);
		//os<<buf;
		os<<'_'<<i<<"=[(";
		for(j=0;j<(np->m_dim-1);j++)
		{
			if((np->m_coefs[j]>=0)&&(j!=0))os<<"+";
			os<<np->m_coefs[j]<<"*";
			if(paramNames)
			{
				if(j<paramNamesDim)os<<paramNames[j];
				else
				{
					//sprintf(buf,"_%d",j-paramNamesDim+1);
					//os<<buf;
					os<<'_'<<(j-paramNamesDim+1);
				}
			}
		}

		if((np->m_coefs[np->m_dim-1]>=0))os<<"+";
		os<<np->m_coefs[np->m_dim-1];

		os<<")/"<<np->m_denom<<"]\n";
		i++;
		first++;
	}
}

void NewParamVector::Print(std::ostream& os,std::string* paramNames,int paramNamesDim)
{
	std::vector<NewParamEquation*>::iterator first=Begin(),
	last=End();
	int i=1,j;
	//char buf[8];
	while(first!=last)
	{	
		NewParamEquation* np=*first;
		//sprintf(buf,"_%d=[(",i);
		//os<<buf;
		os<<'_'<<i<<"=[(";
		for(j=0;j<(np->m_dim-1);j++)
		{
			if((np->m_coefs[j]>=0)&&(j!=0))os<<"+";
			os<<np->m_coefs[j]<<"*";
			if(paramNames)
			{
				if(j<paramNamesDim)os<<paramNames[j];
				else
				{
					//sprintf(buf,"_%d",j-paramNamesDim+1);
					//os<<buf;
					os<<'_'<<(j-paramNamesDim+1);
				}
			}
		}

		if((np->m_coefs[np->m_dim-1]>=0))os<<"+";
		os<<np->m_coefs[np->m_dim-1];

		os<<")/"<<np->m_denom<<"]\n";
		i++;
		first++;
	}
}

//ровно GetSize() последних paramNames должны быть именами новых вводимых параметров
std::string NewParamVector::toString(std::vector<std::string>* paramNames,int marginLeft)
{
    std::string spaces,result; 
    if (GetSize()>0)  {
        for (int i=0;i<marginLeft;i++) spaces+=" ";//отступ от левого края
        for(int i=0;i<GetSize();i++)  {
            result+=spaces;
            SimpleLinearExpression* numerator=paramEqVector[i]->getLinExpr();
            result+=paramNames->at(i+paramNames->size()-GetSize()) + " = ( "+numerator->toString(*paramNames)+ " ) div "
                + OPS::Strings::format("%d",paramEqVector[i]->m_denom) + "\n";
            delete numerator;
        }
    }
    return result;
}

int* NewParamVector::ComputeParams()
{
	int dim=GetSize();
	if(dim==0)return NULL;

	int* res=new int[dim];

	std::vector<NewParamEquation*>::iterator first=Begin(),
	last=End();
	int i=0;

	//Ищем, кол-во внешних параметров (оно равно кол-ву элементов первого NewParamEq., без свободного члена).
	int extParamNumb=(*first)->m_dim-1;

	//Нашли.

	while(first!=last)
	{
		NewParamEquation* tempPE=*first;

		for(int j=0;j<extParamNumb;j++)
			if(tempPE->m_coefs[j]!=0)
			{//Введенный параметр зависит от внешнего параметра. Нельзя подсчитать результат.
				delete[] res;
				return NULL;
			}

		int s=tempPE->GetFree();
		for(int j=extParamNumb;j<(tempPE->m_dim-1);j++)
		{
			s+=res[j-extParamNumb]*tempPE->m_coefs[j];
		}

		res[i]=lackDiv(s,tempPE->m_denom);

		i++;
		first++;
	}

	return res;
}

//вычисляет значения параметров для заданного вектора известных параметров
std::vector<int>&  NewParamVector::evaluate(std::vector<int>& knownParams)
{
    int dim=GetSize();
	if (dim==0) {std::vector<int>& result = *new std::vector<int>; return result;}

    std::vector<int>& result = *new std::vector<int>(dim);

    std::vector<NewParamEquation*>::iterator first=Begin(),
        last=End();
    int i=0;

    //Ищем, кол-во внешних параметров (оно равно кол-ву элементов первого NewParamEq., без свободного члена).
    size_t extParamNumb=(*first)->m_dim-1;
    OPS_ASSERT(extParamNumb<=knownParams.size());

    while(first!=last)
    {
        NewParamEquation* tempPE=*first;
        int s=tempPE->GetFree();
        for (size_t j=0;j<knownParams.size();j++) s+=tempPE->m_coefs[j]*knownParams[j];
        for (size_t j=extParamNumb;j<(size_t)(tempPE->m_dim-1);j++) s+=tempPE->m_coefs[j]*result[j-extParamNumb];
        result[i]=lackDiv(s,tempPE->m_denom);
        i++;
        first++;
    }
    return result;
}



Tableau::Tableau(int _dimi,int _dimj,int _varNum,int _positiveParamNum):dimi(_dimi),dimj(_dimj),spareDim(0),varNum(_varNum),positiveParamNum(_positiveParamNum)
{
	data=new int*[dimi];
	for(int i=0;i<dimi;i++){data[i]=new int[dimj];memset(data[i],0,dimj*sizeof(int));}
	sign=new TSIGN[dimi];
    for (int i=0;i<dimi;i++) sign[i]=UNKNOWN;
}

Tableau::Tableau(Tableau& tb):spareDim(0)
{
	if(!tb.data){data=NULL;dimi=dimj=0;return;}
	dimj=tb.dimj;dimi=tb.dimi;varNum=tb.varNum;
	data=new int*[dimi];
	for(int i=0;i<dimi;i++){data[i]=new int[dimj];memcpy(data[i],tb.data[i],dimj*sizeof(int));}
	sign=new TSIGN[dimi];memcpy(sign,tb.sign,dimi*sizeof(TSIGN));
    positiveParamNum=tb.positiveParamNum;
}

Tableau& Tableau::operator=(Tableau& tb)
{
	if((&tb)!=this)
	{
		Clear();
		if(!tb.data){/*data=NULL;dimi=dimj=0;*/return *this;}
		dimj=tb.dimj;dimi=tb.dimi;varNum=tb.varNum;
		data=new int*[dimi];
		for(int i=0;i<dimi;i++){data[i]=new int[dimj];memcpy(data[i],tb.data[i],dimj*sizeof(int));}
		sign=new TSIGN[dimi];memcpy(sign,tb.sign,dimi*sizeof(TSIGN));
        positiveParamNum=tb.positiveParamNum;
	}
	return *this;
}

void Tableau::Clear()
{
	if(!data)return;
	dimi+=spareDim;
	dimj+=spareDim;
	for(int i=0;i<dimi;i++)
		delete []data[i];
	delete[] data;data=NULL;
	delete[] sign;sign=NULL;
	dimi=dimj=spareDim=0;

}

void Tableau::AddSpareDims()
{
	int **newData,newDimI=dimi,newDimJ=dimj;
    TSIGN* newSign;
	newData=new int*[dimi+SPARE_DIM];
	for(int i=0;i<(dimi+SPARE_DIM);i++)
	{
		newData[i]=new int[dimj+SPARE_DIM];
		if(i<dimi)
		{
			memcpy(newData[i],data[i],dimj*sizeof(int));
			memset(&(newData[i][dimj]),0,SPARE_DIM*sizeof(int));
		}
		else	//Какая необходимость
			memset(newData[i],0,(dimj+SPARE_DIM)*sizeof(int));
	}
	newSign=new TSIGN[dimi+SPARE_DIM];
	memcpy(newSign,sign,dimi*sizeof(TSIGN));
    for (int i=0;i<SPARE_DIM;i++) newSign[i+dimi]=UNKNOWN;

	Clear();
	
	data=newData;sign=newSign;
	dimi=newDimI;dimj=newDimJ;spareDim=SPARE_DIM;
}

void Tableau::AddNewRow()
{
	int **newData;
    TSIGN* newSign;
	newData=new int*[dimi+1];

	memcpy(newData,data,dimi*sizeof(int*));
	newData[dimi]=new int[dimj+spareDim];
	memset(newData[dimi],0,(dimj+spareDim)*sizeof(int));

	newSign=new TSIGN[dimi+1];
	memcpy(newSign,sign,dimi*sizeof(TSIGN));
	newSign[dimi]=UNKNOWN;

	delete[] data;
	data=newData;
	delete[] sign;
	sign=newSign;
	dimi++;
}

void Tableau::Simplify()
{
	if((!data)||(varNum>dimi))return;
	int* redundant=new int[dimi];
	for(int i=0;i<dimi;i++)redundant[i]=0;
	
	// Finding redundant strings...

	for(int i=0;i<dimi;i++)
		if(!redundant[i])
		for(int i2=i+1;i2<dimi;i2++)
		{
			int fl=1;
			for(int j=0;j<dimj;j++)
				if(data[i2][j]!=data[i][j]){fl=0;break;}// String i2 != i

			if(fl)// String i2==i
			{
//				if(sign[i2]==POSITIVE)// String i2 represents a loop bound
//					sign[i]=POSITIVE;
				redundant[i2]=1;

			}
		}

	// End

	// Eleminating redundants...
	int row=-1;
	for(int i=varNum;i<dimi;i++)if(redundant[i]){row=i;break;}	

	if(row!=-1)
	{
		for(int i=row+1;i<dimi;i++)
			if(!redundant[i])
			{	
				memcpy(data[row],data[i],dimj*sizeof(int));
				row++;
			}

		for(int i=row;i<dimi;i++)
			delete[] data[i];
		dimi=row;
	}

	delete[] redundant;
}

std::ostream& operator<<(std::ostream& os,Tableau& tab)
{
	os.width(4);
	for(int i=0;i<tab.dimi;i++)
	{
		for(int j=0;j<tab.dimj;j++)
		{os.width(4);os<<tab.data[i][j]/*<<"  "*/;}
//		if(tab.sign[i]<0)os<<" "<<"-";else if(tab.sign[i]>0)os<<" "<<"+";else os<<" "<<"?";
		switch(tab.sign[i])
		{
		case NEGATIVE:os.width(4);os<<"..."<<"-";break;
		case POSITIVE:os.width(4);os<<"..."<<"+";break;
		case ZERO:os.width(4);os<<"..."<<"0";break;
		case UNKNOWN:os.width(4);os<<"..."<<"?";
		}
		os<<std::endl;os.width(3);
	}
	os.width(0);
	return os;
}


TreeNode::~TreeNode()
{
	if(tableau)delete tableau;
    tableau=NULL;
	if(context)delete context;
    context=NULL;
	if(left)delete left;
    left=NULL;
	if(right)delete right;
    right=NULL;
    if (newParamVector) delete newParamVector;
    newParamVector=NULL;
}

int FindOpposite(Tableau* tab,int toString,int begCol)
{
	for(int i=0;i<tab->dimi;i++)
	{
		if(i==toString)continue;
		int fl=1;
		for(int j=begCol;j<tab->dimj;j++)
		{
			if(tab->data[i][j]!=-tab->data[toString][j]){fl=0;break;}
		}
		if(fl)return i;
	}
	return -1;
}

int  FindNegative(Tableau* tab)
{
	for(int i=0;i<tab->dimi;i++)
		if(tab->sign[i]==NEGATIVE)return i;
	return -1; // NO negative elements found
}


int  FindUnknown(Tableau* tab)
{
	for(int i=0;i<tab->dimi;i++)
		if(tab->sign[i]==UNKNOWN)return i;
	return -1; // NO unknown elements found
}

/*
int  FindUnknown(Tableau* tab)
{
	for(int i=(tab->dimi-1);i>=0;i--)
		if(tab->sign[i]==UNKNOWN)return i;
	return -1; // NO unknown elements found
}
*/

int  LexComp(Tableau* tab,int i1,int d1,int i2,int d2)
{
	for(int i=0;i<tab->dimi;i++)
	{
		int a=tab->data[i][i1]*d2,b=tab->data[i][i2]*d1;
		if(a<b)return -1;
		else if(a>b)return 1;
	}
	return 0;
}

int FindPivot(Tableau* tab,int psNum)
{
	int pcNum=-1;
	for(int i=0;i<tab->varNum;i++)
	{
		if(tab->data[psNum][i]>0)
		{
			if(pcNum==-1)pcNum=i;
			else if(LexComp(tab,pcNum,tab->data[psNum][pcNum],i,tab->data[psNum][i])>0)pcNum=i;
		}
	}

	return pcNum;
}

TSIGN CalcOneTableauSign(int i, Tableau* tab, Polyhedron* con)
{
    int varNum=tab->varNum, positiveParamNum=tab->positiveParamNum;

    //делаем простую проверку
    bool allPositive=true; bool allNegative=true;
    //коэффициенты при положительных параметрах
    for (int j=-1;j<positiveParamNum;j++)  
    {
        if (tab->data[i][j+varNum+1]<0) {allPositive=false;}
        if (tab->data[i][j+varNum+1]>0) {allNegative=false;}
    }
    //теперь на равенство нулю коэффициентов при параметрах произвольного знака
    for (int j=0;j<tab->dimj-varNum-positiveParamNum-1;j++)  
    {
        if (tab->data[i][j+varNum+1+positiveParamNum]!=0) {allPositive=false;}
        if (tab->data[i][j+varNum+1+positiveParamNum]!=0) {allNegative=false;}
    }
    if (allPositive && allNegative) return ZERO;
    //проверяем свободные члены
    if (tab->data[i][varNum]<=0) {allPositive=false;}
    if (tab->data[i][varNum]>=0) {allNegative=false;}

    if (allPositive) return POSITIVE;
    if (allNegative) return NEGATIVE;

    //делаем проверку с учетом контекста
    Polyhedron::InequalityList::iterator it = con->m_ins.begin();
    for ( ;it != con->m_ins.end(); ++it)
    {
        Inequality* conEq = *it;        
        //ищем неравенство с совпадающей частью без свободного члена
        bool equal = true;
        for (int j = 0; j < tab->dimj-varNum-1; j++)
        {
            int coef = j+1 > conEq->m_dim-1 ? 0 : conEq->at(j+1);
            if ( tab->data[i][j+varNum+1] != coef ) 
            {
                equal = false;
                break;
            }
        }
        if (equal) 
        {
            if (tab->data[i][varNum] >= (*it)->at(0)) //если еще свободный член больше, тогда положительность
                return POSITIVE;
            continue;
        }

        //ищем неравенство с совпадающей частью со знаком - без свободного члена
        bool equalNeg = true;
        for (int j = 0; j < tab->dimj-varNum-1; j++)
        {
            int coef = j+1 > conEq->m_dim-1 ? 0 : conEq->at(j+1);
            if ( tab->data[i][j+varNum+1] != -coef ) 
            {
                equalNeg = false;
                break;
            }
        }
        if (equalNeg) 
        {
            if (tab->data[i][varNum] + (*it)->at(0) < 0) return NEGATIVE;
            continue;
        }
    }
    return UNKNOWN;
}

int CalcTableauSigns(Tableau* tab, Polyhedron* con)
{
    //на стр.8 статьи Фотрье правильно написано: знак неизвестен, если прямое и противоположное неравенство 
    //совместимы с контекстом, и известен, если совместимо только одно. Но чтобы проверить совместимость,
    //нужна функция ApplySimplexForFeasibilityProblem, которая тоже использует CalcTableauSigns - замкнутый круг
    //Выход: проверять чисто формально, пользуясь положительностью positiveParamNum штук параметров. 
    //Если все коэф. при positiveParamNum штук
    //положительных параметрах t(z) >=0, а при отрицательных ==0, то t(z)>=0.
    //ВАЖНО: ЗДЕСЬ ПЕРЕСЧИТЫВАЮТСЯ ТОЛЬКО НЕИЗВЕСТНЫЕ ЗНАКИ, ИЗВЕСТНЫЕ - НЕ ТРОГАЮТСЯ
    //возвращает 0, если в ходе работы функции получено, что данная пара: таблица, контекст - не допустимые (->не опт)

	int varNum=tab->varNum, positiveParamNum=tab->positiveParamNum;
	OPS_UNUSED(positiveParamNum);
	for(int i=0;i<tab->dimi;i++)	
    {
        if (tab->sign[i]==UNKNOWN)  
        {
            tab->sign[i] = CalcOneTableauSign(i, tab, con);
            if (tab->sign[i] == NEGATIVE)
            {
                //если при этом все коэффициенты при неизвестных в симплекс методе отрицательны, то 
                //данная пара: таблица, контекст - не допустимые (->не опт)
                bool allNegative=true;
                for (int j=0;j<varNum;j++)
                    if (tab->data[i][j]>0) {allNegative=false; break;}
                if (allNegative) return 0;
            }
        }
	}
	return 1;
}

#ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS

int deepOfTheSimplexSearchForFeas;

#endif

//ищет лексикографический минимум симлекс-методом. Неизвестные - первые varNum столбцов матрицы 
//root->tableau предполагаются положительными, параметры могут быть произвольного знака. 
//Количество положительных параметров содержится в root->tableau->positiveParamNum
//Память под solution, root и newParamVector надо освобождать вручную!!!!!!!!!!!!!!!!!!!
void ApplySimplex(TreeNode* root,std::list<TreeNode*>& solution,NewParamVector* newParamVector)
{
    #ifdef LATTICE_TABLES_DEBUG
	deepOfTheSimplexSearch++;
    #endif
    
	int deleteNewParamList=0;//освобождать или нет newParamVector при выходе из функции
	if(newParamVector==NULL)	{
		newParamVector=new NewParamVector();
		deleteNewParamList=1;
	}
	int psNum,pcNum;
	Tableau* tab=root->tableau;

#ifdef LATTICE_TABLES_DEBUG
    file<<"Tableau before CalcTableauSigns:"<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,0);file<<std::endl;file<<"Up..."<<std::endl;
    file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
    file.flush();
#endif

    CalcTableauSigns(tab,root->context);

#ifdef LATTICE_TABLES_DEBUG
    file<<"Tableau after CalcTableauSigns:"<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,0);file<<std::endl;file<<"Up..."<<std::endl;
    file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
    file.flush();
#endif


	while((psNum=FindNegative(tab))!=-1)	
	{
		if((pcNum=FindPivot(tab,psNum))!=-1)
		{	
#ifdef LATTICE_TABLES_DEBUG
			file<<"The Tableau Before Pivoting step:"<<std::endl;
			file<<"d="<<root->d<<std::endl;
			file<<*tab;file<<"Context:\n";
			root->context->Print(file,(const char**)NULL,0,0);file<<std::endl;
			file<<"psNum="<<psNum<<"_______pcNum="<<pcNum<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
            file.flush();
#endif
            // Pivoting Step...
			for(int j=0;j<tab->dimj;j++)
			{
				if(j==pcNum)continue;
				for(int i=0;i<tab->dimi;i++)
				{
					if(i==psNum)continue;
					tab->data[i][j]=(tab->data[i][j]*tab->data[psNum][pcNum]-tab->data[psNum][j]*tab->data[i][pcNum])/root->d;
                    tab->sign[i]=UNKNOWN;//знак t(z) стал неизвестным
				}
			}
			root->d=tab->data[psNum][pcNum];

			// Зануляем разрешающую строку (кроме разр. элемента)
			memset(tab->data[psNum],0,tab->dimj*sizeof(int));
			tab->data[psNum][pcNum]=root->d;
            tab->sign[psNum]=POSITIVE;//знак t(z) стал положительным

			if(CalcTableauSigns(tab,root->context)==0)
			{//т.е. если таблица+контекст - не допустимые
				root->SetStatus(NO_SOLUTION);

#ifdef LATTICE_TABLES_DEBUG
				file<<"The NOT optimal Tableau:(Sings may not be valid!)"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,0);file<<std::endl;file<<"Up..."<<std::endl;
			    file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
                file.flush();
#endif

				if(deleteNewParamList)delete newParamVector;

#ifdef LATTICE_TABLES_DEBUG
				file<<"Up..."<<std::endl;
				deepOfTheSimplexSearch--;
                file.flush();
#endif

				return;
			}
#ifdef LATTICE_TABLES_DEBUG
			file<<"The Tableau after Pivoting step:"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,0);file<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
            file.flush();
#endif

		}
		else
		{	// Failure node
			root->SetStatus(NO_SOLUTION);

#ifdef LATTICE_TABLES_DEBUG
			file<<"The NOT optimal Tableau:"<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,0);file<<std::endl;file<<"Up..."<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
            file.flush();
#endif

			if   (deleteNewParamList)  delete newParamVector;
			return;
		}
	}

	if  ((psNum = FindUnknown(tab)) != -1)//если строка со своб. членом неизвестного знака найдена - ветвим алгоритм, иначе симплекс алгоритм законыен и у нас есть решение
	{
#ifdef LATTICE_TABLES_DEBUG
        file<<"Find Unknown "<< psNum <<" in Tableau:"<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,0);file<<std::endl;file<<"Up..."<<std::endl;
        file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
        file.flush();
#endif
        //добавляем новые параметры в текущий узел
        root->newParamVector = new NewParamVector(*newParamVector);
        
        Tableau* t;

		// To suppose that unknown sign is positive...
		root->right=new TreeNode();
		t=root->right->tableau=new Tableau(*tab);
		root->right->context=new Polyhedron(*(root->context));
		t->sign[psNum]=POSITIVE;
		root->right->d=root->d;

#ifdef LATTICE_TABLES_DEBUG
		file<<"Adding inequality:";(Inequality(&(t->data[psNum][t->varNum]),t->dimj-t->varNum)).Print(file,(const char**)NULL,0,0);
		file<<std::endl;
		file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
        file.flush();
#endif
        root->m_condition=SimpleLinearExpression(&(t->data[psNum][t->varNum]),t->dimj-t->varNum);
		root->right->context->AddInequality(&(t->data[psNum][t->varNum]),t->dimj-t->varNum);

		if(root->right->context->IsFeasible(tab->positiveParamNum))
			ApplySimplex(root->right,solution,newParamVector);	
		else
		{
			root->right->SetStatus(NO_SOLUTION);

#ifdef LATTICE_TABLES_DEBUG
			file<<"\nThe NOT optimal Tableau: UnFeasible Context"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*t<<std::endl;file<<"Context:\n";root->right->context->Print(file,(const char**)NULL,0,0);file<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
			file.flush();
#endif
		}

		// To suppose that unknown sign is negative...

		root->left=new TreeNode();
		t=root->left->tableau=new Tableau(*tab);
		root->left->context=new Polyhedron(*(root->context));

		t->sign[psNum]=NEGATIVE;
		root->left->d=root->d;

#ifdef LATTICE_TABLES_DEBUG
		file<<"Adding opposite inequality to:";(Inequality(&(t->data[psNum][t->varNum]),t->dimj-t->varNum)).Print(file,(const char**)NULL,0,0);
		file<<std::endl;
		file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
        file.flush();
#endif

		root->left->context->AddOppositeInequality(&(t->data[psNum][t->varNum]),t->dimj-t->varNum);

		if(root->left->context->IsFeasible(tab->positiveParamNum))
			ApplySimplex(root->left,solution,newParamVector);
		else
		{
			root->left->SetStatus(NO_SOLUTION);

#ifdef LATTICE_TABLES_DEBUG
			file<<"\nThe NOT optimal Tableau: UnFeasible Context"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*t;file<<"Context:\n";root->left->context->Print(file,(const char**)NULL,0,0);file<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
            file.flush();
#endif
		}
	}
	else
	{// The solution is FOUND !
		int nonlinear=0;
		int rowForCut = 0;
		if(!(root->d==1))
		{
			//Selecting the earliest arrow i such that (row % root->d) != 0
			for(rowForCut=0;rowForCut<tab->varNum;rowForCut++)
			{
				for(int j=tab->varNum;j<tab->dimj;j++)
					if(tab->data[rowForCut][j]%root->d)
					{
						nonlinear=1;
						break;
					}
				if(nonlinear)break;//Найдена интересующая строка
			}
		}
		if(nonlinear)
		{// т.е. найденое решение НЕ целочисленное, нужно добавлять отсечения...
			//Сначала проверим, будет ли отсечение константным или параметрическим...
			bool constantCut=true;
			for(int j=(tab->varNum+1);j<tab->dimj;j++)
				if(tab->data[rowForCut][j]%root->d)
				{//значит не константное отсечение (нужно добавлять переменную)
					constantCut=false;
					break;
				}
			if(constantCut)	{//Если отсечение константное...
				tab->AddNewRow();

				for(int j=0;j<tab->varNum;j++)
					tab->data[tab->dimi-1][j]=GetRemainder(tab->data[rowForCut][j],root->d);

				for(int j=tab->varNum;j<tab->dimj;j++)
					tab->data[tab->dimi-1][j]=-GetRemainder(-tab->data[rowForCut][j],root->d);

				tab->sign[tab->dimi-1]=NEGATIVE;
			}
			else
			{//Если отсечение параметрическое (нужно вводить новый параметр и т.д.)...
				if(tab->spareDim==0)
				{//т.е. в таблице НЕТ свободных строк и столбцов, добавим...
					tab->AddSpareDims();				
				}			
				for(int j=0;j<tab->varNum;j++)
					tab->data[tab->dimi][j]=GetRemainder(tab->data[rowForCut][j],root->d);

				for(int j=tab->varNum;j<tab->dimj;j++)
					tab->data[tab->dimi][j]=-GetRemainder(-tab->data[rowForCut][j],root->d);

				tab->data[tab->dimi][tab->dimj]=root->d;
				tab->sign[tab->dimi]=NEGATIVE;
				tab->dimi++;tab->dimj++;
				tab->spareDim--;//т.к. добавили строку

				//добавляем равенство, описывающее новую переменную...
				NewParamEquation* newParEq=new NewParamEquation(tab->dimj-tab->varNum-1);
				newParEq->m_denom=root->d;
				for(int j=tab->varNum+1;j<(tab->dimj-1);j++)
					newParEq->m_coefs[j-tab->varNum-1]=GetRemainder(-tab->data[rowForCut][j],root->d);
			
				//free coef...
				newParEq->m_coefs[newParEq->m_dim-1]=GetRemainder(-tab->data[rowForCut][tab->varNum],root->d);
				newParamVector->PushBack(newParEq);
				//добавили

				//Теперь будем добавлять неравенства в контекст...
				Inequality inq(tab->dimj-tab->varNum);
				memcpy(&(inq.m_coefs[1]),newParEq->m_coefs,(newParEq->m_dim-1)*sizeof(int));
				inq.m_coefs[0]=newParEq->m_coefs[newParEq->m_dim-1];
				inq.m_coefs[inq.m_dim-1]=-root->d;
				root->context->AddInequality(inq);

				memcpy(inq.m_coefs,&(tab->data[tab->dimi-1][tab->varNum]),(tab->dimj-tab->varNum)*sizeof(int));
				inq.m_coefs[0]+=root->d-1;
				root->context->AddInequality(inq);
				//добавили неравенства

			}
#ifdef LATTICE_TABLES_DEBUG
			file<<"\n !!! Cut Was ADDED! Applying Simplex...!!!\n\n";
			file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,0);
			file<<std::endl;
			if(!root->context->IsFeasible(tab->positiveParamNum))			{
				std::cout<<_TL("\n WARNING: Unexpected situation in Simplex Algorithm - Context is NOT feasible\n","");
				file<<_TL("\n WARNING: Unexpected situation in Simplex Algorithm - Context is NOT feasible\n","");
			}
            file.flush();
#endif
			if(root->context->IsFeasible(tab->positiveParamNum))
			{
				ApplySimplex(root,solution,newParamVector);// поехали дальше
			}
			newParamVector->DeleteBack();

		}
		else
		{// т.е. найденое решение отвечает требованию целочисленности
            root->newParamVector = new NewParamVector(*newParamVector);
		    solution.push_back(new TreeNode(new Tableau(*tab),root->d,new Polyhedron(*(root->context)),new NewParamVector(*newParamVector)));		

#ifdef LATTICE_TABLES_DEBUG
		    file<<"The optimal Tableau:"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,0);file<<std::endl;
		    file<<"Deep="<<deepOfTheSimplexSearch<<std::endl;
            file.flush();
#endif
		}

	}
#ifdef LATTICE_TABLES_DEBUG
	file<<"Up..."<<std::endl;
	deepOfTheSimplexSearch--;
    file.flush();
#endif

	if (deleteNewParamList)  delete newParamVector;
	return;
}

void ApplySimplexForFeasibilityProblem(TreeNode* root,std::list<TreeNode*>& solution,NewParamVector* newParamVector)
{
    #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
	deepOfTheSimplexSearchForFeas++;
    #endif
	int deleteNewParamList=0;//освобождать или нет newParamVector при выходе из функции
	if(newParamVector==NULL)	{
		newParamVector=new NewParamVector();
		deleteNewParamList=1;
	}
	int psNum,pcNum;
	Tableau* tab=root->tableau;
    CalcTableauSigns(tab,root->context);
	while((psNum=FindNegative(tab))!=-1)		{
		if((pcNum=FindPivot(tab,psNum))!=-1)
		{	// Pivoting Step...
            #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
			file<<"The Tableau Before Pivoting step:"<<std::endl;
			file<<"d="<<root->d<<std::endl;
			file<<*tab;file<<"Context:\n";
			root->context->Print(file,(const char**)NULL,0,1);file<<std::endl;
			file<<"psNum="<<psNum<<"_______pcNum="<<pcNum<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearchForFeas<<std::endl;
            #endif
			for(int j=0;j<tab->dimj;j++)			{
				if(j==pcNum)continue;
				for(int i=0;i<tab->dimi;i++)				{
					if(i==psNum)continue;
					tab->data[i][j]=(tab->data[i][j]*tab->data[psNum][pcNum]-tab->data[psNum][j]*tab->data[i][pcNum])/root->d;
                    tab->sign[i]=UNKNOWN;//знак t(z) стал неизвестным
				}
			}
			root->d=tab->data[psNum][pcNum];
			// Зануляем разрешающую строку (кроме разр. элемента)
			memset(tab->data[psNum],0,tab->dimj*sizeof(int));
			tab->data[psNum][pcNum]=root->d;
            tab->sign[psNum]=POSITIVE;//знак t(z) стал положительным

			if(CalcTableauSigns(tab,root->context)==0)
			{//т.е. если таблица+контекст - не допустимые
				root->SetStatus(NO_SOLUTION);
                
                #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
				file<<"The NOT optimal Tableau:(Sings may not be valid!)"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,1);file<<std::endl;file<<"Up..."<<std::endl;
				file<<"Deep="<<deepOfTheSimplexSearchForFeas<<std::endl;
                #endif

				if(deleteNewParamList)delete newParamVector;

                #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
				file<<"Up..."<<std::endl;
				deepOfTheSimplexSearchForFeas--;
                #endif
				return;
			}

            #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
			file<<"The Tableau after Pivoting step:"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,1);file<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearchForFeas<<std::endl;
            #endif
		}
		else	{	// Failure node
			root->SetStatus(NO_SOLUTION);
            #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
			file<<"The NOT optimal Tableau:"<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,1);file<<std::endl;file<<"Up..."<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearchForFeas<<std::endl;
            #endif

			if (deleteNewParamList) delete newParamVector;
			return;
		}
	}
	if((psNum=FindUnknown(tab))!=-1)	{
		Tableau* t;
		// To suppose that unknown sign is positive...
		root->right=new TreeNode();
		t=root->right->tableau=new Tableau(*tab);
		root->right->context=new Polyhedron(*(root->context));
		t->sign[psNum]=POSITIVE;
		root->right->d=root->d;

        #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
		file<<"Adding inequality:";(Inequality(&(t->data[psNum][t->varNum]),t->dimj-t->varNum)).Print(file,(const char**)NULL,0,1);
		file<<std::endl;
		file<<"Deep="<<deepOfTheSimplexSearchForFeas<<std::endl;
        #endif

		root->right->context->AddInequality(&(t->data[psNum][t->varNum]),t->dimj-t->varNum);
		if(root->right->context->IsFeasible(tab->positiveParamNum))
			ApplySimplex(root->right,solution,newParamVector);	
		else		{
			root->right->SetStatus(NO_SOLUTION);

            #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
			file<<"\nThe NOT optimal Tableau: UnFeasible Context"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*t<<std::endl;file<<"Context:\n";root->right->context->Print(file,(const char**)NULL,0,1);file<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearchForFeas<<std::endl;
			file.flush();
            #endif
		}
		// To suppose that unknown sign is negative...
		root->left=new TreeNode();
		t=root->left->tableau=new Tableau(*tab);
		root->left->context=new Polyhedron(*(root->context));

		t->sign[psNum]=NEGATIVE;
		root->left->d=root->d;

        #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
		file<<"Adding opposite inequality to:";(Inequality(&(t->data[psNum][t->varNum]),t->dimj-t->varNum)).Print(file,(const char**)NULL,0,1);
		file<<std::endl;
		file<<"Deep="<<deepOfTheSimplexSearchForFeas<<std::endl;
        #endif

		root->left->context->AddOppositeInequality(&(t->data[psNum][t->varNum]),t->dimj-t->varNum);
		if(root->left->context->IsFeasible(tab->positiveParamNum))
			ApplySimplex(root->left,solution,newParamVector);
		else		{
			root->left->SetStatus(NO_SOLUTION);

            #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
			file<<"\nThe NOT optimal Tableau: UnFeasible Context"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*t;file<<"Context:\n";root->left->context->Print(file,(const char**)NULL,0,1);file<<std::endl;
			file<<"Deep="<<deepOfTheSimplexSearchForFeas<<std::endl;
            #endif
		}
	}
	else	{// The solution is FOUND !		
		int nonlinear=0;
		int rowForCut=0;
		if(!(root->d==1))		{
			//Selecting the earliest arrow i such that (row % root->d) != 0
			for(rowForCut=0;rowForCut<tab->varNum;rowForCut++)			{
				for(int j=tab->varNum;j<tab->dimj;j++)
					if(tab->data[rowForCut][j]%root->d)		{
						nonlinear=1;
						break;
					}
				if (nonlinear) break;//Найдена интересующая строка
			}
		}
		if(nonlinear)
		{// т.е. найденое решение НЕ целочисленное, нужно добавлять отсечения...
			//Сначала проверим, будет ли отсечение константным или параметрическим...
			bool constantCut=true;
			for(int j=(tab->varNum+1);j<tab->dimj;j++)
				if(tab->data[rowForCut][j]%root->d)
				{//значит не константное отсечение (нужно добавлять переменную)
					constantCut=false;
					break;
				}
			if(constantCut)
			{//Если отсечение константное...
				tab->AddNewRow();
				for(int j=0;j<tab->varNum;j++)
					tab->data[tab->dimi-1][j]=GetRemainder(tab->data[rowForCut][j],root->d);
				for(int j=tab->varNum;j<tab->dimj;j++)
					tab->data[tab->dimi-1][j]=-GetRemainder(-tab->data[rowForCut][j],root->d);
				tab->sign[tab->dimi-1]=NEGATIVE;
			}
			else
			{//Если отсечение параметрическое (нужно вводить новый параметр и т.д.)...
				if(tab->spareDim==0)
				{//т.е. в таблице НЕТ свободных строк и столбцов, добавим...
					tab->AddSpareDims();				
				}
				for(int j=0;j<tab->varNum;j++)
					tab->data[tab->dimi][j]=GetRemainder(tab->data[rowForCut][j],root->d);
				for(int j=tab->varNum;j<tab->dimj;j++)
					tab->data[tab->dimi][j]=-GetRemainder(-tab->data[rowForCut][j],root->d);
				tab->data[tab->dimi][tab->dimj]=root->d;
				tab->sign[tab->dimi]=NEGATIVE;
				tab->dimi++;tab->dimj++;
				tab->spareDim--;//т.к. добавили строку
				//добавляем равенство, описывающее новую переменную...
				NewParamEquation* newParEq=new NewParamEquation(tab->dimj-tab->varNum-1);
				newParEq->m_denom=root->d;
				for(int j=tab->varNum+1;j<(tab->dimj-1);j++)
					newParEq->m_coefs[j-tab->varNum-1]=GetRemainder(-tab->data[rowForCut][j],root->d);			
				//free coef...
				newParEq->m_coefs[newParEq->m_dim-1]=GetRemainder(-tab->data[rowForCut][tab->varNum],root->d);
				newParamVector->PushBack(newParEq);
				//добавили
				
                //Теперь будем добавлять неравенства в контекст...
				Inequality inq(tab->dimj-tab->varNum);
				memcpy(&(inq.m_coefs[1]),newParEq->m_coefs,(newParEq->m_dim-1)*sizeof(int));
				inq.m_coefs[0]=newParEq->m_coefs[newParEq->m_dim-1];
				inq.m_coefs[inq.m_dim-1]=-root->d;
				root->context->AddInequality(inq);

				memcpy(inq.m_coefs,&(tab->data[tab->dimi-1][tab->varNum]),(tab->dimj-tab->varNum)*sizeof(int));
				inq.m_coefs[0]+=root->d-1;
				root->context->AddInequality(inq);
				//добавили неравенства
			}
            //	DEBUGGING THING...
            #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
			file<<"\n !!! Cut Was ADDED! Applying Simplex...!!!\n\n";
			file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,1);
			file<<std::endl;
			if(!root->context->IsFeasible(tab->positiveParamNum))			{
				std::cout<<_TL("\n WARNING: Unexpected situation in Simplex Algorithm - Context is NOT feasible\n","");
				file<<_TL("\n WARNING: Unexpected situation in Simplex Algorithm - Context is NOT feasible\n","");
			}
            #endif
            //	END OF DEBUGGING THING
			if(root->context->IsFeasible(tab->positiveParamNum))			{
				ApplySimplexForFeasibilityProblem(root,solution,newParamVector);// поехали дальше
			}
			newParamVector->DeleteBack();
		}
		else	{// т.е. найденое решение отвечает требованию целочисленности
		solution.push_back(new TreeNode(new Tableau(*tab),root->d,new Polyhedron(*(root->context)),new NewParamVector(*newParamVector)));		
        #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
		file<<"The optimal Tableau:"<<std::endl;file<<"d="<<root->d<<std::endl;file<<*tab;file<<"Context:\n";root->context->Print(file,(const char**)NULL,0,1);file<<std::endl;
		file<<"Deep="<<deepOfTheSimplexSearchForFeas<<std::endl;
        #endif
		}
	}
    #ifdef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
	file<<"Up..."<<std::endl;
	deepOfTheSimplexSearchForFeas--;
    #endif

	if(deleteNewParamList)delete newParamVector;
	return;
}



int GetInvertedMatrix(int** src,int dimi,/*int dimj,*/int**& inverted,int& denom)
{
//	if(dimi>dimj)return 1;//Not Invertable	

	//Теперь можно искать обратную матрицу
	int newDimj=dimi;

	inverted=new int*[dimi];
	for(int i=0;i<dimi;i++)
	{
		inverted[i]=new int[newDimj];
		memset(inverted[i],0,newDimj*sizeof(int));
		inverted[i][i]=1;
	}

	int* data=new int[dimi];//data[i] содержит номер столбца, где находится разрешающий элемент
	memset(data,-1,dimi*sizeof(int));

	denom=1;
	for(int col=0;col<newDimj;col++)
	{
		int row=-1;
		//ищем НЕнулевой элемент && чтобы data[row] было < 0;
		for(int i=0;i<dimi;i++)
			if(data[i]>=0)continue;//значит в этой строке уже брался разреш. элемент
			else
			{
				if(src[i][col]){row=i;data[i]=col;break;}//нашли разреш. элемент
			}

		if(row==-1)
		{//обратная матрица всетаки НЕ существует
			//clearing mem...	
			delete[] data;

			for(int i=0;i<dimi;i++)
				delete[] inverted[i];
			delete[] inverted;
/*
			if(releaseSrcMem)
			{
				for(int i=0;i<dimi;i++)
					delete[] src[i];
				delete[] src;
			}
*/
			return 1;
		}

		//зануляем все элементы столбца, кроме разрешающего...

		// src(row, col) - pivoting element
		for(int i=0;i<dimi;i++)
		{
			if(i==row)continue;
			int temp=src[i][col];
			for(int j=0;j<newDimj;j++)
			{
				src[i][j]=(src[i][j]*src[row][col]-src[row][j]*temp)/denom;
				inverted[i][j]=(inverted[i][j]*src[row][col]-inverted[row][j]*temp)/denom;
			}
		}
		denom=src[row][col];
	}
	
	//проверим модуль определителя исходной матрицы на равенство 1...
	int det=1;
	for(int i=0;i<dimi;i++)
		det*=src[i][data[i]];

//	int realDetAbs=abs(det);
	int detDenom=1;
	if(abs(denom)!=1)
		for(int i=0;i<dimi;i++)
			detDenom*=denom;
		

//	if(!((abs(det/(denom*dimi))==1)&&((det%(denom*dimi))==0)))
//	if(abs(det)!=1)
	if(!((abs(det/detDenom)==1)&&((det%detDenom)==0)))
	{//выходим с ошибкой
		delete[] data;
		for(int i=0;i<dimi;i++)
			delete[] inverted[i];
		delete[] inverted;
		inverted=NULL;
/*		if(releaseSrcMem)
		{
			for(int i=0;i<dimi;i++)
				delete[] src[i];
			delete[] src;
		}
*/		return 2;
	}

	//приводим к диагональному виду...
	for(int i=0;i<dimi;i++)
		if(data[i]!=i)
		{
			int k;
			//ищем ты строку, которая должна стоять на i месте
			for(k=i+1;k<dimi;k++)
				if(data[k]==i)break;

			//swaping i and k strings...
			int* temp=src[i];
			src[i]=src[k];
			src[k]=temp;
			temp=inverted[i];
			inverted[i]=inverted[k];
			inverted[k]=temp;
			data[k]=data[i];
			//data[i]=i;//нет необходимости
		}

	//Found !

	delete[] data;
/*	if(releaseSrcMem)
	{
		for(int i=0;i<dimi;i++)
			delete[] src[i];
		delete[] src;
	}
*/
	return 0;
}

int GetInvertedMatrix(int** src,int dimi,int dimj,int**& inverted)
{

	//подсчитываем кол-во нулевых столбцов...
	int zeroColNumb=0;//кол-во нулевых столбцов

	int* data=new int[dimj];//data[j]==1, iff data[j]-нулевой столбец
	memset(data,0,dimj*sizeof(int));

	for(int j=0;j<dimj;j++)
	{
		int zeroFlag=1;//если zeroFlag==1, то столбец нулевой
		for(int i=0;i<dimi;i++)
			if(src[i][j]!=0)
			{
				zeroFlag=0;
				break;
			}

		if(zeroFlag)
		{
			zeroColNumb++;
			data[j]=1;
		}
	}

	if((dimj-zeroColNumb)!=dimi)
	{//Матрица не обратима на всем пространстве

		delete[] data;
		return 1;
	}

	int newDimj=dimj-zeroColNumb;

	int** newSrc;
	newSrc=new int*[dimi];
	for(int i=0;i<dimi;i++)
		newSrc[i]=new int[newDimj];

	//Copying...
	int curCol=0;
	for(int j=0;j<newDimj;j++)
	{
		if(data[j]==0)//т.е. если столбец НЕнулевой
		{
			for(int i=0;i<dimi;i++)
				newSrc[i][j]=src[i][curCol];
			curCol++;
		}
	}

	delete[] data;

	int denom;

	bool invertedMatrixFound = false;

	if(GetInvertedMatrix(newSrc,dimi,inverted,denom) == 0)
	{//УДАЧА

		bool integralCoeffs = true;

		//Поделим все коэфф. обратной матрицы на denom, если хоть один не делится, то выход
		if(denom!=1)
		{
			for(int i=0;i<dimi;i++)
			{
				for(int j=0;j<dimj;j++)
				{
					if(inverted[i][j]%denom)
					{//НЕУДАЧА

						integralCoeffs = false;
						break;

					}				
					else
						inverted[i][j]/=denom;
				}

				if(integralCoeffs == false)
				//значит, сработал break, нужно еще раз вызвать break;
					break;

			}

		}

		//Циклы отработали, обрабатываем результат...

		if(integralCoeffs == true)
		{//Матрица найдена.
			invertedMatrixFound = true;
		}
		else
		{//Все напрасно... ничего мы не нашли.
			for(int i=0;i<dimi;i++)
				delete[] inverted[i];
			delete[] inverted;

			inverted = 0;
		}
	}

	for(int i=0;i<dimi;i++)
		delete[] newSrc[i];
	delete[] newSrc;


	if(invertedMatrixFound)
		return 0;
	else
		return 1;

}

//область con должна находиться в положительном квадранте пространства
int* FindLexMinimum(Polyhedron& con,int loopNumb)
{
    //найдем максимальную размерность неравенств контекста con...
    int contextInqDim=0;
    std::list<Inequality*>::iterator firstInq=con.m_ins.begin(),lastInq=con.m_ins.end();
    while(firstInq!=lastInq)
    {
        if((*firstInq)->m_dim>contextInqDim)contextInqDim=(*firstInq)->m_dim;
        firstInq++;
    }
    //found

    //будем искать НИЖНИЕ границы (ищем лекс минимум области)
    TreeNode* newRoot=new TreeNode();
    Tableau* tab=newRoot->tableau=new Tableau(con.GetSize()+loopNumb,contextInqDim,loopNumb,0);
    newRoot->context=new Polyhedron();

    //заполняем симплекс таблицу...
    for(int i=0;i<loopNumb;i++)  tab->data[i][i]=1;

    firstInq=con.m_ins.begin();
    for(int i=loopNumb;i<tab->dimi;i++)   	{
        memcpy(tab->data[i],&((*firstInq)->m_coefs[1]),loopNumb*sizeof(int));
        tab->data[i][tab->varNum]=(*firstInq)->m_coefs[0];
        if(loopNumb<((*firstInq)->m_dim-1))
            memcpy(&(tab->data[i][loopNumb+1]),&((*firstInq)->m_coefs[loopNumb+1]),((*firstInq)->m_dim-loopNumb-1)*sizeof(int));
        firstInq++;
    }
    tab->Simplify();
    CalcTableauSigns(tab,newRoot->context);
    //Симплекс таблица подготовлена.
#ifdef LATTICE_TABLES_DEBUG
    file<<"Starting table:\n"<<*tab;
#endif
    newRoot->d=1;
    std::list<TreeNode*> solTabs;
    ApplySimplex(newRoot,solTabs);
    delete newRoot;
    if(solTabs.size()==0)	{
        std::cout<<_TL("No Solution: Empty Context","");
        return NULL;
    }
    if(solTabs.size()>1)	{
        std::cout<<_TL("\nUnexpected situation: More than ONE resulting tables\nAbnormal function termination (FindLexMinimum)\n","");
        std::list<TreeNode*>::iterator stFirst=solTabs.begin(),stLast=solTabs.end();
        while(stFirst!=stLast)		{
            delete *stFirst;
            stFirst++;
        }
        solTabs.clear();
        return NULL;
    }
    std::list<TreeNode*>::iterator stFirst=solTabs.begin();
    int* res=new int[loopNumb];
    int denom=(*stFirst)->d;
    tab=(*stFirst)->tableau;
    int addedVarNum=(*stFirst)->newParamVector->GetSize();
    int* addedRes=NULL;
    if(addedVarNum)	{
        addedRes=(*stFirst)->newParamVector->ComputeParams();
        if(addedRes==NULL)		{
            std::cout<<_TL("\nUnexpected situation: Couldn't compute added params values\nAbnormal function termination (FindLexMinimum)\n","");
            delete *stFirst;
            solTabs.clear();
            delete[] res;
            return NULL;
        }
    }
    for(int i=0;i<loopNumb;i++)
    {
        res[i]=tab->data[i][tab->varNum]/denom;
        if (addedRes)   {
            for(int j=(tab->varNum+1);j<tab->dimj;j++)    {
                if((tab->data[i][j]*addedRes[j-(tab->varNum+1)])%denom)
                    std::cout<<_TL("\nWARNING: Integral result is supposed, BUT it's not Integral! (FindLexMinimum)\n","");
                res[i]+=(tab->data[i][j]*addedRes[j-(tab->varNum+1)])/denom;
            }
        }
    }
    if(addedRes)delete[] addedRes;
    delete *stFirst;
    solTabs.clear();
    // Конец Вытаскиваем

    return res;
}

//область con должна находиться в положительном квадранте пространства
int* FindLexMaximum(Polyhedron& con,int loopNumb)
{
    //найдем максимальную размерность неравенств контекста con...
    int contextInqDim=0;
    std::list<Inequality*>::iterator firstInq=con.m_ins.begin(),lastInq=con.m_ins.end();
    while(firstInq!=lastInq)	{
        if((*firstInq)->m_dim>contextInqDim)contextInqDim=(*firstInq)->m_dim;
        firstInq++;
    }
    //found

    //будем искать ВЕРХНИЕ границы (ищем лекс максимум области)
    TreeNode* newRoot=new TreeNode();
    Tableau* tab=newRoot->tableau=new Tableau(con.GetSize()+2*loopNumb,contextInqDim+1,loopNumb,0);
    newRoot->context=new Polyhedron();

    //заполняем симплекс таблицу...
    for (int i=0;i<loopNumb;i++)  	tab->data[i][i]=1;
    firstInq=con.m_ins.begin();
    for(int i=loopNumb; i<(tab->dimi-loopNumb); i++)	{
        Inequality* tempInq=(*firstInq);
        int s=0;
        for(int j=0;j<loopNumb;j++)		{
            tab->data[i][j]=-tempInq->m_coefs[j+1];
            s+=tempInq->m_coefs[j+1];
        }
        tab->data[i][tab->dimj-1]=s;
        tab->data[i][tab->varNum]=tempInq->m_coefs[0];
        if(loopNumb<(tempInq->m_dim-1))		{
            memcpy(&(tab->data[i][loopNumb+1]),&((*firstInq)->m_coefs[loopNumb+1]),((*firstInq)->m_dim-loopNumb-1)*sizeof(int));
        }
        firstInq++;
    }
    for(int i=con.GetSize()+loopNumb;i<tab->dimi;i++)	{
        tab->data[i][i-(con.GetSize()+loopNumb)]=-1;
        tab->data[i][tab->dimj-1]=1;
    }
    tab->Simplify();
    CalcTableauSigns(tab,newRoot->context);
#ifdef LATTICE_TABLES_DEBUG
    file<<"Starting table:\n"<<*tab;
#endif
    newRoot->d=1;
    std::list<TreeNode*> solTabs;
    ApplySimplex(newRoot,solTabs);

    //Нужно удалить пустые контексты(т.е. учесть большое число) и подсчитать кол-во непустых
    std::list<TreeNode*>::iterator stFirst=solTabs.begin(),stLast=solTabs.end();
    stFirst=solTabs.begin();stLast=solTabs.end();
    int ereased=0;
    while(stFirst!=stLast)	{
        if(!(*stFirst)->context->IsFeasibleEx(tab->dimj-tab->varNum-1))		{
            delete *stFirst;
            *stFirst=NULL;
            ereased++;
        }
        stFirst++;
    }
    delete newRoot;
    if((solTabs.size()-ereased)==0)	{
        std::cout<<_TL("No Solution: Empty Context","");
        return NULL;
    }
    if((solTabs.size()-ereased)>1)	{
        std::cout<<_TL("\nUnexpected situation: More than ONE resulting tables\nAbnormal function termination (FindLexMinimum)\n","");
        std::list<TreeNode*>::iterator stFirst=solTabs.begin(),stLast=solTabs.end();
        while(stFirst!=stLast)		{
            delete *stFirst;
            stFirst++;
        }
        solTabs.clear();
        return NULL;
    }

    //	Вытаскиваем решения из симплекс таблиц, список таблиц очищается...
    int* res=new int[loopNumb];
    stFirst=solTabs.begin();stLast=solTabs.end();
    while(stFirst!=stLast)	{
        if(*stFirst==NULL)//Пробрасываем пустые
        {stFirst++;continue;}
        int denom=(*stFirst)->d;		
        Tableau* tab=(*stFirst)->tableau;
        int addedVarNum=(*stFirst)->newParamVector->GetSize();
        int* addedRes=NULL;
        if(addedVarNum)		{
            addedRes=(*stFirst)->newParamVector->ComputeParams();
            if(addedRes==NULL)			{
                std::cout<<_TL("\nUnexpected situation: Couldn't compute added params values\nAbnormal function termination (FindLexMinimum)\n","");
                delete *stFirst;
                solTabs.clear();
                delete[] res;
                return NULL;
            }
        }
        for(int i=0;i<loopNumb;i++)		{
            res[i]=-tab->data[i][tab->varNum]/denom;
            if (addedRes)   {
                for(int j=(tab->varNum+2);j<tab->dimj;j++)			{
                    if((tab->data[i][j]*addedRes[j-(tab->varNum+2)])%denom)
                        std::cout<<_TL("\nWARNING: Integral result is supposed, BUT it's not Integral! (FindLexMinimum)\n","");
                    res[i]-=(tab->data[i][j]*addedRes[j-(tab->varNum+2)])/denom;
                }
            }
        }
        if(addedRes)delete[] addedRes;
        delete *stFirst;
        stFirst++;
    }
    solTabs.clear();
    // Конец Вытаскиваем

    return res;
}

bool FindMaxValueForVar(const Polyhedron& con,int varPos, int& res)
{
	Polyhedron context(con);
	
	if(varPos!=1)
		context.SwapItems(1,varPos);

	//найдем максимальную размерность неравенств контекста con...
	int contextInqDim=0;
	std::list<Inequality*>::iterator firstInq=context.m_ins.begin(),lastInq=context.m_ins.end();
	while(firstInq!=lastInq)
	{
		if((*firstInq)->m_dim>contextInqDim)contextInqDim=(*firstInq)->m_dim;
		firstInq++;
	}
	//found

	int* resArray=FindLexMaximum(context,contextInqDim-1);
	
	if(resArray)
	{
		//int* res=new int;
		res=resArray[0];
		delete[] resArray;
		return true;
	}
	else
		return false;//Почему-то ничего не найдено...
}

int lackDiv(int a,int b)//деление с недостатком а/b
{

	if((a%b)==0) return a/b;

	if(((a>0)&&(b>0))||((a<0)&&(b<0)))
		return a/b;
	else
		return a/b-1;


}

/// Найти остаток от деления a на b. Получившийся остаток всегда >=0.
int GetRemainder(int a,int b)
{
	int r=a%b;
	if(r<0)
		r+=b;

	return r;
}

}
}
