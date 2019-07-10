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
#include "dlg_modalities.h"

#include "dlg_commoninput.h"
#include "dlg_modalityProperties.h"
#include "iAChartFunctionTransfer.h"
#include "iAChannelData.h"
#include "iAChannelSlicerData.h"
#include "iAConsole.h"
#include "iAFast3DMagicLensWidget.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAModalityTransfer.h"
#include "iARenderer.h"
#include "iASlicer.h"
#include "iAVolumeRenderer.h"
#include "iAvtkInteractStyleActor.h"
#include "io/iAIO.h"
#include "io/iAIOProvider.h"
#include "io/extension2id.h"
#include "mdichild.h"

#include <QVTKInteractor.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkVolume.h>

#include <QFileDialog>
#include <QSettings>
#include <QTextDocument>

#include <cassert>


dlg_modalities::dlg_modalities(iAFast3DMagicLensWidget* magicLensWidget,
	vtkRenderer* mainRenderer, MdiChild* mdiChild) :

	m_modalities(new iAModalityList),
	m_magicLensWidget(magicLensWidget),
	m_mainRenderer(mainRenderer),
	m_mdiChild(mdiChild)
{
	for (int i = 0; i <= iASlicerMode::SlicerCount; ++i)
	{
		m_manualMoveStyle[i] = vtkSmartPointer<iAvtkInteractStyleActor>::New();
		connect(m_manualMoveStyle[i], SIGNAL(actorsUpdated()), mdiChild, SLOT(updateViews()));
	}
	connect(pbAdd,    &QPushButton::clicked, this, &dlg_modalities::addClicked);
	connect(pbRemove, &QPushButton::clicked, this, &dlg_modalities::removeClicked);
	connect(pbEdit,   &QPushButton::clicked, this, &dlg_modalities::editClicked);
	connect(cbManualRegistration, &QCheckBox::clicked, this, &dlg_modalities::manualRegistration);
	connect(cbShowMagicLens, &QCheckBox::clicked, this, &dlg_modalities::magicLens);

	connect(lwModalities, SIGNAL(itemClicked(QListWidgetItem*)),
		this, SLOT(listClicked(QListWidgetItem*)));

	connect(lwModalities, SIGNAL(itemChanged(QListWidgetItem*)),
		this, SLOT(showChecked(QListWidgetItem*)));

	connect(magicLensWidget, SIGNAL(MouseMoved()), this, SLOT(rendererMouseMoved()));
}

void dlg_modalities::setModalities(QSharedPointer<iAModalityList> modList)
{
	m_modalities = modList;
	lwModalities->clear();
}

void dlg_modalities::selectRow(int idx)
{
	lwModalities->setCurrentRow(idx);
}

QString GetCaption(iAModality const & mod)
{
	QFileInfo fi(mod.fileName());
	return mod.name()+" ("+fi.fileName()+")";
}

bool CanHaveMultipleChannels(QString const & fileName)
{
	return fileName.endsWith(iAIO::VolstackExtension) || fileName.endsWith(".oif");
}

void dlg_modalities::addClicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load"),
		m_modalities->size() > 0 ? QFileInfo(m_modalities->get(0)->fileName()).absolutePath() : "",
		iAIOProvider::GetSupportedLoadFormats() + tr("Volume Stack (*.volstack);;"));
	if (fileName.isEmpty())
		return;

	const int DefaultRenderFlags = iAModality::MainRenderer;
	bool split = false;
	if (CanHaveMultipleChannels(fileName))
	{
		QStringList inList;
		inList << tr("$Split Channels");
		QList<QVariant> inPara;
		inPara << tr("%1").arg(true);
		QTextDocument descr;
		descr.setHtml("Input file potentially has multiple channels. "
			"Should they be split into separate datasets, "
			"or kept as one dataset with multiple components ?");
		dlg_commoninput splitInput(this, "Seed File Format", inList, inPara, &descr);
		if (splitInput.exec() != QDialog::Accepted)
		{
			DEBUG_LOG("Aborted by user.");
			return;
		}
		split = splitInput.getCheckValue(0);
	}
	ModalityCollection mods = iAModalityList::load(fileName, "", -1, split, DefaultRenderFlags);
	for (auto mod : mods)
	{
		m_modalities->add(mod);
	}
	emit modalitiesChanged(false, nullptr);
}

