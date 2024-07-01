// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPlaneSliceTool.h"

// guibase
#include <iADataSetRenderer.h>
#include <iADataSetViewer.h>
#include <iADockWidgetWrapper.h>
#include <iAMdiChild.h>
#include <iAMainWindow.h>
#include <iAParameterDlg.h>
#include <iAQCropLabel.h>
#include <iAQVTKWidget.h>
#include <iARenderer.h>
#include <iASlicerMode.h>
#include <iAVolumeViewer.h>
#include <iATransferFunction.h>

// base
#include <iAAABB.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAStringHelper.h>

#include <vtkAbstractTransform.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkConeSource.h>
#include <vtkImageData.h>
#include <vtkImageProperty.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkPlaneWidget.h>
#include <vtkPlane.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkSphereSource.h>

#include <QApplication>
#include <QBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSettings>
#include <QTableWidget>
#include <QToolButton>

class iAvtkPlaneWidget : public vtkPlaneWidget
{
public:
	vtkTypeMacro(iAvtkPlaneWidget, vtkPlaneWidget);
	static iAvtkPlaneWidget* New();

	//! The "automatic" handle size from vtkPlaneWidget somehow doesn't work at all for us - it produces far too small handles for large planes.
	//! Also, it completely ignores the user-settable "HandleSize" and only uses the (internal) HandleSizeFactor for resizing
	//! Workaround: specify a fixed handle size in relation to the dataset size.
	void SizeHandles() override
	{
		for (int i = 0; i < 4; i++)
		{
			this->HandleGeometry[i]->SetRadius(m_handleRadius);
		}
		// Set the height and radius of the cone
		this->ConeSource->SetHeight(2.0 * m_handleRadius);
		this->ConeSource->SetRadius(m_handleRadius);
		this->ConeSource2->SetHeight(2.0 * m_handleRadius);
		this->ConeSource2->SetRadius(m_handleRadius);
	}
	void setHandleRadius(double radius)
	{
		m_handleRadius = radius;
	}
private:
	double m_handleRadius;
};

vtkStandardNewMacro(iAvtkPlaneWidget);

namespace
{
	void transferPlaneParamsToCamera(vtkCamera* cam, vtkPlaneWidget* planeWidget)
	{
		//vtkNew<vtkPlane> p; tool->m_planeWidget->GetPlane(p); tool->m_reslicer->SetSlicePlane(p); -> leads to vtkImageSlice also rotating around
		// instead, we need to change the camera's parameters to adapt where we slice, instead of setting the slice plane in resliceMapper directly;
		// see https://vtkusers.public.kitware.narkive.com/N0oCzOND/setcutplane-not-working-in-vtkimagereslicemapper
		iAVec3d center(planeWidget->GetCenter());
		iAVec3d origin(planeWidget->GetOrigin());
		iAVec3d normal(planeWidget->GetNormal());
		iAVec3d p1(planeWidget->GetPoint1());
		iAVec3d p2(planeWidget->GetPoint2());
		auto distance = cam->GetDistance();
		auto position = center + distance * normal;
		auto viewUp = p1 - origin;
		cam->SetFocalPoint(center.data());
		cam->SetPosition(position.data());
		cam->SetViewUp(viewUp.data());
	}
	enum class TableColumn: int
	{
		ID,
		Position,
		Normal,
		Delete
	};
}

std::shared_ptr<iATool> iAPlaneSliceTool::create(iAMainWindow* mainWnd, iAMdiChild* child)
{
	return std::make_shared<iAPlaneSliceTool>(mainWnd, child);
}

template<typename Layout>
QWidget* createContainerWidget(int spacing)
{
	auto w = new QWidget();
	w->setLayout(new Layout);
	w->layout()->setContentsMargins(0, 0, 0, 0);
	w->layout()->setSpacing(spacing);
	return w;
}

