/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAParameterGeneratorImpl.h"

#include "iAAttributes.h"

#include <iAAttributeDescriptor.h>
#include <iAConsole.h>
#include <iAMathUtility.h>
#include <iAStringHelper.h>

#include <cmath>
#include <random>


iAParameterGenerator::~iAParameterGenerator()
{}

class RandomGenerator
{
public:
	virtual ~RandomGenerator() {}
	virtual double next() =0;
};

class DblLinRandom: public RandomGenerator
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng;
public:
	DblLinRandom(double min, double max):
		dist(min, max)
	{
		rng.seed(std::random_device{}()); //Initialize with non-deterministic seeds
	}
	virtual double next()
	{
		return dist(rng);
	}
};

class DblLogRandom: public RandomGenerator
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng;
public:
	DblLogRandom(double min, double max):
		dist(std::log(min), std::log(max))
	{
		assert(min>0);
		rng.seed(std::random_device{}()); //Initialize with non-deterministic seeds
	}
	virtual double next()
	{
		return exp(dist(rng));
	}
};


class IntLinRandom: public RandomGenerator
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng; //Mersenne Twister: Good quality random number generator
	int m_min;
	int m_max;
public:

	IntLinRandom(int min, int max) :
		dist(0, 1),
		m_min(min),
		m_max(max)
	{
		rng.seed(std::random_device{}()); //Initialize with non-deterministic seeds
	}
	//! return a random number between 0 and max-1, uniformly distributed
	double next()
	{
		return clamp(m_min, m_max,
			static_cast<int>(m_min +  dist(rng) * (m_max-m_min+1))
		);
	}
};

class IntLogRandom: public RandomGenerator
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng; //Mersenne Twister: Good quality random number generator
	int m_min;
	int m_max;
public:
	IntLogRandom(int min, int max) :
		dist(0, 1),
		m_min(min),
		m_max(max)
	{
		rng.seed(std::random_device{}()); //Initialize with non-deterministic seeds
	}
	//! return a random number between 0 and max-1, uniformly distributed
	double next()
	{
		double logMin = std::log(m_min);
		double logRange = std::log(m_max + 1) - logMin;
		return
			clamp(m_min, m_max,
				static_cast<int>(
					exp(logMin + dist(rng) * logRange)
				)
			);
	}
};

class MyExtDblRandom
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng;
public:
	MyExtDblRandom():
		dist(0, 1)
	{
		rng.seed(std::random_device{}()); //Initialize with non-deterministic seeds
	}
	double next(double min, double max)
	{
		return mapNormTo(min, max, dist(rng));
	}
};

template <typename T>
class MyRangeRandom
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng; //Mersenne Twister: Good quality random number generator
public:

	MyRangeRandom():
		dist(0, 1)
	{
		rng.seed(std::random_device{}()); //Initialize with non-deterministic seeds
	}
	//! return a random number between 0 and max-1, uniformly distributed
	T next(T max)
	{
		return clamp(static_cast<T>(0), max-1, static_cast<T>(dist(rng) * max));
	}
};

QSharedPointer<RandomGenerator> CreateRand(bool log, double min, double max, iAValueType valueType)
{
	switch (valueType)
	{
	case Discrete:
	case Categorical:
		if (log) return QSharedPointer<RandomGenerator>(new IntLogRandom(min, max));
		    else return QSharedPointer<RandomGenerator>(new IntLinRandom(min, max));
	default:
	case Continuous:
		if (log) return QSharedPointer<RandomGenerator>(new DblLogRandom(min, max));
		else return QSharedPointer<RandomGenerator>(new DblLinRandom(min, max));
	}
}

QString iARandomParameterGenerator::name() const
{
	return QString("Random");
}

ParameterSetsPointer iARandomParameterGenerator::GetParameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount)
{
	ParameterSetsPointer result(new ParameterSets);

	QVector<QSharedPointer<RandomGenerator> > random;
	for (int p = 0; p < parameter->size(); ++p)
	{
		random.push_back(CreateRand(
			parameter->at(p)->isLogScale(),
			parameter->at(p)->min(),
			parameter->at(p)->max(),
			parameter->at(p)->valueType()
		));
	}

	for (int s = 0; s < sampleCount; ++s)
	{
		ParameterSet set;
		for (int p = 0; p < parameter->size(); ++p)
		{
			set.push_back(random[p]->next());
		}
		result->push_back(set);
	}
	return result;
}


template <typename T>
T pop_at(QVector<T>& v, typename QVector<T>::size_type n)
{
	assert(n < v.size());
	T result = v[n];
	std::swap(v[n], v.back());
	v.pop_back();
	return result;
}


class iARange
{
public:
	virtual ~iARange() {}
	virtual double min(int i) =0;
	virtual double max(int i) =0;
};


class iALinRange: public iARange
{
public:
	iALinRange(double min, double max, int count):
		m_min(min),
		m_step((max-min)/count)
	{}
	double min(int i) override
	{
		return m_min+ i*m_step;
	}
	double max(int i) override
	{
		return m_min+ (i+1)*m_step;
	}
private:
	double m_min;
	double m_step;
};

