/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASlicer.h"

#include "defines.h"    // for NotExistingChannel
#include "dlg_commoninput.h"
#include "dlg_slicer.h"
#include "iAArbitraryProfileOnSlicer.h"
#include "iAChannelData.h"
#include "iAChannelSlicerData.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAMagicLens.h"
#include "iAMathUtility.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAMovieHelper.h"
#include "iARulerWidget.h"
#include "iARulerRepresentation.h"
#include "iASlicer.h"
#include "iASlicerProfile.h"
#include "iASlicerSettings.h"
#include "iASnakeSpline.h"
#include "iAStringHelper.h"
#include "iAToolsITK.h"
#include "iAToolsVTK.h"
#include "iAWrapperText.h"
#include "io/iAIOProvider.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkActor.h>
#include <vtkAxisActor2D.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkCubeSource.h>
#include <vtkDataSetMapper.h>
#include <vtkDiskSource.h>
#include <vtkGenericMovieWriter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageBlend.h>
#include <vtkImageCast.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageResample.h>
#include <vtkImageReslice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLineSource.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
#include <vtkLookupTable.h>
#include <vtkMarchingContourFilter.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQImageToImageSource.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarBarWidget.h>
#include <vtkTextActor3D.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkThinPlateSplineTransform.h>
#include <vtkTransform.h>
#include <vtkVersion.h>
#include <vtkWindowToImageFilter.h>
#include <vtkWorldPointPicker.h>

#include <QBitmap>
#include <QFileDialog>
#include <QIcon>
#include <QKeyEvent>
#include <QtMath>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <QtGlobal> // for QT_VERSION

#include <cassert>

//! Custom interactor style for slicers, for disabling certain vtk interactions we do differently.
class iAInteractorStyleImage : public vtkInteractorStyleImage
{
public:
	static iAInteractorStyleImage *New();
	vtkTypeMacro(iAInteractorStyleImage, vtkInteractorStyleImage)

	//! Disable "window-level" and rotation interaction (anything but shift-dragging)
	void OnLeftButtonDown() override
	{
		if (!this->Interactor->GetShiftKey())
		{
			return;
		}
		vtkInteractorStyleImage::OnLeftButtonDown();
	}
	//! @{ shift and control + mousewheel are used differently - don't use them for zooming!
	void OnMouseWheelForward() override
	{
		if (this->Interactor->GetControlKey() || this->Interactor->GetShiftKey())
		{
			return;
		}
		vtkInteractorStyleImage::OnMouseWheelForward();
	}
	void OnMouseWheelBackward() override
	{
		if (this->Interactor->GetControlKey() || this->Interactor->GetShiftKey())
		{
			return;
		}
		vtkInteractorStyleImage::OnMouseWheelBackward();
	}
	//! @}
	//! @{ Conditionally disable zooming via right button dragging
	void OnRightButtonDown() override
	{
		if (!m_rightButtonDragZoomEnabled)
		{
			return;
		}
		vtkInteractorStyleImage::OnRightButtonDown();
	}
	void SetRightButtonDragZoomEnabled(bool enabled)
	{
		m_rightButtonDragZoomEnabled = enabled;
	}

	//! @}
	/*
	virtual void OnChar()
	{
		vtkRenderWindowInteractor *rwi = this->Interactor;
		switch (rwi->GetKeyCode())
		{ // disable 'picking' action on p
		case 'P':
		case 'p':
			break;
		default:
			vtkInteractorStyleImage::OnChar();
		}
	}
	*/

private:
	bool m_rightButtonDragZoomEnabled = true;
};

vtkStandardNewMacro(iAInteractorStyleImage);


//! observer needs to be a separate class; otherwise there is an error when destructing,
//! as vtk deletes all its observers...
class iAObserverRedirect : public vtkCommand
{
public:
	iAObserverRedirect(iASlicer* redirect) : m_redirect(redirect)
	{}
private:
	void Execute(vtkObject * caller, unsigned long eventId, void * callData)
	{
		m_redirect->execute(caller, eventId, callData);
	}
	iASlicer* m_redirect;
};


iASlicer::iASlicer(QWidget * parent, const iASlicerMode mode,
	bool decorations /*= true*/, bool magicLensAvailable /*= true*/, vtkAbstractTransform *transform, vtkPoints* snakeSlicerPoints) :
	iAVtkWidget(parent),
	m_contextMenuMagicLens(nullptr),
	m_contextMenuSnakeSlicer(nullptr),
	m_interactionMode(Normal),
	m_isSliceProfEnabled(false),
	m_isArbProfEnabled(false),
	m_xInd(0), m_yInd(0), m_zInd(0),
	m_snakeSpline(nullptr),
	m_worldSnakePoints(snakeSlicerPoints),
	m_sliceProfile(nullptr),
	m_arbProfile(nullptr),
	m_mode(mode),
	m_decorations(decorations),
	m_userSetBackground(false),
	m_showPositionMarker(false),
	m_magicLensInput(NotExistingChannel),
	m_fisheyeLensActivated(false),
	m_fisheyeRadius(80.0),
	m_innerFisheyeRadius(70.0),
	m_interactor(nullptr),
	m_interactorStyle(iAInteractorStyleImage::New()),
	m_renWin(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
	m_ren(vtkSmartPointer<vtkRenderer>::New()),
	m_camera(vtkCamera::New()),
	m_cameraOwner(true),
	m_transform(transform ? transform : vtkTransform::New()),
	m_pointPicker(vtkSmartPointer<vtkWorldPointPicker>::New()),
	m_slabThickness(0),
	m_roiActive(false),
	m_sliceNumber(0),
	m_cursorSet(false),
	m_linkedMdiChild(nullptr)
{
	std::fill(m_angle, m_angle + 3, 0);
	setAutoFillBackground(false);
	setFocusPolicy(Qt::StrongFocus);		// to receive the KeyPress Event!
	setMouseTracking(true);					// to receive the Mouse Move Event
	m_renWin->AlphaBitPlanesOn();
	m_renWin->LineSmoothingOn();
	m_renWin->PointSmoothingOn();
	// Turned off, because of gray strokes e.g., on scalarBarActors. Only on NVIDIA graphic cards:
	m_renWin->PolygonSmoothingOff();
#if VTK_MAJOR_VERSION < 9
	SetRenderWindow(m_renWin);
#else
	setRenderWindow(m_renWin);
#endif
	setDefaultInteractor();

	m_renWin->AddRenderer(m_ren);
	m_ren->SetActiveCamera(m_camera);

	m_interactor->SetPicker(m_pointPicker);
	m_interactor->Initialize();
	m_interactorStyle->SetDefaultRenderer(m_ren);

	iAObserverRedirect* redirect(new iAObserverRedirect(this));
	m_interactor->AddObserver(vtkCommand::LeftButtonPressEvent, redirect);
	m_interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, redirect);
	m_interactor->AddObserver(vtkCommand::RightButtonPressEvent, redirect);
	m_interactor->AddObserver(vtkCommand::MouseMoveEvent, redirect);
	m_interactor->AddObserver(vtkCommand::KeyPressEvent, redirect);
	m_interactor->AddObserver(vtkCommand::KeyReleaseEvent, redirect);
	m_interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, redirect);
	m_interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, redirect);

	updateBackground();

	if (magicLensAvailable)
	{
		m_magicLens = QSharedPointer<iAMagicLens>(new iAMagicLens());
		m_magicLens->setRenderWindow(m_renWin);
		// setup context menu for the magic lens view options
		m_contextMenuMagicLens = new QMenu(this);
		QActionGroup * actionGr(new QActionGroup(this));
		auto centeredLens = m_contextMenuMagicLens->addAction(tr("Centered Magic Lens"), this, SLOT(menuCenteredMagicLens()));
		centeredLens->setCheckable(true);
		centeredLens->setChecked(true);
		actionGr->addAction(centeredLens);
		auto offsetLens = m_contextMenuMagicLens->addAction(tr("Offseted Magic Lens"), this, SLOT(menuOffsetMagicLens()));
		offsetLens->setCheckable(true);
		actionGr->addAction(offsetLens);
	}

	if (decorations)
	{
		m_snakeSpline = new iASnakeSpline;

		m_contextMenuSnakeSlicer = new QMenu(this);
		m_contextMenuSnakeSlicer->addAction(QIcon(":/images/loadtrf.png"), tr("Delete Snake Line"), this, SLOT(menuDeleteSnakeLine()));
		m_sliceProfile = new iASlicerProfile();
		m_sliceProfile->setVisibility(false);

		m_arbProfile = new iAArbitraryProfileOnSlicer();
		m_arbProfile->setVisibility(false);

		m_scalarBarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();
		m_textProperty = vtkSmartPointer<vtkTextProperty>::New();
		m_logoWidget = vtkSmartPointer<vtkLogoWidget>::New();
		m_logoRep = vtkSmartPointer<vtkLogoRepresentation>::New();
		m_logoImage = vtkSmartPointer<vtkQImageToImageSource>::New();

		m_positionMarkerSrc = vtkSmartPointer<vtkCubeSource>::New();
		m_positionMarkerMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_positionMarkerActor = vtkSmartPointer<vtkActor>::New();

		m_lineSource = vtkSmartPointer<vtkLineSource>::New();
		m_lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_lineActor = vtkSmartPointer<vtkActor>::New();
		m_diskSource = vtkSmartPointer<vtkDiskSource>::New();
		m_diskMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_diskActor = vtkSmartPointer<vtkActor>::New();

		m_roiSource = vtkSmartPointer<vtkCubeSource>::New();
		m_roiMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_roiActor = vtkSmartPointer<vtkActor>::New();

		for (int i = 0; i < 2; ++i)
		{
			m_axisTransform[i] = vtkSmartPointer<vtkTransform>::New();
			m_axisTextActor[i] = vtkSmartPointer<vtkTextActor3D>::New();
		}
		m_textInfo = vtkSmartPointer<iAWrapperText>::New();
		m_rulerWidget = vtkSmartPointer<iARulerWidget>::New();

		m_textInfo->AddToScene(m_ren);
		m_textInfo->SetText(" ");
		m_textInfo->SetPosition(iAWrapperText::POS_LOWER_LEFT);
		m_textInfo->Show(1);

		QImage img;
		img.load(":/images/fhlogo.png");
		m_logoImage->SetQImage(&img);
		m_logoImage->Update();
		m_logoRep->SetImage(m_logoImage->GetOutput());
		m_logoWidget->SetInteractor(m_interactor);
		m_logoWidget->SetRepresentation(m_logoRep);
		m_logoWidget->SetResizable(false);
		m_logoWidget->SetSelectable(true);
		m_logoWidget->On();

		m_textProperty->SetBold(1);
		m_textProperty->SetItalic(1);
		m_textProperty->SetColor(1, 1, 1);
		m_textProperty->SetJustification(VTK_TEXT_CENTERED);
		m_textProperty->SetVerticalJustification(VTK_TEXT_CENTERED);
		m_textProperty->SetOrientation(1);
		m_scalarBarWidget->GetScalarBarActor()->SetLabelFormat("%.2f");
		m_scalarBarWidget->GetScalarBarActor()->SetTitleTextProperty(m_textProperty);
		m_scalarBarWidget->GetScalarBarActor()->SetLabelTextProperty(m_textProperty);
		m_scalarBarWidget->GetScalarBarRepresentation()->SetOrientation(1);
		m_scalarBarWidget->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue(0.92, 0.2);
		m_scalarBarWidget->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue(0.06, 0.75);
		m_scalarBarWidget->GetScalarBarActor()->SetTitle("Range");
		m_scalarBarWidget->SetRepositionable(true);
		m_scalarBarWidget->SetResizable(true);
		m_scalarBarWidget->SetInteractor(m_interactor);

		m_positionMarkerMapper->SetInputConnection(m_positionMarkerSrc->GetOutputPort());
		m_positionMarkerActor->SetMapper(m_positionMarkerMapper);
		m_positionMarkerActor->GetProperty()->SetColor(0, 1, 0);
		m_positionMarkerActor->GetProperty()->SetOpacity(1);
		m_positionMarkerActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);
		m_positionMarkerActor->SetVisibility(false);

		m_lineSource->SetPoint1(0.0, 0.0, 0.0);
		m_lineSource->SetPoint2(10.0, 10.0, 0.0);
		m_lineSource->Update();
		m_lineMapper->SetInputConnection(m_lineSource->GetOutputPort());
		m_lineActor->SetMapper(m_lineMapper);
		m_lineActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
		m_lineActor->GetProperty()->SetOpacity(1);
		m_lineActor->SetVisibility(false);

		m_diskSource->SetCircumferentialResolution(50);
		m_diskSource->Update();
		m_diskMapper->SetInputConnection(m_diskSource->GetOutputPort());
		m_diskActor->SetMapper(m_diskMapper);
		m_diskActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
		m_diskActor->GetProperty()->SetOpacity(1);
		m_diskActor->SetVisibility(false);
		m_diskActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);

		m_roiMapper->SetInputConnection(m_roiSource->GetOutputPort());
		m_roiActor->SetVisibility(false);
		m_roiActor->SetMapper(m_roiMapper);
		m_roiActor->GetProperty()->SetColor(1, 0, 0);
		m_roiActor->GetProperty()->SetOpacity(1);
		m_roiMapper->Update();
		m_roiActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);

		m_axisTextActor[0]->SetInput(axisName(mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)).toStdString().c_str());
		m_axisTextActor[1]->SetInput(axisName(mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)).toStdString().c_str());

		for (int i = 0; i < 2; ++i)
		{
			m_axisTextActor[i]->SetPickable(false);
			// large font size required to make the font nicely smooth
			m_axisTextActor[i]->GetTextProperty()->SetFontSize(100);
			m_axisTextActor[i]->GetTextProperty()->SetFontFamilyToArial();
			m_axisTextActor[i]->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
			m_ren->AddActor(m_axisTextActor[i]);
			m_axisTextActor[i]->SetVisibility(false);
			m_axisTextActor[i]->SetUserTransform(m_axisTransform[i]);
		}
		m_axisTextActor[0]->GetTextProperty()->SetVerticalJustificationToTop();
		m_axisTextActor[0]->GetTextProperty()->SetJustificationToCentered();
		m_axisTextActor[1]->GetTextProperty()->SetVerticalJustificationToCentered();
		m_axisTextActor[1]->GetTextProperty()->SetJustificationToRight();

		m_rulerWidget->SetInteractor(m_interactor);
		m_rulerWidget->SetEnabled(true);
		m_rulerWidget->SetRepositionable(true);
		m_rulerWidget->SetResizable(true);
		m_rulerWidget->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue(0.333, 0.05);
		m_rulerWidget->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue(0.333, 0.051);

		m_ren->AddActor(m_positionMarkerActor);
		m_ren->AddActor(m_lineActor);
		m_ren->AddActor(m_diskActor);
		m_ren->AddActor(m_roiActor);
	}
	m_renWin->SetNumberOfLayers(3);
	m_camera->SetParallelProjection(true);

	if (m_decorations)
	{
		m_sliceProfile->addToRenderer(m_ren);
		m_arbProfile->addToRenderer(m_ren);
	}
	m_ren->ResetCamera();
}

