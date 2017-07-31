#include "OPS_Core/ExternalCompiler.h"
#include <stdio.h>
#include "OPS_Core/Strings.h"
#include "OPS_Core/Environment.h"
#include <iostream>
#include "OPS_Core/Exceptions.h"
#include "OPS_Core/Helpers.h"
#include <sstream>
#include <locale>
#include <vector>
#include <string>
#include <cstring>
#include <malloc.h>
#include "OPS_Core/IO.h"

using namespace std;
using namespace OPS::IO;

namespace OPS
{

extern std::wstring sourceRoot;

const char *ExtCompilerLanguageStrings[] =
{
    "ExtCompilerLanguage_C",
    "ExtCompilerLanguage_C99",
    "ExtCompilerLanguage_CUDA",
    "ExtCompilerLanguage_OpenCL"
};

bool isUnix()
{
    #if OPS_PLATFORM_IS_UNIX
    return true;
    #else
    return false;
    #endif
}

#if OPS_PLATFORM_IS_WIN32
#include <windows.h>
string getRegKey(string location, string name, bool locMach)
{
    HKEY key;
    WCHAR value[1024];
    DWORD bufLen = 1024*sizeof(WCHAR);
	wstring wlocation = OPS::Strings::widen(location);
    long ret = RegOpenKeyExW(locMach ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
							wlocation.c_str(), 0, KEY_QUERY_VALUE, &key);
    if( ret != ERROR_SUCCESS )    return "";
	ret = RegQueryValueExW(key, OPS::Strings::widen(name).c_str(), 0, 0, (LPBYTE) value, &bufLen);
    RegCloseKey(key);
    if ( ret != ERROR_SUCCESS || bufLen > 1024*sizeof(WCHAR) ) return "";
    wstring ws;
    ws.assign(value, wcslen(value));
    string stringValue = IO::osPathToPosixPath(ws);
    return stringValue;
}
#endif

//true - OK
bool ExternalCompiler::getVSFolders(string& varsbatFile, string& VCinclude, bool forNvcc)
{
    ExternalCompiler& inst = instance();

    if (forNvcc)
    {
        varsbatFile = inst.m_settings.m_vsFolderForNvcc.varsBatFile;
        VCinclude = inst.m_settings.m_vsFolderForNvcc.VCinclude;
        return !varsbatFile.empty();
    }
    else
    {
        if (inst.m_settings.m_vsFolders.empty()) return false;
        varsbatFile = inst.m_settings.m_vsFolders[0].varsBatFile;
        VCinclude = inst.m_settings.m_vsFolders[0].VCinclude;
        return true;
    }
}

std::map<std::string, std::string> ExternalCompiler::findVSFolders()
{
    std::map<std::string, std::string> varsToIncludeMap;
    string tryPath[] = {"VC\\bin", "Common7\\Tools"};
    string tryFiles[] = {"vcvars", "vsvars"};
    string tryBits[] = {"64", "32", "all"};
    string varsbatFile, VCinclude;

    //seach from VSCOMNTOOLS environment=========================================================================================
    const char *envName[] = {"VS140COMNTOOLS", "VS120COMNTOOLS", "VS110COMNTOOLS", "VS100COMNTOOLS", "VS90COMNTOOLS"};
    char * VSCOMNTOOLS;
    int num = sizeof(envName)/sizeof(const char*);
    for (int i = 0; i < num; i++)
    {
        VSCOMNTOOLS = getenv(envName[i]);
        if (VSCOMNTOOLS == NULL || !isDirExists(VSCOMNTOOLS)) continue;

        string vs = VSCOMNTOOLS;
        size_t ind = vs.find("\\Common7");
        if (ind == std::string::npos) {cout << "Can't find Common7 folder in VS path."; cout.flush(); continue;}
        vs = vs.substr(0,ind);

		for (size_t i1 = 0; i1 < sizeof(tryPath)/sizeof(string); i1++)
		for (size_t i2 = 0; i2 < sizeof(tryFiles)/sizeof(string); i2++)
		for (size_t i3 = 0; i3 < sizeof(tryBits)/sizeof(string); i3++)
        {
            varsbatFile = vs + "\\" + tryPath[i1] + "\\" + tryFiles[i2] + tryBits[i3] + ".bat";
            VCinclude = vs + "\\VC\\include";
            if (isFileExists(varsbatFile)) varsToIncludeMap[varsbatFile] = VCinclude;
        }
    }
    string regStart[] = {"Software\\Wow6432Node\\Microsoft\\VisualStudio\\",
                         "Software\\Wow6432Node\\Microsoft\\VSWinExpress\\",
                         "Software\\Microsoft\\VisualStudio\\",
                         "Software\\Microsoft\\VSWinExpress\\",
                         "Software\\Microsoft\\VCExpress\\"};
    string version[] = {"14.0", "12.0","11.0","10.0","9.0"};
	for (size_t i = 0; i < sizeof(regStart)/sizeof(string); i++)
	for (size_t v = 0; v < sizeof(version)/sizeof(string); v++)
	for (size_t j = 0; j < 2; j++)
    {
        string key = regStart[i] + version[v];
        string vs;
#if OPS_PLATFORM_IS_WIN32
		bool hkey[2] = {true, false};
        vs = getRegKey(key, "InstallDir", hkey[j]);
#endif
        if (vs.empty()) continue;
        //cout << "Found registry value: " << vs << "\n";
        if (!isDirExists(vs)) continue;
        //cout << "This dir exists!\n";
        size_t ind = vs.find("\\Common7");
        if (ind == std::string::npos)
        {
            //cout << "Can't find Common7 folder in VS path."; cout.flush();
            continue;
        }
        vs = vs.substr(0,ind);

		for (size_t i1 = 0; i1 < sizeof(tryPath)/sizeof(string); i1++)
		for (size_t i2 = 0; i2 < sizeof(tryFiles)/sizeof(string); i2++)
		for (size_t i3 = 0; i3 < sizeof(tryBits)/sizeof(string); i3++)
        {
            varsbatFile = vs + "\\" + tryPath[i1] + "\\" + tryFiles[i2] + tryBits[i3] + ".bat";
            VCinclude = vs + "\\VC\\include";
            if (isFileExists(varsbatFile)) varsToIncludeMap[varsbatFile] = VCinclude;
        }
    }
    if (varsToIncludeMap.empty())
    {
        cout << "Very strange: can't find varsBat file by registry and VSCOMNTOOLS environment variable.\n";
        cout.flush();
    }
    return varsToIncludeMap;

}

bool ExternalCompiler::isCompilerExists(ExtCompilerLanguage lang)
{
    return instance().m_settings.m_compilerExistence[lang];
}

int ExternalCompiler::compileFile(std::string srcFileName, std::string exeFileName, ExtCompilerLanguage lang,
                std::string linuxFlags, std::string windowsFlags, bool printDebug)
{
    if (!isCompilerExists(lang)) {cout << "You don't have any compiler for language " << ExtCompilerLanguageStrings[lang] << "\n"; cout.flush(); return -1;}
    int err;
    if (isUnix())
    {
        string flags = linuxFlags;
        string compiler = "gcc";
        if (lang == CUDA || lang == OpenCL)
        {
            if (isCompilerExists(CUDA))
            {
                compiler = "nvcc";
                flags += " -lOpenCL";
            }
            else
            {
                OPS_ASSERT(lang == OpenCL);
				string AMD = instance().m_settings.m_ATIpath;
				OPS_ASSERT(!AMD.empty());
                flags += string(" -I")+AMD+"/include -L"+AMD+"/lib/x86_64 "+"-L"+AMD+"/lib/x86 -lOpenCL";
            }
        }
        if (lang == C99) flags += " -std=c99";
        string tempFileName = getTmpFileName("txt", IO::getFileBaseName(srcFileName)+"_compiler_output");
        string command = compiler + " -o " + exeFileName + " " + srcFileName + " " + flags + " >" + tempFileName + " 2>&1";
        err = system(command.c_str());
        if (printDebug)
        {
            if (err != 0)
            {
                cout << "Compiling file: " + srcFileName + "\n";
                cout << readFile(tempFileName) << "\n";
                cout << "Command: " << command << "\n";
                cout << "Content of " << srcFileName << ":\n" << readFile(srcFileName) << "\n";
                cout.flush();
            }
        }
        deleteFile(tempFileName);
    }
    else
    {
        //choose compiler
        string compiler = "cl";
        if (lang == CUDA || lang == OpenCL)
        {
            if (isCompilerExists(CUDA)) compiler = "nvcc";
        }

        string batFileName, compilerOutputFileName, objFileName;
        if (printDebug) {cout << "Creating bat file...\n"; cout.flush();}
        string flags = windowsFlags;
        if (lang == OpenCL)
        {
            if (compiler == "nvcc")  flags += " -lOpenCL";
            else
            {
                char* ATISTREAMSDKROOT = std::getenv("AMDAPPSDKROOT");
                OPS_ASSERT(ATISTREAMSDKROOT != NULL);
                flags += string(" /I\"") + ATISTREAMSDKROOT + "\\include\"";
                flags += string(" \"") + ATISTREAMSDKROOT + "\\lib\\x86\\OpenCL.lib\"";
            }
        }
        string varsbatFile, VCinclude;
        if (!getVSFolders(varsbatFile, VCinclude, lang==CUDA)) return -1;
        string bat = readFile(varsbatFile);
        compilerOutputFileName = getTmpFileName("txt",IO::getFileBaseName(srcFileName)+"_compiler_output");
		objFileName = getTmpFileName("obj", IO::getFileBaseName(srcFileName));
		string libFileName;
		if (compiler == "nvcc")
		{
			size_t dotIndx = exeFileName.find_last_of(".exe");
			if (dotIndx != string::npos)
			{
				objFileName = exeFileName.substr(0,dotIndx)+".exp";
				libFileName = exeFileName.substr(0,dotIndx)+".lib";
			}
		}
		string command;
		if (compiler == "nvcc")
			command = compiler + " " + srcFileName + " -o " + exeFileName + flags + " >" + compilerOutputFileName + " 2>&1";
		else
			command = compiler + " " + ((lang==C99)?"/Tp":"") + "\"" + srcFileName + "\" /Fe\"" + exeFileName 
			+"\" /Fo\"" + objFileName +"\""
                 + " " + flags + " >" + compilerOutputFileName + " 2>&1";
        bat += command + "\n";
        batFileName = getTmpFileName("bat", IO::getFileBaseName(srcFileName));
        writeFile(batFileName, bat);

        if (printDebug) {cout << "Executing bat file...\n"; cout.flush();}
		string tempFileName = getTmpFileName("txt", IO::getFileBaseName(srcFileName)+"_bat_output");
        system((batFileName+" >"+tempFileName+" 2>&1").c_str());
        if (isFileExists(exeFileName)) err = 0;
        else err = -1;
        if (printDebug)
        {
            if (err != 0)
            {
                cout << "Compiling file: " + srcFileName + "\n";
                cout << readFile(compilerOutputFileName);
                cout << "Command: " << command << "\n";
                cout << "Content of " << srcFileName << ":\n" << readFile(srcFileName) << "\n";
				cout << "Output of bat file:\n" << readFile(tempFileName) << "\n";
                cout.flush();
            }
        }
        deleteFile(batFileName);
		IO::deleteFile(tempFileName);
        deleteFile(compilerOutputFileName);
        deleteFile(objFileName);
		deleteFile(libFileName);
    }
    return err;
}

int ExternalCompiler::executeAndGetOutput(std::string exeFileName, string &output, bool printDebug)
{
    string tempFileName = getTmpFileName("txt",IO::getFileBaseName(exeFileName)+"_output");
    if (printDebug) {cout << "Executing file " << exeFileName << "\n"; cout.flush();}
    int err = system((exeFileName + " >" + tempFileName + " 2>&1").c_str());
    if (err != 0)
    {
        if (printDebug)
        {
            cout << "Error while executing " << exeFileName << ". Error code = " << err
                 << "\nPartial output:\n" << readFile(tempFileName) << "\n"; cout.flush();
            cout << "Don't you miss 'return 0' in your code?\n"; cout.flush();
        }
        deleteFile(tempFileName);
        return err;
    }
    output = readFile(tempFileName);
    deleteFile(tempFileName);
    return err;
}

bool ExternalCompiler::compileRunAndGetOutput(std::string srcFileName, ExtCompilerLanguage lang, std::string& output,
                            std::string linuxFlags, std::string windowsFlags, bool printDebug)
{
    string exe = getTmpFileName("exe", IO::getFileBaseName(srcFileName));
    int err = compileFile(srcFileName, exe, lang, linuxFlags, windowsFlags, printDebug);
    if (err != 0)  return false;
    err = executeAndGetOutput(exe, output, printDebug);
    deleteFile(exe);
    return err == 0;
}


bool ExternalCompiler::compileAndCompareOutputs(std::string srcFileName1, std::string srcFileName2, ExtCompilerLanguage lang, bool printDebug)
{
    string o1, o2;
    compileRunAndGetOutput(srcFileName1, lang, o1, "", "", printDebug);
    compileRunAndGetOutput(srcFileName2, lang, o2, "", "", printDebug);
    bool eq = o1 == o2;
    if (!eq && printDebug)
    {
        cout << "Source code of file "<< srcFileName1 << ":\n" << IO::readFile(srcFileName1) << "\n"; cout.flush();
        cout << "Source code of file "<< srcFileName2 << ":\n" << IO::readFile(srcFileName2) << "\n"; cout.flush();
        cout << "Output of file " << srcFileName1 << ":\n" << o1 << "\n"; cout.flush();
        cout << "Output of file " << srcFileName2 << ":\n" << o2 << "\n"; cout.flush();
        cout << "Error! Outputs are not equal!\n"; cout.flush();
    }
    return eq;
}

std::list<std::string> ExternalCompiler::getGCCorMSVSIncludePaths()
{
    return instance().m_settings.m_stdIncludePaths;
}

ExternalCompiler::Settings::Settings()
{
    m_wasSettingsInitialized = false;
}

ExternalCompiler::ExternalCompiler()
{
}

ExternalCompiler& ExternalCompiler::instance()
{
    static ExternalCompiler extComp;
    if (!extComp.m_settings.m_wasSettingsInitialized) extComp.m_settings.initSettings();
    return extComp;
}

string removeCaret(string s)
{
	string s1 = s;
	int j = 0;
	for (size_t i=0; i < s.size(); i++)
	{
		if (s[i] != 13) {s1[j] = s[i]; j++;}
	}
	return s1.substr(0,j);
}

bool equal(string o1, string o2)
{
	string ro1 = removeCaret(o1), ro2 = removeCaret(o2);
	bool eq = ro1==ro2;
	return eq;
}

void ExternalCompiler::Settings::initSettings()
{
    OPS_ASSERT(m_wasSettingsInitialized == false);
    m_wasSettingsInitialized = true;
    if (isUnix())
    {
        m_compilerExistence[C] = true;
        m_compilerExistence[C99] = true;
        m_compilerExistence[CUDA] = true;
        //check CUDA
        string output;
        string output0 = IO::readFile(IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/outputCUDA.txt")));
        string test = IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/testCUDA.cu"));
        if (!ExternalCompiler::compileRunAndGetOutput(test, CUDA, output, "", "", false) || !equal(output,output0))
        {
            m_compilerExistence[CUDA] = false;
        }
        else
            m_compilerExistence[CUDA] = true;
    }
    else
    {
        std::map<std::string, std::string> varsToIncludeMap = ExternalCompiler::findVSFolders();
        string varsbatFile, VCinclude;
        //filter not worked versions of Visual Studio=========================================================================================
        m_vsFolders.resize(1);
        std::map<std::string, std::string>::reverse_iterator it = varsToIncludeMap.rbegin();
        std::list<std::string> checkedPaths;
        string tempFileName = getTmpFileName("txt");
        bool nvccExists = system(("nvcc -V >" + tempFileName + " 2>&1").c_str()) == 0;
        deleteFile(tempFileName);
        m_compilerExistence[C] = true;
        m_compilerExistence[C99] = true;
        m_compilerExistence[CUDA] = true;
        for( ; it != varsToIncludeMap.rend(); ++it)
        {
            bool good = true;
            varsbatFile = it->first;
            VCinclude = it->second;
            m_vsFolders[0].varsBatFile = varsbatFile;
            m_vsFolders[0].VCinclude = VCinclude;
            string output;
            string output0 = IO::readFile(IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/outputC.txt")));
            string test = IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/testC.c"));
            if (!ExternalCompiler::compileRunAndGetOutput(test, C, output, "", "", false) || !equal(output,output0))
                good = false;
            output0 = IO::readFile(IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/outputC99.txt")));
            test = IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/testC99.c"));
            if (!ExternalCompiler::compileRunAndGetOutput(test, C99, output, "", "", false) || !equal(output,output0))
                good = false;
            if (good) checkedPaths.push_back(varsbatFile);

            if (m_vsFolderForNvcc.varsBatFile.empty() && nvccExists)
            {
                m_vsFolderForNvcc.varsBatFile = varsbatFile;
                m_vsFolderForNvcc.VCinclude = VCinclude;
                output0 = IO::readFile(IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/outputCUDA.txt")));
                test = IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/testCUDA.cu"));
                if (!ExternalCompiler::compileRunAndGetOutput(test, CUDA, output, "", "", false) || !equal(output,output0))
                {
                    //nvcc doesn't like this version of Visual Studio
                    m_vsFolderForNvcc.varsBatFile = "";
                    m_vsFolderForNvcc.VCinclude = "";
                }
            }
        }
        if (checkedPaths.empty())
        {
            cout << "Very strange: can't find worked version of Visual Studio\n";
            cout << "Compiling C test file log:\n";
            cout.flush();
            string output;
            string test = IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/testC.c"));
            bool res = ExternalCompiler::compileRunAndGetOutput(test, C99, output, "", "", true);
            cout << "compileRunAndGetOutput returned " << (res?"OK":"can't compile") << "\n";
			if (res) cout << "Output:\n" << output << "\n";
            cout.flush();
        }
        if (nvccExists && m_vsFolderForNvcc.varsBatFile.empty())
        {
            cout << "Very strange: nvcc exists but I can't find worked with nvcc version of Visual Studio\n";
            cout << "Compiling CUDA test file log:\n";
            cout.flush();
            string output;
            string test = IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/testCUDA.cu"));
            bool res = ExternalCompiler::compileRunAndGetOutput(test, CUDA, output, "", "", true);
            cout << "compileRunAndGetOutput returned " << (res?"OK":"can't compile") << "\n";
			if (res) cout << "Output:\n" << output << "\n";
            cout.flush();
        }
        m_vsFolders.resize(checkedPaths.size());
        std::list<std::string>::iterator it2 = checkedPaths.begin();
        for (int i = 0; it2 != checkedPaths.end(); ++it2, ++i)
        {
            m_vsFolders[i].varsBatFile = *it2;
            m_vsFolders[i].VCinclude = varsToIncludeMap[*it2];
            //cout << "m_vsFolders["<<i<<"].varsBatFile = " << m_vsFolders[i].varsBatFile << "\n";
        }

        //init m_compilerExistence=========================================================================================
        if (!checkedPaths.empty())  m_compilerExistence[C] = m_compilerExistence[C99] = true;
        else m_compilerExistence[C] = m_compilerExistence[C99] = false;
        m_compilerExistence[CUDA] = !m_vsFolderForNvcc.varsBatFile.empty();
    }
    //init m_compilerExistence[OpenCL]
    m_compilerExistence[OpenCL] = /*try*/true;
	char* amd = getenv("AMDAPPSDKROOT");
	m_ATIpath = amd==NULL?"" : amd;
	if (m_ATIpath.empty() && !m_compilerExistence[CUDA]) m_compilerExistence[OpenCL] = false;
	else
    {
		string output;
		string output0 = IO::readFile(IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/outputOpenCL.txt")));
		string test = IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/OPS_Core/ExternalCompiler/testOpenCL.c"));
		if (!ExternalCompiler::compileRunAndGetOutput(test, OpenCL, output, "", "", false) || !equal(output,output0))
		{
			bool res = ExternalCompiler::compileRunAndGetOutput(test, OpenCL, output, "", "", true);
			cout << "Very strange: env variable AMDAPPSDKROOT exists but I can't compile OpenCL\n";
			cout << "compileRunAndGetOutput returned " << (res?"OK":"can't compile") << "\n";
			if (res) cout << "Output:\n" << output << "\n";
			cout.flush();
			m_ATIpath = "";
			m_compilerExistence[OpenCL] = false;
		}
		else
			m_compilerExistence[OpenCL] = true;
	}

    //init m_stdIncludePaths====================================================================================================================
    if (isUnix())
    {
        string gccOptFile = getTmpFileName("txt");
        //std::cout << "Adding gcc includes:\n";
        int err = system(("gcc -v -E - < /dev/null > " + gccOptFile + " 2>&1").c_str());
        if (err != 0) return;//empty
        string gccOpts = readFile(gccOptFile);
        size_t ind1, ind0 = gccOpts.find("#include <...>");
        if (ind0 != std::string::npos)
        {
            ind0 = gccOpts.find("\n ", ind0) + 2;
            while (ind0 != std::string::npos)
            {
                ind1 = gccOpts.find("\n", ind0);
                std::string dir = gccOpts.substr(ind0,ind1-ind0);
                //std::cout << dir << "\n";
                if (isDirExists(dir.c_str())) m_stdIncludePaths.push_back(dir);
                size_t pos = gccOpts.find("\n ", ind0);
                if (pos == std::string::npos) break;
                ind0 = pos + 2;
            }
        }
        deleteFile(gccOptFile);
        //std::cout << "Adding system includes:\n";
        const char *includeDirs[] =
        {
            "/usr/local/include",
            "/usr/include",
            "/usr/include/i386-linux-gnu"
        };
        int sz = sizeof(includeDirs)/sizeof(const char*);
        for (int i = 0; i < sz; i++)
            if (isDirExists(includeDirs[i]))
            {
                //std::cout << includeDirs[i] << "\n";
                m_stdIncludePaths.push_back(includeDirs[i]);
            }
    }
    else
    {
        string varsbatFile, VCinclude;
        if (!getVSFolders(varsbatFile, VCinclude, false)) return;//empty
        m_stdIncludePaths.push_back(VCinclude);

        // hint for VS2015
        // ToDo: fix this hardcode from jenkins build slave
        auto uctr_headers = "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.10240.0\\ucrt";
        if (isDirExists(uctr_headers)) m_stdIncludePaths.push_back(uctr_headers);
    }
}

}
