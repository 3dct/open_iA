#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "iAMultidimensionalScaling.h"

//iA
#include "charts/iAHistogramData.h"

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

class MainWindow;
class iACsvDataStorage;
class QVTKOpenGLNativeWidget;
class vtkLookupTable;
class vtkColorSeries;
class vtkNamedColors;
class vtkDataObject;
class vtkActor;
class vtkUnsignedCharArray;

class iACompHistogramTableData;

class iACompHistogramTable : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT
   public:
	iACompHistogramTable(MainWindow* parent, iAMultidimensionalScaling* mds);

   private:
	void calculateHistogramTable();
	void drawHistogramTable();
	void makeLUTFromCTF(vtkLookupTable* lut);
	void makeCellData(vtkLookupTable* lut, vtkUnsignedCharArray* colors, int currDataset);
	void initializeCellRange();
	bool checkCellRange(double value, double low, double high);
	// Create the outline
	vtkSmartPointer<vtkActor> createOutline(vtkDataObject* object, double color[3]);

	QList<csvFileData>* m_inputData;
	iAMultidimensionalScaling* m_mds;
	iACompHistogramTableData* m_histData;

	QVTKOpenGLNativeWidget* qvtkWidget;

	int m_bins;
	int m_amountDatasets;
	int m_tableSize;
	double m_BinRangeLength;
	double m_colSize;
	double m_rowSize;
};