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
#include "iANModalWidget.h"
#include "iANModalController.h"

#include "iALabellingObjects.h"
#include "iANModalLabelControls.h"

#include "dlg_labels.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iASlicer.h"
#include "dlg_modalities.h"
#include "mdichild.h"

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <QStandardItemModel>
#include <QSizePolicy>
#include <QGridLayout>

iANModalWidget::iANModalWidget(MdiChild *mdiChild) {
	m_mdiChild = mdiChild;
	m_c = new iANModalController(mdiChild);

	// QWidgets
	QWidget* widgetTop = new QWidget();
	QWidget *widgetSlicersGrid = new QWidget();

	// Layouts
	QVBoxLayout *layoutMain = new QVBoxLayout(this);
	QHBoxLayout *layoutTop = new QHBoxLayout(widgetTop);
	m_layoutSlicersGrid = new QGridLayout(widgetSlicersGrid);
	//setLayout(layoutMain);

	// Other widgets
	QLabel *labelTitle = new QLabel("n-Modal Transfer Function");
	m_labelControls = new iANModalLabelControls();
	
	// Settings
	//labelTitle->setSizePolicy(QSizePolicy::Minimum); // DOESN'T WORK!!! WHY???

	QPushButton *buttonRefreshModalities = new QPushButton("Refresh modalities");

	layoutMain->addWidget(widgetTop);
	layoutMain->addWidget(m_labelControls);
	layoutMain->addWidget(widgetSlicersGrid);

	layoutTop->addWidget(labelTitle);
	layoutTop->addWidget(buttonRefreshModalities);

	// Connect
	connect(buttonRefreshModalities, SIGNAL(clicked()), this, SLOT(onButtonRefreshModalitiesClicked()));

	connect(m_labelControls, SIGNAL(labelOpacityChanged(int)), this, SLOT(onLabelOpacityChanged(int)));
	connect(m_labelControls, SIGNAL(labelRemoverStateChanged(int)), this, SLOT(onLabelRemoverStateChanged(int)));

	connect(m_c, SIGNAL(allSlicersInitialized()), this, SLOT(onAllSlicersInitialized()));
	connect(m_c, SIGNAL(allSlicersReinitialized()), this, SLOT(onAllSlicersReinitialized()));

	//connect(m_mdiChild->modalitiesDockWidget(), &dlg_modalities::modalitiesChanged, this, &iANModalWidget::onModalitiesChanged);

	connect(m_c->m_dlg_labels, SIGNAL(seedsAdded(QList<iASeed>)), this, SLOT(onSeedsAdded(QList<iASeed>)));
	connect(m_c->m_dlg_labels, SIGNAL(seedsRemoved(QList<iASeed>)), this, SLOT(onSeedsRemoved(QList<iASeed>)));
	connect(m_c->m_dlg_labels, SIGNAL(labelAdded(iALabel)), this, SLOT(onLabelAdded(iALabel)));
	connect(m_c->m_dlg_labels, SIGNAL(labelRemoved(iALabel)), this, SLOT(onLabelRemoved(iALabel)));
	connect(m_c->m_dlg_labels, SIGNAL(labelsColorChanged(QList<iALabel>)), this, SLOT(onLabelsColorChanged(QList<iALabel>)));

	auto list = m_mdiChild->modalities();
	QList<QSharedPointer<iAModality>> modalities;
	for (int i = 0; i < list->size(); i++) {
		modalities.append(list->get(i));
	}
	modalities = m_c->cherryPickModalities(modalities);
	m_c->setModalities(modalities);
	m_c->initialize();
}

void iANModalWidget::onButtonRefreshModalitiesClicked() {
	auto list = m_mdiChild->modalities();
	QList<QSharedPointer<iAModality>> modalities;
	for (int i = 0; i < list->size(); i++) {
		modalities.append(list->get(i));
	}
	modalities = m_c->cherryPickModalities(modalities);
	m_c->setModalities(modalities);
	m_c->reinitialize();
}

