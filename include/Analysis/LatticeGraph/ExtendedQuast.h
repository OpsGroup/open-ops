#pragma once
#include <Analysis/LatticeGraph/LinearLib.h>
#include <iostream>

namespace OPS {
namespace LatticeGraph {
const std::string ADDED_PARAM_NAME_PREFIX="_p";//с этих символов будут начинаться все имена добавленных параметров
struct ComplexCondition;
struct TreeNode;
struct NewParamEquation;
struct ParamPoint;

//описывает двоичное дерево операторов if ККАФ(кусрчно квази аффинной функции) со сложными условиями
//на самом деле это вектор-функция, действующая из R^m_inDim в R^m_outDim
struct ExtendedQuast : public OPS::NonCopyableMix
{ 
public: 

    typedef enum {INNER,LEAF,EMPTY} TypeOfNod;//EMPTY - означает, что это пустой лист дерева

    //создает пустой узел
    ExtendedQuast(){m_trueBranch=NULL; m_falseBranch=NULL; m_father=NULL; m_condition=NULL; m_returnValue=NULL; m_newParamVector=NULL; m_typeOfNod=EMPTY; m_outDim=0; m_inDim=0;}
    //создает внутренний узел
    ExtendedQuast(int inDim, int outDim, ComplexCondition* condition, ExtendedQuast* trueBranch, ExtendedQuast* falseBranch, NewParamVector* newParam);//ничего не копируется!!!
    //создает лист
    ExtendedQuast(int inDim, int outDim, ParamPoint* linExpr, NewParamVector* newParam=NULL);//линейное выражение не копируется!!!!
    ExtendedQuast(int inDim, int outDim, const TreeNode* node);//преобразует 1 решение симплекс метода в ExtendedQuast
	ExtendedQuast(int inDim, int outDim):m_outDim(outDim),m_inDim(inDim){m_trueBranch=NULL; m_falseBranch=NULL; m_father=NULL; m_condition=NULL; m_returnValue=NULL; m_newParamVector=NULL; m_typeOfNod=EMPTY;}
	ExtendedQuast(int inDim, int outDim,TypeOfNod typeOfNod):m_outDim(outDim),m_inDim(inDim){m_trueBranch=NULL; m_falseBranch=NULL; m_father=NULL; m_condition=NULL; m_returnValue=NULL; m_newParamVector=NULL; m_typeOfNod=typeOfNod;}
    ExtendedQuast* clone();//копирование
    ~ExtendedQuast();//удаляет поддерево с вершиной в данном узле

    std::string toString(std::vector<std::string>* argumentNames,std::vector<std::string>* resultNames=NULL,int marginLeft=0);
    std::string toString();
    TypeOfNod getTypeOfNod(){return m_typeOfNod;}
    ParamPoint* getLinExpr(){return m_returnValue;}
    ExtendedQuast* getTrueBranch(){return m_trueBranch;}
    ExtendedQuast* getFalseBranch(){return m_falseBranch;}
    ExtendedQuast* getFather(){return m_father;}
    NewParamVector* getNewParamList(){return m_newParamVector;}
    std::vector<NewParamEquation*>& getWholeNewParamListForCurrentNode();//указателями на элементы не владеет!
    int getInDim(){return m_inDim;}
    int getOutDim(){return m_outDim;}
    void setTrueBranch(ExtendedQuast* trueBranch)//копирования не происходит!!!!!
        {if (m_trueBranch) delete m_trueBranch;  m_trueBranch=trueBranch; trueBranch->setFather(this);}

    void setFalseBranch(ExtendedQuast* falseBranch)//копирования не происходит!!!!!
        {if (m_falseBranch) delete m_falseBranch; m_falseBranch=falseBranch; falseBranch->setFather(this);}

    //можно вызывать только после установки детей у father, иначе она выбросит исключение!!!!!
    void setFather(ExtendedQuast* father);
    void setTypeOfNod(TypeOfNod typeOfNod){m_typeOfNod=typeOfNod;}
    void setCondition(ComplexCondition* condition);
    void setLinExpr(ParamPoint* linExpr);
    void setNewParamVector(NewParamVector* newParamList);

