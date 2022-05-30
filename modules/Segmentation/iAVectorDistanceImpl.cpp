/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVectorDistanceImpl.h"

#include <iAMathUtility.h>  // required for clamp

#include <QVector>

#include <numeric>
#include <cmath>

namespace
{
	double VectorLength(QSharedPointer<iAVectorType const> spec)
	{
		double sum = 0.0;
		for(iAVectorType::IndexType i = 0; i<spec->size(); ++i)
		{
			sum += static_cast<double>(spec->get(i)) * spec->get(i);
		}
		return std::sqrt(sum);
	}

	const char * const MeasureNames[dmCount+1] =
	{
		  "L1norm"
		, "L2norm"
		, "L-infinity-norm"
		, "Cosine dist."
		, "Jensen-Shannon dist."
		, "Kullback-Leibler divergence"
		, "Chi-Square dist."
		, "Earth Mover's dist."
		, "Squared"
	// ----- insert new measures before here
		, "Invalid"
		/*
		, "Mutual information"
		, "Cross Correlation"
		, "Mahalanobis distance"
		, "Hausdorff distance"
		*/
	};

	const char * const MeasureShortNames[dmCount+1] =
	{
		  "l1"
		, "l2"
		, "linf"
		, "cos"
		, "js"
		, "kl"
		, "cs"
		, "em"
		, "sq"
		/*
		, "Mutual information"
		, "Cross Correlation"
		, "Mahalanobis distance"
		, "Hausdorff distance"
		*/
	};
	QSharedPointer<iAVectorDistance> const Measure [dmCount+1] =
	{
		QSharedPointer<iAL1NormDistance>::create(),
		QSharedPointer<iAL2NormDistance>::create(),
		QSharedPointer<iALInfNormDistance>::create(),
		QSharedPointer<iASpectralAngularDistance>::create(),
		QSharedPointer<iAJensenShannonDistance>::create(),
		QSharedPointer<iAKullbackLeiblerDivergence>::create(),
		QSharedPointer<iAChiSquareDistance>::create(),
		QSharedPointer<iAEarthMoversDistance>::create(),
		QSharedPointer<iASquaredDistance>::create(),
		// ----------
		QSharedPointer<iANullDistance>::create()
	};
}


int GetDistanceMeasureCount()
{
	return dmCount;
}


char const * const * GetDistanceMeasureNames()
{
	return MeasureNames;
}

char const * const * GetShortMeasureNames()
{
	return MeasureShortNames;
}


QSharedPointer<iAVectorDistance> GetDistanceMeasure(QString const & distFuncName)
{
	// TODO: static array of all distance measures, only return reference to that
	if (distFuncName == MeasureNames[dmL1])                   return Measure[dmL1];
	else if (distFuncName == MeasureNames[dmL2])              return Measure[dmL2];
	else if (distFuncName == MeasureNames[dmLinf])            return Measure[dmLinf];
	else if (distFuncName == MeasureNames[dmCosine])          return Measure[dmCosine];
	else if (distFuncName == MeasureNames[dmJensenShannon])   return Measure[dmJensenShannon];
	else if (distFuncName == MeasureNames[dmKullbackLeibler]) return Measure[dmKullbackLeibler];
	else if (distFuncName == MeasureNames[dmChiSquare])       return Measure[dmChiSquare];
	else if (distFuncName == MeasureNames[dmEarthMovers])     return Measure[dmEarthMovers];
	else if (distFuncName == MeasureNames[dmSquared])         return Measure[dmSquared];
	// we _should_ never get here...
	assert(false);
	return Measure[dmInvalid];
}

QSharedPointer<iAVectorDistance> GetDistanceMeasureFromShortName(QString const & distFuncName)
{
	if (distFuncName == MeasureShortNames[dmL1])                   return Measure[dmL1];
	else if (distFuncName == MeasureShortNames[dmL2])			   return Measure[dmL2];
	else if (distFuncName == MeasureShortNames[dmLinf])			   return Measure[dmLinf];
	else if (distFuncName == MeasureShortNames[dmCosine])		   return Measure[dmCosine];
	else if (distFuncName == MeasureShortNames[dmJensenShannon])   return Measure[dmJensenShannon];
	else if (distFuncName == MeasureShortNames[dmKullbackLeibler]) return Measure[dmKullbackLeibler];
	else if (distFuncName == MeasureShortNames[dmChiSquare])	   return Measure[dmChiSquare];
	else if (distFuncName == MeasureShortNames[dmEarthMovers])	   return Measure[dmEarthMovers];
	else if (distFuncName == MeasureShortNames[dmSquared])		   return Measure[dmSquared];
	// we _should_ never get here...
	assert(false);
	return Measure[dmInvalid];
}


iAVectorDistance::~iAVectorDistance()
{}

bool iAVectorDistance::isSymmetric() const
{
	return true;
}


double iAVectorDistance::EPSILON = 10e-6;

char const * iASpectralAngularDistance::GetShortName() const
{
	return MeasureShortNames[dmCosine];
}

char const * iASpectralAngularDistance::name() const
{
	return MeasureNames[dmCosine];
}

double iASpectralAngularDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	assert(spec1->size() == spec2->size());
	double prod = 0;
	for(iAVectorType::IndexType i = 0; i<spec1->size(); ++i)
	{
		prod += static_cast<double>(spec1->get(i)) * spec2->get(i);
	}
	//double prod = std::inner_product(&spec1[0], spec1+spec1->size(), spec2, 0.0);
	double len1 = VectorLength(spec1);
	double len2 = VectorLength(spec2);
	//assert (len1 != 0 && len2 != 0);
	if (len1 == 0 || len2 == 0)
	{
		return 0;
	}
	double cosAngle = prod / (len1*len2);
