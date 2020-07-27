#include "iACompCorrelationMap.h"
#include <vtkObjectFactory.h> //for macro!

//CompVis
#include "iACompVisOptions.h"
#include "iACsvDataStorage.h"


//iA
#include "mainwindow.h"

//vtk
#include "QVTKOpenGLNativeWidget.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkProperty.h"

#include "vtkMutableUndirectedGraph.h"
#include "vtkGraphLayoutView.h"
#include <vtkVertexListIterator.h>
#include "vtkGraphToGlyphs.h"
#include "vtkGraphToPolyData.h"

#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDataSetAttributes.h"

#include "vtkViewTheme.h"
#include "vtkLookupTable.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"

#include <vtkEdgeListIterator.h>

#include "vtkArcSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkCubeSource.h"
#include "vtkGlyph3D.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTextActor.h"
#include "vtkImageData.h"
#include "vtkTexture.h"
#include "vtkPolygon.h"
#include "vtkCellArray.h"

#include "vtkPropPicker.h"
#include "vtkTooltipItem.h"

#include "vtkScalarBarWidget.h"
#include "vtkPolygon.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkOutlineFilter.h"
#include "vtkCubeSource.h"

#include "vtkForceDirectedLayoutStrategy.h"

#include <cmath>
#include <math.h> 


vtkStandardNewMacro(iACompCorrelationMap::CorrelationGraphLayout);
vtkStandardNewMacro(iACompCorrelationMap::GraphInteractorStyle);

iACompCorrelationMap::iACompCorrelationMap(MainWindow* parent, iACorrelationCoefficient* corrCalculation, iACsvDataStorage* dataStorage) :
	QDockWidget(parent),
	m_corrCalculation(corrCalculation),
	m_dataStorage(dataStorage),
	m_renderer(vtkSmartPointer<vtkRenderer>::New()),
	m_graphLayoutView(vtkSmartPointer<vtkGraphLayoutView>::New()),
	//m_graphLayout(vtkSmartPointer<CorrelationGraphLayout>::New()),
	m_graph(vtkSmartPointer<vtkMutableUndirectedGraph>::New()),
	m_lutForEdges(vtkSmartPointer<vtkLookupTable>::New()),
	m_lutForVertices(vtkSmartPointer<vtkLookupTable>::New()),
	m_lutForArcs(vtkSmartPointer<vtkLookupTable>::New()),
	m_theme(vtkSmartPointer<vtkViewTheme>::New()),
	m_vertices(new std::map<vtkIdType, QString>()),
	arcActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	legendActors(new std::vector<vtkSmartPointer<vtkActor2D>>()),
	glyphActors(new std::vector<vtkSmartPointer<vtkActor>>())
{
	setupUi(this);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	m_qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(m_qvtkWidget);

	m_renderer->SetViewport(0, 0, 0.8, 1);
	m_renderer->SetUseFXAA(true);
	m_qvtkWidget->GetRenderWindow()->AddRenderer(m_renderer);

	m_graphLayoutView->SetRenderWindow(m_qvtkWidget->GetRenderWindow());
	m_graphLayoutView->SetInteractor(m_qvtkWidget->GetInteractor());
	
	vtkSmartPointer<GraphInteractorStyle> style = vtkSmartPointer<GraphInteractorStyle>::New();
	style->setGraphLayoutView(m_graphLayoutView);
	style->setBaseClass(this);
	m_graphLayoutView->GetInteractor()->SetInteractorStyle(style);
	m_graphLayoutView->GetRenderer()->SetUseFXAA(true);

	m_theme->SetBackgroundColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE));
	m_theme->SetBackgroundColor2(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE));

	//data preparation
	QList<csvFileData>* data = m_dataStorage->getData();
	m_numberOfAttr = data->at(0).header->size(); //amount of attributes
	m_attrNames = *m_dataStorage->getData()->at(0).header;
	m_attrNames.removeFirst();//remove label attribute

	initializeCorrelationMap();
	initializeArcs();
	
	m_graphLayoutView->ResetCamera();
}

void iACompCorrelationMap::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);

	renderWidget();
}

void iACompCorrelationMap::renderWidget()
{
	m_qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
}

void iACompCorrelationMap::initializeCorrelationMap()
{
	//initialize lookup table
	initializeLutForVertices();
	initializeLutForEdges();

	//add & set representation for vertices & edges
	initializeVertices(m_attrNames);
	initializeEdges(m_attrNames);

	//m_graphLayoutView->SetLayoutStrategyToSimple2D();
	m_graphLayout = vtkSmartPointer<CorrelationGraphLayout>::New();
	m_graphLayout->setVertices(m_vertices);
	m_graphLayout->setCorrelations(m_corrCalculation->getCorrelationCoefficients());
	m_graphLayout->SetRandomInitialPoints(true);
	m_graphLayoutView->SetLayoutStrategy(m_graphLayout);

	m_theme->SetPointLookupTable(m_lutForVertices);
	m_theme->SetCellLookupTable(m_lutForEdges);

	//m_graphLayoutView->SetLabelPlacementModeToAll();
	m_graphLayoutView->ApplyViewTheme(m_theme);
}

void iACompCorrelationMap::initializeVertices(QStringList attrNames)
{
	vtkSmartPointer<vtkFloatArray> scales = vtkSmartPointer<vtkFloatArray>::New();
	scales->SetNumberOfComponents(1);
	scales->SetName("Scales");

	vtkSmartPointer<vtkIntArray> vertexColors =	vtkSmartPointer<vtkIntArray>::New();
	vertexColors->SetNumberOfComponents(1);
	vertexColors->SetName("Color");

	vtkSmartPointer<vtkStringArray> vertexIDs = vtkSmartPointer<vtkStringArray>::New();
	vertexIDs->SetNumberOfComponents(1);
	vertexIDs->SetName("VertexIDs");

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

	//add for each attribute a vertex in the graph
	for (int i = 0; i < attrNames.size(); i++)
	{
		vtkIdType currV = m_graph->AddVertex();
		
		scales->InsertNextValue(2.5);
		vertexColors->InsertNextValue(0);
		vertexIDs->InsertNextValue((" " + attrNames.at(i).toStdString() + "\n").c_str());
		points->InsertNextPoint(0, 0, 0);

		m_vertices->insert({ currV, attrNames.at(i) });
	}

	m_graph->SetPoints(points);

	m_graphLayoutView->AddRepresentationFromInput(m_graph);
	m_graphLayoutView->SetVertexLabelVisibility(true);

	m_graphLayoutView->SetScalingArrayName("Scales");
	m_graph->GetVertexData()->AddArray(scales);
	m_graphLayoutView->ScaledGlyphsOn();

	m_graph->GetVertexData()->AddArray(vertexColors);
	m_graphLayoutView->SetVertexColorArrayName("Color");
	m_graphLayoutView->ColorVerticesOn();

	m_graph->GetVertexData()->AddArray(vertexIDs);
	m_graphLayoutView->SetVertexLabelArrayName("VertexIDs");

	//set representation
	vtkRenderedGraphRepresentation* representation = dynamic_cast<vtkRenderedGraphRepresentation*>(m_graphLayoutView->GetRepresentation());
	representation->SetGlyphType(vtkGraphToGlyphs::CIRCLE);
	
	//set text representation for labels
	vtkSmartPointer<vtkTextProperty> labelText = representation->GetVertexLabelTextProperty();
	//labelText->SetLineOffset(-30); not working with vtkViewTheme
	//labelText->SetLineSpacing(-30); not working with vtkViewTheme
	labelText->BoldOff();
	labelText->ItalicOff();
	labelText->ShadowOff();
	labelText->SetFontFamilyToArial();
	labelText->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	labelText->SetVerticalJustificationToBottom();
	labelText->SetJustificationToCentered(); //SetJustificationToLeft();
	labelText->SetColor(0,0,0);
	labelText->Modified();
	
	m_theme->SetPointTextProperty(labelText); 
}

void iACompCorrelationMap::initializeLutForVertices()
{
	int tableSize = 1;

	QColor c1 = QColor(186, 186, 186); //255, 255, 255

	m_lutForVertices->SetNumberOfTableValues(tableSize);
	m_lutForVertices->SetTableValue(0, c1.redF(), c1.greenF(), c1.blueF());
	m_lutForVertices->Build();
}

