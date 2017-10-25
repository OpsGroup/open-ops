#include "AliasAnalysisTester.h"
#include "Analysis/Montego/AliasAnalysis/AliasTypes.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Analysis/Montego/Occurrence.h"
#include "Analysis/Montego/AliasAnalysis/SetAbstractMemoryCell2.h"
#include "Analysis/Montego/AliasAnalysis/MemoryCell.h"
#include "../ProgramState.h"
#include "../MemoryCellContainer.h"
#include "Analysis/Montego/AliasAnalysis/AliasImplementation.h"
#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Backends/RepriseXml/RepriseXml.h"
#include <stdlib.h>
#include <fstream>
#include "Analysis/Montego/SafeStatus.h"
#include "Backends/OutToC/OutToC.h"


const char compileCommand[] = "test\\setsystemvars.bat";
const char fileWithTestFileNames[] = "test\\compile.bat";
const char runTestCommandPrefix[] = "";
const char fileForTestingAliases[] = "fileForTestingAliases";
const unsigned char maxFprintfCallCount = 10;

using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{

AliasAnalysisTester::AliasAnalysisTester()
{
    m_aliasInterface = 0;
    m_occurrenceContainer = 0;
}
AliasAnalysisTester::~AliasAnalysisTester()
{
    clear();
    delete m_aliasInterface;
    delete m_occurrenceContainer;
}
/* не работает для новых fprintf
//ЗАПУСКАТЬ ДЛЯ ТЕЛА ФУНКЦИИ MAIN!!!!!!
//строит список вхождений в функциях fprintf(fileForTestingAliases,...)
//container - уже построенный контейнер вхождений, ai - уже проведенный анализ альясов
class SearchFprintfVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    SearchFprintfVisitor(OccurrenceContainer& container, AliasInterface& ai)
                :m_container(&container),m_ai(&ai),m_errCode(0){}
    ~SearchFprintfVisitor(){}

    std::vector<SetAbstractMemoryCell> m_addresses;
    std::vector<BasicOccurrence*> m_occurs;
    OccurrenceContainer* m_container;
    AliasInterface* m_ai;
    int m_errCode;//код ошибки, см.  AliasAnalysisTester::runTest
    std::string m_outputMessage;//если != "", то произошла ошибка

    void visit(OPS::Reprise::SubroutineCallExpression& e)
    {
        if (e.getChild(0).is_a<Reprise::SubroutineReferenceExpression>())
        {
            Reprise::SubroutineReferenceExpression& sref = e.getChild(0).cast_to<Reprise::SubroutineReferenceExpression>();
            if ((sref.getReference().getName() == "fprintf") &&
                (e.getArgument(0).is_a<Reprise::ReferenceExpression>()) &&
                (e.getArgument(0).cast_to<Reprise::ReferenceExpression>().getReference().getName() == fileForTestingAliases) )
            {
                //это fprintf, добавляем содержимое вхождений в список
                for (int i = 3; i < e.getChildCount(); ++i)
                {
                    BasicOccurrence* o;
                    if (m_container->getOccurBy(e.getChild(i),o))
                    {
                        m_outputMessage = "Cant find occurrence for " + Strings::format("%d",i) + 
                            " argument of printf. It is " + Strings::format("%d",m_addresses.size()+1) + " occurrence in whole program's fprintfs";
                        m_errCode = 2;
                        throw OPS::RuntimeError("");
                    }
                    m_occurs.push_back(o);
                    SetAbstractMemoryCell samc(*(m_ai->m_memoryCellContainer));
                    m_ai->getOccurrenceContent(*o,samc);
                    m_addresses.push_back(samc);
                }
            }
            else
            {
                //в противном случае заходим в определение функции
                std::list<Reprise::SubroutineDeclaration*> sdecls = m_ai->getAllPossibleSubroutinesByPointer(e);
                if (sdecls.size()==0)
                {
                    m_outputMessage = "Alias tester cant determine what function is called by pointer. There are 0 possible functions.";
                    m_errCode = 1;
                    throw OPS::RuntimeError("");
                }
                std::list<Reprise::SubroutineDeclaration*>::iterator it = sdecls.begin();
                for ( ; it != sdecls.end(); ++it)
                    if ((*it)->hasImplementation()) (*it)->getBodyBlock().accept(*this);
            }
        }
    }
};
*/

class ConvertBoolToIntVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    ConvertBoolToIntVisitor(){}
    ~ConvertBoolToIntVisitor(){}
    void visit(VariableDeclaration& decl)
    {
        TypeBase& t = decl.getType();
        BasicType* bt = t.cast_ptr<BasicType>();
        if (bt != 0)
        {
            if (bt->getKind() == BasicType::BT_BOOLEAN)
            {
                BasicType* intType = BasicType::uint32Type();
                decl.setType(intType);
            }
        }
    }
};


