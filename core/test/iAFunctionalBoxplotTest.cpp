#include "iASimpleTester.h"

#include <vector>

#include <iAFunctionalBoxplot.h>

namespace
{
	using TestArgType = size_t;
	using TestValType = int;

	const size_t TestFuncBandSize = 4;

	template <typename ArgType, typename ValType>
	void testFunc(iAFunction<ArgType, ValType> const & f, ArgType argMin, ArgType argMax,
		std::vector<ValType> expectedVals)
	{
		for (ArgType a = argMin; a <= argMax; ++a)
		{
			TestEqual(expectedVals[a], f.at(a));
		}
	}

	template <typename ArgType, typename ValType>
	void testFuncBand(iAFunctionBand<ArgType, ValType> const& band, ArgType argMin, ArgType argMax,
		std::vector<std::vector<ValType>> expectedVals)
	{
		for (ArgType a = argMin; a <= argMax; ++a)
		{
			TestEqual(expectedVals[a][0], band.getMin(a));
			TestEqual(expectedVals[a][1], band.getMax(a));
		}
	};
}

BEGIN_TEST
	iAFunction<TestArgType, TestValType> func1;
	func1.insert(std::make_pair(0, 20));
	func1.insert(std::make_pair(1, 30));
	func1.insert(std::make_pair(2, 20));
	func1.insert(std::make_pair(3, 20));

	iAFunction<TestArgType, TestValType> func2;
	func2.insert(std::make_pair(0, 25));
	func2.insert(std::make_pair(1, 31));
	func2.insert(std::make_pair(2, 22));
	func2.insert(std::make_pair(3, 24));

	iAFunction<TestArgType, TestValType> func3;
	func3.insert(std::make_pair(0, 19));
	func3.insert(std::make_pair(1, 28));
	func3.insert(std::make_pair(2, 17));
	func3.insert(std::make_pair(3, 18));

	iAFunction<TestArgType, TestValType> func4;
	func4.insert(std::make_pair(0, 45));
	func4.insert(std::make_pair(1, 9));
	func4.insert(std::make_pair(2, 34));
	func4.insert(std::make_pair(3, 27));

	std::vector<iAFunction<TestArgType, TestValType> * > functions;
	functions.push_back(&func1);
	functions.push_back(&func2);
	functions.push_back(&func4);
	functions.push_back(&func3);

	TestArgType argMin = 0;
	TestArgType argMax = 3;

	iAModifiedDepthMeasure<TestArgType, TestValType> measure;
	iAFunctionalBoxplot<TestArgType, TestValType> bp(functions, &measure, TestFuncBandSize);
	
	testFunc(bp.getMedian(), argMin, argMax, { 20, 30, 20, 20 } );
	testFuncBand<TestArgType, TestValType>(bp.getCentralRegion(), argMin, argMax, { {19,20}, {28,30}, {17,20}, {18,20} } );
	testFuncBand<TestArgType, TestValType>(bp.getEnvelope(), argMin, argMax, { {19,45}, {9,31}, {17,34}, {18,27} } );
	
	TestEqual(static_cast<size_t>(0), bp.getOutliers().size());
END_TEST
