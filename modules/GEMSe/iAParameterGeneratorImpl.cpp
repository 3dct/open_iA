/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <cmath>
#include <random>

class RandomGenerator
{
public:
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
		m_min(min),
		m_max(max),
		dist(0, 1)
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
		m_min(min),
		m_max(max),
		dist(0, 1)
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


class MyRange
{
public:
	virtual double min(int i) =0;
	virtual double max(int i) =0;
};


class MyLinRange: public MyRange
{
public:
	MyLinRange(double min, double max, int count):
		m_min(min),
		m_step((max-min)/count)
	{}
	virtual double min(int i)
	{
		return m_min+ i*m_step;
	}
	virtual double max(int i)
	{
		return m_min+ (i+1)*m_step;
	}
private:
	double m_min;
	double m_step;
};

class MyLogRange: public MyRange
{
public:
	MyLogRange(double min, double max, int count):
		m_min(min),
		m_factor(std::pow(max/min, 1.0/count))
	{
		assert(min > 0);
	}
	virtual double min(int i)
	{
		return m_min*std::pow(m_factor, i+1);
	}
	virtual double max(int i)
	{
		return m_min*std::pow(m_factor, i+1);
	}
private:
	double m_min;
	double m_factor;
};

QSharedPointer<MyRange> CreateRange(bool log, double min, double max, int count, iAValueType valueType)
{
	if (log)
	{
		assert(valueType != Categorical);
		return QSharedPointer<MyRange>(new MyLogRange(min, max + ((valueType == Discrete) ? 0.999999 : 0), count));
	}
	else return QSharedPointer<MyRange>(new MyLinRange(min, max + ((valueType == Categorical || valueType == Discrete) ? 0.999999 : 0), count));
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
		QSharedPointer<MyRange> range = CreateRange(parameter->at(p)->isLogScale(),
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
	DEBUG_LOG("Cartesian grid sampling broken at the moment!");
	ParameterSetsPointer result(new ParameterSets);
	int samplesPerParameter = static_cast<int>(std::pow(10, std::log10(sampleCount) / parameter->size()));
	samplesPerParameter = std::max(2, samplesPerParameter); // at least 2 sample values per parameter

	// calculate actual sample count (have to adhere to grid structure / powers):
	// maybe get sample count per parameter?
	int actualSampleCount = std::pow(samplesPerParameter, parameter->size());

	/*
	DEBUG_LOG(QString("parameter count: %1, sample count: %2 samplesPerParameter: %3")
		.arg(paramCount)
		.arg(parameterRange->sampleCount)
		.arg(samplesPerParameter)
	);
	*/
	
	QVector<QSharedPointer<MyRange>> ranges;
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

	for (int sampleIdx = 0; sampleIdx< sampleCount; ++sampleIdx)
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

		// increase indices into the parameter range:
		++parameterRangeIdx[0];
		int curIdx = 0;
		while (parameterRangeIdx[curIdx] >= samplesPerParameter)
		{
			parameterRangeIdx[curIdx] = 0;
			++curIdx;
			parameterRangeIdx[curIdx]++;
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
		//parameterGenerators.push_back(QSharedPointer<iAParameterGenerator>(new iACartesianGridParameterGenerator()));
	}
	return parameterGenerators;
}


iASelectionParameterGenerator::iASelectionParameterGenerator(QString const & name, ParameterSetsPointer parameterSets):
	m_name(name),
	m_parameterSets(parameterSets)
{

}

ParameterSetsPointer iASelectionParameterGenerator::GetParameterSets(QSharedPointer<iAAttributes> parameter, int /*sampleCount*/)
{
	return m_parameterSets;
}

QString iASelectionParameterGenerator::name() const
{
	return m_name;
}