void dlg_modalities::magicLens()
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

void dlg_modalities::initDisplay(QSharedPointer<iAModality> mod)
{
	QSharedPointer<iAVolumeRenderer> renderer(new iAVolumeRenderer(mod->transfer().data(), mod->image()));
	mod->setRenderer(renderer);
	if (mod->hasRenderFlag(iAModality::MainRenderer))
	{
		renderer->addTo(m_mainRenderer);
	}
	if (mod->hasRenderFlag(iAModality::BoundingBox))
	{
		renderer->addBoundingBoxTo(m_mainRenderer);
	}
	if (mod->hasRenderFlag(iAModality::MagicLens))
	{
		renderer->addTo(m_magicLensWidget->getLensRenderer());
	}
}

void dlg_modalities::addToList(QSharedPointer<iAModality> mod)
{
	QListWidgetItem* listItem = new QListWidgetItem(GetCaption(*mod));
	lwModalities->addItem(listItem);
	lwModalities->setCurrentItem(listItem);
	listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
	listItem->setCheckState(Qt::Checked);
}

void dlg_modalities::addListItem(QSharedPointer<iAModality> mod)
{
	addToList(mod);
	enableButtons();
}

void dlg_modalities::modalityAdded(QSharedPointer<iAModality> mod)
{
	addListItem(mod);
	initDisplay(mod);
	emit modalityAvailable(lwModalities->count()-1);
}

void dlg_modalities::interactorModeSwitched(int newMode)
{
	cbManualRegistration->setChecked(newMode == 'a');
}

void dlg_modalities::enableUI()
{
	pbAdd->setEnabled(true);
	cbManualRegistration->setEnabled(true);
	cbShowMagicLens->setEnabled(true);
}

void dlg_modalities::removeClicked()
{
	int idx = lwModalities->currentRow();
	if (idx < 0 || idx >= m_modalities->size())
	{
		DEBUG_LOG(QString("Index out of range (%1)").arg(idx));
		return;
	}
	m_mdiChild->clearHistogram();
	QSharedPointer<iAVolumeRenderer> renderer = m_modalities->get(idx)->renderer();
	if (m_modalities->get(idx)->hasRenderFlag(iAModality::MainRenderer) ||
		m_modalities->get(idx)->hasRenderFlag(iAModality::MagicLens))
	{
		renderer->remove();
	}
	if (m_modalities->get(idx)->hasRenderFlag(iAModality::BoundingBox))
	{
		renderer->removeBoundingBox();
	}
	m_modalities->remove(idx);
	delete lwModalities->takeItem(idx);
	lwModalities->setCurrentRow(-1);
	enableButtons();

	m_mainRenderer->GetRenderWindow()->Render();
	
	emit modalitiesChanged(false, nullptr);
}

