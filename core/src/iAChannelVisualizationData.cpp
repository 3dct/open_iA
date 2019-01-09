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
#include "iAChannelVisualizationData.h"

#include <vtkActor.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageMapToColors.h>
#include <vtkImageReslice.h>
#include <vtkLookupTable.h>
#include <vtkMarchingContourFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkScalarsToColors.h>
#include <vtkTransform.h>
#include <vtkVersion.h>

#include <QObject>
#include <QString>
#include <QThread>
#include <QWidget>

#include <cassert>

iAChannelSlicerData::iAChannelSlicerData():
	image(NULL),
	reslicer(vtkImageReslice::New()),
	colormapper(vtkImageMapToColors::New()),
	imageActor(vtkImageActor::New()),
	m_isInitialized(false),
	m_lut( vtkSmartPointer<vtkLookupTable>::New() ),
	cFilter( vtkSmartPointer<vtkMarchingContourFilter>::New() ),
	cMapper( vtkSmartPointer<vtkPolyDataMapper>::New() ),
	cActor( vtkSmartPointer<vtkActor>::New() ),
	m_ctf(nullptr),
	m_otf(nullptr)
{}


iAChannelSlicerData::~iAChannelSlicerData()
{
	colormapper->Delete();
	imageActor->Delete();
	reslicer->ReleaseDataFlagOn();
	reslicer->Delete();
}


void iAChannelSlicerData::SetResliceAxesOrigin(double x, double y, double z)
{
	reslicer->SetResliceAxesOrigin(x, y, z);
	reslicer->Update();
	colormapper->Update();
	imageActor->SetInputData(colormapper->GetOutput());
	imageActor->GetMapper()->BorderOn();
}

void iAChannelSlicerData::Assign(vtkSmartPointer<vtkImageData> imageData, QColor const &col)
{
	color = col;
	image = imageData;
	reslicer->SetInputData(imageData);
	reslicer->SetInformationInput(imageData);
}


void iAChannelSlicerData::SetupOutput( vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf )
{
	m_ctf = ctf;
	m_otf = otf;
	UpdateLUT();
	colormapper->SetLookupTable( m_lut );//colormapper->SetLookupTable( ctf );
	colormapper->PassAlphaToOutputOn();
	colormapper->SetInputConnection(reslicer->GetOutputPort() );
	colormapper->Update();
	imageActor->SetInputData( colormapper->GetOutput() );
}

void iAChannelSlicerData::UpdateLUT()
{
	double rgb[3];
	double range[2]; image->GetScalarRange(range);
	m_lut->SetRange(range);
	const int numCols = 1024;
	m_lut->SetNumberOfTableValues(numCols);
	double scalVal = range[0];
	double scalValDelta = (range[1] - range[0]) / (numCols - 1);
	for (int i = 0; i < numCols; ++i)
	{
		m_ctf->GetColor(scalVal, rgb);
		double alpha = m_otf->GetValue(scalVal);
		m_lut->SetTableValue(i, rgb[0], rgb[1], rgb[2], alpha);
		scalVal += scalValDelta;
	}
	m_lut->Build();
}

void iAChannelSlicerData::Init(iAChannelVisualizationData * chData, int mode)
{
	m_isInitialized = true;
	Assign(chData->GetImage(), chData->GetColor());
	m_name = chData->GetName();

	reslicer->SetOutputDimensionality( 2 );
	reslicer->SetInterpolationModeToCubic();
	reslicer->InterpolateOn();
	reslicer->AutoCropOutputOn();
	reslicer->SetNumberOfThreads(QThread::idealThreadCount());
	UpdateResliceAxesDirectionCosines( mode );
	SetupOutput( chData->GetCTF(), chData->GetOTF());
	InitContours();
}

void iAChannelSlicerData::UpdateResliceAxesDirectionCosines( int mode )
{
	switch( mode )
	{
		case 0:
			reslicer->SetResliceAxesDirectionCosines( 0, 1, 0, 0, 0, 1, 1, 0, 0 );  //yz
			break;
		case 1:
			reslicer->SetResliceAxesDirectionCosines( 1, 0, 0, 0, 1, 0, 0, 0, 1 );  //xy
			break;
		case 2:
			reslicer->SetResliceAxesDirectionCosines( 1, 0, 0, 0, 0, 1, 0, -1, 0 ); //xz
			break;
		default:
			break;
	}
}

void iAChannelSlicerData::ReInit(iAChannelVisualizationData * chData)
{
	Assign(chData->GetImage(), chData->GetColor());
	m_name = chData->GetName();

	reslicer->UpdateWholeExtent();
	reslicer->Update();

	SetupOutput( chData->GetCTF(), chData->GetOTF() );
}