iASlicer::~iASlicer()
{
	disconnect();

	m_interactorStyle->Delete();

	if (m_cameraOwner)
	{
		m_camera->Delete();
	}
	if (m_decorations)
	{
		delete m_snakeSpline;
		delete m_contextMenuSnakeSlicer;
		delete m_contextMenuMagicLens;
		delete m_sliceProfile;
		delete m_arbProfile;
	}
}

void iASlicer::toggleInteractorState()
{
	if (m_interactor->GetEnabled())
	{
		disableInteractor();
		emit msg(tr("Slicer %1 disabled.").arg(slicerModeString(m_mode)));
	}
	else
	{
		enableInteractor();
		emit msg(tr("Slicer %1 enabled.").arg(slicerModeString(m_mode)));
	}
}

void iASlicer::setMode( const iASlicerMode mode )
{
	m_mode = mode;
	for (auto ch : m_channels)
	{
		ch->updateResliceAxesDirectionCosines(m_mode);
	}
	updateBackground();
}

iASlicerMode iASlicer::mode() const
{
	return m_mode;
}

void iASlicer::disableInteractor()
{
	m_interactor->Disable();
}

void iASlicer::enableInteractor()
{
	m_interactor->ReInitialize();
	update();
}

void iASlicer::update()
{
	if (!isVisible())
	{
		return;
	}
	for (auto ch : m_channels)
	{
		ch->updateMapper();
		ch->reslicer()->Update();
	}
	m_interactor->ReInitialize();
	m_interactor->Render();
	m_ren->Render();
	if (m_magicLens)
	{
		m_magicLens->render();
	}
	iAVtkWidget::update();

	emit updateSignal();
}

void iASlicer::saveMovie()
{
	QString movie_file_types = GetAvailableMovieFormats();
	if( movie_file_types.isEmpty() )
	{
		QMessageBox::information( this, "Movie Export", "This version of open_iA was built without movie export support!");
		return;
	}
	QString fileName = QFileDialog::getSaveFileName( this,
		tr( "Export as a movie" ),
		"", // TODO: get directory of file?
		movie_file_types );
	saveSliceMovie( fileName );
}

void iASlicer::setSliceNumber( int sliceNumber )
{
	// TODO: set slice position (in scene coordinates) instead of number
	//       then we wouldn'T need image spacing and origin below
	//       (which don't make too much sense anyway, if it's not the same between loaded datasets)
	if (!hasChannel(0))
	{
		return;
	}
	m_sliceNumber = sliceNumber;
	double xyz[3] = { 0.0, 0.0, 0.0 };
	xyz[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z)] = sliceNumber;
	if (m_roiActive)
	{
		m_roiActor->SetVisibility(m_roiSlice[0] <= m_sliceNumber && m_sliceNumber < (m_roiSlice[1]));
	}
	double const * spacing = m_channels[0]->input()->GetSpacing();
	double const * origin = m_channels[0]->input()->GetOrigin();
	for (auto ch : m_channels)
	{
		ch->setResliceAxesOrigin(origin[0] + xyz[0] * spacing[0], origin[1] + xyz[1] * spacing[1], origin[2] + xyz[2] * spacing[2]);
	}
	updateMagicLensColors();
	update();
	emit sliceNumberChanged( m_mode, sliceNumber );
}

void iASlicer::setup( iASingleSlicerSettings const & settings )
{
	m_settings = settings;
	for (auto channel: m_channels)
	{
		channel->setInterpolate(settings.LinearInterpolation);
	}
	if (m_magicLens)
	{
		m_magicLens->setInterpolate(settings.LinearInterpolation);
	}
	setMouseCursor(settings.CursorMode);
	setContours(settings.NumberOfIsoLines, settings.MinIsoValue, settings.MaxIsoValue);
	showIsolines(settings.ShowIsoLines);
	showPosition(settings.ShowPosition);
	if (m_decorations)
	{
		m_axisTextActor[0]->SetVisibility(settings.ShowAxesCaption);
		m_axisTextActor[1]->SetVisibility(settings.ShowAxesCaption);
		m_textInfo->GetTextMapper()->GetTextProperty()->SetFontSize(settings.ToolTipFontSize);
		m_textInfo->GetActor()->SetVisibility(settings.ShowTooltip);
	}
	if (m_magicLens)
	{
		updateMagicLens();
	}
	m_renWin->Render();
}

void iASlicer::setMagicLensEnabled( bool isEnabled )
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensEnabled called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->setEnabled(isEnabled);
	m_interactorStyle->SetRightButtonDragZoomEnabled(!isEnabled);
	setShowText(!isEnabled);
	updateMagicLens();
}

iAMagicLens * iASlicer::magicLens()
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensEnabled called on slicer which doesn't have a magic lens!");
		return nullptr;
	}
	return m_magicLens.data();
}

void iASlicer::setMagicLensSize(int newSize)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensSize called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->setSize(newSize);
	updateMagicLens();
}

int iASlicer::magicLensSize() const
{
	return m_magicLens ? m_magicLens->size() : 0;
}

void iASlicer::setMagicLensFrameWidth(int newWidth)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensFrameWidth called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->setFrameWidth(newWidth);
	updateMagicLens();
}

void iASlicer::setMagicLensCount(int count)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensCount called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->setLensCount(count);
	updateMagicLens();
}

void iASlicer::setMagicLensInput(uint id)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensInput called on slicer which doesn't have a magic lens!");
		return;
	}
	iAChannelSlicerData * d = channel(id);
	assert(d);
	if (!d)
	{
		return;
	}
	m_magicLensInput = id;
	m_magicLens->addInput(d->reslicer(), d->colorTF(), d->name());
	update();
}

uint iASlicer::magicLensInput() const
{
	return m_magicLensInput;
}

void iASlicer::setMagicLensOpacity(double opacity)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensOpacity called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->setOpacity(opacity);
	update();
}

double iASlicer::magicLensOpacity() const
{
	return (m_magicLens) ? m_magicLens->opacity() : 0;
}

vtkGenericOpenGLRenderWindow * iASlicer::renderWindow()
{
	return m_renWin;
}

vtkRenderer * iASlicer::renderer()
{
	return m_ren;
}

vtkCamera * iASlicer::camera()
{
	return m_camera;
}

vtkRenderWindowInteractor * iASlicer::interactor()
{
	return m_interactor;
}

void iASlicer::addChannel(uint id, iAChannelData const & chData, bool enable)
{
	assert(!m_channels.contains(id));
	bool updateSpacing = m_channels.empty();
	auto chSlicerData = createChannel(id, chData);
	auto image = chData.image();
	double const * imgSpc = image->GetSpacing();
	if (updateSpacing && m_decorations)
	{
		setScalarBarTF(chData.colorTF());
		updatePositionMarkerExtent();
		// TODO: update required for new channels other than to export? export all channels?
		double unitSpacing = std::max(std::max(imgSpc[0], imgSpc[1]), imgSpc[2]);
		double const * spc = m_channels[id]->output()->GetSpacing();
		int    const * dim = m_channels[id]->output()->GetDimensions();
		for (int i = 0; i < 2; ++i)
		{	// scaling required to shrink the text to required size (because of large font size, see initialize method)
			m_axisTransform[i]->Scale(unitSpacing / 10, unitSpacing / 10, unitSpacing / 10);
		}
		double xHalf = (dim[0] - 1) * spc[0] / 2.0;
		double yHalf = (dim[1] - 1) * spc[1] / 2.0;
		// "* 10 / unitSpacing" adjusts for scaling (see above)
		m_axisTextActor[0]->SetPosition(xHalf * 10 / unitSpacing, -20.0, 0);
		m_axisTextActor[1]->SetPosition(-20.0, yHalf * 10 / unitSpacing, 0);
		// TODO: fix snake spline with non-fixed slicer images
		m_snakeSpline->initialize(m_ren, image->GetSpacing()[0]);
		triggerSliceRangeChange();
	}
	double origin[3];
	image->GetOrigin(origin);
	int axis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	origin[axis] += static_cast<double>(sliceNumber()) * imgSpc[axis];
	setResliceChannelAxesOrigin(id, origin[0], origin[1], origin[2]);
	if (enable)
	{
		enableChannel(id, true);
	}
}

void iASlicer::triggerSliceRangeChange()
{
	int axis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	int const * ext = channel(0)->input()->GetExtent();
	emit sliceRangeChanged(ext[axis * 2], ext[axis * 2 + 1]);
}

void iASlicer::updateMagicLensColors()
{
	if (m_magicLens)
	{
		m_magicLens->updateColors();
	}
}

void iASlicer::setTransform(vtkAbstractTransform * tr)
{
	m_transform = tr;
	for (auto ch : m_channels)
	{
		ch->setTransform(m_transform);
	}
}

void iASlicer::setDefaultInteractor()
{
	m_interactor = m_renWin->GetInteractor();
	m_interactor->SetInteractorStyle(m_interactorStyle);
}

void iASlicer::addImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_ren->AddActor(imgActor);
}

void iASlicer::removeImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_ren->RemoveActor(imgActor);
}

