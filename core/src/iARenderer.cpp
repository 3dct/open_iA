/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iARenderer.h"

#include "defines.h"
#include "iAConsole.h"
#include "iAChannelVisualizationData.h"
#include "iAMovieHelper.h"
#include "iARenderObserver.h"
#include "iARenderSettings.h"
#include "mdichild.h"

#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkAppendFilter.h>
#include <vtkAreaPicker.h>
#include <vtkAxesActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellLocator.h>
#include <vtkCubeSource.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractSelectedFrustum.h>
#include <vtkGenericMovieWriter.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkImageData.h>
#include <vtkImageCast.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkLineSource.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPicker.h>
#include <vtkPlane.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQImageToImageSource.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVersion.h>
#include <vtkWindowToImageFilter.h>

#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QImage>
#include <QLocale>
#include <QApplication>
#include <QImage>

#define VTKISRBP_ORIENT 0
#define VTKISRBP_SELECT 1

static void GetCellCenter(vtkUnstructuredGrid* imageData, const unsigned int cellId,
	double center[3], double spacing[3]);

class MouseInteractorStyle : public vtkInteractorStyleRubberBandPick
{
public:
	static MouseInteractorStyle* New();
	
	virtual void OnChar()
	{
		switch (this->Interactor->GetKeyCode())
		{
		case 's':
		case 'S':
		{
			if (this->CurrentMode == VTKISRBP_ORIENT)
			{
				this->CurrentMode = VTKISRBP_SELECT;
			}
			else
			{
				this->CurrentMode = VTKISRBP_ORIENT;
			}
			break;
		}
		case 'p':
		case 'P':
		{
			vtkRenderWindowInteractor *rwi = this->Interactor;
			int *eventPos = rwi->GetEventPosition();
			this->FindPokedRenderer(eventPos[0], eventPos[1]);
			this->StartPosition[0] = eventPos[0];
			this->StartPosition[1] = eventPos[1];
			this->EndPosition[0] = eventPos[0];
			this->EndPosition[1] = eventPos[1];
			this->Pick();
			break;
		}
		default:
			this->Superclass::OnChar();
		}
	}
};
vtkStandardNewMacro(MouseInteractorStyle);

void KeyPressCallbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
	void* clientData, void* vtkNotUsed(callData))
{
	iARenderer *ren = static_cast<iARenderer*>(clientData);
	if( ren->GetInteractor()->GetKeyCode() == 's' ||
		ren->GetInteractor()->GetKeyCode() == 'S')
	{
		ren->GetTxtActor()->SetVisibility(!ren->GetTxtActor()->GetVisibility());
		ren->update();
	}
}

void PickCallbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
	void* clientData, void* vtkNotUsed(callData))
{
	vtkAreaPicker *areaPicker = static_cast<vtkAreaPicker*>(caller);
	iARenderer *ren = static_cast<iARenderer*>(clientData);
	ren->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(ren->selectedActor);

	auto extractSelection = vtkSmartPointer<vtkExtractSelectedFrustum>::New();
	extractSelection->SetInputData(0, ren->getRenderObserver()->GetImageData());
	extractSelection->PreserveTopologyOff();
	extractSelection->SetFrustum(areaPicker->GetFrustum());
	extractSelection->Update();

	if (!extractSelection->GetOutput()->GetNumberOfElements(vtkUnstructuredGrid::CELL))
	{
		ren->emitNoSelectedCells();
		return;
	}
	
	if (ren->GetInteractor()->GetControlKey() &&
		!ren->GetInteractor()->GetShiftKey())
	{
		// Adds cells to selection
		auto append = vtkSmartPointer<vtkAppendFilter>::New();
		append->AddInputData(ren->finalSelection);
		append->AddInputData(extractSelection->GetOutput());
		append->Update();
		ren->finalSelection->ShallowCopy(append->GetOutput());
	}
	else if (ren->GetInteractor()->GetControlKey() &&
		ren->GetInteractor()->GetShiftKey())
	{
		// Removes cells from selection 
		auto newfinalSel = vtkSmartPointer<vtkUnstructuredGrid>::New();
		newfinalSel->Allocate(1, 1);
		newfinalSel->SetPoints(ren->finalSelection->GetPoints());
		auto currSel = vtkSmartPointer<vtkUnstructuredGrid>::New();
		currSel->ShallowCopy(extractSelection->GetOutput());
		double f_Cell[DIM] = { 0,0,0 }, c_Cell[DIM] = { 0,0,0 };
		double* spacing = ren->getRenderObserver()->GetImageData()->GetSpacing();

		for (vtkIdType i = 0; i < ren->finalSelection->GetNumberOfCells(); ++i)
		{
			bool addCell = true;
			GetCellCenter(ren->finalSelection, i, f_Cell, spacing);
			for (vtkIdType j = 0; j < currSel->GetNumberOfCells(); ++j)
			{
				GetCellCenter(currSel, j, c_Cell, spacing);
				if (f_Cell[0] == c_Cell[0] &&
					f_Cell[1] == c_Cell[1] &&
					f_Cell[2] == c_Cell[2])
				{
					addCell = false;
					break;
				}
			}
			if (addCell)
				newfinalSel->InsertNextCell(ren->finalSelection->GetCell(i)->GetCellType(),
					ren->finalSelection->GetCell(i)->GetPointIds());
		}		
		ren->finalSelection->ShallowCopy(newfinalSel);
	}
	else
	{
		// New selection
		ren->finalSelection->ShallowCopy(extractSelection->GetOutput());
	}
	ren->selectedMapper->Update();
	ren->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(ren->selectedActor);
	ren->emitSelectedCells(ren->finalSelection);
}

