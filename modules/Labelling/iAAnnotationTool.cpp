// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAnnotationTool.h"

#include <iAColorTheme.h>
#include <iALog.h>

#include <iADockWidgetWrapper.h>

#include <iAMdiChild.h>
#include <iAParameterDlg.h>
#include <iARenderer.h>
#include <iASlicer.h>

#include <iAStringHelper.h>

#include <QCheckBox>
#include <QHeaderView>
#include <QSettings>
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
	m_id(id), m_coord(coord), m_name(name), m_color(color), m_show(true)
{}

const QString iAAnnotationTool::Name = "Annotation";

namespace
{
	const double CaptionMinimumOpacity = 0.25;
	const double CaptionBackgroundOpacity = 0.2;
	const int CaptionFrameWidth = 2;
}

struct iAVtkAnnotationData
{
	std::array<vtkSmartPointer<vtkCaptionActor2D>, iASlicerMode::SlicerCount> m_txtActor;
	vtkSmartPointer<vtkCaptionWidget> m_caption3D;
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
		auto columnNames = QStringList() << "" << "Name" << "Coordinates" << "Show";
		m_table->setColumnCount(static_cast<int>(columnNames.size()));
		m_table->setHorizontalHeaderLabels(columnNames);
		m_table->verticalHeader()->hide();
		m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
		m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

		auto buttons = new QWidget();
		buttons->setLayout(new QVBoxLayout);
		buttons->layout()->setContentsMargins(0, 0, 0, 0);
		buttons->layout()->setSpacing(4);

		m_addButton->setObjectName("pbAdd");
		m_addButton->setToolTip(QObject::tr("Add a new annotation"));
		buttons->layout()->addWidget(m_addButton);

		auto editButton = new QToolButton();
		editButton->setObjectName("tbEdit");
		editButton->setToolTip(QObject::tr("Edit annotation name"));
		buttons->layout()->addWidget(editButton);

		auto removeButton = new QToolButton();
		removeButton->setObjectName("pbRemove");
		removeButton->setToolTip(QObject::tr("Remove annotation"));
		buttons->layout()->addWidget(removeButton);

