#ifndef OPS_COMMON_CORE_IO_H_
#define OPS_COMMON_CORE_IO_H_

#include <string>

namespace OPS
{
namespace IO
{

/**
	Convert OS-specific path to POSIX path.
*/
std::string osPathToPosixPath(const std::wstring& path);

/**
	Convert posix path to OS-specific path.
*/
std::wstring posixPathToOsPath(const std::string& path);

/**
	Check that path is full path name from root.
*/
bool isPathRooted(const std::wstring& path);

/**
    Returns base name of path (G:\archive.tar.gz, "archive" will returned)
*/
std::wstring getFileBaseName(const std::wstring& path);
std::string getFileBaseName(const std::string& path);

/**
    Returns name of path (G:\archive.tar.gz, "archive.tar.gz" will returned)
*/
std::wstring getFileName(const std::wstring& path);
std::string  getFileName(const std::string &path);

/**
	Combines two parts of path
*/
std::wstring combinePath(const std::wstring& basePath, const std::wstring& path);

bool isDirExists(std::string dir);

bool isFileExists(std::string fileName);

std::string readFile(std::string fileName);

void writeFile(std::string fileName, std::string content);

void deleteFile(std::string fileName);

std::string getOpsTmpFolder();

//files creating in tmp/OPS_Temp folder
//if file startFileName.extension doesn't exists, creates it
//extension examples: "txt", "exe", "c"
std::string getTmpFileName(std::string extension, std::string startFileName = "");

}
}


#endif				// OPS_COMMON_CORE_IO_H_

