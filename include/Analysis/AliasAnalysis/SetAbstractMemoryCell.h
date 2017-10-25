#ifndef OPS_ALIASANALYSIS_SAMC_H_INCLUDED__
#define OPS_ALIASANALYSIS_SAMC_H_INCLUDED__

#include <string>
#include <vector>
#include <map>
#include <list>
#include <iostream>

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"
#include "Analysis/ControlFlowGraph.h"

namespace OPS
{
namespace AliasAnalysis
{

enum AliasResult
{
	NO_ALIAS=0,
	MAY_ALIAS,
	MUST_ALIAS
};

typedef std::string ExprName;

/// Абстрактная ячейка памяти
class AbstractMemoryCell
{
	/// Имя переменной
	ExprName Name;		
	/// Смещение
	long_long_t Offset;	
public:
	AbstractMemoryCell(void) {};
	AbstractMemoryCell(ExprName name, long_long_t offset): Name(name), Offset(offset) {};		
	///	Устанавливает имя переменной
	void SetName(ExprName value) { Name = value; };		
	///	Возвращает имя переменной
	ExprName GetName() { return Name; };		
	///	Устанавливает смещение
	void SetOffset(long_long_t value) { Offset = value; };		
	///	Возвращает смещение
	long_long_t GetOffset() { return Offset; };
};


///	Множество абстрактных ячеек памяти
typedef std::vector<AbstractMemoryCell> SetAbstractMemoryCell;


///	Класс сравнения множеств абстрактных ячеек памяти
class SAMCCompare
{
	///	Проверка на полное совпадение
	bool IsFullMatch(SetAbstractMemoryCell& samcFirst, SetAbstractMemoryCell& samcSecond);
	///	Проверка на полное не совпадение
	bool IsNotFullMatch(SetAbstractMemoryCell& samcFirst, SetAbstractMemoryCell& samcSecond);
public:		
	/// Проверка на псевдоним
	AliasResult IsNamesAlias(SetAbstractMemoryCell& samcFirst, SetAbstractMemoryCell& samcSecond);
};

} // end namespace AliasAnalysis
} // end namespace OPS

#endif
