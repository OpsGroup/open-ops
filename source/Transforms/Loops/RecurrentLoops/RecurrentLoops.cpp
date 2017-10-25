/*Распараллеливание рекуррентных циклов*/
/*Вопросы: Олег 8-903-462-33-24*/
#include "Transforms/Loops/RecurrentLoops/RecurrentLoops.h"
#include "Transforms/Loops/RecurrentLoops/RootScan.h"
#include "Reprise/ServiceFunctions.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/LinearExpressions.h"
#include "Transforms/Loops/RecurrentLoops/RecurrentLoops.h"

#include "Transforms/Loops/IntToVector/IntToVector.h"


#include <iostream>
#include "math.h"

#include <memory>

using namespace OPS::Reprise;
using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Shared;
using namespace std;


const	long NULL_OPERATOR=100000,
		NOT_LOOP_TYPE=100001;



/*Переносит определения переменных в другой блок*/
void newVarDecBlock2(BlockStatement *sourceBlock)
{
	//???проверки!!! блоки относятся к одной подпрограмме? Есть ли у сорсБлока парентБлок

	//цикл проходит по всем переменным VariableList-а который требуется переместить
	Declarations *decls = &(sourceBlock->getDeclarations());
	//перебираем все переменные определенные в подаваемом блоке (sourceBlock)
	for(Declarations::VarIterator it1 = decls->getFirstVar(); it1.isValid(); it1++)
	{
//string str;
//str = it1->dumpState();
//cout << str;
//cout << decls->getVariableCount();
		bool isNeedNewName = false;
		if(it1->hasDefinedBlock())
		{
			//если у переменной есть блок в котором она определена (???А если нет???)
			if (&(it1->getDefinedBlock()) == sourceBlock)
			{
				//перебираем все переменные определенные в родительском блоке подаваемого блока
				for(Declarations::ConstVarIterator it2 = it1; it2 != decls->getLastVar(); it2++)
				{
//str = it2->getDefinedBlock().dumpState();
//cout << str;
//str = sourceBlock->getParent()->getParent()->cast_to<BlockStatement>().dumpState();
//cout << str;
					//если у найденной переменной (второй) блок в котором она определена является родительским блоком подаваемого блока ипри этом ее имя совпадает с рассматриваемой переменной
					if(&(it2->getDefinedBlock()) == &(sourceBlock->getParent()->getParent()->cast_to<BlockStatement>()))
						if(it1->getName() != it2->getName())
						{isNeedNewName = true;}
				}
				//есть ли необходимость создать новое имя переменной
				if(isNeedNewName)
				{	//создаем новое имя для выносимой переменной
					//VariableDeclaration &new_var = Editing::createNewVariable(it1->getType(), sourceBlock->getParentBlock());
					//ReferenceExpression *name_of_new_var = new ReferenceExpression(new_var);
					//заменяем имеющиеся вхождения переменной it1 в блоке sourceBlock на вхождения созданной меременной
					//OPS::Transforms::Scalar::makeSubstitutionForward(sourceBlock, name_of_new_var, &it1,  true);
					//Удаляем переменную it1
					//BlockStatement::Iterator iter = it1;
					//decls->erase(iter);
				}
				else
					{it1->setDefinedBlock(sourceBlock->getParent()->getParent()->cast_to<BlockStatement>());}
			}
		}
	}
}
/*Перемножение матриц*/
/*Помещает результат во вторую матрицу*/
void MultiplyMatrix(const int N, double **Matrix1, double **Matrix2)
{
	int i, j, k;
	double **MulMatrix=new double*[N];
	for (i=0; i<N; i++)
		MulMatrix[i]=new double[N];
	for (i=0; i<N; i++)
		for (j=0; j<N; j++)
			MulMatrix[i][j]=0;

	for (i=0; i<N; i++)
		for (j=0; j<N; j++)
			for (k=0; k<N; k++)
				MulMatrix[i][j]+=Matrix1[i][k] * Matrix2[k][j];

	for (i=0; i<N; i++)
		for (j=0; j<N; j++)
			Matrix2[i][j]=MulMatrix[i][j];

	for (i=0; i<N; i++)
		delete[] MulMatrix[i];
	delete[] MulMatrix;
}
double* gauss_method(const int N, double **init_matrix, double * init_free_vector)
{
	int i, j, k;
	double *free_vector=new double[N];
	double *result=new double[N];
	double **matrix=new double*[N];
	for (i=0; i<N; ++i)
	{
		matrix[i]=new double[N];
	}
	for(i=0; i<N; i++)
	{
		for(j=0; j<N; j++)
		{
			matrix[i][j]=init_matrix[i][j];
		}
		free_vector[i] = init_free_vector[i];
	}

	double multiplier;
	for(i=0; i<N-1; i++)
	{
		for(j=i+1; j<N; j++)
		{
			multiplier = matrix[j][i] / matrix[i][i];
			for(k=0; k<N; k++)
				{matrix[j][k] -= matrix[i][k] * multiplier;}
			free_vector[j] -= free_vector[i] * multiplier; 
		}
	}
	
	for(i=N-1; i>=0; i--)
	{
		for(j=N-1; j>i; j--)
			free_vector[i] -= matrix[i][j] * result[j];
		result[i] = free_vector[i] / matrix[i][i];	
	}

	for (i=0; i<N; i++)
		delete[] matrix[i];
	delete[] free_vector;
//	delete[] result;
	delete[] matrix;

	return result;
}

////////////void MakeRLT2(OPS::Reprise::ForStatement* pFor)
////////////{
////////////	//вспомогательные построения и преобразования
////////////	if (!pFor) assert(NULL_OPERATOR);
////////////	if (pFor->getObjType()!=OpsNewIr::OT_STMT_FOR)
////////////		assert(NOT_LOOP_TYPE); //оператор for
////////////	//тело цикла должно содержать один оператор. Этот оператор является оператором присваивания.
////////////	//этот оператор присваивания должен быть следующего вида: X(k)=a(k)1*X(k-1)+a(k)2*X(k-2)+…+a(k)m*X(k-m).
////////////	int is_transform_possible=1;
////////////	//если в теле цикла(внутри компаненты) больше одного оператора
////////////	OPS::Reprise::BlockStatement &rBlock=pFor->getBodyRef();
////////////	id my_id(rBlock);
////////////
////////////	if (rBlock.getStatementsCount()!=1)
////////////		{is_transform_possible=2;}
////////////	Statement::Iterator RecurentOper=rBlock.getFirst();
////////////	//этот единственный оператор является оператором присваивания
////////////	if(RecurentOper->getObjType()!=OpsNewIr::OT_STMT_EXPR)
////////////		{is_transform_possible=3;}
//////////////Типа к этому моменту мы уже определили что у нас в цикле только 1 оператор, который является оператором присваивания!
//////////////Получаем коэффециенты при x
//////////////Кроме того только что мы узнали что этот оператор имеет требующийся нам вид X(k)=a(k)1*X(k-1)+a(k)2*X(k-2)+…+a(k)m*X(k-m)
////////////	std::vector<OccurrenceInfo> AA;
////////////	std::vector<LoopInfo> BB;
////////////	my_id.getAllVariablesList(AA,1);
////////////	my_id.getOuterLoopsList(BB,1);
////////////	ExprNode* loop_counter = BB[0].pthis->clone();
////////////
////////////	int i=0,j=0;
////////////	std::vector<ExprNode*> t_X;
////////////	std::vector<OccurrenceInfo>::iterator it = AA.begin();
////////////	NameIter name_of_X=(*it).m_oDataObject;
////////////	it++;
////////////	for (; it != AA.end(); ++it)
////////////	{
////////////		OccurrenceInfo r = *it;
////////////		if(name_of_X == r.m_oDataObject)
////////////		{
////////////			t_X.push_back(&r.m_pED->getParentExpr());
////////////			i++;
////////////		}
////////////	}//поместил в вектор t_X указатели на все места где встречается массив X
////////////	int size_of_A=i;//со свободным членом будет ровно i
////////////	int size_of_X;
////////////	ExprBinary* temp_b = dynamic_cast<ExprBinary*>(BB[0].m_right);
//////////////	BB[0].m_right->C_Out(cout);
////////////	if(ExprManagement::checkCalculability(&temp_b->getRightArg()))
////////////		{size_of_X=ExprManagement::calcIntValue(&temp_b->getRightArg());}//количество элл. X участвующих в расчетах. = количеству коэфф a[i] + количество итераций цикла
////////////	else
////////////		{is_transform_possible=8;}
//////////////	BB[0].m_left->C_Out(cout);
////////////
////////////	ExprNode * pExpr;
////////////	Statement * pSt;
////////////	pSt = my_id.getThisOper();
////////////	pExpr = pSt->getAsExpr()->getExpr();
////////////	ExprAssign * pEA = dynamic_cast<ExprAssign*>(pExpr);
////////////	ExprNode *pSource =  pEA->unsafeGetRightExpr();
////////////
////////////	ExprNode* t_fcoef=getFreeCoef(pSource, t_X);
////////////	ExprNode* t_coef;
////////////	double td_fcoef;
////////////	if(ExprManagement::checkCalculability(t_fcoef))//если свободный член вычислим, то он записывается в td_fcoef
////////////		{td_fcoef=ExprManagement::calcDoubleValue(t_fcoef);}
////////////	else
////////////		{is_transform_possible=4;}
////////////	if(t_fcoef==0)
////////////		{is_transform_possible=5;}
////////////
////////////	double *A=new double[size_of_A];
////////////	for(j=0; j<i;j++)
////////////	{
////////////		t_coef=getCoef(pSource, t_X[j]);
////////////		if(ExprManagement::checkCalculability(t_coef))
////////////			{A[j]=ExprManagement::calcDoubleValue(t_coef);}
////////////		else
////////////			{is_transform_possible=6;}
////////////	}
////////////
////////////	//теперь проверяем условие устойчивости (Суховерхова)
////////////	if(rootscan(A, size_of_A))
////////////	{
////////////		is_transform_possible=7;
////////////	}
////////////
////////////	//стоит ли продолжать преобразование
////////////	switch(is_transform_possible)
////////////	{
////////////		case 1: break;
////////////		case 2: throw OPS::RuntimeError("В цикле более 1 оператора");
////////////		case 3: throw OPS::RuntimeError("Единственный оператор в цикле не является оператором присваивания");
////////////		case 4: throw OPS::RuntimeError("Свободный член не является числом");
////////////		case 5: throw OPS::RuntimeError("Свободный член не равен 0");
////////////		case 6: throw OPS::RuntimeError("Коэффициент рассматриваемого массива не вычислим");
////////////		case 7: throw OPS::RuntimeError("Преобразование не является устойчивым");
////////////		case 8: throw OPS::RuntimeError("Итератор преобразуемого цикла не является счетаемым");
////////////		default : throw OPS::RuntimeError("что-то не так :) ");
////////////	}
////////////
////////////int size_of_block=size_of_A; //размер блока строящейся блочной двухдиагональной матрицы
//////////////int Num_of_first_X=4; //номер первого вычисляемого в цикле элл. массива X
//////////////int Right_bound_of_X=Num_of_first_X-size_of_block; //номер минимального элл. массива X фигурирующего в вычислениях
/////////////////////////////////
//////////////временные данные нужный только в процессе написания программы
//////////////X[0]=3;
//////////////X[1]=4;
//////////////X[2]=7;
/////////////////////////////////
////////////
//////////////если коэффециенты A не зависят от счетчика цикла, то блочная двухдиагональная матрица
//////////////будет содержать всего два различных блока. Обозначим их соответственно Block и Block2, а затем заполним
////////////double **Block1=new double*[size_of_block], **Block2=new double*[size_of_block];
////////////for (i=0; i<size_of_block; i++)
////////////{
////////////	Block1[i]=new double[size_of_block];
////////////	Block2[i]=new double[size_of_block];
////////////}
////////////
////////////for(i=0;size_of_block>i;++i)
////////////{
////////////	for(j=i+1;size_of_block>j;++j)
////////////	{
////////////		Block1[j][i]=0;
////////////		Block2[i][j]=0;
////////////	}
////////////}
////////////for(i=0;size_of_block>i;++i)
////////////{
////////////	Block1[i][i]=A[size_of_block-1];
////////////	Block2[i][i]=1;
////////////}
////////////for(i=0;size_of_block>i;++i)
////////////{
////////////	for(j=i+1;size_of_block>j;++j)
////////////	{
////////////		Block1[i][j]=A[size_of_block-j-i];
////////////		Block2[j][i]=A[j-i];
////////////	}
////////////}
//////////////Заполнили блоки блочной двухдиагональной матрицы
////////////
////////////
//////////////находим обратную матрицу к Block2
////////////double *e=new double[size_of_block];
////////////double *res_vector;
////////////double **Block2_invert=new double*[size_of_block];
////////////for (i=0; i<size_of_block; i++)
////////////	{Block2_invert[i]=new double[size_of_block];}
////////////
////////////	for(i=0; i<size_of_block; i++)
////////////	{
////////////		for(j=0; j<size_of_block; j++)
////////////			e[j] = 0.;
////////////		e[i] = 1.;
////////////		res_vector = gauss_method(size_of_block, Block2, e);
////////////		for(j=0; j<size_of_block; j++)
////////////			Block2_invert[j][i] = res_vector[j];
////////////	}
//////////////нашли матрицу обратную к Block2. поместили ее в Block2_invert
////////////
//////////////умножим всю матрицу на матрицу обратную к Block2, таким образом получатся блоки I(единичная матрица) по диагонали
//////////////и под ними одинаковые не нулевые блоки (Block*Block2^(-1)), а остальная часть матрицы будет нулевой
////////////	MultiplyMatrix(size_of_block, Block2_invert, Block1);
////////////
////////////ExprAssign* temp_b_left = dynamic_cast<ExprAssign*>(BB[0].m_left);
////////////ExprData* temp_X=AA[0].m_pED;//получили X[]
////////////int old_left_bound=ExprManagement::calcIntValue(&temp_b_left->getRightExpr());
////////////int l_bound=old_left_bound+size_of_block;
//////////////double *X=new double[size_of_X];

