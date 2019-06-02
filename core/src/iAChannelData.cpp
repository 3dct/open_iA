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
#include "iAChannelData.h"

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



iAChannelData::iAChannelData():
	m_oTF(nullptr),
	m_cTF(nullptr),
	m_enabled(false),
	m_opacity(1.0),
	m_threeD(false),
	m_similarityRenderingEnabled(false)
{}

iAChannelData::iAChannelData(QString const & name, vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf):
	m_image(image), m_cTF(ctf), m_oTF(otf),
	m_name(name)
{}

iAChannelData::~iAChannelData()
{
	reset();
}

void iAChannelData::reset()
{
	m_enabled = false;
}

void iAChannelData::setData(vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	setImage(image);
	setColorTF(ctf);
	setOpacityTF(otf);
}

bool iAChannelData::isEnabled() const
{
	return m_enabled;
}

bool iAChannelData::uses3D() const
{
	return m_threeD;
}

void iAChannelData::set3D(bool enabled)
{
	m_threeD = enabled;
}

void iAChannelData::setEnabled(bool enabled)
{
	m_enabled = enabled;
}

void iAChannelData::setOpacity(double opacity)
{
	m_opacity = opacity;
}

double iAChannelData::opacity() const
{
	return m_opacity;
}

void iAChannelData::setImage( vtkSmartPointer<vtkImageData> img )
{
	m_image = img;
}

void iAChannelData::setColorTF( vtkScalarsToColors* cTF )
{
	m_cTF = cTF;
}

void iAChannelData::setOpacityTF(vtkPiecewiseFunction* oTF)
{
	m_oTF = oTF;
}

void iAChannelData::setName(QString name)
{
	m_name = name;
}

QString const & iAChannelData::name() const
{
	return m_name;
}

vtkPiecewiseFunction * iAChannelData::opacityTF() const
{
	return m_oTF;
}

vtkScalarsToColors * iAChannelData::colorTF() const
{
	return m_cTF;
}

vtkSmartPointer<vtkImageData> iAChannelData::image() const
{
	return m_image;
}

bool iAChannelData::isSimilarityRenderingEnabled() const
{
	return m_similarityRenderingEnabled;
}

void iAChannelData::setSimilarityRenderingEnabled(bool enabled)
{
	m_similarityRenderingEnabled = enabled;
}
