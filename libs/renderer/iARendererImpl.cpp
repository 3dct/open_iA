// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARendererImpl.h"

#include "defines.h"
#include "iAAbortListener.h"
#include "iALog.h"
#include "iAJobListView.h"
#include "iALineSegment.h"
#include "iAMovieHelper.h"
#include "iAProfileColors.h"
#include "iAProgress.h"
#include "iARenderObserver.h"
#include "iARenderSettings.h"
#include "iASlicerMode.h"
#include <iAStringHelper.h>
#include "iAToolsVTK.h"    // for setCamPos

#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkAppendFilter.h>
#include <vtkAreaPicker.h>
#include <vtkAxesActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCubeSource.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractSelectedFrustum.h>
#include <vtkGenericMovieWriter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPicker.h>
#include <vtkPlane.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
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
#include <QImage>

#include <cassert>

#define VTKISRBP_ORIENT 0
#define VTKISRBP_SELECT 1

namespace
{
	const int NumOfProfileLines = 7;
	const double IndicatorsLenMultiplier = 1.3;
}


iARendererImpl::iARendererImpl(QObject* parent, vtkGenericOpenGLRenderWindow* renderWindow): iARenderer(parent),
	m_initialized(false),
	m_renderObserver(nullptr),
	m_renWin(renderWindow),
	m_interactor(renderWindow->GetInteractor()),
	m_ren(vtkSmartPointer<vtkOpenGLRenderer>::New()),
	m_labelRen(vtkSmartPointer<vtkOpenGLRenderer>::New()),
	m_cam(vtkSmartPointer<vtkCamera>::New()),
	m_txtActor(vtkSmartPointer<vtkTextActor>::New()),
	m_cSource(vtkSmartPointer<vtkCubeSource>::New()),
	m_cMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_cActor(vtkSmartPointer<vtkActor>::New()),
	m_annotatedCubeActor(vtkSmartPointer<vtkAnnotatedCubeActor>::New()),
	m_axesActor(vtkSmartPointer<vtkAxesActor>::New()),
	m_orientationMarkerWidget(vtkSmartPointer<vtkOrientationMarkerWidget>::New()),
	m_plane1(vtkSmartPointer<vtkPlane>::New()),
	m_plane2(vtkSmartPointer<vtkPlane>::New()),
	m_plane3(vtkSmartPointer<vtkPlane>::New()),
	m_moveableAxesActor(vtkSmartPointer<vtkAxesActor>::New()),
	m_profileLineStartPointSource(vtkSmartPointer<vtkSphereSource>::New()),
	m_profileLineStartPointMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_profileLineStartPointActor(vtkSmartPointer<vtkActor>::New()),
	m_profileLineEndPointSource(vtkSmartPointer<vtkSphereSource>::New()),
	m_profileLineEndPointMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_profileLineEndPointActor(vtkSmartPointer<vtkActor>::New()),
	m_slicePlaneOpacity(0.8),
	m_roiCube(vtkSmartPointer<vtkCubeSource>::New()),
	m_roiMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_roiActor(vtkSmartPointer<vtkActor>::New()),
	m_stickOutBox{ iAVec3d(0.0, 0.0, 0.0), iAVec3d(0.0, 0.0, 0.0) },
	m_touchStartScale(1.0)
{
	// fill m_profileLine; cannot do it via  m_profileLine(NumOfProfileLines, iALineSegment()),
	// since that would insert copies of one iALineSegment, meaning all would reference the same vtk objects...
	for (int n = 0; n < NumOfProfileLines; ++n)
	{
		m_profileLine.push_back(iALineSegment());
	}
	m_ren->SetLayer(0);
	m_labelRen->SetLayer(1);
	m_labelRen->InteractiveOff();
	m_labelRen->UseDepthPeelingOn();

	m_renWin->SetAlphaBitPlanes(true);
	m_renWin->LineSmoothingOn();
	m_renWin->PointSmoothingOn();
	m_renWin->SetNumberOfLayers(5);
	m_renWin->AddRenderer(m_ren);
	m_renWin->AddRenderer(m_labelRen);

	// interaction mode indicator:
	m_txtActor->SetInput("Selection mode");
	m_txtActor->GetTextProperty()->SetFontSize(24);
	m_txtActor->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
	m_txtActor->GetTextProperty()->SetJustificationToLeft();
	m_txtActor->GetTextProperty()->SetVerticalJustificationToBottom();
	m_txtActor->VisibilityOff();
	m_txtActor->SetPickable(false);
	m_txtActor->SetDragable(false);

	// slice plane display:
	for (int s = 0; s < 3; ++s)
	{
		m_slicePlaneSource[s] = vtkSmartPointer<vtkCubeSource>::New();
		//m_slicePlaneSource[s]->SetOutputPointsPrecision(10);
		m_slicePlaneMapper[s] = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_slicePlaneMapper[s]->SetInputConnection(m_slicePlaneSource[s]->GetOutputPort());
		m_slicePlaneMapper[s]->Update();
		m_slicePlaneActor[s] = vtkSmartPointer<vtkActor>::New();
		m_slicePlaneActor[s]->GetProperty()->LightingOff();
		m_slicePlaneActor[s]->SetPickable(false);
		m_slicePlaneActor[s]->SetDragable(false);
		m_slicePlaneActor[s]->SetMapper(m_slicePlaneMapper[s]);
		m_slicePlaneActor[s]->GetProperty()->SetColor((s == 0) ? 1 : 0, (s == 1) ? 1 : 0, (s == 2) ? 1 : 0);
		m_slicePlaneActor[s]->GetProperty()->SetOpacity(1.0);
	}
	// set up cutting planes:
	m_plane1->SetNormal(1, 0, 0);
	m_plane2->SetNormal(0, 1, 0);
	m_plane3->SetNormal(0, 0, 1);

	m_labelRen->SetActiveCamera(m_cam);
	m_ren->SetActiveCamera(m_cam);
	::setCamPosition(m_cam, static_cast<iACameraPosition>(iACameraPosition::Iso));

	// set up region of interest (ROI) cube marker:
	m_roiMapper->SetInputConnection(m_roiCube->GetOutputPort());
	m_roiActor->SetMapper(m_roiMapper);
	m_roiActor->GetProperty()->SetColor(1.0, 0, 0);
	m_roiActor->GetProperty()->SetRepresentationToWireframe();
	m_roiActor->GetProperty()->SetOpacity(1);
	m_roiActor->GetProperty()->SetLineWidth(2.3);
	m_roiActor->GetProperty()->SetAmbient(1.0);
	m_roiActor->GetProperty()->SetDiffuse(0.0);
	m_roiActor->GetProperty()->SetSpecular(0.0);
	m_roiActor->SetPickable(false);
	m_roiActor->SetDragable(false);
	m_roiActor->SetVisibility(false);
	
	// set up annotated axis cube:
	m_annotatedCubeActor->SetPickable(1);
	m_annotatedCubeActor->SetXPlusFaceText("+X");
	m_annotatedCubeActor->SetXMinusFaceText("-X");
	m_annotatedCubeActor->SetYPlusFaceText("+Y");
	m_annotatedCubeActor->SetYMinusFaceText("-Y");
	m_annotatedCubeActor->SetZPlusFaceText("+Z");
	m_annotatedCubeActor->SetZMinusFaceText("-Z");
	m_annotatedCubeActor->SetXFaceTextRotation(0);
	m_annotatedCubeActor->SetYFaceTextRotation(0);
	m_annotatedCubeActor->SetZFaceTextRotation(90);
	m_annotatedCubeActor->SetFaceTextScale(0.45);
	m_annotatedCubeActor->GetCubeProperty()->SetColor(0.7, 0.78, 1);
	m_annotatedCubeActor->GetTextEdgesProperty()->SetDiffuse(0);
	m_annotatedCubeActor->GetTextEdgesProperty()->SetAmbient(0);
	m_annotatedCubeActor->GetXPlusFaceProperty()->SetColor(1, 0, 0);
	m_annotatedCubeActor->GetXPlusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetXMinusFaceProperty()->SetColor(1, 0, 0);
	m_annotatedCubeActor->GetXMinusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetYPlusFaceProperty()->SetColor(0, 1, 0);
	m_annotatedCubeActor->GetYPlusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetYMinusFaceProperty()->SetColor(0, 1, 0);
	m_annotatedCubeActor->GetYMinusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetZPlusFaceProperty()->SetColor(0, 0, 1);
	m_annotatedCubeActor->GetZPlusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetZMinusFaceProperty()->SetColor(0, 0, 1);
	m_annotatedCubeActor->GetZMinusFaceProperty()->SetInterpolationToFlat();

	// set up position marker:
	m_cSource->SetXLength(1);
	m_cSource->SetYLength(1);
	m_cSource->SetZLength(1);
	m_cMapper->SetInputConnection(m_cSource->GetOutputPort());
	m_cActor->SetMapper(m_cMapper);
	m_cActor->GetProperty()->SetColor(1, 0, 0);
	m_cActor->SetPickable(false);
	m_cActor->SetDragable(false);

	m_axesActor->AxisLabelsOff();
	m_axesActor->SetShaftTypeToCylinder();
	m_axesActor->SetTotalLength(1, 1, 1);
	m_moveableAxesActor->AxisLabelsOff();
	m_moveableAxesActor->SetShaftTypeToCylinder();
	m_moveableAxesActor->SetTotalLength(1, 1, 1);
	m_moveableAxesActor->SetPickable(false);
	m_moveableAxesActor->SetDragable(false);

	// add actors of helpers to renderer:
	m_ren->GradientBackgroundOn();
	m_ren->AddActor2D(m_txtActor);
	m_ren->AddActor(m_cActor);
	m_ren->AddActor(m_axesActor);
	m_ren->AddActor(m_moveableAxesActor);
	for (int i = 0; i < NumOfProfileLines; ++i)
	{
		m_ren->AddActor(m_profileLine[i].actor);
	}
	m_ren->AddActor(m_profileLineStartPointActor);
	m_ren->AddActor(m_profileLineEndPointActor);
	m_ren->AddActor(m_roiActor);

	// Set up orientation marker widget:
	m_orientationMarkerWidget->SetOrientationMarker(m_annotatedCubeActor);
	m_orientationMarkerWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
	m_orientationMarkerWidget->SetInteractor(m_interactor);
	m_orientationMarkerWidget->SetEnabled(1);
	m_orientationMarkerWidget->InteractiveOff();

	m_renderObserver = new iARenderObserver(m_ren, m_interactor, m_moveableAxesTransform, m_plane1, m_plane2, m_plane3);
	m_interactor->AddObserver(vtkCommand::KeyPressEvent, m_renderObserver, 0.0);
	m_interactor->AddObserver(vtkCommand::LeftButtonPressEvent, m_renderObserver);
	m_interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, m_renderObserver);
	m_interactor->AddObserver(vtkCommand::RightButtonPressEvent, m_renderObserver);
	m_interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, m_renderObserver);

	setDefaultInteractor();

	for (int i = 0; i < NumOfProfileLines; ++i)
	{
		m_profileLine[i].actor->SetPickable(false);
		m_profileLine[i].actor->SetDragable(false);
		QColor color = (i == 0) ? ProfileLineColor : (i > 3) ? ProfileEndColor : ProfileStartColor;
		m_profileLine[i].actor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
		m_profileLine[i].actor->GetProperty()->SetLineWidth(2.0);
		m_profileLine[i].actor->GetProperty()->SetLineStipplePattern(0x00ff);//0xf0f0
		m_profileLine[i].actor->GetProperty()->SetLineStippleRepeatFactor(1);
		m_profileLine[i].actor->GetProperty()->SetPointSize(2);
	}
	m_profileLineStartPointMapper->SetInputConnection(m_profileLineStartPointSource->GetOutputPort());
	m_profileLineStartPointActor->SetMapper(m_profileLineStartPointMapper);
	m_profileLineStartPointActor->SetPickable(false);
	m_profileLineStartPointActor->SetDragable(false);
	m_profileLineEndPointMapper->SetInputConnection(m_profileLineEndPointSource->GetOutputPort());
	m_profileLineEndPointActor->SetMapper(m_profileLineEndPointMapper);
	m_profileLineEndPointActor->SetPickable(false);
	m_profileLineEndPointActor->SetDragable(false);
	m_profileLineStartPointActor->GetProperty()->SetColor(ProfileStartColor.redF(), ProfileStartColor.greenF(), ProfileStartColor.blueF());
	m_profileLineEndPointActor->GetProperty()->SetColor(ProfileEndColor.redF(), ProfileEndColor.greenF(), ProfileEndColor.blueF());
	setProfileHandlesOn(false);
	m_initialized = true;
}

