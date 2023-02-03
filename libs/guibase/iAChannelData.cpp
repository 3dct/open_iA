// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAChannelData.h"

#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>
#include <vtkScalarsToColors.h>

#include <QString>


iAChannelData::iAChannelData():
	m_enabled(false),
	m_opacity(1.0),
	m_threeD(false),
	m_cTF(nullptr),
	m_oTF(nullptr)
{}

iAChannelData::iAChannelData(QString const & name, vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf):
	m_enabled(false),
	m_opacity(1.0),
	m_threeD(false),
	m_image(image),
	m_cTF(ctf),
	m_oTF(otf),
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
