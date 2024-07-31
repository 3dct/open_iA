// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

// objectvis:
#include "iACsvIO.h"
#include "iALineObjectVis.h"
#include "iAObjectsData.h"
#include "iAObjectVis.h"

// guibase:
#include <iADockWidgetWrapper.h>
#include <iAJobListView.h>
#include <iAMdiChild.h>
#include <iAMovieHelper.h>
#include <iAParameterDlg.h>
#include <iAQVTKWidget.h>
#include <iARenderer.h>
#include <iARunAsync.h>
#include <iADefaultSettings.h>
#include <iAToolsITK.h>
#include <iAThemeHelper.h>

// io:
#include <iAFileTypeRegistry.h>

// base:
#include <defines.h>    // for DIM
#include <iAConnector.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iALookupTable.h>
#include <iAPolyData.h>
#include <iATypedCallHelper.h>

#include <itkImageRegionIterator.h>

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAxis.h>
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
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
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
#include <vtkVariantArray.h>
#include <vtkVersionMacros.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
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
const QString ClassesProjectFile("Classes");

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

//! define class here directly instead of using iAQTtoUIConnector, to be able to forward-declare
class iAPolarPlotWidget : public QWidget, public Ui_FeatureScoutPP
{
public:
	iAPolarPlotWidget(QWidget* parent) : QWidget(parent)
	{
		setupUi(this);
	}
};

//! define class here directly instead of using iAQTtoUIConnector, to be able to forward-declare
class iAClassExplorer: public QWidget, public Ui_FeatureScoutCE
{
public:
	iAClassExplorer(QWidget* parent) : QWidget(parent)
	{
		setupUi(this);
	}
};

namespace
{
	const QString PCLineWidth = "Line Width";
	const QString PCOpacity = "Opacity";
	const QString PCTickCount = "Tick Count";
	const QString PCFontSize = "Font Size";

	const int PCMinTickCount = 2; //!< minimum number of ticks
}

inline constexpr char FeatureScoutPCSettingsName[] = "FeatureScout: Parallel Coordinates";
inline constexpr char FeatureScoutPCSettingsMenu[] = "Default Settings/FeatureScout: Parallel Coordinates";
//! a FeatureScout slicer window.
class iAFeatureScoutPCSettings : iASettingsObject<FeatureScoutPCSettingsMenu, iAFeatureScoutPCSettings>
{
public:
	static iAAttributes& defaultAttributes()
	{
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			addAttr(attr, PCLineWidth, iAValueType::Continuous, 0.1, 0.001, 100000);     //!< width of the line for each object
			addAttr(attr, PCOpacity, iAValueType::Discrete, 90, 0, 255);                 //!< opacity of lines
			addAttr(attr, PCTickCount, iAValueType::Discrete, 10, PCMinTickCount, 255);  //!< count of ticks per axis
			addAttr(attr, PCFontSize, iAValueType::Discrete, 12, 0, 255);                //!< font size of titles and tick labels
			selfRegister();
		}
		return attr;
	}
};

const QString dlg_FeatureScout::UnclassifiedColorName("darkGray");

dlg_FeatureScout::dlg_FeatureScout(iAMdiChild* parent, iAObjectType objectType, QString const& fileName,
	iAObjectsData const * objData, iAObjectVis* objVis) :
	m_activeChild(parent),
	m_renderer(parent->renderer()),
	m_objCnt(objData->m_table->GetNumberOfRows()),
	m_colCnt(objData->m_table->GetNumberOfColumns()),
	m_objectType(objectType),
	m_draw3DPolarPlot(false),
	m_renderMode(rmSingleClass),
	m_singleObjectSelected(false),
	m_visType(objData->m_visType),
	m_sourcePath(parent->filePath()),
	m_columnMapping(objData->m_colMapping),
	m_csvTable(objData->m_table),
	m_elementTable(vtkSmartPointer<vtkTable>::New()),
	m_chartTable(vtkSmartPointer<vtkTable>::New()),
	m_columnVisibility(m_colCnt, false),
	m_lengthDistrView(vtkSmartPointer<vtkContextView>::New()),
	m_pcView(vtkSmartPointer<vtkContextView>::New()),
	m_pcConnections(vtkSmartPointer<vtkEventQtSlotConnect>::New()),
	m_pcMultiClassLUT(vtkSmartPointer<vtkLookupTable>::New()),
	m_classTreeView(new QTreeView()),
	m_elementTableView(new QTableView()),
	m_classTreeModel(new QStandardItemModel(this)),
	m_elementTableModel(new QStandardItemModel(static_cast<int>(m_colCnt - 1), 4, this)),
	m_blobManager(std::make_unique<iABlobManager>()),
	m_blobVisDialog(new dlg_blobVisualization()),
	m_dvContextView(vtkSmartPointer<vtkContextView>::New()),
	m_pcWidget(new iAQVTKWidget()),
	m_polarPlotWidget(new iAQVTKWidget()),
	m_lengthDistrWidget(new iAQVTKWidget()),
	m_ppWidget(new iAPolarPlotWidget(m_activeChild)),
	m_classExplorer(new iAClassExplorer(parent)),
	m_dwPC(new iADockWidgetWrapper(m_pcWidget, "Parallel Coordinates", "FeatureScoutPC", "https://github.com/3dct/open_iA/wiki/FeatureScout")),
	m_dwPP(new iADockWidgetWrapper(m_ppWidget, "Orientation Plot", "FeatureScoutPP", "https://github.com/3dct/open_iA/wiki/FeatureScout")),
	m_dwCE(new iADockWidgetWrapper(m_classExplorer, "Class Explorer", "FeatureScoutMainDlg", "https://github.com/3dct/open_iA/wiki/FeatureScout")),
	m_dwDV(nullptr),
	m_dwSPM(nullptr),
	m_splom(new iAFeatureScoutSPLOM()),
	m_3dvis(objVis)
{
	setupPolarPlotResolution(3.0);

	m_chartTable->DeepCopy(m_csvTable);
	m_tableList.push_back(m_chartTable); // at start, the unclassified class contains all objects; could be skipped if classes are loaded later...

	initFeatureScoutUI();
	m_lengthDistrWidget->hide();

	initColumnVisibility();
	setupClassExplorer();
	setupViews();
	setupConnections();

	if (objData->m_visType != iAObjectVisType::UseVolume && m_activeChild->dataSetMap().empty())
	{
		parent->setWindowTitleAndFile(QString("FeatureScout - %1 (%2)").arg(QFileInfo(fileName).fileName())
			.arg(MapObjectTypeToString(m_objectType)));
	}
	if (objData->m_visType == iAObjectVisType::UseVolume)
	{
		singleRendering();
	}
	if (m_3dvis)
	{
		m_3dactor = m_3dvis->createActor(parent->renderer()->renderer());
		m_3dactor->show();
		connect(m_3dactor.get(), &iAObjectVisActor::updated, m_activeChild, &iAMdiChild::updateRenderer);
		parent->renderer()->renderer()->ResetCamera();
		m_blobManager->SetRenderers(parent->renderer()->renderer(), m_renderer->labelRenderer());
		m_blobManager->SetBounds(m_3dvis->bounds());
		m_blobManager->SetProtrusion(1.5);
		int dimens[3] = { 50, 50, 50 };
		m_blobManager->SetDimensions(dimens);
	}
}

dlg_FeatureScout::~dlg_FeatureScout()
{
	delete m_dwPC;
	delete m_dwDV;
	delete m_dwSPM;
	delete m_dwPP;
	delete m_dwCE;
}

std::vector<size_t> dlg_FeatureScout::getPCSelection()
{
	std::vector<size_t> selectedIndices;
	auto pcSelection = m_pcChart->GetPlot(0)->GetSelection();
	vtkIdType countSelection = pcSelection->GetNumberOfValues();
	for (vtkIdType idx = 0; idx < countSelection; ++idx)
	{
		size_t objID = pcSelection->GetVariantValue(idx).ToUnsignedLongLong();
		selectedIndices.push_back(objID);
	}
	return selectedIndices;
}

void dlg_FeatureScout::pcSelectionChanged(vtkObject*, unsigned long, void*, void*, vtkCommand*)
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
	renderSelection(selectionIndices);
}

