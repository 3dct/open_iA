#include "iACompBoxPlot.h"

//CompVis
#include "iACompVisOptions.h"

//iA
#include "mainwindow.h"

//Qt
#include "vtkGenericOpenGLRenderWindow.h"
#include "QVTKOpenGLNativeWidget.h"

//vtk
#include "vtkRenderer.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"

#include "vtkChartBox.h"
#include "vtkChart.h"
#include "vtkTable.h"
#include "vtkPlot.h"
#include "vtkTextProperty.h"
#include "vtkAxis.h"

#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"

#include "vtkComputeQuartiles.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkLookupTable.h"
#include "vtkVariant.h"

#include "vtkPlotBox.h"

#include "vtkTextActor.h"
#include "vtkTooltipItem.h"
#include "vtkContextMouseEvent.h"
#include "vtkProperty2D.h"

#include "vtkCellArray.h"
#include "vtkPolyDataMapper.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkRegularPolygonSource.h"
#include  "vtkCoordinate.h"
#include  "vtkActor2D.h"
#include "vtkPolyDataMapper2D.h"

#include <string>

vtkStandardNewMacro(iACompBoxPlot::BoxPlotChart);

iACompBoxPlot::iACompBoxPlot(MainWindow* parent, iACsvDataStorage* dataStorage) : 
	QDockWidget(parent),
	m_dataStorage(dataStorage),
	maxValsAttr(new std::vector<double>()),
	minValsAttr(new std::vector<double>()),
	inputBoxPlotTable(vtkSmartPointer<vtkTable>::New()),
	m_legendAttributes(new std::vector<vtkSmartPointer<vtkTextActor>>()),
	m_numberOfAttr(0),
	finishedInitalization(false)
{
	setupUi(this);

	this->setFeatures(DockWidgetVerticalTitleBar);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	m_qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(m_qvtkWidget);

	m_view = vtkSmartPointer<vtkContextView>::New();
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	m_view->SetRenderWindow(m_qvtkWidget->GetRenderWindow());
	m_view->SetInteractor(m_qvtkWidget->GetInteractor());
#else
	m_view->SetRenderWindow(m_qvtkWidget->renderWindow());
	m_view->SetInteractor(m_qvtkWidget->interactor());
#endif
}

