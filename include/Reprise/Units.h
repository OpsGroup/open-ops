#ifndef OPS_IR_REPRISE_UNITS_H_INCLUDED__
#define OPS_IR_REPRISE_UNITS_H_INCLUDED__

#include "Reprise/Common.h"

namespace OPS
{
namespace Reprise
{

///	C header file
class HeaderFile : public RepriseBase
{
public:
	HeaderFile();
};

class Declarations;
class TranslationUnit;

///	Program Unit 
class ProgramUnit : public RepriseBase
{
public:
	ProgramUnit(void);
	virtual ~ProgramUnit(void);
	void addTranslationUnit(ReprisePtr<TranslationUnit>& unit);
	ReprisePtr<TranslationUnit> removeUnit(int index);

	int getUnitCount(void) const;
	const TranslationUnit& getUnit(int index) const;
	TranslationUnit& getUnit(int index);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(ProgramUnit)

protected:
	ProgramUnit(const ProgramUnit& other);

private:
	typedef std::vector<TranslationUnit*> TUnits;
	TUnits m_units;
};

///	Translation Unit
class TranslationUnit : public RepriseBase
{
public:

	enum SourceLanguage
	{
		SL_C,
		SL_FORTRAN,
	};

	explicit TranslationUnit(SourceLanguage language);

	SourceLanguage getSourceLanguage() const;
	void setSourceLanguage(SourceLanguage language);

	std::string getSourceFilename(void) const;
	void setSourceFilename(const std::string& name);
/*
	int HeaderCount(void) const;
	const HeaderFile& getHeader(int index) const;
	HeaderFile& getHeader(int index);
*/
	const Declarations& getGlobals(void) const;
	Declarations& getGlobals(void);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(TranslationUnit)

protected:
	TranslationUnit(const TranslationUnit& other);

private:
	ReprisePtr<Declarations> m_globals;
	std::string m_sourceName;
	SourceLanguage m_sourceLanguage;
};


}
}

#endif                      // OPS_IR_REPRISE_UNITS_H_INCLUDED__
