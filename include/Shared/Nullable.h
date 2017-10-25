#ifndef _NULLABLE_H_INCLUDED_
#define _NULLABLE_H_INCLUDED_

#include <OPS_Core/Helpers.h>

#include <Reprise/Types.h>

namespace OPS
{
	namespace Shared
	{
		//! for Nullable initializing
		//! you can use it like this:
		//! \code
		//!
		//! void f(Shared::Nullable<MyStruct> param)
		//! {
		//!		...
		//! }
		//!
		//! void g()
		//! {
		//! 	...
		//! 	f(Shared::null);
		//! 	...
		//!		Shared::Nullable<MyStruct> n1 = Shared::null;
		//! 	...
		//!		Shared::Nullable<double> n2 = Shared::null;
		//!		...
		//! }
		//! \endcode

		struct Null{};
		static const Null null = {};

		template<typename T>
		class Nullable
		{
		public:

			Nullable()
				: m_value()
				, m_isNull(true)
			{
			}

			//! implicit conversion - initializing with null
			Nullable(Null)
				: m_value(),
				  m_isNull(true)
			{
			}

			Nullable(const T& value)
				: m_value(value)
				, m_isNull(false)
			{
			}

		public:

			Nullable<T>& operator = (const T& value)
			{
				m_value = value;
				m_isNull = false;

				return *this;
			}

			//! same as constructor - initializing with null
			Nullable<T>& operator = (Null)
			{
				reset();

				return *this;
			}

			operator T() const
			{
				OPS_ASSERT(!m_isNull);

				return m_value;
			}

			operator T&()
			{
				OPS_ASSERT(!m_isNull);

				return m_value;
			}

			T& value()
			{
				OPS_ASSERT(!m_isNull);

				return m_value;
			}

			//! alias (short version) of value()
			T* operator -> ()
			{
				return &value();
			}

			const T& value() const
			{
				OPS_ASSERT(!m_isNull);

				return m_value;
			}

			//! alias (short version) of value()
			const T* operator -> () const
			{
				return &value();
			}

			//! value with default value
			const T& value(const T& defaultValue) const
			{
				if (m_isNull)
				{
					return defaultValue;
				}

				return m_value;
			}

			bool isNull() const
			{
				return m_isNull;
			}

			void reset()
			{
				m_isNull = true;
			}

			bool operator == (const Nullable<T>& other) const
			{
				if (m_isNull && other.m_isNull)
				{
					return true;
				}

				if (m_isNull != other.m_isNull)
				{
					return false;
				}

				return m_value == other.m_value;
			}

			bool operator == (const T& other) const
			{
				if (m_isNull)
				{
					return false;
				}

				return m_value == other;
			}

			bool operator != (const Nullable<T>& other) const
			{
				return !(*this == other);
			}

			bool operator != (const T& other) const
			{
				return !(*this == other);
			}

			bool operator < (const T& other) const
			{
				return isNull() ? true : value() < other;
			}

			bool operator < (const Nullable<T>& other) const
			{
				if (isNull() && other.isNull())
				{
					// equal
					return false;
				}

				if (isNull() && !other.isNull())
				{
					// null less then anything
					return true;
				}

				if (!isNull() && other.isNull())
				{
					// null less then anything
					return false;
				}

				return value() < other.value();
			}

			bool operator <= (const Nullable<T>& other) const
			{
				return *this < other || (*this) == other;
			}

			bool operator > (const Nullable<T>& other) const
			{
				return !(*this <= other);
			}

		private:

			T m_value;
			bool m_isNull;
		};

		template<>
		class Nullable<double>
		{
		public:

			Nullable()
				: m_nanValue(m_NaN)
			{
			}

			//! implicit conversion - initializing with null
			Nullable(Null)
				: m_nanValue(m_NaN)
			{
			}

			Nullable(double value)
				: m_value(value)
			{
			}

		public:

			Nullable<double>& operator = (double value)
			{
				m_value = value;

				return *this;
			}

			//! same as constructor - initializing with null
			Nullable<double>& operator = (Null)
			{
				reset();

				return *this;
			}

			operator double() const
			{
				OPS_ASSERT(!isNull());

				return m_value;
			}

			double& value()
			{
				OPS_ASSERT(!isNull());

				return m_value;
			}

			//! alias (short version) of value()
			double* operator -> ()
			{
				OPS_ASSERT(!isNull());

				return &m_value;
			}

			double value() const
			{
				OPS_ASSERT(!isNull());

				return m_value;
			}

			//! alias (short version) of value()
			const double* operator -> () const
			{
				OPS_ASSERT(!isNull());

				return &m_value;
			}

			bool isNull() const
			{
				return m_nanValue == m_NaN;
			}

			void reset()
			{
				m_nanValue = m_NaN;
			}

			bool operator == (Nullable<double> other) const
			{
				if (isNull() && other.isNull())
				{
					return true;
				}

				if (isNull() != other.isNull())
				{
					return false;
				}

				return m_value == other.m_value;
			}

			bool operator == (double other) const
			{
				if (isNull())
				{
					return false;
				}

				return m_value == other;
			}

			bool operator != (Nullable<double> other) const
			{
				return !(*this == other);
			}

			bool operator != (double other) const
			{
				return !(*this == other);
			}

			bool operator < (double other) const
			{
				return isNull() ? true : value() < other;
			}

			bool operator < (Nullable<double> other) const
			{
				if (isNull() && other.isNull())
				{
					// equal
					return false;
				}

				if (isNull() && !other.isNull())
				{
					// null less then anything
					return true;
				}

				if (!isNull() && other.isNull())
				{
					// null less then anything
					return false;
				}

				return value() < other.value();
			}

			bool operator <= (Nullable<double> other) const
			{
				return *this < other || (*this) == other;
			}

			bool operator > (Nullable<double> other) const
			{
				return !(*this <= other);
			}

		private:

			union
			{
				double m_value;
				qword m_nanValue;
			};

		private:

			static const qword m_NaN = ~static_cast <qword> (0);
		};

		template<class T>
		T coalesce(Nullable<T> t1, T t2)
		{
			return t1.isNull() ? t2 : t1.value();
		}

		template<class T>
		T coalesce(Nullable<T> t1, Nullable<T> t2, T t3)
		{
			return t1.isNull() ? (t2.isNull() ? t3 : t2.value()) : t1.value();
		}

		template<class T>
		Nullable<T> coalesce(Nullable<T> t1, Nullable<T> t2)
		{
			return t1.isNull() ? t2 : t1;
		}

		template<class T>
		Nullable<T> coalesce(Nullable<T> t1, Nullable<T> t2, Nullable<T> t3)
		{
			return t1.isNull() ? (t2.isNull() ? t3 : t2) : t1;
		}
	}
}

#endif // _NULLABLE_H_INCLUDED_
