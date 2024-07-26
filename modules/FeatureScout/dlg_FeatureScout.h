// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "featurescout_export.h"

#include <iAColMap.h>
#include <iAObjectType.h>

#include <vtkSmartPointer.h>

#include <QObject>

#include <vector>

class dlg_blobVisualization;
class iABlobCluster;
class iABlobManager;
class iAFeatureScoutSPLOM;
class iAMeanObject;
class iAPolarPlotWidget;
class iAClassExplorer;

class iADockWidgetWrapper;
class iAQSplom;
class iARenderer;
class iAMdiChild;
class iAQVTKWidget;

class iAObjectsData;
class iAObjectVis;
class iAObjectVisActor;

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

class QAction;
class QComboBox;
class QSettings;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QTableView;
class QXmlStreamReader;
class QXmlStreamWriter;

class FeatureScout_API dlg_FeatureScout : public QObject
{
	Q_OBJECT
public:
	static const QString UnclassifiedColorName;
	dlg_FeatureScout(iAMdiChild* parent, iAObjectType objectType, QString const& fileName, iAObjectsData const* objData,
		iAObjectVis* objvis);
	~dlg_FeatureScout();
	void showPCSettings();            //!< show settings dialog for parallel coordinates
	void showScatterPlot();           //!< show the scatter plot matrix
	void multiClassRendering();       //!< multi-class rendering
	void renderLengthDistribution();  //!< render fiber-length distribution
	void renderMeanObject();          //!< compute and render a mean object for each class
	void renderOrientation();         //!< color all objects according to their orientation
	void saveMesh();                  //!< store the (objectvis) mesh as file

	void saveProject(QSettings& projectFile);
	void loadProject(QSettings const& projectFile);

public slots:
	void selectionChanged3D();

private slots:
	void saveBlobMovie();
	void classSaveButton();
	void classAddButton();
	void classLoadButton();
	void classDeleteButton();
	void wisetexSaveButton();
	void classExportButton();  //!< Export the defined classes to an image file
	void csvDVSaveButton();
	void classClicked(const QModelIndex& index);
	void classDoubleClicked(const QModelIndex& index);
	void enableBlobRendering();
	void disableBlobRendering();
	void showContextMenu(const QPoint& pnt);
	void deleteObject();
	void updateVisibility(QStandardItem* item);
	//! @{ scatterplot-related methods:
	void spSelInformsPCChart(std::vector<size_t> const& selInds);
	void spParameterVisibilityChanged(size_t paramIndex, bool enabled);
	//! @}
	//! set selection in the parallel coordinates charts
	void setPCSelection(std::vector<size_t> const& sortedSelInds);
	//! @{ parallel coordinate chart related methods:
	void pcRightButtonPressed(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command);
	void pcRightButtonReleased(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command);
	void pcViewMouseButtonCallBack(vtkObject* obj, unsigned long, void* client_data, void*, vtkCommand* command);
	//! @}

	void renderLUTChanges(std::shared_ptr<iALookupTable> lut, size_t colInd);

private:
	//create labelled output image based on defined classes
	template <class T> void createLabelledOutputMask(std::shared_ptr<iAConnector> con);
	void setupViews();
	void setupConnections();  //!< define signal and slots connections
	void initColumnVisibility();
	void initElementTableModel(int idx = -10000);
	void initFeatureScoutUI();

	double calculateOpacity(QStandardItem* item);
	void recalculateChartTable(QStandardItem* item);
	//void autoAddClass(int NbOfClasses);
	//! @{ 3D-rendering-related methods:
	void singleRendering(int objectID = -10000);  //!< render a single object (if objectID > 0) or a single class
	void renderSelection(std::vector<size_t> const& selInds);  //!< render a selection (+ the class that contains it)
	//! @}

	iAMdiChild* m_activeChild;    //!< access to the child class in which FeatureScout is embedded
	iARenderer* m_renderer;       //!< access to the child's 3D renderer, used for the 3D visualization
	vtkIdType m_objCnt, m_colCnt; //!< Number of objects (=rows) / columns in the input table
	iAObjectType m_objectType;    //!< Type of objects that are shown
	bool m_draw3DPolarPlot;       //!< Whether polar plot is drawn in 3D, set only in constructor, default false
	int m_renderMode;             //!< What is currently shown: single class, multi-class, orientation rendering, ...
	bool m_singleObjectSelected;  //!< whether single object or whole class is selected (if single class render mode)
	iAObjectVisType m_visType;    //!< 3D visualization being used
	const QString m_sourcePath;   //!< folder of file currently opened
	iAColMapP m_columnMapping;    //!< mapping of which column stores which characteristic

