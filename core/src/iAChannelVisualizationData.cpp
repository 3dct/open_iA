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
	m_piecewiseFunction(nullptr),
	m_colorTransferFunction(nullptr),
	m_enabled(false),
	m_opacity(1.0),
	m_threeD(false),
	m_similarityRenderingEnabled(false)
{}

iAChannelVisualizationData::~iAChannelVisualizationData()
{
	reset();
}

void iAChannelVisualizationData::reset()
{
	m_enabled = false;
}

void iAChannelVisualizationData::setData(vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	setImage(image);
	setColorTF(ctf);
	setOpacityTF(otf);
}

bool iAChannelVisualizationData::isEnabled() const
{
	return m_enabled;
}

bool iAChannelVisualizationData::uses3D() const
{
	return m_threeD;
}

void iAChannelVisualizationData::set3D(bool enabled)
{
	m_threeD = enabled;
}

void iAChannelVisualizationData::setEnabled(bool enabled)
{
	m_enabled = enabled;
}

void iAChannelVisualizationData::setOpacity(double opacity)
{
	m_opacity = opacity;
}

double iAChannelVisualizationData::getOpacity() const
{
	return m_opacity;
}

void iAChannelVisualizationData::setImage( vtkSmartPointer<vtkImageData> img )
{
	m_image = img;
}

void iAChannelVisualizationData::setColorTF( vtkScalarsToColors* cTF )
{
	m_colorTransferFunction = cTF;
}

void iAChannelVisualizationData::setOpacityTF(vtkPiecewiseFunction* oTF)
{
	m_piecewiseFunction = oTF;
}


void iAChannelVisualizationData::setName(QString name)
{
	m_name = name;
}

QString iAChannelVisualizationData::getName() const
{
	return m_name;
}

void iAChannelVisualizationData::setColor(QColor const & col)
{
	m_color = col;
}

QColor iAChannelVisualizationData::getColor() const
{
	return m_color;
}

bool iAChannelVisualizationData::isSimilarityRenderingEnabled() const
{
	return m_similarityRenderingEnabled;
}

void iAChannelVisualizationData::setSimilarityRenderingEnabled(bool enabled)
{
	m_similarityRenderingEnabled = enabled;
}

vtkPiecewiseFunction * iAChannelVisualizationData::getOTF()
{
	return m_piecewiseFunction;
}

vtkScalarsToColors * iAChannelVisualizationData::getCTF()
{
	return m_colorTransferFunction;
}

vtkSmartPointer<vtkImageData> iAChannelVisualizationData::getImage()
{
	return m_image;
}
