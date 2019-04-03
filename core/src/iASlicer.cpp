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
#include "iAPieChartGlyph.h"
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
//#include <vtkAlgorithmOutput.h>
#include <vtkAxisActor2D.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkDataSetMapper.h>
#include <vtkDiskSource.h>
#include <vtkGenericMovieWriter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
//#include <vtkImageBlend.h>
#include <vtkImageCast.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageResample.h>
#include <vtkImageReslice.h>
#include <vtkInteractorStyleImage.h>
//#include <vtkInteractorStyleTrackballActor.h>
#include <vtkLineSource.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
#include <vtkMarchingContourFilter.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkPlaneSource.h>
#include <vtkPointPicker.h>
#include <vtkPoints.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkQImageToImageSource.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
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

#include <QBitmap>
#include <QFileDialog>
#include <QIcon>
#include <QKeyEvent>
#include <qmath.h>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QString>

#include <cassert>


namespace
{
	const double PickTolerance = 100.0;
}

class iAInteractorStyleImage : public vtkInteractorStyleImage
{
public:
	static iAInteractorStyleImage *New();
	vtkTypeMacro(iAInteractorStyleImage, vtkInteractorStyleImage);

	//void OnMouseMove() override{
	//	return; GetInteractor()-> 
	//}

	//! Disable "window-level" and rotation interaction (anything but shift-dragging)
	void OnLeftButtonDown() override
	{
		if (!this->Interactor->GetShiftKey())
			return;
		vtkInteractorStyleImage::OnLeftButtonDown();
	}
	//! @{ shift and control + mousewheel are used differently - don't use them for zooming!
	void OnMouseWheelForward() override
	{
		if (this->Interactor->GetControlKey() || this->Interactor->GetShiftKey())
			return;
		vtkInteractorStyleImage::OnMouseWheelForward();
	}
	void OnMouseWheelBackward() override
	{
		if (this->Interactor->GetControlKey() || this->Interactor->GetShiftKey())
			return;
		vtkInteractorStyleImage::OnMouseWheelBackward();
	}
	//! @}
	//! @{ Conditionally disable zooming via right button dragging
	void OnRightButtonDown() override
	{
		if (!m_rightButtonDragZoomEnabled)
			return;
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
	bool m_rotionEnabled = false;
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
	m_decorations(decorations),
	m_isSliceProfEnabled(false),
	m_isArbProfEnabled(false),
	m_pieGlyphsEnabled(false),
	fisheyeLensActivated(false),
	m_roiActive(false),
	m_showPositionMarker(false),
	m_userSetBackground(false),
	m_cameraOwner(true),
	m_magicLensContextMenu(nullptr),
	m_interactor(nullptr),
	m_renWin(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
	m_ren(vtkSmartPointer<vtkRenderer>::New()),
	m_interactorStyle(iAInteractorStyleImage::New()),
	m_camera(vtkCamera::New()),
	m_pointPicker(vtkSmartPointer<vtkPointPicker>::New()),
	m_transform(transform ? transform : vtkTransform::New()),
	m_magicLensInput(NotExistingChannel),
	m_mode(mode),
	m_interactionMode(NORMAL), // standard m_viewMode
	m_xInd(0), m_yInd(0), m_zInd(0),
	m_sliceNumber(0)
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
	SetRenderWindow(m_renWin);
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
		m_magicLens->SetRenderWindow(m_renWin);
		// setup context menu for the magic lens view options
		m_magicLensContextMenu = new QMenu(this);
		QActionGroup * actionGr(new QActionGroup(this));
		auto centeredLens = m_magicLensContextMenu->addAction(tr("Centered Magic Lens"), this, SLOT(menuCenteredMagicLens()));
		centeredLens->setCheckable(true);
		centeredLens->setChecked(true);
		actionGr->addAction(centeredLens);
		auto offsetLens = m_magicLensContextMenu->addAction(tr("Offseted Magic Lens"), this, SLOT(menuOffsetMagicLens()));
		offsetLens->setCheckable(true);
		actionGr->addAction(offsetLens);
	}
	connect(this, SIGNAL(updateSignal()), this, SLOT(slicerUpdatedSlot()));

	if (decorations)
	{
		m_snakeSpline = new iASnakeSpline;

		m_contextMenu = new QMenu(this);
		m_contextMenu->addAction(QIcon(":/images/loadtrf.png"), tr("Delete Snake Line"), this, SLOT(menuDeleteSnakeLine()));
		m_sliceProfile = new iASlicerProfile();
		m_sliceProfile->SetVisibility(false);

		m_arbProfile = new iAArbitraryProfileOnSlicer();
		m_arbProfile->SetVisibility(false);

		m_scalarBarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();
		m_textProperty = vtkSmartPointer<vtkTextProperty>::New();
		m_logoWidget = vtkSmartPointer<vtkLogoWidget>::New();
		m_logoRep = vtkSmartPointer<vtkLogoRepresentation>::New();
		m_logoImage = vtkSmartPointer<vtkQImageToImageSource>::New();

		m_positionMarkerSrc = vtkSmartPointer<vtkPlaneSource>::New();
		m_positionMarkerMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_positionMarkerActor = vtkSmartPointer<vtkActor>::New();

		pLineSource = vtkSmartPointer<vtkLineSource>::New();
		pLineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		pLineActor = vtkSmartPointer<vtkActor>::New();
		pDiskSource = vtkSmartPointer<vtkDiskSource>::New();
		pDiskMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		pDiskActor = vtkSmartPointer<vtkActor>::New();

		m_roiSource = vtkSmartPointer<vtkPlaneSource>::New();
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

		m_roiSource->SetOrigin(0, 0, 0);
		m_roiSource->SetPoint1(-3, 0, 0);
		m_roiSource->SetPoint2(0, -3, 0);

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

		pLineSource->SetPoint1(0.0, 0.0, 0.0);
		pLineSource->SetPoint2(10.0, 10.0, 0.0);
		pLineSource->Update();
		pLineMapper->SetInputConnection(pLineSource->GetOutputPort());
		pLineActor->SetMapper(pLineMapper);
		pLineActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
		pLineActor->GetProperty()->SetOpacity(1);
		pLineActor->SetVisibility(false);

		pDiskSource->SetCircumferentialResolution(50);
		pDiskSource->Update();
		pDiskMapper->SetInputConnection(pDiskSource->GetOutputPort());
		pDiskActor->SetMapper(pDiskMapper);
		pDiskActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
		pDiskActor->GetProperty()->SetOpacity(1);
		pDiskActor->SetVisibility(false);
		pDiskActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);

		m_roiMapper->SetInputConnection(m_roiSource->GetOutputPort());
		m_roiActor->SetVisibility(false);
		m_roiActor->SetMapper(m_roiMapper);
		m_roiActor->GetProperty()->SetColor(1, 0, 0);
		m_roiActor->GetProperty()->SetOpacity(1);
		m_roiSource->SetCenter(0, 0, 1);
		m_roiMapper->Update();
		m_roiActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);

		m_axisTextActor[0]->SetInput((m_mode == XY || m_mode == XZ) ? "X" : "Y");
		m_axisTextActor[1]->SetInput((m_mode == XZ || m_mode == YZ) ? "Z" : "Y");

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
		m_ren->AddActor(pLineActor);
		m_ren->AddActor(pDiskActor);
		m_ren->AddActor(m_roiActor);
	}

	m_worldSnakePoints = snakeSlicerPoints;
	m_renWin->SetNumberOfLayers(3);
	m_camera->SetParallelProjection(true);

	if (m_decorations)
	{
		// TODO: fix snake spline with non-fixed slicer images
		//m_snakeSpline->initialize(ren, imageData->GetSpacing()[0]);
		m_sliceProfile->initialize(m_ren);
		m_arbProfile->initialize(m_ren);
	}
	m_ren->ResetCamera();
}

iASlicer::~iASlicer()
{
	disconnect();

	m_interactorStyle->Delete();

	if (m_cameraOwner)
		m_camera->Delete();

	if (m_decorations)
	{
		delete m_snakeSpline;
		delete m_contextMenu;
		delete m_sliceProfile;
		delete m_arbProfile;
	}
}

void iASlicer::toggleInteractorState()
{
	if (m_interactor->GetEnabled())
	{
		disableInteractor();
		emit msg(tr("Slicer %1 disabled.").arg(getSlicerModeString(m_mode)));
	}
	else
	{
		enableInteractor();
		emit msg(tr("Slicer %1 enabled.").arg(getSlicerModeString(m_mode)));
	}
}

void iASlicer::setMode( const iASlicerMode mode )
{
	m_mode = mode;
	updateResliceAxesDirectionCosines();
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

/*
void iASlicer::setResliceChannelAxesOrigin(uint id, double x, double y, double z )
{
	m_data->setResliceChannelAxesOrigin(id, x, y, z);
	if (m_magicLens)
		m_magicLens->UpdateColors();
}
*/

void iASlicer::update()
{
	if (!isVisible())
		return;
	for (auto ch : m_channels)
		ch->updateMapper();
	updateReslicer();
	for (auto ch : m_channels)
		ch->reslicer->UpdateWholeExtent();
	m_interactor->ReInitialize();
	m_interactor->Render();
	m_ren->Render();
	emit updateSignal();

	iAVtkWidget::update();
	if (m_magicLens)
		m_magicLens->Render();
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
	// TODO: set slice position (in scene coordinates) instead of number?
	if (!hasChannel(0))
		return;
	m_sliceNumber = sliceNumber;
	double xyz[3] = { 0.0, 0.0, 0.0 };
	xyz[getSlicerDimension(m_mode)] = sliceNumber;
	if (m_roiActive)
		m_roiActor->SetVisibility(m_roiSlice[0] <= m_sliceNumber && m_sliceNumber < (m_roiSlice[1]));
	double * spacing = m_channels[0]->image->GetSpacing();
	double * origin = m_channels[0]->image->GetOrigin();
	for (auto ch : m_channels)
		ch->setResliceAxesOrigin(origin[0] + xyz[0] * spacing[0], origin[1] + xyz[1] * spacing[1], origin[2] + xyz[2] * spacing[2]);
	updateMagicLensColors();
	computeGlyphs();
	update();
	emit sliceNumberChanged( m_mode, sliceNumber );
}

void iASlicer::setup( iASingleSlicerSettings const & settings )
{
	m_settings = settings;
	for (auto channel: m_channels)
	{
		channel->imageActor->SetInterpolate(settings.LinearInterpolation);
	}
	if (m_magicLens)
		m_magicLens->SetInterpolate(settings.LinearInterpolation);
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
		updateMagicLens();
	m_renWin->Render();
}

void iASlicer::setMagicLensEnabled( bool isEnabled )
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensEnabled called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->SetEnabled(isEnabled);
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
	m_magicLens->SetSize(newSize);
	updateMagicLens();
}