bool iAChannelSlicerData::isInitialized()
{
	return m_isInitialized;
}

vtkScalarsToColors* iAChannelSlicerData::GetLookupTable()
{
	return colormapper->GetLookupTable();
}

void iAChannelSlicerData::updateMapper()
{
	colormapper->Update();
}


QColor iAChannelSlicerData::GetColor() const
{
	return color;
}

void iAChannelSlicerData::assignTransform( vtkTransform * transform )
{
	reslicer->SetResliceTransform( transform );
}

void iAChannelSlicerData::updateReslicer()
{
	reslicer->Update();
}

void iAChannelSlicerData::SetContours( int num, const double * contourVals )
{
	cFilter->SetNumberOfContours( num );
	for( int i = 0; i < num; ++i )
		cFilter->SetValue( i, contourVals[i] );
	cFilter->Update();
}

void iAChannelSlicerData::InitContours()
{
	cFilter->SetInputConnection( reslicer->GetOutputPort() );
	cFilter->UseScalarTreeOn();
	cFilter->SetComputeGradients( false );
	cFilter->SetComputeNormals( false );
	cMapper->SetInputConnection( cFilter->GetOutputPort() );
	cMapper->SetResolveCoincidentTopology( VTK_RESOLVE_POLYGON_OFFSET );
	cMapper->ScalarVisibilityOff();
	cActor->SetMapper( cMapper );
	cActor->SetVisibility( false );
}

void iAChannelSlicerData::SetContoursColor( double * rgb )
{
	cActor->GetProperty()->SetColor( rgb[0], rgb[1], rgb[2] );
}

void iAChannelSlicerData::SetShowContours( bool show )
{
	cActor->SetVisibility( show );
}

void iAChannelSlicerData::SetContourLineParams( double lineWidth, bool dashed)
{
	cActor->GetProperty()->SetLineWidth( lineWidth );
	if( dashed )
		cActor->GetProperty()->SetLineStipplePattern( 0xff00 );
}

void iAChannelSlicerData::SetContoursOpacity( double opacity )
{
	cActor->GetProperty()->SetOpacity( opacity );
}


QString iAChannelSlicerData::GetName() const
{
	return m_name;
}



iAChannelVisualizationData::iAChannelVisualizationData():
	piecewiseFunction(NULL),
	colorTransferFunction(NULL),
	enabled(false),
	opacity(1.0),
	threeD(false),
	similarityRenderingEnabled(false)
{}

iAChannelVisualizationData::~iAChannelVisualizationData()
{
	Reset();
}

void iAChannelVisualizationData::Reset()
{
	enabled = false;
}

bool iAChannelVisualizationData::IsEnabled() const
{
	return enabled;
}

bool iAChannelVisualizationData::Uses3D() const
{
	return threeD;
}

void iAChannelVisualizationData::Set3D(bool enabled)
{
	threeD = enabled;
}

void iAChannelVisualizationData::SetEnabled(bool enabled)
{
	this->enabled = enabled;
}

void iAChannelVisualizationData::SetOpacity(double opacity)
{
	this->opacity = opacity;
}

double iAChannelVisualizationData::GetOpacity() const
{
	return opacity;
}

void iAChannelVisualizationData::SetImage( vtkSmartPointer<vtkImageData> img )
{
	image = img;
}

void iAChannelVisualizationData::SetColorTF( vtkScalarsToColors* cTF )
{
	colorTransferFunction = cTF;
}

void iAChannelVisualizationData::SetOpacityTF(vtkPiecewiseFunction* oTF)
{
	piecewiseFunction = oTF;
}


void iAChannelVisualizationData::SetName(QString name)
{
	m_name = name;
}

void iAChannelVisualizationData::SetColor(QColor const & col)
{
	color = col;
}

QColor iAChannelVisualizationData::GetColor() const
{
	return color;
}

bool iAChannelVisualizationData::IsSimilarityRenderingEnabled() const
{
	return similarityRenderingEnabled;
}

void iAChannelVisualizationData::SetSimilarityRenderingEnabled(bool enabled)
{
	similarityRenderingEnabled = enabled;
}

vtkPiecewiseFunction * iAChannelVisualizationData::GetOTF()
{
	return piecewiseFunction;
}

vtkScalarsToColors * iAChannelVisualizationData::GetCTF()
{
	return colorTransferFunction;
}

vtkSmartPointer<vtkImageData> iAChannelVisualizationData::GetImage()
{
	return image;
}

void ResetChannel(iAChannelVisualizationData* chData, vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	chData->SetImage(image);
	chData->SetColorTF(ctf);
	chData->SetOpacityTF(otf);
}

QString iAChannelVisualizationData::GetName() const
{
	return m_name;
}
