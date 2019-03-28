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
#include "iASlicerData.h"

#include "dlg_commoninput.h"
#include "iAChannelData.h"
#include "iAConnector.h"
#include "iAMagicLens.h"
#include "iAMathUtility.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAMovieHelper.h"
#include "iAPieChartGlyph.h"
#include "iARulerWidget.h"
#include "iARulerRepresentation.h"
#include "iASlicer.h"
#include "iAChannelSlicerData.h"
#include "iASlicerSettings.h"
#include "iAStringHelper.h"
#include "iAToolsITK.h"
#include "iAToolsVTK.h"
#include "iAWrapperText.h"
#include "io/iAIOProvider.h"
#include "mdichild.h"
#include "mainwindow.h"

#include <vtkAlgorithmOutput.h>
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
#include <vtkImageReslice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLineSource.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
//#include <vtkLookupTable.h>
#include <vtkMarchingContourFilter.h>
#include <vtkPlaneSource.h>
#include <vtkPointPicker.h>
//#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQImageToImageSource.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarBarWidget.h>
//#include <vtkScalarsToColors.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor3D.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkVersion.h>
#include <vtkWindowToImageFilter.h>

#include <QBitmap>
#include <QDate>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QThread>
#include "vtkInteractorStyleTrackballActor.h"


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


iASlicerData::iASlicerData( iASlicer * slicerMaster, QObject * parent /*= 0 */,
		bool decorations/*=true*/) : QObject( parent ),
	m_mode(slicerMaster->getMode()),
	m_magicLensExternal(slicerMaster->magicLens()),
	m_renWin( vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New() ),
	m_ren( vtkSmartPointer<vtkRenderer>::New() ),
	m_angleX( 0 ), m_angleY( 0 ), m_angleZ( 0 ),
	m_userSetBackground(false),
	m_decorations(decorations),
	m_interactor(0),
	m_sliceNumber(0),
	m_showPositionMarker(false),
	m_roiActive(false),
	m_interactorStyle(iAInteractorStyleImage::New()),
	m_cameraOwner(true),
	m_camera(vtkCamera::New()),
	m_pointPicker(vtkPointPicker::New()),
	m_transform(vtkTransform::New())
{
	m_renWin->AlphaBitPlanesOn();
	m_renWin->LineSmoothingOn();
	m_renWin->PointSmoothingOn();
	// Turned off, because of gray strokes e.g., on scalarBarActors. Only on NVIDIA graphic cards
	m_renWin->PolygonSmoothingOff();


	if (decorations)
	{
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
	}
}

iASlicerData::~iASlicerData(void)
{
	disconnect();

	m_interactorStyle->Delete();
	m_pointPicker->Delete();

	if (m_cameraOwner)
	{
		m_camera->Delete();
	}
}

//! observer needs to be a separate class; otherwise there is an error when destructing,
//! as vtk deletes all its observers...
class iAObserverRedirect: public vtkCommand
{
public:
	iAObserverRedirect(iASlicerData* redirect): m_redirect(redirect)
	{}
private:
	void Execute(vtkObject * caller, unsigned long eventId, void * callData)
	{
		m_redirect->execute(caller, eventId, callData);
	}
	iASlicerData* m_redirect;
};


void iASlicerData::initialize(vtkAbstractTransform *tr)
{
	m_transform = tr;
	m_renWin->AddRenderer(m_ren);
	setDefaultInteractor();

	m_interactor->SetPicker(m_pointPicker);
	m_interactor->Initialize( );
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

	updateResliceAxesDirectionCosines();
	updateBackground();

	if (m_decorations)
	{
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
		m_logoRep->SetImage(m_logoImage->GetOutput( ));
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
		m_scalarBarWidget->SetInteractor(m_interactor);
		m_scalarBarWidget->GetScalarBarActor()->SetLabelFormat("%.2f");
		m_scalarBarWidget->GetScalarBarActor()->SetTitleTextProperty(m_textProperty);
		m_scalarBarWidget->GetScalarBarActor()->SetLabelTextProperty(m_textProperty);
		m_scalarBarWidget->SetEnabled( true );
		m_scalarBarWidget->SetRepositionable( true );
		m_scalarBarWidget->SetResizable( true );
		m_scalarBarWidget->GetScalarBarRepresentation()->SetOrientation(1);
		m_scalarBarWidget->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue(0.92,0.2);
		m_scalarBarWidget->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue(0.06, 0.75);
		m_scalarBarWidget->GetScalarBarActor()->SetTitle("Range");

		m_positionMarkerMapper->SetInputConnection( m_positionMarkerSrc->GetOutputPort() );
		m_positionMarkerActor->SetMapper( m_positionMarkerMapper );
		m_positionMarkerActor->GetProperty()->SetColor( 0,1,0 );
		m_positionMarkerActor->GetProperty()->SetOpacity( 1 );
		m_positionMarkerActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);
		m_positionMarkerActor->SetVisibility(false);

		pLineSource->SetPoint1( 0.0, 0.0, 0.0 );
		pLineSource->SetPoint2( 10.0, 10.0, 0.0 );
		pLineSource->Update( );
		pLineMapper->SetInputConnection( pLineSource->GetOutputPort() );
		pLineActor->SetMapper( pLineMapper );
		pLineActor->GetProperty()->SetColor( 1.0, 1.0, 1.0 );
		pLineActor->GetProperty()->SetOpacity( 1 );
		pLineActor->SetVisibility( false );

		pDiskSource->SetCircumferentialResolution( 50 );
		pDiskSource->Update( );
		pDiskMapper->SetInputConnection( pDiskSource->GetOutputPort() );
		pDiskActor->SetMapper( pDiskMapper );
		pDiskActor->GetProperty()->SetColor( 1.0, 1.0, 1.0 );
		pDiskActor->GetProperty()->SetOpacity( 1 );
		pDiskActor->SetVisibility( false );
		pDiskActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);

		m_roiMapper->SetInputConnection( m_roiSource->GetOutputPort() );
		m_roiActor->SetVisibility( false );
		m_roiActor->SetMapper( m_roiMapper );
		m_roiActor->GetProperty()->SetColor( 1, 0, 0 );
		m_roiActor->GetProperty()->SetOpacity( 1 );
		m_roiSource->SetCenter( 0, 0, 1 );
		m_roiMapper->Update( );
		m_roiActor->GetProperty()->SetRepresentation( VTK_WIREFRAME );

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
		m_rulerWidget->SetEnabled( true );
		m_rulerWidget->SetRepositionable( true );
		m_rulerWidget->SetResizable( true );
		m_rulerWidget->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue(0.333,0.05);
		m_rulerWidget->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue(0.333,0.051);

		m_ren->AddActor(m_positionMarkerActor);
		m_ren->AddActor(pLineActor);
		m_ren->AddActor(pDiskActor);
		m_ren->AddActor(m_roiActor);
	}
	m_ren->SetActiveCamera(m_camera);
	m_ren->ResetCamera();
}