	vtkSmartPointer<vtkTable> m_csvTable;          //!< Input csv table with all objects
	vtkSmartPointer<vtkTable> m_elementTable;      //! Characteristic statistics (min, max, avg) for current class
	vtkSmartPointer<vtkTable> m_chartTable;        //! Objects currently shown in PC view (i.e., obj. in current class)
	QList<vtkSmartPointer<vtkTable>> m_tableList;  //!< The data table for each class.
	QList<QColor> m_colorList;                     //!< The color for each class.
	std::vector<char> m_columnVisibility;          //!< Element(=column) visibility list

	//! @{ polar plot:
	void updatePolarPlotView(vtkTable* it);
	void drawPolarPlotMesh(vtkRenderer* renderer);
	void drawOrientationScalarBar(vtkScalarsToColors* lut);
	void drawAnnotations(vtkRenderer* renderer);
	void setupPolarPlotResolution(float grad);
	int calcOrientationProbability(vtkTable* t, vtkTable* ot);
	int m_gPhi, m_gThe;
	float m_PolarPlotPhiResolution, m_PolarPlotThetaResolution;
	//! @}

	//! @{  length distribution
	void showLengthDistribution(bool show, vtkScalarsToColors* lut = nullptr);
	void showOrientationDistribution();
	vtkSmartPointer<vtkContextView> m_lengthDistrView;
	//! @}

	//! @{ parallel coordinate chart/view:
	void setPCChartData(bool specialRendering = false);
	void applyPCSettings(QVariantMap const& values);
	void updatePCColumnVisibility();
	std::vector<size_t> getPCSelection();
	void updateAxisProperties();  //!< set properties for all axes in parallel coordinates: font size, tick count
	void updateMultiClassLookupTable(double alpha = 0.7);
	vtkSmartPointer<vtkContextView> m_pcView;
	vtkSmartPointer<vtkChartParallelCoordinates> m_pcChart;
	vtkSmartPointer<vtkEventQtSlotConnect> m_pcConnections;
	QVariantMap m_pcSettings;
	vtkSmartPointer<vtkLookupTable> m_multiClassLUT;  //!< Color lookup table for multi-class rendering in PC view
	int m_mousePressPos[2];
	//! @}

	//! @{ Class Explorer
	void setupClassExplorer();
	void calculateElementTable();
	void setActiveClassItem(QStandardItem* item, int situ = 0);
	void updateClassStatistics(QStandardItem* item);
	void saveClassesXML(QXmlStreamWriter& stream, bool idOnly);
	void loadClassesXML(QXmlStreamReader& reader);
	void writeClassesAndChildren(QXmlStreamWriter* writer, QStandardItem* item, bool idOnly) const;
	void writeWisetex(QXmlStreamWriter* writer);
	QTreeView* m_classTreeView;                       //!< Class tree view
	QTableView* m_elementTableView;                   //!< Element(=column) table view
	QStandardItemModel* m_classTreeModel;             //!< Model for class tree view
	QStandardItemModel* m_elementTableModel;          //!< Model for element table
	QStandardItem* m_activeClassItem;                 //!< Currently active class item in classTreeView/Model
	QAction* m_blobRendering, * m_blobRemoveRendering, * m_objectDelete, * m_saveBlobMovie;
	//! @}

	//! @{ blob rendering
	bool openBlobVisDialog();
	std::unique_ptr<iABlobManager> m_blobManager;
	QMap<QString, iABlobCluster*> m_blobMap;
	dlg_blobVisualization* m_blobVisDialog;
	//! @}

	vtkSmartPointer<vtkContextView> m_dvContextView;

	vtkSmartPointer<vtkScalarBarActor> m_scalarBarPP;
	vtkSmartPointer<vtkScalarBarActor> m_scalarBarFLD;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetPP;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarWidgetFLD;

	iAQVTKWidget *m_pcWidget, *m_polarPlotWidget, *m_lengthDistrWidget;
	iAPolarPlotWidget* m_ppWidget;
	iAClassExplorer* m_classExplorer;
	iADockWidgetWrapper *m_dwPC, *m_dwPP, *m_dwCE, *m_dwDV, *m_dwSPM;

	std::shared_ptr<iAFeatureScoutSPLOM> m_splom;
	iAObjectVis* m_3dvis;  //!< object visualization; FeatureScout is NOT owner (typically, dataset viewer is)
	std::shared_ptr<iAObjectVisActor> m_3dactor;
	std::shared_ptr<iAMeanObject> m_meanObject;
};
