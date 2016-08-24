/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
 
#ifndef DLG_FIBERSCOUT_H
#define	DLG_FIBERSCOUT_H

#include <QList>
#include <QMap>

#include <vtkAbstractArray.h>
#include <vtkAnnotationLink.h>
#include <vtkAxis.h>
#include <vtkChartParallelCoordinates.h>
#include <vtkColorTransferFunction.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkDataArray.h>
#include <vtkDataRepresentation.h>
#include <vtkDataSetAttributes.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkFloatArray.h>
#include <vtkStringArray.h>
#include <vtkIntArray.h>
#include <vtkPiecewiseFunction.h>
#include <QVTKWidget.h>
#include <vtkRenderWindow.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarsToColors.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkStdString.h>
#include <vtkTextProperty.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>
#include <vtkVolume.h>

#include "dlg_imageproperty.h"
#include "iAFiberScoutModuleInterface.h"
#include "iAQTtoUIConnector.h"
#include "ui_FiberScoutClassExplorer.h"
#include "ui_FiberScoutParallelCoordinates.h"
#include "ui_FiberScoutPolarPlot.h"
#include "ui_FiberScoutScatterPlotMatrix.h"
#include "ui_FiberScoutDistributionView.h"
#include "ui_FiberScoutMeanObjectView.h"

typedef iAQTtoUIConnector<QDockWidget, Ui_FiberScoutPC> dlg_IOVPC;
typedef iAQTtoUIConnector<QDockWidget, Ui_FiberScoutPP> dlg_IOVPP;
typedef iAQTtoUIConnector<QDockWidget, Ui_FiberScoutSPM> dlg_IOVSPM;
typedef iAQTtoUIConnector<QDockWidget, Ui_FiberScoutDV> dlg_IOVDV;
typedef iAQTtoUIConnector<QDockWidget, Ui_FiberScoutMO> dlg_IOVMO;

class QComboBox;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QTableView;
class QXmlStreamWriter;

class vtkCollection;
class vtkLookupTable;
class vtkDelaunay2D;

class iABlobCluster;
class iABlobManager;
class iARenderer;
class dlg_blobVisualization;

namespace FiberScout
{
	class iAScatterPlotMatrix;
}
class MdiChild;


/**
* \brief	implement vtkChartParallelCoordinates as dialog
*/
class dlg_FiberScout : public QDockWidget, private Ui_FiberScoutCE
{
	Q_OBJECT

public:
	dlg_FiberScout( MdiChild *parent, FilterID fid, vtkRenderer* blobRen);
	~dlg_FiberScout();

	// setups
	void setupModel();
	void setupViews();
	void setupConnections();
	void createSphereView();
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
	void RenderingFiberMeanObject();
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
	bool changeFiberScout_Options( int idx );

protected:
	bool initParallelCoordinates( FilterID fid );


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
	int filterID;

	bool draw3DPolarPlot;
	bool enableRealTimeRendering;
	bool classRendering;
	bool spmActivated;

	QString sourcePath;
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

	FiberScout::iAScatterPlotMatrix *matrix;
	QVTKWidget *pcWidget;
	QVTKWidget *pcPolarPlot;
	QWidget *orientationColorMapSelection;
	QComboBox * orientColormap;
	
	vtkSmartPointer<vtkContextView> dvContextView;

	vtkSmartPointer<vtkScalarBarActor> scalarBarPP;
	vtkSmartPointer<vtkScalarBarActor> scalarBarFLD;
	vtkSmartPointer<vtkScalarBarWidget> scalarWidgetPP;
	vtkSmartPointer<vtkScalarBarWidget> scalarWidgetFLD;
	
	int mousePressedPos [2];

	//void updateViewPorts();
	//vtkSmartPointer<vtkRenderWindow> renderWindow;

	dlg_IOVPC * iovPC;
	dlg_IOVPP * iovPP;
	dlg_IOVSPM * iovSPM;
	dlg_IOVDV * iovDV;
	dlg_IOVMO * iovMO;

	//Mean Object Rendering
	QList<vtkVolume *> m_MOVolumesList;
};

#endif