iAPlaneSliceTool::iAPlaneSliceTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_sliceWidget(new iAQVTKWidget(child)),
	m_snapshotTable(new QTableWidget),
	m_curPosLabel(new iAQCropLabel),
	m_sliceDW(new iADockWidgetWrapper(m_sliceWidget, "Arbitrary Slice", "ArbitrarySliceViewer")),
	
	m_planeWidget(vtkSmartPointer<iAvtkPlaneWidget>::New()),
	m_reslicer(vtkSmartPointer<vtkImageResliceMapper>::New()),
	m_imageSlice(vtkSmartPointer<vtkImageSlice>::New()),
	m_nextSnapshotID(1)
{
	auto dwWidget = createContainerWidget<QVBoxLayout>(1);
	m_listDW = new iADockWidgetWrapper(dwWidget, "Snapshot List", "SnapshotList");
	dwWidget->layout()->addWidget(m_curPosLabel);
	auto listWidget = createContainerWidget<QHBoxLayout>(1);

	QStringList columnNames = QStringList() << "ID" << "Position" << "Normal" << "Delete";
	m_snapshotTable->setColumnCount(static_cast<int>(columnNames.size()));
	m_snapshotTable->setHorizontalHeaderLabels(columnNames);
	m_snapshotTable->verticalHeader()->hide();
	m_snapshotTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_snapshotTable->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	m_snapshotTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_snapshotTable->resizeColumnsToContents();
	connect(m_snapshotTable, &QTableWidget::itemSelectionChanged, this, [this, child]()
	{
		auto row = m_snapshotTable->currentRow();
		if (row < 0)
		{
			return;
		}
		auto posItem = m_snapshotTable->item(row, static_cast<int>(TableColumn::Position));
		std::array<double, 3> pos;
		if (!stringToArray(posItem->text(), pos))
		{
			LOG(lvlWarn, QString("Invalid pos %1 in row %2!").arg(posItem->text()).arg(row));
		}
		auto normItem = m_snapshotTable->item(row, static_cast<int>(TableColumn::Normal));
		std::array<double, 3> norm;
		if (!stringToArray(normItem->text(), norm))
		{
			LOG(lvlWarn, QString("Invalid normal %1 in row %2!").arg(normItem->text()).arg(row));
		}
		m_planeWidget->SetCenter(pos.data());
		m_planeWidget->SetNormal(norm.data());
		child->updateRenderer();
		updateSlice();
	});
	listWidget->layout()->addWidget(m_snapshotTable);

	auto buttonContainer = new QWidget();
	buttonContainer->setLayout(new QVBoxLayout);
	buttonContainer->layout()->setContentsMargins(0, 0, 0, 0);
	buttonContainer->layout()->setSpacing(1);

	auto addAction = new QAction("Add");
	addAction->setToolTip("Add snapshot at current position of slicing plane.");
	QObject::connect(addAction, &QAction::triggered, m_snapshotTable, [this]()
	{
		iASnapshotInfo info;
		// convert double information in plane widget to float in iASnapshotInfo
		std::array<double, 3> center, planeNormal;
		m_planeWidget->GetCenter(center.data());
		m_planeWidget->GetNormal(planeNormal.data());
		std::copy(center.begin(), center.end(), info.position.data());
		std::copy(planeNormal.begin(), planeNormal.end(), info.normal.data());
		auto idRow = addSnapshot(info);
		QSignalBlocker b(m_snapshotTable);
		m_snapshotTable->selectRow(idRow.second);
		emit snapshotAdded(idRow.first, info);
	});
	iAMainWindow::get()->addActionIcon(addAction, "plus");
	auto addButton = new QToolButton(buttonContainer);
	addButton->setDefaultAction(addAction);

	auto resetAction = new QAction("Reset");
	resetAction->setToolTip("Reset snapshot position to middle of first image dataset.");
	QObject::connect(resetAction, &QAction::triggered, child, [this, child]()
	{
		iAAttributes params;
		addAttr(params, "Axis", iAValueType::Categorical, QStringList() << "+X" << "-X" << "+Y" << "-Y" << "+Z" << "-Z");
		iAParameterDlg dlg(m_sliceWidget, "Set file parameters", params);
		if (dlg.exec() != QDialog::Accepted)
		{
			return;
		}
		auto axisStr = dlg.parameterValues()["Axis"].toString();
		resetPlaneParameters(child, nameToAxis(axisStr.right(1)), axisStr[0] == '+');
		child->updateRenderer();
		updateSliceFromUser();
	});
	iAMainWindow::get()->addActionIcon(resetAction, "slice-planes-gray");
	auto resetButton = new QToolButton(buttonContainer);
	resetButton->setDefaultAction(resetAction);

	auto clearAction = new QAction("Clear");
	clearAction->setToolTip("Clear (=remove all) snapshots.");
	QObject::connect(clearAction, &QAction::triggered, m_snapshotTable, [this]()
	{
		clearSnapshots();
		emit snapshotsCleared();
	});
	iAMainWindow::get()->addActionIcon(clearAction, "close");
	auto clearButton = new QToolButton(buttonContainer);
	clearButton->setDefaultAction(clearAction);

	buttonContainer->layout()->addWidget(addButton);
	buttonContainer->layout()->addWidget(clearButton);
	buttonContainer->layout()->addWidget(resetButton);
	buttonContainer->setMinimumWidth(20);

	listWidget->layout()->addWidget(buttonContainer);
	dwWidget->layout()->addWidget(listWidget);

	auto ds = child->firstImageDataSetIdx();
	if (ds == iAMdiChild::NoDataSet)
	{
		QMessageBox::warning(mainWnd, "Arbitrary slicing tool", "No image dataset loaded!");
		return;
	}
	child->splitDockWidget(child->slicerDockWidget(iASlicerMode::XY), m_sliceDW, Qt::Horizontal);
	child->splitDockWidget(m_sliceDW, m_listDW, Qt::Vertical);

	//m_planeWidget->SetDefaultRenderer(child->renderer()->renderer());
	resetPlaneParameters(child, iAAxisIndex::Z, true);
	m_planeWidget->SetInteractor(child->renderer()->interactor());
	//m_planeWidget->SetRepresentationToSurface();
	m_planeWidget->On();
	
	// set to middle of object in z direction (i.e. xy slice default position):
	// set handle size to a 20th of the longest dataset size:
	//m_planeWidget->GetHandleProperty()->
	m_planeWidget->SizeHandles();

	m_reslicer->SetInputData(child->firstImageData());
	m_reslicer->SetSliceFacesCamera(true);
	m_reslicer->SetSliceAtFocalPoint(true);

	auto imgProp = m_imageSlice->GetProperty();
	auto viewer = dynamic_cast<iAVolumeViewer*>(m_child->dataSetViewer(ds));
	auto tf = viewer->transfer();
	auto numComp = viewer->volume()->vtkImage()->GetNumberOfScalarComponents();
	imgProp->SetLookupTable((numComp == 1) ? tf->colorTF(): nullptr);

	m_imageSlice->SetMapper(m_reslicer);
	m_imageSlice->SetProperty(imgProp);
	vtkNew<vtkRenderer> ren;
	ren->AddViewProp(m_imageSlice);
	ren->ResetCamera();
	auto bgc = QApplication::palette().color(QPalette::Window);
	ren->SetBackground(bgc.redF(), bgc.greenF(), bgc.blueF());

	m_sliceWidget->renderWindow()->AddRenderer(ren);
	vtkNew<vtkInteractorStyleImage> style;
	m_sliceWidget->renderWindow()->GetInteractor()->SetInteractorStyle(style);
	//m_planeWidget->SetHandleSize(0.1); // no effect, plane widget automatically sets handle sizes

	transferPlaneParamsToCamera(m_sliceWidget->renderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera(), m_planeWidget);
	child->updateRenderer();

	vtkNew<vtkCallbackCommand> modifiedCallback;
	modifiedCallback->SetCallback([](vtkObject* vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId), void* clientData,
		void* vtkNotUsed(callData))
	{
		auto tool = reinterpret_cast<iAPlaneSliceTool*>(clientData);
		tool->updateSliceFromUser();
	});
	modifiedCallback->SetClientData(this);
	m_planeWidget->AddObserver(vtkCommand::InteractionEvent, modifiedCallback);

	updateSlice();
}

