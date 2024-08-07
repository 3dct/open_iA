// Copyright (c) open_iA contributors
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
	char const * name() const override;
	char const * GetShortName() const override;
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
};

class Segmentation_API iAL1NormDistance: public iAVectorDistance
{
public:
	char const * name() const override;
	char const * GetShortName() const override;
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
};

class Segmentation_API iAL2NormDistance: public iAVectorDistance
{
public:
	char const * name() const override;
	char const * GetShortName() const override;
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
};

class Segmentation_API iALInfNormDistance: public iAVectorDistance
{
public:
	char const * name() const override;
	char const * GetShortName() const override;
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
};

class Segmentation_API iAJensenShannonDistance : public iAVectorDistance
{
public:
	char const * name() const override;
	char const * GetShortName() const override;
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
};

class Segmentation_API iAKullbackLeiblerDivergence : public iAVectorDistance
{
public:
	char const * name() const override;
	char const * GetShortName() const override;
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
	bool isSymmetric() const  override {return false; }
};

class Segmentation_API iAChiSquareDistance: public iAVectorDistance
{
public:
	char const * name() const override;
	char const * GetShortName() const override;
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
};

class Segmentation_API iAEarthMoversDistance: public iAVectorDistance
{
public:
	char const * name() const override;
	char const * GetShortName() const override;
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
};

class Segmentation_API iASquaredDistance: public iAVectorDistance
{
public:
	char const * name() const override;
	char const * GetShortName() const override;
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
};

class Segmentation_API iANullDistance: public iAVectorDistance
{
public:
	char const * name() const override { return "Null Dist."; }
	char const * GetShortName() const  override { return "null"; }
	double GetDistance(std::shared_ptr<iAVectorType const> /*spec1*/, std::shared_ptr<iAVectorType const> /*spec2*/) const override { return 0; }
};

/*
class iAMutualInformation : public iAVectorDistance
{
public:
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
	bool isSymmetric() const override {return true; }
};

class iACrossCorrelation : public iAVectorDistance
{
public:
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
	bool isSymmetric() const override {return true; }
};

class iAMahalanobisDistance : public iAVectorDistance
{
public:
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
	bool isSymmetric() {return true; }
};

class iAHausdorffDistance : public iAVectorDistance
{
public:
	double GetDistance(std::shared_ptr<iAVectorType const> spec1, std::shared_ptr<iAVectorType const> spec2) const override;
	bool isSymmetric() const override {return true; }
};
*/