void iACompCorrelationMap::initializeEdges(QStringList attrNames)
{
	vtkSmartPointer<vtkDoubleArray> edgeColors = vtkSmartPointer<vtkDoubleArray>::New();
	edgeColors->SetNumberOfComponents(1);
	edgeColors->SetName("Color");

	vtkSmartPointer<vtkDoubleArray> weights = vtkSmartPointer<vtkDoubleArray>::New();
	weights->SetNumberOfComponents(1);
	weights->SetName("Weights");

	//add an edge for each vertex to each other vertex
	for (vtkIdType currV = 0; currV < m_graph->GetNumberOfVertices() - 1; currV++)
	{
		for (vtkIdType nextV = currV + 1; nextV < m_graph->GetNumberOfVertices(); nextV++)
		{
			m_graph->AddEdge(currV, nextV);
			double colorInd = colorEdges(currV, nextV, m_corrCalculation->getCorrelationCoefficients(), m_vertices);
			edgeColors->InsertNextValue(colorInd);

			weights->InsertNextValue(edgeColors->GetValue(edgeColors->GetNumberOfValues()-1));
		}
	}

	m_graph->GetEdgeData()->AddArray(edgeColors);
	m_graphLayoutView->SetEdgeColorArrayName("Color");
	m_graphLayoutView->ColorEdgesOn();

	//display hover text
	m_graph->GetEdgeData()->AddArray(weights);
	vtkRenderedGraphRepresentation* representation = dynamic_cast<vtkRenderedGraphRepresentation*>(m_graphLayoutView->GetRepresentation());
	m_graphLayoutView->DisplayHoverTextOn();
	
	//set line width
	m_theme->SetLineWidth(3);
}

double iACompCorrelationMap::colorEdges(vtkIdType startVertex, vtkIdType endVertex, std::map<QString, Correlation::CorrelationStore>* correlations, std::map<vtkIdType, QString>* vertices)
{
	//find name of startVertex
	std::map<vtkIdType, QString >::const_iterator vertexIter = vertices->find(startVertex);
	if (vertexIter == vertices->end()) return 0.0;
	QString nameStartV = vertexIter->second;

	//find name of endVertex
	std::map<vtkIdType, QString >::const_iterator vertexIter1 = vertices->find(endVertex);
	if (vertexIter1 == vertices->end()) return 0.0;
	QString nameEndV = vertexIter1->second;

	//find correlation value of both
	std::map<QString, Correlation::CorrelationStore>::const_iterator pos = correlations->find(nameStartV);
	if (pos == correlations->end()) return 0.0;
	Correlation::CorrelationStore map = pos->second;

	Correlation::CorrelationStore::const_iterator pos1 = map.find(nameEndV);
	if (pos1 == map.end()) return 0.0;

	//DEBUG_LOG(nameStartV  + " to " + nameEndV + " with: " + QString::number(pos1->second));
	
	return pos1->second;	
}

void iACompCorrelationMap::initializeLutForEdges()
{
	int tableSize = 7;

	// red to white to blue
	QColor c7 = QColor(178, 24, 43);
	QColor c6 = QColor(239, 138, 98);
	QColor c5 = QColor(253, 219, 199);
	QColor c4 = QColor(255, 255, 255); // 135, 135, 135
	QColor c3 = QColor(209, 229, 240);
	QColor c2 = QColor(103, 169, 207);
	QColor c1 = QColor(33, 102, 172);

	m_lutForEdges->SetNumberOfTableValues(tableSize);
	m_lutForEdges->Build();

	m_lutForEdges->SetTableValue(0, c1.redF(), c1.greenF(), c1.blueF(), 1); //transparency 1
	m_lutForEdges->SetTableValue(1, c2.redF(), c2.greenF(), c2.blueF(), 1); //transparency 0.9
	m_lutForEdges->SetTableValue(2, c3.redF(), c3.greenF(), c3.blueF(), 1); //transparency  0.7
	m_lutForEdges->SetTableValue(3, c4.redF(), c4.greenF(), c4.blueF(), 1); //transparency 0.35
	m_lutForEdges->SetTableValue(4, c5.redF(), c5.greenF(), c5.blueF(), 1); //transparency  0.7
	m_lutForEdges->SetTableValue(5, c6.redF(), c6.greenF(), c6.blueF(), 1);//transparency 0.9
	m_lutForEdges->SetTableValue(6, c7.redF(), c7.greenF(), c7.blueF(), 1);//transparency 1

	m_lutForEdges->SetTableRange(-1.0, 1.0);
	
	/*double col[3];
	col[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[0];
	col[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[1];
	col[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[2];
	m_lutForEdges->SetBelowRangeColor(col[0], col[1], col[2], 1);
	m_lutForEdges->UseBelowRangeColorOn();

	double col1[3];
	col1[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[0];
	col1[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[1];
	col1[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[2];
	m_lutForEdges->SetAboveRangeColor(col1[0], col1[1], col1[2], 1);
	m_lutForEdges->UseAboveRangeColorOn();*/

	//NAN values are invisible
	m_lutForEdges->SetNanColor(0,0,0,0);
	
	initializeLegend(tableSize);
}

void iACompCorrelationMap::initializeLegend(int numberOfLabels)
{
	vtkSmartPointer<vtkScalarBarActor> scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
	
	scalarBar->SetLookupTable(m_lutForEdges);
	
	scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	scalarBar->GetPositionCoordinate()->SetValue(0.25, 0.1, 0.0);
	scalarBar->SetWidth(0.4);
	scalarBar->SetHeight(0.80);
	scalarBar->SetUnconstrainedFontSize(1);
	
	//scalarBar->DrawFrameOn();
	//scalarBar->GetFrameProperty()->SetColor(0, 0, 0);
	//scalarBar->GetFrameProperty()->SetLineWidth(3);
	//scalarBar->UseOpacityOn();

	scalarBar->SetTitle("Correlation \n   Coefficient");
	scalarBar->SetNumberOfLabels(numberOfLabels);
	scalarBar->SetTextPositionToSucceedScalarBar();

	//title properties
	scalarBar->GetTitleTextProperty()->BoldOn();
	scalarBar->GetTitleTextProperty()->ItalicOff();
	scalarBar->GetTitleTextProperty()->ShadowOff();
	scalarBar->GetTitleTextProperty()->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	scalarBar->GetTitleTextProperty()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	scalarBar->GetTitleTextProperty()->SetJustificationToLeft();
	scalarBar->GetTitleTextProperty()->SetVerticalJustificationToTop();
	scalarBar->SetVerticalTitleSeparation(20);
	scalarBar->GetTitleTextProperty()->Modified();
	
	//text properties
	vtkSmartPointer<vtkTextProperty> propL = scalarBar->GetLabelTextProperty();
	propL->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	propL->SetColor(0,0,0);
	propL->SetJustificationToLeft();
	propL->SetVerticalJustificationToCentered();
	propL->SetBold(false);
	propL->SetShadow(false);
	propL->Modified();

	// Setup render window, renderer, and interactor
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
	renderer->SetViewport(0.80, 0, 1, 1);
	renderer->SetBackground(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE));
	renderer->AddActor2D(scalarBar);
	m_qvtkWidget->GetRenderWindow()->AddRenderer(renderer);

//	scalarBar->Modified();
//	renderer->ResetCamera();
//	renderWidget();

	//add border
//	int rect[4] = {0, 0, 0, 0};
	
//	scalarBar->GetScalarBarRect(rect, renderer);
	/*vtkSmartPointer<vtkCoordinate> coords = vtkSmartPointer<vtkCoordinate>::New();
	coords->SetCoordinateSystemToViewport();
	coords->SetValue(0.25, 0.1, 0.0);
	double* p1 = coords->GetComputedWorldValue(renderer);
	coords->SetValue(rect[0] + rect[2], rect[1], 0);
	double* p2 = coords->GetComputedWorldValue(renderer);
	coords->SetValue(rect[0] + rect[2], rect[1] + rect[3], 0);
	double* p3 = coords->GetComputedWorldValue(renderer);
	coords->SetValue(rect[0], rect[1] + rect[3], 0);
	double* p4 = coords->GetComputedWorldValue(renderer);*/
	
