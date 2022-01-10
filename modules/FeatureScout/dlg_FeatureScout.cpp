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
#include "dlg_FeatureScout.h"

#include "dlg_blobVisualization.h"
#include "iABlobCluster.h"
#include "iABlobManager.h"
#include "iAClassEditDlg.h"
#include "iAFeatureScoutSPLOM.h"
#include "iAFSColorMaps.h"
#include "iAMeanObject.h"
#include "ui_FeatureScoutClassExplorer.h"
#include "ui_FeatureScoutPolarPlot.h"

#include "iA3DLabelledVolumeVis.h"
#include "iA3DLineObjectVis.h"
#include "iA3DCylinderObjectVis.h"
#include "iA3DNoVis.h"
#include "iA3DEllipseObjectVis.h"
#include "iACsvIO.h"
#include "iAObjectType.h"

#include <dlg_modalities.h>
#include <iAMovieHelper.h>
#include <iAModality.h>
#include <iAModalityTransfer.h>
#include <iAModalityList.h>
#include <iAMdiChild.h>
#include <iAParameterDlg.h>
#include <iAPreferences.h>
#include <iAQVTKWidget.h>
#include <iARenderer.h>
#include <iATypedCallHelper.h>

// qthelper:
#include <iADockWidgetWrapper.h>

// base:
#include <defines.h>    // for DIM
#include <iAConnector.h>
#include <iAFileUtils.h>
#include <iALog.h>
#include <iALookupTable.h>
#include <iAProgress.h>
#include <iAToolsITK.h>

#include <itkImageRegionIterator.h>

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAnnotationLink.h>
#include <vtkAxis.h>
#include <vtkCellData.h>
#include <vtkChart.h>
#include <vtkChartMatrix.h>
#include <vtkChartParallelCoordinates.h>
#include <vtkChartXY.h>
#include <vtkColorTransferFunction.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkDelaunay2D.h>
#include <vtkDynamic2DLabelMapper.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPen.h>
#include <vtkPlot.h>
#include <vtkPlotParallelCoordinates.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarsToColors.h>
#include <vtkStringArray.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredGridGeometryFilter.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>
#include <vtkUnicodeString.h>
#include <vtkVariantArray.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QTableView>
#include <QTreeView>
#include <QtMath>
#include <QXmlStreamWriter>

#include <cmath>

// global defines for using QXmlStream
const QString IFVTag("IFV_Class_Tree");
const QString ClassTag("CLASS");
const QString ObjectTag("OBJECT");
const QString VersionAttribute("Version");
const QString CountAllAttribute("Number_Of_Objects");
const QString NameAttribute("NAME");
const QString ColorAttribute("COLOR");
const QString CountAttribute("COUNT");
const QString PercentAttribute("PERCENT");
const QString IDColumnAttribute("IDColumn");
const QString LabelAttribute("Label");
const QString LabelAttributePore("LabelId");

namespace
{
	QColor StandardSPLOMDotColor(128, 128, 128, 255);

	enum RenderMode
	{
		rmSingleClass,
		rmMultiClass,
		rmOrientation,
		rmLengthDistribution,
		rmMeanObject
	};

	QSharedPointer<iA3DObjectVis> create3DObjectVis(int visualization, iAMdiChild* mdi, vtkTable* table,
		QSharedPointer<QMap<uint, uint> > columnMapping, QColor const& color,
		std::map<size_t, std::vector<iAVec3f> >& curvedFiberInfo, int numberOfCylinderSides, size_t segmentSkip)
	{
		switch (visualization)
		{
		default:
		case iACsvConfig::UseVolume:
			return QSharedPointer<iA3DLabelledVolumeVis>::create(mdi->renderer()->renderer(),
				mdi->modality(0)->transfer()->colorTF(), mdi->modality(0)->transfer()->opacityTF(),
				table, columnMapping, mdi->imagePointer()->GetBounds());
		case iACsvConfig::Lines:
			return QSharedPointer<iA3DLineObjectVis>::create(mdi->renderer()->renderer(), table,
				columnMapping, color, curvedFiberInfo, segmentSkip);
		case iACsvConfig::Cylinders:
			return QSharedPointer<iA3DCylinderObjectVis>::create(mdi->renderer()->renderer(), table,
				columnMapping, color, curvedFiberInfo, numberOfCylinderSides, segmentSkip);
		case iACsvConfig::Ellipses:
			return QSharedPointer<iA3DEllipseObjectVis>::create(mdi->renderer()->renderer(), table,
				columnMapping, color);
		case iACsvConfig::NoVis:
			return QSharedPointer<iA3DNoVis>::create();
		}
	}

	float calculateAverage(vtkDataArray* arr)
	{
		double sum = 0.0;
		for (int i = 0; i < arr->GetNumberOfTuples(); ++i)
		{
			sum = sum + arr->GetVariantValue(i).ToDouble();
		}
		return sum / arr->GetNumberOfTuples();
	}

	QList<QStandardItem*> prepareRow(const QString& first, const QString& second, const QString& third)
	{
		// prepare the class header rows for class tree view first grade child unter rootitem
		// for adding child object to this class, use item.first()->appendRow()
		QList<QStandardItem*> rowItems;
		rowItems << new QStandardItem(first);
		rowItems << new QStandardItem(second);
		rowItems << new QStandardItem(third);
		return rowItems;
	}
}

// define class here directly instead of using iAQTtoUIConnector; when using iAQTtoUIConnector we cannot forward-define because of the templates!
class iAPolarPlotWidget : public QDockWidget, public Ui_FeatureScoutPP
{
public:
	iAPolarPlotWidget(QWidget* parent) : QDockWidget(parent)
	{
		setupUi(this);
	}
};

const int dlg_FeatureScout::PCMinTicksCount = 2;

dlg_FeatureScout::dlg_FeatureScout(iAMdiChild* parent, iAObjectType fid, QString const& fileName,
	vtkSmartPointer<vtkTable> csvtbl, int vis, QSharedPointer<QMap<uint, uint>> columnMapping,
	std::map<size_t, std::vector<iAVec3f>>& curvedFiberInfo, int cylinderQuality, size_t segmentSkip) :
	QDockWidget(parent),
	m_activeChild(parent),
	m_elementCount(csvtbl->GetNumberOfColumns()),
	m_objectCount(csvtbl->GetNumberOfRows()),
	m_filterID(fid),
	m_draw3DPolarPlot(false),
	m_renderMode(rmSingleClass),
	m_singleObjectSelected(false),
	m_visualization(vis),
	m_sourcePath(parent->filePath()),
	m_csvTable(csvtbl),
	m_chartTable(vtkSmartPointer<vtkTable>::New()),
	m_multiClassLUT(vtkSmartPointer<vtkLookupTable>::New()),
	m_classTreeModel(new QStandardItemModel()),
	m_elementTableModel(nullptr),
	m_pcLineWidth(0.1f),
	m_pcFontSize(15),
	m_pcTickCount(10),
	m_pcOpacity(90),
	m_renderer(parent->renderer()),
	m_blobManager(new iABlobManager()),
	m_blobVisDialog(new dlg_blobVisualization()),
	m_dwPC(nullptr),
	m_dwDV(nullptr),
	m_dwSPM(nullptr),
	m_dwPP(nullptr),
	m_ui(new Ui_FeatureScoutCE),
	m_columnMapping(columnMapping),
	m_splom(new iAFeatureScoutSPLOM())
{
	m_ui->setupUi(this);
	this->setupPolarPlotResolution(3.0);

	m_chartTable->DeepCopy(m_csvTable);
	m_tableList.push_back(m_chartTable);

	initFeatureScoutUI();
	m_lengthDistrWidget->hide();

	// Initialize the models for QtViews
	initColumnVisibility();
	setupViews();
	setupModel();
	setupConnections();
	m_3dvis = create3DObjectVis(vis, parent, csvtbl, m_columnMapping, m_colorList.at(0), curvedFiberInfo, cylinderQuality, segmentSkip);
	if (vis != iACsvConfig::UseVolume && m_activeChild->modalities()->size() == 0)
	{
		parent->displayResult(QString("FeatureScout - %1 (%2)").arg(QFileInfo(fileName).fileName())
			.arg(MapObjectTypeToString(m_filterID)), nullptr, nullptr);
	}
	if (vis == iACsvConfig::UseVolume)
	{
		SingleRendering();
	}
	m_3dvis->show();
	parent->renderer()->renderer()->ResetCamera();
	m_blobManager->SetRenderers(parent->renderer()->renderer(), m_renderer->labelRenderer());
	m_blobManager->SetBounds(m_3dvis->bounds());
	m_blobManager->SetProtrusion(1.5);
	int dimens[3] = { 50, 50, 50 };
	m_blobManager->SetDimensions(dimens);
	// set first column of the classTreeView to minimal (not stretched)
	m_classTreeView->resizeColumnToContents(0);
	m_classTreeView->header()->setStretchLastSection(false);
	m_classTreeView->setExpandsOnDoubleClick(false);
	m_elementTableView->resizeColumnsToContents();
}

dlg_FeatureScout::~dlg_FeatureScout()
{
	delete m_blobManager;
	delete m_elementTableModel;
	delete m_classTreeModel;
}

std::vector<size_t> dlg_FeatureScout::getPCSelection()
{
	std::vector<size_t> selectedIndices;
	auto pcSelection = m_pcChart->GetPlot(0)->GetSelection();
	int countSelection = pcSelection->GetNumberOfValues();
	for (int idx = 0; idx < countSelection; ++idx)
	{
		size_t objID = pcSelection->GetVariantValue(idx).ToUnsignedLongLong();
		selectedIndices.push_back(objID);
	}
	return selectedIndices;
}

void dlg_FeatureScout::pcViewMouseButtonCallBack(vtkObject*, unsigned long, void*, void*, vtkCommand*)
{
	auto classSelectionIndices = getPCSelection();
	m_splom->setFilteredSelection(classSelectionIndices);
	// map from indices inside the class to global indices:
	std::vector<size_t> selectionIndices;
	int classID = m_activeClassItem->index().row();
	for (size_t filteredSelIdx : classSelectionIndices)
	{
		size_t labelID = m_tableList[classID]->GetValue(filteredSelIdx, 0).ToUnsignedLongLong();
		selectionIndices.push_back(labelID - 1);
	}
	RenderSelection(selectionIndices);
}

void dlg_FeatureScout::setPCChartData(bool specialRendering)
{
	m_pcWidget->setEnabled(!specialRendering); // to disable selection
	if (specialRendering)
	{   // for the special renderings, we use all data:
		m_chartTable = m_csvTable;
	}
	if (m_pcView->GetScene()->GetNumberOfItems() > 0)
	{
		m_pcView->GetScene()->RemoveItem(0u);
	}
	m_pcChart = vtkSmartPointer<vtkChartParallelCoordinates>::New();
	m_pcChart->GetPlot(0)->SetInputData(m_chartTable);
	m_pcChart->GetPlot(0)->GetPen()->SetOpacity(m_pcOpacity);
	m_pcChart->GetPlot(0)->SetWidth(m_pcLineWidth);
	m_pcView->GetScene()->AddItem(m_pcChart);
	m_pcConnections->Connect(m_pcChart,
		vtkCommand::SelectionChangedEvent,
		this,
		SLOT(pcViewMouseButtonCallBack(vtkObject*, unsigned long, void*, void*, vtkCommand*)));
	updatePCColumnVisibility();
}

void dlg_FeatureScout::updateVisibility(QStandardItem* item)
{
	if (!item->isCheckable())
	{
		return;
	}
	int i = item->index().row();
	m_columnVisibility[i] = (item->checkState() == Qt::Checked);
	updatePCColumnVisibility();
	m_splom->setParameterVisibility(i, m_columnVisibility[i]);
}

void dlg_FeatureScout::spParameterVisibilityChanged(size_t paramIndex, bool enabled)
{
	m_elementTableModel->item(paramIndex, 0)->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
	// itemChanged signal from elementTableModel takes care about updating PC (see updatePCColumnValues slot)
}

void dlg_FeatureScout::renderLUTChanges(QSharedPointer<iALookupTable> lut, size_t colInd)
{
	iA3DLineObjectVis* lov = dynamic_cast<iA3DLineObjectVis*>(m_3dvis.data());
	if (lov)
	{
		lov->setLookupTable(lut, colInd);
	}
}

void dlg_FeatureScout::updatePCColumnVisibility()
{
	for (int j = 0; j < m_elementCount; ++j)
	{
		m_pcChart->SetColumnVisibility(m_csvTable->GetColumnName(j), m_columnVisibility[j]);
	}
	updateAxisProperties();
	m_pcView->Update();
	m_pcView->ResetCamera();
	m_pcView->Render();
}

void dlg_FeatureScout::initColumnVisibility()
{
	m_columnVisibility.resize(m_elementCount);
	std::fill(m_columnVisibility.begin(), m_columnVisibility.end(), false);
	if (m_filterID == iAObjectType::Fibers) // Fibers - (a11, a22, a33,) theta, phi, xm, ym, zm, straightlength, diameter(, volume)
	{
		m_columnVisibility[(*m_columnMapping)[iACsvConfig::Theta]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Phi]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::CenterX]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::CenterY]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::CenterZ]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Length]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Diameter]] = true;
	}
	else if (m_filterID == iAObjectType::Voids) // Pores - (volume, dimx, dimy, dimz,) posx, posy, posz(, shapefactor)
	{
		m_columnVisibility[(*m_columnMapping)[iACsvConfig::CenterX]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::CenterY]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::CenterZ]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Phi]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Theta]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Diameter]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Length]] = true;
	}
}

void dlg_FeatureScout::setupModel()
{
	// setup header data
	m_elementTableModel->setHeaderData(0, Qt::Horizontal, tr("Element"));
	m_elementTableModel->setHeaderData(1, Qt::Horizontal, tr("Minimal"));
	m_elementTableModel->setHeaderData(2, Qt::Horizontal, tr("Maximal"));
	m_elementTableModel->setHeaderData(3, Qt::Horizontal, tr("Average"));

	// initialize checkboxes for the first column
	for (int i = 0; i < m_elementCount - 1; ++i)
	{
		Qt::CheckState checkState = (m_columnVisibility[i]) ? Qt::Checked : Qt::Unchecked;
		m_elementTableModel->setData(m_elementTableModel->index(i, 0, QModelIndex()), checkState, Qt::CheckStateRole);
		m_elementTableModel->item(i, 0)->setCheckable(true);
	}

	// initialize element values
	this->calculateElementTable();
	this->initElementTableModel();

	// set up class tree headeritems
	m_activeClassItem = new QStandardItem();
	m_classTreeModel->setHorizontalHeaderItem(0, new QStandardItem("Class"));
	m_classTreeModel->setHorizontalHeaderItem(1, new QStandardItem("Count"));
	m_classTreeModel->setHorizontalHeaderItem(2, new QStandardItem("Percent"));

	// initialize children for default class
	this->initClassTreeModel();
}

