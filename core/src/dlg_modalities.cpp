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
#include "dlg_modalities.h"

#include "dlg_modalityProperties.h"
#include "iAConsole.h"
#include "iAFast3DMagicLensWidget.h"
#include "iAHistogramWidget.h"
#include "iAIOProvider.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "iARenderer.h"
#include "iASlicer.h"
#include "iASlicerData.h"
#include "iAVolumeRenderer.h"
#include "iAVolumeSettings.h"

#include <QVTKInteractor.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>

#include <QFileDialog>
#include <QSettings>

#include <cassert>


dlg_modalities::dlg_modalities(iAFast3DMagicLensWidget* magicLensWidget,
	vtkRenderer* mainRenderer,
	int numBin, QDockWidget* histogramContainer) :

	modalities(new iAModalityList),
	m_magicLensWidget(magicLensWidget),
	m_mainRenderer(mainRenderer),
	m_numBin(numBin),
	m_histogramContainer(histogramContainer),
	m_currentHistogram(0),
	m_showSlicePlanes(false)
{
	connect(pbAdd,    SIGNAL(clicked()), this, SLOT(AddClicked()));
	connect(pbRemove, SIGNAL(clicked()), this, SLOT(RemoveClicked()));
	connect(pbEdit,   SIGNAL(clicked()), this, SLOT(EditClicked()));
	connect(cbManualRegistration, SIGNAL(clicked()), this, SLOT(ManualRegistration()));
	connect(cbShowMagicLens, SIGNAL(clicked()), this, SLOT(MagicLens()));
	
	connect(lwModalities, SIGNAL(itemClicked(QListWidgetItem*)),
		this, SLOT(ListClicked(QListWidgetItem*)));

	connect(magicLensWidget, SIGNAL(MouseMoved()), this, SLOT(RendererMouseMoved()));
}

void dlg_modalities::SetModalities(QSharedPointer<iAModalityList> modList)
{
	modalities = modList;
	lwModalities->clear();
}


void dlg_modalities::Store(QString const & filename)
{								// TODO: VOLUME: not the ideal solution for getting the proper "first" camera
	vtkCamera* cam = m_mainRenderer->GetActiveCamera();
	modalities->Store(filename, cam);
}

void dlg_modalities::SelectRow(int idx)
{
	lwModalities->setCurrentRow(idx);
}

bool dlg_modalities::Load(QString const & filename)
{
	bool result = modalities->Load(filename);
	if (result)
	{
		SelectRow(0);
		EnableButtons();
		emit ModalityAvailable();
	}
	return result;
}

QString GetCaption(iAModality const & mod)
{
	QFileInfo fi(mod.GetFileName());
	return mod.GetName()+" ("+fi.fileName()+")";
}

#include "iAIO.h"
#include "extension2id.h"

void dlg_modalities::AddClicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load"),
		"",
		iAIOProvider::GetSupportedLoadFormats() + tr("Volume Stack (*.volstack);;"));
	if (fileName.isEmpty())
		return;

	const int DefaultRenderFlags = iAModality::MainRenderer;

	// TODO: unify this with mdichild::loadFile!
	if (fileName.endsWith(iAIO::VolstackExtension))
	{
		std::vector<vtkSmartPointer<vtkImageData> > volumes;
		iAIO io(
			0,
			0,
			&volumes
		);
		io.setupIO(VOLUME_STACK_VOLSTACK_READER, fileName.toLatin1().data());
		io.start();
		io.wait();
		assert(volumes.size() > 0);
		if (volumes.size() == 0)
		{
			DEBUG_LOG("No volume found in stack!");
			return;
		}
		int channels = volumes.size();
		QFileInfo fi(fileName);
		for (int i = 0; i < channels; ++i)
		{
			QSharedPointer<iAModality> newModality(new iAModality(
				fi.baseName(), fileName, i, volumes[i], DefaultRenderFlags) );
			modalities->Add(newModality);
		}
	}
	else
	{
		vtkSmartPointer<vtkImageData> img = vtkSmartPointer<vtkImageData>::New();
		std::vector<vtkSmartPointer<vtkImageData> > volumes;
		iAIO io(img, 0, 0, 0, &volumes);

		QFileInfo fileInfo(fileName);
		QString extension = fileInfo.suffix();
		extension = extension.toUpper();
		const mapQString2int * ext2id = &extensionToId;
		if (ext2id->find(extension) == ext2id->end())
		{
			DEBUG_LOG("Unknown file type!");
			return;
		}
		IOType id = ext2id->find(extension).value();
		if (!io.setupIO(id, fileName, false))
		{
			DEBUG_LOG("Error while setting up modality loading!");
			return;
		}
		// TODO: check for errors during actual loading!
		//connect(io, done(bool), this, )
		io.start();
		// TODO: VOLUME: make asynchronous!
		io.wait();
		QFileInfo fi(fileName);
		if (volumes.size() > 0)
		{
			int channels = volumes.size();
			for (int i = 0; i < channels; ++i)
			{
				QSharedPointer<iAModality> newModality(new iAModality(
					fi.baseName() + "-ch" + QString::number(i),
					fileName, i, volumes[i], DefaultRenderFlags));
				modalities->Add(newModality);
			}
		}
		else
		{
			QSharedPointer<iAModality> newModality(new iAModality(
				fi.baseName(), fileName, -1, img, DefaultRenderFlags));
			modalities->Add(newModality);
		}
	}
	emit ModalitiesChanged();
}

