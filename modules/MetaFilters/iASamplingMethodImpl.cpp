// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASamplingMethodImpl.h"

#include "iAParameterNames.h"
#include "iASingleResult.h"    // for iASingleResult::ValueSplitString

#include <iAAttributes.h>
#include <iAAttributeDescriptor.h>
#include <iALog.h>
#include <iAMathUtility.h>
#include <iAStringHelper.h>

#include <QFile>
#include <QTextStream>

#include <cmath>
#include <random>

int iASamplingMethod::sampleCount() const
{
	return m_sampleCount;
}

void iASamplingMethod::setSampleCount(int sampleCount, std::shared_ptr<iAAttributes> parameters)
{
	Q_UNUSED(parameters);
	m_sampleCount = sampleCount;
}

bool iASamplingMethod::supportsSamplesPerParameter() const
{
	return false;
}

void iASamplingMethod::setSamplesPerParameter(std::vector<int> samplesPerParameter)
{
	Q_UNUSED(samplesPerParameter);
}

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
		return m_isLog ? std::exp(rndVal) : rndVal;
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
		int randInt = static_cast<int>(m_isLog ? std::exp(randDbl) : randDbl);
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
		m_intRandom(0, static_cast<int>(options.size()), false)
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

std::shared_ptr<iARandomGenerator> createRandomGenerator(std::shared_ptr<iAAttributeDescriptor> a)
{
	switch (a->valueType())
	{
	case iAValueType::Boolean: [[fallthrough]];
	case iAValueType::Categorical:
		return std::make_shared<iACategoryRandom>(a->defaultValue().toStringList());
	case iAValueType::Discrete:
		return std::make_shared<iAIntRandom>(a->min(), a->max(), a->isLogScale());
	case iAValueType::Continuous:
		return std::make_shared<iADblRandom>(a->min(), a->max(), a->isLogScale());
	default:
		return std::make_shared<iAFixedDummyRandom>(a->defaultValue());
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

std::shared_ptr<iARange> createRange(bool log, double min, double max, int count, iAValueType valueType)
{
	if (log)
	{
		assert(valueType != iAValueType::Categorical);
		return std::make_shared<iALogRange>(min, max + ((valueType == iAValueType::Discrete) ? 0.999999 : 0), count);
	}
	else
	{
		return std::make_shared<iALinRange>(min, max + ((valueType == iAValueType::Categorical || valueType == iAValueType::Discrete) ? 0.999999 : 0), count);
	}
}

// The actual sampling strategies:

iASamplingMethod::~iASamplingMethod()
{}


QString iARandomSamplingMethod::name() const
{
	return iASamplingMethodName::Random;
}

iAParameterSetsPointer iARandomSamplingMethod::parameterSets(std::shared_ptr<iAAttributes> parameter)
{
	iAParameterSetsPointer result(new iAParameterSets);

	QVector<std::shared_ptr<iARandomGenerator> > random;
	for (int p = 0; p < parameter->size(); ++p)
	{
		random.push_back(createRandomGenerator(parameter->at(p)));
	}

	for (int s = 0; s < sampleCount(); ++s)
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

iAParameterSetsPointer iALatinHypercubeSamplingMethod::parameterSets(std::shared_ptr<iAAttributes> parameter)
{
	iAParameterSetsPointer result(new iAParameterSets);

	// for each parameter, create a "range", dividing its interval into sampleCount pieces
	QVector<std::shared_ptr<iARandomGenerator> > random;
	iAParameterSets sampleValues;

	iAExtDblRandom dblRand;
	for (int p = 0; p < parameter->size(); ++p)
	{
		auto param = parameter->at(p);
		iAValueType valueType = param->valueType();
		sampleValues.push_back(iAParameterSet());
		if (valueType == iAValueType::Continuous || valueType == iAValueType::Discrete)
		{
			std::shared_ptr<iARange> range = createRange(param->isLogScale(),
				param->min(), param->max(), sampleCount(), valueType);
			// iterate over sampleCount, and for each parameter, create one value per piece
			for (int s = 0; s < sampleCount(); ++s)
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
			if (options.size() == 0)
			{
				LOG(lvlWarn,
					QString("You have to choose at least one option for Boolean values (parameter %1), using 'true' for all parameter sets!")
						.arg(param->name()));
				for (int s = 0; s < sampleCount(); ++s)
				{
					sampleValues[p].push_back(true);
				}
			}
			else
			{
				auto maxOptIdx = options.size();
				for (int s = 0; s < sampleCount(); ++s)
				{
					auto optIdx = s % maxOptIdx;
					sampleValues[p].push_back(options[optIdx]);
				}
			}
		}
		else
		{
			if (valueType != iAValueType::FileNameSave)
			{
				LOG(lvlWarn, QString("Sampling not supported for value type %1, using default value %2")
						.arg(ValueType2Str(valueType))
						.arg(param->defaultValue().toString()));
			}
			for (int s = 0; s < sampleCount(); ++s)
			{
				sampleValues[p].push_back(param->defaultValue());
			}
		}
	}

	iARangeRandom<int> intRand;
	// iterate over sampleCount, and for each parameter, randomly select one of the pieces
	for (int s = sampleCount(); s > 0; --s)
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

void iACartesianGridSamplingMethod::setSampleCount(int targetedSampleCount, std::shared_ptr<iAAttributes> parameters)
{
	int normalVariedParamCount = 0;
	int actualSampleCount = 1;
	m_samplesPerParameter.resize(parameters->size());
	for (int pIdx = 0; pIdx<parameters->size(); ++pIdx)
	{
		auto p = parameters->at(pIdx);
		auto t = p->valueType();
		if ((t == iAValueType::Continuous || t == iAValueType::Discrete) && p->min() != p->max())
		{
			++normalVariedParamCount;
		}
		else if ((t == iAValueType::Categorical || t == iAValueType::Boolean) && p->defaultValue().toStringList().size() > 1)
		{
			m_samplesPerParameter[pIdx] = static_cast<int>(p->defaultValue().toStringList().size());
			actualSampleCount *= m_samplesPerParameter[pIdx];
		}
		else
		{
			m_samplesPerParameter[pIdx] = 1;
		}
	}
	int sampleCountRemaining = targetedSampleCount / actualSampleCount;
	int samplesPerParameter = static_cast<int>(std::pow(10, std::log10(sampleCountRemaining) / normalVariedParamCount));
	//int samplesPerParameter2 = static_cast<int>(std::pow(sampleCountRemaining, 1.0 / normalVariedParamCount));
	//assert(samplesPerParameter == samplesPerParameter2);
	samplesPerParameter = std::max(1, samplesPerParameter);
	for (int pIdx = 0; pIdx < parameters->size(); ++pIdx)
	{
		auto p = parameters->at(pIdx);
		auto t = p->valueType();
		if ((t == iAValueType::Continuous || t == iAValueType::Discrete) && p->min() != p->max())
		{
			m_samplesPerParameter[pIdx] = samplesPerParameter;
			actualSampleCount *= m_samplesPerParameter[pIdx];
		}
	}
	iASamplingMethod::setSampleCount(actualSampleCount, parameters);
}

bool iACartesianGridSamplingMethod::supportsSamplesPerParameter() const
{
	return true;
}

void iACartesianGridSamplingMethod::setSamplesPerParameter(std::vector<int> samplesPerParameter)
{
	m_samplesPerParameter = samplesPerParameter;
}

iAParameterSetsPointer iACartesianGridSamplingMethod::parameterSets(std::shared_ptr<iAAttributes> parameters)
{
	iAParameterSetsPointer result(new iAParameterSets);

/*
	LOG(lvlInfo, QString("param. count: %1, samples/param.: %2, targeted samples: %3, actual samples: %4")
		.arg(parameter->size())
		.arg(samplesPerParameter)
		.arg(sampleCount)
		.arg(actualSampleCount)
	);
*/

	QVector<std::shared_ptr<iARange>> ranges;
	for (int p = 0; p < parameters->size(); ++p)
	{
		iAValueType valueType = parameters->at(p)->valueType();
		ranges.push_back(
			createRange(
				parameters->at(p)->isLogScale(),
				parameters->at(p)->min(),
				parameters->at(p)->max(),
				m_samplesPerParameter[p] - 1, // -1 because we choose from the edges of the range
				valueType)
			);
	}
	// to keep track of which grid index for which parameter we are currently using
	QVector<int> parameterRangeIdx(parameters->size(), 0);

	for (int sampleIdx = 0; sampleIdx < sampleCount(); ++sampleIdx)
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
		//LOG(lvlInfo, QString("%1: %2").arg(joinNumbersAsString(parameterRangeIdx, ",")).arg(joinNumbersAsString(result->at(result->size() - 1), ",")));

		// increase indices into the parameter range:
		++parameterRangeIdx[0];
		int curIdx = 0;
		while (curIdx < parameters->size() && parameterRangeIdx[curIdx] >= m_samplesPerParameter[curIdx])
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

iAParameterSetsPointer iALocalSensitivitySamplingMethod::parameterSets(std::shared_ptr<iAAttributes> parameters)
{
	int samplesPerParameter = static_cast<int>(std::pow(10, std::log10(sampleCount()) / parameters->size()));
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
		//LOG(lvlInfo, QString("%1: %2").arg(joinNumbersAsString(parameterRangeIdx, ",")).arg(joinNumbersAsString(result->at(result->size() - 1), ",")));

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
	std::shared_ptr<iASamplingMethod> otherGenerator, double delta):
	m_baseGenerator(otherGenerator),
	m_delta(delta)
{}

QString iAGlobalSensitivitySamplingMethod::name() const
{
	return iASamplingMethodName::GlobalSensitivity;
}

iAParameterSetsPointer iAGlobalSensitivitySamplingMethod::parameterSets(std::shared_ptr<iAAttributes> parameters)
{
	//if (m_baseGenerator->supportsSamplesPerParameter())
	m_baseGenerator->setSampleCount(sampleCount(), parameters);
	iAParameterSetsPointer baseParameterSets = m_baseGenerator->parameterSets(parameters);
	iAParameterSetsPointer result(new iAParameterSets);

	//int maxPerParameterValues = static_cast<int>(1.0 / m_delta);
	//int perParameterValues = clamp(1, maxPerParameterValues, m_samplesPerPoint);

	for (auto parameterSet : *baseParameterSets)
	{
		result->push_back(parameterSet);
		for (int p = 0; p < parameters->size(); ++p)
		{
			auto param = parameters->at(p);
			if (param->valueType() != iAValueType::Continuous && param->valueType() != iAValueType::Discrete)
			{	// we only support sensitivity of numerical parameters currently
				continue;	// TODO: supporting categorical would probably work too - but just if all values are tried
			}
			// re-use iARange here? slightly different use case:
			//     - not just full range split by sample count, but fixed delta
			//     - "centered" on given parameter set
			auto range = param->max() - param->min();
			if (range < std::numeric_limits<double>::epsilon())
			{   // skip parameters which don't vary in our sampling
				continue;
			}
			auto step = range * m_delta;
			double paramValue = parameterSet[p].toDouble();
			int numStepsToMin = static_cast<int>(std::floor((paramValue - param->min()) / step));
			for (int count = 1; count <= numStepsToMin; ++count)
			{
				iAParameterSet shiftedSet(parameterSet);
				shiftedSet[p].setValue(paramValue - (count * step) );
				result->push_back(shiftedSet);
			}
			int numStepsToMax = static_cast<int>(std::floor((param->max() - paramValue) / step));
			for (int count = 1; count <= numStepsToMax; ++count)
			{
				iAParameterSet shiftedSet(parameterSet);
				shiftedSet[p].setValue(paramValue + (count * step));
				result->push_back(shiftedSet);
			}
		}
	}
	return result;
}


iAGlobalSensitivitySmallStarSamplingMethod::iAGlobalSensitivitySmallStarSamplingMethod(
	std::shared_ptr<iASamplingMethod> otherGenerator, double delta, int numSteps) :
	m_baseGenerator(otherGenerator), m_delta(delta), m_numSteps(numSteps)
{
}

QString iAGlobalSensitivitySmallStarSamplingMethod::name() const
{
	return iASamplingMethodName::GlobalSensitivitySmall;
}

iAParameterSetsPointer iAGlobalSensitivitySmallStarSamplingMethod::parameterSets(
	std::shared_ptr<iAAttributes> parameters)
{
	// MAYBE: create "margin" around ranges to keep all samples from base generator in a sub-region
	// such that one can go numSteps steps of width delta*(param range) from them and still
	// stay in user-specified min/max range?
	m_baseGenerator->setSampleCount(sampleCount(), parameters);
	iAParameterSetsPointer baseParameterSets = m_baseGenerator->parameterSets(parameters);
	iAParameterSetsPointer result(new iAParameterSets);

	//int maxPerParameterValues = static_cast<int>(1.0 / m_delta);
	//int perParameterValues = clamp(1, maxPerParameterValues, m_samplesPerPoint);

	for (auto parameterSet : *baseParameterSets)
	{
		result->push_back(parameterSet);
		for (int p = 0; p < parameters->size(); ++p)
		{
			auto param = parameters->at(p);
			if (param->valueType() != iAValueType::Continuous && param->valueType() != iAValueType::Discrete)
			{              // we only support sensitivity of numerical parameters currently
				continue;  // TODO: supporting categorical would probably work too - but just if all values are tried
			}
			// re-use iARange here? slightly different use case:
			//     - not just full range split by sample count, but fixed delta
			//     - "centered" on given parameter set
			auto range = param->max() - param->min();
			if (range < std::numeric_limits<double>::epsilon())
			{  // skip parameters which don't vary in our sampling
				continue;
			}
			auto step = range * m_delta;
			double paramValue = parameterSet[p].toDouble();
			for (int count = 1; count <= m_numSteps; ++count)
			{
				iAParameterSet shiftedSet(parameterSet);
				shiftedSet[p].setValue(paramValue - (count * step));
				result->push_back(shiftedSet);
			}
			for (int count = 1; count <= m_numSteps; ++count)
			{
				iAParameterSet shiftedSet(parameterSet);
				shiftedSet[p].setValue(paramValue + (count * step));
				result->push_back(shiftedSet);
			}
		}
	}
	return result;
}



iARerunSamplingMethod::iARerunSamplingMethod(iAParameterSetsPointer parameterSets, QString const& name) :
	m_name(name),
	m_parameterSets(parameterSets)
{}

iARerunSamplingMethod::iARerunSamplingMethod(QString const& fileName):
	m_parameterSets(std::make_shared<iAParameterSets>())
{
	QFile paramFile(fileName);
	if (!paramFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Could not open sample parameter set file '%1' for reading!").arg(fileName));
	}
	QTextStream paramIn(&paramFile);
	QStringList header = paramIn.readLine().split(iASingleResult::ValueSplitString);
	int lineNr = 2;
	while (!paramIn.atEnd())
	{
		QString paramLine = paramIn.readLine();
		auto values = paramLine.split(iASingleResult::ValueSplitString);
		if (values.size() == 0)
		{
			continue;
		}
		iAParameterSet paramSet;
		if (header.size() != values.size())
		{
			LOG(lvlWarn,
				QString("Invalid content '%1' on line '%2' in parameter set file %3!")
					.arg(paramLine)
					.arg(lineNr)
					.arg(fileName));
		}
		for (int p=1; p<header.size()-1; ++p)    // ignore ID and filename -> TODO: adaptive?
		{
			paramSet.push_back(values[p]);
		}
		m_parameterSets->push_back(paramSet);
		++lineNr;
	}
	paramFile.close();
}

iAParameterSetsPointer iARerunSamplingMethod::parameterSets(std::shared_ptr<iAAttributes> /*parameters*/)
{
	return m_parameterSets;
}

QString iARerunSamplingMethod::name() const
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
		result.push_back(iASamplingMethodName::GlobalSensitivitySmall);
		result.push_back(iASamplingMethodName::RerunSampling);
	}
	return result;
}

std::shared_ptr<iASamplingMethod> createSamplingMethod(QVariantMap const& parameters)
{
	QString methodName = parameters[spnSamplingMethod].toString();
	if (methodName == iASamplingMethodName::Random)
	{
		return std::make_shared<iARandomSamplingMethod>();
	}
	else if (methodName == iASamplingMethodName::LatinHypercube)
	{
		return std::make_shared<iALatinHypercubeSamplingMethod>();
	}
	else if (methodName == iASamplingMethodName::CartesianGrid)
	{
		return std::make_shared<iACartesianGridSamplingMethod>();
	}
	else if (methodName == iASamplingMethodName::LocalSensitivity)
	{
		return std::make_shared<iALocalSensitivitySamplingMethod>();
	}
	else if (methodName == iASamplingMethodName::GlobalSensitivity ||
		methodName == iASamplingMethodName::GlobalSensitivitySmall)
	{
		QVariantMap newParams(parameters);
		newParams[spnSamplingMethod] = parameters[spnBaseSamplingMethod];
		if (newParams[spnSamplingMethod] == iASamplingMethodName::GlobalSensitivity)
		{
			LOG(lvlError, QString("Cannot generate global sensitivity sampling: Base sampling method must not also be '%1'").arg(iASamplingMethodName::GlobalSensitivity));
			return std::shared_ptr<iASamplingMethod>();
		}
		double delta = parameters[spnStarDelta].toDouble();
		auto otherSamplingMethod = createSamplingMethod(newParams);
		if (methodName == iASamplingMethodName::GlobalSensitivity)
		{
			return std::make_shared<iAGlobalSensitivitySamplingMethod>(
				otherSamplingMethod, delta);
		}
		else
		{
			int stepNumber = parameters[spnStarStepNumber].toInt();
			return std::make_shared<iAGlobalSensitivitySmallStarSamplingMethod>(
				otherSamplingMethod, delta, stepNumber);
		}
	}
	else if (methodName == iASamplingMethodName::RerunSampling)
	{
		return std::make_shared<iARerunSamplingMethod>(parameters[spnParameterSetFile].toString());
	}
	LOG(lvlError, QString("Could not find sampling method '%1'").arg(methodName));
	return std::shared_ptr<iASamplingMethod>();
}
