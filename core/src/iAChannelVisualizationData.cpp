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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
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
	activeImage(NULL),
	reslicer(vtkImageReslice::New()),
	colormapper(vtkImageMapToColors::New()),
	imageActor(vtkImageActor::New()),
	m_isInitialized(false),
	m_lut( vtkSmartPointer<vtkLookupTable>::New() ),
	cFilter( vtkSmartPointer<vtkMarchingContourFilter>::New() ),
	cMapper( vtkSmartPointer<vtkPolyDataMapper>::New() ),
	cActor( vtkSmartPointer<vtkActor>::New() )
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
/*
	// ORIENTATION / ROTATION FIX:
	// make orientation the same as in other image viewers:
	double orientation[3] = {
		180,
		0,
		0
	};
	imageActor->SetOrientation(orientation);
*/
}

void iAChannelSlicerData::Assign(vtkSmartPointer<vtkImageData> imageData, QColor const &col)
{
	color = col;
	activeImage = imageData;
	reslicer->SetInputData(imageData);
	reslicer->SetInformationInput(imageData);
}


void iAChannelSlicerData::SetupOutput( vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf )
{
	double rgb[3];
	double range[2]; activeImage->GetScalarRange( range );
	m_lut->SetRange( range );
	const int numCols = 1024;
	m_lut->SetNumberOfColors( numCols );
	double alpha;// = otf->GetValue( activeImage->GetScalarRange()[0] );
	//double deltaAlpha = (otf->GetValue( activeImage->GetScalarRange()[1] ) - alpha) / (numCols - 1);
	double scalVal = range[0];
	double scalValDelta = (range[1] - range[0]) / (numCols - 1);
	for( int i = 0; i < numCols; ++i )
	{
		ctf->GetColor( scalVal, rgb );
		alpha = otf->GetValue( scalVal );
		m_lut->SetTableValue( i, rgb[0], rgb[1], rgb[2], alpha );//m_lut->SetTableValue( i, rgb[0], rgb[1], rgb[2], 1 );
		//alpha += deltaAlpha;
		scalVal += scalValDelta;
	}
	m_lut->Build();
	
	colormapper->SetLookupTable( m_lut );//colormapper->SetLookupTable( ctf );
	colormapper->PassAlphaToOutputOn();
	colormapper->SetInputConnection(reslicer->GetOutputPort() );
	colormapper->Update();
	imageActor->SetInputData( colormapper->GetOutput() );
}

void iAChannelSlicerData::Init(iAChannelVisualizationData * chData, int mode)
{
	m_isInitialized = true;
	Assign(chData->GetActiveImage(), chData->GetColor());

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
	Assign(chData->GetActiveImage(), chData->GetColor());

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

void iAChannelSlicerData::SetContourLineParams( double lineWidth, bool dashed /*= false */ )
{
	cActor->GetProperty()->SetLineWidth( lineWidth );
	if( dashed )
		cActor->GetProperty()->SetLineStipplePattern( 0xff00 );
}

void iAChannelSlicerData::SetContoursOpacity( double opacity )
{
	cActor->GetProperty()->SetOpacity( opacity );
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

void iAChannelVisualizationData::SetActiveImage( vtkSmartPointer<vtkImageData> image )
{
	activeImage = image;
}

void iAChannelVisualizationData::SetColorTF( vtkScalarsToColors* cTF )
{
	colorTransferFunction = cTF;
}

void iAChannelVisualizationData::SetOpacityTF(vtkPiecewiseFunction* oTF)
{
	piecewiseFunction = oTF;
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

vtkSmartPointer<vtkImageData> iAChannelVisualizationData::GetActiveImage()
{
	return activeImage;
}

void ResetChannel(iAChannelVisualizationData* chData, vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	chData->SetActiveImage(image);
	chData->SetColorTF(ctf);
	chData->SetOpacityTF(otf);
}