/*	DEBUG_LOG("xmin = " + QString::number(rect[0]));
	DEBUG_LOG("ymin = " + QString::number(rect[1]));
	DEBUG_LOG("width = " + QString::number(rect[2]));
	DEBUG_LOG("heigth = " + QString::number(rect[3]));

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
*/	/*points->InsertNextPoint(0.25, 0.1, 0.0);
	points->InsertNextPoint(rect[0] + rect[2], rect[1], 0);
	points->InsertNextPoint(rect[0] + rect[2], rect[1] + rect[3], 0);
	points->InsertNextPoint(rect[0], rect[1] + rect[3], 0);*/
/*	points->InsertNextPoint(-0.05, 0.005, 0.0);
	points->InsertNextPoint(0.05, 0.005, 0.0);
	points->InsertNextPoint(0.5, 0.08, 0.0);
	points->InsertNextPoint(-0.05, 0.08, 0.0);

	// Create the polygon
	vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New();
	polygon->GetPointIds()->SetNumberOfIds(4); //make a quad
	polygon->GetPointIds()->SetId(0, 0);
	polygon->GetPointIds()->SetId(1, 1);
	polygon->GetPointIds()->SetId(2, 2);
	polygon->GetPointIds()->SetId(3, 3);

	vtkSmartPointer<vtkCellArray> polygons = vtkSmartPointer<vtkCellArray>::New();
	polygons->InsertNextCell(polygon);

	vtkSmartPointer<vtkPolyData> polygonPolyData = vtkSmartPointer<vtkPolyData>::New();
	polygonPolyData->SetPoints(points);
	polygonPolyData->SetPolys(polygons);
*/
/*	vtkSmartPointer<vtkPolyDataMapper2D> mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
	mapper->SetInputData(polygonPolyData);

	vtkSmartPointer<vtkActor2D> actor = vtkSmartPointer<vtkActor2D>::New();
	actor->SetMapper(mapper);
	actor->GetProperty()->SetColor(0,0,0);
	//actor->GetProperty()->SetEdgeVisibility(true);
	//actor->GetProperty()->SetEdgeColor(0, 0, 0);

	renderer->AddActor2D(actor);
	renderer->ResetCamera();*/

	/*vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
	cubeSource->SetXLength(0.8);
	cubeSource->SetYLength(0.4);*/

	//vtkPolyData* cube = cubeSource->GetOutput();

/*	vtkSmartPointer<vtkOutlineFilter> outline = vtkSmartPointer<vtkOutlineFilter>::New();
	outline->SetInputData(polygonPolyData);

	vtkSmartPointer<vtkPolyDataMapper> outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	outlineMapper->SetInputConnection(outline->GetOutputPort());
	vtkSmartPointer<vtkActor> outlineActor = vtkSmartPointer<vtkActor>::New();
	outlineActor->SetMapper(outlineMapper);
	outlineActor->GetProperty()->SetColor(0, 0, 0);
	//outlineActor->SetPosition(0, 0, 0);

	renderer->AddActor(outlineActor);
	*/
}

void iACompCorrelationMap::initializeArcs()
{
	initializeLutForArcs();

	std::vector<int>* objectsPerDataset = csvFileData::getAmountObjectsEveryDataset(m_dataStorage->getData());

	vtkSmartPointer<vtkPoints> labelPositions = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkPoints> glyphPositions = vtkSmartPointer<vtkPoints>::New();

	vtkSmartPointer<vtkDoubleArray> glyphColors = vtkSmartPointer<vtkDoubleArray>::New();
	glyphColors->SetName("colors");
	glyphColors->SetNumberOfComponents(1);

	vtkSmartPointer<vtkDoubleArray> glyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	glyphScales->SetName("scales");
	glyphScales->SetNumberOfComponents(1);

	double pos[3] = { 1, 0, 0 };
	double theta = 0;
	double phi = 0;
	double angle = 0;
	double sum = 0;
	double p = 0;

	for (int i = 0; i < objectsPerDataset->size(); i++)
	{
		sum += objectsPerDataset->at(i);
	}

	p = 2* m_PI / sum; //theta = [0-pi]
	angle = 360.0 / sum; //360 degree of circle

	for (int i = 0; i < objectsPerDataset->size(); i++)
	{
		//length of arc for this dataset according to its amount of objects
		double arcLength = p * objectsPerDataset->at(i);
		double resAngle = angle * objectsPerDataset->at(i);

		pos[0] = (cos(theta) * cos(phi) * m_radius);
		pos[1] = (sin(theta) * cos(phi) * m_radius);
		pos[2] = (sin(phi) * m_radius);

		double* col = m_lutForArcs->GetTableValue(2);
		drawArc(resAngle, pos, col, 7, false, 0, 0);

		glyphPositions->InsertNextPoint(pos[0], pos[1], pos[2]);
		glyphColors->InsertNextValue(0); //0
		glyphScales->InsertNextValue(0.05);

		//calculate label position
		calculateLabelPosition(labelPositions, theta, arcLength, phi, 1.1);

		theta += arcLength;
	}

	//draw space between arcs
	drawGlyphs(glyphPositions, glyphColors, glyphScales);

	//draw legend text
	QStringList datasetNames = *m_dataStorage->getDatasetNames();
	drawLegend(labelPositions, datasetNames);
}

void iACompCorrelationMap::calculateLabelPosition(vtkSmartPointer<vtkPoints> labelPositions, double theta, double arcLength, double phi, double radiusOffset)
{
	double posLabel[3] = { 0, 0, 0 };
	double posAngle = theta + (arcLength*0.5);

	posLabel[0] = (cos(posAngle) * cos(phi) * (m_radius*radiusOffset));
	posLabel[1] = (sin(posAngle) * cos(phi) * (m_radius*radiusOffset));

	labelPositions->InsertNextPoint(posLabel[0], posLabel[1], posLabel[2]);
}

void iACompCorrelationMap::drawGlyphs(vtkSmartPointer<vtkPoints> positions, vtkSmartPointer<vtkDoubleArray> colors, vtkSmartPointer<vtkDoubleArray> scales)
{
	vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
	cubeSource->Update();

	vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();

	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->SetPoints(positions);
	polydata->GetPointData()->SetScalars(scales);
	polydata->GetPointData()->AddArray(colors);

	glyph3D->SetSourceConnection(cubeSource->GetOutputPort());
	glyph3D->SetInputData(polydata);
	glyph3D->SetScaleModeToScaleByScalar();
	glyph3D->Update();

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(glyph3D->GetOutputPort());
	mapper->SetScalarModeToUsePointFieldData();
	mapper->SelectColorArray(1); //mapper uses color array
	mapper->SetLookupTable(m_lutForArcs);

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);

	m_graphLayoutView->GetRenderer()->AddActor(actor);

	glyphActors->push_back(actor);
}

void iACompCorrelationMap::drawLegend(vtkSmartPointer<vtkPoints> positions, QStringList names)
{
	for(int i = 0; i < positions->GetNumberOfPoints(); i++)
	{
		std::string name = names.at(i).toLocal8Bit().constData();
		name.erase(0, name.find_last_of("/\\") + 1);

		vtkSmartPointer<vtkTextActor> legend = vtkSmartPointer<vtkTextActor>::New();
		legend->SetTextScaleModeToNone();
		legend->SetInput(name.c_str());

		double x = positions->GetPoint(i)[0];
		double y = positions->GetPoint(i)[1];
		
		vtkSmartPointer<vtkTextProperty> legendProperty = legend->GetTextProperty();
		legendProperty->BoldOff();
		legendProperty->ItalicOff();
		legendProperty->ShadowOff();
		legendProperty->SetFontFamilyToArial();
		legendProperty->SetColor(0, 0, 0);
		legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
		legendProperty->SetVerticalJustificationToTop();

		if (x < -0.1)
		{
			legendProperty->SetJustification(VTK_TEXT_RIGHT);
		}
		else if (x > 0.1)
		{
			legendProperty->SetJustification(VTK_TEXT_LEFT);
		}
		else
		{
			legendProperty->SetJustification(VTK_TEXT_CENTERED);
		}

		legendProperty->Modified();
		legend->GetPositionCoordinate()->SetCoordinateSystemToWorld();
		legend->GetPositionCoordinate()->SetValue(x, y);
		legend->Modified();

		m_graphLayoutView->GetRenderer()->AddActor(legend);
		legendActors->push_back(legend);
	}
}

