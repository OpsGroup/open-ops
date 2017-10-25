#include "Analysis/LatticeGraph/LoopIndex.h"
#include <iostream>

#include "OPS_Core/msc_leakcheck.h"

namespace OPS
{
namespace LatticeGraph
{

void LoopIndex::SetToLowerBounds()
{
    if (m_loopLowerBounds==NULL)    {
        throw OPS::RuntimeError("Can't SetToLowerBounds LoopIndex, because m_loopLowerBounds or m_loopUpperBounds == NULL");
    }
    if (m_counterValues==NULL) m_counterValues=new int[m_dim];
    if (getLowerBoundForCoord(0)>getUpperBoundForCoord(0)) {m_inBounds=false; return;}
    m_counterValues[0]=getLowerBoundForCoord(0);
    for (int i=1; i<m_dim; i++)  {
        m_inBounds=true;
        //увеличиваем верхние индексы, добиваясь непустоты множества итераций i-того цикла
        while ((getLowerBoundForCoord(i)>getUpperBoundForCoord(i))&&m_inBounds) 
            increase(i);

        if (m_inBounds) m_counterValues[i]=getLowerBoundForCoord(i);
        else return;//множество итераций пусто        
    }
    m_inBounds = true;
}

void LoopIndex::SetToUpperBounds()
{
    if (m_loopUpperBounds==NULL)    {
        throw OPS::RuntimeError("Can't SetToUpperBounds LoopIndex, because m_loopLowerBounds or m_loopUpperBounds == NULL");
    }
    if (m_counterValues==NULL) m_counterValues=new int[m_dim];
    if (getLowerBoundForCoord(0)>getUpperBoundForCoord(0)) {m_inBounds=false; return;}
    m_counterValues[0]=getUpperBoundForCoord(0);
    for (int i=1; i<m_dim; i++)  {
        m_inBounds=true;
        //уменьшаем верхние индексы, добиваясь непустоты множества итераций i-того цикла
        while ((getLowerBoundForCoord(i)>getUpperBoundForCoord(i))&&m_inBounds) decrease(i);

        if (m_inBounds) m_counterValues[i]=getUpperBoundForCoord(i);
        else return;//множество итераций пусто        
    }
    m_inBounds = true;
}

void LoopIndex::InitWithLower(LoopIndex& loopIndex,int commonLoopNumb)
{
    if (!loopIndex.m_inBounds)    {
        m_inBounds = false;
        return;
    }
    //копируем общие счетчики
    if (m_counterValues==NULL) m_counterValues=new int[m_dim];
    memcpy(m_counterValues,loopIndex.getCounterValues(),commonLoopNumb*sizeof(int));
    //остальным присваиваем минимально возможные значения
    for (int i=commonLoopNumb; i<m_dim; i++)   m_counterValues[i]=getLowerBoundForCoord(i);
    m_inBounds = true;
}
void LoopIndex::InitWithUpper(LoopIndex& loopIndex,int commonLoopNumb)
{
    if (!loopIndex.m_inBounds)    {
        m_inBounds = false;
        return;
    }
    //копируем общие счетчики
    if (m_counterValues==NULL) m_counterValues=new int[m_dim];
    memcpy(m_counterValues,loopIndex.getCounterValues(),commonLoopNumb*sizeof(int));
    //остальным присваиваем минимально возможные значения
    for (int i=commonLoopNumb; i<m_dim; i++)   m_counterValues[i]=getUpperBoundForCoord(i);
    m_inBounds = true;
}

//выполняет операцию ++, считая, что размерность индеса равна dimNum
//значения счетчиков с dimNum по m_dim не трогает
void LoopIndex::increase(int dimNum)
{
    if (dimNum>m_dim) throw OPS::RuntimeError("Can't make increase of LoopIndex, because dimNum > m_dim");
    if (!m_inBounds) return;

    if ((m_loopLowerBounds==NULL)||(m_loopUpperBounds==NULL))    {
        throw OPS::RuntimeError("Can't make ++ of LoopIndex, because m_loopLowerBounds or m_loopUpperBounds == NULL");
    }
    bool Continue=true;
    while (Continue)  {
        //находим первый счетчик с конца, не равный верхней границе
        int i=dimNum-1;
        for (;i>=0;i--)
            if (m_counterValues[i]!=getUpperBoundForCoord(i)) break;
        //если все счетчики достигли верха - выходим
        if (i<0) {m_inBounds = false; return;}
        //i - номер счетчика, который можно увеличить
        m_counterValues[i]++;
        Continue=false;
        //всем счетчикам, начиная с i+1-го надо присвоить минимально возможные значения (к сожалению, иногда это не getLowerBoundForCoord)
        for(int j=i+1;j<dimNum;j++)  {
            if (getLowerBoundForCoord(j)<=getUpperBoundForCoord(j)) m_counterValues[j]=getLowerBoundForCoord(j);
            else  {
                //промежуток итераций j-того цикла пуст, надо увеличивать старшие счетчики
                //присваиваем всем оставшимся счетчикам верхние границы и запускаем эту функцию сначала
                for (int k=j; k<dimNum; k++)  m_counterValues[k]=getUpperBoundForCoord(k);
                Continue=true;
                break;
            }
        }
        
    }
}

int  LoopIndex::getUpperBoundForCoord(int i)
{
    if ((m_loopLowerBounds==NULL)||(m_loopUpperBounds==NULL)) throw OPS::RuntimeError("Can't getUpperBoundForCoord for LoopIndex, because m_loopLowerBounds or m_loopUpperBounds == NULL");
    int result=m_loopUpperBounds[i][0];
    for (int j=0; j<i; j++)
        result+=m_loopUpperBounds[i][j+1]*m_counterValues[j];
    return result;
}

int  LoopIndex::getLowerBoundForCoord(int i)
{
    if ((m_loopLowerBounds==NULL)||(m_loopUpperBounds==NULL)) throw OPS::RuntimeError("Can't getLowerBoundForCoord for LoopIndex, because m_loopLowerBounds or m_loopUpperBounds == NULL");
    int result=m_loopLowerBounds[i][0];
    for (int j=0; j<i; j++)
        result+=m_loopLowerBounds[i][j+1]*m_counterValues[j];
    return result;
}

//выполняет операцию --, считая, что размерность индеса равна dimNum
//значения счетчиков с dimNum по m_dim не трогает
void LoopIndex::decrease(int dimNum)
{
   if (dimNum>m_dim) throw OPS::RuntimeError("Can't make decrease of LoopIndex, because dimNum > m_dim");
   if (!m_inBounds) return;
    if ((m_loopLowerBounds==NULL)||(m_loopUpperBounds==NULL))    {
        throw OPS::RuntimeError("Can't make ++ of LoopIndex, because m_loopLowerBounds or m_loopUpperBounds == NULL");
    }
    bool Continue=true;
    while (Continue)  {
        //находим первый счетчик с конца, не равный нижней границе
        int i=dimNum-1;
        for (;i>=0;i--)
            if (m_counterValues[i]!=getLowerBoundForCoord(i)) break;
        //если все счетчики достигли верха - выходим
        if (i<0) {m_inBounds = false; return;}
        //i - номер счетчика, который можно уменьшить
        m_counterValues[i]--;
        Continue=false;
        //всем счетчикам, начиная с i+1-го надо присвоить максимально возможные значения (к сожалению, иногда это не getUpperBoundForCoord)
        for(int j=i+1;j<dimNum;j++)  {
            if (getLowerBoundForCoord(j)<=getUpperBoundForCoord(j)) m_counterValues[j]=getUpperBoundForCoord(j);
            else  {
                //промежуток итераций j-того цикла пуст, надо уменьшать старшие счетчики
                //присваиваем всем оставшимся счетчикам нижние границы и запускаем эту функцию сначала
                for (int k=j; k<dimNum; k++)  m_counterValues[k]=getLowerBoundForCoord(k);
                Continue=true;
                break;
            }            
        }
    }
}

bool LoopIndex::operator==(LoopIndex& li)
{
    int n=m_dim<li.m_dim?m_dim:li.m_dim;//нашли минимальную размерность
    for(int i=0;i<n;i++)
        if (m_counterValues[i]!=li.m_counterValues[i]) return false;
    if(m_dim>li.m_dim)    {
        for(int i=n;i<m_dim;i++)
            if (m_counterValues[i]!=getLowerBoundForCoord(i)) return false;
    }
    else
        for(int i=n;i<li.m_dim;i++)
            if(li.m_counterValues[i]!=li.getLowerBoundForCoord(i)) return false;
    return true;
}

std::ostream& operator<<(std::ostream& os, LoopIndex& li)
{
    if (!li.getCounterValues())return os;

    int* data=li.getCounterValues(),dim=li.getSize();
    os<<'(';
    for(int i=0;i<(dim-1);i++)
        os<<data[i]<<',';

    os<<data[dim-1]<<')';
    return os;
}

//Лексикографическое сравнение векторов
//возвращаемые значения:
//	<0		a < b
//	 0		a = b
//	>0		a > b
int LexCmp(LoopIndex& a, LoopIndex& b)
{
    int n = std::min(a.getSize(), b.getSize());

    for(int i = 0; i < n; ++i)
        if (a[i] != b[i])
		{
            if (a[i] < b[i])
                return -1; // a < b
            else
                return 1; // a > b
		}
    //если сюда дошло управление, то первые n координат - равны
    if (a.getSize() < b.getSize()) {
        for(int i = n; n < b.getSize(); ++i)
            if (b[i] < b.getLowerBoundForCoord(i))
                return 1; // a > b
            else
                if (b[i] > b.getLowerBoundForCoord(i))
                    return -1; // a < b
    }
    else {
        for(int i = n; n < a.getSize(); ++i)
            if (a[i] < a.getLowerBoundForCoord(i))
                return -1; // a < b
            else
                if (a[i] > a.getLowerBoundForCoord(i))
                    return 1; // a > b
    }
    return 0;
}

int LexCmp(LoopIndex& a, LoopIndex& b, int n)
{
    for(int i = 0; i < n; ++i)
        if (a[i] != b[i])
		{
            if (a[i] < b[i])
                return -1; // a < b
            else
                return 1; // a > b
		}

    //если сюда дошло управление, то первые n координат - равны
    return 0;
}

}//end of namespace
}//end of namespace