void dlg_FeatureScout::setPCChartData(bool specialRendering)
{
	m_pcWidget->setEnabled(!specialRendering); // to disable selection
	if (specialRendering)
	{   // for the special renderings, we use all data:
		m_chartTable = vtkSmartPointer<vtkTable>::New();
		m_chartTable->DeepCopy(m_csvTable);
	}
	if (m_pcView->GetScene()->GetNumberOfItems() > 0)
	{
		m_pcView->GetScene()->RemoveItem(0u);
	}
	m_pcChart = vtkSmartPointer<vtkChartParallelCoordinates>::New();
	m_pcChart->GetPlot(0)->SetInputData(m_chartTable);
	applyPCSettings(m_pcSettings);
	m_pcView->GetScene()->AddItem(m_pcChart);
	m_pcConnections->Connect(m_pcChart, vtkCommand::SelectionChangedEvent, this,
		SLOT(pcSelectionChanged(vtkObject*, unsigned long, void*, void*, vtkCommand*)));
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
	m_elementTableModel->item(static_cast<int>(paramIndex), 0)->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
	// itemChanged signal from elementTableModel takes care about updating PC (see updatePCColumnValues slot)
}

void dlg_FeatureScout::renderLUTChanges(std::shared_ptr<iALookupTable> lut, size_t colInd)
{
	iALineObjectVis* lov = dynamic_cast<iALineObjectVis*>(m_3dvis);
	if (lov)
	{
		lov->setLookupTable(lut, colInd);
	}
}

void dlg_FeatureScout::updatePCColumnVisibility()
{
	for (int j = 0; j < m_colCnt; ++j)
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
	if (m_objectType == iAObjectType::Fibers) // Fibers - (a11, a22, a33,) theta, phi, xm, ym, zm, straightlength, diameter(, volume)
	{
		m_columnVisibility[(*m_columnMapping)[iACsvConfig::Theta]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Phi]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::CenterX]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::CenterY]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::CenterZ]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Length]] =
			m_columnVisibility[(*m_columnMapping)[iACsvConfig::Diameter]] = true;
	}
	else if (m_objectType == iAObjectType::Voids) // Pores - (volume, dimx, dimy, dimz,) posx, posy, posz(, shapefactor)
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

void dlg_FeatureScout::setupClassExplorer()
{
	m_elementTableModel->setHeaderData(0, Qt::Horizontal, tr("Element"));
	m_elementTableModel->setHeaderData(1, Qt::Horizontal, tr("Minimum"));
	m_elementTableModel->setHeaderData(2, Qt::Horizontal, tr("Maximum"));
	m_elementTableModel->setHeaderData(3, Qt::Horizontal, tr("Average"));
	for (int i = 0; i < m_colCnt - 1; ++i)
	{
		Qt::CheckState checkState = (m_columnVisibility[i]) ? Qt::Checked : Qt::Unchecked;
		m_elementTableModel->setData(m_elementTableModel->index(i, 0, QModelIndex()), checkState, Qt::CheckStateRole);
		m_elementTableModel->item(i, 0)->setCheckable(true);
	}
	calculateElementTable();
	initElementTableModel();
	m_elementTableView->setModel(m_elementTableModel);
	m_elementTableView->resizeColumnsToContents();

	m_classTreeModel->setHorizontalHeaderItem(0, new QStandardItem("Class"));
	m_classTreeModel->setHorizontalHeaderItem(1, new QStandardItem("Count"));
	m_classTreeModel->setHorizontalHeaderItem(2, new QStandardItem("Percent"));
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();
	QList<QStandardItem*> stammItem = prepareRow("Unclassified", QString("%1").arg(m_objCnt), "100");
	stammItem.first()->setData(QColor(UnclassifiedColorName), Qt::DecorationRole);
	m_colorList.push_back(QColor("darkGray"));
	rootItem->appendRow(stammItem);
	for (int i = 0; i < m_objCnt; ++i)
	{
		vtkVariant v = m_chartTable->GetColumn(0)->GetVariantValue(i);
		QStandardItem* item = new QStandardItem(QString::fromUtf8(v.ToString().c_str()).trimmed());
		stammItem.first()->appendRow(item);
	}
	m_activeClassItem = stammItem.first();
	m_classTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_classExplorer->ClassLayout->addWidget(m_classTreeView);
	m_classExplorer->ElementLayout->addWidget(m_elementTableView);
	m_classTreeView->setModel(m_classTreeModel);
	// set first column of the classTreeView to minimal (not stretched)
	m_classTreeView->resizeColumnToContents(0);
	m_classTreeView->header()->setStretchLastSection(false);
	m_classTreeView->setExpandsOnDoubleClick(false);
}

