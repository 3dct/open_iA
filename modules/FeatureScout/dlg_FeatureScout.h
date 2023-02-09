// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FeatureScout_export.h"
#include "iAObjectType.h"

#include <vtkSmartPointer.h>

#include <QDockWidget>
#include <QMap>
#include <QSharedPointer>

#include <memory>    // for std::unique_ptr
#include <vector>

class dlg_blobVisualization;
class iABlobCluster;
class iABlobManager;
class iAFeatureScoutSPLOM;
class iAMeanObject;
class iAPolarPlotWidget;
class Ui_FeatureScoutCE;

class iADockWidgetWrapper;
class iAQSplom;
class iARenderer;
class iAMdiChild;
class iAQVTKWidget;

class iA3DObjectVis;
class iA3DObjectActor;

class iAConnector;
class iALookupTable;

class vtkAxis;
class vtkChartParallelCoordinates;
class vtkColorTransferFunction;
class vtkCommand;
class vtkContextView;
class vtkDataArray;
class vtkEventQtSlotConnect;
class vtkFixedPointVolumeRayCastMapper;
class vtkIdTypeArray;
class vtkImageData;
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
class QSettings;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QTableView;
class QXmlStreamReader;
class QXmlStreamWriter;

class FeatureScout_API dlg_FeatureScout : public QDockWidget
{
	Q_OBJECT
public:
	static const QString DlgObjectName;
	static const QString UnclassifiedColorName;
	dlg_FeatureScout(iAMdiChild *parent, iAObjectType fid, QString const & fileName, vtkSmartPointer<vtkTable> csvtbl,
		int visType, QSharedPointer<QMap<uint, uint>> columnMapping,
		QSharedPointer<iA3DObjectVis> objvis);
	~dlg_FeatureScout();
	void showPCSettings();            //!< show settings dialog for parallel coordinates
	void showScatterPlot();           //!< show the scatter plot matrix
	void multiClassRendering();       //!< multi-class rendering
	void renderLengthDistribution();  //!< render fiber-length distribution
	void renderMeanObject();          //!< compute and render a mean object for each class
	void renderOrientation();         //!< color all objects according to their orientation

	void saveProject(QSettings& projectFile);
	void loadProject(QSettings const & projectFile);

public slots:
	void selectionChanged3D();

private slots:
	void SaveBlobMovie();
	void ClassSaveButton();
	void ClassAddButton();
	void ClassLoadButton();
	void ClassDeleteButton();
	void WisetexSaveButton();
	void ExportClassButton(); //!< The export defined classes to MDH File.
	void CsvDVSaveButton();
	void classClicked(const QModelIndex &index);
	void classDoubleClicked(const QModelIndex &index);
	void EnableBlobRendering();
	void DisableBlobRendering();
	void showContextMenu(const QPoint &pnt);
	void deleteObject();
	void addObject();
	void updateVisibility(QStandardItem *item);
	//! @{ scatterplot-related methods:
	void spSelInformsPCChart(std::vector<size_t> const & selInds);
	void spParameterVisibilityChanged(size_t paramIndex, bool enabled);
	//! @}
	//! set selection in the parallel coordinates charts
	void setPCSelection(std::vector<size_t> const& sortedSelInds);
	//! @{ parallel coordinate chart related methods:
	void pcRightButtonPressed(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command);
	void pcRightButtonReleased(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command);
	void pcViewMouseButtonCallBack(vtkObject * obj, unsigned long, void * client_data, void*, vtkCommand * command);
	//! @}

	void renderLUTChanges(QSharedPointer<iALookupTable> lut, size_t colInd);
private:
	//create labelled output image based on defined classes
	template <class T> void CreateLabelledOutputMask(std::shared_ptr<iAConnector> con);
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
	void setPCChartData(bool specialRendering = false);
	void updatePCColumnVisibility();
	std::vector<size_t> getPCSelection();
	void updateAxisProperties();               //!< set properties for all axes in parallel coordinates: font size, tick count
	//! @}
	void calculateElementTable();
	void setActiveClassItem(QStandardItem* item, int situ = 0);
	double calculateOpacity(QStandardItem *item);
	void recalculateChartTable(QStandardItem *item);
	void updateLookupTable(double alpha = 0.7);
	void updateClassStatistics(QStandardItem *item);
	int calcOrientationProbability(vtkTable *t, vtkTable *ot);
	void saveClassesXML(QXmlStreamWriter& stream);
	void loadClassesXML(QXmlStreamReader& reader);
	void writeClassesAndChildren(QXmlStreamWriter *writer, QStandardItem *item) const;
	void writeWisetex(QXmlStreamWriter *writer);
	//void autoAddClass(int NbOfClasses);
	bool OpenBlobVisDialog();
	//! @{ 3D-rendering-related methods:
	void SingleRendering(int objectID = -10000);          //!< render a single object (if objectID > 0) or a single class
	void RenderSelection(std::vector<size_t> const & selInds); //!< render a selection (+ the class that contains it)
	//! @}

	//! @{ members referencing iAMdiChild, used for 3D rendering
	iAMdiChild* m_activeChild;
	//! @}

	int m_elementCount;                             //!< Number of elements(=columns) in csv inputTable
	int m_objectCount;                             //!< Number of objects in the specimen
	iAObjectType m_filterID;            //!< Type of objects that are shown
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
	int m_pcOpacity;                  //!< current opacity of lines
	static const int PCMinTicksCount; //!< minimum number of ticks
	//! @}

	vtkSmartPointer<vtkContextView> m_lengthDistrView;

	iARenderer* m_renderer;
	iABlobManager* m_blobManager;
	QMap<QString, iABlobCluster*> m_blobMap;

	//! @{ polar plot view
	int m_gPhi, m_gThe;
	float m_PolarPlotPhiResolution, m_PolarPlotThetaResolution;
	//! @}

	dlg_blobVisualization* m_blobVisDialog;

	iAQVTKWidget* m_pcWidget, *m_polarPlotWidget, *m_lengthDistrWidget;

	vtkSmartPointer<vtkContextView> m_dvContextView;

	vtkSmartPointer<vtkScalarBarActor> m_scalarBarPP;
	vtkSmartPointer<vtkScalarBarActor> m_scalarBarFLD;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetPP;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetFLD;

	int m_mousePressPos[2];

	iADockWidgetWrapper* m_dwPC, *m_dwDV, *m_dwSPM;
	iAPolarPlotWidget* m_dwPP;
	const std::unique_ptr<Ui_FeatureScoutCE> m_ui;

	QSharedPointer<QMap<uint, uint>> m_columnMapping;

	QSharedPointer<iAFeatureScoutSPLOM> m_splom;
	QSharedPointer<iA3DObjectVis> m_3dvis;
	QSharedPointer<iA3DObjectActor> m_3dactor;
	QSharedPointer<iAMeanObject> m_meanObject;
};
