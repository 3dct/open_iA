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
#include "iASimpleFusion.h"
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkAddImageFilter.h>

#include <QLocale>

/**
* Fuses two images using  itk::AddImageFilter.
* \param	p				If non-null, the.
* \param [in,out]	image2	If non-null, the second image.
* \param	image			If non-null, the image.
* \param					The.
* \return	int Status-Code.
*/
template<class T> 
int addImages_template(iAProgress* p, iAConnector* image2, iAConnector* image)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< T, 3 >   OutputImageType;
	typedef itk::AddImageFilter< InputImageType, InputImageType, OutputImageType> AddImageFilter;

	typename AddImageFilter::Pointer fusion = AddImageFilter::New();
	fusion->InPlaceOff();
	//fusion->ReleaseDataFlagOn();
	InputImageType * input1 = dynamic_cast<InputImageType *>(image->GetITKImage());
	InputImageType * input2 = dynamic_cast<InputImageType *>(image2->GetITKImage());
	fusion->SetInput1(input1);
	fusion->SetInput2(input2);

	p->Observe(fusion);
	fusion->Update();

	image->SetImage(fusion->GetOutput());
	image->Modified();

	return EXIT_SUCCESS;
}

iASimpleFusion::iASimpleFusion( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent  )
	: iAAlgorithm( fn, fid, i, p, logger, parent )
{}

void iASimpleFusion::run()
{

	switch (getFilterID())
	{
	case ADD_IMAGES_FUSION:
		addImagesFusion(); break;
	default:
		addMsg(tr("  unknown filter type"));
	}
}

void iASimpleFusion::addImagesFusion()
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData());
	getConnector()->Modified();
	getFixedConnector()->SetImage(image2);
	getFixedConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(addImages_template, itkType,
			getItkProgress(), getFixedConnector(), getConnector());
	}
	catch (itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())
		.arg(Stop()));

	emit startUpdate();
}