void dlg_FeatureScout::setupViews()
{
	// declare element table model
	m_elementTableModel = new QStandardItemModel(m_elementCount - 1, 4, this);
	m_elementTable = vtkSmartPointer<vtkTable>::New();

	//init Distribution View
	m_dvContextView = vtkSmartPointer<vtkContextView>::New();

	// initialize the QtTableView
	m_classTreeView = new QTreeView();
	m_elementTableView = new QTableView();

	// setup context menu
	m_classTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

	// set Widgets for the table views
	m_ui->ClassLayout->addWidget(m_classTreeView);
	m_ui->ElementLayout->addWidget(m_elementTableView);
	// set models
	m_elementTableView->setModel(m_elementTableModel);
	m_classTreeView->setModel(m_classTreeModel);

	// preparing chart and view for Parallel Coordinates
	m_pcView = vtkSmartPointer<vtkContextView>::New();
	m_lengthDistrView = vtkSmartPointer<vtkContextView>::New();
	m_pcView->SetRenderWindow(m_pcWidget->renderWindow());
	m_pcView->SetInteractor(m_pcWidget->interactor());
	m_lengthDistrView->SetRenderWindow(m_lengthDistrWidget->renderWindow());
	m_lengthDistrView->SetInteractor(m_lengthDistrWidget->interactor());

	// Create a popup menu for Parallel Coordinates:
	QMenu* pcPopupMenu = new QMenu(m_pcWidget);
	auto addClass = pcPopupMenu->addAction("Add class");
	connect(addClass, &QAction::triggered, this, &dlg_FeatureScout::ClassAddButton);
	auto pcSettings = pcPopupMenu->addAction("Settings");
	connect(pcSettings, &QAction::triggered, this, &dlg_FeatureScout::showPCSettings);

	m_pcConnections = vtkSmartPointer<vtkEventQtSlotConnect>::New();
	// Gets right button release event (on a parallel coordinates).
	m_pcConnections->Connect(m_pcWidget->renderWindow()->GetInteractor(),
		vtkCommand::RightButtonReleaseEvent,
		this,
		SLOT(pcRightButtonReleased(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
		pcPopupMenu, 1.0);

	// Gets right button press event (on a scatter plot).
	m_pcConnections->Connect(m_pcWidget->renderWindow()->GetInteractor(),
		vtkCommand::RightButtonPressEvent,
		this,
		SLOT(pcRightButtonPressed(vtkObject*, unsigned long, void*, void*, vtkCommand*)));

	setPCChartData(false);
	updatePolarPlotView(m_chartTable);
}

void dlg_FeatureScout::calculateElementTable()
{
	// free table contents first
	m_elementTable->Initialize();

	double range[2] = { 0, 1 };
	auto nameArr = vtkSmartPointer<vtkStringArray>::New();  // name of columns
	auto v1Arr = vtkSmartPointer<vtkFloatArray >::New();	// minimum
	auto v2Arr = vtkSmartPointer<vtkFloatArray >::New();	// maximal
	auto v3Arr = vtkSmartPointer<vtkFloatArray >::New();	// average
	nameArr->SetNumberOfValues(m_elementCount);
	v1Arr->SetNumberOfValues(m_elementCount);
	v2Arr->SetNumberOfValues(m_elementCount);
	v3Arr->SetNumberOfValues(m_elementCount);

	// convert IDs in String to Int
	nameArr->SetValue(0, m_csvTable->GetColumnName(0));
	v1Arr->SetValue(0, m_chartTable->GetColumn(0)->GetVariantValue(0).ToFloat());
	v2Arr->SetValue(0, m_chartTable->GetColumn(0)->GetVariantValue(m_chartTable->GetNumberOfRows() - 1).ToFloat());
	v3Arr->SetValue(0, 0);

	for (int i = 1; i < m_elementCount; ++i)
	{
		nameArr->SetValue(i, m_csvTable->GetColumnName(i));
		vtkDataArray* mmr = vtkDataArray::SafeDownCast(m_chartTable->GetColumn(i));
		mmr->GetRange(range);
		v1Arr->SetValue(i, range[0]);
		v2Arr->SetValue(i, range[1]);
		v3Arr->SetValue(i, calculateAverage(mmr));
	}

	// add new values
	m_elementTable->AddColumn(nameArr);
	m_elementTable->AddColumn(v1Arr);
	m_elementTable->AddColumn(v2Arr);
	m_elementTable->AddColumn(v3Arr);
}

void dlg_FeatureScout::initElementTableModel(int idx)
{
	// here try to convert the vtkTable values into a QtStandardItemModel
	// this should be done every time when initialize realtime values for elementTable
	if (idx < 0)
	{
		m_elementTableModel->setHeaderData(1, Qt::Horizontal, tr("Minimal"));
		m_elementTableView->showColumn(2);
		m_elementTableView->showColumn(3);

		for (int i = 0; i < m_elementCount - 1; ++i) // number of rows
		{
			for (int j = 0; j < 4; ++j)
			{
				vtkVariant v = m_elementTable->GetValue(i, j);
				QString str;
				if (j == 0)
				{
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 1, 0)
					str = QString::fromUtf8(v.ToUnicodeString().utf8_str()).trimmed();
#else
					str = QString::fromUtf8(v.ToString().c_str()).trimmed();
#endif
				}
				else
				{
					if (i == 0 || i == m_elementCount - 1)
					{
						str = QString::number(v.ToInt());
					}
					else
					{
						str = QString::number(v.ToDouble(), 'f', 2);
					}
				}
				m_elementTableModel->setData(m_elementTableModel->index(i, j, QModelIndex()), str);
				// Surpresses changeability of items.
				m_elementTableModel->item(i, j)->setEditable(false);
			}
		}
	}
	else
	{
		// if the values of objectID is given and bigger than zero
		// we then know that is a single selection, and we want to update the
		// element Table view with the new values
		m_elementTableModel->setHeaderData(1, Qt::Horizontal, tr("Value"));
		m_elementTableView->hideColumn(2);
		m_elementTableView->hideColumn(3);

		for (int i = 0; i < m_elementCount - 1; ++i)
		{
			vtkVariant v = m_chartTable->GetValue(idx, i);
			QString str = QString::number(v.ToDouble(), 'f', 2);

			if (i == 0 || i == m_elementCount - 1)
			{
				str = QString::number(v.ToInt());
			}

			m_elementTableModel->setData(m_elementTableModel->index(i, 1, QModelIndex()), str);
		}
	}
}

void dlg_FeatureScout::initClassTreeModel()
{
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();
	QList<QStandardItem*> stammItem = prepareRow("Unclassified", QString("%1").arg(m_objectCount), "100");
	stammItem.first()->setData(QColor("darkGray"), Qt::DecorationRole);
	m_colorList.push_back(QColor("darkGray"));

	rootItem->appendRow(stammItem);
	for (int i = 0; i < m_objectCount; ++i)
	{
		vtkVariant v = m_chartTable->GetColumn(0)->GetVariantValue(i);
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 1, 0)
		QStandardItem* item = new QStandardItem(QString::fromUtf8(v.ToUnicodeString().utf8_str()).trimmed());
#else
		QStandardItem* item = new QStandardItem(QString::fromUtf8(v.ToString().c_str()).trimmed());
#endif
		stammItem.first()->appendRow(item);
	}
	m_activeClassItem = stammItem.first();
}

// BEGIN DEBUG FUNCTIONS
/*
void dlg_FeatureScout::PrintVTKTable(const vtkSmartPointer<vtkTable> anyTable, const bool useTabSeparator, const QString& outputPath, const QString* fileName) const
{
	std::string separator = (useTabSeparator) ? "\t" : ",";
	std::ofstream debugfile;
	std::string OutfileName = "";
	if (fileName)
	{
		OutfileName = getLocalEncodingFileName(*fileName);
	}
	else
	{
		OutfileName = "debugFile";
	}

	if (!QDir(outputPath).exists() || !anyTable)
	{
		return;
	}

	debugfile.open(getLocalEncodingFileName(outputPath) + OutfileName + ".csv");
	if (!debugfile.is_open())
	{
		return;
	}

	vtkVariant spCol, spRow, spCN, spVal;
	spCol = anyTable->GetNumberOfColumns();
	spRow = anyTable->GetNumberOfRows();

	for (int i = 0; i < spCol.ToInt(); ++i)
	{
		spCN = anyTable->GetColumnName(i);
		debugfile << spCN.ToString() << separator;
	}
	debugfile << "\n";
	for (int row = 0; row < spRow.ToInt(); ++row)
	{
		for (int col = 0; col < spCol.ToInt(); ++col)
		{
			spVal = anyTable->GetValue(row, col);
			debugfile << spVal.ToString() << separator; //TODO cast debug to double
		}
		debugfile << "\n";
	}
	debugfile.close();
}

void dlg_FeatureScout::PrintChartTable(const QString& outputPath)
{
	QString fileName = "chartTable";
	PrintVTKTable(m_chartTable, true, outputPath, &fileName);
}

void dlg_FeatureScout::PrintCSVTable(const QString& outputPath)
{
	QString fileName = "csvTable";
	PrintVTKTable(m_csvTable, true, outputPath, &fileName);
}

void dlg_FeatureScout::PrintTableList(const QList<vtkSmartPointer<vtkTable>>& OutTableList, QString& outputPath) const
{
	QString FileName = "TableClass";
	QString fID = "";
	QString outputFile = "";

	if (OutTableList.count() > 1)
	{
		for (int i = 0; i < m_tableList.count(); ++i)
		{
			fID = QString(i);
			auto outputTable = OutTableList[i];
			outputFile = FileName + "_" + fID;
			this->PrintVTKTable(outputTable, true, outputPath, &outputFile);
			outputTable = nullptr;
		}
	}
}
*/
// END DEBUG FUNCTIONS

void dlg_FeatureScout::setupConnections()
{
	// create ClassTreeView context menu actions
	m_blobRendering = new QAction(tr("Enable blob rendering"), m_classTreeView);
	m_blobRemoveRendering = new QAction(tr("Disable blob rendering"), m_classTreeView);
	m_saveBlobMovie = new QAction(tr("Save blob movie"), m_classTreeView);
	m_objectAdd = new QAction(tr("Add object"), m_classTreeView);
	m_objectDelete = new QAction(tr("Delete object"), m_classTreeView);

	connect(m_blobRendering, &QAction::triggered, this, &dlg_FeatureScout::EnableBlobRendering);
	connect(m_blobRemoveRendering, &QAction::triggered, this, &dlg_FeatureScout::DisableBlobRendering);
	connect(m_saveBlobMovie, &QAction::triggered, this, &dlg_FeatureScout::SaveBlobMovie);
	connect(m_objectAdd, &QAction::triggered, this, &dlg_FeatureScout::addObject);
	connect(m_objectDelete, &QAction::triggered, this, &dlg_FeatureScout::deleteObject);
	connect(m_classTreeView, &QTreeView::customContextMenuRequested, this, &dlg_FeatureScout::showContextMenu);
	connect(m_ui->add_class, &QToolButton::clicked, this, &dlg_FeatureScout::ClassAddButton);
	connect(m_ui->save_class, &QToolButton::released, this, &dlg_FeatureScout::ClassSaveButton);
	connect(m_ui->load_class, &QToolButton::released, this, &dlg_FeatureScout::ClassLoadButton);
	connect(m_ui->delete_class, &QToolButton::clicked, this, &dlg_FeatureScout::ClassDeleteButton);
	connect(m_ui->wisetex_save, &QToolButton::released, this, &dlg_FeatureScout::WisetexSaveButton);
	connect(m_ui->export_class, &QPushButton::clicked, this, &dlg_FeatureScout::ExportClassButton);
	connect(m_ui->csv_dv, &QToolButton::released, this, &dlg_FeatureScout::CsvDVSaveButton);

	connect(m_elementTableModel, &QStandardItemModel::itemChanged, this, &dlg_FeatureScout::updateVisibility);
	connect(m_classTreeView, &QTreeView::clicked, this, &dlg_FeatureScout::classClicked);
	connect(m_classTreeView, &QTreeView::activated, this, &dlg_FeatureScout::classClicked);
	connect(m_classTreeView, &QTreeView::doubleClicked, this, &dlg_FeatureScout::classDoubleClicked);

	connect(m_splom.data(), &iAFeatureScoutSPLOM::selectionModified, this, &dlg_FeatureScout::spSelInformsPCChart);
	connect(m_splom.data(), &iAFeatureScoutSPLOM::addClass, this, &dlg_FeatureScout::ClassAddButton);
	connect(m_splom.data(), &iAFeatureScoutSPLOM::parameterVisibilityChanged, this, &dlg_FeatureScout::spParameterVisibilityChanged);
	connect(m_splom.data(), &iAFeatureScoutSPLOM::renderLUTChanges, this, &dlg_FeatureScout::renderLUTChanges);
}

void dlg_FeatureScout::MultiClassRendering()
{
	showOrientationDistribution();
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();
	int classCount = rootItem->rowCount();

	if (classCount == 1)
	{
		return;
	}
	m_renderMode = rmMultiClass;

	double alpha = this->calculateOpacity(rootItem);
	m_splom->multiClassRendering(m_colorList);
	m_splom->enableSelection(false);

	// update lookup table in PC View
	this->updateLookupTable(alpha);
	this->setPCChartData(true);
	static_cast<vtkPlotParallelCoordinates*>(m_pcChart->GetPlot(0))->SetScalarVisibility(1);
	static_cast<vtkPlotParallelCoordinates*>(m_pcChart->GetPlot(0))->SetLookupTable(m_multiClassLUT);
	static_cast<vtkPlotParallelCoordinates*>(m_pcChart->GetPlot(0))->SelectColorArray(iACsvIO::ColNameClassID);
	m_pcChart->SetSize(m_pcChart->GetSize());
	m_pcChart->GetPlot(0)->SetOpacity(0.8);
	m_pcView->Render();

	m_3dvis->multiClassRendering(m_colorList, rootItem, alpha);
}

void dlg_FeatureScout::SingleRendering(int objectID)
{
	int cID = m_activeClassItem->index().row();
	QColor classColor = m_colorList.at(cID);
	m_3dvis->renderSingle(objectID, cID, classColor, m_activeClassItem);
}

void dlg_FeatureScout::RenderSelection(std::vector<size_t> const& selInds)
{
	showOrientationDistribution();

	if (m_activeClassItem->rowCount() <= 0)
	{
		return;
	}

	auto sortedSelInds = selInds;
	std::sort(sortedSelInds.begin(), sortedSelInds.end());

	int selectedClassID = m_activeClassItem->index().row();
	QColor classColor = m_colorList.at(selectedClassID);
	m_3dvis->renderSelection(sortedSelInds, selectedClassID, classColor, m_activeClassItem);
}

