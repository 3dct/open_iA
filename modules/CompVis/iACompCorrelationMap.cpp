#include "iACompCorrelationMap.h"
#include <vtkObjectFactory.h> //for macro!

//CompVis
#include "iACompVisOptions.h"
#include "iACsvDataStorage.h"
#include "iACompVisMain.h"

//iA
#include "iAMainWindow.h"
#include "iAVtkVersion.h"

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
#include "vtkAlgorithmOutput.h"

#include "vtkLegendBoxActor.h"
#include "vtkLineSource.h"
#include "vtkCubeSource.h"

#include "vtkHoverWidget.h"
#include "vtkBalloonRepresentation.h"
#include "vtkBalloonWidget.h"

#include "vtkScalarBarWidget.h"
#include "vtkPolygon.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkOutlineFilter.h"
#include "vtkCubeSource.h"
#include "vtkProperty2D.h"

#include "vtkForceDirectedLayoutStrategy.h"

#include <cmath>


//testing
#include <vtkNamedColors.h>
#include <vtkCubeSource.h>
#include <vtkSphereSource.h>

vtkStandardNewMacro(iACompCorrelationMap::CorrelationGraphLayout);
vtkStandardNewMacro(iACompCorrelationMap::GraphInteractorStyle);

iACompCorrelationMap::iACompCorrelationMap(iAMainWindow* parent, iACorrelationCoefficient* corrCalculation, iACsvDataStorage* dataStorage, iACompVisMain* main) :
	QDockWidget(parent),
	m_corrCalculation(corrCalculation),
	m_main(main),
	m_dataStorage(dataStorage),
	m_renderer(vtkSmartPointer<vtkRenderer>::New()),
	m_graphLayoutView(vtkSmartPointer<vtkGraphLayoutView>::New()),
	m_graph(vtkSmartPointer<vtkMutableUndirectedGraph>::New()),
	m_lutForEdges(vtkSmartPointer<vtkLookupTable>::New()),
	m_lutForVertices(vtkSmartPointer<vtkLookupTable>::New()),
	m_lutForArcs(vtkSmartPointer<vtkLookupTable>::New()),
	m_theme(vtkSmartPointer<vtkViewTheme>::New()),
	m_vertices(new std::map<vtkIdType, QString>()),
	arcActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	legendActors(new std::vector<vtkSmartPointer<vtkTextActor>>()),
	glyphActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	arcPercentPair(new std::map<vtkSmartPointer<vtkActor>, double>()),
	outerArcWithInnerArcs(new std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>>()),
	outerArcWithLegend(new std::map<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkTextActor>>()),
	m_arcDataIndxTypePair(new std::map< vtkSmartPointer<vtkActor>, std::map<int, double>*>())
{
	setupUi(this);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	m_qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(m_qvtkWidget);

	m_renderer->SetUseFXAA(true);

	#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_qvtkWidget->GetRenderWindow()->AddRenderer(m_renderer);
		m_graphLayoutView->SetRenderWindow(m_qvtkWidget->GetRenderWindow());
		m_graphLayoutView->SetInteractor(m_qvtkWidget->GetInteractor());
	#else
		m_qvtkWidget->renderWindow()->AddRenderer(m_renderer);
		m_graphLayoutView->SetRenderWindow(m_qvtkWidget->renderWindow());
		m_graphLayoutView->SetInteractor(m_qvtkWidget->interactor());
	#endif

	style = vtkSmartPointer<GraphInteractorStyle>::New();
	style->setGraphLayoutView(m_graphLayoutView);
	style->setBaseClass(this);
	m_graphLayoutView->GetInteractor()->SetInteractorStyle(style);

	m_graphLayoutView->GetRenderer()->SetUseFXAA(true);
	double col1[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE, col1);
	double col2[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE, col2);
	m_theme->SetBackgroundColor(col1);
	m_theme->SetBackgroundColor2(col2);

	m_attrNames = *m_dataStorage->getAttributeNamesWithoutLabel();
	m_numberOfAttr = m_attrNames.size(); //amount of attributes

	initializeCorrelationMap();
	initializeArcs();
	initializeArcLegend();

	m_graphLayoutView->ResetCamera();
}

void iACompCorrelationMap::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);

	renderWidget();
}