void iACompCorrelationMap::initializeLutForArcs()
{
	int tableSize = 4;

	// light grey to dark grey
	QColor c0 = QColor(255, 255, 255);
	QColor c1 = QColor(189, 189, 189);
	QColor c2 = QColor(115, 115, 115);
	QColor c3 = QColor(37, 37, 37);
	
	//light green to dark green
	/*QColor c0 = QColor(255, 255, 255);
	QColor c1 = QColor(186, 228, 179);
	QColor c2 = QColor(116, 196, 118);
	QColor c3 = QColor(35, 139, 69);*/

	m_lutForArcs->SetNumberOfTableValues(tableSize);
	m_lutForArcs->Build();

	m_lutForArcs->SetTableValue(0, c0.redF(), c0.greenF(), c0.blueF());
	m_lutForArcs->SetTableValue(1, c1.redF(), c1.greenF(), c1.blueF());
	m_lutForArcs->SetTableValue(2, c2.redF(), c2.greenF(), c2.blueF());
	m_lutForArcs->SetTableValue(3, c3.redF(), c3.greenF(), c3.blueF());
}

void iACompCorrelationMap::drawArc(double lengthInDegree, double* startPos, double* color, double lineWidth, bool stippled, int lineStipplePattern, int lineStippleRepeat)
{
	vtkSmartPointer<vtkArcSource> source = vtkSmartPointer<vtkArcSource>::New();
	source->SetUseNormalAndAngle(true);
	source->SetPolarVector(startPos);
	source->SetAngle(lengthInDegree);
	source->SetNormal(0., 0., 1.); // z = 1 or -1
	source->SetResolution(80);

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(source->GetOutputPort());

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper.Get());
	actor->GetProperty()->SetLineWidth(lineWidth);
	actor->GetProperty()->SetColor(color[0], color[1], color[2]);
	actor->GetProperty()->SetEdgeVisibility(true);
	actor->GetProperty()->SetEdgeColor(0,0,0);

	if(stippled)
	{
		stippledLine(actor, lineStipplePattern, lineStippleRepeat);
	}

	m_graphLayoutView->GetRenderer()->AddActor(actor);

	arcActors->push_back(actor);
}

void iACompCorrelationMap::stippledLine(vtkSmartPointer<vtkActor> &actor, int lineStipplePattern, int lineStippleRepeat)
{
	vtkSmartPointer<vtkDoubleArray> tcoords = vtkSmartPointer<vtkDoubleArray>::New();
	vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
	vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();

	// Create texture
	int dimension = 16 * lineStippleRepeat;

	image->SetDimensions(dimension, 1, 1);
	image->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
	image->SetExtent(0, dimension - 1, 0, 0, 0, 0);
	unsigned char  *pixel;
	pixel = static_cast<unsigned char *>(image->GetScalarPointer());
	unsigned char on = 255;
	unsigned char off = 0;
	for (int i = 0; i < 16; ++i)
	{
		unsigned int mask = (1 << i);
		unsigned int bit = (lineStipplePattern & mask) >> i;
		unsigned char value = static_cast<unsigned char>(bit);
		if (value == 0)
		{
			for (int j = 0; j < lineStippleRepeat; ++j)
			{
				*pixel = on;
				*(pixel + 1) = on;
				*(pixel + 2) = on;
				*(pixel + 3) = off;
				pixel += 4;
			}
		}
		else
		{
			for (int j = 0; j < lineStippleRepeat; ++j)
			{
				*pixel = on;
				*(pixel + 1) = on;
				*(pixel + 2) = on;
				*(pixel + 3) = on;
				pixel += 4;
			}
		}
	}
	vtkPolyData *polyData = dynamic_cast<vtkPolyData*>(actor->GetMapper()->GetInput());

	// Create texture coordnates
	tcoords->SetNumberOfComponents(1);
	tcoords->SetNumberOfTuples(polyData->GetNumberOfPoints());
	for (int i = 0; i < polyData->GetNumberOfPoints(); ++i)
	{
		double value = static_cast<double>(i) * .5;
		tcoords->SetTypedTuple(i, &value);
	}

	polyData->GetPointData()->SetTCoords(tcoords);
	texture->SetInputData(image);
	texture->InterpolateOff();
	texture->RepeatOn();

	actor->SetTexture(texture);
}

std::vector<vtkSmartPointer<vtkActor>>* iACompCorrelationMap::getArcActors()
{
	return arcActors;
}

/************************* Update Methods *******************************************/

void iACompCorrelationMap::updateCorrelationMap(std::map<QString, Correlation::CorrelationStore>* correlations, std::map<int, std::vector<double>>* pickStatistic)
{
	removeOldActors();

	//draw new arcs
	updateArcs(pickStatistic);

	//update graph
	updateEdges(correlations);

	m_graphLayout->setCorrelations(correlations);
	m_graphLayoutView->Update();

	renderWidget();
}

void iACompCorrelationMap::removeOldActors()
{
	//remove old arcs
	for (int i = 0; i < arcActors->size(); i++)
	{
		m_graphLayoutView->GetRenderer()->RemoveActor(arcActors->at(i));
	}

	arcActors->clear();

	for (int j = 0; j < legendActors->size(); j++)
	{
		m_graphLayoutView->GetRenderer()->RemoveActor(legendActors->at(j));
	}

	legendActors->clear();

	for (int k = 0; k < glyphActors->size(); k++)
	{
		m_graphLayoutView->GetRenderer()->RemoveActor(glyphActors->at(k));
	}

	glyphActors->clear();
}

void iACompCorrelationMap::updateArcs(std::map<int, std::vector<double>>* pickStatistic)
{
	QStringList allDatasetNames = *m_dataStorage->getDatasetNames();
	QStringList names = QStringList();

	vtkSmartPointer<vtkPoints> labelPositions = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkPoints> glyphPositions = vtkSmartPointer<vtkPoints>::New();

	vtkSmartPointer<vtkDoubleArray> glyphColors = vtkSmartPointer<vtkDoubleArray>::New();
	glyphColors->SetName("colors");
	glyphColors->SetNumberOfComponents(1);

	vtkSmartPointer<vtkDoubleArray> glyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	glyphScales->SetName("scales");
	glyphScales->SetNumberOfComponents(1);

	double pos[3] = { 1, 0, 0 };
	double sum = 0.0;
	double theta = 0.0;
	double phi = 0.0;
	double angle = 0.0;
	double percent = 0.0;
	
	for (std::map<int, std::vector<double>>::const_iterator it = pickStatistic->begin(); it != pickStatistic->end(); ++it)
	{
		std::vector<double> container = it->second;
		sum += container.at(0);
	}

	percent = 2 * m_PI / sum; //theta = [0-pi]
	angle = 360.0 / sum; //360 degree of circle

	for (std::map<int, std::vector<double>>::const_iterator it = pickStatistic->begin(); it != pickStatistic->end(); ++it)
	{
		std::vector<double> container = it->second;

		//length of arc for this dataset according to its amount of objects
		double arcLength = percent * container.at(0);
		double resAngle = angle * container.at(0);

		pos[0] = (cos(theta) * cos(phi) * m_radius);
		pos[1] = (sin(theta) * cos(phi) * m_radius);
		pos[2] = (sin(phi) * m_radius);

		double* col = m_lutForArcs->GetTableValue(2);
		drawArc(resAngle, pos, col, 9, false, 0, 0);

		glyphPositions->InsertNextPoint(pos[0], pos[1], pos[2]);
		glyphColors->InsertNextValue(0);
		glyphScales->InsertNextValue(0.055);

		//calculate label position
		calculateLabelPosition(labelPositions, theta, arcLength, phi, 1.1);
		names.append(allDatasetNames.at(it->first));

		//draw inner arc
		drawInnerArc(container, pos, theta, phi, resAngle, arcLength);

		theta += arcLength;
	}

	drawGlyphs(glyphPositions, glyphColors, glyphScales);

	drawLegend(labelPositions, names);
}