void iASlicer::blend(vtkAlgorithmOutput *data1, vtkAlgorithmOutput *data2,
	double opacity, double * range)
{
	if (!hasChannel(0))
	{
		return;
	}
	// ToDo: check what it does, implement using new slicer channel feature!
	vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
	lut->SetRange( range );
	lut->SetHueRange(0, 1);
	lut->SetSaturationRange(0, 1);
	lut->SetValueRange( 0, 1 );
	lut->Build();

	vtkSmartPointer<vtkImageBlend> imgBlender = vtkSmartPointer<vtkImageBlend>::New();
	imgBlender->SetOpacity( 0, opacity );
	imgBlender->SetOpacity( 1, 1.0-opacity );
	imgBlender->AddInputConnection(data1);
	imgBlender->AddInputConnection(data2);
	imgBlender->UpdateInformation();
	imgBlender->Update();

	channel(0)->update(iAChannelData(QString(""), imgBlender->GetOutput(), lut));
	channel(0)->reslicer()->SetInputConnection(imgBlender->GetOutputPort());
	channel(0)->updateReslicer();
	update();
}

void iASlicer::setROIVisible(bool visible)
{
	if (!m_decorations)
	{
		return;
	}
	m_roiActive = visible;
	m_roiActor->SetVisibility(visible);
}

void iASlicer::updateROI(int const roi[6])
{
	if (!m_decorations || !m_roiActive || !hasChannel(0))
	{
		return;
	}
	double const * spacing = m_channels[0]->output()->GetSpacing();
	int sliceXAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::X);
	int sliceYAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y);
	int sliceZAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	m_roiSlice[0] = roi[sliceZAxis];
	m_roiSlice[1] = roi[sliceZAxis] + roi[sliceZAxis + 3];
	// apparently, image actor starts output at -0,5spacing, -0.5spacing (probably a side effect of BorderOn)
	// That's why we have to subtract 0.5 from the coordinates!
	double xMin = (roi[sliceXAxis] - 0.5)  * spacing[sliceXAxis],
	       yMin = (roi[sliceYAxis] - 0.5)  * spacing[sliceYAxis];
	double xMax = xMin + roi[sliceXAxis+3] * spacing[sliceXAxis],
	       yMax = yMin + roi[sliceYAxis+3] * spacing[sliceYAxis];
	m_roiSource->SetBounds(xMin, xMax, yMin, yMax, 0, 0);
	m_roiActor->SetVisibility(m_roiSlice[0] <= m_sliceNumber && m_sliceNumber < m_roiSlice[1]);
	m_roiMapper->Update();
	m_interactor->Render();
}

void iASlicer::setResliceAxesOrigin(double x, double y, double z)
{
	if (m_interactor->GetEnabled())
	{
		for (auto ch : m_channels)
		{
			ch->setResliceAxesOrigin(x, y, z);
		}
		m_interactor->Render();
	}
}

void iASlicer::setPositionMarkerCenter(double x, double y)
{
	if (!m_decorations)
	{
		return;
	}

	if (m_interactor->GetEnabled() && m_showPositionMarker)
	{
		m_positionMarkerActor->SetVisibility(true);
		m_positionMarkerSrc->SetCenter(x, y, 0);
		m_positionMarkerMapper->Update();
		update();
	}
}

void iASlicer::showIsolines(bool s)
{
	if (!m_decorations)
	{
		return;
	}
	for (auto ch : m_channels)
	{
		ch->setShowContours(m_ren, s);
	}
}

void iASlicer::showPosition(bool s)
{
	if (!m_decorations)
	{
		return;
	}
	m_showPositionMarker = s;
}

void iASlicer::saveSliceMovie(QString const& fileName, int qual /*= 2*/)
{
	// TODO: select channel / for all channels?
	if (!hasChannel(0))
	{
		return;
	}
	QString movie_file_types = GetAvailableMovieFormats();
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(this, "Movie Export", "This version of open_iA was built without movie export support!");
		return;
	}
	auto movieWriter = GetMovieWriter(fileName, qual);
	if (movieWriter.GetPointer() == nullptr)
	{
		return;
	}
	m_interactor->Disable();

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

	int const * imgExtent = m_channels[0]->input()->GetExtent();
	double const * imgOrigin = m_channels[0]->input()->GetOrigin();
	double const * imgSpacing = m_channels[0]->input()->GetSpacing();

	emit msg(tr("Movie export started, output file name: %1.").arg(fileName));

	double oldResliceAxesOrigin[3];
	m_channels[0]->resliceAxesOrigin(oldResliceAxesOrigin);

	double movingOrigin[3];
	for (int i = 0; i < 3; ++i)
	{
		movingOrigin[i] = imgOrigin[i];
	}
	int const sliceZAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	int const sliceFrom = imgExtent[sliceZAxisIdx * 2];
	int const sliceTo = imgExtent[sliceZAxisIdx * 2 + 1];
	for (int slice = sliceFrom; slice <= sliceTo; slice++)
	{
		movingOrigin[sliceZAxisIdx] = imgOrigin[sliceZAxisIdx] + slice * imgSpacing[sliceZAxisIdx];
		m_channels[0]->setResliceAxesOrigin(movingOrigin[0], movingOrigin[1], movingOrigin[2]);
		m_channels[0]->updateReslicer();
		m_renWin->Render();
		windowToImage->Modified();
		windowToImage->Update();
		movieWriter->Write();
		if (movieWriter->GetError())
		{
			emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
			break;
		}
		emit progress(100 * (slice - sliceFrom) / (sliceTo - sliceFrom));
	}
	m_channels[0]->setResliceAxesOrigin(oldResliceAxesOrigin[0], oldResliceAxesOrigin[1], oldResliceAxesOrigin[2]);
	update();
	movieWriter->End();
	m_interactor->Enable();

	if (movieWriter->GetError())
	{
		emit msg(tr("Movie export failed."));
	}
	else
	{
		emit msg(tr("Movie export completed."));
	}
}

void iASlicer::saveAsImage()
{
	if (!hasChannel(0))
	{
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"),
		"", // TODO: get directory of file?
		iAIOProvider::GetSupportedImageFormats());
	if (fileName.isEmpty())
	{
		return;
	}
	bool saveNative = true;
	bool output16Bit = false;

	QStringList inList = (QStringList()
		<< tr("$Save native image (intensity rescaled to output format)"));
	QList<QVariant> inPara = (QList<QVariant>()
		<< (saveNative ? tr("true") : tr("false")));

	bool moreThanOneChannel = m_channels.size() > 1;
	QFileInfo fi(fileName);
	bool outputTif = (QString::compare(fi.suffix(), "TIF", Qt::CaseInsensitive) == 0) ||
		(QString::compare(fi.suffix(), "TIFF", Qt::CaseInsensitive) == 0);

	if (moreThanOneChannel)
	{
		QStringList currentChannels;
		for (auto ch : m_channels)
		{
			currentChannels << ch->name();
		}
		inList << tr("+Channel (native only exports slice of what's selected here)");
		inPara << currentChannels;
	}
	if (outputTif)
	{
		inList << tr("$16 bit native output (if disabled, native output will be 8 bit)");
		inPara << (output16Bit ? tr("true") : tr("false"));
	}

	dlg_commoninput dlg(this, "Save options", inList, inPara, nullptr);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	saveNative = dlg.getCheckValue(0);
	if (inList.size() > 2)
	{
		output16Bit = dlg.getCheckValue(2);
	}
	iAConnector con;
	vtkSmartPointer<vtkImageData> img;
	auto windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
	if (saveNative)
	{
		int selectedChannelID = 0;
		if (moreThanOneChannel)
		{
			QString selectedChannelName = dlg.getComboBoxValue(1);
			for (auto key : m_channels.keys())
			{
				if (m_channels[key]->name() == selectedChannelName)
				{
					selectedChannelID = key;
					break;
				}
			}
		}
		con.setImage(m_channels[selectedChannelID]->output());
		iAITKIO::ImagePointer imgITK;
		if (!output16Bit)
		{
			imgITK = rescaleImageTo<unsigned char>(con.itkImage(), 0, 255);
		}
		else
		{
			imgITK = rescaleImageTo<unsigned short>(con.itkImage(), 0, 65535);
		}
		con.setImage(imgITK);
		img = con.vtkImage();
	}
	else
	{
		windowToImage->SetInput(m_renWin);
		windowToImage->Update();
		img = windowToImage->GetOutput();
	}
	writeSingleSliceImage(fileName, img);
}

void iASlicer::saveImageStack()
{
	// TODO: allow selecting channel to export? export all channels?
	if (!hasChannel(0))
	{
		return;
	}
	auto imageData = m_channels[0]->input();

	QString file = QFileDialog::getSaveFileName(this, tr("Save Image Stack"),
		"", // TODO: get directory of file?
		iAIOProvider::GetSupportedImageFormats());
	if (file.isEmpty())
	{
		return;
	}

	QFileInfo fileInfo(file);
	QString baseName = fileInfo.absolutePath() + "/" + fileInfo.baseName();

	int const * imgExtent = imageData->GetExtent();
	double const * imgSpacing = imageData->GetSpacing();
	int const sliceZAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	int const sliceMin = imgExtent[sliceZAxisIdx * 2];
	int const sliceMax = imgExtent[sliceZAxisIdx * 2 + 1];
	bool saveNative = true;
	bool output16Bit = false;
	QStringList inList = (QStringList() << tr("$Save native image (intensity rescaled to output format)")
		<< tr("*From Slice Number:")
		<< tr("*To Slice Number:"));
	QList<QVariant> inPara = (QList<QVariant>() << (saveNative ? tr("true") : tr("false")) << sliceMin << sliceMax);

	if ((QString::compare(fileInfo.suffix(), "TIF", Qt::CaseInsensitive) == 0) ||
		(QString::compare(fileInfo.suffix(), "TIFF", Qt::CaseInsensitive) == 0))
	{
		inList << tr("$16 bit native output (if disabled, native output will be 8 bit)");
		inPara << (output16Bit ? tr("true") : tr("false"));
	}
	dlg_commoninput dlg(this, "Save options", inList, inPara, nullptr);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}

	saveNative = dlg.getCheckValue(0);
	int sliceFrom = dlg.getIntValue(1);
	int sliceTo = dlg.getIntValue(2);
	if (inList.size() > 3)
	{
		output16Bit = dlg.getCheckValue(3);
	}

	if (sliceFrom < sliceMin || sliceFrom > sliceTo || sliceTo > sliceMax)
	{
		QMessageBox::information(this, "Save Image Stack", QString("Invalid input: 'From Slice Number' is greater than 'To Slice Number',"
			" or 'From Slice Number' or 'To Slice Number' are outside of valid region [0..%1]!").arg(sliceMax));
		return;
	}
	m_interactor->Disable();
	double movingOrigin[3];
	imageData->GetOrigin(movingOrigin);
	double const * imgOrigin = imageData->GetOrigin();
	auto reslicer = m_channels[0]->reslicer();
	for (int slice = sliceFrom; slice <= sliceTo; slice++)
	{
		movingOrigin[sliceZAxisIdx] = imgOrigin[sliceZAxisIdx] + slice * imgSpacing[sliceZAxisIdx];
		setResliceAxesOrigin(movingOrigin[0], movingOrigin[1], movingOrigin[2]);
		m_channels[0]->updateReslicer();

		auto windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
		iAConnector con;
		vtkImageData* img;
		if (saveNative)
		{
			con.setImage(reslicer->GetOutput());
			iAITKIO::ImagePointer imgITK;
			if (!output16Bit)
			{
				imgITK = rescaleImageTo<unsigned char>(con.itkImage(), 0, 255);
			}
			else
			{
				imgITK = rescaleImageTo<unsigned short>(con.itkImage(), 0, 65535);
			}
			con.setImage(imgITK);
			img = con.vtkImage();
		}
		else
		{
			m_renWin->Render();
			windowToImage->SetInput(m_renWin);
			windowToImage->ReadFrontBufferOff();
			windowToImage->Update();
			img = windowToImage->GetOutput();
		}
		emit progress(100 * (slice-sliceFrom) / (sliceTo - sliceFrom) );

		QString newFileName(QString("%1%2.%3").arg(baseName).arg(slice).arg(fileInfo.suffix()));
		writeSingleSliceImage(newFileName, img);
	}
	m_interactor->Enable();
	emit msg(tr("Image stack saved in folder: %1")
		.arg(fileInfo.absoluteDir().absolutePath()));
}