void dlg_FeatureScout::RenderMeanObject()
{
	if (m_visualization != iACsvConfig::UseVolume)
	{
		QMessageBox::warning(this, "FeatureScout", "Mean objects feature only available for the Labelled Volume visualization at the moment!");
		return;
	}
	int classCount = m_classTreeModel->invisibleRootItem()->rowCount();
	if (classCount < 2)	// unclassified class only
	{
		QMessageBox::warning(this, "FeatureScout", "No defined class (except the 'unclassified' class) - please create at least one class first!");
		return;
	}
	m_renderMode = rmMeanObject;
	if (!m_meanObject)
	{
		m_meanObject.reset(new iAMeanObject(m_activeChild, m_sourcePath));
	}
	QStringList classNames;
	for (int c=0; c<classCount; ++c)
	{
		classNames.push_back(m_classTreeModel->invisibleRootItem()->child(c, 0)->text());
	}
	m_meanObject->render(classNames, m_tableList, m_filterID,
		m_dwSPM ? m_dwSPM : (m_dwDV ? m_dwDV : m_dwPC), m_renderer->renderer()->GetActiveCamera(),
		m_colorList);
}

void dlg_FeatureScout::RenderOrientation()
{
	m_renderMode = rmOrientation;
	setPCChartData(true);
	m_splom->enableSelection(false);
	m_splom->setFilter(-1);
	showLengthDistribution(false);
	m_dwPP->setWindowTitle("Orientation Distribution Color Map");

	// define color coding using hsv -> create color palette
	auto oi = vtkSmartPointer<vtkImageData>::New();
	oi->SetExtent(0, 90, 0, 360, 0, 0);
	oi->AllocateScalars(VTK_DOUBLE, 3);

	for (int theta = 0; theta < 91; ++theta)//theta
	{
		for (int phi = 0; phi < 361; ++phi)//phi
		{
			double phi_rad = vtkMath::RadiansFromDegrees((double)phi),
				theta_rad = vtkMath::RadiansFromDegrees((double)theta);
			double recCoord[3] = { sin(theta_rad) * cos(phi_rad),
				sin(theta_rad) * sin(phi_rad),
				cos(theta_rad) };
			double* p = static_cast<double*>(oi->GetScalarPointer(theta, phi, 0));
			vtkMath::Normalize(recCoord);
			getColorMap(m_dwPP->orientationColorMap->currentIndex())(recCoord, p);
		}
	}

	m_3dvis->renderOrientationDistribution(oi);

	// prepare the delaunay triangles
	auto del = vtkSmartPointer<vtkDelaunay2D>::New();
	auto points = vtkSmartPointer<vtkPoints>::New();
	double xx, yy, angle;

	// color array to save the colors for each point
	auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetNumberOfComponents(3);
	colors->SetName("Colors");

	for (int phi = 0; phi < 360; ++phi)
	{
		for (int theta = 0; theta < 91; ++theta)
		{
			angle = phi * M_PI / 180.0;
			xx = theta * cos(angle);
			yy = theta * sin(angle);
			points->InsertNextPoint(xx, yy, 0.0);
			double* p = static_cast<double*>(oi->GetScalarPointer(theta, phi, 0));
			unsigned char color[3];
			for (unsigned int j = 0; j < 3; ++j)
			{
				color[j] = static_cast<unsigned char>(255.0 * p[j]);
			}
			colors->InsertNextTypedTuple(color);
		}
	}

	auto inputPoly = vtkSmartPointer<vtkPolyData>::New();
	inputPoly->SetPoints(points);
	del->SetInputData(inputPoly);
	del->Update();
	vtkPolyData* outputPoly = del->GetOutput();
	outputPoly->GetPointData()->SetScalars(colors);
	auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData(outputPoly);
	auto actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	auto renderer = vtkSmartPointer<vtkRenderer>::New();
	renderer->SetBackground(1, 1, 1);
	renderer->AddActor(actor);
	vtkRenderWindow* renW = m_polarPlotWidget->renderWindow();
	auto ren = renW->GetRenderers()->GetFirstRenderer();
	renW->RemoveRenderer(ren);
	renW->AddRenderer(renderer);
	renderer->ResetCamera();

	// Projection grid and annotations
	this->drawPolarPlotMesh(renderer);
	this->drawAnnotations(renderer);

	m_activeChild->updateViews();
	m_dwPP->colorMapSelection->show();
	renW->Render();
}

void dlg_FeatureScout::RenderLengthDistribution()
{
	m_renderMode = rmLengthDistribution;
	setPCChartData(true);
	m_splom->enableSelection(false);
	m_splom->setFilter(-1);

	double range[2] = { 0.0, 0.0 };
	auto length = vtkDataArray::SafeDownCast(m_csvTable->GetColumn(m_columnMapping->value(iACsvConfig::Length)));
	QString title = QString("%1 Frequency Distribution").arg(m_csvTable->GetColumnName(m_columnMapping->value(iACsvConfig::Length)));
	m_dwPP->setWindowTitle(title);
	int numberOfBins = (m_filterID == iAObjectType::Fibers) ? 8 : 3;  // TODO: setting?

	length->GetRange(range);
	if (range[0] == range[1])
	{
		range[1] = range[0] + 1.0;
	}

	double inc = (range[1] - range[0]) / (numberOfBins) * 1.001;
	double halfInc = inc / 2.0;

	auto extents = vtkSmartPointer<vtkFloatArray>::New();
	extents->SetName("Length [um]");
	extents->SetNumberOfTuples(numberOfBins);

	float* centers = static_cast<float*>(extents->GetVoidPointer(0));
	double min = range[0] - 0.0005 * inc + halfInc;

	for (int j = 0; j < numberOfBins; ++j)
	{
		extents->SetValue(j, min + j * inc);
	}

	auto populations = vtkSmartPointer<vtkIntArray>::New();
	populations->SetName("Probability");
	populations->SetNumberOfTuples(numberOfBins);
	int* pops = static_cast<int*>(populations->GetVoidPointer(0));

	for (int k = 0; k < numberOfBins; ++k)
	{
		pops[k] = 0;
	}

	for (vtkIdType j = 0; j < length->GetNumberOfTuples(); ++j)
	{
		double v = 0.0;
		length->GetTuple(j, &v);

		for (int k = 0; k < numberOfBins; ++k)
		{
			if (vtkMathUtilities::FuzzyCompare(v, double(centers[k]), halfInc))
			{
				++pops[k];
				break;
			}
		}
	}

	auto fldTable = vtkSmartPointer<vtkTable>::New();
	fldTable->AddColumn(extents.GetPointer());
	fldTable->AddColumn(populations.GetPointer());

	//Create a transfer function mapping scalar value to color
	auto cTFun = vtkSmartPointer<vtkColorTransferFunction>::New();
	cTFun->SetColorSpaceToRGB();
	if (m_filterID == iAObjectType::Fibers)
	{
		cTFun->AddRGBPoint(range[0], 1.0, 0.6, 0.0);	//orange
		cTFun->AddRGBPoint(extents->GetValue(0) + halfInc, 1.0, 0.0, 0.0); //red
		cTFun->AddRGBPoint(extents->GetValue(1) + halfInc, 1.0, 0.0, 1.0); //magenta
		cTFun->AddRGBPoint(extents->GetValue(2) + halfInc, 0.0, 0.0, 1.0); //blue
		cTFun->AddRGBPoint(extents->GetValue(7) + halfInc, 0.0, 1.0, 0.7); //cyan
	}
	else
	{
		cTFun->AddRGBPoint(range[0], 1.0, 0.6, 0.0);	//orange
		cTFun->AddRGBPoint(extents->GetValue(1) + halfInc, 1.0, 0.0, 1.0); //magenta
		cTFun->AddRGBPoint(extents->GetValue(2) + halfInc, 0.0, 1.0, 0.7); //cyan
	}

	m_3dvis->renderLengthDistribution(cTFun, extents, halfInc, m_filterID, range);

	m_dwPP->colorMapSelection->hide();
	showLengthDistribution(true, cTFun);
	m_renderer->update();

	// plot length distribution
	auto chart = vtkSmartPointer<vtkChartXY>::New();
	chart->SetTitle(title.toUtf8().constData());
	chart->GetTitleProperties()->SetFontSize((m_filterID == iAObjectType::Fibers) ? 15 : 12); // TODO: setting?
	vtkPlot* plot = chart->AddPlot(vtkChartXY::BAR);
	plot->SetInputData(fldTable, 0, 1);
	plot->GetXAxis()->SetTitle("Length in microns");
	plot->GetYAxis()->SetTitle("Frequency");
	m_lengthDistrView->GetScene()->ClearItems();
	m_lengthDistrView->GetScene()->AddItem(chart);
	m_lengthDistrWidget->renderWindow()->Render();
	m_lengthDistrWidget->update();
	m_dwPP->legendLayout->removeWidget(m_polarPlotWidget);
	m_dwPP->legendLayout->addWidget(m_lengthDistrWidget);
	m_polarPlotWidget->hide();
	m_lengthDistrWidget->show();
}

void dlg_FeatureScout::ClassAddButton()
{
	vtkIdTypeArray* pcSelection = m_pcChart->GetPlot(0)->GetSelection();
	int CountObject = pcSelection->GetNumberOfTuples();
	if (CountObject <= 0)
	{
		QMessageBox::warning(this, "FeatureScout", "No object was selected!");
		return;
	}
	if (CountObject == m_activeClassItem->rowCount() && m_activeClassItem->index().row() != 0)
	{
		QMessageBox::warning(this, "FeatureScout", "All items in current class are selected. There is no need to create a new class out of them. Please select only a subset of items!");
		return;
	}
	if (m_renderMode != rmSingleClass)
	{
		QMessageBox::warning(this, "FeatureScout", "Cannot add a class while in a special rendering mode "
			"(Multi-Class, Fiber Length/Orientation Distribution). "
			"Please click on a class first!");
		return;
	}
	// class name and color
	int cid = m_classTreeModel->invisibleRootItem()->rowCount();
	QString cText = QString("Class %1").arg(cid);
	QColor cColor = getClassColor(cid);

	bool ok;
	cText = iAClassEditDlg::getClassInfo("FeatureScout: Add Class", cText, cColor, ok);
	if (!ok)
	{
		return;
	}
	m_colorList.push_back(cColor);
	// get the root item from class tree
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();
	QStandardItem* item;

	// create a first level child under rootItem as new class
	double percent = 100.0 * CountObject / m_objectCount;
	QList<QStandardItem*> firstLevelItem = prepareRow(cText, QString("%1").arg(CountObject), QString::number(percent, 'f', 1));
	firstLevelItem.first()->setData(cColor, Qt::DecorationRole);

	int classID = rootItem->rowCount();
	int objID = 0;
	QList<int> kIdx; // list to regist the selected index of object IDs in m_activeClassItem

	// add new class
	for (int i = 0; i < CountObject; ++i)
	{
		// get objID from item->text()
		vtkVariant v = pcSelection->GetVariantValue(i);
		objID = v.ToInt() + 1;	//fibre index starting at 1 not at 0
		objID = m_activeClassItem->child(v.ToInt())->text().toInt();

		if (kIdx.contains(v.ToInt()))
		{
			LOG(lvlWarn, QString("Tried to add objID=%1, v=%2 to class which is already contained in other class!").arg(objID).arg(v.ToInt()));
		}
		else
		{
			kIdx.prepend(v.ToInt());

			// add item to the new class
			QString str = QString("%1").arg(objID);
			item = new QStandardItem(str);
			firstLevelItem.first()->appendRow(item);

			m_csvTable->SetValue(objID - 1, m_elementCount - 1, classID); // update Class_ID column in csvTable
		}
	}

	// a simple check of the selections
	if (kIdx.isEmpty())
	{
		QMessageBox::information(this, "FeatureScout", "Selected objects are already classified, please make a new selection.");
		return;
	}

	if (kIdx.count() != CountObject)
	{
		QMessageBox::warning(this, "FeatureScout", "Selection Error, please make a new selection.");
		return;
	}
	m_splom->classAdded(classID);

	// append the new class to class tree
	rootItem->appendRow(firstLevelItem);

	// remove items from m_activeClassItem from table button to top, otherwise you would make a wrong delete
	for (int i = 0; i < CountObject; ++i)
	{
		m_activeClassItem->removeRow(kIdx.value(i));
	}

	updateClassStatistics(m_activeClassItem);
	setActiveClassItem(firstLevelItem.first(), 1);
	calculateElementTable();
	initElementTableModel();
	setPCChartData();
	m_classTreeView->collapseAll();
	m_classTreeView->setCurrentIndex(firstLevelItem.first()->index());
	updatePolarPlotView(m_chartTable);
	SingleRendering();
}

void dlg_FeatureScout::writeWisetex(QXmlStreamWriter* writer)
{
	if (QMessageBox::warning(this, "FeatureScout",
		"This functionality is only available for FCP fiber/ feature characteristics pore csv formats at the moment. Are you sure you want to proceed?",
		QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
	{
		return;
	}
	// Write XML tags using WiseTex specification
	//check if it is a class item
	if (m_classTreeModel->invisibleRootItem()->hasChildren())
	{
		if (m_filterID == iAObjectType::Fibers)
		{
			writer->writeStartElement("FibreClasses"); //start FibreClasses tag

			for (int i = 0; i < m_classTreeModel->invisibleRootItem()->rowCount(); ++i)
			{
				//Gets the fibre class
				QStandardItem* fc = m_classTreeModel->invisibleRootItem()->child(i, 1);

				writer->writeStartElement(QString("FibreClass-%1").arg(i)); //start FibreClass-n tag
				writer->writeTextElement("CColor", QString("0x00").append
				(QString(m_colorList.at(fc->index().row()).name()).remove(0, 1))); //CColor tag
				writer->writeTextElement("NFibres", QString(fc->text())); //NFibres tag

				writer->writeStartElement(QString("Fibres")); //Fibres tag

				for (int j = 0; j < m_classTreeModel->invisibleRootItem()->child(i)->rowCount(); ++j)
				{
					writer->writeStartElement(QString("Fibre-%1").arg(j + 1)); //Fibre-n tag

					//Gets fibre features from csvTable
					writer->writeTextElement("X1", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::StartX)).ToString()));
					writer->writeTextElement("Y1", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::StartY)).ToString()));
					writer->writeTextElement("Z1", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::StartZ)).ToString()));
					writer->writeTextElement("X1", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::EndX)).ToString()));
					writer->writeTextElement("Y2", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::EndY)).ToString()));
					writer->writeTextElement("Z2", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::EndZ)).ToString()));
					writer->writeTextElement("Phi", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::Phi)).ToString()));
					writer->writeTextElement("Theta", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::Theta)).ToString()));
					writer->writeTextElement("Dia", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::Diameter)).ToString()));
					writer->writeTextElement("sL", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::Length)).ToString()));
					// TODO: define mapping?
					writer->writeTextElement("cL", QString(m_tableList[i]->GetValue(j, 19).ToString()));
					writer->writeTextElement("Surf", QString(m_tableList[i]->GetValue(j, 21).ToString()));
					writer->writeTextElement("Vol", QString(m_tableList[i]->GetValue(j, 22).ToString()));

					writer->writeEndElement(); //end Fibre-n tag
				}
				writer->writeEndElement(); //end Fibres tag
				writer->writeEndElement(); //end FibreClass-n tag
			}
			writer->writeEndElement(); //end FibreClasses tag
		}
		else if (m_filterID == iAObjectType::Voids)
		{
			writer->writeStartElement("VoidClasses"); //start FibreClasses tag

			for (int i = 0; i < m_classTreeModel->invisibleRootItem()->rowCount(); ++i)
			{
				//Gets the fibre class
				QStandardItem* fc = m_classTreeModel->invisibleRootItem()->child(i, 1);

				writer->writeStartElement(QString("VoidClass-%1").arg(i)); //start VoidClass-n tag
				writer->writeTextElement("CColor", QString("0x00").append
				(QString(m_colorList.at(fc->index().row()).name()).remove(0, 1))); //CColor tag
				writer->writeTextElement("NVoids", QString(fc->text())); //NVoids tag

				writer->writeStartElement(QString("Voids")); //Voids tag

				for (int j = 0; j < m_classTreeModel->invisibleRootItem()->child(i)->rowCount(); ++j)
				{
					writer->writeStartElement(QString("Void-%1").arg(j + 1)); //Void-n tag

					//Gets void properties from csvTable
					writer->writeTextElement("Volume", QString(m_tableList[i]->GetValue(j, 21).ToString()));
					writer->writeTextElement("dimX", QString(m_tableList[i]->GetValue(j, 13).ToString()));
					writer->writeTextElement("dimY", QString(m_tableList[i]->GetValue(j, 14).ToString()));
					writer->writeTextElement("dimZ", QString(m_tableList[i]->GetValue(j, 15).ToString()));
					writer->writeTextElement("posX", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::CenterX)).ToString()));
					writer->writeTextElement("posY", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::CenterY)).ToString()));
					writer->writeTextElement("posZ", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::CenterZ)).ToString()));
					//writer->writeTextElement("ShapeFactor", QString(m_tableList[i]->GetValue(j,22).ToString()));

					writer->writeEndElement(); //end Void-n tag
				}
				writer->writeEndElement(); //end Voids tag
				writer->writeEndElement(); //end VoidClass-n tag
			}
			writer->writeEndElement(); //end VoidClasses tag
		}
	}
}

