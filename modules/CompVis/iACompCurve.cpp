#include "iACompCurve.h"

//CompVis
#include "iACompHistogramVis.h"
#include "iACompKernelDensityEstimationData.h"
#include "iACompBayesianBlocksData.h"
#include "iACompNaturalBreaksData.h"

//vtk
#include "vtkRenderer.h"
#include "vtkColorTransferFunction.h"
#include "vtkLookupTable.h"
#include "vtkCamera.h"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>

#include <vtkLine.h>
#include <vtkNew.h>

#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPolygon.h>

iACompCurve::iACompCurve(
	iACompHistogramVis* vis, iACompKernelDensityEstimationData* kdeData, double lineWidth, double opacity) :
	iACompTable(vis), 
	m_interactionStyle(vtkSmartPointer<iACompCurveInteractorStyle>::New()),
	m_opacity(opacity),
	m_lineWidth(lineWidth),
	m_kdeData(kdeData),
	m_originalRowActors(new std::vector<vtkSmartPointer<vtkActor>>())
{
	//initialize interaction
	initializeInteraction();

	//initialize visualization
	initializeTable();
}

void iACompCurve::setActive()
{
	//add rendererColorLegend to widget
	addLegendRendererToWidget();

	if (m_lastState == iACompVisOptions::lastState::Undefined)
	{
		drawHistogramTable();

		//init camera
		m_mainRenderer->SetActiveCamera(m_vis->getCamera());
		renderWidget();

		m_lastState = iACompVisOptions::lastState::Defined;
	}
	else if (m_lastState == iACompVisOptions::lastState::Defined)
	{
		reinitalizeState();

		drawHistogramTable();
		renderWidget();
	}
}

void iACompCurve::setInactive()
{
	m_mainRenderer->RemoveAllViewProps();
}

void iACompCurve::initializeCamera()
{
	m_mainRenderer->SetActiveCamera(m_vis->getCamera());
}

void iACompCurve::reinitalizeState()
{
	m_useDarkerLut = false;
}

/****************************************** Initialization **********************************************/
void iACompCurve::initializeTable()
{
	//setup color table
	makeLUTFromCTF();
	makeLUTDarker();

	//initialize legend
	initializeLegend();

	//init camera
	initializeCamera();
}

void iACompCurve::initializeInteraction()
{
	m_interactionStyle->setCurveVisualization(this);
	m_interactionStyle->SetDefaultRenderer(m_mainRenderer);
	m_interactionStyle->setIACompHistogramVis(m_vis);
}

void iACompCurve::makeLUTFromCTF()
{
	//define used colors
	QColor c1 = QColor(103, 21, 45);
	QColor c2 = QColor(128, 0, 38);
	QColor c3 = QColor(189, 0, 38);
	QColor c4 = QColor(227, 26, 28);
	QColor c5 = QColor(252, 78, 42);
	QColor c6 = QColor(253, 141, 60);
	QColor c7 = QColor(254, 178, 76);
	QColor c8 = QColor(254, 217, 118);
	QColor c9 = QColor(255, 237, 160);
	QColor c10 = QColor(255, 255, 204);
	
	//define main lut
	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToRGB();

	ctf->AddRGBPoint(1.0, c1.redF(), c1.greenF(), c1.blueF());
	ctf->AddRGBPoint(0.9, c1.redF(), c1.greenF(), c1.blueF());
	ctf->AddRGBPoint(0.8, c2.redF(), c2.greenF(), c2.blueF());
	ctf->AddRGBPoint(0.7, c3.redF(), c3.greenF(), c3.blueF());
	ctf->AddRGBPoint(0.6, c4.redF(), c4.greenF(), c4.blueF());
	ctf->AddRGBPoint(0.5, c5.redF(), c5.greenF(), c5.blueF());
	ctf->AddRGBPoint(0.4, c6.redF(), c6.greenF(), c6.blueF());
	ctf->AddRGBPoint(0.3, c7.redF(), c7.greenF(), c7.blueF());
	ctf->AddRGBPoint(0.2, c8.redF(), c8.greenF(), c8.blueF());
	ctf->AddRGBPoint(0.1, c9.redF(), c9.greenF(), c9.blueF());
	ctf->AddRGBPoint(0.0, c10.redF(), c10.greenF(), c10.blueF());

	m_lut->SetNumberOfTableValues(m_tableSize);
	m_lut->Build();

	//fill look up tables
	double min = 0;
	double max = 0;
	int startVal = 1;

	double binRange = calculateUniformBinRange();

	for (size_t i = 0; i < m_tableSize; i++)
	{
		double* rgb;
		rgb = ctf->GetColor(static_cast<double>(i) / (double)m_tableSize);
		m_lut->SetTableValue(i, rgb);

		//make format of annotations
		double low = round_up(startVal + (i * binRange), 2);
		double high = round_up(startVal + ((i + 1) * binRange), 2);

		std::string sLow = std::to_string(low);
		std::string sHigh = std::to_string(high);

		std::string lowerString = initializeLegendLabels(sLow);
		std::string upperString = initializeLegendLabels(sHigh);

		//position description in the middle of each color bar in the scalarBar legend
		//m_lut->SetAnnotation(low + ((high - low) * 0.5), lowerString + " - " + upperString);
		m_lut->SetAnnotation(low + ((high - low) * 0.5), lowerString);

		//store min and max value of the dataset
		if (i == 0)
		{
			min = low;
		}
		else if (i == m_tableSize - 1)
		{
			max = high;
		}
	}

	m_lut->SetTableRange(min, max);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE, col);
	m_lut->SetBelowRangeColor(col[0], col[1], col[2], 1);
	m_lut->UseBelowRangeColorOn();

	double* colAbove = ctf->GetColor(1);
	m_lut->SetAboveRangeColor(colAbove[0], colAbove[1], colAbove[2], 1);
	m_lut->UseAboveRangeColorOn();
}

