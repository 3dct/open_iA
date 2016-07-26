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
#include "pch.h"
#include "dlg_modalities.h"

#include <QVTKInteractor.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>

#include "dlg_modalityProperties.h"
#include "iAConsole.h"
#include "iAFast3DMagicLensWidget.h"
#include "iAHistogramWidget.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "iARenderer.h"
#include "iASlicer.h"
#include "iASlicerData.h"
#include "iAVolumeRenderer.h"
#include "iAVolumeSettings.h"

#include <QFileDialog>
#include <QSettings>

#include <cassert>

namespace
{
	char const * ProjectFileTypeFilter("open_iA project file (*.mod);;All files (*.*)");
}

dlg_modalities::dlg_modalities(iAFast3DMagicLensWidget* modalityRenderer, int numBin, QDockWidget* histogramContainer) :

	modalities(new iAModalityList),
	m_selectedRow(-1),
	m_renderer(modalityRenderer),
	m_numBin(numBin),
	m_histogramContainer(histogramContainer),
	m_currentHistogram(0),
	m_showVolumes(true),
	m_showSlicePlanes(false)
{
	connect(pbAdd,    SIGNAL(clicked()), this, SLOT(AddClicked()));
	connect(pbRemove, SIGNAL(clicked()), this, SLOT(RemoveClicked()));
	connect(pbEdit,   SIGNAL(clicked()), this, SLOT(EditClicked()));
	connect(pbStore,  SIGNAL(clicked()), this, SLOT(Store()));
	connect(pbLoad, SIGNAL(clicked()), this, SLOT(Load()));
	connect(cbManualRegistration, SIGNAL(clicked()), this, SLOT(ManualRegistration()));
	connect(cbShowMagicLens, SIGNAL(clicked()), this, SLOT(MagicLens()));
	
	connect(lwModalities, SIGNAL(itemClicked(QListWidgetItem*)),
		this, SLOT(ListClicked(QListWidgetItem*)));

	connect(modalityRenderer, SIGNAL(MouseMoved()), this, SLOT(RendererMouseMoved()));
}

void dlg_modalities::SetModalities(QSharedPointer<iAModalityList> modList)
{
	modalities = modList;
	lwModalities->clear();
	connect(modalities.data(), SIGNAL(Added(QSharedPointer<iAModality>)), this, SLOT(ModalityAdded(QSharedPointer<iAModality>)));
	for (int i=0; i<modalities->size(); ++i)
	{
		ModalityAdded(modalities->Get(i));
	}
	m_selectedRow = 0;
}

void dlg_modalities::Store()
{
	QString modalitiesFileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Select Output File"),
		QString(), // TODO get directory of current file
		tr(ProjectFileTypeFilter ) );
	Store(modalitiesFileName);
}

void dlg_modalities::Store(QString const & filename)
{								// TODO: VOLUME: not the ideal solution for getting the proper "first" camera
	vtkCamera* cam = m_renderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	modalities->Store(filename, cam);
}

void dlg_modalities::Load()
{
	QString modalitiesFileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr("Open Input File"),
		QString(), // TODO get directory of current file
		tr(ProjectFileTypeFilter) );
	if (!modalitiesFileName.isEmpty() && Load(modalitiesFileName))
	{
		EnableButtons();
		emit ModalityAvailable();
	}
}

void dlg_modalities::SelectRow(int idx)
{
	lwModalities->setCurrentRow(idx);
	m_selectedRow = idx;
}

bool dlg_modalities::Load(QString const & filename)
{
	bool result = modalities->Load(filename);
	SelectRow(0);
	return result;
}

QString GetCaption(iAModality const & mod)
{
	QFileInfo fi(mod.GetFileName());
	return mod.GetName()+" ("+fi.fileName()+")";
}

void dlg_modalities::AddClicked()
{
	QSharedPointer<iAModality> newModality(new iAModality);
	dlg_modalityProperties prop(this, newModality);
	if (prop.exec() == QDialog::Rejected)
	{
		return;
	}
	modalities->Add(newModality);
}

void dlg_modalities::MagicLens()
{
	if (cbShowMagicLens->isChecked())
	{
		m_renderer->magicLensOn();
	}
	else
	{
		m_renderer->magicLensOff();
	}
}