void dlg_FeatureScout::setupViews()
{
	m_lengthDistrView->SetRenderWindow(m_lengthDistrWidget->renderWindow());
	m_lengthDistrView->SetInteractor(m_lengthDistrWidget->interactor());

	m_pcView->SetRenderWindow(m_pcWidget->renderWindow());
	m_pcView->SetInteractor(m_pcWidget->interactor());
	m_pcConnections->Connect(m_pcWidget->renderWindow()->GetInteractor(),
		vtkCommand::RightButtonReleaseEvent,
		this,
		SLOT(pcRightButtonReleased(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
		nullptr, 1.0);
	m_pcConnections->Connect(m_pcWidget->renderWindow()->GetInteractor(),
		vtkCommand::RightButtonPressEvent,
		this,
		SLOT(pcRightButtonPressed(vtkObject*, unsigned long, void*, void*, vtkCommand*)));
	m_pcSettings = extractValues(iAFeatureScoutPCSettings::defaultAttributes());
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
	nameArr->SetNumberOfValues(m_colCnt);
	v1Arr->SetNumberOfValues(m_colCnt);
	v2Arr->SetNumberOfValues(m_colCnt);
	v3Arr->SetNumberOfValues(m_colCnt);

	// convert IDs in String to Int
	nameArr->SetValue(0, m_csvTable->GetColumnName(0));
	v1Arr->SetValue(0, m_chartTable->GetColumn(0)->GetVariantValue(0).ToFloat());
	v2Arr->SetValue(0, m_chartTable->GetColumn(0)->GetVariantValue(m_chartTable->GetNumberOfRows() - 1).ToFloat());
	v3Arr->SetValue(0, 0);

	for (int i = 1; i < m_colCnt; ++i)
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

		for (int i = 0; i < m_colCnt - 1; ++i) // number of rows
		{
			for (int j = 0; j < 4; ++j)
			{
				vtkVariant v = m_elementTable->GetValue(i, j);
				QString str;
				if (j == 0)
				{
					str = QString::fromUtf8(v.ToString().c_str()).trimmed();
				}
				else
				{
					if (i == 0 || i == m_colCnt - 1)
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

		for (int i = 0; i < m_colCnt - 1; ++i)
		{
			vtkVariant v = m_chartTable->GetValue(idx, i);
			QString str = QString::number(v.ToDouble(), 'f', 2);

			if (i == 0 || i == m_colCnt - 1)
			{
				str = QString::number(v.ToInt());
			}

			m_elementTableModel->setData(m_elementTableModel->index(i, 1, QModelIndex()), str);
		}
	}
}

void dlg_FeatureScout::setupConnections()
{
	// create ClassTreeView context menu actions
	m_blobRendering = new QAction(tr("Enable blob rendering"), m_classTreeView);
	m_blobRemoveRendering = new QAction(tr("Disable blob rendering"), m_classTreeView);
	m_saveBlobMovie = new QAction(tr("Save blob movie"), m_classTreeView);
	m_objectDelete = new QAction(tr("Delete object"), m_classTreeView);

	connect(m_blobRendering, &QAction::triggered, this, &dlg_FeatureScout::enableBlobRendering);
	connect(m_blobRemoveRendering, &QAction::triggered, this, &dlg_FeatureScout::disableBlobRendering);
	connect(m_saveBlobMovie, &QAction::triggered, this, &dlg_FeatureScout::saveBlobMovie);
	connect(m_objectDelete, &QAction::triggered, this, &dlg_FeatureScout::deleteObject);
	connect(m_classTreeView, &QTreeView::customContextMenuRequested, this, &dlg_FeatureScout::showContextMenu);
	connect(m_classExplorer->add_class, &QToolButton::clicked, this, &dlg_FeatureScout::classAddButton);
	connect(m_classExplorer->save_class, &QToolButton::released, this, &dlg_FeatureScout::classSaveButton);
	connect(m_classExplorer->load_class, &QToolButton::released, this, &dlg_FeatureScout::classLoadButton);
	connect(m_classExplorer->delete_class, &QToolButton::clicked, this, &dlg_FeatureScout::classDeleteButton);
	connect(m_classExplorer->savexml, &QToolButton::released, this, &dlg_FeatureScout::wisetexSaveButton);
	connect(m_classExplorer->export_class, &QPushButton::clicked, this, &dlg_FeatureScout::classExportButton);
	connect(m_classExplorer->distributionCSV, &QToolButton::released, this, &dlg_FeatureScout::csvDVSaveButton);

	connect(m_elementTableModel, &QStandardItemModel::itemChanged, this, &dlg_FeatureScout::updateVisibility);
	connect(m_classTreeView, &QTreeView::clicked, this, &dlg_FeatureScout::itemClicked);
	connect(m_classTreeView, &QTreeView::activated, this, &dlg_FeatureScout::itemClicked);
	connect(m_classTreeView, &QTreeView::doubleClicked, this, &dlg_FeatureScout::itemDoubleClicked);

	connect(m_splom.get(), &iAFeatureScoutSPLOM::selectionModified, this, &dlg_FeatureScout::spSelInformsPCChart);
	connect(m_splom.get(), &iAFeatureScoutSPLOM::addClass, this, &dlg_FeatureScout::classAddButton);
	connect(m_splom.get(), &iAFeatureScoutSPLOM::parameterVisibilityChanged, this, &dlg_FeatureScout::spParameterVisibilityChanged);
	connect(m_splom.get(), &iAFeatureScoutSPLOM::renderLUTChanges, this, &dlg_FeatureScout::renderLUTChanges);

	connect(m_ppWidget->orientationColorMap, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_FeatureScout::renderOrientation);
}

void dlg_FeatureScout::multiClassRendering()
{
	if (!m_3dvis)
	{
		return;
	}
	showOrientationDistribution();
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();
	int classCount = rootItem->rowCount();

	if (classCount == 1)
	{
		return;
	}
	m_renderMode = rmMultiClass;

	double alpha = calculateOpacity(rootItem);
	m_splom->multiClassRendering(m_colorList);
	m_splom->enableSelection(false);

	updateMultiClassLookupTable(alpha);
	setPCChartData(true);
	auto plot = dynamic_cast<vtkPlotParallelCoordinates*>(m_pcChart->GetPlot(0));
	plot->SetScalarVisibility(1);
	plot->SetLookupTable(m_pcMultiClassLUT);
	plot->SelectColorArray(iACsvIO::ColNameClassID);
	m_pcChart->SetSize(m_pcChart->GetSize());
	m_pcChart->GetPlot(0)->SetOpacity(0.8);
	m_pcView->Render();

	m_3dvis->multiClassRendering(m_colorList, rootItem, alpha);
}

void dlg_FeatureScout::singleRendering(int objectID)
{
	int cID = m_activeClassItem->index().row();
	QColor classColor = m_colorList.at(cID);
	m_3dvis->renderSingle(objectID, cID, classColor, m_activeClassItem);
}

void dlg_FeatureScout::renderSelection(std::vector<size_t> const& selInds)
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
	if (m_3dvis)
	{
		m_3dvis->renderSelection(sortedSelInds, selectedClassID, classColor, m_activeClassItem);
	}
}

void dlg_FeatureScout::renderMeanObject()
{
	if (m_visType != iAObjectVisType::UseVolume)
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "Mean objects feature only available for the Labeled Volume visualization at the moment!");
		return;
	}
	int classCount = m_classTreeModel->invisibleRootItem()->rowCount();
	if (classCount < 2)	// unclassified class only
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "No defined class (except the 'unclassified' class) - please create at least one class first!");
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
	m_meanObject->render(classNames, m_tableList, m_objectType,
		m_dwSPM ? m_dwSPM : (m_dwDV ? m_dwDV : m_dwPC), m_renderer->renderer()->GetActiveCamera(),
		m_colorList);
}

void dlg_FeatureScout::renderOrientation()
{
	if (!m_3dvis)
	{
		return;
	}
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
			double recCoord[3] = { std::sin(theta_rad) * std::cos(phi_rad),
				std::sin(theta_rad) * std::sin(phi_rad),
				std::cos(theta_rad) };
			double* p = static_cast<double*>(oi->GetScalarPointer(theta, phi, 0));
			vtkMath::Normalize(recCoord);
			getColorMap(m_ppWidget->orientationColorMap->currentIndex())(recCoord, p);
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
			angle = phi * vtkMath::Pi() / 180.0;
			xx = theta * std::cos(angle);
			yy = theta * std::sin(angle);
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
	drawPolarPlotMesh(renderer);
	drawAnnotations(renderer);

	m_activeChild->updateViews();
	m_ppWidget->colorMapSelection->show();
	renW->Render();
}

void dlg_FeatureScout::selectionChanged3D()
{
	auto vis = dynamic_cast<iAColoredPolyObjectVis*>(m_3dvis);
	if (!vis)
	{
		LOG(lvlError, "Invalid VIS for 3D selection change!");
		return;
	}
	auto sel = vis->selection();
	setPCSelection(sel);
	m_splom->setFilteredSelection(sel);	// TODO: test with classes!
	LOG(lvlDebug, QString("New selection with %1 selected objects!").arg(sel.size()));
}

void dlg_FeatureScout::renderLengthDistribution()
{
	if (!m_3dvis)
	{
		return;
	}
	m_renderMode = rmLengthDistribution;
	setPCChartData(true);
	m_splom->enableSelection(false);
	m_splom->setFilter(-1);

	double range[2] = { 0.0, 0.0 };
	auto length = vtkDataArray::SafeDownCast(m_csvTable->GetColumn(m_columnMapping->value(iACsvConfig::Length)));
	QString title = QString("%1 Frequency Distribution").arg(m_csvTable->GetColumnName(m_columnMapping->value(iACsvConfig::Length)));
	m_dwPP->setWindowTitle(title);
	int numberOfBins = (m_objectType == iAObjectType::Fibers) ? 8 : 3;  // TODO: setting?

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
	if (m_objectType == iAObjectType::Fibers)
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

	m_3dvis->renderLengthDistribution(cTFun, extents, halfInc, m_objectType, range);

	m_ppWidget->colorMapSelection->hide();
	showLengthDistribution(true, cTFun);
	m_renderer->update();

	// plot length distribution
	auto chart = vtkSmartPointer<vtkChartXY>::New();
	chart->SetTitle(title.toUtf8().constData());
	chart->GetTitleProperties()->SetFontSize((m_objectType == iAObjectType::Fibers) ? 15 : 12); // TODO: setting?
	vtkPlot* plot = chart->AddPlot(vtkChartXY::BAR);
	plot->SetInputData(fldTable, 0, 1);
	plot->GetXAxis()->SetTitle("Length in microns");
	plot->GetYAxis()->SetTitle("Frequency");
	m_lengthDistrView->GetScene()->ClearItems();
	m_lengthDistrView->GetScene()->AddItem(chart);
	m_lengthDistrWidget->renderWindow()->Render();
	m_lengthDistrWidget->update();
	m_ppWidget->legendLayout->removeWidget(m_polarPlotWidget);
	m_ppWidget->legendLayout->addWidget(m_lengthDistrWidget);
	m_polarPlotWidget->hide();
	m_lengthDistrWidget->show();
}

