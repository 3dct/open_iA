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

#include "iAIntensityMapper.h"
#include "io/iAITKIO.h"
#include "iATypedCallHelper.h"

#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageToVTKImageFilter.h>

#include <Hilbert.hpp>


template<class T>
void getIntensities(iAProgress &imp, PathID m_pathID, ImagePointer &image, QList<icData> &intensityList,
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
				QVector<unsigned int> coord(QVector<unsigned int>(3));
				auto size = input->GetLargestPossibleRegion().GetSize();
				unsigned int HilbertCnt = size[0] * size[1] * size[2];
				int nbOfBitsPerDim[DIM];
				for (int i = 0; i < DIM; ++i)
					nbOfBitsPerDim[i] = ceil(sqrt((size[i] - 1)));

				for (unsigned int h = 0; h < HilbertCnt; ++h)
				{
					CFixBitVec *coordPtr = new CFixBitVec[HilbertCnt];
					CFixBitVec compHilbertIdx;
					compHilbertIdx = (FBV_UINT)h;
					Hilbert::compactIndexToCoords(coordPtr,
						nbOfBitsPerDim, DIM, compHilbertIdx);

					for (int i = 0; i < DIM; i++)
						coord[i] = coordPtr[i].rack();

					delete[] coordPtr;
					coordList.append(coord);
					imp.emitProgress((h + 1) * 100 / HilbertCnt);
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
		ScalarPixelType pixelType;
		ImagePointer image = iAITKIO::readFile(dataset, pixelType, true);
		ITK_TYPED_CALL(getIntensities, pixelType, m_iMProgress, m_pathID, image, intensityList,
			m_imgDataList, minEnsembleIntensityList, maxEnsembleIntensityList, coordList);
		m_DatasetIntensityMap.push_back(qMakePair(datasetsList.at(i), intensityList));
	}
	m_minEnsembleIntensity = *std::min_element(
		std::begin(minEnsembleIntensityList), std::end(minEnsembleIntensityList));
	m_maxEnsembleIntensity = *std::max_element(
		std::begin(maxEnsembleIntensityList), std::end(maxEnsembleIntensityList));
	emit finished();
}
