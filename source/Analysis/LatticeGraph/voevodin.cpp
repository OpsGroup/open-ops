#include "Analysis/LatticeGraph/voevodin.h"
#include "Shared/LoopShared.h"
#include<fstream>

#include "OPS_Core/msc_leakcheck.h"

namespace OPS
{
namespace LatticeGraph
{

void VoevodinSolution::SimplifyAreas()
{
    VoevodinSolution::iterator firstS=begin(),lastS=end();
    while(firstS!=lastS)    {
        (*firstS)->areas.Simplify();
        firstS++;
    }
}

void VoevodinSolution::clear()
{
    VoevodinSolution::iterator firstS=begin(),lastS=end();
    while(firstS!=lastS) {
        delete *firstS;
        firstS++;
    }
    std::list<ArrowFunc*>::clear();
}


//TODO:исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void VoevodinSolution::Print(const char* fileName,EN_PrintModes printMode)
{
    //std::ofstream file(fileName);

    ////	PrintElemLatticeHeader(file);
    //if(GetStatus(LG_ERROR_INIT))
    //{
    //    switch(printMode)
    //    {
    //    case PM_DEVELOPER_RUSSIAN:
    //    case PM_POPULAR_RUSSIAN:
    //        file<<"Не удалось построить элементарный решетчатый граф.\n";

    //        file<<"Статус:\n";

    //        if(m_srcEntry->m_varDecl!=m_depEntry->m_varDecl)
    //            file<<"Для построения были поданы вхождения разных переменных. Для линейного класса они всегда независимы.\n";


    //        //				if((!(srcEntry.pris&&depEntry.pris)&&(srcEntry.dim!=0)))//if nonlinear indexis
    //        //				{SetStatus(LG_ERROR_INIT);return;}

    //        if(m_srcEntry->GetStatus(NONLINEAR))
    //            file<<"Вхождение-источник имеет НЕлинейное индексное выражение. Такие случаи не рассматриваются.\n";

    //        if(m_srcEntry->GetStatus(FREE_IS_PARAMETER))
    //            file<<"Вхождение-источник имеет внешнюю переменную в свободном члене индексного выражения. Такие случаи не обрабатываются в текущей реализации.\n";

    //        if(m_srcEntry->GetStatus(INDEX_COEF_IS_PARAMETER))
    //            file<<"Вхождение-источник имеет внешнюю переменную при счетчике цикла в индексном выражении. Такие случаи не обрабатываются в текущей реализации.\n";

    //        if(m_depEntry->GetStatus(NONLINEAR))
    //            file<<"Зависимое вхождение имеет НЕлинейное индексное выражение. Такие случаи не рассматриваются.\n";

    //        if(m_depEntry->GetStatus(FREE_IS_PARAMETER))
    //            file<<"Зависимое вхождение имеет внешнюю переменную в свободном члене индексного выражения. Такие случаи не обрабатываются в текущей реализации.\n";

    //        if(m_depEntry->GetStatus(INDEX_COEF_IS_PARAMETER))
    //            file<<"Зависимое вхождение имеет внешнюю переменную при счетчике цикла в индексном выражении. Такие случаи не обрабатываются в текущей реализации.\n";


    //        if((m_srcEntry->suppPolyhedron==NULL)&&m_srcEntry->loopNumb)
    //            file<<"Не удалось построить опорный многогранник для вхождения-источника. Возможные причины: границы охватывающих циклов нелинейны, содержат внешние переменные или вызовы функций.\n";

    //        if((m_depEntry->suppPolyhedron==NULL)&&m_depEntry->loopNumb)
    //            file<<"Не удалось построить опорный многогранник для зависимого вхождения. Возможные причины: границы охватывающих циклов нелинейны, содержат внешние переменные или вызовы функций.\n";

    //        break;

    //    case PM_POPULAR_ENGLISH:
    //        file<<"Couldn't build Elementary.\n";
    //        file<<"Couldn't build Elementary.\n";

    //        break;

    //    case PM_DEVELOPER_ENGLISH:
    //        file<<"Couldn't build ElemLatticeGraph.\n";
    //        break;
    //    }				

    //    return;
    //}


    //if(GetStatus(LG_NOSOLUTION))
    //{
    //    switch(printMode)
    //    {
    //    case PM_DEVELOPER_RUSSIAN:
    //    case PM_POPULAR_RUSSIAN:
    //        file<<"\nЭлементарный решетчатый граф не содержит дуг.\n";
    //        break;

    //    case PM_POPULAR_ENGLISH:
    //        file<<"\nElementary LatticeGraph contains NO Rows.\n";
    //        break;

    //    case PM_DEVELOPER_ENGLISH:
    //        file<<"\nElemLatticeGraph contains NO Rows.\n";
    //        break;
    //    }				

    //    return;
    //}

    //VoevodinSolution::iterator first=m_voevodinSolution.begin(),last=m_voevodinSolution.end();
    //while(first!=last)
    //{

    //    switch(printMode)
    //    {
    //    case PM_DEVELOPER_RUSSIAN:
    //    case PM_POPULAR_RUSSIAN:
    //        file<<_TL("Function:\n","Функция:\n");
    //        break;

    //    case PM_POPULAR_ENGLISH:
    //        file<<_TL("Function:\n","Функция:\n");
    //        break;

    //    case PM_DEVELOPER_ENGLISH:

    //        break;
    //    }

    //    ArrowFunc* temp = *first;
    //    int addedVarNum = temp->newParamList->GetSize();

    //    for(int i = 0; i < varNum; ++i)
    //    {
    //        file << varNames[i] << "' = ";
    //        bool somePrinted = false;
    //        for(int j = 0; j < paramVarNum+m_externalParamNum + addedVarNum; ++j)
    //        {
    //            if (temp->data[i][j] == 0)
    //                continue;

    //            if (temp->data[i][j] >= 0 && somePrinted)
    //                file << "+";

    //            somePrinted = true;

    //            if (temp->data[i][j] != 1)
    //                file << temp->data[i][j] << '*';//<<m_srcEntryCounterNames[j+m_srcEntry->loopNumb];

    //            if (j < paramVarNum+m_externalParamNum) {
    //                file << varNames[j+varNum] << " ";
    //            }
    //            else {
    //                //char buf[8];
    //                //sprintf(buf,"_%d",j-m_depEntry->loopNumb+1);
    //                //file << buf;
    //                file<<'_'<<(j-paramVarNum-m_externalParamNum+1);
    //            }
    //        }

    //        if (temp->data[i][paramVarNum+m_externalParamNum+addedVarNum] >= 0 && somePrinted)
    //            file << "+";
    //        file << temp->data[i][paramVarNum+m_externalParamNum+addedVarNum] << std::endl;
    //    }



    //    switch(printMode)
    //    {
    //    case PM_DEVELOPER_RUSSIAN:
    //    case PM_POPULAR_RUSSIAN:
    //        if(temp->newParamList->GetSize())
    //        {
    //            if(temp->newParamList->GetSize()==1)
    //            {
    //                file<<"\nРавенство, определяющее введенную переменную:\n";
    //            }
    //            else
    //            {
    //                file<<"\nРавенства, определяющие введенные переменные:\n";
    //            }
    //        }
    //        break;

    //    case PM_POPULAR_ENGLISH:
    //    case PM_DEVELOPER_ENGLISH:
    //        file<<"\nAdded Var(s)' Equation(s):\n\n";
    //        break;
    //    }
    //    temp->newParamList->Print(file,&(varNames[varNum]),paramVarNum+m_externalParamNum);


    //    switch(printMode)
    //    {
    //    case PM_DEVELOPER_RUSSIAN:
    //    case PM_POPULAR_RUSSIAN:
    //        if(temp->areas.GetSize()==1)
    //            file<<"\nОбласть определения функции:\n";
    //        else
    //            file<<"\nОбласти определения функции:\n";
    //        break;

    //    case PM_POPULAR_ENGLISH:
    //        file<<"\nDefinition Area:\n\n";
    //        break;

    //    case PM_DEVELOPER_ENGLISH:
    //        file<<"\nContext(s):\n\n";
    //        break;
    //    }
    //    temp->areas.Print(file,&(varNames[varNum]),paramVarNum+m_externalParamNum,1);

    //    switch(printMode)
    //    {
    //    case PM_POPULAR_RUSSIAN:

    //        break;
    //    case PM_DEVELOPER_ENGLISH:
    //        file<<"\n altPolyNumber="<<temp->altPolyNumb;
    //        break;
    //    }


    //    file<<"\n==========================================================\n\n";
    //    first++;
    //}


    //if(printMode==PM_POPULAR_RUSSIAN)
    //{//Нужно напечатать области, в которые не входят дуги.

    //    GenArea spaceWithoutRowSinks;//область, в которую дуги решетчатого графа не входят
    //    if(m_depEntry->suppPolyhedron)
    //        spaceWithoutRowSinks=GenArea(*(m_depEntry->suppPolyhedron));

    //    VoevodinSolution::iterator first = getAndMakeVoevodinSolutionInTransformedVars().begin(), last = getAndMakeVoevodinSolutionInTransformedVars().end();
    //    while(first!=last)
    //    {
    //        spaceWithoutRowSinks.DifferenceFrom((*first)->areas);
    //        first++;
    //    }		

    //    if(spaceWithoutRowSinks.GetSize()==1)
    //        file<<"\nОбласть, в которую дуги не входят:\n";
    //    else
    //        file<<"\nОбласти, в которые дуги не входят:\n";

    //    spaceWithoutRowSinks.Print(file,&(varNames[varNum]),paramVarNum+m_externalParamNum,1);		
    //}
}

//ArrowFunc::ArrowFunc(int varNum,int paramVarNum,int** _data):newParamList(NULL)
//{
//    dimi=varNum;
//    dimj=paramVarNum+1;
//    data=new int*[dimi];
//    if(_data==NULL)
//        for(int i=0;i<dimi;i++)
//            data[i]=new int[dimj];
//    else
//        for(int i=0;i<dimi;i++)
//        {
//            data[i]=new int[dimj];
//            memcpy(data[i],_data[i],dimj*sizeof(int));
//        }
//}
//
//ArrowFunc::ArrowFunc(const ArrowFunc& arrowFunc)
//{
//    if(!arrowFunc.data){data=NULL;dimi=dimj=0;return;}
//    dimj=arrowFunc.dimj;dimi=arrowFunc.dimi;
//    data=new int*[dimi];
//    for(int i=0;i<dimi;i++){data[i]=new int[dimj];memcpy(data[i],arrowFunc.data[i],dimj*sizeof(int));}
//    areas=arrowFunc.areas;
//    altPolyNumb=arrowFunc.altPolyNumb;
//    newParamList=new NewParamVector(*arrowFunc.newParamList);
//}
//
//ArrowFunc& ArrowFunc::operator=(const ArrowFunc& arrowFunc)
//{
//    if((&arrowFunc)!=this)
//    {
//        Clear();
//        if(!arrowFunc.data){data=NULL;dimi=dimj=0;return *this;}
//        dimj=arrowFunc.dimj;dimi=arrowFunc.dimi;
//        data=new int*[dimi];
//        for(int i=0;i<dimi;i++){data[i]=new int[dimj];memcpy(data[i],arrowFunc.data[i],dimj*sizeof(int));}
//        areas=arrowFunc.areas;
//        altPolyNumb=arrowFunc.altPolyNumb;
//        newParamList=new NewParamVector(*arrowFunc.newParamList);
//    }
//    return *this;
//}
//
//void ArrowFunc::Clear()
//{
//    if(!data)return;
//    for(int i=0;i<dimi;i++)delete[] data[i];
//    delete[] data;
//    data=NULL;
//    areas.Clear();
//    if(newParamList!=NULL){delete newParamList;newParamList=NULL;}
//}
//
//void ArrowFunc::GetSource(LoopIndex& source,const LoopIndex& sink)
//{
//    int* newVarArray=NULL;//массив, элементы которого равны значениям добавленных переменных
//    if(newParamList->GetSize())
//    {
//        newVarArray=new int[newParamList->GetSize()];//массив, элементы которого равны значениям добавленных переменных
//        for(int i=0;i<newParamList->GetSize();i++)
//        {
//            //			newVarArray[i]=0;
//            NewParamEquation* equation=newParamList->operator[](i);
//            newVarArray[i]=equation->GetFree();
//            for(int j=0;j<sink.GetSize();j++)
//                newVarArray[i]+=equation->m_coefs[j]*sink[j];
//
//            for(int j=0;j<i;j++)
//                newVarArray[i]+=equation->m_coefs[sink.GetSize()+j]*newVarArray[j];
//
//            newVarArray[i]=lackDiv(newVarArray[i],equation->m_denom);
//        }
//    }
//
//    for(int i=0;i<dimi;i++)
//    {
//        source[i]=data[i][dimj-1];
//        for(int j=0;j<sink.GetSize();j++)
//            source[i]+=data[i][j]*sink[j];
//
//        for(int j=0;j<newParamList->GetSize();j++)
//            source[i]+=data[i][sink.GetSize()+j]*newVarArray[j];
//
//    }
//
//    if(newVarArray)delete[] newVarArray;
//
//}
//
//
//LatticeDataElem::LatticeDataElem(ArrowFunc& _arrowFunc,StatementBase* _pStmt,const ReferenceExpression* _pOccur,OccurDesc* _occurDesc,int _occurNumb,int _commonLoopsNumb,std::string* _varNames)
//:arrowFunc(_arrowFunc),pStmt(_pStmt),pOccur(_pOccur),occurNumb(_occurNumb),commonLoopsNumb(_commonLoopsNumb)
//{
//    int dim=arrowFunc.dimi+arrowFunc.dimj-arrowFunc.newParamList->GetSize()-1;
//    varNames=new std::string[dim];
//    for(int i=0;i<dim;i++)
//    {
//        //		varNames[i]=new char[strlen(_varNames[i])+1];
//        //		strcpy(varNames[i],_varNames[i]);
//        varNames[i]=_varNames[i];
//    }
//
//    occurDesc = _occurDesc;
//}
//
//LatticeDataElem::LatticeDataElem(const LatticeDataElem& latticeDataElem):arrowFunc(latticeDataElem.arrowFunc),
//pStmt(latticeDataElem.pStmt),pOccur(latticeDataElem.pOccur),occurNumb(latticeDataElem.occurNumb),commonLoopsNumb(latticeDataElem.commonLoopsNumb)
//{
//    int dim=arrowFunc.dimi+arrowFunc.dimj-arrowFunc.newParamList->GetSize()-1;
//    varNames=new std::string[dim];
//    for(int i=0;i<dim;i++)
//    {
//        //		varNames[i]=new char[strlen(_varNames[i])+1];
//        //		strcpy(varNames[i],_varNames[i]);
//        varNames[i]=latticeDataElem.varNames[i];
//    }
//
//    occurDesc = latticeDataElem.occurDesc;
//}
//
//LatticeDataElem::~LatticeDataElem()
//{
//    //	free(operNum);
//    if(varNames!=NULL)
//    {
//        //		int dim=arrowFunc.dimi+rowFunc.dimj-rowFunc.newParamList->GetSize()-1;
//        //		for(int i=0;i<dim;i++)	delete[] varNames[i];
//        delete[] varNames;
//        varNames=NULL;
//    }
//
//}
//
//int RowExistsEx(Context* fromCon,Context* toCon,ArrowFunc* arrowFunc)
//{
//    //	Context tempToCon(*toCon);
//
//    //	if(tempToCon.IsFeasible())
//    //	{
//    Context tempFromCon(*fromCon);
//    tempFromCon.Transform(arrowFunc->data,arrowFunc->dimi,arrowFunc->dimj);
//    //		cout<<"Translated context\n";
//    //		tempFromCon.Print(cout,NULL,0,1);
//    //		cout.flush();
//    tempFromCon.IntersectWith(*toCon);
//    //		cout<<"Intersected context\n";
//    //		tempFromCon.Print(cout,NULL,0,1);
//    //		cout.flush();
//
//    if(tempFromCon.IsFeasible())
//        return 1;
//    //	}
//
//    return 0;
//}
//
//
//int CombineRowFuncs(LatticeDataElem* lde1,LatticeDataElem* lde2)
//{
//
//    if((!lde1)||(!lde2))return 2;
//
//    ArrowFunc& rf1=lde1->arrowFunc, &rf2=lde2->arrowFunc;
//
//    if(rf1.dimj!=rf2.dimj)return 4;
//
//    GenArea& oof1=rf1.areas,&oof2=rf2.areas;
//
//    GenArea intersectionArea(oof1);
//
//    intersectionArea.IntersectWith(oof2);
//
//    if(!intersectionArea.IsFeasible())return 1;
//
//    int commonLoopNumb=OPS::Shared::getCommonUpperLoopsAmount(lde1->pStmt,lde2->pStmt);
//
//    GenArea maxOOF1,maxOOF2;//maxOOFi - те области (из пересечения), где значения функции Fi выполняются позже, соответствующих значений Fj
//    GenArea addOnArea;
//
//    int eqNumb=commonLoopNumb;//кол-во равенств в создаваемом контексте
//
//    Inequality* inq=new Inequality(rf1.dimj);
//
//    for(;eqNumb>=0;eqNumb--)
//    {
//        Context con;
//
//        //Строим контекст...
//
//        for(int i=0;i<eqNumb;i++)
//        {
//            inq->m_coefs[0]=rf1.data[i][rf1.dimj-1]-rf2.data[i][rf2.dimj-1];
//
//            for(int j=1;j<inq->m_dim;j++)
//                inq->m_coefs[j]=rf1.data[i][j-1]-rf2.data[i][j-1];
//
//            con.AddInequality(*inq);
//
//            for(int j=0;j<inq->m_dim;j++)
//                inq->m_coefs[j]=-inq->m_coefs[j];
//
//            con.AddInequality(*inq);
//        }
//
//        if(eqNumb!=commonLoopNumb)
//        {//То необходимо добавить еще одно неравенство...
//
//            inq->m_coefs[0]=rf2.data[eqNumb][rf2.dimj-1]-rf1.data[eqNumb][rf1.dimj-1]-1;
//
//            for(int j=1;j<inq->m_dim;j++)
//                inq->m_coefs[j]=rf2.data[eqNumb][j-1]-rf1.data[eqNumb][j-1];
//
//            con.AddInequality(*inq);
//
//        }
//
//        // Построили контекс
//
//        //Теперь модифицируем пересечение ООФ-ов
//
//        addOnArea=intersectionArea;
//        addOnArea.IntersectWith(con);
//
//        if(eqNumb==commonLoopNumb)
//        {//Для определения порядка в этом случае, необходимо сравнить порядок следования вхождений в программе...
//
//
//
//            //			if(strcmp(lde1->operNum,lde2->operNum)<0)// БЫЛО НА СТАРОМ ВП
//            if(PrecedesStrictly(*lde1->occurDesc, *lde2->occurDesc))
//            {//на этом контексте значения rf2 позже выполняются, чем значения rf1
//                maxOOF2.UnionWith(addOnArea);
//                //				maxOOF1.DifferenceFrom(con);
//            }
//            else
//            {//на этом контексте значения rf1 позже выполняются, чем значения rf2
//                //				maxOOF1.UnionWith(addOnArea);
//                //				maxOOF2.DifferenceFrom(con);
//                5;
//            }
//        }
//        else
//        {
//            maxOOF2.UnionWith(addOnArea);
//            //			maxOOF1.DifferenceFrom(con);
//        }			
//
//        intersectionArea.DifferenceFrom(con);
//
//        //модифицировали
//
//        //if(intersectionArea.GetStatus(GE_UNFEASIBLE))
//        if(intersectionArea.IsFeasible() == 0)
//            break;//т.е. в таком случае, уже больше нечего делить
//    }
//
//    delete inq;
//
//    if(intersectionArea.IsFeasible())
//        maxOOF1.UnionWith(intersectionArea);
//
//    //модифицируем ООФы входных функций...
//
//    //if(!maxOOF2.GetStatus(GE_UNFEASIBLE))
//    if(maxOOF2.IsFeasible())
//        oof1.DifferenceFrom(maxOOF2);
//
//    //if(!maxOOF1.GetStatus(GE_UNFEASIBLE))
//    if(maxOOF1.IsFeasible())
//        oof2.DifferenceFrom(maxOOF1);
//
//    //модифицировали
//
//    return 0;
//}
//
//int GetInvertedArrowFunc(const ArrowFunc& srcArrowFunc, ArrowFunc& invertedArrowFunc)
//{
//    if(srcArrowFunc.newParamList)
//        if(srcArrowFunc.newParamList->GetSize() != 0)
//            //Такое мы вообще не обращаем
//            return 1;
//
//    int** inverted;
//
//    if(GetInvertedMatrix(srcArrowFunc.data, srcArrowFunc.dimi, srcArrowFunc.dimj-1, inverted) == 0)
//    {//Это самый простой случай: матрица обратилась на всем Rn. 
//        invertedArrowFunc.dimi = srcArrowFunc.dimi;
//        invertedArrowFunc.dimj = srcArrowFunc.dimj;
//
//        invertedArrowFunc.altPolyNumb = srcArrowFunc.altPolyNumb;
//        invertedArrowFunc.newParamList = NULL;
//
//        invertedArrowFunc.data = new int*[invertedArrowFunc.dimi];
//        for(int i=0;i<invertedArrowFunc.dimi;i++)
//            invertedArrowFunc.data[i]=new int[invertedArrowFunc.dimj];
//
//        int** func = invertedArrowFunc.data;
//
//        for(int i=0;i<invertedArrowFunc.dimi;i++)
//        {
//            for(int j=0;j<invertedArrowFunc.dimj;j++)
//            {
//
//                if(j!=(invertedArrowFunc.dimj-1))
//                    func[i][j]=inverted[i][j];
//                else
//                {
//                    func[i][j]=0;
//
//                    for(int k=0;k<invertedArrowFunc.dimi;k++)
//                        func[i][j]+=inverted[i][k]*(-srcArrowFunc.data[k][srcArrowFunc.dimj-1]);
//                }
//
//            }
//        }
//
//        //Теперь просто построим ООф...
//        invertedArrowFunc.areas = srcArrowFunc.areas;
//        invertedArrowFunc.areas.Transform(invertedArrowFunc.data, invertedArrowFunc.dimi, invertedArrowFunc.dimj);
//
//        for(int i=0;i<invertedArrowFunc.dimi;i++)
//            delete[] inverted[i];
//        delete[] inverted;
//
//        return 0;
//    }
//
//    return 1;// Ничего не построили.
//}
//
//ExpressionBase* GetExprNodeFromRowFuncLine(ArrowFunc& arrowFunc,int line,Declarations& ns,std::string* varNames)
//{//Вся сложность в том, что в ArrowFunc.data свободный член находится в конце
//    SimpleLinearExpression le(arrowFunc.dimj);
//
//    le.m_coefs[0]=arrowFunc.data[line][arrowFunc.dimj-1];
//
//    memcpy(&(le.m_coefs[1]),arrowFunc.data[line],(arrowFunc.dimj-1)*sizeof(int));
//
//    return GetExprNode(le,ns,varNames);
//}


}//end of namespace
}//end of namespace
