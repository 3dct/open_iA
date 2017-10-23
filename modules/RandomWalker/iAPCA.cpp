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
#include "pch.h"
#include "iAPCA.h"

#include "defines.h"    // for DIM
#include "iAConnector.h"
#include "iATypedCallHelper.h"

#include <itkImage.h>
#include <itkMultiplyImageFilter.h>
#include <itkImagePCAShapeModelEstimator.h>
#include <itkNumericSeriesFileNames.h>


iAPCA::iAPCA() :
	iAFilter("Principal Component Analysis", "Shape Analysis",
		"Computes the principal component analysis on a collection of input images.<br/>"
		"Given a number of input channels or images of same dimensions, this filter "
		"performs a transformation and reduces the number of output channels to the "
		"number given in the <em>Cutoff</em> parameter.<br/>"
		"Note that you will receive one more image than specified in the Cutoff parameter "
		"- the first image returned is the mean image.<br/>"
		"For more information see "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ImagePCAShapeModelEstimator.html/\">"
		"Image PCA Shape Model Estimator</a> ITK documentation.")
{
	AddParameter("Cutoff", Discrete, 1);
}

IAFILTER_CREATE(iAPCA)


template <typename PixelType>
void pca_template(QVector<iAConnector*> cons, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<PixelType, DIM> ImageType;
	typedef itk::MultiplyImageFilter<ImageType, ImageType, ImageType> ScaleType;
	typedef itk::ImagePCAShapeModelEstimator<ImageType, ImageType>  EstimatorType;

	auto filter = EstimatorType::New();
	filter->SetNumberOfTrainingImages(cons.size());
	filter->SetNumberOfPrincipalComponentsRequired(parameters["Cutoff"].toUInt());
	for (unsigned int k = 0; k < cons.size(); k++)
	{
		filter->SetInput(k, dynamic_cast<ImageType*>(cons[k]->GetITKImage()));
	}

	// actual PCA calculation:
	filter->Update();
	auto scaler = ScaleType::New();
	EstimatorType::VectorOfDoubleType v = filter->GetEigenValues();
	double sv_mean = sqrt(v[0]);
	for (int o = cons.size(); o < parameters["Cutoff"].toUInt() + 1; ++o)
	{
		cons.push_back(new iAConnector);
	}
	for (int o = 0; o < parameters["Cutoff"].toUInt() + 1; ++o)
	{
		double sv = sqrt(v[o]);
		double sv_n = sv / sv_mean;
		scaler->SetConstant(sv_n);
		scaler->SetInput(filter->GetOutput(o));
		cons[o]->SetImage(scaler->GetOutput());
	}
}

void iAPCA::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(pca_template, m_con->GetITKScalarPixelType(), m_cons, parameters);
	SetOutputCount(parameters["Cutoff"].toUInt() + 1);
}