void iACompCurve::makeLUTDarker()
{
	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToRGB();

	QColor c1 = QColor(51, 10, 23);
	QColor c2 = QColor(64, 0, 19);
	QColor c3 = QColor(64, 0, 19);
	QColor c4 = QColor(113, 13, 14);
	QColor c5 = QColor(126, 39, 21);
	QColor c6 = QColor(126, 70, 30);
	QColor c7 = QColor(127, 89, 38);
	QColor c8 = QColor(127, 108, 59);
	QColor c9 = QColor(127, 118, 80);
	QColor c10 = QColor(127, 127, 102);

	ctf->AddRGBPoint(1.0, c1.redF(), c1.greenF(), c1.blueF());
	ctf->AddRGBPoint(0.9, c1.redF(), c1.greenF(), c1.blueF());
	ctf->AddRGBPoint(0.8, c2.redF(), c2.greenF(), c2.blueF());
	ctf->AddRGBPoint(0.7, c3.redF(), c3.greenF(), c3.blueF());
	ctf->AddRGBPoint(0.6, c4.redF(), c4.greenF(), c4.blueF());
	ctf->AddRGBPoint(0.5, c5.redF(), c5.greenF(), c5.blueF());
	ctf->AddRGBPoint(0.4, c6.redF(), c6.greenF(), c6.blueF());
	ctf->AddRGBPoint(0.3, c7.redF(), c7.greenF(), c7.blueF());
	ctf->AddRGBPoint(0.2, c8.redF(), c8.greenF(), c8.blueF());
	ctf->AddRGBPoint(0.1, c9.redF(), c9.greenF(), c9.blueF());
	ctf->AddRGBPoint(0.0, c10.redF(), c10.greenF(), c10.blueF());

	m_lutDarker->SetNumberOfTableValues(m_tableSize);
	m_lutDarker->Build();

	double min = 0;
	double max = 0;
	int startVal = 1;

	double binRange = calculateUniformBinRange();

	for (size_t i = 0; i < m_tableSize; i++)
	{
		double* rgb;
		rgb = ctf->GetColor(static_cast<double>(i) / (double)m_tableSize);
		m_lutDarker->SetTableValue(i, rgb);

		//make format of annotations
		double low = round_up(startVal + (i * binRange), 2);
		double high = round_up(startVal + ((i + 1) * binRange), 2);

		std::string sLow = std::to_string(low);
		std::string sHigh = std::to_string(high);

		std::string lowerString = initializeLegendLabels(sLow);
		std::string upperString = initializeLegendLabels(sHigh);

		//position description in the middle of each color bar in the scalarBar legend
		//m_lut->SetAnnotation(low + ((high - low) * 0.5), lowerString + " - " + upperString);
		m_lutDarker->SetAnnotation(low + ((high - low) * 0.5), lowerString);

		//store min and max value of the dataset
		if (i == 0)
		{
			min = low;
		}
		else if (i == m_tableSize - 1)
		{
			max = high;
		}
	}

	m_lutDarker->SetTableRange(min, max);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	m_lutDarker->SetBelowRangeColor(col[0], col[1], col[2], 1);
	m_lutDarker->UseBelowRangeColorOn();

	double* colAbove = ctf->GetColor(1);
	m_lutDarker->SetAboveRangeColor(colAbove[0], colAbove[1], colAbove[2], 1);
	m_lutDarker->UseAboveRangeColorOn();
}

