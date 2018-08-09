/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#pragma once

#include "iADockWidgetWrapper.h"

#include "iAFeatureScoutModuleInterface.h"
#include "iAFeatureScoutObjectType.h"
#include "iAQTtoUIConnector.h"
#include "ui_FeatureScoutClassExplorer.h"
#include "ui_FeatureScoutPolarPlot.h"
#include "ui_FeatureScoutMeanObjectView.h"

#include <vtkSmartPointer.h>

#include <vector>

typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutPP> dlg_IOVPP;
typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutMO> dlg_IOVMO;

class iABlobCluster;
class iABlobManager;
class iAFeatureScoutSPLOM;
class iAMeanObjectTFView;
class iAModalityTransfer;
class iAQSplom;
class iARenderer;
class dlg_blobVisualization;
class MdiChild;

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
class QVTKOpenGLWidget;
class vtkGenericOpenGLRenderWindow;
#else
class QVTKWidget;
class vtkRenderWindow;
#endif
class vtkChartParallelCoordinates;
class vtkColorTransferFunction;
class vtkCommand;
class vtkContextView;
class vtkDataArray;
class vtkDelaunay2D;
class vtkEventQtSlotConnect;
class vtkFixedPointVolumeRayCastMapper;
class vtkIdTypeArray;
class vtkLookupTable;
class vtkObject;
class vtkPiecewiseFunction;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkScalarsToColors;
class vtkSmartVolumeMapper;
class vtkStringArray;
class vtkStructuredGrid;
class vtkTable;
class vtkUnsignedCharArray;
class vtkVolume;
class vtkVolumeProperty;

class QComboBox;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QTableView;
class QXmlStreamWriter;

class iAQSPLOM;

struct moData
{
	QList<iAModalityTransfer *> moHistogramList;
	QList<vtkSmartPointer<vtkVolume> > moVolumesList;
	QList<vtkSmartPointer<vtkRenderer> > moRendererList;
	QList<vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> > moVolumeMapperList;
	QList<vtkSmartPointer<vtkVolumeProperty> > moVolumePropertyList;
	QList<vtkSmartPointer<vtkImageData> > moImageDataList;
};

class dlg_FeatureScout : public QDockWidget, public Ui_FeatureScoutCE
{
	Q_OBJECT
public:
	dlg_FeatureScout( MdiChild *parent, iAFeatureScoutObjectType fid, QString const & fileName, vtkRenderer* blobRen,
		vtkSmartPointer<vtkTable> csvtbl, int vis, QMap<uint, uint> const & columnMapping);
	~dlg_FeatureScout();
	void changeFeatureScout_Options(int idx);
private slots:
	void SaveBlobMovie();
	void ClassSaveButton();
	void ClassAddButton();
	void ClassLoadButton();
	void ClassDeleteButton();
	void WisetexSaveButton();
	void CsvDVSaveButton();
	void RenderingOrientation();
	void classClicked(const QModelIndex &index);
	void classDoubleClicked(const QModelIndex &index);
	void EnableBlobRendering();
	void DisableBlobRendering();
	void showContextMenu(const QPoint &pnt);
	void deleteObject();
	void addObject();
	//! @{ scatterplot-related methods:
	void spBigChartMouseButtonPressed(vtkObject * obj, unsigned long, void * client_data, void *, vtkCommand * command);
	void spPopup(vtkObject * obj, unsigned long, void * client_data, void *, vtkCommand * command);
	void spPopupSelection(QAction *selection);
	void spSelInformsPCChart(std::vector<size_t> const & selInds);
	//! @}
	//! @{ parallel coordinate chart related methods:
	void pcViewMouseButtonCallBack(vtkObject * obj, unsigned long, void * client_data, void*, vtkCommand * command);
	void updatePCColumnValues(QStandardItem *item);
	//! @}
	void modifyMeanObjectTF();
	void updateMOView();
	void browseFolderDialog();
	void saveStl();
	void updateStlProgress(int i);
	void updateMarProgress(int i);
private:
	void showScatterPlot();
	void setupModel();
	void setupViews();
	void setupConnections();  //!< define signal and slots connections
	void initColumnVisibility();
	void initElementTableModel(int idx = -10000);
	void initClassTreeModel();
	void initFeatureScoutUI();
	void updateObjectOrientationID(vtkTable *table);
	//! @{ polar plot related methods:
	void setupPolarPlotView(vtkTable *it);
	void updatePolarPlotColorScalar(vtkTable *it);
	void createPolarPlotLookupTable(vtkLookupTable *lut);
	void createFLDODLookupTable(vtkLookupTable *lut, int Num);
	void drawPolarPlotMesh(vtkRenderer *renderer);
	void drawScalarBar(vtkScalarsToColors *lut, vtkRenderer *renderer, int RenderType = 0);
	void drawAnnotations(vtkRenderer *renderer);
	void setupPolarPlotResolution(float grad);
	//! @}
	//! @{ parallel coordinate chart related methods:
	void setPCChartData(bool lookupTable = false);
	void updatePCColumnVisibility();
	//! @}
	void calculateElementTable();
	void setActiveClassItem(QStandardItem* item, int situ = 0);
	double calculateOpacity(QStandardItem *item);
	void recalculateChartTable(QStandardItem *item);
	void updateLookupTable(double alpha = 0.7);
	void updateClassStatistics(QStandardItem *item);
	int calcOrientationProbability(vtkTable *t, vtkTable *ot);
	QList<QStandardItem *> prepareRow(const QString &first, const QString &second, const QString &third);
	void writeClassesAndChildren(QXmlStreamWriter *writer, QStandardItem *item);
	void writeWisetex(QXmlStreamWriter *writer);
	void autoAddClass(int NbOfClasses);
	bool OpenBlobVisDialog();
	//! @{ 3D-rendering-related methods:
	void SingleRendering(int idx = -10000);               //!< render a single fiber or a single class
	void MultiClassRendering();                           //!< multi-class rendering
	void RenderSelection(std::vector<size_t> const & selInds); //!< render a selection (+ the class that contains it)
	void RenderingFLD();                                  //!< render fiber-length distribution
	void RenderingMeanObject();                           //!< compute and render a mean object for each class
	void SetPolyPointColor(int ptIdx, QColor const & qcolor);
	void UpdatePolyMapper();
	//! @}

