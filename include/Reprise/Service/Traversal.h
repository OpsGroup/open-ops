#ifndef OPS_IR_REPRISE_SERVICE_TRAVERSAL_H_INCLUDED__
#define OPS_IR_REPRISE_SERVICE_TRAVERSAL_H_INCLUDED__

#include "Reprise/Common.h"

namespace OPS
{
namespace Reprise
{
namespace Service
{

// Visit root first
template<typename Action>
    void makePreOrderTraversal(RepriseBase& root, Action& action)
{
    action(root);

    const int childCount = root.getChildCount();
    for(int i = 0; i < childCount; ++i)
    {
        makePreOrderTraversal(root.getChild(i), action);
    }
}

// Visit subtrees first
template<typename Action>
    void makePostOrderTraversal(RepriseBase& root, Action& action)
{
    const int childCount = root.getChildCount();
    for(int i = 0; i < childCount; ++i)
    {
        makePostOrderTraversal(root.getChild(i), action);
    }

    action(root);
}

void makePreOrderVisitorTraversal(RepriseBase& root, OPS::BaseVisitor& visitor);

void makePostOrderVisitorTraversal(RepriseBase& root, OPS::BaseVisitor& visitor);

}
}
}


#endif // OPS_IR_REPRISE_SERVICE_TRAVERSAL_H_INCLUDED__
