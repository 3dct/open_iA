/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "pch.h"
#include "iARenderer.h"

#include "defines.h"
#include "iAChannelID.h"
#include "iAConsole.h"
#include "iAChannelVisualizationData.h"
#include "iAMovieHelper.h"
#include "iAObserverProgress.h"
#include "iARenderObserver.h"
#include "iARenderSettings.h"

#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellLocator.h>
#include <vtkColorTransferFunction.h>
#include <vtkCubeSource.h>
#include <vtkGenericMovieWriter.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageBlend.h>
#include <vtkImageData.h>
#include <vtkImageCast.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPicker.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQImageToImageSource.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTransform.h>
#include <vtkVolumeProperty.h>
#include <vtkWindowToImageFilter.h>

//
#include <vtkAreaPicker.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkCallbackCommand.h>
#include <vtkExtractSelectedFrustum.h>
#include <vtkDataSetMapper.h>
#include <vtkUnstructuredGrid.h>
#include <qcolor.h>
#include <vtkRendererCollection.h>
#include <vtkObjectFactory.h>
#include <vtkAppendFilter.h>
//

#include <QApplication>
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
			if (this->CurrentMode == VTKISRBP_ORIENT)
				this->CurrentMode = VTKISRBP_SELECT;
			else
				this->CurrentMode = VTKISRBP_ORIENT;
			break;
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

void PickCallbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
	void* clientData, void* vtkNotUsed(callData))
{
	//TODO: pick event (add to prev selection) + only visible cells (slicer)?
	vtkAreaPicker *areaPicker = static_cast<vtkAreaPicker*>(caller);
	iARenderer *ren = static_cast<iARenderer*>(clientData);
	ren->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(ren->selectedActor);

	auto extractSelection = vtkSmartPointer<vtkExtractSelectedFrustum>::New();
	extractSelection->SetInputData(0, ren->getRenderObserver()->GetImageData());
	extractSelection->PreserveTopologyOff();
	extractSelection->SetFrustum(areaPicker->GetFrustum());
	extractSelection->Update();

	if (!extractSelection->GetOutput()->GetNumberOfElements(vtkUnstructuredGrid::CELL))
		return;
	
	if (ren->GetInteractor()->GetControlKey() &&
		!ren->GetInteractor()->GetShiftKey())
	{
		auto append = vtkSmartPointer<vtkAppendFilter>::New();
		append->AddInputData(ren->finalSelection);
		append->AddInputData(extractSelection->GetOutput());
		append->Update();
		ren->finalSelection->ShallowCopy(append->GetOutput());
	}
	else if (ren->GetInteractor()->GetControlKey() &&
		ren->GetInteractor()->GetShiftKey())
	{
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
	interactor(0)
{
	renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();		// TODO: move out of here?
	renWin->AlphaBitPlanesOn();
	renWin->LineSmoothingOn();
	renWin->PointSmoothingOn();

	cam = vtkSmartPointer<vtkCamera>::New();
	interactorStyle = vtkSmartPointer<vtkInteractorStyleSwitch>::New();

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

	pointPicker = vtkSmartPointer<vtkPicker>::New();
	renderObserver = NULL;

	plane1 = vtkSmartPointer<vtkPlane>::New();
	plane2 = vtkSmartPointer<vtkPlane>::New();
	plane3 = vtkSmartPointer<vtkPlane>::New();

	cellLocator = vtkSmartPointer<vtkCellLocator>::New();

	finalSelection = vtkSmartPointer<vtkUnstructuredGrid>::New();
	selectedMapper = vtkSmartPointer<vtkDataSetMapper>::New();
	selectedMapper->SetScalarModeToUseCellData();
	selectedMapper->SetInputData(finalSelection);
	selectedActor = vtkSmartPointer<vtkActor>::New();
	QColor c(254, 153, 41);	//  // Selection color: orange
	selectedActor->SetMapper(selectedMapper);
	selectedActor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
	selectedActor->GetProperty()->SetRepresentationToWireframe();
	selectedActor->GetProperty()->EdgeVisibilityOn();
	selectedActor->GetProperty()->SetEdgeColor(c.redF(), c.greenF(), c.blueF());
	selectedActor->GetProperty()->SetLineWidth(3);
}

iARenderer::~iARenderer(void)
{
	ren->RemoveAllObservers();
	renWin->RemoveAllObservers();

	if (renderObserver) renderObserver->Delete();
}

void iARenderer::initialize( vtkImageData* ds, vtkPolyData* pd, int e )
{
	auto areaPicker = vtkSmartPointer<vtkAreaPicker>::New();

	imageData = ds;
	polyData = pd;
	cellLocator->SetDataSet(polyData);
	if(polyData)
		if( polyData->GetNumberOfCells() )
			cellLocator->BuildLocator();
	ext = e;
	double spacing[3];	ds->GetSpacing(spacing);
	ren->SetLayer(0);
	labelRen->SetLayer(1);
	labelRen->InteractiveOff();
	labelRen->UseDepthPeelingOn();
	renWin->SetNumberOfLayers(5);
	renWin->AddRenderer(ren);
	renWin->AddRenderer(labelRen);
	pointPicker->SetTolerance(0.00005);//spacing[0]/150);
	interactor = renWin->GetInteractor();
	
	// TODO: implement switch between vtkInteractorStyleSwitch and MouseInteractorStyle
	//interactor->SetPicker(pointPicker);
	interactor->SetPicker(areaPicker);
	auto style = vtkSmartPointer<MouseInteractorStyle>::New();
	//interactorStyle->SetCurrentStyleToTrackballCamera();
	//interactor->SetInteractorStyle(interactorStyle);
	interactor->SetInteractorStyle(style);

	auto pickCallback = vtkSmartPointer<vtkCallbackCommand>::New();
	pickCallback->SetCallback(PickCallbackFunction);
	pickCallback->SetClientData(this);
	areaPicker->AddObserver(vtkCommand::EndPickEvent, pickCallback, 1.0);
	
	InitObserver();

	QImage img;
	if( QDate::currentDate().dayOfYear() >= 340 )img.load(":/images/Xmas.png");
	else img.load(":/images/fhlogo.png");

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
	setupRenderer(ds);

	labelRen->SetActiveCamera(cam);
	ren->SetActiveCamera(cam);
	setCamPosition( 0,0,1, 1,1,1 ); // +Z
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
		axesTransform, ds,
		plane1, plane2, plane3, cellLocator );
	interactor->ReInitialize();

	emit reInitialized();

	update();
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

	axesTransform->Scale(spacing[0]*3, spacing[1]*3, spacing[2]*3);

	moveableAxesActor->SetUserTransform(axesTransform);
}

void iARenderer::setupOrientationMarker()
{
	orientationMarkerWidget->SetOrientationMarker(annotatedCubeActor);
	orientationMarkerWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
	orientationMarkerWidget->SetInteractor(interactor);
	orientationMarkerWidget->SetEnabled( 1 );
	orientationMarkerWidget->InteractiveOff();
}

void iARenderer::setupRenderer(vtkImageData* ds)
{
	polyMapper->SetInputData(polyData);
	polyMapper->SelectColorArray("Colors");
	polyMapper->SetScalarModeToUsePointFieldData();
	polyActor->SetMapper(polyMapper);

	ren->GradientBackgroundOn();
	ren->AddActor(polyActor);
	ren->AddActor(cActor);
	ren->AddActor(axesActor);
	ren->AddActor(moveableAxesActor);
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

	if (movieWriter.GetPointer() == NULL)
		return;

	interactor->Disable();

	vtkSmartPointer<vtkWindowToImageFilter> w2if = vtkSmartPointer<vtkWindowToImageFilter>::New();
	int* rws = renWin->GetSize();
	if (rws[0] % 2 != 0) rws[0]++;
	if (rws[1] % 2 != 0) rws[1]++;
	renWin->SetSize(rws);
	renWin->Render();

	w2if->SetInput(renWin);
	w2if->ReadFrontBufferOff();

	movieWriter->SetInputConnection(w2if->GetOutputPort());
	movieWriter->SetFileName(fileName.toLatin1().data());
	movieWriter->Start();

	int i;
	int* extent = imageData->GetExtent();

	emit msg(tr("%1  MOVIE export started. Output: %2").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat), fileName));

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
	for ( i =0; i < numRenderings; i++ ) {
		ren->ResetCamera();
		renWin->Render();

		w2if->Modified();
		movieWriter->Write();
		if (movieWriter->GetError()) { 
			emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode())); 
			break;
		}
		emit progress( 100 * (i+1) / (extent[1]-extent[0]));
		cam->ApplyTransform(rot);
	}

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

void iARenderer::InitObserver()
{
	renderObserver = iARenderObserver::New(ren, labelRen, interactor, pointPicker,
		axesTransform, imageData,
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

vtkTransform* iARenderer::getCoordinateSystemTransform() { axesTransform->Update(); return axesTransform; }
void iARenderer::GetImageDataBounds(double bounds[6]) { imageData->GetBounds(bounds); }

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

void iARenderer::ApplySettings(iARenderSettings & settings)
{
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
	ren->SetBackground(bgTop.redF(), bgTop.greenF(), bgTop.blueF());
	ren->SetBackground2(bgBottom.redF(), bgBottom.greenF(), bgBottom.blueF());
	showHelpers(settings.ShowHelpers);
	showRPosition(settings.ShowRPosition);
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