void iACompCorrelationMap::reinitializeCorrelationMap(iACorrelationCoefficient* newCorrCalculation)
{
	m_corrCalculation = newCorrCalculation;

	
	m_graphLayoutView = vtkSmartPointer<vtkGraphLayoutView>::New();
	m_graph = vtkSmartPointer<vtkMutableUndirectedGraph>::New();
	m_lutForEdges = vtkSmartPointer<vtkLookupTable>::New();
	m_lutForVertices = vtkSmartPointer<vtkLookupTable>::New(); 
	m_lutForArcs = vtkSmartPointer<vtkLookupTable>::New();
	m_theme = vtkSmartPointer<vtkViewTheme>::New();

	m_vertices->clear();
	delete m_vertices;
	m_vertices = new std::map<vtkIdType, QString>();

	arcActors->clear();
	delete arcActors;
	arcActors = new std::vector<vtkSmartPointer<vtkActor>>();

	legendActors->clear();
	delete legendActors;
	legendActors = new std::vector<vtkSmartPointer<vtkTextActor>>();

	glyphActors->clear();
	delete glyphActors;
	glyphActors = new std::vector<vtkSmartPointer<vtkActor>>();

	arcPercentPair->clear();
	delete arcPercentPair;
	arcPercentPair = new std::map<vtkSmartPointer<vtkActor>, double>();

	outerArcWithInnerArcs->clear();
	delete outerArcWithInnerArcs;
	outerArcWithInnerArcs = new std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>>();
	
	outerArcWithLegend->clear();
	delete outerArcWithLegend;
	outerArcWithLegend = new std::map<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkTextActor>>();

	m_arcDataIndxTypePair->clear();
	delete m_arcDataIndxTypePair;
	m_arcDataIndxTypePair = new std::map< vtkSmartPointer<vtkActor>, std::map<int, double>*>();

	#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_qvtkWidget->GetRenderWindow()->RemoveRenderer(m_renderer);
	#else
		m_qvtkWidget->renderWindow()->RemoveRenderer(m_renderer);
	#endif

	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	m_renderer->SetUseFXAA(true);

	#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_qvtkWidget->GetRenderWindow()->AddRenderer(m_renderer);
		m_graphLayoutView->SetRenderWindow(m_qvtkWidget->GetRenderWindow());
		m_graphLayoutView->SetInteractor(m_qvtkWidget->GetInteractor());
	#else
		m_qvtkWidget->renderWindow()->AddRenderer(m_renderer);
		m_graphLayoutView->SetRenderWindow(m_qvtkWidget->renderWindow());
		m_graphLayoutView->SetInteractor(m_qvtkWidget->interactor());
	#endif

	style = vtkSmartPointer<GraphInteractorStyle>::New();
	style->setGraphLayoutView(m_graphLayoutView);
	style->setBaseClass(this);
	m_graphLayoutView->GetInteractor()->SetInteractorStyle(style);

	double col1[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE, col1);
	double col2[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE, col2);
	m_theme->SetBackgroundColor(col1);
	m_theme->SetBackgroundColor2(col2);

	//data preparation
	m_attrNames = *m_dataStorage->getAttributeNamesWithoutLabel();
	m_numberOfAttr = m_attrNames.size(); //amount of attributes
	
	initializeCorrelationMap();
	initializeArcs();
	initializeArcLegend();

	m_graphLayoutView->ResetCamera();
	renderWidget();
}

void iACompCorrelationMap::renderWidget()
{
	#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
	#else
		m_qvtkWidget->renderWindow()->GetInteractor()->Render();
	#endif
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
	m_theme->SetScaleCellLookupTable(false); //necessary, otherwise coloring of edges not correct!

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
			weights->InsertNextValue(colorInd);
		}
	}

	m_graph->GetEdgeData()->AddArray(edgeColors);
	m_graphLayoutView->SetEdgeColorArrayName("Color");
	m_graphLayoutView->ColorEdgesOn();

	//display hover text
	m_graph->GetEdgeData()->AddArray(weights);
	m_graphLayoutView->SetEdgeLabelArrayName("Weights");
	
	vtkRenderedGraphRepresentation* representation = dynamic_cast<vtkRenderedGraphRepresentation*>(m_graphLayoutView->GetRepresentation());
	representation->SetEdgeHoverArrayName("Weights");
	m_graphLayoutView->DisplayHoverTextOn();

	representation->SetEdgeScalarBarVisibility(true);
	vtkScalarBarWidget* widget = representation->GetEdgeScalarBar();
	initializeLegend(widget);

	//set line width
	m_theme->SetLineWidth(iACompVisOptions::LINE_WIDTH);
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

	//LOG(lvlDebug,nameStartV  + " to " + nameEndV + " with: " + QString::number(pos1->second));
	
	return pos1->second;	
}

void iACompCorrelationMap::initializeLutForEdges()
{
	int tableSize = 7;

	// blue to white to red
	QColor c1 = QColor(33, 102, 172); //blue
	QColor c2 = QColor(103, 169, 207);
	QColor c3 = QColor(209, 229, 240 );
	QColor c4 = QColor(255, 255, 255);
	QColor c5 = QColor(253, 219, 199);
	QColor c6 = QColor(239, 138, 98);
	QColor c7 = QColor(178, 24, 43); //red

	//green to white to yellow to red
	/*QColor c7 = QColor(26, 152, 80);
	QColor c6 = QColor(145, 207, 96);
	QColor c5 = QColor(217, 239, 139);
	QColor c4 = QColor(255, 255, 255);
	QColor c3 = QColor(254, 224, 139);
	QColor c2 = QColor(252, 141, 89);
	QColor c1 = QColor(215, 48, 39);*/

	m_lutForEdges->SetNumberOfTableValues(tableSize);
	m_lutForEdges->Build();

	m_lutForEdges->SetTableValue(0, c1.redF(), c1.greenF(), c1.blueF(), 1); 
	m_lutForEdges->SetTableValue(1, c2.redF(), c2.greenF(), c2.blueF(), 1); 
	m_lutForEdges->SetTableValue(2, c3.redF(), c3.greenF(), c3.blueF(), 1); 
	m_lutForEdges->SetTableValue(3, c4.redF(), c4.greenF(), c4.blueF(), 0); //transparency ON
	m_lutForEdges->SetTableValue(4, c5.redF(), c5.greenF(), c5.blueF(), 1); 
	m_lutForEdges->SetTableValue(5, c6.redF(), c6.greenF(), c6.blueF(), 1); 
	m_lutForEdges->SetTableValue(6, c7.redF(), c7.greenF(), c7.blueF(), 1); 

	//initialize annotation
	int startVal = -1.0;
	double binRangeLength = 2.0 / tableSize;
	for (size_t i = 0; i < tableSize; i++)
	{
		//make format of annotations
		double low = iACompVisOptions::round_up(startVal + (i * binRangeLength), 2);
		double high = iACompVisOptions::round_up(startVal + ((i + 1) * binRangeLength), 2);

		std::string sLow = iACompVisOptions::cutStringAfterNDecimal(std::to_string(low),2);
		std::string sHigh = iACompVisOptions::cutStringAfterNDecimal(std::to_string(high), 2);

		//position description in the middle of each color bar in the scalarBar legend
		m_lutForEdges->SetAnnotation(low + ((high - low)*0.5), sLow + " to " + sHigh);
	}

	m_lutForEdges->SetTableRange(-1.0, 1.0);

	//NAN values are invisible
	m_lutForEdges->SetNanColor(0,0,0,0);
}

