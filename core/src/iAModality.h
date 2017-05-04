/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QString>

#include <vector>

class iAImageCoordConverter;
class iAModalityTransfer;
class iAVolumeRenderer;

class vtkImageData;
class vtkVolume;
class vtkSmartVolumeMapper;
class vtkVolumeProperty;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;

//! class holding the data of a single image channel
class open_iA_Core_API iAModality
{
public:
	enum RenderFlag
	{
		NoRenderer = 0x0,
		MainRenderer = 0x01,
		MagicLens = 0x02,
		BoundingBox = 0x04 // TODO: check if that is a good idea or whether that should go somewhere else (VolumeRenderer)?
	};
	//! create modality from name, file and image data
	iAModality(QString const & name, QString const & filename, int channelNo, vtkSmartPointer<vtkImageData> imgData, int renderFlags);
	//! create modality from name, file and image data
	iAModality(QString const & name, QString const & filename, std::vector<vtkSmartPointer<vtkImageData> > imgs, int renderFlags);
	//! returns name of the modality
	QString GetName() const;
	//! returns file holding the modality data
	QString GetFileName() const;
	//! return the channel in the specified file that the data in this class comes from
	int GetChannel() const;
	//! return the number of components in this modality
	int ComponentCount() const;
	//! return a specific component of this modality
	vtkSmartPointer<vtkImageData> GetComponent(int idx) const;
	//! get the name of the transfer function file
	QString GetTransferFileName() const;
	//! set name of the modality
	void SetName(QString const & name);
	//! set the filename (to be used if it has changed externally; does not save the file!)
	void SetFileName(QString const & fileName);
	//! set flag indicating location where to render
	void SetRenderFlag(int renderFlag);
	//! get the main image of this modality (typically only the one is available,
	//! unless there are multiple components, see ComponentCount() and GetComponent()
	vtkSmartPointer<vtkImageData> GetImage() const;
	//! return the name of the given component
	QString GetImageName(int componentIdx);

	QString GetOrientationString();
	QString GetPositionString();

	int GetWidth() const;
	int GetHeight() const;
	int GetDepth() const;
	double const * GetSpacing() const;
	double const * GetOrigin() const;
	void SetSpacing(double spacing[3]);
	void SetOrigin(double origin[3]);
	iAImageCoordConverter const & GetConverter() const;

	bool hasRenderFlag(RenderFlag flag) const;
	int RenderFlags() const;

	void LoadTransferFunction();
	void SetTransfer(QSharedPointer<iAModalityTransfer> transfer);
	QSharedPointer<iAModalityTransfer> GetTransfer();
	void SetRenderer(QSharedPointer<iAVolumeRenderer> renderer);
	QSharedPointer<iAVolumeRenderer> GetRenderer();

	void SetStringSettings(QString const & pos, QString const & ori, QString const & tfFile);
private:
	void SetData(vtkSmartPointer<vtkImageData> imgData);

	QString m_name;
	QString m_filename;
	int     m_channel;     //!< in case the file contains multiple channels, the channel no. for this modality
	int     renderFlags;
	QSharedPointer<iAImageCoordConverter> m_converter;
	QSharedPointer<iAModalityTransfer> m_transfer;
	QSharedPointer<iAVolumeRenderer> m_renderer;
	std::vector<vtkSmartPointer<vtkImageData> > m_imgs;	// TODO: implement lazy loading
	vtkSmartPointer<vtkImageData> m_imgData;

	// TODO: Refactor
	QString positionSettings;
	QString orientationSettings;
	QString tfFileName;
};
