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
#include "iAConnectedComponentFilters.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkConnectedComponentImageFilter.h>
#include <itkScalarConnectedComponentImageFilter.h>
#include <itkRelabelComponentImageFilter.h>

#include <vtkImageData.h>

#include <QLocale>

/**
* Simple connected component filter template initializes itk******* .
* \param	c		Switch on fully connected. 
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param	T		Template datatype. 
* \return	int		Status code 
*/
template<class T> 
int SimpleConnectedComponentFilter_template(int c, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< unsigned short, 3 >   OutputImageType;

	typedef itk::ConnectedComponentImageFilter< InputImageType, OutputImageType > CCIFType;
	typename CCIFType::Pointer filter = CCIFType::New();
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	filter->SetBackgroundValue(0);

	if ( c == 2 )
		filter->FullyConnectedOn();
	else
		filter->FullyConnectedOff();

	p->Observe( filter );

	filter->Update(); 

	image->SetImage(filter->GetOutput());
	image->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

/**
* Scalar connected component filter template initializes itk******* .
* \param	distTreshold	Distance treshold value.
* \param	p		Filter progress information.
* \param	image	Input image.
* \param	T		Template datatype.
* \return	int		Status code
*/
template<class T> 
int ScalarConnectedComponentFilter_template( double distTreshold, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< long, 3 >   OutputImageType;

	typedef itk::ScalarConnectedComponentImageFilter< InputImageType, OutputImageType > SCCIFType;
	typename SCCIFType::Pointer filter = SCCIFType::New();
	filter->SetInput( dynamic_cast<InputImageType *>(image->GetITKImage()) );
	filter->SetDistanceThreshold( distTreshold );

	p->Observe( filter );

	filter->Update();

	image->SetImage( filter->GetOutput() );
	image->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

/**
* Simple relabel component image filte template initializes itk******* .
* \param	w		The width. 
* \param	s		minimum object size. 
* \param	f		The format string. 
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param	T		Template datatype. 
* \return	int		Status code  
*/
template<class T> 
int SimpleRelabelComponentImageFilter_template( bool w, int s, QString f, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< long, 3 >   OutputImageType;

	typedef itk::RelabelComponentImageFilter< InputImageType, OutputImageType > RCIFType;
	typename RCIFType::Pointer filter = RCIFType::New();
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	filter->SetMinimumObjectSize( s );
	filter->SetInPlace(true);
	p->Observe( filter );
	filter->Update(); 

	if ( w )
	{
		long int no_of_Objects = filter->GetNumberOfObjects();

		//text file writer
		ofstream myfile;
		myfile.open(f.toStdString());
		myfile << " Total Objects " << "," << no_of_Objects << endl;
		myfile << "Object Number" << "," << "Object Size (PhysicalUnits)" << endl;

		for ( int i = 0; i < no_of_Objects; i++ )
			myfile << i << "," << filter->GetSizeOfObjectsInPhysicalUnits()[i] << endl;
		
		myfile.close();
	}

	image->SetImage(filter->GetOutput());
	image->Modified();

	filter->ReleaseDataFlagOn();
	return EXIT_SUCCESS;
}

iAConnectedComponentFilters::iAConnectedComponentFilters( QString fn, iAConnCompType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent )
	: iAAlgorithm( fn, i, p, logger, parent ), m_type(fid)
{}



void iAConnectedComponentFilters::run()
{
	switch (m_type)
	{
	case SIMPLE_CONNECTED_COMPONENT_FILTER: 
		SimpleConnectedComponentFilter(); break;
	case SCALAR_CONNECTED_COMPONENT_FILTER:
		ScalarConnectedComponentFilter(); break;
	case SIMPLE_RELABEL_COMPONENT_IMAGE_FILTER: 
		SimpleRelabelComponentImageFilter(); break;
	default:
		addMsg(tr("unknown filter type"));
	}
}


void iAConnectedComponentFilters::SimpleConnectedComponentFilter( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(SimpleConnectedComponentFilter_template, itkType,
			c, getItkProgress(), getConnector());
	}
	catch( itk::ExceptionObject &excep)
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


void iAConnectedComponentFilters::ScalarConnectedComponentFilter()
{
	addMsg( tr( "%1  %2 started." ).arg( QLocale().toString( Start(), QLocale::ShortFormat ) )
		.arg( getFilterName() ) );

	getConnector()->SetImage( getVtkImageData() ); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(ScalarConnectedComponentFilter_template, itkType,
			distTreshold, getItkProgress(), getConnector());
	}
	catch( itk::ExceptionObject &excep )
	{
		addMsg( tr( "%1  %2 terminated unexpectedly. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
			.arg( getFilterName() )
			.arg( Stop() ) );
		addMsg( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
			.arg( excep.GetFile() )
			.arg( excep.GetLine() ) );
		return;
	}
	addMsg( tr( "%1  %2 finished. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
		.arg( getFilterName() )
		.arg( Stop() ) );

	emit startUpdate();
}


void iAConnectedComponentFilters::SimpleRelabelComponentImageFilter( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(SimpleRelabelComponentImageFilter_template, itkType,
			w, s, f, getItkProgress(), getConnector());
	}
	catch( itk::ExceptionObject &excep)
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
