/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
 
#include "iAIntensityMapper.h"
#include "io/iAITKIO.h"
#include "iATypedCallHelper.h"

#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageToVTKImageFilter.h>

#include <Hilbert.hpp>

//
#include "iAConsole.h"

template<class T>
void getIntensities(PathID m_pathID, ImagePointer &image, QList<icData> &intensityList, 
	QList<vtkSmartPointer<vtkImageData>> &m_imgDataList, QList<double> &minEnsembleIntensityList, 
	QList<double> &maxEnsembleIntensityList)
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
			auto size = input->GetLargestPossibleRegion().GetSize();
			unsigned int HilbertCnt = size[0] * size[1] * size[2];
			int precArray[DIM] = { static_cast<int>(size[0])-1,
				static_cast<int>(size[1])-1, static_cast<int>(size[2])-1 };
			for (unsigned int h = 0; h < HilbertCnt; ++h)
			{
				CFixBitVec *coordPtr = new CFixBitVec[HilbertCnt];
				CFixBitVec compHilbertIdx;
				compHilbertIdx = (FBV_UINT)h;
				Hilbert::compactIndexToCoords(coordPtr, precArray, DIM, compHilbertIdx);

				InputImageType::IndexType coord;
				for (int i = 0; i < DIM; i++)
					coord[i] = coordPtr[i].rack();
				delete[] coordPtr;

				icData data(input->GetPixel(coord), coord);
				intensityList.append(data);
				//DEBUG_LOG(QString("Hidx: %1: x:%2 y:%3 z:%4 int:%5").arg(h).arg(coord[0]).arg(coord[1]).arg(coord[2]).arg(input->GetPixel(coord)));
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

iAIntensityMapper::iAIntensityMapper(QDir datasetsDir, PathID pathID, QList<QPair<QString,
	QList<icData>>> &datasetIntensityMap, QList<vtkSmartPointer<vtkImageData>> &imgDataList,
	double &minEnsembleIntensity, double &maxEnsembleIntensity) :
	m_DatasetIntensityMap(datasetIntensityMap),
	m_datasetsDir(datasetsDir),
	m_pathID(pathID),
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
	for (int i = 0; i < datasetsList.size(); ++i)
	{
		QList<icData> intensityList;
		QString dataset = m_datasetsDir.filePath(datasetsList.at(i));
		ScalarPixelType pixelType;
		ImagePointer image = iAITKIO::readFile(dataset, pixelType, true);
		ITK_TYPED_CALL(getIntensities, pixelType, m_pathID, image, intensityList,
			m_imgDataList, minEnsembleIntensityList, maxEnsembleIntensityList);
		m_DatasetIntensityMap.push_back(qMakePair(datasetsList.at(i), intensityList));
	}
	m_minEnsembleIntensity = *std::min_element(
		std::begin(minEnsembleIntensityList), std::end(minEnsembleIntensityList));
	m_maxEnsembleIntensity = *std::max_element(
		std::begin(maxEnsembleIntensityList), std::end(maxEnsembleIntensityList));
	emit finished();
}