void dlg_FeatureScout::CsvDVSaveButton()
{
	// Get selected rows out of elementTable
	QModelIndexList indexes = m_elementTableView->selectionModel()->selection().indexes();
	QList<ushort> characteristicsList;

	
	iAParameterDlg::ParamListT params;
	addParameter(params, "Save file", iAValueType::Boolean, false);
	addParameter(params, "Show histograms", iAValueType::Boolean, true);
	QStringList colNames;
	for (int i = 0; i < indexes.count(); ++i)
	{
		//Ensures that indices are unique
		if (characteristicsList.contains(indexes.at(i).row()))
		{
			continue;
		}
		characteristicsList.append(indexes.at(i).row());

		QString columnName(m_elementTable->GetColumn(0)->GetVariantValue(characteristicsList.at(i)).ToString().c_str());
		columnName.remove("Â");
		colNames.push_back(columnName);
		addParameter(params, QString("HistoMin for %1").arg(columnName), iAValueType::Continuous,
			m_elementTable->GetColumn(1)->GetVariantValue(characteristicsList.at(i)).ToFloat());
		addParameter(params, QString("HistoMax for %1").arg(columnName), iAValueType::Continuous,
			m_elementTable->GetColumn(2)->GetVariantValue(characteristicsList.at(i)).ToFloat());
		addParameter(params, QString("HistoBinNbr for %1").arg(columnName), iAValueType::Discrete, 100, 2);
	}

	if (characteristicsList.count() == 0)
	{
		QMessageBox::information(this, "FeatureScout", "No characteristic specified in the element explorer.");
		return;
	}

	iAParameterDlg dlg(this, "Save Distribution CSV", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	bool saveFile = values["Save file"].toBool();
	bool showHistogram = values["Show histograms"].toBool();
	if (!saveFile && !showHistogram)
	{
		QMessageBox::warning(this, "FeatureScout", "Please check either 'Save file' or 'Show histogram' (or both).");
		return;
	}

	QString filename;
	if (saveFile)
	{
		filename = QFileDialog::getSaveFileName(this, tr("Save characteristic distributions"), m_sourcePath, tr("CSV Files (*.csv *.CSV)"));
		if (filename.isEmpty())
		{
			return;
		}
	}

	m_dvContextView->GetScene()->ClearItems();

	//Sets up a chart matrix for the feature distribution charts
	vtkNew<vtkChartMatrix> distributionChartMatrix;
	distributionChartMatrix->SetSize(vtkVector2i(characteristicsList.count() < 3 ?
		characteristicsList.count() % 3 : 3, ceil(characteristicsList.count() / 3.0)));
	distributionChartMatrix->SetGutter(vtkVector2f(70.0, 70.0));

	//Calculates histogram for each selected characteristic
	for (int characteristicIdx = 0; characteristicIdx < characteristicsList.count(); ++characteristicIdx)
	{
		double range[2] = { 0.0, 0.0 };
		vtkDataArray* length = vtkDataArray::SafeDownCast(
			m_tableList[m_activeClassItem->index().row()]->GetColumn(characteristicsList.at(characteristicIdx)));
		range[0] = values[QString("HistoMin for %1").arg(colNames[characteristicIdx])].toDouble();
		range[1] = values[QString("HistoMax for %1").arg(colNames[characteristicIdx])].toDouble();

		if (range[0] == range[1])
		{
			range[1] = range[0] + 1.0;
		}

		int numberOfBins = values[QString("HistoBinNbr for %1").arg(colNames[characteristicIdx])].toInt();
		double inc = (range[1] - range[0]) / (numberOfBins);
		double halfInc = inc / 2.0;

		auto extents = vtkSmartPointer<vtkFloatArray>::New();
		extents->SetName("Value");
		extents->SetNumberOfTuples(numberOfBins);

		float* centers = static_cast<float*>(extents->GetVoidPointer(0));
		double min = range[0] + halfInc;

		for (int j = 0; j < numberOfBins; ++j)
		{
			extents->SetValue(j, min + j * inc);
		}

		auto populations = vtkSmartPointer<vtkIntArray>::New();
		populations->SetName("Probability");
		populations->SetNumberOfTuples(numberOfBins);
		int* pops = static_cast<int*>(populations->GetVoidPointer(0));

		for (int k = 0; k < numberOfBins; ++k)
		{
			pops[k] = 0;
		}

		for (vtkIdType j = 0; j < length->GetNumberOfTuples(); ++j)
		{
			double v(0.0);
			length->GetTuple(j, &v);

			for (int k = 0; k < numberOfBins; ++k)
			{
				//value lies between [center-halfInc & center+halfInc]
				if ((fabs(v - double(centers[k]))) <= halfInc)
				{
					//value lies between ]center-halfInc & center+halfInc[ and is assigned to class k
					if ((fabs(v - double(centers[k]))) < halfInc)
					{
						++pops[k];
						break;
					}
					// value = lower limit and is assigned to class k
					else if ((v - centers[k]) == (halfInc * -1.0))
					{
						++pops[k];
						break;
					}
					// value = upper limit and is assigned to class k+1
					else if (((v - centers[k]) == halfInc) && (v != range[1]))
					{
						++pops[k + 1];
						break;
					}
					// if value = range[1] assigned it to class k
					else if (v == range[1])
					{
						++pops[k];
						break;
					}
				}
			}
		}

		auto disTable = vtkSmartPointer<vtkTable>::New();
		disTable->AddColumn(extents.GetPointer());
		disTable->AddColumn(populations.GetPointer());

		if (saveFile)
		{
			std::ofstream file(getLocalEncodingFileName(filename).c_str(), std::ios::app);
			if (file.is_open())
			{
				vtkIdType tColNb = disTable->GetNumberOfColumns();
				vtkIdType tRowNb = disTable->GetNumberOfRows();

				file << QString("%1 Distribution of '%2'")
					.arg(m_csvTable->GetColumnName(characteristicsList.at(characteristicIdx)))
					.arg(m_activeClassItem->text()).toStdString()
					<< "\n";

				file << "HistoBinID;" << "HistoBinCenter" << "Frequency;" << "\n";

				for (vtkIdType row = 0; row < tRowNb; ++row)
				{
					for (vtkIdType col = 0; col < tColNb; ++col)
					{
						vtkVariant tVal = disTable->GetValue(row, col);
						switch (col)
						{
						case 0:
							file << row + 1 << ";"
								<< std::setprecision(20) << tVal.ToDouble() << ";";
							break;
						case 1:
							file << tVal.ToTypeUInt64() << "\n";
							break;
						}
					}
				}
				file.close();
			}
		}

		//Creates chart for each selected characteristic
		if (showHistogram)
		{
			vtkChartXY* chart = vtkChartXY::SafeDownCast(distributionChartMatrix
				->GetChart(vtkVector2i(characteristicIdx % (characteristicsList.count() < 3 ? characteristicsList.count() % 3 : 3), characteristicIdx / 3)));
			//chart->SetBarWidthFraction(0.95);
			chart->GetTitleProperties()->SetFontSize(18);
			vtkPlot* plot = chart->AddPlot(vtkChartXY::BAR);
			plot->SetInputData(disTable, 0, 1);
			plot->GetXAxis()->SetTitle(m_csvTable->GetColumnName(characteristicsList.at(characteristicIdx)));
			plot->GetYAxis()->SetTitle("Frequency");
			plot->GetXAxis()->GetLabelProperties()->SetFontSize(19);
			plot->GetYAxis()->GetLabelProperties()->SetFontSize(19);
			plot->GetXAxis()->GetTitleProperties()->SetFontSize(19);
			plot->GetYAxis()->GetTitleProperties()->SetFontSize(19);
		}
	}

	//Renders the distributionMatrix in a new dockWidget
	if (showHistogram)
	{
		if (!m_dwDV)
		{
			iAQVTKWidget* dvqvtkWidget(new iAQVTKWidget());
			m_dwDV = new iADockWidgetWrapper(dvqvtkWidget, "Distribution View", "FeatureScoutDV");
			m_dvContextView->SetRenderWindow(dvqvtkWidget->renderWindow());
			m_dvContextView->SetInteractor(dvqvtkWidget->interactor());
			m_activeChild->addDockWidget(Qt::RightDockWidgetArea, m_dwDV);
			m_dwDV->show();
		}
		m_activeChild->tabifyDockWidget(m_dwSPM ? m_dwSPM : m_dwPC, m_dwDV);
		m_dwDV->show();
		m_dwDV->raise();
		m_dvContextView->GetScene()->AddItem(distributionChartMatrix.GetPointer());
		m_dvContextView->GetRenderWindow()->Render();
	}
}

void dlg_FeatureScout::WisetexSaveButton()
{
	if (!m_classTreeModel->invisibleRootItem()->hasChildren())
	{
		QMessageBox::warning(this, "FeatureScout", "No data available!");
		return;
	}

	//XML file save path
	QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), m_sourcePath, tr("XML Files (*.xml *.XML)"));
	if (filename.isEmpty())
	{
		return;
	}

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QMessageBox::warning(this, "FeatureScout", "Could not open XML file for writing.");
		return;
	}

	//Creates XML file
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("WiseTex-XML");
	writeWisetex(&stream);
	stream.writeEndElement();
	stream.writeEndDocument();
}