//	assert ( cosAngle >= -1 && cosAngle <= 1 );
	return clamp(-1.0, 1.0, cosAngle);
}

char const * iAL1NormDistance::GetShortName() const
{
	return MeasureShortNames[dmL1];
}

char const * iAL1NormDistance::name() const
{
	return MeasureNames[dmL1];
}

double iAL1NormDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	assert(spec1->size() == spec2->size());
	double sum = 0;
	for(iAVectorType::IndexType i = 0; i<spec1->size(); ++i)
	{
		sum += std::abs(static_cast<double>(spec1->get(i)) - spec2->get(i));
	}
	return sum;
}

char const * iAL2NormDistance::GetShortName() const
{
	return MeasureShortNames[dmL2];
}

char const * iAL2NormDistance::name() const
{
	return MeasureNames[dmL2];
}

double iAL2NormDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	assert(spec1->size() == spec2->size());
	double sum = 0;
	for(iAVectorType::IndexType i = 0; i<spec1->size(); ++i)
	{
		double a = static_cast<double>(spec1->get(i)) - spec2->get(i);
		sum += std::pow(a, 2);
	}
	return std::sqrt(sum);
}

char const * iALInfNormDistance::GetShortName() const
{
	return MeasureShortNames[dmLinf];
}

char const * iALInfNormDistance::name() const
{
	return MeasureNames[dmLinf];
}

double iALInfNormDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	assert(spec1->size() == spec2->size());
	double maxDist=0;
	for(iAVectorType::IndexType i = 0; i<spec1->size(); ++i)
	{
		double dist = std::abs(static_cast<double>(spec1->get(i)) - spec2->get(i));
		if (dist > maxDist)
		{
			maxDist = dist;
		}
	}
	return maxDist;
}

char const * iAJensenShannonDistance::GetShortName() const
{
	return MeasureShortNames[dmJensenShannon];
}

char const * iAJensenShannonDistance::name() const
{
	return MeasureNames[dmJensenShannon];
}

double iAJensenShannonDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	iAKullbackLeiblerDivergence kld;
	return std::sqrt(0.5 * kld.GetDistance(spec1, spec2) + 0.5 * kld.GetDistance(spec2, spec1));
}

char const * iAKullbackLeiblerDivergence::GetShortName() const
{
	return MeasureShortNames[dmKullbackLeibler];
}

char const * iAKullbackLeiblerDivergence::name() const
{
	return MeasureNames[dmKullbackLeibler];
}

double iAKullbackLeiblerDivergence::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	assert(spec1->size() == spec2->size());
	double kldiv = 0;
	QSharedPointer<iAVectorType const> s1 = spec1->normalized();
	QSharedPointer<iAVectorType const> s2 = spec2->normalized();
	for(iAVectorType::IndexType i = 0; i<s1->size(); ++i)
	{
		double logTerm = (s2->get(i) == 0)? 0 : (s1->get(i) / s2->get(i));
		if (qIsInf(logTerm) || qIsNaN(logTerm))
		{
			logTerm = 0;
		}
		kldiv += (logTerm == 0)? 0 : (std::log(logTerm) * (s1->get(i)));
		if (qIsInf(kldiv) || qIsNaN(kldiv))
		{
			kldiv = 0;
		}
	}
	return kldiv;
}

char const * iAChiSquareDistance::GetShortName() const
{
	return MeasureShortNames[dmChiSquare];
}

char const * iAChiSquareDistance::name() const
{
	return MeasureNames[dmChiSquare];
}

double iAChiSquareDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	assert(spec1->size() == spec2->size());
	double chiSquare = 0;
	QSharedPointer<iAVectorType const> s1 = spec1->normalized();
	QSharedPointer<iAVectorType const> s2 = spec2->normalized();
	for(iAVectorType::IndexType i = 0; i<s1->size(); ++i)
	{
		chiSquare += std::pow(s1->get(i) - s2->get(i), 2) / (s1->get(i) + s2->get(i));
	}
	return chiSquare / 2.0;
}

char const * iAEarthMoversDistance::GetShortName() const
{
	return MeasureShortNames[dmEarthMovers];
}

char const * iAEarthMoversDistance::name() const
{
	return MeasureNames[dmEarthMovers];
}


double iAEarthMoversDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	assert(spec1->size() == spec2->size());
	double emd = 0;
	double lastEmd = 0;
	QSharedPointer<iAVectorType const> s1 = spec1->normalized();
	QSharedPointer<iAVectorType const> s2 = spec2->normalized();
	for(iAVectorType::IndexType i = 0; i<s1->size(); ++i)
	{
		double newEmd = s1->get(i) + lastEmd - s2->get(i);
		emd += std::abs(newEmd);
		lastEmd = newEmd;
	}
	return emd;
}

char const * iASquaredDistance::GetShortName() const
{
	return MeasureShortNames[dmSquared];
}

char const * iASquaredDistance::name() const
{
	return MeasureNames[dmSquared];
}

double iASquaredDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	double sum = 0;
	for(iAVectorType::IndexType i = 0; i<spec1->size(); ++i)
	{
		double a = static_cast<double>(spec1->get(i)) - spec2->get(i);
		sum += std::pow(a, 2);
	}
	return sum;
}


/*

double iAMutualInformation::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	return 0.5;
}

double iACrossCorrelation::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	return 0.5;
}

double iAMahalanobisDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	return 0.5;
}

double iAHausdorffDistance::GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const
{
	return 0.5;
}

*/
