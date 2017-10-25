#pragma once
/*
здесь описан клас анализатора  операций с адресами в выражениях.
Он вызывается для выражения или ExpressionStatment с параметром - состоянием программы
на данный момент, анализирует каждую операцию в выражении, если нужно изменяет
состояние программы, и добавляет новую информацию в SAMC вхождений в выражении
*/

#include "Analysis/Montego/AliasAnalysis/AliasTypes.h"
#include "Analysis/Montego/Occurrence.h"
#include "Analysis/Montego/AliasAnalysis/SetAbstractMemoryCell2.h"
#include "Analysis/Montego/AliasAnalysis/MemoryCell.h"
#include "ProgramState.h"
#include "MemoryCellContainer.h"
#include "Reprise/Reprise.h"
#include "Analysis/Montego/SafeStatus.h"

namespace OPS
{
namespace Montego
{
class ProgramState;
class OccurrenceContainer;
class AliasAnalysisContext;
class AliasInformationContainer;

class ExpressionAnalyser : public OPS::BaseVisitor,
    public OPS::Visitor<OPS::Reprise::ExpressionStatement>,
    public OPS::Visitor<OPS::Reprise::BasicLiteralExpression>,
    public OPS::Visitor<OPS::Reprise::StrictLiteralExpression>,
    public OPS::Visitor<OPS::Reprise::CompoundLiteralExpression>,
    public OPS::Visitor<OPS::Reprise::ReferenceExpression>,
    public OPS::Visitor<OPS::Reprise::SubroutineReferenceExpression>,
    public OPS::Visitor<OPS::Reprise::StructAccessExpression>,
    public OPS::Visitor<OPS::Reprise::EnumAccessExpression>,
    public OPS::Visitor<OPS::Reprise::TypeCastExpression>,
    public OPS::Visitor<OPS::Reprise::BasicCallExpression>,
    public OPS::Visitor<OPS::Reprise::SubroutineCallExpression>,
    public OPS::Visitor<OPS::Reprise::EmptyExpression>,
    public OPS::NonCopyableMix
{
public:

    ExpressionAnalyser(AliasAnalysisContext *analysisContext);

    ~ExpressionAnalyser();

    void clear();

    //анализирует оператор-выражение, меняет текущее состояние и обновляет SAMC вхождений
    //возвращает true, если состояние изменилось, false - не поменялось
    bool visit(const OPS::Reprise::ExpressionStatement&, ProgramState&, const FunctionContext*);

    //анализирует выражение, меняет текущее состояние и обновляет SAMC вхождений
    //возвращает true, если состояние изменилось, false - не поменялось
    bool visit(const OPS::Reprise::ExpressionBase&, ProgramState&, const FunctionContext*);

    SetAbstractMemoryCell getVisitReturnValue();
    SuboffsetsContent getStructReturnValue();
    AliasAnalysisContext& getAnalysisContext();

    //строит (НО НЕ ЗАПОЛНЯЕТ) SAMC вхождения в соответствии с текущим состоянием памяти программы
    SAMCForOneContext buildOccurrenceSAMC(BasicOccurrence& o, ProgramState& programState, SuboffsetsContent *structReturnValue = 0, bool printDebug = false);

    //обновляет SAMC вхождения в соответствии с текущим состоянием памяти программы
    //возвращает true, если SAMC изменилась, false - не поменялась
    //применяется только при анализе псевдонимов
    bool addToOccurrenceSAMC(BasicOccurrence& o, ProgramState& state, const FunctionContext* functionContext);

    //возвращает возможные ЯЧЕЙКИ ПАМЯТИ, в которых храниться содержимое вхождения по заданному состоянию всех ячеек памяти программы
    //применяется только при анализе псевдонимов
    SetAbstractMemoryCell getOccurrenceAddress(BasicOccurrence& o, ProgramState& state);

    //возвращает возможные ЗНАЧЕНИЯ данного вхождения по заданному состоянию всех ячеек памяти программы, заполняет structReturnValue, если оно не 0 и вхождение возвращает структуру
    //применяется только при анализе псевдонимов
    SetAbstractMemoryCell getOccurrenceContent(BasicOccurrence& o, ProgramState& state, SuboffsetsContent *structReturnValue);

    int getRetCode();

private:
    SetAbstractMemoryCell copy(BasicOccurrencePtr dst, OPS::Reprise::ExpressionBase& src, bool flagIsMemcpy);
    void visit(OPS::Reprise::ExpressionStatement&);
    void visit(OPS::Reprise::BasicLiteralExpression&);
    void visit(OPS::Reprise::StrictLiteralExpression&);
    void visit(OPS::Reprise::CompoundLiteralExpression&);
    void visit(OPS::Reprise::ReferenceExpression&);
    void visit(OPS::Reprise::SubroutineReferenceExpression&);
    void visit(OPS::Reprise::StructAccessExpression&);
    void visit(OPS::Reprise::EnumAccessExpression&);
    void visit(OPS::Reprise::TypeCastExpression&);
    void visit(OPS::Reprise::BasicCallExpression&);
    void visit(OPS::Reprise::SubroutineCallExpression&);
    void visit(OPS::Reprise::EmptyExpression&);

    ProgramState* m_programState;
    const FunctionContext* m_functionContext;
    bool m_programStateChanged;
    SetAbstractMemoryCell m_visitReturnValue;//возвращаемое визиторами значение хранится здесь
    //если возвращается структура, то она храниться здесь (список пар: смещение внутри структуры -> его содержимое)
    SuboffsetsContent m_structReturnValue;
    AliasAnalysisContext* m_analysisContext;
    int m_retCode; //код ошибки. 0 - все нормально, 1 - недопустимое состояние программы
};

}//end of namespace
}//end of namespace