//////////////теперь находим столбец свободных членов
//////////////double *h=new double[size_of_X];
//////////////	for(i=0; i<size_of_block; ++i)
//////////////		{h[i]=X[old_left_bound-size_of_block+i];}
//////////////
//////////////	for(i=size_of_block; i<size_of_X; ++i)
//////////////		{h[i]=0;}
//////////////
//////////////	for(i=size_of_block; i<2*size_of_block; ++i)
//////////////		for(j=0; j<size_of_block; ++j)
//////////////			{h[i]-=h[j]*Block1[i-size_of_block][j];}
////////////
////////////	Block *pBlock=pFor->getParentBlock();
////////////	Block::Iterator pF_it = pFor->getThisInBlock();
////////////	ExprImm *temp_imm_sob = ExprImm::create(ImmValue::IV_INT, size_of_block), *temp_imm, *temp_imm_b1, *temp_imm_j;
////////////	ExprBinary *temp_binary, *RightVal;
//////////////	ExprNode *pLeftVal, *pRightVal;
//////////////вставляем в цикл size_of_block штук операторов присваивания вида:
//////////////x[j]=-b[size_of_block-1][j]*x[j-size_of_block]-...-b[0][j]*x[j-1]
////////////	for(j=0; j<size_of_block; ++j)
////////////	{
////////////		////////////////получаем x[j] - левая часть
////////////		temp_imm_j = ExprImm::create(ImmValue::IV_INT, l_bound-size_of_block+j);//получили l_bound-size_of_block
////////////		// Левая часть вставляемого оператора присваивания
////////////		ExprNode* pLeftVal(ExprArray::create(temp_X->getData(),temp_imm_j));//получили X[l_bound-size_of_block]
////////////
////////////		//////////////получаем правую чать оператора присваивания
////////////		//-b[size_of_block-1][j]*x[j-size_of_block]-...-b[0][j]*x[j-1]
////////////		temp_imm_b1 = ExprImm::create(ImmValue::IV_DOUBLE, -Block1[j][0]);//получили -b[size_of_block-1][0]
////////////		temp_imm = ExprImm::create(ImmValue::IV_INT, l_bound-2*size_of_block);//получили l_bound-2*size_of_block
////////////		RightVal = ExprBinary::create(ExprOper::OT_MUL,temp_imm_b1,ExprArray::create(temp_X->getData(), temp_imm));//-b[size_of_block-1][0]*x[j-size_of_block]
////////////		for(int k=size_of_block-1; k>=1; --k)
////////////		{
////////////			temp_imm_b1 = ExprImm::create(ImmValue::IV_DOUBLE, -Block1[j][k]);//получили -b[k][j]
////////////			temp_imm = ExprImm::create(ImmValue::IV_INT, l_bound-size_of_block-k);//получили j-k
////////////			temp_binary = ExprBinary::create(ExprOper::OT_MUL,temp_imm_b1,ExprArray::create(temp_X->getData(), temp_imm));//b1[k][j]*X[j-k]
////////////			RightVal = ExprBinary::create(ExprOper::OT_PLUS,RightVal,temp_binary);//получили -b[size_of_block-1][j]*x[j-size_of_block]-...-b[0][j]*x[j-1]
////////////		}
////////////		//правая часть оператора присваивания
////////////		ExprNode* pRightVal(RightVal);//получили -b[size_of_block-1][j]*x[j-size_of_block]-...-b[0][j]*x[j-1]
////////////		// Добавляем оператор присваивания
////////////
////////////		pBlock->insertBefore(pF_it,StmtExpr::create(ExprAssign::create(pLeftVal,pRightVal)));
//////////////		pBlock->addFront(StmtExpr::create(ExprAssign::create(pLeftVal,pRightVal)));
////////////	}
//////////////вычитаем каждую строку умноженную на Block из последующей строки
////////////	MultiplyMatrix(size_of_block, Block1, Block1);
////////////	for(i=0; i<size_of_block; i++)
////////////	{
////////////		for(j=0; j<size_of_block; j++)
////////////			{Block1[i][j]=-Block1[i][j];}
////////////	}
////////////
////////////
//////////////вставляем в цикл size_of_block штук операторов присваивания вида:
//////////////x[i]=b[j][0]*x[i-size_of_block]+...+b[j][size_of_block-1]*x[i-1]
////////////	for(j=size_of_block-1; j>=0; --j)
////////////	{
////////////		//////////////получаем x[i+j] - левая часть
////////////		temp_imm_j = ExprImm::create(ImmValue::IV_INT, j+size_of_block);//получили j+size_of_block
////////////		temp_binary = ExprBinary::create(ExprOper::OT_PLUS, loop_counter->clone(), temp_imm_j);//получили i+j+size_of_block
////////////		// Левая часть вставляемого оператора присваивания
////////////		ExprNode *pLeftVal( ExprArray::create(temp_X->getData(), temp_binary) );//получили X[i+j+size_of_block]
////////////		//////////////получаем правую чать оператора присваивания
////////////		//b[j][0]*x[i-size_of_block-1]+...+b[j][size_of_block-1]*x[i-2*size_of_block]
////////////		temp_imm_b1 = ExprImm::create(ImmValue::IV_DOUBLE, Block1[j][0]);//получили b[j][size_of_block-1]
////////////		temp_imm = ExprImm::create(ImmValue::IV_INT, size_of_block);//получили size_of_block
////////////		temp_binary = ExprBinary::create(ExprOper::OT_MINUS,loop_counter->clone(),temp_imm);//i-size_of_block
////////////		RightVal = ExprBinary::create(ExprOper::OT_MUL,temp_imm_b1,ExprArray::create(temp_X->getData(), temp_binary));//b[0][size_of_block-1]*X[i-size_of_block]
////////////		for(int k=size_of_block-2; k>=0; --k)
////////////		{
////////////			temp_imm_b1 = ExprImm::create(ImmValue::IV_DOUBLE, Block1[j][size_of_block-k-1]);//получили b[j][k]
////////////			temp_imm = ExprImm::create(ImmValue::IV_INT, k+1);//получили k+1
////////////			temp_binary = ExprBinary::create(ExprOper::OT_MINUS,loop_counter->clone(),temp_imm);//i+k+1
////////////			ExprBinary* RightVal2 = ExprBinary::create(ExprOper::OT_MUL,temp_imm_b1,ExprArray::create(temp_X->getData(), temp_binary));//X[i-size_of_block-(k+1)]
////////////			RightVal = ExprBinary::create(ExprOper::OT_PLUS,RightVal2,RightVal);//получили b[0][k]*x[i+k+1]
////////////		}
////////////		//правая часть оператора присваивания
////////////		ExprNode *pRightVal(RightVal);//получили b[j][0]*x[i+j]+...+b[j][size_of_block-1]*x[i+j+size_of_block-1]
////////////		// Добавляем оператор присваивания
////////////		rBlock.addFront(StmtExpr::create(ExprAssign::create(pLeftVal,pRightVal)));
////////////	}
////////////	for(j=size_of_block-1; j>=1; --j)
////////////	{
////////////		//////////////получаем x[i+j] - левая часть
////////////		temp_imm_j = ExprImm::create(ImmValue::IV_INT, j);//получили j
////////////		temp_binary = ExprBinary::create(ExprOper::OT_PLUS, loop_counter->clone(), temp_imm_j);//получили i+j
////////////		// Левая часть вставляемого оператора присваивания
////////////		ExprNode *pLeftVal( ExprArray::create(temp_X->getData(), temp_binary) );//получили X[i+j]
////////////		//////////////получаем правую чать оператора присваивания
//////////////b[j][0]*x[i-size_of_block-1]+...+b[j][size_of_block-1]*x[i-2*size_of_block]
////////////		temp_imm_b1 = ExprImm::create(ImmValue::IV_DOUBLE, Block1[j][0]);//получили b[j][size_of_block-1]
////////////		temp_imm = ExprImm::create(ImmValue::IV_INT, 2*size_of_block);//получили 2*size_of_block
////////////		temp_binary = ExprBinary::create(ExprOper::OT_MINUS,loop_counter->clone(),temp_imm);//i-2*size_of_block
////////////		RightVal = ExprBinary::create(ExprOper::OT_MUL,temp_imm_b1,ExprArray::create(temp_X->getData(), temp_binary));//b[0][size_of_block-1]*X[i-2*size_of_block]
////////////		for(int k=size_of_block-2; k>=0; --k)
////////////		{
////////////			temp_imm_b1 = ExprImm::create(ImmValue::IV_DOUBLE, Block1[j][size_of_block-k-1]);//получили b[j][k]
////////////			temp_imm = ExprImm::create(ImmValue::IV_INT, size_of_block+k+1);//получили size_of_block+(k+1)
////////////			temp_binary = ExprBinary::create(ExprOper::OT_MINUS,loop_counter->clone(),temp_imm);//i-size_of_block-(k+1)
////////////			ExprBinary* RightVal2 = ExprBinary::create(ExprOper::OT_MUL,temp_imm_b1,ExprArray::create(temp_X->getData(), temp_binary));//X[i-size_of_block-(k+1)]
////////////			RightVal = ExprBinary::create(ExprOper::OT_PLUS,RightVal2,RightVal);//получили b[0][k]*x[i-size_of_block-(k+1)]
////////////		}
////////////		//правая часть оператора присваивания
////////////		ExprNode *pRightVal(RightVal);//получили b[j][0]*x[i+j]+...+b[j][size_of_block-1]*x[i+j+size_of_block-1]
////////////		// Добавляем оператор присваивания
////////////		rBlock.addFront(StmtExpr::create(ExprAssign::create(pLeftVal,pRightVal)));
////////////	}
////////////	//////////////получаем левую часть оператора присваивания x[i]
////////////	ExprNode *pLeftVal( ExprArray::create(temp_X->getData(), loop_counter->clone()) );//получили X[i]
////////////	//////////////получаем правую чать оператора присваивания b[0][0]*x[i-size_of_block-1]+...+b[0][size_of_block-1]*x[i-2*size_of_block]
////////////	temp_imm_b1 = ExprImm::create(ImmValue::IV_DOUBLE, Block1[0][0]);//получили b[0][0]
////////////	temp_imm = ExprImm::create(ImmValue::IV_INT, 2*size_of_block);//получили 2*size_of_block
////////////	temp_binary = ExprBinary::create(ExprOper::OT_MINUS,loop_counter->clone(),temp_imm);//i-2*size_of_block
////////////	RightVal = ExprBinary::create(ExprOper::OT_MUL,temp_imm_b1,ExprArray::create(temp_X->getData(), temp_binary));//b[0][size_of_block-1]*X[i-2*size_of_block]
////////////	for(int k=size_of_block-2; k>=0; --k)
////////////	{
////////////		temp_imm_b1 = ExprImm::create(ImmValue::IV_DOUBLE, Block1[0][size_of_block-k-1]);//получили b[0][k]
////////////		temp_imm = ExprImm::create(ImmValue::IV_INT, size_of_block+k+1);//получили size_of_block+(k+1)
////////////		temp_binary = ExprBinary::create(ExprOper::OT_MINUS,loop_counter->clone(),temp_imm);//i-size_of_block-(k+1)
////////////		ExprBinary* RightVal2 = ExprBinary::create(ExprOper::OT_MUL,temp_imm_b1,ExprArray::create(temp_X->getData(), temp_binary));//X[i-size_of_block-(k+1)]
////////////		RightVal = ExprBinary::create(ExprOper::OT_PLUS,RightVal2,RightVal);//получили b[0][k]*x[i-size_of_block-(k+1)]
////////////	}
////////////	//правая часть оператора присваивания
////////////	ExprNode *pRightVal(RightVal);//получили b[0][0]*x[i-1]+...+b[0][size_of_block-1]*x[i-size_of_block]
////////////	// Добавляем оператор присваивания
////////////	rBlock.addFront(StmtExpr::create(ExprAssign::create(pLeftVal,pRightVal)));
////////////
////////////
////////////	//удаляем последний оператор присваивания (тот который пришел на преобразование)
////////////	rBlock.erase(rBlock.getLast());
////////////
////////////	//формируем левую границу результирующего цикла
////////////	ExprImm* temp_imm_left = ExprImm::create(ImmValue::IV_INT, l_bound);//получили l_bound
////////////	temp_binary = ExprBinary::create(ExprOper::OT_ASSIGN,loop_counter->clone(),temp_imm_left);//получили i=l_bound+size_of_block
////////////	pFor->setInit(temp_binary);
////////////	//формируем шаг результирующего цикла
////////////	temp_imm = ExprImm::create(ImmValue::IV_INT, 2*size_of_block);
////////////	temp_binary = ExprBinary::create(ExprOper::OT_PLUS,loop_counter->clone(),temp_imm);//получили i=l_bound+2*size_of_block
////////////	temp_binary = ExprBinary::create(ExprOper::OT_ASSIGN,loop_counter->clone(),temp_binary);//получили i=l_bound+size_of_block
//////////////	ExprImm* temp_imm_step = ExprImm::create(ImmValue::IV_INT, size_of_block);//получили l_bound
////////////	pFor->setIncr(temp_binary);
////////////
////////////
////////////
////////////
////////////
////////////
////////////
////////////
//////////////группировка по полученым данным новых двух циклов
//////////////
//////////////	for(i=0; i<size_of_block; ++i)
//////////////	{
//////////////		x[i] - левая часть
//////////////		h[i] - правая часть
//////////////	}
//////////////
//////////////	for(i=0; i<(size_of_X/size_of_block)-1; ++i)
//////////////	{
//////////////		for(j=0; j<size_of_block; ++j)
//////////////		{
//////////////			x[(i+2)*size_of_block+j] - левая часть
//////////////			for(k=0; size_of_block; ++k)
//////////////				{a[k]*x[i*size_of_block+j+k]} - правая часть
//////////////		}
//////////////	}
//////////////		for(j=3; j<50; j=j+2)
//////////////		{
//////////////			x[i]=b11*x[i-2*s_o_b]+b12*x[i-2*s_o_b+1]
//////////////			x[i+1]=b21*x[i-2*s_o_b]+b22*x[i-2*s_o_b+1]
//////////////		}
//////////////
//////////////		x[3]=
//////////////		x[4]=
//////////////		for(j=3+s_o_b; j<50; j=j+2)
//////////////		{
//////////////			x[i]=b11*x[i-2*s_o_b]+b12*x[i-2*s_o_b+1]
//////////////			x[i+1]=b21*x[i-2*s_o_b]+b22*x[i-2*s_o_b+1]
//////////////		}
//////////////
//////////////
////////////	for (i=0; i<size_of_block; i++)
////////////	{
////////////		delete[] Block1[i];
////////////		delete[] Block2[i];
////////////		delete[] Block2_invert[i];
////////////	}
////////////	delete[] Block1;
////////////	delete[] Block2;
////////////	delete[] Block2_invert;
////////////	delete[] A;
////////////	delete[] e;
////////////
////////////
//////////////Получаем по ней блочную двухдиагональную матрицу(притом блоки будут размером m*m)
//////////////Куда девать излишки??????? то есть правый край. Крайний правый блок скорее всего не будет размером m*m, равно как и нижняя граница
//////////////снизу равно как и справа останется (m+N) mod m ,где m - длина реккурентности (то есть номер минимального x в правой части), а N - количество итераций цикла
//////////////делаем структуру представляющую из себя блок. Заполняем матрицу блоков размера ((m+N) div m)*((m+N) div m),
//////////////а точнее только ту ее часть которая не является нулевой - две диагонали
////////////}