void dlg_modalities::ModalityAdded(QSharedPointer<iAModality> mod)
{
	QListWidgetItem* listItem = new QListWidgetItem(GetCaption(*mod));
	lwModalities->addItem (listItem);
	EnableButtons();
	m_selectedRow = lwModalities->row( listItem );
	vtkSmartPointer<vtkImageData> imgData = mod->GetImage();
	QSharedPointer<iAModalityTransfer> modTransfer(new iAModalityTransfer(
		imgData,
		mod->GetName(),
		this,
		m_numBin));
	mod->SetTransfer(modTransfer);
	SwitchHistogram(modTransfer);
	QSharedPointer<iAVolumeRenderer> modDisp(new iAVolumeRenderer(modTransfer.data(), imgData));
	mod->SetDisplay(modDisp);
	if (mod->hasRenderFlag(iAModality::MainRenderer) && m_showVolumes)
	{
		modDisp->AddToWindow(m_renderer->GetRenderWindow());
	}
	if (mod->hasRenderFlag(iAModality::MagicLens))
	{
		// TODO: VOLUME: use render window for magic lens as well?
		m_renderer->getLensRenderer()->AddVolume(modDisp->GetVolume());
	}
	emit ModalityAvailable();
}


void  dlg_modalities::SwitchHistogram(QSharedPointer<iAModalityTransfer> modTrans)
{
	if (m_currentHistogram)
	{
		m_currentHistogram->disconnect();
	}
	m_currentHistogram = modTrans->ShowHistogram(m_histogramContainer);
	connect(m_currentHistogram, SIGNAL(updateViews()), this, SIGNAL(UpdateViews()));
	connect(m_currentHistogram, SIGNAL(pointSelected()), this, SIGNAL(PointSelected()));
	connect(m_currentHistogram, SIGNAL(noPointSelected()), this, SIGNAL(NoPointSelected()));
	connect(m_currentHistogram, SIGNAL(endPointSelected()), this, SIGNAL(EndPointSelected()));
	connect(m_currentHistogram, SIGNAL(active()), this, SIGNAL(Active()));
	connect(m_currentHistogram, SIGNAL(autoUpdateChanged(bool)), this, SIGNAL(AutoUpdateChanged(bool)));
}

void dlg_modalities::RemoveClicked()
{
	int idx = lwModalities->currentRow();
	if (idx < 0 || idx >= modalities->size())
	{
		DEBUG_LOG(QString("Index out of range (%1)\n").arg(idx));
		return;
	}
	// TODO: refactor
	QSharedPointer<iAVolumeRenderer> modDisp = modalities->Get(idx)->GetDisplay();
	if (modalities->Get(idx)->hasRenderFlag(iAModality::MainRenderer))
	{
		modDisp->RemoveFromWindow();
	}
	if (modalities->Get(idx)->hasRenderFlag(iAModality::MagicLens))
	{
		m_renderer->getLensRenderer()->RemoveVolume(modDisp->GetVolume());
	}
	modalities->Remove(idx);
	delete lwModalities->takeItem(idx);
	EnableButtons();
}

void dlg_modalities::EditClicked()
{
	int idx = lwModalities->currentRow();
	if (idx < 0 || idx >= modalities->size())
	{
		DEBUG_LOG(QString("Index out of range (%1)\n").arg(idx));
		return;
	}
	QString fnBefore = modalities->Get(idx)->GetFileName();
	int renderFlagsBefore = modalities->Get(idx)->RenderFlags();
	QSharedPointer<iAModality> editModality(modalities->Get(idx));
	dlg_modalityProperties prop(this, editModality);
	if (prop.exec() == QDialog::Rejected)
	{
		return;
	}
	if (fnBefore != editModality->GetFileName())
	{
		DEBUG_LOG("Changing file not supported!\n");
	}
	QSharedPointer<iAVolumeRenderer> modDisp = modalities->Get(idx)->GetDisplay();
	if ((renderFlagsBefore & iAModality::MainRenderer) == iAModality::MainRenderer
		&& !editModality->hasRenderFlag(iAModality::MainRenderer))
	{
		modDisp->RemoveFromWindow();
	}
	if ((renderFlagsBefore & iAModality::MainRenderer) == 0
		&& editModality->hasRenderFlag(iAModality::MainRenderer))
	{
		modDisp->AddToWindow(m_renderer->GetRenderWindow());
	}
	if ((renderFlagsBefore & iAModality::MagicLens) == iAModality::MagicLens
		&& !editModality->hasRenderFlag(iAModality::MagicLens))
	{
		m_renderer->getLensRenderer()->RemoveVolume(modDisp->GetVolume());
	}
	if ((renderFlagsBefore & iAModality::MagicLens) == 0
		&& editModality->hasRenderFlag(iAModality::MagicLens))
	{
		m_renderer->getLensRenderer()->AddVolume(modDisp->GetVolume());
	}
	lwModalities->item(idx)->setText(GetCaption(*editModality));
}

