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
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkAppendFilter.h>

iACompCurve::iACompCurve(
	iACompHistogramVis* vis, iACompKernelDensityEstimationData* kdeData, double lineWidth, double opacity) :
	iACompTable(vis), 
	m_interactionStyle(vtkSmartPointer<iACompCurveInteractorStyle>::New()),
	m_opacity(opacity),
	m_lineWidth(lineWidth),
	m_kdeData(kdeData),
	m_originalRowActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_UBbinPolyDatasets(nullptr),
	m_NBbinPolyDatasets(nullptr),
	m_BBbinPolyDatasets(nullptr),
	m_drawWhiteCurves(false)
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
	else if (m_lastState == iACompVisOptions::lastState::Changed)
	{
		drawHistogramTable();
		renderWidget();
	}
}

void iACompCurve::setDrawWhite(bool drawWhite)
{
	m_drawWhiteCurves = drawWhite;
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
	////define used colors
	//QColor c1 = QColor(103, 21, 45);
	//QColor c2 = QColor(128, 0, 38);
	//QColor c3 = QColor(189, 0, 38);
	//QColor c4 = QColor(227, 26, 28);
	//QColor c5 = QColor(252, 78, 42);
	//QColor c6 = QColor(253, 141, 60);
	//QColor c7 = QColor(254, 178, 76);
	//QColor c8 = QColor(254, 217, 118);
	//QColor c9 = QColor(255, 237, 160);
	//QColor c10 = QColor(255, 255, 204);

	//Virdis: dark blue to yellow
	QColor c10 = QColor(68, 1, 84);
	QColor c9 = QColor(72, 40, 120);
	QColor c8 = QColor(62, 73, 137);
	QColor c7 = QColor(49, 104, 142);
	QColor c6 = QColor(38, 130, 142);
	QColor c5 = QColor(31, 158, 137);
	QColor c4 = QColor(53, 183, 121);
	QColor c3 = QColor(110, 206, 88);
	QColor c2 = QColor(181, 222, 43);
	QColor c1 = QColor(253, 231, 37);
	
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
		m_lut->SetAnnotation(low + ((high - low) * 0.5), lowerString + " - " + upperString);
		//m_lut->SetAnnotation(low + ((high - low) * 0.5), lowerString);

		//store min and max value of the dataset
		if (i == 0)
		{
			min = low;
		}
		else if (i == m_tableSize - 1.0)
		{
			max = high;
		}
	}

	m_lut->SetTableRange(min, max);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_BLACK, col);  //iACompVisOptions::BACKGROUNDCOLOR_WHITE
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
		else if (i == m_tableSize - 1.0)
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

QList<std::vector<double>>* iACompCurve::getNumberOfObjectsOfActiveData()
{
	if (m_vis->getActiveBinning() == iACompVisOptions::binningType::Uniform)
	{
		return m_kdeData->getNumberOfObjectsPerBinUB();
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::JenksNaturalBreaks)
	{
		return m_kdeData->getNumberOfObjectsPerBinNB();
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::BayesianBlocks)
	{
		return m_kdeData->getNumberOfObjectsPerBinBB();
	}
	else
	{
		return m_kdeData->getNumberOfObjectsPerBinUB();
	}
}

QList<vtkSmartPointer<vtkPolyData>>* iACompCurve::getActiveBinPolyData()
{
	if (m_vis->getActiveBinning() == iACompVisOptions::binningType::Uniform)
	{
		return m_UBbinPolyDatasets;
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::JenksNaturalBreaks)
	{
		return m_NBbinPolyDatasets;
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::BayesianBlocks)
	{
		return m_BBbinPolyDatasets;
	}
	else
	{
		return m_UBbinPolyDatasets;
	}
}

void iACompCurve::setUBBinData(QList<vtkSmartPointer<vtkPolyData>>* ubPolyData)
{
	m_UBbinPolyDatasets = ubPolyData;
}

void iACompCurve::setNBBinData(QList<vtkSmartPointer<vtkPolyData>>* nbPolyData)
{
	m_NBbinPolyDatasets = nbPolyData;
}

void iACompCurve::setBBBinData(QList<vtkSmartPointer<vtkPolyData>>* bbPolyData)
{
	m_BBbinPolyDatasets = bbPolyData;
}

QList<vtkSmartPointer<vtkPolyData>>* iACompCurve::getUBBinData()
{
	return m_UBbinPolyDatasets;
}