iARendererImpl::~iARendererImpl(void)
{
	if (m_renderObserver)
	{
		m_renderObserver->Delete();
	}
}

void iARendererImpl::setSceneBounds(iAAABB const & boundingBox)
{
	iAVec3d origin(boundingBox.minCorner());
	auto size = (boundingBox.maxCorner() - boundingBox.minCorner());
	// for stick out size, we compute average size over all 3 dimensions; maybe use max instead?
	double stickOutSize = (size[0] + size[1] + size[2]) * (IndicatorsLenMultiplier - 1) / 6;
	double oldStickOutBoxSize = (m_stickOutBox[1] - m_stickOutBox[0]).length();
	m_stickOutBox[0] = origin - stickOutSize;
	m_stickOutBox[1] = origin + size + stickOutSize;

	const double IndicatorsSizeDivisor = 10; // maybe make configurable?
	auto indicatorsSize = size / IndicatorsSizeDivisor;

	m_axesActor->SetTotalLength(indicatorsSize[0], indicatorsSize[0], indicatorsSize[0]);
	m_moveableAxesActor->SetTotalLength(indicatorsSize[0], indicatorsSize[0], indicatorsSize[0]);

	m_profileLineStartPointSource->SetRadius(indicatorsSize[0]);
	m_profileLineEndPointSource->SetRadius(indicatorsSize[0]);

	iAVec3d center = origin + size / 2;
	for (int s = 0; s < 3; ++s)
	{
		m_slicePlaneSource[s]->SetXLength((s == iASlicerMode::XY || s == iASlicerMode::XZ) ?
			IndicatorsLenMultiplier * size[0] : m_unitSize[0]);
		m_slicePlaneSource[s]->SetYLength((s == iASlicerMode::XY || s == iASlicerMode::YZ) ?
			IndicatorsLenMultiplier * size[1] : m_unitSize[1]);
		m_slicePlaneSource[s]->SetZLength((s == iASlicerMode::XZ || s == iASlicerMode::YZ) ?
			IndicatorsLenMultiplier * size[2] : m_unitSize[2]);
		m_slicePlaneSource[s]->SetCenter(center.data());
	}
	const double ResetCameraBoxFactor = 1.2;
	if ((m_stickOutBox[1] - m_stickOutBox[0]).length() > ResetCameraBoxFactor * oldStickOutBoxSize)
	{
		m_ren->ResetCamera();
	}

}