void iACompCorrelationMap::drawInnerArc(std::vector<double> data, double* parentPosition, double parentTheta, double parentPhi, double parentAngle, double parentArcLength)
{
	//draw not selected arc
	double posNotSelected[3] = {0.0, 0.0, parentPosition[2]};
	posNotSelected[0] = (cos(parentTheta) * cos(parentPhi) * (m_radius*0.925));
	posNotSelected[1] = (sin(parentTheta) * cos(parentPhi) * (m_radius*0.925));

	double amountNotPicked = data.at(0) - data.at(1);
	double percentNotPicked = amountNotPicked / data.at(0);

	double resAngle = percentNotPicked * parentAngle;
	double* col = m_lutForArcs->GetTableValue(1);
	drawArc(resAngle, posNotSelected, col, 6, true, 0xDBDB, 20); /////

	//draw selected arc
	double arcLengthToSelectedPoint = parentArcLength * percentNotPicked;
	double theta = parentTheta + arcLengthToSelectedPoint;

	double posSelected[3] = { 0.0, 0.0, parentPosition[2] };
	posSelected[0] = (cos(theta) * cos(parentPhi) * (m_radius*0.925));
	posSelected[1] = (sin(theta) * cos(parentPhi) * (m_radius*0.925));

	double* col1 = m_lutForArcs->GetTableValue(3);
	double angleSelected = parentAngle - resAngle;
	drawArc(angleSelected, posSelected, col1, 7, false, 0, 0);

	//draw glyphs
	vtkSmartPointer<vtkPoints> glyphPositions = vtkSmartPointer<vtkPoints>::New();

	vtkSmartPointer<vtkDoubleArray> glyphColors = vtkSmartPointer<vtkDoubleArray>::New();
	glyphColors->SetName("colors");
	glyphColors->SetNumberOfComponents(1);

	vtkSmartPointer<vtkDoubleArray> glyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	glyphScales->SetName("scales");
	glyphScales->SetNumberOfComponents(1);

	glyphPositions->InsertNextPoint(posNotSelected[0], posNotSelected[1], posNotSelected[2]);
	glyphColors->InsertNextValue(0);
	glyphScales->InsertNextValue(0.055);
	drawGlyphs(glyphPositions,glyphColors, glyphScales);
}

void iACompCorrelationMap::updateEdges(std::map<QString, Correlation::CorrelationStore>* correlations)
{
	vtkDoubleArray* edgeColors = vtkDoubleArray::SafeDownCast(m_graph->GetEdgeData()->GetArray("Color"));
	vtkDoubleArray* weights = vtkDoubleArray::SafeDownCast(m_graph->GetEdgeData()->GetArray("Weights"));

	for (vtkIdType currE = 0; currE < m_graph->GetNumberOfEdges(); currE++)
	{
		vtkIdType startV = m_graph->GetSourceVertex(currE);
		vtkIdType endV = m_graph->GetTargetVertex(currE);

		double colId = colorEdges(startV, endV, correlations, m_vertices);
		edgeColors->SetValue(currE, colId);
		weights->SetValue(currE, colId);
	}

	edgeColors->Modified();
	weights->Modified();
	m_graph->Modified();
}

void iACompCorrelationMap::resetCorrelationMap()
{
	removeOldActors();

	//reset graph
	updateEdges(m_corrCalculation->getCorrelationCoefficients());

	m_graphLayout->setCorrelations(m_corrCalculation->getCorrelationCoefficients());
	m_graphLayoutView->Update();

	//reset arcs
	initializeArcs();

	renderWidget();
}

/************************* INNER CLASS GraphInteractorStyle *******************************************/
iACompCorrelationMap::GraphInteractorStyle::GraphInteractorStyle(): 
	m_actorPicker(vtkSmartPointer<vtkPropPicker>::New()),
	m_Tooltip(vtkSmartPointer<vtkTooltipItem>::New())
{
	m_Tooltip->SetVisible(false);
}

void iACompCorrelationMap::GraphInteractorStyle::OnLeftButtonDown()
{
	//TODO implement tooltip to show percent!!! look in BoxPlotChart & HistogramTableStyle for picking
	vtkInteractorStyleRubberBand2D::OnLeftButtonDown();

/*	bool working = setPickList();
	if (!working) { return; }

	// Get the location of the click (in window coordinates)
	int* pos = this->GetInteractor()->GetEventPosition();
	this->FindPokedRenderer(pos[0], pos[1]);
	auto currentRenderer = this->GetDefaultRenderer();

	int is = m_actorPicker->Pick(pos[0], pos[1], 0, currentRenderer);
	if (is != 0)
	{
		vtkSmartPointer<vtkActor> pickedA = m_actorPicker->GetActor();

		if (pickedA != NULL)
		{

		}
	}
*/
}

bool iACompCorrelationMap::GraphInteractorStyle::setPickList()
{
	m_actorPicker->InitializePickList();

	std::vector<vtkSmartPointer<vtkActor>>* actors = m_baseClass->getArcActors();

	if(actors == nullptr)
	{
		return false;
	}

	for (int i = 0; i < actors->size(); i++)
	{
		m_actorPicker->AddPickList(actors->at(i));
	}

	m_actorPicker->SetPickFromList(true);

	return true;
}

void iACompCorrelationMap::GraphInteractorStyle::OnMiddleButtonDown() { vtkInteractorStyleRubberBand2D::OnMiddleButtonDown(); }
void iACompCorrelationMap::GraphInteractorStyle::OnRightButtonDown() { vtkInteractorStyleRubberBand2D::OnRightButtonDown();  }
void iACompCorrelationMap::GraphInteractorStyle::OnMouseWheelForward() { vtkInteractorStyleRubberBand2D::OnMouseWheelForward(); }
void iACompCorrelationMap::GraphInteractorStyle::OnMouseWheelBackward() { vtkInteractorStyleRubberBand2D::OnMouseWheelBackward(); }
void iACompCorrelationMap::GraphInteractorStyle::OnKeyPress() { vtkInteractorStyleRubberBand2D::OnKeyPress(); }
void iACompCorrelationMap::GraphInteractorStyle::OnKeyRelease() { vtkInteractorStyleRubberBand2D::OnKeyRelease(); }

void iACompCorrelationMap::GraphInteractorStyle::setGraphLayoutView(vtkSmartPointer<vtkGraphLayoutView> graphLayoutView)
{
	m_graphLayoutView = graphLayoutView;
}

void iACompCorrelationMap::GraphInteractorStyle::setBaseClass(iACompCorrelationMap* baseClass)
{
	m_baseClass = baseClass;
}

/************************* INNER CLASS CorrelationGraphLayout *******************************************/
iACompCorrelationMap::CorrelationGraphLayout::CorrelationGraphLayout()
{
	this->RandomSeed = 123;
	this->GraphBounds[0] = this->GraphBounds[2] = this->GraphBounds[4] = -0.5;
	this->GraphBounds[1] = this->GraphBounds[3] = this->GraphBounds[5] = 0.5;
	this->MaxNumberOfIterations = 50;
	this->IterationsPerLayout = 50;
	this->InitialTemperature = 10.0;
	this->CoolDownRate = 10.0;
	this->LayoutComplete = 0;
	this->AutomaticBoundsComputation = false;
	this->ThreeDimensionalLayout = false;
	this->RandomInitialPoints = true;
	this->v = nullptr;
	this->e = nullptr;
}

iACompCorrelationMap::CorrelationGraphLayout::~CorrelationGraphLayout() {
	delete[] this->v;
	delete[] this->e;
}

void iACompCorrelationMap::CorrelationGraphLayout::setVertices(std::map<vtkIdType, QString>* vertices)
{
	m_vertices = vertices;
}

void iACompCorrelationMap::CorrelationGraphLayout::setCorrelations(std::map<QString, Correlation::CorrelationStore>* correlations)
{
	m_correlations = correlations;
}

double iACompCorrelationMap::CorrelationGraphLayout::CoolDown(double t, double r){
	if (t < .01) return .01;
	return t - (t / r);
}

double iACompCorrelationMap::CorrelationGraphLayout::forceAttract(double x, double k, double weight)
{
	double coefficient = std::abs(weight);
	//DEBUG_LOG("coefficient = " + QString::number(coefficient));
	//DEBUG_LOG("old fa = " + QString::number(((x * x) / k)));
	//DEBUG_LOG("fa = " + QString::number(((x * x) / k) * (coefficient / 100)));

	return ((x * x) / k) * coefficient; //(coefficient/100);
}

