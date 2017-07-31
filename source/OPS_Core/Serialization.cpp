/*
    OPS_Service/Serialization.cpp - OPS_Service module, serialization implementation

*/

//  Standard includes
#include <limits>
#include <locale>
#include <map>
#include <set>
#include <stack>
#include <sstream>
#include <string>
#include <valarray>
#include <vector>
#include <fstream>

//  Local includes
#include "OPS_Core/Compiler.h"
#include "OPS_Core/Serialization.h"
#include "OPS_Core/xmlsp.h"
#include "OPS_Core/Helpers.h"

//  Namespaces using
using namespace OPS;

//  Defines and macros

//  Enter namespace
namespace OPS
{
namespace Serialization
{

//  Constants and enums
//      Base64 chars
static const char* const BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//      Empty items
static Item::items_type EMPTY_ITEMS;

//  Classes
//      XML parse context
class XMLParseContext : public XMLSP::Parser
{
public:
//  Constructors/destructor
    explicit XMLParseContext(const Schema& root_schema);

    ~XMLParseContext(void);

//  Methods
    std::unique_ptr<Item> parse(const std::wstring& filePath, bool skip_unknown_data);

private:
//  Private classes
//      Level type
    typedef std::pair<const Schema*, Item*> level_type;

//  Private methods
    inline std::wstring get_location(void) const
    {
        return Strings::format(L"%ls, line %u, column %u", m_source_name.c_str(), 
			static_cast<unsigned>(get_line()), static_cast<unsigned>(get_col()));
	}

	bool on_tag_open(const std::string &tag_name, XMLSP::StringMap &attributes);

	bool on_tag_close(const std::string &tag_name);