void dlg_FeatureScout::classAddButton()
{
	vtkIdTypeArray* pcSelection = m_pcChart->GetPlot(0)->GetSelection();
	auto CountObject = pcSelection->GetNumberOfTuples();
	if (CountObject <= 0)
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "No object was selected!");
		return;
	}
	if (CountObject == m_activeClassItem->rowCount() && m_activeClassItem->index().row() != 0)
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "All items in current class are selected. There is no need to create a new class out of them. Please select only a subset of items!");
		return;
	}
	if (m_renderMode != rmSingleClass)
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "Cannot add a class while in a special rendering mode "
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

	// create a first level child under rootItem as new class
	double percent = 100.0 * CountObject / m_objCnt;
	auto newClassItem = prepareRow(cText, QString("%1").arg(CountObject), QString::number(percent, 'f', 1));
	newClassItem.first()->setData(cColor, Qt::DecorationRole);

	int classID = rootItem->rowCount();
	int objID = 0;
	QList<int> kIdx; // list to register the selected index of object IDs in m_activeClassItem

	// add new class
	for (int i = 0; i < CountObject; ++i)
	{
		// get objID from item->text()
		vtkVariant v = pcSelection->GetVariantValue(i);
		objID = v.ToInt() + 1;	//fibre index starting at 1 not at 0
		auto child = m_activeClassItem->child(v.ToInt());
		if (!child)
		{
			LOG(lvlWarn, QString("Invalid state: Active class does not contain item (objID=%1) from selection!").arg(objID));
			continue;
		}
		objID = child->text().toInt();

		if (kIdx.contains(v.ToInt()))
		{
			LOG(lvlWarn, QString("Tried to add objID=%1 to class which is already contained in other class!").arg(objID));
			continue;
		}
		kIdx.prepend(v.ToInt());

		// add item to the new class
		QString str = QString("%1").arg(objID);
		newClassItem.first()->appendRow(new QStandardItem(str));

		m_csvTable->SetValue(objID - 1, m_colCnt - 1, classID); // update Class_ID column in csvTable
	}

	// a simple check of the selections
	if (kIdx.isEmpty())
	{
		QMessageBox::information(m_activeChild, "FeatureScout", "Selected objects are already classified, please make a new selection.");
		return;
	}

	if (kIdx.count() != CountObject)
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "Selection Error, please make a new selection.");
		return;
	}
	m_splom->classAdded(classID);

	rootItem->appendRow(newClassItem);

	// remove items from m_activeClassItem from table button to top, otherwise you would make a wrong delete
	for (int i = 0; i < CountObject; ++i)
	{
		m_activeClassItem->removeRow(kIdx.value(i));
	}

	updateClassStatistics(m_activeClassItem);
	setActiveClassItem(newClassItem.first(), 1);
	calculateElementTable();
	initElementTableModel();
	setPCChartData();
	m_classTreeView->collapseAll();
	m_classTreeView->setCurrentIndex(newClassItem.first()->index());
	updatePolarPlotView(m_chartTable);
	singleRendering();
}

void dlg_FeatureScout::writeWisetex(QXmlStreamWriter* writer)
{
	if (QMessageBox::warning(m_activeChild, "FeatureScout",
		"This functionality is only available for FCP fiber/ feature characteristics pore csv formats at the moment. Are you sure you want to proceed?",
		QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
	{
		return;
	}
	// Write XML tags using WiseTex specification
	//check if it is a class item
	if (m_classTreeModel->invisibleRootItem()->hasChildren())
	{
		if (m_objectType == iAObjectType::Fibers)
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
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9,3,0)
					writer->writeTextElement("X1", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::StartX)).ToString().c_str()));
					writer->writeTextElement("Y1", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::StartY)).ToString().c_str()));
					writer->writeTextElement("Z1", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::StartZ)).ToString().c_str()));
					writer->writeTextElement("X1", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::EndX)).ToString().c_str()));
					writer->writeTextElement("Y2", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::EndY)).ToString().c_str()));
					writer->writeTextElement("Z2", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::EndZ)).ToString().c_str()));
					writer->writeTextElement("Phi", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::Phi)).ToString().c_str()));
					writer->writeTextElement("Theta", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::Theta)).ToString().c_str()));
					writer->writeTextElement("Dia", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::Diameter)).ToString().c_str()));
					writer->writeTextElement("sL", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::Length)).ToString().c_str()));
					// TODO: define mapping?
					writer->writeTextElement("cL", QString(m_tableList[i]->GetValue(j, 19).ToString().c_str()));
					writer->writeTextElement("Surf", QString(m_tableList[i]->GetValue(j, 21).ToString().c_str()));
					writer->writeTextElement("Vol", QString(m_tableList[i]->GetValue(j, 22).ToString().c_str()));
#else
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
#endif

					writer->writeEndElement(); //end Fibre-n tag
				}
				writer->writeEndElement(); //end Fibres tag
				writer->writeEndElement(); //end FibreClass-n tag
			}
			writer->writeEndElement(); //end FibreClasses tag
		}
		else if (m_objectType == iAObjectType::Voids)
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
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9,3,0)
					writer->writeTextElement("Volume", QString(m_tableList[i]->GetValue(j, 21).ToString().c_str()));
					writer->writeTextElement("dimX", QString(m_tableList[i]->GetValue(j, 13).ToString().c_str()));
					writer->writeTextElement("dimY", QString(m_tableList[i]->GetValue(j, 14).ToString().c_str()));
					writer->writeTextElement("dimZ", QString(m_tableList[i]->GetValue(j, 15).ToString().c_str()));
					writer->writeTextElement("posX", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::CenterX)).ToString().c_str()));
					writer->writeTextElement("posY", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::CenterY)).ToString().c_str()));
					writer->writeTextElement("posZ", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::CenterZ)).ToString().c_str()));
#else
					writer->writeTextElement("Volume", QString(m_tableList[i]->GetValue(j, 21).ToString()));
					writer->writeTextElement("dimX", QString(m_tableList[i]->GetValue(j, 13).ToString()));
					writer->writeTextElement("dimY", QString(m_tableList[i]->GetValue(j, 14).ToString()));
					writer->writeTextElement("dimZ", QString(m_tableList[i]->GetValue(j, 15).ToString()));
					writer->writeTextElement("posX", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::CenterX)).ToString()));
					writer->writeTextElement("posY", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::CenterY)).ToString()));
					writer->writeTextElement("posZ", QString(m_tableList[i]->GetValue(j, m_columnMapping->value(iACsvConfig::CenterZ)).ToString()));
#endif
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

void dlg_FeatureScout::csvDVSaveButton()
{
	// Get selected rows out of elementTable
	QModelIndexList indexes = m_elementTableView->selectionModel()->selection().indexes();
	QList<ushort> characteristicsList;

	iAAttributes params;
	addAttr(params, "Save file", iAValueType::Boolean, false);
	addAttr(params, "Show histograms", iAValueType::Boolean, true);
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
		addAttr(params, QString("HistoMin for %1").arg(columnName), iAValueType::Continuous,
			m_elementTable->GetColumn(1)->GetVariantValue(characteristicsList.at(i)).ToFloat());
		addAttr(params, QString("HistoMax for %1").arg(columnName), iAValueType::Continuous,
			m_elementTable->GetColumn(2)->GetVariantValue(characteristicsList.at(i)).ToFloat());
		addAttr(params, QString("HistoBinNbr for %1").arg(columnName), iAValueType::Discrete, 100, 2);
	}

	if (characteristicsList.count() == 0)
	{
		QMessageBox::information(m_activeChild, "FeatureScout", "No characteristic specified in the element explorer.");
		return;
	}

	iAParameterDlg dlg(m_activeChild, "Save Distribution CSV", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	bool saveFile = values["Save file"].toBool();
	bool showHistogram = values["Show histograms"].toBool();
	if (!saveFile && !showHistogram)
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "Please check either 'Save file' or 'Show histogram' (or both).");
		return;
	}

	QString filename;
	if (saveFile)
	{
		filename = QFileDialog::getSaveFileName(m_activeChild, tr("Save characteristic distributions"), m_sourcePath, tr("CSV Files (*.csv *.CSV)"));
		if (filename.isEmpty())
		{
			return;
		}
	}

	m_dvContextView->GetScene()->ClearItems();

	//Sets up a chart matrix for the feature distribution charts
	vtkNew<vtkChartMatrix> distributionChartMatrix;
	distributionChartMatrix->SetSize(vtkVector2i(characteristicsList.count() < 3 ?
		characteristicsList.count() % 3 : 3, std::ceil(characteristicsList.count() / 3.0)));
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
			std::ofstream file(filename.toStdString(), std::ios::app);
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
			m_dwDV = new iADockWidgetWrapper(dvqvtkWidget, "Distribution View", "FeatureScoutDV", "https://github.com/3dct/open_iA/wiki/FeatureScout");
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

void dlg_FeatureScout::wisetexSaveButton()
{
	if (!m_classTreeModel->invisibleRootItem()->hasChildren())
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "No data available!");
		return;
	}

	//XML file save path
	QString filename = QFileDialog::getSaveFileName(m_activeChild, tr("Save File"), m_sourcePath, tr("XML Files (*.xml *.XML)"));
	if (filename.isEmpty())
	{
		return;
	}

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "Could not open XML file for writing.");
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