int isMakeRLTPossible(ForStatement* pFor)
{
	//не пустой ли оператор передан?
	OPS_ASSERT(pFor != 0);

	//в теле цикла присутствует только один оператор присваивания?
	int t_count = 0;
	for (BlockStatement::Iterator iter = pFor->getBody().getFirst(); iter.isValid(); ++iter)
	{
///////////////////////////удаляем пустые операторы
		while(iter->is_a<EmptyStatement>())
		{
			BlockStatement::Iterator tempIter = iter++;
			pFor->getBody().erase(tempIter);
			continue;
		}
///////////////////////////////////////////////////

		if(!iter->is_a<ExpressionStatement>())
			return 0;//в теле цикла пресутствует не только оператор присваивания
		if(!iter->cast_to<ExpressionStatement>().get().is_a<BasicCallExpression>())
			return 0;//в теле цикла пресутствует не только оператор присваивания
		if(iter->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getKind() != BasicCallExpression::BCK_ASSIGN)
			return 0;//в теле цикла пресутствует не только оператор присваивания

		t_count++;
		if(t_count > 1)
			return 0;//в теле цикла больше одного оператора присваивания
	}

	ExpressionBase& LeftExpression = pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(0);
	//ReferenceExpression *name_of_x;
	if(LeftExpression.is_a<BasicCallExpression>())
	{//случаи : x[i]=a*x[i]+b, x[i]=a[i]*x[i]+b, x[i]=a*x[i]+b[i], x[i]=a[i]*x[i]+b[i], x[i]=a*x[i-n]+b, x[i]=a[i]*x[i-n]+b, x[i]=a*x[i-n]+b[i], x[i]=a[i]*x[i-n]+b[i],  x[i]=a1[i]*x[i-1]+a2[i]*x[i-2]+...+an[i]*x[i-n]
		//name_of_x = LeftExpression.cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>();
		//for(int i = 0; i < LeftExpression.cast_to<BasicCallExpression>().getArgumentCount(); i++)
		{
			//LeftExpression.cast_to<BasicCallExpression>().getArgument(i+1);
		}
	}
	else
	{//случаи : x=a*x+b, x=a[i]*x+b, x=a*x+b[i], x=a[i]*x+b[i]
		//name_of_x = LeftExpression.cast_to<ReferenceExpression>();
	}

	//if(){}
	//ReferenceExpression* name_of_x = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>());
	//ReferenceExpression* name_of_a = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>());
	//ReferenceExpression* name_of_b = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>());
	return 1;
}

