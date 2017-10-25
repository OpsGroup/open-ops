#ifndef LOOP_NEST_UNIFICATION
#define LOOP_NEST_UNIFICATION

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{

// From

//  for(i = i0; i < n; ++i)
//      for(j = j0; j < m; ++j)
//      {
//          Block;
//      }

//  to

//  for(s = 0; s < (n - i0) * (m - j0); ++s)
//  {
//      i = i0 + s / (m - j0);
//      j = j0 + s % (m - j0);
//      {
//          Block;
//      }
//  }

OPS::Reprise::ForStatement& loopNestUnification(OPS::Reprise::ForStatement &oldLoop, int maxDepth = 2);

}   //Loops
}   //Transforms
}   //OPS
#endif
