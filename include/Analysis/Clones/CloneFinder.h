#include "Analysis/Clones/HashDeepWalker.h"

#include <iostream>
#include <map>
#include <set>

namespace OPS {

namespace Clones {

class CloneFinder {
    size_t MassThreshold;
    double SimilarityThreshold;

public:
    struct Clone {
        int size;
        set<shared_ptr<HashDeepWalker::SubTreeInfo>> refs;
        Clone() {}
        Clone(int s) :size(s), refs() {}
    };
private:

    map<size_t, Clone> clones;

    int basicTreeCompare(shared_ptr <HashDeepWalker::SubTreeInfo> t1, shared_ptr <HashDeepWalker::SubTreeInfo> t2);
    int isSeqSimilar(shared_ptr < HashDeepWalker::SubTreeInfo> t1, shared_ptr < HashDeepWalker::SubTreeInfo> t2);
    bool isSimilar(shared_ptr <HashDeepWalker::SubTreeInfo> t1, shared_ptr <HashDeepWalker::SubTreeInfo> t2);
    void cloneGeneralizing();
    void eraseByHash(shared_ptr < HashDeepWalker::SubTreeInfo> n);
    void eraseChildClones(shared_ptr < HashDeepWalker::SubTreeInfo> root);
    void eraseSubClones();
    void addClonePair(shared_ptr < HashDeepWalker::SubTreeInfo> s1, shared_ptr < HashDeepWalker::SubTreeInfo> s2);

public:
    CloneFinder(size_t mt, double st) : MassThreshold(mt), SimilarityThreshold(st) {
        clones = map<size_t, Clone>();
    }

    vector<Clone> getClones(TranslationUnit& unit, bool removeSubClones=false);
};
        
}
}