void dlg_FeatureScout::ExportClassButton()
{
	// if no volume loaded, then exit
	if (m_visualization != iACsvConfig::UseVolume)
	{
		if (m_activeChild->modalities()->size() == 0)
		{
			QMessageBox::information(this, "FeatureScout", "Feature only available if labeled volume is loaded!");
			return;
		}
		else if (QMessageBox::question(this, "FeatureScout", "A labeled volume is required."
			"You are not using labeled volume visualization - are you sure a labeled volume is loaded?",
			QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;
	}
	int classCount = m_classTreeModel->invisibleRootItem()->rowCount();
	if (classCount < 2)	// unclassified class only
	{
		QMessageBox::warning(this, "FeatureScout", "No defined class (except the 'unclassified' class) - please create at least one class first!");
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Save Classes..."), "",
		tr("mhd (*.mhd)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAConnector con;
	auto img_data = m_activeChild->imagePointer();
	con.setImage(img_data);
	ITK_TYPED_CALL(CreateLabelledOutputMask, con.itkScalarPixelType(), con, fileName);
}

template <class T>
void dlg_FeatureScout::CreateLabelledOutputMask(iAConnector& con, const QString& fOutPath)
{
	typedef int ClassIDType;
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<ClassIDType, DIM>   OutputImageType;

	QString mode1 = "Export all classes";
	QString mode2 = "Export single selected class";
	QStringList modes = (QStringList() << mode1 << mode2);
	iAParameterDlg::ParamListT params;
	addParameter(params, "Classification", iAValueType::Categorical, modes);
	iAParameterDlg dlg(this, "Save classification options", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	QString mode = dlg.parameterValues()["Classification"].toString();
	bool exportAllClassified = (mode == mode1);

	QMap<size_t, ClassIDType> currentEntries;
	// Skip first, as classes start with 1, 0 is the uncategorized class
	for (int i = 1; i < m_classTreeModel->invisibleRootItem()->rowCount(); i++)
	{

		if (!exportAllClassified && i != m_activeClassItem->row())
		{
			continue;
		}

		QStandardItem* item = m_classTreeModel->invisibleRootItem()->child(i);
		for (int j = 0; j < item->rowCount(); ++j)
		{
			size_t labelID = item->child(j)->text().toULongLong();
			currentEntries.insert(labelID, i);
		}
	}

	//export class ID or fiber labels for single class
	bool fiberIDLabelling = (m_classTreeModel->invisibleRootItem()->rowCount() >= 2 &&
		(m_renderMode == rmSingleClass && m_activeClassItem->row() > 0) && (!exportAllClassified));
	if (fiberIDLabelling &&
		(QMessageBox::question(this, "FeatureScout", "Only one class selected, "
			"should we export the individual fiber IDs? "
			"If you select No, all fibers in the class will be labeled with the class ID.",
			QMessageBox::Yes | QMessageBox::No)
			== QMessageBox::No))
	{
		fiberIDLabelling = false;
	}

	auto in_img = dynamic_cast<InputImageType*>(con.itkImage());
	auto region_in = in_img->GetLargestPossibleRegion();
	const OutputImageType::SpacingType outSpacing = in_img->GetSpacing();
	auto out_img = createImage<OutputImageType>(region_in.GetSize(), outSpacing);
	itk::ImageRegionConstIterator<InputImageType> in(in_img, region_in);
	itk::ImageRegionIterator<OutputImageType> out(out_img, region_in);
	while (!in.IsAtEnd())
	{
		size_t labelID = static_cast<size_t>(in.Get());
		if (fiberIDLabelling)
		{
			if (currentEntries.contains(labelID))
			{
				out.Set(static_cast<ClassIDType>(labelID));
			}
			else
			{
				out.Set(0);
			}
		}
		else
		{
			out.Set(static_cast<ClassIDType>(currentEntries[labelID]));
		}
		++in;
		++out;
	}
	storeImage(out_img, fOutPath, m_activeChild->preferences().Compression);
	LOG(lvlInfo, "Stored image of of classes.");
}

void dlg_FeatureScout::ClassSaveButton()
{
	if (m_classTreeModel->invisibleRootItem()->rowCount() == 1)
	{
		QMessageBox::warning(this, "FeatureScout", "No classes was defined.");
		return;
	}

	QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), m_sourcePath, tr("XML Files (*.xml *.XML)"));
	if (filename.isEmpty())
	{
		return;
	}

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QMessageBox::warning(this, "FeatureScout", "Could not open XML file for writing.");
		return;
	}

	QXmlStreamWriter stream(&file);

	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement(IFVTag);
	stream.writeAttribute(VersionAttribute, "1.0");
	stream.writeAttribute(CountAllAttribute, QString("%1").arg(m_objectCount));
	stream.writeAttribute(IDColumnAttribute, m_csvTable->GetColumnName(0)); // store name of ID  -> TODO: ID mapping!

	for (int i = 0; i < m_classTreeModel->invisibleRootItem()->rowCount(); ++i)
	{
		writeClassesAndChildren(&stream, m_classTreeModel->invisibleRootItem()->child(i));
	}

	stream.writeEndElement(); // ClassTree
	stream.writeEndDocument();
}

void dlg_FeatureScout::ClassLoadButton()
{
	// open xml file and get meta information
	QString filename = QFileDialog::getOpenFileName(this, tr("Load xml file"), m_sourcePath, tr("XML Files (*.xml *.XML)"));
	if (filename.isEmpty())
	{
		return;
	}

	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, "FeatureScout", "Class load error: Could not open source xml file.");
		return;
	}

	// checking xml file correctness
	QXmlStreamReader checker(&file);
	checker.readNext(); // skip xml tag?
	checker.readNext(); // read IFV_Class_Tree element
	QString IDColumnName = (m_filterID == iAObjectType::Fibers) ? LabelAttribute : LabelAttributePore;
	if (checker.name() == IFVTag)
	{
		// if the object number is not correct, stop the load process
		if (checker.attributes().value(CountAllAttribute).toString().toInt() != m_objectCount)
		{
			QMessageBox::warning(this, "FeatureScout", "Class load error: Incorrect xml file for current dataset, please check.");
			checker.clear();
			return;
		}
		if (checker.attributes().hasAttribute(IDColumnAttribute))
		{
			IDColumnName = checker.attributes().value(IDColumnAttribute).toString();
		}
	}
	else // incompatible xml file
	{
		QMessageBox::warning(this, "FeatureScout", "Class load error: xml file incompatible with FeatureScout, please check.");
		checker.clear();
		return;
	}
	checker.clear();
	file.close();

	m_chartTable->DeepCopy(m_csvTable); // reset charttable
	m_tableList.clear();
	m_tableList.push_back(m_chartTable);
	m_colorList.clear();
	m_classTreeModel->clear();

	// init header names for class tree model
	m_classTreeModel->setHorizontalHeaderItem(0, new QStandardItem("Class"));
	m_classTreeModel->setHorizontalHeaderItem(1, new QStandardItem("Count"));
	m_classTreeModel->setHorizontalHeaderItem(2, new QStandardItem("Percent"));

	int idxClass = 0;

	// create xml reader
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();
	QStandardItem* activeItem = nullptr;

	QFile readFile(filename);
	if (!readFile.open(QIODevice::ReadOnly))
	{
		return;
	}

	QXmlStreamReader reader(&readFile);
	while (!reader.atEnd())
	{
		if (reader.readNext() != QXmlStreamReader::EndDocument && reader.isStartElement())
		{
			if (reader.name() == ObjectTag)
			{
				QString label = reader.attributes().value(IDColumnName).toString();
				QStandardItem* item = new QStandardItem(label);

				// add objects to firstLevelClassItem
				activeItem->appendRow(item);

				// update Class_ID number in csvTable;
				m_csvTable->SetValue(label.toInt() - 1, m_elementCount - 1, idxClass - 1);
				m_splom->changeClass(label.toInt() - 1, idxClass - 1);
			}
			else if (reader.name() == ClassTag)
			{
				// prepare the first level class items
				const QString name = reader.attributes().value(NameAttribute).toString();
				const QString color = reader.attributes().value(ColorAttribute).toString();
				const QString count = reader.attributes().value(CountAttribute).toString();
				const QString percent = reader.attributes().value(PercentAttribute).toString();

				QList<QStandardItem*> stammItem = prepareRow(name, count, percent);
				stammItem.first()->setData(QColor(color), Qt::DecorationRole);
				m_colorList.append(QColor(color));
				rootItem->appendRow(stammItem);
				activeItem = rootItem->child(idxClass);
				++idxClass;
			}
		}
	}
	if (reader.hasError())
	{
		LOG(lvlError, QString("Error while parsing XML: %1").arg(reader.errorString()));
	}
	m_splom->classesChanged();

	//upadate TableList
	if (rootItem->rowCount() == idxClass)
	{
		for (int i = 0; i < idxClass; ++i)
		{
			this->recalculateChartTable(rootItem->child(i));
		}
		this->setActiveClassItem(rootItem->child(0), 0);
		MultiClassRendering();
	}
	else
	{
		QMessageBox::warning(this, "FeatureScout", "Class load error: unclear class load process.");
	}
	reader.clear();
	readFile.close();
}

void dlg_FeatureScout::ClassDeleteButton()
{
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();
	QStandardItem* stammItem = rootItem->child(0);
	if (m_activeClassItem->index().row() == 0)
	{
		QMessageBox::warning(this, "FeatureScout", "You are trying to delete the unclassified class, please select another class.");
		return;
	}

	// define a list to sort the items in stammItem
	QList<int> list;
	// get Class_ID
	int deleteClassID = m_activeClassItem->index().row();
	int countActive = m_activeClassItem->rowCount();

	// append stamm item values to list
	for (int i = 0; i < stammItem->rowCount(); ++i)
	{
		list.append(stammItem->child(i)->text().toInt());
	}
	for (int j = 0; j < countActive; ++j)
	{
		int labelID = m_activeClassItem->child(j)->text().toInt();
		// update Class_ID column, prepare values for LookupTable
		m_csvTable->SetValue(labelID - 1, m_elementCount - 1, 0);
		// append the deleted object IDs to list
		list.append(labelID);
	}
	m_splom->classDeleted(deleteClassID);
	// sort the new stamm list
	std::sort(list.begin(), list.end());
	// give the values from list to stammitem
	stammItem->removeRows(0, stammItem->rowCount());
	for (int k = 0; k < list.size(); ++k)
	{
		QStandardItem* item = new QStandardItem(QString("%1").arg(list.at(k)));
		stammItem->appendRow(item);
	}

	// remove the deleted class from tree view, its entry in m_tableList and its color
	m_tableList.removeAt(deleteClassID);
	rootItem->removeRow(deleteClassID);
	m_colorList.removeAt(deleteClassID);

	// Update class ID for all remaining classes elements
	int classCount = rootItem->rowCount();

	//set new class ID, iterate
	if (classCount > 0)
	{
		for (int classID = deleteClassID; classID < classCount; ++classID)
		{
			QStandardItem* item = rootItem->child(classID, 0);

			//go for each element in the class and reset id
			//element number = number of colums
			for (int j = 0; j < item->rowCount(); ++j)
			{
				int labelID = item->child(j, 0)->text().toInt();
				m_csvTable->SetValue(labelID - 1, m_elementCount - 1, classID);
			}
			for (int k = 0; k < m_tableList[classID]->GetNumberOfRows(); ++k)
			{
				m_tableList[classID]->SetValue(k, m_elementCount - 1, classID);
			}
			m_tableList[classID]->GetColumn(classID)->Modified();
		}
	}

	// update statistics for m_activeClassItem
	this->updateClassStatistics(stammItem);

	// update m_tableList and setup m_activeClassItem
	this->setActiveClassItem(stammItem, 2);
	QSignalBlocker ctvBlocker(m_classTreeView);
	m_classTreeView->setCurrentIndex(m_classTreeView->model()->index(0, 0));

	// update element view
	this->setPCChartData();
	this->calculateElementTable();
	this->initElementTableModel();
	this->SingleRendering();
	if (m_renderMode != rmSingleClass)
	{
		m_renderMode = rmSingleClass;
		// reset color in SPLOM
		m_splom->setDotColor(StandardSPLOMDotColor);
		m_splom->enableSelection(true);
	}
}

void dlg_FeatureScout::showScatterPlot()
{
	if (m_dwSPM)
	{
		return;
	}
	QSignalBlocker spmBlock(m_splom.data()); //< no need to trigger updates while we're creating SPM
	m_splom->initScatterPlot(m_csvTable, m_columnVisibility);
	m_dwSPM = new iADockWidgetWrapper(m_splom->matrixWidget(), "Scatter Plot Matrix", "FeatureScoutSPM");
	m_activeChild->splitDockWidget(m_activeChild->renderDockWidget(), m_dwSPM, Qt::Vertical);
	m_dwSPM->show();
	m_dwSPM->raise();
	if (m_renderMode == rmMultiClass)
	{
		m_splom->multiClassRendering(m_colorList);
	}
	else
	{
		m_splom->setDotColor(StandardSPLOMDotColor);
		if (m_renderMode == rmSingleClass)
		{
			auto filteredSelInds = getPCSelection();
			m_splom->setFilteredSelection(filteredSelInds);
			QStandardItem* item = m_activeClassItem;
			if (item)
			{
				if (!item->hasChildren())
				{
					item = item->parent();
				}
				int classID = item->row();
				m_splom->setFilter(classID);
			}
		}
	}
}