iAPlaneSliceTool::~iAPlaneSliceTool()
{
	delete m_listDW;
	delete m_sliceDW;
}

namespace
{
	QString const ProjectSnapshotCount("SnapshotCount");
	QString const ProjectSnapshotBase("Snapshot%1");
	QString const ProjectValuesSeparator(";");
	QString const ProjectPlaneParameters("PlaneParameters");
}

void iAPlaneSliceTool::loadState(QSettings& projectFile, QString const& fileName)
{
	Q_UNUSED(fileName);
	auto count = projectFile.value(ProjectSnapshotCount, 0).toInt();
	for (int s = 0; s < count; ++s)
	{
		auto key = ProjectSnapshotBase.arg(s);
		auto str = projectFile.value(key, "").toString();
		auto parts = str.split(ProjectValuesSeparator);
		if (parts.size() != 3)
		{
			LOG(lvlError, QString("Invalid snapshot spec in entry %1: %2").arg(s).arg(str));
			continue;
		}
		// ID (parts[0]) currently ignored
		iASnapshotInfo info;
		if (!stringToArray(parts[1], info.position) || !stringToArray(parts[2], info.normal))
		{
			LOG(lvlError, QString("Invalid snapshot spec in entry %1: %2 - could not extract position or rotation array!").arg(s).arg(str));
			continue;
		}
		addSnapshot(info);
	}

	auto planeParamStr = projectFile.value(ProjectPlaneParameters, "").toString();
	auto planeParams = planeParamStr.split(ProjectValuesSeparator);
	const int ExpectedPlaneParamParts = 2;
	if (planeParams.size() != ExpectedPlaneParamParts)
	{
		LOG(lvlError, QString("Invalid plane parameter spec: %1: Expected %2 but got %3 parts!")
			.arg(planeParamStr).arg(ExpectedPlaneParamParts).arg(planeParams.size()) );
		return;
	}
	std::array<double, 3> orig, normal;
	if (!stringToArray(planeParams[0], orig) ||
		!stringToArray(planeParams[1], normal))
	{
		LOG(lvlError, QString("Invalid plane parameter spec: %1: Could not parse an array!")
			.arg(planeParamStr).arg(ExpectedPlaneParamParts).arg(planeParams.size()));
		return;
	}
	m_planeWidget->SetOrigin(orig.data());
	m_planeWidget->SetNormal(normal.data());
	updateSliceFromUser();
}

