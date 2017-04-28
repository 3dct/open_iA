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
#include "iAModality.h"

#include "iAImageCoordinate.h"
#include "iAModalityTransfer.h"
#include "iASettings.h"
#include "iAStringHelper.h" // for Str2Vec3D
#include "iAVolumeRenderer.h"

#include <vtkImageData.h>
#include <vtkVolume.h>

#include <cassert>

iAModality::iAModality(QString const & name, QString const & filename, int channel, vtkSmartPointer<vtkImageData> imgData, int renderFlags) :
	m_name(name),
	m_filename(filename),
	renderFlags(renderFlags),
	m_channel(channel),
	m_imgs(1)
{
	SetData(imgData);
}


iAModality::iAModality(QString const & name, QString const & filename, std::vector<vtkSmartPointer<vtkImageData> > imgs, int renderFlags) :
	m_name(name),
	m_filename(filename),
	renderFlags(renderFlags),
	m_channel(-1),
	m_imgs(imgs)
{
	SetData(imgs[0]);
}

QString iAModality::GetName() const
{
	return m_name;
}

QString iAModality::GetFileName() const
{
	return m_filename;
}

int iAModality::GetChannel() const
{
	return m_channel;
}

int iAModality::ComponentCount() const
{
	return m_imgs.size();
}

vtkSmartPointer<vtkImageData> iAModality::GetComponent(int componentIdx) const
{
	return m_imgs[componentIdx];
}

QString iAModality::GetTransferFileName() const
{
	return tfFileName;
}

void iAModality::SetName(QString const & name)
{
	m_name = name;
}

void iAModality::SetFileName(QString const & fileName)
{
	m_filename = fileName;
}

void iAModality::SetRenderFlag(int renderFlags)
{
	this->renderFlags = renderFlags;
}

int iAModality::GetWidth() const
{
	assert(m_converter);
	return m_converter->GetWidth();
}

int iAModality::GetHeight() const
{
	assert(m_converter);
	return m_converter->GetHeight();
}

int iAModality::GetDepth() const
{
	assert(m_converter);
	return m_converter->GetDepth();
}

double const * iAModality::GetSpacing() const
{
	return m_imgs[0]->GetSpacing();
}

double const * iAModality::GetOrigin() const
{
	return m_imgs[0]->GetOrigin();
}

void iAModality::SetSpacing(double spacing[3])
{
	m_imgs[0]->SetSpacing(spacing);
}

void iAModality::SetOrigin(double origin[3])
{
	m_imgs[0]->SetOrigin(origin);
}

iAImageCoordConverter const & iAModality::GetConverter() const
{
	assert(m_converter);
	return *m_converter;
}

vtkSmartPointer<vtkImageData> iAModality::GetImage() const
{
	return m_imgs[0];
}

QString iAModality::GetImageName(int componentIdx)
{
	QString name(GetName());
	if (ComponentCount() > 1)
	{
		return QString("%1-%2").arg(name).arg(componentIdx);
	}
	return name;
}

bool iAModality::hasRenderFlag(RenderFlag loc) const
{
	return (renderFlags & loc) == loc;
}


int iAModality::RenderFlags() const
{
	return renderFlags;
}

void iAModality::SetTransfer(QSharedPointer<iAModalityTransfer> transfer)
{
	// TODO: VOLUME: rewrite / move to iAModalityTransfer constructor if possible!
	m_transfer = transfer;
}

void iAModality::LoadTransferFunction()
{
	if (tfFileName.isEmpty())
	{
		return;
	}
	Settings s(tfFileName);
	s.LoadTransferFunction(GetTransfer().data(), GetImage()->GetScalarRange());
}

QSharedPointer<iAModalityTransfer> iAModality::GetTransfer()
{
	return m_transfer;
}

void iAModality::SetRenderer(QSharedPointer<iAVolumeRenderer> renderer)
{
	m_renderer = renderer;
	if (orientationSettings.isEmpty() || positionSettings.isEmpty())
	{
		return;
	}
	double position[3];
	double orientation[3];
	if (!Str2Vec3D(orientationSettings, orientation) ||
		!Str2Vec3D(positionSettings, position))
	{
		return;
	}
	renderer->SetPosition(position);
	renderer->SetOrientation(orientation);
}

QSharedPointer<iAVolumeRenderer> iAModality::GetRenderer()
{
	return m_renderer;
}

void iAModality::SetData(vtkSmartPointer<vtkImageData> imgData)
{
	assert(imgData);
	m_imgs[0] = imgData;
	int extent[6];
	imgData->GetExtent(extent);
	m_converter = QSharedPointer<iAImageCoordConverter>(new iAImageCoordConverter(
		extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1));
}


void iAModality::SetStringSettings(QString const & pos, QString const & ori, QString const & tfFile)
{
	positionSettings = pos;
	orientationSettings = ori;
	tfFileName = tfFile;
}


QString iAModality::GetOrientationString()
{
	return m_renderer ? Vec3D2String(m_renderer->GetOrientation()) : QString();
}


QString iAModality::GetPositionString()
{
	return m_renderer ? Vec3D2String(m_renderer->GetPosition()) : QString();
}
