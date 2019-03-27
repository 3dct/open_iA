/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