namespace {
	inline void populateLabel(QSharedPointer<iANModalLabel> label, QStandardItem* item, iANModalLabelControls* labelControls, int row) {
		label->id = row;
		label->name = item->text();
		label->color = qvariant_cast<QColor>(item->data(Qt::DecorationRole));
		if (labelControls->containsLabel(row)) {
			label->opacity = labelControls->opacity(label->id);
		} else {
			label->opacity = 1.0f;
		}
	}
}

void iANModalWidget::onAllSlicersInitialized() {
	for (int i = 0; i < m_c->m_slicers.size(); i++) {
		auto slicer = m_c->m_slicers[i];
		m_layoutSlicersGrid->addWidget(slicer, 0, i);
	}
}

void iANModalWidget::onAllSlicersReinitialized() {
	while (auto slicer = m_layoutSlicersGrid->takeAt(0)) {
		delete slicer;
	}
	onAllSlicersInitialized();
}

void iANModalWidget::onSeedsAdded(QList<iASeed> seeds) {
	QHash<int, QList<iANModalSeed>> nmSeeds;
	for (auto seed : seeds) {
		int id = seed.label->id;
		if (!nmSeeds.contains(id)) {
			nmSeeds.insert(id, QList<iANModalSeed>());
		}
		//auto modality = m_c->m_mapOverlayImageId2modality.value(seed.overlayImageId);
		//double scalar = modality->image()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
		nmSeeds.find(id).value().append(iANModalSeed(seed.x, seed.y, seed.z, seed.overlayImageId));
	}
	for (auto ite = nmSeeds.constBegin(); ite != nmSeeds.constEnd(); ite++) {
		int id = ite.key();
		if (m_labels.contains(id)) {
			auto label = m_labels.value(id);
			auto list = ite.value();
			m_c->addSeeds(list, label);
		}
	}
	m_c->update();
}

void iANModalWidget::onSeedsRemoved(QList<iASeed> seeds) {
	QList<iANModalSeed> nmSeeds;
	for (auto seed : seeds) {
		//auto modality = m_c->m_mapOverlayImageId2modality.value(seed.overlayImageId);
		//double scalar = modality->image()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
		nmSeeds.append(iANModalSeed(seed.x, seed.y, seed.z, seed.overlayImageId));
	}
	m_c->removeSeeds(nmSeeds);
	m_c->update();
}

void iANModalWidget::onLabelAdded(iALabel label) {
	auto nmLabel = iANModalLabel(label.id, label.name, false, label.color, 1.0f);
	m_labels.insert(label.id, nmLabel);
	m_labelControls->updateTable(m_labels.values());
}

void iANModalWidget::onLabelRemoved(iALabel label) {
	m_c->removeSeeds(label.id);
	m_labels.remove(label.id);
	m_labelControls->updateTable(m_labels.values());
	m_c->update();
}

void iANModalWidget::onLabelsColorChanged(QList<iALabel> labels) {
	QList<iANModalLabel> nmLabels;
	for (auto label : labels) {
		auto ite = m_labels.find(label.id);
		ite.value().color = label.color;
		float opacity = m_labelControls->opacity(label.id);
		bool remover = m_labelControls->remover(label.id);
		nmLabels.append(iANModalLabel(label.id, label.name, remover, label.color, opacity));
	}
	m_labelControls->updateTable(m_labels.values());
	m_c->updateLabels(nmLabels);
	m_c->update();
}

void iANModalWidget::onLabelOpacityChanged(int labelId) {
	if (m_labels.contains(labelId)) {
		iANModalLabel label = m_labels.value(labelId);
		float opacity = m_labelControls->opacity(labelId);
		if (label.opacity != opacity) {
			label.opacity = opacity;
			//m_labels.remove(labelId);
			m_labels.insert(labelId, label);
			m_c->updateLabel(label);
		}
		m_c->update();
	}
}

void iANModalWidget::onLabelRemoverStateChanged(int labelId) {
	if (m_labels.contains(labelId)) {
		iANModalLabel label = m_labels.value(labelId);
		label.remover = m_labelControls->remover(labelId);
		m_labels.insert(labelId, label);
		m_c->updateLabel(label);
		m_c->update();
	}
}