bool isPtrType(BasicOccurrence & o)
{
    if (o.isAddress()) return true;

    ExpressionBase* hn = o.getHeadNode();
    ExpressionBase* ch, *par;
    ch = o.getRefExpr();
    par = ch;
    TypeBase* curType = &o.getRefExpr()->getReference().getType();
    bool flagEnd = (par == hn);
    while (!flagEnd)
    {
        par = ch->getParent()->cast_ptr<ExpressionBase>();
        BasicCallExpression* rab = par->cast_ptr<BasicCallExpression>();
        if (rab != 0)
        {
            //это операция []
            for (int i = 0; i < rab->getChildCount()-1; ++i)
            {
                if (curType->getChildCount() == 0)
                {
                    StatementBase* stmt = o.getParentStatement();
                    Backends::OutToC writer(std::cout);
                    stmt->accept(writer);
                }
                OPS_ASSERT(curType->getChildCount() > 0);
                curType = curType->getChild(0).cast_ptr<OPS::Reprise::TypeBase>();
                OPS_ASSERT(curType != 0);
            }
        }
        else
        {
            //это операция .
            StructAccessExpression* rab2 = par->cast_ptr<StructAccessExpression>();
            OPS_ASSERT(rab2 != 0);
            StructMemberDescriptor& smd = rab2->getMember();
            curType = &smd.getType();
        }
        ch = par;
        flagEnd = (par == hn);
    }
    return (curType->is_a<PtrType>()) || (curType->is_a<ArrayType>());
}

std::vector<ExpressionBase*> buildOccurArgs(const std::vector<BasicOccurrencePtr>& occurs, int& occursWithAddrCount)
{
    std::vector<ExpressionBase*> occurArgs;
    for (size_t i = 0; i < occurs.size(); ++i)
    {
        ExpressionBase* hn;
        hn = occurs[i]->getHeadNode();
        //добавляем содержимое вхождения
        if (isPtrType(*occurs[i]))
        {
            occurArgs.push_back(hn->clone());
            ++occursWithAddrCount;
        }
        if (!(occurs[i]->isAddress()))
            occurArgs.push_back(new BasicCallExpression(BasicCallExpression::BCK_TAKE_ADDRESS,hn->clone()));
        //добавляем то же вхождение, но с меньшим числом скобок
        ExpressionBase* ch, *par = occurs[i]->isAddress() ? (ExpressionBase*)&hn->getChild(0) : hn ;
        //добавляем в список вхождений нулевой аргумент каждой операции []
        bool flagRefExpr = par->is_a<ReferenceExpression>();
        while (!flagRefExpr)
        {
            //добавляем адрес
            occurArgs.push_back(new BasicCallExpression(BasicCallExpression::BCK_TAKE_ADDRESS, par->clone()));
            ch = par->getChild(0).cast_ptr<ExpressionBase>();
            BasicCallExpression* rab = par->cast_ptr<BasicCallExpression>();
            if (rab != 0)
            {
                //это операция [][][]
                //добавляем ее нулевой аргумент
                occurArgs.push_back(ch->clone());
                //добавляем нулевой аргумент с меньшим числом скобок, чем на самом деле
                for (int childIndex = 0; childIndex < ch->getChildCount()-2; ++childIndex)
                {
                    BasicCallExpression* bce = new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS);
                    bce->addArgument(ch->clone());
                    for (int j = 0; j < childIndex+1; ++j)
                    {
                        bce->addArgument(ch->getChild(j+1).clone()->cast_ptr<ExpressionBase>());
                    }
                    occurArgs.push_back(bce);
                }
            }
            par = ch;
            flagRefExpr = par->is_a<ReferenceExpression>();
        }
    }
    return occurArgs;
}


