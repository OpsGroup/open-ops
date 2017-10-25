#pragma once
/*
Здесь описан клас тестировщика анализатора альясов.
Тестирование происходит следующим образом.
В файле теста командами fprintf(fileForTestingAliases,...) печатается содержимое 
вхождений (печатаются адреса: &a, &s.b[i], или содержимое указателей).
Тестировщик компилирует и запускает программу. Затем в файле fileForTestingAliases.txt
ищутся ОДИНАКОВЫЕ числа и проверяется, являются ли соответствующие вхождения псевдонимами.

Один fprintf может срабатывать несколько раз или ни одного раза, но не больше maxFprintfCallCount 
(константа определена в AliasAnalysisTester.cpp). 
В файл записываются: № fprintf, и через пробел адресное содержимое вхождений (только адреса!!!)

Требования к файлам тестов - в файле с main должен быть подключен
#include <stdio.h>

Файлы тестов преобразуются следующим образом:
добавляется глобальная переменная файла для вывода содержимого вхождений
FILE* fileForTestingAliases;

В начале функции main файл открывается для записи
fileForTestingAliases = fopen("fileForTestingAliases.txt","wt");

В программе после каждого выражения, содержащего вхождения с адресным содержимым вставляется
(?? - № вставляемой функции fprintf)
if (fprintfCallCounts[??] <= maxFprintfCallCount)
    {
        fprintfCallCounts[??] = fprintfCallCounts[??] + 1;
        fprintf(fileForTestingAliases,"%d %d %d %d ...",??,...);
    }

В конце функции main нужно закрыть файл fclose(fileForTestingAliases)

Добавляется глобальный массив 
unsigned char fprintfCallCounts[суммарное кол-во fprintf];

Возможные результаты тестирования:
// 0 - анализатор вернул правильную инф. 
// 1 - анализатор ошибся 
// 2 - программа теста неправильная (файл не найден или ошибки компиляции/выполнения)
// 3 - во время анализа альясов выскочило исключение (например, если к программе не применялись 
// преобразования запутанных программ (см. docs/Montego))

*/

#include "Analysis/Montego/AliasAnalysis/AliasTypes.h"
#include "Analysis/Montego/Occurrence.h"

namespace OPS
{
namespace Montego
{

class BasicOccurrence;
class AliasImplementation;
class OccurrenceContainer;

class AliasAnalysisTester : public NonCopyableMix
{
public:

    AliasAnalysisTester();

    ~AliasAnalysisTester();

    //Запуск теста. Возможные результаты тестирования: 
    // 0 - анализатор вернул правильную инф. 
    // 1 - анализатор ошибся 
    // 2 - программа теста неправильная (файл не найден или ошибки компиляции/выполнения)
    // 3 - во время анализа альясов выскочило исключение (например, если к программе не применялись 
    // преобразования запутанных программ (см. docs/Montego))
    int runTest(const std::string fileName, std::string& outputMessage);

    //очистка
    void clear();

private:
    
    //расставляет в программе функции fprintf для вхождений, содержащих адресные данные
    //заполняет массив указателей m_fprintfs на добавленные функции
    //добавляет также глобальный массив, который обеспечивают небольшое количество запусков каждого fprintf
    int addFprintfToProgram();

    //сверяет результат работы теста с результатом анализатора альясов
    int checkResults();
    
    //считывает из файла результат работы тестируемой программы
    int parseFileForTestingAliases();

    //проводит анализ альясов измененной программы и заполняет m_addresses
    int buildAddresses(std::string& outputMessage);

    //проверяет, пересекается ли содержимое вхождений, посчитанное анализатором альясов
    bool isContentIntersect(BasicOccurrence& o1, BasicOccurrence& o2);

    //вхождения в соответствующих fprintf
    std::vector< std::vector<BasicOccurrencePtr> > m_addresses;

    //содержимое вхождений в fprintf, посчитанное самой программой
    //т.к. fprintf могут прорабатывать несколько раз, то для каждого получим несолько чисел
    std::vector< std::vector< std::set<int> > > m_numbers;

    //тестируемая программа
    Reprise::TranslationUnit* m_program;

    std::vector<Reprise::SubroutineCallExpression*> m_fprintfs;

    OccurrenceContainer* m_occurrenceContainer;

    AliasImplementation* m_aliasInterface;

    std::string m_outputMessage;

    int m_occursWithAddrCount;

};

}//end of namespace
}//end of namespace

