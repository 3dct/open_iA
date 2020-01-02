/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAPCA.h"

#include <defines.h>    // for DIM
#include <iAConnector.h>
#include <iATypedCallHelper.h>

#include <itkImage.h>
#include <itkMultiplyImageFilter.h>
#include <itkImagePCAShapeModelEstimator.h>
#include <itkNumericSeriesFileNames.h>


iAPCA::iAPCA() :
	iAFilter("Principal Component Analysis", "Segmentation/Shape Analysis",
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
	addParameter("Cutoff", Discrete, 1);
}

IAFILTER_CREATE(iAPCA)


template <typename PixelType>
void pca(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<PixelType, DIM> ImageType;
	typedef itk::MultiplyImageFilter<ImageType, ImageType, ImageType> ScaleType;
	typedef itk::ImagePCAShapeModelEstimator<ImageType, ImageType>  EstimatorType;

	auto pcaFilter = EstimatorType::New();
	pcaFilter->SetNumberOfTrainingImages(filter->input().size());
	pcaFilter->SetNumberOfPrincipalComponentsRequired(parameters["Cutoff"].toUInt());
	for (unsigned int k = 0; k < filter->input().size(); k++)
		pcaFilter->SetInput(k, dynamic_cast<ImageType*>(filter->input()[k]->itkImage()));
	pcaFilter->Update();
	auto scaler = ScaleType::New();
	auto v = pcaFilter->GetEigenValues();
	double sv_mean = sqrt(v[0]);
	for (int o = 0; o < parameters["Cutoff"].toUInt() + 1; ++o)
	{
		double sv = sqrt(v[o]);
		double sv_n = sv / sv_mean;
		scaler->SetConstant(sv_n);
		scaler->SetInput(pcaFilter->GetOutput(o));
		scaler->Update();
		filter->addOutput(scaler->GetOutput());
	}
}

void iAPCA::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(pca, inputPixelType(), this, parameters);
}