double iACompCurve::calculateUniformBinRange()
{
	int maxAmountInAllBins = m_kdeData->getMaxAmountInAllBins();
	return ((double)maxAmountInAllBins) / ((double)m_tableSize);
}

void iACompCurve::calculateBinRange()
{
}

/****************************************** Getter & Setter **********************************************/
vtkSmartPointer<iACompCurveInteractorStyle> iACompCurve::getInteractorStyle()
{
	return m_interactionStyle;
}

std::vector<vtkSmartPointer<vtkActor>>* iACompCurve::getOriginalRowActors()
{
	return m_originalRowActors;
}

QList<kdeData::kdeBins>* iACompCurve::getActiveData()
{
	if (m_vis->getActiveBinning() == iACompVisOptions::binningType::Uniform)
	{
		return m_kdeData->getKDEDataUniform();
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::JenksNaturalBreaks)
	{
		return m_kdeData->getKDEDataNB();
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::BayesianBlocks)
	{
		return m_kdeData->getKDEDataBB();
	}
	else
	{
		return m_kdeData->getKDEDataUniform();
	}
}

QList<std::vector<double>>* iACompCurve::getNumberOfObjectsInsideBin()
{
	if (m_vis->getActiveBinning() == iACompVisOptions::binningType::Uniform)
	{
		return m_kdeData->getObjectsPerBinUB();
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::JenksNaturalBreaks)
	{
		return m_kdeData->getObjectsPerBinNB();
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::BayesianBlocks)
	{
		return m_kdeData->getObjectsPerBinBB();
	}
	else
	{
		return m_kdeData->getObjectsPerBinUB();
	}
}

/****************************************** Rendering **********************************************/
void iACompCurve::drawHistogramTable()
{
	if (m_mainRenderer->GetViewProps()->GetNumberOfItems() > 0)
	{
		m_mainRenderer->RemoveAllViewProps();
	}

	m_vis->calculateRowWidthAndHeight(m_vis->getWindowWidth(), m_vis->getWindowHeight(), m_vis->getAmountDatasets());

	//draw cells from bottom to top --> so start with last dataset and go to first

	for (int currCol = 0; currCol < m_vis->getAmountDatasets(); currCol++)
	{
		int dataInd = m_vis->getOrderOfIndicesDatasets()->at(currCol);
		drawRow(dataInd, currCol, 0);
	}

	renderWidget();
}

