#include "Analysis/LatticeGraph/GaussianElimination.h"
#include "OPS_Core/Exceptions.h"
#include <iostream>

namespace OPS
{
namespace LatticeGraph
{

GaussianEliminator::GaussianEliminator(const RationalMatrix &matrix)
	:m_matrix(matrix)
	,m_status(None)
{
}

GaussianEliminator::GaussianEliminator(const std::vector<std::vector<int> > &integerMatrix)
	:m_status(None)
{
	m_matrix.resize(integerMatrix.size());
	for(size_t i = 0; i < integerMatrix.size(); ++i)
	{
		for(size_t j = 0; j < integerMatrix[i].size(); ++j)
		{
			m_matrix[i].push_back(RationalNumber(integerMatrix[i][j]));
		}
	}
}

GaussianEliminator::Status GaussianEliminator::eliminate()
{
	makeForwardElimination();
	if (makeBackSubstitution())
	{
		if (!m_matrix.empty() && m_matrix.size() == m_matrix[0].size()-1)
			m_status = ExactSolution;
		else
			m_status = FundamentalSystemSolution;
	}
	else
	{
		m_status = Incompatible;
	}

	return m_status;
}

void GaussianEliminator::makeForwardElimination()
{
	const size_t NRows = m_matrix.size();
	const size_t NCols = m_matrix[0].size();

	size_t row = 0, col = 1;
	while(row < NRows && row < NCols && col < NCols)
	{
		// найти ненулевой элемент в столбце
		int nonZeroRow = -1;
		for(size_t i = row; i < NRows; ++ i)
			if (!m_matrix[i][col].isZero())
			{
				nonZeroRow = i;
				break;
			}

		if (nonZeroRow == -1)
		{
			col++;
			continue;
		}

		// переместить строку
		if ((int)row != nonZeroRow)
			m_matrix[row].swap(m_matrix[nonZeroRow]);

		// нормализуем строку
		for(size_t i = 0; i < NCols; ++i)
			m_matrix[row][i] /= m_matrix[row][col];

		// вычесть из остальных строк
		for(size_t i = row+1; i < NRows; ++i)
		{
			const RationalNumber a = m_matrix[i][col];
			for(size_t j = 0; j < NCols; ++j)
			{
				m_matrix[i][j] -= m_matrix[row][j]*a;
			}
		}

		row++;
		col++;
	}
}

bool GaussianEliminator::makeBackSubstitution()
{
	const size_t NRows = m_matrix.size();
	const size_t NCols = m_matrix[0].size();

	for(int row = NRows - 1; row > 0; --row)
	{
		int col = -1;
		// ищем ненулевой элемент
		for(size_t i = 1; i < NCols; ++i)
			if (!m_matrix[row][i].isZero())
			{
				col = i;
				break;
			}

		if (col != -1)
		{
			for(int i = row-1; i >= 0; --i)
			{
				const RationalNumber a = m_matrix[i][col];
				for(size_t j = col; j < NCols; ++j)
				{
					m_matrix[i][j] -= m_matrix[row][j]*a;
				}
			}
		}
		else
		{
			// все элементы в строке нулевые
			if (!m_matrix[row][0].isZero())
			{
				// система несовместна
				return false;
			}
			else
			{
				// удаляем нулевую строку
				m_matrix.erase(m_matrix.begin() + row);
			}
		}
	}

	return true;
}

const RationalMatrix& GaussianEliminator::getSolution() const
{
	if (m_status == Incompatible)
		throw OPS::StateError("GaussianEliminator : trying to get solution of incompatible system");
	return m_matrix;
}

GaussianEliminator::Status GaussianEliminator::getStatus() const
{
	return m_status;
}

bool GaussianEliminator::getIntegerSolution(std::vector<std::vector<int> > &solution) const
{
	if (m_status == Incompatible)
		throw OPS::StateError("GaussianEliminator : trying to get solution of incompatible system");

	solution.resize(m_matrix.size());

	for(size_t i = 0; i < m_matrix.size(); ++i)
	{
		solution[i].resize(m_matrix[i].size());
		for(size_t j = 0; j < m_matrix[i].size(); ++j)
		{
			if (m_matrix[i][j].isInteger())
				solution[i][j] = m_matrix[i][j].getNum();
			else
				return false;
		}
	}
	return true;
}

void GaussianEliminator::printMatrix()
{
	for(size_t i = 0; i < m_matrix.size(); ++i)
	{
		for(size_t j = 0; j < m_matrix[0].size(); ++j)
		{
			std::cout << m_matrix[i][j] << ", ";
		}
		std::cout << std::endl;
	}
	std::cout<< std::endl;
}
}
}
