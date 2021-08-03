/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "dlg_modalities.h"

#include "dlg_modalityProperties.h"
#include "iAChannelData.h"
#include "iAChannelSlicerData.h"
#include "iALog.h"
#include "iAFast3DMagicLensWidget.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAModalityTransfer.h"
#include "iAParameterDlg.h"
#include "iARenderer.h"
#include "iASlicer.h"
#include "iAVolumeRenderer.h"
#include "iAvtkInteractStyleActor.h"
#include "io/iAIO.h"
#include "io/iAIOProvider.h"
#include "iAMdiChild.h"
#include "ui_modalities.h"

#include <QVTKInteractor.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkVolume.h>

#include <QFileDialog>
#include <QSettings>
#include <QSignalBlocker>

#include <cassert>


dlg_modalities::dlg_modalities(iAFast3DMagicLensWidget* magicLensWidget,
	vtkRenderer* mainRenderer, iAMdiChild* mdiChild) :

	m_modalities(new iAModalityList),
	m_magicLensWidget(magicLensWidget),
	m_mainRenderer(mainRenderer),
	m_mdiChild(mdiChild),
	m_ui(new Ui_modalities())
{
	m_ui->setupUi(this);
	for (int i = 0; i <= iASlicerMode::SlicerCount; ++i)
	{
		m_manualMoveStyle[i] = vtkSmartPointer<iAvtkInteractStyleActor>::New();
		connect(m_manualMoveStyle[i].Get(), &iAvtkInteractStyleActor::actorsUpdated, mdiChild, &iAMdiChild::updateViews);
	}
	connect(m_ui->pbAdd,    &QPushButton::clicked, this, &dlg_modalities::addClicked);
	connect(m_ui->pbRemove, &QPushButton::clicked, this, &dlg_modalities::removeClicked);
	connect(m_ui->pbEdit,   &QPushButton::clicked, this, &dlg_modalities::editClicked);

	connect(m_ui->lwModalities, &QListWidget::itemClicked, this, &dlg_modalities::listClicked);
	connect(m_ui->lwModalities, &QListWidget::itemChanged, this, &dlg_modalities::checkboxClicked);
}

void dlg_modalities::setModalities(QSharedPointer<iAModalityList> modList)
{
	m_modalities = modList;
	m_ui->lwModalities->clear();
}

void dlg_modalities::selectRow(int idx)
{
	m_ui->lwModalities->setCurrentRow(idx);
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
	{
		return;
	}

	const int DefaultRenderFlags = iAModality::MainRenderer;
	bool split = false;
	if (CanHaveMultipleChannels(fileName))
	{
		iAParameterDlg::ParamListT params;
		addParameter(params, "Split Channels", iAValueType::Boolean, true);
		iAParameterDlg dlg(this, "Multi-channel input", params,
			"Input file potentially has multiple channels. "
			"Should they be split into separate datasets, "
			"or kept as one dataset with multiple components ?");
		if (dlg.exec() != QDialog::Accepted)
		{
			LOG(lvlInfo, "Aborted by user.");
			return;
		}
		split = dlg.parameterValues()["Split Channels"].toBool();
	}
	ModalityCollection mods = iAModalityList::load(fileName, "", -1, split, DefaultRenderFlags);
	for (auto mod : mods)
	{
		m_modalities->add(mod);
	}
	emit modalitiesChanged(false, nullptr);
}

void dlg_modalities::initDisplay(QSharedPointer<iAModality> mod)
{
	auto renderer = QSharedPointer<iAVolumeRenderer>::create(mod->transfer().data(), mod->image());
	mod->setRenderer(renderer);
	renderer->applySettings(m_mdiChild->volumeSettings());
	// TODO: Duplication between initDisplay / removeClicked / editClicked
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
	/*
	// don't do this here (yet?) otherwise manually created slicer with no transparency will be overruled and transparency used again in 1st slicer
	if (mod->hasRenderFlag(iAModality::Slicer))
	{
		if (mod->channelID() == NotExistingChannel)
			mod->setChannelID(m_mdiChild->createChannel());
		m_mdiChild->updateChannel(mod->channelID(), mod->image(), mod->transfer()->colorTF(), mod->transfer()->opacityTF(), true);
		m_mdiChild->updateChannelOpacity(mod->channelID(), 1);
		m_mdiChild->updateViews();
	}
	*/
}

void dlg_modalities::addToList(QSharedPointer<iAModality> mod)
{
	QListWidgetItem* listItem = new QListWidgetItem(GetCaption(*mod));
	m_ui->lwModalities->addItem(listItem);
	m_ui->lwModalities->setCurrentItem(listItem);
	listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
	setChecked(listItem, Qt::Checked);
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
	emit modalityAvailable(m_ui->lwModalities->count() - 1);
}