	void on_error(int errnr, int line, int col, const std::string &message);

//  Members
    const Schema& m_root_schema;
    bool m_skip_unknown_data;
    std::wstring m_source_name;
    std::unique_ptr<Item> m_item;
    std::stack<level_type> m_path;
};

//  Functions declaration
//      Name checker
static bool check_name(const std::wstring& name);
//      Tag encoder
static std::string encode_tag(const std::wstring& name);
//      Attribute encoder
static std::string encode_attribute(const std::wstring& value);
//      Element writer
static void write_element(const Schema& schema, const Item& item, std::ostringstream& output, unsigned level);
//      Parameter checker
struct check_parameter : public NonAssignableMix
{
    void operator()(const std::wstring& name) const
    {
        switch (schema.getParameterInfo(name).mode)
        {
        case ParameterInfo::MODE_UNIQUE:
            return;

        case ParameterInfo::MODE_REQUIRED_UNIQUE:
            item.getParameter(name);
            break;

        OPS_DEFAULT_CASE_LABEL
        }
    }

//  Members
    const Schema& schema;
    const Item& item;

//  Constructors/destructor
    inline check_parameter(const Schema& base_schema, const Item& target_item) :
        schema(base_schema), item(target_item)
    {
    }

};
//      Required item checker
struct check_required_item : public NonAssignableMix
{
    void operator()(const std::wstring& name) const
    {
        const Schema& item_schema = schema.getSchema(name);
        if (!parent.hasUniqueItem(name))
        {
            switch (item_schema.getMode())
            {
            case Schema::MODE_NORMAL:
                return;

            case Schema::MODE_UNIQUE:
                return;

            case Schema::MODE_REQUIRED_UNIQUE:
                parent.getUniqueItem(name);
                break;

            OPS_DEFAULT_CASE_LABEL
            }
        }
    }

//  Members
    const Schema& schema;
    const Item& parent;

//  Constructors/destructor
    inline check_required_item(const Schema& base_schema, const Item& parent_item) :
        schema(base_schema), parent(parent_item)
    {
    }
};
//      Parameter writer
struct write_parameter : public NonAssignableMix
{
    void operator()(const std::wstring& name) const;

//  Members
    const Schema& schema;
    const Item& item;
    std::ostringstream& output;

//  Constructors/destructor
    write_parameter(const Schema& base_schema, const Item& target_item, std::ostringstream& target_output) :
        schema(base_schema), item(target_item), output(target_output)
    {
    }

};
//      Unique item writer
struct write_unique_item : public NonAssignableMix
{
    void operator()(const std::wstring& name) const;

//  Members
    const Schema& schema;
    const Item& parent;
    std::ostringstream& output;
    unsigned level;

//  Constructors/destructor
    write_unique_item(const Schema& base_schema, const Item& parent_item, 
        std::ostringstream& target_output, const unsigned item_level) :
        schema(base_schema), parent(parent_item), output(target_output), level(item_level)
    {
    }

};
//      Item writer
struct write_item : public NonAssignableMix
{
    void operator()(const Item& item) const;

//  Members
    const Schema& schema;
    std::ostringstream& output;
    unsigned level;

//  Constructors/destructor
    write_item(const Schema& base_schema, std::ostringstream& target_output, const unsigned item_level) :
        schema(base_schema), output(target_output), level(item_level)
    {
    }
};

//  Variables
//      Empty string
static const std::wstring v_empty_string;
//      Empty blob
static const Parameter::BlobType v_empty_blob;

//  Classes implementation
//      XMLParseContext - constructors/destructor
XMLParseContext::XMLParseContext(const Schema& root_schema) :
    m_root_schema(root_schema), m_skip_unknown_data(false)
{
    if (m_root_schema.getMode() != Schema::MODE_REQUIRED_UNIQUE)
        throw XMLContentError(Strings::format(L"Non-required/unique root schema '%ls' (%ls).", 
            m_root_schema.getName().c_str(), get_location().c_str()));
}

XMLParseContext::~XMLParseContext(void)
{
}

//      XMLParseContext - methods
std::unique_ptr<Item> XMLParseContext::parse(const std::wstring& filePath, const bool skip_unknown_data)
{
    m_skip_unknown_data = skip_unknown_data;
    m_source_name = filePath;
    try
    {
        try
        {
			std::ifstream is;
			std::string chunk;

			#if OPS_COMPILER_MSC_VER > 0
			is.open(filePath.c_str());
			#else
			is.open(OPS::Strings::narrow(filePath).c_str());
			#endif

			if (!is)
				throw XMLParseError(L"Error opening file '" + filePath + L"'");

			if (!begin())
				throw XMLParseError(L"Failed to initialize parser");

			while (!is.eof() && is.good())
			{
				char buff[64];
				is.read(buff, 64);
				chunk.assign(buff, (unsigned int)is.gcount());
				parse_chunk(chunk);
			}

			if (is.bad())
			{
				throw XMLParseError("Error reading file");
			}

			if (!end())
			{
				throw XMLParseError("Failed to finalize parsing");
			}
        }
        catch (SchemaError& exception)
        {
            exception.appendMessage(Strings::format(L" Location: %ls.", get_location().c_str()));
//            exception.raise();
			throw;
        }
        catch (DataError& exception)
        {
            exception.appendMessage(Strings::format(L" Location: %ls.", get_location().c_str()));
//            exception.raise();
			throw;
        }
        if (!m_path.empty())
            throw XMLParseError(Strings::format(L"Item path is not empty (%ls).", get_location().c_str()));
    }
    catch (...)
    {
        m_item.reset();
        while (!m_path.empty())
            m_path.pop();
        throw;
    }
    if (m_item.get() == 0)
        throw XMLParseError(Strings::format(L"Root item not set (%ls).", get_location().c_str()));
    return std::unique_ptr<Item>(m_item.release());
}

void XMLParseContext::on_error(int errnr, int line, int col, const std::string &message)
{
	OPS_UNUSED(errnr);
	throw XMLParseError(OPS::Strings::format("Error parsing: %s (%d:%d)",
											 message.c_str(), line, col));
}

//      XMLParseContext - private methods
bool XMLParseContext::on_tag_open(const std::string &tag_name, XMLSP::StringMap &attributes)
{
	const std::wstring name(OPS::Strings::widen(tag_name));
    level_type level;
    if (!m_path.empty())
    {
        const level_type& previous_level = m_path.top();
        if (m_skip_unknown_data && (previous_level.first == 0 || !previous_level.first->hasSchema(name)))
        {
            level.first = 0;
            level.second = 0;
            m_path.push(level);
			return true;
        }
        const std::wstring string_name(name);
        level.first = &previous_level.first->getSchema(string_name);
        switch (level.first->getMode())
        {
        case Schema::MODE_NORMAL:
            level.second = &previous_level.second->addItem(string_name);
            break;

        case Schema::MODE_UNIQUE:
        case Schema::MODE_REQUIRED_UNIQUE:
            level.second = &previous_level.second->addUniqueItem(string_name);
            break;

        OPS_DEFAULT_CASE_LABEL
        }
    }
    else
    {
        if (m_root_schema.getName() != name)
            throw XMLContentError(Strings::format(L"Unexpected root tag '%ls', expected '%ls' (%ls).", 
				name.c_str(), m_root_schema.getName().c_str(), get_location().c_str()));
        if (m_item.get() != 0)
            throw XMLContentError(Strings::format(L"Root item already present (%ls).", get_location().c_str()));
        m_item.reset(new Item(m_root_schema.getName()));
        level.first = &m_root_schema;
        level.second = m_item.get();
    }
    std::wstring parameter_name;
	for (XMLSP::StringMap::const_iterator attrIter = attributes.begin(); attrIter != attributes.end(); ++attrIter)
    {
		parameter_name = OPS::Strings::widen(attrIter->first);
        if (parameter_name.empty())
            throw XMLParseError(Strings::format(L"Empty attribute name in tag '%ls' (%ls).", 
				name.c_str(), get_location().c_str()));
        const ParameterInfo& parameter_info = level.first->getParameterInfo(parameter_name);
        Parameter parameter(parameter_info.type, parameter_name);
        parameter_name.clear();
		const std::wstring valueString = OPS::Strings::widen(attrIter->second);
		const wchar_t* const value = valueString.c_str();
        switch (parameter_info.type)
        {
        case Parameter::PARAMETER_TYPE_BOOL:
            {
                bool result;
                if (!Strings::fetch(value, result))
	                throw XMLDataError(Strings::format(L"Invalid boolean value '%ls' for parameter '%ls' (%ls).", 
                        value, parameter.getName().c_str(), get_location().c_str()));
                parameter.setBool(result);
            }
	        break;

        case Parameter::PARAMETER_TYPE_INT:
            {
                int result;
                if (!Strings::fetch(value, result))
	                throw XMLDataError(Strings::format(L"Invalid integer value '%ls' for parameter '%ls' (%ls).",
		                value, parameter.getName().c_str(), get_location().c_str()));
                parameter.setInt(result);
            }
	        break;

        case Parameter::PARAMETER_TYPE_UNSIGNED:
            {
                unsigned result;
                if (!Strings::fetch(value, result))
	                throw XMLDataError(Strings::format(L"Invalid unsigned value '%ls' for parameter '%ls' (%ls).",
		                value, parameter.getName().c_str(), get_location().c_str()));
                parameter.setUnsigned(result);
            }
	        break;

        case Parameter::PARAMETER_TYPE_LONG_INT:
            {
                long_long_t result;
                if (!Strings::fetch(value, result))
	                throw XMLDataError(Strings::format(
                        L"Invalid long integer value '%ls' for parameter '%ls' (%ls).",
		                value, parameter.getName().c_str(), get_location().c_str()));
                parameter.setLongInt(result);
            }
	        break;

        case Parameter::PARAMETER_TYPE_LONG_UNSIGNED:
            {
                unsigned_long_long_t result;
                if (!Strings::fetch(value, result))
	                throw XMLDataError(Strings::format(
                        L"Invalid long unsigned value '%ls' for parameter '%ls' (%ls).",
		                value, parameter.getName().c_str(), get_location().c_str()));
                parameter.setLongUnsigned(result);
            }
	        break;

        case Parameter::PARAMETER_TYPE_REAL:
            {
                double result;
                if (!Strings::fetch(value, result))
	                throw XMLDataError(Strings::format(L"Invalid real value '%ls' for parameter '%ls' (%ls).",
		                value, parameter.getName().c_str(), get_location().c_str()));
                parameter.setReal(result);
            }
	        break;

        case Parameter::PARAMETER_TYPE_STRING:
            parameter.setString(value);
	        break;

        case Parameter::PARAMETER_TYPE_BLOB:
            {
                size_t index = 0;
                const wchar_t* const end_data = value + std::wcslen(value);
                for (const wchar_t* data = value;data != end_data;++data)
                {
                    if (*data <= 0xFF && std::strchr(BASE64_CHARS, static_cast<byte>(*data)) != 0)
                        ++index;
                }
				if (index > 0)
				{
					index = index - ((index - 1) / 4 + 1);
				}
				Parameter::BlobType blob(index);
				byte* result = &blob[0];
				index = 0;
				byte c4[4];
				for (const wchar_t* data = value;data != end_data;++data)
				{
					if (*data > 0xFF)
						continue;
					const char* code = std::strchr(BASE64_CHARS, static_cast<byte>(*data));
					if (code == 0)
						continue;
					c4[index++] = static_cast<byte>(code - BASE64_CHARS);
					if (index == 4)
					{
						*result++ = (c4[0] << 2) | ((c4[1] & 0x30) >> 4);
						*result++ = ((c4[1] & 0x0F) << 4) | ((c4[2] & 0x3c) >> 2);
						*result++ = ((c4[2] & 0x3) << 6) | c4[3];
						index = 0;
					}
				}
				if (index > 1)
				{
					*result++ = (c4[0] << 2) | ((c4[1] & 0x30) >> 4);
					if (index > 2)
						*result++ = ((c4[1] & 0x0F) << 4) | ((c4[2] & 0x3c) >> 2);
				}
				parameter.setBlob(blob);
            }
	        break;

        OPS_DEFAULT_CASE_LABEL
        }
        level.second->addParameter(parameter);
    }
    level.first->processParameters(check_parameter(*level.first, *level.second));
    m_path.push(level);
    return true;
}

bool XMLParseContext::on_tag_close(const std::string &tag_name)
{
	const std::wstring name = OPS::Strings::widen(tag_name);
    if (m_path.empty())
        throw XMLParseError(Strings::format(L"Tag stack underflow (%ls).", get_location().c_str()));
    const level_type& level = m_path.top();
#if OPS_BUILD_DEBUG
    if (level.first != 0 && level.first->getName() != name)
        throw XMLParseError(Strings::format(L"Unexpected tag stack schema '%ls', expected '%ls' (%ls).", 
			name.c_str(), level.first->getName().c_str(), get_location().c_str()));
    if (level.second != 0 && level.second->getName() != name)
        throw XMLParseError(Strings::format(L"Unexpected tag stack item '%ls', expected '%ls' (%ls).", 
			name.c_str(), level.second->getName().c_str(), get_location().c_str()));
#else
    OPS_UNUSED(name)
#endif
    if (level.first != 0 && level.second != 0)
        level.first->processUniqueItems(check_required_item(*level.first, *level.second));
    m_path.pop();
    return true;
}

//  Global classes implementation
//      Parameter - constructors/destructor
Parameter::Parameter(const Type type, const std::wstring& name) :
	m_type(type), m_name(name)
{
	switch (m_type)
	{
	case PARAMETER_TYPE_BOOL:
	case PARAMETER_TYPE_INT:
	case PARAMETER_TYPE_UNSIGNED:
	case PARAMETER_TYPE_LONG_INT:
	case PARAMETER_TYPE_LONG_UNSIGNED:
	case PARAMETER_TYPE_REAL:
        break;

	case PARAMETER_TYPE_STRING:
	case PARAMETER_TYPE_BLOB:
        m_value.composite_value = 0;
		break;

	default:
		throw ArgumentError(Strings::format(L"Invalid parameter type %i (parameter '%ls').", type, name.c_str()));
	}
    if (m_name.empty() || m_name.find(L'-') != std::wstring::npos)
        throw ArgumentError(Strings::format(L"Invalid parameter name '%ls'.", m_name.c_str()));
}

Parameter::Parameter(const Parameter& parameter) :
	m_type(parameter.m_type), m_name(parameter.m_name)
{
	switch (m_type)
	{
	case PARAMETER_TYPE_BOOL:
        m_value.bool_value = parameter.m_value.bool_value;
        break;

	case PARAMETER_TYPE_INT:
        m_value.int_value = parameter.m_value.int_value;
        break;

	case PARAMETER_TYPE_UNSIGNED:
        m_value.unsigned_value = parameter.m_value.unsigned_value;
        break;

	case PARAMETER_TYPE_LONG_INT:
        m_value.long_int_value = parameter.m_value.long_int_value;
        break;

	case PARAMETER_TYPE_LONG_UNSIGNED:
        m_value.long_unsigned_value = parameter.m_value.long_unsigned_value;
        break;

	case PARAMETER_TYPE_REAL:
        m_value.real_value = parameter.m_value.real_value;
        break;

	case PARAMETER_TYPE_STRING:
        {
            const std::wstring* const composite_value = static_cast<const std::wstring*>(parameter.m_value.composite_value);
            m_value.composite_value = composite_value != 0 ? new std::wstring(*composite_value) : 0;
        }
        break;

	case PARAMETER_TYPE_BLOB:
        {
            const BlobType* const composite_value = static_cast<const BlobType*>(parameter.m_value.composite_value);
            m_value.composite_value = composite_value != 0 ? new BlobType(*composite_value) : 0;
        }
		break;

	OPS_DEFAULT_CASE_LABEL
	}
}

Parameter::~Parameter(void)
{
	switch (m_type)
	{
    case PARAMETER_TYPE_BOOL:
    case PARAMETER_TYPE_INT:
    case PARAMETER_TYPE_UNSIGNED:
    case PARAMETER_TYPE_LONG_INT:
    case PARAMETER_TYPE_LONG_UNSIGNED:
    case PARAMETER_TYPE_REAL:
        break;

    case PARAMETER_TYPE_STRING:
        delete static_cast<std::wstring*>(m_value.composite_value);
        break;

	case PARAMETER_TYPE_BLOB:
        delete static_cast<BlobType*>(m_value.composite_value);
        break;

    OPS_DEFAULT_CASE_LABEL
	}
}

//		Parameter - methods
Parameter::Type Parameter::getType(void) const
{
	return m_type;
}

const std::wstring& Parameter::getName(void) const
{
    return m_name;
}

bool Parameter::getBool(void) const
{
    checkType(PARAMETER_TYPE_BOOL);
	return m_value.bool_value;
}

int Parameter::getInt(void) const
{
    checkType(PARAMETER_TYPE_INT);
	return m_value.int_value;
}

unsigned Parameter::getUnsigned(void) const
{
    checkType(PARAMETER_TYPE_UNSIGNED);
	return m_value.unsigned_value;
}

long_long_t Parameter::getLongInt(void) const
{
    checkType(PARAMETER_TYPE_LONG_INT);
	return m_value.long_int_value;
}

unsigned_long_long_t Parameter::getLongUnsigned(void) const
{
    checkType(PARAMETER_TYPE_LONG_UNSIGNED);
	return m_value.long_unsigned_value;
}

double Parameter::getReal(void) const
{
    checkType(PARAMETER_TYPE_REAL);
	return m_value.real_value;
}

const std::wstring& Parameter::getString(void) const
{
    checkType(PARAMETER_TYPE_STRING);
    const std::wstring* const value = static_cast<const std::wstring*>(m_value.composite_value);
    return value != 0 ? *value : v_empty_string;
}

const Parameter::BlobType& Parameter::getBlob(void) const
{
    checkType(PARAMETER_TYPE_BLOB);
	const BlobType* const value = static_cast<const BlobType*>(m_value.composite_value);
    return value != 0 ? *value : v_empty_blob;
}

void Parameter::setBool(const bool value)
{
    checkType(PARAMETER_TYPE_BOOL);
	m_value.bool_value = value;
}

void Parameter::setInt(const int value)
{
    checkType(PARAMETER_TYPE_INT);
	m_value.int_value = value;
}

void Parameter::setUnsigned(const unsigned value)
{
    checkType(PARAMETER_TYPE_UNSIGNED);
	m_value.unsigned_value = value;
}

void Parameter::setLongInt(const long_long_t value)
{
    checkType(PARAMETER_TYPE_LONG_INT);
	m_value.long_int_value = value;
}

void Parameter::setLongUnsigned(const unsigned_long_long_t value)
{
    checkType(PARAMETER_TYPE_LONG_UNSIGNED);
	m_value.long_unsigned_value = value;
}

void Parameter::setReal(const double value)
{
    checkType(PARAMETER_TYPE_REAL);
	m_value.real_value = value;
}

void Parameter::setString(const std::wstring& value)
{
    checkType(PARAMETER_TYPE_STRING);
    std::unique_ptr<std::wstring> new_value(!value.empty() ? new std::wstring(value) : 0);
    delete static_cast<std::wstring*>(m_value.composite_value);
    m_value.composite_value = new_value.release();
}

void Parameter::setString(const wchar_t* const value)
{
    checkType(PARAMETER_TYPE_STRING);
    const size_t size = std::wcslen(value);
    std::unique_ptr<std::wstring> new_value(size != 0 ? new std::wstring(value, size) : 0);
    delete static_cast<std::wstring*>(m_value.composite_value);
    m_value.composite_value = new_value.release();
}

void Parameter::setBlob(const BlobType& value)
{
    checkType(PARAMETER_TYPE_BLOB);
    std::unique_ptr<BlobType> new_value(value.size() != 0 ? new BlobType(value) : 0);
    delete static_cast<BlobType*>(m_value.composite_value);
    m_value.composite_value = new_value.release();
}

bool Parameter::operator==(const Parameter& parameter) const
{
	if (this == &parameter)
		return true;

	if (m_name != parameter.m_name || m_type != parameter.m_type)
		return false;

    bool result = false;
	switch (m_type)
	{
    case PARAMETER_TYPE_BOOL:
		result = m_value.bool_value == parameter.m_value.bool_value;
        break;

    case PARAMETER_TYPE_INT:
		result = m_value.int_value == parameter.m_value.int_value;
        break;

    case PARAMETER_TYPE_UNSIGNED:
		result = m_value.unsigned_value == parameter.m_value.unsigned_value;
        break;

    case PARAMETER_TYPE_LONG_INT:
		result = m_value.long_int_value == parameter.m_value.long_int_value;
        break;

    case PARAMETER_TYPE_LONG_UNSIGNED:
		result = m_value.long_unsigned_value == parameter.m_value.long_unsigned_value;
        break;

    case PARAMETER_TYPE_REAL:
		result = m_value.real_value == parameter.m_value.real_value;
        break;

    case PARAMETER_TYPE_STRING:
        result = StlHelpers::equal_container_pointer(static_cast<const std::wstring*>(m_value.composite_value),
            static_cast<const std::wstring*>(parameter.m_value.composite_value));
        break;

   	case PARAMETER_TYPE_BLOB:
        result = StlHelpers::equal_valarray(getBlob(), parameter.getBlob());
        break;

    OPS_DEFAULT_CASE_LABEL
	}
    return result;
}

bool Parameter::operator!=(const Parameter& parameter) const
{
	return !(*this == parameter);
}

//      Parameter - private methods
void Parameter::checkType(const Type type) const
{
	if (m_type != type)
    	throw DataParameterIncompatibleType(Strings::format(L"Incompatible parameter type %i (expected %i)"
            L" (parameter '%ls').", m_type, type, m_name.c_str()));
}

//      Item - constructors/destructor
Item::Item(const std::wstring& name) :
    m_name(name)
{
    if (!check_name(m_name))
        throw ArgumentError(Strings::format(L"Invalid item name '%ls'.", m_name.c_str()));
}

Item::Item(const Item& item) :
    m_name(item.m_name)
{
    if (item.m_items.get() != 0)
        m_items.reset(new items_type(*item.m_items));
    if (item.m_unique_items.get() != 0)
        m_unique_items.reset(new unique_items_type(*item.m_unique_items));
    if (item.m_parameters.get() != 0)
        m_parameters.reset(new parameters_type(*item.m_parameters));
}

//      Item - methods
const std::wstring& Item::getName(void) const
{
    return m_name;
}

Item& Item::addItem(const std::wstring& name)
{
    return addItem(Item(name));
}

Item& Item::addItem(const Item& item)
{
    const std::wstring& name = item.getName();
    if (m_unique_items.get() != 0 && m_unique_items->count(name) != 0)
        throw DataItemAlreadyPresent(Strings::format(L"Non-unique item '%ls' already present as unique in item '%ls'.", 
            name.c_str(), m_name.c_str()));
    if (m_items.get() == 0)
        m_items.reset(new items_type());
    m_items->push_back(item);
    return m_items->back();
}

void Item::removeItem(const std::wstring& name)
{
    if (m_items.get() == 0)
        return;
	items_type::iterator location = m_items->begin();
	while (location != m_items->end())
	{
		items_type::iterator current_location = location++;
		if (current_location->getName() == name)
			m_items->erase(current_location);
	}
    if (m_items->empty())
        m_items.reset();
}

void Item::removeItem(Item* const item)
{
    if (m_items.get() != 0)
    {
        const items_type::iterator location = StlHelpers::find_if(*m_items, 
            StlHelpers::combine_unary(StlHelpers::make_pointer<Item>(), std::bind2nd(std::equal_to<Item*>(), item)));
        if (location != m_items->end())
        {
            m_items->erase(location);
            if (m_items->empty())
                m_items.reset();
            return;
        }
    }
    throw DataItemNotFound(Strings::format(L"Item '%ls'(%p) not found in item '%ls'.", 
        item->getName().c_str(), item, m_name.c_str()));
}

Item& Item::addUniqueItem(const std::wstring& name)
{
    return addUniqueItem(Item(name));
}

Item& Item::addUniqueItem(const Item& item)
{
    const std::wstring& name = item.getName();
    if (m_unique_items.get() != 0 && m_unique_items->count(name) != 0)
        throw DataItemAlreadyPresent(Strings::format(L"Unique item '%ls' already present in item '%ls'.", 
            name.c_str(), m_name.c_str()));
    if (m_items.get() != 0)
        for (items_type::iterator location = m_items->begin();location != m_items->end();++location)
            if (location->getName() == name)
                throw DataItemAlreadyPresent(Strings::format(
                    L"Unique item '%ls' already present as non-unique in item '%ls'.", name.c_str(), m_name.c_str()));
    if (m_unique_items.get() == 0)
        m_unique_items.reset(new unique_items_type());
    return m_unique_items->insert(unique_items_type::value_type(name, item)).first->second;
}

bool Item::hasUniqueItem(const std::wstring& name) const
{
	return m_unique_items.get() != 0 && m_unique_items->count(name) != 0;
}

Item& Item::getUniqueItem(const std::wstring& name)
{
    if (m_unique_items.get() != 0)
    {
        const unique_items_type::iterator location = m_unique_items->find(name);
        if (location != m_unique_items->end())
            return location->second;
    }
    throw DataItemNotFound(Strings::format(L"Item '%ls' not found in item '%ls'.", 
        name.c_str(), m_name.c_str()));
}

const Item& Item::getUniqueItem(const std::wstring& name) const
{
    return const_cast<Item&>(*this).getUniqueItem(name);
}

void Item::removeUniqueItem(const std::wstring& name)
{
	if (m_unique_items.get() == 0 || m_unique_items->erase(name) != 1)
		throw DataItemNotFound(Strings::format(L"Item '%ls' not found in item '%ls'.", 
            name.c_str(), m_name.c_str()));
}

bool Item::hasSubitem(const Parameter& parameter) const
{
	const std::wstring& name = parameter.getName();
    if (m_items.get() != 0)
    {
	    for (items_type::const_iterator location = m_items->begin();location != m_items->end();++location)
	    {
		    if (location->hasParameter(name))
		    {
			    const Parameter& candidate = location->getParameter(name);
			    if (candidate == parameter)
				    return true;
		    }
	    }
    }
    if (m_unique_items.get() != 0)
    {
	    for (unique_items_type::const_iterator location = m_unique_items->begin();
            location != m_unique_items->end();
            ++location)
	    {
		    if (location->second.hasParameter(name))
		    {
			    const Parameter& candidate = location->second.getParameter(name);
			    if (candidate == parameter)
				    return true;
		    }
	    }
    }
	return false;
}

Item& Item::findSubitem(const Parameter& parameter)
{
	const std::wstring& name = parameter.getName();
    if (m_items.get() != 0)
    {
	    for (items_type::iterator location = m_items->begin();location != m_items->end();++location)
	    {
		    if (location->hasParameter(name))
		    {
			    const Parameter& candidate = location->getParameter(name);
			    if (candidate == parameter)
				    return *location;
		    }
	    }
    }
    if (m_unique_items.get() != 0)
    {
	    for (unique_items_type::iterator location = m_unique_items->begin();
            location != m_unique_items->end();
            ++location)
	    {
		    if (location->second.hasParameter(name))
		    {
			    const Parameter& candidate = location->second.getParameter(name);
			    if (candidate == parameter)
				    return location->second;
		    }
	    }
    }
	std::wstring exception_message = L"unknown";
	switch (parameter.getType())
	{
    case Parameter::PARAMETER_TYPE_BOOL:
		exception_message = Strings::format(L"boolean, value %i", parameter.getBool());
		break;

    case Parameter::PARAMETER_TYPE_INT:
		exception_message = Strings::format(L"integer, value %i", parameter.getInt());
		break;

    case Parameter::PARAMETER_TYPE_UNSIGNED:
		exception_message = Strings::format(L"unsigned, value %u", parameter.getUnsigned());
		break;

    case Parameter::PARAMETER_TYPE_LONG_INT:
		exception_message = Strings::format(L"long integer, value %" OPS_CRT_FORMAT_LONG_LONG_LPREFIX L"i", 
            parameter.getInt());
		break;

    case Parameter::PARAMETER_TYPE_LONG_UNSIGNED:
		exception_message = Strings::format(L"long unsigned, value %" OPS_CRT_FORMAT_LONG_LONG_LPREFIX L"u", 
            parameter.getUnsigned());
		break;

    case Parameter::PARAMETER_TYPE_REAL:
		exception_message = Strings::format(L"real, value %g", parameter.getReal());
		break;

    case Parameter::PARAMETER_TYPE_STRING:
		exception_message = Strings::format(L"string, value '%ls'", parameter.getString().c_str());
		break;

	case Parameter::PARAMETER_TYPE_BLOB:
		exception_message = L"blob";
		break;

    OPS_DEFAULT_CASE_LABEL
	}
	throw DataItemNotFound(Strings::format(L"Item containing parameter '%ls' (%ls) not found in item '%ls'.", 
        name.c_str(), exception_message.c_str(), m_name.c_str()));
}

const Item& Item::findSubitem(const Parameter& parameter) const
{
    return const_cast<Item&>(*this).findSubitem(parameter);
}

Item::items_type::iterator Item::begin()
{
	if (m_items.get() == 0)
		return EMPTY_ITEMS.begin();
	return m_items->begin();
}

Item::items_type::iterator Item::end()
{
	if (m_items.get() == 0)
		return EMPTY_ITEMS.end();
	return m_items->end();
}

Item::items_type::const_iterator Item::begin() const
{
	if (m_items.get() == 0)
		return EMPTY_ITEMS.begin();
	return m_items->begin();
}

Item::items_type::const_iterator Item::end() const
{
	if (m_items.get() == 0)
		return EMPTY_ITEMS.end();
	return m_items->end();
}

Parameter& Item::addParameter(const Parameter& parameter)
{
    const std::wstring& name = parameter.getName();
    if (m_parameters.get() == 0)
        m_parameters.reset(new parameters_type());
    std::pair<parameters_type::iterator, bool> result = m_parameters->insert(parameters_type::value_type(name, parameter));
    if (!result.second)
        throw DataParameterAlreadyPresent(Strings::format(L"Parameter '%ls' already present in item '%ls'.", 
            name.c_str(), m_name.c_str()));
    return result.first->second;
}

Parameter& Item::addParameter(const Parameter::Type type, const std::wstring& name)
{
    return addParameter(Parameter(type, name));
}

bool Item::hasParameter(const std::wstring& name) const
{
	return m_parameters.get() != 0 && m_parameters->count(name) != 0;
}

Parameter& Item::getParameter(const std::wstring& name)
{
    if (m_parameters.get() != 0)
    {
        const parameters_type::iterator location = m_parameters->find(name);
        if (location != m_parameters->end())
            return location->second;
    }
    throw DataParameterNotFound(Strings::format(L"Parameter '%ls' not found in item '%ls'.", 
        name.c_str(), m_name.c_str()));
}

const Parameter& Item::getParameter(const std::wstring& name) const
{
    return const_cast<Item&>(*this).getParameter(name);
}

void Item::removeParameter(const std::wstring& name)
{
	if (m_parameters.get() == 0 || m_parameters->erase(name) != 1)
		throw DataParameterNotFound(Strings::format(L"Parameter '%ls' not found in item '%ls'.", 
            name.c_str(), m_name.c_str()));
}

bool Item::operator==(const Item& item) const
{
	return (this == &item) || (m_name == item.m_name && 
        StlHelpers::equal_container_pointer(m_items.get(), item.m_items.get()) &&
        StlHelpers::equal_container_pointer(m_unique_items.get(), item.m_unique_items.get()) &&
        StlHelpers::equal_container_pointer(m_parameters.get(), item.m_parameters.get()));
}

//      ParameterInfo - constructors/destructor
ParameterInfo::ParameterInfo(const Parameter::Type parameter_type, const Mode parameter_mode) :
    type(parameter_type), mode(parameter_mode)
{
	switch (type)
	{
    case Parameter::PARAMETER_TYPE_BOOL:
	case Parameter::PARAMETER_TYPE_INT:
	case Parameter::PARAMETER_TYPE_UNSIGNED:
	case Parameter::PARAMETER_TYPE_LONG_INT:
	case Parameter::PARAMETER_TYPE_LONG_UNSIGNED:
	case Parameter::PARAMETER_TYPE_REAL:
	case Parameter::PARAMETER_TYPE_STRING:
	case Parameter::PARAMETER_TYPE_BLOB:
		break;

	default:
		throw ArgumentError(Strings::format(L"Invalid parameter type %i.", type));
	}

	switch (mode)
	{
	case MODE_UNIQUE:
	case MODE_REQUIRED_UNIQUE:
		break;

	default:
		throw ArgumentError(Strings::format(L"Invalid parameter mode %i.", mode));
	}
}

//      Schema - constructors/destructor
Schema::Schema(const std::wstring& name, const Mode mode) :
    m_name(name), m_mode(mode)
{
    if (!check_name(m_name))
        throw ArgumentError(Strings::format(L"Invalid schema item name '%ls'.", m_name.c_str()));
    switch (m_mode)
    {
    case MODE_NORMAL:
    case MODE_UNIQUE:
    case MODE_REQUIRED_UNIQUE:
        break;

    default:
        throw ArgumentError(Strings::format(L"Invalid schema mode %i for schema '%ls'.", mode, name.c_str()));
    }
}

//      Schema - methods
const std::wstring& Schema::getName(void) const
{
    return m_name;
}

Schema::Mode Schema::getMode(void) const
{
    return m_mode;
}

bool Schema::hasSchema(const std::wstring& name) const
{
    return m_items.count(name) != 0;
}

bool Schema::hasParameter(const std::wstring& name) const
{
    return m_parameters.count(name) != 0;
}

Schema& Schema::addSchema(const std::wstring& name, const Mode mode)
{
    return addSchema(Schema(name, mode));
}

Schema& Schema::addSchema(const Schema& schema)
{
    const std::wstring& name = schema.getName();
    if (m_items.count(name) != 0)
        throw SchemaItemAlreadyPresent(Strings::format(L"Schema item '%ls' already present in schema '%ls'.", 
            name.c_str(), m_name.c_str()));
    if (schema.getMode() == MODE_UNIQUE || schema.getMode() == MODE_REQUIRED_UNIQUE)
        m_items_ordering.push_back(name);
    return m_items.insert(items_type::value_type(name, schema)).first->second;
}

Schema& Schema::getSchema(const std::wstring& name)
{
    const items_type::iterator location = m_items.find(name);
    if (location == m_items.end())
        throw SchemaItemNotFound(Strings::format(L"Schema item '%ls' not found in schema '%ls'.", 
            name.c_str(), m_name.c_str()));
    return location->second;
}

const Schema& Schema::getSchema(const std::wstring& name) const
{
    return const_cast<Schema&>(*this).getSchema(name);
}

void Schema::addParameter(const std::wstring& name, const ParameterInfo& info)
{
    if (!check_name(name))
        throw ArgumentError(Strings::format(L"Invalid schema parameter name '%ls' in schema '%ls'.", 
            name.c_str(), m_name.c_str()));
    if (m_parameters.count(name) != 0)
        throw SchemaParameterAlreadyPresent(Strings::format(L"Schema parameter '%ls' already present in schema '%ls'.", 
            name.c_str(), m_name.c_str()));
    m_parameters_ordering.push_back(name);
    m_parameters.insert(parameters_type::value_type(name, info));
}

void Schema::addParameter(const std::wstring& name, const Parameter::Type type, 
    const ParameterInfo::Mode mode)
{
    addParameter(name, ParameterInfo(type, mode));
}

const ParameterInfo& Schema::getParameterInfo(const std::wstring& name) const
{
    const parameters_type::const_iterator location = m_parameters.find(name);
    if (location == m_parameters.end())
        throw SchemaParameterNotFound(Strings::format(L"Schema parameter '%ls' not found in schema '%ls'.", 
            name.c_str(), m_name.c_str()));
    return location->second;
}

//      Serializer - constructors/destructor
Serializer::Serializer(const Schema& schema) :
    m_schema(schema)
{
}

Serializer::~Serializer(void)
{
}

//      Serializer - methods
const Schema& Serializer::getSchema(void) const
{
    return m_schema;
}

//      XMLSerializer - constructors/destructor
XMLSerializer::XMLSerializer(const Schema& schema) :
    Serializer(schema), m_data(0)
{
    const Schema& root_schema = getSchema();
    if (root_schema.getMode() != Schema::MODE_REQUIRED_UNIQUE)
        throw XMLContentError(Strings::format(L"Non-required/unique root schema '%ls'.", 
            root_schema.getName().c_str()));
}

XMLSerializer::~XMLSerializer(void)
{
    delete static_cast<XMLParseContext*>(m_data);
}

//      XMLSerializer - methods
std::unique_ptr<Item> XMLSerializer::read(const std::wstring& filePath, const bool skip_unknown_data) const
{
    if (m_data == 0)
        m_data = new XMLParseContext(getSchema());
    return static_cast<XMLParseContext*>(m_data)->parse(filePath, skip_unknown_data);
}

void XMLSerializer::write(const std::wstring& filePath, const Item& item) const
{
    const Schema& root_schema = getSchema();
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output.setf(std::ios::boolalpha);
    output.precision(std::numeric_limits<double>::digits10 + 2);
    output << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n";
    write_element(root_schema, item, output, 0);
    const std::string& result = output.str();
    std::ofstream os;
    #if OPS_COMPILER_MSC_VER > 0
    os.open(filePath.c_str());
    #else
    os.open(OPS::Strings::narrow(filePath).c_str());
    #endif
    os.write(result.c_str(), result.size());
}

//  Global functions implementation

//  Functions implementation
static bool check_name(const std::wstring& name)
{
    if (name.empty())
        return false;
    for (const wchar_t* position = name.c_str(), * const position_end = position + name.size();position != position_end;++position)
        if ((*position < '0' || *position > '9') && (*position < 'A' || *position > 'Z') && (*position < 'a' || *position > 'z') &&
            *position != '_')
            return false;
    return true;
}

static std::string encode_tag(const std::wstring& name)
{
    return Strings::narrow_utf8(name);
}

static std::string encode_attribute(const std::wstring& value)
{
    std::string result = Strings::narrow_utf8(value);
    size_t offset = 0;
    while (offset < result.size())
    {
        const char* replacement;
        switch (result[offset])
        {
        case '<':
            replacement = "&lt;";
            break;

        case '>':
            replacement = "&gt;";
            break;

        case '&':
            replacement = "&amp;";
            break;

        case '"':
            replacement = "&quot;";
            break;

        case '\'':
            replacement = "&apos;";
            break;

        default:
            ++offset;
            continue;
        }
        const size_t size = std::strlen(replacement);
        result.replace(offset, 1, replacement, size);
        offset += size;
    }
    return result;
}

static void write_element(const Schema& schema, const Item& item, std::ostringstream& output, const unsigned level)
{
    const std::string& tag_name = encode_tag(item.getName());
    for (unsigned tab = 0;tab < level;++tab)
        output << '\t';
    output << '<' << tag_name;
    schema.processParameters(write_parameter(schema, item, output));
    output << '>' << '\r' << '\n';
    std::ostringstream::pos_type position = output.tellp();
    schema.processUniqueItems(write_unique_item(schema, item, output, level + 1));
    item.processItems(write_item(schema, output, level + 1));
    if (output.tellp() != position)
    {
        for (unsigned tab = 0;tab < level;++tab)
            output << '\t';
        output << '<' << '/' << tag_name << '>' << '\r' << '\n';
    }
    else
    {
        output.seekp(-3, std::ios_base::cur);
        output << '/' << '>' << '\r' << '\n';
    }
}

void write_parameter::operator()(const std::wstring& name) const
{
	const ParameterInfo& parameter_info = schema.getParameterInfo(name);
    if (!item.hasParameter(name))
    {
        switch (parameter_info.mode)
        {
        case ParameterInfo::MODE_UNIQUE:
            return;

        case ParameterInfo::MODE_REQUIRED_UNIQUE:
            break;

        OPS_DEFAULT_CASE_LABEL
        }
    }
    const Parameter& parameter = item.getParameter(name);
	if (parameter.getType() != parameter_info.type)
		throw DataParameterIncompatibleType(Strings::format(
			L"Incompatible parameter type %i (expected %i) (parameter '%ls', item '%ls').", 
			parameter.getType(), parameter_info.type, name.c_str(), item.getName().c_str()));
    output << ' ' << encode_tag(parameter.getName()) << '=' << '"';
	switch (parameter.getType())
	{
	case Parameter::PARAMETER_TYPE_BOOL:
        output << parameter.getBool();
		break;

	case Parameter::PARAMETER_TYPE_INT:
        output << parameter.getInt();
		break;

	case Parameter::PARAMETER_TYPE_UNSIGNED:
        output << parameter.getUnsigned();
		break;

	case Parameter::PARAMETER_TYPE_LONG_INT:
        output << parameter.getLongInt();
		break;

	case Parameter::PARAMETER_TYPE_LONG_UNSIGNED:
        output << parameter.getLongUnsigned();
		break;

	case Parameter::PARAMETER_TYPE_REAL:
        output << parameter.getReal();
		break;

	case Parameter::PARAMETER_TYPE_STRING:
        output << encode_attribute(parameter.getString());
		break;

	case Parameter::PARAMETER_TYPE_BLOB:
        {
            const Parameter::BlobType& blob = parameter.getBlob();
            size_t index = 0;
            byte c3[3];
            for (size_t offset = 0;offset < blob.size();++offset)
            {
                c3[index++] = blob[offset];
                if (index == 3)
                {
                    output << 
                        BASE64_CHARS[(c3[0] & 0xFC) >> 2] << 
                        BASE64_CHARS[((c3[0] & 0x03) << 4) | ((c3[1] & 0xF0) >> 4)] << 
                        BASE64_CHARS[((c3[1] & 0x0F) << 2) | ((c3[2] & 0xC0) >> 6)] << 
                        BASE64_CHARS[c3[2] & 0x3F];
                    index = 0;
                }
            }
            if (index > 0)
            {
                output << BASE64_CHARS[(c3[0] & 0xFC) >> 2];
                if (index > 1)
                    output << BASE64_CHARS[((c3[0] & 0x03) << 4) | ((c3[1] & 0xF0) >> 4)] << 
                        BASE64_CHARS[(c3[1] & 0x0F) << 2];
                else
                    output << BASE64_CHARS[(c3[0] & 0x03) << 4] << '=';
                output << '=';
            }
        }
		break;

	OPS_DEFAULT_CASE_LABEL
	}
    output << '"';
}

void write_unique_item::operator()(const std::wstring& name) const
{
    const Schema& item_schema = schema.getSchema(name);
    const Schema::Mode mode = item_schema.getMode();
    if (!parent.hasUniqueItem(name))
    {
        switch (mode)
        {
        case Schema::MODE_NORMAL:
            return;

        case Schema::MODE_UNIQUE:
            return;

        case Schema::MODE_REQUIRED_UNIQUE:
            break;
 
        OPS_DEFAULT_CASE_LABEL
        }
    }
    const Item& item = parent.getUniqueItem(name);
    if (mode != Schema::MODE_UNIQUE && mode != Schema::MODE_REQUIRED_UNIQUE)
        throw InvalidDataItemState(Strings::format(L"Unique item '%ls' not marked as such in schema '%ls'.", 
            name.c_str(), schema.getName().c_str()));
    write_element(item_schema, item, output, level);
}

void write_item::operator()(const Item& item) const
{
    const std::wstring& name = item.getName();
    const Schema& item_schema = schema.getSchema(name);
    if (item_schema.getMode() != Schema::MODE_NORMAL)
        throw InvalidDataItemState(Strings::format(L"Normal item '%ls' not marked as such in schema '%ls'.", 
            name.c_str(), schema.getName().c_str()));
    write_element(item_schema, item, output, level);
}

//  Exit namespace
}
}
