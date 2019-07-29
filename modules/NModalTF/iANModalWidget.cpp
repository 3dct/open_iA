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

#include "iAConsole.h"
#include "mdichild.h"


// Module interface and Attachment --------------------------------------------------------

iANModalWidgetAttachment::iANModalWidgetAttachment(MainWindow * mainWnd, MdiChild *child) :
	iAModuleAttachmentToChild(mainWnd, child),
	m_nModalWidget(nullptr)
{
	// Do nothing
}

iANModalWidgetAttachment* iANModalWidgetAttachment::create(MainWindow * mainWnd, MdiChild *child) {
	auto newAttachment = new iANModalWidgetAttachment(mainWnd, child);
	return newAttachment;
}

void iANModalWidgetAttachment::start() {
	if (!m_nModalWidget) {
		m_nModalWidget = new iANModalWidget(m_child);
		m_child->tabifyDockWidget(m_child->logDockWidget(), m_nModalWidget);
	}
	m_nModalWidget->show();
	m_nModalWidget->raise();
}


// n-Modal Widget -------------------------------------------------------------------------

#include "iAModality.h"
#include "iAModalityTransfer.h"

#include "dlg_labels.h"

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSharedPointer>

// TEMPORARY
#include <QStandardItemModel>
#include <QObjectList>
#include <QColor>
#include <QStandardItem>

iANModalWidget::iANModalWidget(MdiChild *mdiChild):
	QDockWidget("n-Modal Transfer Function", mdiChild)
{
	m_mdiChild = mdiChild;
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetVerticalTitleBar);

	m_label = new QLabel("n-Modal Transfer Function");

	QPushButton *button = new QPushButton("Click me!");

	QHBoxLayout *layout = new QHBoxLayout();
	layout->addWidget(m_label);
	layout->addWidget(button);

	QWidget *widget = new QWidget();
	widget->setLayout(layout);

	setWidget(widget);

	// Connect
	connect(button, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
}

void iANModalWidget::onButtonClicked() {
	//m_label->setText("Clicked");
	adjustTf();
}

void iANModalWidget::adjustTf() {

	QSharedPointer<iAModality> modality = m_mdiChild->modality(0);
	vtkSmartPointer<vtkImageData> image = modality->image();
	QSharedPointer<iAModalityTransfer> tf = modality->transfer();

	double range[2];
	image->GetScalarRange(range);
	double min = range[0];
	double max = range[1];

	QList<LabeledVoxel> voxels;

	{
		QObject *obj = m_mdiChild->findChild<QObject*>("labels");
		dlg_labels* labeling = static_cast<dlg_labels*>(obj);
		QString text = QString();

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

				text += v.text() + "\n";
			}
		}

		m_label->setText(text);
	}

	tf->colorTF()->RemoveAllPoints();
	tf->colorTF()->AddRGBPoint(min, 0.0, 0.0, 0.0);
	tf->colorTF()->AddRGBPoint(max, 0.0, 0.0, 0.0);

	tf->opacityTF()->RemoveAllPoints();
	tf->opacityTF()->AddPoint(min, 0.0);
	tf->opacityTF()->AddPoint(max, 0.0);

	for (int i = 0; i < voxels.size(); i++) {
		LabeledVoxel v = voxels[i];

		double opacity = v.remover ? 0.0 : 0.5;

		tf->colorTF()->AddRGBPoint(v.scalar, v.r, v.g, v.b);
		tf->opacityTF()->AddPoint(v.scalar, opacity);
	}
}
