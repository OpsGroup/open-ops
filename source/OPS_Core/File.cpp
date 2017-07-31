#include "OPS_Core/File.h"
#include "OPS_Core/IO.h"
#include "OPS_Core/Exceptions.h"
#include "OPS_Core/Strings.h"

#if OPS_PLATFORM_IS_WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#elif OPS_PLATFORM_IS_UNIX
#include <glob.h>
#endif

namespace OPS
{

FilesCollector::FilesCollector(const std::string& mask) : m_mask(mask)
{
}

#if OPS_PLATFORM_IS_WIN32
void FilesCollector::getFilesRecursive(const std::string& path, FilesCollector::List& files) const
{
	if (path.empty())
		return;
	std::wstring findOsPath = IO::posixPathToOsPath(path);
	WIN32_FIND_DATA fd;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	if (findOsPath[findOsPath.size() - 1] != L'\\' && findOsPath[findOsPath.size() - 1] != L'/')
		findOsPath += L"/";

	hFind = ::FindFirstFileW((findOsPath + L"*.*").c_str(), &fd);
	try
	{
		if (hFind != INVALID_HANDLE_VALUE) 
		{
			do 
			{
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
				{
					const std::wstring currentName(fd.cFileName);
					if (currentName != L"." && currentName != L"..")
						getFilesRecursive(Strings::narrow(findOsPath + currentName), files);
				}
			} while (::FindNextFile(hFind, &fd) != 0);
		}

		hFind = ::FindFirstFileW((findOsPath + Strings::widen(m_mask)).c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE) 
		{
			do 
			{
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					files.push_back(std::string(OPS::Strings::narrow(findOsPath + fd.cFileName)));
				}
			} while (::FindNextFile(hFind, &fd) != 0);
		}
	}
	catch (...) 
	{
		::FindClose(hFind);
		throw;
	}
	::FindClose(hFind);
}
#elif OPS_PLATFORM_IS_UNIX
void FilesCollector::getFilesRecursive(const std::string& path, FilesCollector::List& files) const
{
	if (path.empty())
		return;

	std::string basePath = path;
	if (basePath[basePath.size() - 1] != '/')
		basePath += "/";

	{
		glob_t handle;
		const int res= glob((basePath + "*").c_str(), GLOB_MARK, 0, &handle);
		if (res== 0)
		{
			for(size_t i = 0; i < handle.gl_pathc; ++i)
			{
				std::string subpath = handle.gl_pathv[i];
				if (*subpath.rbegin() == '/')
					getFilesRecursive(subpath, files);
			}
			globfree(&handle);
		}
	}

	{
		glob_t handle;
		const int res = glob((basePath + m_mask).c_str(), 0, 0, &handle);

		if (res == 0)
		{
			for(size_t i = 0; i < handle.gl_pathc; ++i)
			{
				files.push_back(handle.gl_pathv[i]);
			}
			globfree(&handle);
		}
		else
		{
			// TODO: report error
		}
	}
}
#endif

void FilesCollector::setMask(const std::string& mask)
{
	m_mask = mask;
}

std::string FilesCollector::getMask() const
{
	return m_mask;
}

}