void iACompBoxPlot::showEvent(QShowEvent* event)
{
	// data preparation
	QList<csvFileData>* data = m_dataStorage->getData();
	m_numberOfAttr = data->at(0).header->size(); //amount of attributes
	QStringList* attrNames = m_dataStorage->getData()->at(0).header;

	//create chart (a plot is stored inside a chart)
	chart = vtkSmartPointer<BoxPlotChart>::New();
	chart->setOuterClass(this);
	chart->Update();
	m_view->GetScene()->AddItem(chart);

	// Set the labels
	vtkSmartPointer<vtkStringArray> labels = vtkSmartPointer<vtkStringArray>::New();
	
	//set amount of attributes
	for (int i = 0; i < m_numberOfAttr; i++)
	{
		labels->InsertNextValue(attrNames->at(i).toStdString());

		vtkSmartPointer<vtkIntArray> arrIndex = vtkSmartPointer<vtkIntArray>::New();
		arrIndex->SetName(attrNames->at(i).toStdString().c_str());
		inputBoxPlotTable->AddColumn(arrIndex);
	}

	//calculate amount of objects(fibers)/rows
	int numberOfRows = 0;
	for(int i = 0; i < data->size(); i++)
	{
		numberOfRows += csvDataType::getRows(data->at(i).values);
	}
	inputBoxPlotTable->SetNumberOfRows(numberOfRows);
	
	int row = 0;
	//fill table with data
	for (int i = 0; i < data->size(); i++)
	{//for all datasets
		for(int dataInd = 0; dataInd < data->at(i).values->size(); dataInd++)
		{ //for all values
			//for (int attrInd = 1; attrInd <= m_numberOfAttr; attrInd++)
			for (int attrInd = 1; attrInd < m_numberOfAttr; attrInd++)
			{//for all attributes but without the label attribute

				int col = attrInd-1;
				int newCol = m_orderedPositions->at(attrInd);
				double val = data->at(i).values->at(dataInd).at(m_orderedPositions->at(col)+1);

				inputBoxPlotTable->SetValue(row, col, vtkVariant(val));
			}
			row++;
		}
	}

	//calculate quartiles
	vtkSmartPointer<vtkComputeQuartiles> quartiles = vtkSmartPointer<vtkComputeQuartiles>::New();
	quartiles->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, inputBoxPlotTable);
	quartiles->Update();
	outTable = quartiles->GetOutput();
	
	vtkSmartPointer<vtkTable> normalizedTable = vtkSmartPointer<vtkTable>::New();
	normalizedTable->DeepCopy(outTable);

	//normalize quartiles in the interval [0,1]
	for (vtkIdType c = 0; c < outTable->GetNumberOfColumns(); c++)
	{
		for (vtkIdType r = 0; r < outTable->GetNumberOfRows(); r++)
		{
			vtkVariant v = outTable->GetValue(r, c);
			double newV = iACompVisOptions::histogramNormalization(v.ToDouble(), 0.0, 1.0, outTable->GetValue(0, c).ToDouble(), outTable->GetValue(outTable->GetNumberOfRows()-1, c).ToDouble());
			normalizedTable->SetValue(r, c, vtkVariant(newV));
		}
	}

	//create lookup table
	vtkSmartPointer<vtkLookupTable> lookup = vtkSmartPointer<vtkLookupTable>::New();
	lookup->SetNumberOfColors(2);
	lookup->SetRange(0, 1);
	lookup->Build();
	double col[3];
	col[0] = iACompVisOptions::BACKGROUNDCOLOR_GREY[0];
	col[1] = iACompVisOptions::BACKGROUNDCOLOR_GREY[1];
	col[2] = iACompVisOptions::BACKGROUNDCOLOR_GREY[2];
	lookup->SetTableValue(0, col[0], col[1], col[2]);
	lookup->SetTableValue(1, col[0], col[1], col[2]);

	//create box plot
	box = vtkSmartPointer<vtkPlotBox>::New();
	box->SetInputData(normalizedTable);
	box->SetLookupTable(lookup);
	
	chart->SetPlot(box);
	chart->SetColumnVisibilityAll(true);
	chart->Update();

	//render all to get position of columns
	renderWidget();

	//set size of chart
	chart->SetPoint1(m_qvtkWidget->width()*0.0, (m_qvtkWidget->height()*0.3)); //0.4
	chart->SetPoint2(m_qvtkWidget->width()*0.75, (m_qvtkWidget->height())*0.85);
	chart->Update();

	//create title
	chart->SetTitle("Interval Similarity");
	chart->GetTitleProperties()->BoldOn();
	chart->GetTitleProperties()->ItalicOff();
	chart->GetTitleProperties()->ShadowOff();
	chart->GetTitleProperties()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	chart->GetTitleProperties()->SetFontFamilyToArial();
	chart->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	chart->GetTitleProperties()->Modified();
	
	//create y axis
	vtkAxis *axisLeft = chart->GetYAxis();
	axisLeft->SetBehavior(1);
	axisLeft->SetTitle("Converted Interval Size");
	axisLeft->GetTitleProperties()->ItalicOff();
	axisLeft->GetTitleProperties()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	axisLeft->GetTitleProperties()->SetFontFamilyToArial();
	axisLeft->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	axisLeft->GetTitleProperties()->SetLineOffset(-20);
	axisLeft->GetTitleProperties()->SetOrientation(90);
	axisLeft->GetTitleProperties()->SetVerticalJustification(VTK_TEXT_CENTERED);
	axisLeft->GetTitleProperties()->SetJustification(VTK_TEXT_CENTERED);
	axisLeft->SetMaximum(1.0);
	axisLeft->SetMinimum(0.0);
	axisLeft->AutoScale();
	axisLeft->SetNumberOfTicks(5);

	//for each column draw a legend
	double offset = 1.0;
	double factor = offset/ ((double)(m_numberOfAttr -1));
	for (int i = 0; i < m_numberOfAttr; i++)
	{
		vtkSmartPointer<vtkTextActor> legend = vtkSmartPointer<vtkTextActor>::New();
		legend->SetTextScaleModeToNone();	
		legend->SetInput(labels->GetValue(m_orderedPositions->at(i))); //reordered positions

		vtkSmartPointer<vtkTextProperty> legendProperty = legend->GetTextProperty();
		legendProperty->BoldOff();
		legendProperty->ItalicOff();
		legendProperty->ShadowOff();
		legendProperty->SetFontFamilyToArial();
		legendProperty->SetColor(0, 0, 0);
		legendProperty->SetOrientation(20);
		legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
		legendProperty->SetJustification(VTK_TEXT_RIGHT);
		legendProperty->SetVerticalJustificationToTop();
		legendProperty->Modified();
	
		float x = chart->GetXPosition(i) + (box->GetBoxWidth()*offset);
		legend->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
		legend->GetPositionCoordinate()->SetValue(x, m_qvtkWidget->height()*0.27);
		legend->Modified();

		m_view->GetRenderer()->AddActor(legend);
		m_legendAttributes->push_back(legend);

		//TESTING
		/*DEBUG_LOG("chart->GetXPosition(i) = (" + QString::number(chart->GetXPosition(i)) + ")");
		DEBUG_LOG("(box->GetBoxWidth()*0.5) = (" + QString::number((box->GetBoxWidth() * offset)) + ")");

		vtkSmartPointer<vtkRegularPolygonSource> point = vtkSmartPointer<vtkRegularPolygonSource>::New();
		point->SetNumberOfSides(50);
		point->SetRadius(1);
		point->SetCenter(x, m_qvtkWidget->height()*0.25, 0);

		// Visualize
		vtkSmartPointer<vtkPolyDataMapper2D> mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
		mapper->SetInputConnection(point->GetOutputPort());

		vtkSmartPointer<vtkActor2D> actor = vtkSmartPointer<vtkActor2D>::New();
		actor->SetMapper(mapper);
		actor->GetProperty()->SetColor(1, 0, 0);
		m_view->GetRenderer()->AddActor2D(actor);*/
		

		offset = offset - factor;
	}

	finishedInitalization = true;
}

