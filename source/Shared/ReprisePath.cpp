#include "Shared/ReprisePath.h"
#include <sstream>

using namespace OPS::Reprise;

OPS::Shared::ReprisePath OPS::Shared::makePath(const OPS::Reprise::RepriseBase &obj,
											   const OPS::Reprise::RepriseBase* root)
{
	ReprisePath path;

	const RepriseBase* current = &obj;
	RepriseBase* parent = obj.getParent();

	while(parent != 0 && current != root)
	{
		const int childCount = parent->getChildCount();
		bool found = false;

		for(int i = 0; i < childCount && !found; ++i)
		{
			if (&parent->getChild(i) == current)
			{
				path.push_front(i);
				found = true;
			}
		}

		if (found == false)
			throw OPS::RuntimeError("Child node not found");

		current = parent;
		parent = parent->getParent();
	}

	return path;
}

RepriseBase* OPS::Shared::findByPath(RepriseBase& root, ReprisePath path)
{
	RepriseBase* current = &root;

	for( ;!path.empty() && current != 0; path.pop_front())
	{
		int child = path.front();

		if (child >= 0 && child < current->getChildCount())
			current = &current->getChild(child);
		else
			throw InvalidReprisePath("Node '" + current->dumpState() + "' has no children " + OPS::Strings::format("%d", child));
	}

	return current;
}

OPS::Shared::ReprisePath OPS::Shared::makePathFromString(const std::string &path)
{
	ReprisePath res;
	if (path.empty())
		return res;

	std::stringstream ss(path);

	while(!ss.eof())
	{
		std::string token;
		std::getline(ss, token, ':');

		int child = 0;
		if (OPS::Strings::fetch(token, child))
			res.push_back(child);
		else
			throw InvalidReprisePath("Token is not a number:" + token);
	}

	return res;
}

std::string OPS::Shared::makeStringFromPath(const ReprisePath &path)
{
	std::string res;
	for(ReprisePath::const_iterator it = path.begin(); it != path.end(); ++it)
	{
		res += (it == path.begin() ? "" : ":") + OPS::Strings::format("%d", *it);
	}

	return res;
}

std::ostream& OPS::Shared::operator<<(std::ostream& os, const ReprisePath& path)
{
	os << OPS::Shared::makeStringFromPath(path);
	return os;
}