void iACompCorrelationMap::initializeLegend(vtkScalarBarWidget* widget)
{
	vtkSmartPointer<vtkScalarBarActor> scalarBar = widget->GetScalarBarActor();

	double width = 0.15;
	double height = 0.80;
	scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	scalarBar->GetPositionCoordinate()->SetValue(0.75, 0.1, 0.0);
	scalarBar->SetWidth(width);
	scalarBar->SetHeight(height);
	scalarBar->SetUnconstrainedFontSize(1);

	scalarBar->SetTitle("");
	scalarBar->SetNumberOfLabels(0);
	scalarBar->SetTextPositionToPrecedeScalarBar();
	
	//text properties
	vtkSmartPointer<vtkTextProperty> propL = scalarBar->GetAnnotationTextProperty();
	propL->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	propL->SetColor(0,0,0);
	propL->SetJustificationToLeft();
	propL->SetVerticalJustificationToCentered();
	propL->SetBold(false);
	propL->SetShadow(false);
	propL->Modified();

	//add title & title properties
	vtkSmartPointer<vtkTextActor> titleActor = vtkSmartPointer<vtkTextActor>::New();
	titleActor->SetInput("Correlation \n   Coefficient");
	titleActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	double* newPos = scalarBar->GetPositionCoordinate()->GetValue();
	titleActor->GetPositionCoordinate()->SetValue(newPos[0] + (width * 0.5), (1-0.005), newPos[2]);

	vtkSmartPointer<vtkTextProperty> titleTextProp = titleActor->GetTextProperty();
	titleTextProp->BoldOn();
	titleTextProp->ItalicOff();
	titleTextProp->ShadowOff();
	titleTextProp->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);
	titleTextProp->SetColor(col);
	titleTextProp->SetJustificationToLeft();
	titleTextProp->SetVerticalJustificationToTop();
	titleTextProp->Modified();

	m_graphLayoutView->GetRenderer()->AddActor2D(titleActor);
}

void iACompCorrelationMap::initializeArcLegend()
{
	vtkSmartPointer<vtkLegendBoxActor> legend = vtkSmartPointer<vtkLegendBoxActor>::New();
	legend->SetNumberOfEntries(3);

	vtkSmartPointer<vtkLineSource> legendBox = vtkSmartPointer<vtkLineSource>::New();
	legendBox->Update();

	double dataColor[4] = { 0, 0, 0, 1 };
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY,dataColor);
	legend->SetEntry(0, legendBox->GetOutput(), "Dataset", dataColor);

	double unselectedColor[4] = { 0, 0, 0, 1 };
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTERGREY, unselectedColor);
	legend->SetEntry(1, legendBox->GetOutput(), "unselected Objects", unselectedColor);

	double selectedColor[4] = { 0, 0, 0, 1 };
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, selectedColor);
	legend->SetEntry(2, legendBox->GetOutput(), "selected Objects", selectedColor);

	legend->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
	legend->GetPositionCoordinate()->SetValue(0.0, 0.0);

	legend->GetPosition2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
	legend->GetPosition2Coordinate()->SetValue(0.15, 0.15);

	legend->UseBackgroundOn();
	double background[4] = {1,1,1,1};
	legend->SetBackgroundColor(background);

	legend->GetBoxProperty()->SetPointSize(iACompVisOptions::LINE_WIDTH);
	legend->GetBoxProperty()->Modified();

	legend->GetEntryTextProperty()->SetJustificationToCentered();
	legend->GetEntryTextProperty()->SetVerticalJustificationToCentered();
	legend->GetEntryTextProperty()->BoldOn();
	legend->GetEntryTextProperty()->Modified();

	legend->Modified();

	m_graphLayoutView->GetRenderer()->AddActor(legend);
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

		//store for each arc how many percent it is
		arcPercentPair->insert({ arcActors->at(arcActors->size() - 1), objectsPerDataset->at(i)/sum });
		outerArcWithInnerArcs->insert({ arcActors->at(arcActors->size() - 1) , std::vector<vtkSmartPointer<vtkActor>>() });
		
		//store for each arc to which dataset it belongs to and that it is an outer arc
		std::map<int, double>* dataIndexWithArcType = new std::map<int, double>();
		dataIndexWithArcType->insert({i, 0.0});
		m_arcDataIndxTypePair->insert({ arcActors->at(arcActors->size() - 1), dataIndexWithArcType});

		glyphPositions->InsertNextPoint(pos[0], pos[1], pos[2]);
		glyphColors->InsertNextValue(0);
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

	for(int i = 0; i < arcActors->size(); i++)
	{
		outerArcWithLegend->insert({arcActors->at(i), legendActors->at(i)});
	}
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
	QColor c2 = iACompVisOptions::getQColor(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY);
	QColor c3 = iACompVisOptions::getQColor(iACompVisOptions::HIGHLIGHTCOLOR_GREEN);

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

	if(stippled)
	{
		iACompVisOptions::stippledLine(actor, lineStipplePattern, lineStippleRepeat);
	}

	m_graphLayoutView->GetRenderer()->AddActor(actor);

	arcActors->push_back(actor);
}

std::vector<vtkSmartPointer<vtkActor>>* iACompCorrelationMap::getArcActors()
{
	return arcActors;
}

std::map<vtkSmartPointer<vtkActor>, double>* iACompCorrelationMap::getArcPercentPairs()
{
	return arcPercentPair;
}

