#ifndef OPS_IR_REPRISE_LAYOUTS_H_INCLUDED__
#define OPS_IR_REPRISE_LAYOUTS_H_INCLUDED__

//  Standard includes
#include <string>
#include <vector>
#include <map>

//  OPS includes

//  Local includes
#include "Reprise/Common.h"

//	Enter namespaces
namespace OPS
{
namespace Reprise
{

///	Class to hold information about source code positions.
struct SourceCodeLocation
{
	typedef std::pair<int, int> LocationPair;

	SourceCodeLocation(void) 
        : FileId(-1), HeadIncludeFileId(-1)
	{}

    SourceCodeLocation(int fileId, int headIncludeFileId, int rowStart, int rowEnd, int colStart, int colEnd)
        : FileId(fileId),
          HeadIncludeFileId(headIncludeFileId),
          Row(std::make_pair(rowStart, rowEnd)),
          Column(std::make_pair(colStart, colEnd))
	{}
    SourceCodeLocation(int fileId, int headIncludeFileId, const LocationPair& row, const LocationPair& column)
        : FileId(fileId), HeadIncludeFileId(headIncludeFileId), Row(row), Column(column)
	{}

	int FileId;
    int HeadIncludeFileId; //for printing only head includes in OutToC
	LocationPair Row;
	LocationPair Column;
};

class SourceCodeManager
{
public:
	explicit SourceCodeManager(RepriseContext& context);

	int registerFile(const char* path);
	int getFileId(const char* path) const;
	std::string getFilePath(int id) const;

	int addLocation(const SourceCodeLocation& location);

	SourceCodeLocation getLocation(const RepriseBase& node) const;

private:
	typedef std::vector<std::string> TSourceFileList;
	typedef std::map<std::string, int> TIdsMap;
	typedef std::vector<SourceCodeLocation> TSourceLocationList;

	TSourceFileList m_files;
	TIdsMap m_idsMap;
	TSourceLocationList m_locations;

	RepriseContext* m_context;
};

//	Exit namespaces
}
}

#endif                      // OPS_IR_REPRISE_LAYOUTS_H_INCLUDED__