QList<vtkSmartPointer<vtkPolyData>>* iACompCurve::getNBBinData()
{
	return m_NBbinPolyDatasets;
}

QList<vtkSmartPointer<vtkPolyData>>* iACompCurve::getBBBinData()
{
	return m_BBbinPolyDatasets;
}

void iACompCurve::setKDEData(iACompKernelDensityEstimationData* kdeData)
{
	m_kdeData = kdeData;
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

QList<std::vector<double>>* iACompCurve::getBoundariesofActiveData()
{
	if (m_vis->getActiveBinning() == iACompVisOptions::binningType::Uniform)
	{
		return m_kdeData->getBoundariesUB();
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::JenksNaturalBreaks)
	{
		return m_kdeData->getBoundariesNB();
	}
	else if (m_vis->getActiveBinning() == iACompVisOptions::binningType::BayesianBlocks)
	{
		return m_kdeData->getBoundariesBB();
	}
	else
	{
		return m_kdeData->getBoundariesUB();
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

	//draw x-axis on the bottom
	double min_x = 0.0;
	double max_x = m_vis->getRowSize();
	double max_y = m_vis->getColSize() * -0.25;
	double min_y = m_vis->getColSize() * -0.75;
	double drawingDimensions[4] = {min_x, max_x, min_y, max_y};
	drawXAxis(drawingDimensions);

	renderWidget();
}

void iACompCurve::drawRow(int currDataInd, int currentColumn, double offset)
{
	kdeData::kdeBins currDataset = getActiveData()->at(currDataInd);

	double min_x = 0.0;
	double min_y = (m_vis->getColSize() * currentColumn) + offset;
	double max_x = m_vis->getRowSize();
	double max_y = (m_vis->getColSize() * (1.0+currentColumn)) + offset;
	double drawingDimensions[4] = {min_x, max_x, min_y, max_y};

	//draw border line
	vtkSmartPointer<vtkPoints> linePoints = vtkSmartPointer<vtkPoints>::New();
	linePoints->InsertNextPoint(min_x, min_y, 0.0);
	linePoints->InsertNextPoint(max_x, min_y, 0.0);
	linePoints->InsertNextPoint(max_x, max_y, 0.0);
	linePoints->InsertNextPoint(min_x, max_y, 0.0);
	linePoints->InsertNextPoint(min_x, min_y, 0.0);
	vtkSmartPointer<vtkPolyData> lineData = drawLine(linePoints);

	//draw 4 tick axes
	double numberOfTicks = 4;
	drawTicks(numberOfTicks, drawingDimensions);

	//draw curve and polygons
	drawCurveAndPolygon(drawingDimensions, currDataset, getActiveBinPolyData()->at(currentColumn), currDataInd, currentColumn, offset);
	
	//add name of dataset/row
	double pos[3] = {-(m_vis->getRowSize()) * 0.05, min_y + (m_vis->getColSize() * 0.5), 0.0};
	addDatasetName(currDataInd, pos);

	//add X Ticks
	if (m_vis->getXAxis())
	{
		double yheight = min_y;
		double tickLength = (max_y - min_y) * 0.05;
		drawXTicks(drawingDimensions, yheight, tickLength);
	}
}

void iACompCurve::drawCurveAndPolygon(double drawingDimensions[4], kdeData::kdeBins currDataset,
	vtkSmartPointer<vtkPolyData> currBinPolyData, int currDataInd, int currentColumn, double offset)
{
	double min_x = drawingDimensions[0];
	double min_y = drawingDimensions[2];

	int numberOfBins = static_cast<int>(currDataset.size());

	vtkSmartPointer<vtkPoints> curvePoints = vtkSmartPointer<vtkPoints>::New();
	int numberOPoints = 0;
	for (auto i = 0; i < numberOfBins; i++)
	{
		numberOPoints = numberOPoints + static_cast<int>(currDataset.at(i).size());
	}

	vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorArray->SetName("colorArray");
	colorArray->SetNumberOfComponents(3);

	for (int binId = 0; binId < numberOfBins; binId++)
	{
		int numberOfObjectsForColor = (int)getNumberOfObjectsOfActiveData()->at(currDataInd).at(binId);
		int numberOfObjects = static_cast<int>(currDataset.at(binId).size());
		double binXMin = currBinPolyData->GetPointData()->GetArray("originArray")->GetTuple3(binId)[0];
		double binXMax = currBinPolyData->GetPointData()->GetArray("point1Array")->GetTuple3(binId)[0];

		vtkSmartPointer<vtkPoints> finalBinPoints = vtkSmartPointer<vtkPoints>::New();
		vtkSmartPointer<vtkPoints> binPoints = vtkSmartPointer<vtkPoints>::New();

		vtkSmartPointer<vtkUnsignedCharArray> binColorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
		binColorArray->SetName("binColorArray");
		binColorArray->SetNumberOfComponents(3);

		if (numberOfObjects == 0)
		{
			binPoints->InsertNextPoint(binXMin, min_y, 0.0);
			binPoints->InsertNextPoint(binXMax, min_y, 0.0);
		}
		else if (numberOfObjects == 1)
		{
			vtkSmartPointer<vtkPoints> point = vtkSmartPointer<vtkPoints>::New();
			double p[3] = {binXMin + ((binXMax - binXMin) * 0.5), min_y, 0.0};
			if (currDataset.at(binId).size() != 0)
			{
				//fill vtkPoints with points of this bin
				computePoints(&currDataset.at(binId), currentColumn, offset, point);
				p[0] = point->GetPoint(0)[0];
				p[1] = point->GetPoint(0)[1];
				p[2] = point->GetPoint(0)[2];
			}

			binPoints->InsertNextPoint(binXMin, p[1], 0.0);
			binPoints->InsertNextPoint(p);
			binPoints->InsertNextPoint(binXMax, p[1], 0.0);
		}
		else
		{
			//fill vtkPoints with points of this bin
			computePoints(&currDataset.at(binId), currentColumn, offset, binPoints);
		}

		//look if start and endpoint of bin are inside
		double startP[3] = {0.0, 0.0, 0.0};
		double endP[3] = {0.0, 0.0, 0.0};
		bool minXContained = false;
		bool maxXContained = false;
		for (int i = 0; i < binPoints->GetNumberOfPoints(); i++)
		{
			if (binXMin == binPoints->GetPoint(i)[0])
			{
				minXContained = true;
			}

			if (binXMax == binPoints->GetPoint(i)[0])
			{
				maxXContained = true;
			}

			if (minXContained && maxXContained)
			{
				break;
			}
		}

		if (!minXContained)
		{
			double lastPoint[3] = {min_x, min_y, 0.0};

			if (binId != 0)
			{
				startP[0] = binXMin;
				startP[1] = curvePoints->GetPoint(curvePoints->GetNumberOfPoints() - 1)[1];
				startP[2] = 0.0;
			}
			else
			{
				startP[0] = binXMin;
				startP[1] = lastPoint[1];
				startP[2] = 0.0;
			}

			finalBinPoints->InsertNextPoint(startP);
		}

		//fill intermediate points
		if (finalBinPoints->GetNumberOfPoints() == 0)
		{
			finalBinPoints->InsertPoints(0, binPoints->GetNumberOfPoints(), 0, binPoints);
		}
		else
		{
			for (int j = 0; j < binPoints->GetNumberOfPoints(); j++)
			{
				double* p = binPoints->GetPoint(j);
				if (p[0] >= binXMin && p[0] <= binXMax)
				{
					finalBinPoints->InsertNextPoint(p);
				}
			}
		}

		if (!maxXContained)
		{
			//	get points of next segment
			double nextPoint[3] = {0.0, min_y, 0.0};

			vtkSmartPointer<vtkPoints> pointsOfNextSegment = vtkSmartPointer<vtkPoints>::New();
			bool nextSegmentIsAvailable = (binId < (numberOfBins - 1.0)) && (currDataset.at(binId + 1.0).size() != 0);
			if (nextSegmentIsAvailable)
			{
				computePoints(&currDataset.at(binId + 1.0), currentColumn, offset, pointsOfNextSegment);
				for (int nP = 0; nP < pointsOfNextSegment->GetNumberOfPoints(); nP++)
				{
					nextPoint[0] = pointsOfNextSegment->GetPoint(0)[0];
					nextPoint[1] = binPoints->GetPoint(binPoints->GetNumberOfPoints() - 1)[1];
					nextPoint[2] = pointsOfNextSegment->GetPoint(0)[2];
				}

				endP[0] = binXMax;
				endP[1] = nextPoint[1];
				endP[2] = 0.0;
			}
			else
			{
				endP[0] = binXMax;
				endP[1] = min_y;
				endP[2] = 0.0;
			}

			finalBinPoints->InsertNextPoint(endP);
		}

		binColorArray->SetNumberOfTuples(finalBinPoints->GetNumberOfPoints());
		colorCurve(finalBinPoints, binColorArray, numberOfObjectsForColor);

		//fill resutling points and color array
		if (curvePoints->GetNumberOfPoints() == 0)
		{
			curvePoints->InsertPoints(0, finalBinPoints->GetNumberOfPoints(), 0, finalBinPoints);
			colorArray->InsertTuples(0, binColorArray->GetNumberOfTuples(), 0, binColorArray);
		}
		else
		{
			curvePoints->InsertPoints(
				curvePoints->GetNumberOfPoints(), finalBinPoints->GetNumberOfPoints(), 0, finalBinPoints);
			colorArray->InsertTuples(
				colorArray->GetNumberOfTuples(), binColorArray->GetNumberOfTuples(), 0, binColorArray);
		}
	}

	if (m_drawWhiteCurves)
	{
		drawWhiteCurve(curvePoints);
	}
	else
	{
		drawPolygon(curvePoints, colorArray, min_y);

		//draw the curve
		drawCurve(curvePoints, colorArray);
	}
}

void iACompCurve::drawWhiteCurve(vtkSmartPointer<vtkPoints> curvePoints)
{
	//color should be white for user study
	vtkSmartPointer<vtkUnsignedCharArray>  colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorArray->SetName("colorArray");
	colorArray->SetNumberOfComponents(3);
	colorArray->SetNumberOfTuples(curvePoints->GetNumberOfPoints());
	for (int i = 0; i < curvePoints->GetNumberOfPoints(); i++)
	{
		colorArray->InsertTuple3(i, 255,255,255);
	}

	vtkNew<vtkPolyLine> polyLine;
	polyLine->GetPointIds()->SetNumberOfIds(curvePoints->GetNumberOfPoints());
	for (unsigned int i = 0; i < curvePoints->GetNumberOfPoints(); i++)
	{
		polyLine->GetPointIds()->SetId(i, i);
	}

	// Create a cell array to store the lines in and add the lines to it
	vtkNew<vtkCellArray> cells;
	cells->InsertNextCell(polyLine);

	vtkNew<vtkPolyData> curvePolyData;
	curvePolyData->SetPoints(curvePoints);
	curvePolyData->SetLines(cells);
	curvePolyData->GetPointData()->AddArray(colorArray);
	curvePolyData->GetPointData()->SetActiveScalars("colorArray");
	vtkNew<vtkPolyDataMapper> lineMapper;
	lineMapper->SetInputData(curvePolyData);
	lineMapper->SetScalarRange(curvePolyData->GetScalarRange());
	lineMapper->SetColorModeToDefault();
	lineMapper->SetScalarModeToUsePointData();
	lineMapper->GetInput()->GetPointData()->SetScalars(curvePolyData->GetPointData()->GetArray("colorArray"));
	lineMapper->InterpolateScalarsBeforeMappingOff();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(lineMapper);
	actor->GetProperty()->SetLineWidth(m_lineWidth);

	m_mainRenderer->AddActor(actor);
}

vtkSmartPointer<vtkPolyData> iACompCurve::createPolygon(vtkSmartPointer<vtkPoints> points, int numberOfObjectsInsideBin)
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

	return polyData;
}

void iACompCurve::drawPolygon(vtkSmartPointer<vtkPoints> curvePoints, vtkSmartPointer<vtkUnsignedCharArray> colorArray, double bottomY)
{
	vtkNew<vtkAppendPolyData> appendFilter;

	for (unsigned int i = 0; i < curvePoints->GetNumberOfPoints()-1; i++)
	{ //compute each bin individually
		
		int numberOfPointsFormingBin = 5;

		//compute color array of bin
		vtkSmartPointer<vtkUnsignedCharArray> currBinColorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
		currBinColorArray->SetNumberOfTuples(numberOfPointsFormingBin);
		currBinColorArray->SetNumberOfComponents(3);
		currBinColorArray->SetName("CurrBinColorArray");
		double* color = colorArray->GetTuple3(i);
		for (int j = 0; j < numberOfPointsFormingBin; j++)
		{
			currBinColorArray->InsertTuple3(j, color[0], color[1], color[2]);
		}
		
		//compute points of bin
		vtkSmartPointer<vtkPoints> currBinCurvePoints = vtkSmartPointer<vtkPoints>::New();
		currBinCurvePoints->SetNumberOfPoints(numberOfPointsFormingBin);
		
		double currPoint[3];
		curvePoints->GetPoint(i, currPoint);
		double nextPoint[3];
		curvePoints->GetPoint(vtkIdType(i + 1.0), nextPoint);

		currBinCurvePoints->InsertPoint(0, currPoint);
		currBinCurvePoints->InsertPoint(1, nextPoint);
		currBinCurvePoints->InsertPoint(2, nextPoint[0], bottomY, nextPoint[2]);
		currBinCurvePoints->InsertPoint(3, currPoint[0], bottomY, currPoint[2]);
		currBinCurvePoints->InsertPoint(4, currPoint);

		vtkNew<vtkPolygon> polygon;
		polygon->GetPointIds()->SetNumberOfIds(numberOfPointsFormingBin);
		for (int j = 0; j < numberOfPointsFormingBin; j++)
		{
			polygon->GetPointIds()->SetId(j, j);
		}

		// Create a cell array to store the lines in and add the lines to it
		vtkNew<vtkCellArray> cells;
		cells->InsertNextCell(polygon);

		vtkNew<vtkPolyData> curvePolyData;
		curvePolyData->SetPoints(currBinCurvePoints);
		curvePolyData->SetPolys(cells);
		curvePolyData->GetCellData()->SetScalars(currBinColorArray);
		
		appendFilter->AddInputData(curvePolyData);
	}
	
	appendFilter->Update();

	// Remove any duplicate points.
	vtkNew<vtkCleanPolyData> cleanFilter;
	cleanFilter->SetInputConnection(appendFilter->GetOutputPort());
	cleanFilter->Update();

	vtkNew<vtkPolyDataMapper> lineMapper;
	lineMapper->SetInputConnection(cleanFilter->GetOutputPort());
	lineMapper->SetScalarRange(colorArray->GetRange());
	lineMapper->SetColorModeToDirectScalars();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(lineMapper);

	m_mainRenderer->AddActor(actor);
}

void iACompCurve::drawCurve(
	vtkSmartPointer<vtkPoints> curvePoints, vtkSmartPointer<vtkUnsignedCharArray> colorArray)
{
	////color should be white for user study
	//colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	//colorArray->SetName("colorArray");
	//colorArray->SetNumberOfComponents(3);
	//colorArray->SetNumberOfTuples(curvePoints->GetNumberOfPoints());
	//for (int i = 0; i < curvePoints->GetNumberOfPoints(); i++)
	//{
	//	colorArray->InsertTuple3(i, 255,255,255);
	//}

	vtkNew<vtkPolyLine> polyLine;
	polyLine->GetPointIds()->SetNumberOfIds(curvePoints->GetNumberOfPoints());
	for (unsigned int i = 0; i < curvePoints->GetNumberOfPoints(); i++)
	{
		polyLine->GetPointIds()->SetId(i, i);
	}

	// Create a cell array to store the lines in and add the lines to it
	vtkNew<vtkCellArray> cells;
	cells->InsertNextCell(polyLine);

	vtkNew<vtkPolyData> curvePolyData;
	curvePolyData->SetPoints(curvePoints);
	curvePolyData->SetLines(cells);
	curvePolyData->GetPointData()->AddArray(colorArray);
	curvePolyData->GetPointData()->SetActiveScalars("colorArray");
	vtkNew<vtkPolyDataMapper> lineMapper;
	lineMapper->SetInputData(curvePolyData);
	lineMapper->SetScalarRange(curvePolyData->GetScalarRange());
	lineMapper->SetColorModeToDefault();
	lineMapper->SetScalarModeToUsePointData();
	lineMapper->GetInput()->GetPointData()->SetScalars(curvePolyData->GetPointData()->GetArray("colorArray"));
	lineMapper->InterpolateScalarsBeforeMappingOff();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(lineMapper);
	actor->GetProperty()->SetLineWidth(m_lineWidth);

	m_mainRenderer->AddActor(actor);
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

void iACompCurve::drawTicks(double numberOfTicks, double drawingDimensions[4])
{
	double min_x = drawingDimensions[0];
	double max_x = drawingDimensions[1];
	double min_y = drawingDimensions[2];
	double max_y = drawingDimensions[3];

	double tickLength = (max_x - min_x) * 0.01;
	double yheight = max_y - min_y;
	double tickDistance = yheight / numberOfTicks;

	vtkNew<vtkAppendPolyData> appendFilter;
	for (int i = 0; i < numberOfTicks; i++)
	{
		vtkSmartPointer<vtkPoints> tickPoints = vtkSmartPointer<vtkPoints>::New();
		tickPoints->InsertNextPoint(min_x - (tickLength), min_y + (tickDistance * i), 0.0);
		tickPoints->InsertNextPoint(min_x + (tickLength), min_y + (tickDistance * i), 0.0);

		vtkNew<vtkPolyLine> polyLine;
		polyLine->GetPointIds()->SetNumberOfIds(tickPoints->GetNumberOfPoints());
		for (unsigned int j = 0; j < tickPoints->GetNumberOfPoints(); j++)
		{
			polyLine->GetPointIds()->SetId(j, j);
		}

		// Create a cell array to store the lines in and add the lines to it
		vtkNew<vtkCellArray> cells;
		cells->InsertNextCell(polyLine);

		vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
		colorArray->SetName("colorArray");
		colorArray->SetNumberOfComponents(3);
		colorArray->SetNumberOfTuples(tickPoints->GetNumberOfPoints());

		for (int pointId = 0; pointId < tickPoints->GetNumberOfPoints(); pointId++)
		{
			colorArray->InsertTuple3(pointId, 255, 255, 255);
		}

		// Create a polydata to store everything in
		vtkNew<vtkPolyData> tickPolyData;
		tickPolyData->SetPoints(tickPoints);
		tickPolyData->SetLines(cells);
		tickPolyData->GetCellData()->AddArray(colorArray);
		tickPolyData->GetCellData()->SetActiveScalars("colorArray");

		appendFilter->AddInputData(tickPolyData);
	}

	// Setup actor and mapper
	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputConnection(appendFilter->GetOutputPort());
	
	vtkNew<vtkActor> actor;
	actor->SetMapper(mapper);

	m_mainRenderer->AddActor(actor);
}

void iACompCurve::colorCurve(vtkSmartPointer<vtkPoints> points, vtkSmartPointer<vtkUnsignedCharArray> colorArray,
	int numberOfObjectsInsideBin)
{
	//calculate color for each bin
	double numberOfObjects = (double) numberOfObjectsInsideBin;

	for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++)
	{
		double rgb[3];
		computeColor(numberOfObjects, rgb);

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
		double rgb[3];
		computeColor((double) numberOfObjectsInsideBin, rgb);

		double rgba[4] = {
			rgb[0],
			rgb[1],
			rgb[2],
			1
		};

		unsigned char ucrgba[4];
		iACompVisOptions::getColorArray4(rgba, ucrgba);
		colorArray->InsertTuple4(pointId, ucrgba[0], ucrgba[1], ucrgba[2], ucrgba[3]);
	}
}

double* iACompCurve::computeColor(double numberOfObjects, double result[3])
{
	double rgb[3] = {0.0, 0.0, 0.0};
	double* rgbBorder = nullptr;

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

	if(isBorder)
	{
		result[0] = rgbBorder[0];
		result[1] = rgbBorder[1];
		result[2] = rgbBorder[2];
	}
	else
	{
		result[0] = rgb[0];
		result[1] = rgb[1];
		result[2] = rgb[2];
	}

	return result;
}

void iACompCurve::computePoints(
	kdeData::kdeBin* currBinData, int currentColumn, double offset, vtkSmartPointer<vtkPoints> points)
{
	int numberOfPoints = static_cast<int>(currBinData->size());

	//drawing positions of square where the line should be drawn inbetween
	double min_x = 0.0;
	double max_x = m_vis->getRowSize();
	double min_y = (m_vis->getColSize() * currentColumn) + offset;
	double max_y = min_y + (m_vis->getColSize() * 0.97);

	//computed positions
	double minMds = m_kdeData->getMinVal();
	double maxMds = m_kdeData->getMaxVal();
	double minKDE = m_kdeData->getMinKDEVal();
	double maxKDE = m_kdeData->getMaxKDEVal();

	//interval change
	for (int pairId = 0; pairId < numberOfPoints; pairId++)
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
void iACompCurve::showSelectionOfCorrelationMap(std::map<int, double>*)
{
}

void iACompCurve::removeSelectionOfCorrelationMap()
{
}

/****************************************** Interaction Picking **********************************************/
void iACompCurve::highlightSelectedCell(vtkSmartPointer<vtkActor>, vtkIdType)
{
}

std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> iACompCurve::getSelectedData(
	Pick::PickedMap*)
{
	auto tuple = std::make_tuple(nullptr, nullptr);
	return tuple;
}