std::map<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkTextActor>>* iACompCorrelationMap::getOuterArcsWithLegends()
{
	return outerArcWithLegend;
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
	arcPercentPair->clear();
	outerArcWithInnerArcs->clear();

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

	style->removeHighlighting();
	outerArcWithLegend->clear();
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
		
		//store for each arc how many percent it is
		arcPercentPair->insert({ arcActors->at(arcActors->size() - 1), container.at(0) / sum });
		
		//store for arc to which dataset it belongs to and that it is an outer arc
		std::map<int, double>* dataIndexWithArcType = new std::map<int, double>();
		dataIndexWithArcType->insert({ it->first, 0.0 });
		m_arcDataIndxTypePair->insert({ arcActors->at(arcActors->size() - 1), dataIndexWithArcType });

		glyphPositions->InsertNextPoint(pos[0], pos[1], pos[2]);
		glyphColors->InsertNextValue(0);
		glyphScales->InsertNextValue(0.055);

		//calculate label position
		calculateLabelPosition(labelPositions, theta, arcLength, phi, 1.1);
		names.append(allDatasetNames.at(it->first));

		//draw inner arc
		drawInnerArc(container, pos, theta, phi, resAngle, arcLength, it->first);
		
		//store outer arc with its inner arcs
		std::vector<vtkSmartPointer<vtkActor>> innerArcs = std::vector<vtkSmartPointer<vtkActor>>();
		innerArcs.push_back(arcActors->at(arcActors->size() - 2));
		innerArcs.push_back(arcActors->at(arcActors->size() - 1));
		outerArcWithInnerArcs->insert({ arcActors->at(arcActors->size() - 3), innerArcs });

		theta += arcLength;
	}

	drawGlyphs(glyphPositions, glyphColors, glyphScales);

	drawLegend(labelPositions, names);

	//store legend for each outer arc
	int k = 0;
	for(int i = 0; i < arcActors->size(); i = i + 3)
	{
		outerArcWithLegend->insert({ arcActors->at(i), legendActors->at(k) });
		k++;
	}
}