double iACompCorrelationMap::CorrelationGraphLayout::forceRepulse(double x, double k,  double weight)
{
	double coefficient = 1-std::abs(weight);

	if (x != 0.0)
	{
		//return k * k / x;
		return (k * k / x) * coefficient; //(coefficient/100);
	}
	else
	{
		return VTK_DOUBLE_MAX;
	}
}

double iACompCorrelationMap::CorrelationGraphLayout::calculateHooksLaw(double displacement)
{
	return -K * displacement;
}

// In the future this method should setup data
// structures, etc... so that Layout doesn't have to
// do that every time it's called

void iACompCorrelationMap::CorrelationGraphLayout::Initialize()
{
	vtkPoints* pts = this->Graph->GetPoints();
	vtkIdType numVertices = this->Graph->GetNumberOfVertices();
	vtkIdType numEdges = this->Graph->GetNumberOfEdges();

	// Generate bounds automatically if necessary. It's the same
	// as the input bounds.
	if (this->AutomaticBoundsComputation)
	{
		pts->GetBounds(this->GraphBounds);
	}

	for (int i = 0; i < 3; i++)
	{
		if (this->GraphBounds[2 * i + 1] <= this->GraphBounds[2 * i])
		{
			this->GraphBounds[2 * i + 1] = this->GraphBounds[2 * i] + 1;
		}
	}

	delete[] this->v;
	delete[] this->e;
	this->v = new vtkLayoutVertex[numVertices];
	this->e = new vtkLayoutEdge[numEdges];

	int maxCoord = this->ThreeDimensionalLayout ? 3 : 2;

	// Get the points, either x,y,0 or x,y,z or random
	if (this->RandomInitialPoints)
	{
		vtkMath::RandomSeed(this->RandomSeed);

		for (vtkIdType i = 0; i < numVertices; i++)
		{
			for (int j = 0; j < maxCoord; j++)
			{
				double r = vtkMath::Random();
				v[i].x[j] = (this->GraphBounds[2 * j + 1] - this->GraphBounds[2 * j])*r + this->GraphBounds[2 * j];
			}
			if (!this->ThreeDimensionalLayout)
			{
				v[i].x[2] = 0;
			}
		}
	}
	else
	{
		for (vtkIdType i = 0; i < numVertices; i++)
		{
			pts->GetPoint(i, v[i].x);
			if (!this->ThreeDimensionalLayout)
			{
				v[i].x[2] = 0;
			}
		}
	}

	// Get the edges
	vtkSmartPointer<vtkEdgeListIterator> edges = vtkSmartPointer<vtkEdgeListIterator>::New();
	this->Graph->GetEdges(edges);
	while (edges->HasNext())
	{
		vtkEdgeType edge = edges->Next();
		//cerr << edge.Id << ": " << edge.Source << "," << edge.Target << endl;
		e[edge.Id].t = edge.Source;
		e[edge.Id].u = edge.Target;
	}

	//cerr << endl << endl;
	//vtkSmartPointer<vtkOutEdgeIterator> it =
	//  vtkSmartPointer<vtkOutEdgeIterator>::New();
	//for (vtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); ++i)
	//  {
	//  this->Graph->GetOutEdges(i, it);
	//  cerr << i << ": ";
	//  while (it->HasNext())
	//    {
	//    vtkOutEdgeType edge = it->Next();
	//    cerr << edge.Target << " ";
	//    }
	//  cerr << endl;
	//  }

	//DEBUG_LOG("GraphBounds = " + QString::number(this->GraphBounds[0]) + ", " + QString::number(this->GraphBounds[1]) + "\n" +
	//	QString::number(this->GraphBounds[2]) + ", " + QString::number(this->GraphBounds[3]) + "\n" +
	//	QString::number(this->GraphBounds[4]) + ", " + QString::number(this->GraphBounds[5]) );

	// More variable definitions
	double volume = (this->GraphBounds[1] - this->GraphBounds[0]) *
		(this->GraphBounds[3] - this->GraphBounds[2]) *
		(this->GraphBounds[5] - this->GraphBounds[4]);

	//DEBUG_LOG("volume = " + QString::number(volume));

	this->Temp = sqrt((this->GraphBounds[1] - this->GraphBounds[0])*
		(this->GraphBounds[1] - this->GraphBounds[0]) +
		(this->GraphBounds[3] - this->GraphBounds[2])*
		(this->GraphBounds[3] - this->GraphBounds[2]) +
		(this->GraphBounds[5] - this->GraphBounds[4])*
		(this->GraphBounds[5] - this->GraphBounds[4]));

	if (this->InitialTemperature > 0)
	{
		this->Temp = this->InitialTemperature;
	}

	//DEBUG_LOG("Temp = " + QString::number(volume));

	// The optimal distance between vertices.
	//this->optDist = pow(volume / numVertices, 0.33333);
	this->optDist = pow(volume / numVertices, 0.33333);
	this->minDist = optDist;//1 * std::sqrt(volume / numVertices); //pow(volume / numVertices, 2);
	this->maxDist = 2*optDist;//3 * std::sqrt(volume / numVertices); //pow(volume / numVertices, 0.1);

	//DEBUG_LOG("optDist = " + QString::number(optDist));
	//DEBUG_LOG("maxDist = " + QString::number(maxDist));
	//DEBUG_LOG("minDist = " + QString::number(minDist));

	// Set some vars
	this->TotalIterations = 0;
	this->LayoutComplete = 0;

};

/*
void iACompCorrelationMap::CorrelationGraphLayout::Initialize()
{
	vtkPoints* pts = this->Graph->GetPoints();
	vtkIdType numVertices = this->Graph->GetNumberOfVertices();
	vtkIdType numEdges = this->Graph->GetNumberOfEdges();

	// Generate bounds automatically if necessary. It's the same
	// as the input bounds.
	if (this->AutomaticBoundsComputation)
	{
		pts->GetBounds(this->GraphBounds);
	}

	for (int i = 0; i < 3; i++)
	{
		if (this->GraphBounds[2 * i + 1] <= this->GraphBounds[2 * i])
		{
			this->GraphBounds[2 * i + 1] = this->GraphBounds[2 * i] + 1;
		}
	}

	delete[] this->v;
	delete[] this->e;
	this->v = new vtkLayoutVertex[numVertices];
	this->e = new vtkLayoutEdge[numEdges];

	int maxCoord = this->ThreeDimensionalLayout ? 3 : 2;

	// Get the points, either x,y,0 or x,y,z or random
	if (this->RandomInitialPoints)
	{
		vtkMath::RandomSeed(this->RandomSeed);

		for (vtkIdType i = 0; i < numVertices; i++)
		{
			for (int j = 0; j < maxCoord; j++)
			{
				double r = vtkMath::Random();
				//v[i].x[j] = (this->GraphBounds[2 * j + 1] - this->GraphBounds[2 * j])*r + this->GraphBounds[2 * j];
				v[i].x[j] = 0;
			}
			if (!this->ThreeDimensionalLayout)
			{
				v[i].x[2] = 0;
			}
		}
	}
	else
	{
		for (vtkIdType i = 0; i < numVertices; i++)
		{
			pts->GetPoint(i, v[i].x);
			if (!this->ThreeDimensionalLayout)
			{
				v[i].x[2] = 0;
			}
		}
	}

	// Get the edges
	vtkSmartPointer<vtkEdgeListIterator> edges = vtkSmartPointer<vtkEdgeListIterator>::New();
	this->Graph->GetEdges(edges);
	while (edges->HasNext())
	{
		vtkEdgeType edge = edges->Next();
		//cerr << edge.Id << ": " << edge.Source << "," << edge.Target << endl;
		e[edge.Id].t = edge.Source;
		e[edge.Id].u = edge.Target;
	}

	//cerr << endl << endl;
	//vtkSmartPointer<vtkOutEdgeIterator> it =
	//  vtkSmartPointer<vtkOutEdgeIterator>::New();
	//for (vtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); ++i)
	//  {
	//  this->Graph->GetOutEdges(i, it);
	//  cerr << i << ": ";
	//  while (it->HasNext())
	//    {
	//    vtkOutEdgeType edge = it->Next();
	//    cerr << edge.Target << " ";
	//    }
	//  cerr << endl;
	//  }

	DEBUG_LOG("GraphBounds = " + QString::number(this->GraphBounds[0]) + ", " + QString::number(this->GraphBounds[1]) + "\n" +
		QString::number(this->GraphBounds[2]) + ", " + QString::number(this->GraphBounds[3]) + "\n" +
		QString::number(this->GraphBounds[4]) + ", " + QString::number(this->GraphBounds[5]));

	// More variable definitions
	double volume = (this->GraphBounds[1] - this->GraphBounds[0]) *
		(this->GraphBounds[3] - this->GraphBounds[2]) *
		(this->GraphBounds[5] - this->GraphBounds[4]);

	DEBUG_LOG("volume = " + QString::number(volume));

	this->Temp = sqrt((this->GraphBounds[1] - this->GraphBounds[0])*
		(this->GraphBounds[1] - this->GraphBounds[0]) +
		(this->GraphBounds[3] - this->GraphBounds[2])*
		(this->GraphBounds[3] - this->GraphBounds[2]) +
		(this->GraphBounds[5] - this->GraphBounds[4])*
		(this->GraphBounds[5] - this->GraphBounds[4]));

	if (this->InitialTemperature > 0)
	{
		this->Temp = this->InitialTemperature;
	}

	DEBUG_LOG("Temp = " + QString::number(volume));

	// The optimal distance between vertices.
	//this->optDist = pow(volume / numVertices, 0.33333);
	this->optDist = pow(volume / numVertices, 0.33333);
	this->minDist = optDist; //1 * std::sqrt(volume / numVertices);//pow(volume / numVertices, 2);
	this->maxDist = optDist; //3 * std::sqrt(volume / numVertices);//pow(volume / numVertices, 0.1);

	DEBUG_LOG("optDist = " + QString::number(optDist));
	DEBUG_LOG("maxDist = " + QString::number(maxDist));
	DEBUG_LOG("minDist = " + QString::number(minDist));

	// Set some vars
	this->TotalIterations = 0;
	this->LayoutComplete = 0;

};
*/

