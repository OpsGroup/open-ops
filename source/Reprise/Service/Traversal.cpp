#include "Reprise/Service/Traversal.h"

namespace OPS
{
namespace Reprise
{
namespace Service
{

class VisitorAction
{
public:
    VisitorAction(OPS::BaseVisitor& visitor)
        :m_visitor(visitor)
    {
    }

    void operator()(RepriseBase& node)
    {
        node.accept(m_visitor);
    }

private:
    OPS::BaseVisitor& m_visitor;
};

void makePreOrderVisitorTraversal(RepriseBase& root, OPS::BaseVisitor& visitor)
{
    VisitorAction act(visitor);
    makePreOrderTraversal(root, act);
}

// Visit subtrees first
void makePostOrderVisitorTraversal(RepriseBase& root, OPS::BaseVisitor& visitor)
{
    VisitorAction act(visitor);
    makePostOrderTraversal(root, act);
}

}
}
}