/*конструирует
(?? - № вставляемой функции fprintf)
    if (fprintfCallCounts[??] <= maxFprintfCallCount)
    {
        fprintfCallCounts[??] = fprintfCallCounts[??] + 1;
        fprintf(fileForTestingAliases,"%d %d %d %d ...",??,...);
    }
*/
IfStatement* buildIfStatementWithFprintf(Reprise::TranslationUnit* program, ExpressionBase* e, std::vector<Reprise::SubroutineCallExpression*>& fprintfs, OccurrenceContainer& container, int& occursWithAddrCount)
{
    size_t currentFprintfIndex = fprintfs.size();
    Declarations& globals = program->getGlobals();
    VariableDeclaration* fileDecl = globals.findVariable((std::string)fileForTestingAliases);
    std::vector<BasicOccurrencePtr> occurs;
    occurs = container.getAllBasicOccurrencesIn(e);
    
    //аргументы fprintf
    ReferenceExpression* arg0 = new ReferenceExpression(*fileDecl);
    StrictLiteralExpression* arg1 = new StrictLiteralExpression(BasicType::BT_STRING);
    StrictLiteralExpression* arg2 = new StrictLiteralExpression(BasicType::BT_INT16);
    arg2->setUInt16((word)currentFprintfIndex);
    std::string fprintfString = "%d";
    std::vector<ExpressionBase*> occurArgs = buildOccurArgs(occurs, occursWithAddrCount);
    for (size_t i = 0; i < occurArgs.size(); ++i)  fprintfString += " %d";
    fprintfString += "\n";
    arg1->setString(fprintfString);

    //вызов функции fprintf
    SubroutineDeclaration* fprintf_sdec = globals.findSubroutine("fprintf");
    OPS_ASSERT(fprintf_sdec != 0);
    SubroutineReferenceExpression* fprintf_sref = new SubroutineReferenceExpression(*fprintf_sdec);
    SubroutineCallExpression* scallFprintf = new SubroutineCallExpression(fprintf_sref);
    fprintfs.push_back(scallFprintf);
    scallFprintf->addArgument(arg0);
    scallFprintf->addArgument(arg1);
    scallFprintf->addArgument(arg2);
    for (size_t i = 0; i < occurArgs.size(); ++i)
        scallFprintf->addArgument(occurArgs[i]);

    //оператор fprintfCallCounts[??] = fprintfCallCounts[??] + 1;
    ReferenceExpression* arrRef = new ReferenceExpression(*(globals.findVariable("fprintfCallCounts")));
    StrictLiteralExpression* num1 = new StrictLiteralExpression(BasicType::BT_INT16);
    num1->setUInt16((word)currentFprintfIndex);
    StrictLiteralExpression* one = new StrictLiteralExpression(BasicType::BT_INT16);
    one->setUInt16(1);
    BasicCallExpression* arrAcc = new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS,arrRef,num1);
    BasicCallExpression* arrInc = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS,arrAcc,one);
    BasicCallExpression* arrAssign = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN,arrAcc->clone(),arrInc);

    //блок с увеличением массива и fprintf
    BlockStatement* if_bl = new BlockStatement();
    if_bl->addLast(new ExpressionStatement(arrAssign));
    if_bl->addLast(new ExpressionStatement(scallFprintf));

    //условие оператора if: fprintfCallCounts[??] <= maxFprintfCallCount
    StrictLiteralExpression* maxFprintfCallCount_ex = new StrictLiteralExpression(BasicType::BT_INT16);
    maxFprintfCallCount_ex->setUInt16(maxFprintfCallCount);
    BasicCallExpression* ifCond = new BasicCallExpression(BasicCallExpression::BCK_LESS_EQUAL,arrAcc->clone(),maxFprintfCallCount_ex);
    
    //оператор if
    IfStatement* ifstmt = new IfStatement(ifCond);
    ifstmt->setThenBody(if_bl);
    
    return ifstmt;
}