void iARendererImpl::setDefaultInteractor()
{
	m_interactor->SetInteractorStyle(vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New());
}

void iARendererImpl::update()
{   // just to be on the safe side, we abort if widget is not fully initialized; calling below Render() methods
	if (!m_initialized)    // before that leads to display problems for the whole application
	{                      // (no more proper redrawing, whole application window turns black on resize)
		LOG(lvlWarn, "Invalid call to update() on an uninitialized iARendererImpl");
		return;
	}
	m_ren->Render();
	m_renWin->Render();
	m_renWin->GetInteractor()->Render();
}

void iARendererImpl::showHelpers(bool show)
{
	m_orientationMarkerWidget->SetEnabled(show);
	m_axesActor->SetVisibility(show);
	m_moveableAxesActor->SetVisibility(show);
}

void iARendererImpl::showRPosition(bool s)
{
	m_cActor->SetVisibility(s);
}

void iARendererImpl::showSlicePlane(int axis, bool show)
{
	if (show)
	{
		m_ren->AddActor(m_slicePlaneActor[axis]);
	}
	else
	{
		m_ren->RemoveActor(m_slicePlaneActor[axis]);
	}
	m_slicePlaneActor[axis]->GetProperty()->SetOpacity(m_slicePlaneOpacity);
}

