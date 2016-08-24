/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include <cstddef>
#include <iostream>
#include <cmath>

int simpleTesterTestsPassed = 0;
int simpleTesterTestsFailed = 0;

char const * TestPassed = "Test passed. ";
char const * TestNotPassed = "Test NOT passed";

void PrivateTestAssert(bool expression, char const * expressionStr)
{
	if (expression)
	{
		++simpleTesterTestsPassed;
		if (expressionStr)
		{
			std::cout << TestPassed << expressionStr << " true as expected." << std::endl;
		}
	}
	else
	{
		++simpleTesterTestsFailed;
		if (expressionStr)
		{
			std::cout << TestNotPassed << expressionStr << " NOT true!" << std::endl;
		}
	}
}

template <typename T>
void PrivateTestEqual(T const & expected, T const & actual, char const * expectedStr, char const * actualStr)
{
	bool equal = (expected == actual);
	if (equal)
	{
		std::cout << TestPassed << expectedStr << "(="<<expected<<") == " << actualStr << "(="<<actual<<")"<<std::endl;
	}
	else
	{
		std::cout << TestNotPassed << " (" << expectedStr << " = " << actualStr << ") "
			<< " got " << expected << " != " << actual << " instead " << std::endl;
	}
	PrivateTestAssert(equal, 0);
}

const double MyEpsilon = 0.00001;

template <typename T>
void PrivateTestEqualFloatingPoint(T const & expected, T const & actual, char const * expectedStr, char const * actualStr)
{
	double diff = static_cast<double>(expected) - actual;
	bool equal = std::abs(diff) < MyEpsilon;// std::numeric_limits<T>::epsilon();
	if (equal)
	{
		std::cout << TestPassed << expectedStr << " == " << actualStr << std::endl;
	}
	else
	{
		std::cout << TestNotPassed << " (" << expectedStr << " = " << actualStr << ") "
			<< " got " << expected << " != " << actual << " instead " << std::endl;
	}
	PrivateTestAssert(equal, 0);
}

#define TestAssert(expression) \
	PrivateTestAssert(expression, #expression)

#define TestEqual(expected, actual) \
	PrivateTestEqual(expected, actual, #expected, #actual)

#define TestEqualFloatingPoint(expected, actual) \
	PrivateTestEqualFloatingPoint(expected, actual, #expected, #actual)


#define BEGIN_TEST \
int main(int argc, char** argv) {

#define END_TEST \
	std::cout << "Passed " << simpleTesterTestsPassed << " of " << (simpleTesterTestsPassed+simpleTesterTestsFailed) << " tests." << std::endl; \
	std::cout << "Overall: " << ((simpleTesterTestsFailed>0)? "FAILED" : "PASSED") << std::endl; \
	return simpleTesterTestsFailed; \
}