    void simplifyEmptyLeaves();//если левая и правая ветки в каком-нибудь потомке - пустые, заменяем их пустым узлом
    void clear();//удаляет поддерево с вершиной в данном узле
    void makeMeEmpty();

    //вычисляет значение этой функции в точке argument
    //возвращает NULL если argument не принадлежит области определения
    std::vector<int> evaluate(std::vector<int>& argument);
    std::vector<int> evaluate(const std::vector<int>& argument);
    //проверяет принадлежит ли точка области определения функции
    bool isContain(std::vector<int>& argument);
    //подставляет вместо первых point.size переменных линейные выражения из point
    ExtendedQuast* evaluate(ParamPoint& point);//TODO (это будет по сути - замена переменных)

    //прибавляет к текущей заданную ККАФ (для ККА-многогранников получается - объединение текущего с заданным)
    void add(ExtendedQuast& other);
    //умножает текущую на заданную ККАФ (для ККА-многогранников получается - пересечение текущего с заданным)
    //умножение векторов - поэлементное, если длины разные, то выбрасывается исключение
    void mul(ExtendedQuast& other);
    //вычитает из области определения текущего заданный ККА-многгранник (для ККА-многогранников получается - вычетание из текущего заданный)
    void subPQAPolyhedra(ExtendedQuast& other);

    //вставляет глубокую копию дерева q вместо узла this
    void insertIntoTreeHere(ExtendedQuast* q);

    //вставляет вместо пустых листьев копии дерева m_toAssume
    void insertInsteadEmptyLeaves(ExtendedQuast* toAssume);

    //находит ККАФ лексикографического минимума набора решений симплекс метода
    //ВНИМАНИЕ! каждое следующие решение набора должно быть для всех значений параметров лексикографически больше предыдущего!
    //алгоритм: заменяет пустые ветки предыдущего дерева деревом следующего решения
    static ExtendedQuast* lexMinimum(std::list<ExtendedQuast*>& funcList);
    //В ККАФ F(x) делает аффинную замену переменных x=t(y) и результата F: F_new=rt(F), таким образом получаем rt(F(t(y)))
    //transformMatrix - рамера m_inDim+1 x m_inDim+1
    void transform(int** transformMatrix,int** resultTransformMatrix, int resultTransformMatrixDimj);

    //проверяет, является ли совместной система вышестоящих неравенств для каждого листа
    //заменяет недостижимые ветки на пустые листья 
    void removeExclusiveInequalities(GenArea area);

    //выполняет легкое упрощение функции, удаляя операторы if, если в обоих ветках возвращаются одинаковые значения
    void combinePartsButRemainSameConditions();

    //приводит функцию к форме со сложными условиями, когда возвращаемые значения не повторяются 
    //в разных сколь угодно удаленных ветках операторов if
    void combineParts();


private:
    NewParamVector* m_newParamVector;
    ParamPoint* m_returnValue;//==NULL если этот узел не является листом или если он пустой
    ComplexCondition* m_condition;  
    ExtendedQuast* m_trueBranch;
    ExtendedQuast* m_falseBranch;
    ExtendedQuast* m_father;
    TypeOfNod m_typeOfNod;
    int m_outDim,m_inDim;//размерности области значений и области определения функции (добавленные параметры на эти числа никак не влияют)

    //рекурсивно преобразует 1 решение симплекс метода в ExtendedQuast
    //addedParams - список уже добавленных в вышестоящих узлах новых праметров
    //addedInequalities - список уже добавленных в вышестоящих узлах неравенств
    void constructFromTreeNode(const TreeNode* node,int addedParamsNum);
    //см. removeExclusiveInequalities() ниже
    void removeExclusiveInequalitiesHelper(GenArea polytop);
    //см. simplifyBigParam() ниже
    void simplifyBigParamHelper(int index);
    //копирует указатели из exQ и обнуляет в exQ. ВНИМАНИЕ перед использованием этой функции надо вызвать clear()!!!!!!!!!!!!!!!!!!!
    void copyPtrExceptFatherFrom(ExtendedQuast& exQ);
    //служебная, название отражает ее действие
    void insertEverywhereNZerosBeforeNewParamCoefs(int N);


} ;      

}//end of namespace
}//end of namespace