QListWidgetItem* dlg_modalities::item(QSharedPointer<iAModality> modWanted)
{
	for (int i = 0; i < m_modalities->size(); ++i)
	{
		auto modFound = m_modalities->get(i);
		if (modWanted == modFound)
		{
			return m_ui->lwModalities->item(i);
		}
	}
	return nullptr;
}

void dlg_modalities::setChecked(QSharedPointer<iAModality> modality, Qt::CheckState checked)
{
	auto item = this->item(modality);
	setChecked(item, checked);
}

void dlg_modalities::setAllChecked(Qt::CheckState checked)
{
	for (int i = 0; i < m_modalities->size(); ++i) {
		auto mod = m_modalities->get(i);
		auto item = this->item(mod);
		if (item != nullptr) {
			setChecked(item, checked);
		}
	}
}

void dlg_modalities::enableUI()
{
	m_ui->pbAdd->setEnabled(true);
}

void dlg_modalities::removeClicked()
{
	int idx = m_ui->lwModalities->currentRow();
	if (idx < 0 || idx >= m_modalities->size())
	{
		LOG(lvlError, QString("Index out of range (%1)").arg(idx));
		return;
	}
	m_mdiChild->clearHistogram();
	// TODO: Duplication between initDisplay / removeClicked / editClicked
	auto mod = m_modalities->get(idx);
	QSharedPointer<iAVolumeRenderer> renderer = mod->renderer();
	if (mod->hasRenderFlag(iAModality::MainRenderer) ||
		mod->hasRenderFlag(iAModality::MagicLens))
	{
		renderer->remove();
	}
	if (mod->hasRenderFlag(iAModality::BoundingBox))
	{
		renderer->removeBoundingBox();
	}
	if (mod->hasRenderFlag(iAModality::Slicer) && mod->channelID() != NotExistingChannel)
	{
		m_mdiChild->removeChannel(mod->channelID());
	}

	m_modalities->remove(idx);
	delete m_ui->lwModalities->takeItem(idx);
	m_ui->lwModalities->setCurrentRow(-1);
	enableButtons();

	m_mainRenderer->GetRenderWindow()->Render();

	emit modalitiesChanged(false, nullptr);
}

void dlg_modalities::editClicked()
{
	int idx = m_ui->lwModalities->currentRow();
	if (idx < 0 || idx >= m_modalities->size())
	{
		LOG(lvlError, QString("Index out of range (%1).").arg(idx));
		return;
	}
	int renderFlagsBefore = m_modalities->get(idx)->renderFlags();
	QSharedPointer<iAModality> editModality(m_modalities->get(idx));
	if (!editModality->renderer())
	{
		LOG(lvlWarn, QString("Volume renderer not yet initialized, please wait..."));
		return;
	}
	dlg_modalityProperties prop(this, editModality);
	if (prop.exec() == QDialog::Rejected)
	{
		return;
	}
	// TODO: Duplication between initDisplay / removeClicked / editClicked
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
			m_mdiChild->removeChannel(editModality->channelID());
			editModality->setChannelID(NotExistingChannel); //reset id to not existing
		}
	}
	if ((renderFlagsBefore & iAModality::Slicer) == 0
		&& editModality->hasRenderFlag(iAModality::Slicer))
	{
		if (editModality->channelID() == NotExistingChannel)
		{
			editModality->setChannelID(m_mdiChild->createChannel());
		}
		m_mdiChild->updateChannel(editModality->channelID(), editModality->image(), editModality->transfer()->colorTF(), editModality->transfer()->opacityTF(), true);
	}
	m_mdiChild->updateChannelOpacity(editModality->channelID(), editModality->slicerOpacity());
	m_ui->lwModalities->item(idx)->setText(GetCaption(*editModality));
	emit modalitiesChanged(prop.spacingChanged(),prop.newSpacing());
}

void dlg_modalities::enableButtons()
{
	bool enable = m_modalities->size() > 0;
	m_ui->pbEdit->setEnabled(enable);
	m_ui->pbRemove->setEnabled(enable);
}