void iACompCorrelationMap::drawInnerArc(std::vector<double> dataPoints, double* parentPosition, double parentTheta, double parentPhi, double parentAngle, double parentArcLength, int dataIndex)
{
	//draw not selected arc
	double posNotSelected[3] = {0.0, 0.0, parentPosition[2]};
	posNotSelected[0] = (cos(parentTheta) * cos(parentPhi) * (m_radius*0.925));
	posNotSelected[1] = (sin(parentTheta) * cos(parentPhi) * (m_radius*0.925));

	double amountNotPicked = dataPoints.at(0) - dataPoints.at(1);
	double percentNotPicked = amountNotPicked / dataPoints.at(0);

	double resAngle = percentNotPicked * parentAngle;
	double* col = m_lutForArcs->GetTableValue(1);
	drawArc(resAngle, posNotSelected, col, 6, true, 0xDBDB, 20); /////
	//store for each arc how many percent it is
	arcPercentPair->insert({ arcActors->at(arcActors->size() - 1), percentNotPicked });
	//store for inner arc to which dataset it belongs to and that it is an outer arc
	std::map<int, double>* dataIndexWithArcTypeNotSelectedArc = new std::map<int, double>();
	dataIndexWithArcTypeNotSelectedArc->insert({ dataIndex, 2.0 });
	m_arcDataIndxTypePair->insert({ arcActors->at(arcActors->size() - 1), dataIndexWithArcTypeNotSelectedArc });


	//draw selected arc
	double arcLengthToSelectedPoint = parentArcLength * percentNotPicked;
	double theta = parentTheta + arcLengthToSelectedPoint;

	double posSelected[3] = { 0.0, 0.0, parentPosition[2] };
	posSelected[0] = (cos(theta) * cos(parentPhi) * (m_radius*0.925));
	posSelected[1] = (sin(theta) * cos(parentPhi) * (m_radius*0.925));

	double* col1 = m_lutForArcs->GetTableValue(3);
	double angleSelected = parentAngle - resAngle;
	drawArc(angleSelected, posSelected, col1, 7, false, 0, 0);
	
	//store for each arc how many percent it is
	arcPercentPair->insert({ arcActors->at(arcActors->size() - 1), std::abs(1.0 - percentNotPicked) });
	//store for inner arc to which dataset it belongs to and that it is an inner selected arc
	std::map<int, double>* dataIndexWithSelectedArc = new std::map<int, double>();
	dataIndexWithSelectedArc->insert({ dataIndex, 1.0 });
	m_arcDataIndxTypePair->insert({ arcActors->at(arcActors->size() - 1), dataIndexWithSelectedArc });

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
	style->resetEdgeVisualization();

	vtkDoubleArray* edgeColors = vtkDoubleArray::SafeDownCast(m_graph->GetEdgeData()->GetArray("Color"));
	vtkDoubleArray* weights = vtkDoubleArray::SafeDownCast(m_graph->GetEdgeData()->GetArray("Weights"));

	for (vtkIdType currE = 0; currE < m_graph->GetNumberOfEdges(); currE++)
	{
		vtkIdType startV = m_graph->GetSourceVertex(currE);
		vtkIdType endV = m_graph->GetTargetVertex(currE);

		double colId = colorEdges(startV, endV, correlations, m_vertices);
		edgeColors->SetValue(currE, colId);
		weights->SetValue(currE, colId);
		edgeColors->Modified();
		weights->Modified();
	}

	m_graph->Modified();

	vtkRenderedGraphRepresentation* representation = dynamic_cast<vtkRenderedGraphRepresentation*>(m_graphLayoutView->GetRepresentation());
	representation->Modified();

	m_graphLayoutView->Modified();
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
iACompCorrelationMap::GraphInteractorStyle::GraphInteractorStyle() :
	m_actorPicker(vtkSmartPointer<vtkPropPicker>::New()),
	m_percentLegend(vtkSmartPointer<vtkTextActor>::New()),
	m_oldAttributesForOldPickedActors(new std::map<vtkSmartPointer<vtkActor>, std::vector<double>>()),
	m_oldPickedActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	highlightingActor(vtkSmartPointer<vtkActor>::New()),
	movedLabels(new std::vector <vtkSmartPointer<vtkTextActor>>()),
	m_oldLabelPositionInWorldCoords(new std::map<vtkSmartPointer<vtkTextActor>, std::vector<double>>()),
	edgeLabelsShown(false)
{
	initializeLutForEdgesWithLabel();
}

void iACompCorrelationMap::GraphInteractorStyle::OnLeftButtonDown()
{
	bool working = setPickList();
	if (!working) { return; }

	// Get the location of the click (in window coordinates)
	int* pos = m_graphLayoutView->GetInteractor()->GetEventPosition();

	//this line triggers openGL error, when clicking from another window into this
	int is = m_actorPicker->Pick(pos[0], pos[1], 0, m_graphLayoutView->GetRenderer());

	if (is != 0)
	{
		vtkSmartPointer<vtkActor> pickedA = m_actorPicker->GetActor();

		if (pickedA != NULL)
		{
			//reset previously picked actor
			resetOldPickedActor();
			
			//draw selection of actor
			std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>>::iterator arcIterator = m_baseClass->outerArcWithInnerArcs->find(pickedA);

			if(arcIterator != m_baseClass->outerArcWithInnerArcs->end())
			{ //selected arc is outer arc

				drawSelectedArc(pickedA);

				addHighlightingBelow(pickedA);
				moveLabel(pickedA);

				std::vector<vtkSmartPointer<vtkActor>> innerArcs = arcIterator->second;
				for(int i = 0; i < innerArcs.size(); i++)
				{
					drawSelectedArc(innerArcs.at(i));
				}
			}
			else
			{ //selected arc is inner arc
				std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>>::iterator innerArcIter;

				for (innerArcIter = m_baseClass->outerArcWithInnerArcs->begin(); innerArcIter != m_baseClass->outerArcWithInnerArcs->end(); innerArcIter++)
				{
					std::vector<vtkSmartPointer<vtkActor>> innerArcs = innerArcIter->second;

					for (int i = 0; i < innerArcs.size(); i++) 
					{
						if(innerArcs.at(i) == pickedA)
						{
							drawSelectedArc(innerArcIter->first);
							
							for (int k = 0; k < innerArcs.size(); k++)
							{
								drawSelectedArc(innerArcs.at(k));
							}

							addHighlightingBelow(pickedA);
							moveLabel(innerArcIter->first);
						}
					}
				}

			}	

			//update Histogram Table
			std::map< vtkSmartPointer<vtkActor>, std::map<int, double>*>::iterator iter = m_baseClass->m_arcDataIndxTypePair->find(pickedA);
			if (iter != m_baseClass->m_arcDataIndxTypePair->end())
			{
				m_baseClass->m_main->updateHistogramTableFromCorrelationMap(iter->second);
			}
		}
		else
		{
			//reset Histogram Table
			m_baseClass->m_main->resetHistogramTableFromCorrelationMap();
		}
	}
	else
	{
		//reset Histogram Table
		m_baseClass->m_main->resetHistogramTableFromCorrelationMap();

		m_graphLayoutView->GetRenderer()->RemoveActor(m_percentLegend);

		if (m_oldPickedActors->size() != 0)
		{
			resetOldPickedActor();
		}
	}

	m_baseClass->renderWidget();
}

void iACompCorrelationMap::GraphInteractorStyle::moveLabel(vtkSmartPointer<vtkActor> arcActor)
{
	std::map<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkTextActor>>* outerArcsWithLegend = m_baseClass->getOuterArcsWithLegends();
	std::map<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkTextActor>>::iterator iter = outerArcsWithLegend->find(arcActor);
	if (iter == outerArcsWithLegend->end()) { return; }

	vtkSmartPointer<vtkTextActor> labelActor = iter->second;
	
	labelActor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
	double* pos = labelActor->GetPositionCoordinate()->GetValue();

	std::vector<double> oldLabelPos = std::vector<double>(3,0);
	oldLabelPos.at(0) = pos[0];
	oldLabelPos.at(1) = pos[1];
	oldLabelPos.at(2) = pos[2];

	double newPos[3] = { 0, 0, 0 };
	newPos[0] = (pos[0] / m_baseClass->m_radius) * (m_baseClass->m_radius * 1.15);
	newPos[1] = (pos[1] / m_baseClass->m_radius) * (m_baseClass->m_radius * 1.15);

	labelActor->GetPositionCoordinate()->SetValue(newPos);

	vtkSmartPointer<vtkTextProperty> legendProperty = labelActor->GetTextProperty();
	legendProperty->BoldOn();
	legendProperty->Modified();

	labelActor->Modified();

	movedLabels->push_back(labelActor);
	m_oldLabelPositionInWorldCoords->insert({ labelActor , oldLabelPos });
}

void iACompCorrelationMap::GraphInteractorStyle::addHighlightingBelow(vtkSmartPointer<vtkActor> arcActor)
{
	vtkSmartPointer<vtkAlgorithm> algorithm = arcActor->GetMapper()->GetInputConnection(0, 0)->GetProducer();
	vtkSmartPointer<vtkArcSource> arc = vtkArcSource::SafeDownCast(algorithm);

	vtkSmartPointer<vtkArcSource> source = vtkSmartPointer<vtkArcSource>::New();

	source->SetUseNormalAndAngle(true);
	source->SetPolarVector(arc->GetPolarVector());
	source->SetAngle(arc->GetAngle());
	source->SetNormal(arc->GetNormal());
	source->SetResolution(arc->GetResolution());

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(source->GetOutputPort());

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper.Get());
	actor->GetProperty()->SetLineWidth(arcActor->GetProperty()->GetLineWidth()*1.5);

	double color[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_YELLOW, color);

	actor->GetProperty()->SetColor(color);

	m_graphLayoutView->GetRenderer()->AddActor(actor);
	highlightingActor = actor;

	m_graphLayoutView->GetRenderer()->RemoveActor(arcActor);
	m_graphLayoutView->GetRenderer()->AddActor(arcActor);

	//draw percentage label
	drawPercentLabel(arcActor);
}