	// members referencing MdiChild
	MdiChild *activeChild;
	vtkPiecewiseFunction     *oTF;
	vtkColorTransferFunction *cTF;
	int elementsCount;      //!< Number of elements(=columns) in csv inputTable
	int objectsCount;       //!< Number of objects in the specimen
	iAFeatureScoutObjectType filterID;

	bool draw3DPolarPlot;
	bool classRendering;
	int visualization;

	const QString sourcePath;
	vtkSmartPointer<vtkStringArray> nameArr;

	void PrintVTKTable(const vtkSmartPointer<vtkTable> anyTable, const bool useTabSeparator, const QString &outputPath, const QString* fileName) const ; //!< print out a vtkTabel
	void PrintChartTable(const QString &outputPath); //! < Print current chartTable
	void PrintCSVTable(const QString &outputPath);	//! <Print current CSVTable

	void PrintTableList(const QList<vtkSmartPointer<vtkTable>> &OutTableList, QString &outputPath) const;

	float calculateAverage(vtkDataArray* arr); //!< calculate the average value of a 1D array

	// input csv table with all objects, column names updated for vtk rendering problem
	// by solving this rendering problem satisfacted here a pointer to the orginal table
	vtkSmartPointer<vtkTable> csvTable;
	//! table of elements (=parameters) with min, max and average computed for each elements and every individual class
	vtkSmartPointer<vtkTable> elementTable;
	// table for ParallelCoordinates view, should be initialized every time when a new class is defined
	// or a class is selected in the class tree view
	vtkSmartPointer<vtkTable> chartTable;

	QList<vtkSmartPointer<vtkTable> > tableList;  //! < contains a table for each class; 
	QList<QColor> colorList;

	QList<int> ObjectOrientationProbabilityList; //Probability distribution of every single object
	int pcMaxC; // maximal count of the object orientation

	// column visibility list
	std::vector<bool> columnVisibility;
	// color lookup table for PC view
	vtkSmartPointer<vtkLookupTable> lut;

	// element and class views
	QTreeView* classTreeView;
	QTableView* elementTableView;
	// models
	QStandardItemModel* elementTableModel;
	// view of the different classes (->invisibleRootItem->child(0,...,i, 0,..,2))
	QStandardItemModel* classTreeModel;
	// context menu actions for classTreeView
	QAction *blobRendering;
	QAction *blobRemoveRendering;
	QAction *objectDelete;
	QAction *objectAdd;
	QAction *saveBlobMovie;
	// the active first level item
	QStandardItem *activeClassItem;

	// Parallel coordinates view
	vtkSmartPointer<vtkContextView> pcView;
	vtkSmartPointer<vtkChartParallelCoordinates> pcChart;
	vtkSmartPointer<vtkEventQtSlotConnect> pcConnections;

	iARenderer *raycaster;
	iABlobManager *blobManager;
	QMap <QString, iABlobCluster*> blobMap;

	// pcPolarPlot view
	int gPhi, gThe;
	float PolarPlotPhiResolution, PolarPlotThetaResolution;
	vtkDelaunay2D *delaunay;
	vtkPolyData *PolarPlotPolyData;
	vtkStructuredGrid *PolarPlotGrid;

	dlg_blobVisualization *blobVisDialog;

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	QVTKOpenGLWidget *pcWidget, *polarPlot, *meanObjectWidget;
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_meanObjectRenderWindow;
#else
	QVTKWidget *pcWidget, *polarPlot, *meanObjectWidget;
	vtkSmartPointer<vtkRenderWindow> m_meanObjectRenderWindow;
#endif
	QWidget *orientationColorMapSelection;
	QComboBox * orientColormap;

	vtkSmartPointer<vtkContextView> m_dvContextView;

	vtkSmartPointer<vtkScalarBarActor> m_scalarBarPP;
	vtkSmartPointer<vtkScalarBarActor> m_scalarBarFLD;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetPP;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetFLD;

	int mousePressedPos [2];

	iADockWidgetWrapper * iovPC, *iovDV, *iovSPM;
	dlg_IOVPP * iovPP;
	dlg_IOVMO * iovMO;

	//Mean Object Rendering
	iAMeanObjectTFView* m_motfView;
	moData m_MOData;

	QMap<uint, uint> m_columnMapping;
	float m_pcLineWidth;   //!< width of line in Parallel Coordinates

	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
	vtkSmartPointer<vtkUnsignedCharArray> m_colors;

	QSharedPointer<iAFeatureScoutSPLOM> m_splom;
};