void iAPlaneSliceTool::saveState(QSettings& projectFile, QString const& fileName)
{
	Q_UNUSED(fileName);
	projectFile.setValue(ProjectSnapshotCount, static_cast<quint64>(m_snapshots.size()));
	int storeID = 0;
	for (auto const & s: m_snapshots)
	{
		auto snapshotStr = QString::number(s.first) + ProjectValuesSeparator +
			arrayToString(s.second.position) + ProjectValuesSeparator +
			arrayToString(s.second.normal);
		projectFile.setValue(ProjectSnapshotBase.arg(storeID), snapshotStr);
		++storeID;
	}
	std::array<double, 3> orig, normal;
	m_planeWidget->GetOrigin(orig.data());
	m_planeWidget->GetNormal(normal.data());
	auto planeParamStr = arrayToString(orig) + ProjectValuesSeparator + arrayToString(normal);
	projectFile.setValue(ProjectPlaneParameters, planeParamStr);
}

std::pair<quint64, int> iAPlaneSliceTool::addSnapshot(iASnapshotInfo info)
{
	auto id = m_nextSnapshotID++;
	m_snapshots[id] = info;
	//nameItem->setToolTip();
	auto idItem = new QTableWidgetItem(QString::number(id));
	idItem->setData(Qt::UserRole, id);
	int row = m_snapshotTable->rowCount();
	m_snapshotTable->insertRow(row);
	m_snapshotTable->setItem(row, static_cast<int>(TableColumn::ID), idItem);
	m_snapshotTable->setItem(row, static_cast<int>(TableColumn::Position), new QTableWidgetItem(arrayToString(info.position)));
	m_snapshotTable->setItem(row, static_cast<int>(TableColumn::Normal), new QTableWidgetItem(arrayToString(info.normal)));

	auto actionContainer = createContainerWidget<QHBoxLayout>(1);
	auto removeAction = new QAction("Remove");
	removeAction->setToolTip("Remove snapshot.");
	QObject::connect(removeAction, &QAction::triggered, m_snapshotTable, [this, id]()
	{
		removeSnapshot(id);
		emit snapshotRemoved(id);
	});
	iAMainWindow::get()->addActionIcon(removeAction, "delete");
	auto removeButton = new QToolButton(actionContainer);
	removeButton->setDefaultAction(removeAction);
	actionContainer->layout()->addWidget(removeButton);

	m_snapshotTable->setCellWidget(row, static_cast<int>(TableColumn::Delete), actionContainer);
	m_snapshotTable->resizeColumnsToContents();
	return std::make_pair(id, row);
}