// ForceDirected graph layout method

void iACompCorrelationMap::CorrelationGraphLayout::Layout()
{
	vtkIdType numVertices = this->Graph->GetNumberOfVertices();
	vtkIdType numEdges = this->Graph->GetNumberOfEdges();

	// Begin iterations.
	double norm, fr, fa, minimum;
	double diff[3];
	//DEBUG_LOG("IterationsPerLayout: " + QString::number(IterationsPerLayout));
	//DEBUG_LOG("intial Temp = " + QString::number(Temp));
	for (int i = 0; i < this->IterationsPerLayout; i++)
	{
		// Calculate the repulsive forces.
		for (vtkIdType j = 0; j < numVertices; j++)
		{ //for each vertex calculate repulsive force

			//set displacement
			v[j].d[0] = 0.0;
			v[j].d[1] = 0.0;
			v[j].d[2] = 0.0;

			for (vtkIdType l = 0; l < numVertices; l++)
			{ //for each vertex
				if (j != l)
				{
					//get difference in position
					diff[0] = v[j].x[0] - v[l].x[0];
					diff[1] = v[j].x[1] - v[l].x[1];
					diff[2] = v[j].x[2] - v[l].x[2];
					norm = vtkMath::Normalize(diff);
				
					//DEBUG_LOG("norm: " + QString::number(norm));

					if (norm > 2 * maxDist)
					{
						fr = 0;
					}
					else
					{
						fr = forceRepulse(norm, maxDist, 0.0);
					}

					//set displacement
					v[j].d[0] += diff[0] * fr;
					v[j].d[1] += diff[1] * fr;
					v[j].d[2] += diff[2] * fr;
				}
			}
		}

		// Calculate the attractive forces.
		for (vtkIdType j = 0; j < numEdges; j++)
		{//for each edge calculate attractive force

			//vertex j and its neighbors
			diff[0] = v[e[j].u].x[0] - v[e[j].t].x[0];
			diff[1] = v[e[j].u].x[1] - v[e[j].t].x[1];
			diff[2] = v[e[j].u].x[2] - v[e[j].t].x[2];
			norm = vtkMath::Normalize(diff);

			double weight = getWeightForEdge(e[j].u, e[j].t);
			//new
			if (norm < minDist)
			{
				fa = 0;
			}
			else
			{
				fa = forceAttract(norm, minDist, weight);
			}

			v[e[j].u].d[0] -= diff[0] * fa;
			v[e[j].u].d[1] -= diff[1] * fa;
			v[e[j].u].d[2] -= diff[2] * fa;

			v[e[j].t].d[0] += diff[0] * fa;
			v[e[j].t].d[1] += diff[1] * fa;
			v[e[j].t].d[2] += diff[2] * fa;

			//calculate repulsive force
			/*diff[0] = v[e[j].u].x[0] - v[e[j].t].x[0];
			diff[1] = v[e[j].u].x[1] - v[e[j].t].x[1];
			diff[2] = v[e[j].u].x[2] - v[e[j].t].x[2];
			norm = vtkMath::Normalize(diff);

			if (norm > 2 * maxDist)
			{
				fr = 0;
			}
			else
			{
				fr = forceRepulse(norm, maxDist, weight);
			}

			//set displacement		
			v[e[j].u].d[0] -= diff[0] * fr;
			v[e[j].u].d[1] -= diff[1] * fr;
			v[e[j].u].d[2] -= diff[2] * fr;

			v[e[j].t].d[0] += diff[0] * fr;
			v[e[j].t].d[1] += diff[1] * fr;
			v[e[j].t].d[2] += diff[2] * fr;*/
		}

		// Combine the forces for a new configuration
		for (vtkIdType j = 0; j < numVertices; j++)
		{
			norm = vtkMath::Normalize(v[j].d);
			minimum = (norm < this->Temp ? norm : this->Temp);
			v[j].x[0] += v[j].d[0] * minimum;
			v[j].x[1] += v[j].d[1] * minimum;
			v[j].x[2] += v[j].d[2] * minimum;
		}

		// Reduce temperature as layout approaches a better configuration.
		this->Temp = CoolDown(this->Temp, this->CoolDownRate);
	}

	// Get the bounds of the graph and scale and translate to
	// bring them within the bounds specified.
	vtkPoints *newPts = vtkPoints::New();
	newPts->SetNumberOfPoints(numVertices);
	for (vtkIdType i = 0; i < numVertices; i++)
	{
		newPts->SetPoint(i, v[i].x);
	}
	double bounds[6], sf[3], x[3], xNew[3];
	double center[3], graphCenter[3];
	double len;
	newPts->GetBounds(bounds);
	for (int i = 0; i < 3; i++)
	{
		if ((len = (bounds[2 * i + 1] - bounds[2 * i])) == 0.0)
		{
			len = 1.0;
		}
		sf[i] = (this->GraphBounds[2 * i + 1] - this->GraphBounds[2 * i]) / len;
		center[i] = (bounds[2 * i + 1] + bounds[2 * i]) / 2.0;
		graphCenter[i] = (this->GraphBounds[2 * i + 1] + this->GraphBounds[2 * i]) / 2.0;
	}

	double scale = sf[0];
	scale = (scale < sf[1] ? scale : sf[1]);
	scale = (scale < sf[2] ? scale : sf[2]);

	for (vtkIdType i = 0; i < numVertices; i++)
	{
		newPts->GetPoint(i, x);
		for (int j = 0; j < 3; j++)
		{
			xNew[j] = graphCenter[j] + scale * (x[j] - center[j]);
		}
		newPts->SetPoint(i, xNew);
	}

	// Send the data to output.
	this->Graph->SetPoints(newPts);

	// Clean up.
	newPts->Delete();


	// Check for completion of layout
	this->TotalIterations += this->IterationsPerLayout;
	if (this->TotalIterations >= this->MaxNumberOfIterations)
	{
		// I'm done
		this->LayoutComplete = 1;
	}
}