void dlg_FeatureScout::showPCSettings()
{
	iAParameterDlg::ParamListT params;
	addParameter(params, "Line Width", iAValueType::Continuous, QString::number(m_pcLineWidth, 'f', 2), 0.001, 100000);
	addParameter(params, "Opacity", iAValueType::Discrete, m_pcOpacity, 0, 255);
	addParameter(params, "Tick Count", iAValueType::Discrete, m_pcTickCount, 0, 255);
	addParameter(params, "Font Size", iAValueType::Discrete, m_pcFontSize, 0, 255);

	iAParameterDlg pcSettingsDlg(this, "Parallel Coordinates Settings", params);
	if (pcSettingsDlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = pcSettingsDlg.parameterValues();
	m_pcLineWidth = values["Line Width"].toFloat();
	m_pcOpacity   = values["Opacity"].toInt();
	m_pcTickCount = values["Tick Count"].toInt();
	m_pcFontSize  = values["Font Size"].toInt();
		
	m_pcChart->GetPlot(0)->GetPen()->SetOpacity(m_pcOpacity);
	m_pcChart->GetPlot(0)->SetWidth(m_pcLineWidth);
	
	updateAxisProperties();

	m_pcView->Update();
	m_pcView->ResetCamera();
	m_pcView->Render();
}

void dlg_FeatureScout::spSelInformsPCChart(std::vector<size_t> const& selInds)
{	// If scatter plot selection changes, Parallel Coordinates gets informed
	RenderSelection(selInds);
	QCoreApplication::processEvents();
	auto sortedSelInds = m_splom->getFilteredSelection();
	int countSelection = sortedSelInds.size();
	auto vtk_selInd = vtkSmartPointer<vtkIdTypeArray>::New();
	vtk_selInd->Allocate(countSelection);
	vtk_selInd->SetNumberOfValues(countSelection);
	int idx = 0;
	for (auto ind : sortedSelInds)
	{
		vtkVariant var_Idx = ind;
		long long curr_selInd = var_Idx.ToLongLong() /*+1*/;
		vtk_selInd->SetVariantValue(idx, curr_selInd);
		++idx;
	}
	m_pcChart->GetPlot(0)->SetSelection(vtk_selInd);
	m_pcView->Render();
}

void dlg_FeatureScout::pcRightButtonPressed(vtkObject* obj, unsigned long, void* /*client_data*/, void*, vtkCommand* /*command*/)
{
	// Gets the right mouse button press event for scatter plot matrix.
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
	m_mousePressPos[0] = iren->GetEventPosition()[0];
	m_mousePressPos[1] = iren->GetEventPosition()[1];
}

void dlg_FeatureScout::pcRightButtonReleased(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command)
{
	// Gets the mouse button event for scatter plot matrix and opens a popup menu.
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
	int* mouseReleasePos = iren->GetLastEventPosition();
	if (mouseReleasePos[0] == m_mousePressPos[0] && mouseReleasePos[1] == m_mousePressPos[1])
	{
		command->AbortFlagOn();    //< Consume event so the interactor style doesn't get it
		QMenu* popupMenu = static_cast<QMenu*>(client_data);
		int* sz = iren->GetSize();
		QPoint pt(mouseReleasePos[0], sz[1] - mouseReleasePos[1]); // flip y
		// VTK delivers "device pixel" coordinates, while Qt expects "device independent pixel" coordinates
		// (see https://doc.qt.io/qt-5/highdpi.html#glossary-of-high-dpi-terms); convert:
		pt = pt / m_dwPC->devicePixelRatio();
		auto global_pt = popupMenu->parentWidget()->mapToGlobal(pt);
		popupMenu->popup(global_pt);
	}
	else
	{
		// semi-automatic classification not ported to new SPM yet
		/*
		QMenu* popupMenu = static_cast<QMenu*>( client_data );     // Reset to original SPM popup
		if ( popupMenu->actions().count() > 1 )                    // Check if SPM or PC popups
			popupMenu->actions().at( 1 )->setText( "Suggest Classification" );
		*/
	}
}

// from previous right-click menu event handler:
	/*
	if ( selection->text() == "Suggest Classification" )
	{
		bool ok;
		int i = QInputDialog::getInt( m_dwSPM, tr( "kMeans-Classification" ), tr( "Number of Classes" ), 3, 1, 7, 1, &ok );

		if ( ok )
		{
			//matrix->NumberOfClusters = i;
			//matrix->SetkMeansMode( true );
			selection->setText( "Accept Classification" );
		}
	}
	else if ( selection->text() == "Accept Classification" )
	{
		//autoAddClass( matrix->GetkMeansClusterCount() );
		selection->setText( "Suggest Classification" );
	}

void dlg_FeatureScout::autoAddClass( int NbOfClusters )
{
	QStandardItem *motherClassItem = m_activeClassItem;

	for ( int i = 1; i <= NbOfClusters; ++i )
	{
		// recieve the current selections from annotationlink
		// semi-automatic classification not ported to new SPM yet
		//vtkAbstractArray *SelArr = matrix->GetkMeansCluster( i )->GetNode( 0 )->GetSelectionList();
		int CountObject = 0; //  SelArr->GetNumberOfTuples();
		/---*
		if ( CountObject > 0 )
		{
			// class name and color
			int cid = m_classTreeModel->invisibleRootItem()->rowCount();
			QString cText = QString( "Class %1" ).arg( cid );
			QColor cColor = getClassColor(cid);
			m_colorList.append( cColor );

			// create a first level child under rootItem as new class
			double percent = 100.0*CountObject / objectNr;
			QList<QStandardItem *> firstLevelItem = prepareRow( cText, QString( "%1" ).arg( CountObject ), QString::number( percent, 'f', 1 ) );
			firstLevelItem.first()->setData( cColor, Qt::DecorationRole );

			QList<int> kIdx; // list to regist the selected index of object IDs in m_activeClassItem
			int objID = 0;
			// add new class
			for ( int i = 0; i < CountObject; ++i )
			{
				objID = SelArr->GetVariantValue( i ).ToInt();
				if ( !kIdx.contains( objID ) )
				{
					kIdx.prepend( objID );
					// add item to the new class
					firstLevelItem.first()->appendRow( new QStandardItem( QString( "%1" ).arg( objID ) ) );
					// update Class_ID column, prepare values for LookupTable
					m_csvTable->SetValue( objID - 1, elementNr - 1, cid );
				}
			}

			// append the new class to class tree
			m_classTreeModel->invisibleRootItem()->appendRow( firstLevelItem );

			// update chartTable
			this->setActiveClassItem( firstLevelItem.first(), 1 );
			m_classTreeView->setCurrentIndex( firstLevelItem.first()->index() );
		}
		else
		{
			QMessageBox::warning(this, "FeatureScout", "No object was selected!" );
			// return; (?)
		}
		* /
	}

	//  Mother Class is empty after kMeans auto class added, therefore rows are removed
	m_classTreeModel->invisibleRootItem()->child( motherClassItem->index().row() )->
		removeRows( 0, m_classTreeModel->invisibleRootItem()->child( motherClassItem->index().row() )->rowCount() );

	// Update statistics for motherClassItem
	this->updateClassStatistics( m_classTreeModel->invisibleRootItem()->child( motherClassItem->index().row() ) );

	if ( m_activeClassItem->rowCount() == 0 && m_activeClassItem->index().row() != 0 )//new
	{
		// delete the activeItem
		int cID = m_activeClassItem->index().row();
		m_classTreeModel->invisibleRootItem()->removeRow( cID );
		m_colorList.removeAt( cID );
		//update Class_ID and lookupTable??
	}

	calculateElementTable();
	initElementTableModel();
	setPCChartData();
	m_classTreeView->collapseAll();
	SingleRendering();
	updatePolarPlotView( m_chartTable );

	//Updates scatter plot matrix when a class is added.
	// TODO SPM
	//matrix->UpdateColorInfo( m_classTreeModel, m_colorList );
	//matrix->SetClass2Plot( m_activeClassItem->index().row() );
	//matrix->UpdateLayout();
}
*/

void dlg_FeatureScout::classDoubleClicked(const QModelIndex& index)
{
	QStandardItem* item;
	// Gets right item from ClassTreeModel to remove AccesViolationError on item double click).
	// A class was clicked. (and no -1 row index - 'undefined' area click)
	if (index.parent().row() == -1 && index.isValid())
	{
		item = m_classTreeModel->item(index.row(), 0);
		// Surpresses changeability items (class level) after iAClassEditDlg dialog.
		m_classTreeModel->itemFromIndex(index)->setEditable(false);
	}
	else if (index.parent().row() != -1 && index.isValid())
	{	// An item was clicked.
		// Surpresses changeability of items (single fiber level) after iAClassEditDlg dialog.
		m_classTreeModel->itemFromIndex(index)->setEditable(false);
		return;
	}
	else	// An 'undefined area' was clicked.
	{
		return;
	}

	m_pcWidget->setEnabled(true);

	if (item->hasChildren())
	{
		int classID = index.row();
		bool ok;
		QColor old_cColor = m_colorList.at(classID);
		QString old_cText = item->text();
		QColor new_cColor = old_cColor;
		QString new_cText = old_cText;
		new_cText = iAClassEditDlg::getClassInfo("FeatureScout: Edit Class", new_cText, new_cColor, ok);

		if (ok && (old_cText.compare(new_cText) != 0 || new_cColor != old_cColor))
		{
			m_colorList[index.row()] = new_cColor;
			item->setText(new_cText);
			item->setData(new_cColor, Qt::DecorationRole);
			this->SingleRendering();
			m_splom->clearSelection();
			m_splom->setFilter(classID);
			m_splom->setDotColor(StandardSPLOMDotColor);
		}
	}
}

void dlg_FeatureScout::showOrientationDistribution()
{
	showLengthDistribution(false);
	updatePolarPlotView(m_chartTable); // maybe orientation distribution is already shown; we could add a check, and skip this call in that case
}

void dlg_FeatureScout::showLengthDistribution(bool show, vtkScalarsToColors* lut)
{
	if (m_scalarWidgetFLD)
	{
		m_scalarWidgetFLD->SetEnabled(show);
	}
	else if (show)
	{
		m_scalarWidgetFLD = vtkSmartPointer<vtkScalarBarWidget>::New();
		m_scalarBarFLD = vtkSmartPointer<vtkScalarBarActor>::New();
		m_scalarBarFLD->SetLookupTable(lut);
		m_scalarBarFLD->GetLabelTextProperty()->SetColor(0, 0, 0);
		m_scalarBarFLD->GetTitleTextProperty()->SetColor(0, 0, 0);
		m_scalarBarFLD->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
		m_scalarBarFLD->SetTitle("Length in microns");
		m_scalarBarFLD->SetNumberOfLabels(9);
		m_scalarWidgetFLD->SetInteractor(m_renderer->interactor());
		m_scalarWidgetFLD->SetScalarBarActor(m_scalarBarFLD);
		m_scalarWidgetFLD->SetEnabled(true);
		m_scalarWidgetFLD->SetRepositionable(true);
		m_scalarWidgetFLD->SetResizable(true);
		m_scalarWidgetFLD->GetScalarBarActor()->SetTextPositionToSucceedScalarBar();
		auto sbr = vtkScalarBarRepresentation::SafeDownCast(m_scalarWidgetFLD->GetRepresentation());
		sbr->SetPosition(0.93, 0.20);
		sbr->SetPosition2(0.07, 0.80);
	}
	if (!show && m_lengthDistrWidget->isVisible())
	{
		m_dwPP->legendLayout->removeWidget(m_lengthDistrWidget);
		m_dwPP->legendLayout->addWidget(m_polarPlotWidget);
		m_lengthDistrWidget->hide();
		m_polarPlotWidget->show();
	}
	m_dwPP->colorMapSelection->hide();
}

void dlg_FeatureScout::classClicked(const QModelIndex& index)
{
	showOrientationDistribution();

	// Gets right Item from ClassTreeModel
	QStandardItem* item;

	// checks if click on class 'level'
	if (index.parent().row() == -1)
	{
		item = m_classTreeModel->item(index.row(), 0);
	}
	else
	{
		item = m_classTreeModel->itemFromIndex(index);
	}

	// check if unclassified class is empty
	if (m_classTreeModel->invisibleRootItem()->child(0) == item && item->rowCount() == 0)
	{
		QMessageBox::information(this, "FeatureScout", "Unclassified class contains no objects, please make another selection.");
		return;
	}
	//MOD kMeans
	if (item->rowCount() == 0 && item->parent()->index() == m_classTreeModel->invisibleRootItem()->index())
	{
		QMessageBox::information(this, "FeatureScout", "This class contains no object, please make another selection or delete the class.");
		return;
	}
	QStandardItem* classItem = item->hasChildren() ? item : item->parent();
	if (classItem != m_activeClassItem || m_renderMode != rmSingleClass ||
		(m_singleObjectSelected && item->hasChildren()))
	{
		int classID = classItem->index().row();
		m_splom->setFilter(classID);
		setActiveClassItem(classItem);
		calculateElementTable();
		setPCChartData();
		updatePolarPlotView(m_chartTable);
		if (item->hasChildren())  // has children => a class was selected
		{
			SingleRendering();
			initElementTableModel();
			m_splom->clearSelection();
		}
	}
	m_singleObjectSelected = !item->hasChildren();
	if (m_renderMode != rmSingleClass)  // special rendering was enabled before
	{
		m_renderMode = rmSingleClass;
		// reset color in SPLOM
		m_splom->setDotColor(StandardSPLOMDotColor);
		m_splom->enableSelection(true);
	}
	if (!item->hasChildren()) // has no children => single object selected
	{
		// update ParallelCoordinates view selection
		int sID = item->index().row();

		// Fill selection with IDs
		auto testArr = vtkSmartPointer<vtkIdTypeArray>::New();
		testArr->SetName("Label");
		testArr->InsertNextValue(sID);

		m_pcChart->GetPlot(0)->SetSelection(testArr);
		m_pcView->ResetCamera();
		m_pcView->Render();

		// update elementTableView
		this->initElementTableModel(sID);
		int objectID = item->text().toInt();
		this->SingleRendering(objectID);
		m_elementTableView->update();

		// update SPLOM selection
		std::vector<size_t> filteredSelInds;
		filteredSelInds.push_back(sID);
		m_splom->setFilteredSelection(filteredSelInds);
	}
}

double dlg_FeatureScout::calculateOpacity(QStandardItem* item)
{
	// chart opacity dependence of number of objects
	// TODO: replace by some kind of logarithmic formula
	// for multi rendering
	if (item == m_classTreeModel->invisibleRootItem())
	{
		if (m_objectCount < 1000)
		{
			return 1.0;
		}
		if (m_objectCount < 3000)
		{
			return 0.8;
		}
		if (m_objectCount < 10000)
		{
			return 0.6;
		}
		return 0.5;
	}
	// for single class rendering
	else if (item->hasChildren())
	{
		int n = item->rowCount();

		if (n < 1000)
		{
			return 1.0;
		}
		if (n < 3000)
		{
			return 0.8;
		}
		if (n < 10000)
		{
			return 0.6;
		}
		if (n < 20000)
		{
			return 0.2;
		}
		return 0.4;
	}
	// default opacity:
	return 1.0;
}

namespace
{
	QString filterToXMLAttributeName(QString const& str)
	{
		QString result(str);
		const QRegularExpression validFirstChar("^[a-zA-Z_:]");
		while (!validFirstChar.match(result).hasMatch() && result.size() > 0)
		{
			result.remove(0, 1);
		}
		const QRegularExpression invalidChars("[^a-zA-Z0-9_:.-]");
		result.remove(invalidChars);
		return result;
	}
}

void dlg_FeatureScout::writeClassesAndChildren(QXmlStreamWriter* writer, QStandardItem* item) const
{
	// check if it is a class item
	if (item->hasChildren())
	{
		writer->writeStartElement(ClassTag);
		writer->writeAttribute(NameAttribute, item->text());

		QString color = QString(m_colorList.at(item->index().row()).name());

		writer->writeAttribute(ColorAttribute, color);
		writer->writeAttribute(CountAttribute, m_classTreeModel->invisibleRootItem()->child(item->index().row(), 1)->text());
		writer->writeAttribute(PercentAttribute, m_classTreeModel->invisibleRootItem()->child(item->index().row(), 2)->text());
		for (int i = 0; i < item->rowCount(); ++i)
		{
			writer->writeStartElement(ObjectTag);
			for (int j = 0; j < m_elementCount; ++j)
			{
				vtkVariant v = m_csvTable->GetValue(item->child(i)->text().toInt() - 1, j);
				vtkVariant v1 = m_elementTable->GetValue(j, 0);
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 1, 0)
				QString str = QString::fromUtf8(v.ToUnicodeString().utf8_str()).trimmed();
				QString str1 = filterToXMLAttributeName(QString::fromUtf8(v1.ToUnicodeString().utf8_str()).trimmed());
#else
				QString str = QString::fromUtf8(v.ToString().c_str()).trimmed();
				QString str1 = filterToXMLAttributeName(QString::fromUtf8(v1.ToString().c_str()).trimmed());
#endif
				writer->writeAttribute(str1, str);
			}
			writer->writeEndElement(); // end object tag
		}
		writer->writeEndElement(); // end class tag
	}
}

void dlg_FeatureScout::setActiveClassItem(QStandardItem* item, int situ)
{
	// check once more, if its really a class item
	if (!item->hasChildren())
	{
		return;
	}

	if (situ == 0)	// class clicked
	{
		m_activeClassItem = item;
		int id = item->index().row();
		m_chartTable = m_tableList[id];
	}
	else if (situ == 1)	// add class
	{
		// recalculate the old active class
		this->recalculateChartTable(m_activeClassItem);

		// calculate the new class table and set up chartTable
		this->recalculateChartTable(item);

		m_activeClassItem = item;
		int id = item->index().row();
		m_chartTable = m_tableList[id];
	}
	else if (situ == 2)	// delete class
	{
		// merge the deleted class table to stamm table
		this->recalculateChartTable(item);
		m_activeClassItem = item;
		m_chartTable = m_tableList[0];
	}
}

void dlg_FeatureScout::recalculateChartTable(QStandardItem* item)
{
	if (!item->hasChildren())
	{
		return;
	}

	auto table = vtkSmartPointer<vtkTable>::New();
	auto arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName(m_chartTable->GetColumnName(0));
	table->AddColumn(arr);
	for (int i = 1; i < m_elementCount - 1; ++i)
	{
		auto arrX = vtkSmartPointer<vtkFloatArray>::New();
		arrX->SetName(m_chartTable->GetColumnName(i));
		table->AddColumn(arrX);
	}
	auto arrI = vtkSmartPointer<vtkIntArray>::New();
	arrI->SetName(m_chartTable->GetColumnName(m_elementCount - 1));
	table->AddColumn(arrI);

	int oCount = item->rowCount();
	table->SetNumberOfRows(oCount);

	int csvID = 0;
	for (int j = 0; j < oCount; ++j)
	{
		csvID = item->child(j)->text().toInt();
		auto arrRow = vtkSmartPointer<vtkVariantArray>::New();
		m_csvTable->GetRow(csvID - 1, arrRow);
		table->SetRow(j, arrRow);
	}

	// if item already exists
	int itemID = item->index().row();
	if (itemID + 1 <= m_tableList.size())
	{
		// add the new active class table to m_tableList
		m_tableList.insert(itemID, table);
		// delete the old active class table
		m_tableList.removeAt(itemID + 1);
	}
	else
	{
		// add the new table to the end of the m_tableList
		// maka a copy to chartTable
		m_tableList.push_back(table);
		m_chartTable = m_tableList[item->index().row()];
	}
}