void dlg_FeatureScout::classExportButton()
{
	// if no volume loaded, then exit
	if (m_visType != iAObjectVisType::UseVolume)
	{
		if (m_activeChild->firstImageData() == nullptr)
		{
			QMessageBox::information(m_activeChild, "FeatureScout", "Feature only available if labeled volume is loaded!");
			return;
		}
		else if (QMessageBox::question(m_activeChild, "FeatureScout", "A labeled volume is required."
			"You are not using labeled volume visualization - are you sure a labeled volume is loaded?",
			QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;
	}
	int classCount = m_classTreeModel->invisibleRootItem()->rowCount();
	if (classCount < 2)	// unclassified class only
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "No defined class (except the 'unclassified' class) - please create at least one class first!");
		return;
	}
	auto img = m_activeChild->firstImageData();
	if (!img)
	{
		return;
	}
	auto con = std::make_shared<iAConnector>();
	con->setImage(img);
	ITK_TYPED_CALL(createLabelledOutputMask, con->itkScalarType(), con);
}

template <class T>
void dlg_FeatureScout::createLabelledOutputMask(std::shared_ptr<iAConnector> con)
{
	typedef int ClassIDType;
	typedef itk::Image<T, DIM>   InputImageType;
	typedef itk::Image<ClassIDType, DIM>   OutputImageType;

	const QString ParamClasses = "Classes to export";
	const QString modeExportAllClasses = "Export all classes";
	const QString modeExportSingleClass = "Export single selected class";
	iAAttributes params;
	QStringList classModes = QStringList() << modeExportAllClasses;
	QString descr;
	if (m_activeClassItem->row() > 0)
	{
		classModes << modeExportSingleClass;
	}
	else
	{
		descr = "NOTE: In order to select a single class, you need to select one in the class list first. Currently, there is no class selected!";
	}
	addAttr(params, ParamClasses, iAValueType::Categorical, classModes);

	const QString ParamValues = "Value to use";
	const QString modeFiberID = "Fiber ID";
	const QString modeClassID = "Class ID";
	addAttr(params, ParamValues, iAValueType::Categorical, QStringList() << modeFiberID << modeClassID);
	iAParameterDlg dlg(m_activeChild, "Save classification options", params, descr);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	bool exportAllClasses = (dlg.parameterValues()[ParamClasses].toString() == modeExportAllClasses);
	// create map from fiber ID to class ID for the classes to export:
	QMap<size_t, ClassIDType> currentEntries;
	// Skip first, as classes start with 1, 0 is the uncategorized class
	for (int i = 1; i < m_classTreeModel->invisibleRootItem()->rowCount(); i++)
	{

		if (!exportAllClasses && i != m_activeClassItem->row())
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
	bool fiberIDLabelling = (dlg.parameterValues()[ParamValues].toString() == modeFiberID);
	auto in_img = dynamic_cast<InputImageType*>(con->itkImage());
	auto region_in = in_img->GetLargestPossibleRegion();
	const OutputImageType::SpacingType outSpacing = in_img->GetSpacing();
	auto out_img = createImage<OutputImageType>(region_in.GetSize(), outSpacing);
	auto fw = runAsync(
		[in_img, out_img, region_in, fiberIDLabelling, currentEntries, con]
		{
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
		},
		[out_img, this]
		{
			auto ds = std::make_shared<iAImageData>(out_img);
			ds->setMetaData(iADataSet::NameKey, "Extracted classes");
			m_activeChild->addDataSet(ds);
		},
		this
	);
	iAJobListView::get()->addJob("Extracting class(es) as labeled volume...", nullptr, fw);
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

void dlg_FeatureScout::classSaveButton()
{
	if (m_classTreeModel->invisibleRootItem()->rowCount() == 1)
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "No classes were defined.");
		return;
	}

	QString fileName = QFileDialog::getSaveFileName(m_activeChild, tr("Save File"), m_sourcePath, tr("XML Files (*.xml *.XML)"));
	if (fileName.isEmpty())
	{
		return;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Could not open classes XML file (%1) for writing.").arg(fileName));
		QMessageBox::warning(m_activeChild, "FeatureScout", QString("Could not open classes XML file (%1) for writing.").arg(fileName));
		return;
	}
	iAAttributes params;
	const QString SaveOnlyID("Save only ID");
	addAttr(params, SaveOnlyID, iAValueType::Boolean, true);
	iAParameterDlg dlg(m_activeChild, "Save classes XML", params, "Whether to store only IDs (<em>Save only ID</em> checked, new default) "
		"or whether all attribute values should be stored (old default, in versions in which this dialog is not available).");
	if (!dlg.exec())
	{
		return;
	}
	QXmlStreamWriter stream(&file);
	saveClassesXML(stream, dlg.parameterValues()[SaveOnlyID].toBool());
}

void dlg_FeatureScout::saveClassesXML(QXmlStreamWriter& stream, bool idOnly)
{
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement(IFVTag);
	stream.writeAttribute(VersionAttribute, "1.0");
	stream.writeAttribute(CountAllAttribute, QString("%1").arg(m_objCnt));
	stream.writeAttribute(IDColumnAttribute, filterToXMLAttributeName(m_csvTable->GetColumnName(0)));  // store name of ID  -> TODO: ID mapping!

	for (int i = 0; i < m_classTreeModel->invisibleRootItem()->rowCount(); ++i)
	{
		writeClassesAndChildren(&stream, m_classTreeModel->invisibleRootItem()->child(i), idOnly);
	}

	stream.writeEndElement(); // ClassTree
	stream.writeEndDocument();
}

void dlg_FeatureScout::classLoadButton()
{
	QString fileName = QFileDialog::getOpenFileName(m_activeChild, tr("Load xml file"), m_sourcePath, tr("XML Files (*.xml *.XML)"));
	if (fileName.isEmpty())
	{
		return;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "Class load error: Could not open source xml file.");
		return;
	}
	QXmlStreamReader reader(&file);
	loadClassesXML(reader);
}