void iARendererImpl::setPlaneNormals( vtkTransform *tr )
{
	double normal[4], temp[4];

	normal[0] = 1; normal[1] = 0; normal[2] = 0; normal[3] = 1;
	tr->GetMatrix()->MultiplyPoint(normal, temp);
	m_plane1->SetNormal( temp[0], temp[1], temp[2] );

	normal[0] = 0; normal[1] = 1; normal[2] = 0; normal[3] = 1;
	tr->GetMatrix()->MultiplyPoint(normal, temp);
	m_plane2->SetNormal( temp[0], temp[1], temp[2] );

	normal[0] = 0; normal[1] = 0; normal[2] = 1; normal[3] = 1;
	tr->GetMatrix()->MultiplyPoint(normal, temp);
	m_plane3->SetNormal( temp[0], temp[1], temp[2] );

	m_renWin->Render();
	m_ren->Render();
};

void iARendererImpl::setPositionMarkerCenter(double x, double y, double z )
{
	if (!m_interactor->GetEnabled())
	{
		return;
	}
	m_cSource->SetCenter(x, y, z);
	update();
}

void iARendererImpl::setCamPosition(int pos)
{
	::setCamPosition(m_cam, static_cast<iACameraPosition>(pos));
	m_ren->ResetCamera();
	update();
}