class iALogRange: public iARange
{
public:
	iALogRange(double min, double max, int count):
		m_min(min),
		m_factor(std::pow(max/min, 1.0/count))
	{
		assert(min > 0);
	}
	double min(int i) override
	{
		return m_min*std::pow(m_factor, i+1);
	}
	double max(int i) override
	{
		return m_min*std::pow(m_factor, i+1);
	}
private:
	double m_min;
	double m_factor;
};

QSharedPointer<iARange> CreateRange(bool log, double min, double max, int count, iAValueType valueType)
{
	if (log)
	{
		assert(valueType != Categorical);
		return QSharedPointer<iARange>(new iALogRange(min, max + ((valueType == Discrete) ? 0.999999 : 0), count));
	}
	else return QSharedPointer<iARange>(new iALinRange(min, max + ((valueType == Categorical || valueType == Discrete) ? 0.999999 : 0), count));
}


QString iALatinHypercubeParameterGenerator::name() const
{
	return QString("Latin HyperCube");
}

ParameterSetsPointer iALatinHypercubeParameterGenerator::GetParameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount)
{
	ParameterSetsPointer result(new ParameterSets);

	// for each parameter, create a "range", dividing its interval into sampleCount pieces
	QVector<QSharedPointer<RandomGenerator> > random;
	QVector<QVector<double> > sampleValues;

	MyExtDblRandom dblRand;
	for (int p = 0; p < parameter->size(); ++p)
	{
		iAValueType valueType = parameter->at(p)->valueType();
		QSharedPointer<iARange> range = CreateRange(parameter->at(p)->isLogScale(),
			parameter->at(p)->min(),
			parameter->at(p)->max(),
			sampleCount,
			valueType);

		sampleValues.push_back(QVector<double>());
		// iterate over sampleCount, and for each parameter, create one value per piece
		for (int s = 0; s < sampleCount; ++s)
		{
			// TODO: special handling for log? otherwise within the piece, we have linear distribution
			double value = dblRand.next(range->min(s), range->max(s));
			if (valueType == Discrete || valueType == Categorical)
			{
				value = static_cast<int>(value);
			}
			sampleValues[p].push_back(value);
		}
	}

	MyRangeRandom<int> intRand;
	// iterate over sampleCount, and for each parameter, randomly select one of the pieces
	for (int s = sampleCount; s > 0 ; --s)
	{
		ParameterSet set;
		for (int p = 0; p < parameter->size(); ++p)
		{
			// randomly select one of the previously chosen values, and put it into the parameter set
			set.push_back(pop_at(sampleValues[p], intRand.next(s)));
		}
		result->push_back(set);
	}
	return result;
}

QString iACartesianGridParameterGenerator::name() const
{
	return QString("Cartesian Grid");
}

ParameterSetsPointer iACartesianGridParameterGenerator::GetParameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount)
{
	ParameterSetsPointer result(new ParameterSets);
	int samplesPerParameter = static_cast<int>(std::pow(10, std::log10(sampleCount) / parameter->size()));
	samplesPerParameter = std::max(2, samplesPerParameter); // at least 2 sample values per parameter

	// calculate actual sample count (have to adhere to grid structure / powers):
	// maybe get sample count per parameter?
	int actualSampleCount = std::pow(samplesPerParameter, parameter->size());
/*
	DEBUG_LOG(QString("param. count: %1, samples/param.: %2, targeted samples: %3, actual samples: %4")
		.arg(parameter->size())
		.arg(samplesPerParameter)
		.arg(sampleCount)
		.arg(actualSampleCount)
	);
*/

	QVector<QSharedPointer<iARange>> ranges;
	for (int p = 0; p < parameter->size(); ++p)
	{
		iAValueType valueType = parameter->at(p)->valueType();
		ranges.push_back(
			CreateRange(
				parameter->at(p)->isLogScale(),
				parameter->at(p)->min(),
				parameter->at(p)->max(),
				samplesPerParameter-1, // -1 because we choose from the edges of the range
				valueType)
			);
	}
	// to keep track of which grid index for which parameter we are currently using
	QVector<int> parameterRangeIdx(parameter->size(), 0);

	for (int sampleIdx = 0; sampleIdx < actualSampleCount; ++sampleIdx)
	{
		ParameterSet set;
		for (int p = 0; p < parameter->size(); ++p)
		{
			double value = ranges[p]->min(parameterRangeIdx[p]);
			iAValueType valueType = parameter->at(p)->valueType();
			if (valueType == Discrete || valueType == Categorical)
			{
				value = static_cast<int>(value);
			}
			set.push_back(value);
		}
		result->append(set);
		//DEBUG_LOG(QString("%1: %2").arg(joinAsString(parameterRangeIdx, ",")).arg(joinAsString(result->at(result->size() - 1), ",")));

		// increase indices into the parameter range:
		++parameterRangeIdx[0];
		int curIdx = 0;
		while (curIdx < parameter->size() && parameterRangeIdx[curIdx] >= samplesPerParameter)
		{
			parameterRangeIdx[curIdx] = 0;
			++curIdx;
			if (curIdx < parameter->size())
			{
				parameterRangeIdx[curIdx]++;
			}
		}
	}
	return result;
}

