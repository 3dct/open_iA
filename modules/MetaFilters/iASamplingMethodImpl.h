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
#pragma once

#include "MetaFilters_export.h"

#include "iASamplingMethod.h"

//! Generate a given number of parameter sets, randomly distributed (uniform distribution).
class iARandomSamplingMethod : public iASamplingMethod
{
public:
	QString name() const override;
	iAParameterSetsPointer parameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount) override;
};

//! Uniform / Cartesian grid sampling.
//! This algorithm will yield a different amount of samples than specified (typically smaller).
//! Number of samples per parameter = 10 ^ ( log10(sampleCount) / (number of parameters) ) -> but a minimum of 2
//! Total number of samples = (Number of samples per parameter) * (number of parameters)
class iACartesianGridSamplingMethod : public iASamplingMethod
{
public:
	QString name() const override;
	iAParameterSetsPointer parameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount) override;
};

//! Use latin hypercube sampling to generate the parameter sets.
//! This method works the same as distributing 8 queens on a chess board without any "overlaps" in rows or columns.
class iALatinHypercubeSamplingMethod : public iASamplingMethod
{
public:
	QString name() const override;
	iAParameterSetsPointer parameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount) override;
};

//! Generates parameters around middle of given range for each parameter - "One at a time" sensitivity sampling.
//! For linear range, equivalent to Cartesian Grid sampler;
//! For logarithmic range, it starts in the middle, and expands outward.
class iALocalSensitivitySamplingMethod : public iASamplingMethod
{
public:
	QString name() const override;
	iAParameterSetsPointer parameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount) override;
};

//! Generates a global sensitivity sampling by generating additional parameter sets
//! in a star shape around each parameter set generated by another given generator.
class MetaFilters_API iAGlobalSensitivitySamplingMethod : public iASamplingMethod
{
public:
	iAGlobalSensitivitySamplingMethod(QSharedPointer<iASamplingMethod> otherGenerator, double delta);
	QString name() const override;
	iAParameterSetsPointer parameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount) override;
private:
	QSharedPointer<iASamplingMethod> m_baseGenerator;
	double m_delta;
};

class MetaFilters_API iAGlobalSensitivitySmallStarSamplingMethod: public iASamplingMethod
{
public:
	iAGlobalSensitivitySmallStarSamplingMethod(QSharedPointer<iASamplingMethod> otherGenerator,
		double delta, int numSteps);
	QString name() const override;
	iAParameterSetsPointer parameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount) override;

private:
	QSharedPointer<iASamplingMethod> m_baseGenerator;
	double m_delta;
	int m_numSteps;
};

class MetaFilters_API iARerunSamplingMethod : public iASamplingMethod
{
public:
	iARerunSamplingMethod(QString const& fileName);
	iARerunSamplingMethod(iAParameterSetsPointer parameterSets, QString const& name);
	virtual QString name() const;
	virtual iAParameterSetsPointer parameterSets(QSharedPointer<iAAttributes> parameter, int sampleCount);
private:
	QString m_name;
	iAParameterSetsPointer m_parameterSets;
};

//! Get the names of all sampling strategies that can be constructed via createSamplingMethod(...)
MetaFilters_API QStringList const & samplingMethodNames();

//! Construct a sampling strategy from the given parameters.
//! Note that most strategies don't require any parameters,
//! but to provide a generic interface for the construction
//! of all strategies, the parameters are required.
MetaFilters_API QSharedPointer<iASamplingMethod> createSamplingMethod(QVariantMap const & parameters);
