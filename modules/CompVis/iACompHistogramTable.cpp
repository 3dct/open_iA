#include "iACompHistogramTable.h"

//iA
#include "mainwindow.h"

//Qt
#include "QVTKOpenGLNativeWidget.h"

//vtk testing
#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNamedColors.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQuad.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>


iACompHistogramTable::iACompHistogramTable(MainWindow* parent, csvDataType::ArrayType* mdsResult) :
	QDockWidget(parent),
	m_mdsMatrix(mdsResult)
{
	//TODO draw table
	setupUi(this);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(qvtkWidget);

	//testing
	vtkSmartPointer<vtkNamedColors> colors = vtkSmartPointer<vtkNamedColors>::New();

	// Create four points (must be in counter clockwise order)
	double p0[3] = {0.0, 0.0, 0.0};
	double p1[3] = {1.0, 0.0, 0.0};
	double p2[3] = {1.0, 1.0, 0.0};
	double p3[3] = {0.0, 1.0, 0.0};

	// Add the points to a vtkPoints object
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	points->InsertNextPoint(p0);
	points->InsertNextPoint(p1);
	points->InsertNextPoint(p2);
	points->InsertNextPoint(p3);

	// Create a quad on the four points
	vtkSmartPointer<vtkQuad> quad = vtkSmartPointer<vtkQuad>::New();
	quad->GetPointIds()->SetId(0, 0);
	quad->GetPointIds()->SetId(1, 1);
	quad->GetPointIds()->SetId(2, 2);
	quad->GetPointIds()->SetId(3, 3);

	// Create a cell array to store the quad in
	vtkSmartPointer<vtkCellArray> quads = vtkSmartPointer<vtkCellArray>::New();
	quads->InsertNextCell(quad);

	// Create a polydata to store everything in
	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();

	// Add the points and quads to the dataset
	polydata->SetPoints(points);
	polydata->SetPolys(quads);

	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData(polydata);

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	actor->GetProperty()->SetColor(colors->GetColor3d("Silver").GetData());

	// Setup render window, renderer, and interactor
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

	renderer->AddActor(actor);
	renderer->SetBackground(colors->GetColor3d("Salmon").GetData());

	vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;	
	qvtkWidget->SetRenderWindow(renderWindow);
	qvtkWidget->GetRenderWindow()->AddRenderer(renderer);

	qvtkWidget->show();
}