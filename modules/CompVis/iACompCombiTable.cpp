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

void iACompCombiTable::drawRow(int currDataInd, int currentColumn, double offset)
{
	kdeData::kdeBins currDataset = getActiveData()->at(currDataInd);
	int numberOfBins = currDataset.size();

	double min_x = 0.0;
	double min_y = (m_vis->getColSize() * currentColumn) + offset;
	double max_x = m_vis->getRowSize();
	double max_y = (m_vis->getColSize() * (1 + currentColumn)) + offset;
	double drawingDimensions[4] = {min_x, max_x, min_y, max_y};

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

	std::vector<std::vector<double>>* binPoints = new std::vector<std::vector<double>>(numberOfBins, std::vector<double>(2, 0));
	double binBorders[2];
	binBorders[0] = min_x;
	std::vector<double>* numberOfObjectsInBins = new std::vector<double>(numberOfBins, 0);

	//every dataset consists of several vtkPolylines --> each bin is its own line
	for (int binId = 0; binId < numberOfBins; binId++)
	{
		vtkSmartPointer<vtkPoints> curvePoints = vtkSmartPointer<vtkPoints>::New();
		int numberOfObjects = getNumberOfObjectsInsideBin()->at(currDataInd).at(binId);
		

		if (currDataset.at(binId).size() == 0 || currDataset.at(binId).size() == 1)
		{  //if there are no values in the bin

			//start point of segment
			curvePoints->InsertNextPoint(startPoint->GetPoint(0));

			if (currDataset.at(binId).size() == 1)
			{  //intermediate point of segment
				vtkSmartPointer<vtkPoints> intermediatePoints = vtkSmartPointer<vtkPoints>::New();
				computePoints(&currDataset.at(binId), currentColumn, offset, intermediatePoints);
				curvePoints->InsertNextPoint(intermediatePoints->GetPoint(0));
			}

			//end point of segment
			if (binId != numberOfBins - 1)
			{
				if (currDataset.at(1 + binId).size() == 0)
				{  //what to do when also the next bin is also empty?
					double endPointX = curvePoints->GetPoint(0)[0] + 0.01;
					curvePoints->InsertNextPoint(endPointX, min_y, 0.0);
				}
				else
				{
					vtkSmartPointer<vtkPoints> curvePointsNextSegment = vtkSmartPointer<vtkPoints>::New();
					computePoints(&currDataset.at(1 + binId), currentColumn, offset, curvePointsNextSegment);
					curvePoints->InsertNextPoint(curvePointsNextSegment->GetPoint(0)[0], min_y, 0.0);

					//TODO make better!
				}

				binBorders[1] = curvePoints->GetPoint(curvePoints->GetNumberOfPoints()-1)[0];
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

		if (binId == 0)
		{  //make the first spline of all datasets start at the same point
			double* p = curvePoints->GetPoint(0);

			if (p[0] != min_x)
			{
				curvePoints->InsertPoint(0, min_x, min_y, 0.0);
			}
		}
		else if (binId != 0)
		{  //connect the individual splines
			curvePoints->InsertPoint(0, startPoint->GetPoint(0));
		}

		if (binId == numberOfBins - 1)
		{  //make the last spline of all datasets end at the same point

			double* p = curvePoints->GetPoint(curvePoints->GetNumberOfPoints() - 1);
			double max_x = m_vis->getRowSize();
			double min_y = (m_vis->getColSize() * currentColumn) + offset;

			if (p[0] != max_x)
			{
				curvePoints->InsertNextPoint(max_x, min_y, 0.0);
			}
		}

		//draw borders of curve segment
		drawCurve(curvePoints, numberOfObjects);

		startPoint->SetPoint(0, curvePoints->GetPoint(curvePoints->GetNumberOfPoints()-1));

		binBorders[1] = curvePoints->GetPoint(curvePoints->GetNumberOfPoints() - 1)[0];
		binPoints->at(binId) = std::vector<double>{binBorders[0], binBorders[1]};
		numberOfObjectsInBins->at(binId) = numberOfObjects;
		binBorders[0] = curvePoints->GetPoint(curvePoints->GetNumberOfPoints() - 1)[0];
	}

	//draw bins
	drawBins(binPoints, numberOfObjectsInBins, drawingDimensions);

	//add name of dataset/row
	double pos[3] = {-(m_vis->getRowSize()) * 0.05, min_y + (m_vis->getColSize() * 0.5), 0.0};
	addDatasetName(currDataInd, pos);
}

void iACompCombiTable::drawBins(
	std::vector<std::vector<double>>* drawingPoints, std::vector<double>* numberOfObjectsInBins, double drawingDimensions[4])
{
	double min_x = drawingDimensions[0];
	double max_x = drawingDimensions[1];
	double min_y = drawingDimensions[2];
	double max_y = drawingDimensions[3];

	double numberOfBins = numberOfObjectsInBins->size();

	//each row consists of a certain number of bins and each bin will be drawn as glyph
	vtkSmartPointer<vtkPoints> glyphPoints = vtkSmartPointer<vtkPoints>::New();
	glyphPoints->SetDataTypeToDouble();
	glyphPoints->SetNumberOfPoints(numberOfBins);

	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->SetPoints(glyphPoints);

	vtkSmartPointer<vtkDoubleArray> originArray = vtkSmartPointer<vtkDoubleArray>::New();
	originArray->SetName("originArray");
	originArray->SetNumberOfComponents(3);
	originArray->SetNumberOfTuples(numberOfBins);

	vtkSmartPointer<vtkDoubleArray> point1Array = vtkSmartPointer<vtkDoubleArray>::New();
	point1Array->SetName("point1Array");
	point1Array->SetNumberOfComponents(3);
	point1Array->SetNumberOfTuples(numberOfBins);

	vtkSmartPointer<vtkDoubleArray> point2Array = vtkSmartPointer<vtkDoubleArray>::New();
	point2Array->SetName("point2Array");
	point2Array->SetNumberOfComponents(3);
	point2Array->SetNumberOfTuples(numberOfBins);

	vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorArray->SetName("colorArray");
	colorArray->SetNumberOfComponents(4);
	colorArray->SetNumberOfTuples(numberOfBins);

	for (int i = 0; i < drawingPoints->size(); i++)
	{
		//set position for bin
		double posXMin = drawingPoints->at(i).at(0);
		double posXMax = drawingPoints->at(i).at(1);
		originArray->InsertTuple3(i, posXMin, min_y, 0.0);
		point1Array->InsertTuple3(i, posXMax, min_y, 0.0);  //width
		point2Array->InsertTuple3(i, posXMin, max_y, 0.0); 

		//set color for bin
		double* rgb = computeColor(numberOfObjectsInBins->at(i));
		double rgba[4] = {rgb[0], rgb[1], rgb[2], m_opacity};
		unsigned char ucrgb[4];
		iACompVisOptions::getColorArray4(rgba, ucrgb);
		colorArray->InsertTuple4(i, ucrgb[0], ucrgb[1], ucrgb[2], ucrgb[3]);
	}

	polydata->GetPointData()->AddArray(originArray);
	polydata->GetPointData()->AddArray(point1Array);
	polydata->GetPointData()->AddArray(point2Array);
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
	glyphMapper->GetInput()->GetCellData()->SetScalars(colorArray);
	glyphMapper->ScalarVisibilityOn();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(glyphMapper);

	m_mainRenderer->AddActor(actor);
}

/****************************************** Ordering/Ranking **********************************************/
void iACompCombiTable::drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset)
{
}

/****************************************** Update THIS **********************************************/
void iACompCombiTable::showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType)
{
}

void iACompCombiTable::removeSelectionOfCorrelationMap()
{
}

/****************************************** Interaction Picking **********************************************/
void iACompCombiTable::highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId)
{
}

std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> iACompCombiTable::getSelectedData(
	Pick::PickedMap* map)
{
	auto tuple = std::make_tuple(nullptr, nullptr);
	return tuple;
}