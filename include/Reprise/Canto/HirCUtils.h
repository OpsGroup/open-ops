#ifndef OPS_IR_REPRISE_CANTO_HIRCUTILS_H_INCLUDED__
#define OPS_IR_REPRISE_CANTO_HIRCUTILS_H_INCLUDED__

#include "Reprise/Expressions.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{

//	Global functions
ReprisePtr<StrictLiteralExpression> HirCdecodeLiteral(const std::string& literal);

}
}
}

#endif                      // OPS_IR_REPRISE_CANTO_HIRCUTILS_H_INCLUDED__