void dlg_modalities::MagicLens()
{
	if (cbShowMagicLens->isChecked())
	{
		m_magicLensWidget->magicLensOn();
	}
	else
	{
		m_magicLensWidget->magicLensOff();
	}
}

void dlg_modalities::InitTransfer(QSharedPointer<iAModality> mod)
{
	vtkSmartPointer<vtkImageData> imgData = mod->GetImage();
	QSharedPointer<iAModalityTransfer> modTransfer(new iAModalityTransfer(
		imgData,
		mod->GetName(),
		this,
		m_numBin));
	mod->SetTransfer(modTransfer);
}

void dlg_modalities::InitDisplay(QSharedPointer<iAModality> mod)
{
	QSharedPointer<iAVolumeRenderer> renderer(new iAVolumeRenderer(mod->GetTransfer().data(), mod->GetImage()));
	mod->SetRenderer(renderer);
	if (mod->hasRenderFlag(iAModality::MainRenderer))
	{
		renderer->AddTo(m_mainRenderer);
	}
	if (mod->hasRenderFlag(iAModality::BoundingBox))
	{
		renderer->AddBoundingBoxTo(m_mainRenderer);
	}
	if (mod->hasRenderFlag(iAModality::MagicLens))
	{
		renderer->AddTo(m_magicLensWidget->getLensRenderer());
	}
}

void dlg_modalities::AddToList(QSharedPointer<iAModality> mod)
{
	QListWidgetItem* listItem = new QListWidgetItem(GetCaption(*mod));
	lwModalities->addItem(listItem);
	lwModalities->setCurrentItem(listItem);
}

void dlg_modalities::AddListItemAndTransfer(QSharedPointer<iAModality> mod)
{
	// TODO: VOLUME: split up somehow
	AddToList(mod);
	EnableButtons();
	InitTransfer(mod);
}

