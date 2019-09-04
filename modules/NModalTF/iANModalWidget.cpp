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

#include "iANModalObjects.h"
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
	QPushButton* buttonApplyLabels = new QPushButton("Apply labels");

	layoutMain->addWidget(widgetTop);
	layoutMain->addWidget(m_labelControls);
	layoutMain->addWidget(widgetSlicersGrid);

	layoutTop->addWidget(labelTitle);
	layoutTop->addWidget(buttonRefreshModalities);
	layoutTop->addWidget(buttonApplyLabels);

	// Connect
	connect(buttonRefreshModalities, SIGNAL(clicked()), this, SLOT(onButtonRefreshModalitiesClicked()));
	connect(buttonApplyLabels, SIGNAL(clicked()), this, SLOT(onButtonClicked()));

	connect(m_c, SIGNAL(allSlicersInitialized()), this, SLOT(onAllSlicersInitialized()));
	connect(m_c, SIGNAL(allSlicersReinitialized()), this, SLOT(onAllSlicersReinitialized()));

	//connect(m_mdiChild->modalitiesDockWidget(), &dlg_modalities::modalitiesChanged, this, &iANModalWidget::onModalitiesChanged);

	connect(m_c->m_dlg_labels, SIGNAL(seedAdded(int, int, int, iASlicer*)), this, SLOT(onSeedAdded(int, int, int, iASlicer*)));
	connect(m_c->m_dlg_labels, SIGNAL(labelAdded()), this, SLOT(onLabelAdded()));
	connect(m_c->m_dlg_labels, SIGNAL(labelRemoved()), this, SLOT(onLabelRemoved()));

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
	inline void populateLabel(QSharedPointer<iANModalLabelAbstract> label, QStandardItem* item, iANModalLabelControls* labelControls, int row) {
		label->id = row;
		if (row == 0) {
			label->remover = true;
			item->setText("Remover");
			item->setData(QColor(0, 0, 0), Qt::DecorationRole);
		}
		label->name = item->text();
		label->color = qvariant_cast<QColor>(item->data(Qt::DecorationRole));
		if (labelControls->containsLabel(row)) {
			label->opacity = labelControls->opacity(label->id);
		} else {
			label->opacity = 1.0f;
		}
	}
}

void iANModalWidget::onButtonClicked() {
	QList<QSharedPointer<iANModalLabel>> labels;
	QList<QSharedPointer<iANModalLabelAbstract>> labelsSimple;

	{
		auto labeling = m_c->m_dlg_labels;

		QStandardItemModel *items = labeling->m_itemModel;
		for (int row = 0; row < items->rowCount(); row++) {
			QStandardItem *item = items->item(row, 0);

			auto label = QSharedPointer<iANModalLabel>(new iANModalLabel);
			populateLabel(label, item, m_labelControls, row);
			int count = items->item(row, 1)->text().toInt();
			for (int childRow = 0; childRow < item->rowCount(); childRow++) {
				auto child = item->child(childRow, 0);

				int x = child->data(Qt::UserRole + 1).toInt();
				int y = child->data(Qt::UserRole + 2).toInt();
				int z = child->data(Qt::UserRole + 3).toInt();
				int overlayImageId = child->data(Qt::UserRole + 4).toInt();

				auto modality = m_c->m_mapOverlayImageId2modality.value(overlayImageId);
				double scalar = modality->image()->GetScalarComponentAsDouble(x, y, z, 0);

				auto v = iANModalVoxel();
				v.x = x;
				v.y = y;
				v.z = z;
				v.overlayImageId = overlayImageId;
				v.scalar = scalar;

				label->voxels.append(v);
			}
			labels.append(label);


			auto labelSimple = QSharedPointer<iANModalLabelSimple>(new iANModalLabelSimple());
			populateLabel(labelSimple, item, m_labelControls, row);
			labelSimple->voxelsCount = item->rowCount();
			labelsSimple.append(labelSimple);
		}
	}

	m_labelControls->updateTable(labelsSimple);
	m_c->adjustTf(labels);
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

QList<QSharedPointer<iANModalLabelAbstract>> iANModalWidget::labels() {
	QList<QSharedPointer<iANModalLabelAbstract>> labels;
	QStandardItemModel* items = m_c->m_dlg_labels->m_itemModel;
	for (int row = 0; row < items->rowCount(); row++) {
		QStandardItem* item = items->item(row, 0);
		auto label = QSharedPointer<iANModalLabelSimple>(new iANModalLabelSimple());
		populateLabel(label, item, m_labelControls, row);
		label->voxelsCount = item->rowCount();
		labels.append(label);
	}
	return labels;
}

void iANModalWidget::onSeedAdded(int x, int y, int z, iASlicer* slicer) {
	m_labelControls->updateTable(labels());
}

void iANModalWidget::onLabelAdded() {
	m_labelControls->updateTable(labels());
}

void iANModalWidget::onLabelRemoved() {
	m_labelControls->updateTable(labels());
}