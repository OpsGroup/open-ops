#include "Analysis/LatticeGraph/RationalNumber.h"
#include "OPS_Core/Exceptions.h"
#include <iostream>
#include <cstdlib>
#include "OPS_Core/msc_leakcheck.h"

namespace OPS
{
namespace LatticeGraph 
{

RationalNumber::RationalNumber(long num, long denom):m_num(num),m_denom(denom)
{
	if (denom == 0) throw OPS::ArgumentError("Denominator of rational number should not be zero!!!");
	simplify();
}

RationalNumber& RationalNumber::operator+=(const RationalNumber &r) 
{
    set(m_num*r.m_denom + r.m_num*m_denom, m_denom*r.m_denom);
    return *this;
}

const RationalNumber RationalNumber::operator+(const RationalNumber &r) const
{
    RationalNumber result = *this;     
    result += r;
    return result;
}

RationalNumber& RationalNumber::operator-=(const RationalNumber &r) 
{
    set(m_num*r.m_denom - r.m_num*m_denom, m_denom*r.m_denom);
    return *this;
}

const RationalNumber RationalNumber::operator-(const RationalNumber &r) const
{
    RationalNumber result = *this;     
    result -= r;
    return result;
}

RationalNumber& RationalNumber::operator*=(const RationalNumber &r) 
{
    if (this->isZero() || r.isZero())
    {
        setZero();
        return *this;
    }

    set(m_num*r.m_num, m_denom*r.m_denom);
    return *this;
}

const RationalNumber RationalNumber::operator*(const RationalNumber &r) const
{
    RationalNumber result = *this;     
    result *= r;
    return result;
}

RationalNumber& RationalNumber::operator/=(const RationalNumber &r) 
{
	if (r.isZero()) throw OPS::ArgumentError("RationalNumber::div: Divisor should not be zero!!!");
	if (this->isZero()) return *this;
	set(m_num*r.m_denom, m_denom*r.m_num);
	return *this;
}

const RationalNumber RationalNumber::operator/(const RationalNumber &r) const
{
    RationalNumber result = *this;     
    result /= r;
    return result;
}

void RationalNumber::setNum(long num)
{
	set(num, m_denom);
}

void RationalNumber::setDenom(unsigned long denom)
{
	if (denom != 0)
    {
		set(m_num, denom);
    } 
    else throw OPS::ArgumentError("Denominator of rational number should not be zero!!!");
}

void RationalNumber::set(long num, long denom)
{
	if (denom == 0) throw OPS::ArgumentError("Denominator of rational number should not be zero!!!");

	if (denom < 0)
	{
		num = -num;
		denom = -denom;
	}

	m_num = num;
	m_denom = denom;
	simplify();
}

void RationalNumber::setZero()
{
	m_num = 0;
	m_denom = 1;
}

bool RationalNumber::operator==(const RationalNumber& r2) const
{
    return (m_num==r2.m_num) && (m_denom==r2.m_denom);
}

bool RationalNumber::operator !=(const RationalNumber& r2) const
{
	return !(*this == r2);
}

void RationalNumber::swap(RationalNumber &r)
{
	std::swap(m_num, r.m_num);
	std::swap(m_denom, r.m_denom);
}

static long findNod(long a, long b)//наибольший общий делитель
{
	if ((a==0)||(b==0)) throw OPS::ArgumentError("Can't find NOD of zero!!!");
	long x,y,r;
	a=abs(a); b=abs(b);
	x=a>b?a:b;//max
	y=a>b?b:a;//min
	while (y!=0) {
		r = x % y;
		x=y; y=r;
	}
	return x;
}

double RationalNumber::cast2Double()
{
    return (double)m_num / (double)m_denom;
}

void RationalNumber::simplify()
{
	if (isZero())
	{
		if (m_denom != 1)
			m_denom = 1;
	}
	else if (!isInteger())
	{
		long gcd = findNod(m_num, m_denom);
		if (gcd != 1)
		{
			m_num /= gcd;
			m_denom /= gcd;
		}
	}
}

std::ostream& operator<<(std::ostream& os, const RationalNumber& n)
{
	if (n.getDenom() == 1)
		os << n.getNum();
	else
		os << n.getNum() << "/" << n.getDenom();
	return os;
}

}//end of namespace
}//end of namespace
