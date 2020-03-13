/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "open_iA_Core_export.h"
#include "iAVolumeSettings.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QString>
#include <QThread>

#include <vector>

class iAHistogramData;
class iAImageCoordConverter;
class iAImageInfo;
class iAModalityTransfer;
class iAVolumeRenderer;
class vtkImageData;

//! class holding the data of a single image channel
class open_iA_Core_API iAModality
{
public:
	enum RenderFlag
	{
		NoRenderer = 0x0,
		MainRenderer = 0x01,
		MagicLens = 0x02,
		BoundingBox = 0x04,	// TODO: check if that is a good idea or whether that should go somewhere else (VolumeRenderer)?
		Slicer = 0x08
	};
	//! create modality from name, file and image data
	iAModality(QString const & name, QString const & filename, int channelNo, vtkSmartPointer<vtkImageData> imgData, int renderFlags);
	//! create modality from name, file and image data
	iAModality(QString const & name, QString const & filename, std::vector<vtkSmartPointer<vtkImageData> > imgs, int renderFlags);
	//! returns name of the modality
	QString name() const;
	//! returns file holding the modality data
	QString fileName() const;
	//! return the channel in the specified file that the data in this class comes from (don't confuse with channelID!)
	int channel() const;
	//! return the number of components in this modality
	size_t componentCount() const;
	//! return a specific component of this modality
	vtkSmartPointer<vtkImageData> component(size_t idx) const;
	//! get the name of the transfer function file
	QString transferFileName() const;
	//! set name of the modality
	void setName(QString const & name);
	//! set the filename (to be used if it has changed externally; does not save the file!)
	void setFileName(QString const & fileName);
	//! set flag indicating location where to render
	void setRenderFlag(int renderFlag);
	//! get the main image of this modality (typically only the one is available,
	//! unless there are multiple components, see ComponentCount() and GetComponent()
	vtkSmartPointer<vtkImageData> image() const;
	//! return the name of the given component
	QString imageName(int componentIdx);
	//! return statistical information about the image
	iAImageInfo const & info() const;
	//! return ID of channel used in mdichild to represent this modality in slicer (don't confuse with channelID!)
	uint channelID() const;
	//! set ID of channel used in mdichild to represent this modality in slicer
	void setChannelID(uint id);

	QString orientationString();
	QString positionString();

	int width() const;
	int height() const;
	int depth() const;
	double const * spacing() const;
	double const * origin() const;
	void setSpacing(double spacing[3]);
	void setOrigin(double origin[3]);
	iAImageCoordConverter const & converter() const;

	bool hasRenderFlag(RenderFlag flag) const;
	int renderFlags() const;

	void loadTransferFunction();
	QSharedPointer<iAModalityTransfer> transfer();
	void setRenderer(QSharedPointer<iAVolumeRenderer> renderer);
	QSharedPointer<iAVolumeRenderer> renderer();
	void updateRenderer();

	void setStringSettings(QString const & pos, QString const & ori, QString const & tfFile);
	void setData(vtkSmartPointer<vtkImageData> imgData);
	void computeImageStatistics();
	void computeHistogramData(size_t numBin);
	QSharedPointer<iAHistogramData> const histogramData() const;

	void setVolSettings(const iAVolumeSettings &volSettings);

	const iAVolumeSettings &volumeSettings() const;

	inline bool volSettingsSavedStatus() {
		return this->m_VolSettingsSavedStatus;
	}

	inline void setVolSettingsSavedStatusFalse() {
		this->m_VolSettingsSavedStatus = false;
	}

	void setSlicerOpacity(double opacity)
	{
		m_slicerOpacity = opacity;
	}
	double slicerOpacity()
	{
		return m_slicerOpacity;
	}

private:
	iAVolumeSettings m_volSettings;
	bool m_VolSettingsSavedStatus;

	QString m_name;
	QString m_filename;
	int     m_channel;     //!< in case the file contains multiple channels, the channel no. for this modality
	int     m_renderFlags;
	uint    m_channelID;
	double m_slicerOpacity;  //!< overall opacity in the slicers
	QSharedPointer<iAImageCoordConverter> m_converter;
	QSharedPointer<iAModalityTransfer> m_transfer;
	QSharedPointer<iAVolumeRenderer> m_renderer;
	std::vector<vtkSmartPointer<vtkImageData> > m_imgs;	// TODO: implement lazy loading
	vtkSmartPointer<vtkImageData> m_imgData;

	// TODO: Refactor
	QString m_positionSettings;
	QString m_orientationSettings;
	QString m_tfFileName;
};


class iAStatisticsUpdater : public QThread
{
Q_OBJECT
	void run() override;
signals:
	void StatisticsReady(int modalityIdx);
private:
	int m_modalityIdx;
	QSharedPointer<iAModality> m_modality;
public:
	iAStatisticsUpdater(int modalityIdx, QSharedPointer<iAModality> modality);
};


//! class for updating the histogram of a modality
class iAHistogramUpdater : public QThread
{
Q_OBJECT
	void run() override;
signals:
	void HistogramReady(int modalityIdx);
private:
	int m_modalityIdx;
	QSharedPointer<iAModality> m_modality;
	size_t m_binCount;
public:
	iAHistogramUpdater(int modalityIdx, QSharedPointer<iAModality> modality, size_t binCount);
};
