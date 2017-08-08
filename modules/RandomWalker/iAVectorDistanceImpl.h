/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#pragma once

#include "iAVectorDistance.h"

#include <QString>

int GetDistanceMeasureCount();
char const * const * const GetDistanceMeasureNames();
char const * const * const GetShortMeasureNames();
QSharedPointer<iAVectorDistance> GetDistanceMeasure(QString const & distFuncName);
QSharedPointer<iAVectorDistance> GetDistanceMeasureFromShortName(QString const & distFuncName);

class iASpectralAngularDistance: public iAVectorDistance
{
public:
	virtual char const * GetName() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const;
};

class iAL1NormDistance: public iAVectorDistance
{
public:
	virtual char const * GetName() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const;
};

class iAL2NormDistance: public iAVectorDistance
{
public:
	virtual char const * GetName() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const;
};

class iALInfNormDistance: public iAVectorDistance
{
public:
	virtual char const * GetName() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const;
};

class iAJensenShannonDistance : public iAVectorDistance
{
public:
	virtual char const * GetName() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const;
};

class iAKullbackLeiblerDivergence : public iAVectorDistance
{
public:
	virtual char const * GetName() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const;
	virtual bool isSymmetric() const {return false; }
};

class iAChiSquareDistance: public iAVectorDistance
{
public:
	virtual char const * GetName() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const;
};

class iAEarthMoversDistance: public iAVectorDistance
{
public:
	virtual char const * GetName() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const;
};

class iASquaredDistance: public iAVectorDistance
{
public:
	virtual char const * GetName() const;
	virtual char const * GetShortName() const;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const;
};

class iANullDistance: public iAVectorDistance
{
public:
	virtual char const * GetName() const { return "Null Dist."; }
	virtual char const * GetShortName() const { return "null"; }
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const { return 0; }
};

/*
class iAMutualInformation : public iAVectorDistance
{
public:
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2);
	virtual bool isSymmetric() {return true; }
};

class iACrossCorrelation : public iAVectorDistance
{
public:
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2);
	virtual bool isSymmetric() {return true; }
};

class iAMahalanobisDistance : public iAVectorDistance
{
public:
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2);
	virtual bool isSymmetric() {return true; }
};

class iAHausdorffDistance : public iAVectorDistance
{
public:
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2);
	virtual bool isSymmetric() {return true; }
};
*/