void iASlicer::updatePositionMarkerExtent()
{
	if (m_channels.empty() || !m_positionMarkerSrc)
	{
		return;
	}
	// TODO: how to choose spacing? currently fixed from first image? export all channels?
	auto imageData = m_channels[0]->input();
	m_positionMarkerSrc->SetXLength(m_ext * imageData->GetSpacing()[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)]);
	m_positionMarkerSrc->SetYLength(m_ext * imageData->GetSpacing()[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]);
	m_positionMarkerSrc->SetZLength(0);
}

void iASlicer::setStatisticalExtent(int statExt)
{
	m_ext = statExt;
	updatePositionMarkerExtent();
}

void iASlicer::updateBackground()
{
	if (m_userSetBackground)
	{
		m_ren->SetBackground(m_backgroundRGB);
		return;
	}
	switch (m_mode)
	{
		default:
		case iASlicerMode::YZ: m_ren->SetBackground(0.2, 0.2, 0.2); break;
		case iASlicerMode::XY: m_ren->SetBackground(0.3, 0.3, 0.3); break;
		case iASlicerMode::XZ: m_ren->SetBackground(0.6, 0.6, 0.6); break;
	}
}

void iASlicer::setBackground(double r, double g, double b)
{
	m_userSetBackground = true;
	m_backgroundRGB[0] = r;
	m_backgroundRGB[1] = g;
	m_backgroundRGB[2] = b;
	updateBackground();
}

void iASlicer::execute(vtkObject * /*caller*/, unsigned long eventId, void * /*callData*/)
{
	if (m_channels.empty())
	{
		return;
	}
	if (eventId == vtkCommand::LeftButtonPressEvent)
	{
		emit clicked();
	}
	if (eventId == vtkCommand::MouseWheelForwardEvent ||
		eventId == vtkCommand::MouseWheelBackwardEvent)
	{
		emit userInteraction();
	}
	updatePosition();

	double channel0Coords[3];
	computeCoords(channel0Coords, 0);
	switch (eventId)
	{
	case vtkCommand::LeftButtonPressEvent:
	{
		emit clicked(channel0Coords[0], channel0Coords[1], channel0Coords[2]);
		emit userInteraction();
		break;
	}
	case vtkCommand::LeftButtonReleaseEvent:
	{
		emit released(channel0Coords[0], channel0Coords[1], channel0Coords[2]);
		emit userInteraction();
		break;
	}
	case vtkCommand::RightButtonPressEvent:
	{
		emit rightClicked(channel0Coords[0], channel0Coords[1], channel0Coords[2]);
		break;
	}
	case vtkCommand::MouseMoveEvent:
	{
		//DEBUG_LOG("iASlicer::execute vtkCommand::MouseMoveEvent");
		if (m_decorations)
		{
			m_positionMarkerActor->SetVisibility(false);
			printVoxelInformation();
		}
		emit oslicerPos(channel0Coords[0], channel0Coords[1], channel0Coords[2], m_mode);
		emit userInteraction();
		break;
	}
	case vtkCommand::KeyPressEvent:
		if (m_decorations)
		{
			executeKeyPressEvent();
		}
		break;
	case vtkCommand::KeyReleaseEvent:
		if (m_interactor->GetKeyCode() == 'p')
		{
			emit pick();
		}
		break;
	default:
		if (m_interactor->GetKeyCode() == 'p')
		{
			emit pick();
		}
		break;
	}

	m_interactor->Render();
}

void iASlicer::updatePosition()
{
	// get slicer event position:
	int const * epos = m_interactor->GetEventPosition();
	m_pointPicker->Pick(epos[0], epos[1], 0, m_ren); // z is always zero
	m_pointPicker->GetPickPosition(m_slicerPt);      // get position in local slicer scene/world coordinates

	// compute global point:
	const int ChannelID = 0; //< TODO: avoid using specific channel here!
	if (!hasChannel(ChannelID))
	{
		std::fill(m_globalPt, m_globalPt + 3, 0);
		return;
	}
	double point[4] = { m_slicerPt[0], m_slicerPt[1], m_slicerPt[2], 1 };
	auto reslicer = m_channels[ChannelID]->reslicer();
	vtkMatrix4x4 *resliceAxes = vtkMatrix4x4::New();
	resliceAxes->DeepCopy(reslicer->GetResliceAxes());
	resliceAxes->MultiplyPoint(point, m_globalPt);
	resliceAxes->Delete();
}

void iASlicer::computeCoords(double * coord, uint channelID)
{
	if (!hasChannel(channelID))
	{
		return;
	}

	auto imageData = m_channels[channelID]->input();
	double const * imgSpacing = imageData->GetSpacing();
	double const * imgOrigin  = imageData->GetOrigin();
	int    const * imgExtent  = imageData->GetExtent();
	// coords will contain voxel coordinates for the given channel
	for (int i = 0; i < 3; ++i)
	{
		coord[i] = clamp(
			static_cast<double>(imgExtent[i * 2]),
			imgExtent[i * 2 + 1] + 1 - std::numeric_limits<double>::epsilon(),
			(m_globalPt[i] - imgOrigin[i]) / imgSpacing[i] + 0.5);	// + 0.5 to correct for BorderOn
	}

	// TODO: check for negative origin images!
}

namespace
{
	const int MaxNameLength = 15;

	QString filePixel(iASlicer* slicer, int const * coord, int slicerXAxisIdx, int slicerYAxisIdx)
	{
		vtkImageData* sliceImg = slicer->channel(0)->output();
		int const * dim = sliceImg->GetDimensions();
		bool inRange =
			coord[slicerXAxisIdx] >= 0     && coord[slicerYAxisIdx] >= 0 &&
			coord[slicerXAxisIdx] < dim[0] && coord[slicerYAxisIdx] < dim[1];
		double tmpPix = (inRange) ?
			sliceImg->GetScalarComponentAsDouble(coord[slicerXAxisIdx], coord[slicerYAxisIdx], 0, 0): 0;
		QString file = slicer->channel(0)->name();
		return QString("%1: %2 [%3 %4 %5]\n")
			.arg(padOrTruncate(file, MaxNameLength))
			.arg(inRange ? QString::number(tmpPix) : "exceeds img. dim.")
			.arg(coord[0]).arg(coord[1]).arg(coord[2]);
	}

	const double FisheyeMinRadius = 4.0;
	const double FisheyeMaxRadius = 220.0;

	double fisheyeMinInnerRadius(double radius) { return std::max(1, static_cast<int>((radius - 1) * 0.7)); }
}

// Qt versions before 5.10 don't have these operators yet:
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
bool operator==(QCursor const & a, QCursor const & b)
{
	if (a.shape() != Qt::BitmapCursor)
	{
		return a.shape() == b.shape();
	}
	return b.shape() == Qt::BitmapCursor &&
		a.hotSpot() == b.hotSpot() &&
		(a.pixmap() == b.pixmap() || (a.bitmap() == b.bitmap() && a.mask() == b.mask()));
}
bool operator!=(QCursor const & a, QCursor const & b)
{
	return !operator==(a, b);
}
#endif

void iASlicer::printVoxelInformation()
{
	if (!m_decorations)
	{
		return;
	}
	if (m_cursorSet && cursor() != mouseCursor())
	{
		setCursor(mouseCursor());
	}
	QString strDetails(QString("%1: %2, %3, %4\n").arg(padOrTruncate("Position", MaxNameLength))
		.arg(m_globalPt[0]).arg(m_globalPt[1]).arg(m_globalPt[2]));
	for (auto channelID: m_channels.keys())
	{
		if (!m_channels[channelID]->isEnabled())
		{
			continue;
		}

		double const * slicerSpacing = m_channels[channelID]->output()->GetSpacing();
		int    const * slicerExtent  = m_channels[channelID]->output()->GetExtent();
		double const * slicerBounds  = m_channels[channelID]->output()->GetBounds();
		double dcX = (m_slicerPt[0] - slicerBounds[0]) / slicerSpacing[0] + 0.5;
		double dcY = (m_slicerPt[1] - slicerBounds[2]) / slicerSpacing[1] + 0.5;
		int cX = static_cast<int>(std::floor(dcX));
		int cY = static_cast<int>(std::floor(dcY));

		// check image extent; if outside ==> default output
		if (cX < slicerExtent[0] || cX > slicerExtent[1] || cY < slicerExtent[2] || cY > slicerExtent[3])
		{
			continue;
		}
		QString valueStr;
		for (int i = 0; i < m_channels[channelID]->input()->GetNumberOfScalarComponents(); i++)
		{
			// TODO:
			//   - consider slab thickness / print slab projection result
			double value = m_channels[channelID]->output()->GetScalarComponentAsDouble(cX, cY, 0, i);
			if (i > 0)
			{
				valueStr += " ";
			}
			valueStr += QString::number(value);
		}
		double coords[3];
		computeCoords(coords, channelID);
		strDetails += QString("%1: %2 [%3 %4 %5]\n")
			.arg(padOrTruncate(m_channels[channelID]->name(), MaxNameLength))
			.arg(valueStr)
			.arg(static_cast<int>(coords[0])).arg(static_cast<int>(coords[1])).arg(static_cast<int>(coords[2]));
	}
	if (m_linkedMdiChild)
	{
		QList<MdiChild*> mdiwindows = m_linkedMdiChild->mainWnd()->mdiChildList();
		for (int i = 0; i < mdiwindows.size(); i++)
		{
			MdiChild *tmpChild = mdiwindows.at(i);
			if (m_linkedMdiChild == tmpChild)
			{
				continue;
			}

			double * const tmpSpacing = tmpChild->imagePointer()->GetSpacing();
			int tmpCoord[3];
			for (int c = 0; c < 3; ++c)
			{
				tmpCoord[c] = static_cast<int>(m_globalPt[c] / tmpSpacing[c]);
			}
			int slicerXAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::X),
				slicerYAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y),
				slicerZAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
			tmpChild->slicer(m_mode)->setPositionMarkerCenter(m_globalPt[slicerXAxisIdx], m_globalPt[slicerYAxisIdx]);
			tmpChild->slicer(m_mode)->setIndex(tmpCoord[0], tmpCoord[1], tmpCoord[2]);
			tmpChild->slicerDockWidget(m_mode)->sbSlice->setValue(tmpCoord[slicerZAxisIdx]);
			tmpChild->slicer(m_mode)->update();
			strDetails += filePixel(tmpChild->slicer(m_mode), tmpCoord, slicerXAxisIdx, slicerYAxisIdx);
			tmpChild->update();
		}
	}

	// if requested calculate distance and show actor
	if (m_lineActor && m_lineActor->GetVisibility())
	{
		double distance = sqrt(pow((m_startMeasurePoint[0] - m_slicerPt[0]), 2) +
			pow((m_startMeasurePoint[1] - m_slicerPt[1]), 2));
		m_lineSource->SetPoint2(m_slicerPt[0], m_slicerPt[1], 0.0);
		m_diskSource->SetOuterRadius(distance);
		m_diskSource->SetInnerRadius(distance);
		m_diskSource->Update();
		strDetails += QString("%1: %2\n").arg(padOrTruncate("Distance", MaxNameLength)).arg(distance);
	}

	// Update the info text with pixel coordinates/value if requested.
	m_textInfo->GetActor()->SetPosition(m_interactor->GetEventPosition()[0] + 10, m_interactor->GetEventPosition()[1] + 10);
	m_textInfo->GetTextMapper()->SetInput(strDetails.toStdString().c_str());
	m_positionMarkerMapper->Update();
}