void dlg_FeatureScout::loadClassesXML(QXmlStreamReader& reader)
{
	// checking xml file correctness
	reader.readNext();  // skip xml tag?
	reader.readNext();  // read IFV_Class_Tree element
	QString IDColumnName = (m_objectType == iAObjectType::Fibers) ? LabelAttribute : LabelAttributePore;
	if (reader.name() != IFVTag) // incompatible xml file
	{
		auto msg = QString("Class load error: xml file incompatible with FeatureScout (root element called %1 instead of %2), please check.").arg(reader.name()).arg(IFVTag);
		LOG(lvlWarn, msg);
		QMessageBox::warning(m_activeChild, "FeatureScout", msg);
		return;
	}
	auto xmlObjectCount = reader.attributes().value(CountAllAttribute).toString().toInt();
	if (xmlObjectCount != m_objCnt)
	{
		QString msg = QString("Class load error: Object count in xml file (%1) does not match object count of current dataset (%2)"
			", please check; the xml file was probably created from a different dataset.").arg(xmlObjectCount).arg(m_objCnt);
		LOG(lvlWarn, msg);
		QMessageBox::warning(m_activeChild, "FeatureScout", msg);
		return;
	}
	if (reader.attributes().hasAttribute(IDColumnAttribute))
	{
		IDColumnName = reader.attributes().value(IDColumnAttribute).toString();
	}

	m_tableList.clear();
	m_colorList.clear();
	m_classTreeModel->removeRows(0, m_classTreeModel->rowCount());

	int idxClass = 0;

	// create xml reader
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();
	QStandardItem* activeItem = nullptr;

	while (!reader.atEnd())
	{
		if (reader.readNext() != QXmlStreamReader::EndDocument && reader.isStartElement())
		{
			if (reader.name() == ObjectTag)
			{
				QString label = reader.attributes().value(IDColumnName).toString();
				if (!reader.attributes().hasAttribute(IDColumnName))
				{
					LOG(lvlError, QString("Invalid XML: ID attribute %1 is not set!").arg(IDColumnName));
				}
				activeItem->appendRow(new QStandardItem(label));

				// update Class_ID number in csvTable;
				m_csvTable->SetValue(label.toInt() - 1, m_colCnt - 1, idxClass - 1);
				m_splom->changeClass(label.toInt() - 1, idxClass - 1);
			}
			else if (reader.name() == ClassTag)
			{
				// prepare the first level class items
				const auto name = reader.attributes().value(NameAttribute).toString();
				const auto color = QColor(reader.attributes().value(ColorAttribute).toString());
				const auto count = reader.attributes().value(CountAttribute).toString();
				const auto percent = reader.attributes().value(PercentAttribute).toString();

				QList<QStandardItem*> stammItem = prepareRow(name, count, percent);
				stammItem.first()->setData(color, Qt::DecorationRole);
				m_colorList.append(color);
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

	assert(rootItem->rowCount() == idxClass);

	for (int i = 0; i < idxClass; ++i)
	{
		recalculateChartTable(rootItem->child(i));
	}
	multiClassRendering();
}

void dlg_FeatureScout::classDeleteButton()
{
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();
	QStandardItem* stammItem = rootItem->child(0);
	if (m_activeClassItem->index().row() == 0)
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "You are trying to delete the unclassified class, please select another class.");
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
		m_csvTable->SetValue(labelID - 1, m_colCnt - 1, 0);
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
				m_csvTable->SetValue(labelID - 1, m_colCnt - 1, classID);
			}
			for (int k = 0; k < m_tableList[classID]->GetNumberOfRows(); ++k)
			{
				m_tableList[classID]->SetValue(k, m_colCnt - 1, classID);
			}
			m_tableList[classID]->GetColumn(classID)->Modified();
		}
	}

	// update statistics for m_activeClassItem
	updateClassStatistics(stammItem);

	// update m_tableList and setup m_activeClassItem
	setActiveClassItem(stammItem, 2);
	QSignalBlocker ctvBlocker(m_classTreeView);
	m_classTreeView->setCurrentIndex(m_classTreeView->model()->index(0, 0));

	// update element view
	setPCChartData();
	calculateElementTable();
	initElementTableModel();
	singleRendering();
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
		QMessageBox::information(m_activeChild, "FeatureScout", "Scatterplot Matrix already created.");
		return;
	}
	QSignalBlocker spmBlock(m_splom.get()); // no need to trigger updates while we're creating SPM
	m_splom->initScatterPlot(m_csvTable, m_columnVisibility);
	m_dwSPM = new iADockWidgetWrapper(m_splom->matrixWidget(), "Scatter Plot Matrix", "FeatureScoutSPM",
		"https://github.com/3dct/open_iA/wiki/FeatureScout");
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
	if (editSettingsDialog<dlg_FeatureScout>(iAFeatureScoutPCSettings::defaultAttributes(), m_pcSettings,
		FeatureScoutPCSettingsName, *this, &dlg_FeatureScout::applyPCSettings))
	{
		updateAxisProperties();
		m_pcView->Update();
		m_pcView->ResetCamera();
		m_pcView->Render();
	}
}

void dlg_FeatureScout::applyPCSettings(QVariantMap const& values)
{
	m_pcSettings = values;
	m_pcChart->GetPlot(0)->GetPen()->SetOpacity(values[PCOpacity].toDouble());
	m_pcChart->GetPlot(0)->SetWidth(values[PCLineWidth].toDouble());
}

void dlg_FeatureScout::saveMesh()
{
	// TODO: instad, make objectvis available in datasets!
	if (m_visType == iAObjectVisType::UseVolume)
	{
		QMessageBox::warning(m_activeChild, "FeatureScout", "Cannot export mesh for labelled volume visualization!");
		return;
	}
	auto polyVis = dynamic_cast<iAColoredPolyObjectVis*>(m_3dvis);
	if (!polyVis)
	{
		return;
	}
	QString defaultFilter = iAFileTypeRegistry::defaultExtFilterString(iADataSetType::Mesh);
	QString fileName = QFileDialog::getSaveFileName(m_activeChild, tr("Save File"),
		m_sourcePath,
		iAFileTypeRegistry::registeredFileTypes(iAFileIO::Save, iADataSetType::Mesh),
		&defaultFilter);
	if (fileName.isEmpty())
	{
		return;
	}
	auto dataSet = std::make_shared<iAPolyData>(polyVis->finalPolyData());
	auto io = iAFileTypeRegistry::createIO(fileName, iAFileIO::Save);
	if (!io || !io->isDataSetSupported(dataSet, fileName, iAFileIO::Save))
	{
		auto msg = QString("The chosen file format (%1) does not support this kind of dataset!").arg(io->name());
		QMessageBox::warning(m_activeChild, "Save: Error", msg);
		LOG(lvlError, msg);
	}
	QVariantMap paramValues;
	auto attr = io->parameter(iAFileIO::Save);
	if (!attr.isEmpty())
	{
		iAParameterDlg dlg(m_activeChild, "Save mesh parameters", attr);
		if (dlg.exec() != QDialog::Accepted)
		{
			return;
		}
	}
	auto p = std::make_shared<iAProgress>();
	auto fw = runAsync([fileName, p, io, dataSet, paramValues]()
		{
			if (!io->save(fileName, dataSet, paramValues, *p.get()))
			{
				LOG(lvlError, "Error saving mesh!");
			}
		}, []() {}, this);
	iAJobListView::get()->addJob("Save mesh", p.get(), fw);
}

void dlg_FeatureScout::spSelInformsPCChart(std::vector<size_t> const& selInds)
{  // If scatter plot selection changes, Parallel Coordinates gets informed
	renderSelection(selInds);
	QCoreApplication::processEvents();
	auto sortedSelInds = m_splom->getFilteredSelection();
	setPCSelection(sortedSelInds);
}

void dlg_FeatureScout::setPCSelection(std::vector<size_t> const& sortedSelInds)
{
	auto countSelection = sortedSelInds.size();
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
	m_pcMousePressPos[0] = iren->GetEventPosition()[0];
	m_pcMousePressPos[1] = iren->GetEventPosition()[1];
}

void dlg_FeatureScout::pcRightButtonReleased(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command)
{
	Q_UNUSED(client_data);
	Q_UNUSED(command);
	// Gets the mouse button event for scatter plot matrix and opens a popup menu.
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
	int* mouseReleasePos = iren->GetLastEventPosition();
	if (mouseReleasePos[0] == m_pcMousePressPos[0] && mouseReleasePos[1] == m_pcMousePressPos[1])
	{
		//command->AbortFlagOn();  // Consume event so the interactor style doesn't get it -> not a good idea; right button somehow starts axis drag; if we consume, then axis drag mode doesn't stop!
		QMenu popupMenu;
		auto addClass = popupMenu.addAction("Add class");
		addClass->setIcon(iAThemeHelper::icon("plus"));
		connect(addClass, &QAction::triggered, this, &dlg_FeatureScout::classAddButton);
		auto pcSettings = popupMenu.addAction("Settings");
		pcSettings->setIcon(iAThemeHelper::icon("settings_PC"));
		connect(pcSettings, &QAction::triggered, this, &dlg_FeatureScout::showPCSettings);
		int* sz = iren->GetSize();
		QPoint pt(mouseReleasePos[0], sz[1] - mouseReleasePos[1]); // flip y
		// VTK delivers "device pixel" coordinates, while Qt expects "device independent pixel" coordinates
		// (see https://doc.qt.io/qt-5/highdpi.html#glossary-of-high-dpi-terms); convert:
		pt = pt / m_dwPC->devicePixelRatio();
		// ToDo: Re-add "Suggest Classification" / "Accept Classification" functionality
		popupMenu.exec(m_pcWidget->mapToGlobal(pt));
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
			setActiveClassItem( firstLevelItem.first(), 1 );
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
	updateClassStatistics( m_classTreeModel->invisibleRootItem()->child( motherClassItem->index().row() ) );

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
	singleRendering();
	updatePolarPlotView( m_chartTable );

	//Updates scatter plot matrix when a class is added.
	// TODO SPM
	//matrix->UpdateColorInfo( m_classTreeModel, m_colorList );
	//matrix->SetClass2Plot( m_activeClassItem->index().row() );
	//matrix->UpdateLayout();
}
*/

void dlg_FeatureScout::itemDoubleClicked(const QModelIndex& index)
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
			singleRendering();
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
		m_ppWidget->legendLayout->removeWidget(m_lengthDistrWidget);
		m_ppWidget->legendLayout->addWidget(m_polarPlotWidget);
		m_lengthDistrWidget->hide();
		m_polarPlotWidget->show();
	}
	m_ppWidget->colorMapSelection->hide();
}

