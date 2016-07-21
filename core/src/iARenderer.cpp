/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iARenderer.h"

#include "defines.h"
#include "iAChannelID.h"
#include "iAChannelVisualizationData.h"
#include "iAMovieHelper.h"
#include "iAObserverProgress.h"
#include "iARenderObserver.h"

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
#include <vtkOutlineFilter.h>
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

#include <QApplication>
#include <QDateTime>
#include <QImage>
#include <QLocale>
#include <QApplication>
#include <QImage>


iARenderer::iARenderer(QObject *par)  :  QObject( par ),
	interactor(0)
{
	parent = (QWidget*)par;
	labelRen = vtkOpenGLRenderer::New();

	renWin = vtkGenericOpenGLRenderWindow::New();
	renWin->AlphaBitPlanesOn();
	renWin->LineSmoothingOn();
	renWin->PointSmoothingOn();

	cam = vtkSmartPointer<vtkCamera>::New();

	interactorStyle = vtkInteractorStyleSwitch::New();
	outlineFilter = vtkOutlineFilter::New();
	outlineMapper = vtkPolyDataMapper::New();
	outlineActor = vtkActor::New();

	cSource = vtkCubeSource::New();
	cMapper = vtkPolyDataMapper::New();
	cActor = vtkActor::New();

	rep = vtkLogoRepresentation::New();
	logowidget = vtkLogoWidget::New();
	image1 = vtkQImageToImageSource::New();

	ren = vtkOpenGLRenderer::New();

	polyMapper = vtkPolyDataMapper::New();
	polyActor = vtkActor::New();

	annotatedCubeActor = vtkAnnotatedCubeActor::New();
	axesActor = vtkAxesActor::New();
	moveableAxesActor = vtkAxesActor::New();
	orientationMarkerWidget = vtkOrientationMarkerWidget::New();

	pointPicker = vtkPicker::New();
	renderObserver = NULL;
	observerFPProgress = iAObserverProgress::New();
	observerGPUProgress = iAObserverProgress::New();

	plane1 = vtkPlane::New();
	plane2 = vtkPlane::New();
	plane3 = vtkPlane::New();

	cellLocator = vtkCellLocator::New();

	// mobject image members initialize
	volumeHighlight = vtkVolume::New();
	volumePropertyHighlight = vtkVolumeProperty::New();
	highlightMode = false;
	meanObjectSelected = false;
	meanObjectHighlighted = false;
}

iARenderer::~iARenderer(void)
{
	ren->RemoveAllObservers();
	renWin->RemoveAllObservers();
	//multiChannelImageData->Delete();
	pointPicker->Delete();
	if (renderObserver) renderObserver->Delete();
	observerFPProgress->Delete();
	observerGPUProgress->Delete();

	plane1->Delete();
	plane2->Delete();
	plane3->Delete();

	cSource->Delete();
	cMapper->Delete();
	cActor->Delete();

	orientationMarkerWidget	->Delete();
	axesActor->Delete();
	moveableAxesActor->Delete();

	outlineActor->Delete();
	outlineMapper->ReleaseDataFlagOn();
	outlineMapper->Delete();
	outlineFilter->ReleaseDataFlagOn();
	outlineFilter->Delete();

	interactorStyle->Delete();

	rep->Delete();
	logowidget->Delete();
	image1->Delete();

	renWin->Delete();

	ren->Delete();
	labelRen->Delete();
	cellLocator->Delete();
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
	labelRen->SetLayer(1);
	labelRen->InteractiveOff();
	labelRen->UseDepthPeelingOn();
	renWin->SetNumberOfLayers(5);
	renWin->AddRenderer(ren);
	renWin->AddRenderer(labelRen);
	renWin->LineSmoothingOn();
	pointPicker->SetTolerance(0.00005);//spacing[0]/150);
	interactor = renWin->GetInteractor();
	interactor->SetPicker(pointPicker);
	interactorStyle->SetCurrentStyleToTrackballCamera();
	interactor->SetInteractorStyle(interactorStyle);
	InitObserver();

	QImage img;
	if( QDate::currentDate().dayOfYear() >= 340 )img.load(":/images/Xmas.png");
	else img.load(":/images/fhlogo.png");

	image1->SetQImage(&img);
	image1->Update();
	rep->SetImage(image1->GetOutput( ));
	logowidget->SetInteractor(interactor);
	logowidget->SetRepresentation(rep);
	logowidget->SetResizable(false);
	logowidget->SetSelectable(false);
	logowidget->On();

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

	outlineFilter->SetInputData(ds);
	polyMapper->SetInputData(polyData);

	renderObserver->ReInitialize( ren, labelRen, interactor, pointPicker,
		axesTransform, ds,
		plane1, plane2, plane3, cellLocator );
	interactor->ReInitialize();

	emit reInitialized();

	update();
}