void myAddStmt(StatementBase& afterWhat, StatementBase& smthToAdd, bool after)
{
    BlockStatement& outerBlock = afterWhat.getParentBlock();
    BlockStatement::Iterator it = outerBlock.getFirst();
    while (&(*it) != &afterWhat) 
    {
        OPS_ASSERT(it != outerBlock.getLast());
        ++it;
    }
    if (after)
        outerBlock.addAfter(it,&smthToAdd);
    else
        outerBlock.addBefore(it,&smthToAdd);
}


class AddFprintfVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    AddFprintfVisitor(Reprise::TranslationUnit& program):m_program(&program), m_occursWithAddrCount(0)
    {
        m_container = new OccurrenceContainer(program);  
        m_flagStmtWasAdded = false;
    }
    
    ~AddFprintfVisitor(){delete m_container;}

    std::vector<Reprise::SubroutineCallExpression*> m_fprintfs;
    std::vector< std::vector<BasicOccurrencePtr> > m_addresses;
    OccurrenceContainer* m_container;
    Reprise::TranslationUnit* m_program;
    int m_occursWithAddrCount;
    bool m_flagStmtWasAdded;

    void visit(BlockStatement& block)
    {
        for (BlockStatement::Iterator iter = block.getFirst(); iter.isValid(); iter.goNext())
        {
            iter->accept(*this);
            if (m_flagStmtWasAdded) 
            {
                m_flagStmtWasAdded = false;
                iter.goNext();
            }
        }
    }

    void visit(ExpressionStatement& e)
    {
        IfStatement* ifstmt = buildIfStatementWithFprintf(m_program, &e.get(), m_fprintfs, *m_container, m_occursWithAddrCount);
        myAddStmt(e,*ifstmt,true);
        m_flagStmtWasAdded = true;
    }

    void visit(IfStatement& ifs)
    {
        visit(ifs.getThenBody());
        visit(ifs.getElseBody());
        IfStatement* ifstmt = buildIfStatementWithFprintf(m_program, &ifs.getCondition(), m_fprintfs, *m_container, m_occursWithAddrCount);
        myAddStmt(ifs,*ifstmt,true);
        m_flagStmtWasAdded = true;
        IfStatement* ifstmt2 = buildIfStatementWithFprintf(m_program, &ifs.getCondition(), m_fprintfs, *m_container, m_occursWithAddrCount);
        ifs.getThenBody().addBefore(ifs.getThenBody().getFirst(),ifstmt2);
    }

    void visit(ForStatement& fors)
    {
        visit(fors.getBody());
        IfStatement* ifstmt1 = buildIfStatementWithFprintf(m_program, &fors.getInitExpression(), m_fprintfs, *m_container, m_occursWithAddrCount);
        IfStatement* ifstmt2 = buildIfStatementWithFprintf(m_program, &fors.getFinalExpression(), m_fprintfs, *m_container, m_occursWithAddrCount);
        IfStatement* ifstmt3 = buildIfStatementWithFprintf(m_program, &fors.getStepExpression(), m_fprintfs, *m_container, m_occursWithAddrCount);
        BlockStatement& forBlock = fors.getBody();
        forBlock.addBefore(forBlock.getFirst(),ifstmt3);
        forBlock.addBefore(forBlock.getFirst(),ifstmt2);
        forBlock.addBefore(forBlock.getFirst(),ifstmt1);
    }
    void visit(WhileStatement& whs)
    {
        visit(whs.getBody());
        IfStatement* ifstmt = buildIfStatementWithFprintf(m_program, &whs.getCondition(), m_fprintfs, *m_container, m_occursWithAddrCount);
        whs.getBody().addBefore(whs.getBody().getFirst(),ifstmt);
    }
};