void iACompCorrelationMap::GraphInteractorStyle::resetOldPickedActor()
{
	//reset the picked arcs
	for(int i= 0; i < m_oldPickedActors->size(); i++)
	{
		vtkSmartPointer<vtkActor> currAct = m_oldPickedActors->at(i);

		std::map<vtkSmartPointer<vtkActor>, std::vector<double>>::iterator iter = m_oldAttributesForOldPickedActors->find(currAct);
		if (iter == m_oldAttributesForOldPickedActors->end()) { continue;  }

		double oldPos[3] = { 0,0,0 };
		oldPos[0] = iter->second[0];
		oldPos[1] = iter->second[1];
		oldPos[2] = iter->second[2];

		double oldLineWidth = iter->second[3];

		vtkSmartPointer<vtkAlgorithm> algorithm = currAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
		vtkSmartPointer<vtkArcSource> arc = vtkArcSource::SafeDownCast(algorithm);
		arc->SetPolarVector(oldPos);
		arc->Modified();

		currAct->GetProperty()->SetLineWidth(oldLineWidth);
	}

	m_graphLayoutView->GetRenderer()->RemoveActor(highlightingActor);

	//reset label position & textproperty belonging to the picked arcs
	for(int i = 0; i < movedLabels->size(); i++)
	{
		vtkSmartPointer<vtkTextActor> labelActor = movedLabels->at(i);

		std::map<vtkSmartPointer<vtkTextActor>, std::vector<double>>::iterator iter = m_oldLabelPositionInWorldCoords->find(labelActor);
		if (iter == m_oldLabelPositionInWorldCoords->end()) { continue; }

		std::vector<double> pos = iter->second;
		labelActor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
		labelActor->GetPositionCoordinate()->SetValue(pos.at(0), pos.at(1), pos.at(2));

		vtkSmartPointer<vtkTextProperty> legendProperty = labelActor->GetTextProperty();
		legendProperty->BoldOff();
		legendProperty->Modified();

		labelActor->Modified();
	}

	m_oldAttributesForOldPickedActors->clear();
	m_oldPickedActors->clear();
	m_oldLabelPositionInWorldCoords->clear();
	movedLabels->clear();
}

void iACompCorrelationMap::GraphInteractorStyle::storePickedActor(vtkSmartPointer<vtkActor> arcActor, double* position, double lineWidth)
{
	//store picked actor
	m_oldPickedActors->push_back(arcActor);

	//store old position and old linewidth
	std::vector<double> characteristics = std::vector<double>(4,0);
	characteristics.at(0) = position[0];
	characteristics.at(1) = position[1];
	characteristics.at(2) = position[2];
	characteristics.at(3) = lineWidth;

	m_oldAttributesForOldPickedActors->insert({arcActor, characteristics });
}

void iACompCorrelationMap::GraphInteractorStyle::drawSelectedArc(vtkSmartPointer<vtkActor> arcActor)
{
	double oldPosition[3] = { 0,0,0 };

	vtkSmartPointer<vtkAlgorithm> algorithm = arcActor->GetMapper()->GetInputConnection(0, 0)->GetProducer();
	vtkSmartPointer<vtkArcSource> arc = vtkArcSource::SafeDownCast(algorithm);

	double newPos[3] = { 0, 0, 0 };
	double* oldPos = arc->GetPolarVector();

	oldPosition[0] = oldPos[0];
	oldPosition[1] = oldPos[1];
	oldPosition[2] = oldPos[2];

	newPos[0] = (oldPos[0] / m_baseClass->m_radius) * (m_baseClass->m_radius * 1.1);
	newPos[1] = (oldPos[1] / m_baseClass->m_radius) * (m_baseClass->m_radius * 1.1);
	newPos[2] = (oldPos[2] / m_baseClass->m_radius) * (m_baseClass->m_radius * 1.1);

	arc->SetPolarVector(newPos);
	arc->Modified();

	double oldLineWidth = arcActor->GetProperty()->GetLineWidth();
	arcActor->GetProperty()->SetLineWidth(oldLineWidth * 1.5);

	//store currently picked actor as old actor
	storePickedActor(arcActor, oldPosition, oldLineWidth);
}