void iASlicer::executeKeyPressEvent()
{
	switch (m_interactor->GetKeyCode())
	{
	case 'm':
		m_startMeasurePoint[0] = m_slicerPt[0];
		m_startMeasurePoint[1] = m_slicerPt[1];
		// does not work reliably (often snaps to positions not the highest gradient close to the current position)
		// and causes access to pixels outside of the image:
		//snapToHighGradient(m_startMeasurePoint[0], m_startMeasurePoint[1]);
		if (m_decorations && m_lineSource)
		{
			m_lineSource->SetPoint1(m_startMeasurePoint[0], m_startMeasurePoint[1], 0.0);
			m_diskActor->SetPosition(m_startMeasurePoint[0], m_startMeasurePoint[1], 0.0);
			m_lineActor->SetVisibility(true);
			m_diskActor->SetVisibility(true);
			printVoxelInformation();
		}
		break;
	case 27: //ESCAPE
		if (m_decorations && m_lineActor)
		{
			m_lineActor->SetVisibility(false);
			m_diskActor->SetVisibility(false);
			printVoxelInformation();
		}
		break;
	}
}

void iASlicer::defaultOutput()
{
	if (!m_decorations)
	{
		return;
	}
	QString strDetails(" ");
	m_textInfo->GetActor()->SetPosition(20, 20);
	m_textInfo->GetTextMapper()->SetInput(strDetails.toStdString().c_str());
	m_positionMarkerActor->SetVisibility(false);
	m_interactor->ReInitialize();
	m_interactor->Render();
}

/*
void iASlicer::snapToHighGradient(double &x, double &y)
{
	double range[2];
	imageData->GetScalarRange(range);
	double gradThresh = range[1] * 0.05;

	double * imageSpacing = imageData->GetSpacing();
	double * imageBounds = imageData->GetBounds();

	int xInd = mapSliceToGlobalAxis(m_mode, iAAxisIndex::X);
	int yInd = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y);
	double coord1 = (int)(x/imageSpacing[xInd]);
	double coord2 = (int)(y/imageSpacing[yInd]);
	double dataCoord1 = (int)((x-imageBounds[xInd*2])/imageSpacing[xInd]);
	double dataCoord2 = (int)((y-imageBounds[yInd*2])/imageSpacing[yInd]);

	std::list<double>H_x;
	std::list<double>H_y;
	std::list<double>H_grad;

	double H_maxGradMag = 0;
	double H_maxcoord[2];H_maxcoord[0] = 0; H_maxcoord[1] = 0;
	double cursorposition[2]; cursorposition[0] = coord1; cursorposition[1] = coord2;
	//move horizontally
	for (int i = -2; i <= 2; i++)
	{
		//if ( i != 0)
		{
			double center[2], right[2], left[2], top[2], bottom[2];
			center[0] = dataCoord1 + i; center[1] = dataCoord2;
			left[0] = dataCoord1 + i - 1; left[1] = dataCoord2;
			right[0] = dataCoord1 + i + 1; right[1] = dataCoord2;
			top[0] = dataCoord1 + i; top[1] = dataCoord2+1;
			bottom[0] = dataCoord1 + i; bottom[1] = dataCoord2 - 1;

			double left_pix = imageData->GetScalarComponentAsDouble(left[0],left[1],0,0);
			double right_pix = imageData->GetScalarComponentAsDouble(right[0],right[1],0,0);

			double top_pix = imageData->GetScalarComponentAsDouble(top[0],top[1],0,0);
			double bottom_pix = imageData->GetScalarComponentAsDouble(bottom[0],bottom[1],0,0);

			double derivativeX = fabs(right_pix - left_pix);
			double derivativeY = fabs(top_pix - bottom_pix);

			double gradmag = sqrt ( pow(derivativeX,2) + pow(derivativeY,2) );

			H_x.push_back(center[0]);
			H_y.push_back(center[1]);
			H_grad.push_back(gradmag);

			//check whether the calculated gradient magnitude => maxGradientMagnitude
			if ( gradmag >= H_maxGradMag )
			{
				//check whether the gradient magnitude = to maxgradient magnitude
				//if yes calculate the distance between the cursorposition and center called newdist
				// and calculate the distance between the cursorposition and HMaxCoord called the maxdist
				//if NO take the current center position as the HMaxCoord
				if ( gradmag == H_maxGradMag )
				{
					//calculate the distance
					double newdist = sqrt (pow( (cursorposition[0]-center[0]),2) + pow( (cursorposition[1]-center[1]),2));
					double maxdist = sqrt (pow( (cursorposition[0]-H_maxcoord[0]),2) + pow( (cursorposition[1]-H_maxcoord[1]),2));
					//if newdist is < than the maxdist (meaning the current center position is closer to the cursor position
					//replace the hMaxCoord with the current center position
					if ( newdist < maxdist )
					{
						H_maxcoord[0] = center[0];
						H_maxcoord[1] = center[1];
					}//if
				}//if else
				else
				{
					H_maxGradMag = gradmag;
					H_maxcoord[0] = center[0];
					H_maxcoord[1] = center[1];
				}//if else

			}//if
		}//if
	}//for

	std::list<double>V_x;
	std::list<double>V_y;
	std::list<double>V_grad;

	double V_maxGradMag = 0;
	double V_maxcoord[2]; V_maxcoord[0] = 0; V_maxcoord[1] = 0;
	//move vertically
	for (int i = -2; i <= 2; i++)
	{
		if ( i != 0 )
		{
			double center[2], right[2], left[2], top[2], bottom[2];
			center[0] = dataCoord1; center[1] = dataCoord2+i;
			left[0] = dataCoord1-1; left[1] = dataCoord2+i;
			right[0] = dataCoord1+1; right[1] = dataCoord2+i;
			top[0] = dataCoord1; top[1] = dataCoord2+i+1;
			bottom[0] = dataCoord1; bottom[1] = dataCoord2+i-1;

			double left_pix = imageData->GetScalarComponentAsDouble(left[0],left[1],0,0);
			double right_pix = imageData->GetScalarComponentAsDouble(right[0],right[1],0,0);

			double top_pix = imageData->GetScalarComponentAsDouble(top[0],top[1],0,0);
			double bottom_pix = imageData->GetScalarComponentAsDouble(bottom[0],bottom[1],0,0);

			double derivativeX = fabs(right_pix - left_pix);
			double derivativeY = fabs(top_pix - bottom_pix);

			double gradmag = sqrt ( pow(derivativeX,2) + pow(derivativeY,2) );

			V_x.push_back(center[0]);
			V_y.push_back(center[1]);
			V_grad.push_back(gradmag);

			//check whether the calculated gradient magnitude => maxGradientMagnitude
			if ( gradmag >= V_maxGradMag )
			{
				//check whether the gradient magnitude = to maxgradient magnitude
				//if yes calculate the distance between the cursorposition and center called newdist
				// and calculate the distance between the cursorposition and HMaxCoord called the maxdist
				//if NO take the current center position as the HMaxCoord
				if ( gradmag == V_maxGradMag )
				{
					//calculate the distance
					double newdist = sqrt (pow( (cursorposition[0]-center[0]),2) + pow( (cursorposition[1]-center[1]),2));
					double maxdist = sqrt (pow( (cursorposition[0]-V_maxcoord[0]),2) + pow( (cursorposition[1]-V_maxcoord[1]),2));
					//if newdist is < than the maxdist (meaning the current center position is closer to the cursor position
					//replace the hMaxCoord with the current center position
					if ( newdist < maxdist )
					{
						V_maxcoord[0] = center[0];
						V_maxcoord[1] = center[1];
					}//if
				}//if else
				else
				{
					V_maxGradMag = gradmag;
					V_maxcoord[0] = center[0];
					V_maxcoord[1] = center[1];
				}//if else
			}//if
		}//if
	}//for

	//checking whether the V_maxGradMag and H_maxGradMag is higher than the gradient threshold
	bool v_bool = false, h_bool = false;
	if ( V_maxGradMag >= gradThresh )
		v_bool = true;
	if (H_maxGradMag >= gradThresh)
		h_bool = true;

	//selection of V_maxGradMag or H_maxGradMag as new point
	int pointselectionkey;
	if ( v_bool == false && h_bool == true )
		pointselectionkey = 1; //new point is in horizontal direction H_maxcoord
	else if ( v_bool == true && h_bool == false )
		pointselectionkey = 2; //new point is in horizontal direction V_maxcoord
	else if ( v_bool == true && h_bool == true )
	{
		//pointselectionkey = 3; //new point is shortest distance between V_maxcoord,currentposition and H_maxcoord ,currentposition
		double Hdist = sqrt (pow( (cursorposition[0]-H_maxcoord[0]),2) + pow( (cursorposition[1]-H_maxcoord[1]),2));
		double Vdist = sqrt (pow( (cursorposition[0]-V_maxcoord[0]),2) + pow( (cursorposition[1]-V_maxcoord[1]),2));
		if ( Hdist < Vdist )
			pointselectionkey = 1; //new point is in horizontal direction H_maxcoord
		else if ( Hdist > Vdist )
			pointselectionkey = 2; //new point is in horizontal direction V_maxcoord
		else
			pointselectionkey = 1; //new point is in horizontal direction H_maxcoord
	}
	else
	{
		// do nothing as v_bool and h_bool are false which means the gradient difference in both direction is not >= to grad threshold
		// and the cursor position is the final position meaning no change in the position
	}
	switch(pointselectionkey)
	{
	case 1:
		x = H_maxcoord[0]*imageSpacing[xInd];
		y = H_maxcoord[1]*imageSpacing[yInd];
		break;
	case 2:
		x = V_maxcoord[0]*imageSpacing[xInd];
		y = V_maxcoord[1]*imageSpacing[yInd];
		break;
	}
}
*/

void iASlicer::setShowText(bool isVisible)
{
	if (!m_decorations)
	{
		return;
	}
	m_textInfo->Show(isVisible);
}

void iASlicer::enableChannel(uint id, bool enabled)
{
	channel(id)->setEnabled(m_ren, enabled);
}

void iASlicer::updateChannel(uint id, iAChannelData const & chData)
{
	channel(id)->update(chData);
}

void iASlicer::setResliceChannelAxesOrigin(uint id, double x, double y, double z)
{
	channel(id)->setResliceAxesOrigin(x, y, z);
}

void iASlicer::setChannelOpacity(uint id, double opacity)
{
	channel(id)->setActorOpacity(opacity);
}

void iASlicer::setSlabThickness(int thickness)
{
	m_slabThickness = thickness;
	for (auto ch : m_channels)
	{
		ch->setSlabNumberOfSlices(thickness);
	}
	update();
}

void iASlicer::setSlabCompositeMode(int slabCompositeMode)
{
	m_slabCompositeMode = slabCompositeMode;
	for (auto ch : m_channels)
	{
		ch->setSlabMode(slabCompositeMode);
	}
	update();
}

QSharedPointer<iAChannelSlicerData> iASlicer::createChannel(uint id, iAChannelData const & chData)
{
	if (m_channels.contains(id))
	{
		throw std::runtime_error(QString("iASlicer: Channel with ID %1 already exists!").arg(id).toStdString());
	}

	QSharedPointer<iAChannelSlicerData> newData(new iAChannelSlicerData(chData, m_mode));
	newData->setInterpolate(m_settings.LinearInterpolation);
	newData->setSlabNumberOfSlices(m_slabThickness);
	newData->setSlabMode(m_slabCompositeMode);
	newData->setTransform(m_transform);
	m_channels.insert(id, newData);
	return newData;
}

iAChannelSlicerData * iASlicer::channel(uint id)
{
	assert(m_channels.contains(id));
	if (!m_channels.contains(id))
	{
		return nullptr;
	}
	return m_channels.find(id)->data();
}

void iASlicer::removeChannel(uint id)
{
	if (channel(id)->isEnabled())
	{
		enableChannel(id, false);
	}
	m_channels.remove(id);
}

bool iASlicer::hasChannel(uint id) const
{
	return m_channels.contains(id);
}

void iASlicer::setCamera(vtkCamera* cam, bool camOwner /*=true*/)
{
	m_ren->SetActiveCamera(cam);
	if (m_camera && m_cameraOwner)
	{
		m_camera->Delete();
	}
	m_cameraOwner = camOwner;
	m_camera = cam;
}

void iASlicer::resetCamera()
{
	m_ren->ResetCamera();
}