class SearchUserFunctionsVisitor : public Service::DeepWalker
{
public:
    SearchUserFunctionsVisitor(Reprise::TranslationUnit& program):m_program(&program)
    {
        m_container = new OccurrenceContainer(program);  
        m_ai = AliasInterface::create(*program.findProgramUnit(), *m_container);
        int err = m_ai->runAliasAnalysis();
        if (err > 0)
        {
            throw OPS::RuntimeError("Alias analysis was not done. May be canonical transformations were not done?");
        }
        Declarations& globals = m_program->getGlobals();
        SubroutineDeclaration* mains = globals.findSubroutine("main");
        m_blocksToAnalyse.insert(&mains->getBodyBlock());
        m_funcsToAnalyse.insert(mains);
        mains->accept(*this);
    }

    ~SearchUserFunctionsVisitor(){delete m_container; delete m_ai;}

    std::set<BlockStatement*> m_blocksToAnalyse;
    std::set<SubroutineDeclaration*> m_funcsToAnalyse;
    OccurrenceContainer* m_container;
    Reprise::TranslationUnit* m_program;
    AliasInterface* m_ai;

	void visit(OPS::Reprise::SubroutineCallExpression& e)
    {
        std::list<SubroutineDeclaration*> sdecls = m_ai->getAllPossibleSubroutinesByPointer(e);
        std::list<SubroutineDeclaration*>::iterator it;
        for (it = sdecls.begin(); it != sdecls.end(); ++it)
        {
            if ((*it)->hasDefinition())
            {
                Reprise::SubroutineDeclaration* sdecl = &(*it)->getDefinition();
                BlockStatement* body = &sdecl->getBodyBlock();
				size_t oldSize = m_blocksToAnalyse.size();
                m_blocksToAnalyse.insert(body);
                m_funcsToAnalyse.insert(*it);
                if (m_blocksToAnalyse.size() > oldSize)
                {
                    body->accept(*this);
                }
            }
        }
    }
};

//расставляет в программе функции fprintf для вхождений, содержащих адресные данные
//заполняет массив указателей m_fprintfs на добавленные функции
//добавляет также глобальный массив флагов, которые обеспечивают единственность запуска каждого fprintf
int AliasAnalysisTester::addFprintfToProgram()
{
    //составляем список тел функций для анализа (нужны только те, в стеке вызова которых есть main)
    SearchUserFunctionsVisitor searchFuncs(*m_program);
    std::set<BlockStatement*> blocksToAnalyse = searchFuncs.m_blocksToAnalyse;
    std::set<SubroutineDeclaration*> funcsToAnalyse = searchFuncs.m_funcsToAnalyse;
    
    //находим первую анализируемую функцию. Перед ней мы должны вставить глобальные переменные
    Declarations& globals = m_program->getGlobals();
	Declarations::SubrIterator firstSubrIter = globals.getFirstSubr();
    for (; firstSubrIter.isValid(); ++firstSubrIter) 
	{
		if (funcsToAnalyse.find(&*firstSubrIter) != funcsToAnalyse.end())
			break;
	}
    SubroutineDeclaration* firstSubr = 0;
	if (firstSubrIter.isValid())
	{
		firstSubr = &*firstSubrIter;
	}

    //создаем файловую переменную
    TypeDeclaration* fileTypeDecl = globals.findType("FILE");
    if (fileTypeDecl == 0) 
    {
        std::cout << "Add #include <stdio.h> to program\n";
        return 2;
    }
    DeclaredType* fileType = new DeclaredType(*fileTypeDecl);
    PtrType* pFileType = new PtrType(fileType);
    VariableDeclaration* file = new VariableDeclaration(pFileType,(std::string)fileForTestingAliases);
    
    globals.addBefore(globals.convertToIterator(firstSubr),file);
    
    //создаем массив пока с одним элементом, потом увеличим количество
    BasicType* ucharType = BasicType::uint8Type();
    ArrayType* arrType = new ArrayType(1, ucharType);
    VariableDeclaration* fprintfCallCounts = new VariableDeclaration(arrType,"fprintfCallCounts");
    globals.addBefore(globals.convertToIterator(firstSubr),fprintfCallCounts);

    //вставляем fprintf
    std::set<BlockStatement*>::iterator it;
    AddFprintfVisitor addFprf(*m_program);
    for (it = blocksToAnalyse.begin(); it != blocksToAnalyse.end(); ++it)
    {
        (*it)->accept(addFprf);
    }
    m_addresses = addFprf.m_addresses;
    m_fprintfs = addFprf.m_fprintfs;
    m_occursWithAddrCount = addFprf.m_occursWithAddrCount;

    //изменяем размер массива на правильный
    arrType->setElementCount(m_fprintfs.size());

    //находим main
    SubroutineDeclaration* mainFunc = m_program->getGlobals().findSubroutine("main");
    if (!mainFunc)         return 2;
    BlockStatement& mainBodyBlock = mainFunc->getBodyBlock();

    //вставляем fileForTestingAliases = fopen("fileForTestingAliases.txt","wt");
    SubroutineDeclaration* fopen_sdec = globals.findSubroutine("fopen");
    if (fopen_sdec == 0) return 2;
    SubroutineReferenceExpression* fopen_sref = new SubroutineReferenceExpression(*fopen_sdec);
    SubroutineCallExpression* scallFopen = new SubroutineCallExpression(fopen_sref);
    StrictLiteralExpression* s1 = new StrictLiteralExpression(BasicType::BT_STRING);
    StrictLiteralExpression* s2 = new StrictLiteralExpression(BasicType::BT_STRING);
    s1->setString("fileForTestingAliases.txt");
    s2->setString("wt");
    scallFopen->addArgument(s1);
    scallFopen->addArgument(s2);
    ReferenceExpression* file_foropen = new ReferenceExpression(*file);
    BasicCallExpression* fopenAss = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN,file_foropen,scallFopen);
    ExpressionStatement* stmt_fopen = new ExpressionStatement(fopenAss);
    mainBodyBlock.addFirst(stmt_fopen);

    //вставляем fclose(fileForTestingAliases);
    SubroutineDeclaration* fclose_sdec = globals.findSubroutine("fclose");
    if (fclose_sdec == 0) return 2;
    SubroutineReferenceExpression* fclose_sref = new SubroutineReferenceExpression(*fclose_sdec);
    SubroutineCallExpression* scallFclose = new SubroutineCallExpression(fclose_sref);
    ReferenceExpression* file_forclose = new ReferenceExpression(*file);
    scallFclose->addArgument(file_forclose);
    ExpressionStatement* stmt_fclose = new ExpressionStatement(scallFclose);
    BlockStatement::Iterator it2 = mainBodyBlock.getLast();
    if (it2->is_a<ReturnStatement>()) --it2;
    mainBodyBlock.addAfter(it2,stmt_fclose);

    return 0;
}