/*
// TODO: VOLUME: check MObjects, at least those used in FiberScout, how to adapt them to use new stuff!
void iARenderer::setTransferFunctions( vtkPiecewiseFunction* opacityTFHighlight, vtkColorTransferFunction* colorTFHighlight, vtkPiecewiseFunction* opacityTFTransparent, vtkColorTransferFunction* colorTFTransparent )
{
	piecewiseFunctionHighlight = opacityTFHighlight;
	colorTransferFunctionHighlight = colorTFHighlight;
	piecewiseFunctionTransparent = opacityTFTransparent;
	colorTransferFunctionTransparent = colorTFTransparent;

	update();
}


void iARenderer::initializeHighlight( vtkImageData* ds, vtkPiecewiseFunction* otfHighlight, vtkColorTransferFunction* ctfHighlight, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf )
{
	imageDataHighlight = ds;
	piecewiseFunctionHighlight = otfHighlight;
	colorTransferFunctionHighlight = ctfHighlight;

	piecewiseFunction = otf;
	colorTransferFunction = ctf;

	highlightMode = true;

	volumeProperty->SetColor(0, colorTransferFunction);
	volumeProperty->SetScalarOpacity(0, piecewiseFunction);

	volumePropertyHighlight->SetColor(colorTransferFunctionHighlight);
	volumePropertyHighlight->SetScalarOpacity(piecewiseFunctionHighlight);
	volumePropertyHighlight->SetAmbient(volumeProperty->GetAmbient());
	volumePropertyHighlight->SetDiffuse(volumeProperty->GetDiffuse());
	volumePropertyHighlight->SetSpecular(volumeProperty->GetSpecular());
	volumePropertyHighlight->SetSpecularPower(volumeProperty->GetSpecularPower());
	volumePropertyHighlight->SetInterpolationType(volumeProperty->GetInterpolationType()); 
	volumePropertyHighlight->SetShade(volumeProperty->GetShade());

	volumeMapper->SetInputData(imageDataHighlight);
	volumeMapper->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->observerFPProgress);
	volumeHighlight->SetMapper(volumeMapper);
	volumeHighlight->SetProperty(volumePropertyHighlight);

	volumeHighlight->SetPosition((imageData->GetDimensions()[0]-imageDataHighlight->GetDimensions()[0])/2*10-10,
		(imageData->GetDimensions()[1]-imageDataHighlight->GetDimensions()[1])/2*10-10,
		(imageData->GetDimensions()[2]-imageDataHighlight->GetDimensions()[2])/2*10-10);

	volumeHighlight->Update();

	ren->AddVolume(volumeHighlight);

	update();
}

void iARenderer::reInitializeHighlight( vtkImageData* ds, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf )
{
	highlightMode = true;

	imageDataHighlight = ds;
	piecewiseFunctionHighlight = otf;
	colorTransferFunctionHighlight = ctf;

	volumePropertyHighlight->SetColor(colorTransferFunctionHighlight);
	volumePropertyHighlight->SetScalarOpacity(piecewiseFunctionHighlight);

	volumeMapper->SetInputData(imageDataHighlight);
	volumeHighlight->SetMapper(volumeMapper);
	volumeHighlight->SetProperty(volumePropertyHighlight);

	ren->AddVolume(volumeHighlight);

	update();
}

void iARenderer::visualizeHighlight( bool enabled )
{
	if(enabled)
	{
		ren->AddVolume(volumeHighlight);
	}
	else
	{
		ren->RemoveVolume(volumeHighlight);
	}
}
*/

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
	outlineFilter->SetInputData(ds);
	outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
	outlineActor->GetProperty()->SetColor(0,0,0);
	outlineActor->PickableOff();
	outlineActor->SetMapper(outlineMapper);

	polyMapper->SetInputData(polyData);
	polyMapper->SelectColorArray("Colors");
	polyMapper->SetScalarModeToUsePointFieldData();
	polyActor->SetMapper(polyMapper);

	ren->GradientBackgroundOn();
	ren->AddActor(polyActor);
	ren->AddActor(cActor);
	ren->AddActor(axesActor);
	ren->AddActor(moveableAxesActor);
	ren->AddActor(outlineActor);
	emit onSetupRenderer();
}

void iARenderer::update()
{
	// TODO: VOLUME: hook here and update all currently added volumes? is volumeMapper / volume update even necessary?
	polyMapper->Update();
	renWin->Render();
}

void iARenderer::showHelpers(bool show)
{
	orientationMarkerWidget->SetEnabled(show);
	axesActor->SetVisibility(show);
	moveableAxesActor->SetVisibility(show);
	logowidget->SetEnabled(show);
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
	renderObserver = RenderObserver::New(ren, labelRen, interactor, pointPicker,
		axesTransform, imageData,
		plane1, plane2, plane3, cellLocator);

	interactor->AddObserver(vtkCommand::KeyPressEvent, renderObserver);
	interactor->AddObserver(vtkCommand::LeftButtonPressEvent, renderObserver);
	//There is a VTK bug, observer does not catch mouse release events!
	// workaround using QVTKWidgetMouseReleaseWorkaround used
	interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, renderObserver);
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