void GetCellCenter(vtkUnstructuredGrid* data, const unsigned int cellId, double center[DIM], double spacing[DIM])
{
	double pcoords[DIM] = { 0,0,0 };
	double *weights = new double[data->GetMaxCellSize()];
	vtkCell* cell = data->GetCell(cellId);
	int np = cell->GetPoints()->GetNumberOfPoints();
	int subId = cell->GetParametricCenter(pcoords);
	cell->EvaluateLocation(subId, pcoords, center, weights);
	for (int i = 0; i < DIM; ++i)
		center[i] = floor(center[i] / spacing[i]);
}

iARenderer::iARenderer(QObject *par)  :  QObject( par ),
	interactor(0),
	renderObserver(0),
    m_SlicePlaneOpacity(0.8)
{
	renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();		// TODO: move out of here?
	renWin->AlphaBitPlanesOn();
	renWin->LineSmoothingOn();
	renWin->PointSmoothingOn();

	cam = vtkSmartPointer<vtkCamera>::New();

	cSource = vtkSmartPointer<vtkCubeSource>::New();
	cMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	cActor = vtkSmartPointer<vtkActor>::New();

	logoRep = vtkSmartPointer<vtkLogoRepresentation>::New();
	logoWidget = vtkSmartPointer<vtkLogoWidget>::New();
	logoImage = vtkSmartPointer<vtkQImageToImageSource>::New();

	labelRen = vtkSmartPointer<vtkOpenGLRenderer>::New();
	ren = vtkSmartPointer<vtkOpenGLRenderer>::New();

	polyMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	polyActor = vtkSmartPointer<vtkActor>::New();

	annotatedCubeActor = vtkSmartPointer<vtkAnnotatedCubeActor>::New();
	axesActor = vtkSmartPointer<vtkAxesActor>::New();
	moveableAxesActor = vtkSmartPointer<vtkAxesActor>::New();
	orientationMarkerWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();

	plane1 = vtkSmartPointer<vtkPlane>::New();
	plane2 = vtkSmartPointer<vtkPlane>::New();
	plane3 = vtkSmartPointer<vtkPlane>::New();

	m_slicingCube = vtkSmartPointer<vtkCubeSource>::New();
	m_sliceCubeMapper = vtkSmartPointer<vtkPolyDataMapper>::New(); 
	m_sliceCubeActor = vtkSmartPointer<vtkActor>::New(); 

	txtActor = vtkSmartPointer<vtkTextActor>::New();
	txtActor->SetInput("Selection mode");
	txtActor->GetTextProperty()->SetFontSize(24);
	txtActor->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
	txtActor->GetTextProperty()->SetJustificationToLeft();
	txtActor->GetTextProperty()->SetVerticalJustificationToBottom();
	txtActor->VisibilityOff();

	cellLocator = vtkSmartPointer<vtkCellLocator>::New();

	m_profileLineSource = vtkSmartPointer<vtkLineSource>::New();
	m_profileLineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_profileLineActor = vtkSmartPointer<vtkActor>::New();
	m_profileLineStartPointSource = vtkSmartPointer<vtkSphereSource>::New();
	m_profileLineStartPointMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_profileLineStartPointActor = vtkSmartPointer<vtkActor>::New();
	m_profileLineEndPointSource = vtkSmartPointer<vtkSphereSource>::New();
	m_profileLineEndPointMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_profileLineEndPointActor = vtkSmartPointer<vtkActor>::New();
	for (int s = 0; s < 3; ++s)
	{
		m_slicePlaneSource[s] = vtkSmartPointer<vtkPlaneSource>::New();
		m_slicePlaneMapper[s] = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_slicePlaneActor[s] = vtkSmartPointer<vtkActor>::New();
		m_slicePlaneActor[s]->GetProperty()->LightingOff();
	}

	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if (mdi_parent)
	{
		connect(this, SIGNAL(msg(QString)), mdi_parent, SLOT(addMsg(QString)));
		connect(this, SIGNAL(progress(int)), mdi_parent, SLOT(updateProgressBar(int)));
	}
}