void iACompCorrelationMap::GraphInteractorStyle::drawPercentLabel(vtkSmartPointer<vtkActor> arcActor)
{
	std::map<vtkSmartPointer<vtkActor>, double>* arcPercentPair = m_baseClass->getArcPercentPairs();
	std::map<vtkSmartPointer<vtkActor>, double>::iterator iter = arcPercentPair->find(arcActor);
	if (iter == arcPercentPair->end()) return;

	vtkSmartPointer<vtkAlgorithm> algorithm = arcActor->GetMapper()->GetInputConnection(0, 0)->GetProducer();
	vtkSmartPointer<vtkArcSource> arc = vtkArcSource::SafeDownCast(algorithm);

	double x = (arc->GetPolarVector()[0] / (m_baseClass->m_radius * 1.1)) * (m_baseClass->m_radius * 1.2);
	double y = (arc->GetPolarVector()[1] / (m_baseClass->m_radius * 1.1)) * (m_baseClass->m_radius * 1.2);

	m_percentLegend->SetTextScaleModeToNone();

	double percent = iter->second * 100;
	std::string text = std::to_string(percent).substr(0, 4) + "%";

	m_percentLegend->SetInput(text.c_str());

	vtkSmartPointer<vtkTextProperty> legendProperty = m_percentLegend->GetTextProperty();
	legendProperty->BoldOn();
	legendProperty->ItalicOn();
	legendProperty->ShadowOn();
	legendProperty->SetFontFamilyToArial();

	double color[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_YELLOW, color);

	legendProperty->SetColor(color);
	legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);

	if (y < 0)
	{
		legendProperty->SetVerticalJustification(VTK_TEXT_TOP);
	}
	else if (y > 0)
	{
		legendProperty->SetVerticalJustification(VTK_TEXT_BOTTOM);
	}
	

	if (x < -0.1)
	{
		legendProperty->SetJustification(VTK_TEXT_RIGHT);
	}
	else if (x >  0.1)
	{
		legendProperty->SetJustification(VTK_TEXT_LEFT);
	}
	else
	{
		legendProperty->SetJustification(VTK_TEXT_CENTERED);
	}

	legendProperty->Modified();

	m_percentLegend->GetPositionCoordinate()->SetCoordinateSystemToWorld();
	m_percentLegend->GetPositionCoordinate()->SetValue(x, y);
	m_percentLegend->Modified();

	m_graphLayoutView->GetRenderer()->AddActor(m_percentLegend);
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

void iACompCorrelationMap::GraphInteractorStyle::OnRightButtonDown() 
{ 
	//activate/deactivate if all edge labels should be shown
	if(!edgeLabelsShown)
	{
		m_baseClass->m_graphLayoutView->SetEdgeLabelVisibility(true);

		m_baseClass->m_theme->SetCellLookupTable(lutForLabels);
		m_baseClass->m_theme->SetScaleCellLookupTable(false); //necessary, otherwise coloring of edges not correct!
		m_baseClass->m_theme->Modified();

		m_baseClass->m_graphLayoutView->ApplyViewTheme(m_baseClass->m_theme);

	}else
	{
		m_baseClass->m_graphLayoutView->SetEdgeLabelVisibility(false);

		m_baseClass->m_theme->SetCellLookupTable(m_baseClass->m_lutForEdges);
		m_baseClass->m_theme->SetScaleCellLookupTable(false); //necessary, otherwise coloring of edges not correct!
		m_baseClass->m_theme->Modified();

		m_baseClass->m_graphLayoutView->ApplyViewTheme(m_baseClass->m_theme);
	}


	m_baseClass->m_graphLayoutView->Modified();
	m_baseClass->renderWidget();

	edgeLabelsShown = (!edgeLabelsShown);
}

void iACompCorrelationMap::GraphInteractorStyle::resetEdgeVisualization()
{
	m_baseClass->m_graphLayoutView->SetEdgeLabelVisibility(false);

	m_baseClass->m_theme->SetCellLookupTable(m_baseClass->m_lutForEdges);
	m_baseClass->m_theme->SetScaleCellLookupTable(false); //necessary, otherwise coloring of edges not correct!
	m_baseClass->m_theme->Modified();

	m_baseClass->m_graphLayoutView->ApplyViewTheme(m_baseClass->m_theme);
	m_baseClass->m_graphLayoutView->Modified();

	edgeLabelsShown = (!edgeLabelsShown);
}

void iACompCorrelationMap::GraphInteractorStyle::initializeLutForEdgesWithLabel()
{
	// blue to white to red
	QColor c1 = QColor(33, 102, 172); //blue
	QColor c2 = QColor(103, 169, 207);
	QColor c3 = QColor(209, 229, 240);
	QColor c4 = QColor(205, 205, 205);
	QColor c5 = QColor(253, 219, 199);
	QColor c6 = QColor(239, 138, 98);
	QColor c7 = QColor(178, 24, 43); //red

	lutForLabels = vtkSmartPointer<vtkLookupTable>::New();
	lutForLabels->SetNumberOfTableValues(7);
	lutForLabels->Build();

	lutForLabels->SetTableValue(0, c1.redF(), c1.greenF(), c1.blueF(), 1);
	lutForLabels->SetTableValue(1, c2.redF(), c2.greenF(), c2.blueF(), 1);
	lutForLabels->SetTableValue(2, c3.redF(), c3.greenF(), c3.blueF(), 1);
	lutForLabels->SetTableValue(3, c4.redF(), c4.greenF(), c4.blueF(), 1);
	lutForLabels->SetTableValue(4, c5.redF(), c5.greenF(), c5.blueF(), 1);
	lutForLabels->SetTableValue(5, c6.redF(), c6.greenF(), c6.blueF(), 1);
	lutForLabels->SetTableValue(6, c7.redF(), c7.greenF(), c7.blueF(), 1);

	//initialize annotation
	int startVal = -1.0;
	double binRangeLength = 2.0 / 7;
	for (size_t i = 0; i < 7; i++)
	{
		//make format of annotations
		double low = iACompVisOptions::round_up(startVal + (i * binRangeLength), 2);
		double high = iACompVisOptions::round_up(startVal + ((i + 1) * binRangeLength), 2);

		std::string sLow = iACompVisOptions::cutStringAfterNDecimal(std::to_string(low), 2);
		std::string sHigh = iACompVisOptions::cutStringAfterNDecimal(std::to_string(high), 2);

		//position description in the middle of each color bar in the scalarBar legend
		lutForLabels->SetAnnotation(low + ((high - low)*0.5), sLow + " to " + sHigh);
	}

	lutForLabels->SetTableRange(-1.0, 1.0);

	//NAN values are invisible
	lutForLabels->SetNanColor(0, 0, 0, 0);
}

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