void iARendererImpl::camPosition( double * camOptions )
{
	double pS = m_cam->GetParallelScale();
	double a[3] = {0};
	double b[3] = {0};
	double c[3] = {0};
	m_cam->GetViewUp(a);
	m_cam->GetPosition(b);
	m_cam->GetFocalPoint(c);

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

	m_ren->ResetCamera();
	update();
}

void iARendererImpl::setCamPosition( double * camOptions, bool rsParallelProjection )
{
	m_cam->SetViewUp ( camOptions[0], camOptions[1], camOptions[2] );
	m_cam->SetPosition ( camOptions[3], camOptions[4], camOptions[5] );
	m_cam->SetFocalPoint( camOptions[6], camOptions[7], camOptions[8] );

	if (rsParallelProjection)
	{
		m_cam->SetParallelScale(camOptions[9]);
	}
	update();
}

void iARendererImpl::setCamera(vtkCamera* c)
{
	m_cam = c;
	m_labelRen->SetActiveCamera(m_cam);
	m_ren->SetActiveCamera(m_cam);
	emit onSetCamera();
}

vtkCamera* iARendererImpl::camera()
{
	return m_cam;
}

void iARendererImpl::setUnitSize(std::array<double, 3> size)
{
	m_unitSize = size;
	m_cSource->SetXLength(size[0]);
	m_cSource->SetYLength(size[1]);
	m_cSource->SetZLength(size[2]);
}

void iARendererImpl::setSlicePlaneOpacity(float opc)
{
	if ((opc > 1.0) || (opc < 0.0f))
	{
		LOG(lvlWarn, QString("Invalid slice plane opacity %1").arg(opc));
		return;
	}

	m_slicePlaneOpacity = opc;
}

void iARendererImpl::saveMovie(const QString& fileName, int mode, int qual /*= 2*/)
{
	auto movieWriter = GetMovieWriter(fileName, qual);

	if (movieWriter.GetPointer() == nullptr)
	{
		return;
	}
	iAProgress p;
	iASimpleAbortListener aborter;
	auto jobHandle = iAJobListView::get()->addJob("Exporting Movie", &p, &aborter);
	LOG(lvlInfo, tr("Movie export started, output file name: %1").arg(fileName));
	// save current state and disable interaction:
	m_interactor->Disable();
	auto oldCam = vtkSmartPointer<vtkCamera>::New();
	oldCam->DeepCopy(m_cam);

	// set up movie export pipeline:
	auto windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
	int* rws = m_renWin->GetSize();
	if (rws[0] % 2 != 0)
	{
		rws[0]++;
	}
	if (rws[1] % 2 != 0)
	{
		rws[1]++;
	}
	m_renWin->SetSize(rws);
	m_renWin->Render();
	windowToImage->SetInput(m_renWin);
	windowToImage->ReadFrontBufferOff();
	movieWriter->SetInputConnection(windowToImage->GetOutputPort());
	movieWriter->Start();

	int numRenderings = 360;  //TODO
	auto rot = vtkSmartPointer<vtkTransform>::New();
	m_cam->SetFocalPoint(0, 0, 0);
	double view[3];
	double point[3];
	if (mode == 0)
	{  // YZ
		double _view[3] = {0, 0, -1};
		double _point[3] = {1, 0, 0};
		for (int ind = 0; ind < 3; ind++)
		{
			view[ind] = _view[ind];
			point[ind] = _point[ind];
		}
		rot->RotateZ(360 / numRenderings);
	}
	else if (mode == 1)
	{  // XY
		double _view[3] = {0, 0, -1};
		double _point[3] = {0, 1, 0};
		for (int ind = 0; ind < 3; ind++)
		{
			view[ind] = _view[ind];
			point[ind] = _point[ind];
		}
		rot->RotateX(360 / numRenderings);
	}
	else if (mode == 2)
	{  // XZ
		double _view[3] = {0, 1, 0};
		double _point[3] = {0, 0, 1};
		for (int ind = 0; ind < 3; ind++)
		{
			view[ind] = _view[ind];
			point[ind] = _point[ind];
		}
		rot->RotateY(360 / numRenderings);
	}
	m_cam->SetViewUp(view);
	m_cam->SetPosition(point);
	for (int i = 0; i < numRenderings && !aborter.isAborted(); i++)
	{
		m_ren->ResetCamera();
		m_renWin->Render();

		windowToImage->Modified();
		movieWriter->Write();
		if (movieWriter->GetError())
		{
			LOG(lvlError, movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
			break;
		}
		p.emitProgress((i + 1) * 100.0 / numRenderings);
		m_cam->ApplyTransform(rot);
		QCoreApplication::processEvents();
	}

	m_cam->DeepCopy(oldCam);
	movieWriter->End();
	m_interactor->Enable();

	printFinalLogMessage(movieWriter, aborter);
}

void iARendererImpl::mouseRightButtonReleasedSlot()
{
	if (!m_interactor)
	{
		return;
	}
	m_interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent);
}

