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
#include "iAConnectedComponentFilters.h"

#include <defines.h> // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>
#include <io/iAFileUtils.h>

#include <itkConnectedComponentImageFilter.h>
#include <itkScalarConnectedComponentImageFilter.h>
#include <itkRelabelComponentImageFilter.h>

#include <QFileDialog>
#include <QMessageBox>

template<class T> 
void SimpleConnectedComponentFilter(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<long, DIM>   OutputImageType;
	typedef itk::ConnectedComponentImageFilter< InputImageType, OutputImageType > CCIFType;
	typename CCIFType::Pointer ccFilter = CCIFType::New();
	ccFilter->SetInput( dynamic_cast< InputImageType * >(filter->Input()[0]->GetITKImage()) );
	ccFilter->SetBackgroundValue(0);
	ccFilter->SetFullyConnected(parameters["Fully Connected"].toBool());
	filter->Progress()->Observe(ccFilter);
	ccFilter->Update();
	filter->AddOutput(ccFilter->GetOutput());
}

void iASimpleConnectedComponents::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(SimpleConnectedComponentFilter, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iASimpleConnectedComponents)

iASimpleConnectedComponents::iASimpleConnectedComponents() :
	iAFilter("Connected Component Filter", "Connected Component Filters",
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
void ScalarConnectedComponentFilter(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<long, DIM>   OutputImageType;
	typedef itk::ScalarConnectedComponentImageFilter< InputImageType, OutputImageType > SCCIFType;
	typename SCCIFType::Pointer sccFilter = SCCIFType::New();
	sccFilter->SetInput( dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage()) );
	sccFilter->SetDistanceThreshold(parameters["Distance Threshold"].toDouble());
	filter->Progress()->Observe(sccFilter);
	sccFilter->Update();
	filter->AddOutput(sccFilter->GetOutput());
}

IAFILTER_CREATE(iAScalarConnectedComponents)

void iAScalarConnectedComponents::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(ScalarConnectedComponentFilter, InputPixelType(), this, parameters);
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
void SimpleRelabelComponentImageFilter(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<long, DIM>   OutputImageType;
	typedef itk::RelabelComponentImageFilter< InputImageType, OutputImageType > RCIFType;
	typename RCIFType::Pointer rccFilter = RCIFType::New();
	rccFilter->SetInput( dynamic_cast< InputImageType * >(filter->Input()[0]->GetITKImage()) );
	rccFilter->SetMinimumObjectSize(parameters["Minimum object size"].toInt());
	filter->Progress()->Observe(rccFilter);
	rccFilter->Update();
	if (parameters["Write labels to file"].toBool())
	{
		long int no_of_Objects = rccFilter->GetNumberOfObjects();
		ofstream myfile;
		myfile.open(getLocalEncodingFileName(parameters["Label file"].toString()));
		myfile << " Total Objects " << "," << no_of_Objects << endl;
		myfile << "Object Number" << "," << "Object Size (PhysicalUnits)" << endl;
		for ( int i = 0; i < no_of_Objects; i++ )
			myfile << i << "," << rccFilter->GetSizeOfObjectsInPhysicalUnits()[i] << endl;
		myfile.close();
	}
	filter->AddOutput(rccFilter->GetOutput());
}

IAFILTER_CREATE(iASimpleRelabelConnectedComponents)

void iASimpleRelabelConnectedComponents::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(SimpleRelabelComponentImageFilter, InputPixelType(), this, parameters);
}

iASimpleRelabelConnectedComponents::iASimpleRelabelConnectedComponents() :
	iAFilter("Relabel Components", "Connected Component Filters",
		"Remaps the labels associated with the objects in an image such that the "
		"label numbers are consecutive with no gaps.<br/>"
		"The input could for example be the output of the Simple Connected "
		"Component Filter. By default, the relabeling will also sort the labels "
		"based on the size of the object: the largest object will have label #1, "
		"the second largest will have label #2, etc. If two labels have the same "
		"size, their initial order is kept. Label #0 is assumed to be the "
		"background and is left unaltered by the relabeling.<br/>"
		"If the user sets a <em>Minimum object size</em>, all objects with fewer pixels than "
		"the minimum will be discarded, so that the number of objects reported "
		"will be only those remaining. Enabling the option <em>Write labels to file</em>"
		"will save details of each object to the file specified under <em>Label file.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1RelabelComponentImageFilter.html\">"
		"Relabel Component Filter</a> in the ITK documentation.")
{
	AddParameter("Minimum object size", Discrete, 1, 1);
	AddParameter("Write labels to file", Boolean, false);
	AddParameter("Label file", String /* should be FileName choice! */, "");
}
