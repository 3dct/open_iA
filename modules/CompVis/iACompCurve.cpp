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
	m_BBbinPolyDatasets(nullptr)
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
	double drawingDimensions[4] = {min_x, max_x, min_y, max_y};

	//draw border line
	vtkSmartPointer<vtkPoints> linePoints = vtkSmartPointer<vtkPoints>::New();
	linePoints->InsertNextPoint(min_x, min_y, 0.0);
	linePoints->InsertNextPoint(max_x, min_y, 0.0);
	linePoints->InsertNextPoint(max_x, max_y, 0.0);
	linePoints->InsertNextPoint(min_x, max_y, 0.0);
	linePoints->InsertNextPoint(min_x, min_y, 0.0);
	vtkSmartPointer<vtkPolyData> lineData = drawLine(linePoints);

	vtkSmartPointer<vtkPolyData> curveData = drawCurve(
		drawingDimensions, currDataset, getActiveBinPolyData()->at(currentColumn), currDataInd, currentColumn, offset);

	// Setup actor and mapper
	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputData(curveData);
	mapper->SetColorModeToDefault();
	mapper->SetScalarModeToUseCellData();
	mapper->GetInput()->GetCellData()->SetScalars(curveData->GetCellData()->GetArray("colorArray"));
	mapper->ScalarVisibilityOn();
	
	vtkNew<vtkActor> actor;
	actor->SetMapper(mapper);

	m_mainRenderer->AddActor(actor);
	
	//add name of dataset/row
	double pos[3] = {-(m_vis->getRowSize()) * 0.05, min_y + (m_vis->getColSize() * 0.5), 0.0};
	addDatasetName(currDataInd, pos);
}

vtkSmartPointer<vtkPolyData>  iACompCurve::drawCurve(double drawingDimensions[4], kdeData::kdeBins currDataset,
	vtkSmartPointer<vtkPolyData> currBinPolyData, int currDataInd, int currentColumn, double offset)
{
	double min_x = drawingDimensions[0];
	double max_x = drawingDimensions[1];
	double min_y = drawingDimensions[2];
	double max_y = drawingDimensions[3];

	int numberOfBins = currDataset.size();

	vtkSmartPointer<vtkPoints> curvePoints = vtkSmartPointer<vtkPoints>::New();
	vtkNew<vtkAppendPolyData> appendFilter;

	int numberOPoints = 0;
	for (int i = 0; i < numberOfBins; i++)
	{
		numberOPoints = numberOPoints + currDataset.at(i).size();
	}

	vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorArray->SetName("colorArray");
	colorArray->SetNumberOfComponents(3);

	for (int binId = 0; binId < numberOfBins; binId++)
	{
		int numberOfObjectsForColor = (int)getNumberOfObjectsOfActiveData()->at(currDataInd).at(binId);
		int numberOfObjects = currDataset.at(binId).size();
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
			bool nextSegmentIsAvailable = (binId < (numberOfBins - 1)) && (currDataset.at(binId + 1).size() != 0);
			if (nextSegmentIsAvailable)
			{
				computePoints(&currDataset.at(binId + 1), currentColumn, offset, pointsOfNextSegment);
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

		//draw polygon of bin
		vtkSmartPointer<vtkPoints> polygonPoints = vtkSmartPointer<vtkPoints>::New();
		polygonPoints->InsertPoints(0, finalBinPoints->GetNumberOfPoints(), 0, finalBinPoints);
		polygonPoints->InsertNextPoint(finalBinPoints->GetPoint(finalBinPoints->GetNumberOfPoints() - 1)[0], min_y,
			finalBinPoints->GetPoint(finalBinPoints->GetNumberOfPoints() - 1)[2]);
		polygonPoints->InsertNextPoint(finalBinPoints->GetPoint(0)[0], min_y, finalBinPoints->GetPoint(0)[2]);
		polygonPoints->InsertNextPoint(finalBinPoints->GetPoint(0));
		vtkSmartPointer<vtkPolyData> thisPolygonData = drawPolygon(polygonPoints, numberOfObjectsForColor);

		//Append polydata
		appendFilter->AddInputData(thisPolygonData);
		appendFilter->Update();
		
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

	vtkSmartPointer<vtkPolyData> finalPolyData = appendFilter->GetOutput();
	
	return finalPolyData;
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
			1
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