//Случай дробно-линейной рекуррентности вида x[i] = a[i]*x[i-1] + b[i]/c[i]*x[i-1] + d[i]
/////////////////////////////////////////////////////
bool MakeRLT3(ForStatement* pFor, int number_of_proc,  const double Tf, const double Ts)
{
//	if(!isMakeRLTPossible(pFor))
//		return false;

	//вид результирующего фрагмента кода
	//int log2P = округ в большую сторону(log2(P));
	//int deg_of_two = 1;
	//for(i = 0; i < log2P - 1; i++)
	//{
	//	for(s = 0; s < deg_of_two; s++)
	//		for(k = M[i]; k < M[i + 1]; k += deg_of_two)
	//			{x[k] = (a[i][k] * x[k-1] + b[i][k]) / (c[i][k] * x[k-1] + d[i][k]);}
	//	for(l = 0; l < P - deg_of_two; l++)
	//		for(m = M[i + 1]; m < M[i + 2]; m += P - deg_of_two)
	//		{
	//			a[i+1][m+l] = a[i][m+l] * a[i][m-1+l];
	//			b[i+1][m+l] = a[i][m+l] * b[i][m-1+l] + b[i][m+l];
	//			c[i+1][m+l] = c[i][m+l] * a[i][m-1+l];
	//			d[i+1][m+l] = c[i][m+l] * b[i][m-1+l] + d[i][m+l];
	//		}
	//	deg_of_two *= 2;
	//}
	//for(t = 0; t < P; t++)
	//	for(j = M[log2P]; j < M[log2P + 1]; j += P)
	//		{(x[j+t] = a[log2P][j+t] * x[j-1+t] + b[log2P][j+t]) / (c[i][k] * x[k-1] + d[i][k]);}

    IntegerHelper c(BasicType::BT_INT32);
	ForStatement *newFor1, *newFor2;
	ExpressionBase *temp_eb, *newFor_initExpression, *newFor_finalExpression, *newFor_stepExpression;
	StatementBase *temp_sb;
	VariableDeclaration &new_var_sa = Editing::createNewVariable(*BasicType::float64Type(), pFor->getParentBlock()),//new ArrayType(log2P, new ArrayType(N-M[0], BasicType::float64Type())), pFor->getBody());
						&new_var_sb = Editing::createNewVariable(*BasicType::float64Type(), pFor->getParentBlock()),
						&new_var_sc = Editing::createNewVariable(*BasicType::float64Type(), pFor->getParentBlock()),//new ArrayType(log2P, new ArrayType(N-M[0], BasicType::float64Type())), pFor->getBody());
						&new_var_sd = Editing::createNewVariable(*BasicType::float64Type(), pFor->getParentBlock()),
						&new_var_counter = Editing::createNewVariable(*BasicType::int32Type(), pFor->getParentBlock());
	ReferenceExpression *name_of_x = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>()),
						//*name_of_a = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>()),
						//*name_of_b = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>()),
						//*name_of_c = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>()),
						//*name_of_d = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>()),
						*name_of_sa = new ReferenceExpression(new_var_sa),
						*name_of_sb = new ReferenceExpression(new_var_sb),
						*name_of_sc = new ReferenceExpression(new_var_sc),
						*name_of_sd = new ReferenceExpression(new_var_sd),
						*pFor_counter = Editing::getBasicForCounter(*pFor).clone(),
						*new_counter = new ReferenceExpression(new_var_counter);

	//переносит определения переменных из блока подаваемого цикла в родительский его блок
	newVarDecBlock2(&pFor->getBody());

	//вычисление количества этапов алгоритма (log2P можно конечно воспользоваться функцией вычисления двоичного логарифма и округления... но и так временные затраты минимальны)
	//int log2P = округ в большую сторону(log2(P));
	//const int log2P = int(ceil(::log(P) / ::log(2.0)));//надо сделать так чтобы бралось из текста программы
	int log2P;
	int deg_of_two = 1;
	for(log2P = 0; deg_of_two < number_of_proc; log2P++)
		{deg_of_two *= 2;}
//	const double P = deg_of_two / 2;

//вычисление значений массива M[]
////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//const double T = Ts/Tf;
	int *M = new int[log2P+2];//массив границ этапов алгоритма
	int zero = 1;//!!!???номер элелмента с которого начинаются вычисления x[i] (это точно не 0, потому что для вычисления x[0] требуется x[-1], а x[-1] не бывает) (левая граница??? откуда и как его брать?)
	M[log2P+1] = 800;//!!!???номер последнего вычисляемого элелмента x[i] (правая граница??? откуда и как его брать?)
	//int zero = pFor->getInitExpression();//!!!???номер элелмента с которого начинаются вычисления x[i] (это точно не 0, потому что для вычисления x[0] требуется x[-1], а x[-1] не бывает) (левая граница??? откуда и как его брать?)
	//M[log2P+1] = pFor->getFinalExpression();//!!!???номер последнего вычисляемого элелмента x[i] (правая граница??? откуда и как его брать?)
	deg_of_two = 1;
	M[0] = 0;
	for(int i = 1; i < log2P+1; i++)
	{
		M[i] = int(ceil((M[log2P+1]*Ts*deg_of_two + (number_of_proc-deg_of_two)*Tf*M[i-1]) / ((number_of_proc-deg_of_two)*Tf + deg_of_two*Ts)) + zero);
		//M[i] = (M[log2P+1]*T + (number_of_proc-deg_of_two)*M[i-1]) / (number_of_proc-deg_of_two) + deg_of_two*T;
		deg_of_two *= 2;
	}
	M[0] = zero;