void iASlicer::updateChannelMappers()
{
	for (auto chData: m_channels)
	{
		chData->updateLUT();
		chData->updateMapper();
	}
}

void iASlicer::rotateSlice(double angle)
{
	m_angle[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z)] = angle;

	if (!hasChannel(0))
	{
		return;
	}

	double center[3];

	// TODO: allow selecting center for rotation? current: always use first image!
	auto imageData = m_channels[0]->input();
	double* spacing = imageData->GetSpacing();
	int* ext = imageData->GetExtent();

	center[0] = (ext[1] - ext[0]) / 2 * spacing[0];
	center[1] = (ext[3] - ext[2]) / 2 * spacing[1];
	center[2] = (ext[5] - ext[4]) / 2 * spacing[2];

	vtkTransform *t1 = vtkTransform::New();
	t1->Translate(-center[0], -center[1], -center[2]);

	vtkTransform *t2 = vtkTransform::New();
	t2->RotateWXYZ(m_angle[0], 1, 0, 0);
	t2->RotateWXYZ(m_angle[1], 0, 1, 0);
	t2->RotateWXYZ(m_angle[2], 0, 0, 1);

	vtkTransform *t3 = vtkTransform::New();
	t3->Translate(center[0], center[1], center[2]);

	auto transform = vtkTransform::New();
	transform->Identity();
	transform->PostMultiply();
	transform->Concatenate(t1);
	transform->Concatenate(t2);
	transform->Concatenate(t3);
	setTransform(transform);

	update();
	emit sliceRotated();
}

/*
void iASlicer::switchContourSourceToChannel(uint id )
{
	if (!m_decorations)
		return;

	cFilter->SetInputConnection( getChannel(id)->reslicer->GetOutputPort() );
}
*/

void iASlicer::setContours(int numberOfContours, double contourMin, double contourMax)
{
	if (!m_decorations)
	{
		return;
	}
	for (auto ch : m_channels)
	{
		ch->setContours(numberOfContours, contourMin, contourMax);
	}
}

void iASlicer::setContours(int numberOfContours, double const * contourValues)
{
	if (!m_decorations)
	{
		return;
	}
	for (auto ch : m_channels)
	{
		ch->setContours(numberOfContours, contourValues);
	}
}

void iASlicer::setMouseCursor(QString const & s)
{
	QString color = s.section(' ', -1);
	if (color != "default")
	{
		QString shape = s.section(' ', 0, 0);
		QPixmap pm;
		if (shape == "Crosshair")
		{
			pm = QPixmap(":/images/" + s.section(' ', 0, 1) + ".png");
		}
		QPixmap cpm(pm.size());
		cpm.fill(color);
		cpm.setMask(pm.createMaskFromColor(Qt::transparent));
		m_mouseCursor = QCursor(cpm);
	}
	else
	{
		m_mouseCursor = QCursor(Qt::CrossCursor);
	}
	setCursor(mouseCursor());
	m_cursorSet = true;
}

void iASlicer::setScalarBarTF(vtkScalarsToColors* ctf)
{
	if (!m_scalarBarWidget)
	{
		return;
	}
	m_scalarBarWidget->GetScalarBarActor()->SetLookupTable(ctf);
	m_scalarBarWidget->SetEnabled(ctf != nullptr);
}

QCursor iASlicer::mouseCursor()
{
	return m_mouseCursor;
}

int iASlicer::sliceNumber() const
{
	return m_sliceNumber;
}

void iASlicer::keyPressEvent(QKeyEvent *event)
{
	// TODO: merge with iASlicer::execute, switch branch vtkCommand::KeyPressEvent ?
	if (!hasChannel(0))
	{
		return;
	}

	vtkRenderer * ren = m_renWin->GetRenderers()->GetFirstRenderer();
	if (event->key() == Qt::Key_R)
	{
		ren->ResetCamera();
	}
	if (event->key() == Qt::Key_O)
	{
		// TODO: fisheye lens on all channels???

		auto reslicer = channel(0)->reslicer();
		if (!m_fisheyeLensActivated)
		{
			m_fisheyeLensActivated = true;
			reslicer->SetAutoCropOutput(!reslicer->GetAutoCropOutput());
			ren->SetWorldPoint(m_slicerPt[0], m_slicerPt[1], 0, 1);

			initializeFisheyeLens(reslicer);

			updateFisheyeTransform(ren->GetWorldPoint(), reslicer, m_fisheyeRadius, m_innerFisheyeRadius);
		}
		else
		{
			m_fisheyeLensActivated = false;
			reslicer->SetAutoCropOutput(!reslicer->GetAutoCropOutput());

			// Clear outdated circles and actors (not needed for final version)
			for (int i = 0; i < m_circle1ActList.length(); ++i)
			{
				ren->RemoveActor(m_circle1ActList.at(i));
			}
			//circle1List.clear();
			m_circle1ActList.clear();

			for (int i = 0; i < m_circle2ActList.length(); ++i)
			{
				ren->RemoveActor(m_circle2ActList.at(i));
			}
			m_circle2List.clear();
			m_circle2ActList.clear(); //*/

			ren->RemoveActor(m_fisheyeActor);

			// No fisheye transform
			double bounds[6];
			reslicer->GetInformationInput()->GetBounds(bounds);
			m_pointsTarget->SetNumberOfPoints(4);
			m_pointsSource->SetNumberOfPoints(4);
			m_pointsTarget->SetPoint(0, bounds[0], bounds[2], 0); //x_min, y_min, bottom left
			m_pointsTarget->SetPoint(1, bounds[0], bounds[3], 0); //x_min, y_max, top left
			m_pointsTarget->SetPoint(2, bounds[1], bounds[3], 0); //x_max, y_max, top right
			m_pointsTarget->SetPoint(3, bounds[1], bounds[2], 0); //x_max, y_min, bottom right
			m_pointsSource->SetPoint(0, bounds[0], bounds[2], 0); //x_min, y_min, bottom left
			m_pointsSource->SetPoint(1, bounds[0], bounds[3], 0); //x_min, y_max, top left
			m_pointsSource->SetPoint(2, bounds[1], bounds[3], 0); //x_max, y_max, top right
			m_pointsSource->SetPoint(3, bounds[1], bounds[2], 0); //x_max, y_min, bottom right

			m_fisheyeTransform->SetSourceLandmarks(m_pointsSource);
			m_fisheyeTransform->SetTargetLandmarks(m_pointsTarget);
			reslicer->SetResliceTransform(m_fisheyeTransform);
			update();
		}
	}

	// magnify and unmagnify fisheye lens and distortion radius
	if (m_fisheyeLensActivated && (
		event->key() == Qt::Key_Minus ||
		event->key() == Qt::Key_Plus
#if defined(VK_OEM_PLUS)
		|| event->nativeVirtualKey() == VK_OEM_PLUS // "native..." - workaround required for Ctrl+ "+" with non-numpad "+" key
#endif
		))
	{
		ren->SetWorldPoint(m_slicerPt[0], m_slicerPt[1], 0, 1);

		// TODO: fisheye lens on all channels???
		auto reslicer = channel(0)->reslicer();
		double oldInnerRadius = m_innerFisheyeRadius,
			oldRadius = m_fisheyeRadius;
		if (event->modifiers().testFlag(Qt::ControlModifier))
		{
			// change inner radius (~ zoom level) alone:
			double ofs = ((m_fisheyeRadius <= 20.0) ? 1 : 2)    // how much to change it
				* ((event->key() != Qt::Key_Minus) ? -1 : 1);   // which direction (!= instead of ==, as below, is intended!)
			m_innerFisheyeRadius = clamp(fisheyeMinInnerRadius(m_fisheyeRadius), m_fisheyeRadius, m_innerFisheyeRadius + ofs);
		}
		else
		{
			// change radius of displayed fisheye lens, which requires changing inner radius as well:
			double ofs = ((m_fisheyeRadius <= 20) ? 1 : 10)     // how much to change it
				* ((event->key() == Qt::Key_Minus) ? -1 : 1);   // which direction
			m_fisheyeRadius = clamp(FisheyeMinRadius, FisheyeMaxRadius, m_fisheyeRadius + ofs);
			if (m_fisheyeRadius != oldRadius)
			{
				m_innerFisheyeRadius = clamp(fisheyeMinInnerRadius(m_fisheyeRadius), m_fisheyeRadius, m_innerFisheyeRadius + ofs);
			}
		}
		if (oldRadius != m_fisheyeRadius || oldInnerRadius != m_innerFisheyeRadius) // only update if something changed
		{
			updateFisheyeTransform(ren->GetWorldPoint(), reslicer, m_fisheyeRadius, m_innerFisheyeRadius);
		}
	}
	if (event->key() == Qt::Key_S)
	{
		switch (m_interactionMode)
		{   // toggle between interaction modes:
			case Normal   : switchInteractionMode(SnakeEdit); break;
			case SnakeEdit: switchInteractionMode(/*SnakeShow*/Normal); break;
			case SnakeShow: switchInteractionMode(Normal);    break;
		}
		// let other slice views know that interaction mode changed
		emit switchedMode(m_interactionMode);
	}
	iAVtkWidget::keyPressEvent(event);
}

void iASlicer::mousePressEvent(QMouseEvent *event)
{
	// TODO: merge with iASlicer::execute, switch branch vtkCommand::LeftButtonPressEvent ?
	if (m_channels.empty())
	{
		iAVtkWidget::mousePressEvent(event);
		return;
	}
	if (m_isSliceProfEnabled
		&& (event->modifiers() == Qt::NoModifier)
		&& event->button() == Qt::LeftButton)//if slice profile m_viewMode is enabled do all the necessary operations
	{
		updateRawProfile(m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]);
	}

	if (m_isArbProfEnabled
		&& (event->modifiers() == Qt::NoModifier)
		&& event->button() == Qt::LeftButton)//if arbitrary profile m_viewMode is enabled do all the necessary operations
	{
		m_arbProfile->findSelectedPointIdx(m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)], m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]);
	}

	if (m_decorations && m_interactionMode == SnakeEdit && event->button() == Qt::LeftButton)
	{
		// define a new snake slicer point:
		const double x = m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)];
		const double y = m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)];

		// if no point is found at picked position add a new one
		if (m_snakeSpline->CalculateSelectedPoint(x, y) == iASnakeSpline::NoPointSelected)
		{
			m_snakeSpline->addPoint(x, y);
			// add the point to the world point list only once because it is a member of MdiChild
			m_worldSnakePoints->InsertNextPoint(m_globalPt[0], m_globalPt[1], m_globalPt[2]);
			// let other slices views know that a new point was created
			emit addedPoint(m_globalPt[0], m_globalPt[1], m_globalPt[2]);
		}
	}
	iAVtkWidget::mousePressEvent(event);
}