//Запуск теста. Возможные результаты тестирования:
// 0 - анализатор вернул правильную инф.
// 1 - анализатор ошибся
// 2 - программа теста неправильная (файл не найден или ошибки компиляции/выполнения)
// 3 - во время анализа альясов выскочило исключение
int AliasAnalysisTester::runTest(const std::string fileName, std::string& outputMessage)
{
    clear();
    OPS::Frontend::Frontend frontend;  
    clang::ClangParserSettings& sets = frontend.clangSettings();
    sets.addIncludePath("test\\include");
    sets.defineMacro("__int64","long int");
    frontend.addSourceFile(fileName);
    try
    {
        frontend.compile();
    }
    catch (unsigned int) 
    {
        outputMessage = "There was an exception in frontend.compile()!";
        return 2;
    };
    const CompileResult& compileResult = frontend.getResult(0);
    if (compileResult.errorCount() != 0) 
    {
        outputMessage = compileResult.errorText();
        return 2;
    }
    m_program = &frontend.getProgramUnit().getUnit(0);
    
    //добавляем в программу fprintf для отладки
    int err = addFprintfToProgram();
    if (err > 0)
    {
        outputMessage = "Errors while adding fprintf to test program?";
        return 2;
    }

    SubroutineDeclaration* mainFunc = m_program->getGlobals().findSubroutine("main");
    if (!mainFunc) 
    {
        outputMessage = "I cant find function main???";
        return 2;
    }
    //проводит анализ альясов измененной программы и заполняет m_addresses
    err = buildAddresses(outputMessage);
    if (err > 0 ) return err;

    int dotPos = fileName.rfind('.');
    std::string fileName0 = fileName.substr(0,dotPos)+ "_fprintf";
    
    //записываем в файл измененную программу с fprintf
    std::ofstream fileWithFprintfs((fileName0 + ".c").c_str());
    Backends::OutToC writer(fileWithFprintfs);
    ConvertBoolToIntVisitor convertBoolToInt;
    m_program->accept(convertBoolToInt);// bool  int,   Microsoft 
    m_program->accept(writer);
    fileWithFprintfs.close();

    //записываем в xml файл внутреннее представление измененной программы
    std::ofstream xmlFprintfFile((fileName0 + ".xml").c_str());
    XmlBuilder builder(xmlFprintfFile);
    Backends::RepriseXml reprXml(builder);
    frontend.getProgramUnit().getUnit(0).accept(reprXml);
    xmlFprintfFile.close();

    //далее работаем с файлом после ручного редактирования
    //fileName0 += "1";

    //компилируем и запускаем измененную программу
    std::ofstream batFile(fileWithTestFileNames);
    batFile << "cl " + fileName0 + ".c /Fo" + fileName0 + ".obj /Fe" + fileName0+".exe";
    batFile.close();
    err = system(compileCommand);
    if (err != 0)
    {
        outputMessage = "Transformed program was not built!";
        return 2;
    }
    system((runTestCommandPrefix+fileName0+".exe").c_str());

    //удаляем obj и exe файлы
    system(("del "+fileName0+".obj").c_str());
    system(("del "+fileName0+".exe").c_str());
    
    err = parseFileForTestingAliases();
    if (err > 0)
    {
        outputMessage = m_outputMessage;
        return err;
    }
    err = checkResults();
    outputMessage = "Number of occurrences with address data = " + Strings::format("%d",m_occursWithAddrCount) + "\n" 
        + m_outputMessage;
    
    return err;
}