void dlg_modalities::editClicked()
{
	int idx = lwModalities->currentRow();
	if (idx < 0 || idx >= m_modalities->size())
	{
		DEBUG_LOG(QString("Index out of range (%1).").arg(idx));
		return;
	}
	int renderFlagsBefore = m_modalities->get(idx)->renderFlags();
	QSharedPointer<iAModality> editModality(m_modalities->get(idx));
	if (!editModality->renderer())
	{
		DEBUG_LOG(QString("Volume renderer not yet initialized, please wait..."));
		return;
	}
	dlg_modalityProperties prop(this, editModality);
	if (prop.exec() == QDialog::Rejected)
	{
		return;
	}
	QSharedPointer<iAVolumeRenderer> renderer = m_modalities->get(idx)->renderer();
	if (!renderer)
	{
		return;
	}
	if ((renderFlagsBefore & iAModality::MainRenderer) == iAModality::MainRenderer
		&& !editModality->hasRenderFlag(iAModality::MainRenderer))
	{
		renderer->remove();
	}
	if ((renderFlagsBefore & iAModality::MainRenderer) == 0
		&& editModality->hasRenderFlag(iAModality::MainRenderer))
	{
		renderer->addTo(m_mainRenderer);
	}
	if ((renderFlagsBefore & iAModality::BoundingBox) == iAModality::BoundingBox
		&& !editModality->hasRenderFlag(iAModality::BoundingBox))
	{
		renderer->removeBoundingBox();
	}
	if ((renderFlagsBefore & iAModality::BoundingBox) == 0
		&& editModality->hasRenderFlag(iAModality::BoundingBox))
	{
		renderer->addBoundingBoxTo(m_mainRenderer);
	}
	if ((renderFlagsBefore & iAModality::MagicLens) == iAModality::MagicLens
		&& !editModality->hasRenderFlag(iAModality::MagicLens))
	{
		renderer->remove();
	}
	if ((renderFlagsBefore & iAModality::MagicLens) == 0
		&& editModality->hasRenderFlag(iAModality::MagicLens))
	{
		renderer->addTo(m_magicLensWidget->getLensRenderer());
	}
	if ((renderFlagsBefore & iAModality::Slicer) == iAModality::Slicer
		&& !editModality->hasRenderFlag(iAModality::Slicer))
	{
		if (editModality->channelID() != NotExistingChannel)
		{
			m_mdiChild->setSlicerChannelEnabled(editModality->channelID(), false);
			editModality->setChannelID(NotExistingChannel); //reset id to not existing
		}
	}
	if ((renderFlagsBefore & iAModality::Slicer) == 0
		&& editModality->hasRenderFlag(iAModality::Slicer))
	{
		if (editModality->channelID() == NotExistingChannel)
			editModality->setChannelID(m_mdiChild->createChannel());
		m_mdiChild->updateChannel(editModality->channelID(), editModality->image(), editModality->transfer()->colorTF(), editModality->transfer()->opacityTF(), true);
		m_mdiChild->updateChannelOpacity(editModality->channelID(), 1);
		m_mdiChild->updateViews();
	}
	lwModalities->item(idx)->setText(GetCaption(*editModality));
	emit modalitiesChanged(prop.spacingChanged(),prop.newSpacing());
}

void dlg_modalities::enableButtons()
{
	bool enable = m_modalities->size() > 0;
	pbEdit->setEnabled(enable);
	pbRemove->setEnabled(enable);
}

void dlg_modalities::manualRegistration()
{
	try
	{
		int idx = lwModalities->currentRow();
		if (idx < 0 || idx >= m_modalities->size())
		{
			DEBUG_LOG(QString("Index out of range (%1).").arg(idx));
			return;
		}
		QSharedPointer<iAModality> editModality(m_modalities->get(idx));
		
		setModalitySelectionMovable(idx);

		if (!editModality->renderer())
		{
			DEBUG_LOG(QString("Volume renderer not yet initialized, please wait..."));
			return;
		}
		
		if (cbManualRegistration->isChecked())
		{
			configureInterActorStyles(editModality);
			m_mdiChild->renderer()->interactor()->SetInteractorStyle(m_manualMoveStyle[iASlicerMode::SlicerCount]);
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
				m_mdiChild->slicer(i)->GetInteractor()->SetInteractorStyle(m_manualMoveStyle[i]);
		}
		else
		{
			m_mdiChild->renderer()->setDefaultInteractor();
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
				m_mdiChild->slicer(i)->setDefaultInteractor();
		}
	}
	catch (std::invalid_argument &ivae)
	{
		DEBUG_LOG(ivae.what()); 
	}
}

void dlg_modalities::configureInterActorStyles(QSharedPointer<iAModality> editModality)
{
	auto img = editModality->image();
	auto volRend = editModality->renderer().data();
	//vtkProp3D *PropVol_3d = volRend->GetVolume().Get();
	if (!img)
	{
		DEBUG_LOG("img is null!");
		return;
	}
	uint chID = editModality->channelID();

	//properties of slicer for channelID
	iAChannelSlicerData * props[3];
	for (int i=0; i<iASlicerMode::SlicerCount; ++i)
	{
		if (!m_mdiChild->slicer(i)->hasChannel(chID)) {
			DEBUG_LOG("This modality cannot be moved as it isn't active in slicer, please select another one!")
			return;
		}
		else {
			props[i] = m_mdiChild->slicer(i)->channel(chID);
		}
	};

	//intialize slicers and 3D interactor for registration
	for (int i=0; i<= iASlicerMode::SlicerCount; ++i)
		m_manualMoveStyle[i]->initialize(img, volRend, props, i, m_mdiChild);
}

void dlg_modalities::listClicked(QListWidgetItem* item)
{
	int selectedRow = lwModalities->row( item );
	if (selectedRow < 0)
	{
		return;
	}
	setModalitySelectionMovable(selectedRow);
	configureInterActorStyles(m_modalities->get(selectedRow));

	emit modalitySelected(selectedRow);
}