iARenderer::~iARenderer(void)
{
	ren->RemoveAllObservers();
	renWin->RemoveAllObservers();
	if (renderObserver) renderObserver->Delete();
}

namespace
{
	int GetSliceAxis(int axis, int index)
	{
		switch (axis)
		{
		default: // note: switch case labels are currently NOT equal to iASlicerMode numbers, see note there!
		case 0: return index == 0 ? 1 : 2; // YZ
		case 1: return index == 0 ? 0 : 2; // XZ
		case 2: return index == 0 ? 0 : 1; // YZ
		}
	}
}

void iARenderer::initialize( vtkImageData* ds, vtkPolyData* pd, int e )
{
	imageData = ds;
	polyData = pd;
	cellLocator->SetDataSet(polyData);
	if(polyData)
		if( polyData->GetNumberOfCells() )
			cellLocator->BuildLocator();
	ext = e;
	double spacing[3];	ds->GetSpacing(spacing);
	ren->SetLayer(0);
	ren->UseDepthPeelingOn();
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
	ren->UseDepthPeelingForVolumesOn();
#endif
	labelRen->SetLayer(1);
	labelRen->InteractiveOff();
	labelRen->UseDepthPeelingOn();
	renWin->SetNumberOfLayers(5);
	renWin->AddRenderer(ren);
	renWin->AddRenderer(labelRen);
	interactor = renWin->GetInteractor();
	setPointPicker();	
	InitObserver();

	QImage img;
	img.load(":/images/fhlogo.png");
	logoImage->SetQImage(&img);
	logoImage->Update();
	logoRep->SetImage(logoImage->GetOutput( ));
	logoWidget->SetInteractor(interactor);
	logoWidget->SetRepresentation(logoRep);
	logoWidget->SetResizable(false);
	logoWidget->SetSelectable(true);
	logoWidget->On();

	interactor->Initialize();

	// setup cube source
	cSource->SetXLength(ext * spacing[0]);
	cSource->SetYLength(ext * spacing[1]);
	cSource->SetZLength(ext * spacing[2]);
	cMapper->SetInputConnection(cSource->GetOutputPort());
	cActor->SetMapper(cMapper);
	cActor->GetProperty()->SetColor(1,0,0);

	setupCutter();
	setupCube();
	setupAxes(spacing);
	setupOrientationMarker();
	setupRenderer();

	labelRen->SetActiveCamera(cam);
	ren->SetActiveCamera(cam);
	setCamPosition( 0,0,1, 1,1,1 ); // +Z

	m_profileLineMapper->SetInputConnection(m_profileLineSource->GetOutputPort());
	m_profileLineActor->SetMapper(m_profileLineMapper);
	m_profileLineStartPointMapper->SetInputConnection(m_profileLineStartPointSource->GetOutputPort());
	m_profileLineStartPointActor->SetMapper(m_profileLineStartPointMapper);
	m_profileLineEndPointMapper->SetInputConnection(m_profileLineEndPointSource->GetOutputPort());
	m_profileLineEndPointActor->SetMapper(m_profileLineEndPointMapper);
	m_profileLineActor->GetProperty()->SetColor(0.59, 0.73, 0.94);//ffa800//150, 186, 240
	m_profileLineActor->GetProperty()->SetLineWidth(2.0);
	m_profileLineActor->GetProperty()->SetLineStipplePattern(0x00ff);//0xf0f0
	m_profileLineActor->GetProperty()->SetLineStippleRepeatFactor(1);
	m_profileLineActor->GetProperty()->SetPointSize(2);
	m_profileLineStartPointSource->SetRadius(2 * spacing[0]);
	m_profileLineEndPointSource->SetRadius(2 * spacing[0]);
	m_profileLineStartPointActor->GetProperty()->SetColor(1.0, 0.65, 0.0);
	m_profileLineEndPointActor->GetProperty()->SetColor(0.0, 0.65, 1.0);


	 //slicing cube settings for surface extraction
	 m_sliceCubeMapper->SetInputConnection(m_slicingCube->GetOutputPort());
	 m_sliceCubeActor->SetMapper(m_sliceCubeMapper);
	 m_sliceCubeActor->GetProperty()->SetColor(1.0, 0, 0);
	 m_sliceCubeActor->GetProperty()->SetRepresentationToWireframe();
	 m_sliceCubeActor->GetProperty()->SetOpacity(1); 
	 m_sliceCubeActor->GetProperty()->SetLineWidth(2.3); 	 
	 m_sliceCubeActor->GetProperty()->SetAmbient(1.0);
	 m_sliceCubeActor->GetProperty()->SetDiffuse(0.0);
	 m_sliceCubeActor->GetProperty()->SetSpecular(0.0);
	 
	 m_sliceCubeActor->SetVisibility(false);

	 this->	setArbitraryProfileOn(false);

	double center[3], origin[3];
	const int * dim = imageData->GetDimensions();
	if (dim[0] == 0 || dim[1] == 0 || dim[2] == 0)
		return;
	const double * spc = imageData->GetSpacing();
	for (int i = 0; i < 3; ++i)
	{
		center[i] = dim[i] * spc[i] / 2;
		origin[i] = 0;
	}
	for (int s = 0; s < 3; ++s)
	{
		m_slicePlaneSource[s]->SetOrigin(origin);
		double point1[3], point2[3];
		for (int j = 0; j < 3; ++j)
		{
			point1[j] = 0;
			point2[j] = 0;
		}
		point1[GetSliceAxis(s, 0)] += 1.1 * dim[GetSliceAxis(s, 0)] * spc[GetSliceAxis(s, 0)];
		point2[GetSliceAxis(s, 1)] += 1.1 * dim[GetSliceAxis(s, 1)] * spc[GetSliceAxis(s, 1)];
		m_slicePlaneSource[s]->SetPoint1(point1);
		m_slicePlaneSource[s]->SetPoint2(point2);
		m_slicePlaneSource[s]->SetCenter(center);
		m_slicePlaneMapper[s]->SetInputConnection(m_slicePlaneSource[s]->GetOutputPort());
		m_slicePlaneActor[s]->SetMapper(m_slicePlaneMapper[s]);
		m_slicePlaneActor[s]->GetProperty()->SetColor( (s == 0) ? 1:0, (s == 1) ? 1 : 0, (s == 2) ? 1 : 0);
		m_slicePlaneActor[s]->GetProperty()->SetOpacity(1.0);
		m_slicePlaneActor[s]->SetVisibility(false);
		m_slicePlaneMapper[s]->Update();
	}
}

