#include "Reprise/Layouts.h"

//	Enter namespace
namespace OPS
{
namespace Reprise
{

SourceCodeManager::SourceCodeManager(RepriseContext& context) : m_context(&context)
{
}

int SourceCodeManager::registerFile(const char* path)
{
	TIdsMap::const_iterator fileId = m_idsMap.find(path);
	if (fileId == m_idsMap.end())
	{
		m_files.push_back(path);
		m_idsMap.insert(std::make_pair(path, m_files.size() - 1));
		return m_files.size() - 1;
	}
	return fileId->second;
}

int SourceCodeManager::getFileId(const char* path) const
{
	TIdsMap::const_iterator fileId = m_idsMap.find(path);
	if (fileId == m_idsMap.end())
	{
		return -1;
	}
	return fileId->second;
}

std::string SourceCodeManager::getFilePath(int id) const
{
	if (id < 0 || id >= static_cast<int>(m_files.size()))
		return 0;
	return m_files[id];
}

int SourceCodeManager::addLocation(const SourceCodeLocation& location)
{
	m_locations.push_back(location);
	return m_locations.size() - 1;
}

SourceCodeLocation SourceCodeManager::getLocation(const RepriseBase& node) const
{
	const int location = node.getLocationId();
	if (location < 0 || location >= static_cast<int>(m_locations.size()))
		return SourceCodeLocation();
	return m_locations[location];
}


}
}
