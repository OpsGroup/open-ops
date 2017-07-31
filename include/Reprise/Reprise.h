#ifndef OPS_IR_REPRISE_REPRISE_H__
#define OPS_IR_REPRISE_REPRISE_H__

/**
\mainpage Reprise (Intermediate Representation)

\author Victor Petrenko (vpetrenko@gmail.com)

\version 0.2.0 от 20.11.2009

\section concept1 Концепция построения иерархии классов
    


\section concept2 Методология создания и удаления объектов внутреннего представления
\subsection method_create Методология создания

\section history История изменений (History)

Добавлена схема управления памятью через Master/Track Ptrs.

 - Версия 0.2.0 от 20.11.2009 - Много мелких добавлений и изменений. Регенерация документации.
 - Версия 0.1.0 от 10.03.2009 - Добавлено Reprise::Service. Практически завершено формирование дерева узлов Reprise.
 - Версия 0.0.1 от 09.11.2008 - Пробная версия с Lifetime управлением памятью.

*/


//  Standard includes

//  Local includes
#include "Reprise/Collections.h"
#include "Reprise/Common.h"
#include "Reprise/Declarations.h"
#include "Reprise/Exceptions.h"
#include "Reprise/Expressions.h"
#include "Reprise/Layouts.h"
#include "Reprise/ProgramFragment.h"
#include "Reprise/Statements.h"
#include "Reprise/Types.h"
#include "Reprise/Units.h"
#include "Reprise/Utils.h"

#include "Reprise/ServiceFunctions.h"

#endif                      // OPS_IR_REPRISE_REPRISE_H__
