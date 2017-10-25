#pragma once

#include <iosfwd>

namespace OPS 
{ 

namespace LatticeGraph 
{ 

class RationalNumber  
{
public:

	explicit RationalNumber(long num=0, long denom=1);

    RationalNumber& operator+=(const RationalNumber &r);

    const RationalNumber operator+(const RationalNumber &r) const;

    RationalNumber& operator-=(const RationalNumber &r);

    const RationalNumber operator-(const RationalNumber &r) const;

    RationalNumber& operator*=(const RationalNumber &r);

    const RationalNumber operator*(const RationalNumber &r) const;

    RationalNumber& operator/=(const RationalNumber &r);

    const RationalNumber operator/(const RationalNumber &r) const;

	long getNum() const { return m_num; }

	long getDenom() const { return m_denom; }

    void setNum(long num);

    void setDenom(unsigned long denom);

	void set(long num, long denom);

    void setZero();

    double cast2Double();

	bool isZero() const { return m_num == 0; }
	bool isOne() const { return m_num == 1 && m_denom == 1; }
	bool isInteger() const { return m_denom == 1; }

    bool operator==(const RationalNumber& r2) const;
	bool operator!=(const RationalNumber& r2) const;

	void swap(RationalNumber& r);

private:

    void simplify();//сокращает

    long m_num; //числитель

    unsigned long m_denom; //знаменатель
};

std::ostream& operator<<(std::ostream& os, const RationalNumber&);

}//end of namespace
}//end of namespace