void iARenderer::reInitialize( vtkImageData* ds, vtkPolyData* pd, int e )
{
	imageData = ds;
	polyData = pd;
	if( polyData )
	{
		cellLocator->SetDataSet( polyData );
		if( polyData->GetNumberOfCells() )
			cellLocator->BuildLocator();
	}
	ext = e;
	polyMapper->SetInputData(polyData);

	renderObserver->ReInitialize( ren, labelRen, interactor, pointPicker,
		moveableAxesTransform, ds,
		plane1, plane2, plane3, cellLocator );
	interactor->ReInitialize();
	emit reInitialized();
	update();
}

void iARenderer::setAreaPicker()
{
	auto areaPicker = vtkSmartPointer<vtkAreaPicker>::New();
	interactor->SetPicker(areaPicker);
	auto style = vtkSmartPointer<MouseInteractorStyle>::New();
	interactor->SetInteractorStyle(style);

	auto keyPressCallback = vtkSmartPointer<vtkCallbackCommand>::New();
	keyPressCallback->SetCallback(KeyPressCallbackFunction);
	keyPressCallback->SetClientData(this);
	interactor->AddObserver(vtkCommand::KeyReleaseEvent, keyPressCallback, 1.0);

	auto pickCallback = vtkSmartPointer<vtkCallbackCommand>::New();
	pickCallback->SetCallback(PickCallbackFunction);
	pickCallback->SetClientData(this);
	areaPicker->AddObserver(vtkCommand::EndPickEvent, pickCallback, 1.0);

	finalSelection = vtkSmartPointer<vtkUnstructuredGrid>::New();
	selectedMapper = vtkSmartPointer<vtkDataSetMapper>::New();
	selectedMapper->SetScalarModeToUseCellData();
	selectedMapper->SetInputData(finalSelection);
	selectedActor = vtkSmartPointer<vtkActor>::New();
	QColor c(255, 0, 0);
	selectedActor->SetMapper(selectedMapper);
	selectedActor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
	selectedActor->GetProperty()->SetRepresentationToWireframe();
	selectedActor->GetProperty()->EdgeVisibilityOn();
	selectedActor->GetProperty()->SetEdgeColor(c.redF(), c.greenF(), c.blueF());
	selectedActor->GetProperty()->SetLineWidth(3);
}

