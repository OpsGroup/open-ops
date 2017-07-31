#include "Reprise/Exceptions.h"
#include "Reprise/Statements.h"
#include "Reprise/Expressions.h"
#include "Reprise/Types.h"
#include "Reprise/Declarations.h"
#include "Reprise/Units.h"


//	Enter namespace
namespace OPS
{
namespace Reprise
{

//	ProgramUnit - implementation
ProgramUnit::ProgramUnit(void)
{
}

ProgramUnit::ProgramUnit(const ProgramUnit& other) : RepriseBase(other)
{
	for (int index = 0; index < other.getUnitCount(); ++index)
	{
		ReprisePtr<TranslationUnit> unit(other.getUnit(index).clone());
		addTranslationUnit(unit);
	}
}

ProgramUnit::~ProgramUnit(void)
{
	for (TUnits::iterator unit = m_units.begin(); unit != m_units.end(); ++unit)
	{
		(*unit)->setParent(0);
		delete *unit;
	}
	m_units.clear();
}

void ProgramUnit::addTranslationUnit(ReprisePtr<TranslationUnit>& unit)
{
	unit->setParent(this);
	m_units.push_back(unit.release());
}

ReprisePtr<TranslationUnit> ProgramUnit::removeUnit(int index)
{
	if (index < 0 || index >= getUnitCount())
		throw UnexpectedChildError("ProgramUnit::removeUnit()");

	ReprisePtr<TranslationUnit> unit(m_units[index]);
	unit->setParent(0);

	m_units.erase(m_units.begin() + index);
	return unit;
}

int ProgramUnit::getUnitCount(void) const
{
	return static_cast<int>(m_units.size());
}

const TranslationUnit& ProgramUnit::getUnit(const int index) const
{
	if (index < 0 || index >= getUnitCount())
		throw UnexpectedChildError("ProgramUnit::getUnit()");
	return *m_units[index];
}

TranslationUnit& ProgramUnit::getUnit(const int index)
{
	if (index < 0 || index >= getUnitCount())
		throw UnexpectedChildError("ProgramUnit::getUnit()");
	return *m_units[index];
}

int ProgramUnit::getChildCount(void) const
{
	return static_cast<int>(m_units.size());
}

RepriseBase& ProgramUnit::getChild(const int index)
{
	if (index < 0 || index >= getChildCount())
		throw UnexpectedChildError("ProgramUnit::getChild()");
	return *m_units[index];
}

std::string ProgramUnit::dumpState(void) const
{
	std::string state = RepriseBase::dumpState();
	for (TUnits::const_iterator unit = m_units.begin(); unit != m_units.end(); ++unit)
	{
		state += (*unit)->dumpState();
	}
	return state;
}

//	TranslationUnit - implementation
TranslationUnit::TranslationUnit(SourceLanguage language)
	: m_globals(new Declarations())
	, m_sourceLanguage(language)
{
	m_globals->setParent(this);
}

TranslationUnit::TranslationUnit(const TranslationUnit& other)
	: RepriseBase(other)
	, m_sourceName(other.m_sourceName)
	, m_sourceLanguage(other.m_sourceLanguage)
{
	m_globals.reset(other.m_globals->clone());
	m_globals->setParent(this);
}

TranslationUnit::SourceLanguage TranslationUnit::getSourceLanguage() const
{
	return m_sourceLanguage;
}

void TranslationUnit::setSourceLanguage(SourceLanguage language)
{
	m_sourceLanguage = language;
}

std::string TranslationUnit::getSourceFilename(void) const
{
	return m_sourceName;
}

void TranslationUnit::setSourceFilename(const std::string& name)
{
	m_sourceName = name;
}

const Declarations& TranslationUnit::getGlobals(void) const
{
	return *m_globals;
}

Declarations& TranslationUnit::getGlobals(void)
{
	return *m_globals;
}

//		RepriseBase implementation
int TranslationUnit::getChildCount(void) const
{
	return 1;
}

RepriseBase& TranslationUnit::getChild(const int index)
{
	if (index < 0 || index >= getChildCount())
		throw UnexpectedChildError("TranslationUnit::getChild()");
	return *m_globals;
}

std::string TranslationUnit::dumpState(void) const
{
	std::string state = RepriseBase::dumpState();
	state += m_sourceName;
	state += "\n";
	state += m_globals->dumpState();
	return state;
}

//	Exit namespace
}
}
