#ifndef EXPRESSION_OPTIMIZATION_ACCESSORS_H
#define EXPRESSION_OPTIMIZATION_ACCESSORS_H

#include "Reprise/Reprise.h"
#include "LiteralTypesLists.h"

namespace OPS
{
namespace ExpressionSimplifier
{
namespace Helpers
{
	using OPS::Reprise::StrictLiteralExpression;
	using OPS::Reprise::BasicType;
	using OPS::Reprise::BasicCallExpression;

	class Getter
	{
	public:		
		template<int TypeLabel> 
                static inline typename TypeByLabel<LiteralTypesList, TypeLabel>::Result
			Get(const StrictLiteralExpression* arg) 
	    { 			  
                        return typename TypeByLabel<LiteralTypesList, TypeLabel>::Result();
	    }        

	};   

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_CHAR>::Result
                    Getter::Get<BasicType::BT_CHAR>(const StrictLiteralExpression* arg)
        {
            return arg->getChar();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_WIDE_CHAR>::Result
                    Getter::Get<BasicType::BT_WIDE_CHAR>(const StrictLiteralExpression* arg)
        {
            return arg->getWideChar();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_FLOAT32>::Result
                    Getter::Get<BasicType::BT_FLOAT32>(const StrictLiteralExpression* arg)
        {
            return arg->getFloat32();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_FLOAT64>::Result
                    Getter::Get<BasicType::BT_FLOAT64>(const StrictLiteralExpression* arg)
        {
            return arg->getFloat64();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_INT8>::Result
                    Getter::Get<BasicType::BT_INT8>(const StrictLiteralExpression* arg)
        {
            return arg->getInt8();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_INT16>::Result
                    Getter::Get<BasicType::BT_INT16>(const StrictLiteralExpression* arg)
        {
            return arg->getInt16();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_INT32>::Result
                    Getter::Get<BasicType::BT_INT32>(const StrictLiteralExpression* arg)
        {
            return arg->getInt32();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_INT64>::Result
                    Getter::Get<BasicType::BT_INT64>(const StrictLiteralExpression* arg)
        {
            return arg->getInt64();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_UINT8>::Result
                    Getter::Get<BasicType::BT_UINT8>(const StrictLiteralExpression* arg)
        {
            return arg->getUInt8();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_UINT16>::Result
                    Getter::Get<BasicType::BT_UINT16>(const StrictLiteralExpression* arg)
        {
            return arg->getUInt16();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_UINT32>::Result
                    Getter::Get<BasicType::BT_UINT32>(const StrictLiteralExpression* arg)
        {
            return arg->getUInt32();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_UINT64>::Result
                    Getter::Get<BasicType::BT_UINT64>(const StrictLiteralExpression* arg)
        {
            return arg->getUInt64();
        }
			
			template<>
			inline TypeByLabel<LiteralTypesList, BasicType::BT_BOOLEAN>::Result
				Getter::Get<BasicType::BT_BOOLEAN>(const StrictLiteralExpression* arg)
		{
				return arg->getBoolean();
		}

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_STRING>::Result
                    Getter::Get<BasicType::BT_STRING>(const StrictLiteralExpression* arg)
        {
            return arg->getString();
        }

            template<>
            inline TypeByLabel<LiteralTypesList, BasicType::BT_WIDE_STRING>::Result
                    Getter::Get<BasicType::BT_WIDE_STRING>(const StrictLiteralExpression* arg)
        {
            return arg->getWideString();
        }

	class Setter
	{
	public:		
		template<int TypeLabel> 
		static inline StrictLiteralExpression* Set
			(StrictLiteralExpression* const arg, 
			typename TypeByLabel<LiteralTypesList, TypeLabel>::Result value) 
	    { 			 			
			return arg;
	    }           

        };



           template<>
        inline StrictLiteralExpression* Setter::Set<BasicType::BT_WIDE_CHAR>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_WIDE_CHAR>::Result value)
       {
                   arg->setWideChar(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_FLOAT32>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_FLOAT32>::Result value)
       {
                   arg->setFloat32(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_FLOAT64>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_FLOAT64>::Result value)
       {
                   arg->setFloat64(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_INT8>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_INT8>::Result value)
       {
                   arg->setInt8(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_INT16>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_INT16>::Result value)
       {
                   arg->setInt16(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_INT32>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_INT32>::Result value)
       {
                   arg->setInt32(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_INT64>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_INT64>::Result value)
       {
                   arg->setInt64(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_UINT8>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_UINT8>::Result value)
       {
                   arg->setUInt8(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_UINT16>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_UINT16>::Result value)
       {
                   arg->setUInt16(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_UINT32>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_UINT32>::Result value)
       {
                   arg->setUInt32(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_UINT64>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_UINT64>::Result value)
       {
                   arg->setUInt64(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_STRING>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_STRING>::Result value)
       {
                   arg->setString(value);
                   return arg;
       }

           template<>
           inline StrictLiteralExpression* Setter::Set<BasicType::BT_WIDE_STRING>
                   (StrictLiteralExpression* const arg,
                   TypeByLabel<LiteralTypesList, BasicType::BT_WIDE_STRING>::Result value)
       {
                   arg->setWideString(value);
                   return arg;
       }

}
}
}

#define GETTER(TypeLabel, liter) Getter::Get<TypeLabel>(liter)
#define SETTER(TypeLabel, liter, Value) Setter::Set<TypeLabel>(liter, Value)

#endif //ACCESSORS_H 