void dlg_FeatureScout::itemClicked(const QModelIndex& index)
{
	showLengthDistribution(false);

	// Gets right item depending on whether click was on class 'level'
	auto item = (index.parent().row() == -1) ?
		m_classTreeModel->item(index.row(), 0):
		m_classTreeModel->itemFromIndex(index);

	// Selected an empty class - nothing to render!
	if (item->rowCount() == 0 && item->parent() == m_classTreeModel->invisibleRootItem())
	{
		QMessageBox::information(m_activeChild, "FeatureScout", "This class contains no object, nothing to show!");
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
			singleRendering();
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
		initElementTableModel(sID);
		int objectID = item->text().toInt();
		singleRendering(objectID);
		m_elementTableView->update();

		// update SPLOM selection
		std::vector<size_t> filteredSelInds;
		filteredSelInds.push_back(sID);
		m_splom->setFilteredSelection(filteredSelInds);
	}
}

double dlg_FeatureScout::calculateOpacity(QStandardItem* item)
{
	// chart opacity dependent of number of objects
	// TODO: replace by some kind of logarithmic formula
	// for multi rendering
	if (item == m_classTreeModel->invisibleRootItem())
	{
		if (m_objCnt < 1000)
		{
			return 1.0;
		}
		if (m_objCnt < 3000)
		{
			return 0.8;
		}
		if (m_objCnt < 10000)
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

void dlg_FeatureScout::writeClassesAndChildren(QXmlStreamWriter* writer, QStandardItem* item, bool idOnly) const
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
		for (int j = 0; j < m_colCnt && (!idOnly || j == 0); ++j)
		{
			vtkVariant v = m_csvTable->GetValue(item->child(i)->text().toInt() - 1, j);
			vtkVariant v1 = m_elementTable->GetValue(j, 0);
			QString str = QString::fromUtf8(v.ToString().c_str()).trimmed();
			QString str1 = filterToXMLAttributeName(QString::fromUtf8(v1.ToString().c_str()).trimmed());
			writer->writeAttribute(str1, str);
		}
		writer->writeEndElement(); // end object tag
	}
	writer->writeEndElement(); // end class tag
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
		recalculateChartTable(m_activeClassItem);

		// calculate the new class table and set up chartTable
		recalculateChartTable(item);

		m_activeClassItem = item;
		int id = item->index().row();
		m_chartTable = m_tableList[id];
	}
	else if (situ == 2)	// delete class
	{
		// merge the deleted class table to stamm table
		recalculateChartTable(item);
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
	for (int i = 1; i < m_colCnt - 1; ++i)
	{
		auto arrX = vtkSmartPointer<vtkFloatArray>::New();
		arrX->SetName(m_chartTable->GetColumnName(i));
		table->AddColumn(arrX);
	}
	auto arrI = vtkSmartPointer<vtkIntArray>::New();
	arrI->SetName(m_chartTable->GetColumnName(m_colCnt - 1));
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

void dlg_FeatureScout::updateMultiClassLookupTable(double alpha)
{
	auto lutNum = m_colorList.size();
	m_pcMultiClassLUT->SetNumberOfTableValues(lutNum);
	for (vtkIdType i = 0; i < lutNum; ++i)
	{
		m_pcMultiClassLUT->SetTableValue(i,
			m_colorList.at(i).red() / 255.0,
			m_colorList.at(i).green() / 255.0,
			m_colorList.at(i).blue() / 255.0,
			m_colorList.at(i).alpha() / 255.0);
	}

	m_pcMultiClassLUT->SetRange(0, lutNum - 1);
	m_pcMultiClassLUT->SetAlpha(alpha);
	m_pcMultiClassLUT->Build();
}

void dlg_FeatureScout::enableBlobRendering()
{
	if (!openBlobVisDialog())
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
	const double percentage = 100.0 * count / m_objCnt;
	blob->SetObjectType(MapObjectTypeToString(m_objectType));
	blob->SetCluster(objects);
	blob->SetName(m_activeClassItem->text());
	blob->SetBlobColor(color);
	blob->SetStats(count, percentage);
	m_blobManager->Update();
}

void dlg_FeatureScout::disableBlobRendering()
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
		if (item->hasChildren()) // actions for classes:
		{
			actions.append(m_blobRendering);
			actions.append(m_blobRemoveRendering);
			actions.append(m_saveBlobMovie);
		}
		else if (item->parent() && item->parent()->index().row() != 0)
		{                        // actions for single objects:
			actions.append(m_objectDelete);
		}
	}
	if (actions.count() > 0)
	{
		QMenu::exec(actions, m_classTreeView->mapToGlobal(pnt));
	}
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
		QMessageBox::information(m_activeChild, "FeatureScout", "An object in the unclassified class can not be deleted.");
		return;
	}

	if (item->parent()->rowCount() == 1)
	{
		classDeleteButton();
	}
	else
	{
		int oID = item->text().toInt();
		m_csvTable->SetValue(oID - 1, m_colCnt - 1, 0);

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
		updateClassStatistics(sItem);
		recalculateChartTable(sItem);

		QStandardItem* rItem = item->parent();
		rItem->removeRow(item->index().row());
		updateClassStatistics(rItem);
		recalculateChartTable(rItem);
		setActiveClassItem(rItem, 0);
	}
}

