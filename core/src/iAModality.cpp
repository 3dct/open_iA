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
#include "iAModality.h"

#include "defines.h"  // for NotExistingChannel
#include "iAImageCoordinate.h"
#include "iAModalityTransfer.h"
#include "iASettings.h"
#include "iAStringHelper.h" // for Str2Vec3D
#include "iATypedCallHelper.h"
#include "iAVolumeRenderer.h"

#include <vtkImageData.h>
#include <vtkVolume.h>

#include <cassert>
#include <limits>

iAModality::iAModality(QString const & name, QString const & filename, int channel,
	vtkSmartPointer<vtkImageData> imgData, int renderFlags) :
	m_name(name),
	m_filename(filename),
	renderFlags(renderFlags),
	m_channel(channel),
	m_imgs(1),
	m_VolSettingsSavedStatus(false),
	m_channelID(NotExistingChannel)
{
	SetData(imgData);
}

iAModality::iAModality(QString const & name, QString const & filename, std::vector<vtkSmartPointer<vtkImageData> > imgs, int renderFlags) :
	m_name(name),
	m_filename(filename),
	renderFlags(renderFlags),
	m_channel(-1),
	m_imgs(imgs),
	m_VolSettingsSavedStatus(false)
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
	assert(m_transfer);
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

iAImageInfo const & iAModality::Info() const
{
	assert(m_transfer);
	return m_transfer->Info();
}

bool iAModality::hasRenderFlag(RenderFlag loc) const
{
	return (renderFlags & loc) == loc;
}


int iAModality::RenderFlags() const
{
	return renderFlags;
}

void iAModality::LoadTransferFunction()
{
	iASettings s(tfFileName);
	s.LoadTransferFunction(GetTransfer().data());
}

QSharedPointer<iAModalityTransfer> iAModality::GetTransfer()
{
	assert(m_transfer);
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

void iAModality::UpdateRenderer()
{
	GetRenderer()->SetImage(GetTransfer().data(), GetImage());
}

template <typename T>
void getTypeMinMaxRange(double & minR, double & maxR)
{
	minR = std::numeric_limits<T>::lowest();
	maxR = std::numeric_limits<T>::max();
}

void iAModality::SetData(vtkSmartPointer<vtkImageData> imgData)
{
	assert(imgData);
	m_imgs[0] = imgData;
	int extent[6];
	imgData->GetExtent(extent);
	m_converter = QSharedPointer<iAImageCoordConverter>(new iAImageCoordConverter(
		extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1));
	double maxRange[2];
	VTK_TYPED_CALL(getTypeMinMaxRange, imgData->GetScalarType(), maxRange[0], maxRange[1])
	m_transfer = QSharedPointer<iAModalityTransfer>(new iAModalityTransfer(maxRange));
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

void iAModality::ComputeHistogramData(size_t numBin)
{
	m_transfer->computeHistogramData(GetImage(), numBin);
}

void iAModality::ComputeImageStatistics()
{
	m_transfer->computeStatistics(GetImage());
	if (!tfFileName.isEmpty())
	{
		LoadTransferFunction();
		tfFileName = "";
	}
}

QSharedPointer<iAHistogramData> const iAModality::GetHistogramData() const
{
	return m_transfer->getHistogramData();
}

void iAModality::setVolSettings(const iAVolumeSettings &volSettings)
{
	this->m_volSettings = volSettings;
	this->m_VolSettingsSavedStatus = true;
}

const iAVolumeSettings &iAModality::getVolumeSettings() const
{
	return this->m_volSettings;
}

uint iAModality::channelID() const
{
	return m_channelID;
}

void iAModality::setChannelID(uint channelID)
{
	m_channelID = channelID;
}

// iAStatisticsUpdater

void iAStatisticsUpdater::run()
{
	m_modality->ComputeImageStatistics();
	emit StatisticsReady(m_modalityIdx);
}

iAStatisticsUpdater::iAStatisticsUpdater(int modalityIdx, QSharedPointer<iAModality> modality) :
	m_modalityIdx(modalityIdx),
	m_modality(modality)
{}


// iAHistogramUpdater

void iAHistogramUpdater::run()
{
	m_modality->ComputeHistogramData(m_binCount);
	emit HistogramReady(m_modalityIdx);
}

iAHistogramUpdater::iAHistogramUpdater(int modalityIdx, QSharedPointer<iAModality> modality, size_t binCount) :
	m_modalityIdx(modalityIdx),
	m_modality(modality),
	m_binCount(binCount)
{}