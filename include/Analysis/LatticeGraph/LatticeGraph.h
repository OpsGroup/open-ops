#include "Analysis/LatticeGraph/ElemLatticeGraph.h"


//!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
///*
//	Простой решетчатый граф.
//*/
//class LatticeGraph : public Status
//				   , public OPS::NonCopyableMix
//{
//	/// Набор функций, описывающих дуги
//	std::list<LatticeDataElem*> arrowFuncList;
//
//	///далее идет описание вхождения, которое описывает конечные вершины дуг решетчатого графа
//	///	!! В случае изменения внутр. представления следующая информация (или ее часть) может потерять актуальность
//
//	///указатель на оператор, которому оно принадлежит
//	StatementBase* pStmt;		
//
//	///указатель на данное вхождение во внутреннем представлении
//	const ReferenceExpression* pOccur;		
//
//	/// описание вхождения. (ВЛАДЕЕТ, при построении происходит создание копии).
//	OccurDesc* occurDesc;
//
//	/// номер вхождения во фрагменте программы.
//	int occurNumb;
//
//public:
//	LatticeGraph():pStmt(NULL),pOccur(NULL),occurDesc(NULL){}
//	~LatticeGraph(){Clear();}
//
//	/// Получить итераторы на начало и конец списка функций
//	LatticeDataElemIterator Begin(){return arrowFuncList.begin();}
//	LatticeDataElemIterator End(){return arrowFuncList.end();}
//
//	/// Получить сам список функций
//	std::list<LatticeDataElem*>& GetArrowFuncList(){return arrowFuncList;}
//
//	/// Получить количество функций в списке
//	size_t GetArrowFuncListSize(){return arrowFuncList.size();}
//
//	///Очистить граф.
//	void Clear();
//
//	/// Построить граф.
//	void Build(const std::list<OccurDesc*>& srcEntries,OccurDesc& depEntry);
//
//	/// По концу дуги получить ее начало.
//	///возвращает NULL, если дуга с концом sink НЕ существует.
//	// иначе, возвращает указатель (LatticeDataElem*) на описание функции, определяющей начало дуги, а source будет содержать начало дуги
//	const LatticeDataElem* GetSource(LoopIndex& source,const LoopIndex& sink);
//											
//	
//	StatementBase* GetDepStatement(){return pStmt;}
//	const ReferenceExpression* GetDepOccur(){return pOccur;}
//	OccurDesc* getDepOccurDesc(){return occurDesc;}
//
//	/// Инициализировать зависимое вхождение в графе.
//	void InitDepOccurData(OccurDesc& _occurDesc)
//	{
//		pStmt=_occurDesc.GetStatement();
//		pOccur=_occurDesc.pOccur;
//		occurDesc= new OccurDesc(_occurDesc);
//		occurNumb=_occurDesc.GetNumber();
//	}
//
//	/// Возвращает true, если граф содержит набор дуг, начала которых определяются вхождением srcOccur
//	bool HasSrcOccur(const ReferenceExpression* srcOccur);
//
//	/// Получить список носителей, вычисленных по данному графу, зависимости pDepOccur от pSrcOccur
//	/// Подаваемый список предварительно очищается
//	void GetLatticeBasedSuppList(const ReferenceExpression* pSrcOccur,const ReferenceExpression* pDepOccur,std::list<int>& latticeBasedSuppList);
//
//	/// Вывести описание решетчатого графа в текстовый файл
//	void Print(const char* fileName);
//
//	///возвращает НЕ 0, если граф построен правильно. (Выполнение может занять много времени - это перебор. Зато - 100% результат!)
//	int selfTest(std::list<OccurDesc*>& srcEntries,OccurDesc& depEntry);
//
//	/// Объединить данный граф с графом lg СПЕЦИАЛЬНЫМ ОБРАЗОМ. При добавлении учитываются имеющиеся дуги в графе по следующему правилу:
//	/// для двух дуг, входящих в одну точку, если начало добавляемой дуги лекс больше начала имеющейся, то имеющаяся исключается из графа, иначе исключается добавляемая.
//	/// ВАЖНО: После объединения, подаваемый на вход граф lg ЯВЛЯЕТСЯ ПУСТЫМ (его функции переносятся в данный граф).
//	void UnionWithLatticeGraph(LatticeGraph& lg);
//
//	/// Удалить из графа все функции, которым соответствует носитель меньший supp
//	void DeleteFuncsWithSuppLessThan(int supp);
//	
//private:
//	/// Добавить к графу набор дуг СПЕЦИАЛЬНЫМ ОБРАЗОМ. При добавлении учитываются имеющиеся дуги в графе по следующему правилу:
//	/// для двух дуг, входящих в одну точку, если начало добавляемой дуги лекс больше начала имеющейся, то имеющаяся исключается из графа, иначе исключается добавляемая.
//	/// После добавления, подаваемый на вход элемент lde считается принадлежащим контейнеру графа, его указатель зануляется.
//	void UnionWithArrowFunc(LatticeDataElem*& lde);
//};
