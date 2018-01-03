/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "dlg_imageproperty.h"
#include "iAFeatureScoutModuleInterface.h"
#include "iAObjectAnalysisType.h"
#include "iAQTtoUIConnector.h"
#include "ui_FeatureScoutClassExplorer.h"
#include "ui_FeatureScoutParallelCoordinates.h"
#include "ui_FeatureScoutPolarPlot.h"
#include "ui_FeatureScoutDistributionView.h"
#include "ui_FeatureScoutMeanObjectView.h"

typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutPC> dlg_IOVPC;
typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutPP> dlg_IOVPP;
typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutDV> dlg_IOVDV;
typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutMO> dlg_IOVMO;

class iABlobCluster;
class iABlobManager;
class iAMeanObjectTFView;
class iAModalityTransfer;
class iARenderer;
class dlg_blobVisualization;
class MdiChild;

class vtkChartParallelCoordinates;
class vtkContextView;
class vtkDataArray;
class vtkDelaunay2D;
class vtkFixedPointVolumeRayCastMapper;
class vtkIdTypeArray;
class vtkLookupTable;
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkSmartVolumeMapper;
class vtkStringArray;
class vtkTable;
class vtkVolume;
class vtkVolumeProperty;
class vtkStructuredGrid;

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

/**
* \brief	implement vtkChartParallelCoordinates as dialog
*/
class dlg_FeatureScout : public QDockWidget, private Ui_FeatureScoutCE
{
	Q_OBJECT

public:
	dlg_FeatureScout( MdiChild *parent, iAObjectAnalysisType fid, vtkRenderer* blobRen, vtkSmartPointer<vtkTable> csvtbl);
	~dlg_FeatureScout();

	// setups
	void setupModel();
	void setupViews();
	void setupConnections();
	void setupPolarPlotView(vtkTable *it);
	void updatePolarPlotColorScalar(vtkTable *it);
	void updateObjectOrientationID(vtkTable *table);
	void createPolarPlotLookupTable(vtkLookupTable *lut);
	void createFLDODLookupTable(vtkLookupTable *lut, int Num);
	void updatePolarPlotView(vtkTable *it);
	void drawPolarPlotMesh(vtkRenderer *renderer);
	void drawScalarBar(vtkScalarsToColors *lut, vtkRenderer *renderer, int RenderType = 0);
	void drawAnnotations(vtkRenderer *renderer);
	void setupPolarPlotResolution(float grad);
	void updatePolarPlotForOrientationRendering(vtkLookupTable *lut);
	void setupNewPcView(bool lookupTable=false);
	void deletePcViewPointer();
	void calculateElementTable();
	void initElementTableModel(int idx = -10000);
	void initClassTreeModel();
	void setActiveClassItem(QStandardItem* item, int situ = 0);
	double calculateOpacity(QStandardItem *item);
	void recalculateChartTable(QStandardItem *item);
	void updateColumnNames();
	void updateColumnOrder();
	void setupDefaultElement();
	void SingleRendering(int idx = -10000);
	void updateLookupTable(double alpha= 0.7);
	void updatePCColumnVisibility();
	void updateClassStatistics(QStandardItem *item);
	int calcOrientationProbability(vtkTable *t, vtkTable *ot);
	void addLogMsg(const QString &str);

	QStringList getNamesOfObjectCharakteristics(bool withUnit);
	QList<QStandardItem *> prepareRow(const QString &first, const QString &second, const QString &third);
	void writeClassesAndChildren(QXmlStreamWriter *writer, QStandardItem *item);
	void writeWisetex(QXmlStreamWriter *writer);
	void autoAddClass(int NbOfClasses);
	void initOrientationColorMap();

Q_SIGNALS:
	void updateViews();

public slots:
	void SaveBlobMovie();
	void pcChangeOptions(int idx);
	void ClassSaveButton();
	void ClassAddButton();
	void ClassLoadButton();
	void ClassDeleteButton();
	void WisetexSaveButton();
	void CsvDVSaveButton();

