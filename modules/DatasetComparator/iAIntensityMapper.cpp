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
#include "iAIntensityMapper.h"
#include "iAITKIO.h"
#include "iATypedCallHelper.h"

#include <itkHilbertPath.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageToVTKImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>

//#include "iAConsole.h"

template<class T>
void getIntensities(PathID m_pathID, ImagePointer &image, QList<icData> &intensityList, 
	QList<vtkSmartPointer<vtkImageData>> &m_imgDataList, QList<double> &minEnsembleIntensityList, 
	QList<double> &maxEnsembleIntensityList)
{
	// TODO: Typecheck QList for e.g., float images + cubic region only check
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	typedef itk::MinimumMaximumImageCalculator< InputImageType >  MinMaxCalcType;
	typename MinMaxCalcType::Pointer minMaxCalc = MinMaxCalcType::New();
	minMaxCalc->SetImage(input);
	minMaxCalc->Compute();
	minEnsembleIntensityList.append(minMaxCalc->GetMinimum());
	maxEnsembleIntensityList.append(minMaxCalc->GetMaximum());

	typedef itk::ImageToVTKImageFilter<InputImageType> ITKTOVTKConverterType;
	ITKTOVTKConverterType::Pointer itkToVTKConverter = ITKTOVTKConverterType::New();
	itkToVTKConverter->SetInput(input);
	itkToVTKConverter->Update();
	auto imageData = vtkSmartPointer<vtkImageData>::New();
	imageData->DeepCopy(itkToVTKConverter->GetOutput());
	m_imgDataList.append(imageData);

	switch (m_pathID)
	{
		case P_HILBERT:
		{
			typedef itk::HilbertPath<unsigned int, DIM> PathType;
			PathType::Pointer m_HPath = PathType::New();
			typedef PathType::IndexType IndexType;
			InputImageType::RegionType region = input->GetLargestPossibleRegion();
			InputImageType::SizeType size = region.GetSize();

			unsigned int order = log(size[0]) / log(2);
			m_HPath->SetHilbertOrder(order);
			m_HPath->Initialize();
			unsigned int h = 0;
			for (; h < m_HPath->NumberOfSteps(); h++)
			{
					IndexType coord = m_HPath->Evaluate(h);
					icData data;
					data.intensity = input->GetPixel(coord);
					data.x = coord[0];
					data.y = coord[1];
					data.z = coord[2];
					intensityList.append(data);
					//DEBUG_LOG(QString("Mapping %1 of %2 done").arg(h).arg(m_HPath->NumberOfSteps()));
			}
		}
		break;

		case P_SCAN_LINE:
		{
			itk::ImageRegionIteratorWithIndex<InputImageType> imageIterator(
				input, input->GetLargestPossibleRegion());
			for (imageIterator.GoToBegin(); !imageIterator.IsAtEnd(); ++imageIterator)
			{
				InputImageType::IndexType coord = imageIterator.GetIndex();
				icData data;
				data.intensity = input->GetPixel(coord);
				data.x = coord[0];
				data.y = coord[1];
				data.z = coord[2];
				intensityList.append(data);
			}
		}
		break;
	}
	itkToVTKConverter->ReleaseDataFlagOn();
}

iAIntensityMapper::iAIntensityMapper(QDir datasetsDir, PathID pathID, QList<QPair<QString,
	QList<icData>>> &datasetIntensityMap, QList<vtkSmartPointer<vtkImageData>> &imgDataList, 
	double &minEnsembleIntensity, double &maxEnsembleIntensity):
	m_DatasetIntensityMap(datasetIntensityMap),
	m_datasetsDir(datasetsDir),
	m_pathID(pathID),
	m_imgDataList(imgDataList),
	m_minEnsembleIntensity(minEnsembleIntensity),
	m_maxEnsembleIntensity(maxEnsembleIntensity)

{
}

iAIntensityMapper::~iAIntensityMapper()
{
}

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
		ImagePointer image = iAITKIO::readFile( dataset, pixelType, true);
		try
		{
			ITK_TYPED_CALL(getIntensities, pixelType, m_pathID, image, intensityList,
				m_imgDataList, minEnsembleIntensityList, maxEnsembleIntensityList);
		}
		catch (itk::ExceptionObject &excep)
		{
			emit error("ITK exception"); // TODO: Better description
		}
		m_DatasetIntensityMap.push_back(qMakePair(datasetsList.at(i), intensityList));
	}
	m_minEnsembleIntensity = *std::min_element(
		std::begin(minEnsembleIntensityList), std::end(minEnsembleIntensityList));
	m_maxEnsembleIntensity = *std::max_element(
		std::begin(maxEnsembleIntensityList), std::end(maxEnsembleIntensityList));
	emit finished();
}