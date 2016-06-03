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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAMMSegParameterGeneratorImpl.h"

#include "iAConsole.h"
#include "iAMMSegParameter.h"
#include "iAMMSegParameterRange.h"
#include "iAMathUtility.h"

#include <cmath>
#include <random>

class RandomGenerator
{
public:
	virtual double next() =0;
};

class MyLinRandom: public RandomGenerator
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng;
public:
	MyLinRandom(double min, double max):
		dist(min, max)
	{
		rng.seed(std::random_device{}()); //Initialize with non-deterministic seeds
	}
	virtual double next()
	{
		return dist(rng);
	}
};

class MyLogRandom: public RandomGenerator
{
private:
	std::uniform_real_distribution<double> dist;
	std::mt19937 rng;
public:
	MyLogRandom(double min, double max):
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

QSharedPointer<RandomGenerator> CreateRand(bool log, double min, double max)
{
	if (log) return QSharedPointer<RandomGenerator>(new MyLogRandom(min, max));
		else return QSharedPointer<RandomGenerator>(new MyLinRandom(min, max));
}

QString iARandomParameterGenerator::GetName() const
{
	return QString("Random");
}

ParameterListPointer iARandomParameterGenerator::GetParameterSets(QSharedPointer<iAMMSegParameterRange> parameterRange)
{
	ParameterListPointer result(new ParameterList);
	int count = parameterRange->sampleCount;
	QSharedPointer<RandomGenerator> erw_beta_Rand(CreateRand(parameterRange->erw_beta_logScale, parameterRange->erw_beta_From, parameterRange->erw_beta_To));
	QSharedPointer<RandomGenerator> erw_gamma_Rand(CreateRand(parameterRange->erw_gamma_logScale, parameterRange->erw_gamma_From, parameterRange->erw_gamma_To));
	QSharedPointer<RandomGenerator> erw_maxIter_Rand(CreateRand(parameterRange->erw_maxIter_logScale, parameterRange->erw_maxIter_From, parameterRange->erw_maxIter_To+0.99999999));
	QSharedPointer<RandomGenerator> svm_C_Rand(CreateRand(parameterRange->svm_C_logScale, parameterRange->svm_C_From, parameterRange->svm_C_To));
	QSharedPointer<RandomGenerator> svm_Gamma_Rand(CreateRand(parameterRange->svm_gamma_logScale, parameterRange->svm_gamma_From, parameterRange->svm_gamma_To));
	QVector<QSharedPointer<RandomGenerator> > weight_Rand;
	for (int i=0; i<parameterRange->modalityParamRange.size(); ++i)
	{
		weight_Rand.push_back(CreateRand(parameterRange->weightLogScale, parameterRange->modalityParamRange[i].weightFrom, parameterRange->modalityParamRange[i].weightTo));
	}
	MyRangeRandom<int> intRand;
	for (int i=0; i<count; ++i)
	{
		// completely random:
		double erw_beta = erw_beta_Rand->next();
		double erw_gamma = erw_gamma_Rand->next();
		int erw_maxIter = static_cast<int>(erw_maxIter_Rand->next());
		double svm_C = svm_C_Rand->next();
		double svm_gamma = svm_Gamma_Rand->next();
		int svm_channels = intRand.next(parameterRange->svm_channels_To - parameterRange->svm_channels_From + 1) + parameterRange->svm_channels_From;

		double weightSum = 0;
		QVector<iAMMSegModalityParameter> modParams;
		bool valid = true;
		do
		{
			modParams.clear();
			valid = true;
			for (int i=0; i<parameterRange->modalityParamRange.size(); ++i)
			{
				iAMMSegModalityParameter modParam;

				// make sure weights sum up to 1
				if (i == parameterRange->modalityParamRange.size()-1)
				{
					modParam.weight = 1.0 - weightSum;
					if (modParam.weight < parameterRange->modalityParamRange[i].weightFrom ||
						modParam.weight > parameterRange->modalityParamRange[i].weightTo)
					{
						valid = false;
						/*
						DebugOut() << "Weight "<< modParam.weight <<" is outside of valid interval [" << 
							parameterRange->modalityParamRange[i].weightFrom << ", " << 
							parameterRange->modalityParamRange[i].weightTo << "]" << std::endl;
						*/
					}
				}
				else
				{
					modParam.weight = weight_Rand[i]->next();
				}
				modParam.distanceFuncIdx = intRand.next(parameterRange->modalityParamRange[i].distanceFuncs.size());
				modParam.pcaDim = intRand.next(parameterRange->modalityParamRange[i].pcaDimMax-parameterRange->modalityParamRange[i].pcaDimMin+1)+parameterRange->modalityParamRange[i].pcaDimMin;
				weightSum += modParam.weight;
				modParams.push_back(modParam);
			}
		} while ( !valid );

		QSharedPointer<iAMMSegParameter> p(
			new iAMMSegParameter(erw_beta, erw_gamma, erw_maxIter, modParams,
			svm_C, svm_gamma, svm_channels,
			parameterRange));
		result->append(p);
	}
	return result;
}

template <typename T>
T pop_at(std::vector<T>& v, typename std::vector<T>::size_type n)
{
	assert(n < v.size());
	T result = v[n];
	std::swap(v[n], v.back());
	v.pop_back();
	return result;
}

template <typename T>
T GetAndRemove(std::vector<std::map<int, T> > & v, int modIdx, MyRangeRandom<size_t> & rand, size_t* fixedIdx=0)
{
	size_t idx = 0;
	if (fixedIdx && *fixedIdx != std::numeric_limits<T>::max())
	{
		idx = *fixedIdx;
	}
	else
	{
		do {
			idx = rand.next(v.size());
		} while (v[idx].find(modIdx) == v[idx].end());
		if (fixedIdx)
		{
			*fixedIdx = idx;
		}
	}
	T result = v[idx][modIdx];
	v[idx].erase(modIdx);
	if (v[idx].empty())
	{
		std::swap(v[idx], v.back());
		v.pop_back();
	}
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

QSharedPointer<MyRange> CreateRange(bool log, double min, double max, int count)
{
	if (log) return QSharedPointer<MyRange>(new MyLogRange(min, max, count));
		else return QSharedPointer<MyRange>(new MyLinRange(min, max, count));
}


QString iALatinHypercubeParameterGenerator::GetName() const
{
	return QString("Latin HyperCube");
}


double nextLinLogRand(MyExtDblRandom& dblRand, bool isLog, double lower, double upper)
{
	if (isLog)
	{
		return exp(dblRand.next(log(lower),
			log(upper)));
	}
	else
	{
		return dblRand.next(lower, upper);
	}
}

ParameterListPointer iALatinHypercubeParameterGenerator::GetParameterSets(QSharedPointer<iAMMSegParameterRange> parameterRange)
{
	ParameterListPointer result(new ParameterList);
	int count = parameterRange->sampleCount;
	// for each parameter, create count values

	std::vector<double> erw_beta_Values;
	std::vector<double> erw_gamma_Values;
	std::vector<int>    erw_maxIter_Values;
	std::vector<double> svm_C_Values;
	std::vector<double> svm_gamma_Values;
	std::vector<int>    svm_channels_Values;
	std::vector<std::map<int, double> > weightValues;
	std::vector<std::map<int, int> > distanceIdxs;
	std::vector<std::map<int, int> > pcaDims;
	MyExtDblRandom dblRand;
	QSharedPointer<MyRange> erw_beta_Range   = CreateRange(parameterRange->erw_beta_logScale,  parameterRange->erw_beta_From, parameterRange->erw_beta_To, count);
	QSharedPointer<MyRange> erw_gamma_Range  = CreateRange(parameterRange->erw_gamma_logScale, parameterRange->erw_gamma_From, parameterRange->erw_gamma_To, count);
	QSharedPointer<MyRange> svm_gamma_Range = CreateRange(parameterRange->svm_gamma_logScale, parameterRange->svm_gamma_From, parameterRange->svm_gamma_To, count);
	QSharedPointer<MyRange> svm_C_Range = CreateRange(parameterRange->svm_C_logScale, parameterRange->svm_C_From, parameterRange->svm_C_To, count);
	
	// "int" ranges: add +0.99999999 to "To" to give equal probability to that last value
	QSharedPointer<MyRange> erw_maxIter_Range = CreateRange(parameterRange->erw_maxIter_logScale, parameterRange->erw_maxIter_From, parameterRange->erw_maxIter_To+0.99999999, count);
	QSharedPointer<MyRange> svm_channel_Range = CreateRange(false, parameterRange->svm_channels_From, parameterRange->svm_channels_To+0.99999999, count);

	QVector<QSharedPointer<RandomGenerator> > weightRand;
	for (int i=0; i<parameterRange->modalityParamRange.size(); ++i)
	{
		weightRand.push_back(CreateRand(parameterRange->weightLogScale, parameterRange->modalityParamRange[i].weightFrom, parameterRange->modalityParamRange[i].weightTo));
	}
	// TODO: 
	double weight0Range = parameterRange->modalityParamRange[0].weightTo - parameterRange->modalityParamRange[0].weightFrom;
	double weight0Step = weight0Range / count;
	for (int i=0; i<count; ++i)
	{
		erw_beta_Values.push_back(dblRand.next(erw_beta_Range->min(i), erw_beta_Range->max(i)));
		erw_gamma_Values.push_back(dblRand.next(erw_gamma_Range->min(i), erw_gamma_Range->max(i)));
		svm_gamma_Values.push_back(dblRand.next(svm_gamma_Range->min(i), svm_gamma_Range->max(i)));
		svm_C_Values.push_back(dblRand.next(svm_C_Range->min(i), svm_C_Range->max(i)));
		
		erw_maxIter_Values.push_back(static_cast<int>(dblRand.next(erw_maxIter_Range->min(i), erw_maxIter_Range->max(i))));
		svm_channels_Values.push_back(static_cast<int>(dblRand.next(svm_channel_Range->min(i), svm_channel_Range->max(i))));

		double weightSum = 0;
		std::map<int, double> weights;

		// TODO: find a real latin-hypercube version of that, currently this is random-sampling
		bool valid = true;
		do
		{
			weights.clear();

			// make at least first weight follow latin hypercube distribution:
			double lower = parameterRange->modalityParamRange[0].weightFrom + i*weight0Step;
			double upper = lower+weight0Step;
			weights[0] = nextLinLogRand(dblRand, parameterRange->weightLogScale, lower, upper);
			weightSum = weights[0];
			for (int modIdx=1; modIdx<parameterRange->modalityParamRange.size(); ++modIdx)
			{
				// make sure weights sum up to 1
				double weight = 0;
				if (modIdx == parameterRange->modalityParamRange.size()-1)
				{
					weight = 1.0 - weightSum;
					if (weight < parameterRange->modalityParamRange[modIdx].weightFrom ||
						weight > parameterRange->modalityParamRange[modIdx].weightTo)
					{
						valid = false;
						/*
						DebugOut() << "Weight "<< weight <<" is outside of valid interval [" << 
							parameterRange->modalityParamRange[modIdx].weightFrom << ", " << 
							parameterRange->modalityParamRange[modIdx].weightTo << "]" << std::endl;
						*/
					}
				}
				else
				{
					double lower = parameterRange->modalityParamRange[modIdx].weightFrom;
					if (lower > 1-weightSum)
					{
						valid = false;
						if (modIdx == 1)
						{
							DEBUG_LOG("Impossible situation: not enough interval left for modality 1!\n");
						}
						break;
					}
					double upper = std::min(parameterRange->modalityParamRange[modIdx].weightTo, 1-weightSum);
					weight = nextLinLogRand(dblRand, parameterRange->weightLogScale, lower, upper);
				}
				weightSum += weight;
				weights[modIdx] = weight;
			}
		} while (!valid);
		
		std::map<int, int> pca;
		std::map<int, int> distIdx;
		for (int modIdx=0; modIdx<parameterRange->modalityParamRange.size(); ++modIdx)
		{
			distIdx[modIdx] = mapValue(0, count, 0, parameterRange->modalityParamRange[modIdx].distanceFuncs.size(), i);
			pca[modIdx]     = mapValue(0, count, parameterRange->modalityParamRange[modIdx].pcaDimMin, parameterRange->modalityParamRange[modIdx].pcaDimMax+1, i);
		}
		pcaDims.push_back(pca);
		distanceIdxs.push_back(distIdx);
		weightValues.push_back(weights);
	}
	// store in list
	// count times, draw one random parameter from each list (and remove that parameter from the list)
	MyRangeRandom<int> intRand;
	MyRangeRandom<size_t> sizeTRand;
	for (int i=count; i>0; --i)
	{
		double erw_beta     = pop_at(erw_beta_Values, intRand.next(i));
		double erw_gamma    = pop_at(erw_gamma_Values, intRand.next(i));
		int    erw_maxIter  = pop_at(erw_maxIter_Values, intRand.next(i));
		double svm_C        = pop_at(svm_C_Values, intRand.next(i));
		double svm_gamma    = pop_at(svm_gamma_Values, intRand.next(i));
		int    svm_channels = pop_at(svm_channels_Values, intRand.next(i));

		/*
		int weightIdx = intRand.next(i);
		int scalarIdxIdx =  intRand.next(i);
		int spectralIdxIdx =  intRand.next(i);
		int pcaDimIdx = intRand.next(i);
		double scalarWeight = pop_at(weightValues, weightIdx);
		double spectralWeight = 1 - scalarWeight;
		int scalarIdx = pop_at(scalarDistanceIdxs, scalarIdxIdx);
		int spectralIdx = pop_at(spectralDistanceIdxs, spectralIdxIdx);
		int pcaDimensions = pop_at(pcaDims, pcaDimIdx);
		*/
		QVector<iAMMSegModalityParameter> modalityParams;
		for (int modIdx=0; modIdx<parameterRange->modalityParamRange.size(); ++modIdx)
		{
			iAMMSegModalityParameter modParam;
			modParam.pcaDim = GetAndRemove(pcaDims, modIdx, sizeTRand);
			modParam.weight = GetAndRemove(weightValues, modIdx, sizeTRand);
			modParam.distanceFuncIdx = GetAndRemove(distanceIdxs, modIdx, sizeTRand);
			modalityParams.push_back(modParam);
		}
		QSharedPointer<iAMMSegParameter> p(new iAMMSegParameter(
			erw_beta, erw_gamma, erw_maxIter,
			modalityParams,
			svm_C, svm_gamma, svm_channels,
			parameterRange));
		result->append(p);
	}
	return result;
}

QString iACartesianGridParameterGenerator::GetName() const
{
	return QString("Cartesian Grid");
}


ParameterListPointer iACartesianGridParameterGenerator::GetParameterSets(QSharedPointer<iAMMSegParameterRange> parameterRange)
{
	ParameterListPointer result(new ParameterList);
	int paramCount = parameterRange->GetInputParameterCount();
	int samplesPerParameter = static_cast<int>(std::pow(10, std::log10(parameterRange->sampleCount) / paramCount));
	
	if (samplesPerParameter = 1) // make sure we use at least 2 sample values per parameter (otherwise we only have a single sample!)
	{
		samplesPerParameter = 2;
	}

	int sampleCount = std::pow(samplesPerParameter, paramCount);

	std::vector<int> parameterRangeIdx(paramCount, 0);
	
	/*
	DEBUG_LOG(QString("parameter count: %1, sample count: %2 samplesPerParameter: %3")
		.arg(paramCount)
		.arg(parameterRange->sampleCount)
		.arg(samplesPerParameter)
	);
	*/

	for (int sampleIdx = 0; sampleIdx< sampleCount; ++sampleIdx)
	{
		//QString rangeIndices; // DEBUG

		QSharedPointer<iAMMSegParameter> parameterSet(new iAMMSegParameter(parameterRange));
		for (int paramIdx = 0; paramIdx < paramCount; ++paramIdx)
		{
			//rangeIndices += QString::number(parameterRangeIdx[paramIdx]) + " "; // DEBUG

			double rangeMin = parameterRange->GetMin(paramIdx);
			double rangeMax = parameterRange->GetMax(paramIdx);

			// TODO: logarithmic sampling
			double paramValue = mapValue(0, samplesPerParameter - 1, rangeMin, rangeMax, parameterRangeIdx[paramIdx]);
			parameterSet->setParam(paramIdx, paramValue);
		}
		result->append(parameterSet);

		//DEBUG_LOG("Parameter set: " + rangeIndices);  // DEBUG
		
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

QVector<QSharedPointer<iAMMSegParameterGenerator> > & GetParameterGenerators()
{
	static QVector<QSharedPointer<iAMMSegParameterGenerator> > parameterGenerators;
	if (parameterGenerators.empty())
	{
		parameterGenerators.push_back(QSharedPointer<iAMMSegParameterGenerator>(new iARandomParameterGenerator()));
		parameterGenerators.push_back(QSharedPointer<iAMMSegParameterGenerator>(new iALatinHypercubeParameterGenerator()));
		parameterGenerators.push_back(QSharedPointer<iAMMSegParameterGenerator>(new iACartesianGridParameterGenerator()));
	}
	return parameterGenerators;
}
