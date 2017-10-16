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

#include "iAAttributeDescriptor.h"
#include "iAFilter.h"
#include "iAITKIO.h"

#include <itkKFCMSClassifierInitializationImageFilter.h>
#include <itkSimpleFilterWatcher.h>
#include <itkFuzzyClassifierImageFilter.h>

#include <vtkSmartPointer.h>

class vtkImageData;

const unsigned int ImageDimension = 3;
typedef double ProbabilityPixelType;
typedef itk::VectorImage<ProbabilityPixelType, ImageDimension> VectorImageType;

class iAProbabilitySource
{
public:
	virtual QVector<vtkSmartPointer<vtkImageData> > & Probabilities();
	void SetProbabilities(VectorImageType::Pointer vectorImg);
private:
	QVector<vtkSmartPointer<vtkImageData> > m_probOut;
};

typedef iAAttributeDescriptor ParamDesc;

class iAFCMFilter : public iAFilter, public iAProbabilitySource
{
public:
	static QSharedPointer<iAFCMFilter> Create();
	bool CheckParameters(QMap<QString, QVariant> & parameters) override;
	void Run(QMap<QString, QVariant> const & parameters) override;
private:
	iAFCMFilter();
};

class iAKFCMFilter : public iAFilter, public iAProbabilitySource
{
public:
	static QSharedPointer<iAKFCMFilter> Create();
	bool CheckParameters(QMap<QString, QVariant> & parameters) override;
	void Run(QMap<QString, QVariant> const & parameters) override;
private:
	iAKFCMFilter();
};

class iAMSKFCMFilter : public iAFilter, public iAProbabilitySource
{
public:
	static QSharedPointer<iAMSKFCMFilter> Create();
	bool CheckParameters(QMap<QString, QVariant> & parameters) override;
	void Run(QMap<QString, QVariant> const & parameters) override;
private:
	iAMSKFCMFilter();
};