void dlg_modalities::setModalitySelectionMovable(int selectedRow)
{
	QSharedPointer<iAModality> currentData = m_modalities->get(selectedRow);
	//QSharedPointer<iAModalityTransfer> modTransfer = currentData->transfer();
	for (int i = 0; i < m_modalities->size(); ++i)
	{
		QSharedPointer<iAModality> mod = m_modalities->get(i);
		if (!mod->renderer())
		{
			DEBUG_LOG(QString("Renderer for modality %1 not yet created. Please try again later!").arg(i));
			continue;
		}

		//enable / disable dragging
		mod->renderer()->setMovable(mod == currentData);
		
		for (int sl = 0; sl < iASlicerMode::SlicerCount; sl++)
		{
			if (mod->channelID() == NotExistingChannel)
				continue;
			m_mdiChild->slicer(sl)->channel(mod->channelID())->setMovable(currentData->channelID() == mod->channelID());
		}
	}
}

void dlg_modalities::showChecked(QListWidgetItem* item)
{
	int i = lwModalities->row(item);
	QSharedPointer<iAVolumeRenderer> renderer = m_modalities->get(i)->renderer();
	if (!renderer)
	{
		return;
	}
	bool isChecked = item->checkState() == Qt::Checked;
	renderer->showVolume(isChecked);
	m_mainRenderer->GetRenderWindow()->Render();
}

QSharedPointer<iAModalityList const> dlg_modalities::modalities() const
{
	return m_modalities;
}

QSharedPointer<iAModalityList> dlg_modalities::modalities()
{
	return m_modalities;
}

int dlg_modalities::selected() const
{
	return lwModalities->currentRow();
}

vtkSmartPointer<vtkColorTransferFunction> dlg_modalities::colorTF(int modality)
{
	return m_modalities->get(modality)->transfer()->colorTF();
}

vtkSmartPointer<vtkPiecewiseFunction> dlg_modalities::opacityTF(int modality)
{
	return m_modalities->get(modality)->transfer()->opacityTF();
}

void dlg_modalities::changeRenderSettings(iAVolumeSettings const & rs, const bool loadSavedVolumeSettings)
{
	for (int i = 0; i < m_modalities->size(); ++i)
	{
		QSharedPointer<iAVolumeRenderer> renderer = m_modalities->get(i)->renderer();
		if (!renderer)
		{
			DEBUG_LOG("ChangeRenderSettings: No Renderer set!");
			return;
		}
		//load volume settings from file otherwise use default rs
		//check if a volume setting is saved for a modality
		//set saved status to false after loading
		if (loadSavedVolumeSettings  &&
			m_modalities->get(i)->volSettingsSavedStatus())
		{
			renderer->applySettings(m_modalities->get(i)->volumeSettings());
			m_modalities->get(i)->setVolSettingsSavedStatusFalse();
		}
		//use default settings
		else
		{
			renderer->applySettings(rs);
		}
	}
}

void dlg_modalities::rendererMouseMoved()
{
	for (int i = 0; i < m_modalities->size(); ++i)
	{
		if (!m_modalities->get(i)->renderer())
		{
			return;
		}
		m_modalities->get(i)->renderer()->updateBoundingBox();
	}
}

void dlg_modalities::showSlicers(bool enabled, vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3)
{
	for (int i = 0; i < m_modalities->size(); ++i)
	{
		QSharedPointer<iAVolumeRenderer> renderer = m_modalities->get(i)->renderer();
		if (!renderer)
		{
			DEBUG_LOG("ShowSlicePlanes: No Renderer set!");
			return;
		}
		if (enabled)
		{
			renderer->setCuttingPlanes(plane1, plane2, plane3);
		}
		else
		{
			renderer->removeCuttingPlanes();
		}
	}
}

void dlg_modalities::addModality(vtkSmartPointer<vtkImageData> img, QString const & name)
{
	QSharedPointer<iAModality> newModality(new iAModality(name, "", -1, img, iAModality::MainRenderer));
	m_modalities->add(newModality);
}

void dlg_modalities::setFileName(int modality, QString const & fileName)
{
	m_modalities->get(modality)->setFileName(fileName);
	lwModalities->item(modality)->setText(GetCaption(*m_modalities->get(modality).data()));
}