		auto spacer = new QSpacerItem(10, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
		buttons->layout()->addItem(spacer);

		m_container->setLayout(new QHBoxLayout);
		m_container->layout()->addWidget(m_table);
		m_container->layout()->addWidget(buttons);
		m_container->layout()->setContentsMargins(1, 0, 0, 0);
		m_container->layout()->setSpacing(4);

		QObject::connect(m_table, &QTableWidget::cellClicked, tool,
			[tool, this](int row,int /*cell*/)
			{
				auto id = m_table->item(row, 0)->data(Qt::UserRole).toULongLong();
				emit tool->focusedToAnnotation(id);
				tool->focusToAnnotation(id);
			});

		QObject::connect(m_addButton, &QToolButton::clicked, tool,
			[tool]()
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



iAAnnotationTool::iAAnnotationTool(iAMainWindow* mainWnd, iAMdiChild* child):
	iATool(mainWnd,child),
	m_ui(std::make_shared<iAAnnotationToolUI>(this))
{
	child->splitDockWidget(child->renderDockWidget(), m_ui->m_dockWidget, Qt::Vertical);
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		auto s = child->slicer(i);
		QObject::connect(s, &iASlicer::sliceNumberChanged, this, [this, i, s]()
		{
			const double LinearRampFraction = 0.05;
			for (auto& a : m_ui->m_annotations)
			{
				double dist = std::abs(a.m_coord[s->mode()] - s->slicePosition());
				auto sMinMax = s->sliceRange();
				auto sRange = (sMinMax.second - sMinMax.first) * LinearRampFraction;
				double opacity = std::max(CaptionMinimumOpacity, // caption should have a minimum opacity
					1 - dist / sRange);  // linearly decrease based on distance between current slice and annotation
				auto vtkData = m_ui->m_vtkAnnotateData[a.m_id];
				vtkData.m_txtActor[i]->GetProperty()->SetOpacity(opacity);
				vtkData.m_txtActor[i]->GetTextActor()->GetProperty()->SetOpacity(opacity);
				vtkData.m_txtActor[i]->GetCaptionTextProperty()->SetOpacity(opacity);
				vtkData.m_txtActor[i]->GetCaptionTextProperty()->SetFrameWidth(opacity * CaptionFrameWidth);
				vtkData.m_txtActor[i]->GetCaptionTextProperty()->SetBackgroundOpacity(CaptionBackgroundOpacity * (1 - dist / sRange));
			}
		});
	}
}

size_t iAAnnotationTool::addAnnotation(iAVec3d const& coord)
{
	static size_t id = 0;
	auto newID = id;
	++id;
	auto name = QString("Annotation %1").arg(newID);
	QColor col = iAColorThemeManager::instance().theme("Brewer Dark2 (max. 8)")->color(newID);
	addAnnotation(iAAnnotation(newID, coord, name, col));
	m_child->updateViews();
	emit annotationsUpdated(m_ui->m_annotations);
	return newID;
}

void setupCaptionActor(vtkCaptionActor2D* actor, QString const & txt, QColor const & c)
{
	actor->SetCaption(txt.toStdString().c_str());
	auto prop = actor->GetProperty();
	prop->SetColor(c.redF(), c.greenF(), c.blueF());

	actor->BorderOff();      // disable border, as its color cannot be changed (use frame instead, see below)
	actor->PickableOn();
	actor->DragableOn();
	actor->GetTextActor()->SetTextScaleModeToNone();
	//actor->SetPadding(10); // does not seem to have any effect (on distance text <-> frame; probably only on distance text <-> border);

	auto txtProp = actor->GetCaptionTextProperty();
	txtProp->SetFontFamily(VTK_ARIAL);
	//txtProp->BoldOff();
	txtProp->ItalicOff();
	txtProp->ShadowOff();
	txtProp->SetBackgroundColor(0.0, 0.0, 0.0);
	txtProp->SetBackgroundOpacity(CaptionBackgroundOpacity);
	txtProp->SetColor(c.redF(), c.greenF(), c.blueF());
	txtProp->SetFontSize(16);
	txtProp->SetFrameWidth(CaptionFrameWidth);
	txtProp->SetFrameColor(c.redF(), c.greenF(), c.blueF());
	txtProp->FrameOn();
	txtProp->UseTightBoundingBoxOn();  // if not enabled, unevenly distributed distance to frame if text with no descenders
}

void iAAnnotationTool::addAnnotation(iAAnnotation a)
{
	m_ui->m_annotations.push_back(a);
	int row = m_ui->m_table->rowCount();
	m_ui->m_table->insertRow(row);
	auto colorItem = new QTableWidgetItem();
	colorItem->setData(Qt::DecorationRole, a.m_color);
	colorItem->setData(Qt::UserRole, static_cast<quint64>(a.m_id));
	m_ui->m_table->setItem(row, 0, colorItem);
	m_ui->m_table->setItem(row, 1, new QTableWidgetItem(a.m_name));
	m_ui->m_table->setItem(row, 2, new QTableWidgetItem(a.m_coord.toString()));

	auto showCB = new QCheckBox();
	showCB->setStyleSheet("text-align: center; margin-left:50%; margin-right:50%; unchecked{ color: red; }; checked{ color: red; } ");
	m_ui->m_table->setCellWidget(row, 3, showCB);
	m_ui->m_table->resizeColumnsToContents();
	adjustTableItemShown(row, a.m_show);

	QObject::connect(showCB, &QCheckBox::clicked, this,
		[ this]()
		{
			auto rows = m_ui->m_table->selectionModel()->selectedRows();
			if (rows.size() != 1)
			{
				LOG(lvlWarn, "Please select exactly one row for editing!");
				return;
			}
			int row = rows[0].row();
			auto annotation_id = m_ui->m_table->item(row, 0)->data(Qt::UserRole).toULongLong();
			toggleAnnotation(annotation_id);
		});

	iAVtkAnnotationData vtkAnnot;
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		auto actor = vtkSmartPointer<vtkCaptionActor2D>::New();
		actor->GetProperty()->SetLineWidth(3.0); // only works for 2D leader
		actor->SetThreeDimensionalLeader(false);
		double pt[3] = {
			a.m_coord[m_child->slicer(i)->globalAxis(0)],
			a.m_coord[m_child->slicer(i)->globalAxis(1)],
			0,
		};
		actor->SetAttachmentPoint(pt);
		actor->SetDisplayPosition(100, 100);  // position relative to attachment point
		// todo: placement outside of object

		setupCaptionActor(actor, a.m_name, a.m_color);
		vtkAnnot.m_txtActor[i] = actor;
	}
	vtkAnnot.m_caption3D = vtkSmartPointer<vtkCaptionWidget>::New();
	vtkNew<vtkCaptionRepresentation> captionRep;
	captionRep->SetAnchorPosition(a.m_coord.data());
	captionRep->GetCaptionActor2D()->SetAttachmentPoint(a.m_coord.data());
	captionRep->SetFontFactor(0.6);      // necessary for the font size not to be too large (in comparison to slicers)
	vtkAnnot.m_caption3D->SetInteractor(m_child->renderer()->renderWindow()->GetInteractor());
	vtkAnnot.m_caption3D->SetRepresentation(captionRep);
	vtkAnnot.m_caption3D->GetBorderRepresentation()->EnforceNormalizedViewportBoundsOn();
	captionRep->SetPosition(0.8, 0.8);   // upper right cornder
	//captionRep->SetPosition2(0.2, 0.2);  // should set size of annotation, but does not seem to have any affect (maybe it would with BorderOn?)
	setupCaptionActor(captionRep->GetCaptionActor2D(), a.m_name, a.m_color);
	// automatic scaling still done, see https://discourse.vtk.org/t/vtkcaptionwidget-and-text-scaling/13726
	m_ui->m_vtkAnnotateData[a.m_id] = vtkAnnot;
	if (a.m_show)
	{
		showActors(a.m_id, true);
	}
}

void iAAnnotationTool::renameAnnotation(size_t id, QString const& newName)
{
	for (auto &a: m_ui->m_annotations)
	{
		if (a.m_id == id)
		{
			a.m_name = newName;
		}
	}
	for (auto row = 0; row < m_ui->m_table->rowCount(); ++row)
	{
		if (m_ui->m_table->item(row, 0)->data(Qt::UserRole).toULongLong() == id)
		{
			m_ui->m_table->item(row, 1)->setText(newName);
		}
	}
	for (int i = 0; i < m_ui->m_vtkAnnotateData[id].m_txtActor.size(); ++i)
	{
		m_ui->m_vtkAnnotateData[id].m_txtActor[i]->SetCaption(newName.toStdString().c_str());
	}
	m_ui->m_vtkAnnotateData[id].m_caption3D->GetCaptionActor2D()->SetCaption(newName.toStdString().c_str());
	m_child->updateViews();
	emit annotationsUpdated(m_ui->m_annotations);
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
		if (m_ui->m_table->item(row, 0)->data(Qt::UserRole).toULongLong() == id)
		{
			m_ui->m_table->removeRow(row);
		}
	}
	showActors(id, false);
	m_ui->m_vtkAnnotateData.erase(id);
	m_child->updateViews();
	emit annotationsUpdated(m_ui->m_annotations);
}