void iACompCurve::drawRow(int currDataInd, int currentColumn, double offset)
{
	kdeData::kdeBins currDataset = getActiveData()->at(currDataInd);
	int numberOfBins = currDataset.size();

	double min_x = 0.0;
	double min_y = (m_vis->getColSize() * currentColumn) + offset;
	double max_x = m_vis->getRowSize();
	double max_y = (m_vis->getColSize() * (1+currentColumn)) + offset;

	//draw border line
	vtkSmartPointer<vtkPoints> linePoints = vtkSmartPointer<vtkPoints>::New();
	linePoints->InsertNextPoint(min_x, min_y, 0.0);
	linePoints->InsertNextPoint(max_x, min_y, 0.0);
	linePoints->InsertNextPoint(max_x, max_y, 0.0);
	linePoints->InsertNextPoint(min_x, max_y, 0.0);
	linePoints->InsertNextPoint(min_x, min_y, 0.0);
	vtkSmartPointer<vtkPolyData> lineData = drawLine(linePoints);

	//compute curve
	vtkSmartPointer<vtkPoints> startPoint = vtkSmartPointer<vtkPoints>::New();
	startPoint->InsertNextPoint(min_x, min_y, 0.0);

	//every dataset consists of several vtkPolylines --> each bin is its own line
	for (int binId = 0; binId < numberOfBins; binId++)
	{
		vtkSmartPointer<vtkPoints> curvePoints = vtkSmartPointer<vtkPoints>::New();
		int numberOfObjects = getNumberOfObjectsInsideBin()->at(currDataInd).at(binId);
		
		if (currDataset.at(binId).size() == 0 || currDataset.at(binId).size() == 1)
		{//if there are no values in the bin

			//start point of segment
			curvePoints->InsertNextPoint(startPoint->GetPoint(0));

			if (currDataset.at(binId).size() == 1)
			{//intermediate point of segment
				vtkSmartPointer<vtkPoints> intermediatePoints = vtkSmartPointer<vtkPoints>::New();
				computePoints(&currDataset.at(binId), currentColumn, offset, intermediatePoints);
				curvePoints->InsertNextPoint(intermediatePoints->GetPoint(0));
			}

			//end point of segment
			if (binId != numberOfBins-1)
			{
				if (currDataset.at(1+binId).size() == 0)
				{//what to do when also the next bin is also empty?
					double endPointX = curvePoints->GetPoint(0)[0] + 0.01;
					curvePoints->InsertNextPoint(endPointX, min_y, 0.0);
				}
				else
				{
					vtkSmartPointer<vtkPoints> curvePointsNextSegment = vtkSmartPointer<vtkPoints>::New();
					computePoints(&currDataset.at(1+binId), currentColumn, offset, curvePointsNextSegment);
					curvePoints->InsertNextPoint(curvePointsNextSegment->GetPoint(0)[0], min_y, 0.0);

					//TODO make better!
				}
			}
			else
			{
				curvePoints->InsertNextPoint(max_x, min_y, 0.0);
			}
		}
		else
		{
			computePoints(&currDataset.at(binId), currentColumn, offset, curvePoints);
		}
		
		if(binId == 0)
		{//make the first spline of all datasets start at the same point
			double* p = curvePoints->GetPoint(0);
			
			if (p[0] != min_x)
			{
				curvePoints->InsertPoint(0, min_x, min_y, 0.0);
			}
		}
		else if (binId != 0)
		{ //connect the individual splines
			curvePoints->InsertPoint(0, startPoint->GetPoint(0));
		}
		
		if (binId == numberOfBins - 1)
		{ //make the last spline of all datasets end at the same point
			
			double* p = curvePoints->GetPoint(curvePoints->GetNumberOfPoints() - 1);
			double max_x = m_vis->getRowSize();
			double min_y = (m_vis->getColSize() * currentColumn) + offset;

			if (p[0] != max_x)
			{
				curvePoints->InsertNextPoint(max_x, min_y, 0.0);
			}
		}

		curvePoints->InsertNextPoint(curvePoints->GetPoint(curvePoints->GetNumberOfPoints() - 1)[0], min_y, 0.0);
		curvePoints->InsertNextPoint(curvePoints->GetPoint(0)[0], min_y, 0.0);

		//draw inner polygon of curve segment
		drawPolygon(curvePoints, numberOfObjects);
		//draw borders of curve segment
		drawCurve(curvePoints, numberOfObjects);

		startPoint->SetPoint(0, curvePoints->GetPoint(curvePoints->GetNumberOfPoints() - 3));
	}

	//add name of dataset/row
	double pos[3] = {-(m_vis->getRowSize()) * 0.05, min_y + (m_vis->getColSize() * 0.5), 0.0};
	addDatasetName(currDataInd, pos);
}

vtkSmartPointer<vtkPolyData> iACompCurve::drawCurve(
	vtkSmartPointer<vtkPoints> drawingPoints, int numberOfObjectsInsideBin)
{
	vtkNew<vtkPolyLine> polyLine;
	polyLine->GetPointIds()->SetNumberOfIds(drawingPoints->GetNumberOfPoints());
	for (unsigned int i = 0; i < drawingPoints->GetNumberOfPoints(); i++)
	{
		polyLine->GetPointIds()->SetId(i, i);
	}

	// Create a cell array to store the lines in and add the lines to it
	vtkNew<vtkCellArray> cells;
	cells->InsertNextCell(polyLine);

	vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorArray->SetName("colorArray");
	colorArray->SetNumberOfComponents(3);
	colorArray->SetNumberOfTuples(drawingPoints->GetNumberOfPoints());

	colorCurve(drawingPoints, colorArray, numberOfObjectsInsideBin);

	vtkNew<vtkPolyData> polyData;
	polyData->SetPoints(drawingPoints);
	polyData->SetLines(cells);
	polyData->GetCellData()->AddArray(colorArray); 
	polyData->GetCellData()->SetActiveScalars("colorArray");

	vtkNew<vtkPolyDataMapper> lineMapper;
	lineMapper->SetInputData(polyData);
	lineMapper->SetScalarRange(polyData->GetScalarRange());
	lineMapper->SetColorModeToDefault();
	lineMapper->SetScalarModeToUseCellData();
	lineMapper->GetInput()->GetCellData()->SetScalars(colorArray);
	lineMapper->ScalarVisibilityOn();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(lineMapper);
	actor->GetProperty()->SetLineWidth(m_lineWidth);

	m_mainRenderer->AddActor(actor);

	return polyData;
}

