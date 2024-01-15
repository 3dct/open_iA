// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

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
int main(int /*argCount*/, char** /*argValues*/) {

#define END_TEST \
	std::cout << "Passed " << simpleTesterTestsPassed << " of " << (simpleTesterTestsPassed+simpleTesterTestsFailed) << " tests." << std::endl; \
	std::cout << "Overall: " << ((simpleTesterTestsFailed>0)? "FAILED" : "PASSED") << std::endl; \
	return simpleTesterTestsFailed; \
}
