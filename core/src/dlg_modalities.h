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
#include <QVector>

#include "ui_modalities.h"
#include <iAQTtoUIConnector.h>
typedef iAQTtoUIConnector<QDockWidget, Ui_modalities> dlg_modalitiesUI;

class dlg_planeSlicer;
class iAFast3DMagicLensWidget;
class iAHistogramWidget;
class iAModality;
class iAModalityList;
class iAVolumeRenderer;
class iAVolumeSettings;
class iAModalityTransfer;

class vtkActor;
class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkPlane;
class vtkRenderer;

// TODO: VOLUME: split off volume manager for the management of the actual volume rendering stuff
// TODO: VOLUME: rename to dlg_dataList or such
class open_iA_Core_API dlg_modalities : public dlg_modalitiesUI
{
	Q_OBJECT
public:
	dlg_modalities(iAFast3DMagicLensWidget* renderer,
		vtkRenderer* mainRenderer, int numBin, QDockWidget* histogramContainer);
	void SetModalities(QSharedPointer<iAModalityList> modalities);
	QSharedPointer<iAModalityList const> GetModalities() const;
	QSharedPointer<iAModalityList> GetModalities();
	int GetSelected() const;
	vtkSmartPointer<vtkColorTransferFunction> GetCTF(int modality);
	vtkSmartPointer<vtkPiecewiseFunction> GetOTF(int modality);
	void ChangeRenderSettings(iAVolumeSettings const & rs);
	void Store(QString const & filename);
	bool Load(QString const & filename);
	iAHistogramWidget* GetHistogram();
	void ShowSlicePlanes(bool enabled);
	void SetSlicePlanes(vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3);
	//! double responsibility function, adding modality to list and initializing its transfer function
	// TODO: VOLUME: split up!
	void AddListItemAndTransfer(QSharedPointer<iAModality> mod);
	//! initialize a modality's display in renderers
	void InitDisplay(QSharedPointer<iAModality> mod);
	void AddModality(vtkSmartPointer<vtkImageData>, QString const & name);
	void SelectRow(int idx);
	void SwitchHistogram(QSharedPointer<iAModalityTransfer> modTrans);
	void EnableUI();
	void SetFileName(int modality, QString const & fileName);
public slots:
	//! add modality to list, create transfer function, show histogram, add volume to renderers
	void ModalityAdded(QSharedPointer<iAModality> mod);
	void InteractorModeSwitched(int newMode);
signals:
	void ModalityAvailable();
	void ModalitySelected(int modalityIdx);

	//! @{ for histogram:
	void PointSelected();
	void NoPointSelected();
	void EndPointSelected();
	void Active();
	void AutoUpdateChanged(bool toogled);
	void UpdateViews();
	void ModalitiesChanged();
	void ModalityTFChanged();	// ideally we would also emit here which modality (idx?) has changed
	//! @}

private slots:
	void AddClicked();
	void RemoveClicked();
	void EditClicked();
	void ManualRegistration();
	void MagicLens();
	//void CuttingPlane();
	void RendererMouseMoved();

	void EnableButtons();
	void ListClicked(QListWidgetItem* item);
	void ShowChecked(QListWidgetItem* item);

private:
	QSharedPointer<iAModalityList> modalities;
	QString m_FileName;
	iAFast3DMagicLensWidget* m_magicLensWidget;
	int m_numBin;	// only serves to store the numBin from preferences in the MdiChild; this should be a direct reference there to always have the newest value!
	QDockWidget* m_histogramContainer;
	iAHistogramWidget* m_currentHistogram;
	bool m_showSlicePlanes;
	vtkPlane *m_plane1, *m_plane2, *m_plane3;
	vtkRenderer* m_mainRenderer;

	void AddToList(QSharedPointer<iAModality> mod);
	//! initialize a modality's transfer function
	void InitTransfer(QSharedPointer<iAModality> mod);
};
