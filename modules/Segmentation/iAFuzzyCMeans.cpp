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
#include "iAFuzzyCMeans.h"

#include "iAConnector.h"
#include "iATypedCallHelper.h"

#include <itkFCMClassifierInitializationImageFilter.h>
#include <itkFuzzyClassifierImageFilter.h>

template <typename ImagePixelType>
void fuzzycmeans_template(iAConnector * img, unsigned int maxIter, double maxError, double m, unsigned int numOfThreads, unsigned int numOfClasses,
	QVector<double> const & centroids, bool ignoreBackgroundPixels, double backgroundPixel)
{
	typedef unsigned int OPixelType;
	typedef itk::Image<ImagePixelType, 3> IType;
	typedef itk::Image<OPixelType, 3> OType;
	typedef itk::FuzzyClassifierInitializationImageFilter<IType> TFuzzyClassifier;
	typedef itk::FCMClassifierInitializationImageFilter<IType> TClassifierFCM;

	TClassifierFCM::Pointer classifier = TClassifierFCM::New();
	classifier->SetMaximumNumberOfIterations(maxIter);
	classifier->SetMaximumError(maxError);
	classifier->SetM(m);
	classifier->SetNumberOfThreads(numOfThreads);
	classifier->SetNumberOfClasses(numOfClasses);
	TFuzzyClassifier::CentroidArrayType centroidsArray;
	for (int i = 0; i < numOfClasses; i++)
	{
		centroidsArray.push_back(centroids[i]);
	}
	classifier->SetCentroids(centroidsArray);
	classifier->SetIgnoreBackgroundPixels(ignoreBackgroundPixels);
	classifier->SetBackgroundPixel(backgroundPixel);
	classifier->SetInput(dynamic_cast< IType * >(img->GetITKImage()));

	auto probabilities = classifier->GetOutput();

	typedef itk::FuzzyClassifierImageFilter<TClassifierFCM::OutputImageType> TLabelClassifier;
	TLabelClassifier::Pointer labelClass = TLabelClassifier::New();
	labelClass->SetInput(probabilities);
	img->SetImage(labelClass->GetOutput());
}

iAFuzzyCMeans::iAFuzzyCMeans(QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent)
	: iAAlgorithm(fn, i, p, logger, parent)
{}

void iAFuzzyCMeans::setParameters(unsigned int maxIter, double maxError, double m, unsigned int numOfThreads, unsigned int numOfClasses,
	QVector<double> centroids, bool ignoreBg, double bgPixel)
{
	m_maxIter = maxIter;
	m_maxError = maxError;
	m_m = m;
	m_numOfThreads = numOfThreads;
	m_numOfClasses = numOfClasses;
	m_centroids = centroids;
	m_ignoreBg = ignoreBg;
	m_bgPixel = bgPixel;
}

void iAFuzzyCMeans::performWork()
{
	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	ITK_TYPED_CALL(fuzzycmeans_template, itkType, getConnector(), m_maxIter, m_maxError, m_m, m_numOfThreads, m_numOfClasses, m_centroids, m_ignoreBg, m_bgPixel);
}
