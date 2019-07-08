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
#pragma once

#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QVector>

#include "ui_modalities.h"
#include "qthelper/iAQTtoUIConnector.h"
typedef iAQTtoUIConnector<QDockWidget, Ui_modalities> dlg_modalitiesUI;

class dlg_planeSlicer;
class iAFast3DMagicLensWidget;
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
	dlg_modalities(iAFast3DMagicLensWidget* renderer, vtkRenderer* mainRenderer, int numBin);
	void SetModalities(QSharedPointer<iAModalityList> modalities);
	QSharedPointer<iAModalityList const> GetModalities() const;
	QSharedPointer<iAModalityList> GetModalities();
	int GetSelected() const;
	vtkSmartPointer<vtkColorTransferFunction> GetCTF(int modality);
	vtkSmartPointer<vtkPiecewiseFunction> GetOTF(int modality);
	void ChangeRenderSettings(iAVolumeSettings const & rs, const bool loadSavedVolumeSettings);
	void ShowSlicers(bool enabled, vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3);
	void AddListItem(QSharedPointer<iAModality> mod);
	//! initialize a modality's display in renderers
	void InitDisplay(QSharedPointer<iAModality> mod);
	void AddModality(vtkSmartPointer<vtkImageData>, QString const & name);
	void SelectRow(int idx);
	void EnableUI();
	void SetFileName(int modality, QString const & fileName);
public slots:
	//! add modality to list, create transfer function, add volume to renderers
	void ModalityAdded(QSharedPointer<iAModality> mod);
	void InteractorModeSwitched(int newMode);
signals:
	void ModalityAvailable(int modalityIdx);
	void ModalitySelected(int modalityIdx);
	void ModalitiesChanged(bool spacingChanged, double const * newSpacing);

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
	// TODO: move modalities out of here (mdichild? common data repository?)
	QSharedPointer<iAModalityList> modalities;
	QString m_FileName;
	iAFast3DMagicLensWidget* m_magicLensWidget;
	vtkRenderer* m_mainRenderer;

	void AddToList(QSharedPointer<iAModality> mod);
	//! initialize a modality's transfer function
	void InitTransfer(QSharedPointer<iAModality> mod);
};