std::map<quint64, iASnapshotInfo> const& iAPlaneSliceTool::snapshots()
{
	return m_snapshots;
}

void iAPlaneSliceTool::removeSnapshot(quint64 id)
{
	m_snapshots.erase(id);
	removeTableEntry(m_snapshotTable, id);
}

void iAPlaneSliceTool::clearSnapshots()
{
	m_snapshots.clear();
	m_snapshotTable->setRowCount(0);
}

void iAPlaneSliceTool::updateSlice()
{
	auto cam = m_sliceWidget->renderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	std::array<double, 3> pos, normal;
	m_planeWidget->GetCenter(pos.data());
	m_planeWidget->GetNormal(normal.data());
	m_curPosLabel->setText(QString("Plane position=(%1), normal=(%2)").arg(arrayToString(pos)).arg(arrayToString(normal)));
	transferPlaneParamsToCamera(cam, m_planeWidget);
	m_sliceWidget->interactor()->Render();
}

void iAPlaneSliceTool::updateSliceFromUser()
{
	updateSlice();
	if (m_snapshotTable->currentRow() != -1)
	{
		QSignalBlocker b(m_snapshotTable);
		m_snapshotTable->setCurrentCell(-1, -1);
	}
}

void iAPlaneSliceTool::resetPlaneParameters(iAMdiChild* child, iAAxisIndex axis, bool posSign)
{
	auto bounds = child->renderer()->sceneBounds();
	auto objCenter = (bounds.maxCorner() - bounds.minCorner()) / 2;

	m_planeWidget->SetOrigin(bounds.minCorner().x(), bounds.minCorner().y(), objCenter.z());
	m_planeWidget->SetPoint1(bounds.maxCorner().x(), bounds.minCorner().y(), objCenter.z());
	m_planeWidget->SetPoint2(bounds.minCorner().x(), bounds.maxCorner().y(), objCenter.z());

	double normal[3] = {
		axis == iAAxisIndex::X ? (posSign ? 1.0 : -1.0) : 0.0,
		axis == iAAxisIndex::Y ? (posSign ? 1.0 : -1.0) : 0.0,
		axis == iAAxisIndex::Z ? (posSign ? 1.0 : -1.0) : 0.0
	};
	m_planeWidget->SetNormal(normal);

	auto lengths = bounds.maxCorner() - bounds.minCorner();
	auto maxSideLen = std::max(std::max(lengths.x(), lengths.y()), lengths.z());
	auto handleSize = maxSideLen / 40.0;
	m_planeWidget->setHandleRadius(handleSize);
}

const QString iAPlaneSliceTool::Name("Arbitrary Slice Plane");



// TODO: better generalization / move to some common qt table widget helper file:
bool removeTableEntry(QTableWidget* tw, quint64 id)
{
	for (int row = 0; row < tw->rowCount(); ++row)
	{
		if (tw->item(row, 0)->data(Qt::UserRole).toULongLong() == id)
		{
			tw->removeRow(row);
			return true;
		}
	}
	return false;
}