int iASlicer::getMagicLensSize() const
{
	return m_magicLens ? m_magicLens->GetSize() : 0;
}

void iASlicer::setMagicLensFrameWidth(int newWidth)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensFrameWidth called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->SetFrameWidth(newWidth);
	updateMagicLens();
}

void iASlicer::setMagicLensCount(int count)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensCount called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->SetLensCount(count);
	updateMagicLens();
}

void iASlicer::setMagicLensInput(uint id)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensInput called on slicer which doesn't have a magic lens!");
		return;
	}
	iAChannelSlicerData * data = getChannel(id);
	assert(data);
	if (!data)
		return;
	m_magicLensInput = id;
	m_magicLens->AddInput(data->reslicer, data->getColorTransferFunction(), data->getName());
	update();
}

uint iASlicer::getMagicLensInput() const
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
	m_magicLens->SetOpacity(opacity);
	update();
}

double iASlicer::getMagicLensOpacity() const
{
	return (m_magicLens) ? m_magicLens->GetOpacity() : 0;
}

vtkGenericOpenGLRenderWindow * iASlicer::getRenderWindow()
{
	return m_renWin;
}

vtkRenderer * iASlicer::getRenderer()
{
	return m_ren;
}

vtkCamera * iASlicer::getCamera()
{
	return m_camera;
}

vtkRenderWindowInteractor * iASlicer::getInteractor()
{
	return m_interactor;
}

void iASlicer::addChannel(uint id, iAChannelData const & chData, bool enable)
{
	assert(!m_channels.contains(id));
	bool updateSpacing = m_channels.empty();
	auto chSlicerData = createChannel(id);
	chSlicerData->init(chData, m_mode);
	double curTol = m_pointPicker->GetTolerance();
	int axis = getSlicerDimension(m_mode);
	auto image = chData.getImage();
	double const * const imgSpc = image->GetSpacing();
	double newTol = imgSpc[axis] / 3;
	if (newTol < curTol)
		m_pointPicker->SetTolerance(newTol);
	if (updateSpacing)
	{
		setScalarBarTF(chData.getCTF());
		updatePositionMarkerExtent();
		// TODO: update required for new channels other than to export? export all channels?
		auto reslicer = m_channels[id]->reslicer;
		int const * const imgExt = image->GetExtent();
		double unitSpacing = std::max(std::max(imgSpc[0], imgSpc[1]), imgSpc[2]);
		double const * const spc = reslicer->GetOutput()->GetSpacing();
		int    const * const dim = reslicer->GetOutput()->GetDimensions();
		for (int i = 0; i < 2; ++i)
			// scaling required to shrink the text to required size (because of large font size, see initialize method)
			m_axisTransform[i]->Scale(unitSpacing / 10, unitSpacing / 10, unitSpacing / 10);
		double xHalf = (dim[0] - 1) * spc[0] / 2.0;
		double yHalf = (dim[1] - 1) * spc[1] / 2.0;
		// "* 10 / unitSpacing" adjusts for scaling (see above)
		m_axisTextActor[0]->SetPosition(xHalf * 10 / unitSpacing, -20.0, 0);
		m_axisTextActor[1]->SetPosition(-20.0, yHalf * 10 / unitSpacing, 0);
		emit firstChannelAdded(imgExt[axis*2], imgExt[axis*2+1]);
	}
	double origin[3];
	getChannel(id)->image->GetOrigin(origin);
	origin[axis] += static_cast<double>(sliceNumber()) * imgSpc[axis];
	setResliceChannelAxesOrigin(id, origin[0], origin[1], origin[2]);
	if (enable)
		enableChannel(id, true);
}

void iASlicer::updateMagicLensColors()
{
	if (m_magicLens)
		m_magicLens->UpdateColors();
}

void iASlicer::setTransform(vtkAbstractTransform * tr)
{
	m_transform = tr;
	for (auto ch : m_channels)
		ch->setTransform(m_transform);
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

/*
void iASlicer::blend(vtkAlgorithmOutput *data, vtkAlgorithmOutput *data2,
	double opacity, double * range)
{
	vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
	lut->SetRange( range );
	lut->SetHueRange(0, 1);
	lut->SetSaturationRange(0, 1);
	lut->SetValueRange( 0, 1 );
	lut->Build();

	vtkSmartPointer<vtkImageBlend> imgBlender = vtkSmartPointer<vtkImageBlend>::New();
	imgBlender->SetOpacity( 0, opacity );
	imgBlender->SetOpacity( 1, 1.0-opacity );
	imgBlender->AddInputConnection(data);
	imgBlender->AddInputConnection(data2);
	imgBlender->UpdateInformation();
	imgBlender->Update();

	reslicer->SetInputConnection(imgBlender->GetOutputPort());
	pointPicker->SetTolerance(PickTolerance);

	this->imageData = imgBlender->GetOutput();
	this->imageData->Modified();

	imgBlender->ReleaseDataFlagOn();

	InitReslicerWithImageData();
	update();
}
*/

void iASlicer::setROIVisible(bool visible)
{
	if (!m_decorations)
		return;
	m_roiActive = visible;
	m_roiActor->SetVisibility(visible);
}

void iASlicer::updateROI(int const roi[6])
{
	// TODO: ROI coordinates as scene coordinates?
	if (!m_decorations || !m_roiActive || !hasChannel(0))
		return;
	double* spacing = m_channels[0]->reslicer->GetOutput()->GetSpacing();

	// apparently, image actor starts output at -0,5spacing, -0.5spacing (probably a side effect of BorderOn)
	// That's why we have to subtract 0.5 from the coordinates!
	if (m_mode == iASlicerMode::YZ)
	{
		m_roiSlice[0] = roi[0];
		m_roiSlice[1] = roi[0] + roi[3];
		m_roiSource->SetOrigin((roi[1] - 0.5)*spacing[1], (roi[2] - 0.5)*spacing[2], 0);
		m_roiSource->SetPoint1((roi[1] - 0.5)*spacing[1] + roi[4] * spacing[0], (roi[2] - 0.5)*spacing[2], 0);
		m_roiSource->SetPoint2((roi[1] - 0.5)*spacing[1], (roi[2] - 0.5)*spacing[2] + roi[5] * spacing[2], 0);
	}
	else if (m_mode == iASlicerMode::XY)
	{
		m_roiSlice[0] = roi[2];
		m_roiSlice[1] = roi[2] + roi[5];
		m_roiSource->SetOrigin((roi[0] - 0.5)*spacing[0], (roi[1] - 0.5)*spacing[1], 0);
		m_roiSource->SetPoint1((roi[0] - 0.5)*spacing[0] + roi[3] * spacing[0], (roi[1] - 0.5)*spacing[1], 0);
		m_roiSource->SetPoint2((roi[0] - 0.5)*spacing[0], (roi[1] - 0.5)*spacing[1] + roi[4] * spacing[1], 0);
	}
	else if (m_mode == iASlicerMode::XZ)
	{
		m_roiSlice[0] = roi[1];
		m_roiSlice[1] = roi[1] + roi[4];
		m_roiSource->SetOrigin((roi[0] - 0.5)*spacing[0], (roi[2] - 0.5)*spacing[2], 0);
		m_roiSource->SetPoint1((roi[0] - 0.5)*spacing[0] + roi[3] * spacing[0], (roi[2] - 0.5)*spacing[2], 0);
		m_roiSource->SetPoint2((roi[0] - 0.5)*spacing[0], (roi[2] - 0.5)*spacing[2] + roi[5] * spacing[2], 0);
	}
	m_roiActor->SetVisibility(m_roiSlice[0] <= m_sliceNumber && m_sliceNumber < m_roiSlice[1]);
	m_roiMapper->Update();
	m_interactor->Render();
}

void iASlicer::setResliceAxesOrigin(double x, double y, double z)
{
	if (m_interactor->GetEnabled())
	{
		for (auto ch : m_channels)
			ch->reslicer->SetResliceAxesOrigin(x, y, z);
		updateReslicer();
		m_interactor->Render();
	}
}

void iASlicer::setPositionMarkerCenter(double x, double y)
{
	if (!m_decorations)
		return;

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
		return;
	for (auto ch : m_channels)
		ch->setShowContours(s);
}

void iASlicer::showPosition(bool s)
{
	if (!m_decorations)
		return;
	m_showPositionMarker = s;
}

void iASlicer::saveSliceMovie(QString const & fileName, int qual /*= 2*/)
{
	// TODO: select channel / for all channels?
	if (!hasChannel(0))
		return;
	QString movie_file_types = GetAvailableMovieFormats();
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(this, "Movie Export", "This version of open_iA was built without movie export support!");
		return;
	}
	vtkSmartPointer<vtkGenericMovieWriter> movieWriter = GetMovieWriter(fileName, qual);
	if (movieWriter.GetPointer() == nullptr)
		return;

	m_interactor->Disable();

	vtkSmartPointer<vtkWindowToImageFilter> w2if = vtkSmartPointer<vtkWindowToImageFilter>::New();
	int* rws = m_renWin->GetSize();
	if (rws[0] % 2 != 0) rws[0]++;
	if (rws[1] % 2 != 0) rws[1]++;
	m_renWin->SetSize(rws);
	m_renWin->Render();

	w2if->SetInput(m_renWin);
	w2if->ReadFrontBufferOff();

	movieWriter->SetInputConnection(w2if->GetOutputPort());
	movieWriter->Start();

	auto reslicer = m_channels[0]->reslicer;
	auto img0 = reslicer->GetOutput();
	int* extent = img0->GetExtent();
	double* origin = img0->GetOrigin();
	double* spacing = img0->GetSpacing();

	emit msg(tr("MOVIE export started. Output: %1").arg(fileName));

	double oldResliceAxesOrigin[3];
	reslicer->GetResliceAxesOrigin(oldResliceAxesOrigin);

	if (m_mode == iASlicerMode::YZ)      // YZ
	{
		for (int i = extent[0]; i < extent[1]; i++)
		{
			reslicer->SetResliceAxesOrigin(origin[0] + i * spacing[0], origin[1], origin[2]);
			update();
			w2if->Modified();
			movieWriter->Write();
			if (movieWriter->GetError()) {
				emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
				break;
			}
			emit progress(100 * (i + 1) / (extent[1] - extent[0]));
		}
	}
	else if (m_mode == iASlicerMode::XY)  // XY
	{
		for (int i = extent[4]; i < extent[5]; i++)
		{
			reslicer->SetResliceAxesOrigin(origin[0], origin[1], origin[2] + i * spacing[2]);
			update();
			w2if->Modified();
			movieWriter->Write();
			if (movieWriter->GetError()) {
				emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
				break;
			}
			emit progress(100 * (i + 1) / (extent[5] - extent[4]));
		}
	}
	else if (m_mode == iASlicerMode::XZ)  // XZ
	{
		for (int i = extent[2]; i < extent[3]; i++)
		{
			reslicer->SetResliceAxesOrigin(origin[0], origin[1] + i * spacing[1], origin[2]);
			update();
			w2if->Modified();
			movieWriter->Write();
			if (movieWriter->GetError()) {
				emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
				break;
			}
			emit progress(100 * (i + 1) / (extent[3] - extent[2]));
		}
	}

	reslicer->SetResliceAxesOrigin(oldResliceAxesOrigin);
	update();
	movieWriter->End();
	movieWriter->ReleaseDataFlagOn();
	w2if->ReleaseDataFlagOn();

	m_interactor->Enable();

	if (movieWriter->GetError())
		emit msg(tr("  MOVIE export failed."));
	else
		emit msg(tr("MOVIE export completed."));

	return;
}

