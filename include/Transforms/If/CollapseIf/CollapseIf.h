#ifndef COLLAPSE_IF_H
#define COLLAPSE_IF_H

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
/**
     * @brief CollapseIf
     * convert IfStatement to simple ExpressionStatement
     * @param ifstatement
     * @return
     * new ExpressionStatement - result of transformation; 0 if cannot transform
     */
    Reprise::ExpressionStatement* collapseIfToAssign(Reprise::IfStatement* ifstatement);
}
}

#endif //COLLAPSE_IF_H
