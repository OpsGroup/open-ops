#ifndef EXPRESSION_OPTIMIZATION_LITERAL_TYPES_LIST_H
#define EXPRESSION_OPTIMIZATION_LITERAL_TYPES_LIST_H

#include "Typelist.h"
#include "Reprise/Reprise.h"

namespace OPS
{
namespace ExpressionSimplifier
{
namespace Helpers
{

using Loki::Typelist;
using Loki::Select;
using Loki::TL::MakeTypelist;
using OPS::Reprise::BasicType;

template<int val, typename T>
struct LiteralMap
{
    enum { key = val };
    typedef T type;
};

typedef MakeTypelist<
	LiteralMap<BasicType::BT_BOOLEAN, bool>,

	LiteralMap<BasicType::BT_CHAR, char>,
	LiteralMap<BasicType::BT_WIDE_CHAR, wchar_t>,
	
	LiteralMap<BasicType::BT_FLOAT32, float>,
	LiteralMap<BasicType::BT_FLOAT64, double>,

	LiteralMap<BasicType::BT_INT8,  sbyte>,
	LiteralMap<BasicType::BT_INT16, sword>,
	LiteralMap<BasicType::BT_INT32, sdword>,
	LiteralMap<BasicType::BT_INT64, sqword>,
	
			
	LiteralMap<BasicType::BT_UINT8,  byte>,
	LiteralMap<BasicType::BT_UINT16, word>,
	LiteralMap<BasicType::BT_UINT32, dword>,
	LiteralMap<BasicType::BT_UINT64, qword>,		
	
	LiteralMap<BasicType::BT_STRING, std::string>,
	LiteralMap<BasicType::BT_WIDE_STRING, std::wstring> >::Result LiteralTypesList;  


typedef MakeTypelist<
	LiteralMap<BasicType::BT_BOOLEAN, bool>,

	LiteralMap<BasicType::BT_CHAR, char>,
	LiteralMap<BasicType::BT_WIDE_CHAR, wchar_t>,
	
	LiteralMap<BasicType::BT_FLOAT32, float>,
	LiteralMap<BasicType::BT_FLOAT64, double>,

	LiteralMap<BasicType::BT_INT8,  sbyte>,
	LiteralMap<BasicType::BT_INT16, sword>,
	LiteralMap<BasicType::BT_INT32, sdword>,
	LiteralMap<BasicType::BT_INT64, sqword>,
	
			
	LiteralMap<BasicType::BT_UINT8,  byte>,
	LiteralMap<BasicType::BT_UINT16, word>,
	LiteralMap<BasicType::BT_UINT32, dword>,
	LiteralMap<BasicType::BT_UINT64, qword> >::Result NumericTypesList;


	template<typename TList, int TypeLabel> struct TypeByLabel;
	
	template<typename Head, typename Tail, int TypeLabel>
	struct TypeByLabel<Typelist<Head, Tail>, TypeLabel>
	{
	private:
		typedef typename TypeByLabel<Tail, TypeLabel>::Result temp;
	public:
		typedef typename Select<TypeLabel == Head::key, typename Head::type, temp>::Result Result;		
	};
	
	template<int TypeLabel>
	struct TypeByLabel<Loki::NullType, TypeLabel>
	{
		typedef Loki::NullType Result;	
	};	
}
}
}

#endif

