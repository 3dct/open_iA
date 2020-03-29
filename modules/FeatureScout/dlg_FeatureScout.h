/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iAFeatureScoutModuleInterface.h"
#include "iAFeatureScoutObjectType.h"
#include "ui_FeatureScoutClassExplorer.h"
#include "ui_FeatureScoutPolarPlot.h"
#include "ui_FeatureScoutMeanObjectView.h"

#include <iAvec3.h>
#include <iAVtkWidgetFwd.h>
#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <map>
#include <vector>

typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutPP> dlg_PolarPlot;
typedef iAQTtoUIConnector<QDockWidget, Ui_FeatureScoutMO> dlg_MeanObject;

class iA3DObjectVis;
class iABlobCluster;
class iABlobManager;
class iAConnector;
class iADockWidgetWrapper;
class iAFeatureScoutSPLOM;
class iAMeanObjectTFView;
class dlg_blobVisualization;

class iALookupTable;
class iAModalityTransfer;
class iAQSplom;
class iARenderer;
class MdiChild;

class vtkAxis;
class vtkChartParallelCoordinates;
class vtkColorTransferFunction;
class vtkCommand;
class vtkContextView;
class vtkDataArray;
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
		vtkSmartPointer<vtkTable> csvtbl, int vis, QSharedPointer<QMap<uint, uint> > columnMapping, std::map<size_t,
		std::vector<iAVec3f> > & curvedFiberInfo, int cylinderQuality, size_t segmentSkip);
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
	void renderLUTChanges(QSharedPointer<iALookupTable> lut, size_t colInd);
private:
	//create labelled output image based on defined classes
	template <class T> void CreateLabelledOutputMask(iAConnector & con, const QString & fOutPath);
	void showScatterPlot();
	void setupModel();
	void setupViews();
	void setupConnections();  //!< define signal and slots connections
	void initColumnVisibility();
	void initElementTableModel(int idx = -10000);
	void initClassTreeModel();
	void initFeatureScoutUI();
	//! @{ polar plot / length distribution related methods:
	void updatePolarPlotView(vtkTable *it);
	void drawPolarPlotMesh(vtkRenderer *renderer);
	void drawOrientationScalarBar(vtkScalarsToColors *lut);
	void drawAnnotations(vtkRenderer *renderer);
	void setupPolarPlotResolution(float grad);
	void showLengthDistribution(bool show, vtkScalarsToColors* lut = nullptr);
	void showOrientationDistribution();
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
	//void autoAddClass(int NbOfClasses);
	bool OpenBlobVisDialog();
	//! @{ 3D-rendering-related methods:
	void SingleRendering(int objectID = -10000);          //!< render a single object (if objectID > 0) or a single class
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
	MdiChild* m_activeChild;
	//! @}

	int m_elementCount;                             //!< Number of elements(=columns) in csv inputTable
	int m_objectCount;                             //!< Number of objects in the specimen
	iAFeatureScoutObjectType m_filterID;            //!< Type of objects that are shown
	bool m_draw3DPolarPlot;                         //!< Whether the polar plot is drawn in 3D, set only in constructor, default false
	int m_renderMode;                               //!< Indicates what is currently shown: single classes, or special rendering (multi-class, orientation, ...)
	bool m_singleObjectSelected;                    //!< Indicates whether a single object or a whole class is selected (if m_renderMode is rmSingleClass)
	int m_visualization;                            //!< 3D visualization being used (a value out of iACsvConfig::VisualizationType
	const QString m_sourcePath;                     //!< folder of file currently opened

	//! Input csv table with all objects.
	vtkSmartPointer<vtkTable> m_csvTable;
	//! Table of elements (=parameters) with min, max and average computed for each object in the current class.
	vtkSmartPointer<vtkTable> m_elementTable;
	//! Table for the objects shown in the parallel coordinates view (i.e., the objects of the current class)
	vtkSmartPointer<vtkTable> m_chartTable;

	QList<vtkSmartPointer<vtkTable> > m_tableList;  //!< The data table for each class.
	QList<QColor> m_colorList;                      //!< The color for each class.
	std::vector<char> m_columnVisibility;           //!< Element(=column) visibility list
	vtkSmartPointer<vtkLookupTable> m_multiClassLUT;//!< Color lookup table for multi-class rendering in parallel coordinate view
	QTreeView* m_classTreeView;                     //!< Class tree view
	QTableView* m_elementTableView;                 //!< Element(=column) table view
	QStandardItemModel* m_classTreeModel;           //!< Model for class tree view (->invisibleRootItem->child(0,...,i, 0,..,2))
	QStandardItemModel* m_elementTableModel;        //!< Model for element table
	QStandardItem* m_activeClassItem;               //!< Currently active class item in classTreeView/Model

	//! @{ context menu actions for classTreeView
	QAction *m_blobRendering;
	QAction *m_blobRemoveRendering;
	QAction *m_objectDelete;
	QAction *m_objectAdd;
	QAction *m_saveBlobMovie;
	//! @}

	//! @{ Parallel coordinates view
	vtkSmartPointer<vtkContextView> m_pcView;
	vtkSmartPointer<vtkChartParallelCoordinates> m_pcChart;
	vtkSmartPointer<vtkEventQtSlotConnect> m_pcConnections;
	float m_pcLineWidth;              //!< width of the line for each object in Parallel Coordinates
	int m_pcFontSize;                 //!< current font size of titles and tick labels
	int m_pcTickCount;                //!< current tick count
	static const int PCMinTicksCount; //!< minimum number of ticks
	//! @}

	vtkSmartPointer<vtkContextView> m_lengthDistrView;

	iARenderer* m_renderer;
	iABlobManager* m_blobManager;
	QMap <QString, iABlobCluster*> m_blobMap;

	//! @{ polar plot view
	int m_gPhi, m_gThe;
	float m_PolarPlotPhiResolution, m_PolarPlotThetaResolution;
	//! @}

	dlg_blobVisualization* m_blobVisDialog;

	iAVtkOldWidget* m_pcWidget, *m_polarPlotWidget, * m_meanObjectWidget, *m_lengthDistrWidget;

	vtkSmartPointer<vtkContextView> m_dvContextView;

	vtkSmartPointer<vtkScalarBarActor> m_scalarBarPP;
	vtkSmartPointer<vtkScalarBarActor> m_scalarBarFLD;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetPP;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetFLD;

	int m_mousePressPos[2];

	iADockWidgetWrapper * m_dwPC, *m_dwDV, *m_dwSPM;
	dlg_PolarPlot * m_dwPP;
	dlg_MeanObject * m_dwMO;

	//Mean Object Rendering
	iAMeanObjectTFView* m_motfView;
	moData m_MOData;

	QSharedPointer<QMap<uint, uint>> m_columnMapping;

	QSharedPointer<iAFeatureScoutSPLOM> m_splom;
	QSharedPointer<iA3DObjectVis> m_3dvis;
};