vtkSmartPointer<vtkPolyData> iACompCurve::drawPolygon(vtkSmartPointer<vtkPoints> points, int numberOfObjectsInsideBin)
{
	vtkNew<vtkPolygon> polygon;
	polygon->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
	for (unsigned int i = 0; i < points->GetNumberOfPoints(); i++)
	{
		polygon->GetPointIds()->SetId(i, i);
	}

	vtkNew<vtkCellArray> cells;
	cells->InsertNextCell(polygon);

	vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorArray->SetName("colorArray");
	colorArray->SetNumberOfComponents(4);
	colorArray->SetNumberOfTuples(points->GetNumberOfPoints());

	colorPolygon(points, colorArray, numberOfObjectsInsideBin);

	// Create a polydata to store everything in
	vtkNew<vtkPolyData> polyData;
	polyData->SetPoints(points);
	polyData->SetPolys(cells);
	polyData->GetCellData()->AddArray(colorArray);
	polyData->GetCellData()->SetActiveScalars("colorArray");

	// Setup actor and mapper
	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputData(polyData);
	mapper->SetColorModeToDefault();
	mapper->SetScalarModeToUseCellData();
	mapper->GetInput()->GetCellData()->SetScalars(colorArray);
	mapper->ScalarVisibilityOn();
	
	vtkNew<vtkActor> actor;
	actor->SetMapper(mapper);

	m_mainRenderer->AddActor(actor);

	return polyData;
}

vtkSmartPointer<vtkPolyData> iACompCurve::drawLine(vtkSmartPointer<vtkPoints> points)
{
	vtkNew<vtkPolyLine> polyLine;
	polyLine->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
	for (unsigned int i = 0; i < points->GetNumberOfPoints(); i++)
	{
		polyLine->GetPointIds()->SetId(i, i);
	}

	// Create a cell array to store the lines in and add the lines to it
	vtkNew<vtkCellArray> cells;
	cells->InsertNextCell(polyLine);

	vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorArray->SetName("colorArray");
	colorArray->SetNumberOfComponents(3);
	colorArray->SetNumberOfTuples(points->GetNumberOfPoints());

	for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++)
	{
		colorArray->InsertTuple3(pointId, 255, 255, 255);
	}

	// Create a polydata to store everything in
	vtkNew<vtkPolyData> polyData;
	polyData->SetPoints(points);
	polyData->SetLines(cells);
	polyData->GetCellData()->AddArray(colorArray);
	polyData->GetCellData()->SetActiveScalars("colorArray");

	// Setup actor and mapper
	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputData(polyData);
	mapper->SetColorModeToDefault();
	mapper->SetScalarModeToUseCellData();
	mapper->GetInput()->GetCellData()->SetScalars(colorArray);
	mapper->ScalarVisibilityOn();

	vtkNew<vtkActor> actor;
	actor->SetMapper(mapper);

	m_mainRenderer->AddActor(actor);

	return polyData;
}

void iACompCurve::colorCurve(vtkSmartPointer<vtkPoints> points, vtkSmartPointer<vtkUnsignedCharArray> colorArray,
	int numberOfObjectsInsideBin)
{
	//calculate color for each bin
	double numberOfObjects = (double) numberOfObjectsInsideBin;

	for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++)
	{
		double* rgb = computeColor(numberOfObjects);

		unsigned char ucrgb[3];
		iACompVisOptions::getColorArray3(rgb, ucrgb);
		colorArray->InsertTuple3(pointId, ucrgb[0], ucrgb[1], ucrgb[2]);
	}
}

void iACompCurve::colorPolygon(
	vtkSmartPointer<vtkPoints> points, vtkSmartPointer<vtkUnsignedCharArray> colorArray, int numberOfObjectsInsideBin)
{
	for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++)
	{
		double* rgb = computeColor((double) numberOfObjectsInsideBin);

		double rgba[4] = {
			rgb[0],
			rgb[1],
			rgb[2],
			m_opacity
		};

		unsigned char ucrgba[4];
		iACompVisOptions::getColorArray4(rgba, ucrgba);
		colorArray->InsertTuple4(pointId, ucrgba[0], ucrgba[1], ucrgba[2], ucrgba[3]);
	}
}

