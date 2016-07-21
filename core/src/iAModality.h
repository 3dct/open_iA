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
#pragma once

#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QString>
#include <QVector>

class iAImageCoordConverter;
class iASpectralVoxelData;
class iAModalityTransfer;
class iAVolumeRenderer;

class vtkCamera;
class vtkImageData;
class vtkVolume;
class vtkSmartVolumeMapper;
class vtkVolumeProperty;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class open_iA_Core_API iAModality
{
public:
	enum RenderFlag
	{
		NoRenderer = 0x0,
		MainRenderer = 0x01,
		MagicLens = 0x02
	};
	//! create uninitialized modality
	iAModality();
	//! create modality from name and file
	iAModality(QString const & name, QString const & filename, int renderFlags);
	//! create modality from name, file and image data
	iAModality(QString const & name, QString const & filename, vtkSmartPointer<vtkImageData> imgData, int renderFlags);
	//! returns name of the modality
	QString GetName() const;
	//! returns file holding the modality data
	QString GetFileName() const;
	//! set name of the modality
	void SetName(QString const & name);
	//! set file containing the modality data
	void SetFileName(QString const & filename);
	//! set flag indicating location where to render
	void SetRenderFlag(int renderFlag);

	int GetWidth() const;
	int GetHeight() const;
	int GetDepth() const;
	double const * GetSpacing() const;
	iAImageCoordConverter const & GetConverter() const;
	
	// TODO: retrieve modality image/data:
	QSharedPointer<iASpectralVoxelData const> GetData() const;

	vtkSmartPointer<vtkImageData> GetImage() const;

	bool hasRenderFlag(RenderFlag flag) const;
	int RenderFlags() const;

	bool LoadData();

	void SetTransfer(QSharedPointer<iAModalityTransfer> transfer);
	QSharedPointer<iAModalityTransfer> GetTransfer();
	void SetDisplay(QSharedPointer<iAVolumeRenderer> display);
	QSharedPointer<iAVolumeRenderer> GetDisplay();

	// TODO: Refactor
	QString positionSettings;
	QString orientationSettings;
	QString tfFileName;
private:
	QString m_name;
	QString m_filename;
	int     renderFlags;

	QSharedPointer<iASpectralVoxelData> m_data;
	QSharedPointer<iAImageCoordConverter> m_converter;
	QSharedPointer<iAModalityTransfer> transfer;
	QSharedPointer<iAVolumeRenderer> display;
	double m_spacing[3];

	// IO-related methods:
	void SetData(vtkSmartPointer<vtkImageData> imgData);
	
	vtkSmartPointer<vtkImageData> m_imgData;
};


typedef QVector<QSharedPointer<iAModality> > ModalityCollection;


class open_iA_Core_API iAModalityList: public QObject
{
	Q_OBJECT
public:
	iAModalityList();
	void Store(QString const & filename, vtkCamera* cam);
	bool Load(QString const & filename, vtkCamera* cam);
	
	int size() const;
	QSharedPointer<iAModality> Get(int idx);
	QSharedPointer<iAModality const> Get(int idx) const;
	void Add(QSharedPointer<iAModality> mod);
	void Remove(int idx);
	QString const & GetFileName() const;
signals:
	void Added(QSharedPointer<iAModality> mod);
private:
	bool ModalityExists(QString const & filename) const;

	ModalityCollection m_modalities;
	double m_spacing[3];
	QString m_fileName;
};
