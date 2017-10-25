#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <queue>

#include "AliasAnalysisContext.h"
#include "AliasProcedureAnalyzer.h"
#include "Analysis/ControlFlowGraph.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/Occurrence.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "ExpressionAnalyser.h"
#include "Navigation.h"
#include "Shared/StatementsShared.h"

using namespace OPS;
using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{
    AliasProcedureAnalyzer::AliasProcedureAnalyzer(
        AliasAnalysisContext* analysisContext)
       :m_currentProgramState(nullptr),
        m_finalProgramState(nullptr),
        m_funContext(nullptr),
        m_exprAnalyzer(nullptr),
        m_returnSAMC(nullptr),
        m_analysisContext(analysisContext), m_exprAnalyserRetCode(0),
        m_thereIsSyncPoint(false)
    {
    }

    void AliasProcedureAnalyzer::clear()
    {
        delete m_exprAnalyzer;
        m_exprAnalyzer = nullptr;
        delete m_currentProgramState;
        m_currentProgramState = nullptr;
        delete m_finalProgramState;
        m_finalProgramState = nullptr;
        delete m_returnSAMC;
        m_returnSAMC = nullptr;
        m_returnedStruct.clear();
        if (m_funContext->isRoot())
        {
            FunctionContext::freeRootContext(m_funContext);
            m_funContext = 0;
        }
    }


    AliasProcedureAnalyzer::~AliasProcedureAnalyzer()
    {
        clear();
    }


    /// Запуск анализа главной процедуры (main в C)
    /// Возвращает 0 - все плохо, 1 - проработала нормально
    bool AliasProcedureAnalyzer::Run(SubroutineDeclaration *subDeclaration)
    {
		OPS_ASSERT(subDeclaration != nullptr);
        m_exprAnalyserRetCode = 0;
        if (subDeclaration->hasDefinition()) subDeclaration = &subDeclaration->getDefinition();

        m_funContext = FunctionContext::createRootContext(*subDeclaration);
        delete m_exprAnalyzer;
        m_exprAnalyzer = new ExpressionAnalyser(m_analysisContext);
        delete m_returnSAMC;
        m_returnSAMC = new SetAbstractMemoryCell(m_analysisContext->memoryCellContainer());
        m_returnedStruct.clear();
        delete m_currentProgramState;
        m_analysisContext->memoryCellContainer().addGlobalCells(*subDeclaration->findProgramUnit());
        m_analysisContext->memoryCellContainer().addCellsFrom(*m_funContext);
        m_currentProgramState = new ProgramState(m_analysisContext->memoryCellContainer(), m_funContext, m_analysisContext->options().recognizeStructFields);
        delete m_finalProgramState;
        m_finalProgramState = new ProgramState(m_analysisContext->memoryCellContainer(), m_funContext, m_analysisContext->options().recognizeStructFields);
        m_analysisContext->m_navigation.putNextSubroutine(subDeclaration);

        if (subDeclaration->hasImplementation())
        {
            m_savedProgramStates.Clear();
            m_currentProgramState->init(m_funContext, m_exprAnalyzer);
            m_analysisContext->visitedProcs().SaveState(subDeclaration, *m_currentProgramState);
            ControlFlowGraphExpr& CFG = m_analysisContext->cfgBuilder().buildControlFlowGraph(subDeclaration->getBodyBlock());
#ifdef OPS_BUILD_DEBUG
            depth = 0;
#endif
            GirthCFG(CFG.getEntry());
            return true;
        }
        return false;
    }

    bool isInside(const StatementBase& inner, const StatementBase& outer)
    {
        return OPS::Shared::contain(&outer, &inner);
    }

    bool isBlockLastInProcedure(ControlFlowGraphExpr::BasicBlock& block)
    {
        return block.getOutBlocks().size() == 1 &&
               block.getOutBlocks().front()->isExit();
    }

    /// Запуск анализа процедуры, по заданному месту вызова данной процедуры и текущему состоянию программы
    /// Состояние state обновляется!
    bool AliasProcedureAnalyzer::Run(OPS::Reprise::SubroutineDeclaration* subDeclaration,
        const FunctionContext* funContext,
        ProgramState& state)
    {
        if (m_analysisContext->options().cacheProcedureAnalys)
        {
            //auto cached = m_runCache.find(std::make_pair(subDeclaration->getName(), nullptr/*funContext*/));
            AliasProcedureCallContext curContext(subDeclaration->getName(), funContext);
            auto cached = m_analysisContext->m_runCache.find(curContext);
            if (cached != m_analysisContext->m_runCache.end()) {
                state = cached->second;
                m_funContext = funContext;
                m_exprAnalyserRetCode = 0;
                delete m_exprAnalyzer;
                m_exprAnalyzer = new ExpressionAnalyser(m_analysisContext);
                delete m_returnSAMC;
                m_returnSAMC = new SetAbstractMemoryCell(m_analysisContext->memoryCellContainer());
                //std::cout << "Run was return cached state.\n";
                return true;
            }
        }

        if (m_analysisContext->options().debug)
        {
            std::cout << "AliasProcedureAnalyzer::Run called for subroutine " << subDeclaration->getName() << "\n";
            std::cout << "Procedure analyser visited " << m_analysisContext->visitedProcs().GetNumberOfKeys() <<" procs and saved "
                << m_analysisContext->visitedProcs().GetNumberOfSavedStates() << " program states\n";
            std::cout << "There are " << funContext->getDepth() << " procedures in context." << std::endl;
            if (m_analysisContext->visitedProcs().GetNumberOfSavedStates() > 500)
            {
                static ProgramState lastSavedState(m_analysisContext->memoryCellContainer(), 0, true);
                std::cout << "AliasProcedureAnalyzer::Run called for subroutine " << subDeclaration->getName() << "\n";
                std::cout << "Too many saved states. Current state:\n" << state.toString() << "\n";
                std::cout << m_analysisContext->memoryCellContainer().toString();
                if (m_analysisContext->visitedProcs().GetNumberOfSavedStates() > 510)
                    state.printDifferenceFrom(lastSavedState);
                lastSavedState = state;
            }
        }
        m_funContext = funContext;
        m_exprAnalyserRetCode = 0;
        delete m_exprAnalyzer;
        m_exprAnalyzer = new ExpressionAnalyser(m_analysisContext);
        delete m_returnSAMC;
        m_returnSAMC = new SetAbstractMemoryCell(m_analysisContext->memoryCellContainer());
        delete m_currentProgramState;  m_currentProgramState = nullptr;
        delete m_finalProgramState;  m_finalProgramState = nullptr;
        if (subDeclaration->hasDefinition()) subDeclaration = &subDeclaration->getDefinition();
        m_analysisContext->m_navigation.putNextSubroutine(subDeclaration);
        if (subDeclaration->hasImplementation())
        {
            m_savedProgramStates.Clear();
            m_currentProgramState = new ProgramState(state);
            m_finalProgramState = new ProgramState(m_analysisContext->memoryCellContainer(), m_funContext, m_analysisContext->options().recognizeStructFields);
            //если уже заходили в эту функцию с таким состоянием - выходим
            if (!m_analysisContext->visitedProcs().HasProgramStateChanged(subDeclaration, *m_currentProgramState))
                return true;

            if (funContext->recursion(m_analysisContext->options().recursionType) >= m_analysisContext->options().maximumRecursionDepth)
                return true;
            m_analysisContext->visitedProcs().SaveState(subDeclaration, *m_currentProgramState);
            ControlFlowGraphExpr& CFG = m_analysisContext->cfgBuilder().buildControlFlowGraph(subDeclaration->getBodyBlock());
            GirthCFG(CFG.getEntry());
            state = *m_finalProgramState;
            if (m_analysisContext->options().cacheProcedureAnalys)
            {
                //m_runCache.insert(std::make_pair(std::make_pair(subDeclaration->getName(), nullptr/*funContext*/), state));
                m_analysisContext->m_runCache.insert(std::make_pair(AliasProcedureCallContext(subDeclaration->getName(), funContext), state));

            }
            //check(subDeclaration); - здесь вставка проверки - неверна!!!
            return true;
        }
        return false;
    }

    void markGraph(const ControlFlowGraphExpr::BasicBlock* block, std::set<const ControlFlowGraphExpr::BasicBlock*>& marked)
    {
        if (marked.find(block) != marked.end())
            return;

        marked.insert(block);
        for (auto child : block->getOutBlocks())
        {
            markGraph(child, marked);
        }
    }

    // ToDo: Сейчас берется ближайший и черти какой. Нужно подумать, как взять правильного общего потомка.
    const ControlFlowGraphExpr::BasicBlock* findCommonBlock( const ControlFlowGraphExpr::BasicBlock* block
                                                           , const std::set<const ControlFlowGraphExpr::BasicBlock*>& marked, int maxdepth = 20 )
    {
        if (maxdepth == 0)
            return nullptr;

        if (marked.find(block) != marked.end())
            return block;

        for (auto child : block->getOutBlocks())
        {
            auto b = findCommonBlock(child, marked, maxdepth-1);
            if (b != nullptr)
                return b;
        }

        return nullptr;
    }

    // Поиск для двух блоков такого, что все пути из них пересекаются в этом блоке и он такой ближайший. 
    const ControlFlowGraphExpr::BasicBlock* findCommonChild_old(ControlFlowGraphExpr::BasicBlock &firstBlock, ControlFlowGraphExpr::BasicBlock &secondBlock)
    {
        auto marked = std::set<const ControlFlowGraphExpr::BasicBlock*>();
        markGraph(&firstBlock, marked);
        auto commonChild = findCommonBlock(&secondBlock, marked);

        return commonChild;
    }

    // Поиск для двух блоков такого, что все пути из них пересекаются в этом блоке и он такой ближайший. 
    const ControlFlowGraphExpr::BasicBlock* findCommonChild( const ControlFlowGraphExpr::BasicBlock &rootBlock
                                                           , ControlFlowGraphExpr::BasicBlock &leftBlock
                                                           , ControlFlowGraphExpr::BasicBlock &rightBlock )
    {
        auto leftQueue = std::queue<const ControlFlowGraphExpr::BasicBlock*>();
        auto markedLeft = std::set<const ControlFlowGraphExpr::BasicBlock*>();
        auto rightQueue = std::queue<const ControlFlowGraphExpr::BasicBlock*>();
        auto markedRight = std::set<const ControlFlowGraphExpr::BasicBlock*>();
        
        
        markedLeft.insert(&rootBlock);
        markedLeft.insert(&leftBlock);

        markedRight.insert(&rootBlock);
        markedRight.insert(&rightBlock);

        leftQueue.push(&leftBlock);
        rightQueue.push(&rightBlock);

        while (!leftQueue.empty() || !rightQueue.empty())
        {
            // Идем по левой половине пока не наткнемся на блок с более чем одним входом
            while ( !leftQueue.empty() )
            {
                bool needChange = false;
                auto block = leftQueue.front();
                leftQueue.pop();

                for (auto child : block->getOutBlocks())
                {   
                    if (child->getInBlocks().size() > 1)
                        needChange = true;

                    if (markedRight.find(child) != markedRight.end())
                    {
                        return child;
                    }
                    
                    // Второй раз засовывать этого потомка не нужно.
                    if (markedLeft.find(child) != markedLeft.end())
                        continue;

                    markedLeft.insert(child);
                    leftQueue.push(child);
                }

                // Если хотя бы один из потомков имел несколько входных дуг, то перебираемся анализировать вторую сторону
                if (needChange)
                    break;
            }

            // Идем по правой половине пока не наткнемся на блок с более чем одним входом
            while (!rightQueue.empty())
            {
                bool needChange = false;
                auto block = rightQueue.front();
                rightQueue.pop();

                for (auto child : block->getOutBlocks())
                {
                    if (child->getInBlocks().size() > 1)
                        needChange = true;

                    if (markedLeft.find(child) != markedLeft.end())
                    {
                        return child;
                    }

                    // Второй раз засовывать этого потомка не нужно.
                    if (markedRight.find(child) != markedRight.end())
                        continue;

                    markedRight.insert(child);
                    rightQueue.push(child);
                }

                // Если хотя бы один из потомков имел несколько входных дуг, то перебираемся анализировать вторую сторону
                if (needChange)
                    break;
            }
        }


        return nullptr;
    }

    std::unique_ptr<ProgramState> AliasProcedureAnalyzer::StepOfGirthCFG(ControlFlowGraphExpr::BasicBlock &block)
    {
        std::unique_ptr<ProgramState> state;

#ifdef OPS_BUILD_DEBUG
        std::cout << "Step Of GirthCFG: yet another block in depth " << depth << std::endl;
#endif

        // Обработка текущего блока
        const bool isLastInProcedure = isBlockLastInProcedure(block);



        m_savedProgramStates.SaveState(&block, *m_currentProgramState);

        for (size_t i = 0; i < block.size(); ++i)
        {
            visitExpr(*block[i]);
            //анализатор выражений установил, что текущее состояние программы невозможно в данном месте
            //так получается, например, когда мы идем по второй ветке if (...) p = malloc(); else p = NULL;
            /*if (m_exprAnalyserRetCode == 1)
            {
                m_exprAnalyserRetCode = 0;
                return;
            }*/
            m_analysisContext->m_exprVisitsCount[block[i]]++;
        }

        /*if (!block.empty() &&
            block.back()->getParent()->is_a<ReturnStatement>())
        {
            SetAbstractMemoryCell retValue = m_exprAnalyzer->getVisitReturnValue();
            m_returnSAMC->unionWith(retValue);
            if (Editing::desugarType(*block.back()->getResultType()).is_a<StructType>())
            {
                SuboffsetsContent exprRes = m_exprAnalyzer->getStructReturnValue();
                m_returnedStruct.unionWith(exprRes);
            }

            m_finalProgramState->unionWithProgramState(*m_currentProgramState);
            return;
        }*/

        //если этот оператор - последний в теле процедуры, то добавляем текущее состояние к финальному
        /*if (isLastInProcedure)
        {
            m_finalProgramState->unionWithProgramState(*m_currentProgramState);
            return;
        }*/

        //Обработка переходов управления
        ControlFlowGraphExpr::BasicBlockList& outBlocks = block.getOutBlocks();

        std::unique_ptr<ProgramState> savedPS(new ProgramState(*m_currentProgramState));

        // Если мы пришли к if и включен флаг объединения анализа веток, то порядок обхода должен быть несколько другим
        if (m_analysisContext->options().meshingIfBranches && outBlocks.size() == 2)
        {
            // ToDo: Нужно не идти вглубь, а посмотреть два блока и объединить их результаты.
            auto thenState = StepOfGirthCFG(*outBlocks[0]);
            auto elseState = StepOfGirthCFG(*outBlocks[1]);

            thenState->unionWithProgramState(*elseState);

        }

        for (size_t i = 0; i < outBlocks.size(); ++i)
        {
            if (i != 0)
                *m_currentProgramState = *savedPS;
        #ifdef OPS_BUILD_DEBUG
            depth++;
        #endif
            GirthCFG(*outBlocks[i]);
        #ifdef OPS_BUILD_DEBUG
            depth--;
        #endif
        }

        return state;
    }

    void AliasProcedureAnalyzer::GirthCFG(ControlFlowGraphExpr::BasicBlock &block)
    {
#ifdef OPS_BUILD_DEBUG
        if (m_analysisContext->options().debug)
            std::cout << "GirthCFG: yet another block in depth " << depth << "[" << &block << "]" << std::endl;
#endif

        // Обработка текущего блока
        const bool isLastInProcedure = isBlockLastInProcedure(block);

        // Проверка изменения состояния
        if (m_analysisContext->options().debug)
            std::cout << ">> There are " << m_savedProgramStates.GetNumberOfSavedStates(&block) << " program states of an operator\n";
        if (!m_savedProgramStates.HasProgramStateChanged(&block, *m_currentProgramState) ||
             //чтобы ускорить работу анализатора ограничиваем количество циклических обходов
             m_savedProgramStates.GetNumberOfSavedStates(&block) > 5)
        {
            if (isLastInProcedure)
                m_finalProgramState->unionWithProgramState(*m_currentProgramState);
            return;
        }

        m_savedProgramStates.SaveState(&block, *m_currentProgramState);

        for(size_t i = 0; i < block.size(); ++i)
        {
            visitExpr(*block[i]);
            //анализатор выражений установил, что текущее состояние программы невозможно в данном месте
            //так получается, например, когда мы идем по второй ветке if (...) p = malloc(); else p = NULL;
            if (m_exprAnalyserRetCode == 1)
            {
                m_exprAnalyserRetCode = 0;
                return;
            }
            m_analysisContext->m_exprVisitsCount[block[i]]++;
        }

        if (!block.empty() &&
             block.back()->getParent()->is_a<ReturnStatement>())
        {
            SetAbstractMemoryCell retValue = m_exprAnalyzer->getVisitReturnValue();
            m_returnSAMC->unionWith(retValue);
            if (Editing::desugarType(*block.back()->getResultType()).is_a<StructType>())
            {
                SuboffsetsContent exprRes = m_exprAnalyzer->getStructReturnValue();
                m_returnedStruct.unionWith(exprRes);
            }

            m_finalProgramState->unionWithProgramState(*m_currentProgramState);
            return;
        }

        //если этот оператор - последний в теле процедуры, то добавляем текущее состояние к финальному
        if (isLastInProcedure)
        {
            m_finalProgramState->unionWithProgramState(*m_currentProgramState);
            return;
        }

        // Проверяем, а не точка ли это синхронизации анализа веток if и не идем дальше, если счетчик еще полон
        if (m_thereIsSyncPoint && &block == m_syncPoint)
        {
#ifdef OPS_BUILD_DEBUG
            if (m_analysisContext->options().debug)
                std::cout << "    syncPoint " << "[" << m_syncPoint << "]... ";
#endif
            if (--m_syncCounter)
            {
#ifdef OPS_BUILD_DEBUG
                if (m_analysisContext->options().debug)
                    std::cout << "We need " << m_syncCounter << " attempts." << std::endl;
#endif
                return;
            }
            else
            {
#ifdef OPS_BUILD_DEBUG
                if (m_analysisContext->options().debug)
                    std::cout << "We can go away." << std::endl;
#endif
                m_thereIsSyncPoint = false;
                // Дальнейшие обнуления, в общем-то не нужны
                m_syncPoint = nullptr;
            }
        }

        //Обработка переходов управления
        ControlFlowGraphExpr::BasicBlockList& outBlocks = block.getOutBlocks();

        std::unique_ptr<ProgramState> savedPS(new ProgramState(*m_currentProgramState));

        // Если мы пришли к if и включен флаг объединения анализа веток и при этом точки синхронризации еще нет,
        // то порядок обхода должен быть несколько другим.
        if ( m_analysisContext->options().meshingIfBranches
          && !m_thereIsSyncPoint
          && outBlocks.size() == 2)
        {
            //m_syncPoint = findCommonChild_old(*outBlocks[0], *outBlocks[1]);
            m_syncPoint = findCommonChild(block, *outBlocks[0], *outBlocks[1]);
            if (m_syncPoint == &block)
                m_syncPoint = nullptr;

            if (m_syncPoint != nullptr)
            {
                m_thereIsSyncPoint = true;
                m_syncCounter = 2; // m_syncPoint->getInBlocks().size();
#ifdef OPS_BUILD_DEBUG
                if (m_analysisContext->options().debug)
                    std::cout << "    New syncPoint " << "[" << m_syncPoint << "] with " << m_syncCounter << " attempts." << std::endl;
#endif
            }
            // ToDo: Нужно не идти вглубь, а посмотреть два блока и объединить их результаты.
            //auto thenState = StepOfGirthCFG(*outBlocks[0]);
            //auto elseState = StepOfGirthCFG(*outBlocks[1]);

            //thenState->unionWithProgramState(*elseState);

        }

        for(size_t i = 0; i < outBlocks.size(); ++i)
        {
            if (i != 0)
                *m_currentProgramState = *savedPS;
#ifdef OPS_BUILD_DEBUG
            if (m_analysisContext->options().debug)
                depth++;
#endif
            GirthCFG(*outBlocks[i]);
#ifdef OPS_BUILD_DEBUG
            if (m_analysisContext->options().debug)
                depth--;
#endif
        }
    }


    void AliasProcedureAnalyzer::visitExpr(const ExpressionBase& expr)
    {
        m_exprAnalyzer->clear();
        m_exprAnalyzer->visit(expr, *m_currentProgramState, m_funContext);
        m_exprAnalyserRetCode = m_exprAnalyzer->getRetCode();
    }

    SetAbstractMemoryCell AliasProcedureAnalyzer::getReturnSAMC()
    {
        return *m_returnSAMC;
    }

    SuboffsetsContent AliasProcedureAnalyzer::getReturnedStruct()
    {
        return m_returnedStruct;
    }

    template<typename _Key>
        void SavedProgramStates<_Key>::SaveState(_Key *key, const ProgramState &state)
    {
        const size_t h = state.getHashCode();
        HashToProgramState::const_iterator it = m_hashToState.lower_bound(h);
        ProgramState* storedState = 0;

        for(; it != m_hashToState.end() && it->first == h; ++it)
        {
            if (*it->second == state)
            {
                storedState = it->second;
                break;
            }
        }

        if (storedState == 0)
        {
            storedState = new ProgramState(state);
            m_hashToState.insert(std::make_pair(h, storedState));
        }

        HashToProgramState& hashes = m_keyToHashes[key];
        hashes.insert(std::make_pair(h, storedState));
    }

    template<typename _Key>
        bool SavedProgramStates<_Key>::HasProgramStateChanged(_Key *key, const ProgramState &currState)
    {
        const HashToProgramState& hashes = m_keyToHashes[key];
        const size_t currHash = currState.getHashCode();

        HashToProgramState::const_iterator itHash = hashes.lower_bound(currHash),
            itEnd = hashes.end();

        for(; itHash != itEnd && itHash->first == currHash; ++itHash)
        {
            if (*itHash->second == currState)
                return false;
        }
        return true;
    }

    template<typename _Key>
        size_t SavedProgramStates<_Key>::GetNumberOfSavedStates(_Key *key) const
    {
        typename KeyToHashCodes::const_iterator it = m_keyToHashes.find(key);
        if (it != m_keyToHashes.end())
            return it->second.size();
        else
            return 0;
    }

    template<typename _Key>
        size_t SavedProgramStates<_Key>::GetNumberOfSavedStates() const
    {
        size_t numberOfStates = 0;
        typename KeyToHashCodes::const_iterator it = m_keyToHashes.begin();
        for(; it != m_keyToHashes.end(); ++it)
        {
            numberOfStates += it->second.size();
        }
        return numberOfStates;
    }

} // end namespace Montego
} // end namespace OPS