void dlg_FeatureScout::updateLookupTable(double alpha)
{
	int lutNum = m_colorList.size();
	m_multiClassLUT->SetNumberOfTableValues(lutNum);
	for (int i = 0; i < lutNum; ++i)
	{
		m_multiClassLUT->SetTableValue(i,
			m_colorList.at(i).red() / 255.0,
			m_colorList.at(i).green() / 255.0,
			m_colorList.at(i).blue() / 255.0,
			m_colorList.at(i).alpha() / 255.0);
	}

	m_multiClassLUT->SetRange(0, lutNum - 1);
	m_multiClassLUT->SetAlpha(alpha);
	m_multiClassLUT->Build();
}

void dlg_FeatureScout::EnableBlobRendering()
{
	if (!OpenBlobVisDialog())
	{
		return;
	}

	iABlobCluster* blob = nullptr;
	// check if that class is already visualized; if yes, update the existing blob:
	if (m_blobMap.contains(m_activeClassItem->text()))
	{
		blob = m_blobMap[m_activeClassItem->text()];
	}
	else
	{
		blob = new iABlobCluster;
		m_blobManager->AddBlob(blob);
		m_blobMap.insert(m_activeClassItem->text(), blob);
	}

	m_blobManager->UpdateBlobSettings(blob);

	QVector<FeatureInfo> objects;
	for (int i = 0; i < m_chartTable->GetNumberOfRows(); ++i)
	{
		FeatureInfo fi;
		//int index = chartTable->GetValue( i, 0 ).ToInt();
		fi.x1 = m_chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::StartX)).ToDouble();
		fi.y1 = m_chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::StartY)).ToDouble();
		fi.z1 = m_chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::StartZ)).ToDouble();
		fi.x2 = m_chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::EndX)).ToDouble();
		fi.y2 = m_chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::EndY)).ToDouble();
		fi.z2 = m_chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::EndZ)).ToDouble();
		fi.diameter = m_chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::Diameter)).ToDouble();
		objects.append(fi);
	}

	QColor color = m_colorList.at(m_activeClassItem->index().row());
	const double count = m_activeClassItem->rowCount();
	const double percentage = 100.0 * count / m_objectCount;
	blob->SetObjectType(MapObjectTypeToString(m_filterID));
	blob->SetCluster(objects);
	blob->SetName(m_activeClassItem->text());
	blob->SetBlobColor(color);
	blob->SetStats(count, percentage);
	m_blobManager->Update();
}

void dlg_FeatureScout::DisableBlobRendering()
{
	// delete blob for class
	if (m_blobMap.contains(m_activeClassItem->text()))
	{
		iABlobCluster* blob = m_blobMap.take(m_activeClassItem->text());
		m_blobManager->RemoveBlob(blob);
		delete blob;
	}
}

void dlg_FeatureScout::showContextMenu(const QPoint& pnt)
{
	QStandardItem* item = m_classTreeModel->itemFromIndex(m_classTreeView->currentIndex());

	if (!item || item->column() > 0)
	{
		return;
	}

	QList<QAction*> actions;
	if (m_classTreeView->indexAt(pnt).isValid())
	{
		if (item->hasChildren())
		{
			actions.append(m_blobRendering);
			actions.append(m_blobRemoveRendering);
			actions.append(m_saveBlobMovie);
			actions.append(m_objectAdd);
		}
		else if (item->parent()) // item also might have no children because it's an empty class!
		{
			if (item->parent()->index().row() != 0)
			{
				actions.append(m_objectDelete);
			}
		}
	}
	if (actions.count() > 0)
	{
		QMenu::exec(actions, m_classTreeView->mapToGlobal(pnt));
	}
}

void dlg_FeatureScout::addObject()
{
	QMessageBox::warning(this, "FeatureScout", "Adding an object to the active class is not implemented yet!");
	// checking Class_ID, delete the object from the formerly class, update the corresponding class Table in Table list
	// adding the new object to the active class, update class table, reordering the label id...
}

void dlg_FeatureScout::deleteObject()
{
	QStandardItem* item = m_classTreeModel->itemFromIndex(m_classTreeView->currentIndex());
	if (item->hasChildren())
	{
		return;
	}

	// if the parent item is the unclassified item
	if (item->parent()->index() == m_classTreeModel->invisibleRootItem()->child(0)->index())
	{
		QMessageBox::information(this, "FeatureScout", "An object in the unclassified class can not be deleted.");
		return;
	}

	if (item->parent()->rowCount() == 1)
	{
		this->ClassDeleteButton();
	}
	else
	{
		int oID = item->text().toInt();
		m_csvTable->SetValue(oID - 1, m_elementCount - 1, 0);

		QStandardItem* sItem = m_classTreeModel->invisibleRootItem()->child(0);
		QStandardItem* newItem = new QStandardItem(QString("%1").arg(oID));

		if (oID == 1 || sItem->rowCount() == 0)
		{
			sItem->insertRow(0, newItem);
		}
		else if (oID > sItem->child(sItem->rowCount() - 1)->text().toInt())
		{
			sItem->appendRow(newItem);
		}
		else
		{
			int i = 0;
			while (oID > sItem->child(i)->text().toInt())
			{
				++i;
			}
			sItem->insertRow(i, newItem);
		}
		this->updateClassStatistics(sItem);
		this->recalculateChartTable(sItem);

		QStandardItem* rItem = item->parent();
		rItem->removeRow(item->index().row());
		this->updateClassStatistics(rItem);
		this->recalculateChartTable(rItem);
		this->setActiveClassItem(rItem, 0);
	}
}

void dlg_FeatureScout::updateClassStatistics(QStandardItem* item)
{
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();

	if (item->hasChildren())
	{
		rootItem->child(item->index().row(), 1)->setText(QString("%1").arg(item->rowCount()));
		double percent = 100.0 * item->rowCount() / m_objectCount;
		rootItem->child(item->index().row(), 2)->setText(QString::number(percent, 'f', 1));
	}
	else
	{
		if (item->index() == rootItem->child(0)->index())
		{
			rootItem->child(0, 1)->setText("0");
			rootItem->child(0, 2)->setText("0.00");
		}
		//MOD kMeans
		else if (item->rowCount() == 0)
		{
			rootItem->child(item->index().row(), 1)->setText("0");
			rootItem->child(item->index().row(), 2)->setText("0.00");
		}
	}
}

int dlg_FeatureScout::calcOrientationProbability(vtkTable* t, vtkTable* ot)
{
	// compute the probability distribution of orientations
	ot->Initialize();
	int maxF = 0;
	double fp, ft;
	int ip, it, tt, length;

	for (int i = 0; i < m_gThe; ++i)
	{
		auto arr = vtkSmartPointer<vtkIntArray>::New();
		arr->SetNumberOfValues(m_gPhi);
		ot->AddColumn(arr);
		for (int j = 0; j < m_gPhi; ++j)
		{
			ot->SetValue(j, i, 0);
		}
	}

	length = t->GetNumberOfRows();

	for (int k = 0; k < length; ++k)
	{
		fp = t->GetValue(k, m_columnMapping->value(iACsvConfig::Phi)).ToDouble() / m_PolarPlotPhiResolution;
		ft = t->GetValue(k, m_columnMapping->value(iACsvConfig::Theta)).ToDouble() / m_PolarPlotThetaResolution;
		ip = vtkMath::Round(fp);
		it = vtkMath::Round(ft);

		//if(ip > 360 || ip < 0 || it < 0 || it > 91)
		//	QString( "error computed phi" );

		if (ip == m_gPhi)
		{
			ip = 0;
		}

		tt = ot->GetValue(ip, it).ToInt();
		ot->SetValue(ip, it, tt + 1);

		if (maxF < tt + 1)
		{
			maxF = tt + 1;
		}
	}
	return maxF;
}

void dlg_FeatureScout::drawAnnotations(vtkRenderer* renderer)
{
	// annotations for phi
	vtkIdType numPoints = 12 + 6;
	double re = 30.0;

	auto poly = vtkSmartPointer<vtkPolyData>::New();
	auto pts = vtkSmartPointer<vtkPoints>::New();
	auto nameArray = vtkSmartPointer<vtkStringArray>::New();

	nameArray->SetName("name");
	pts->SetNumberOfPoints(numPoints);
	double x[3];

	for (vtkIdType i = 0; i < 12; ++i)
	{
		double phi = i * re * M_PI / 180.0;
		double rx = 100.0;

		if (i < 2)
		{
			rx = 95.0;
		}

		x[0] = rx * cos(phi);
		x[1] = rx * sin(phi);
		x[2] = 0.0;

		pts->SetPoint(i, x);
		vtkVariant v = i * re;
		nameArray->InsertNextValue(v.ToString());
	}

	// annotation for theta
	for (vtkIdType i = 12; i < numPoints; ++i)
	{
		double phi = 270.0 * M_PI / 180.0;
		double rx = (numPoints - i) * 15.0;
		x[0] = rx * cos(phi);
		x[1] = rx * sin(phi);
		x[2] = 0.0;

		pts->SetPoint(i, x);
		vtkVariant v = rx;
		nameArray->InsertNextValue(v.ToString());
	}
	poly->SetPoints(pts);
	poly->GetPointData()->AddArray(nameArray);

	auto mapper = vtkSmartPointer<vtkDynamic2DLabelMapper>::New();
	mapper->SetInputData(poly);
	mapper->SetLabelFormat("%s");
	mapper->SetLabelModeToLabelFieldData();
	mapper->SetFieldDataName("name");
	mapper->GetLabelTextProperty()->SetColor(0.0, 0.0, 0.0);
	mapper->GetLabelTextProperty()->SetFontSize(16);

	auto actor = vtkSmartPointer<vtkActor2D>::New();
	actor->SetMapper(mapper);
	renderer->AddActor(actor);
}

void dlg_FeatureScout::drawPolarPlotMesh(vtkRenderer* renderer)
{
	auto actor = vtkSmartPointer<vtkActor>::New();

	double xx, yy;
	double re = 15.0;
	int ap = 25;
	int at = 7;

	auto sGrid = vtkSmartPointer<vtkStructuredGrid>::New();
	sGrid->SetDimensions(at, ap, 1);
	int anzP = sGrid->GetNumberOfPoints();

	auto points = vtkSmartPointer<vtkPoints>::New();
	points->Allocate(anzP);

	for (int i = 0; i < ap; ++i)
	{
		double phi = i * re * M_PI / 180.0;

		for (int j = 0; j < at; ++j)
		{
			double rx = j * re;
			xx = rx * cos(phi);
			yy = rx * sin(phi);
			points->InsertNextPoint(xx, yy, 0.0);
		}
	}

	// add points to grid
	sGrid->SetPoints(points);

	auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

	auto plane = vtkSmartPointer<vtkStructuredGridGeometryFilter>::New();
	plane->SetExtent(0, at, 0, ap, 0, 0);
	plane->SetInputData(sGrid);
	mapper->SetInputConnection(plane->GetOutputPort());

	actor->SetMapper(mapper);
	actor->GetProperty()->SetColor(1, 1, 1);
	actor->GetProperty()->SetRepresentationToWireframe();
	actor->GetProperty()->SetLineWidth(0.1);
	actor->GetProperty()->SetOpacity(0.2);
	renderer->AddActor(actor);
}

void dlg_FeatureScout::drawOrientationScalarBar(vtkScalarsToColors* lut)
{
	m_scalarWidgetPP = vtkSmartPointer<vtkScalarBarWidget>::New();
	m_scalarBarPP = vtkSmartPointer<vtkScalarBarActor>::New();
	m_scalarBarPP->SetLookupTable(lut);
	m_scalarBarPP->GetLabelTextProperty()->SetColor(0, 0, 0);
	m_scalarBarPP->GetTitleTextProperty()->SetColor(0, 0, 0);
	m_scalarBarPP->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	m_scalarBarPP->SetTitle("Frequency");
	m_scalarBarPP->SetNumberOfLabels(5);
	m_scalarWidgetPP->SetInteractor(m_polarPlotWidget->interactor());
	m_scalarWidgetPP->SetScalarBarActor(m_scalarBarPP);
	m_scalarWidgetPP->SetEnabled(true);
	m_scalarWidgetPP->SetRepositionable(true);
	m_scalarWidgetPP->SetResizable(true);
	m_scalarWidgetPP->GetScalarBarActor()->SetTextPositionToSucceedScalarBar();
	auto sbr = vtkScalarBarRepresentation::SafeDownCast(m_scalarWidgetPP->GetRepresentation());
	sbr->SetPosition(0.88, 0.14);
	sbr->SetPosition2(0.11, 0.80);

}

void dlg_FeatureScout::updatePolarPlotView(vtkTable* it)
{
	if (!m_columnMapping->contains(iACsvConfig::Phi) || !m_columnMapping->contains(iACsvConfig::Theta))
	{
		LOG(lvlWarn, "It wasn't defined in which columns the angles phi and theta can be found, cannot set up polar plot view.");
		return;
	}
	m_dwPP->setWindowTitle("Orientation Distribution");

	// calculate object probability and save it to a table
	auto table = vtkSmartPointer<vtkTable>::New();
	int maxCount = this->calcOrientationProbability(it, table); // maximal count of the object orientation

	// Create a transfer function mapping scalar value to color
	auto cTFun = vtkSmartPointer<vtkColorTransferFunction>::New();

	//cold-warm-map
	//cTFun->AddRGBPoint(   0, 1.0, 1.0, 1.0 );
	//cTFun->AddRGBPoint(   1, 0.0, 1.0, 1.0 );
	//cTFun->AddRGBPoint( maxCount, 1.0, 0.0, 1.0 );

	//heatmap
	cTFun->AddRGBPoint(0.0, 0.74, 0.74, 0.74, 0.1, 0.0);					//gray
	cTFun->AddRGBPoint(maxCount * 1 / 9.0, 0.0, 0.0, 1.0, 0.1, 0.0);		//blue
	cTFun->AddRGBPoint(maxCount * 4 / 9.0, 1.0, 0.0, 0.0, 0.1, 0.0);		//red
	cTFun->AddRGBPoint(maxCount * 9 / 9.0, 1.0, 1.0, 0.0, 0.1, 0.0);		//yellow
	//cTFun->AddRGBPoint( maxCount, 1.0, 1.0, 1.0 );						//white

	// color array to save the colors for each point
	auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetNumberOfComponents(3);
	colors->SetName("Colors");

	auto points = vtkSmartPointer<vtkPoints>::New();
	for (int x = 0; x < m_gThe; ++x)
	{
		double rx = x * m_PolarPlotThetaResolution;

		for (int y = 0; y < m_gPhi; ++y)
		{
			double phi = y * m_PolarPlotPhiResolution * M_PI / 180.0;
			double xx = rx * cos(phi);
			double yy = rx * sin(phi);
			double zz = table->GetValue(y, x).ToDouble();

			if (m_draw3DPolarPlot)
			{
				points->InsertNextPoint(xx, yy, zz);
			}
			else
			{
				points->InsertNextPoint(xx, yy, 0.0);
			}

			double dcolor[3];

			cTFun->GetColor(zz, dcolor);
			unsigned char color[3];

			for (unsigned int j = 0; j < 3; ++j)
			{
				color[j] = static_cast<unsigned char>(255.0 * dcolor[j]);
			}

			colors->InsertNextTypedTuple(color);
		}
	}

	// Add the grid points to a polydata object
	auto inputPolyData = vtkSmartPointer<vtkPolyData>::New();
	inputPolyData->SetPoints(points);
	inputPolyData->GetPointData()->SetScalars(colors);

	// initialize and triangulate the grid points for one time
	// the triangulated net will be reused later to get the polydata
	auto delaunay = vtkSmartPointer<vtkDelaunay2D>::New();
	delaunay->SetInputData(inputPolyData);
	delaunay->Update();
	vtkPolyData* outputPolyData = delaunay->GetOutput();
	auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData(outputPolyData);
	mapper->SetLookupTable(cTFun);
	auto actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	actor->GetProperty()->LightingOff();

	auto renderer = vtkSmartPointer<vtkRenderer>::New();
	renderer->SetBackground(1, 1, 1);

	auto renW = m_polarPlotWidget->renderWindow();
	auto ren = renW->GetRenderers()->GetFirstRenderer();
	if (ren)
	{
		renW->RemoveRenderer(ren);
	}
	renderer->AddActor(actor);
	renW->AddRenderer(renderer);

	drawPolarPlotMesh(renderer);
	drawAnnotations(renderer);
	drawOrientationScalarBar(cTFun);
	renderer->ResetCamera();
	m_polarPlotWidget->renderWindow()->Render();
}

