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
#include "mdichild.h"

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <QStandardItemModel>

iANModalWidget::iANModalWidget(MdiChild *mdiChild) {
	m_mdiChild = mdiChild;
	m_c = new iANModalController(mdiChild);

	m_label = new QLabel("n-Modal Transfer Function");

	QPushButton *button = new QPushButton("Click me!");

	QHBoxLayout *layout = new QHBoxLayout();
	layout->addWidget(m_label);
	layout->addWidget(button);

	setLayout(layout);

	// Connect
	{
		connect(button, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
	}
}

void iANModalWidget::onButtonClicked() {

	QSharedPointer<iAModality> modality = m_mdiChild->modality(0);
	vtkSmartPointer<vtkImageData> image = modality->image();

	QList<LabeledVoxel> voxels;

	{
		QObject *obj = m_mdiChild->findChild<QObject*>("labels");
		dlg_labels* labeling = static_cast<dlg_labels*>(obj);

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
				QString t = item->child(childRow, 0)->text();
				QString nums = t.mid(1, t.size() - 2); // Remove parentheses
				QString nospace = nums.replace(" ", ""); // Remove spaces
				QStringList coords = nospace.split(","); // Separate by comma
				int x = coords[0].toInt();
				int y = coords[1].toInt();
				int z = coords[2].toInt();

				double scalar = image->GetScalarComponentAsDouble(x, y, z, 0);

				auto v = LabeledVoxel();
				v.x = x;
				v.y = y;
				v.z = z;
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