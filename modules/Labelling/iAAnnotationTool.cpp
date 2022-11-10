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
#include "iAAnnotationTool.h"

#include <iAColorTheme.h>
#include <iALog.h>

#include <iADockWidgetWrapper.h>

#include <iAMdiChild.h>
#include <iASlicer.h>

#include <QStandardItemModel>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>


iAAnnotation::iAAnnotation(size_t id, iAVec3d coord, QString const& name, QColor color):
	m_id(id), m_coord(coord), m_name(name), m_color(color)
{}

const QString iAAnnotationTool::Name = "Annotation";

class iAAnnotationToolUI
{
public:
	iAAnnotationToolUI(iAMdiChild* child, iAAnnotationTool* tool):
		m_annotationList(&m_container),
		m_dockWidget(&m_container, "Annotations", "dwAnnotations")
	{
		m_annotationList.setModel(&m_items);
		
		auto buttons = new QWidget();
		buttons->setLayout(new QVBoxLayout);
		buttons->layout()->setContentsMargins(0, 0, 0, 0);
		buttons->layout()->setSpacing(4);

		m_addButton.setObjectName("tbAdd");
		m_addButton.setToolTip(QObject::tr("Add a new annotation"));
		buttons->layout()->addWidget(&m_addButton);

		auto editButton = new QToolButton();
		editButton->setObjectName("tbEdit");
		editButton->setToolTip(QObject::tr("Edit annotation name"));
		buttons->layout()->addWidget(editButton);

		auto removeButton = new QToolButton();
		removeButton->setObjectName("tbRemove");
		removeButton->setToolTip(QObject::tr("Remove annotation"));
		buttons->layout()->addWidget(removeButton);

		auto spacer = new QSpacerItem(10, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
		buttons->layout()->addItem(spacer);

		m_container.setLayout(new QHBoxLayout);
		m_container.layout()->addWidget(&m_annotationList);
		m_container.layout()->addWidget(buttons);
		m_container.layout()->setContentsMargins(1, 0, 0, 0);
		m_container.layout()->setSpacing(4);

		QObject::connect(&m_addButton, &QToolButton::clicked, tool,
			[tool, this]()
			{
				tool->startAddMode();
				//tool->addAnnotation(iAVec3d());
			});
		QObject::connect(removeButton, &QToolButton::clicked, tool,
			[tool, this]()
			{
				auto rows = m_annotationList.selectionModel()->selectedRows();
				if (rows.size() != 1)
				{
					LOG(lvlWarn, "Please select exactly one row for editing!");
					return;
				}
				int row = rows[0].row();
				auto id = m_items.item(row, 0)->data(Qt::UserRole).toULongLong();
				tool->removeAnnotation(id);
			});
	}
	QWidget m_container;
	QTableView m_annotationList;
	QStandardItemModel m_items;
	iADockWidgetWrapper m_dockWidget;
	std::vector<iAAnnotation> m_annotations;
	QToolButton m_addButton;
};

iAAnnotationTool::iAAnnotationTool(iAMainWindow* mainWin, iAMdiChild* child):
	m_ui(std::make_shared<iAAnnotationToolUI>(child))
{
	setMainWindow(mainWin);
	setChild(child);
}

size_t iAAnnotationTool::addAnnotation(iAVec3d const& coord)
{
	static size_t id = 0;
	QString name = QString("Annotation %1").arg(id + 1);
	QColor col = iAColorThemeManager::instance().theme("Brewer Set3 (max. 12)")->color(id);
	m_ui->m_annotations.push_back(iAAnnotation(id++, coord, name, col));
	QList<QStandardItem*> rowItems = {
		new QStandardItem(),
		new QStandardItem(name),
	};
	rowItems[0]->setData(col, Qt::DecorationRole);
	rowItems[0]->setData(id, Qt::UserRole);
	m_ui->m_items.appendRow(rowItems);
}

void iAAnnotationTool::renameAnnotation(size_t id, QString const& newName)
{
	for (auto a: m_ui->m_annotations)
	{
		if (a.m_id == id)
		{
			a.m_name = newName;
		}
	}
	for (auto row = 0; row < m_ui->m_items.rowCount(); ++row)
	{
		if (m_ui->m_items.item(row, 0)->data(Qt::UserRole) == id)
		{
			m_ui->m_items.item(row, 1)->setText(newName);
		}
	}
}

void iAAnnotationTool::removeAnnotation(size_t id)
{
	for (size_t i=0; i<m_ui->m_annotations.size(); ++i)
	{
		if (m_ui->m_annotations[i].m_id == id)
		{
			m_ui->m_annotations.erase(m_ui->m_annotations.begin() + i);
			break;
		}
	}
	for (auto row = 0; row < m_ui->m_items.rowCount(); ++row)
	{
		if (m_ui->m_items.item(row, 0)->data(Qt::UserRole) == id)
		{
			m_ui->m_items.removeRow(row);
		}
	}
}

std::vector<iAAnnotation> const& iAAnnotationTool::annotations() const
{
	return m_ui->m_annotations;
}


void iAAnnotationTool::startAddMode()
{
	m_ui->m_addButton.setDown(true);
	for (int i = 0; i < 3; ++i)
	{
		connect(m_mdiChild->slicer(i), &iASlicer::leftClicked, this, &iAAnnotationTool::slicerPointClicked);
	}
}

void iAAnnotationTool::slicerPointClicked(double x, double y, double z)
{
	m_ui->m_addButton.setDown(false);
	LOG(lvlInfo, QString("%1, %2, %3").arg(x).arg(y).arg(z));
	addAnnotation(iAVec3d(x, y, z));
	for (int i = 0; i < 3; ++i)
	{
		disconnect(m_mdiChild->slicer(i), &iASlicer::leftClicked, this, &iAAnnotationTool::slicerPointClicked);
	}
}

//std::shared_ptr<iAAnnotationTool> iAAnnotationTool::create()
//{
//	return 
//}