void dlg_modalities::EnableButtons()
{
	bool enable = modalities->size() > 0;
	pbEdit->setEnabled(enable);
	pbRemove->setEnabled(enable);
	pbStore->setEnabled(enable);
}

void dlg_modalities::ManualRegistration()
{
	if (cbManualRegistration->isChecked())
	{
		m_renderer->GetInteractor()->SetInteractorStyle(vtkInteractorStyleTrackballActor::New());
	}
	else
	{
		m_renderer->GetInteractor()->SetInteractorStyle(vtkInteractorStyleTrackballCamera::New());
	}
}

void dlg_modalities::ListClicked(QListWidgetItem* item)
{
	m_selectedRow = lwModalities->row( item );
	QSharedPointer<iAModality> currentData = modalities->Get(m_selectedRow);
	if (m_selectedRow >= 0)
	{
		QSharedPointer<iAModalityTransfer> modTransfer = currentData->GetTransfer();
		SwitchHistogram(modTransfer);
	}
	emit ShowImage(currentData->GetImage());
}

QSharedPointer<iAModalityList const> dlg_modalities::GetModalities() const
{
	return modalities;
}

QSharedPointer<iAModalityList> dlg_modalities::GetModalities()
{
	return modalities;
}

int dlg_modalities::GetSelected() const
{
	return m_selectedRow;
}

vtkSmartPointer<vtkColorTransferFunction> dlg_modalities::GetCTF(int modality)
{
	return modalities->Get(modality)->GetTransfer()->GetColorFunction();
}

vtkSmartPointer<vtkPiecewiseFunction> dlg_modalities::GetOTF(int modality)
{
	return modalities->Get(modality)->GetTransfer()->GetOpacityFunction();
}

void dlg_modalities::ChangeRenderSettings(iAVolumeSettings const & rs)
{
	for (int i = 0; i < modalities->size(); ++i)
	{
		QSharedPointer<iAVolumeRenderer> modDisp = modalities->Get(i)->GetDisplay();
		modDisp->ApplySettings(rs);
	}
}

void dlg_modalities::RendererMouseMoved()
{
	double baseVector[3] = { 0, 0, 1 };
	double basePosition[3] = { -0.1, -0.1, -0.1 };
}

iAHistogramWidget* dlg_modalities::GetHistogram()
{
	return m_currentHistogram;
}


void dlg_modalities::ShowVolumes(bool show)
{
	m_showVolumes = show;
	for (int i = 0; i < modalities->size(); ++i)
	{
		if (!modalities->Get(i)->hasRenderFlag(iAModality::MainRenderer))
		{
			continue;
		}
		ShowVolume(modalities->Get(i)->GetDisplay(), show);
	}
}

void dlg_modalities::ShowVolume(QSharedPointer<iAVolumeRenderer> renderer, bool enabled)
{
	if (enabled)
	{
		renderer->AddToWindow(m_renderer->GetRenderWindow());
	}
	else
	{
		renderer->RemoveFromWindow();
	}
}

void dlg_modalities::ShowSlicePlanes(bool enabled)
{
	m_showSlicePlanes = enabled;
	for (int i = 0; i < modalities->size(); ++i)
	{
		QSharedPointer<iAVolumeRenderer> renderer = modalities->Get(i)->GetDisplay();
		if (enabled)
		{
			renderer->SetCuttingPlanes(m_plane1, m_plane2, m_plane3);
		}
		else
		{
			renderer->RemoveCuttingPlanes();
		}
	}
}

void dlg_modalities::SetSlicePlanes(vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3)
{

	m_plane1 = plane1;
	m_plane2 = plane2;
	m_plane3 = plane3;
}



void dlg_modalities::AddModality(vtkSmartPointer<vtkImageData> img, QString const & name)
{
	QSharedPointer<iAModality> newModality(new iAModality(name, "", img, iAModality::MainRenderer));
	modalities->Add(newModality);
}