void iACompCorrelationMap::GraphInteractorStyle::removeHighlighting()
{
	m_graphLayoutView->GetRenderer()->RemoveActor(highlightingActor);
	m_graphLayoutView->GetRenderer()->RemoveActor2D(m_percentLegend);
}

/************************* INNER CLASS CorrelationGraphLayout *******************************************/
iACompCorrelationMap::CorrelationGraphLayout::CorrelationGraphLayout()
{
	this->TotalIterations = 0;
	this->Temp = 0.0;
	this->m_correlations = nullptr;
	this->m_vertices = nullptr;
	this->maxDist = 0.0;
	this->minDist = 0.0;
	this->optDist = 0.0;

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
	//LOG(lvlDebug,"coefficient = " + QString::number(coefficient));
	//LOG(lvlDebug,"old fa = " + QString::number(((x * x) / k)));
	//LOG(lvlDebug,"fa = " + QString::number(((x * x) / k) * (coefficient / 100)));

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

	//LOG(lvlDebug,"GraphBounds = " + QString::number(this->GraphBounds[0]) + ", " + QString::number(this->GraphBounds[1]) + "\n" +
	//	QString::number(this->GraphBounds[2]) + ", " + QString::number(this->GraphBounds[3]) + "\n" +
	//	QString::number(this->GraphBounds[4]) + ", " + QString::number(this->GraphBounds[5]) );

	// More variable definitions
	double volume = (this->GraphBounds[1] - this->GraphBounds[0]) *
		(this->GraphBounds[3] - this->GraphBounds[2]) *
		(this->GraphBounds[5] - this->GraphBounds[4]);

	//LOG(lvlDebug,"volume = " + QString::number(volume));

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

	//LOG(lvlDebug,"Temp = " + QString::number(volume));

	// The optimal distance between vertices.
	//this->optDist = pow(volume / numVertices, 0.33333);
	this->optDist = pow(volume / numVertices, 0.33333);
	this->minDist = optDist;//1 * std::sqrt(volume / numVertices); //pow(volume / numVertices, 2);
	this->maxDist = 2*optDist;//3 * std::sqrt(volume / numVertices); //pow(volume / numVertices, 0.1);

	//LOG(lvlDebug,"optDist = " + QString::number(optDist));
	//LOG(lvlDebug,"maxDist = " + QString::number(maxDist));
	//LOG(lvlDebug,"minDist = " + QString::number(minDist));

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

	LOG(lvlDebug,"GraphBounds = " + QString::number(this->GraphBounds[0]) + ", " + QString::number(this->GraphBounds[1]) + "\n" +
		QString::number(this->GraphBounds[2]) + ", " + QString::number(this->GraphBounds[3]) + "\n" +
		QString::number(this->GraphBounds[4]) + ", " + QString::number(this->GraphBounds[5]));

	// More variable definitions
	double volume = (this->GraphBounds[1] - this->GraphBounds[0]) *
		(this->GraphBounds[3] - this->GraphBounds[2]) *
		(this->GraphBounds[5] - this->GraphBounds[4]);

	LOG(lvlDebug,"volume = " + QString::number(volume));

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

	LOG(lvlDebug,"Temp = " + QString::number(volume));

	// The optimal distance between vertices.
	//this->optDist = pow(volume / numVertices, 0.33333);
	this->optDist = pow(volume / numVertices, 0.33333);
	this->minDist = optDist; //1 * std::sqrt(volume / numVertices);//pow(volume / numVertices, 2);
	this->maxDist = optDist; //3 * std::sqrt(volume / numVertices);//pow(volume / numVertices, 0.1);

	LOG(lvlDebug,"optDist = " + QString::number(optDist));
	LOG(lvlDebug,"maxDist = " + QString::number(maxDist));
	LOG(lvlDebug,"minDist = " + QString::number(minDist));

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
	//LOG(lvlDebug,"IterationsPerLayout: " + QString::number(IterationsPerLayout));
	//LOG(lvlDebug,"intial Temp = " + QString::number(Temp));
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
				
					//LOG(lvlDebug,"norm: " + QString::number(norm));

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
	LOG(lvlDebug,"IterationsPerLayout: " + QString::number(IterationsPerLayout));
	LOG(lvlDebug,"intial Temp = " + QString::number(Temp));
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

					//LOG(lvlDebug,"norm: " + QString::number(norm));

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
		LOG(lvlDebug,"");
		LOG(lvlDebug,"++++++++++++++++++++++++++++++++++++++++");
		LOG(lvlDebug,"timeStep = " + QString::number(timeStep));
		for (vtkIdType j = 0; j < numEdges; j++)
		{
			if (j == 0 || j == 1)
			{
				LOG(lvlDebug,"j = " + QString::number(j) );
				LOG(lvlDebug,"Pos = " + QString::number(v[e[j].u].x[0]) + ", " + QString::number(v[e[j].u].x[1]) + ", " + QString::number(v[e[j].u].x[2]));
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
				LOG(lvlDebug,"weight = " + QString::number(weight));
				LOG(lvlDebug,"displacement = " + QString::number(displacement));
				LOG(lvlDebug,"fa = " + QString::number(fa));
				LOG(lvlDebug,"acc = " + QString::number(acc));
				LOG(lvlDebug,"velocity = " + QString::number(velocity));
				LOG(lvlDebug,"new Pos = " + QString::number(v[e[j].u].x[0]) + ", " + QString::number(v[e[j].u].x[1]) + ", " + QString::number(v[e[j].u].x[2]));
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