void iARenderer::setPointPicker()
{
	pointPicker = vtkSmartPointer<vtkPicker>::New();
	pointPicker->SetTolerance(0.00005);//spacing[0]/150);
	interactor->SetPicker(pointPicker);
	interactorStyle = vtkSmartPointer<vtkInteractorStyleSwitch>::New();
	interactorStyle->SetCurrentStyleToTrackballCamera();
	interactor->SetInteractorStyle(interactorStyle);
}

void iARenderer::setupCutter()
{
	plane1->SetNormal(1, 0, 0);
	plane2->SetNormal(0, 1, 0);
	plane3->SetNormal(0, 0, 1);
}

void iARenderer::setupCube()
{
	annotatedCubeActor->SetPickable(1);
	annotatedCubeActor->SetXPlusFaceText("+X");
	annotatedCubeActor->SetXMinusFaceText("-X");
	annotatedCubeActor->SetYPlusFaceText("+Y");
	annotatedCubeActor->SetYMinusFaceText("-Y");
	annotatedCubeActor->SetZPlusFaceText("+Z");
	annotatedCubeActor->SetZMinusFaceText("-Z");
	annotatedCubeActor->SetXFaceTextRotation(0);
	annotatedCubeActor->SetYFaceTextRotation(0);
	annotatedCubeActor->SetZFaceTextRotation(90);
	annotatedCubeActor->SetFaceTextScale(0.45);
	annotatedCubeActor->GetCubeProperty()->SetColor(0.7, 0.78, 1);
	annotatedCubeActor->GetTextEdgesProperty()->SetDiffuse(0);
	annotatedCubeActor->GetTextEdgesProperty()->SetAmbient(0);
	annotatedCubeActor->GetXPlusFaceProperty()->SetColor(1, 0, 0);
	annotatedCubeActor->GetXPlusFaceProperty()->SetInterpolationToFlat();
	annotatedCubeActor->GetXMinusFaceProperty()->SetColor(1, 0, 0);
	annotatedCubeActor->GetXMinusFaceProperty()->SetInterpolationToFlat();
	annotatedCubeActor->GetYPlusFaceProperty()->SetColor(0, 1, 0);
	annotatedCubeActor->GetYPlusFaceProperty()->SetInterpolationToFlat();
	annotatedCubeActor->GetYMinusFaceProperty()->SetColor(0, 1, 0);
	annotatedCubeActor->GetYMinusFaceProperty()->SetInterpolationToFlat();
	annotatedCubeActor->GetZPlusFaceProperty()->SetColor(0, 0, 1);
	annotatedCubeActor->GetZPlusFaceProperty()->SetInterpolationToFlat();
	annotatedCubeActor->GetZMinusFaceProperty()->SetColor(0, 0, 1);
	annotatedCubeActor->GetZMinusFaceProperty()->SetInterpolationToFlat();
}

void iARenderer::setupAxes(double spacing[3])
{
	axesActor->AxisLabelsOff();
	axesActor->SetShaftTypeToCylinder();
	axesActor->SetTotalLength(15, 15, 15);

	vtkTransform *transform = vtkTransform::New();
	transform->Scale(spacing[0]*3, spacing[1]*3, spacing[2]*3);

	axesActor->SetUserTransform(transform);
	transform->Delete();

	moveableAxesActor->AxisLabelsOff();
	moveableAxesActor->SetShaftTypeToCylinder();
	moveableAxesActor->SetTotalLength(15, 15, 15);

	moveableAxesTransform->Scale(spacing[0]*3, spacing[1]*3, spacing[2]*3);

	moveableAxesActor->SetUserTransform(moveableAxesTransform);
}

void iARenderer::setupOrientationMarker()
{
	orientationMarkerWidget->SetOrientationMarker(annotatedCubeActor);
	orientationMarkerWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
	orientationMarkerWidget->SetInteractor(interactor);
	orientationMarkerWidget->SetEnabled( 1 );
	orientationMarkerWidget->InteractiveOff();
}

void iARenderer::setupRenderer()
{
	polyMapper->SetInputData(polyData);
	polyMapper->SelectColorArray("Colors");
	polyMapper->SetScalarModeToUsePointFieldData();
	polyActor->SetMapper(polyMapper);

	ren->GradientBackgroundOn();
	ren->AddActor2D(txtActor);
	ren->AddActor(polyActor);
	ren->AddActor(cActor);
	ren->AddActor(axesActor);
	ren->AddActor(moveableAxesActor);
	ren->AddActor(m_profileLineActor);
	ren->AddActor(m_profileLineStartPointActor);
	ren->AddActor(m_profileLineEndPointActor);
	ren->AddActor(m_sliceCubeActor); 
	for (int s = 0; s < 3; ++s)
		ren->AddActor(m_slicePlaneActor[s]);
	emit onSetupRenderer();
}

