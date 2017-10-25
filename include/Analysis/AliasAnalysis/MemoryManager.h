#ifndef OPS_ALIASANALYSIS_MEMORYMANAGER_H_INCLUDED__
#define OPS_ALIASANALYSIS_MEMORYMANAGER_H_INCLUDED__

#include "Analysis/AliasAnalysis/CallGraphEx.h"
#include "Analysis/AliasAnalysis/VariablesClassifier.h"
#include "Shared/DataShared.h"

namespace OPS
{
namespace AliasAnalysis
{

///	Менеджер памяти
class MemoryManager
{
	///	Параметры размещения в памяти
	class ParamLocation
	{
		int Offset;		///	Смещение
		int Size;		///	Размер
		int TypeSize;	///	Тип
	public:
		ParamLocation(void) {};
		ParamLocation(int offset, int size, int type_size): Offset(offset), Size(size), TypeSize(type_size) {};
		///	Устанавливает смещение
		void SetOffset(int value) { Offset = value; };
		///	Возвращает смещение
		int GetOffset() { return Offset; };
		///	Устанавливает размер области памяти
		void SetSize(int value) { Size = value; };
		///	Возвращает размер области памяти
		int GetSize() { return Size; };
		///	Устанавливает размер типа
		void SetTypeSize(int value) { TypeSize = value; };
		///	Возвращает размер типа
		int GetTypeSize() { return TypeSize; };
	};		

	typedef std::map<ExprName, ParamLocation> VariablesLocation;

public:
	MemoryManager(TranslationUnit* aTranslationUnit, CallGraphEx* aCallGraphEx): m_TranslationUnit(aTranslationUnit), m_CallGraphEx(aCallGraphEx) { m_VarClassif = new VariablesClassifier(m_TranslationUnit); Build(); };
	~MemoryManager(void) { delete m_VarClassif; }		
	///	Установка текущего блока
	void SetBlock(BlockStatement* block);

	///	Возвращает класс программы
	TranslationUnit* GetProgram() { return m_TranslationUnit; };		
	/// Возвращает расширенный граф вызовов
	CallGraphEx* GetCallGraphEx() { return m_CallGraphEx; }
	/// Возвращает классификатор
	VariablesClassifier* GetVariablesClassifier() { return m_VarClassif; }
	
	/// Проверка типа
	bool CheckType(ReferenceExpression* firstPointer, ReferenceExpression* secondPointer);
	///Возвращает имя функции
	std::string GetFuncName(ReferenceExpression* refExpr);
	
	///	Возвращает множество абстрактных ячеек памяти с учетом смещения
	SetAbstractMemoryCell GetSAMCExprValue(SetAbstractMemoryCell samcSrc, long_long_t iOffset);
	/// Возвращает смещение
	long_long_t GetIntLiteralValue(LiteralExpression* literalExpression, char iAction);	
	///	Возвращает смещение для вхождения массива
	long_long_t GetArrayOffset(CallExpressionBase& basicCallExprArray);

private:
	///	Разбор пространства имен
	void Build();
	///	Возвращает размер области памяти для переменной
	int GetSizeOf(TypeBase& typeBase, bool isFullSize);
private:
	TranslationUnit* m_TranslationUnit;	/// Класс программы
	VariablesClassifier* m_VarClassif;	/// Классификатор переменных
	CallGraphEx* m_CallGraphEx;			/// Расширенный граф вызовов
	BlockStatement* m_block;			///	Текущий блок		
	int m_CurrentOffset;				///	Текущее смещение		
	VariablesLocation m_varLocation;	///	Размещение переменных

};

} // end namespace AliasAnalysis
} // end namespace OPS


#endif
