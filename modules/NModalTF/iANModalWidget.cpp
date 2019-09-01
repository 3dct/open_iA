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
	QWidget* widgetTop2 = new QWidget();
	QWidget *widgetSlicersGrid = new QWidget();

	// Layouts
	QVBoxLayout *layoutMain = new QVBoxLayout();
	QHBoxLayout *layoutTop = new QHBoxLayout(widgetTop);
	QHBoxLayout *layoutTop2 = new QHBoxLayout(widgetTop2);
	m_layoutSlicersGrid = new QGridLayout(widgetSlicersGrid);
	setLayout(layoutMain);

	// Other widgets
	QLabel *labelTitle = new QLabel("n-Modal Transfer Function");

	
	// Settings
	//labelTitle->setSizePolicy(QSizePolicy::Minimum); // DOESN'T WORK!!! WHY???

	QPushButton *buttonRefreshModalities = new QPushButton("Refresh modalities");
	QPushButton* buttonApplyLabels = new QPushButton("Apply labels");

	layoutMain->addWidget(widgetTop);
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

void iANModalWidget::onButtonClicked() {

	QSharedPointer<iAModality> modality = m_mdiChild->modality(0);
	vtkSmartPointer<vtkImageData> image = modality->image();

	QList<LabeledVoxel> voxels;

	{
		auto labeling = m_c->m_dlg_labels;

		QStandardItemModel *items = labeling->m_itemModel;
		for (int row = 0; row < items->rowCount(); row++) {
			QStandardItem *item = items->item(row, 0);

			if (row == 0) {
				item->setText("Remover");
				item->setData(QColor(0, 0, 0), Qt::DecorationRole);
			}

			QColor color = qvariant_cast<QColor>(item->data(Qt::DecorationRole));
			int count = items->item(row, 1)->text().toInt();
			for (int childRow = 0; childRow < item->rowCount(); childRow++) {
				auto child = item->child(childRow, 0);

				int x = child->data(Qt::UserRole + 1).toInt();
				int y = child->data(Qt::UserRole + 2).toInt();
				int z = child->data(Qt::UserRole + 3).toInt();
				int id = child->data(Qt::UserRole + 4).toInt();

				double scalar = image->GetScalarComponentAsDouble(x, y, z, 0);

				auto v = LabeledVoxel();
				v.x = x;
				v.y = y;
				v.z = z;
				v.id = id;
				v.scalar = scalar;
				v.r = color.redF();
				v.g = color.greenF();
				v.b = color.blueF();
				v.remover = (row == 0);

				voxels.append(v);
			}
		}
	}

	m_c->adjustTf(modality, voxels);
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