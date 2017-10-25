#ifndef OPS_SHARED_PLATFORM_SETTINGS_H_INCLUDED__
#define OPS_SHARED_PLATFORM_SETTINGS_H_INCLUDED__

#include "Reprise/Reprise.h"
#include <map>


namespace OPS
{
namespace Shared
{

struct HardwareOptions
{
	HardwareOptions()
		: Complexity(0), SizeCommand(0), SizeData(0) {};

	HardwareOptions(long_long_t complexity, long_long_t sizeCommand, long_long_t sizeData)
		: Complexity(complexity), SizeCommand(sizeCommand), SizeData(sizeData) {};
	
	friend const HardwareOptions operator+(const HardwareOptions& left, const HardwareOptions& right)
	{
    return HardwareOptions(left.Complexity + right.Complexity, left.SizeCommand + right.SizeCommand, left.SizeData + right.SizeData);
	}

	friend const HardwareOptions operator*(const HardwareOptions& left, long_long_t right)
	{
    return HardwareOptions(left.Complexity * right, left.SizeCommand * right, left.SizeData * right);
	}

	long_long_t Complexity;
	long_long_t SizeCommand;
	long_long_t SizeData;
};

/// Параметры платформы
class PlatformSettings
{
public:
	/// Дополнение к BuiltinCallKind
	enum OtherCallKind
	{
		OCK_READ = 100,
		OCK_CONST,
		OCK_ARGUMENT,
		OCK_CALL
	};

private:
	/// Map опций платформы по операциям
	typedef std::map<int, HardwareOptions> MapHardwareOptionsForCallKind;
	/// Таблица сложностей операций по типам 
	typedef std::map<OPS::Reprise::BasicType::BasicTypes, MapHardwareOptionsForCallKind> TableHardwareOptions;
public:
	PlatformSettings(const std::wstring pathToFile = L"PlatformSettings.xml");
	/// Возвращает опции платформы
	HardwareOptions GetValue(int callKind, OPS::Reprise::BasicType::BasicTypes typeKind);
private:
	bool LoadFromFileOld(const std::wstring pathToFile);
	bool LoadFromFileNew(const std::wstring pathToFile);
	OPS::Reprise::BasicType::BasicTypes GetKindForPtrType();
	long_long_t GetSizeForType(OPS::Reprise::BasicType::BasicTypes type);
private:
	TableHardwareOptions m_TabHardwareOptions;
	int m_BitPlatform;
};

}
}

#endif //OPS_SHARED_PLATFORM_SETTINGS_H_INCLUDED__
