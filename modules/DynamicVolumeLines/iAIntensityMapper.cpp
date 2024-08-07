// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAIntensityMapper.h"
#include "iAITKIO.h"
#include "iATypedCallHelper.h"

#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageToVTKImageFilter.h>

#include <Hilbert.hpp>

#include <QDir>

template<class T>
void getIntensities(iAProgress &imp, PathID m_pathID, iAITKIO::ImagePointer &image, QList<icData> &intensityList,
	QList<vtkSmartPointer<vtkImageData>> &m_imgDataList, QList<double> &minEnsembleIntensityList,
	QList<double> &maxEnsembleIntensityList, QList<QVector<unsigned int>> &coordList)
{
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());
	typedef itk::ImageToVTKImageFilter<InputImageType> ITKTOVTKConverterType;
	auto itkToVTKConverter = ITKTOVTKConverterType::New();
	itkToVTKConverter->SetInput(input);
	itkToVTKConverter->Update();
	auto imageData = vtkSmartPointer<vtkImageData>::New();
	imageData->DeepCopy(itkToVTKConverter->GetOutput());
	minEnsembleIntensityList.append(imageData->GetScalarRange()[0]);
	maxEnsembleIntensityList.append(imageData->GetScalarRange()[1]);
	m_imgDataList.append(imageData);

	switch (m_pathID)
	{
		case P_HILBERT:
		{
			if (coordList.size() == 0)
			{
				QVector<unsigned int> coord(3);
				auto size = input->GetLargestPossibleRegion().GetSize();
				long long HilbertCnt = size[0] * size[1] * size[2];
				int nbOfBitsPerDim[DIM];
				for (int i = 0; i < DIM; ++i)
				{
					nbOfBitsPerDim[i] = std::ceil(std::sqrt((size[i] - 1)));
				}

				#pragma omp parallel for
				for (long long h = 0; h < HilbertCnt; ++h)
				{
					CFixBitVec *coordPtr = new CFixBitVec[DIM];
					CFixBitVec compHilbertIdx;
					compHilbertIdx = (FBV_UINT)h;
					Hilbert::compactIndexToCoords(coordPtr,
						nbOfBitsPerDim, DIM, compHilbertIdx);


					#pragma omp critical
					{
						for (int i = 0; i < DIM; i++)
						{
							coord[i] = static_cast<unsigned int>(coordPtr[i].rack());
						}

						delete[] coordPtr;
						coordList.append(coord);
						if (coordList.size() % 64 == 0)
						{
							imp.emitProgress(coordList.size() * 100.0 / HilbertCnt);
						}
					}
				}
			}

			for (int h = 0; h < coordList.size(); ++h)
			{
				typename InputImageType::IndexType c;
				for (int i = 0; i < DIM; i++)
				{
					c[i] = coordList[h][i];
				}
				icData data(input->GetPixel(c), c);
				intensityList.append(data);
			}
		}
		break;

		case P_SCAN_LINE:
		{
			itk::ImageRegionIteratorWithIndex<InputImageType> imageIterator(
				input, input->GetLargestPossibleRegion());
			for (imageIterator.GoToBegin(); !imageIterator.IsAtEnd(); ++imageIterator)
			{
				auto coord = imageIterator.GetIndex();
				icData data(input->GetPixel(coord), coord);
				intensityList.append(data);
			}
		}
		break;
	}
	itkToVTKConverter->ReleaseDataFlagOn();
}

iAIntensityMapper::iAIntensityMapper(iAProgress &iMProgress, QDir datasetsDir, PathID pathID, QList<QPair<QString,
	QList<icData>>> &datasetIntensityMap, QList<vtkSmartPointer<vtkImageData>> &imgDataList,
	double &minEnsembleIntensity, double &maxEnsembleIntensity) :
	m_iMProgress(iMProgress),
	m_datasetsDir(datasetsDir),
	m_pathID(pathID),
	m_DatasetIntensityMap(datasetIntensityMap),
	m_imgDataList(imgDataList),
	m_minEnsembleIntensity(minEnsembleIntensity),
	m_maxEnsembleIntensity(maxEnsembleIntensity)
{}

iAIntensityMapper::~iAIntensityMapper()
{}

void iAIntensityMapper::process()
{
	QStringList datasetsList = m_datasetsDir.entryList();
	QList<double> minEnsembleIntensityList;
	QList<double> maxEnsembleIntensityList;
	QList<QVector<unsigned int>> coordList;
	for (int i = 0; i < datasetsList.size(); ++i)
	{
		QList<icData> intensityList;
		QString dataset = m_datasetsDir.filePath(datasetsList.at(i));

		iAITKIO::ScalarType scalarType;
		iAITKIO::PixelType pixelType;
		auto image = iAITKIO::readFile(dataset, pixelType, scalarType, true);
		assert(pixelType == iAITKIO::PixelType::SCALAR);
		ITK_TYPED_CALL(getIntensities, scalarType,  m_iMProgress, m_pathID, image, intensityList,
			m_imgDataList, minEnsembleIntensityList, maxEnsembleIntensityList, coordList);
		m_DatasetIntensityMap.push_back(qMakePair(datasetsList.at(i), intensityList));
	}
	m_minEnsembleIntensity = *std::min_element(
		std::begin(minEnsembleIntensityList), std::end(minEnsembleIntensityList));
	m_maxEnsembleIntensity = *std::max_element(
		std::begin(maxEnsembleIntensityList), std::end(maxEnsembleIntensityList));
	emit finished();
}