double iACompCorrelationMap::CorrelationGraphLayout::getWeightForEdge(vtkIdType startV, vtkIdType endV)
{
	//find name of startVertex
	std::map<vtkIdType, QString >::const_iterator vertexIter = m_vertices->find(startV);
	if (vertexIter == m_vertices->end()) return 0.0;
	QString nameStartV = vertexIter->second;

	//find name of endVertex
	std::map<vtkIdType, QString >::const_iterator vertexIter1 = m_vertices->find(endV);
	if (vertexIter1 == m_vertices->end()) return 0.0;
	QString nameEndV = vertexIter1->second;

	//find correlation value of both
	std::map<QString, Correlation::CorrelationStore>::const_iterator pos = m_correlations->find(nameStartV);
	if (pos == m_correlations->end()) return 0.0;
	Correlation::CorrelationStore map = pos->second;

	Correlation::CorrelationStore::const_iterator pos1 = map.find(nameEndV);
	if (pos1 == map.end()) return 0.0;

	double weight = pos1->second;
	if(std::isnan(weight))
	{
		return 0.0;
	}

	return weight;
}

//TODO implement !!!!!!!!!!!!!!!!!

//algorithm with velocity & acceleration
/*
void iACompCorrelationMap::CorrelationGraphLayout::Layout()
{
	vtkIdType numVertices = this->Graph->GetNumberOfVertices();
	vtkIdType numEdges = this->Graph->GetNumberOfEdges();

	// Begin iterations.
	double norm, fr, fa, minimum;
	double diff[3];

	double velocity = 0;
	double oldTemp = this->Temp;
	double displacement;
	DEBUG_LOG("IterationsPerLayout: " + QString::number(IterationsPerLayout));
	DEBUG_LOG("intial Temp = " + QString::number(Temp));
	for (int i = 0; i < this->IterationsPerLayout; i++)
	{
		// Calculate the repulsive forces.
		/*for (vtkIdType j = 0; j < numVertices; j++)
		{ //for each vertex calculate repulsive force

			//set displacement
			v[j].d[0] = 0.0;
			v[j].d[1] = 0.0;
			v[j].d[2] = 0.0;

			for (vtkIdType l = 0; l < numVertices; l++)
			{ //for each vertex
				if (j != l)
				{
					//get difference in position
					diff[0] = v[j].x[0] - v[l].x[0];
					diff[1] = v[j].x[1] - v[l].x[1];
					diff[2] = v[j].x[2] - v[l].x[2];
					norm = vtkMath::Normalize(diff);

					//DEBUG_LOG("norm: " + QString::number(norm));

					if (norm > 2 * optDist)
					{
						fr = 0;
					}
					else
					{
						fr = forceRepulse(norm, optDist);
					}

					//set displacement
					v[j].d[0] += diff[0] * fr;
					v[j].d[1] += diff[1] * fr;
					v[j].d[2] += diff[2] * fr;
				}
			}
		}*/

		// Calculate the attractive forces.
		/*for (vtkIdType j = 0; j < numEdges; j++)
		{//for each edge calculate attractive force

			//vertex j and its neighbors
			diff[0] = v[e[j].u].x[0] - v[e[j].t].x[0];
			diff[1] = v[e[j].u].x[1] - v[e[j].t].x[1];
			diff[2] = v[e[j].u].x[2] - v[e[j].t].x[2];
			norm = vtkMath::Normalize(diff);

			v[e[j].u].d[0] -= diff[0] * fa;
			v[e[j].u].d[1] -= diff[1] * fa;
			v[e[j].u].d[2] -= diff[2] * fa;

			v[e[j].t].d[0] += diff[0] * fa;
			v[e[j].t].d[1] += diff[1] * fa;
			v[e[j].t].d[2] += diff[2] * fa;
		}*/

		// Combine the forces for a new configuration
		/*for (vtkIdType j = 0; j < numVertices; j++)
		{
			norm = vtkMath::Normalize(v[j].d);
			minimum = (norm < this->Temp ? norm : this->Temp);
			v[j].x[0] += v[j].d[0] * minimum;
			v[j].x[1] += v[j].d[1] * minimum;
			v[j].x[2] += v[j].d[2] * minimum;
		}*/

		// Reduce temperature as layout approaches a better configuration.
/*		this->Temp = CoolDown(this->Temp, this->CoolDownRate);

		double timeStep = oldTemp - this->Temp;//0.5; //
		DEBUG_LOG("");
		DEBUG_LOG("++++++++++++++++++++++++++++++++++++++++");
		DEBUG_LOG("timeStep = " + QString::number(timeStep));
		for (vtkIdType j = 0; j < numEdges; j++)
		{
			if (j == 0 || j == 1)
			{
				DEBUG_LOG("j = " + QString::number(j) );
				DEBUG_LOG("Pos = " + QString::number(v[e[j].u].x[0]) + ", " + QString::number(v[e[j].u].x[1]) + ", " + QString::number(v[e[j].u].x[2]));
			}

			double weight = getWeightForEdge(e[j].u, e[j].t);
			
			diff[0] = v[e[j].u].x[0] - v[e[j].t].x[0];
			diff[1] = v[e[j].u].x[1] - v[e[j].t].x[1];
			diff[2] = v[e[j].u].x[2] - v[e[j].t].x[2];
			//displacement = vtkMath::Normalize(diff) * (std::abs((std::abs(weight)-1))/100);
			displacement = (std::abs((std::abs(weight) - 1)) / 100);
			
			fa = calculateHooksLaw(displacement);
			
			double acc = fa / MASS;
			velocity = velocity + (acc * timeStep);

			/*v[e[j].u].x[0] = v[e[j].u].x[0] + diff[0] * (velocity * timeStep);
			v[e[j].u].x[1] = v[e[j].u].x[1] + diff[1] * (velocity * timeStep);
			v[e[j].u].x[2] = v[e[j].u].x[2] + diff[2] * (velocity * timeStep);*/
/*			v[e[j].u].x[0] = v[e[j].u].x[0] + (velocity * timeStep);
			v[e[j].u].x[1] = v[e[j].u].x[1] + (velocity * timeStep);
			v[e[j].u].x[2] = v[e[j].u].x[2] + (velocity * timeStep);

			if(j == 0 || j == 1)
			{
				DEBUG_LOG("weight = " + QString::number(weight));
				DEBUG_LOG("displacement = " + QString::number(displacement));
				DEBUG_LOG("fa = " + QString::number(fa));
				DEBUG_LOG("acc = " + QString::number(acc));
				DEBUG_LOG("velocity = " + QString::number(velocity));
				DEBUG_LOG("new Pos = " + QString::number(v[e[j].u].x[0]) + ", " + QString::number(v[e[j].u].x[1]) + ", " + QString::number(v[e[j].u].x[2]));
			}
		}
		DEBUG_LOG("");
		DEBUG_LOG("++++++++++++++++++++++++++++++++++++++++");
		oldTemp = this->Temp;
	}

	// Get the bounds of the graph and scale and translate to
	// bring them within the bounds specified.
	vtkPoints *newPts = vtkPoints::New();
	newPts->SetNumberOfPoints(numVertices);
	for (vtkIdType i = 0; i < numVertices; i++)
	{
		newPts->SetPoint(i, v[i].x);
	}
	double bounds[6], sf[3], x[3], xNew[3];
	double center[3], graphCenter[3];
	double len;
	newPts->GetBounds(bounds);
	for (int i = 0; i < 3; i++)
	{
		if ((len = (bounds[2 * i + 1] - bounds[2 * i])) == 0.0)
		{
			len = 1.0;
		}
		sf[i] = (this->GraphBounds[2 * i + 1] - this->GraphBounds[2 * i]) / len;
		center[i] = (bounds[2 * i + 1] + bounds[2 * i]) / 2.0;
		graphCenter[i] = (this->GraphBounds[2 * i + 1] + this->GraphBounds[2 * i]) / 2.0;
	}

	double scale = sf[0];
	scale = (scale < sf[1] ? scale : sf[1]);
	scale = (scale < sf[2] ? scale : sf[2]);

	for (vtkIdType i = 0; i < numVertices; i++)
	{
		newPts->GetPoint(i, x);
		for (int j = 0; j < 3; j++)
		{
			xNew[j] = graphCenter[j] + scale * (x[j] - center[j]);
		}
		newPts->SetPoint(i, xNew);
	}

	// Send the data to output.
	this->Graph->SetPoints(newPts);

	// Clean up.
	newPts->Delete();


	// Check for completion of layout
	this->TotalIterations += this->IterationsPerLayout;
	if (this->TotalIterations >= this->MaxNumberOfIterations)
	{
		// I'm done
		this->LayoutComplete = 1;
	}
}
*/