/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iASamplingMethodImpl.h"

#include "iAParameterNames.h"

#include <iAAttributes.h>
#include <iAAttributeDescriptor.h>
#include <iALog.h>
#include <iAMathUtility.h>
#include <iAStringHelper.h>

#include <cmath>
#include <random>

// Various helper classes for random generation:

class iARandomGenerator
{
protected:
	std::mt19937 rng;
public:
	iARandomGenerator()
	{
		rng.seed(std::random_device{}());
	}
	virtual ~iARandomGenerator() {}
	virtual QVariant next() =0;
};

class iADblRandom: public iARandomGenerator
{
private:
	bool m_isLog;
	std::uniform_real_distribution<double> dist;
public:
	iADblRandom(double min, double max, bool isLog):
		m_isLog(isLog),
		dist(isLog ? std::log(min) : min, isLog ? std::log(max) : max)
	{}
	QVariant next() override
	{
		double rndVal = dist(rng);
		return m_isLog ? exp(rndVal) : rndVal;
	}
};

class iAIntRandom: public iARandomGenerator
{
private:
	std::uniform_real_distribution<double> dist;
	int m_min, m_max;
	bool m_isLog;
public:
	iAIntRandom(int min, int max, bool isLog) :
		dist(0, 1),
		m_min(min),
		m_max(max),
		m_isLog(isLog)
	{}
	//! return a random number between 0 and max-1, uniformly distributed
	QVariant next() override
	{
		double randMin = m_isLog ? std::log(m_min) : m_min;
		double randRng = m_isLog ? std::log(m_max + 1) - randMin : (m_max - m_min + 1);
		double randDbl = randMin + dist(rng) * randRng;
		int randInt = static_cast<int>(m_isLog ? exp(randDbl) : randDbl);
		return clamp(m_min, m_max, randInt);
	}
};

class iACategoryRandom : public iARandomGenerator
{
private:
	QStringList m_options;
	iAIntRandom m_intRandom;
public:
	iACategoryRandom(QStringList const & options):
		m_options(options),
		m_intRandom(0, options.size(), false)
	{}
	QVariant next() override
	{
		return m_options[m_intRandom.next().toInt()];
	}
};

class iAFixedDummyRandom : public iARandomGenerator
{
private:
	QVariant m_value;
public:
	iAFixedDummyRandom(QVariant v) : m_value(v)
	{}
	QVariant next() override
	{
		return m_value;
	}
};

class iAExtDblRandom
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng;
public:
	iAExtDblRandom():
		dist(0, 1)
	{
		rng.seed(std::random_device{}());
	}
	double next(double min, double max)
	{
		return mapNormTo(min, max, dist(rng));
	}
};

template <typename T>
class iARangeRandom
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng;
public:
	iARangeRandom():
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

QSharedPointer<iARandomGenerator> createRandomGenerator(QSharedPointer<iAAttributeDescriptor> a)
{
	switch (a->valueType())
	{
	case iAValueType::Boolean:
#if __cplusplus >= 201703L
		[[fallthrough]];
#endif
	case iAValueType::Categorical:
		return QSharedPointer<iARandomGenerator>(new iACategoryRandom(a->defaultValue().toStringList()));
	case iAValueType::Discrete:
		return QSharedPointer<iARandomGenerator>(new iAIntRandom(a->min(), a->max(), a->isLogScale()));
	case iAValueType::Continuous:
		return QSharedPointer<iARandomGenerator>(new iADblRandom(a->min(), a->max(), a->isLogScale()));
	default:
		return QSharedPointer<iARandomGenerator>(new iAFixedDummyRandom(a->defaultValue()));
	}
}

class iARange
{
public:
	virtual ~iARange() {}
	virtual double min(int i) = 0;
	virtual double max(int i) = 0;
};


class iALinRange : public iARange
{
public:
	iALinRange(double min, double max, int count) :
		m_min(min),
		m_step((max - min) / count)
	{}
	double min(int i) override
	{
		return m_min + i * m_step;
	}
	double max(int i) override
	{
		return m_min + (i + 1) * m_step;
	}
private:
	double m_min;
	double m_step;
};

class iALogRange : public iARange
{
public:
	iALogRange(double min, double max, int count) :
		m_min(min),
		m_factor(std::pow(max / min, 1.0 / count))
	{
		assert(min > 0);
	}
	double min(int i) override
	{
		return m_min * std::pow(m_factor, i + 1);
	}
	double max(int i) override
	{
		return m_min * std::pow(m_factor, i + 1);
	}
private:
	double m_min;
	double m_factor;
};

