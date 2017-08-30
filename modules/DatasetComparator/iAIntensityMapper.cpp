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

#include "defines.h"
#include "iAITKIO.h"
#include "iATypedCallHelper.h"

#include <itkImageBase.h>
#include <itkImage.h>
#include <itkImageIOBase.h>
#include <itkHilbertPath.h>
#include <itkImageRegionIteratorWithIndex.h>

#include <math.h>

#include "iAConsole.h"

typedef itk::ImageBase< DIM > ImageBaseType;
typedef ImageBaseType::Pointer ImagePointer;
typedef itk::ImageIOBase::IOComponentType ScalarPixelType;

template<class T>
void getIntensities(PathID m_pathID, ImagePointer &image, QList<icData> &intensityList )
{
	// TODO: Typecheck QList for e.g., float images + check max size of list
	typedef itk::Image< T, DIM >   InputImageType;

	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	switch (m_pathID)
	{
		case P_HILBERT:
		{
			typedef itk::HilbertPath<unsigned int, DIM> PathType;
			PathType::Pointer m_HPath = PathType::New();
			typedef PathType::IndexType IndexType;
			InputImageType::RegionType region = input->GetLargestPossibleRegion();
			InputImageType::SizeType size = region.GetSize();

			m_HPath->SetHilbertOrder((int) (log(size[0]) / log(2)));
			DEBUG_LOG(QString("HPath started with Order %1").arg((int) (log(size[0]) / log(2))));
			m_HPath->Initialize();
			DEBUG_LOG(QString("HPath initialized"));
			for (unsigned int h = 0; h < m_HPath->NumberOfSteps(); h++)
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
			itk::ImageRegionIteratorWithIndex<InputImageType> imageIterator(input, input->GetLargestPossibleRegion());
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
}

iAIntensityMapper::iAIntensityMapper(QDir datasetsDir, PathID pathID, QMap<QString, QList<icData> > &datasetIntensityMap):
	m_DatasetIntensityMap(datasetIntensityMap),
	m_datasetsDir(datasetsDir),
	m_pathID(pathID)
{
}

iAIntensityMapper::~iAIntensityMapper()
{
}

void iAIntensityMapper::process()
{
	QStringList datasetsList = m_datasetsDir.entryList();

	for (int i = 0; i < datasetsList.size(); ++i)
	{
		QList<icData> intensityList;
		QString dataset = m_datasetsDir.filePath(datasetsList.at(i));
		ScalarPixelType pixelType;
		ImagePointer image = iAITKIO::readFile( dataset, pixelType, true);
		try
		{
			ITK_TYPED_CALL(getIntensities, pixelType, m_pathID, image, intensityList);
		}
		catch (itk::ExceptionObject &excep)
		{
			emit error("ITK exception"); // TODO: Better description
		}
		m_DatasetIntensityMap.insert(datasetsList.at(i), intensityList);
	}
	emit finished();
}