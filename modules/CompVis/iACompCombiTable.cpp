// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompCombiTable.h"

//CompVis
#include "iACompHistogramVis.h"

#include "iALog.h"

//vtk
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkLookupTable.h"

#include "vtkActor.h"
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>

#include <vtkLine.h>
#include <vtkNew.h>
#include <vtkProgrammableGlyphFilter.h>
#include <vtkPlaneSource.h>
#include <vtkCellData.h>
#include <vtkUnsignedCharArray.h>

#include <vtkParametricSpline.h>
#include <vtkParametricFunctionSource.h>
#include <vtkTubeFilter.h>

iACompCombiTable::iACompCombiTable(
	iACompHistogramVis* vis, iACompKernelDensityEstimationData* kdeData, double lineWidth, double opacity) :
	iACompCurve(vis, kdeData, lineWidth, opacity),
	m_interactionStyle(vtkSmartPointer<iACompCombiTableInteractionStyle>::New())
{
}

/****************************************** Getter & Setter **********************************************/
vtkSmartPointer<iACompCombiTableInteractionStyle> iACompCombiTable::getInteractorStyle()
{
	return m_interactionStyle;
}

/****************************************** Rendering **********************************************/

void iACompCombiTable::drawHistogramTable()
{
	if (m_mainRenderer->GetViewProps()->GetNumberOfItems() > 0)
	{
		m_mainRenderer->RemoveAllViewProps();
	}

	m_vis->calculateRowWidthAndHeight(
		m_vis->getWindowWidth(), m_vis->getWindowHeight(), m_vis->getAmountDatasets());

	//draw bins of all datasets
	drawBins(getActiveBinPolyData());

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

void iACompCombiTable::drawRow(int currDataInd, int currentColumn, double offset)
{
	kdeData::kdeBins currDataset = getActiveData()->at(currDataInd);
	vtkSmartPointer<vtkPolyData> currBinPolyData = getActiveBinPolyData()->at(currentColumn);

	double min_x = 0.0;
	double min_y = (m_vis->getColSize() * currentColumn) + offset;
	double max_x = m_vis->getRowSize();
	double max_y = (m_vis->getColSize() * (1.0 + currentColumn)) + offset;
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

	//draw curve
	vtkSmartPointer<vtkPolyData> curveData = this->drawCurve(
		drawingDimensions, currDataset, getActiveBinPolyData()->at(currentColumn), currDataInd, currentColumn, offset);

	vtkNew<vtkPolyDataMapper> lineMapper;
	lineMapper->SetInputData(curveData);
	lineMapper->SetScalarRange(curveData->GetScalarRange());
	lineMapper->SetColorModeToDefault();
	lineMapper->SetScalarModeToUsePointData();
	lineMapper->GetInput()->GetPointData()->SetScalars(curveData->GetPointData()->GetArray("colorArray"));
	lineMapper->InterpolateScalarsBeforeMappingOff();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(lineMapper.Get());
	actor->GetProperty()->SetLineWidth(m_lineWidth);

	m_mainRenderer->AddActor(actor);

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

vtkSmartPointer<vtkPolyData> iACompCombiTable::drawCurve(double drawingDimensions[4], kdeData::kdeBins currDataset,
	vtkSmartPointer<vtkPolyData> currBinPolyData, int currDataInd, int currentColumn, double offset)
{
	double min_x = drawingDimensions[0];
	double min_y = drawingDimensions[2];

	int numberOfBins = static_cast<int>(currDataset.size());

	vtkSmartPointer<vtkPoints> curvePoints = vtkSmartPointer<vtkPoints>::New();
	int numberOPoints = 0;
	for (int i = 0; i < numberOfBins; i++)
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
				for (int nP = 0; nP < static_cast<int>(pointsOfNextSegment->GetNumberOfPoints()); nP++)
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

	vtkNew<vtkPolyLine> polyLine;
	polyLine->GetPointIds()->SetNumberOfIds(curvePoints->GetNumberOfPoints());
	for (unsigned int i = 0; i < curvePoints->GetNumberOfPoints(); i++)
	{
		polyLine->GetPointIds()->SetId(i, i);
	}

	// Create a cell array to store the lines in and add the lines to it
	vtkNew<vtkCellArray> cells;
	cells->InsertNextCell(polyLine.Get());

	auto polyData = vtkSmartPointer<vtkPolyData>::New();
	polyData->SetPoints(curvePoints);
	polyData->SetLines(cells.Get());
	polyData->GetPointData()->AddArray(colorArray);
	polyData->GetPointData()->SetActiveScalars("colorArray");

	return polyData;
}

void iACompCombiTable::drawBins(QList<vtkSmartPointer<vtkPolyData>>* binPolyData)
{
	for (int dataId = 0; dataId < binPolyData->size(); dataId++)
	{
		double numberOfBins = binPolyData->at(dataId)->GetNumberOfPoints();

		//each row consists of a certain number of bins and each bin will be drawn as glyph
		vtkSmartPointer<vtkPoints> glyphPoints = vtkSmartPointer<vtkPoints>::New();
		glyphPoints->SetDataTypeToDouble();
		glyphPoints->SetNumberOfPoints(numberOfBins);

		vtkSmartPointer<vtkPolyData> currDataset = binPolyData->at(dataId);
		vtkSmartPointer<vtkDataArray> oriColorArray = currDataset->GetCellData()->GetArray("colorArray");

		vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
		polydata->SetPoints(glyphPoints);
		polydata->GetPointData()->AddArray(currDataset->GetPointData()->GetArray("originArray"));
		polydata->GetPointData()->AddArray(currDataset->GetPointData()->GetArray("point1Array"));
		polydata->GetPointData()->AddArray(currDataset->GetPointData()->GetArray("point2Array"));

		vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
		colorArray->SetName("colorArray");
		colorArray->SetNumberOfComponents(4);
		colorArray->SetNumberOfTuples(numberOfBins);

		//set color for bin
		for (int i = 0; i < numberOfBins; i++)
		{
			double* rgb = oriColorArray->GetTuple3(i);
			double rgba[4] = {0, 0, 0, m_opacity};
			unsigned char ucrgb[4];
			iACompVisOptions::getColorArray4(rgba, ucrgb);
			colorArray->InsertTuple4(i, rgb[0], rgb[1], rgb[2], ucrgb[3]);
		}
		
		polydata->GetCellData()->AddArray(colorArray);
		polydata->GetCellData()->SetActiveScalars("colorArray");

		vtkSmartPointer<vtkPlaneSource> planeSource = vtkSmartPointer<vtkPlaneSource>::New();
		planeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
		planeSource->SetCenter(0, 0, 0);
		planeSource->Update();

		vtkSmartPointer<vtkProgrammableGlyphFilter> glypher = vtkSmartPointer<vtkProgrammableGlyphFilter>::New();
		glypher->SetInputData(polydata);
		glypher->SetSourceData(planeSource->GetOutput());
		glypher->SetGlyphMethod(buildGlyphRepresentation, glypher);
		glypher->Update();

		vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		glyphMapper->SetInputConnection(glypher->GetOutputPort());
		glyphMapper->SetColorModeToDefault();
		glyphMapper->SetScalarModeToUseCellData();
		glyphMapper->GetInput()->GetCellData()->SetScalars(polydata->GetCellData()->GetArray("colorArray"));
		glyphMapper->ScalarVisibilityOn();

		vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
		actor->SetMapper(glyphMapper);

		m_mainRenderer->AddActor(actor);
	}
}

/****************************************** Ordering/Ranking **********************************************/
void iACompCombiTable::drawBarChartShowingAmountOfObjects(std::vector<int> )
{
}

/****************************************** Update THIS **********************************************/
void iACompCombiTable::showSelectionOfCorrelationMap(std::map<int, double>* )
{
}

void iACompCombiTable::removeSelectionOfCorrelationMap()
{
}

/****************************************** Interaction Picking **********************************************/
void iACompCombiTable::highlightSelectedCell(vtkSmartPointer<vtkActor> , vtkIdType )
{
}

std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> iACompCombiTable::getSelectedData(
	Pick::PickedMap* )
{
	auto tuple = std::make_tuple(nullptr, nullptr);
	return tuple;
}