void iARenderer::update()
{
	if (polyData)
	{
		polyMapper->Update();
	}
	renWin->Render();
}

void iARenderer::showHelpers(bool show)
{
	orientationMarkerWidget->SetEnabled(show);
	axesActor->SetVisibility(show);
	moveableAxesActor->SetVisibility(show);
	logoWidget->SetEnabled(show);
	cActor->SetVisibility(show);
}

void iARenderer::showRPosition(bool s)
{
	cActor->SetVisibility(s);
}

void iARenderer::showSlicePlanes(bool show)
{
	for (int s = 0; s < 3; ++s) {
		m_slicePlaneActor[s]->SetVisibility(show);
		m_slicePlaneActor[s]->GetProperty()->SetOpacity(m_SlicePlaneOpacity);
	}
}

void iARenderer::setPlaneNormals( vtkTransform *tr )
{
	double normal[4], temp[4];

	normal[0] = 1; normal[1] = 0; normal[2] = 0; normal[3] = 1;
	tr->GetMatrix()->MultiplyPoint(normal, temp);
	plane1->SetNormal( temp[0], temp[1], temp[2] );

	normal[0] = 0; normal[1] = 1; normal[2] = 0; normal[3] = 1;
	tr->GetMatrix()->MultiplyPoint(normal, temp);
	plane2->SetNormal( temp[0], temp[1], temp[2] );

	normal[0] = 0; normal[1] = 0; normal[2] = 1; normal[3] = 1;
	tr->GetMatrix()->MultiplyPoint(normal, temp);
	plane3->SetNormal( temp[0], temp[1], temp[2] );

	renWin->Render();
	ren->Render();
};

void iARenderer::setCubeCenter( int x, int y, int z )
{
	if (interactor->GetEnabled()) {
		cSource->SetCenter( x * imageData->GetSpacing()[0],
			y * imageData->GetSpacing()[1],
			z * imageData->GetSpacing()[2] );
		update();
	}
};

void iARenderer::setCamPosition( int uvx, int uvy, int uvz, int px, int py, int pz )
{
	cam->SetViewUp ( uvx, uvy, uvz );
	cam->SetPosition ( px, py, pz );
	cam->SetFocalPoint( 0,0,0 );
	ren->ResetCamera();
	update();
}


void iARenderer::getCamPosition( double * camOptions )
{
	double pS = cam->GetParallelScale();
	double a[3] = {0};
	double b[3] = {0};
	double c[3] = {0};
	cam->GetViewUp(a);
	cam->GetPosition(b);
	cam->GetFocalPoint(c);

	camOptions[0] = a[0];
	camOptions[1] = a[1];
	camOptions[2] = a[2];
	camOptions[3] = b[0];
	camOptions[4] = b[1];
	camOptions[5] = b[2];
	camOptions[6] = c[0];
	camOptions[7] = c[1];
	camOptions[8] = c[2];
	camOptions[9] = pS;

	ren->ResetCamera();
	update();
}


void iARenderer::setCamPosition( double * camOptions, bool rsParallelProjection )
{
	cam->SetViewUp ( camOptions[0], camOptions[1], camOptions[2] );
	cam->SetPosition ( camOptions[3], camOptions[4], camOptions[5] );
	cam->SetFocalPoint( camOptions[6], camOptions[7], camOptions[8] );

	if(rsParallelProjection)
		cam->SetParallelScale( camOptions[9] );

	update();
}


void iARenderer::setCamera(vtkCamera* c)
{
	cam = c;
	labelRen->SetActiveCamera(cam);
	ren->SetActiveCamera(cam);
	emit onSetCamera();
}


vtkCamera* iARenderer::getCamera()
{
	return cam;
}