void iASlicerData::setTransform(vtkAbstractTransform * tr)
{
	m_transform = tr;
	for(auto ch: m_channels)
		ch->setTransform(m_transform);
}

void iASlicerData::setDefaultInteractor()
{
	m_interactor = m_renWin->GetInteractor();
	m_interactor->SetInteractorStyle(m_interactorStyle);
}

void iASlicerData::addImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_ren->AddActor(imgActor);
}

void iASlicerData::removeImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_ren->RemoveActor(imgActor);
}

/*
void iASlicerData::blend(vtkAlgorithmOutput *data, vtkAlgorithmOutput *data2,
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

void iASlicerData::reInitialize( vtkImageData *ds, vtkTransform *tr, vtkScalarsToColors* ctf,
	bool showIsoLines, bool showPolygon)
{
	imageData = ds;
	transform = tr;
	colorTransferFunction = ctf;
	isolines = showIsoLines;
	poly = showPolygon;

	interactor->ReInitialize( );

	InitReslicerWithImageData();

	pointPicker->SetTolerance(PickTolerance);

	if (m_decorations)
	{
		scalarBarWidget->GetScalarBarActor()->SetLookupTable( colorTransferFunction );
	}

	setupColorMapper();
}

void iASlicerData::setupColorMapper()
{
	if (imageData->GetNumberOfScalarComponents() == 1)
	{
		colormapper->SetLookupTable(colorTransferFunction);
	}
	else
	{
		colormapper->SetLookupTable(0);
		if (imageData->GetNumberOfScalarComponents() == 3)
		{
			colormapper->SetOutputFormatToRGB();		// default is RGBA
		}
	}
	if (m_decorations)
		scalarBarWidget->GetRepresentation()->SetVisibility(imageData->GetNumberOfScalarComponents() == 1);
	colormapper->Update();
}
*/

void iASlicerData::setROIVisible(bool visible)
{
	if (!m_decorations)
		return;
	m_roiActive = visible;
	m_roiActor->SetVisibility(visible);
}

void iASlicerData::updateROI(int const roi[6])
{
	if (!m_decorations || !m_roiActive || m_channels.empty())
		return;

	double* spacing = m_channels[m_channels.keys()[0]]->reslicer->GetOutput()->GetSpacing();

	// apparently, image actor starts output at -0,5spacing, -0.5spacing (probably a side effect of BorderOn)
	// That's why we have to subtract 0.5 from the coordinates!
	if (m_mode == iASlicerMode::YZ)
	{
		m_roiSlice[0] = roi[0];
		m_roiSlice[1] = roi[0] + roi[3];
		m_roiSource->SetOrigin((roi[1]-0.5)*spacing[1]                  , (roi[2]-0.5)*spacing[2]                  , 0);
		m_roiSource->SetPoint1((roi[1]-0.5)*spacing[1]+roi[4]*spacing[0], (roi[2]-0.5)*spacing[2]                  , 0);
		m_roiSource->SetPoint2((roi[1]-0.5)*spacing[1]                  , (roi[2]-0.5)*spacing[2]+roi[5]*spacing[2], 0);
	}
	else if (m_mode == iASlicerMode::XY)
	{
		m_roiSlice[0] = roi[2];
		m_roiSlice[1] = roi[2] + roi[5];
		m_roiSource->SetOrigin((roi[0]-0.5)*spacing[0]                  , (roi[1]-0.5)*spacing[1]                  , 0);
		m_roiSource->SetPoint1((roi[0]-0.5)*spacing[0]+roi[3]*spacing[0], (roi[1]-0.5)*spacing[1]                  , 0);
		m_roiSource->SetPoint2((roi[0]-0.5)*spacing[0]                  , (roi[1]-0.5)*spacing[1]+roi[4]*spacing[1], 0);
	}
	else if (m_mode == iASlicerMode::XZ)
	{
		m_roiSlice[0] = roi[1];
		m_roiSlice[1] = roi[1] + roi[4];
		m_roiSource->SetOrigin((roi[0]-0.5)*spacing[0]                  , (roi[2]-0.5)*spacing[2]                  , 0);
		m_roiSource->SetPoint1((roi[0]-0.5)*spacing[0]+roi[3]*spacing[0], (roi[2]-0.5)*spacing[2]                  , 0);
		m_roiSource->SetPoint2((roi[0]-0.5)*spacing[0]                  , (roi[2]-0.5)*spacing[2]+roi[5]*spacing[2], 0);
	}
	m_roiActor->SetVisibility(m_roiSlice[0] <= m_sliceNumber && m_sliceNumber < m_roiSlice[1]);
	m_roiMapper->Update();
	m_interactor->Render();
}

