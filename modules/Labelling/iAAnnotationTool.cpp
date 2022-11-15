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
#include <iAParameterDlg.h>
#include <iARenderer.h>
#include <iASlicer.h>
#include <iASlicerImpl.h>    // for mapSliceToGlobalAxis

#include <QHeaderView>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCaptionActor2D.h>
#include <vtkCaptionWidget.h>
#include <vtkCaptionRepresentation.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty2D.h>
#include <vtkRendererCollection.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

#include <array>


iAAnnotation::iAAnnotation(size_t id, iAVec3d coord, QString const& name, QColor color):
	m_id(id), m_coord(coord), m_name(name), m_color(color)
{}

const QString iAAnnotationTool::Name = "Annotation";

struct iAVtkAnnotationData
{
	std::array<vtkSmartPointer<vtkCaptionActor2D>, 4> m_txtActor;
};

class iAAnnotationToolUI
{
public:
	iAAnnotationToolUI(iAAnnotationTool* tool):
		m_container(new QWidget),
		m_table(new QTableWidget(m_container)),
		m_dockWidget(new iADockWidgetWrapper(m_container, "Annotations", "dwAnnotations")),
		m_addButton(new QToolButton())
	{
		QStringList columnNames = QStringList() << "" << "Name" << "Coordinates";
		m_table->setColumnCount(columnNames.size());
		m_table->setHorizontalHeaderLabels(columnNames);
		m_table->verticalHeader()->hide();
		m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
		m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		
		auto buttons = new QWidget();
		buttons->setLayout(new QVBoxLayout);
		buttons->layout()->setContentsMargins(0, 0, 0, 0);
		buttons->layout()->setSpacing(4);

		m_addButton->setObjectName("tbAdd");
		m_addButton->setToolTip(QObject::tr("Add a new annotation"));
		buttons->layout()->addWidget(m_addButton);

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

		m_container->setLayout(new QHBoxLayout);
		m_container->layout()->addWidget(m_table);
		m_container->layout()->addWidget(buttons);
		m_container->layout()->setContentsMargins(1, 0, 0, 0);
		m_container->layout()->setSpacing(4);

		QObject::connect(m_addButton, &QToolButton::clicked, tool,
			[tool, this]()
			{
				tool->startAddMode();
			});
		QObject::connect(editButton, &QToolButton::clicked, tool,
			[tool, this]()
			{
				auto rows = m_table->selectionModel()->selectedRows();
				if (rows.size() != 1)
				{
					LOG(lvlWarn, "Please select exactly one row for editing!");
					return;
				}
				int row = rows[0].row();
				auto id = m_table->item(row, 0)->data(Qt::UserRole).toULongLong();

				iAAttributes params;
				addAttr(params, "Name", iAValueType::String, m_table->item(row, 1)->text());
				// would like to pass in tool as parent, but cannot, as it is const...
				iAParameterDlg dlg(nullptr, "Annotation", params);
				if (dlg.exec() == QDialog::Accepted)
				{
					tool->renameAnnotation(id, dlg.parameterValues()["Name"].toString());
				}
			});
		QObject::connect(removeButton, &QToolButton::clicked, tool,
			[tool, this]()
			{
				auto rows = m_table->selectionModel()->selectedRows();
				if (rows.size() != 1)
				{
					LOG(lvlWarn, "Please select exactly one row for editing!");
					return;
				}
				int row = rows[0].row();
				auto id = m_table->item(row, 0)->data(Qt::UserRole).toULongLong();
				tool->removeAnnotation(id);
			});
	}
	QWidget* m_container;
	QTableWidget* m_table;
	iADockWidgetWrapper* m_dockWidget;
	std::vector<iAAnnotation> m_annotations;
	std::map<size_t, iAVtkAnnotationData> m_vtkAnnotateData;
	QToolButton* m_addButton;
};

iAAnnotationTool::iAAnnotationTool(iAMainWindow* mainWin):
	m_ui(std::make_shared<iAAnnotationToolUI>(this))
{
	setMainWindow(mainWin);
}

void iAAnnotationTool::setChild(iAMdiChild* child)
{
	iATool::setChild(child);
	child->splitDockWidget(child->renderDockWidget(), m_ui->m_dockWidget, Qt::Vertical);
}