//проводит анализ альясов измененной программы и заполняет m_addresses
int AliasAnalysisTester::buildAddresses(std::string& outputMessage)
{
    int err;
    //  
    m_occurrenceContainer = new OccurrenceContainer(*m_program->findProgramUnit());
    m_aliasInterface = new AliasImplementation(*m_program->findProgramUnit(), *m_occurrenceContainer);
    try
    {
        err = m_aliasInterface->runAliasAnalysis();
    }
    catch (OPS::RuntimeError& err)
    {
        outputMessage = err.getMessage();
        return 3;
    }
    if (err == 1) 
    {
        outputMessage = "Canonical transformations were not done!";
        return 2;
    }    
    m_addresses.resize(m_fprintfs.size());
    for(size_t i = 0; i < m_fprintfs.size(); ++i)
    {
        m_addresses[i].resize(m_fprintfs[i]->getArgumentCount()-3);
        for(size_t j = 0; j < m_addresses[i].size(); ++j)
        {
            BasicOccurrencePtr o;
            err = m_occurrenceContainer->getOccurBy(m_fprintfs[i]->getArgument(j+3), o);
            if (err > 0) 
            {
                outputMessage = "Can't get occurrence in fprintf argument";
                return 2;
            }
            else
            {
                m_addresses[i][j] = o;
            }
        }
    }
    return 0;
}


int AliasAnalysisTester::parseFileForTestingAliases()
{
    char buffer[1000];
    //считываем числа из файла с числами-адресами
    std::ifstream numbersFile(((std::string)fileForTestingAliases+".txt").c_str());
    if (numbersFile.fail())
    {
        m_outputMessage = (std::string)"Cant find file with numbers: " + fileForTestingAliases+".txt";
        return 2;
    }    
    int i = 0;
    while (!numbersFile.eof())
    {
        std::string s;
        numbersFile.getline(buffer,1000);
        s = buffer;

        if (s.size() == 0) continue;
        
		int one_num, fprintfNum, spNum = 0;
		size_t lastSp = 0, newSp;

        //считаем количество пробелов
        for (size_t j = 0; j < s.size(); ++j)
            if (s[j] == ' ') spNum++;

        //выделяем номер fprintf
        newSp = s.find(' ');
        Strings::fetch(s.substr(lastSp,newSp),fprintfNum);
        lastSp = newSp;
		if (fprintfNum >= (int)m_numbers.size()) m_numbers.resize(fprintfNum+1);
        if (m_numbers[fprintfNum].size() == 0)
        {
            m_numbers[fprintfNum].resize(spNum);
        }
        else OPS_ASSERT((int)m_numbers[fprintfNum].size() == spNum);

        //распознаем содержимое вхождений
        int j = 0;
        while (newSp < s.size())
        {
            newSp = s.find(' ',lastSp+1);
			if (newSp == std::string::npos) newSp = s.size();
            Strings::fetch(s.substr(lastSp+1,newSp),one_num);
            m_numbers[fprintfNum][j].insert(one_num);
            ++j;
            lastSp = newSp;
        }        
        ++i;
    }
    return 0;
}