void dlg_modalities::ModalityAdded(QSharedPointer<iAModality> mod)
{
	// TODO: VOLUME: split up somehow
	AddListItemAndTransfer(mod);
	SwitchHistogram(mod->GetTransfer());
	InitDisplay(mod);
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


void dlg_modalities::EnableUI()
{
	pbAdd->setEnabled(true);
	cbManualRegistration->setEnabled(true);
	cbShowMagicLens->setEnabled(true);
}

void dlg_modalities::RemoveClicked()
{
	int idx = lwModalities->currentRow();
	if (idx < 0 || idx >= modalities->size())
	{
		DEBUG_LOG(QString("Index out of range (%1)").arg(idx));
		return;
	}
	QSharedPointer<iAVolumeRenderer> renderer = modalities->Get(idx)->GetRenderer();
	if (modalities->Get(idx)->hasRenderFlag(iAModality::MainRenderer) ||
		modalities->Get(idx)->hasRenderFlag(iAModality::MagicLens))
	{
		renderer->Remove();
	}
	if (modalities->Get(idx)->hasRenderFlag(iAModality::BoundingBox))
	{
		renderer->RemoveBoundingBox();
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
		DEBUG_LOG(QString("Index out of range (%1).").arg(idx));
		return;
	}
	int renderFlagsBefore = modalities->Get(idx)->RenderFlags();
	QSharedPointer<iAModality> editModality(modalities->Get(idx));
	dlg_modalityProperties prop(this, editModality);
	if (prop.exec() == QDialog::Rejected)
	{
		return;
	}
	QSharedPointer<iAVolumeRenderer> renderer = modalities->Get(idx)->GetRenderer();
	if (!renderer)
	{
		return;
	}
	if ((renderFlagsBefore & iAModality::MainRenderer) == iAModality::MainRenderer
		&& !editModality->hasRenderFlag(iAModality::MainRenderer))
	{
		renderer->Remove();
	}
	if ((renderFlagsBefore & iAModality::MainRenderer) == 0
		&& editModality->hasRenderFlag(iAModality::MainRenderer))
	{
		renderer->AddTo(m_mainRenderer);
	}
	if ((renderFlagsBefore & iAModality::BoundingBox) == iAModality::BoundingBox
		&& !editModality->hasRenderFlag(iAModality::BoundingBox))
	{
		renderer->RemoveBoundingBox();
	}
	if ((renderFlagsBefore & iAModality::BoundingBox) == 0
		&& editModality->hasRenderFlag(iAModality::BoundingBox))
	{
		renderer->AddBoundingBoxTo(m_mainRenderer);
	}
	if ((renderFlagsBefore & iAModality::MagicLens) == iAModality::MagicLens
		&& !editModality->hasRenderFlag(iAModality::MagicLens))
	{
		renderer->Remove();
	}
	if ((renderFlagsBefore & iAModality::MagicLens) == 0
		&& editModality->hasRenderFlag(iAModality::MagicLens))
	{
		renderer->AddTo(m_magicLensWidget->getLensRenderer());
	}
	lwModalities->item(idx)->setText(GetCaption(*editModality));
	emit ModalitiesChanged();
}

void dlg_modalities::EnableButtons()
{
	bool enable = modalities->size() > 0;
	pbEdit->setEnabled(enable);
	pbRemove->setEnabled(enable);
}

void dlg_modalities::ManualRegistration()
{
	vtkInteractorStyleSwitch* interactSwitch = dynamic_cast<vtkInteractorStyleSwitch*>(m_magicLensWidget->GetInteractor()->GetInteractorStyle());
	if (!interactSwitch)
	{
		return;
	}
	if (cbManualRegistration->isChecked())
	{
		interactSwitch->SetCurrentStyleToTrackballActor();
	}
	else
	{
		interactSwitch->SetCurrentStyleToTrackballCamera();
	}
}

void dlg_modalities::ListClicked(QListWidgetItem* item)
{
	int selectedRow = lwModalities->row( item );
	if (selectedRow < 0)
	{
		return;
	}
	QSharedPointer<iAModality> currentData = modalities->Get(selectedRow);
	QSharedPointer<iAModalityTransfer> modTransfer = currentData->GetTransfer();
	SwitchHistogram(modTransfer);
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
	return lwModalities->currentRow();
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
		QSharedPointer<iAVolumeRenderer> renderer = modalities->Get(i)->GetRenderer();
		if (!renderer)
		{
			DEBUG_LOG("ChangeRenderSettings: No Renderer set!");
			return;
		}
		renderer->ApplySettings(rs);
	}
}

void dlg_modalities::RendererMouseMoved()
{
	for (int i = 0; i < modalities->size(); ++i)
	{
		if (!modalities->Get(i)->GetRenderer())
		{
			return;
		}
		modalities->Get(i)->GetRenderer()->UpdateBoundingBox();
	}
}

iAHistogramWidget* dlg_modalities::GetHistogram()
{
	return m_currentHistogram;
}

void dlg_modalities::ShowSlicePlanes(bool enabled)
{
	m_showSlicePlanes = enabled;
	for (int i = 0; i < modalities->size(); ++i)
	{
		QSharedPointer<iAVolumeRenderer> renderer = modalities->Get(i)->GetRenderer();
		if (!renderer)
		{
			DEBUG_LOG("ShowSlicePlanes: No Renderer set!");
			return;
		}
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
	QSharedPointer<iAModality> newModality(new iAModality(name, "", -1, img, iAModality::MainRenderer));
	modalities->Add(newModality);
}
