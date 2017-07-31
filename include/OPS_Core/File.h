#ifndef OPS_COMMON_CORE_FILE_H_
#define OPS_COMMON_CORE_FILE_H_

#include <string>
#include <vector>

namespace OPS
{

class FilesCollector
{
public:
	typedef std::vector<std::string> List;

	explicit FilesCollector(const std::string& mask = "*.*");

	void getFilesRecursive(const std::string& path, FilesCollector::List& files) const;

	void setMask(const std::string& mask);

	std::string getMask() const;

private:

	std::string m_mask;
};




}


#endif				// OPS_COMMON_CORE_FILE_H_

