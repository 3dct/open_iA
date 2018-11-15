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
#include <vtkVersion.h>

#include <vector>

typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutPP> dlg_IOVPP;
typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutMO> dlg_IOVMO;

class iA3DObjectVis;
class iABlobCluster;
class iABlobManager;
class iAConnector;
class iAFeatureScoutSPLOM;
class iAMeanObjectTFView;
class dlg_blobVisualization;

class iAModalityTransfer;
class iAQSplom;
class iARenderer;
class MdiChild;

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
class QVTKOpenGLWidget;
class vtkGenericOpenGLRenderWindow;
#else
class QVTKWidget;
class vtkRenderWindow;
#endif
class vtkAxis;
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
		vtkSmartPointer<vtkTable> csvtbl, int vis, QSharedPointer<QMap<uint, uint> > columnMapping);
	~dlg_FeatureScout();
	void changeFeatureScout_Options(int idx);
private slots:
	void SaveBlobMovie();
	void ClassSaveButton();
	void ClassAddButton();
	void ClassLoadButton();
	void ClassDeleteButton();
	void WisetexSaveButton();
	void ExportClassButton(); //!< The export defined classes to MDH File.
	void CsvDVSaveButton();
	void RenderOrientation();
	void classClicked(const QModelIndex &index);
	void classDoubleClicked(const QModelIndex &index);
	void EnableBlobRendering();
	void DisableBlobRendering();
	void showContextMenu(const QPoint &pnt);
	void deleteObject();
	void addObject();
	void updateVisibility(QStandardItem *item);
	//! @{ scatterplot-related methods:
	void spBigChartMouseButtonPressed(vtkObject * obj, unsigned long, void * client_data, void *, vtkCommand * command);
	void spPopup(vtkObject * obj, unsigned long, void * client_data, void *, vtkCommand * command);
	void spPopupSelection(QAction *selection);
	void spSelInformsPCChart(std::vector<size_t> const & selInds);
	void spParameterVisibilityChanged(size_t paramIndex, bool enabled);
	//! @}
	//! @{ parallel coordinate chart related methods:
	void pcViewMouseButtonCallBack(vtkObject * obj, unsigned long, void * client_data, void*, vtkCommand * command);
	//! @}
	void modifyMeanObjectTF();
	void updateMOView();
	void browseFolderDialog();
	void saveStl();
	void updateStlProgress(int i);
	void updateMarProgress(int i);