void dlg_FeatureScout::setupPolarPlotResolution(float grad)
{
	m_gPhi = vtkMath::Floor(360.0f / grad);
	m_gThe = vtkMath::Floor(90.0f / grad);
	m_PolarPlotPhiResolution = 360.0f / m_gPhi;
	m_PolarPlotThetaResolution = 90.0f / m_gThe;
	m_gThe = m_gThe + 1;
}

bool dlg_FeatureScout::OpenBlobVisDialog()
{
	iABlobCluster* blob = nullptr;
	if (m_blobMap.contains(m_activeClassItem->text()))
	{
		blob = m_blobMap[m_activeClassItem->text()];
	}
	iAParameterDlg::ParamListT params;
	addParameter(params, "Range:", iAValueType::Continuous, blob ? blob->GetRange() : m_blobManager->GetRange());
	addParameter(params, "Blob body:", iAValueType::Boolean, blob ? blob->GetShowBlob() : m_blobManager->GetShowBlob());
	addParameter(params, "Use Depth peeling:", iAValueType::Boolean, m_blobManager->GetUseDepthPeeling());
	addParameter(params, "Blob opacity [0,1]:", iAValueType::Continuous, blob ? blob->GetBlobOpacity() : m_blobManager->GetBlobOpacity());
	addParameter(params, "Silhouettes:", iAValueType::Boolean, blob ? blob->GetSilhouette() : m_blobManager->GetSilhouettes());
	addParameter(params, "Silhouettes opacity [0,1]:", iAValueType::Continuous, blob ? blob->GetSilhouetteOpacity() : m_blobManager->GetSilhouetteOpacity());
	addParameter(params, "3D labels:", iAValueType::Boolean, blob ? blob->GetLabel() : m_blobManager->GetLabeling());
	addParameter(params, "Smart overlapping:", iAValueType::Boolean, m_blobManager->OverlappingIsEnabled());
	addParameter(params, "Separation distance (if smart overlapping):", iAValueType::Continuous, m_blobManager->GetOverlapThreshold());
	addParameter(params, "Smooth after smart overlapping:", iAValueType::Boolean, blob ? blob->GetSmoothing() : m_blobManager->GetSmoothing());
	addParameter(params, "Gaussian blurring of the blob:", iAValueType::Boolean, m_blobManager->GetGaussianBlur());
	addParameter(params, "Gaussian blur variance:", iAValueType::Continuous, blob ? blob->GetGaussianBlurVariance() : m_blobManager->GetGaussianBlurVariance());
	addParameter(params, "Dimension X", iAValueType::Discrete, blob ? blob->GetDimensions()[0] : m_blobManager->GetDimensions()[0]);
	addParameter(params, "Dimension Y", iAValueType::Discrete, blob ? blob->GetDimensions()[1] : m_blobManager->GetDimensions()[1]);
	addParameter(params, "Dimension Z", iAValueType::Discrete, blob ? blob->GetDimensions()[2] : m_blobManager->GetDimensions()[2]);
	iAParameterDlg dlg(this, "Blob rendering preferences", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	auto values = dlg.parameterValues();
	m_blobManager->SetRange(values["Range:"].toDouble());
	m_blobManager->SetShowBlob(values["Blob body:"].toBool());
	m_blobManager->SetUseDepthPeeling(values["Use Depth peeling:"].toBool());
	m_blobManager->SetBlobOpacity(values["Blob opacity [0,1]:"].toDouble());
	m_blobManager->SetSilhouettes(values["Silhouettes:"].toBool());
	m_blobManager->SetSilhouetteOpacity(values["Silhouettes opacity [0,1]:"].toDouble());
	m_blobManager->SetLabeling(values["3D labels:"].toBool());
	m_blobManager->SetOverlappingEnabled(values["Smart overlapping:"].toBool());
	m_blobManager->SetOverlapThreshold(values["Separation distance (if smart overlapping):"].toDouble());
	m_blobManager->SetSmoothing(values["Smooth after smart overlapping:"].toBool());
	m_blobManager->SetGaussianBlur(values["Gaussian blurring of the blob:"].toBool());
	m_blobManager->SetGaussianBlurVariance(values["Gaussian blur variance:"].toDouble());
	int dimens[3] = {values["Dimension X"].toInt(), values["Dimension Y"].toInt(), values["Dimension Z"].toInt()};
	m_blobManager->SetDimensions(dimens);
	return true;
}

void dlg_FeatureScout::SaveBlobMovie()
{
	QString movie_file_types = GetAvailableMovieFormats();
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(this, "Movie Export", "Sorry, but movie export support is disabled.");
		return;
	}
	iAParameterDlg::ParamListT params;
	QStringList modes = (QStringList() << tr("No rotation") << tr("Rotate Z") << tr("Rotate X") << tr("Rotate Y"));
	addParameter(params, "Rotation mode", iAValueType::Categorical, modes);
	addParameter(params, "Number of frames:", iAValueType::Discrete, 24, 1);
	addParameter(params, "Range from:", iAValueType::Continuous, m_blobManager->GetRange());
	addParameter(params, "Range to:", iAValueType::Continuous, m_blobManager->GetRange());
	addParameter(params, "Blob body:", iAValueType::Boolean, m_blobManager->GetShowBlob());
	addParameter(params, "Blob opacity from [0,1]:", iAValueType::Continuous, m_blobManager->GetBlobOpacity(), 0, 1);
	addParameter(params, "Blob opacity to:", iAValueType::Continuous, m_blobManager->GetBlobOpacity(), 0, 1);
	addParameter(params, "Silhouettes:", iAValueType::Boolean, m_blobManager->GetSilhouettes());
	addParameter(params, "Silhouettes opacity from [0,1]:", iAValueType::Continuous, m_blobManager->GetSilhouetteOpacity());
	addParameter(params, "Silhouettes opacity to:", iAValueType::Continuous, m_blobManager->GetSilhouetteOpacity());
	addParameter(params, "3D labels:", iAValueType::Boolean, m_blobManager->GetLabeling());
	addParameter(params, "Smart overlapping:", iAValueType::Boolean, m_blobManager->OverlappingIsEnabled());
	addParameter(params, "Separation distance from (if smart overlapping):", iAValueType::Continuous, m_blobManager->GetOverlapThreshold());
	addParameter(params, "Separation distance to:", iAValueType::Continuous, m_blobManager->GetOverlapThreshold());
	addParameter(params, "Smooth after smart overlapping:", iAValueType::Boolean, m_blobManager->GetSmoothing());
	addParameter(params, "Gaussian blurring of the blob:", iAValueType::Boolean, m_blobManager->GetGaussianBlur());
	addParameter(params, "Gaussian blur variance from:", iAValueType::Continuous, m_blobManager->GetGaussianBlurVariance());
	addParameter(params, "Gaussian blur variance to:", iAValueType::Continuous, m_blobManager->GetGaussianBlurVariance());
	addParameter(params, "Dimension X from", iAValueType::Discrete, m_blobManager->GetDimensions()[0]);
	addParameter(params, "Dimension X to:" , iAValueType::Discrete, m_blobManager->GetDimensions()[0]);
	addParameter(params, "Dimension Y from", iAValueType::Discrete, m_blobManager->GetDimensions()[1]);
	addParameter(params, "Dimension Y to:" , iAValueType::Discrete, m_blobManager->GetDimensions()[1]);
	addParameter(params, "Dimension Z from", iAValueType::Discrete, m_blobManager->GetDimensions()[2]);
	addParameter(params, "Dimension Z to:" , iAValueType::Discrete, m_blobManager->GetDimensions()[2]);

	iAParameterDlg dlg(this, "Blob movie rendering options", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	QString mode = values["Rotation mode"].toString();
	int imode = modes.indexOf(mode);
	m_blobManager->SetShowBlob(values["Blob body:"].toBool());
	m_blobManager->SetSilhouettes(values["Silhouettes:"].toBool());
	m_blobManager->SetLabeling(values["3D labels:"].toBool());
	m_blobManager->SetOverlappingEnabled(values["Smart overlapping:"].toBool());
	m_blobManager->SetSmoothing(values["Smooth after smart overlapping:"].toBool());
	m_blobManager->SetGaussianBlur(values["Gaussian blurring of the blob:"].toBool());
	size_t numFrames = values["Number of frames:"].toInt();
	double range[2] = {values["Range from:"].toDouble(), values["Range to:"].toDouble()};
	double blobOpacity[2] = {values["Blob opacity from [0,1]:"].toDouble(), values["Blob opacity to:"].toDouble()};
	double silhouetteOpacity[2] = {values["Silhouettes opacity from [0,1]:"].toDouble(), values["Silhouettes opacity to:"].toDouble()};
	double overlapThreshold[2] = {values["Separation distance from (if smart overlapping):"].toDouble(), values["Separation distance to:"].toDouble()};
	double gaussianBlurVariance[2] = {values["Gaussian blur variance from:"].toDouble(), values["Gaussian blur variance to:"].toDouble()};
	int dimX[2] = {values["Dimension X from"].toInt(), values["Dimension X to"].toInt()};
	int dimY[2] = {values["Dimension Y from"].toInt(), values["Dimension Y to"].toInt()};
	int dimZ[2] = {values["Dimension Z from"].toInt(), values["Dimension Z to"].toInt()};

	QFileInfo fileInfo = m_activeChild->fileInfo();

	m_blobManager->SaveMovie(m_activeChild,
		m_renderer,
		m_renderer->renderer()->GetActiveCamera(),
		m_renderer->interactor(),
		m_renderer->renderWindow(),
		numFrames,
		range,
		blobOpacity,
		silhouetteOpacity,
		overlapThreshold,
		gaussianBlurVariance,
		dimX, dimY, dimZ,
		QFileDialog::getSaveFileName(
			this,
			tr("Export movie %1").arg(mode),
			fileInfo.absolutePath() + "/" + ((mode.isEmpty()) ? fileInfo.baseName() : fileInfo.baseName() + "_" + mode), movie_file_types),
		imode
	);
}

void dlg_FeatureScout::initFeatureScoutUI()
{
	m_pcWidget = new iAQVTKWidget();
	m_polarPlotWidget = new iAQVTKWidget();
	m_lengthDistrWidget = new iAQVTKWidget();
	m_dwPC = new iADockWidgetWrapper(m_pcWidget, "Parallel Coordinates", "FeatureScoutPC");
	m_dwPP = new iAPolarPlotWidget(this);
	m_dwPP->legendLayout->addWidget(m_polarPlotWidget);
	m_activeChild->addDockWidget(Qt::RightDockWidgetArea, this);
	m_activeChild->addDockWidget(Qt::RightDockWidgetArea, m_dwPC);
	m_activeChild->addDockWidget(Qt::RightDockWidgetArea, m_dwPP);
	m_dwPP->colorMapSelection->hide();
	if (m_filterID == iAObjectType::Voids)
	{
		m_dwPP->hide();
	}
	connect(m_dwPP->orientationColorMap, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_FeatureScout::RenderOrientation);

	if (m_visualization == iACsvConfig::UseVolume)
	{
		m_activeChild->imagePropertyDockWidget()->hide();
	}
	m_activeChild->histogramDockWidget()->hide();
	m_activeChild->renderDockWidget()->hide();
	for (int i = 0; i < 3; ++i)
	{
		m_activeChild->slicerDockWidget(i)->hide();
	}
	m_activeChild->dataDockWidget()->hide();
}

void dlg_FeatureScout::changeFeatureScout_Options(int idx)
{
	switch (idx)
	{
	case 0:         // option menu, do nothing
	default:
		break;
	case 1:			// blob Rendering, trigger OpenBlobVisDialog()
		this->OpenBlobVisDialog();
		break;

	case 3: MultiClassRendering();      break;
	case 4: RenderMeanObject();         break;
	case 5: RenderOrientation();        break;
	case 7: RenderLengthDistribution(); break;

	case 6:			// plot scatterplot matrix
		if (m_dwSPM)
		{
			QMessageBox::information(this, "FeatureScout", "Scatterplot Matrix already created.");
			return;
		}
		showScatterPlot();
		break;
	case 8:
		showPCSettings();
		break;
	}
}

void dlg_FeatureScout::updateAxisProperties()
{
	int axis_count = m_pcChart->GetNumberOfAxes();
	m_pcTickCount = (m_pcTickCount < PCMinTicksCount) ? PCMinTicksCount : m_pcTickCount;
	int visibleColIdx = 0;
	for (int i = 0; i < axis_count; i++)
	{
		while (!m_columnVisibility[visibleColIdx])
		{
			++visibleColIdx;
		}
		vtkAxis* axis = m_pcChart->GetAxis(i);
		if (!axis)
		{
			LOG(lvlWarn, QString("Invalid axis %1 in Parallel Coordinates!").arg(i));
			continue;
		}
		axis->GetLabelProperties()->SetFontSize(m_pcFontSize);
		axis->GetTitleProperties()->SetFontSize(m_pcFontSize);
		vtkDataArray* columnData = vtkDataArray::SafeDownCast(m_chartTable->GetColumn(visibleColIdx));
		double* const range = columnData->GetRange();
		if (range[0] != range[1])
		{
			// if min == max, then leave NumberOfTicks at default -1, otherwise there will be no ticks and no lines shown
			axis->SetNumberOfTicks(m_pcTickCount);
		}
		axis->Update();
		++visibleColIdx;
	}
	m_pcChart->Update();
}
