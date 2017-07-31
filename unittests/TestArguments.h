#include <list>
#include <string>
#include <algorithm>
#include "Reprise/Service/DeepWalker.h"

#include "Reprise/Reprise.h"


namespace {
	template<typename ResultType>
	class TestArgsWalker: public OPS::Reprise::Service::DeepWalker
	{
	private:
		std::list<ResultType*> m_collection;
	public:
		TestArgsWalker() {}

		std::list<ResultType*>& getTestArguments() { return m_collection; }

		void visit(ResultType& node)
		{
			if(node.hasNote("test_argument"))
			{
				m_collection.push_back(&node);
			}
		}
	};

	struct ArgumentFinder
	{
	private:
		std::string m_nameToFind;
	public:
		ArgumentFinder(std::string& argumentName): m_nameToFind(argumentName) {}

		bool operator()(OPS::Reprise::RepriseBase* r)
		{
			OPS_ASSERT(r->hasNote("test_argument"));
			OPS::Reprise::Note& n = r->getNote("test_argument");
			OPS_ASSERT(n.getKind() == OPS::Reprise::Note::NK_STRING);
			return (n.getString() == m_nameToFind);
		}
	};
}

namespace OPS
{
	namespace UnitTests
	{
		template<typename ResultType>
		ResultType* findTestArgument(OPS::Reprise::RepriseBase& rootNode, std::string argumentName);


		template<typename ResultType>
		bool findTestArgument(OPS::Reprise::RepriseBase& rootNode, std::string argumentName, ResultType*& result)
		{
			TestArgsWalker<ResultType> w;
			rootNode.accept(w);
			std::list<ResultType*>& arguments = w.getTestArguments();
			typename std::list<ResultType*>::iterator pResult = std::find_if(arguments.begin(), arguments.end(), ArgumentFinder(argumentName));
			if(pResult != arguments.end())
			{
				result = *pResult;
				return true;
			}
			return false;
		}
	}
}
