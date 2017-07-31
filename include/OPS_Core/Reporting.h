#ifndef OPS_STAGE_REPORTING_H__
#define OPS_STAGE_REPORTING_H__

#include <string>
#include "OPS_Core/OPS_Core.h"

namespace OPS
{
namespace Stage
{

class OPSSubsystems
{
	enum Subsystem
	{
		OSS_UNKNOWN = 0,
		OSS_C_PARSER,
		OSS_FORTRAN_PARSER,
		OSS_REPRISE,
		OSS_CANTO,

	};
};

class MessageInfo
{
public:
	enum StatusType
	{
		ST_ERROR,
		ST_WARNING,
		ST_INFO,
		ST_DEBUG
	};

	StatusType getType(void) const;

	unsigned getCode(void) const;

	std::wstring getDescription(void) const;

	unsigned getLine(void) const;

	unsigned getPosition(void) const;

	std::wstring getSubsystem(void) const;


private:
	MessageInfo(StatusType statusType, unsigned code, const std::wstring& description, unsigned line, const std::wstring& subsystem);

	StatusType m_type;
	unsigned m_code;
	std::wstring m_description;
	unsigned m_line;
	unsigned m_position;
	std::wstring m_subsystem;

};


class Reporting : public OPS::NonCopyableMix
{

};

}
}

#endif 						//	OPS_STAGE_REPORTING_H__
