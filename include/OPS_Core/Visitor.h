#ifndef OPS_COMMON_CORE_VISITOR_H_
#define OPS_COMMON_CORE_VISITOR_H_

/*
	Acyclic visitor pattern.
	Implementation taken from Andrei Alexandrescu "Modern C++ Design" book (Chapter 10)
*/

#define OPS_DEFINE_VISITABLE()	\
	virtual ReturnType accept(OPS::BaseVisitor& guest)	\
	{	return acceptImpl(*this, guest);	}	



namespace OPS
{

//	Visitor part
class BaseVisitor
{
public:
	virtual ~BaseVisitor()
	{
	}
};

template <class T, typename R = void>
class Visitor
{
public:
	typedef R ReturnType;
	virtual ~Visitor() { }
	virtual ReturnType visit(T&) = 0;
};

//	Visitable part
template <typename R = void>
class BaseVisitable
{
public:
	typedef R ReturnType;
	virtual ~BaseVisitable()
	{
	}
	virtual ReturnType accept(BaseVisitor&) = 0;

protected:
	template <class T>
	static ReturnType acceptImpl(T& visited, BaseVisitor& guest)
	{
		if (Visitor<T, R>* p = dynamic_cast<Visitor<T, R>*>(&guest))
		{
			return p->visit(visited);
		}
		return ReturnType();
	}
};

}

#endif                      // OPS_COMMON_CORE_VISITOR_H_
