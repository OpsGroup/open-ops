#ifndef GAUSSIANELIMINATION_H
#define GAUSSIANELIMINATION_H

#include <vector>
#include "Analysis/LatticeGraph/RationalNumber.h"

namespace OPS
{

namespace LatticeGraph
{

typedef std::vector<RationalNumber> RationalMatrixRow;
typedef std::vector<RationalMatrixRow> RationalMatrix;

class GaussianEliminator
{
public:
	enum Status
	{
		None = 0,
		ExactSolution,
		FundamentalSystemSolution,
		Incompatible
	};

	explicit GaussianEliminator(const RationalMatrix& matrix);
	explicit GaussianEliminator(const std::vector<std::vector<int> >& integerMatrix);

	Status eliminate();

	const RationalMatrix& getSolution() const;
	Status getStatus() const;

	bool getIntegerSolution(std::vector<std::vector<int> >& solution) const;

private:
	void makeForwardElimination();
	bool makeBackSubstitution();

	void printMatrix();

	RationalMatrix m_matrix;
	Status m_status;
};

}
}
#endif // GAUSSIANELIMINATION_H