void dlg_FeatureScout::updateClassStatistics(QStandardItem* item)
{
	QStandardItem* rootItem = m_classTreeModel->invisibleRootItem();

	if (item->hasChildren())
	{
		rootItem->child(item->index().row(), 1)->setText(QString("%1").arg(item->rowCount()));
		double percent = 100.0 * item->rowCount() / m_objCnt;
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
	int ip, it, tt;

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

	auto length = t->GetNumberOfRows();

	for (vtkIdType k = 0; k < length; ++k)
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
		double phi = i * re * vtkMath::Pi() / 180.0;
		double rx = 100.0;

		if (i < 2)
		{
			rx = 95.0;
		}

		x[0] = rx * std::cos(phi);
		x[1] = rx * std::sin(phi);
		x[2] = 0.0;

		pts->SetPoint(i, x);
		vtkVariant v = i * re;
		nameArray->InsertNextValue(v.ToString());
	}

	// annotation for theta
	constexpr double phi = 270.0 * vtkMath::Pi() / 180.0;
	for (vtkIdType i = 12; i < numPoints; ++i)
	{
		double rx = (numPoints - i) * 15.0;
		x[0] = rx * std::cos(phi);
		x[1] = rx * std::sin(phi);
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

	const double re = 15.0;
	const int ap = 25;
	const int at = 7;

	auto sGrid = vtkSmartPointer<vtkStructuredGrid>::New();
	sGrid->SetDimensions(at, ap, 1);

	auto points = vtkSmartPointer<vtkPoints>::New();
	points->Allocate(sGrid->GetNumberOfPoints());

	for (int i = 0; i < ap; ++i)
	{
		double phi = i * re * vtkMath::Pi() / 180.0;

		for (int j = 0; j < at; ++j)
		{
			double rx = j * re;
			double xx = rx * std::cos(phi);
			double yy = rx * std::sin(phi);
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
	int maxCount = calcOrientationProbability(it, table); // maximal count of the object orientation

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
			double phi = y * m_PolarPlotPhiResolution * vtkMath::Pi() / 180.0;
			double xx = rx * std::cos(phi);
			double yy = rx * std::sin(phi);
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

bool dlg_FeatureScout::openBlobVisDialog()
{
	iABlobCluster* blob = nullptr;
	if (m_blobMap.contains(m_activeClassItem->text()))
	{
		blob = m_blobMap[m_activeClassItem->text()];
	}
	iAAttributes params;
	addAttr(params, "Range:", iAValueType::Continuous, blob ? blob->GetRange() : m_blobManager->GetRange());
	addAttr(params, "Blob body:", iAValueType::Boolean, blob ? blob->GetShowBlob() : m_blobManager->GetShowBlob());
	addAttr(params, "Use Depth peeling:", iAValueType::Boolean, m_blobManager->GetUseDepthPeeling());
	addAttr(params, "Blob opacity [0,1]:", iAValueType::Continuous, blob ? blob->GetBlobOpacity() : m_blobManager->GetBlobOpacity());
	addAttr(params, "Silhouettes:", iAValueType::Boolean, blob ? blob->GetSilhouette() : m_blobManager->GetSilhouettes());
	addAttr(params, "Silhouettes opacity [0,1]:", iAValueType::Continuous, blob ? blob->GetSilhouetteOpacity() : m_blobManager->GetSilhouetteOpacity());
	addAttr(params, "3D labels:", iAValueType::Boolean, blob ? blob->GetLabel() : m_blobManager->GetLabeling());
	addAttr(params, "Smart overlapping:", iAValueType::Boolean, m_blobManager->OverlappingIsEnabled());
	addAttr(params, "Separation distance (if smart overlapping):", iAValueType::Continuous, m_blobManager->GetOverlapThreshold());
	addAttr(params, "Smooth after smart overlapping:", iAValueType::Boolean, blob ? blob->GetSmoothing() : m_blobManager->GetSmoothing());
	addAttr(params, "Gaussian blurring of the blob:", iAValueType::Boolean, m_blobManager->GetGaussianBlur());
	addAttr(params, "Gaussian blur variance:", iAValueType::Continuous, blob ? blob->GetGaussianBlurVariance() : m_blobManager->GetGaussianBlurVariance());
	addAttr(params, "Dimension X", iAValueType::Discrete, blob ? blob->GetDimensions()[0] : m_blobManager->GetDimensions()[0]);
	addAttr(params, "Dimension Y", iAValueType::Discrete, blob ? blob->GetDimensions()[1] : m_blobManager->GetDimensions()[1]);
	addAttr(params, "Dimension Z", iAValueType::Discrete, blob ? blob->GetDimensions()[2] : m_blobManager->GetDimensions()[2]);
	iAParameterDlg dlg(m_activeChild, "Blob rendering preferences", params);
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

void dlg_FeatureScout::saveBlobMovie()
{
	QString movie_file_types = GetAvailableMovieFormats();
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(m_activeChild, "Movie Export", "Sorry, but movie export support is disabled.");
		return;
	}
	iAAttributes params;
	QStringList modes = (QStringList() << tr("No rotation") << tr("Rotate Z") << tr("Rotate X") << tr("Rotate Y"));
	addAttr(params, "Rotation mode", iAValueType::Categorical, modes);
	addAttr(params, "Number of frames:", iAValueType::Discrete, 24, 1);
	addAttr(params, "Range from:", iAValueType::Continuous, m_blobManager->GetRange());
	addAttr(params, "Range to:", iAValueType::Continuous, m_blobManager->GetRange());
	addAttr(params, "Blob body:", iAValueType::Boolean, m_blobManager->GetShowBlob());
	addAttr(params, "Blob opacity from [0,1]:", iAValueType::Continuous, m_blobManager->GetBlobOpacity(), 0, 1);
	addAttr(params, "Blob opacity to:", iAValueType::Continuous, m_blobManager->GetBlobOpacity(), 0, 1);
	addAttr(params, "Silhouettes:", iAValueType::Boolean, m_blobManager->GetSilhouettes());
	addAttr(params, "Silhouettes opacity from [0,1]:", iAValueType::Continuous, m_blobManager->GetSilhouetteOpacity());
	addAttr(params, "Silhouettes opacity to:", iAValueType::Continuous, m_blobManager->GetSilhouetteOpacity());
	addAttr(params, "3D labels:", iAValueType::Boolean, m_blobManager->GetLabeling());
	addAttr(params, "Smart overlapping:", iAValueType::Boolean, m_blobManager->OverlappingIsEnabled());
	addAttr(params, "Separation distance from (if smart overlapping):", iAValueType::Continuous, m_blobManager->GetOverlapThreshold());
	addAttr(params, "Separation distance to:", iAValueType::Continuous, m_blobManager->GetOverlapThreshold());
	addAttr(params, "Smooth after smart overlapping:", iAValueType::Boolean, m_blobManager->GetSmoothing());
	addAttr(params, "Gaussian blurring of the blob:", iAValueType::Boolean, m_blobManager->GetGaussianBlur());
	addAttr(params, "Gaussian blur variance from:", iAValueType::Continuous, m_blobManager->GetGaussianBlurVariance());
	addAttr(params, "Gaussian blur variance to:", iAValueType::Continuous, m_blobManager->GetGaussianBlurVariance());
	addAttr(params, "Dimension X from", iAValueType::Discrete, m_blobManager->GetDimensions()[0]);
	addAttr(params, "Dimension X to:" , iAValueType::Discrete, m_blobManager->GetDimensions()[0]);
	addAttr(params, "Dimension Y from", iAValueType::Discrete, m_blobManager->GetDimensions()[1]);
	addAttr(params, "Dimension Y to:" , iAValueType::Discrete, m_blobManager->GetDimensions()[1]);
	addAttr(params, "Dimension Z from", iAValueType::Discrete, m_blobManager->GetDimensions()[2]);
	addAttr(params, "Dimension Z to:" , iAValueType::Discrete, m_blobManager->GetDimensions()[2]);
	addAttr(params, "Video quality", iAValueType::Discrete, 2, 0, 2);
	addAttr(params, "Frame rate", iAValueType::Discrete, 25, 1, 1000);

	iAParameterDlg dlg(m_activeChild, "Blob movie rendering options", params,
		"Creates a movie of the blob, rotating around it. "
		"The <em>video quality</em> specifies the quality of the output video "
			"(range: 0..2, 0 - worst, 2 - best; default: 2). "
		"The <em>frame rate</em> specifies the frames per second (default: 25). ");
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	QString mode = values["Rotation mode"].toString();
	auto imode = static_cast<int>(modes.indexOf(mode));
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
	auto quality = values["Video quality"].toInt();
	auto rate = values["Frame rate"].toInt();
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
			m_activeChild,
			tr("Export movie %1").arg(mode),
			fileInfo.absolutePath() + "/" + ((mode.isEmpty()) ? fileInfo.baseName() : fileInfo.baseName() + "_" + mode), movie_file_types),
		imode,
		quality,
		rate
	);
}

void dlg_FeatureScout::initFeatureScoutUI()
{
	m_ppWidget->legendLayout->addWidget(m_polarPlotWidget);
	m_activeChild->addDockWidget(Qt::RightDockWidgetArea, m_dwCE);
	m_activeChild->addDockWidget(Qt::RightDockWidgetArea, m_dwPC);
	m_activeChild->addDockWidget(Qt::RightDockWidgetArea, m_dwPP);
	m_ppWidget->colorMapSelection->hide();
	if (m_objectType == iAObjectType::Voids)
	{
		m_dwPP->hide();
	}
}

void dlg_FeatureScout::saveProject(QSettings& projectFile)
{
	if (m_classTreeModel->invisibleRootItem()->rowCount() <= 1)
	{
		return;
	}
	QString outXML;
	QXmlStreamWriter writer(&outXML);
	//writer.setAutoFormatting(false);  // apparently auto formatting is enabled by default, and cannot be disabled
	saveClassesXML(writer, true);
	auto filteredXML = outXML.replace("\n", "").replace(QRegularExpression("[ ]+"), " ");
	projectFile.setValue(ClassesProjectFile, filteredXML);
}

void dlg_FeatureScout::loadProject(QSettings const & projectFile)
{
	if (!projectFile.contains(ClassesProjectFile))
	{
		return;
	}
	QString classesString = projectFile.value(ClassesProjectFile).toString();
	QXmlStreamReader reader(classesString);
	loadClassesXML(reader);
}

void dlg_FeatureScout::updateAxisProperties()
{
	int visibleColIdx = 0;
	for (int i = 0; i < static_cast<int>(m_pcChart->GetNumberOfAxes()); i++)
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
		axis->GetLabelProperties()->SetFontSize(m_pcSettings[PCFontSize].toInt());
		axis->GetTitleProperties()->SetFontSize(m_pcSettings[PCFontSize].toInt());
		vtkDataArray* columnData = vtkDataArray::SafeDownCast(m_chartTable->GetColumn(visibleColIdx));
		double* const range = columnData->GetRange();
		if (range[0] != range[1])
		{
			// if min == max, then leave NumberOfTicks at default -1, otherwise there will be no ticks and no lines shown
			axis->SetNumberOfTicks(m_pcSettings[PCTickCount].toInt());
		}
		axis->Update();
		++visibleColIdx;
	}
	m_pcChart->Update();
}