void dlg_modalities::setInteractionMode(bool manualRegistration)
{
	try
	{
		int idx = m_ui->lwModalities->currentRow();
		if (idx < 0 || idx >= m_modalities->size())
		{
			LOG(lvlError, QString("Index out of range (%1).").arg(idx));
			return;
		}
		QSharedPointer<iAModality> editModality(m_modalities->get(idx));

		setModalitySelectionMovable(idx);

		if (!editModality->renderer())
		{
			LOG(lvlWarn, QString("Volume renderer not yet initialized, please wait..."));
			return;
		}

		if (manualRegistration)
		{
			connect(m_magicLensWidget, &iAFast3DMagicLensWidget::mouseMoved, this, &dlg_modalities::rendererMouseMoved);
			configureInterActorStyles(editModality);
			m_mdiChild->renderer()->interactor()->SetInteractorStyle(m_manualMoveStyle[iASlicerMode::SlicerCount]);
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
			{
				m_mdiChild->slicer(i)->interactor()->SetInteractorStyle(m_manualMoveStyle[i]);
			}
		}
		else
		{
			disconnect(m_magicLensWidget, &iAFast3DMagicLensWidget::mouseMoved, this, &dlg_modalities::rendererMouseMoved);
			m_mdiChild->renderer()->setDefaultInteractor();
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
			{
				m_mdiChild->slicer(i)->setDefaultInteractor();
			}
		}
	}
	catch (std::invalid_argument &ivae)
	{
		LOG(lvlError, ivae.what());
	}
}

void dlg_modalities::configureInterActorStyles(QSharedPointer<iAModality> editModality)
{
	auto img = editModality->image();
	auto volRend = editModality->renderer().data();
	//vtkProp3D *PropVol_3d = volRend->GetVolume().Get();
	if (!img)
	{
		LOG(lvlError, "img is null!");
		return;
	}
	uint chID = editModality->channelID();

	//properties of slicer for channelID
	iAChannelSlicerData * props[3];
	for (int i=0; i<iASlicerMode::SlicerCount; ++i)
	{
		if (!m_mdiChild->slicer(i)->hasChannel(chID))
		{
			LOG(lvlWarn, "This modality cannot be moved as it isn't active in slicer, please select another one!")
			return;
		}
		else
		{
			props[i] = m_mdiChild->slicer(i)->channel(chID);
		}
	}

	//intialize slicers and 3D interactor for registration
	for (int i = 0; i <= iASlicerMode::SlicerCount; ++i)
	{
		m_manualMoveStyle[i]->initialize(img, volRend, props, i, m_mdiChild);
	}
}

void dlg_modalities::listClicked(QListWidgetItem* item)
{
	int selectedRow = m_ui->lwModalities->row(item);
	if (selectedRow < 0)
	{
		return;
	}
	if (m_mdiChild->interactionMode() == iAMdiChild::imRegistration)
	{
		setModalitySelectionMovable(selectedRow);
		configureInterActorStyles(m_modalities->get(selectedRow));
	}
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
			LOG(lvlWarn, QString("Renderer for modality %1 not yet created. Please try again later!").arg(i));
			continue;
		}

		//enable / disable dragging
		mod->renderer()->setMovable(mod == currentData);

		for (int sl = 0; sl < iASlicerMode::SlicerCount; sl++)
		{
			if (mod->channelID() == NotExistingChannel)
			{
				continue;
			}
			m_mdiChild->slicer(sl)->channel(mod->channelID())->setMovable(currentData->channelID() == mod->channelID());
		}
	}
}

void dlg_modalities::checkboxClicked(QListWidgetItem* item) {
	int i = m_ui->lwModalities->row(item);
	auto mod = m_modalities->get(i);
	setModalityVisibility(mod, item->checkState() == Qt::Checked);
}

void dlg_modalities::setChecked(QListWidgetItem* item, Qt::CheckState checked)
{
	QSignalBlocker blocker(m_ui->lwModalities);
	{
		item->setCheckState(checked);
	}
	blocker.unblock();

	int i = m_ui->lwModalities->row(item);
	auto mod = m_modalities->get(i);
	setModalityVisibility(mod, checked == Qt::Checked);
}

void dlg_modalities::setModalityVisibility(QSharedPointer<iAModality> mod, bool visible) {
	QSharedPointer<iAVolumeRenderer> renderer = mod->renderer();
	if (!renderer)
	{
		return;
	}
	renderer->showVolume(visible);
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
	return m_ui->lwModalities->currentRow();
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
			LOG(lvlWarn, "ChangeRenderSettings: No Renderer set!");
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
			LOG(lvlWarn, "ShowSlicePlanes: No Renderer set!");
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
	auto newModality = QSharedPointer<iAModality>::create(name, "", -1, img, iAModality::MainRenderer);
	m_modalities->add(newModality);
}

void dlg_modalities::addModality(QSharedPointer<iAModality> mod) {
	mod->setRenderFlag(iAModality::MainRenderer);
	m_modalities->add(mod);
}

void dlg_modalities::setFileName(int modality, QString const & fileName)
{
	m_modalities->get(modality)->setFileName(fileName);
	m_ui->lwModalities->item(modality)->setText(GetCaption(*m_modalities->get(modality).data()));
}