QSharedPointer<iARange> createRange(bool log, double min, double max, int count, iAValueType valueType)
{
	if (log)
	{
		assert(valueType != iAValueType::Categorical);
		return QSharedPointer<iARange>(new iALogRange(min, max + ((valueType == iAValueType::Discrete) ? 0.999999 : 0), count));
	}
	else
	{
		return QSharedPointer<iARange>(new iALinRange(min, max + ((valueType == iAValueType::Categorical || valueType == iAValueType::Discrete) ? 0.999999 : 0), count));
	}
}

// The actual sampling strategies:

iASamplingMethod::~iASamplingMethod()
{}


QString iARandomSamplingMethod::name() const
{
	return iASamplingMethodName::Random;
}

iAParameterSetsPointer iARandomSamplingMethod::parameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount)
{
	iAParameterSetsPointer result(new iAParameterSets);

	QVector<QSharedPointer<iARandomGenerator> > random;
	for (int p = 0; p < parameter->size(); ++p)
	{
		random.push_back(createRandomGenerator(parameter->at(p)));
	}

	for (int s = 0; s < sampleCount; ++s)
	{
		iAParameterSet set;
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


QString iALatinHypercubeSamplingMethod::name() const
{
	return iASamplingMethodName::LatinHypercube;
}

iAParameterSetsPointer iALatinHypercubeSamplingMethod::parameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount)
{
	iAParameterSetsPointer result(new iAParameterSets);

	// for each parameter, create a "range", dividing its interval into sampleCount pieces
	QVector<QSharedPointer<iARandomGenerator> > random;
	iAParameterSets sampleValues;

	iAExtDblRandom dblRand;
	for (int p = 0; p < parameter->size(); ++p)
	{
		auto param = parameter->at(p);
		iAValueType valueType = param->valueType();
		sampleValues.push_back(iAParameterSet());
		if (valueType == iAValueType::Continuous || valueType == iAValueType::Discrete)
		{
			QSharedPointer<iARange> range = createRange(param->isLogScale(),
				param->min(), param->max(), sampleCount, valueType);
			// iterate over sampleCount, and for each parameter, create one value per piece
			for (int s = 0; s < sampleCount; ++s)
			{
				// TODO: special handling for log? otherwise within the piece, we have linear distribution
				double value = dblRand.next(range->min(s), range->max(s));
				if (valueType == iAValueType::Discrete)
				{
					value = static_cast<int>(value);
				}
				sampleValues[p].push_back(value);
			}
		}
		else if(valueType == iAValueType::Boolean || valueType == iAValueType::Categorical)
		{
			auto options = param->defaultValue().toStringList();
			int maxOptIdx = options.size();
			for (int s = 0; s < sampleCount; ++s)
			{
				int optIdx = s % maxOptIdx;
				sampleValues[p].push_back(options[optIdx]);
			}
		}
		else
		{
			LOG(lvlWarn, QString("Sampling not supported for value type %1, using default value %2")
				.arg(ValueType2Str(valueType)).arg(param->defaultValue().toString()));
			for (int s = 0; s < sampleCount; ++s)
			{
				sampleValues[p].push_back(param->defaultValue());
			}
		}
	}

	iARangeRandom<int> intRand;
	// iterate over sampleCount, and for each parameter, randomly select one of the pieces
	for (int s = sampleCount; s > 0 ; --s)
	{
		iAParameterSet set;
		for (int p = 0; p < parameter->size(); ++p)
		{
			// randomly select one of the previously chosen values, and put it into the parameter set
			set.push_back(pop_at(sampleValues[p], intRand.next(s)));
		}
		result->push_back(set);
	}
	return result;
}


QString iACartesianGridSamplingMethod::name() const
{
	return iASamplingMethodName::CartesianGrid;
}

iAParameterSetsPointer iACartesianGridSamplingMethod::parameterSets(QSharedPointer<iAAttributes> parameters, int sampleCount)
{
	iAParameterSetsPointer result(new iAParameterSets);
	int samplesPerParameter = static_cast<int>(std::pow(10, std::log10(sampleCount) / parameters->size()));
	samplesPerParameter = std::max(2, samplesPerParameter); // at least 2 sample values per parameter

	// calculate actual sample count (have to adhere to grid structure / powers):
	// maybe get sample count per parameter?
	int actualSampleCount = std::pow(samplesPerParameter, parameters->size());
/*
	LOG(lvlInfo, QString("param. count: %1, samples/param.: %2, targeted samples: %3, actual samples: %4")
		.arg(parameter->size())
		.arg(samplesPerParameter)
		.arg(sampleCount)
		.arg(actualSampleCount)
	);
*/

	QVector<QSharedPointer<iARange>> ranges;
	for (int p = 0; p < parameters->size(); ++p)
	{
		iAValueType valueType = parameters->at(p)->valueType();
		ranges.push_back(
			createRange(
				parameters->at(p)->isLogScale(),
				parameters->at(p)->min(),
				parameters->at(p)->max(),
				samplesPerParameter-1, // -1 because we choose from the edges of the range
				valueType)
			);
	}
	// to keep track of which grid index for which parameter we are currently using
	QVector<int> parameterRangeIdx(parameters->size(), 0);

	for (int sampleIdx = 0; sampleIdx < actualSampleCount; ++sampleIdx)
	{
		iAParameterSet set;
		for (int p = 0; p < parameters->size(); ++p)
		{
			double value = ranges[p]->min(parameterRangeIdx[p]);
			iAValueType valueType = parameters->at(p)->valueType();
			if (valueType == iAValueType::Discrete || valueType == iAValueType::Categorical)
			{
				value = static_cast<int>(value);
			}
			set.push_back(value);
		}
		result->append(set);
		//LOG(lvlInfo, QString("%1: %2").arg(joinAsString(parameterRangeIdx, ",")).arg(joinAsString(result->at(result->size() - 1), ",")));

		// increase indices into the parameter range:
		++parameterRangeIdx[0];
		int curIdx = 0;
		while (curIdx < parameters->size() && parameterRangeIdx[curIdx] >= samplesPerParameter)
		{
			parameterRangeIdx[curIdx] = 0;
			++curIdx;
			if (curIdx < parameters->size())
			{
				parameterRangeIdx[curIdx]++;
			}
		}
	}
	return result;
}


QString iALocalSensitivitySamplingMethod::name() const
{
	return iASamplingMethodName::LocalSensitivity;
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

iAParameterSetsPointer iALocalSensitivitySamplingMethod::parameterSets(QSharedPointer<iAAttributes> parameters, int sampleCount)
{
	int samplesPerParameter = static_cast<int>(std::pow(10, std::log10(sampleCount) / parameters->size()));
	samplesPerParameter = std::max(1, samplesPerParameter); // at least 2 sample values per parameter
	if (samplesPerParameter % 2 == 0)
	{
		samplesPerParameter += 1; // must be odd - centered at the middle!
	}
	// calculate actual sample count (have to adhere to grid structure / powers):
	// maybe get sample count per parameter?
	int actualSampleCount = std::pow(samplesPerParameter, parameters->size());
	iAParameterSetsPointer result(new iAParameterSets);

	QVector<double> offsetFactors;
	int samplesPerSide = samplesPerParameter / 2;
	double maxSensitivityValue = getSensitivityValue(samplesPerSide - 1);
	for (int i = 0; i < samplesPerSide; ++i)
	{
		offsetFactors.push_back(getSensitivityValue(i) / maxSensitivityValue);
	}
	iAParameterSets allValues(parameters->size());
	for (int p = 0; p < parameters->size(); ++p)
	{
		allValues[p].resize(samplesPerParameter);
		iAValueType valueType = parameters->at(p)->valueType();
		if (valueType != iAValueType::Continuous)
		{
			LOG(lvlError, "Sensitivity Parameter Generator only works for Continuous parameters!");
			return result;
		}

		double halfRange = (parameters->at(p)->max() - parameters->at(p)->min()) / 2;
		double middle = parameters->at(p)->min() + halfRange;
		for (int s =  0; s < samplesPerSide; ++s)
		{
			double curOfs = offsetFactors[s] * halfRange;
			allValues[p][samplesPerSide-1-s] = middle - curOfs;
			allValues[p][samplesPerSide+1+s] = middle + curOfs;
		}
		allValues[p][samplesPerSide] = middle;
	}

	// build power set of above values
	QVector<int> parameterRangeIdx(parameters->size(), 0);
	for (int sampleIdx = 0; sampleIdx < actualSampleCount; ++sampleIdx)
	{
		iAParameterSet set;
		for (int p = 0; p < parameters->size(); ++p)
		{
			set.push_back(allValues[p][parameterRangeIdx[p]]);
		}
		result->append(set);
		//LOG(lvlInfo, QString("%1: %2").arg(joinAsString(parameterRangeIdx, ",")).arg(joinAsString(result->at(result->size() - 1), ",")));

		// increase indices into the parameter range:
		++parameterRangeIdx[0];
		int curIdx = 0;
		while (curIdx < parameters->size() && parameterRangeIdx[curIdx] >= samplesPerParameter)
		{
			parameterRangeIdx[curIdx] = 0;
			++curIdx;
			if (curIdx < parameters->size())
			{
				parameterRangeIdx[curIdx]++;
			}
		}
	}

	return result;
}


iAGlobalSensitivitySamplingMethod::iAGlobalSensitivitySamplingMethod(
	QSharedPointer<iASamplingMethod> otherGenerator, double delta):
	m_baseGenerator(otherGenerator),
	m_delta(delta)
{}

QString iAGlobalSensitivitySamplingMethod::name() const
{
	return iASamplingMethodName::GlobalSensitivity;
}

iAParameterSetsPointer iAGlobalSensitivitySamplingMethod::parameterSets(QSharedPointer<iAAttributes> parameters, int sampleCount)
{
	iAParameterSetsPointer baseParameterSets = m_baseGenerator->parameterSets(parameters, sampleCount);
	iAParameterSetsPointer result(new iAParameterSets);
	for (auto parameterSet: *baseParameterSets)
	{
		for (int p = 0; p < parameters->size(); ++p)
		{
		}
		result->push_back(parameterSet);
	}
	return result;
}


iASelectionSamplingMethod::iASelectionSamplingMethod(QString const & name, iAParameterSetsPointer parameterSets):
	m_name(name),
	m_parameterSets(parameterSets)
{}

iAParameterSetsPointer iASelectionSamplingMethod::parameterSets(QSharedPointer<iAAttributes> /*parameters*/, int /*sampleCount*/)
{
	return m_parameterSets;
}

QString iASelectionSamplingMethod::name() const
{
	return m_name;
}


QStringList const& samplingMethodNames()
{
	static QStringList result;
	if (result.empty())
	{
		result.push_back(iASamplingMethodName::Random);
		result.push_back(iASamplingMethodName::LatinHypercube);
		result.push_back(iASamplingMethodName::CartesianGrid);
		result.push_back(iASamplingMethodName::LocalSensitivity);
		result.push_back(iASamplingMethodName::GlobalSensitivity);
	}
	return result;
}

QSharedPointer<iASamplingMethod> createSamplingMethod(iASettings const& parameters)
{
	QString methodName = parameters[spnSamplingMethod].toString();
	if (methodName == iASamplingMethodName::Random)
	{
		return QSharedPointer<iASamplingMethod>(new iARandomSamplingMethod);
	}
	else if (methodName == iASamplingMethodName::LatinHypercube)
	{
		return QSharedPointer<iASamplingMethod>(new iALatinHypercubeSamplingMethod);
	}
	else if (methodName == iASamplingMethodName::CartesianGrid)
	{
		return QSharedPointer<iASamplingMethod>(new iACartesianGridSamplingMethod);
	}
	else if (methodName == iASamplingMethodName::LocalSensitivity)
	{
		return QSharedPointer<iASamplingMethod>(new iALocalSensitivitySamplingMethod);
	}
	else if (methodName == iASamplingMethodName::GlobalSensitivity)
	{
		iASettings newParams(parameters);
		newParams[spnSamplingMethod] = parameters[spnBaseSamplingMethod];
		if (newParams[spnSamplingMethod] == iASamplingMethodName::GlobalSensitivity)
		{
			LOG(lvlError, QString("Cannot generate global sensitivity sampling: Base sampling method must not also be '%1'").arg(iASamplingMethodName::GlobalSensitivity));
			return QSharedPointer<iASamplingMethod>();
		}
		double delta = parameters[spnSensitivityDelta].toDouble();
		auto otherSamplingMethod = createSamplingMethod(parameters);
		return QSharedPointer<iASamplingMethod>(new iAGlobalSensitivitySamplingMethod(otherSamplingMethod, delta));
	}
	LOG(lvlError, QString("Could not find sampling method '%1'").arg(methodName));
	return QSharedPointer<iASamplingMethod>();
}