void iARendererImpl::mouseLeftButtonReleasedSlot()
{
	if (!m_interactor)
	{
		return;
	}
	m_interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
}

void iARendererImpl::setProfilePoint(int pointIndex, double const * coords)
{
	m_profileLine[0].setPoint(pointIndex, coords[0], coords[1], coords[2]);
	m_profileLine[0].mapper->Update();

	for (int i = 0; i < 3; ++i)
	{
		int profLineIdx = (pointIndex * 3) + 1 + i;
		assert(profLineIdx >= 0 && profLineIdx < NumOfProfileLines);
		for (int ptIdx = 0; ptIdx < 2; ++ptIdx)
		{
			iAVec3d pt(coords);
			pt[i] = m_stickOutBox[ptIdx][i];
			m_profileLine[profLineIdx].setPoint(ptIdx, pt[0], pt[1], pt[2]);
			m_profileLine[profLineIdx].mapper->Update();
		}
	}
	if (pointIndex == 0)
	{
		m_profileLineStartPointSource->SetCenter(coords);
		m_profileLineStartPointMapper->Update();
	}
	else
	{
		m_profileLineEndPointSource->SetCenter(coords);
		m_profileLineEndPointMapper->Update();
	}
	update();
}

void iARendererImpl::setProfileHandlesOn(bool isOn)
{
	for (int p = 0; p < NumOfProfileLines; ++p)
	{
		m_profileLine[p].actor->SetVisibility(isOn);
	}
	m_profileLineStartPointActor->SetVisibility(isOn);
	m_profileLineEndPointActor->SetVisibility(isOn);
}

void iARendererImpl::addRenderer(vtkRenderer* renderer)
{
	m_renWin->AddRenderer(renderer);
}

void iARendererImpl::enableInteractor(bool enable)
{
	if (enable)
	{
		m_interactor->ReInitialize();
	}
	else
	{
		m_interactor->Disable();
	}
	LOG(lvlInfo, tr("Renderer interaction %1.").arg(iAConverter<bool>::toString(enable)));
}
bool iARendererImpl::isInteractorEnabled() const
{
	return m_interactor->GetEnabled();
}
vtkPlane* iARendererImpl::plane1() { return m_plane1; };
vtkPlane* iARendererImpl::plane2() { return m_plane2; };
vtkPlane* iARendererImpl::plane3() { return m_plane3; };
vtkRenderer * iARendererImpl::renderer() { return m_ren; };
vtkRenderWindowInteractor* iARendererImpl::interactor() { return m_interactor; }
vtkRenderWindow* iARendererImpl::renderWindow() { return m_renWin; }
vtkRenderer * iARendererImpl::labelRenderer(void) { return m_labelRen; }
vtkTextActor* iARendererImpl::txtActor() { return m_txtActor; }
vtkActor* iARendererImpl::selectedActor() { return m_selectedActor; }
vtkUnstructuredGrid* iARendererImpl::finalSelection() { return m_finalSelection; }
vtkDataSetMapper* iARendererImpl::selectedMapper() { return m_selectedMapper; }
vtkTransform* iARendererImpl::coordinateSystemTransform() { m_moveableAxesTransform->Update(); return m_moveableAxesTransform; }
void iARendererImpl::setAxesTransform(vtkTransform* transform) { m_moveableAxesTransform = transform; }
iARenderObserver * iARendererImpl::getRenderObserver() { return m_renderObserver; }