////////////////////////////////////////////////////////////////////////////////////////////////////////////



	//VariableDeclaration	&new_var_counter2 = Editing::createNewVariable(*BasicType::int32Type(), pFor->getParentBlock());
	//ReferenceExpression	*new_counter2 = new ReferenceExpression(new_var_counter2);
	////for(m = 0; m < log2P; m += 1)
	//newFor_initExpression = new BasicCallExpression(op(new_counter2) R_AS c(0));
	//newFor_finalExpression = new BasicCallExpression(op(new_counter2) < c(log2P));
	//newFor_stepExpression = new BasicCallExpression(op(new_counter2) R_AS op(new_counter2) + c(1));
	//newFor1 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);
	//pFor->getParentBlock().addBefore(pFor->getParentBlock().convertToIterator(pFor) ,newFor1);
	//newFor1->getBody().addFirst(pFor);

	deg_of_two = 1;
	for(int i = 0; i < log2P; i++)
	{
		//for(s = 0; s < deg_of_two; s += 1)
		newFor_initExpression = new BasicCallExpression(op(new_counter) R_AS c(0));
		newFor_finalExpression = new BasicCallExpression(op(new_counter) < c(deg_of_two));
		newFor_stepExpression = new BasicCallExpression(op(new_counter) R_AS op(new_counter) + c(1));
		newFor1 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);
		pFor->getParentBlock().addBefore(pFor->getParentBlock().convertToIterator(pFor) ,newFor1);

		//for(k = M[i]; k < M[i + 1]; k += deg_of_two)
		newFor_initExpression = new BasicCallExpression(op(pFor_counter) R_AS c(M[i]));
		newFor_finalExpression = new BasicCallExpression(op(pFor_counter) < c(M[i + 1]));
		newFor_stepExpression = new BasicCallExpression(op(pFor_counter) R_AS op(pFor_counter) + c(1));
		newFor2 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);

		//{x[k] = (a[i][k] * x[k-1] + b[i][k]) / (c[i][k] * x[k-1] + d[i][k]);}
		temp_eb = new BasicCallExpression((op(name_of_x) R_BK ((op(pFor_counter) + op(new_counter))) R_AS 
											op(name_of_sa) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) * 
											(op(name_of_x) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) + 
											op(name_of_sb) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter)))) /
											
											(op(name_of_sc) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) * 
											(op(name_of_x) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) + 
											op(name_of_sd) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))))	);
		temp_sb = new ExpressionStatement(temp_eb);
		newFor2->getBody().addLast(temp_sb);

		newFor1->getBody().addFirst(newFor2);


		//for(s = 0; s < P - deg_of_two; s += 1)
		newFor_initExpression = new BasicCallExpression(op(new_counter) R_AS c(0));
		newFor_finalExpression = new BasicCallExpression(op(new_counter) < c(number_of_proc - deg_of_two));
		newFor_stepExpression = new BasicCallExpression(op(new_counter) R_AS op(new_counter) + c(1));
		newFor1 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);
		pFor->getParentBlock().addBefore(pFor->getParentBlock().convertToIterator(pFor) ,newFor1);

		//for(k = M[i + 1]; k < M[log2P+1]; k += 1)
		newFor_initExpression = new BasicCallExpression(op(pFor_counter) R_AS c(M[i + 1]));
		newFor_finalExpression = new BasicCallExpression(op(pFor_counter) < c(M[i+2]));
		newFor_stepExpression = new BasicCallExpression(op(pFor_counter) R_AS op(pFor_counter) + c(1));
		newFor2 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);

		//a[i+1][k+s] = a[i][k+s] * a[i][k-1+s];
		temp_eb = new BasicCallExpression(op(name_of_sa) R_BK (c(i+1)) R_BK ((op(pFor_counter) + op(new_counter))) R_AS
											op(name_of_sa) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) *
											(op(name_of_sa) R_BK (c(i)) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) );
		temp_sb = new ExpressionStatement(temp_eb);
		newFor2->getBody().addLast(temp_sb);

		//b[i+1][k+s] = a[i][k+s] * b[i][k-1+s] + b[i][k+s];
		temp_eb = new BasicCallExpression(op(name_of_sb) R_BK (c(i+1)) R_BK ((op(pFor_counter) + op(new_counter))) R_AS
											op(name_of_sa) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) *
											(op(name_of_sb) R_BK (c(i)) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) +
											op(name_of_sb) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) );
		temp_sb = new ExpressionStatement(temp_eb);
		newFor2->getBody().addLast(temp_sb);

		//c[i+1][k+s] = c[i][k+s] * a[i][k-1+s];
		temp_eb = new BasicCallExpression(op(name_of_sc) R_BK (c(i+1)) R_BK ((op(pFor_counter) + op(new_counter))) R_AS
											op(name_of_sc) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) *
											(op(name_of_sa) R_BK (c(i)) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) );
		temp_sb = new ExpressionStatement(temp_eb);
		newFor2->getBody().addLast(temp_sb);

		//d[i+1][k+s] = c[i][k+s] * b[i][k-1+s] + d[i][k+s];
		temp_eb = new BasicCallExpression(op(name_of_sd) R_BK (c(i+1)) R_BK ((op(pFor_counter) + op(new_counter))) R_AS
											op(name_of_sc) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) *
											(op(name_of_sb) R_BK (c(i)) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) +
											op(name_of_sd) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) );
		temp_sb = new ExpressionStatement(temp_eb);
		newFor2->getBody().addLast(temp_sb);

		newFor1->getBody().addFirst(newFor2);


		deg_of_two *= 2;
	}

	//for(m = 0; m < number_of_proc; m += 1)
	newFor_initExpression = new BasicCallExpression(op(new_counter) R_AS c(0));
	newFor_finalExpression = new BasicCallExpression(op(new_counter) < c(number_of_proc));
	newFor_stepExpression = new BasicCallExpression(op(new_counter) R_AS op(new_counter) + c(1));
	newFor1 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);
	pFor->getParentBlock().addBefore(pFor->getParentBlock().convertToIterator(pFor) ,newFor1);

	//for(k = M[log2P]; k < M[log2P+1]; k += 1)
	newFor_initExpression = new BasicCallExpression(op(pFor_counter) R_AS c(M[log2P]));
	newFor_finalExpression = new BasicCallExpression(op(pFor_counter) < c(M[log2P+1]));
	newFor_stepExpression = new BasicCallExpression(op(pFor_counter) R_AS op(pFor_counter) + c(1));
	newFor2 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);

	//{x[k+l] = a[i][k+l] * x[k-1+l] + b[i][k+l];}
	temp_eb = new BasicCallExpression(op(name_of_x) R_BK ((op(pFor_counter) + op(new_counter))) R_AS
										op(name_of_sa) R_BK (c(log2P)) R_BK ((op(pFor_counter) + op(new_counter)))
										* (op(name_of_x) R_BK ((op(pFor_counter) - c(1) + op(new_counter))))
										+ op(name_of_sb) R_BK (c(log2P)) R_BK ((op(pFor_counter) + op(new_counter))) /
											
										(op(name_of_sc) R_BK (c(log2P)) R_BK ((op(pFor_counter) + op(new_counter))) * 
										(op(name_of_x) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) + 
										op(name_of_sd) R_BK (c(log2P)) R_BK ((op(pFor_counter) + op(new_counter))))	);
	temp_sb = new ExpressionStatement(temp_eb);
	newFor2->getBody().addLast(temp_sb);

	newFor1->getBody().addFirst(newFor2);

//////удаляем исходный цикл
	BlockStatement::Iterator it = pFor->getParentBlock().convertToIterator(pFor);
	pFor->getParentBlock().erase(it);

	delete M;

	return true;	
}

//Случай линейной рекуррентности вида x[i] = a[i]*x[i-1] + b[i]
/////////////////////////////////////////////////////
bool MakeRLT2(ForStatement* pFor, int number_of_proc, const double Tf, const double Ts)
{
//	if(!isMakeRTL2Possible(pFor))
//		return false;

	//вид результирующего фрагмента кода
	//int log2P = округ в большую сторону(log2(P));
	//int deg_of_two = 1;
	//for(i = 0; i < log2P - 1; i++)
	//{
	//	for(k = M[i]; k < M[i + 1]; k += deg_of_two)
	//		for(s = 0; s < deg_of_two; s++)
	//			{x[k] = a[i][k] * x[k-1] + b[i][k];}
	//	for(m = M[i + 1]; m < M[i + 2]; m += P - deg_of_two)
	//	{
	//		for(l = 0; l < P - deg_of_two; l++)
	//		{
	//			a[i+1][m+l] = a[i][m+l] * a[i][m-1+l];
	//			b[i+1][m+l] = a[i][m+l] * b[i][m-1+l] + b[i][m+l];
	//		}
	//	}
	//	deg_of_two *= 2;
	//}
	//for(j = M[log2P]; j < M[log2P + 1]; j += P)
	//	for(t = 0; t < P; t++)
	//		{x[j+t] = a[log2P][j+t] * x[j-1+t] + b[log2P][j+t];}

    IntegerHelper c(BasicType::BT_INT32);
	ForStatement *newFor1, *newFor2;
	ExpressionBase *temp_eb, *newFor_initExpression, *newFor_finalExpression, *newFor_stepExpression;
	StatementBase *temp_sb;
	//В текст программы добавляется описание новых массивов new_var_sa, new_var_sb и счетчика цикла new_var_counter
	VariableDeclaration &new_var_sa = Editing::createNewVariable(*BasicType::float64Type(), pFor->getParentBlock()),//new ArrayType(log2P, new ArrayType(N-M[0], BasicType::float64Type())), pFor->getBody());
						&new_var_sb = Editing::createNewVariable(*BasicType::float64Type(), pFor->getParentBlock()),
						&new_var_counter = Editing::createNewVariable(*BasicType::int32Type(), pFor->getParentBlock());
	ReferenceExpression *name_of_x = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>()),
						//*name_of_a = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>()),
						//*name_of_b = &(pFor->getBody().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>()),
						*name_of_sa = new ReferenceExpression(new_var_sa),
						*name_of_sb = new ReferenceExpression(new_var_sb),
						*pFor_counter = Editing::getBasicForCounter(*pFor).clone(),
						*new_counter = new ReferenceExpression(new_var_counter);
	
	//переносит определения переменных из блока подаваемого цикла в родительский его блок
	newVarDecBlock2(&pFor->getBody());

	//вычисление количества этапов алгоритма (log2P можно конечно воспользоваться функцией вычисления двоичного логарифма и округления... но и так временные затраты минимальны)
	//int log2P = округ в большую сторону(log2(P));
	//const int log2P = int(ceil(::log(P) / ::log(2.0)));//надо сделать так чтобы бралось из текста программы
	int log2P;
	int deg_of_two = 1;
	for(log2P = 0; deg_of_two < number_of_proc; log2P++)
		{deg_of_two *= 2;}
//	const double P = deg_of_two / 2;