void iACompBoxPlot::updateLegend()
{
	double offset = 1.0;
	double factor = offset / ((double)(m_numberOfAttr - 1));

	for (int i = 0; i < m_numberOfAttr; i++)
	{
		float x = chart->GetXPosition(i) + (box->GetBoxWidth()*offset);
		vtkSmartPointer<vtkTextActor> legend = m_legendAttributes->at(i);
		legend->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
		legend->GetPositionCoordinate()->SetValue(x, m_qvtkWidget->height()*0.27);

		legend->Modified();

		offset = offset - factor;
	}
}

void iACompBoxPlot::renderWidget()
{
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	m_qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
#else
	m_qvtkWidget->renderWindow()->GetInteractor()->Render();
#endif
}

void iACompBoxPlot::setOrderedPositions(std::vector<double>* orderedPositions)
{
	m_orderedPositions = orderedPositions;
}

/************************* INNER CLASS BoxPlotChart *******************************************/
void iACompBoxPlot::BoxPlotChart::SetTooltipInfo(const vtkContextMouseEvent& mouse,
	const vtkVector2d &plotPos,
	vtkIdType seriesIndex, vtkPlot* plot,
	vtkIdType segmentIndex)
{
	if (!this->Tooltip)
	  {
		return;
	  }
	
	std::vector<std::string> tokens;
	
	for (vtkIdType r = 0; r < m_outerClass->outTable->GetNumberOfRows(); r++)
	{
		vtkVariant v = m_outerClass->outTable->GetValue(r, plotPos.GetX());
		tokens.push_back(v.ToString());
	}

	vtkStdString tooltipLabel = 
		"Minimum = " + tokens.at(0) + "\n" +
		"First quartile = " + tokens.at(1) + "\n" + 
		"Median = " + tokens.at(2) + "\n" + 
		"Third quartile = " + tokens.at(3) + "\n" + 
		"Maximum = " + tokens.at(4);

  // Set the tooltip
  this->Tooltip->SetText(tooltipLabel);
  this->Tooltip->SetPosition(mouse.GetScreenPos()[0] + 2, mouse.GetScreenPos()[1] + 2);
}

void iACompBoxPlot::BoxPlotChart::Update()
{
	/*if (m_outerClass->finishedInitalization)
	{
		m_outerClass->chart->SetPoint1(m_outerClass->m_qvtkWidget->width()*0.0, (m_outerClass->m_qvtkWidget->height()*0.3)); //0.4
		m_outerClass->chart->SetPoint2(m_outerClass->m_qvtkWidget->width()*0.75, (m_outerClass->m_qvtkWidget->height())*0.85);
		m_outerClass->chart->Modified();
	}*/
	
	vtkChartBox::Update();

	if(m_outerClass->finishedInitalization)
	{
		//m_outerClass->updateLegend();
	}

}

void iACompBoxPlot::BoxPlotChart::setOuterClass(iACompBoxPlot * outerClass)
{
	m_outerClass = outerClass;
}

iACompBoxPlot::BoxPlotChart::BoxPlotChart()
{
}
