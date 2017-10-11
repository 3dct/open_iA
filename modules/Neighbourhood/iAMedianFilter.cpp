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
#include "iAMedianFilter.h"

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkMedianImageFilter.h>
#include <itkCastImageFilter.h>

template<class T> void median_template( unsigned int r_x, unsigned int r_y, unsigned int r_z, iAProgress* p, iAConnector* image  )
{
	typedef itk::Image< T, DIM>   InputImageType;
	typedef itk::Image< float, DIM >   RealImageType;
	typedef itk::CastImageFilter< InputImageType, RealImageType> CastToRealFilterType;
	typedef itk::MedianImageFilter<RealImageType, RealImageType > FilterType;
	
	auto toReal = CastToRealFilterType::New();
	toReal->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	auto filter = FilterType::New();
	FilterType::InputSizeType indexRadius;
	indexRadius[0] = r_x; 
	indexRadius[1] = r_y; 
	indexRadius[2] = r_z;
	filter->SetRadius(indexRadius);
	filter->SetInput( toReal->GetOutput() );
	p->Observe( filter );
	filter->Update(); 
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

void iAMedianFilter::Run(QMap<QString, QVariant> parameters)
{
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(median_template, itkType,
		parameters["Kernel Radius X"].toUInt(),
		parameters["Kernel Radius Y"].toUInt(),
		parameters["Kernel Radius Z"].toUInt(), m_progress, m_con);
}

IAFILTER_CREATE(iAMedianFilter)

iAMedianFilter::iAMedianFilter() :
	iAFilter("Median Filter", "Neighbourhood",
		"Applies a median filter to the volume.<br/>"
		"Computes an image where an output voxel is assigned the median value of the voxels "
		"in a neighborhood around the input voxel at that position. The median filter belongs "
		"to the family of nonlinear filters. It is used to smooth an image without being "
		"biased by outliers or shot noise.<br/>"
		"The parameters define the radius of the kernel x,y and z direction.<br/>"
		"For more information, see the "
		"<a href=\"http://www.itk.org/Doxygen/html/classitk_1_1MedianImageFilter.html\">"
		"Median Image Filter</a> in the ITK documentation.")
{
	AddParameter("Kernel Radius X", Discrete, 1, 1);
	AddParameter("Kernel Radius Y", Discrete, 1, 1);
	AddParameter("Kernel Radius Z", Discrete, 1, 1);
}