void iASlicer::mouseMoveEvent(QMouseEvent *event)
{
	// TODO: merge with iASlicer::execute, switch branch vtkCommand::MouseMoveEvent ?
	iAVtkWidget::mouseMoveEvent(event);

	if (!hasChannel(0)) // nothing to do if no data
	{
		return;
	}

	if (m_fisheyeLensActivated)
	{
		// TODO: fisheye lens on all channels???
		auto reslicer = channel(0)->reslicer();
		vtkRenderer * ren = m_renWin->GetRenderers()->GetFirstRenderer();
		ren->SetWorldPoint(m_slicerPt[0], m_slicerPt[1], 0, 1);
		updateFisheyeTransform(ren->GetWorldPoint(), reslicer, m_fisheyeRadius, m_innerFisheyeRadius);
	}

	if (!event->modifiers().testFlag(Qt::ShiftModifier))
	{
		updateMagicLens();
	}

	// only do something in spline drawing mode and if a point is selected
	if (m_decorations && m_interactionMode == SnakeEdit && m_snakeSpline->selectedPointIndex() != iASnakeSpline::NoPointSelected)
	{
		// Move world and slice view points
		double const * point = m_worldSnakePoints->GetPoint(m_snakeSpline->selectedPointIndex());

		double pos[3];
		int indxs[3] = { mapSliceToGlobalAxis(m_mode, iAAxisIndex::X), mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y), mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z) };

		for (int i = 0; i < 2; ++i)         // Update the two coordinates in the slice plane (slicer's x and y) from the current position in the slicer
		{
			pos[indxs[i]] = m_slicerPt[i];
		}
		pos[indxs[2]] = point[indxs[2]];	// Update the other coordinate (slicer's z) from the current global position of the point

		movePoint(m_snakeSpline->selectedPointIndex(), pos[0], pos[1], pos[2]);

		// update world point list only once because it is a member of MdiChild
		m_worldSnakePoints->SetPoint(m_snakeSpline->selectedPointIndex(), pos[0], pos[1], pos[2]);

		// let other slice views know that a point was moved
		emit movedPoint(m_snakeSpline->selectedPointIndex(), pos[0], pos[1], pos[2]);
	}

	if (m_isSliceProfEnabled && (event->modifiers() == Qt::NoModifier) && (event->buttons() & Qt::LeftButton))
	{
		updateRawProfile(m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]);
	}
	if (m_isArbProfEnabled)
	{
		int arbProfPointIdx = m_arbProfile->pointIdx();
		if (event->modifiers() == Qt::NoModifier && arbProfPointIdx >= 0)
		{
			if (event->buttons() & Qt::LeftButton)
			{
				double const * ptPos = m_arbProfile->position(arbProfPointIdx);
				const int zind = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
				double globalPos[3];
				std::copy(m_globalPt, m_globalPt + 3, globalPos);
				globalPos[zind] = ptPos[zind];

				if (setArbitraryProfile(arbProfPointIdx, globalPos, true))
				{
					emit arbitraryProfileChanged(arbProfPointIdx, globalPos);
				}
			}
		}
	}
}

void iASlicer::deselectPoint()
{
	if (!m_decorations)
	{
		return;
	}
	m_snakeSpline->deselectPoint();
}

void iASlicer::mouseReleaseEvent(QMouseEvent *event)
{
	iAVtkWidget::mouseReleaseEvent(event);
	if (m_decorations)
	{
		m_snakeSpline->deselectPoint();
		emit deselectedPoint();  // let other slice views know that the point was deselected
	}
}

void iASlicer::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		emit dblClicked();
	}
}

void iASlicer::contextMenuEvent(QContextMenuEvent *event)
{
	if (m_magicLens && m_magicLens->isEnabled())
	{
		m_contextMenuMagicLens->exec(event->globalPos());
	}
	else if (m_decorations && m_interactionMode == SnakeEdit)
	{
		m_contextMenuSnakeSlicer->exec(event->globalPos());
	}
}

void iASlicer::switchInteractionMode(int mode)
{
	if (!m_decorations)
	{
		return;
	}
	m_interactionMode = static_cast<InteractionMode>(mode);
	m_snakeSpline->SetVisibility(m_interactionMode == SnakeEdit);
	m_renWin->GetInteractor()->Render();
}

void iASlicer::addPoint(double xPos, double yPos, double zPos)
{
	if (!m_decorations)
	{
		return;
	}
	double pos[3] = { xPos, yPos, zPos };
	double x = pos[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)];
	double y = pos[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)];

	//add point to the snake slicer spline
	m_snakeSpline->addPoint(x, y);

	// render slice view
#if VTK_MAJOR_VERSION < 9
	GetRenderWindow()->GetInteractor()->Render();
#else
	renderWindow()->GetInteractor()->Render();
#endif
}

void iASlicer::updateRawProfile(double posY)
{
	if (!hasChannel(0))
	{
		return;
	}
	// TODO: slice "raw" profile on selected/current channel
	vtkImageData * reslicedImgData = channel(0)->output();
	if (!m_sliceProfile->updatePosition(posY, reslicedImgData))
	{
		return;
	}
	// render slice view
#if VTK_MAJOR_VERSION < 9
	GetRenderWindow()->GetInteractor()->Render();
#else
	renderWindow()->GetInteractor()->Render();
#endif
}

bool iASlicer::setArbitraryProfile(int pointInd, double * Pos, bool doClamp)
{
	if (!m_decorations || !hasChannel(0))
	{
		return false;
	}
	// TODO: slice profile on selected/current channel
	auto imageData = channel(0)->input();
	if (doClamp)
	{
		double * spacing = imageData->GetSpacing();
		double * origin = imageData->GetOrigin();
		int * dimensions = imageData->GetDimensions();
		for (int i = 0; i < 3; ++i)
		{
			Pos[i] = clamp(origin[i], origin[i] + (dimensions[i] - 1) * spacing[i], Pos[i]);
		}
	}
	double profileCoord2d[2] = { Pos[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)], Pos[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)] };
	if (!m_arbProfile->setup(pointInd, Pos, profileCoord2d, channel(0)->output()))
	{
		return false;
	}
#if VTK_MAJOR_VERSION < 9
	GetRenderWindow()->GetInteractor()->Render();
#else
	renderWindow()->GetInteractor()->Render();
#endif
	return true;
}

void iASlicer::movePoint(size_t selectedPointIndex, double xPos, double yPos, double zPos)
{
	if (!m_decorations)
	{
		return;
	}
	double pos[3] = { xPos, yPos, zPos };
	double x = pos[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)];
	double y = pos[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)];

	// move only if a point is selected
	if (selectedPointIndex != iASnakeSpline::NoPointSelected)
	{
		// move only if a point is selected
		m_snakeSpline->movePoint(selectedPointIndex, x, y);

		// render slice view
#if VTK_MAJOR_VERSION < 9
		GetRenderWindow()->GetInteractor()->Render();
#else
		renderWindow()->GetInteractor()->Render();
#endif
	}
}

void iASlicer::menuDeleteSnakeLine()
{
	deleteSnakeLine();
	emit deletedSnakeLine();
}

void iASlicer::deleteSnakeLine()
{
	if (!m_decorations)
	{
		return;
	}
	m_snakeSpline->deleteAllPoints();
	m_worldSnakePoints->Reset();
	m_renWin->GetInteractor()->Render();
}

void iASlicer::setSliceProfileOn(bool isOn)
{
	m_isSliceProfEnabled = isOn;
	m_sliceProfile->setVisibility(m_isSliceProfEnabled);
	updateRawProfile(channel(0)->output()->GetOrigin()[1]);
}

void iASlicer::setArbitraryProfileOn(bool isOn)
{
	if (!m_decorations)
	{
		return;
	}
	m_isArbProfEnabled = isOn;
	m_arbProfile->setVisibility(m_isArbProfEnabled);
#if VTK_MAJOR_VERSION < 9
	GetRenderWindow()->GetInteractor()->Render();
#else
	renderWindow()->GetInteractor()->Render();
#endif
}

void iASlicer::setIndex(int x, int y, int z)
{
	m_xInd = x; m_yInd = y; m_zInd = z;
}

void iASlicer::setLinkedMdiChild(MdiChild* mdiChild)
{
	m_linkedMdiChild = mdiChild;
}

void iASlicer::resizeEvent(QResizeEvent * event)
{
	updateMagicLens();
	iAVtkWidget::resizeEvent(event);
}

void iASlicer::wheelEvent(QWheelEvent* event)
{
	event->accept();
	if (event->modifiers().testFlag(Qt::ControlModifier) && event->modifiers().testFlag(Qt::ShiftModifier) && receivers(SIGNAL(altMouseWheel(int))) > 0)
	{
		this->setSliceNumber(event->angleDelta().y() / 120.0 + this->sliceNumber());
	}
	else if (event->modifiers().testFlag(Qt::ControlModifier) && receivers(SIGNAL(ctrlMouseWheel(int))) > 0)
	{
		emit ctrlMouseWheel(event->angleDelta().y() / 120.0);
	}
	else if (event->modifiers().testFlag(Qt::ShiftModifier) && receivers(SIGNAL(shiftMouseWheel(int))) > 0)
	{
		emit shiftMouseWheel(event->angleDelta().y() / 120);
	}
	else if (event->modifiers().testFlag(Qt::AltModifier) && receivers(SIGNAL(altMouseWheel(int))) > 0)
	{
		emit altMouseWheel(event->angleDelta().x() / 120);
	}
	else if (event->angleDelta().x() != 0 && receivers(SIGNAL(altMouseWheel(int))) > 0)
	{
		this->setSliceNumber(event->angleDelta().x() / 120.0 + this->sliceNumber());
	}
	else
	{
		iAVtkWidget::wheelEvent(event);
	}
	updateMagicLens();
}

void iASlicer::menuCenteredMagicLens()
{
	if (!m_magicLens)
	{
		return;
	}
	m_magicLens->setViewMode(iAMagicLens::CENTERED);
	updateMagicLens();
}

void iASlicer::menuOffsetMagicLens()
{
	if (!m_magicLens)
	{
		return;
	}
	m_magicLens->setViewMode(iAMagicLens::OFFSET);
	updateMagicLens();
}

void iASlicer::initializeFisheyeLens(vtkImageReslice* reslicer)
{
#if VTK_MAJOR_VERSION < 9
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();
#else
	vtkRenderer * ren = renderWindow()->GetRenderers()->GetFirstRenderer();
#endif

	m_fisheyeTransform = vtkSmartPointer<vtkThinPlateSplineTransform>::New();
	m_fisheyeTransform->SetBasisToR2LogR();
	m_pointsSource = vtkSmartPointer<vtkPoints>::New();
	m_pointsTarget = vtkSmartPointer<vtkPoints>::New();
	m_pointsSource->SetNumberOfPoints(32);
	m_pointsTarget->SetNumberOfPoints(32);
	reslicer->SetInterpolationModeToLinear(); // added while testing

	m_fisheye = vtkSmartPointer<vtkRegularPolygonSource>::New();
	m_fisheye->SetNumberOfSides(60);
	m_fisheye->GeneratePolygonOff(); // just outlines;
	m_fisheye->SetRadius(50.0);
	m_fisheyeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_fisheyeMapper->SetInputConnection(m_fisheye->GetOutputPort());
	m_fisheyeActor = vtkSmartPointer<vtkActor>::New();
	m_fisheyeActor->GetProperty()->SetColor(1.00000, 0.50196, 0.00000);
	m_fisheyeActor->GetProperty()->SetOpacity(1.0);
	m_fisheyeActor->SetMapper(m_fisheyeMapper);
	ren->AddActor(m_fisheyeActor);

	// Create circle actors (green and red) to show the transform landmarks
	for (int i = 0; i < m_pointsTarget->GetNumberOfPoints(); ++i)
	{
		// Create a sphere and its associated mapper and actor.
		vtkSmartPointer<vtkRegularPolygonSource> circle = vtkSmartPointer<vtkRegularPolygonSource>::New();
		circle->GeneratePolygonOff(); // Uncomment to generate only the outline of the circle
		circle->SetNumberOfSides(50);
		circle->SetRadius(1.0 * reslicer->GetOutput()->GetSpacing()[0]);

		vtkSmartPointer<vtkPolyDataMapper> circleMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		circleMapper->SetInputConnection(circle->GetOutputPort());

		vtkSmartPointer<vtkActor> circleActor = vtkSmartPointer<vtkActor>::New();
		circleActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
		circleActor->GetProperty()->SetOpacity(1.0);
		circleActor->SetMapper(circleMapper);
		circleActor->VisibilityOff(); // comment to show landmarks

		m_circle1List.append(circle);
		m_circle1ActList.append(circleActor);
		ren->AddActor(circleActor);
	}

	for (int i = 0; i < m_pointsSource->GetNumberOfPoints(); ++i)
	{
		vtkSmartPointer<vtkRegularPolygonSource> circle = vtkSmartPointer<vtkRegularPolygonSource>::New();
		circle->GeneratePolygonOff(); // Uncomment to generate only the outline of the circle
		circle->SetNumberOfSides(50);
		circle->SetRadius(3.0 * reslicer->GetOutput()->GetSpacing()[0]);

		vtkSmartPointer<vtkPolyDataMapper> circleMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		circleMapper->SetInputConnection(circle->GetOutputPort());

		vtkSmartPointer<vtkActor> circleActor = vtkSmartPointer<vtkActor>::New();
		circleActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
		circleActor->GetProperty()->SetOpacity(1.0);
		circleActor->SetMapper(circleMapper);
		circleActor->VisibilityOff(); // comment to show landmarks

		m_circle2List.append(circle);
		m_circle2ActList.append(circleActor);
		ren->AddActor(circleActor);
	}
}