void iAAnnotationTool::toggleAnnotation(size_t id)
{
	for (size_t i = 0; i < m_ui->m_annotations.size(); ++i)
	{
		if (m_ui->m_annotations[i].m_id == id)
		{
			showAnnotation(id, !m_ui->m_annotations[i].m_show);
			break;
		}
	}
}

void iAAnnotationTool::showAnnotation(size_t id, bool show)
{
	for (size_t i = 0; i < m_ui->m_annotations.size(); ++i)
	{
		if (m_ui->m_annotations[i].m_id == id)
		{
			m_ui->m_annotations[i].m_show = show;
			break;
		}
	}

	for (auto row = 0; row < m_ui->m_table->rowCount(); ++row)
	{
		if (m_ui->m_table->item(row, 0)->data(Qt::UserRole).toULongLong() == id)
		{
			adjustTableItemShown(row, show);
			break;
		}
	}
	showActors(id, show);
	//m_ui->m_vtkAnnotateData.erase(id);
	m_child->updateViews();

	emit annotationsUpdated(m_ui->m_annotations);
}

void iAAnnotationTool::adjustTableItemShown(int row, bool show)
{
	auto color = show ? QColorConstants::Black : QColorConstants::Gray;
	m_ui->m_table->item(row, 1)->setForeground(color);
	m_ui->m_table->item(row, 2)->setForeground(color);
	QCheckBox* checkBox = (QCheckBox*)m_ui->m_table->cellWidget(row, 3);
	checkBox->setChecked(show);
}

void showActor(vtkRenderer* renderer, vtkProp* actor, bool show)
{
	if (show)
	{
		renderer->AddActor(actor);
	}
	else
	{
		renderer->RemoveActor(actor);
	}
}

