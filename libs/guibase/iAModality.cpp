/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iALog.h"
#include "iAImageCoordinate.h"
#include "iAModalityTransfer.h"
#include "iAXmlSettings.h"
#include "iAStringHelper.h"   // for arrayToString, stringToArray
#include "iATypedCallHelper.h"
#include "iAVolumeRenderer.h"

#include <cassert>
#include <limits>

iAModality::iAModality(QString const & name, QString const & filename, int channel,
	vtkSmartPointer<vtkImageData> imgData, int renderFlags) :
	m_VolSettingsSavedStatus(false),
	m_name(name),
	m_filename(filename),
	m_channel(channel),
	m_renderFlags(renderFlags),
	m_channelID(NotExistingChannel),
	m_slicerOpacity(1),
	m_imgs(1)
{
	setData(imgData);
}

iAModality::iAModality(QString const & name, QString const & filename, std::vector<vtkSmartPointer<vtkImageData> > imgs, int renderFlags) :
	m_VolSettingsSavedStatus(false),
	m_name(name),
	m_filename(filename),
	m_channel(-1),
	m_renderFlags(renderFlags),
	m_imgs(imgs)
{
	setData(imgs[0]);
}

QString iAModality::name() const
{
	return m_name;
}

QString iAModality::fileName() const
{
	return m_filename;
}

int iAModality::channel() const
{
	return m_channel;
}

size_t iAModality::componentCount() const
{
	return m_imgs.size();
}

vtkSmartPointer<vtkImageData> iAModality::component(size_t componentIdx) const
{
	return m_imgs[componentIdx];
}

QString iAModality::transferFileName() const
{
	return m_tfFileName;
}

void iAModality::setName(QString const & name)
{
	m_name = name;
	assert(m_transfer);
}

void iAModality::setFileName(QString const & fileName)
{
	m_filename = fileName;
}

void iAModality::setRenderFlag(int renderFlags)
{
	m_renderFlags = renderFlags;
}

int iAModality::width() const
{
	assert(m_converter);
	return m_converter->width();
}

int iAModality::height() const
{
	assert(m_converter);
	return m_converter->height();
}

int iAModality::depth() const
{
	assert(m_converter);
	return m_converter->depth();
}

double const * iAModality::spacing() const
{
	return m_imgs[0]->GetSpacing();
}

double const * iAModality::origin() const
{
	return m_imgs[0]->GetOrigin();
}

void iAModality::setSpacing(double spacing[3])
{
	m_imgs[0]->SetSpacing(spacing);
}

void iAModality::setOrigin(double origin[3])
{
	m_imgs[0]->SetOrigin(origin);
}

iAImageCoordConverter const & iAModality::converter() const
{
	assert(m_converter);
	return *m_converter;
}

vtkSmartPointer<vtkImageData> iAModality::image() const
{
	return m_imgs[0];
}

QString iAModality::imageName(int componentIdx)
{
	QString result(name());
	if (componentCount() > 1)
	{
		return QString("%1-%2").arg(result).arg(componentIdx);
	}
	return result;
}

bool iAModality::hasRenderFlag(RenderFlag loc) const
{
	return (m_renderFlags & loc) == loc;
}

int iAModality::renderFlags() const
{
	return m_renderFlags;
}

void iAModality::loadTransferFunction()
{
	iAXmlSettings s;
	if (!s.read(m_tfFileName))
	{
		LOG(lvlWarn, QString("Failed to read transfer function from file %1").arg(m_tfFileName));
		return;
	}
	s.loadTransferFunction(transfer().data());
}

QSharedPointer<iAModalityTransfer> iAModality::transfer()
{
	assert(m_transfer);
	return m_transfer;
}

void iAModality::setRenderer(QSharedPointer<iAVolumeRenderer> renderer)
{
	m_renderer = renderer;
	if (m_orientationSettings.isEmpty() || m_positionSettings.isEmpty())
	{
		return;
	}
	double position[3];
	double orientation[3];
	if (!stringToArray<double>(m_orientationSettings, orientation, 3) ||
		!stringToArray<double>(m_positionSettings, position, 3))
	{
		return;
	}
	renderer->setPosition(position);
	renderer->setOrientation(orientation);
}

QSharedPointer<iAVolumeRenderer> iAModality::renderer()
{
	return m_renderer;
}

void iAModality::updateRenderer()
{
	renderer()->setImage(transfer().data(), image());
}

template <typename T>
void getTypeMinMaxRange(double & minR, double & maxR)
{	// closest representation in double for data type range; for 64bit types this deviates slightly from actual range!
	minR = static_cast<double>(std::numeric_limits<T>::lowest());
	maxR = static_cast<double>(std::numeric_limits<T>::max());
}

void iAModality::setData(vtkSmartPointer<vtkImageData> imgData)
{
	assert(imgData);
	m_imgs[0] = imgData;
	int extent[6];
	imgData->GetExtent(extent);
	m_converter = QSharedPointer<iAImageCoordConverter>::create(
		extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1);
	double maxRange[2];
	VTK_TYPED_CALL(getTypeMinMaxRange, imgData->GetScalarType(), maxRange[0], maxRange[1])
	m_transfer = QSharedPointer<iAModalityTransfer>::create(maxRange);
}

void iAModality::setStringSettings(QString const & pos, QString const & ori, QString const & tfFile)
{
	m_positionSettings = pos;
	m_orientationSettings = ori;
	m_tfFileName = tfFile;
}

QString iAModality::orientationString()
{
	return m_renderer ? arrayToString(m_renderer->orientation(), 3) : QString();
}

QString iAModality::positionString()
{
	return m_renderer ? arrayToString(m_renderer->position(), 3) : QString();
}

void iAModality::computeImageStatistics()
{
	m_transfer->computeRange(image());
	if (!m_tfFileName.isEmpty())
	{
		loadTransferFunction();
		m_tfFileName = "";
	}
}

void iAModality::setHistogramData(QSharedPointer<iAHistogramData> histogramData)
{
	m_histogramData = histogramData;
}

QSharedPointer<iAHistogramData> const iAModality::histogramData() const
{
	return m_histogramData;
}

void iAModality::setVolSettings(const iAVolumeSettings &volSettings)
{
	m_volSettings = volSettings;
	m_VolSettingsSavedStatus = true;
}

const iAVolumeSettings &iAModality::volumeSettings() const
{
	return m_volSettings;
}

uint iAModality::channelID() const
{
	return m_channelID;
}

void iAModality::setChannelID(uint channelID)
{
	m_channelID = channelID;
}
