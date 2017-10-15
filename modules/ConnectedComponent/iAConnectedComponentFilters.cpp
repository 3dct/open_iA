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
#include "iAConnectedComponentFilters.h"

#include "iAConnector.h"
#include "defines.h" // for DIM
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkConnectedComponentImageFilter.h>
#include <itkScalarConnectedComponentImageFilter.h>
#include <itkRelabelComponentImageFilter.h>

#include <QFileDialog>
#include <QMessageBox>

template<class T> 
void SimpleConnectedComponentFilter_template(bool fullyConnected, iAProgress* p, iAConnector* image )
{
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<long, DIM>   OutputImageType;
	typedef itk::ConnectedComponentImageFilter< InputImageType, OutputImageType > CCIFType;
	typename CCIFType::Pointer filter = CCIFType::New();
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	filter->SetBackgroundValue(0);
	filter->SetFullyConnected(fullyConnected);
	p->Observe( filter );
	filter->Update(); 
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iASimpleConnectedComponents)

void iASimpleConnectedComponents::Run(QMap<QString, QVariant> parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(SimpleConnectedComponentFilter_template, pixelType,
		parameters["Fully Connected"].toBool(),
		m_progress, m_con);
}

iASimpleConnectedComponents::iASimpleConnectedComponents() :
	iAFilter("Simple Connected Component Filter", "Connected Component Filters",
		"Assigns each distinct object in a binary image a unique label.<br/>"
		"Non-zero pixels are considered to be objects, zero-valued pixels are "
		"considered to be background).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConnectedComponentImageFilter.html\">"
		"Connected Component Filter</a> in the ITK documentation.")
{
	AddParameter("Fully Connected", Boolean, false);
}


template<class T> 
void ScalarConnectedComponentFilter_template( double distTreshold, iAProgress* p, iAConnector* image )
{
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<long, DIM>   OutputImageType;
	typedef itk::ScalarConnectedComponentImageFilter< InputImageType, OutputImageType > SCCIFType;
	typename SCCIFType::Pointer filter = SCCIFType::New();
	filter->SetInput( dynamic_cast<InputImageType *>(image->GetITKImage()) );
	filter->SetDistanceThreshold( distTreshold );
	p->Observe( filter );
	filter->Update();
	image->SetImage( filter->GetOutput() );
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iAScalarConnectedComponents)

void iAScalarConnectedComponents::Run(QMap<QString, QVariant> parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(ScalarConnectedComponentFilter_template, pixelType,
		parameters["Distance Threshold"].toDouble(),
		m_progress, m_con);
}

iAScalarConnectedComponents::iAScalarConnectedComponents() :
	iAFilter("Scalar Connected Component Filter", "Connected Component Filters",
		"Labels the objects in an arbitrary image.<br/>"
		"Two pixels are similar if they are within <em>Distance Threshold</em> of each other.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ScalarConnectedComponentImageFilter.html\">"
		"Scalar Connected Component Filter</a> in the ITK documentation.")
{
	AddParameter("Distance Threshold", Continuous, 1);
}


template<class T> 
void SimpleRelabelComponentImageFilter_template( bool w, int s, QString f, iAProgress* p, iAConnector* image )
{
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<long, DIM>   OutputImageType;
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
}

IAFILTER_CREATE(iASimpleRelabelConnectedComponents)

void iASimpleRelabelConnectedComponents::Run(QMap<QString, QVariant> parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(SimpleRelabelComponentImageFilter_template, pixelType,
		parameters["Write labels to file"].toBool(),
		parameters["Minimum Object Size"].toInt(),
		m_outFile,
		m_progress, m_con);
}

bool iASimpleRelabelConnectedComponents::CheckParameters(QMap<QString, QVariant> parameters)
{
	if (parameters["Write labels to file"].toBool())
	{
		m_outFile = QFileDialog::getSaveFileName(0, "Save file", 0, "txt Files (*.txt *.TXT)");
		if (m_outFile.isEmpty())
		{
			QMessageBox msgBox;
			msgBox.setText("No destination file was specified!");
			msgBox.setWindowTitle(Name());
			msgBox.exec();
			return false;
		}
	}
	return iAFilter::CheckParameters(parameters);
}

iASimpleRelabelConnectedComponents::iASimpleRelabelConnectedComponents() :
	iAFilter("Simple Relabel Connected Component Filter", "Connected Component Filters",
		"Remaps the labels associated with the objects in an image such that the "
		"label numbers are consecutive with no gaps.<br/>"
		"The input could for example be the output of the Simple Connected "
		"Component Filter. By default, the relabeling will also sort the labels "
		"based on the size of the object: the largest object will have label #1, "
		"the second largest will have label #2, etc. If two labels have the same "
		"size their initial order is kept. Label #0 is assumed to be the "
		"background and is left unaltered by the relabeling.<br/>"
		"If user sets a minimum object size, all objects with fewer pixels than "
		"the minimum will be discarded, so that the number of objects reported "
		"will be only those remaining. Enabling the write option will save details "
		"of each object to a user specified file path.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1RelabelComponentImageFilter.html\">"
		"Relabel Component Filter</a> in the ITK documentation.")
{
	AddParameter("Minimum Object Size", Discrete, 1, 1);
	AddParameter("Write labels to file", Boolean, false);
}