void iARenderer::saveMovie( const QString& fileName, int mode, int qual /*= 2*/ )
{
	vtkSmartPointer<vtkGenericMovieWriter> movieWriter = GetMovieWriter(fileName, qual);

	if (movieWriter.GetPointer() == nullptr)
		return;

	interactor->Disable();

	vtkSmartPointer<vtkCamera> oldCam = vtkSmartPointer<vtkCamera>::New();
	oldCam->DeepCopy(cam);

	vtkSmartPointer<vtkWindowToImageFilter> w2if = vtkSmartPointer<vtkWindowToImageFilter>::New();
	int* rws = renWin->GetSize();
	if (rws[0] % 2 != 0) rws[0]++;
	if (rws[1] % 2 != 0) rws[1]++;
	renWin->SetSize(rws);
	renWin->Render();

	w2if->SetInput(renWin);
	w2if->ReadFrontBufferOff();

	movieWriter->SetInputConnection(w2if->GetOutputPort());
	movieWriter->Start();

	emit msg(tr("MOVIE export started. Output: %1").arg(fileName));

	int numRenderings = 360;//TODO
	vtkSmartPointer<vtkTransform> rot = vtkSmartPointer<vtkTransform>::New();
	cam->SetFocalPoint( 0,0,0 );
	double view[3];
	double point[3];
	if (mode == 0) { // YZ
		double _view[3]  = { 0 ,0, -1 };
		double _point[3] = { 1, 0, 0 };
		for (int ind=0; ind<3; ind++)
		{
			view[ind] = _view[ind];
			point[ind] = _point[ind];
		}
		rot->RotateZ(360/numRenderings);
	} else if (mode == 1) { // XY
		double _view[3]  = { 0, 0, -1 };
		double _point[3] = { 0, 1, 0 };
		for (int ind=0; ind<3; ind++)
		{
			view[ind] = _view[ind];
			point[ind] = _point[ind];
		}
		rot->RotateX(360/numRenderings);
	} else if (mode == 2) { // XZ
		double _view[3]  = { 0, 1, 0 };
		double _point[3] = { 0, 0, 1 };
		for (int ind=0; ind<3; ind++)
		{
			view[ind] = _view[ind];
			point[ind] = _point[ind];
		}
		rot->RotateY(360/numRenderings);
	}
	cam->SetViewUp ( view );
	cam->SetPosition ( point );
	for (int i =0; i < numRenderings; i++ ) {
		ren->ResetCamera();
		renWin->Render();

		w2if->Modified();
		movieWriter->Write();
		if (movieWriter->GetError()) {
			emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
			break;
		}
		emit progress( 100 * (i+1) / numRenderings);
		cam->ApplyTransform(rot);
	}

	cam->DeepCopy(oldCam);
	movieWriter->End();
	movieWriter->ReleaseDataFlagOn();
	w2if->ReleaseDataFlagOn();

	interactor->Enable();

	if (movieWriter->GetError()) emit msg(tr("  MOVIE export failed."));
	else emit msg(tr("  MOVIE export completed."));

	return;
}

void iARenderer::mouseRightButtonReleasedSlot()
{
	if (!interactor)
		return;
	interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent);
}

void iARenderer::mouseLeftButtonReleasedSlot()
{
	if (!interactor)
		return;
	interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
}

void iARenderer::setArbitraryProfile(int pointIndex, double * coords)
{
	if (pointIndex == 0)
	{
		m_profileLineSource->SetPoint1(coords);
		m_profileLineStartPointSource->SetCenter(coords);
		m_profileLineMapper->Update();
		m_profileLineStartPointMapper->Update();
	}
	else
	{
		m_profileLineSource->SetPoint2(coords);
		m_profileLineEndPointSource->SetCenter(coords);
		m_profileLineMapper->Update();
		m_profileLineEndPointMapper->Update();
	}
	update();
}

void iARenderer::setArbitraryProfileOn(bool isOn)
{
	m_profileLineActor->SetVisibility(isOn);
	m_profileLineStartPointActor->SetVisibility(isOn);
	m_profileLineEndPointActor->SetVisibility(isOn);
}

void iARenderer::InitObserver()
{
	renderObserver = iARenderObserver::New(ren, labelRen, interactor, pointPicker,
		moveableAxesTransform, imageData,
		plane1, plane2, plane3, cellLocator);

	interactor->AddObserver(vtkCommand::KeyPressEvent, renderObserver, 0.0);
	interactor->AddObserver(vtkCommand::LeftButtonPressEvent, renderObserver);
	interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, renderObserver);
	interactor->AddObserver(vtkCommand::RightButtonPressEvent, renderObserver);
	interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, renderObserver);
}

void iARenderer::setPolyData(vtkPolyData* pd)
{
	polyData = pd;
	if (polyData)
	{
		cellLocator->SetDataSet(polyData);
		if (polyData->GetNumberOfCells())
			cellLocator->BuildLocator();
	}
	polyMapper->SetInputData( polyData );
}

vtkPolyData* iARenderer::getPolyData()
{
	return polyData;
}

void iARenderer::disableInteractor() { interactor->Disable(); }
void iARenderer::enableInteractor() { interactor->ReInitialize(); }