double* iACompCurve::computeColor(double numberOfObjects)
{
	double rgb[3] = {0.0, 0.0, 0.0};
	double* rgbBorder;

	double maxNumber = m_lut->GetRange()[1];
	double minNumber = m_lut->GetRange()[0];

	bool isBorder = false;

	if (numberOfObjects > maxNumber)
	{
		if (m_useDarkerLut)
		{
			rgbBorder = m_lutDarker->GetAboveRangeColor();
		}
		else
		{
			rgbBorder = m_lut->GetAboveRangeColor();
		}

		isBorder = true;
	}
	else if (numberOfObjects < minNumber)
	{
		if (m_useDarkerLut)
		{
			rgbBorder = m_lutDarker->GetBelowRangeColor();
		}
		else
		{
			rgbBorder = m_lut->GetBelowRangeColor();
		}

		isBorder = true;
	}
	else
	{
		if (m_useDarkerLut)
		{
			m_lutDarker->GetColor(numberOfObjects, rgb);
		}
		else
		{
			m_lut->GetColor(numberOfObjects, rgb);
		}
	}

	double* result;
	if(isBorder)
	{
		return rgbBorder;
	}
	else
	{
		return rgb;
	}
}

void iACompCurve::computePoints(
	kdeData::kdeBin* currBinData, int currentColumn, double offset, vtkSmartPointer<vtkPoints> points)
{
	int numberOfPoints = currBinData->size();

	//drawing positions of square where the line should be drawn inbetween
	double min_x = 0.0;
	double max_x = m_vis->getRowSize();
	double min_y = (m_vis->getColSize() * currentColumn) + offset;
	double max_y = min_y + m_vis->getColSize() * 0.95;

	//computed positions
	double minMds = m_kdeData->getMinVal();
	double maxMds = m_kdeData->getMaxVal();
	double minKDE = m_kdeData->getMinKDEVal();
	double maxKDE = m_kdeData->getMaxKDEVal();

	//interval change
	for (int pairId = 0; pairId < currBinData->size(); pairId++)
	{
		double currMDS = currBinData->at(pairId).at(0);
		double currKDE = currBinData->at(pairId).at(1);

		double newX = iACompVisOptions::histogramNormalization(currMDS, min_x, max_x, minMds, maxMds);
		double newY = iACompVisOptions::histogramNormalization(currKDE, min_y,max_y, minKDE, maxKDE);

		double p[3] = {newX, newY, 0.0};
		points->InsertNextPoint(p);
	}
}

/****************************************** Ordering/Ranking **********************************************/

void iACompCurve::drawHistogramTableInAscendingOrder()
{
	std::vector<int> amountObjectsEveryDataset = *(m_kdeData->getAmountObjectsEveryDataset());

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = m_vis->sortWithMemory(amountObjectsEveryDataset, 0);
	m_vis->setOrderOfIndicesDatasets(m_vis->reorderAccordingTo(newOrder));

	if (m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	m_useDarkerLut = true;

	drawHistogramTable();
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
	renderWidget();
}

void iACompCurve::drawHistogramTableInDescendingOrder()
{
	std::vector<int> amountObjectsEveryDataset = *(m_kdeData->getAmountObjectsEveryDataset());

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = m_vis->sortWithMemory(amountObjectsEveryDataset, 1);
	m_vis->setOrderOfIndicesDatasets(m_vis->reorderAccordingTo(newOrder));

	if (m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	m_useDarkerLut = true;

	drawHistogramTable();
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
	renderWidget();
}

void iACompCurve::drawHistogramTableInOriginalOrder()
{
	std::vector<int>* originalOrderOfIndicesDatasets = m_vis->getOriginalOrderOfIndicesDatasets();
	std::vector<int>* orderOfIndicesDatasets = m_vis->getOrderOfIndicesDatasets();

	iACompVisOptions::copyVector(originalOrderOfIndicesDatasets, orderOfIndicesDatasets);
	m_useDarkerLut = true;

	drawHistogramTable();

	std::vector<int> amountObjectsEveryDataset = *(m_kdeData->getAmountObjectsEveryDataset());
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);

	renderWidget();
}

void iACompCurve::drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset)
{
}

/****************************************** Update THIS **********************************************/
void iACompCurve::showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType)
{
}

void iACompCurve::removeSelectionOfCorrelationMap()
{
}

/****************************************** Interaction Picking **********************************************/
void iACompCurve::highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId)
{
}
std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> iACompCurve::getSelectedData(
	Pick::PickedMap* map)
{
	auto tuple = std::make_tuple(nullptr, nullptr);
	return tuple;
}