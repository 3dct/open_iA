#include "iACompHistogramTable.h"

//Debug
#include "iAConsole.h"

//CompVis
#include "iACompHistogramTableData.h"

//iA
#include "mainwindow.h"

//Qt
#include "QVTKOpenGLNativeWidget.h"
#include <QColor>

//vtk
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>

#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkLookupTable.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>

#include <vtkColorSeries.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataObject.h>
#include <vtkNamedColors.h>
#include <vtkOutlineFilter.h>
#include <vtkUnsignedCharArray.h>
#include <algorithm>

iACompHistogramTable::iACompHistogramTable(MainWindow* parent, iAMultidimensionalScaling* mds) :
	QDockWidget(parent),
	m_mds(mds),
	m_inputData(mds->getCSVFileData()),
	m_bins(10),
	m_BinRangeLength(0),
	m_tableSize(10)
{
	//initialize GUI
	setupUi(this);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(qvtkWidget);

	std::vector<int>* dataResolution = csvFileData::getAmountObjectsEveryDataset(m_inputData);
	m_amountDatasets = dataResolution->size();
	m_colSize = qvtkWidget->size().height() / m_amountDatasets;
	m_rowSize = qvtkWidget->size().width();

	//initialize datastructure
	calculateHistogramTable();
	//initialize cell range
	initializeCellRange();
	//initialize visualization
	drawHistogramTable();
}

void iACompHistogramTable::calculateHistogramTable()
{
	m_histData = new iACompHistogramTableData(m_mds);
}

void iACompHistogramTable::makeLUTFromCTF(vtkLookupTable* lut)
{
	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToRGB();

	QColor red = QColor(164, 0, 0, 255);
	QColor white = QColor(255, 255, 255, 255);
	// red to white
	ctf->AddRGBPoint(1.0, red.redF(), red.greenF(), red.blueF());
	ctf->AddRGBPoint(0.9, red.redF(), red.greenF(), red.blueF());
	ctf->AddRGBPoint(0.0, white.redF(), white.greenF(), white.blueF());

	lut->SetNumberOfTableValues(m_tableSize);
	lut->Build();

	for (size_t i = 0; i < m_tableSize; ++i)
	{
		double* rgb;
		rgb = ctf->GetColor(static_cast<double>(i) / m_tableSize);
		lut->SetTableValue(i, rgb);
	}
}

void iACompHistogramTable::makeCellData(vtkLookupTable* lut, vtkUnsignedCharArray* colors, int currDataset)
{
	//todo get data of binData for correct coloring

	/*for (size_t i = 1; i < tableSize; i++)
	{
		double rgb[3];
		unsigned char ucrgb[3];
		// Get the interpolated color.
		// Of course you can use any function whose range is [0...1]
		// to get the required color and assign it to a cell Id.
		// In this case we are just using the cell (Id + 1)/(tableSize - 1)
		// to get the interpolated color.
		lut->GetColor(static_cast<double>(i) / (tableSize - 1), rgb);


		for (size_t j = 0; j < 3; ++j)
		{
			ucrgb[j] = static_cast<unsigned char>(rgb[j] * 255);
		}
		colors->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
	}
	*/
	QList<bin::BinType*>* binData = m_histData->getBinData();

	for (int bins = 0; bins < binData->at(currDataset)->size(); bins++) 
	{
		double rgb[3];
		unsigned char ucrgb[3];
	
		int amountVals = binData->at(currDataset)->at(bins).size();
		int startVal = 0;

		//DEBUG_LOG(" ");
		//DEBUG_LOG("amountVals " + QString::number(amountVals));
		//DEBUG_LOG("m_BinRangeLength: " + QString::number(m_BinRangeLength));

		for (int i = 0; i < m_tableSize; i++)
		{
			double low = startVal + (i * m_BinRangeLength);
			double high = low + ((i + 1) * m_BinRangeLength);

			//DEBUG_LOG("low: " + QString::number(low));
			//DEBUG_LOG("high: " + QString::number(high));
			//DEBUG_LOG("inside: " + QString::number(checkCellRange(amountVals, low, high)));

			if (checkCellRange((double)amountVals, low, high))
			{
				//DEBUG_LOG("i: " + QString::number(i));
				lut->GetColor(static_cast<double>(i) / (m_tableSize - 1), rgb);
				break;
			}
		}

		for (size_t j = 0; j < 3; ++j)
		{
			ucrgb[j] = static_cast<unsigned char>(rgb[j] * 255);
		}
		colors->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
		
	}
}

void iACompHistogramTable::initializeCellRange()
{
	int maxAmountInAllBins = m_histData->getMaxAmountInAllBins();
	m_BinRangeLength = ((double) maxAmountInAllBins) / ((double) m_tableSize);
}


bool iACompHistogramTable::checkCellRange(double value, double low, double high)
{	
   return (low <= value) && (value <= high);
}

void iACompHistogramTable::drawHistogramTable()
{
	// Setup render window, renderer, and interactor
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
	int thisCol = 1;
	vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();

	for (int cols = 0; cols < m_amountDatasets; cols++)
	{
		vtkSmartPointer<vtkPlaneSource> aPlane = vtkSmartPointer<vtkPlaneSource>::New();
		aPlane->SetXResolution(m_bins);
		aPlane->SetYResolution(thisCol);

		aPlane->SetOrigin(-m_rowSize, -(m_colSize * (cols + 1) + (m_colSize * cols)), 0.0);
		aPlane->SetPoint1(m_rowSize, -(m_colSize * (cols + 1) + (m_colSize * cols)), 0.0);  //width
		aPlane->SetPoint2(-m_rowSize, (m_colSize * (cols + 1) - (m_colSize * cols)), 0.0);  // height
		aPlane->Update();

		vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
		int tableSize = 10;
		makeLUTFromCTF(lut);

		vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
		colorData->SetName("colors");
		colorData->SetNumberOfComponents(3);
		makeCellData(lut, colorData, cols);
		aPlane->GetOutput()->GetCellData()->SetScalars(colorData);

		// Setup actor and mapper
		vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		mapper->SetInputConnection(aPlane->GetOutputPort());
		mapper->SetScalarModeToUseCellData();
		mapper->Update();

		vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
		actor->SetMapper(mapper);
		actor->GetProperty()->EdgeVisibilityOn();

		renderer->AddActor(actor);
	}

	renderer->SetBackground(nc->GetColor3d("SlateGray").GetData());

	vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;

	qvtkWidget->SetRenderWindow(renderWindow);
	qvtkWidget->GetRenderWindow()->AddRenderer(renderer);

	renderer->ResetCamera();
	qvtkWidget->show();
}

vtkSmartPointer<vtkActor> iACompHistogramTable::createOutline(vtkDataObject* object, double color[3])
{
	vtkSmartPointer<vtkOutlineFilter> outline = vtkSmartPointer<vtkOutlineFilter>::New();
	outline->SetInputData(object);
	vtkSmartPointer<vtkPolyDataMapper> outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	outlineMapper->SetInputConnection(outline->GetOutputPort());
	vtkSmartPointer<vtkActor> outlineActor = vtkSmartPointer<vtkActor>::New();
	outlineActor->SetMapper(outlineMapper);
	outlineActor->GetProperty()->SetColor(color[0], color[1], color[2]);

	return outlineActor;
}