//вычисление значений массива M[]
////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//const double T = Ts/Tf;
	int *M = new int[log2P+2];//массив границ этапов алгоритма
	int zero = 1;//!!!???номер элелмента с которого начинаются вычисления x[i] (это точно не 0, потому что для вычисления x[0] требуется x[-1], а x[-1] не бывает) (левая граница??? откуда и как его брать?)
	M[log2P+1] = 800;//!!!???номер последнего вычисляемого элелмента x[i] (правая граница??? откуда и как его брать?)
	//int zero = pFor->getInitExpression();//!!!???номер элелмента с которого начинаются вычисления x[i] (это точно не 0, потому что для вычисления x[0] требуется x[-1], а x[-1] не бывает) (левая граница??? откуда и как его брать?)
	//M[log2P+1] = pFor->getFinalExpression();//!!!???номер последнего вычисляемого элелмента x[i] (правая граница??? откуда и как его брать?)
	deg_of_two = 1;
	M[0] = 0;
	for(int i = 1; i < log2P+1; i++)
	{
		M[i] = int(ceil((M[log2P+1]*Ts*deg_of_two + (number_of_proc-deg_of_two)*Tf*M[i-1]) / ((number_of_proc-deg_of_two)*Tf + deg_of_two*Ts)) + zero);
		//M[i] = (M[log2P+1]*T + (number_of_proc-deg_of_two)*M[i-1]) / (number_of_proc-deg_of_two) + deg_of_two*T;
		deg_of_two *= 2;
	}
	M[0] = zero;
////////////////////////////////////////////////////////////////////////////////////////////////////////////



	//VariableDeclaration	&new_var_counter2 = Editing::createNewVariable(*BasicType::int32Type(), pFor->getParentBlock());
	//ReferenceExpression	*new_counter2 = new ReferenceExpression(new_var_counter2);
	////for(m = 0; m < log2P; m += 1)
	//newFor_initExpression = new BasicCallExpression(op(new_counter2) R_AS c(0));
	//newFor_finalExpression = new BasicCallExpression(op(new_counter2) < c(log2P));
	//newFor_stepExpression = new BasicCallExpression(op(new_counter2) R_AS op(new_counter2) + c(1));
	//newFor1 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);
	//pFor->getParentBlock().addBefore(pFor->getParentBlock().convertToIterator(pFor) ,newFor1);
	//newFor1->getBody().addFirst(pFor);

	deg_of_two = 1;
	for(int i = 0; i < log2P; i++)
	{
		//for(m = 0; m < deg_of_two; m += 1)
		newFor_initExpression = new BasicCallExpression(op(new_counter) R_AS c(0));
		newFor_finalExpression = new BasicCallExpression(op(new_counter) < c(deg_of_two));
		newFor_stepExpression = new BasicCallExpression(op(new_counter) R_AS op(new_counter) + c(1));
		newFor1 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);
		pFor->getParentBlock().addBefore(pFor->getParentBlock().convertToIterator(pFor) ,newFor1);

		//for(k = M[i]; k < M[i + 1]; k += deg_of_two)
		newFor_initExpression = new BasicCallExpression(op(pFor_counter) R_AS c(M[i]));
		newFor_finalExpression = new BasicCallExpression(op(pFor_counter) < c(M[i + 1]));
		newFor_stepExpression = new BasicCallExpression(op(pFor_counter) R_AS op(pFor_counter) + c(1));
		newFor2 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);

		//{x[k+l] = a[i][k+l] * x[k-1+l] + b[i][k+l];}
		temp_eb = new BasicCallExpression(op(name_of_x) R_BK ((op(pFor_counter) + op(new_counter))) R_AS 
											op(name_of_sa) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) * 
											(op(name_of_x) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) + 
											op(name_of_sb) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) );
		temp_sb = new ExpressionStatement(temp_eb);
		newFor2->getBody().addLast(temp_sb);

		newFor1->getBody().addFirst(newFor2);


		//for(m = 0; m < P - deg_of_two; m += 1)
		newFor_initExpression = new BasicCallExpression(op(new_counter) R_AS c(0));
		newFor_finalExpression = new BasicCallExpression(op(new_counter) < c(number_of_proc - deg_of_two));
		newFor_stepExpression = new BasicCallExpression(op(new_counter) R_AS op(new_counter) + c(1));
		newFor1 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);
		pFor->getParentBlock().addBefore(pFor->getParentBlock().convertToIterator(pFor) ,newFor1);

		//for(m = M[i + 1]; m < M[log2P+1]; m += 1)
		newFor_initExpression = new BasicCallExpression(op(pFor_counter) R_AS c(M[i + 1]));
		newFor_finalExpression = new BasicCallExpression(op(pFor_counter) < c(M[i+2]));
		newFor_stepExpression = new BasicCallExpression(op(pFor_counter) R_AS op(pFor_counter) + c(1));
		newFor2 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);

		//a[i+1][m+l] = a[i][m+l] * a[i][m-1+l];
		temp_eb = new BasicCallExpression(op(name_of_sa) R_BK (c(i+1)) R_BK ((op(pFor_counter) + op(new_counter))) R_AS
											op(name_of_sa) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) *
											(op(name_of_sa) R_BK (c(i)) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) );
		temp_sb = new ExpressionStatement(temp_eb);
		newFor2->getBody().addLast(temp_sb);

		//b[i+1][m+l] = a[i][m+l] * b[i][m-1+l] + b[i][m+l];
		temp_eb = new BasicCallExpression(op(name_of_sb) R_BK (c(i+1)) R_BK ((op(pFor_counter) + op(new_counter))) R_AS
											op(name_of_sa) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) *
											(op(name_of_sb) R_BK (c(i)) R_BK ((op(pFor_counter) - c(1) + op(new_counter)))) +
											op(name_of_sb) R_BK (c(i)) R_BK ((op(pFor_counter) + op(new_counter))) );
		temp_sb = new ExpressionStatement(temp_eb);
		newFor2->getBody().addLast(temp_sb);

		newFor1->getBody().addFirst(newFor2);


		deg_of_two *= 2;
	}

	//for(m = 0; m < number_of_proc; m += 1)
	newFor_initExpression = new BasicCallExpression(op(new_counter) R_AS c(0));
	newFor_finalExpression = new BasicCallExpression(op(new_counter) < c(number_of_proc));
	newFor_stepExpression = new BasicCallExpression(op(new_counter) R_AS op(new_counter) + c(1));
	newFor1 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);
	pFor->getParentBlock().addBefore(pFor->getParentBlock().convertToIterator(pFor) ,newFor1);

	//for(k = M[log2P]; k < M[log2P+1]; k += 1)
	newFor_initExpression = new BasicCallExpression(op(pFor_counter) R_AS c(M[log2P]));
	newFor_finalExpression = new BasicCallExpression(op(pFor_counter) < c(M[log2P+1]));
	newFor_stepExpression = new BasicCallExpression(op(pFor_counter) R_AS op(pFor_counter) + c(1));
	newFor2 = new ForStatement(newFor_initExpression, newFor_finalExpression, newFor_stepExpression);

	//{x[k+l] = a[i][k+l] * x[k-1+l] + b[i][k+l];}
	temp_eb = new BasicCallExpression(op(name_of_x) R_BK ((op(pFor_counter) + op(new_counter))) R_AS
										op(name_of_sa) R_BK (c(log2P)) R_BK ((op(pFor_counter) + op(new_counter)))
										* (op(name_of_x) R_BK ((op(pFor_counter) - c(1) + op(new_counter))))
										+ op(name_of_sb) R_BK (c(log2P)) R_BK ((op(pFor_counter) + op(new_counter))) );
	temp_sb = new ExpressionStatement(temp_eb);
	newFor2->getBody().addLast(temp_sb);

	newFor1->getBody().addFirst(newFor2);

//////удаляем исходный цикл
	BlockStatement::Iterator it = pFor->getParentBlock().convertToIterator(pFor);
	pFor->getParentBlock().erase(it);

	delete M;

	return true;
}