size_t iAAnnotationTool::addAnnotation(iAVec3d const& coord)
{
	static size_t id = 0;
	QString name = QString("Annotation %1").arg(id + 1);
	QColor col = iAColorThemeManager::instance().theme("Brewer Dark2 (max. 8)")->color(id);
	m_ui->m_annotations.push_back(iAAnnotation(id, coord, name, col));
	++id;
	int row = m_ui->m_table->rowCount();
	m_ui->m_table->insertRow(row);
	auto colorItem = new QTableWidgetItem();
	colorItem->setData(Qt::DecorationRole, col);
	colorItem->setData(Qt::UserRole, id);
	m_ui->m_table->setItem(row, 0, colorItem);
	m_ui->m_table->setItem(row, 1, new QTableWidgetItem(name));
	m_ui->m_table->setItem(row, 2, new QTableWidgetItem(coord.toString()));
	m_ui->m_table->resizeColumnsToContents();

	// Create a text actor.
	iAVtkAnnotationData vtkAnnot;
	for (int i = 0; i < 4; ++i)
	{
		auto txt = vtkSmartPointer<vtkCaptionActor2D>::New();
		txt->SetCaption(name.toStdString().c_str());
		auto prop = txt->GetProperty();
		prop->SetColor(col.redF(), col.greenF(), col.blueF());
		prop->SetLineWidth(1000.0); // does not work, tried 1, 10, 100

		// Does not work; there seems to be a slight thickening of the tip at the moment, but no real arrow visible:
		//vtkNew<vtkArrowSource> arrowSource;
		//arrowSource->SetShaftRadius(10.0);
		//arrowSource->SetTipLength(10.0);
		//arrowSource->Update();
		//txt->SetLeaderGlyphConnection(arrowSource->GetOutputPort());
		//txt->SetLeaderGlyphSize(10);
		//txt->SetMaximumLeaderGlyphSize(10);
		
		double pt[3] = {
			coord[i < 3 ? mapSliceToGlobalAxis(static_cast<iASlicerMode>(i), 0) : 0],
			coord[i < 3 ? mapSliceToGlobalAxis(static_cast<iASlicerMode>(i), 1) : 1],
			i < 3 ? 0: coord[2],
		};
		txt->SetAttachmentPoint(pt);
		txt->SetDisplayPosition(100, 100);    // position relative to attachment point
		// todo: placement outside of object

		txt->BorderOff();
		txt->PickableOn();
		txt->DragableOn();

		txt->GetTextActor()->SetTextScaleModeToNone();
		txt->GetCaptionTextProperty()->SetFontFamily(VTK_ARIAL);
		//txt->GetCaptionTextProperty()->BoldOff();
		txt->GetCaptionTextProperty()->ItalicOff();
		txt->GetCaptionTextProperty()->ShadowOff();
		txt->GetCaptionTextProperty()->SetBackgroundColor(0.0, 0.0, 0.0);
		txt->GetCaptionTextProperty()->SetBackgroundOpacity(0.2);
		txt->GetCaptionTextProperty()->SetColor(col.redF(), col.greenF(), col.blueF());
		txt->GetCaptionTextProperty()->SetFontSize(16);
		txt->GetCaptionTextProperty()->SetFrameWidth(2);
		txt->GetCaptionTextProperty()->SetFrameColor(col.redF(), col.greenF(), col.blueF());
		txt->GetCaptionTextProperty()->FrameOn();
		//txt->GetCaptionTextProperty()->UseTightBoundingBoxOn();
		
		vtkAnnot.m_txtActor[i] = txt;
		auto renWin = (i < 3) ?
			m_mdiChild->slicer(i)->renderWindow() :
			m_mdiChild->renderer()->renderWindow();
		renWin->GetRenderers()->GetFirstRenderer()->AddActor(txt);
	}
	m_ui->m_vtkAnnotateData[id] = vtkAnnot;
	m_mdiChild->updateViews();
	return id;
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
	for (auto row = 0; row < m_ui->m_table->rowCount(); ++row)
	{
		if (m_ui->m_table->item(row, 0)->data(Qt::UserRole) == id)
		{
			m_ui->m_table->item(row, 1)->setText(newName);
		}
	}
	for (int i = 0; i < 4; ++i)
	{
		m_ui->m_vtkAnnotateData[id].m_txtActor[i]->SetCaption(newName.toStdString().c_str());
	}
	m_mdiChild->updateViews();
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
	for (auto row = 0; row < m_ui->m_table->rowCount(); ++row)
	{
		if (m_ui->m_table->item(row, 0)->data(Qt::UserRole) == id)
		{
			m_ui->m_table->removeRow(row);
		}
	}
	for (int j=0; j<3; ++j)
	{
		m_mdiChild->slicer(j)->renderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(m_ui->m_vtkAnnotateData[id].m_txtActor[j]);
	}
	m_mdiChild->renderer()->renderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(m_ui->m_vtkAnnotateData[id].m_txtActor[3]);
	m_ui->m_vtkAnnotateData.erase(id);
	m_mdiChild->updateViews();
}

std::vector<iAAnnotation> const& iAAnnotationTool::annotations() const
{
	return m_ui->m_annotations;
}


void iAAnnotationTool::startAddMode()
{
	m_ui->m_addButton->setDown(true);
	for (int i = 0; i < 3; ++i)
	{
		connect(m_mdiChild->slicer(i), &iASlicer::leftClicked, this, &iAAnnotationTool::slicerPointClicked);
	}
}

void iAAnnotationTool::slicerPointClicked(double x, double y, double z)
{
	m_ui->m_addButton->setDown(false);
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