void iASlicerData::setup(iASingleSlicerSettings const & settings)
{
	m_settings = settings;
	for (uint channelID: m_channels.keys())
	{
		m_channels[channelID]->imageActor->SetInterpolate(settings.LinearInterpolation);
	}
	if (m_magicLensExternal)
	{
		m_magicLensExternal->SetInterpolate(settings.LinearInterpolation);
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
}

void iASlicerData::setResliceAxesOrigin( double x, double y, double z )
{
	if (m_interactor->GetEnabled())
	{
		for (auto ch : m_channels)
			ch->reslicer->SetResliceAxesOrigin(x, y, z);
		updateReslicer();
		m_interactor->Render();
	}
}

void iASlicerData::update()
{
	for(auto ch: m_channels)
		ch->updateMapper();
	updateReslicer();
	for (auto ch : m_channels)
		ch->reslicer->UpdateWholeExtent();
	m_interactor->ReInitialize();
	m_interactor->Render();
	m_ren->Render();

	emit updateSignal();
}

void iASlicerData::setPositionMarkerCenter(double x, double y)
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

void iASlicerData::disableInteractor()
{
	m_interactor->Disable();
}

void iASlicerData::enableInteractor()
{
	m_interactor->ReInitialize();
}

void iASlicerData::showIsolines(bool s)
{
	if (!m_decorations)
		return;
	for (auto ch : m_channels)
		ch->setShowContours(s);
}

void iASlicerData::showPosition(bool s)
{
	if (!m_decorations)
		return;
	m_showPositionMarker = s;
}

void iASlicerData::saveMovie( QString& fileName, int qual /*= 2*/ )
{
	QString movie_file_types = GetAvailableMovieFormats();

	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if(!mdi_parent || m_channels.empty())
		return;
	// If VTK was built without video support, display error message and quit.
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(mdi_parent, "Movie Export", "Sorry, but movie export support is disabled.");
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
	
	// TODO: allow selecting channel to export? export all channels?
	auto reslicer = m_channels[m_channels.keys()[0]]->reslicer;
	auto img0 = reslicer->GetOutput();
	int* extent = img0->GetExtent();
	double* origin = img0->GetOrigin();
	double* spacing = img0->GetSpacing();

	emit msg(tr("MOVIE export started. Output: %1").arg(fileName));

	double oldResliceAxesOrigin[3];
	reslicer->GetResliceAxesOrigin(oldResliceAxesOrigin);

	if (m_mode == iASlicerMode::YZ)      // YZ
	{
		for (int i = extent[0]; i < extent[1]; i++ )
		{
			reslicer->SetResliceAxesOrigin( origin[0] + i * spacing[0], origin[1], origin[2] );
			update();
			w2if->Modified();
			movieWriter->Write();
			if (movieWriter->GetError()) {
				emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
				break;
			}
			emit progress( 100 * (i+1) / (extent[1]-extent[0]));
		}
	}
	else if (m_mode == iASlicerMode::XY)  // XY
	{
		for (int i = extent[4]; i < extent[5]; i++ )
		{
			reslicer->SetResliceAxesOrigin( origin[0], origin[1], origin[2] + i * spacing[2] );
			update();
			w2if->Modified();
			movieWriter->Write();
			if (movieWriter->GetError()) {
				emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
				break;
			}
			emit progress( 100 * (i+1) / (extent[5]-extent[4]));
		}
	}
	else if (m_mode == iASlicerMode::XZ)  // XZ
	{
		for (int i = extent[2]; i < extent[3]; i++ )
		{
			reslicer->SetResliceAxesOrigin( origin[0], origin[1] + i * spacing[1], origin[2]);
			update();
			w2if->Modified();
			movieWriter->Write();
			if (movieWriter->GetError()) {
				emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
				break;
			}
			emit progress( 100 * (i+1) / (extent[3]-extent[2]));
		}
	}

	reslicer->SetResliceAxesOrigin(oldResliceAxesOrigin);
	update();
	movieWriter->End();
	movieWriter->ReleaseDataFlagOn();
	w2if->ReleaseDataFlagOn();

	m_interactor->Enable();

	if (movieWriter->GetError()) emit msg(tr("  MOVIE export failed."));
	else emit msg(tr("MOVIE export completed."));

	return;
}

void iASlicerData::saveAsImage() const
{
	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if (!mdi_parent || m_channels.empty())
		return;
	QString fileName = QFileDialog::getSaveFileName(mdi_parent, tr("Save Image"),
		"", iAIOProvider::GetSupportedImageFormats());
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
	dlg_commoninput dlg(mdi_parent, "Save options", inList, inPara, NULL);
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
	auto reslicer = m_channels[m_channels.keys()[0]]->reslicer;
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

void iASlicerData::saveImageStack()
{
	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if(!mdi_parent)
		return;
	QString file = QFileDialog::getSaveFileName(mdi_parent, tr("Save Image Stack"),
		mdi_parent->getFilePath(), iAIOProvider::GetSupportedImageFormats() );
	if (file.isEmpty())
		return;
	
	QFileInfo fileInfo(file);
	QString baseName = fileInfo.absolutePath() + "/" + fileInfo.baseName();

	// TODO: allow selecting channel to export? export all channels?
	auto imageData = m_channels[m_channels.keys()[0]]->image;
	int const * arr = imageData->GetDimensions();
	double const * spacing = imageData->GetSpacing();

	//Determine index of number of slice in array
	int nums[3] = {0, 2, 1};
	int num = nums[m_mode];

	//Set slice range
	int sliceFirst = 0, sliceLast = arr[num]-1;
	bool saveNative = true;
	bool output16Bit = false;
	QStringList inList = ( QStringList() << tr("$Save native image (intensity rescaled to output format)")
		<< tr("#From Slice Number:")
		<<  tr("#To Slice Number:") );
	QList<QVariant> inPara = ( QList<QVariant>() << (saveNative ? tr("true") : tr("false"))<<tr("%1")
		.arg(sliceFirst) <<tr("%1").arg(sliceLast) );

	if ((QString::compare(fileInfo.suffix(), "TIF", Qt::CaseInsensitive) == 0) ||
		(QString::compare(fileInfo.suffix(), "TIFF", Qt::CaseInsensitive) == 0))
	{
		inList << tr("$16 bit native output (if disabled, native output will be 8 bit)");
		inPara << (output16Bit ? tr("true") : tr("false"));
	}
	dlg_commoninput dlg(mdi_parent, "Save options", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	saveNative = dlg.getCheckValue(0);
	sliceFirst = dlg.getDblValue(1);
	sliceLast  = dlg.getDblValue(2);
	if (inList.size() > 3)
		output16Bit = dlg.getCheckValue(3);

	if(sliceFirst<0 || sliceFirst>sliceLast || sliceLast>arr[num])
	{
		QMessageBox msgBox;
		msgBox.setText("Invalid Input.");
		msgBox.exec();
		return;
	}
	m_interactor->Disable();
	vtkImageData* img;
	double* origin = imageData->GetOrigin();
	// TODO: allow selecting channel to export? export all channels?
	auto reslicer = m_channels[m_channels.keys()[0]]->reslicer;
	for(int slice=sliceFirst; slice<=sliceLast; slice++)
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

void iASlicerData::updatePositionMarkerExtent()
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

void iASlicerData::setStatisticalExtent( int statExt )
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

void iASlicerData::setMode( const iASlicerMode mode )
{
	m_mode = mode;
	updateResliceAxesDirectionCosines();
	updateBackground();
}

void iASlicerData::updateResliceAxesDirectionCosines()
{
	for(auto ch: m_channels)
		ch->updateResliceAxesDirectionCosines( m_mode );
}

void iASlicerData::updateBackground()
{
	if (m_userSetBackground)
	{
		m_ren->SetBackground(m_backgroundRGB);
		return;
	}
	switch(m_mode)
	{
	case iASlicerMode::YZ:
		m_ren->SetBackground(0.2,0.2,0.2); break;
	case iASlicerMode::XY:
		m_ren->SetBackground(0.3,0.3,0.3); break;
	case iASlicerMode::XZ:
		m_ren->SetBackground(0.6,0.6,0.6); break;
	default:
		break;
	}
}

void iASlicerData::setManualBackground(double r, double g, double b)
{
	m_userSetBackground = true;
	m_backgroundRGB[0] = r;
	m_backgroundRGB[1] = g;
	m_backgroundRGB[2] = b;
	updateBackground();
}

void iASlicerData::execute( vtkObject * caller, unsigned long eventId, void * callData )
{
	if (eventId == vtkCommand::LeftButtonPressEvent)
	{
		emit clicked();
	}
	if (eventId == vtkCommand::MouseWheelForwardEvent ||
		eventId == vtkCommand::MouseWheelBackwardEvent)
	{
		emit UserInteraction();
	}
	// Do the pick. It will return a non-zero value if we intersected the image.
	int * epos = m_interactor->GetEventPosition();
	if ( !m_pointPicker->Pick(epos[0], epos[1], 0, m_ren) ) // z is always zero.
	{
		defaultOutput();
		return;
	}

	// Get the mapped position of the mouse using the picker.
	m_pointPicker->GetPickedPositions()->GetPoint(0, m_ptMapped);

	// TODO: how to choose spacing? currently fixed from first image!
	auto imageData = m_channels[m_channels.keys()[0]]->image;
	double* spacing = imageData->GetSpacing();
	m_ptMapped[0] += 0.5*spacing[0];
	m_ptMapped[1] += 0.5*spacing[1];
	m_ptMapped[2] += 0.5*spacing[2];

	switch(eventId)
	{
	case vtkCommand::LeftButtonPressEvent:
	{
		double result[4];
		double x, y, z;
		getMouseCoord(x, y, z, result);
		emit clicked(x, y, z);
		emit UserInteraction();
		break;
	}
	case vtkCommand::LeftButtonReleaseEvent:
	{
		double result[4];
		double x, y, z;
		getMouseCoord(x, y, z, result);
		emit released(x, y, z);
		emit UserInteraction();
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
		emit UserInteraction();
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
			emit Pick( );
		break;
	default:
		if (m_interactor->GetKeyCode() == 'p')
			emit Pick( );
		break;
	}

	m_interactor->Render();
}

void iASlicerData::getMouseCoord(double & xCoord, double & yCoord, double & zCoord, double* result)
{
	result[0] = result[1] = result[2] = result[3] = 0;
	double point[4] = { m_ptMapped[0], m_ptMapped[1], m_ptMapped[2], 1 };
	
	// TODO: find out what "mouseCoord" exactly means - pixel coordinates or image coordinates? differentiate scene coordinates / each images pixel coordinates
	auto imageData = m_channels[m_channels.keys()[0]]->image;
	auto reslicer = m_channels[m_channels.keys()[0]]->reslicer;

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
	xCoord = clamp(static_cast<double>(extent[0]), extent[1]+1-std::numeric_limits<double>::epsilon(), xCoord);
	yCoord = clamp(static_cast<double>(extent[2]), extent[3]+1-std::numeric_limits<double>::epsilon(), yCoord);
	zCoord = clamp(static_cast<double>(extent[4]), extent[5]+1-std::numeric_limits<double>::epsilon(), zCoord);
}

namespace
{
	QString GetSlicerCoordString(int coord1, int coord2, int coord3, int mode)
	{
		switch (mode) {
		default:
		case XY: return QString("%1 %2 %3").arg(coord1).arg(coord2).arg(coord3);
		case XZ: return QString("%1 %2 %3").arg(coord1).arg(coord3).arg(coord2);
		case YZ: return QString("%1 %2 %3").arg(coord3).arg(coord1).arg(coord2);
		}
	}

	QString GetFilePixel(MdiChild* tmpChild, iASlicerData* data, double slicerX, double slicerY, int thirdCoord, int mode)
	{
		// TODO: find out what "mouseCoord" exactly means - pixel coordinates or image coordinates? differentiate scene coordinates / each images pixel coordinates
		auto reslicer = data->getChannel(0)->reslicer;
		vtkImageData* img = reslicer->GetOutput();
		int const * dim = img->GetDimensions();
		bool inRange = slicerX < dim[0] && slicerY < dim[1];
		double tmpPix = 0;
		if (inRange)
		{
			tmpPix = img->GetScalarComponentAsDouble(slicerX, slicerY, 0, 0);
		}
		QString file = tmpChild->getFileInfo().fileName();
		return QString("%1 [%2]\t: %3\n")
			.arg(file)
			.arg(GetSlicerCoordString(slicerX, slicerY, thirdCoord, mode))
			.arg(inRange ? QString::number(tmpPix) : "exceeds img. dim.");
	}
}

void iASlicerData::printVoxelInformation(double xCoord, double yCoord, double zCoord)
{
	if (!m_decorations)
		return;

	// TODO: differentiate scene coordinates / each images pixel coordinates
	auto reslicer = m_channels[m_channels.keys()[0]]->reslicer;
	vtkImageData * reslicerOutput = reslicer->GetOutput();
	double const * const slicerSpacing = reslicerOutput->GetSpacing();
	int const * const slicerExtent = reslicerOutput->GetExtent();
	double const * const slicerBounds = reslicerOutput->GetBounds();

	// We have to manually set the physical z-coordinate which requires us to get the volume spacing.
	m_ptMapped[2] = 0;

	int cX = static_cast<int>(floor((m_ptMapped[0] - slicerBounds[0]) / slicerSpacing[0]));
	int cY = static_cast<int>(floor((m_ptMapped[1] - slicerBounds[2]) / slicerSpacing[1]));

	// check image extent; if outside ==> default output
	if ( cX < slicerExtent[0] || cX > slicerExtent[1] || cY < slicerExtent[2] || cY > slicerExtent[3] ) 
	{
		defaultOutput(); return;
	}

	// get index, coords and value to display
	QString strDetails(QString("index        [ %1, %2, %3 ]\n")
		.arg(static_cast<int>(xCoord)).arg(static_cast<int>(yCoord)).arg(static_cast<int>(zCoord)));

	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if (mdi_parent)
	{
		for (int m=0; m < mdi_parent->getModalities()->size(); ++m)
		{
			auto mod = mdi_parent->getModality(m);
			strDetails += PadOrTruncate(mod->GetName(), 12)+" ";
			for (int c = 0; c < mod->ComponentCount(); ++c)
			{
				strDetails += "[ ";
				auto img = mod->GetComponent(c);
				for (int i = 0; i < img->GetNumberOfScalarComponents(); i++)
				{
					// TODO: check muliple componets + MDIChild Linked Views
					// TODO: limit slab thickness
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
						strDetails += " ";
					strDetails += QString::number(value);
				}
				strDetails += " ] ";
			}
			strDetails += "\n";
		}
		if (mdi_parent->getLinkedMDIs())
		{
			QList<MdiChild*> mdiwindows = mdi_parent->getMainWnd()->mdiChildList();
			for (int i = 0; i < mdiwindows.size(); i++)
			{
				MdiChild *tmpChild = mdiwindows.at(i);
				if (tmpChild != mdi_parent) {
					double * const tmpSpacing = tmpChild->getImagePointer()->GetSpacing();
					// TODO: check which spacing makes sense here!
					auto imageData = m_channels[m_channels.keys()[0]]->image;
					double const * const origImgSpacing = imageData->GetSpacing();
					int tmpX = xCoord * origImgSpacing[0] / tmpSpacing[0];
					int tmpY = yCoord * origImgSpacing[1] / tmpSpacing[1];
					int tmpZ = zCoord * origImgSpacing[2] / tmpSpacing[2];
					switch (m_mode)
					{
					case iASlicerMode::XY://XY
						tmpChild->getSlicerDataXY()->setPositionMarkerCenter(tmpX * tmpSpacing[0], tmpY * tmpSpacing[1]);
						tmpChild->getSlicerXY()->setIndex(tmpX, tmpY, tmpZ);
						tmpChild->getSlicerDlgXY()->spinBoxXY->setValue(tmpZ);

						tmpChild->getSlicerDataXY()->update();
						tmpChild->getSlicerXY()->update();
						tmpChild->getSlicerDlgYZ()->update();

						strDetails += GetFilePixel(tmpChild, tmpChild->getSlicerDataXY(), tmpX, tmpY, tmpZ, m_mode);
						break;
					case iASlicerMode::YZ://YZ
						tmpChild->getSlicerDataYZ()->setPositionMarkerCenter(tmpY * tmpSpacing[1], tmpZ * tmpSpacing[2]);
						tmpChild->getSlicerYZ()->setIndex(tmpX, tmpY, tmpZ);
						tmpChild->getSlicerDlgYZ()->spinBoxYZ->setValue(tmpX);

						tmpChild->getSlicerDataYZ()->update();
						tmpChild->getSlicerYZ()->update();
						tmpChild->getSlicerDlgYZ()->update();

						strDetails += GetFilePixel(tmpChild, tmpChild->getSlicerDataYZ(), tmpY, tmpZ, tmpX, m_mode);
						break;
					case iASlicerMode::XZ://XZ
						tmpChild->getSlicerDataXZ()->setPositionMarkerCenter(tmpX * tmpSpacing[0], tmpZ * tmpSpacing[2]);
						tmpChild->getSlicerXZ()->setIndex(tmpX, tmpY, tmpZ);
						tmpChild->getSlicerDlgXZ()->spinBoxXZ->setValue(tmpY);

						tmpChild->getSlicerDataXZ()->update();
						tmpChild->getSlicerXZ()->update();
						tmpChild->getSlicerDlgXZ()->update();

						strDetails += GetFilePixel(tmpChild, tmpChild->getSlicerDataXZ(), tmpX, tmpZ, tmpY, m_mode);
						break;
					default://ERROR
						break;
					}
					tmpChild->update();
				}
			}
		}
	}

	// if requested calculate distance and show actor
	if (pLineActor->GetVisibility())
	{
		double distance = sqrt(	pow((m_startMeasurePoint[0] - m_ptMapped[0]), 2) +
			pow((m_startMeasurePoint[1] - m_ptMapped[1]), 2) );
		pLineSource->SetPoint2(m_ptMapped[0]-(0.5*slicerSpacing[0]), m_ptMapped[1] - (0.5*slicerSpacing[1]), 0.0);
		pDiskSource->SetOuterRadius(distance);
		pDiskSource->SetInnerRadius(distance);
		strDetails += QString("distance [ %1 ]\n").arg(distance);

	}

	// Update the info text with pixel coordinates/value if requested.
	m_textInfo->GetActor()->SetPosition(m_interactor->GetEventPosition()[0]+10, m_interactor->GetEventPosition()[1]+10);
	m_textInfo->GetTextMapper()->SetInput(strDetails.toStdString().c_str());
	m_positionMarkerMapper->Update();
}

/*
void iASlicerData::setMeasurementStartPoint(int x, int y)
{
	m_measureStart[0] = x;
	m_measureStart[1] = y;
}
*/

void iASlicerData::executeKeyPressEvent()
{
	switch (m_interactor->GetKeyCode())
	{
	case 'm':
		m_startMeasurePoint[0] = m_ptMapped[0];
		m_startMeasurePoint[1] = m_ptMapped[1];
		// does not work reliably (often snaps to positions not the highest gradient close to the current position)
		// and causes access to pixels outside of the image:
		//snapToHighGradient(m_startMeasurePoint[0], m_startMeasurePoint[1]);

		if (m_decorations && pLineSource != NULL)
		{
			// TODO: check which reslicer makes sense here!
			auto reslicer = m_channels[m_channels.keys()[0]]->reslicer;
			double * slicerSpacing = reslicer->GetOutput()->GetSpacing();
			pLineSource->SetPoint1(m_startMeasurePoint[0]-(0.5*slicerSpacing[0]), m_startMeasurePoint[1]-(0.5*slicerSpacing[1]), 0.0);
			pDiskActor->SetPosition(m_startMeasurePoint[0]-(0.5*slicerSpacing[0]), m_startMeasurePoint[1]-(0.5*slicerSpacing[1]), 1.0);
			pLineActor->SetVisibility(true);
			pDiskActor->SetVisibility(true);
			double result[4];
			double xCoord, yCoord, zCoord;
			getMouseCoord(xCoord, yCoord, zCoord, result);
			printVoxelInformation(xCoord, yCoord, zCoord);
		}
		break;
	case 27: //ESCAPE
		if (m_decorations && pLineActor != NULL){
			pLineActor->SetVisibility(false);
			pDiskActor->SetVisibility(false);
		}
		break;
	}
}


void iASlicerData::defaultOutput()
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
void iASlicerData::snapToHighGradient(double &x, double &y)
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

void iASlicerData::updateReslicer()
{
	for(auto ch: m_channels)
		ch->updateReslicer();
}

void iASlicerData::setShowText( bool isVisible )
{
	if (!m_decorations)
		return;
	m_textInfo->Show(isVisible);
}

void iASlicerData::enableChannel(uint id, bool enabled, double x, double y, double z)
{
	if (enabled)
		setResliceChannelAxesOrigin(id, x, y, z);
	enableChannel( id, enabled );
}

void iASlicerData::enableChannel( uint id, bool enabled )
{
	// TODO: move into channeldata!
	if( enabled )
	{
		m_ren->AddActor(getChannel(id)->imageActor );
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

void iASlicerData::updateChannel( uint id, iAChannelData const & chData )
{
	getChannel(id)->reInit(chData);
}

void iASlicerData::addChannel( uint id, iAChannelData const & chData )
{
	assert(!m_channels.contains(id));
	// TODO: Which color transfer function to use in scalar bar widget?
	m_scalarBarWidget->GetScalarBarActor()->SetLookupTable(chData.getCTF());
	bool updateSpacing = m_channels.empty();
	auto chSlicerData = createChannel(id);
	chSlicerData->init(chData, m_mode);
	double curTol = m_pointPicker->GetTolerance();
	double newTol = chData.getImage()->GetSpacing()[0] / 3;
	if (newTol < curTol)
		m_pointPicker->SetTolerance(newTol);
	if (updateSpacing)
	{
		updatePositionMarkerExtent();
		// TODO: update required for new channels other than to export? export all channels?
		auto imageData = m_channels[id]->image;
		auto reslicer = m_channels[id]->reslicer;
		double const * const imgSpc = imageData->GetSpacing();
		double unitSpacing = std::max(std::max(imgSpc[0], imgSpc[1]), imgSpc[2]);
		double const * const spc = reslicer->GetOutput()->GetSpacing();
		int    const * const dim = reslicer->GetOutput()->GetDimensions();
		for (int i=0; i<2; ++i)
			// scaling required to shrink the text to required size (because of large font size, see initialize method)
			m_axisTransform[i]->Scale(unitSpacing / 10, unitSpacing / 10, unitSpacing / 10);
		double xHalf = (dim[0] - 1) * spc[0] / 2.0;
		double yHalf = (dim[1] - 1) * spc[1] / 2.0;
		// "* 10 / unitSpacing" adjusts for scaling (see above)
		m_axisTextActor[0]->SetPosition(xHalf * 10 / unitSpacing, -20.0, 0);
		m_axisTextActor[1]->SetPosition(-20.0, yHalf * 10 / unitSpacing, 0);
	}
}

void iASlicerData::setResliceChannelAxesOrigin( uint id, double x, double y, double z )
{
	getChannel(id)->setResliceAxesOrigin(x, y, z);
}

void iASlicerData::setChannelOpacity(uint id, double opacity )
{
	 getChannel(id)->imageActor->SetOpacity(opacity);
}

void iASlicerData::setSliceNumber( int sliceNumber )
{
	m_sliceNumber = sliceNumber;
	double xyz[3] = {0.0, 0.0, 0.0};
	switch(m_mode)
	{
	case iASlicerMode::XY://XY
		xyz[2] = sliceNumber;
		break;
	case iASlicerMode::YZ://YZ
		xyz[0] = sliceNumber;
		break;
	case iASlicerMode::XZ://XZ
		xyz[1] = sliceNumber;
		break;
	default://ERROR
		break;
	}
	if (m_roiActive)
		m_roiActor->SetVisibility(m_roiSlice[0] <= m_sliceNumber && m_sliceNumber < (m_roiSlice[1]));
	if (m_channels.empty())
		return;
	double * spacing = m_channels[m_channels.keys()[0]]->image->GetSpacing();
	double * origin = m_channels[m_channels.keys()[0]]->image->GetOrigin();
	//also apply to enabled channels
	for(auto ch: m_channels)
		ch->setResliceAxesOrigin( origin[0] + xyz[0] * spacing[0], origin[1] + xyz[1] * spacing[1], origin[2] + xyz[2] * spacing[2] );
}

void iASlicerData::setSlabThickness(int thickness)
{
	m_slabThickness = thickness;
	for (auto ch : m_channels)
		ch->reslicer->SetSlabNumberOfSlices(thickness);
	update();
}

void iASlicerData::setSlabCompositeMode(int slabCompositeMode)
{
	m_slabCompositeMode = slabCompositeMode;
	for (auto ch : m_channels)
		ch->reslicer->SetSlabMode(slabCompositeMode);
	update();
}

QSharedPointer<iAChannelSlicerData> iASlicerData::createChannel(uint id)
{
	if (m_channels.contains(id))
		throw std::runtime_error(QString("iASlicerData: Channel with ID %1 already exists!").arg(id).toStdString());

	QSharedPointer<iAChannelSlicerData> newData(new iAChannelSlicerData);
	newData->imageActor->SetInterpolate(m_settings.LinearInterpolation);
	newData->reslicer->SetSlabNumberOfSlices(m_slabThickness);
	newData->reslicer->SetSlabMode(m_slabCompositeMode);
	newData->setTransform(m_transform);
	m_channels.insert(id, newData);
	return newData;
}

iAChannelSlicerData * iASlicerData::getChannel(uint id)
{
	if (!m_channels.contains(id))
		return nullptr;
	return m_channels.find(id)->data();
}

void iASlicerData::removeChannel(uint id)
{
	m_channels.remove(id);
}

size_t iASlicerData::GetEnabledChannels()
{
	return m_channels.size();
}

void iASlicerData::setMagicLensInput(uint id)
{
	iAChannelSlicerData * data = getChannel(id);
	assert ( data );
	if (!data)
	{
		return;
	}
	if (m_magicLensExternal)
	{
		m_magicLensExternal->AddInput(data->reslicer, data->getColorTransferFunction(), data->getName());
	}
}

vtkGenericOpenGLRenderWindow* iASlicerData::getRenderWindow()
{
	return m_renWin.GetPointer();
}

vtkRenderWindowInteractor* iASlicerData::getInteractor()
{
	return m_interactor;
}

vtkRenderer* iASlicerData::getRenderer()
{
	return m_ren.GetPointer();
}

vtkCamera* iASlicerData::getCamera()
{
	return m_camera;
}

void iASlicerData::setCamera(vtkCamera* cam, bool camOwner /*=true*/)
{
	m_ren->SetActiveCamera(cam);
	if (m_camera && m_cameraOwner)
	{
		m_camera->Delete();
	}
	m_cameraOwner = camOwner;
	m_camera = cam;
}

void iASlicerData::resetCamera()
{
	m_ren->ResetCamera();
}

void iASlicerData::updateChannelMappers()
{
	for (uint key: m_channels.keys())
	{
		iAChannelSlicerData * chData = m_channels.value(key).data();
		chData->updateLUT();
		chData->updateMapper();
	}
}

void iASlicerData::rotateSlice( double angle )
{
	switch( m_mode )
	{
		case iASlicerMode::XY://XY
			m_angleZ = angle;
			break;
		case iASlicerMode::YZ://YZ
			m_angleX = angle;
			break;
		case iASlicerMode::XZ://XZ
			m_angleY = angle;
			break;
		default://ERROR
			break;
	}

	double center[3];

	// TODO: allow selecting center for rotation? current: always use first image!
	auto imageData = m_channels[m_channels.keys()[0]]->image;
	double* spacing = imageData->GetSpacing();
	int* ext = imageData->GetExtent();

	center[0] = (ext[1]-ext[0])/2 * spacing[0];
	center[1] = (ext[3]-ext[2])/2 * spacing[1];
	center[2] = (ext[5]-ext[4])/2 * spacing[2];

	vtkTransform *t1 = vtkTransform::New();
	t1->Translate(-center[0], -center[1], -center[2]);

	vtkTransform *t2 = vtkTransform::New();
	t2->RotateWXYZ(m_angleX, 1, 0, 0);
	t2->RotateWXYZ(m_angleY, 0, 1, 0);
	t2->RotateWXYZ(m_angleZ, 0, 0, 1);

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
}

/*
void iASlicerData::switchContourSourceToChannel(uint id )
{
	if (!m_decorations)
		return;

	cFilter->SetInputConnection( getChannel(id)->reslicer->GetOutputPort() );
}
*/

void iASlicerData::setContours(int numberOfContours, double contourMin, double contourMax)
{
	if (!m_decorations)
		return;
	for (auto ch: m_channels)
		ch->cFilter->GenerateValues(numberOfContours, contourMin, contourMax);
}

void iASlicerData::setContours( int n, double * contourValues )
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

void iASlicerData::setMouseCursor( QString s )
{
	QString color = s.section( ' ', -1 );
	if ( color != "default" )
	{
		QString shape = s.section( ' ', 0, 0 );
		QPixmap pm;
		if ( shape == "Crosshair" )
			pm = QPixmap( ":/images/" + s.section(' ', 0, 1) + ".png" );
		QPixmap cpm( pm.size() );
		cpm.fill( color );
		cpm.setMask( pm.createMaskFromColor( Qt::transparent ) );
		m_mouseCursor = QCursor( cpm );
	}
	else
	{
		m_mouseCursor = QCursor( Qt::CrossCursor );
	}
	emit updateSignal();
}

vtkScalarBarWidget * iASlicerData::getScalarBarWidget()
{
	assert(m_decorations);
	return m_scalarBarWidget;
}

QCursor iASlicerData::getMouseCursor()
{
	return m_mouseCursor;
}

int iASlicerData::getSliceNumber() const
{
	return m_sliceNumber;
}

void iASlicerData::setRightButtonDragZoomEnabled(bool enabled)
{
	m_interactorStyle->SetRightButtonDragZoomEnabled(enabled);
}
