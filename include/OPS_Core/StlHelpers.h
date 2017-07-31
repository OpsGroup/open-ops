#ifndef OPS_COMMON_CORE_STLHELPERS_H_
#define OPS_COMMON_CORE_STLHELPERS_H_

//	Standard includes
#include <algorithm>
#include <functional>
#include <valarray>
#include <cstring>

//	Local includes
#include "OPS_Core/Environment.h"
#include "OPS_Core/Exceptions.h"
#include "OPS_Core/Strings.h"

//	Enter namespace
namespace OPS
{
namespace StlHelpers
{
/*
    Container equality tester (in valarrays)
    Params: first valarray, second valarray
    Return: equality testing result
*/
template <class ValueType>
inline bool equal_valarray(const std::valarray<ValueType>& first_valarray, const std::valarray<ValueType>& second_valarray)
{
    if (&first_valarray == &second_valarray)
        return true;
    const size_t size = first_valarray.size();
    if (size != second_valarray.size())
        return false;
    return std::memcmp(&const_cast<std::valarray<ValueType>&>(first_valarray)[0], 
		&const_cast<std::valarray<ValueType>&>(second_valarray)[0], size * sizeof(ValueType)) == 0;
}

/*
    Container equality tester (using container zeroable pointers)
    Params: first container pointer or 0, second container pointer or 0
    Return: equality testing result
*/
template <class ContainerType>
bool equal_container_pointer(const ContainerType* const first, const ContainerType* const second)
{
	return ((first == 0 || first->empty()) && (second == 0 || second->empty())) ||
		(first != 0 && second != 0 && *first == *second);
}

/*
    Element finder with predicate
    Params: container to search, filter predicate
    Return: iterator, pointing to value location
*/
template <class ContainerType, class PredicateType>
inline typename ContainerType::iterator find_if(ContainerType& container, const PredicateType& predicate)
{
    return std::find_if(container.begin(), container.end(), predicate);
}

/*
    Container iterator (for_each based on container)
    Params: container to iterate, functor
    Return: resulting functor
*/
template <class ContainerType, class FunctorType>
inline FunctorType for_each(ContainerType& container, const FunctorType& functor)
{
    return std::for_each(container.begin(), container.end(), functor);
}

/*
    Unary functor combiner
    Params: base functor, result functor
    Return: none
*/
template <class BaseFunctorType, class ResultFunctorType>
struct combiner_unary : 
    public std::unary_function<typename BaseFunctorType::argument_type, typename ResultFunctorType::result_type>
{
    inline typename ResultFunctorType::result_type operator()(typename BaseFunctorType::argument_type argument)
    {
        return result(base(argument));
    }

    inline typename ResultFunctorType::result_type operator()(typename BaseFunctorType::argument_type argument) const
    {
        return result(base(argument));
    }

//  Members
//      Base functor
    BaseFunctorType base;
//      Result functor
    ResultFunctorType result;

//  Constructors/destructor
/*
        combiner_unary constructor
        Params: base functor, result functor
*/
    inline combiner_unary(const BaseFunctorType& base_functor, const ResultFunctorType& result_functor) :
        base(base_functor), result(result_functor)
    {
    }
};

/*
    Unary functor combiner creator
    Params: base functor, result functor
    Return: unary functor combiner
*/
template <class BaseFunctorType, class ResultFunctorType>
inline combiner_unary<BaseFunctorType, ResultFunctorType> combine_unary(
    const BaseFunctorType& base_functor, const ResultFunctorType& result_functor)
{
    return combiner_unary<BaseFunctorType, ResultFunctorType>(base_functor, result_functor);
}

/*
    Binary functor combiner
    Params: base functor, result functor
    Return: none
*/
template <class BaseFunctorType, class ResultFunctorType>
struct combiner_binary : 
    public std::binary_function<typename BaseFunctorType::first_argument_type, 
        typename BaseFunctorType::second_argument_type, typename ResultFunctorType::result_type>
{
    inline typename ResultFunctorType::result_type operator()(
        typename BaseFunctorType::first_argument_type first_argument,
        typename BaseFunctorType::second_argument_type second_argument)
    {
        return result(base(first_argument, second_argument));
    }

    inline typename ResultFunctorType::result_type operator()(
        typename BaseFunctorType::first_argument_type first_argument,
        typename BaseFunctorType::second_argument_type second_argument) const
    {
        return result(base(first_argument, second_argument));
    }

//  Members
//      Base functor
    BaseFunctorType base;
//      Result functor
    ResultFunctorType result;

//  Constructors/destructor
/*
        combiner_binary constructor
        Params: base functor, result functor
*/
    inline combiner_binary(const BaseFunctorType& base_functor, const ResultFunctorType& result_functor) :
        base(base_functor), result(result_functor)
    {
    }
};

/*
    Binary functor combiner creator
    Params: base functor, result functor
    Return: binary functor combiner
*/
template <class BaseFunctorType, class ResultFunctorType>
inline combiner_binary<BaseFunctorType, ResultFunctorType> combine_binary(
    const BaseFunctorType& base_functor, const ResultFunctorType& result_functor)
{
    return combiner_binary<BaseFunctorType, ResultFunctorType>(base_functor, result_functor);
}

/*
    std::binder1st reference argument version
    Params: <functor type>
*/
template <class FunctorType>
class binder1st_ref : 
    public std::unary_function<typename FunctorType::second_argument_type, typename FunctorType::result_type>
{
public:
	inline typename FunctorType::result_type operator()(typename FunctorType::second_argument_type argument) const
	{
	    return op(value, argument);
	}

//  Constructors/destructor
/*
        binder1st_ref constructor
        Params: functor, argument
*/
	inline binder1st_ref(const FunctorType& functor, typename FunctorType::first_argument_type argument) : 
        op(functor), value(argument)
    {
	}

protected:
//  Protected members
//      Functor
	FunctorType op;
//      Value
	typename FunctorType::first_argument_type value;

private:
//  Members
/*
        Prohibited assignment operator
        Params: source object
        Return: self
*/
    binder1st_ref& operator=(const binder1st_ref&);
};

/*
    std::bind1st reference argument version
    Params: functor, argument
    Return: binder1st_ref
*/
template <class FunctorType>
inline binder1st_ref<FunctorType> bind1st_ref(const FunctorType& functor, 
    typename FunctorType::first_argument_type argument)
{
    return binder1st_ref<FunctorType>(functor, argument);
}

/*
    std::binder2nd reference argument version
    Params: <functor type>
*/
template <class FunctorType>
class binder2nd_ref : 
    public std::unary_function<typename FunctorType::first_argument_type, typename FunctorType::result_type>
{
public:
	inline typename FunctorType::result_type operator()(typename FunctorType::first_argument_type argument) const
	{
	    return op(argument, value);
	}

//  Constructors/destructor
/*
        binder2nd_ref constructor
        Params: functor, argument
*/
	inline binder2nd_ref(const FunctorType& functor, typename FunctorType::second_argument_type argument) : 
        op(functor), value(argument)
    {
	}

protected:
//  Protected members
//      Functor
	FunctorType op;
//      Value
	typename FunctorType::second_argument_type value;

private:
//  Members
/*
        Prohibited assignment operator
        Params: source object
        Return: self
*/
    binder2nd_ref& operator=(const binder2nd_ref&);
};

/*
    std::bind2nd reference argument version
    Params: functor, argument
    Return: binder2nd_ref
*/
template <class FunctorType>
inline binder2nd_ref<FunctorType> bind2nd_ref(const FunctorType& functor, 
    typename FunctorType::second_argument_type argument)
{
    return binder2nd_ref<FunctorType>(functor, argument);
}

/*
    Pair first element extracting functor, constant version
    Params: <pair type>, pair value
    Return: pair first object
*/
template <class PairType>
struct const_pair1st : public std::unary_function<const PairType&, const typename PairType::first_type&>
{
    inline const typename PairType::first_type& operator()(const PairType& value) const
    {
        return value.first;
    }
};

/*
    Pair second element extracting functor, constant version
    Params: <pair type>, pair value
    Return: pair second object
*/
template <class PairType>
struct const_pair2nd : public std::unary_function<const PairType&, const typename PairType::second_type&>
{
    inline const typename PairType::second_type& operator()(const PairType& value) const
    {
        return value.second;
    }
};


/*
    Pointer maker functor
    Params: <object type>, object
    Return: object pointer
*/
template <class ObjectType>
struct make_pointer : public std::unary_function<ObjectType&, ObjectType*>
{
    inline ObjectType* operator()(ObjectType& object) const
    {
        return &object;
    }
};

//	Exit namespace
}
}

#endif                      // OPS_COMMON_CORE_STLHELPERS_H_
