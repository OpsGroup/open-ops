#pragma once

#include <map>
#include <set>
#include "Reprise/Declarations.h"
#include "SetAbstractMemoryCell2.h"

namespace OPS
{
namespace Montego
{

class ProgramState;
class MemoryCellContainer;
class BasicOccurrence;

OPS_DEFINE_EXCEPTION_CLASS(AliasAnalysisException, OPS::RuntimeError)

class AliasCheckerException : public AliasAnalysisException
{
public:
    typedef std::pair<const OPS::Reprise::ExpressionBase*,
                      const OPS::Reprise::SubroutineDeclaration*> NotVisitedExpr;
    typedef std::vector<NotVisitedExpr> NotVisitedExprList;

    AliasCheckerException();
    ~AliasCheckerException() throw() {}
    void addNotVisitedExpr(const NotVisitedExpr&);
    void addNotVisitedSubr(const OPS::Reprise::SubroutineDeclaration*);
    bool isError();

    const NotVisitedExprList& getNotVisitedExprs() const { return m_list; }

protected:
    NotVisitedExprList m_list;
    bool m_isError;
};

class SAMCForOneContext
{
public:
    SAMCForOneContext(MemoryCellContainer& cont);

    // что возвращается данным выражением (это не совсем содержимое, например
    // для int c[10][10]; вхождения c; c[2]; ничего не содержат, но возвращают адреса ячеек массива)
    SetAbstractMemoryCell m_returnValue;

    // ячейки памяти, которые содержат результат данного вхождения (не пусто <=> вхождение - l-value)
    SetAbstractMemoryCell m_containerCells;

    // ячейки памяти, от которых зависит результат данного вхождения (все кроме m_containerCells)
    SetAbstractMemoryCell m_dependentCells;

private:
    SAMCForOneContext();
};
typedef std::pair< const FunctionContext*, SAMCForOneContext > PairForOccurrenceSAMC;
class OccurrenceSAMC
{
public:
    typedef std::list< PairForOccurrenceSAMC >::iterator iterator;
    OccurrenceSAMC(MemoryCellContainer& cont);
    iterator begin();
    iterator end();
    void push_back(PairForOccurrenceSAMC p);
    size_t size();
    PairForOccurrenceSAMC& front();
    SAMCForOneContext getUnion();
    std::string toString();
private:
    OccurrenceSAMC();
    std::list< PairForOccurrenceSAMC > m_occurrenceSAMC;
    MemoryCellContainer* m_memoryCellContainer;
};

class AliasInformationContainer
{
public:
    AliasInformationContainer(MemoryCellContainer& memCont);
    OccurrenceSAMC& operator[](BasicOccurrence* o);
    std::string toString(bool onlyNonEmpty);
private:
    AliasInformationContainer();
    std::map<BasicOccurrence*, OccurrenceSAMC> m_cont;
    MemoryCellContainer* m_memoryCellContainer;
};

template<typename _Key>
    class SavedProgramStates
{
public:
    typedef std::multimap<size_t, ProgramState*> HashToProgramState;
    typedef std::map<_Key*, HashToProgramState> KeyToHashCodes;

    ~SavedProgramStates() { Clear(); }
    void Clear()
    {
        HashToProgramState::iterator it = m_hashToState.begin();
        for(; it != m_hashToState.end(); ++it) delete it->second;
        m_keyToHashes.clear();
        m_hashToState.clear();
    }
    /// Сохранение текущего состояния программы
    void SaveState(_Key* key, const ProgramState& state);
    /// Проверка изменения состояния
    bool HasProgramStateChanged(_Key* key, const ProgramState& currState);

    size_t GetNumberOfSavedStates(_Key* key) const;
    size_t GetNumberOfSavedStates() const;

    size_t GetNumberOfKeys() const { return m_keyToHashes.size(); }

    typename KeyToHashCodes::const_iterator Begin() const { return m_keyToHashes.begin(); }
    typename KeyToHashCodes::const_iterator End() const { return m_keyToHashes.end(); }

private:

    KeyToHashCodes m_keyToHashes;
    HashToProgramState m_hashToState;
};

typedef SavedProgramStates<OPS::Reprise::SubroutineDeclaration> ProgramStateForProcs;

typedef std::pair<std::list<MemoryCellOffset::ElementaryOffset>, SetAbstractMemoryCell> PairForSuboffsetsContent;
typedef std::list<PairForSuboffsetsContent>::iterator SuboffsetsContentIterator;
class SuboffsetsContent
{
public:
    SuboffsetsContent();
    SuboffsetsContentIterator begin();
    SuboffsetsContentIterator end();
    void clear();
    void push_back(PairForSuboffsetsContent &p);
    void unionWith(SuboffsetsContent& other);
    std::string toString();
private:
    std::list<PairForSuboffsetsContent> m_content;
};



}//end of namespace
}//end of namespace