//Случай линейной рекуррентности вида X(k)=a1*X(k-1)+a2*X(k-2)+…+am*X(k-m)
bool MakeRLT4(ForStatement* pFor, int number_of_proc)
{
//////вспомогательные построения и преобразования
	/////проверки
	//не пустой ли оператор цикла передан?
	OPS_ASSERT(pFor != 0);

	if(!Editing::forHeaderIsCanonized(*pFor))
		return false;//цикл не канонизирован

	//в теле цикла только один оператор, который является оператором присваивания
	int t_count = 0;
	for (BlockStatement::Iterator iter = pFor->getBody().getFirst(); iter.isValid(); ++iter)
	{
		t_count++;
		if(!iter->is_a<ExpressionStatement>())
			return false;//в теле цикла пресутствуют не только операторы присваивания
		if(!iter->cast_to<ExpressionStatement>().get().is_a<BasicCallExpression>())
			return false;//в теле цикла пресутствуют не только операторы присваивания
		if(iter->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getKind() != BasicCallExpression::BCK_ASSIGN)
			return false;//в теле цикла пресутствуют не только операторы присваивания
		if(t_count != 1)
			return false;//в теле цикла более одного оператора присваивания
	}

//К этому моменту мы уже определили что у нас в цикле только 1 оператор, который является оператором присваивания!
	//этот оператор присваивания должен быть следующего вида: X(k)=a(k)1*X(k-1)+a(k)2*X(k-2)+…+a(k)m*X(k-m).

	BlockStatement &pBlock = pFor->getParentBlock();
	ReferenceExpression* name_of_X2 = &(pBlock.getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>());

	BasicCallExpression* OccurenceExpression = &(pBlock.getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>());
	LinearExpressionMatrix* LEM = new LinearExpressionMatrix(OccurenceExpression);




		//while((temp_bce->getParent() != NULL) && (temp_bce->getParent()->is_a<BasicCallExpression>()))
		//	if(temp_bce->getParent()->cast_to<BasicCallExpression>().getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
		//		temp_bce = &(temp_bce->getParent()->cast_to<BasicCallExpression>());
		//	else
		//		break;


			//for (int i=0; i<LEM->getArrayDimensionsCount(); ++i)
			//{
			//	for (int j=0; j<LEM->getLoopNestDepth(); ++j)
			//	{
			//		if ((*LEM)[i].isEvaluatable())
			//			cout << (*LEM)[i].getCoefficientAsInteger(LoopCounters[j]) << " ";
			//		else
		  	//			cout << (*LEM)[i][j]->dumpState() << " ";
			//	}

			//	if ((*LEM)[i].isEvaluatable())
			//		cout << (*LEM)[i].getFreeCoefficientAsInteger() << " ";
			//	else
			//		cout << (*LEM)[i][LEM->getLoopNestDepth()]->dumpState() << " ";
			//	cout << endl;
			//}
			delete LEM;




//?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
//????Мы узнали что этот оператор имеет требующийся нам вид X(k)=a(k)1*X(k-1)+a(k)2*X(k-2)+…+a(k)m*X(k-m)
//?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????

int l_bound = 2;
int r_bound = 800;
int size_of_block = 2; //размер блока строящейся блочной двухдиагональной матрицы



//Получаем коэффециенты при x
//////////	std::vector<OccurrenceInfo> AA;
//////////	std::vector<LoopInfo> BB;
//////////	my_id.getAllVariablesList(AA,1);
//////////	my_id.getOuterLoopsList(BB,1);
//////////	ExprNode* loop_counter = BB[0].pthis->clone();
//////////
//	int i=0, j=0, k=0;
//////////	std::vector<ExprNode*> t_X;
//////////	std::vector<OccurrenceInfo>::iterator it = AA.begin();
//////////	NameIter name_of_X=(*it).m_oDataObject;
//////////	OccurrenceInfo gen_X=*it;
//////////	ExprNode* x_coef;
//////////
//////////
//////////
//////////	it++;
//////////	for (; it != AA.end(); ++it)
//////////	{
////////////		for(i=0;i<gen_X.m_dim;++i)
////////////		{
////////////		if((*it).m_data[i]-gen_X.m_data[i]==0)
////////////			{
////////////				gh
////////////			}
////////////			else
////////////			{}
////////////		}
//////////
//////////		OccurrenceInfo r = *it;
//////////		if(name_of_X == r.m_oDataObject)
//////////		{
//////////			t_X.push_back(&r.m_pED->getParentExpr());
//////////			i++;
//////////		}
//////////	}//поместил в вектор t_X указатели на все места где встречается массив X
//	int size_of_A=i=2;//со свободным членом будет ровно i
//////////	int size_of_X;
//////////	ExprBinary* temp_b = dynamic_cast<ExprBinary*>(BB[0].m_right);
////////////	BB[0].m_right->C_Out(cout);
//////////	if(ExprManagement::checkCalculability(&temp_b->getRightArg()))
//////////		{size_of_X=ExprManagement::calcIntValue(&temp_b->getRightArg());}//количество элл. X участвующих в расчетах. = количеству коэфф a[i] + количество итераций цикла
//////////	else
//////////		{
//////////			throw OPS::RuntimeError("Итератор преобразуемого цикла не является счетаемым");
//////////			//is_transform_possible=8;
//////////		}
////////////	BB[0].m_left->C_Out(cout);
//////////
//////////	ExprNode * pExpr;
//////////	Statement * pSt;
//////////	pSt = my_id.getThisOper();
//////////	pExpr = pSt->getAsExpr()->getExpr();
//////////	ExprAssign * pEA = dynamic_cast<ExprAssign*>(pExpr);
//////////	ExprNode *pSource =  pEA->unsafeGetRightExpr();
//////////
//////////	ExprNode* t_fcoef=getFreeCoef(pSource, t_X);
//////////	ExprNode* t_coef;
//////////	double td_fcoef;
//////////	if(ExprManagement::checkCalculability(t_fcoef))//если свободный член вычислим, то он записывается в td_fcoef
//////////		{td_fcoef=ExprManagement::calcDoubleValue(t_fcoef);}
//////////	else
//////////		{
//////////			throw OPS::RuntimeError("Свободный член не является числом");
//////////			//is_transform_possible=4;
//////////		}
//////////	if(t_fcoef==0)
//////////		{
//////////			throw OPS::RuntimeError("Свободный член не равен 0");
//////////			//is_transform_possible=5;
//////////		}
//////////
	double *A = new double[size_of_block];
	double *X = new double[r_bound];
	for(int j = 0; j < size_of_block; j++)
	{
		//////////t_coef=getCoef(pSource, t_X[j]);
		//////////if(ExprManagement::checkCalculability(t_coef))
		//////////	{A[j]=ExprManagement::calcDoubleValue(t_coef);}
		//////////else
		//////////	{
		//////////		throw OPS::RuntimeError("Коэффициент рассматриваемого массива не вычислим");
		//////////		//is_transform_possible=6;
		//////////	}
	}
X[0] = 1;
X[1] = 5;
A[0] = 0.25;
A[1] = 0.25;

//////////
///////////////////////////////////////////////////////
//////////	//находим коэффициенты при массиве X
///////////////////////////////////////////////////////
////////////просматриваем все размерности массива с целью найти рекуррентность
////////////	for()
////////////	{
////////////	}


//////////	//теперь проверяем условие устойчивости (Суховерхова)
//////////	if(rootscan(A, size_of_A))
//////////	{
//////////		throw OPS::RuntimeError("Преобразование не является устойчивым");
//////////	}

//int Num_of_first_X=4; //номер первого вычисляемого в цикле элл. массива X
//int Right_bound_of_X=Num_of_first_X-size_of_block; //номер минимального элл. массива X фигурирующего в вычислениях

//если коэффециенты A НЕ ЗАВИСЯТ ОТ СЧЕТЧИКА ЦИКЛА, то блочная двухдиагональная матрица
//будет содержать всего два различных блока. Обозначим их соответственно Block и Block2, а затем заполним
	double **Block1=new double*[size_of_block];
	double **Block2=new double*[size_of_block];
	for (int i=0; i<size_of_block; i++)
	{
		Block1[i] = new double[size_of_block];
		Block2[i] = new double[size_of_block];
	}

	for(int i = 0; size_of_block > i; ++i)
	{
		for(int j = i+1; size_of_block > j;++j)
		{
			Block1[j][i]=0;
			Block2[i][j]=0;
		}
	}
	for(int i = 0; size_of_block > i; ++i)
	{
		Block1[i][i]=-A[size_of_block-1];
		Block2[i][i]=1;
	}
	for(int i = 0; size_of_block > i; ++i)
	{
		for(int j = i + 1; size_of_block > j; ++j)
		{
			Block1[i][j] =- A[size_of_block - j - i];
			Block2[j][i] =- A[j-i];
		}
	}
//Заполнили блоки блочной двухдиагональной матрицы


//находим обратную матрицу к Block2
	double *e=new double[size_of_block];
	double *res_vector;
	double **Block2_invert=new double*[size_of_block];
	for (int i = 0; i < size_of_block; i++)
		{Block2_invert[i] = new double[size_of_block];}

	for(int i = 0; i<size_of_block; i++)
	{
		for(int j = 0; j<size_of_block; j++)
			e[j] = 0.;
		e[i] = 1.;

		res_vector = gauss_method(size_of_block, Block2, e);
		for(int j = 0; j<size_of_block; j++)
			Block2_invert[j][i] = res_vector[j];
	}
//нашли матрицу обратную к Block2. поместили ее в Block2_invert

//умножим всю матрицу (кроме первой строки) на матрицу обратную к Block2, таким образом получатся блоки I(единичная матрица) по диагонали
//и под ними одинаковые не нулевые блоки (Block*Block2^(-1)), а остальная часть матрицы будет нулевой
	MultiplyMatrix(size_of_block, Block2_invert, Block1);


//double *X=new double[size_of_X];
//теперь находим столбец свободных членов
//double *h=new double[size_of_X];
//	for(i=0; i<size_of_block; ++i)
//		{h[i]=X[old_left_bound-size_of_block+i];}
//
//	for(i=size_of_block; i<size_of_X; ++i)
//		{h[i]=0;}
//
//	for(i=size_of_block; i<2*size_of_block; ++i)
//		for(j=0; j<size_of_block; ++j)
//			{h[i]-=h[j]*Block1[i-size_of_block][j];}
	//находим параметры входящего цикла (инкримент, сдвиг, условие)
	//StatementBase *last_pFor_incr = pFor->get????????
	//////////ExpressionStatement last_pFor_step = pFor->getStepExpression();
	//////////int l_bound = pFor->getInitExpression();//last_pFor_init
	//////////int r_bound = pFor->getFinalExpression();//last_pFor_final

	BlockStatement::Iterator pF_it = pBlock.convertToIterator(pFor);
	StrictLiteralExpression *temp_sle, *temp_sle_b1;//, *temp_sle_sob = StrictLiteralExpression::createInt32(size_of_block);
	std::unique_ptr <BasicCallExpression> temp_bce, temp_bce2, temp_expr, left_part, right_part, temp_result;
	int l, deg_of_two=1;
	//перед результирующим циклом нужно вставить number_of_proc-1 оператора присваивания
	for(int z = 0; z < number_of_proc-1;)
	{
		for(l = 0; l < deg_of_two; ++l, ++z)
		{
		//вставляем перед циклом size_of_block*(number_of_proc-1) штук операторов присваивания вида:
		//x[l_bound+size_of_block*z+j]=-b[size_of_block-1][j]*x[j-size_of_block]-...-b[0][j]*x[j-1]
			for(int j = 0; j<size_of_block; ++j)
			{
				////////////////получаем x[l_bound+size_of_block*z+j] - левая часть
				temp_sle = StrictLiteralExpression::createInt32(l_bound+size_of_block*z+j);//получили l_bound+size_of_block*z+j
				// Левая часть вставляемого оператора присваивания
				left_part.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS,name_of_X2->clone(),temp_sle->clone()));//получили X[l_bound+size_of_block*z+j]

				//////////////получаем правую чать оператора присваивания
				//-b[j][0]*x[j-size_of_block]-...-b[0][j]*x[j-1]
//если вектор h состоитиз чисел, x[i] стоящие перед циклом можно считать так
//double result = 0;
//for(int k = 0; k < size_of_block; ++k)
//{
//	result += -Block1[j][k]*X[l_bound+(z-deg_of_two)*size_of_block+k];
//}//надо еще довычислять новые получающиеся значения x[i]
//

				temp_sle_b1 = StrictLiteralExpression::createFloat64(-Block1[j][0]);//получили -b[j][0]
				temp_sle = StrictLiteralExpression::createInt32(l_bound+(z-deg_of_two)*size_of_block);//получили new_l_bound-2*size_of_block
				temp_bce.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS,name_of_X2->clone(),temp_sle->clone()));
				right_part.reset(new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY, temp_sle_b1->clone() ,temp_bce->clone()));//-b[size_of_block-1][0]*x[j-size_of_block]

				for(int k=size_of_block-1; k>=1; --k)
				{
					temp_sle_b1 = StrictLiteralExpression::createFloat64(-Block1[j][k]);//получили -b[j][k]
					temp_sle = StrictLiteralExpression::createInt32(l_bound+(z-deg_of_two)*size_of_block+k);//получили l_bound+(z-deg_of_two)*size_of_block+k
					temp_bce.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS,name_of_X2->clone(),temp_sle->clone()));
					temp_bce2.reset(new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY, temp_sle_b1->clone() ,temp_bce->clone()));//-b[size_of_block-1][0]*x[j-size_of_block]
					right_part.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS, right_part->clone() ,temp_bce2->clone()));
				}//правая часть оператора присваивания

				// Добавляем оператор присваивания
				//temp_result.reset(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, left_part->clone() ,right_part->clone()));
				ExpressionBase *temp_eb = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, left_part->clone() ,right_part->clone());
				StatementBase *temp_sb = new ExpressionStatement(temp_eb);
				pBlock.addBefore(pF_it, temp_sb);
			}

			//далее по алгоритму вычитаем каждую строку умноженную на Block из последующей строки(НЕ ОБЯЗАТЕЛЬНО НЕПОСРЕДСТВЕННО ПОСЛЕДУЮЩЕЙ)
			//то есть кроме диагонали единичных блоков остается еще диагональ блоков Block1 расположенная через 1 диагональ ниже
			MultiplyMatrix(size_of_block, Block1, Block1);
			for(int i = 0; i < size_of_block; i++)
			{
				for(int j = 0; j < size_of_block; j++)
					{Block1[i][j] =- Block1[i][j];}
			}
		}
		deg_of_two *= 2;
	}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//создается цикл содержащий остаток итераций исходного не попадающий под преобразование
    IntegerHelper c(BasicType::BT_INT32);
	ReferenceExpression* pFor_counter = Editing::getBasicForCounter(*pFor).clone();