private:
	//create labelled output image based on defined classes
	template <class T> void CreateLabelledOutputMask(iAConnector *con, const QString fOutPath);
	void showScatterPlot();
	void setupModel();
	void setupViews();
	void setupConnections();  //!< define signal and slots connections
	void initColumnVisibility();
	void initElementTableModel(int idx = -10000);
	void initClassTreeModel();
	void initFeatureScoutUI();
	//! @{ polar plot / length distribution related methods:
	void setupPolarPlotView(vtkTable *it);
	void updatePolarPlotColorScalar(vtkTable *it);
	void drawPolarPlotMesh(vtkRenderer *renderer);
	void drawScalarBar(vtkScalarsToColors *lut, int RenderType = 0);
	void drawAnnotations(vtkRenderer *renderer);
	void setupPolarPlotResolution(float grad);
	void hideLengthDistribution();
	//! @}
	//! @{ parallel coordinate chart related methods:
	void setPCChartData(bool lookupTable = false);
	void updatePCColumnVisibility();
	std::vector<size_t> getPCSelection();
	void updateAxisProperties();               //!< set properties for all axes in parallel coordinates: font size, tick count
	//! @}
	float calculateAverage(vtkDataArray* arr); //!< calculate the average value of a 1D array
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
	void RenderLengthDistribution();                      //!< render fiber-length distribution
	void RenderMeanObject();                              //!< compute and render a mean object for each class
	//! @}

	//! @{ debug functions
	void PrintVTKTable(const vtkSmartPointer<vtkTable> anyTable, const bool useTabSeparator, const QString &outputPath, const QString* fileName) const ; //!< print out a vtkTable
	void PrintChartTable(const QString &outputPath);      //! < Print current chartTable
	void PrintCSVTable(const QString &outputPath);	      //! <Print current CSVTable
	void PrintTableList(const QList<vtkSmartPointer<vtkTable>> &OutTableList, QString &outputPath) const;
	//! @}

	//! @{ members referencing MdiChild, used for 3D rendering
	MdiChild *activeChild;
	//! @}

	int elementsCount;                              //!< Number of elements(=columns) in csv inputTable
	int objectsCount;                               //!< Number of objects in the specimen
	iAFeatureScoutObjectType filterID;              //!< Type of objects that are shown
	bool draw3DPolarPlot;                           //!< Whether the polar plot is drawn in 3D, set only in constructor, default false
	int m_renderMode;                               //!< Indicates what is currently shown: single classes, or special rendering (multi-class, orientation, ...)
	int visualization;                              //!< 3D visualization being used (a value out of iACsvConfig::VisualizationType
	const QString m_sourcePath;                     //!< folder of file currently opened

	//! Input csv table with all objects.
	vtkSmartPointer<vtkTable> csvTable;
	//! Table of elements (=parameters) with min, max and average computed for each object in the current class.
	vtkSmartPointer<vtkTable> elementTable;
	//! Table for the objects shown in the parallel coordinates view (i.e., the objects of the current class)
	vtkSmartPointer<vtkTable> chartTable;

	QList<vtkSmartPointer<vtkTable> > tableList;    //!< The data table for each class.
	QList<QColor> m_colorList;                      //!< The color for each class.
	std::vector<char> columnVisibility;             //!< Element(=column) visibility list
	vtkSmartPointer<vtkLookupTable> lut;            //!< Color lookup table for multi-class rendering in parallel coordinate view
	QTreeView* classTreeView;                       //!< Class tree view
	QTableView* elementTableView;                   //!< Element(=column) table view
	QStandardItemModel* elementTableModel;          //!< Model for element table
	QStandardItemModel* classTreeModel;             //!< Model for class tree view (->invisibleRootItem->child(0,...,i, 0,..,2))
	QStandardItem *activeClassItem;                 //!< Currently active class item in classTreeView/Model

	//! @{ context menu actions for classTreeView
	QAction *blobRendering;
	QAction *blobRemoveRendering;
	QAction *objectDelete;
	QAction *objectAdd;
	QAction *saveBlobMovie;
	//! @}

	//! @{ Parallel coordinates view
	vtkSmartPointer<vtkContextView> pcView;
	vtkSmartPointer<vtkChartParallelCoordinates> pcChart;
	vtkSmartPointer<vtkEventQtSlotConnect> pcConnections;
	float m_pcLineWidth;              //!< width of the line for each object in Parallel Coordinates
	int m_pcFontSize;                 //!< current font size of titles and tick labels
	int m_pcTickCount;                //!< current tick count
	static const int PCMinTicksCount; //!< minimum number of ticks
	//! @}

	vtkSmartPointer<vtkContextView> m_lengthDistrView;

	iARenderer *raycaster;
	iABlobManager *blobManager;
	QMap <QString, iABlobCluster*> blobMap;

	//! @{ polar plot view
	int gPhi, gThe;
	float PolarPlotPhiResolution, PolarPlotThetaResolution;
	vtkSmartPointer<vtkDelaunay2D> delaunay;
	vtkPolyData *PolarPlotPolyData;
	vtkStructuredGrid *PolarPlotGrid;
	//! @}

	dlg_blobVisualization *blobVisDialog;

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	QVTKOpenGLWidget *pcWidget, *m_polarPlotWidget, *meanObjectWidget, *m_lengthDistrWidget;
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_meanObjectRenderWindow;
#else
	QVTKWidget *pcWidget, *m_polarPlotWidget, *meanObjectWidget, *m_lengthDistrWidget;
	vtkSmartPointer<vtkRenderWindow> m_meanObjectRenderWindow;
#endif

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

	QSharedPointer<QMap<uint, uint>> m_columnMapping;

	QSharedPointer<iAFeatureScoutSPLOM> m_splom;
	QSharedPointer<iA3DObjectVis> m_3dvis;
};