void iAAnnotationTool::showActors(size_t id, bool show)
{
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		showActor(m_child->slicer(i)->renderWindow()->GetRenderers()->GetFirstRenderer(), m_ui->m_vtkAnnotateData[id].m_txtActor[i], show);
	}
	m_ui->m_vtkAnnotateData[id].m_caption3D->SetEnabled(show);
}

std::vector<iAAnnotation> const& iAAnnotationTool::annotations() const
{
	return m_ui->m_annotations;
}

namespace
{
	const QString PrjAnnotation("Annotation%1");
	const QString FieldSeparator("|");
	const int NumFields = 5;
	QString toString(iAAnnotation const& a)
	{
		return QString("%1").arg(a.m_id) + FieldSeparator +
			QString("%1").arg(arrayToString(a.m_coord.data(), 3)) + FieldSeparator +
			QString("%1").arg(a.m_color.name()) + FieldSeparator +
			iAConverter<bool>::toString(a.m_show) + FieldSeparator +
			a.m_name;
	}
	iAAnnotation fromString(QString const& s)
	{
		auto t = s.split(FieldSeparator);
		if (t.size() < NumFields)
		{
			throw std::runtime_error(QString("Too few fields (%1, expected %2) in annotation %3")
				.arg(t.size()).arg(NumFields).arg(s).toStdString());
		}
		bool ok;
		auto id = iAConverter<size_t>::toT(t[0], &ok);
		if (!ok)
		{
			throw std::runtime_error(QString("Invalid id in annotation %1").arg(s).toStdString());
		}
		iAVec3d coord;
		if (!stringToArray<double>(t[1], coord.data(), 3))
		{
			throw std::runtime_error(QString("Invalid coord in annotation %1").arg(s).toStdString());
		}
		QColor color(t[2]);
		if (!color.isValid())
		{
			throw std::runtime_error(QString("Invalid color in annotation %1").arg(s).toStdString());
		}
		QString name = t[4];
		// special handling if separator used in name:
		qsizetype tIdx = 5;
		while (tIdx < t.size())
		{
			name += FieldSeparator + t[tIdx];
			tIdx += 1;
		}
		auto a = iAAnnotation(id, coord, name, color);
		a.m_show = iAConverter<bool>::toT(t[3], &ok);
		if (!ok)
		{
			throw std::runtime_error(QString("Invalid show value in annotation %1").arg(s).toStdString());
		}
		return a;
	}
}

void iAAnnotationTool::loadState(QSettings& projectFile, QString const& fileName)
{
	Q_UNUSED(fileName);
	try
	{
		size_t annIdx = 0;
		while (projectFile.contains(PrjAnnotation.arg(annIdx)))
		{
			auto a = fromString(projectFile.value(PrjAnnotation.arg(annIdx), "").toString());
			addAnnotation(a);
			++annIdx;
		}
	}
	catch (std::exception& e)
	{
		LOG(lvlError, QString("Invalid annotation information: %1").arg(e.what()));
	}
}

void iAAnnotationTool::saveState(QSettings& projectFile, QString const& fileName)
{
	Q_UNUSED(fileName);
	auto const& annList = annotations();
	size_t annIdx = 0;
	for (auto const& a : annList)
	{
		projectFile.setValue(PrjAnnotation.arg(annIdx++), toString(a));
	}
}

void iAAnnotationTool::startAddMode()
{
	m_ui->m_addButton->setDown(true);
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		connect(m_child->slicer(i), &iASlicer::leftClicked, this, &iAAnnotationTool::slicerPointClicked);
	}
}

void iAAnnotationTool::slicerPointClicked(double x, double y, double z)
{
	m_ui->m_addButton->setDown(false);
	LOG(lvlInfo, QString("New Annotation: %1, %2, %3").arg(x).arg(y).arg(z));
	addAnnotation(iAVec3d(x, y, z));
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		disconnect(m_child->slicer(i), &iASlicer::leftClicked, this, &iAAnnotationTool::slicerPointClicked);
	}
}

void iAAnnotationTool::focusToAnnotation(size_t id)
{
	for (auto annotation : m_ui->m_annotations)
	{
		if (annotation.m_id == id)
		{
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
			{
				m_child->slicer(i)->setSlicePosition(annotation.m_coord[i]);
			}
			break;
		}
	}
}