//new!!!		int32 fin_expr = Editing::getBasicForFinalExpression(*pFor);
		int new_fin_expr = 800 - 0; //старая граница - остаток от деления на P*s_o_b
// new!!!	if(fin_expr - new_fin_expr)
	{
		ForStatement *newFor = pFor->clone();
		pFor->getParentBlock().addAfter(pFor->getParentBlock().convertToIterator(pFor) ,newFor);
		ExpressionBase& newFor_initExpression = op(pFor_counter) R_AS c(new_fin_expr);
		newFor->setInitExpression(&newFor_initExpression);
	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//меняются начальное условие, конечное условие и шаг входного цикла
	ExpressionBase& initExpression = op(pFor_counter) R_AS c(l_bound + (number_of_proc-1)*size_of_block);
	ExpressionBase& finalExpression = op(pFor_counter) < c(new_fin_expr);
	ExpressionBase& stepExpression = op(pFor_counter) R_AS op(pFor_counter) + c(number_of_proc*size_of_block);
	pFor->setInitExpression(&initExpression);
	pFor->setFinalExpression(&finalExpression);
	pFor->setStepExpression(&stepExpression);
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//создается вложенный цикл
	VariableDeclaration& new_var1 = Editing::createNewVariable(*BasicType::int32Type(), pFor->getBody());
	ReferenceExpression* variable = new ReferenceExpression(new_var1);
	ExpressionBase& initExpression2 = op(variable) R_AS c(0);
	ExpressionBase& finalExpression2 = op(variable) < c(number_of_proc);
	ExpressionBase& stepExpression2 = op(variable) R_AS op(variable) + c(1);
	ForStatement* pFor2 = new ForStatement(&initExpression2, &finalExpression2, &stepExpression2);
	pFor->getBody().addFirst(pFor2);

	for(int temp_i = 0; temp_i < size_of_block; temp_i++)
	{
		////////////////получаем x[i] - левая часть
		// Левая часть вставляемого оператора присваивания
		temp_sle = StrictLiteralExpression::createInt32(number_of_proc);//number_of_proc
		temp_bce.reset(new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY, variable->clone(), temp_sle->clone()));//j*number__of_proc
		temp_expr.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS, pFor_counter->clone(), temp_bce->clone()));//получили i+j*number_of_proc
		temp_sle = StrictLiteralExpression::createInt32(temp_i);//proc_id
		temp_bce.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS, temp_expr->clone(), temp_sle->clone()));//получили i+j*number_of_proc+proc_id
		left_part.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, name_of_X2->clone(),temp_bce->clone()));//получили X[i+j*number_of_proc+proc_id]

		//////////////получаем правую чать оператора присваивания
		//-b[j][0]*x[j-size_of_block]-...-b[0][j]*x[j-1]
		temp_sle_b1 = StrictLiteralExpression::createFloat64(-Block1[temp_i][0]);//получили -Block1[temp_i][0]

		temp_sle = StrictLiteralExpression::createInt32(size_of_block*number_of_proc);//получили size_of_block*number_of_proc
		temp_bce.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS, temp_expr->clone(), temp_sle->clone()));//получили i+j*number_of_proc-size_of_block*number_of_proc
		temp_bce.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, name_of_X2->clone(), temp_bce->clone()));//получили X[i+j*number_of_proc-size_of_block*number_of_proc]

		right_part.reset(new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY, temp_sle_b1->clone() ,temp_bce->clone()));//получили -Block1[temp_i][0]*X[i+j*number_of_proc-size_of_block*number_of_proc]

		for(int k = 1; k < size_of_block; k++)
		{
			temp_sle_b1 = StrictLiteralExpression::createFloat64(-Block1[temp_i][k]);//

			temp_sle = StrictLiteralExpression::createInt32(size_of_block*number_of_proc-k);//
			temp_bce.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS, temp_expr->clone(), temp_sle->clone()));
			temp_bce.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, name_of_X2->clone(), temp_bce->clone()));
			temp_bce2.reset(new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY, temp_sle_b1->clone() ,temp_bce->clone()));

			right_part.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS, right_part->clone() ,temp_bce2->clone()));
		}//правая часть оператора присваивания

		// Добавляем оператор присваивания
		//temp_result.reset(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, left_part->clone() ,right_part->clone()));
		ExpressionBase *temp_eb2 = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, left_part->clone() ,right_part->clone());
		StatementBase *temp_sb2 = new ExpressionStatement(temp_eb2);
		pFor2->getBody().addLast(temp_sb2);
	}

	//удаляем последний оператор входного цикла (тот единственный оператор цикла который был на входе)
	BlockStatement::Iterator iter = pFor->getBody().getLast();
	pFor->getBody().erase(iter);

	for (int i=0; i<size_of_block; i++)
	{
		delete[] Block1[i];
		delete[] Block2[i];
		delete[] Block2_invert[i];
	}
	delete[] Block1;
	delete[] Block2;
	delete[] Block2_invert;
	delete[] pFor_counter;
	delete[] A;
	delete[] e;

	return true;


//Получаем по ней блочную двухдиагональную матрицу(притом блоки будут размером m*m)
//Куда девать излишки??????? то есть правый край. Крайний правый блок скорее всего не будет размером m*m, равно как и нижняя граница
//снизу равно как и справа останется (m+N) mod m ,где m - длина реккурентности (то есть номер минимального x в правой части), а N - количество итераций цикла
//делаем структуру представляющую из себя блок. Заполняем матрицу блоков размера ((m+N) div m)*((m+N) div m),
//а точнее только ту ее часть которая не является нулевой - две диагонали
}



/*преобразование исходного рекуррентного цикла в эквивалентный ему кусок программы
//для данного преобразования необходимы константы P, M[i], G[i][j]
for (i = 0; i < log(P)/log(2.0)+1; ++i)////////
{
	for (j = M[i]; j < M[i+1]-(2^i)+1; ++j)///////////
	{
		for (k = 0; k < (2^i); ++k)///////////
		{
			x[j+k] = Schet(G[j+k][j+k-2^i+1], x[j+k-(2^i)]);///////////
		}
	}
	for (j = M[i+1]; j < N-P+(2^i) ; ++j)////////////
	{
		for (k = 0; k < P-(2^i); ++k)//////////
		{
			G[j+k][j-(2^i)] = Superpos(G[j+k][j+k-(2^i)/2], G[j+k-((2^i)/2)+1][j-(2^i)]);/////////////
		}
	}
}
*/

//???!!!реализация будет содержать баг в случае x[y[i]] = a*x[y[i]]+b и др подобных
namespace OPS
{
namespace Transforms
{
namespace Loops
{
bool MakeRLT(ForStatement* pFor, int number_of_proc, const double Tf, const double Ts)
{

	//!!!потом заменить на полиморфизм
//	switch(isMakeRLTPossible(pFor))
	switch(4)
	{
		case 0: return false;
		case 1: return MakeRLT2(pFor, number_of_proc, Tf, Ts);
		case 2: return MakeRLT3(pFor, number_of_proc, Tf, Ts);
		case 3: return MakeRLT4(pFor, number_of_proc);
		case 4:	return OPS::Transforms::Loops::IntToVector(pFor);

		//временно!!!
	}

}
}	// Loops
}	// Transforms
}	// OPS