	void RenderingButton();
	void RealTimeRendering(vtkIdTypeArray *selection, bool enabled);
	void RenderingMeanObject();
	void RenderingOrientation();
	void ScatterPlotButton();
	void RenderingFLD();

	void updatePCColumnValues(QStandardItem *item);
	void classClicked(const QModelIndex &index);
	void classDoubleClicked(const QModelIndex &index);

	int OpenBlobVisDialog ();
	void EnableBlobRendering();
	void DisableBlobRendering();

	void showContextMenu(const QPoint &pnt);
	void deleteObject();
	void addObject();
	
	void spBigChartMouseButtonPressed(vtkObject * obj, unsigned long, void * client_data, void *, vtkCommand * command);
	void spPopup(vtkObject * obj, unsigned long, void * client_data, void *, vtkCommand * command);
	void spPopupSelection(QAction *selection);
	void spSelInformsPCChart(vtkObject * obj, unsigned long, void * client_data, void *, vtkCommand * command);
	void spUpdateSPColumnVisibility();

	void pcViewMouseButtonCallBack(vtkObject * obj, unsigned long, void * client_data, void*, vtkCommand * command);
	bool changeFeatureScout_Options( int idx );

	void modifyMeanObjectTF();
	void updateMOView();
	void browseFolderDialog();
	void saveStl();
	void updateStlProgress(int i);
	void updateMarProgress(int i);

protected:
	bool initParallelCoordinates( iAObjectAnalysisType fid );


private:
	// Qt members
	QWidget *activeChild;

	// members referencing MdiChild
	vtkPiecewiseFunction     *oTF;
	vtkColorTransferFunction *cTF;

	// private members
	int	width, height;
	int elementNr;		// Number of elements in csv inputTable
	int objectNr;		// Number of objects in the specimen
	iAObjectAnalysisType filterID;

	bool draw3DPolarPlot;
	bool enableRealTimeRendering;
	bool classRendering;
	bool spmActivated;

	const QString sourcePath;
	vtkSmartPointer<vtkStringArray> nameArr;
	
	// calculate the average value of a 1D array
	float calculateAverage(vtkDataArray* arr);

	// input csv table with all objects, column names updated for vtk rendering problem
	// by solving this rendering problem satisfacted here a pointer to the orginal table
	vtkSmartPointer<vtkTable> csvTable; 
	// element table with calculated elments values for every individual class
	vtkSmartPointer<vtkTable> elementTable;
	// table for ParallelCoordinates view, should be initialized every time when a new class is defined
	// or a class is selected in the class tree view
	vtkSmartPointer<vtkTable> chartTable;

	QList<vtkSmartPointer<vtkTable> > tableList;
	QList<QColor> colorList;
	QList<int> selectedObjID;

	QList<int> ObjectOrientationProbabilityList; //Probability distribution of every single object
	int pcMaxC; // maximal count of the object orientation

	// elementNameStringList for using fibre csv file
	QStringList eleString;

	// column visiability list
	vtkSmartPointer<vtkStringArray> columnVisArr;
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
	vtkContextView *pcView;
	vtkChartParallelCoordinates *pcChart;

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

	iAQSplom *matrix;
	QVTKWidget *pcWidget;
	QVTKWidget *pcPolarPlot;
	QWidget *orientationColorMapSelection;
	QComboBox * orientColormap;
	
	vtkSmartPointer<vtkContextView> m_dvContextView;

	vtkSmartPointer<vtkScalarBarActor> m_scalarBarPP;
	vtkSmartPointer<vtkScalarBarActor> m_scalarBarFLD;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetPP;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetFLD;
	
	int mousePressedPos [2];

	dlg_IOVPC * iovPC;
	dlg_IOVPP * iovPP;
	dlg_IOVDV * iovDV;
	dlg_IOVMO * iovMO;
	QDockWidget* iovSPM;

	//Mean Object Rendering	
	iAMeanObjectTFView* m_motfView;	
	moData m_MOData;
	vtkSmartPointer<vtkRenderWindow> m_renderWindow;
};
