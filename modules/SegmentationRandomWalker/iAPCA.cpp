/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iAImageCoordinate.h"
#include "iAitkImagesMultiChannelAdapter.h"
#include "iASpectrumType.h"

#include <itkImage.h>
#include <itkMultiplyImageFilter.h>
#include <itkImagePCAShapeModelEstimator.h>
#include <itkNumericSeriesFileNames.h>

iAPCA::iAPCA(QSharedPointer<iASpectralVoxelData const> spectralData):
	m_spectralData(spectralData)
{
}

QSharedPointer<iASpectralVoxelData const> iAPCA::GetReduced(iAImageCoordConverter const & convert, int cutOff)
{
	const int spectraCount = m_spectralData->size();
	assert(spectraCount > 0);
	const int spectrumBinCount = m_spectralData->channelCount();

	const unsigned int Dimensions = 3;
	typedef iASpectrumDataType PixelType;
	typedef itk::Image<PixelType, Dimensions> ImageType;
	typedef itk::MultiplyImageFilter<ImageType, ImageType, ImageType> ScaleType;
	typedef itk::ImagePCAShapeModelEstimator<ImageType, ImageType>  EstimatorType;
 
	EstimatorType::Pointer filter = EstimatorType::New();
	filter->SetNumberOfTrainingImages(spectrumBinCount);
	filter->SetNumberOfPrincipalComponentsRequired(cutOff);

	std::vector<ImageType::Pointer> trainingImages(spectrumBinCount);
	// allocate and fill input images:

	// TODO: reuse existing images ???
	for ( unsigned int k = 0; k < spectrumBinCount; k++ )
	{
		ImageType::Pointer myImg(ImageType::New());
		ImageType::IndexType orig;
		orig.Fill(0);
		ImageType::SizeType size;
		size[0] = convert.GetWidth();
		size[1] = convert.GetHeight();
		size[2] = convert.GetDepth();
		ImageType::RegionType reg;
		reg.SetIndex(orig); 
		reg.SetSize(size);
		myImg->SetRegions(reg);
		myImg->Allocate();

		ImageType::IndexType idx;
		for (idx[0] = 0; idx[0] < convert.GetWidth(); ++idx[0])
		{
			for (idx[1] = 0; idx[1] < convert.GetHeight(); ++idx[1])
			{
				for (idx[2] = 0; idx[2] < convert.GetDepth(); ++idx[2])
				{
					int specIdx = convert.GetIndexFromCoordinates(iAImageCoordinate(idx[0], idx[1], idx[2]));
					iASpectrumDataType voxelValue = m_spectralData->get(specIdx, k);
					myImg->SetPixel(idx, voxelValue);
				}
			}
		}
		trainingImages[k] = myImg;
		filter->SetInput(k, trainingImages[k] );
	}
 
	// actual PCA calculation:
	filter->Update();
	EstimatorType::VectorOfDoubleType v = filter->GetEigenValues();
	double sv_mean=sqrt(v[0]);

	// create output in required format:
	QSharedPointer<iAitkImagesMultiChannelAdapter<ImageType> >
		result(new iAitkImagesMultiChannelAdapter<ImageType>(convert.GetWidth(), convert.GetHeight(), convert.GetDepth()));
	for (int binIdx = 0; binIdx < cutOff; ++binIdx)
	{
		ImageType::Pointer binImg = filter->GetOutput(binIdx);
		result->AddImage(binImg);
	}
	return result;
}

void iAPCA::run()
{
}