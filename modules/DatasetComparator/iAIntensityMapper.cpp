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
#include "dlg_DatasetComparator.h"
#include "iATypedCallHelper.h"

#include <itkImageBase.h>
#include <itkImage.h>
#include <itkImageIOBase.h>

typedef itk::ImageBase< DIM > ImageBaseType;
typedef ImageBaseType::Pointer ImagePointer;
typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
typedef itk::HilbertPath<unsigned int, DIM> PathType;

template<class T>
void getIntensities( PathType::Pointer path, ImagePointer & image, QList<int> & intensityList )
{
	// TODO: Typecheck QList for e.g., float images + check max size of list
	typedef itk::Image< T, DIM >   InputImageType;
	typedef PathType::IndexType IndexType;

	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());
	
	for (unsigned int h = 0; h < path->NumberOfSteps(); h++)
	{
		IndexType coord = path->Evaluate(h);
		intensityList.append(input->GetPixel(coord));
	}
}

iAIntensityMapper::iAIntensityMapper(dlg_DatasetComparator * dc)
{
	m_dc = dc;
}

iAIntensityMapper::~iAIntensityMapper()
{
}

void iAIntensityMapper::process()
{
	QStringList datsetsList = m_dc->m_datasetsDir.entryList();

	for (int i = 0; i < datsetsList.size(); ++i)
	{
		QList<int> intensityList;
		QString dataset = m_dc->m_datasetsDir.filePath(datsetsList.at(i));
		ScalarPixelType pixelType;
		ImagePointer image = iAITKIO::readFile( dataset, pixelType, true);
		try
		{
			ITK_TYPED_CALL(getIntensities, pixelType, m_dc->m_HPath, image, intensityList);
		}
		catch (itk::ExceptionObject &excep)
		{
			emit error("ITK exception"); // TODO: Better description
		}
		m_dc->m_DatasetIntensityMap.insert(datsetsList.at(i), intensityList);
	}
	emit finished();
}