bool isIntersect(std::set<int>& s1, std::set<int>& s2)
{
    std::set<int> s(s1.begin(),s1.end());
    s.insert(s2.begin(),s2.end());
    return s.size() != (s1.size() + s2.size());
}

bool AliasAnalysisTester::isContentIntersect(BasicOccurrence& o1, BasicOccurrence& o2)
{
    SetAbstractMemoryCell s1(*m_aliasInterface->getMemoryCellContainer()), s2(*m_aliasInterface->getMemoryCellContainer());
    m_aliasInterface->getOccurrenceContent(o1,s1);
    m_aliasInterface->getOccurrenceContent(o2,s2);
    return s1.isIntersectWith(s2, true);
}

//сверяет результат работы теста с результатом анализатора альясов
int AliasAnalysisTester::checkResults()
{
    AliasImplementation* ai = m_aliasInterface;
    int intersectedCount = 0;
    std::string aliasContent;
    // выполняем проверку
    int result = 0;
    //ищем повторяющиеся числа
    for (size_t i = 0; i < m_numbers.size(); ++i)
    {
        for (size_t j = i; j < m_numbers.size(); ++j)
        {
            //сравниваем все вхождения в i-том и j-том fprintf
            for (size_t i_fpf = 0; i_fpf < m_numbers[i].size(); i_fpf++)
            {
                size_t j_fpf_min = i == j ? i_fpf+1 : 0;
                for (size_t j_fpf = j_fpf_min; j_fpf < m_numbers[j].size(); j_fpf++)
                {
                    if (isIntersect(m_numbers[i][i_fpf],m_numbers[j][j_fpf]))
                    {
                        intersectedCount++;
                        if (! isContentIntersect(*(m_addresses[i][i_fpf]), *(m_addresses[j][j_fpf])))
                        {
                            m_outputMessage += "Occurrences " + Strings::format("%d",i_fpf+1) + " and " +
                                Strings::format("%d",j_fpf+1) + " in fprintf's " + Strings::format("%d",i) +
                                " and " + Strings::format("%d",j) + 
                                " are not aliases! But numbers are equal!\n";
                            result = 1;
                            aliasContent += "SAMC of occurrences " + Strings::format("%d",i_fpf+1) + " and " +
                                Strings::format("%d",j_fpf+1) + " in fprintf's " + Strings::format("%d",i) +
                                " and " + Strings::format("%d",j) + 
                                " are: \n";
                            SetAbstractMemoryCell samc1(*ai->getMemoryCellContainer()), samc2(*ai->getMemoryCellContainer());
                            ai->getOccurrenceContent(*(m_addresses[i][i_fpf]),samc1);
                            ai->getOccurrenceContent(*(m_addresses[j][j_fpf]),samc2);
                            aliasContent += samc1.toString() + " and \n";
                            aliasContent += samc2.toString();
                        }
                    }
                }
            }
        }
    }
    if (m_outputMessage.size() == 0)  
    {
        m_outputMessage = "Intersected numbers count: " + Strings::format("%d",intersectedCount) + "\n";
        m_outputMessage += "Everything is all right!\n";
    }
    else
    {
        m_outputMessage += aliasContent;
    }
    return result;
}


//очистка
void AliasAnalysisTester::clear()
{
    m_addresses.clear();
    m_numbers.clear();
}


}//end of namespace
}//end of namespace