void iASlicer::saveAsImage()
{
	// TODO: select channel / for all channels?
	if (!hasChannel(0))
		return;
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"),
		"", // TODO: get directory of file?
		iAIOProvider::GetSupportedImageFormats());
	if (fileName.isEmpty())
		return;
	bool saveNative = true;
	bool output16Bit = false;
	QStringList inList = (QStringList() << tr("$Save native image (intensity rescaled to output format)"));
	QList<QVariant> inPara = (QList<QVariant>() << (saveNative ? tr("true") : tr("false")));
	QFileInfo fi(fileName);
	if ((QString::compare(fi.suffix(), "TIF", Qt::CaseInsensitive) == 0) ||
		(QString::compare(fi.suffix(), "TIFF", Qt::CaseInsensitive) == 0))
	{
		inList << tr("$16 bit native output (if disabled, native output will be 8 bit)");
		inPara << (output16Bit ? tr("true") : tr("false"));
	}
	dlg_commoninput dlg(this, "Save options", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	saveNative = dlg.getCheckValue(0);
	if (inList.size() > 1)
	{
		output16Bit = dlg.getCheckValue(1);
	}
	iAConnector con;
	vtkSmartPointer<vtkImageData> img;

	// TODO: allow selecting channel to export? export all channels?
	auto reslicer = m_channels[0]->reslicer;
	if (saveNative)
	{
		con.SetImage(reslicer->GetOutput());
		iAITKIO::ImagePointer imgITK;
		if (!output16Bit)
		{
			imgITK = RescaleImageTo<unsigned char>(con.GetITKImage(), 0, 255);
		}
		else
		{
			imgITK = RescaleImageTo<unsigned short>(con.GetITKImage(), 0, 65535);
		}
		con.SetImage(imgITK);
		img = con.GetVTKImage();
	}
	else
	{
		vtkSmartPointer<vtkWindowToImageFilter> wtif = vtkSmartPointer<vtkWindowToImageFilter>::New();
		wtif->SetInput(m_renWin);
		wtif->Update();
		img = wtif->GetOutput();
	}
	WriteSingleSliceImage(fileName, img);
}

