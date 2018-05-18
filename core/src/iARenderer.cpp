/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iARenderer.h"

#include "defines.h"
#include "iAChannelID.h"
#include "iAConsole.h"
#include "iAChannelVisualizationData.h"
#include "iAMovieHelper.h"
#include "iARenderObserver.h"
#include "iARenderSettings.h"
#include "mdichild.h"

#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellLocator.h>
#include <vtkCubeSource.h>
#include <vtkGenericMovieWriter.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkImageData.h>
#include <vtkImageCast.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkLineSource.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPicker.h>
#include <vtkPlane.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQImageToImageSource.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
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

	m_profileLineSource = vtkSmartPointer<vtkLineSource>::New();
	m_profileLineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_profileLineActor = vtkSmartPointer<vtkActor>::New();
	m_profileLineStartPointSource = vtkSmartPointer<vtkSphereSource>::New();
	m_profileLineStartPointMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_profileLineStartPointActor = vtkSmartPointer<vtkActor>::New();
	m_profileLineEndPointSource = vtkSmartPointer<vtkSphereSource>::New();
	m_profileLineEndPointMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_profileLineEndPointActor = vtkSmartPointer<vtkActor>::New();

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
	pointPicker->SetTolerance(0.00005);//spacing[0]/150);
	interactor = renWin->GetInteractor();
	interactor->SetPicker(pointPicker);
	interactorStyle->SetCurrentStyleToTrackballCamera();
	interactor->SetInteractorStyle(interactorStyle);
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
	setArbitraryProfileOn(false);
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

void iARenderer::setupRenderer()
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
	ren->AddActor(m_profileLineActor);
	ren->AddActor(m_profileLineStartPointActor);
	ren->AddActor(m_profileLineEndPointActor);
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
	movieWriter->SetFileName(fileName.toLatin1().data());
	movieWriter->Start();

	emit msg(tr("%1  MOVIE export started. Output: %2").arg(QLocale()
		.toString(QDateTime::currentDateTime(), QLocale::ShortFormat), fileName));

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
		axesTransform, imageData,
		plane1, plane2, plane3, cellLocator);

	interactor->AddObserver(vtkCommand::KeyPressEvent, renderObserver);
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
	ren->SetBackground2(bgTop.redF(), bgTop.greenF(), bgTop.blueF());
	ren->SetBackground(bgBottom.redF(), bgBottom.greenF(), bgBottom.blueF());
	showHelpers(settings.ShowHelpers);
	showRPosition(settings.ShowRPosition);
}