void iASlicer::updateFisheyeTransform(double focalPt[3], vtkImageReslice* reslicer, double lensRadius, double innerLensRadius)
{
	vtkImageData * reslicedImgData = reslicer->GetOutput();

	double * spacing = reslicedImgData->GetSpacing();

	double bounds[6];
	reslicer->GetInformationInput()->GetBounds(bounds);

	m_pointsTarget->SetNumberOfPoints(32); // already set above!
	m_pointsSource->SetNumberOfPoints(32);
	int sn = sliceNumber();

	switch (m_mode)
	{
	case iASlicerMode::YZ:
		m_pointsTarget->SetPoint(0, sn * spacing[0], bounds[2], bounds[2]); //x_min, y_min, bottom left // left border points
		m_pointsTarget->SetPoint(1, sn * spacing[0], bounds[2], 0.5  * bounds[5]);
		m_pointsTarget->SetPoint(2, sn * spacing[0], bounds[2], bounds[5]); //x_min, y_max, top left // top border points
		m_pointsTarget->SetPoint(3, sn * spacing[0], 0.5  * bounds[3], bounds[5]);
		m_pointsTarget->SetPoint(4, sn * spacing[0], bounds[3], bounds[5]); //x_max, y_max, top right // right border points
		m_pointsTarget->SetPoint(5, sn * spacing[0], bounds[3], 0.5  * bounds[5]);
		m_pointsTarget->SetPoint(6, sn * spacing[0], bounds[3], bounds[2]); //x_max, y_min, bottom right // bottom border points
		m_pointsTarget->SetPoint(7, sn * spacing[0], 0.5  * bounds[3], bounds[2]);
		break;
	case iASlicerMode::XZ:
		m_pointsTarget->SetPoint(0, bounds[0], sn * spacing[1], bounds[4]); //x_min, y_min, bottom left // left border points
		m_pointsTarget->SetPoint(1, bounds[0], sn * spacing[1], 0.5  * bounds[5]);
		m_pointsTarget->SetPoint(2, bounds[0], sn * spacing[1], bounds[5]); //x_min, y_max, top left // top border points
		m_pointsTarget->SetPoint(3, 0.5  * bounds[1], sn * spacing[1], bounds[5]);
		m_pointsTarget->SetPoint(4, bounds[1], sn * spacing[1], bounds[5]); //x_max, y_max, top right // right border points
		m_pointsTarget->SetPoint(5, bounds[1], sn * spacing[1], 0.5  * bounds[5]);
		m_pointsTarget->SetPoint(6, bounds[1], sn * spacing[1], bounds[4]); //x_max, y_min, bottom right // bottom border points
		m_pointsTarget->SetPoint(7, 0.5  * bounds[1], sn * spacing[1], bounds[4]);
		break;
	case iASlicerMode::XY:
		m_pointsTarget->SetPoint(0, bounds[0], bounds[2], sn * spacing[2]); //x_min, y_min, bottom left // left border points
		m_pointsTarget->SetPoint(1, bounds[0], 0.5  * bounds[3], sn * spacing[2]);
		m_pointsTarget->SetPoint(2, bounds[0], bounds[3], sn * spacing[2]); //x_min, y_max, top left // top border points
		m_pointsTarget->SetPoint(3, 0.5  * bounds[1], bounds[3], sn * spacing[2]);
		m_pointsTarget->SetPoint(4, bounds[1], bounds[3], sn * spacing[2]); //x_max, y_max, top right // right border points
		m_pointsTarget->SetPoint(5, bounds[1], 0.5  * bounds[3], sn * spacing[2]);
		m_pointsTarget->SetPoint(6, bounds[1], bounds[2], sn * spacing[2]); //x_max, y_min, bottom right // bottom border points
		m_pointsTarget->SetPoint(7, 0.5  * bounds[1], bounds[2], sn * spacing[2]);
		break;
	default:
		break;
	}

	for (int i = 0; i < m_pointsTarget->GetNumberOfPoints() - (m_pointsTarget->GetNumberOfPoints() - 8); ++i)
	{
		m_pointsSource->SetPoint(i, m_pointsTarget->GetPoint(i));
	}
	int fixPoints = 8;
	// outer circle 1
	double fixRadiusX;
	double fixRadiusY;
	for (int fix = m_pointsTarget->GetNumberOfPoints() - 8 - 8 - 8; fix < m_pointsTarget->GetNumberOfPoints() - 8 - 8; fix++)
	{
		fixRadiusX = (lensRadius + 15.0)* std::cos(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];
		fixRadiusY = (lensRadius + 15.0)* std::sin(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];

		switch (m_mode)
		{
		case iASlicerMode::YZ:
			m_pointsTarget->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			m_pointsSource->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XZ:
			m_pointsTarget->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			m_pointsSource->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XY:
			m_pointsTarget->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			m_pointsSource->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			break;
		default:
			break;
		}
	}
	// outer circle 2
	fixPoints = 8;
	for (int fix = m_pointsTarget->GetNumberOfPoints() - 8 - 8; fix < m_pointsTarget->GetNumberOfPoints() - 8; fix++)
	{
		fixRadiusX = (lensRadius + 80.0)* std::cos(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];
		fixRadiusY = (lensRadius + 80.0)* std::sin(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];

		switch (m_mode)
		{
		case iASlicerMode::YZ:
			m_pointsTarget->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			m_pointsSource->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XZ:
			m_pointsTarget->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			m_pointsSource->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XY:
			m_pointsTarget->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			m_pointsSource->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			break;
		default:
			break;
		}
	}

	int pointsCount = 8;
	for (int i = m_pointsTarget->GetNumberOfPoints() - pointsCount; i < m_pointsTarget->GetNumberOfPoints(); ++i)
	{
		double xCoordCircle1 = (innerLensRadius)* std::cos(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];
		double yCoordCircle1 = (innerLensRadius)* std::sin(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];

		double xCoordCircle2 = (lensRadius)* std::cos(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];
		double yCoordCircle2 = (lensRadius)* std::sin(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];

		switch (m_mode)
		{
		case iASlicerMode::YZ:
			m_pointsTarget->SetPoint(i, sn * spacing[0], focalPt[0] + xCoordCircle1, focalPt[1] + yCoordCircle1);
			m_pointsSource->SetPoint(i, sn * spacing[0], focalPt[0] + xCoordCircle2, focalPt[1] + yCoordCircle2);
			break;
		case iASlicerMode::XZ:
			m_pointsTarget->SetPoint(i, focalPt[0] + xCoordCircle1, sn * spacing[1], focalPt[1] + yCoordCircle1);
			m_pointsSource->SetPoint(i, focalPt[0] + xCoordCircle2, sn * spacing[1], focalPt[1] + yCoordCircle2);
			break;
		case iASlicerMode::XY:
			m_pointsTarget->SetPoint(i, focalPt[0] + xCoordCircle1, focalPt[1] + yCoordCircle1, sn * spacing[2]);
			m_pointsSource->SetPoint(i, focalPt[0] + xCoordCircle2, focalPt[1] + yCoordCircle2, sn * spacing[2]);
			break;
		default:
			break;
		}
	}

	// Set position and text for green circle1 actors
	for (int i = 0; i < m_pointsTarget->GetNumberOfPoints(); ++i)
	{
		if (m_mode == iASlicerMode::YZ)
		{
			m_circle1List.at(i)->SetCenter(m_pointsTarget->GetPoint(i)[1], m_pointsTarget->GetPoint(i)[2], 0.0);
			m_circle2List.at(i)->SetCenter(m_pointsSource->GetPoint(i)[1], m_pointsSource->GetPoint(i)[2], 0.0);
		}
		if (m_mode == iASlicerMode::XZ)
		{
			m_circle1List.at(i)->SetCenter(m_pointsTarget->GetPoint(i)[0], m_pointsTarget->GetPoint(i)[2], 0.0);
			m_circle2List.at(i)->SetCenter(m_pointsSource->GetPoint(i)[0], m_pointsSource->GetPoint(i)[2], 0.0);
		}
		if (m_mode == iASlicerMode::XY)
		{
			m_circle1List.at(i)->SetCenter(m_pointsTarget->GetPoint(i)[0], m_pointsTarget->GetPoint(i)[1], 0.0);
			m_circle2List.at(i)->SetCenter(m_pointsSource->GetPoint(i)[0], m_pointsSource->GetPoint(i)[1], 0.0);
		}
	}

	m_fisheye->SetCenter(focalPt[0], focalPt[1], 0.0);
	m_fisheye->SetRadius(lensRadius * reslicer->GetOutput()->GetSpacing()[0]);

	m_fisheyeTransform->SetSourceLandmarks(m_pointsSource); // red
	m_fisheyeTransform->SetTargetLandmarks(m_pointsTarget);  // green

	reslicer->SetResliceTransform(m_fisheyeTransform);
	reslicer->Update();
	update();
}

void iASlicer::updateMagicLens()
{
	if (!m_magicLens || !m_magicLens->isEnabled())
	{
		return;
	}
	vtkRenderer * ren = m_renWin->GetRenderers()->GetFirstRenderer();
	ren->SetWorldPoint(m_slicerPt[0], m_slicerPt[1], 0, 1);
	ren->WorldToDisplay();
	double dpos[3];
	ren->GetDisplayPoint(dpos);
	int lensSz = m_magicLens->size();
	lensSz = (std::min)(lensSz, (std::min)(geometry().width(), geometry().height())); // restrict size to size of smallest side
	int lensSzHalf = 0.5*lensSz;
	// clamp to image, round to int (=pixels)
	dpos[0] = clamp(lensSzHalf, geometry().width() - lensSzHalf - 1, qRound(dpos[0]));
	dpos[1] = clamp(lensSzHalf, geometry().height() - lensSzHalf - 1, qRound(dpos[1]));
	dpos[2] = qRound(dpos[2]);
	ren->SetDisplayPoint(dpos);
	ren->DisplayToWorld();
	int const mousePos[2] = { static_cast<int>(dpos[0]), static_cast<int>(dpos[1]) };
	double const * worldP = ren->GetWorldPoint();
	m_magicLens->updatePosition(ren->GetActiveCamera(), worldP, mousePos);
}


// Declaration of following functions in iASlicerMode.h:

QString axisName(int axis)
{
	switch (axis)
	{
	case iAAxisIndex::X: return "X";
	case iAAxisIndex::Y: return "Y";
	case iAAxisIndex::Z: return "Z";
	default: return "?";
	}
}

QString slicerModeString(int mode)
{
	return axisName(mapSliceToGlobalAxis(mode, iAAxisIndex::X)) + axisName(mapSliceToGlobalAxis(mode, iAAxisIndex::Y));
}

namespace
{
	static const int SliceToGlobalAxisMapping[3][3] =
	{
		//            YZ              XZ              XY
		{ iAAxisIndex::Y, iAAxisIndex::X, iAAxisIndex::X },  // x
		{ iAAxisIndex::Z, iAAxisIndex::Z, iAAxisIndex::Y },  // y
		{ iAAxisIndex::X, iAAxisIndex::Y, iAAxisIndex::Z }   // z
	};
	/*
	static const int GlobalToSliceAxisMapping[3][3] =
	{   //            YZ              XZ              XY
		{ iAAxisIndex::Z, iAAxisIndex::X, iAAxisIndex::X }, // x
		{ iAAxisIndex::X, iAAxisIndex::Z, iAAxisIndex::Y }, // y
		{ iAAxisIndex::Y, iAAxisIndex::Y, iAAxisIndex::Z }  // z
	};
	*/
}

int mapSliceToGlobalAxis(int mode, int index)
{
	assert(0 <= mode  && mode  < iASlicerMode::SlicerCount);
	assert(0 <= index && index < iAAxisIndex::AxisCount);
	return SliceToGlobalAxisMapping[index][mode];
}
/*
int mapGlobalToSliceAxis(int mode, int index)
{
	return GlobalToSliceAxisMapping[index][mode];
}
*/
