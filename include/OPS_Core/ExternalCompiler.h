#include <stdlib.h>
#include <string>
#include <list>
#include <map>
#include <vector>

namespace OPS
{

class ExternalCompiler
{
public:

    enum ExtCompilerLanguage { C = 0, C99, CUDA, OpenCL };

    static bool isCompilerExists(ExtCompilerLanguage lang);

    //возвращает 0 если все прошло нормально, иначе - код ошибки gcc или cl
    static int compileFile(std::string srcFileName, std::string exeFileName, ExtCompilerLanguage lang,
                    std::string linuxFlags = "", std::string windowsFlags = "", bool printDebug = false);

    //return 0 - OK, <>0 - error code
    static int executeAndGetOutput(std::string exeFileName, std::string& output, bool printDebug = false);

    static bool compileRunAndGetOutput(std::string srcFileName, ExtCompilerLanguage lang, std::string& output,
                                std::string linuxFlags = "", std::string windowsFlags = "", bool printDebug = false);

    //возвращает true - все откомпилировалось и результаты одинаковы, false - что-то не так
    static bool compileAndCompareOutputs(std::string srcFileName1, std::string srcFileName2,
                                  ExtCompilerLanguage lang, bool printDebug = false);

    static std::list<std::string> getGCCorMSVSIncludePaths();

private:
    ExternalCompiler();
    ExternalCompiler(const ExternalCompiler&);
    const ExternalCompiler& operator=(ExternalCompiler&);

    static ExternalCompiler& instance();

    static std::map<std::string, std::string> findVSFolders();

    static bool getVSFolders(std::string& varsBatFile, std::string& VCinclude, bool forNvcc);

    struct Settings
    {
        Settings();

        void initSettings();

        std::map<ExtCompilerLanguage, bool> m_compilerExistence;

        struct VSFolders { std::string varsBatFile, VCinclude; };

        std::vector<VSFolders> m_vsFolders;//new versions first

        VSFolders m_vsFolderForNvcc;

        std::string m_ATIpath;

        bool m_wasSettingsInitialized;

        std::list<std::string> m_stdIncludePaths;

    };

    Settings m_settings;

};

}
