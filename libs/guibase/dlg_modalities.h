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
#pragma once

#include "iAguibase_export.h"

#include <vtkSmartPointer.h>

#include <QDockWidget>
#include <QSharedPointer>
#include <QVector>

class iAvtkInteractStyleActor;
class iAFast3DMagicLensWidget;
class iAModality;
class iAModalityList;
class iAVolumeRenderer;
class iAVolumeSettings;
class iAModalityTransfer;
class iAMdiChild;

class Ui_modalities;

class vtkActor;
class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkPlane;
class vtkRenderer;

class QListWidgetItem;

// TODO: VOLUME: split off volume manager for the management of the actual volume rendering stuff
// TODO: VOLUME: rename to dlg_dataList or such
class iAguibase_API dlg_modalities : public QDockWidget
{
	Q_OBJECT
public:
	dlg_modalities(iAFast3DMagicLensWidget* renderer, vtkRenderer* mainRenderer, iAMdiChild* mdiChild);
	void setModalities(QSharedPointer<iAModalityList> modalities);
	QSharedPointer<iAModalityList const> modalities() const;
	QSharedPointer<iAModalityList> modalities();
	int selected() const;
	vtkSmartPointer<vtkColorTransferFunction> colorTF(int modality);
	vtkSmartPointer<vtkPiecewiseFunction> opacityTF(int modality);
	void changeRenderSettings(iAVolumeSettings const & rs, const bool loadSavedVolumeSettings);
	void showSlicers(bool enabled, vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3);
	void addListItem(QSharedPointer<iAModality> mod);
	//! initialize a modality's display in renderers
	void initDisplay(QSharedPointer<iAModality> mod);
	void addModality(vtkSmartPointer<vtkImageData>, QString const & name);
	void addModality(QSharedPointer<iAModality> mod);
	void selectRow(int idx);
	void enableUI();
	void setFileName(int modality, QString const & fileName);
	void setInteractionMode(bool manualRegistration);

	void setChecked(QSharedPointer<iAModality>, Qt::CheckState checked);
	void setAllChecked(Qt::CheckState checked);

public slots:
	//! add modality to list, create transfer function, add volume to renderers
	void modalityAdded(QSharedPointer<iAModality> mod);

signals:
	void modalityAvailable(int modalityIdx);
	void modalitySelected(int modalityIdx);
	void modalitiesChanged(bool spacingChanged, double const * newSpacing);
	void modalityVisibilityChanged(bool visible);

private slots:
	void addClicked();
	void removeClicked();
	void editClicked();

	void rendererMouseMoved();
	void enableButtons();

	//! enable dragging / picking of clicked modality
	void listClicked(QListWidgetItem* item);

	//! enable/ picking dragging of selected modality
	void setModalitySelectionMovable(int selectedRow);

	void checkboxClicked(QListWidgetItem* item);

private:
	//! connects interactor styles  slicer to each other and with 3D renderer
	void configureInterActorStyles(QSharedPointer<iAModality> editModality);
	//! add a modality to the list
	void addToList(QSharedPointer<iAModality> mod);

	// TODO: move modalities out of here (mdichild? common data repository?)
	QSharedPointer<iAModalityList> m_modalities;
	iAFast3DMagicLensWidget* m_magicLensWidget;
	vtkRenderer* m_mainRenderer;
	iAMdiChild* m_mdiChild;

	vtkSmartPointer<iAvtkInteractStyleActor> m_manualMoveStyle[4];

	QSharedPointer<Ui_modalities> m_ui;

	void setChecked(QListWidgetItem* item, Qt::CheckState checked);
	void setModalityVisibility(QSharedPointer<iAModality>, bool visible);

	QListWidgetItem* item(QSharedPointer<iAModality>);
};