QString iASensitivityParameterGenerator::name() const
{
	return QString("Sensitivity");
}

namespace
{
	const double Base = 10.0;

	//! return series 1, 5, 10, 50, 100, ... (for input 0, 1, 2, 3, 4, ...)
	double getSensitivityValue(int i)
	{
		double value = std::pow(Base, (i+1) / 2);
		if ((i+1) % 2 == 0)
		{
			value /= 2;
		}
		return value;
	}
}

ParameterSetsPointer iASensitivityParameterGenerator::GetParameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount)
{
	int samplesPerParameter = static_cast<int>(std::pow(10, std::log10(sampleCount) / parameter->size()));
	samplesPerParameter = std::max(1, samplesPerParameter); // at least 2 sample values per parameter
	if (samplesPerParameter % 2 == 0)
	{
		samplesPerParameter += 1; // must be odd - centered at the middle!
	}
	// calculate actual sample count (have to adhere to grid structure / powers):
	// maybe get sample count per parameter?
	int actualSampleCount = std::pow(samplesPerParameter, parameter->size());
	ParameterSetsPointer result(new ParameterSets);

	QVector<double> offsetFactors;
	int samplesPerSide = samplesPerParameter / 2;
	double maxSensitivityValue = getSensitivityValue(samplesPerSide - 1);
	for (int i = 0; i < samplesPerSide; ++i)
	{
		offsetFactors.push_back(getSensitivityValue(i) / maxSensitivityValue);
	}
	QVector<QVector<double>> allValues(parameter->size());
	for (int p = 0; p < parameter->size(); ++p)
	{
		allValues[p].resize(samplesPerParameter);
		iAValueType valueType = parameter->at(p)->valueType();
		if (valueType != Continuous)
		{
			DEBUG_LOG("Sensitivity Parameter Generator only works for Continuous parameters!");
			return result;
		}

		double halfRange = (parameter->at(p)->max() - parameter->at(p)->min()) / 2;
		double middle = parameter->at(p)->min() + halfRange;
		for (int s =  0; s < samplesPerSide; ++s)
		{
			double curOfs = offsetFactors[s] * halfRange;
			allValues[p][samplesPerSide-1-s] = middle - curOfs;
			allValues[p][samplesPerSide+1+s] = middle + curOfs;
		}
		allValues[p][samplesPerSide] = middle;
	}

	// build power set of above values
	QVector<int> parameterRangeIdx(parameter->size(), 0);
	for (int sampleIdx = 0; sampleIdx < actualSampleCount; ++sampleIdx)
	{
		ParameterSet set;
		for (int p = 0; p < parameter->size(); ++p)
		{
			set.push_back(allValues[p][parameterRangeIdx[p]]);
		}
		result->append(set);
		//DEBUG_LOG(QString("%1: %2").arg(joinAsString(parameterRangeIdx, ",")).arg(joinAsString(result->at(result->size() - 1), ",")));

		// increase indices into the parameter range:
		++parameterRangeIdx[0];
		int curIdx = 0;
		while (curIdx < parameter->size() && parameterRangeIdx[curIdx] >= samplesPerParameter)
		{
			parameterRangeIdx[curIdx] = 0;
			++curIdx;
			if (curIdx < parameter->size())
			{
				parameterRangeIdx[curIdx]++;
			}
		}
	}

	return result;
}

QVector<QSharedPointer<iAParameterGenerator> > & GetParameterGenerators()
{
	static QVector<QSharedPointer<iAParameterGenerator> > parameterGenerators;
	if (parameterGenerators.empty())
	{
		parameterGenerators.push_back(QSharedPointer<iAParameterGenerator>(new iARandomParameterGenerator()));
		parameterGenerators.push_back(QSharedPointer<iAParameterGenerator>(new iALatinHypercubeParameterGenerator()));
		parameterGenerators.push_back(QSharedPointer<iAParameterGenerator>(new iACartesianGridParameterGenerator()));
		parameterGenerators.push_back(QSharedPointer<iAParameterGenerator>(new iASensitivityParameterGenerator));
	}
	return parameterGenerators;
}


iASelectionParameterGenerator::iASelectionParameterGenerator(QString const & name, ParameterSetsPointer parameterSets):
	m_name(name),
	m_parameterSets(parameterSets)
{

}

ParameterSetsPointer iASelectionParameterGenerator::GetParameterSets(QSharedPointer<iAAttributes> /*parameter*/, int /*sampleCount*/)
{
	return m_parameterSets;
}

QString iASelectionParameterGenerator::name() const
{
	return m_name;
}
