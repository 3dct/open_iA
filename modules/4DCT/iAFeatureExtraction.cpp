// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFeatureExtraction.h"

#include "iAFeature.h"

#include <iAFileUtils.h>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkLabelGeometryImageFilter2.h>

#include <fstream>
#include <vector>

typedef itk::Image<unsigned short, 3>	ImageType;

void iAFeatureExtraction::run(QString inputImgPath, QString outputImgPath)
{
	std::cout << "Feature extracted started\n";

	ImageType::Pointer labelImage = ImageType::New();

	typedef itk::ImageFileReader<ImageType> ImageReaderType;
	ImageReaderType::Pointer labelReader = ImageReaderType::New();
	labelReader->SetFileName( getLocalEncodingFileName(inputImgPath) );
	labelReader->Update();

	labelImage = labelReader->GetOutput();

	double origin[3] = {0.0, 0.0, 0.0};
	labelImage->SetOrigin(origin);

	typedef itk::LabelGeometryImageFilter2<ImageType> LabelGeometryImageFilterType;
	LabelGeometryImageFilterType::Pointer labelGeometryImageFilter = LabelGeometryImageFilterType::New();
	labelGeometryImageFilter->SetInput(labelImage);

	// These generate optional outputs.
	labelGeometryImageFilter->CalculatePixelIndicesOn();
	labelGeometryImageFilter->CalculateOrientedBoundingBoxOn();
	//labelGeometryImageFilter->CalculateOrientedLabelRegionsOn();
	//labelGeometryImageFilter->CalculateOrientedIntensityRegionsOn();
	labelGeometryImageFilter->Update();

	LabelGeometryImageFilterType::LabelsType allLabels = labelGeometryImageFilter->GetLabels();
	LabelGeometryImageFilterType::LabelsType::iterator allLabelsIt;
	std::vector<iAFeature> features;
	for( allLabelsIt = allLabels.begin(); allLabelsIt != allLabels.end(); allLabelsIt++ )
	{
		LabelGeometryImageFilterType::LabelPixelType labelValue = *allLabelsIt;
		iAFeature f;
		f.id = (int)labelValue;
		f.volume = labelGeometryImageFilter->GetVolume(labelValue);
		f.centroid[0] = labelGeometryImageFilter->GetCentroid(labelValue)[0];
		f.centroid[1] = labelGeometryImageFilter->GetCentroid(labelValue)[1];
		f.centroid[2] = labelGeometryImageFilter->GetCentroid(labelValue)[2];
		f.eigenvalues[0] = labelGeometryImageFilter->GetEigenvalues(labelValue)[0];
		f.eigenvalues[1] = labelGeometryImageFilter->GetEigenvalues(labelValue)[1];
		f.eigenvalues[2] = labelGeometryImageFilter->GetEigenvalues(labelValue)[2];
		f.eigenvectors[0][0] = labelGeometryImageFilter->GetEigenvectors(labelValue)[0][0];
		f.eigenvectors[0][1] = labelGeometryImageFilter->GetEigenvectors(labelValue)[0][1];
		f.eigenvectors[0][2] = labelGeometryImageFilter->GetEigenvectors(labelValue)[0][2];
		f.eigenvectors[1][0] = labelGeometryImageFilter->GetEigenvectors(labelValue)[1][0];
		f.eigenvectors[1][1] = labelGeometryImageFilter->GetEigenvectors(labelValue)[1][1];
		f.eigenvectors[1][2] = labelGeometryImageFilter->GetEigenvectors(labelValue)[1][2];
		f.eigenvectors[2][0] = labelGeometryImageFilter->GetEigenvectors(labelValue)[2][0];
		f.eigenvectors[2][1] = labelGeometryImageFilter->GetEigenvectors(labelValue)[2][1];
		f.eigenvectors[2][2] = labelGeometryImageFilter->GetEigenvectors(labelValue)[2][2];
		f.axesLength[0] = labelGeometryImageFilter->GetAxesLength(labelValue)[0];
		f.axesLength[1] = labelGeometryImageFilter->GetAxesLength(labelValue)[1];
		f.axesLength[2] = labelGeometryImageFilter->GetAxesLength(labelValue)[2];
		f.bb[0] = labelGeometryImageFilter->GetBoundingBox(labelValue)[0];
		f.bb[1] = labelGeometryImageFilter->GetBoundingBox(labelValue)[1];
		f.bb[2] = labelGeometryImageFilter->GetBoundingBox(labelValue)[2];
		f.bb[3] = labelGeometryImageFilter->GetBoundingBox(labelValue)[3];
		f.bb[4] = labelGeometryImageFilter->GetBoundingBox(labelValue)[4];
		f.bb[5] = labelGeometryImageFilter->GetBoundingBox(labelValue)[5];
		f.bbVolume = labelGeometryImageFilter->GetBoundingBoxVolume(labelValue);
		f.bbSize[0] = labelGeometryImageFilter->GetBoundingBoxSize(labelValue)[0];
		f.bbSize[1] = labelGeometryImageFilter->GetBoundingBoxSize(labelValue)[1];
		f.bbSize[2] = labelGeometryImageFilter->GetBoundingBoxSize(labelValue)[2];
		f.obbVertices[0][0] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[0][0];
		f.obbVertices[0][1] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[0][1];
		f.obbVertices[0][2] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[0][2];
		f.obbVertices[1][0] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[1][0];
		f.obbVertices[1][1] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[1][1];
		f.obbVertices[1][2] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[1][2];
		f.obbVertices[2][0] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[2][0];
		f.obbVertices[2][1] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[2][1];
		f.obbVertices[2][2] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[2][2];
		f.obbVertices[3][0] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[3][0];
		f.obbVertices[3][1] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[3][1];
		f.obbVertices[3][2] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[3][2];
		f.obbVertices[4][0] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[4][0];
		f.obbVertices[4][1] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[4][1];
		f.obbVertices[4][2] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[4][2];
		f.obbVertices[5][0] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[5][0];
		f.obbVertices[5][1] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[5][1];
		f.obbVertices[5][2] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[5][2];
		f.obbVertices[6][0] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[6][0];
		f.obbVertices[6][1] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[6][1];
		f.obbVertices[6][2] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[6][2];
		f.obbVertices[7][0] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[7][0];
		f.obbVertices[7][1] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[7][1];
		f.obbVertices[7][2] = labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue)[7][2];
		f.obbVolume = labelGeometryImageFilter->GetOrientedBoundingBoxVolume(labelValue);
		f.obbSize[0] = labelGeometryImageFilter->GetOrientedBoundingBoxSize(labelValue)[0];
		f.obbSize[1] = labelGeometryImageFilter->GetOrientedBoundingBoxSize(labelValue)[1];
		f.obbSize[2] = labelGeometryImageFilter->GetOrientedBoundingBoxSize(labelValue)[2];
		features.push_back(f);
	}

	std::ofstream file;
	file.open( getLocalEncodingFileName(outputImgPath).c_str() );
	for (auto f : features)
	{
		file << f;
	}
	file.close();

	std::cout << "Feature extracted finished\n";
}
