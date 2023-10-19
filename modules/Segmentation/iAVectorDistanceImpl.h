// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVectorDistance.h"

#include <QString>

enum MeasureIndices
{
	dmL1,
	dmL2,
	dmLinf,
	dmCosine,
	dmJensenShannon,
	dmKullbackLeibler,
	dmChiSquare,
	dmEarthMovers,
	dmSquared,

	dmCount,
	dmInvalid = dmCount
};

int GetDistanceMeasureCount();
char const * const * GetDistanceMeasureNames();
char const * const * GetShortMeasureNames();
std::shared_ptr<iAVectorDistance> GetDistanceMeasure(QString const & distFuncName);
std::shared_ptr<iAVectorDistance> GetDistanceMeasureFromShortName(QString const & distFuncName);

class Segmentation_API iASpectralAngularDistance: public iAVectorDistance
{
public:
	virtual char const * name() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const;
};

class Segmentation_API iAL1NormDistance: public iAVectorDistance
{
public:
	virtual char const * name() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const;
};

class Segmentation_API iAL2NormDistance: public iAVectorDistance
{
public:
	virtual char const * name() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const;
};

class Segmentation_API iALInfNormDistance: public iAVectorDistance
{
public:
	virtual char const * name() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const;
};

class Segmentation_API iAJensenShannonDistance : public iAVectorDistance
{
public:
	virtual char const * name() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const;
};

class Segmentation_API iAKullbackLeiblerDivergence : public iAVectorDistance
{
public:
	virtual char const * name() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const;
	virtual bool isSymmetric() const {return false; }
};

class Segmentation_API iAChiSquareDistance: public iAVectorDistance
{
public:
	virtual char const * name() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const;
};

class Segmentation_API iAEarthMoversDistance: public iAVectorDistance
{
public:
	virtual char const * name() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const;
};

class Segmentation_API iASquaredDistance: public iAVectorDistance
{
public:
	virtual char const * name() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const;
};

class Segmentation_API iANullDistance: public iAVectorDistance
{
public:
	virtual char const * name() const { return "Null Dist."; }
	virtual char const * GetShortName() const { return "null"; }
	virtual double GetDistance(std::shared_ptr<iAVectorType const> /*spec1*/, std::shared_ptr<iAVectorType const> /*spec2*/) const { return 0; }
};

/*
class iAMutualInformation : public iAVectorDistance
{
public:
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2);
	virtual bool isSymmetric() {return true; }
};

class iACrossCorrelation : public iAVectorDistance
{
public:
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2);
	virtual bool isSymmetric() {return true; }
};

class iAMahalanobisDistance : public iAVectorDistance
{
public:
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2);
	virtual bool isSymmetric() {return true; }
};

class iAHausdorffDistance : public iAVectorDistance
{
public:
	virtual double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2);
	virtual bool isSymmetric() {return true; }
};
*/
