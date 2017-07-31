/*
    OPS_Service/Serialization.h - OPS_Service module, Serialization header

*/

//  Multiple include guard start
#ifndef OPS_SERVICE_SERIALIZATION_H__
#define OPS_SERVICE_SERIALIZATION_H__

//  Standard includes
#include <list>
#include <map>
#include <set>
#include <string>
#include <valarray>
#include <vector>
#include <memory>

//  OPS includes
#include "OPS_Core/Exceptions.h"
#include "OPS_Core/Types.h"
#include "OPS_Core/StlHelpers.h"

//  Local includes

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Serialization
{

//  Constants and enums

//  Global classes
//      "Serialization error" exception
OPS_DEFINE_EXCEPTION_CLASS(SerializationError, OPS::RuntimeError)
//      "Data error" exception
OPS_DEFINE_EXCEPTION_CLASS(DataError, SerializationError)
//      "Data item already present" exception
OPS_DEFINE_EXCEPTION_CLASS(DataItemAlreadyPresent, DataError)
//      "Data item not found" exception
OPS_DEFINE_EXCEPTION_CLASS(DataItemNotFound, DataError)
//      "Data parameter already present" exception
OPS_DEFINE_EXCEPTION_CLASS(DataParameterAlreadyPresent, DataError)
//      "Invalid data item state" exception
OPS_DEFINE_EXCEPTION_CLASS(InvalidDataItemState, DataError)
//      "Data parameter not found" exception
OPS_DEFINE_EXCEPTION_CLASS(DataParameterNotFound, DataError)
//      "Data parameter incompatible type" exception
OPS_DEFINE_EXCEPTION_CLASS(DataParameterIncompatibleType, DataError)
//      "Schema error" exception
OPS_DEFINE_EXCEPTION_CLASS(SchemaError, SerializationError)
//      "Schema item already present" exception
OPS_DEFINE_EXCEPTION_CLASS(SchemaItemAlreadyPresent, SchemaError)
//      "Schema item not found" exception
OPS_DEFINE_EXCEPTION_CLASS(SchemaItemNotFound, SchemaError)
//      "Schema parameter already present" exception
OPS_DEFINE_EXCEPTION_CLASS(SchemaParameterAlreadyPresent, SchemaError)
//      "Schema parameter not found" exception
OPS_DEFINE_EXCEPTION_CLASS(SchemaParameterNotFound, SchemaError)
//      "XML error" exception
OPS_DEFINE_EXCEPTION_CLASS(XMLError, SerializationError)
//      "XML parse error" exception
OPS_DEFINE_EXCEPTION_CLASS(XMLParseError, XMLError)
//      "XML content error" exception
OPS_DEFINE_EXCEPTION_CLASS(XMLContentError, XMLError)
//      "XML data error" exception
OPS_DEFINE_EXCEPTION_CLASS(XMLDataError, XMLError)

//      Serialization parameter
class Parameter : public OPS::NonAssignableMix
{
public:
//  Constants and enums
//      Parameter type
    enum Type
    {
    //  Boolean type
        PARAMETER_TYPE_BOOL,
    //  Integer type
        PARAMETER_TYPE_INT,
    //  Unsigned type
        PARAMETER_TYPE_UNSIGNED,
    //  Long integer type
        PARAMETER_TYPE_LONG_INT,
    //  Long unsigned type
        PARAMETER_TYPE_LONG_UNSIGNED,
    //  Real (floating-point) type
        PARAMETER_TYPE_REAL,
    //  String type
        PARAMETER_TYPE_STRING,
    //  Blob type
        PARAMETER_TYPE_BLOB
	};

//	Classes
//		Blob type
	typedef std::valarray<byte> BlobType;

    Parameter(Type type, const std::wstring& name);

    Parameter(const Parameter& parameter);

    ~Parameter(void);

//	Methods
	Type getType(void) const;

    const std::wstring& getName(void) const;

	bool getBool(void) const;

	int getInt(void) const;

	unsigned getUnsigned(void) const;

	long_long_t getLongInt(void) const;

	unsigned_long_long_t getLongUnsigned(void) const;

	double getReal(void) const;

	const std::wstring& getString(void) const;

	const BlobType& getBlob(void) const;

	void setBool(bool value);

	void setInt(int value);

	void setUnsigned(unsigned value);

	void setLongInt(long_long_t value);

	void setLongUnsigned(unsigned_long_long_t value);

	void setReal(double value);

	void setString(const std::wstring& value);

	void setString(const wchar_t* value);

	void setBlob(const BlobType& value);

	bool operator==(const Parameter& parameter) const;

	bool operator!=(const Parameter& parameter) const;

private:
//  Private methods
    void checkType(Type type) const;

//  Members
//      Type
    Type m_type;
//      Name
    std::wstring m_name;
//      Values
    union value_type
    {
    //  Members
    //      Boolean value
        bool bool_value;
    //      Integer value
        int int_value;
    //      Unsigned value
        unsigned unsigned_value;
    //      Long integer value
        long_long_t long_int_value;
    //      Long unsigned value
        unsigned_long_long_t long_unsigned_value;
    //      Real value
        double real_value;
    //      Composite value
        void* composite_value;
    } m_value;
};

//      Serialization item
class Item : public OPS::NonAssignableMix
{
public:
//      Items type
    typedef std::list<Item> items_type;

//  Constructors/destructor
    explicit Item(const std::wstring& name);

    Item(const Item& item);

//  Methods
    const std::wstring& getName(void) const;

    Item& addItem(const std::wstring& name);

    Item& addItem(const Item& item);

	void removeItem(const std::wstring& name);

	void removeItem(Item* item);

    Item& addUniqueItem(const std::wstring& name);

    Item& addUniqueItem(const Item& item);

	bool hasUniqueItem(const std::wstring& name) const;

    Item& getUniqueItem(const std::wstring& name);

    const Item& getUniqueItem(const std::wstring& name) const;

    void removeUniqueItem(const std::wstring& name);

	bool hasSubitem(const Parameter& parameter) const;

	Item& findSubitem(const Parameter& parameter);

	const Item& findSubitem(const Parameter& parameter) const;

	items_type::iterator begin();

	items_type::iterator end();

	items_type::const_iterator begin() const;

	items_type::const_iterator end() const;

    template <class UnaryFunctorType>
    inline UnaryFunctorType processItems(const UnaryFunctorType& functor) const
    {
        return m_items.get() != 0 ? OPS::StlHelpers::for_each(*m_items, functor) : functor;
    }

    template <class UnaryFunctorType>
    inline UnaryFunctorType processUniqueItems(const UnaryFunctorType& functor) const
    {
        return processUniqueItemsHelper(functor);
    }

    template <class UnaryFunctorType>
    inline UnaryFunctorType processAllItems(const UnaryFunctorType& functor) const
    {
        return processItems(processUniqueItems(functor));
    }

    template <class UnaryFunctorType>
    inline UnaryFunctorType processParameters(const UnaryFunctorType& functor) const
    {
        return processParametersHelper(functor);
    }

    Parameter& addParameter(const Parameter& parameter);

    Parameter& addParameter(Parameter::Type type, const std::wstring& name);

    bool hasParameter(const std::wstring& name) const;

    Parameter& getParameter(const std::wstring& name);

    const Parameter& getParameter(const std::wstring& name) const;

     void removeParameter(const std::wstring& name);

	 bool operator==(const Item& item) const;

	 inline bool operator!=(const Item& item) const
	 {
		 return !(*this == item);
	 }

private:
//  Private classes
    typedef std::map<std::wstring, Item> unique_items_type;
    typedef std::map<std::wstring, Parameter> parameters_type;

//  Private methods
    template <class UnaryFunctorType>
    inline UnaryFunctorType processUniqueItemsHelper(const UnaryFunctorType& functor) const
    {
        return m_unique_items.get() != 0 ? 
            OPS::StlHelpers::for_each(*m_unique_items, OPS::StlHelpers::combine_unary(
                OPS::StlHelpers::const_pair2nd<unique_items_type::value_type>(), functor)).result : 
            functor;
    }

    template <class UnaryFunctorType>
    inline UnaryFunctorType processParametersHelper(const UnaryFunctorType& functor) const
    {
        return m_parameters.get() != 0 ? 
            OPS::StlHelpers::for_each(*m_parameters, OPS::StlHelpers::combine_unary(
                OPS::StlHelpers::const_pair2nd<parameters_type::value_type>(), functor)).result : 
            functor;
    }

//  Members
    std::wstring m_name;
    std::unique_ptr<items_type> m_items;
    std::unique_ptr<unique_items_type> m_unique_items;
    std::unique_ptr<parameters_type> m_parameters;
};

//      Parameter info
struct ParameterInfo
{
//  Constants and enums
    enum Mode
    {
    //  Unique parameter
        MODE_UNIQUE,
    //  Required unique parameter
        MODE_REQUIRED_UNIQUE
    };

//  Members
    Parameter::Type type;
    Mode mode;

//  Constructors/destructor
    ParameterInfo(Parameter::Type parameter_type, Mode parameter_mode);
};

//      Serialization schema
class Schema
{
public:
//  Constants and enums
    enum Mode
    {
    //  Normal
        MODE_NORMAL,
    //  Unique item
        MODE_UNIQUE,
    //  Required unique item
        MODE_REQUIRED_UNIQUE
    };

//  Constructors/destructor
    Schema(const std::wstring& name, Mode mode);

//  Methods
    const std::wstring& getName(void) const;

    Mode getMode(void) const;

    bool hasSchema(const std::wstring& name) const;

    bool hasParameter(const std::wstring& name) const;

    Schema& addSchema(const std::wstring& name, Mode mode);

    Schema& addSchema(const Schema& schema);

    Schema& getSchema(const std::wstring& name);

    const Schema& getSchema(const std::wstring& name) const;

    void addParameter(const std::wstring& name, const ParameterInfo& info);

    void addParameter(const std::wstring& name, Parameter::Type type, ParameterInfo::Mode mode);

    const ParameterInfo& getParameterInfo(const std::wstring& name) const;

    template <class UnaryFunctorType>
    inline UnaryFunctorType processUniqueItems(const UnaryFunctorType& functor) const
    {
        for (ordering_type::const_iterator location = m_items_ordering.begin();
            location != m_items_ordering.end();
            ++location)
            functor(*location);
        return functor;
    }

    template <class UnaryFunctorType>
    inline UnaryFunctorType processParameters(const UnaryFunctorType& functor) const
    {
        for (ordering_type::const_iterator location = m_parameters_ordering.begin();
            location != m_parameters_ordering.end();
            ++location)
            functor(*location);
        return functor;
    }

private:
//  Private classes
    typedef std::map<std::wstring, ParameterInfo> parameters_type;
    typedef std::map<std::wstring, Schema> items_type;
    typedef std::vector<std::wstring> ordering_type;

//  Members
    std::wstring m_name;
    Mode m_mode;
    parameters_type m_parameters;
    items_type m_items;
    ordering_type m_items_ordering;
    ordering_type m_parameters_ordering;
};

/*
    Serializer

*/
class Serializer : virtual public OPS::NonCopyableMix
{
public:
//  Constructors/destructor
    explicit Serializer(const Schema& schema);

    virtual ~Serializer(void);

//  Methods
    const Schema& getSchema(void) const;

	virtual std::unique_ptr<Item> read(const std::wstring& filePath, bool skip_unknown_data = false) const = 0;

    virtual void write(const std::wstring& filePath, const Item& item) const = 0;

private:
//  Members
    const Schema m_schema;
};

/*
    XML serializer
*/
class XMLSerializer : public Serializer, virtual public OPS::NonCopyableMix
{
public:
//  Constructors/destructor
    explicit XMLSerializer(const Schema& schema);

    virtual ~XMLSerializer(void);

//  Methods
    virtual std::unique_ptr<Item> read(const std::wstring& filePath, bool skip_unknown_data = false) const;

    virtual void write(const std::wstring& filePath, const Item& item) const;

private:
//  Members
    mutable void* m_data;
};

//  Global functions

//  Exit namespace
}
}

//  Multiple include guard end
#endif 						//	OPS_SERVICE_SERIALIZATION_H__
