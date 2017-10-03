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

#include "iAAlgorithm.h"

class iAProbabilitySource
{
public:
	virtual QVector<vtkSmartPointer<vtkImageData> > & Probabilities() =0;
};

class iAFuzzyCMeans : public iAAlgorithm, public iAProbabilitySource
{
public:
	iAFuzzyCMeans(QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0);
	void setParameters(unsigned int maxIter, double maxError, double m, unsigned int numOfThreads, unsigned int numOfClasses,
		QVector<double> centroids, bool ignoreBg, double bgPixel);
	QVector<vtkSmartPointer<vtkImageData> > & Probabilities();
protected:
	virtual void performWork();
private:
	unsigned int m_maxIter;
	double m_maxError;
	double m_m;
	unsigned int m_numOfThreads;
	unsigned int m_numOfClasses;
	QVector<double> m_centroids;
	bool m_ignoreBg;
	double m_bgPixel;
	
	QVector<vtkSmartPointer<vtkImageData> > m_probOut;
};

#include "iAAttributeDescriptor.h"
#include "iAITKIO.h"

#include <itkKFCMSClassifierInitializationImageFilter.h>
#include <itkSimpleFilterWatcher.h>
#include <itkFuzzyClassifierImageFilter.h>

#include <QMap>

typedef QSharedPointer<iAAttributeDescriptor> pParameter;


//! Base class for image filters
//! Derived classes should:
//!     - fill the m_parameters vector in their constructor
//!     - override the SetParameters method to transfer the parameters to the actual algorithm
//!     - override the Run method to perform the actual calculations
//!       on m_inImg and (allocate and) store the result in m_outImg

class iAFilter
{
public:
	iAFilter(QString const & name, QString const & category, QString const & description):
		m_name(name),
		m_category(category),
		m_description(description)
	{}
	virtual ~iAFilter()         {}
	QString Name() const        { return m_name;     }
	QString Category() const	{ return m_category; }
	QString Description() const { return m_description; }
	QVector<pParameter> const & Parameters() const
	{
		return m_parameters;
	}
	void SetInput(vtkSmartPointer<vtkImageData> inImg)
	{
		m_inImg = inImg;
	}
	vtkSmartPointer<vtkImageData> Output() const
	{
		return m_outImg;
	}
	virtual void Run(QMap<QString, QVariant> parameters) = 0;
protected:
	QVector<pParameter> m_parameters;
	vtkSmartPointer<vtkImageData> m_inImg;
	vtkSmartPointer<vtkImageData> m_outImg;
	QString m_name, m_category, m_description;
};

typedef iAAttributeDescriptor ParamDesc;

class iAKFCMFilter : public iAFilter, public iAProbabilitySource
{
public:
	static QSharedPointer<iAKFCMFilter> Create();
	void Run(QMap<QString, QVariant> parameters) override;
	QVector<vtkSmartPointer<vtkImageData> > & Probabilities();
private:
	iAKFCMFilter();
	QVector<vtkSmartPointer<vtkImageData> > m_probOut;
};
