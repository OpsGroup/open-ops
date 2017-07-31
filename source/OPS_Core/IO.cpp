#include "OPS_Core/IO.h"
#include "OPS_Core/Exceptions.h"
#include "OPS_Core/Strings.h"
#if OPS_PLATFORM_IS_WIN32
#include <direct.h>
#endif
#include <fstream>
#include <streambuf>
#include <sys/stat.h>
#include "OPS_Core/Exceptions.h"
#include <stdlib.h>

namespace OPS
{
namespace IO
{

std::wstring getFileName(const std::wstring& path)
{
	std::wstring::size_type start = path.find_last_of(L"\\/");
	if (start == std::wstring::npos)
		start = 0;
	else
		start += 1;
	return path.substr(start);
}

std::string getFileName(const std::string& path)
{
    std::string::size_type start = path.find_last_of("\\/");
    if (start == std::string::npos)
        start = 0;
    else
        start += 1;
    return path.substr(start);
}

std::wstring getFileBaseName(const std::wstring& path)
{
	std::wstring::size_type start = path.find_last_of(L"\\/");
	if (start == std::wstring::npos)
		start = 0;
	else
		start += 1;
	std::wstring::size_type end = path.find_first_of(L".", start);
	if (end == std::wstring::npos)
		end = path.size();
	return path.substr(start, end - start);
}

std::string getFileBaseName(const std::string& path)
{
    std::string fileName = getFileName(path);
    std::string::size_type end = fileName.find_first_of('.');
    if (end == std::string::npos)
        return fileName;
    else
        return fileName.substr(0, end);
}


#if OPS_PLATFORM_IS_WIN32
std::string osPathToPosixPath(const std::wstring& path)
{
    return Strings::narrow_mbcs(path);
}

std::wstring posixPathToOsPath(const std::string& path)
{
    return Strings::widen_mbcs(path);
}

bool isPathRooted(const std::wstring& path)
{
    if (path.empty())
        return false;
    return path[0] == L'\\' || path[0] == L'/' || path.size() > 1 && path[1] == L':' && 
		(path[0] >= L'A' && path[0] <= L'Z' || path[0] >= L'a' && path[0] <= L'z');
}

std::wstring combinePath(const std::wstring& base_path, const std::wstring& path)
{
    if (base_path.empty() || isPathRooted(path))
        return path;
    if (path.empty())
        return base_path;
    const wchar_t trail = base_path[base_path.size() - 1];
    if (trail != L'\\' && trail != L'/')
        return base_path + L'\\' + path;
    return base_path + path;
}


#elif OPS_PLATFORM_IS_UNIX
std::string osPathToPosixPath(const std::wstring& path)
{
#if OPS_PLATFORM_FLAVOR_IS_MACOSX
    const std::basic_string<ucs2char_t>& ucs2_path = Strings::to_ucs2(path);
    std::string fs_path;
    const CFStringRef path_ref = ::CFStringCreateWithCharactersNoCopy(0, ucs2_path.c_str(), ucs2_path.size(), kCFAllocatorNull);
    if (path_ref != 0)
    {
    	CFURLRef url_ref;
        try
        {
            url_ref = ::CFURLCreateWithFileSystemPath(0, path_ref, kCFURLPOSIXPathStyle, FALSE);
        }
        catch (...)
        {
            ::CFRelease(path_ref);
            throw;
        }
        ::CFRelease(path_ref);
        if (url_ref != 0)
        {
            try
            {
                std::valarray<UInt8> buffer(PATH_MAX + 1);
	            while (::CFURLGetFileSystemRepresentation(url_ref, FALSE, &buffer[0], buffer.size()) == FALSE)
                    buffer.resize(buffer.size() + PATH_MAX);
                fs_path = reinterpret_cast<char*>(&buffer[0]);
            }
            catch (...)
            {
                ::CFRelease(url_ref);
                throw;
            }
            ::CFRelease(url_ref);
        }
    }
    if (fs_path.empty())
        fs_path = Strings::narrow_mbcs(path);
    return fs_path_to_posix_path(fs_path);
#else
    return Strings::narrow_mbcs(path);
#endif
}

std::wstring posixPathToOsPath(const std::string& path)
{
#if OPS_PLATFORM_FLAVOR_IS_MACOSX
    std::wstring os_path;
    const CFURLRef url_ref = 
        ::CFURLCreateFromFileSystemRepresentation(0, reinterpret_cast<const UInt8*>(path.c_str()), path.size(), FALSE);
    if (url_ref != 0)
    {
        try
        {
            const CFStringRef string_ref = ::CFURLCopyFileSystemPath(url_ref, kCFURLPOSIXPathStyle);
	        try
    	    {
	    	    CFIndex length = ::CFStringGetLength(string_ref);
		        if (length > 0)
	            {
		            std::basic_string<ucs2char_t> buffer(length, '\0');
		            ::CFStringGetCharacters(string_ref, ::CFRangeMake(0, length), &buffer[0]);
	                os_path = Strings::from_ucs2(buffer);
	            }
	        }
	        catch (...)
	        {
    	        ::CFRelease(string_ref);
    	        throw;
	        }
	        ::CFRelease(string_ref);
        }
        catch (...)
        {
            ::CFRelease(url_ref);
            throw;
        }
        ::CFRelease(url_ref);
    }
    if (os_path.empty())
        os_path = Strings::widen_mbcs(path);
    return os_path;
#else
    return Strings::widen_mbcs(path);
#endif
}

bool isPathRooted(const std::wstring& path)
{
    return !path.empty() && path[0] == L'/';
}


std::wstring combinePath(const std::wstring& base_path, const std::wstring& path)
{
    if (base_path.empty() || isPathRooted(path))
        return path;
    if (path.empty())
        return base_path;
    if (base_path[base_path.size() - 1] != L'/')
        return base_path + L'/' + path;
    return base_path + path;
}


#else
#error Unsupported platform
#endif

bool isUnix()
{
    #if OPS_PLATFORM_IS_UNIX
    return true;
    #else
    return false;
    #endif
}

bool is32bit()
{
    return sizeof(size_t) == 4;
}

bool isDirOrFileExists(std::string name)
{
    //remove trailing slash
    if (name[name.size()-1] == '/' || name[name.size()-1] == '\\')
        name = name.substr(name.size()-1);
    struct stat st;
    return stat(name.c_str(),&st) == 0;
}

bool isDirExists(std::string dir)
{
    //remove trailing slash
    if (dir[dir.size()-1] == '/' || dir[dir.size()-1] == '\\')
        dir = dir.substr(dir.size()-1);
    bool result = false;
    struct stat st;
    if(stat(dir.c_str(),&st) == 0)
        if ((st.st_mode & S_IFDIR) != 0) result = true;
    return result;
}

bool isFileExists(std::string fileName)
{
    if (isDirExists(fileName)) return false;

    std::ifstream f(fileName.c_str());
    if (f.is_open())
    {
        f.close();
        return true;
    }
    else return false;
}

std::string readFile(std::string fileName)
{
    std::ifstream t(fileName.c_str());
    std::string result((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    return result;
}
void writeFile(std::string fileName, std::string content)
{
    std::ofstream f(fileName.c_str());
    f << content;
    f.close();
}

static std::string OPS_TEMP_FOLDER = "";

std::string getOpsTmpFolder()
{
    if (!OPS_TEMP_FOLDER.empty()) return OPS_TEMP_FOLDER;
    std::string slash = isUnix() ? "/" : "\\";
    //getting temp directory
    std::string tmpDir;
    if (isUnix())
    {
        const char *tryTmp[] = {"/tmp", "/temp"};
		for (size_t i = 0; i < sizeof(tryTmp)/sizeof(const char*); i++)
            if (isDirExists(tryTmp[i])) {tmpDir = tryTmp[i]; break;}
    }
    else
    {
        const char *tryTmp[] = {"TMPDIR", "TEMP", "TMP"};
        std::string sysDrive = std::string(getenv ("SystemDrive"));
		if (sysDrive[1] == ':')
			sysDrive = sysDrive.substr(0,1);
        if (sysDrive.empty()) sysDrive = "C";
		for (size_t i = 0; i < sizeof(tryTmp)/sizeof(const char*); i++)
        {
            std::string tryDir = sysDrive + ":\\" + tryTmp[i];
            if (isDirExists(tryDir)) {tmpDir = tryDir; break;}
        }
    }
    //trying environment
    if (tmpDir.empty())
    {
        const char *envName[] = {"TMPDIR", "TEMP", "TMP"};
        char * rab = NULL;
        int num = sizeof(envName)/sizeof(const char*);
        for (int i = 0; i < num; i++)
        {
            rab = getenv(envName[i]);
            if (rab != NULL && isDirExists(rab)) break;
            else rab = NULL;
        }
        if (rab != NULL)  tmpDir = rab;
    }
    if (tmpDir.empty()) throw RuntimeError("Can't get tmp dir");
    if (tmpDir[tmpDir.size()-1] != slash[0])  tmpDir += slash;
    //cout << "tmpDir = " << tmpDir << "\n";

    std::string opsTempDir = tmpDir + "OPS_Temp";
    if (isFileExists(opsTempDir))
    {
        opsTempDir = tmpDir + "OPS_Temp_" + Strings::format("%d",rand()%10000);
        int i = 0;
        while (isFileExists(opsTempDir))
        {
            opsTempDir = tmpDir + "OPS_Temp_" + Strings::format("%d",rand()%10000);
            i++;
            if (i>100) throw RuntimeError("Can't get tmp folder name");
        }
    }
    if (!isDirExists(opsTempDir))
    {
#if OPS_PLATFORM_IS_UNIX
        mkdir(opsTempDir.c_str(), 0777);
        system(("chmod 777 " + opsTempDir).c_str());
#else
        mkdir(opsTempDir.c_str());
#endif
    }
#if OPS_PLATFORM_IS_UNIX
    else system(("chmod 777 " + opsTempDir).c_str());
#endif
    OPS_TEMP_FOLDER = opsTempDir;
    return opsTempDir;
}

std::string getTmpFileName(std::string extension, std::string startFileName)
{
    std::string slash = isUnix() ? "/" : "\\";
    std::string opsTempDir = getOpsTmpFolder();
    opsTempDir += slash;
    //get not existent file name
    std::string start = startFileName.empty() ? "tmp" : startFileName;
    std::string tempFileName = opsTempDir + start + "." + extension;
    int i = 0;
    while (isDirOrFileExists(tempFileName))
    {
        tempFileName = opsTempDir + start + "_" + Strings::format("%d",rand()%10000) + "." + extension;
        i++;
        if (i>100) throw RuntimeError("Can't get tmp file name");
    }
    return tempFileName;
}

void deleteFile(std::string fileName)
{
	if (fileName.empty()) return;
    if (isFileExists(fileName))
        remove(fileName.c_str());
}

}
}