vtkTransform* iARenderer::getCoordinateSystemTransform() { moveableAxesTransform->Update(); return moveableAxesTransform; }

void iARenderer::AddRenderer(vtkRenderer* renderer)
{
	renWin->AddRenderer(renderer);
}

vtkPlane* iARenderer::getPlane1() { return plane1; };
vtkPlane* iARenderer::getPlane2() { return plane2; };
vtkPlane* iARenderer::getPlane3() { return plane3; };
vtkOpenGLRenderer * iARenderer::GetRenderer() { return ren; };
vtkActor* iARenderer::GetPolyActor() { return polyActor; };
vtkOpenGLRenderer * iARenderer::GetLabelRenderer(void) { return labelRen; }
vtkPolyDataMapper* iARenderer::GetPolyMapper() const { return polyMapper; }
vtkTextActor* iARenderer::GetTxtActor() { return txtActor; }

//spacing is an array of 3 
void iARenderer::setSlicingBounds(const int roi[6], const double * spacing)
{
	double x_min, xMax, yMin, yMax, zMin, zMax; 
	/* roi[6]:  xmin, xsize, ymin, ysize, zmin, zSize;
	*	roi[0] : x; roi[1]: y, roi[2]-> z; roy[3] -> xzise, roi[4] ysize, roi[5] -> zSize
	*/
	
	x_min = roi[0] *  spacing[0]; yMin = roi[1] * spacing[1]; zMin = roi[2] *spacing[2];
	xMax = x_min + roi[3] * spacing[0]; 
	yMax = yMin + roi[4] * spacing[1]; 
	zMax = zMin + roi[5] * spacing[2];
	m_slicingCube->SetBounds(x_min, xMax, yMin, yMax, zMin, zMax);
	this->update(); 

}

void iARenderer::setCubeVisible(bool visible)
{
	m_sliceCubeActor->SetVisibility(visible);
}

void iARenderer::setSlicePlane(int planeID, double originX, double originY, double originZ)
{
	switch (planeID)
	{
		default: // note: switch case labels are currently NOT equal to iASlicerMode numbers, see note there!
		case 0: plane1->SetOrigin(originX, originY, originZ); break; // YZ
		case 1: plane2->SetOrigin(originX, originY, originZ); break; // XZ
		case 2: plane3->SetOrigin(originX, originY, originZ); break; // YZ
	}
	double center[3];
	m_slicePlaneSource[planeID]->GetCenter(center);
	center[planeID] = (planeID == 0) ? originX : ((planeID == 1) ? originY : originZ);
	m_slicePlaneSource[planeID]->SetCenter(center);
	m_slicePlaneMapper[planeID]->Update();
	update();
}

void iARenderer::ApplySettings(iARenderSettings & settings)
{
#if (VTK_MAJOR_VERSION > 7 || (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION > 0))
	ren->SetUseFXAA(settings.UseFXAA);
#else
	DEBUG_LOG("FXAA Anti-Aliasing is not support with your VTK version");
#endif
	cam->SetParallelProjection(settings.ParallelProjection);
	QColor bgTop(settings.BackgroundTop);
	QColor bgBottom(settings.BackgroundBottom);
	if (!bgTop.isValid()) {
		bgTop.setRgbF(0.5, 0.666666666666666667, 1.0);
		settings.BackgroundTop = bgTop.name();
	}
	if (!bgBottom.isValid()) {
		bgBottom.setRgbF(1.0, 1.0, 1.0);
		settings.BackgroundBottom = bgTop.name();
	}
	
	setSlicePlaneOpacity(settings.PlaneOpacity);

	ren->SetBackground2(bgTop.redF(), bgTop.greenF(), bgTop.blueF());
	ren->SetBackground(bgBottom.redF(), bgBottom.greenF(), bgBottom.blueF());
	showHelpers(settings.ShowHelpers);
	showRPosition(settings.ShowRPosition);
	showSlicePlanes(settings.ShowSlicePlanes);
	renWin->Render();
}

void iARenderer::emitSelectedCells(vtkUnstructuredGrid* selectedCells)
{
	double cell[DIM] = { 0,0,0 };
	double* spacing = getRenderObserver()->GetImageData()->GetSpacing();
	auto selCellPoints = vtkSmartPointer<vtkPoints>::New();
	for (vtkIdType i = 0; i < selectedCells->GetNumberOfCells(); ++i)
	{
		GetCellCenter(selectedCells, i, cell, spacing);
		selCellPoints->InsertNextPoint(cell);
	}
	emit cellsSelected(selCellPoints);
}

void iARenderer::emitNoSelectedCells()
{
	emit noCellsSelected();
}