void iARendererImpl::setSlicingBounds(const int roi[6], const double * spacing)
{
	double xMin = roi[0] * spacing[0],
	       yMin = roi[1] * spacing[1],
	       zMin = roi[2] * spacing[2];
	double xMax = xMin + roi[3] * spacing[0],
	       yMax = yMin + roi[4] * spacing[1],
	       zMax = zMin + roi[5] * spacing[2];
	m_roiCube->SetBounds(xMin, xMax, yMin, yMax, zMin, zMax);
	update();
}

void iARendererImpl::setROIVisible(bool visible)
{
	m_roiActor->SetVisibility(visible);
}

void iARendererImpl::setSlicePlanePos(int planeID, double originX, double originY, double originZ)
{
	switch (planeID)
	{
		default:
		case iASlicerMode::YZ: m_plane1->SetOrigin(originX, originY, originZ); break;
		case iASlicerMode::XZ: m_plane2->SetOrigin(originX, originY, originZ); break;
		case iASlicerMode::XY: m_plane3->SetOrigin(originX, originY, originZ); break;
	}
	double center[3];
	m_slicePlaneSource[planeID]->GetCenter(center);
	center[planeID] = (planeID == 0) ? originX : ((planeID == 1) ? originY : originZ);
	m_slicePlaneSource[planeID]->SetCenter(center);
	m_slicePlaneMapper[planeID]->Update();
	update();
}

void iARendererImpl::applySettings(iARenderSettings const & settings, bool slicePlaneVisibility[3])
{
	m_ren->SetUseDepthPeeling(settings.UseDepthPeeling);
	m_ren->SetUseDepthPeelingForVolumes(settings.UseDepthPeeling);
	m_ren->SetMaximumNumberOfPeels(settings.DepthPeels);
	m_ren->SetOcclusionRatio(settings.OcclusionRatio);
	m_ren->SetUseFXAA(settings.UseFXAA);
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 1, 0)
	m_ren->SetUseSSAO(settings.UseSSAO);
#endif
	m_renWin->SetMultiSamples(settings.MultiSamples);
	m_cam->SetParallelProjection(settings.ParallelProjection);
	setSlicePlaneOpacity(settings.PlaneOpacity);
	setBackgroundColors(settings);
	showHelpers(settings.ShowHelpers);
	showRPosition(settings.ShowRPosition);
	for (int i = 0; i < 3; ++i)
	{
		showSlicePlane(i, settings.ShowSlicePlanes && slicePlaneVisibility[i]);
	}
}

void iARendererImpl::setBackgroundColors(iARenderSettings const& settings)
{
	QColor bgTop(settings.BackgroundTop);
	QColor bgBottom(settings.BackgroundBottom);
	if (settings.UseStyleBGColor)
	{
		bgBottom = bgTop = QApplication::palette().color(QPalette::Window);
	}
	m_ren->SetBackground2(bgTop.redF(), bgTop.greenF(), bgTop.blueF());
	m_ren->SetBackground(bgBottom.redF(), bgBottom.greenF(), bgBottom.blueF());
	emit bgColorChanged(bgTop, bgBottom);
}

void iARendererImpl::touchStart()
{
	m_touchStartScale = m_cam->GetParallelScale();
	m_touchStartCamPos = iAVec3d(m_cam->GetPosition());
	LOG(lvlDebug, QString("Init scale: %1").arg(m_touchStartScale));
}

void iARendererImpl::touchScaleSlot(float relScale)
{
	if (m_cam->GetParallelProjection())
	{
		float newScale = m_touchStartScale / relScale;
		//LOG(lvlDebug, QString("PARALLEL: Scale before: %1; new scale: %2").arg(m_touchStartScale).arg(newScale));
		m_cam->SetParallelScale(newScale);
	}
	else
	{
		auto camDir = m_touchStartCamPos - iAVec3d(m_cam->GetFocalPoint());
		double diff = 1 - relScale;
		auto newCamPos = m_touchStartCamPos + camDir * diff;
		//LOG(lvlDebug, QString("PERSPECTIVE: start pos: %1; new pos: %3")
		//	.arg(QString("%1,%2,%3").arg(m_touchStartCamPos[0]).arg(m_touchStartCamPos[2]).arg(m_touchStartCamPos[2]))
		//	.arg(QString("%1,%2,%3").arg(newCamPos[0]).arg(newCamPos[2]).arg(newCamPos[2])));
		m_cam->SetPosition(newCamPos.data());
	}
	update();
}
