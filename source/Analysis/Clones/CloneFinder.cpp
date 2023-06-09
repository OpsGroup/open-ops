#include "Analysis/Clones/HashDeepWalker.h"
#include "Analysis/Clones/CloneFinder.h"

#include <iostream>
#include <map>
#include <set>

namespace OPS {

	namespace Clones {


		int CloneFinder::basicTreeCompare(shared_ptr <HashDeepWalker::SubTreeInfo> t1, shared_ptr <HashDeepWalker::SubTreeInfo> t2)
		{
			int shared = 0;

			if (t1->node->is_a<BlockStatement>() && t2->node->is_a<BlockStatement>())
			{
				return isSeqSimilar(t1,t2);
			}
			else if (typeid(*t1->node) == typeid(*t2->node))
			{
				shared++;
			}

			int minChildren = min(t1->children.size(), t2->children.size());
			for (int i = 0; i < minChildren; i++)
				shared += this->basicTreeCompare(t1->children[i], t2->children[i]);

			return shared;
		}

		int CloneFinder::isSeqSimilar(shared_ptr < HashDeepWalker::SubTreeInfo> t1, shared_ptr < HashDeepWalker::SubTreeInfo> t2)
		{
			set<int> t1UsedInd = set<int>();
			set<int> t2UsedInd = set<int>();

			double sharedNodes = 0;

			for (int i = 0; i < t1->children.size(); i++)
			{
				for (int j = 0; j < t2->children.size(); j++)
				{
					if (t1->children[i]->hashCode == t2->children[j]->hashCode)
					{
						sharedNodes += basicTreeCompare(t1->children[i], t2->children[j]);
						t1UsedInd.insert(i);
						t2UsedInd.insert(j);
						break;
					}

				}
			}

			for (int i = 0; i < t1->children.size(); i++)
			{
				if (t1UsedInd.find(i) != t1UsedInd.end())
				{
					continue;
				}

				for (int j = 0; j < t2->children.size(); j++)
				{
					if (t2UsedInd.find(j) != t2UsedInd.end())
					{
						continue;
					}

					sharedNodes += basicTreeCompare(t1->children[i], t2->children[j]);
					break;
				}
			}

			return sharedNodes;
		}

		bool CloneFinder::isSimilar(shared_ptr <HashDeepWalker::SubTreeInfo> t1, shared_ptr <HashDeepWalker::SubTreeInfo> t2)
		{
			double sharedNodes = (double)basicTreeCompare(t1, t2);
			double similarity = (2 * sharedNodes) / (2 * sharedNodes + (t1->subTreeSize - sharedNodes) + (t2->subTreeSize - sharedNodes));
			return similarity > SimilarityThreshold;
		}

		void CloneFinder::cloneGeneralizing()
		{
			for (auto& clone : clones)
			{
				for (auto& i : clone.second.refs)
				{
					for (auto& j : clone.second.refs)
					{
						if (i != j)
						{
							if (i->parent && j->parent)
							{
								if (isSimilar(i->parent, j->parent))
								{
									if (clones.find(i->hashCode) == clones.end())
									{
										clones[i->hashCode] = Clone(i->subTreeSize);
									}
									clones[i->hashCode].refs.insert(i);
									clones[i->hashCode].refs.insert(j);
									clone.second.refs.erase(i);
									clone.second.refs.erase(j);
								}
							}
						}
					}
				}
			}
		}

		void CloneFinder::eraseByHash(shared_ptr < HashDeepWalker::SubTreeInfo> n)
		{
			if (clones.find(n->hashCode) != clones.end())
			{
				clones[n->hashCode].refs.erase(n);
				if (clones[n->hashCode].refs.size() == 1)
					clones[n->hashCode].refs.clear();
			}
			eraseChildClones(n);
		}

		void CloneFinder::eraseChildClones(shared_ptr < HashDeepWalker::SubTreeInfo> root)
		{
			for (int i = 0; i < root->children.size(); i++)
				eraseByHash(root->children[i]);
		}

		void CloneFinder::eraseSubClones()
		{
			for (auto& clone : clones)
			{
				for (auto& ref : clone.second.refs)
				{
					eraseChildClones(ref);
				}
			}
		}

		void CloneFinder::addClonePair(shared_ptr < HashDeepWalker::SubTreeInfo> s1, shared_ptr < HashDeepWalker::SubTreeInfo> s2)
		{
			if (clones.find(s1->hashCode) == clones.end())
			{
				clones[s1->hashCode] = Clone(s1->subTreeSize);
			}
			clones[s1->hashCode].refs.insert(s1);
			clones[s1->hashCode].refs.insert(s2);
		}

		vector<CloneFinder::Clone> CloneFinder::getClones(TranslationUnit& unit, bool removeSubClones)
		{
			HashDeepWalker hdw;
			hdw.visit(unit);
			auto buckets = hdw.getBuckets(MassThreshold);

			for (auto& bucket : buckets)
			{
				for (int i = 0; i < bucket.second.size(); i++)
				{
					for (int j = i + 1; j < bucket.second.size(); j++)
					{
						if (isSimilar(bucket.second[i], bucket.second[j]))
						{
							//cout << "Seems like clone found" << endl;
							addClonePair(bucket.second[i], bucket.second[j]);
						}
					}
				}
			}

			if (removeSubClones)
				eraseSubClones();

			vector<Clone> cloneLst = vector<Clone>();

			for (auto& c : clones)
			{
				cloneLst.push_back(c.second);
			}

			return cloneLst;
		}
	}
}