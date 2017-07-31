#ifndef OPS_COMMON_CORE_EXCEPTIONS_H_
#define OPS_COMMON_CORE_EXCEPTIONS_H_

//	Standard includes
#include <string>
#include <exception>
#include "OPS_Core/Mixins.h"

namespace OPS
{

/**
	Класс Exception является базовым классом исключений в системе OPS.


	Пример.

	\code
  	try
	{
		throw Exception("Exception!");
	}
	catch (OPSException& e) 
	{
		cout << e.GetErrorText();
	}
	\endcode	

*/
class Exception : public std::exception, virtual public ClonableMix<Exception>
{
public:
	/// Создать экземпляр класса исключений
	/**
	Конструктор

	\param message	-	Сообщение об ошибке
	*/
	explicit Exception(const std::string& message) throw();

	/// Создать экземпляр класса исключений
	/**
	Конструктор

	\param message	-	Сообщение об ошибке
	*/
	explicit Exception(const std::wstring& message) throw();

	virtual ~Exception() throw() {}

	/// Возвращяет текст сообщения об ошибке
	const std::string& getMessage() const;

	/// Возвращяет текст сообщения об ошибке
	virtual const char* what(void) const throw();

	/// Добавляет к существующему сообщению еще одно
	void appendMessage(const std::string& message);

	/// Добавляет к существующему сообщению еще одно
	void appendMessage(const std::wstring& message);

	OPS_DEFINE_CLONABLE_INTERFACE(Exception)

protected:

	std::string m_strErrorMsg;
};

/**
    "System call error" exception
*/
class SystemCallError : public Exception
{
public:
    explicit SystemCallError(const std::string& origin);
	~SystemCallError() throw() {}

    SystemCallError(const std::string& origin, int errorCode);

    inline int getErrorCode(void) const
    {
        return m_errorCode;
    }

    inline const std::string& getOrigin(void) const
    {
        return m_origin;
    }

    std::string getErrorId(void) const;

    std::string getErrorMessage(void) const;

	OPS_DEFINE_CLONABLE_INTERFACE(SystemCallError)

private:
//  Members
//      Origin
    std::string m_origin;
//      Error code
    int m_errorCode;
};

/**
    "OS call error" exception
*/
class OSCallError : public Exception
{
public:
    explicit OSCallError(const std::string& origin);
	~OSCallError() throw() {}

    OSCallError(const std::string& origin, int errorCode);

    inline int getErrorCode(void) const
    {
        return m_errorCode;
    }

    inline const std::string& getOrigin(void) const
    {
        return m_origin;
    }

    std::string getErrorId(void) const;

    std::string getErrorMessage(void) const;

	OPS_DEFINE_CLONABLE_INTERFACE(OSCallError)

private:
//  Members
//      Origin
    std::string m_origin;
//      Error code
    int m_errorCode;
};


/**
	Макрос для конструирования простых типов исключений.

	\param	NewException - класс нового исключения
	\param	BasedOnException - класс исключения, на котором базируется данное исключение
*/
#define OPS_DEFINE_EXCEPTION_CLASS(NewException, BasedOnException) \
class NewException : public BasedOnException\
{\
public:\
	explicit NewException(const std::string& message) :\
		BasedOnException(message)\
	{\
	}\
\
	explicit NewException(const std::wstring& message) :\
		BasedOnException(message)\
	{\
	}\
};


OPS_DEFINE_EXCEPTION_CLASS(RuntimeError, OPS::Exception)

OPS_DEFINE_EXCEPTION_CLASS(ArgumentError, OPS::RuntimeError)
OPS_DEFINE_EXCEPTION_CLASS(StateError, OPS::RuntimeError)

OPS_DEFINE_EXCEPTION_CLASS(TypeCastError, OPS::RuntimeError)
OPS_DEFINE_EXCEPTION_CLASS(UnsignedToSignedOverflowError, OPS::TypeCastError)

OPS_DEFINE_EXCEPTION_CLASS(AssertionFailed, OPS::RuntimeError)


}

#endif                      // OPS_COMMON_CORE_EXCEPTIONS_H_