void iASlicer::saveImageStack()
{
	// TODO: allow selecting channel to export? export all channels?
	if (!hasChannel(0))
		return;
	auto imageData = m_channels[0]->image;

	QString file = QFileDialog::getSaveFileName(this, tr("Save Image Stack"),
		"", // TODO: get directory of file?
		iAIOProvider::GetSupportedImageFormats());
	if (file.isEmpty())
		return;

	QFileInfo fileInfo(file);
	QString baseName = fileInfo.absolutePath() + "/" + fileInfo.baseName();

	int const * arr = imageData->GetDimensions();
	double const * spacing = imageData->GetSpacing();

	//Determine index of number of slice in array
	int nums[3] = { 0, 2, 1 };
	int num = nums[m_mode];

	//Set slice range
	int sliceFirst = 0, sliceLast = arr[num] - 1;
	bool saveNative = true;
	bool output16Bit = false;
	QStringList inList = (QStringList() << tr("$Save native image (intensity rescaled to output format)")
		<< tr("#From Slice Number:")
		<< tr("#To Slice Number:"));
	QList<QVariant> inPara = (QList<QVariant>() << (saveNative ? tr("true") : tr("false")) << tr("%1")
		.arg(sliceFirst) << tr("%1").arg(sliceLast));

	if ((QString::compare(fileInfo.suffix(), "TIF", Qt::CaseInsensitive) == 0) ||
		(QString::compare(fileInfo.suffix(), "TIFF", Qt::CaseInsensitive) == 0))
	{
		inList << tr("$16 bit native output (if disabled, native output will be 8 bit)");
		inPara << (output16Bit ? tr("true") : tr("false"));
	}
	dlg_commoninput dlg(this, "Save options", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	saveNative = dlg.getCheckValue(0);
	sliceFirst = dlg.getDblValue(1);
	sliceLast = dlg.getDblValue(2);
	if (inList.size() > 3)
		output16Bit = dlg.getCheckValue(3);

	if (sliceFirst<0 || sliceFirst>sliceLast || sliceLast > arr[num])
	{
		QMessageBox msgBox;
		msgBox.setText("Invalid Input.");
		msgBox.exec();
		return;
	}
	m_interactor->Disable();
	vtkImageData* img;
	double* origin = imageData->GetOrigin();
	auto reslicer = m_channels[0]->reslicer;
	for (int slice = sliceFirst; slice <= sliceLast; slice++)
	{
		//Determine which axis
		if (m_mode == 0) //yz
			setResliceAxesOrigin(origin[0] + slice * spacing[0], origin[1], origin[2]);
		else if (m_mode == 1)  //xy
			setResliceAxesOrigin(origin[0], origin[1], origin[2] + slice * spacing[2]);
		else if (m_mode == 2)  //xz
			setResliceAxesOrigin(origin[0], origin[1] + slice * spacing[1], origin[2]);
		update();

		vtkSmartPointer<vtkWindowToImageFilter> wtif = vtkSmartPointer<vtkWindowToImageFilter>::New();
		iAConnector con;
		if (saveNative)
		{
			con.SetImage(reslicer->GetOutput());
			iAITKIO::ImagePointer imgITK;
			if (!output16Bit)
				imgITK = RescaleImageTo<unsigned char>(con.GetITKImage(), 0, 255);
			else
				imgITK = RescaleImageTo<unsigned short>(con.GetITKImage(), 0, 65535);
			con.SetImage(imgITK);
			img = con.GetVTKImage();
		}
		else
		{
			wtif->SetInput(m_renWin);
			wtif->ReadFrontBufferOff();
			wtif->Update();
			img = wtif->GetOutput();
		}

		emit progress(100 * slice / sliceLast);

		QString newFileName(QString("%1%2.%3").arg(baseName).arg(slice).arg(fileInfo.suffix()));
		WriteSingleSliceImage(newFileName, img);
	}
	m_interactor->Enable();
	emit msg(tr("Image stack saved in folder: %1")
		.arg(fileInfo.absoluteDir().absolutePath()));
}

void iASlicer::updatePositionMarkerExtent()
{
	// TODO: how to choose spacing? currently fixed from first image? export all channels?
	if (m_channels.empty())
		return;
	auto imageData = m_channels[0]->image;
	double spacing[2] = {
		imageData->GetSpacing()[SlicerXInd(m_mode)],
		imageData->GetSpacing()[SlicerYInd(m_mode)]
	};
	m_positionMarkerSrc->SetOrigin(0, 0, 0);
	m_positionMarkerSrc->SetPoint1((m_ext * spacing[0]), 0, 0);
	m_positionMarkerSrc->SetPoint2(0, (m_ext * spacing[1]), 0);
}

void iASlicer::setStatisticalExtent(int statExt)
{
	m_ext = statExt;
	if (m_positionMarkerSrc)
	{
		double center[3];
		m_positionMarkerSrc->GetCenter(center);
		updatePositionMarkerExtent();
		m_positionMarkerSrc->SetCenter(center);
	}
}

void iASlicer::updateResliceAxesDirectionCosines()
{
	for (auto ch : m_channels)
		ch->updateResliceAxesDirectionCosines(m_mode);
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
	case iASlicerMode::YZ:
		m_ren->SetBackground(0.2, 0.2, 0.2); break;
	case iASlicerMode::XY:
		m_ren->SetBackground(0.3, 0.3, 0.3); break;
	case iASlicerMode::XZ:
		m_ren->SetBackground(0.6, 0.6, 0.6); break;
	default:
		break;
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

void iASlicer::execute(vtkObject * caller, unsigned long eventId, void * callData)
{
	if (eventId == vtkCommand::LeftButtonPressEvent)
	{
		emit clicked();
	}
	if (eventId == vtkCommand::MouseWheelForwardEvent ||
		eventId == vtkCommand::MouseWheelBackwardEvent)
	{
		emit userInteraction();
	}
	// Do the pick. It will return a non-zero value if we intersected the image.
	int * epos = m_interactor->GetEventPosition();
	if (!m_pointPicker->Pick(epos[0], epos[1], 0, m_ren)) // z is always zero.
	{
		defaultOutput();
		return;
	}

	// Get the mapped position of the mouse using the picker.
	m_pointPicker->GetPickedPositions()->GetPoint(0, m_ptMapped);

	// TODO: how to choose spacing? currently fixed from first image!
	auto imageData = m_channels[0]->image;
	double* spacing = imageData->GetSpacing();
	m_ptMapped[0] += 0.5*spacing[0];
	m_ptMapped[1] += 0.5*spacing[1];
	m_ptMapped[2] += 0.5*spacing[2];

	switch (eventId)
	{
	case vtkCommand::LeftButtonPressEvent:
	{
		double result[4];
		double x, y, z;
		getMouseCoord(x, y, z, result);
		emit clicked(x, y, z);
		emit userInteraction();
		break;
	}
	case vtkCommand::LeftButtonReleaseEvent:
	{
		double result[4];
		double x, y, z;
		getMouseCoord(x, y, z, result);
		emit released(x, y, z);
		emit userInteraction();
		break;
	}
	case vtkCommand::RightButtonPressEvent:
	{
		double result[4];
		double x, y, z;
		getMouseCoord(x, y, z, result);
		emit rightClicked(x, y, z);
		break;
	}
	case vtkCommand::MouseMoveEvent:
	{
		double result[4];
		double xCoord, yCoord, zCoord;
		getMouseCoord(xCoord, yCoord, zCoord, result);
		double mouseCoord[3] = { result[0], result[1], result[2] };
		//updateFisheyeTransform(mouseCoord, reslicer, 50.0);
		if (m_decorations)
		{
			m_positionMarkerActor->SetVisibility(false);
			printVoxelInformation(xCoord, yCoord, zCoord);
		}
		emit oslicerPos(xCoord, yCoord, zCoord, m_mode);
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
			emit pick();
		break;
	default:
		if (m_interactor->GetKeyCode() == 'p')
			emit pick();
		break;
	}

	m_interactor->Render();
}

void iASlicer::getMouseCoord(double & xCoord, double & yCoord, double & zCoord, double* result)
{
	if (!hasChannel(0))
		return;
	result[0] = result[1] = result[2] = result[3] = 0;
	double point[4] = { m_ptMapped[0], m_ptMapped[1], m_ptMapped[2], 1 };

	// TODO: find out what "mouseCoord" exactly means - pixel coordinates or image coordinates? differentiate scene coordinates / each images pixel coordinates
	auto imageData = m_channels[0]->image;
	auto reslicer = m_channels[0]->reslicer;

	// get a shortcut to the pixel data.
	vtkMatrix4x4 *resliceAxes = vtkMatrix4x4::New();
	resliceAxes->DeepCopy(reslicer->GetResliceAxes());
	resliceAxes->MultiplyPoint(point, result);
	resliceAxes->Delete();

	double * imageSpacing = imageData->GetSpacing();	// +/- 0.5 to correct for BorderOn
	double * origin = imageData->GetOrigin();
	xCoord = ((result[0] - origin[0]) / imageSpacing[0]);	if (m_mode == YZ) xCoord -= 0.5;
	yCoord = ((result[1] - origin[1]) / imageSpacing[1]);	if (m_mode == XZ) yCoord += 0.5; // not sure yet why +0.5 required here...
	zCoord = ((result[2] - origin[2]) / imageSpacing[2]);	if (m_mode == XY) zCoord -= 0.5;

	// TODO: check for negative origin images!
	int* extent = imageData->GetExtent();
	xCoord = clamp(static_cast<double>(extent[0]), extent[1] + 1 - std::numeric_limits<double>::epsilon(), xCoord);
	yCoord = clamp(static_cast<double>(extent[2]), extent[3] + 1 - std::numeric_limits<double>::epsilon(), yCoord);
	zCoord = clamp(static_cast<double>(extent[4]), extent[5] + 1 - std::numeric_limits<double>::epsilon(), zCoord);
}

namespace
{
	const int MaxNameLength = 20;

	QString GetSlicerCoordString(int coord1, int coord2, int coord3, int mode)
	{
		switch (mode) {
		default:
		case XY: return QString("%1 %2 %3").arg(coord1).arg(coord2).arg(coord3);
		case XZ: return QString("%1 %2 %3").arg(coord1).arg(coord3).arg(coord2);
		case YZ: return QString("%1 %2 %3").arg(coord3).arg(coord1).arg(coord2);
		}
	}

	QString GetFilePixel(MdiChild* tmpChild, iASlicer* slicer, double slicerX, double slicerY, int thirdCoord, int mode)
	{
		// TODO: find out what "mouseCoord" exactly means - pixel coordinates or image coordinates? differentiate scene coordinates / each images pixel coordinates
		auto reslicer = slicer->getChannel(0)->reslicer;
		vtkImageData* img = reslicer->GetOutput();
		int const * dim = img->GetDimensions();
		bool inRange = slicerX < dim[0] && slicerY < dim[1];
		double tmpPix = 0;
		if (inRange)
		{
			tmpPix = img->GetScalarComponentAsDouble(slicerX, slicerY, 0, 0);
		}
		QString file = tmpChild->getFileInfo().fileName();
		return QString("%1 [%2]: %3\n")
			.arg(PadOrTruncate(file, MaxNameLength))
			.arg(GetSlicerCoordString(slicerX, slicerY, thirdCoord, mode))
			.arg(inRange ? QString::number(tmpPix) : "exceeds img. dim.");
	}
}

void iASlicer::printVoxelInformation(double xCoord, double yCoord, double zCoord)
{
	if (!m_decorations)
		return;

	// TODO: differentiate scene coordinates / each images pixel coordinates
	if (!hasChannel(0))
		return;
	auto reslicer = m_channels[0]->reslicer;
	vtkImageData * reslicerOutput = reslicer->GetOutput();
	double const * const slicerSpacing = reslicerOutput->GetSpacing();
	int const * const slicerExtent = reslicerOutput->GetExtent();
	double const * const slicerBounds = reslicerOutput->GetBounds();

	// We have to manually set the physical z-coordinate which requires us to get the volume spacing.
	m_ptMapped[2] = 0;

	int cX = static_cast<int>(floor((m_ptMapped[0] - slicerBounds[0]) / slicerSpacing[0]));
	int cY = static_cast<int>(floor((m_ptMapped[1] - slicerBounds[2]) / slicerSpacing[1]));

	// check image extent; if outside ==> default output
	if (cX < slicerExtent[0] || cX > slicerExtent[1] || cY < slicerExtent[2] || cY > slicerExtent[3])
	{
		defaultOutput(); return;
	}

	// get index, coords and value to display
	QString strDetails;
	for (auto channel: m_channels)
	{
		QString valueStr;
		for (int i = 0; i < channel->image->GetNumberOfScalarComponents(); i++)
		{
			// TODO:
			//   - consider different spacings in channels!
			//   - consider slab thickness / print slab projection result
			double value = -1.0;
			switch (m_mode)
			{
			case iASlicerMode::XY:
				value = reslicerOutput->GetScalarComponentAsDouble(
					static_cast<int>(xCoord), static_cast<int>(yCoord), 0, i);
				break;
			case iASlicerMode::YZ:
				value = reslicerOutput->GetScalarComponentAsDouble(
					static_cast<int>(yCoord), static_cast<int>(zCoord), 0, i);
				break;
			case iASlicerMode::XZ:
				value = reslicerOutput->GetScalarComponentAsDouble(
					static_cast<int>(xCoord), static_cast<int>(zCoord), 0, i);
				break;
			}
			if (i > 0)
				valueStr += " ";
			valueStr += QString::number(value);
		}
		strDetails += QString("%1 [%2 %3 %4]: %5\n")
			.arg(PadOrTruncate(channel->getName(), MaxNameLength))
			.arg(static_cast<int>(xCoord)).arg(static_cast<int>(yCoord)).arg(static_cast<int>(zCoord))
			.arg(valueStr);
	}
	if (m_linkedMdiChild)
	{
		QList<MdiChild*> mdiwindows = m_linkedMdiChild->getMainWnd()->mdiChildList();
		for (int i = 0; i < mdiwindows.size(); i++)
		{
			MdiChild *tmpChild = mdiwindows.at(i);
			if (m_linkedMdiChild == tmpChild)
				continue;
			double * const tmpSpacing = tmpChild->getImagePointer()->GetSpacing();
			// TODO: check which spacing makes sense here!
			auto imageData = m_channels[0]->image;
			double const * const origImgSpacing = imageData->GetSpacing();
			int tmpX = xCoord * origImgSpacing[0] / tmpSpacing[0];
			int tmpY = yCoord * origImgSpacing[1] / tmpSpacing[1];
			int tmpZ = zCoord * origImgSpacing[2] / tmpSpacing[2];
			switch (m_mode)
			{
			case iASlicerMode::XY://XY
				tmpChild->slicer(iASlicerMode::XY)->setPositionMarkerCenter(tmpX * tmpSpacing[0], tmpY * tmpSpacing[1]);
				tmpChild->slicer(iASlicerMode::XY)->setIndex(tmpX, tmpY, tmpZ);
				tmpChild->slicerDlg(iASlicerMode::XY)->sbSlice->setValue(tmpZ);

				tmpChild->slicer(iASlicerMode::XY)->update();
				tmpChild->slicer(iASlicerMode::XY)->update();
				tmpChild->slicerDlg(iASlicerMode::XY)->update();

				strDetails += GetFilePixel(tmpChild, tmpChild->slicer(iASlicerMode::XY), tmpX, tmpY, tmpZ, m_mode);
				break;
			case iASlicerMode::YZ://YZ
				tmpChild->slicer(iASlicerMode::YZ)->setPositionMarkerCenter(tmpY * tmpSpacing[1], tmpZ * tmpSpacing[2]);
				tmpChild->slicer(iASlicerMode::YZ)->setIndex(tmpX, tmpY, tmpZ);
				tmpChild->slicerDlg(iASlicerMode::YZ)->sbSlice->setValue(tmpX);

				tmpChild->slicer(iASlicerMode::YZ)->update();
				tmpChild->slicer(iASlicerMode::YZ)->update();
				tmpChild->slicerDlg(iASlicerMode::YZ)->update();

				strDetails += GetFilePixel(tmpChild, tmpChild->slicer(iASlicerMode::YZ), tmpY, tmpZ, tmpX, m_mode);
				break;
			case iASlicerMode::XZ://XZ
				tmpChild->slicer(iASlicerMode::XZ)->setPositionMarkerCenter(tmpX * tmpSpacing[0], tmpZ * tmpSpacing[2]);
				tmpChild->slicer(iASlicerMode::XZ)->setIndex(tmpX, tmpY, tmpZ);
				tmpChild->slicerDlg(iASlicerMode::XZ)->sbSlice->setValue(tmpY);

				tmpChild->slicer(iASlicerMode::XZ)->update();
				tmpChild->slicer(iASlicerMode::XZ)->update();
				tmpChild->slicerDlg(iASlicerMode::XZ)->update();

				strDetails += GetFilePixel(tmpChild, tmpChild->slicer(iASlicerMode::XZ), tmpX, tmpZ, tmpY, m_mode);
				break;
			default://ERROR
				break;
			}
			tmpChild->update();
		}
	}

	// if requested calculate distance and show actor
	if (pLineActor->GetVisibility())
	{
		double distance = sqrt(pow((m_startMeasurePoint[0] - m_ptMapped[0]), 2) +
			pow((m_startMeasurePoint[1] - m_ptMapped[1]), 2));
		pLineSource->SetPoint2(m_ptMapped[0] - (0.5*slicerSpacing[0]), m_ptMapped[1] - (0.5*slicerSpacing[1]), 0.0);
		pDiskSource->SetOuterRadius(distance);
		pDiskSource->SetInnerRadius(distance);
		strDetails += QString("distance [ %1 ]\n").arg(distance);

	}

	// Update the info text with pixel coordinates/value if requested.
	m_textInfo->GetActor()->SetPosition(m_interactor->GetEventPosition()[0] + 10, m_interactor->GetEventPosition()[1] + 10);
	m_textInfo->GetTextMapper()->SetInput(strDetails.toStdString().c_str());
	m_positionMarkerMapper->Update();
}

/*
void iASlicer::setMeasurementStartPoint(int x, int y)
{
	m_measureStart[0] = x;
	m_measureStart[1] = y;
}
*/

void iASlicer::executeKeyPressEvent()
{
	switch (m_interactor->GetKeyCode())
	{
	case 'm':
		m_startMeasurePoint[0] = m_ptMapped[0];
		m_startMeasurePoint[1] = m_ptMapped[1];
		// does not work reliably (often snaps to positions not the highest gradient close to the current position)
		// and causes access to pixels outside of the image:
		//snapToHighGradient(m_startMeasurePoint[0], m_startMeasurePoint[1]);

		if (m_decorations && pLineSource && hasChannel(0))
		{
			// TODO: check which reslicer makes sense here!
			auto reslicer = m_channels[0]->reslicer;
			double * slicerSpacing = reslicer->GetOutput()->GetSpacing();
			pLineSource->SetPoint1(m_startMeasurePoint[0] - (0.5*slicerSpacing[0]), m_startMeasurePoint[1] - (0.5*slicerSpacing[1]), 0.0);
			pDiskActor->SetPosition(m_startMeasurePoint[0] - (0.5*slicerSpacing[0]), m_startMeasurePoint[1] - (0.5*slicerSpacing[1]), 1.0);
			pLineActor->SetVisibility(true);
			pDiskActor->SetVisibility(true);
			double result[4];
			double xCoord, yCoord, zCoord;
			getMouseCoord(xCoord, yCoord, zCoord, result);
			printVoxelInformation(xCoord, yCoord, zCoord);
		}
		break;
	case 27: //ESCAPE
		if (m_decorations && pLineActor != NULL) {
			pLineActor->SetVisibility(false);
			pDiskActor->SetVisibility(false);
		}
		break;
	}
}


void iASlicer::defaultOutput()
{
	if (!m_decorations)
		return;

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

	int xInd = SlicerXInd(m_mode);
	int yInd = SlicerYInd(m_mode);
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

void iASlicer::updateReslicer()
{
	for (auto ch : m_channels)
		ch->updateReslicer();
}

void iASlicer::setShowText(bool isVisible)
{
	if (!m_decorations)
		return;
	m_textInfo->Show(isVisible);
}

void iASlicer::enableChannel(uint id, bool enabled)
{
	if (enabled)
	{
		m_ren->AddActor(getChannel(id)->imageActor);
		if (m_decorations)
		{
			m_ren->AddActor(getChannel(id)->cActor);
		}
	}
	else
	{
		m_ren->RemoveActor(getChannel(id)->imageActor);
		if (m_decorations)
		{
			m_ren->RemoveActor(getChannel(id)->cActor);
		}
	}
}

void iASlicer::updateChannel(uint id, iAChannelData const & chData)
{
	getChannel(id)->reInit(chData);
}

void iASlicer::setResliceChannelAxesOrigin(uint id, double x, double y, double z)
{
	getChannel(id)->setResliceAxesOrigin(x, y, z);
}

void iASlicer::setChannelOpacity(uint id, double opacity)
{
	getChannel(id)->imageActor->SetOpacity(opacity);
}

void iASlicer::setSlabThickness(int thickness)
{
	m_slabThickness = thickness;
	for (auto ch : m_channels)
		ch->reslicer->SetSlabNumberOfSlices(thickness);
	update();
}

void iASlicer::setSlabCompositeMode(int slabCompositeMode)
{
	m_slabCompositeMode = slabCompositeMode;
	for (auto ch : m_channels)
		ch->reslicer->SetSlabMode(slabCompositeMode);
	update();
}

QSharedPointer<iAChannelSlicerData> iASlicer::createChannel(uint id)
{
	if (m_channels.contains(id))
		throw std::runtime_error(QString("iASlicer: Channel with ID %1 already exists!").arg(id).toStdString());

	QSharedPointer<iAChannelSlicerData> newData(new iAChannelSlicerData);
	newData->imageActor->SetInterpolate(m_settings.LinearInterpolation);
	newData->reslicer->SetSlabNumberOfSlices(m_slabThickness);
	newData->reslicer->SetSlabMode(m_slabCompositeMode);
	newData->setTransform(m_transform);
	m_channels.insert(id, newData);
	return newData;
}

iAChannelSlicerData * iASlicer::getChannel(uint id)
{
	assert(m_channels.contains(id));
	if (!m_channels.contains(id))
		return nullptr;
	return m_channels.find(id)->data();
}

void iASlicer::removeChannel(uint id)
{
	m_channels.remove(id);
}

/*
size_t iASlicer::channelCount() const
{
	return m_channels.size();
}
*/

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
	m_angle[getSlicerDimension(m_mode)] = angle;

	if (!hasChannel(0))
		return;

	double center[3];

	// TODO: allow selecting center for rotation? current: always use first image!
	auto imageData = m_channels[0]->image;
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
		return;
	for (auto ch : m_channels)
		ch->cFilter->GenerateValues(numberOfContours, contourMin, contourMax);
}

void iASlicer::setContours(int n, double * contourValues)
{
	if (!m_decorations)
		return;
	for (auto ch : m_channels)
	{
		ch->cFilter->SetNumberOfContours(n);
		for (int i = 0; i < n; ++i)
			ch->cFilter->SetValue(i, contourValues[i]);
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
			pm = QPixmap(":/images/" + s.section(' ', 0, 1) + ".png");
		QPixmap cpm(pm.size());
		cpm.fill(color);
		cpm.setMask(pm.createMaskFromColor(Qt::transparent));
		m_mouseCursor = QCursor(cpm);
	}
	else
	{
		m_mouseCursor = QCursor(Qt::CrossCursor);
	}
	emit updateSignal();
}

/*
vtkScalarBarWidget * iASlicer::getScalarBarWidget()
{
	assert(m_decorations);
	return m_scalarBarWidget;
}
*/

void iASlicer::setScalarBarTF(vtkScalarsToColors* ctf)
{
	m_scalarBarWidget->GetScalarBarActor()->SetLookupTable(ctf);
	m_scalarBarWidget->SetEnabled(ctf != nullptr);
}

QCursor iASlicer::getMouseCursor()
{
	return m_mouseCursor;
}

int iASlicer::sliceNumber() const
{
	return m_sliceNumber;
}

#define EPSILON 0.0015

struct PickedData
{
	double pos[3];
	double res[3];
	int ind[3];
};
PickedData	pickedData;

void iASlicer::keyPressEvent(QKeyEvent *event)
{
	vtkRenderer * ren = m_renWin->GetRenderers()->GetFirstRenderer();

	if (event->key() == Qt::Key_R)
	{
		ren->ResetCamera();
	}
	if (event->key() == Qt::Key_O)
	{
		pickPoint(pickedData.pos, pickedData.res, pickedData.ind);
		// TODO: fisheye lens on all channels???
		if (!hasChannel(0))
			return;
		auto reslicer = getChannel(0)->reslicer;
		if (!fisheyeLensActivated)
		{
			fisheyeLensActivated = true;
			reslicer->SetAutoCropOutput(!reslicer->GetAutoCropOutput());
			ren->SetWorldPoint(pickedData.res[SlicerXInd(m_mode)], pickedData.res[SlicerYInd(m_mode)], 0, 1);

			initializeFisheyeLens(reslicer);

			updateFisheyeTransform(ren->GetWorldPoint(), reslicer, fisheyeRadius, innerFisheyeRadius);
		}
		else
		{
			fisheyeLensActivated = false;
			reslicer->SetAutoCropOutput(!reslicer->GetAutoCropOutput());

			// Clear outdated circles and actors (not needed for final version)
			for (int i = 0; i < circle1ActList.length(); ++i)
				ren->RemoveActor(circle1ActList.at(i));
			//circle1List.clear();
			circle1ActList.clear();

			for (int i = 0; i < circle2ActList.length(); ++i)
				ren->RemoveActor(circle2ActList.at(i));
			circle2List.clear();
			circle2ActList.clear(); //*/

			ren->RemoveActor(fisheyeActor);

			// No fisheye transform
			double bounds[6];
			reslicer->GetInformationInput()->GetBounds(bounds);
			p_target->SetNumberOfPoints(4);
			p_source->SetNumberOfPoints(4);
			p_target->SetPoint(0, bounds[0], bounds[2], 0); //x_min, y_min, bottom left
			p_target->SetPoint(1, bounds[0], bounds[3], 0); //x_min, y_max, top left
			p_target->SetPoint(2, bounds[1], bounds[3], 0); //x_max, y_max, top right
			p_target->SetPoint(3, bounds[1], bounds[2], 0); //x_max, y_min, bottom right
			p_source->SetPoint(0, bounds[0], bounds[2], 0); //x_min, y_min, bottom left
			p_source->SetPoint(1, bounds[0], bounds[3], 0); //x_min, y_max, top left
			p_source->SetPoint(2, bounds[1], bounds[3], 0); //x_max, y_max, top right
			p_source->SetPoint(3, bounds[1], bounds[2], 0); //x_max, y_min, bottom right

			fisheyeTransform->SetSourceLandmarks(p_source);
			fisheyeTransform->SetTargetLandmarks(p_target);
			reslicer->SetResliceTransform(fisheyeTransform);
			update();
		}
	}

	// magnify and unmagnify fisheye lens and distortion radius
	if (fisheyeLensActivated) {

		pickPoint(pickedData.pos, pickedData.res, pickedData.ind);
		ren->SetWorldPoint(pickedData.res[SlicerXInd(m_mode)], pickedData.res[SlicerYInd(m_mode)], 0, 1);
		if (!hasChannel(0))
			return;
		// TODO: fisheye lens on all channels???
		auto reslicer = getChannel(0)->reslicer;
		if (event->modifiers().testFlag(Qt::ControlModifier)) {
			if (event->key() == Qt::Key_Minus) {

				if (fisheyeRadius <= 20 && innerFisheyeRadius + 1.0 <= 20.0) {
					innerFisheyeRadius = innerFisheyeRadius + 1.0;
					updateFisheyeTransform(ren->GetWorldPoint(), reslicer, fisheyeRadius, innerFisheyeRadius);

				}

				else {
					if ((innerFisheyeRadius + 2.0) <= fisheyeRadius) {
						innerFisheyeRadius = innerFisheyeRadius + 2.0;
						updateFisheyeTransform(ren->GetWorldPoint(), reslicer, fisheyeRadius, innerFisheyeRadius);
					}

				}
			}
			if (event->key() == Qt::Key_Plus) {

				if (fisheyeRadius <= 20 && innerFisheyeRadius - 1.0 >= 1.0) {
					innerFisheyeRadius = innerFisheyeRadius - 1.0;
					updateFisheyeTransform(ren->GetWorldPoint(), reslicer, fisheyeRadius, innerFisheyeRadius);

				}
				else {
					if ((innerFisheyeRadius - 2.0) >= (innerFisheyeMinRadius)) {
						innerFisheyeRadius = innerFisheyeRadius - 2.0;
						updateFisheyeTransform(ren->GetWorldPoint(), reslicer, fisheyeRadius, innerFisheyeRadius);

					}
				}
			}
		}
		else if (!(event->modifiers().testFlag(Qt::ControlModifier))) {
			if (event->key() == Qt::Key_Plus) {

				if (fisheyeRadius + 1.0 <= 20.0) {
					fisheyeRadius = fisheyeRadius + 1.0;
					innerFisheyeRadius = innerFisheyeRadius + 1.0;
					updateFisheyeTransform(ren->GetWorldPoint(), reslicer, fisheyeRadius, innerFisheyeRadius);

				}
				else {
					if (!(fisheyeRadius + 10.0 > maxFisheyeRadius)) {
						fisheyeRadius = fisheyeRadius + 10.0;
						innerFisheyeRadius = innerFisheyeRadius + 10.0;
						innerFisheyeMinRadius = innerFisheyeMinRadius + 8;
						updateFisheyeTransform(ren->GetWorldPoint(), reslicer, fisheyeRadius, innerFisheyeRadius);

					}
				}
			}
			if (event->key() == Qt::Key_Minus) {

				if (fisheyeRadius - 1.0 < 20.0 && fisheyeRadius - 1.0 >= minFisheyeRadius) {
					fisheyeRadius = fisheyeRadius - 1.0;
					innerFisheyeRadius = innerFisheyeRadius - 1.0;
					if (innerFisheyeMinRadius > 1.0)
						innerFisheyeMinRadius = 1.0;

					updateFisheyeTransform(ren->GetWorldPoint(), reslicer, fisheyeRadius, innerFisheyeRadius);

				}
				else {

					if (!(fisheyeRadius - 10.0 < minFisheyeRadius)) {
						fisheyeRadius = fisheyeRadius - 10.0;
						innerFisheyeRadius = innerFisheyeRadius - 10.0;
						innerFisheyeMinRadius = innerFisheyeMinRadius - 8;

						if (innerFisheyeRadius < innerFisheyeMinRadius)
							innerFisheyeRadius = innerFisheyeMinRadius;

						updateFisheyeTransform(ren->GetWorldPoint() /*pickedData.pos*/, reslicer, fisheyeRadius, innerFisheyeRadius);
					}
				}
			}
		}
	}

	if (!fisheyeLensActivated)
	{
		iAVtkWidget::keyPressEvent(event);
		return;
	}
	// if not in snake m_viewMode
	if (m_interactionMode != SHOW)
	{
		if (event->key() == Qt::Key_Tab && m_interactionMode == NORMAL)
			switchInteractionMode(DEFINE_SPLINE);

		else
			switchInteractionMode(NORMAL);

		// let other slice views know that slice view m_viewMode changed
		emit switchedMode(m_interactionMode);
	}
	iAVtkWidget::keyPressEvent(event);
}

void iASlicer::mousePressEvent(QMouseEvent *event)
{
	if (!fisheyeLensActivated)
	{
		iAVtkWidget::mousePressEvent(event);
		return;
	}

	double pos[3];
	double result[4];
	int indxs[3];
	if (!pickPoint(pos, result, indxs))
	{
		iAVtkWidget::mousePressEvent(event);
		return;
	}

	if (m_isSliceProfEnabled
		&& (event->modifiers() == Qt::NoModifier)
		&& event->button() == Qt::LeftButton)//if slice profile m_viewMode is enabled do all the necessary operations
	{
		setSliceProfile(result);
	}

	if (m_isArbProfEnabled
		&& (event->modifiers() == Qt::NoModifier)
		&& event->button() == Qt::LeftButton)//if arbitrary profile m_viewMode is enabled do all the necessary operations
	{
		m_arbProfile->FindSelectedPntInd(result[SlicerXInd(m_mode)], result[SlicerYInd(m_mode)]);
	}

	// only changed mouse press behaviour if m_viewMode is in drawing m_viewMode
	// and left mouse button is pressed
	if (m_decorations && m_interactionMode == DEFINE_SPLINE && event->button() == Qt::LeftButton)
	{
		const double x = result[SlicerXInd(m_mode)];
		const double y = result[SlicerYInd(m_mode)];

		// if no point is found at picked position add a new one
		if (m_snakeSpline->CalculateSelectedPoint(x, y) == -1)
		{
			m_snakeSpline->addPoint(x, y);

			// add the point to the world point list only once because it is a member of MdiChild
			m_worldSnakePoints->InsertNextPoint(result[0], result[1], result[2]);

			// let other slices views know that a new point was created
			emit addedPoint(result[0], result[1], result[2]);
		}
	}
	else
	{
		emit clicked();
	}
	iAVtkWidget::mousePressEvent(event);
}

void iASlicer::mouseMoveEvent(QMouseEvent *event)
{
	iAVtkWidget::mouseMoveEvent(event);
	if (!fisheyeLensActivated)
		return;

	pickPoint(pickedData.pos, pickedData.res, pickedData.ind);

	if (fisheyeLensActivated)
	{
		if (!hasChannel(0))
			return;
		// TODO: fisheye lens on all channels???
		auto reslicer = getChannel(0)->reslicer;
		vtkRenderer * ren = m_renWin->GetRenderers()->GetFirstRenderer();
		ren->SetWorldPoint(pickedData.res[SlicerXInd(m_mode)], pickedData.res[SlicerYInd(m_mode)], 0, 1);
		updateFisheyeTransform(ren->GetWorldPoint(), reslicer, fisheyeRadius, innerFisheyeRadius);
	}

	if (!event->modifiers().testFlag(Qt::ShiftModifier))
	{
		updateMagicLens();
	}

	// only do something in spline drawing mode and if a point is selected
	if (m_decorations && m_interactionMode == DEFINE_SPLINE && m_snakeSpline->selectedPointIndex() != -1)
	{
		// Do a pick. It will return a non-zero value if we intersected the image.
		if (!m_pointPicker->Pick(m_interactor->GetEventPosition()[0],
			m_interactor->GetEventPosition()[1],
			0,  // always zero.
			m_renWin->GetRenderers()->GetFirstRenderer()))
		{
			return;
		}

		// Get the first point of found intersections
		double *ptMapped = m_pointPicker->GetPickedPositions()->GetPoint(0);

		// Move world and slice view points
		double *point = m_worldSnakePoints->GetPoint(m_snakeSpline->selectedPointIndex());

		double pos[3];
		int indxs[3] = { SlicerXInd(m_mode), SlicerYInd(m_mode), SlicerZInd(m_mode) };

		for (int i = 0; i < 2; ++i)				//2d: x, y
			pos[indxs[i]] = ptMapped[i];
		pos[indxs[2]] = point[indxs[2]];	//z

		movePoint(m_snakeSpline->selectedPointIndex(), pos[0], pos[1], pos[2]);

		// update world point list only once because it is a member of MdiChild
		m_worldSnakePoints->SetPoint(m_snakeSpline->selectedPointIndex(), pos[0], pos[1], pos[2]);

		// let other slice views know that a point was moved
		emit movedPoint(m_snakeSpline->selectedPointIndex(), pos[0], pos[1], pos[2]);
	}

	if (m_isSliceProfEnabled
		&& (event->modifiers() == Qt::NoModifier))//if profile m_viewMode is enabled do all the necessary operations
	{
		if (event->buttons()&Qt::LeftButton)
		{
			double xPos, yPos, zPos;
			double result[4];
			int x, y, z;
			if (!pickPoint(xPos, yPos, zPos, result, x, y, z))
				return;
			setSliceProfile(result);
		}
	}

	if (m_isArbProfEnabled)
	{
		int arbProfPtInd = m_arbProfile->GetPntInd();
		if (event->modifiers() == Qt::NoModifier && arbProfPtInd >= 0)//if profile m_viewMode is enabled do all the necessary operations
		{
			if (event->buttons()&Qt::LeftButton)
			{
				double xPos, yPos, zPos;
				double result[4];
				int x, y, z;
				if (!pickPoint(xPos, yPos, zPos, result, x, y, z))
					return;

				double *ptPos = m_arbProfile->GetPosition(arbProfPtInd);
				const int zind = SlicerZInd(m_mode);
				result[zind] = ptPos[zind];

				if (setArbitraryProfile(arbProfPtInd, result, true))
					emit arbitraryProfileChanged(arbProfPtInd, result);
			}
		}
	}
}

void iASlicer::deselectPoint()
{
	if (!m_decorations)
		return;
	m_snakeSpline->deselectPoint();
}

void iASlicer::mouseReleaseEvent(QMouseEvent *event)
{
	if (!fisheyeLensActivated)
	{
		iAVtkWidget::mouseReleaseEvent(event);
		return;
	}

	if (m_decorations)
	{
		m_snakeSpline->deselectPoint();
	}
	emit deselectedPoint();  // let other slice views know that the point was deselected
	iAVtkWidget::mouseReleaseEvent(event);
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
	// is m_viewMode spline drawing m_viewMode?
	if (m_decorations && m_interactionMode == DEFINE_SPLINE)
		m_contextMenu->exec(event->globalPos());

	if (m_magicLens && m_magicLens->IsEnabled())
		m_magicLensContextMenu->exec(event->globalPos());
}

void iASlicer::switchInteractionMode(int mode)
{
	if (!m_decorations)
		return;

	m_interactionMode = static_cast<InteractionMode>(mode);
	switch (mode)
	{
	case NORMAL: // normal
		m_snakeSpline->SetVisibility(false);
		break;
	case DEFINE_SPLINE: // define spline
		m_snakeSpline->SetVisibility(true);
		break;
	case SHOW: // show
		m_snakeSpline->SetVisibility(false);
		break;
	}
	m_renWin->GetInteractor()->Render();
}

void iASlicer::addPoint(double xPos, double yPos, double zPos)
{
	if (!m_decorations)
		return;
	double pos[3] = { xPos, yPos, zPos };
	double x = pos[SlicerXInd(m_mode)];
	double y = pos[SlicerYInd(m_mode)];

	//add point to the snake slicer spline
	m_snakeSpline->addPoint(x, y);

	// render slice view
	GetRenderWindow()->GetInteractor()->Render();
}

void iASlicer::setSliceProfile(double Pos[3])
{
	if (!hasChannel(0))
		return;
	// TODO: slice profile on selected/current channel
	auto reslicer = getChannel(0)->reslicer;
	vtkImageData * reslicedImgData = reslicer->GetOutput();
	double PosY = Pos[SlicerYInd(m_mode)];
	if (!m_sliceProfile->updatePosition(PosY, reslicedImgData))
		return;
	// render slice view
	GetRenderWindow()->GetInteractor()->Render();
}

bool iASlicer::setArbitraryProfile(int pointInd, double * Pos, bool doClamp)
{
	if (!m_decorations || !hasChannel(0))
		return false;
	// TODO: slice profile on selected/current channel
	auto reslicer = getChannel(0)->reslicer;
	auto imageData = getChannel(0)->image;
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
	double profileCoord2d[2] = { Pos[SlicerXInd(m_mode)], Pos[SlicerYInd(m_mode)] };
	if (!m_arbProfile->setup(pointInd, Pos, profileCoord2d, reslicer->GetOutput()))
		return false;
	GetRenderWindow()->GetInteractor()->Render();
	return true;
}

void iASlicer::movePoint(size_t selectedPointIndex, double xPos, double yPos, double zPos)
{
	if (!m_decorations)
	{
		return;
	}
	double pos[3] = { xPos, yPos, zPos };
	double x = pos[SlicerXInd(m_mode)];
	double y = pos[SlicerYInd(m_mode)];

	// move only if a point is selected
	if (selectedPointIndex != -1)
	{
		// move only if a point is selected
		m_snakeSpline->movePoint(selectedPointIndex, x, y);

		// render slice view
		GetRenderWindow()->GetInteractor()->Render();
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
		return;

	//delete all points from snake slicer spline
	m_snakeSpline->deleteAllPoints();

	// reset point lists
	m_worldSnakePoints->Reset();

	// render slice view
	m_renWin->GetInteractor()->Render();
}

void iASlicer::setSliceProfileOn(bool isOn)
{
	m_isSliceProfEnabled = isOn;
	m_sliceProfile->SetVisibility(m_isSliceProfEnabled);
	m_renWin->GetInteractor()->Render();
}

int iASlicer::pickPoint(double *pos_out, double *result_out, int * ind_out)
{
	return pickPoint(pos_out[0], pos_out[1], pos_out[2], result_out, ind_out[0], ind_out[1], ind_out[2]);
}

int iASlicer::pickPoint(double &xPos_out, double &yPos_out, double &zPos_out,
	double *result_out, int &xInd_out, int &yInd_out, int &zInd_out)
{
	// Do a pick. It will return a non-zero value if we intersected the image.
	vtkPointPicker* pointPicker = (vtkPointPicker*)GetInteractor()->GetPicker();

	int * eventPos = GetInteractor()->GetEventPosition();
	if (!pointPicker->Pick(eventPos[0], eventPos[1], 0,  // Z is always zero.
		GetRenderWindow()->GetRenderers()->GetFirstRenderer()))
	{
		return 0;
	}

	// get coordinates of the picked point
	double ptMapped[3];
	vtkIdType total_points = pointPicker->GetPickedPositions()->GetNumberOfPoints();
	pointPicker->GetPickedPositions()->GetPoint(total_points - 1, ptMapped);

	if (!hasChannel(0))
		return 0;
	// TODO: slice profile on selected/current channel
	auto reslicer = getChannel(0)->reslicer;
	auto imageData = getChannel(0)->image;

	// get image spacing to be able to select a point independent of zoom level
	double spacing[3];
	imageData->GetSpacing(spacing);

	vtkMatrix4x4 * resliceAxes = reslicer->GetResliceAxes();
	double point[4] = { ptMapped[0], ptMapped[1], 0, 1 }; // We have to set the physical z-coordinate which requires us to get the volume spacing.
	resliceAxes->MultiplyPoint(point, result_out);

	xInd_out = (int)(result_out[0] / spacing[0] + 0.5);
	yInd_out = (int)(result_out[1] / spacing[1] + 0.5);
	zInd_out = (int)(result_out[2] / spacing[2] + 0.5);

	// initialize positions depending on slice view
	switch (m_mode)
	{
	default:
	case iASlicerMode::YZ: { xPos_out = m_xInd * spacing[0]; yPos_out = ptMapped[0];         zPos_out = ptMapped[1];         } break;
	case iASlicerMode::XZ: { xPos_out = ptMapped[0];         yPos_out = m_yInd * spacing[1]; zPos_out = ptMapped[1];         } break;
	case iASlicerMode::XY: { xPos_out = ptMapped[0];         yPos_out = ptMapped[1];         zPos_out = m_zInd * spacing[2]; } break;
	}

	return 1;
}

void iASlicer::setIndex(int x, int y, int z)
{
	m_xInd = x; m_yInd = y; m_zInd = z;
}

void iASlicer::setLinkedMdiChild(MdiChild* mdiChild)
{
	m_linkedMdiChild = mdiChild;
}

void iASlicer::slicerUpdatedSlot()
{
	setCursor(getMouseCursor());
	if (m_isSliceProfEnabled)
		updateProfile();
}

void iASlicer::updateProfile()
{
	double oldPos[3];
	m_sliceProfile->GetPoint(0, oldPos);
	double pos[3] = { oldPos[1], oldPos[1], oldPos[1] };
	setSliceProfile(pos);
}

void iASlicer::setArbitraryProfileOn(bool isOn)
{
	if (!m_decorations)
	{
		return;
	}
	m_isArbProfEnabled = isOn;
	m_arbProfile->SetVisibility(m_isArbProfEnabled);
	GetRenderWindow()->GetInteractor()->Render();
}

void iASlicer::setPieGlyphsOn(bool isOn)
{
	m_pieGlyphsEnabled = isOn;
	computeGlyphs();
}

void iASlicer::resizeEvent(QResizeEvent * event)
{
	updateMagicLens();
	iAVtkWidget::resizeEvent(event);
}

void iASlicer::wheelEvent(QWheelEvent* event)
{
	event->accept();
	if (event->modifiers().testFlag(Qt::ControlModifier) && receivers(SIGNAL(ctrlMouseWheel(int))) > 0)
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
	else
	{
		iAVtkWidget::wheelEvent(event);
		pickPoint(pickedData.pos, pickedData.res, pickedData.ind);
	}
	updateMagicLens();
}

void iASlicer::menuCenteredMagicLens()
{
	if (!m_magicLens)
		return;
	m_magicLens->SetViewMode(iAMagicLens::CENTERED);
	updateMagicLens();
}

void iASlicer::menuOffsetMagicLens()
{
	if (!m_magicLens)
		return;
	m_magicLens->SetViewMode(iAMagicLens::OFFSET);
	updateMagicLens();
}

void iASlicer::initializeFisheyeLens(vtkImageReslice* reslicer)
{
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();

	fisheyeTransform = vtkSmartPointer<vtkThinPlateSplineTransform>::New();
	fisheyeTransform->SetBasisToR2LogR();
	p_source = vtkSmartPointer<vtkPoints>::New();
	p_target = vtkSmartPointer<vtkPoints>::New();
	p_source->SetNumberOfPoints(32);
	p_target->SetNumberOfPoints(32);
	reslicer->SetInterpolationModeToLinear(); // added while testing

	fisheye = vtkSmartPointer<vtkRegularPolygonSource>::New();
	fisheye->SetNumberOfSides(60);
	fisheye->GeneratePolygonOff(); // just outlines;
	fisheye->SetRadius(50.0);
	fisheyeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	fisheyeMapper->SetInputConnection(fisheye->GetOutputPort());
	fisheyeActor = vtkSmartPointer<vtkActor>::New();
	fisheyeActor->GetProperty()->SetColor(1.00000, 0.50196, 0.00000);
	fisheyeActor->GetProperty()->SetOpacity(1.0);
	fisheyeActor->SetMapper(fisheyeMapper);
	ren->AddActor(fisheyeActor);

	// Create circle actors (green and red) to show the transform landmarks
	for (int i = 0; i < p_target->GetNumberOfPoints(); ++i)
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

		circle1List.append(circle);
		circle1ActList.append(circleActor);
		ren->AddActor(circleActor);
	}

	for (int i = 0; i < p_source->GetNumberOfPoints(); ++i)
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

		circle2List.append(circle);
		circle2ActList.append(circleActor);
		ren->AddActor(circleActor);
	}
}

void iASlicer::updateFisheyeTransform(double focalPt[3], vtkImageReslice* reslicer, double lensRadius, double innerLensRadius)
{
	vtkImageData * reslicedImgData = reslicer->GetOutput();
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();

	double * spacing = reslicedImgData->GetSpacing();
	double * origin = reslicedImgData->GetOrigin();

	double bounds[6];
	reslicer->GetInformationInput()->GetBounds(bounds);

	p_target->SetNumberOfPoints(32); // already set above!
	p_source->SetNumberOfPoints(32);
	int sn = sliceNumber();

	std::cout << bounds[0] << " " << bounds[1] << " " << bounds[2] << " " << bounds[3] << " " << bounds[4] << " " << bounds[5] << std::endl;
	std::cout << focalPt[0] << " " << focalPt[1] << " " << focalPt[2] << std::endl;

	switch (m_mode)
	{
	case iASlicerMode::YZ:
		p_target->SetPoint(0, sn * spacing[0], bounds[2], bounds[2]); //x_min, y_min, bottom left // left border points
		p_target->SetPoint(1, sn * spacing[0], bounds[2], 0.5  * bounds[5]);
		p_target->SetPoint(2, sn * spacing[0], bounds[2], bounds[5]); //x_min, y_max, top left // top border points
		p_target->SetPoint(3, sn * spacing[0], 0.5  * bounds[3], bounds[5]);
		p_target->SetPoint(4, sn * spacing[0], bounds[3], bounds[5]); //x_max, y_max, top right // right border points
		p_target->SetPoint(5, sn * spacing[0], bounds[3], 0.5  * bounds[5]);
		p_target->SetPoint(6, sn * spacing[0], bounds[3], bounds[2]); //x_max, y_min, bottom right // bottom border points
		p_target->SetPoint(7, sn * spacing[0], 0.5  * bounds[3], bounds[2]);
		break;
	case iASlicerMode::XY:
		p_target->SetPoint(0, bounds[0], bounds[2], sn * spacing[2]); //x_min, y_min, bottom left // left border points
		p_target->SetPoint(1, bounds[0], 0.5  * bounds[3], sn * spacing[2]);
		p_target->SetPoint(2, bounds[0], bounds[3], sn * spacing[2]); //x_min, y_max, top left // top border points
		p_target->SetPoint(3, 0.5  * bounds[1], bounds[3], sn * spacing[2]);
		p_target->SetPoint(4, bounds[1], bounds[3], sn * spacing[2]); //x_max, y_max, top right // right border points
		p_target->SetPoint(5, bounds[1], 0.5  * bounds[3], sn * spacing[2]);
		p_target->SetPoint(6, bounds[1], bounds[2], sn * spacing[2]); //x_max, y_min, bottom right // bottom border points
		p_target->SetPoint(7, 0.5  * bounds[1], bounds[2], sn * spacing[2]);
		break;
	case iASlicerMode::XZ:
		p_target->SetPoint(0, bounds[0], sn * spacing[1], bounds[4]); //x_min, y_min, bottom left // left border points
		p_target->SetPoint(1, bounds[0], sn * spacing[1], 0.5  * bounds[5]);
		p_target->SetPoint(2, bounds[0], sn * spacing[1], bounds[5]); //x_min, y_max, top left // top border points
		p_target->SetPoint(3, 0.5  * bounds[1], sn * spacing[1], bounds[5]);
		p_target->SetPoint(4, bounds[1], sn * spacing[1], bounds[5]); //x_max, y_max, top right // right border points
		p_target->SetPoint(5, bounds[1], sn * spacing[1], 0.5  * bounds[5]);
		p_target->SetPoint(6, bounds[1], sn * spacing[1], bounds[4]); //x_max, y_min, bottom right // bottom border points
		p_target->SetPoint(7, 0.5  * bounds[1], sn * spacing[1], bounds[4]);
		break;
	default:
		break;
	}

	for (int i = 0; i < p_target->GetNumberOfPoints() - (p_target->GetNumberOfPoints() - 8); ++i)
		p_source->SetPoint(i, p_target->GetPoint(i));

	int fixPoints = 8;
	// outer circle 1
	double fixRadiusX;
	double fixRadiusY;
	for (int fix = p_target->GetNumberOfPoints() - 8 - 8 - 8; fix < p_target->GetNumberOfPoints() - 8 - 8; fix++)
	{
		fixRadiusX = (lensRadius + 15.0)* std::cos(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];
		fixRadiusY = (lensRadius + 15.0)* std::sin(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];

		switch (m_mode)
		{
		case iASlicerMode::YZ:
			p_source->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			p_target->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XY:
			p_target->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			p_source->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			break;
		case iASlicerMode::XZ:
			p_target->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			p_source->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			break;
		default:
			break;
		}
	}
	// outer circle 2
	fixPoints = 8;
	for (int fix = p_target->GetNumberOfPoints() - 8 - 8; fix < p_target->GetNumberOfPoints() - 8; fix++)
	{
		fixRadiusX = (lensRadius + 80.0)* std::cos(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];
		fixRadiusY = (lensRadius + 80.0)* std::sin(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];

		switch (m_mode)
		{
		case iASlicerMode::YZ:
			p_source->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			p_target->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XY:
			p_target->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			p_source->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			break;
		case iASlicerMode::XZ:
			p_target->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			p_source->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			break;
		default:
			break;
		}
	}

	int pointsCount = 8;
	for (int i = p_target->GetNumberOfPoints() - pointsCount; i < p_target->GetNumberOfPoints(); ++i)
	{
		double xCoordCircle1 = (innerLensRadius)* std::cos(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];
		double yCoordCircle1 = (innerLensRadius)* std::sin(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];

		double xCoordCircle2 = (lensRadius)* std::cos(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];
		double yCoordCircle2 = (lensRadius)* std::sin(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];

		switch (m_mode)
		{
		case iASlicerMode::YZ:
			p_source->SetPoint(i, sn * spacing[0], focalPt[0] + xCoordCircle2, focalPt[1] + yCoordCircle2);
			p_target->SetPoint(i, sn * spacing[0], focalPt[0] + xCoordCircle1, focalPt[1] + yCoordCircle1);
			break;
		case iASlicerMode::XY:
			p_target->SetPoint(i, focalPt[0] + xCoordCircle1, focalPt[1] + yCoordCircle1, sn * spacing[2]);
			p_source->SetPoint(i, focalPt[0] + xCoordCircle2, focalPt[1] + yCoordCircle2, sn * spacing[2]);
			break;
		case iASlicerMode::XZ:
			p_target->SetPoint(i, focalPt[0] + xCoordCircle1, sn * spacing[1], focalPt[1] + yCoordCircle1);
			p_source->SetPoint(i, focalPt[0] + xCoordCircle2, sn * spacing[1], focalPt[1] + yCoordCircle2);
			break;
		default:
			break;
		}
	}

	// Set position and text for green circle1 actors
	for (int i = 0; i < p_target->GetNumberOfPoints(); ++i)
	{
		if (m_mode == iASlicerMode::YZ)
		{
			circle1List.at(i)->SetCenter(p_target->GetPoint(i)[1], p_target->GetPoint(i)[2], 0.0);
			circle2List.at(i)->SetCenter(p_source->GetPoint(i)[1], p_source->GetPoint(i)[2], 0.0);
		}
		if (m_mode == iASlicerMode::XY)
		{
			circle1List.at(i)->SetCenter(p_target->GetPoint(i)[0], p_target->GetPoint(i)[1], 0.0);
			circle2List.at(i)->SetCenter(p_source->GetPoint(i)[0], p_source->GetPoint(i)[1], 0.0);
		}

		if (m_mode == iASlicerMode::XZ)
		{
			circle1List.at(i)->SetCenter(p_target->GetPoint(i)[0], p_target->GetPoint(i)[2], 0.0);
			circle2List.at(i)->SetCenter(p_source->GetPoint(i)[0], p_source->GetPoint(i)[2], 0.0);
		}
	}

	fisheye->SetCenter(focalPt[0], focalPt[1], 0.0);
	fisheye->SetRadius(lensRadius * reslicer->GetOutput()->GetSpacing()[0]);

	fisheyeTransform->SetSourceLandmarks(p_source); // red
	fisheyeTransform->SetTargetLandmarks(p_target);  // green

	reslicer->SetResliceTransform(fisheyeTransform);
	reslicer->Update();
	update();
}

void iASlicer::updateMagicLens()
{
	if (!m_magicLens || !m_magicLens->IsEnabled())
		return;
	vtkRenderer * ren = m_renWin->GetRenderers()->GetFirstRenderer();
	ren->SetWorldPoint(pickedData.res[SlicerXInd(m_mode)], pickedData.res[SlicerYInd(m_mode)], 0, 1);
	ren->WorldToDisplay();
	double dpos[3];
	ren->GetDisplayPoint(dpos);
	int lensSz = m_magicLens->GetSize();
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
	m_magicLens->UpdatePosition(ren->GetActiveCamera(), worldP, mousePos);
}

void iASlicer::computeGlyphs()
{
	/*
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();
	bool hasPieGlyphs = (m_pieGlyphs.size() > 0);
	if(hasPieGlyphs)
	{
		for(int i = 0; i< m_pieGlyphs.size(); ++i)
			ren->RemoveActor(m_pieGlyphs[i]->actor);
		m_pieGlyphs.clear();
	}

	if(!m_pieGlyphsEnabled)
	{
		if(hasPieGlyphs)
			GetRenderWindow()->GetInteractor()->Render();
		return;
	}

	QVector<double> angleOffsets;

	for (int chan = 0; chan < GetEnabledChannels(); ++chan)
	{
		iAChannelSlicerData * chSlicerData = GetChannel(static_cast<iAChannelID>(ch_Concentration0 + chan));
		if (!chSlicerData || !chSlicerData->isInitialized())
			continue;

		vtkSmartPointer<vtkImageResample> resampler = vtkSmartPointer<vtkImageResample>::New();
		resampler->SetInputConnection( chSlicerData->reslicer->GetOutputPort() );
		resampler->InterpolateOn();
		resampler->SetAxisMagnificationFactor(0, m_pieGlyphMagFactor);
		resampler->SetAxisMagnificationFactor(1, m_pieGlyphMagFactor);
		resampler->SetAxisMagnificationFactor(2, m_pieGlyphMagFactor);
		resampler->Update();

		vtkImageData * imgData = resampler->GetOutput();

		int dims[3];
		imgData->GetDimensions(dims);
		QString scalarTypeStr( imgData->GetScalarTypeAsString() );

		double o[3], s[3];
		imgData->GetOrigin(o); imgData->GetSpacing(s);

		int index = 0;
		for (int y = 0; y < dims[1]; y++)
		{
			for (int x = 0; x < dims[0]; ++x, ++index)
			{
				float portion = static_cast<float*>(imgData->GetScalarPointer(x, y, 0))[0];
				double angularRange[2] = { 0.0, 360.0*portion};
				if(0 != chan)
				{
					angularRange[0] += angleOffsets[index];
					angularRange[1] += angleOffsets[index];
				}

				if(portion > EPSILON)
				{
					iAPieChartGlyph * pieGlyph = new iAPieChartGlyph(angularRange[0], angularRange[1]);
					double pos[3] = { o[0] + x*s[0], o[1] + y*s[1], 1.0 };
					pieGlyph->actor->SetPosition(pos);
					pieGlyph->actor->SetScale( (std::min)(s[0], s[1]) * m_pieGlyphSpacing );
					QColor c(chSlicerData->GetColor());
					double color[3] = { c.redF(), c.greenF(), c.blueF() };
					pieGlyph->actor->GetProperty()->SetColor(color);
					pieGlyph->actor->GetProperty()->SetOpacity(m_pieGlyphOpacity);
					ren->AddActor(pieGlyph->actor);
					m_pieGlyphs.push_back( QSharedPointer<iAPieChartGlyph>(pieGlyph) );
				}

				if(0 == chan)
					angleOffsets.push_back(angularRange[1]);
				else
					angleOffsets[index] = angularRange[1];
			}
		}
	}
	GetRenderWindow()->GetInteractor()->Render();
	*/
}

void iASlicer::setPieGlyphParameters(double opacity, double spacing, double magFactor)
{
	m_pieGlyphOpacity = opacity;
	m_pieGlyphSpacing = spacing;
	m_pieGlyphMagFactor = magFactor;
	computeGlyphs();
}




// declaration in iASlicerMode.h
QString getSlicerModeString(int mode)
{
	switch (mode)
	{
	case YZ: return "YZ";
	case XZ: return "XZ";
	case XY: return "XY";
	default: return "??";
	}
}

QString getSliceAxis(int mode)
{
	switch (mode)
	{
	case YZ: return "X";
	case XZ: return "Y";
	case XY: return "Z";
	default: return "?";
	}
}

int getSlicerDimension(int mode)
{
	switch (mode)
	{
	case YZ: return 0;
	case XZ: return 1;
	case XY: return 2;
	default: return -1;
	}
}