#ifndef OPS_COMMON_CORE_MIXINS_H_
#define OPS_COMMON_CORE_MIXINS_H_

#include <typeinfo>

#define OPS_DEFINE_CLONABLE_INTERFACE(ObjectType_type) \
    virtual ObjectType_type* clone(void) const\
    {\
        return new ObjectType_type(*this);\
    }

namespace OPS
{

class NonCopyableMix
{
protected:
	inline NonCopyableMix()
	{
	}

	inline ~NonCopyableMix()
	{
	}

private:
	NonCopyableMix(const NonCopyableMix&);
	NonCopyableMix& operator=(const NonCopyableMix&);
};

class NonAssignableMix
{
protected:
	inline NonAssignableMix()
	{
	}

	inline ~NonAssignableMix()
	{
	}

private:
	NonAssignableMix& operator=(const NonAssignableMix&);
};

class TypeConvertibleMix
{
public:
	virtual ~TypeConvertibleMix(void)
	{
	}

    template <class InstanceType>
    inline bool is_a(void) const
    {
        return dynamic_cast<const InstanceType*>(this) != 0;
    }

    template <class ResultType>
    inline ResultType& cast_to(void)
    {
        ResultType* const result = dynamic_cast<ResultType*>(this);
        if (result == 0)
            castToErrorHandler(typeid(ResultType).name());
        return *result;
    }

    template <class ResultType>
    inline const ResultType& cast_to(void) const
    {
        const ResultType* const result = dynamic_cast<const ResultType*>(this);
        if (result == 0)
            castToErrorHandler(typeid(ResultType).name());
        return *result;
    }

    template <class ResultType>
    inline ResultType* cast_ptr(void)
    {
        ResultType* const result = dynamic_cast<ResultType*>(this);
        return result;
    }

    template <class ResultType>
    inline const ResultType* cast_ptr(void) const
    {
        const ResultType* const result = dynamic_cast<const ResultType*>(this);
        return result;
    }

private:
	void castToErrorHandler(const char* const typeName) const;
};


template <class BaseType>
class ClonableMix
{
public:
    typedef BaseType TClonableBase;
    virtual TClonableBase* clone(void) const = 0;
};


}

#endif                      // OPS_COMMON_CORE_MIXINS_H_
