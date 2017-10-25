#include "Analysis/LatticeGraph/ExtendedQuast.h"
#include "Analysis/LatticeGraph/ComplexCondition.h"
#include "Analysis/LatticeGraph/LinearLib.h"
#include "Analysis/LatticeGraph/PIP.h"
#include "Analysis/LatticeGraph/ParamPoint.h"
//#include "Analysis/LatticeGraph/RationalNumbers.h"
#include <fstream>

#ifdef LATTICE_TABLES_DEBUG
std::ofstream file("lattice_eval.txt");
#endif

#include "OPS_Core/msc_leakcheck.h"

namespace OPS {
namespace LatticeGraph {

//создает внутренний узел
//если trueBranch или falseBranch == NULL, создает для них пустые узлы
ExtendedQuast::ExtendedQuast(int inDim, int outDim, ComplexCondition* condition, ExtendedQuast* trueBranch, ExtendedQuast* falseBranch, NewParamVector* newParam)
{
    m_inDim=inDim;  m_outDim=outDim;
    m_newParamVector=newParam;
    OPS_ASSERT(condition!=NULL);
    m_condition=condition;
    if (trueBranch==NULL) m_trueBranch=new ExtendedQuast();  else  m_trueBranch=trueBranch;
    if (falseBranch==NULL) m_falseBranch=new ExtendedQuast(); else m_falseBranch=falseBranch;
    m_typeOfNod=INNER;
    m_returnValue=NULL;
    m_father=NULL;
}

//создает лист
ExtendedQuast::ExtendedQuast(int inDim, int outDim, ParamPoint* linExpr, NewParamVector* newParam)//линейное выражение не копируется!!!!
{
    m_inDim=inDim;  m_outDim=outDim;
    m_father=NULL;
    m_typeOfNod=LEAF;
    OPS_ASSERT(linExpr!=NULL);
    m_returnValue=linExpr;
    m_condition=NULL;
    m_newParamVector=newParam;
    m_trueBranch=NULL;
    m_falseBranch=NULL;
}

//удаляет поддерево с вершиной в данном узле
ExtendedQuast::~ExtendedQuast()
{
    clear();
}

//копирует указатели из exQ и обнуляет в exQ. ВНИМАНИЕ перед использованием этой функции надо вызвать clear()!!!!!!!!!!!!!!!!!!!
void ExtendedQuast::copyPtrExceptFatherFrom(ExtendedQuast& exQ)
{
    m_trueBranch=exQ.m_trueBranch; 
    m_falseBranch=exQ.m_falseBranch; 
    m_condition=exQ.m_condition; 
    m_returnValue=exQ.m_returnValue; 
    m_newParamVector=exQ.m_newParamVector; 
    m_typeOfNod=exQ.m_typeOfNod; 
    m_outDim=exQ.m_outDim; 
    m_inDim=exQ.m_inDim;
    if (m_trueBranch)
        m_trueBranch->setFather(this);
    if (m_falseBranch)
        m_falseBranch->setFather(this);
    
    exQ.m_trueBranch=NULL; 
    exQ.m_falseBranch=NULL; 
    exQ.m_father=NULL; 
    exQ.m_condition=NULL; 
    exQ.m_returnValue=NULL; 
    exQ.m_newParamVector=NULL; 
    exQ.m_typeOfNod=EMPTY; 
    exQ.m_outDim=0; 
    exQ.m_inDim=0;
}


void ExtendedQuast::setCondition(ComplexCondition* condition)
{
    OPS_ASSERT(condition!=NULL); 
    delete m_condition;  
    m_condition=condition;
}

void ExtendedQuast::setLinExpr(ParamPoint* linExpr)
{
	OPS_ASSERT(linExpr!=NULL); 
	if (m_returnValue) 
		delete m_returnValue; 
	m_returnValue = linExpr;
}

void ExtendedQuast::setNewParamVector(NewParamVector* newParamList)
{
    OPS_ASSERT(newParamList!=NULL); 
    delete m_newParamVector;  
    m_newParamVector=newParamList;
}

//копирование. Предок - зануляется!!!!!!!!!!!!!!!!!!!!!
ExtendedQuast* ExtendedQuast::clone()
{
    //обходим рекурсивно дерево выражения и снизу вверх (с листев до корня) копируем его
    ExtendedQuast* current=new ExtendedQuast();
    current->setTypeOfNod(m_typeOfNod);
    if (m_newParamVector) current->setNewParamVector(new NewParamVector(*m_newParamVector));
    else current->m_newParamVector=NULL;
    if (m_returnValue) 
    {
        ParamPoint* tempLinExpr = new ParamPoint(m_outDim);
        for (int i=0;i<m_outDim; i++) tempLinExpr->at(i) = new SimpleLinearExpression(*m_returnValue->at(i));
        current->setLinExpr(tempLinExpr);
    }
    else current->m_returnValue=NULL;
    if (m_condition) current->setCondition(m_condition->clone());
    else current->m_condition=NULL;
    current->m_inDim=m_inDim;  current->m_outDim=m_outDim;
    if (m_typeOfNod==INNER)
    {
        ExtendedQuast* trueBranch=m_trueBranch->clone();  
        ExtendedQuast* falseBranch=m_falseBranch->clone();
        current->setTrueBranch(trueBranch);  
        current->setFalseBranch(falseBranch);  
        trueBranch->setFather(current);
        falseBranch->setFather(current);
    }
    return current;
}

void ExtendedQuast::clear()//удаляет поддерево с вершиной в данном узле, поле father не трогает
{
    //удаляем рекурсивно
    delete m_newParamVector;
    if (m_returnValue) 
    {
        for (int i=0;i<m_outDim;i++) delete m_returnValue->at(i);
        delete m_returnValue;
    }
    delete m_condition;
    delete m_trueBranch;
    delete m_falseBranch;

    m_newParamVector=NULL;
    m_returnValue=NULL;
    m_condition=NULL;
    m_typeOfNod=EMPTY;
    m_trueBranch=NULL;
    m_falseBranch=NULL;
}

//добавляет к списку параметров еще n штук
void addParams(std::vector<std::string>& paramNames,int n)
{
    if (n>0)
    {
        size_t m=ADDED_PARAM_NAME_PREFIX.length();
        int nextParamNum;
        if (paramNames.size()!=0)  {
            std::string& last=paramNames.back();
            size_t min= m<last.length()?m:last.length();
            if ( last.substr(0,min)==ADDED_PARAM_NAME_PREFIX )  {
                nextParamNum=atoi( (last.substr(m,last.length())).c_str() )+1;
            }
            else  nextParamNum=1;
        }
        else  nextParamNum=1;
        for (int i=0;i<n;++i) {
            paramNames.push_back(ADDED_PARAM_NAME_PREFIX+OPS::Strings::format("%d",i+nextParamNum));
        }
    }
}

std::string ExtendedQuast::toString()
{
    std::vector<std::string>& argumentNames = *new std::vector<std::string>(m_inDim);
    for (int i=0; i<m_inDim; i++) argumentNames[i]="x"+OPS::Strings::format("%d",i+1);
    const std::string& s = toString(&argumentNames,NULL,0);
    delete &argumentNames; 
    return s;
}

std::string ExtendedQuast::toString(std::vector<std::string>* argumentNames,std::vector<std::string>* resultNames,int marginLeft)
{
    std::string s;
	std::string spaces(marginLeft, ' '); //отступ от левого края
    if (m_newParamVector) {
        addParams(*argumentNames,m_newParamVector->GetSize());
        s+=m_newParamVector->toString(argumentNames,marginLeft)+"\n";
    }
    switch (m_typeOfNod)    {
    case INNER:  {
        marginLeft+=4;
        s+=spaces+"if " + m_condition->toString(*argumentNames) + "\n"+spaces+"{\n" 
			+ m_trueBranch->toString(argumentNames,resultNames,marginLeft) + "\n"+spaces+"}";
		if (m_falseBranch->getTypeOfNod() != ExtendedQuast::EMPTY)
		{
			s += "\n" + spaces + "else\n" + spaces + "{\n"
			+ m_falseBranch->toString(argumentNames,resultNames,marginLeft) + "\n"+spaces+"}";
		}
        break;
                 }
    case LEAF:   {
        for (int i=0;i<m_outDim;++i)  {
			if (i != 0)
				s += "\n";
			s += spaces;
            if (resultNames) s+=resultNames->at(i); else s+="result"+OPS::Strings::format("%d",i);
			s +=" = "+m_returnValue->at(i)->toString(*argumentNames);
        }
        break;
                 }
    case EMPTY: s+=""; break;
    }
    //удаляем введенные параметры
    if (m_newParamVector) {
        std::vector<std::string>::iterator it=argumentNames->end();
        it--;
        for (int i=0; i<m_newParamVector->GetSize(); i++)  {
            it=argumentNames->erase(it);
        }
    }

    return s;
}

//можно вызывать только после установки детей у father, иначе она выбросит исключение!!!!!
void ExtendedQuast::setFather(ExtendedQuast* father)
{
    if ( (m_inDim!=father->m_inDim) || ( m_outDim!=father->m_outDim) )
        throw OPS::RuntimeError("ExtendedQuast::setFather:  Father should have the same m_inDim and m_outDim");
    if (father!=NULL)
    {
        OPS_ASSERT( ( (father->getTrueBranch()==this)&& (father->getFalseBranch()!=this) ) || ( (father->getTrueBranch()!=this)&& (father->getFalseBranch()==this) ) );
        if ( ( (father->getTrueBranch()==this)&& (father->getFalseBranch()!=this) ) || ( (father->getTrueBranch()!=this)&& (father->getFalseBranch()==this) ) )
            m_father=father;
        else
            throw OPS::RuntimeError("ExtendedQuast::setFather:  Exactly one child of father must have property m_father initialized by father.");
    }
    else m_father=NULL;
}

//если левая и правая ветки в каком-нибудь потомке - пустые, заменяем их пустым узлом
void ExtendedQuast::simplifyEmptyLeaves()
{
    if (m_typeOfNod==INNER)    
    {
        m_trueBranch->simplifyEmptyLeaves();
        m_falseBranch->simplifyEmptyLeaves();
        if ( (m_trueBranch->m_typeOfNod==EMPTY) && (m_falseBranch->m_typeOfNod==EMPTY) ) 
            makeMeEmpty();
    }
}

void ExtendedQuast::makeMeEmpty()
{
    ExtendedQuast* f=m_father;//спасаем от clear
    clear();
    m_father=f;
    m_typeOfNod=EMPTY;
}


#ifdef LATTICE_TABLES_DEBUG
int iterNum;
#endif

ExtendedQuast::ExtendedQuast(int inDim, int outDim, const TreeNode* node)//преобразует 1 решение симплекс метода в ExtendedQuast
{
#ifdef LATTICE_TABLES_DEBUG
    iterNum=0;
#endif

    m_inDim=inDim;  m_outDim=outDim;
    m_newParamVector=NULL;
    m_returnValue=NULL;
    m_condition=NULL;
    m_typeOfNod=EMPTY;
    m_trueBranch=NULL;
    m_falseBranch=NULL;
    m_father=NULL;
    constructFromTreeNode(node,0);
    simplifyEmptyLeaves();//удаляем пустые ветки


}

//рекурсивно преобразует 1 решение симплекс метода в ExtendedQuast
//addedParams - список уже добавленных в вышестоящих узлах новых праметров
//addedInequalities - список уже добавленных в вышестоящих узлах неравенств
void ExtendedQuast::constructFromTreeNode(const TreeNode* node, int addedParamsNum)
{
#ifdef LATTICE_TABLES_DEBUG
    iterNum++;
//    if (iterNum==93) надо комментировать обе строки вместе!!!!!!!!!!!!!!!!!!!!!!!! Иначе ошибки не найти!!!
//        std::cout << " iterNum = " << iterNum;
#endif

    if (node->isStatus(NO_SOLUTION)) 
    {
        makeMeEmpty(); 
        return;
    }
    //добавляем новые параметры, если они есть
    NewParamVector* newParams=node->newParamVector;
    int toAddNum;
    if (newParams) toAddNum=newParams->GetSize() - addedParamsNum; else toAddNum=0;
    for (int i=0;i<toAddNum;++i) {
        if (m_newParamVector==NULL) m_newParamVector = new NewParamVector;
        m_newParamVector->PushBack(new NewParamEquation( * (newParams->at(i+addedParamsNum)) ));
    }
    addedParamsNum+=toAddNum;
    if (node->left==NULL)//значит, это лист дерева
    {
        //делаем узел листом
        m_typeOfNod=LEAF;
        int newParamsSize;
        if (newParams) newParamsSize=newParams->GetSize(); else newParamsSize=0;
        m_returnValue=new ParamPoint(m_outDim);
        for(int i=0;i<m_outDim;i++)    {
            m_returnValue->at(i)=new SimpleLinearExpression(m_inDim+newParamsSize+1);
            for(int j=0;j<(m_inDim+newParamsSize+1);j++)    {
                m_returnValue->at(i)->at(j)=node->tableau->data[i][j+node->tableau->varNum]/node->d;
            }
        }
    }
    else //это внутренний узел дерева
    {
        m_typeOfNod=INNER;
        //заполняем условие оператора if
        m_condition=new ComplexCondition(new Inequality(node->m_condition));
        ExtendedQuast* trueBranch=new ExtendedQuast(m_inDim,m_outDim,INNER);
        ExtendedQuast* falseBranch=new ExtendedQuast(m_inDim,m_outDim,INNER);
        trueBranch->constructFromTreeNode(node->right,addedParamsNum);
        falseBranch->constructFromTreeNode(node->left,addedParamsNum);
        setTrueBranch(trueBranch);
        setFalseBranch(falseBranch);
        trueBranch->setFather(this);
        falseBranch->setFather(this);
    }
}

//вставляет вместо пустых листьев копии дерева m_toAssume
void ExtendedQuast::insertInsteadEmptyLeaves(ExtendedQuast* toAssume)
{
        if (m_typeOfNod==INNER)     {
            m_trueBranch->insertInsteadEmptyLeaves(toAssume); 
            m_falseBranch->insertInsteadEmptyLeaves(toAssume);
        }
        else
            if (m_typeOfNod==EMPTY) insertIntoTreeHere(toAssume);
};


//находит ККАФ лексикографического минимума набора решений симплекс метода
//ВНИМАНИЕ! каждое следующие решение набора должно быть для всех значений параметров лексикографически больше предыдущего!
//алгоритм: заменяет пустые ветки предыдущего дерева деревом следующего решения
ExtendedQuast* ExtendedQuast::lexMinimum(std::list<ExtendedQuast*>& funcList)
{
    std::list<ExtendedQuast*>::iterator it;
    if (funcList.size()>0)
    {
        ExtendedQuast* bigTree=funcList.front()->clone();
        for (it=funcList.begin(),++it;it!=funcList.end();++it)    
        {
            bigTree->insertInsteadEmptyLeaves(*it);
        }
        return bigTree;
    }
    else return new ExtendedQuast;
}

#ifdef LATTICE_TABLES_DEBUG
int recursNum=0;
#endif


//вычисляет значение этой функции в точке argument
//возвращает вектор нулевого размера, если argument не принадлежит области определения
std::vector<int> ExtendedQuast::evaluate(std::vector<int>& argument)
{
#ifdef LATTICE_TABLES_DEBUG
    if (recursNum==0) file << "\n";
    recursNum++;
#endif
    std::vector<int> result;
    //добавляем к аргументу вычисленные новые параметры
    int m = m_newParamVector ? m_newParamVector->GetSize() : 0;
    std::vector<int> newArgument(argument.size()+m);
    for (int i=0;i<(int)argument.size();++i) newArgument[i]=argument[i];
    if (m_newParamVector) {
		std::vector<int>* newParams;
        newParams=&m_newParamVector->evaluate(argument);
        for (int i=0;i<m;++i) 
            newArgument[argument.size()+i]=newParams->at(i);
		delete newParams;
    }

    switch (m_typeOfNod)    
    {
    case INNER:  
        {
            if (m_condition->evaluate(newArgument))  {
                #ifdef LATTICE_TABLES_DEBUG
                file << "true  ";
                #endif
                result = m_trueBranch->evaluate(newArgument);
            }
            else   {
                #ifdef LATTICE_TABLES_DEBUG
                file << "false ";
                #endif
                result = m_falseBranch->evaluate(newArgument);
            }
            break;
        }
    case LEAF:   
        {
            result.resize(m_outDim);
            for (int i=0;i<m_outDim;i++)   {
                result[i]=m_returnValue->at(i)->evaluate(newArgument);
            }
            break;
        }
    case EMPTY: break;
    }
#ifdef LATTICE_TABLES_DEBUG
    recursNum--;
#endif
    return result;
}

std::vector<int> ExtendedQuast::evaluate(const std::vector<int>& argument)
{
    return evaluate((std::vector<int>&) argument);
}

//проверяет принадлежит ли точка области определения функции
bool ExtendedQuast::isContain(std::vector<int>& argument)
{
    return evaluate(argument).size() != 0;
}


//подставляет вместо первых point.size переменных линейные выражения из Point
ExtendedQuast* ExtendedQuast::evaluate(ParamPoint& point)//TODO
{
    ExtendedQuast* result = 0;
    //TODO:
    OPS_ASSERT(!"Еще не реализовано!");

    return result;
}

//прибавляет к текущей заданную ККАФ (для ККА-многогранников получается - объединение текущего с заданным)
void ExtendedQuast::add(ExtendedQuast& other)
{
    OPS_ASSERT(!"Еще не реализовано!");

}
//умножает текущую на заданную ККАФ (для ККА-многогранников получается - пересечение текущего с заданным)
//умножение векторов - поэлементное, если длины разные, то выбрасывается исключение
void ExtendedQuast::mul(ExtendedQuast& other)
{
    OPS_ASSERT(!"Еще не реализовано!");

}
//вычитает из области определения текущего заданный ККА-многгранник (для ККА-многогранников получается - вычетание из текущего заданный)
void ExtendedQuast::subPQAPolyhedra(ExtendedQuast& other)
{
    OPS_ASSERT(!"Еще не реализовано!");

}



//вставляет глубокую копию дерева q вместо узла this
void ExtendedQuast::insertIntoTreeHere(ExtendedQuast* q)
{
    OPS_ASSERT(q!=NULL);
    ExtendedQuast* saveFather = m_father;
    ExtendedQuast* qCopy = q->clone();
    std::vector<NewParamEquation*>& newParams = getWholeNewParamListForCurrentNode();
    //вставляем нужное количество нулей, расширяя все линейные выражения (кол-во новых параметров, теперь другое)
    if (newParams.size()>0)  qCopy->insertEverywhereNZerosBeforeNewParamCoefs((int)newParams.size());
    //копируем
    clear();
    copyPtrExceptFatherFrom(*qCopy);
    m_father = saveFather;
    delete qCopy;
    delete &newParams;
}

//В ККАФ F(x) делает аффинную замену переменных x=t(y) и результата F: F_new=rt(F), таким образом получаем rt(F(t(y)))
//transformMatrix - рамера m_inDim+1 x m_inDim+1
void ExtendedQuast::transform(int** transformMatrix,int** resultTransformMatrix, int resultTransformMatrixDimj)
{
    OPS_ASSERT(resultTransformMatrixDimj>0);
    if (m_typeOfNod!=EMPTY)
    {
        //трансформируем новые переменные
        if (m_newParamVector) 
            m_newParamVector->transform(transformMatrix,m_inDim,m_inDim+1);
        switch (m_typeOfNod)
        {
		case EMPTY: break;
        case INNER:
            {
                //трансформируем условие
                m_condition->transform(transformMatrix,m_inDim,m_inDim+1); 
                m_trueBranch->transform(transformMatrix,resultTransformMatrix,resultTransformMatrixDimj);
                m_falseBranch->transform(transformMatrix,resultTransformMatrix,resultTransformMatrixDimj);
                break;
            }
        case LEAF:
            {
                //трансформируем линейные выражения в координатах возвращаемого выражения 
                if (m_outDim > 0)
                {
                    for (int i=0;i<m_outDim;i++)
                        m_returnValue->at(i)->transform(transformMatrix,m_inDim,m_inDim+1);
                    //трансформируем само возвращаемое выражение
                    //ЗАМЕТИМ, что сейчаc возвращаемые линейные выражения - это и есть выражение старых через новые,
                    //таким образом они должны быть строчками матрицы трансформации
                    //а строки resultTransformMatrix - это преобразуемые линейные выражения
                    
                    //составляем матрицу трансформации
                    int dim=m_returnValue->at(0)->m_dim;
                    int** trMatr=new int*[resultTransformMatrixDimj-1];
                    for (int i=0;i<m_outDim;i++)  {
                        trMatr[i]=new int[dim];//нельзя писать m_inDim+1 т.к. могут быть еще коэффициенты при новых параметрах
                        for (int j=0;j<dim-1;j++) trMatr[i][j]=m_returnValue->at(i)->at(j+1);
                        trMatr[i][dim-1]=m_returnValue->at(i)->at(0);
                    }
                    for (int i=m_outDim;i<resultTransformMatrixDimj-1;i++)  {
                        trMatr[i]=new int[dim];
                        for (int j=0;j<dim;j++) trMatr[i][j]=0;
						if (i<dim) trMatr[i][i]=1;
                    }
                    //составляем линейные выражения, которые будем трансформировать и записывать в возвращаемый функцией результат
                    ParamPoint result(m_outDim);
                    for (int i=0;i<m_outDim;i++)  {
                        result[i]=new SimpleLinearExpression(resultTransformMatrixDimj);
                        result[i]->at(0)=resultTransformMatrix[i][resultTransformMatrixDimj-1];//своб. член
                        //коэффициенты при счетчиках и внешних параметрах (в результате трансформации 
                        //линейное выражение изменит размерность и появятся параметры, добавленные в симплекс-методе)
                        for (int j=0;j<resultTransformMatrixDimj-1;j++)  result[i]->at(j+1)=resultTransformMatrix[i][j];
                        //применяем трансформацию
                        result[i]->transform(trMatr,resultTransformMatrixDimj-1,dim);
                    }
                    //записываем трансформированные линейные выражения в m_returnValue
                    for (int i=0;i<m_outDim;i++)  {
                        delete m_returnValue->at(i);//освобождаем память
                        m_returnValue->at(i)=result[i];
                    }
                    //освобождаем память
                    for (int i=0;i<resultTransformMatrixDimj-1;i++) delete[] trMatr[i];
                    delete[] trMatr;
                }
            }
        }
    }
}

void ExtendedQuast::removeExclusiveInequalities(GenArea area)
{
    removeExclusiveInequalitiesHelper(area);
    simplifyEmptyLeaves();
}

void ExtendedQuast::removeExclusiveInequalitiesHelper(GenArea area)
{
    if (m_typeOfNod!=INNER) return;
    //добавляем к area неравенства, определяющие новые параметры
    if (m_newParamVector)  
        for (int i=0;i<m_newParamVector->GetSize(); i++)
		{
			Polyhedron cntx = m_newParamVector->at(i)->getInequalities();
			area.IntersectWith(cntx);
		}
    //трансформируем условие в area
    GenArea trueArea(*m_condition);
    GenArea falseArea;
    falseArea.DifferenceFrom(trueArea);
    trueArea.IntersectWith(area);
    falseArea.IntersectWith(area);
    bool trueAreaIsEmpty = !trueArea.IsFeasible();
    bool falseAreaIsEmpty = !falseArea.IsFeasible();
    if (trueAreaIsEmpty || falseAreaIsEmpty)  {
        NewParamVector* saveNewParams=m_newParamVector;
        m_newParamVector=NULL;//чтобы не удалили
        if (trueAreaIsEmpty && falseAreaIsEmpty) throw OPS::RuntimeError("ExtendedQuast::removeExclusiveInequalitiesHelper: Very strange, trueAreaIsEmpty and falseAreaIsEmpty !");
        //если trueArea пустой, то условие m_condition НЕ выполняется НИКОГДА!
        if (trueAreaIsEmpty) {
            m_trueBranch->makeMeEmpty();
            m_falseBranch->removeExclusiveInequalitiesHelper(falseArea);
            ExtendedQuast* save = m_falseBranch;
            m_falseBranch=NULL;//чтобы не удалили
            clear();
            copyPtrExceptFatherFrom(*save);
            delete save;
        }
        //если falseArea пустой, то условие m_condition выполняется ВСЕГДА!
        if (falseAreaIsEmpty) {
            m_falseBranch->makeMeEmpty();
            m_trueBranch->removeExclusiveInequalitiesHelper(trueArea);
            ExtendedQuast* save = m_trueBranch;
            m_trueBranch=NULL;//чтобы не удалили
            clear();
            copyPtrExceptFatherFrom(*save);
            delete save;
        }
        //если были новые параметры, их надо объединить
        if (saveNewParams!=NULL)  {
            if (m_newParamVector!=NULL)  {//объединяем
                saveNewParams->paramEqVector.insert(saveNewParams->paramEqVector.end(),m_newParamVector->Begin(),m_newParamVector->End());
                m_newParamVector->paramEqVector.clear();
                delete m_newParamVector;
                m_newParamVector=saveNewParams;
            }
            else m_newParamVector=saveNewParams;
        }
    }
    else  {//оба непусты - упрощать нечего, условие m_condition здесь необходимо
        m_trueBranch->removeExclusiveInequalitiesHelper(trueArea);
        m_falseBranch->removeExclusiveInequalitiesHelper(falseArea);
    }
}

//выполняет легкое упрощение функции, удаляя операторы if, если в обоих ветках возвращаются одинаковые значения
void ExtendedQuast::combinePartsButRemainSameConditions()
{
    //TODO:
}

//приводит функцию к форме со сложными условиями, когда возвращаемые значения не повторяются 
//в разных сколь угодно удаленных ветках операторов if
void ExtendedQuast::combineParts()
{
    //TODO:
}

//освободить память должен тот, кто пользуется!
//NewParamEquation не копируются! Возвращаются указатели на них. Удалять их нельзя!
std::vector<NewParamEquation*>& ExtendedQuast::getWholeNewParamListForCurrentNode()
{
    std::vector<NewParamEquation*>& result = *new std::vector<NewParamEquation*>;
    ExtendedQuast* node;
    for (node=this; node!=NULL; node=node->m_father) {
        if (node->m_newParamVector) {
            result.resize(result.size()+node->m_newParamVector->GetSize());
            for (int i=0;i<node->m_newParamVector->GetSize();i++)
                 result[result.size()-node->m_newParamVector->GetSize()+i] = node->m_newParamVector->at(i);
        }
    }
    return result;
}

//служебная, название отражает ее действие
void ExtendedQuast::insertEverywhereNZerosBeforeNewParamCoefs(int N)
{
    if (N<=0) return;
    if (m_newParamVector) m_newParamVector->insertNZerosBeforeNewParamCoefs(N,m_inDim);
    switch (m_typeOfNod)    
    {
	case EMPTY: break;
    case INNER:  
        {
            m_condition->insertEverywhereNZerosBeforeNewParamCoefs(N,m_inDim);
            m_trueBranch->insertEverywhereNZerosBeforeNewParamCoefs(N);
            m_falseBranch->insertEverywhereNZerosBeforeNewParamCoefs(N);
            break;
        }
    case LEAF:   
        {
            for (int i=0;i<m_outDim;i++) m_returnValue->at(i)->insertNZerosBeforeNewParamCoefs(N,m_inDim);
            break;
        }
    }
}


}//end of namespace
}//end of namespace
