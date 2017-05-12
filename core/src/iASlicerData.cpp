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
#include "iASlicerData.h"

#include "dlg_commoninput.h"
#include "iAIOProvider.h"
#include "iAMagicLens.h"
#include "iAMathUtility.h"
#include "iAMovieHelper.h"
#include "iAPieChartGlyph.h"
#include "iARulerWidget.h"
#include "iARulerRepresentation.h"
#include "iASlicer.h"
#include "iASlicerSettings.h"
#include "iAToolsVTK.h"
#include "mdichild.h"

#include <vtkAlgorithmOutput.h>
#include <vtkAxisActor2D.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataSetMapper.h>
#include <vtkDiskSource.h>
#include <vtkGenericMovieWriter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageBlend.h>
#include <vtkImageCast.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageMapper3D.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkImageReslice.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLineSource.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
#include <vtkLookupTable.h>
#include <vtkMarchingContourFilter.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointPicker.h>
#include <vtkProperty.h>
#include <vtkQImageToImageSource.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarBarWidget.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor3D.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkVersion.h>
#include <vtkWindowToImageFilter.h>
#include <vtkCommand.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QBitmap>

#include <string>
#include <sstream>

namespace
{
	const double PickTolerance = 100.0;
}

class iAInteractorStyleImage : public vtkInteractorStyleImage
{
public:
	static iAInteractorStyleImage *New();
	vtkTypeMacro(iAInteractorStyleImage, vtkInteractorStyleImage);

	virtual void OnLeftButtonDown()
	{
		// disable "window-level" and rotation interaction
		if (!this->Interactor->GetShiftKey())
		{
			return;
		}
		vtkInteractorStyleImage::OnLeftButtonDown();
	}
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
};

vtkStandardNewMacro(iAInteractorStyleImage);


iASlicerData::iASlicerData( iASlicer const * slicerMaster, QObject * parent /*= 0 */,
		bool decorations/*=true*/) : QObject( parent ),
	m_mode(slicerMaster->m_mode),
	m_magicLensExternal(slicerMaster->magicLens()),
	renWin( vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New() ),
	ren( vtkSmartPointer<vtkRenderer>::New() ),
	imageData( 0 ),
	angleX( 0 ), angleY( 0 ), angleZ( 0 ),
	m_userSetBackground(false),
	m_decorations(decorations),
	m_cameraOwner(true),
	scalarWidget(0),
	textProperty(0),
	cFilter(0),
	cMapper(0),
	cActor(0),
	textInfo(0),
	rulerWidget(0),
	interactor(0),
	m_showPositionMarker(false)
{
	renWin->AlphaBitPlanesOn();
	renWin->LineSmoothingOn();
	renWin->PointSmoothingOn();
	renWin->PolygonSmoothingOn();
	interactorStyle = iAInteractorStyleImage::New();
	m_camera = vtkCamera::New();

	reslicer = vtkImageReslice::New();
	colormapper = vtkImageMapToColors::New();
	imageActor = vtkImageActor::New();

	pointPicker = vtkPointPicker::New();

	if (decorations)
	{
		scalarWidget = vtkScalarBarWidget::New();
		textProperty = vtkTextProperty::New();
		logoWidget = vtkSmartPointer<vtkLogoWidget>::New();
		logoRep = vtkSmartPointer<vtkLogoRepresentation>::New();
		logoImage = vtkSmartPointer<vtkQImageToImageSource>::New();

		m_positionMarkerSrc = vtkSmartPointer<vtkPlaneSource>::New();
		m_positionMarkerMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_positionMarkerActor = vtkSmartPointer<vtkActor>::New();

		cFilter = vtkMarchingContourFilter::New();
		cMapper = vtkPolyDataMapper::New();
		cActor = vtkActor::New();

		pLineSource = vtkSmartPointer<vtkLineSource>::New();
		pLineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		pLineActor = vtkSmartPointer<vtkActor>::New();
		pDiskSource = vtkSmartPointer<vtkDiskSource>::New();
		pDiskMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		pDiskActor = vtkSmartPointer<vtkActor>::New();
		
		roiSource = vtkSmartPointer<vtkPlaneSource>::New();
		roiMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		roiActor = vtkSmartPointer<vtkActor>::New();

		for (int i = 0; i < 2; ++i)
		{
			axisTransform[i] = vtkSmartPointer<vtkTransform>::New();
			axisTextActor[i] = vtkSmartPointer<vtkTextActor3D>::New();
		}
		
		textInfo = iAWrapperText::New();
		
		rulerWidget = iARulerWidget::New();
	}
	transform = vtkTransform::New();
}


iASlicerData::~iASlicerData(void)
{
	disconnect();

	colormapper->Delete();
	imageActor->Delete();

	reslicer->ReleaseDataFlagOn();
	reslicer->Delete();
	interactorStyle->Delete();
	
	pointPicker->Delete();

	//scaleActor->Delete();
	if (m_decorations)
	{
		scalarWidget->Delete();
		textInfo->Delete();	

		textProperty->Delete();

		cFilter->Delete();
		cMapper->Delete();
		cActor->Delete();

		rulerWidget->Delete();
	}

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
		m_redirect->Execute(caller, eventId, callData);
	}
	iASlicerData* m_redirect;
};

void iASlicerData::initialize( vtkImageData *ds, vtkTransform *tr, vtkColorTransferFunction *ctf,
	bool showIsoLines, bool showPolygon)
{
	imageData = ds;
	transform = tr;
	colorTransferFunction = ctf;
	isolines = showIsoLines;
	poly = showPolygon;

	renWin->AddRenderer(ren);
	interactor = renWin->GetInteractor();
	interactor->SetInteractorStyle(interactorStyle);
	interactor->SetPicker(pointPicker);
	interactor->Initialize( );

	iAObserverRedirect* redirect(new iAObserverRedirect(this));
	interactor->AddObserver(vtkCommand::LeftButtonPressEvent, redirect);
	interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, redirect);
	interactor->AddObserver(vtkCommand::RightButtonPressEvent, redirect);
	interactor->AddObserver(vtkCommand::MouseMoveEvent, redirect);
	interactor->AddObserver(vtkCommand::KeyPressEvent, redirect);
	interactor->AddObserver(vtkCommand::KeyReleaseEvent, redirect);
	interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, redirect);
	interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, redirect);

	InitReslicerWithImageData();
	
	UpdateResliceAxesDirectionCosines();
	UpdateBackground();
	pointPicker->SetTolerance(PickTolerance);

	if (m_decorations)
	{
		textInfo->AddToScene(ren);
		textInfo->SetText(" ");
		textInfo->SetPosition(iAWrapperText::POS_LOWER_LEFT);
		textInfo->Show(1);

		roiSource->SetOrigin(0, 0, 0);
		roiSource->SetPoint1(-3, 0, 0);
		roiSource->SetPoint2(0, -3, 0);
		roi = NULL;

		QImage img;
		if( QDate::currentDate().dayOfYear() >= 340 )img.load(":/images/Xmas.png");
		else img.load(":/images/fhlogo.png");
		logoImage->SetQImage(&img);
		logoImage->Update();
		logoRep->SetImage(logoImage->GetOutput( ));
		logoWidget->SetInteractor( interactor );
		logoWidget->SetRepresentation(logoRep);
		logoWidget->SetResizable(false);
		logoWidget->SetSelectable(true);
		logoWidget->On();

		textProperty->SetBold(1);
		textProperty->SetItalic(1);
		textProperty->SetColor(1, 1, 1);
		textProperty->SetJustification(VTK_TEXT_CENTERED);
		textProperty->SetVerticalJustification(VTK_TEXT_CENTERED);
		textProperty->SetOrientation(1);
		scalarWidget->SetInteractor(interactor);
		scalarWidget->GetScalarBarActor()->SetLookupTable( colorTransferFunction );
		scalarWidget->GetScalarBarActor()->SetLabelFormat("%.2f");
		scalarWidget->GetScalarBarActor()->SetTitleTextProperty(textProperty);
		scalarWidget->GetScalarBarActor()->SetLabelTextProperty(textProperty);
		scalarWidget->SetEnabled( true );
		scalarWidget->SetRepositionable( true );
		scalarWidget->SetResizable( true );
		scalarWidget->GetScalarBarRepresentation()->SetOrientation(1);
		scalarWidget->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue(0.92,0.2);
		scalarWidget->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue(0.06, 0.75);
		scalarWidget->GetScalarBarActor()->SetTitle("Range");

		UpdatePositionMarkerExtent();
		m_positionMarkerMapper->SetInputConnection( m_positionMarkerSrc->GetOutputPort() );
		m_positionMarkerActor->SetMapper( m_positionMarkerMapper );
		m_positionMarkerActor->GetProperty()->SetColor( 0,1,0 );
		m_positionMarkerActor->GetProperty()->SetOpacity( 1 );
		m_positionMarkerActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);
		m_positionMarkerActor->SetVisibility(false);

		cFilter->SetInputConnection( reslicer->GetOutputPort() );
		cFilter->UseScalarTreeOn( );
		cFilter->SetComputeGradients( false );
		cFilter->SetComputeNormals( false );
		cFilter->GenerateValues( 0, 0, 0 );
		cMapper->SetInputConnection( cFilter->GetOutputPort() );

		cMapper->SetResolveCoincidentTopology( VTK_RESOLVE_POLYGON_OFFSET );
		cActor->SetMapper( cMapper );
		cActor->SetVisibility( isolines );

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

		roiMapper->SetInputConnection( roiSource->GetOutputPort() );
		roiActor->SetVisibility( false );
		roiActor->SetMapper( roiMapper );
		roiActor->GetProperty()->SetColor( 1, 0, 0 );
		roiActor->GetProperty()->SetOpacity( 1 );
		roiSource->SetCenter( 0, 0, 1 );
		roiMapper->Update( );
		roiActor->GetProperty()->SetRepresentation( VTK_WIREFRAME );
		
		reslicer->Update();
		double const * const imgSpc = imageData->GetSpacing();
		double unitSpacing = std::max(std::max(imgSpc[0], imgSpc[1]), imgSpc[2]);
		double const * const spc = reslicer->GetOutput()->GetSpacing();
		int    const * const dim = reslicer->GetOutput()->GetDimensions();

		axisTextActor[0]->SetInput((m_mode == XY || m_mode == XZ) ? "X" : "Y");
		axisTextActor[1]->SetInput((m_mode == XZ || m_mode == YZ) ? "Z" : "Y");

		for (int i = 0; i < 2; ++i)
		{
			axisTextActor[i]->SetPickable(false);
			// large font size required to make the font nicely smooth
			axisTextActor[i]->GetTextProperty()->SetFontSize(100);
			axisTextActor[i]->GetTextProperty()->SetFontFamilyToArial();
			axisTextActor[i]->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
			ren->AddActor(axisTextActor[i]);
			axisTextActor[i]->SetVisibility(false);
			// scaling required to shrink the text to required size (because of large font size, see above)
			axisTransform[i]->Scale(unitSpacing / 10, unitSpacing / 10, unitSpacing / 10);
			axisTextActor[i]->SetUserTransform(axisTransform[i]);
		}
		double xHalf = (dim[0] - 1) * spc[0] / 2.0;
		double yHalf = (dim[1] - 1) * spc[1] / 2.0;
		// "* 10 / unitSpacing" adjusts for scaling (see above)
		axisTextActor[0]->SetPosition(xHalf * 10 / unitSpacing, -20.0, 0);
		axisTextActor[0]->GetTextProperty()->SetVerticalJustificationToTop();
		axisTextActor[0]->GetTextProperty()->SetJustificationToCentered();
		axisTextActor[1]->SetPosition(-20.0, yHalf * 10 / unitSpacing, 0);
		axisTextActor[1]->GetTextProperty()->SetVerticalJustificationToCentered();
		axisTextActor[1]->GetTextProperty()->SetJustificationToRight();
		
		rulerWidget->SetInteractor(interactor);
		rulerWidget->SetEnabled( true );
		rulerWidget->SetRepositionable( true );
		rulerWidget->SetResizable( true );
		rulerWidget->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue(0.333,0.05);
		rulerWidget->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue(0.333,0.051);
		
		ren->AddActor(m_positionMarkerActor);
		ren->AddActor(cActor);
		ren->AddActor(pLineActor);
		ren->AddActor(pDiskActor);
		ren->AddActor(roiActor);
	}

	colormapper->SetLookupTable(colorTransferFunction);

	if ( imageData->GetNumberOfScalarComponents() > 1 )
	{
		colormapper->SetLookupTable( 0 );
	}

	colormapper->SetInputConnection(reslicer->GetOutputPort());
	colormapper->Update();
	imageActor->SetInputData(colormapper->GetOutput());
	imageActor->GetMapper()->BorderOn();
/*
	// ORIENTATION / ROTATION FIX:
	// make orientation the same as in other image viewers:
	double orientation[3] = {
		180,
		0,
		0
	};
	imageActor->SetOrientation(orientation);
*/
	ren->AddActor(imageActor);

	ren->SetActiveCamera(m_camera);
	ren->ResetCamera();
}

void iASlicerData::AddImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	ren->AddActor(imgActor);
}

void iASlicerData::RemoveImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	ren->RemoveActor(imgActor);
}

void iASlicerData::blend(vtkAlgorithmOutput *data, vtkAlgorithmOutput *data2, double opacity, double * range)
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


void iASlicerData::reInitialize( vtkImageData *ds, vtkTransform *tr, vtkColorTransferFunction* ctf,
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
		scalarWidget->GetScalarBarActor()->SetLookupTable( colorTransferFunction );
	}

	colormapper->SetLookupTable(colorTransferFunction);

	if ( imageData->GetNumberOfScalarComponents() > 1 )
	{
		colormapper->SetLookupTable( 0 );
	}

	colormapper->Update();
}


void iASlicerData::updateROI()
{
	if (!m_decorations)
	{
		return;
	}
	if (roi != NULL && roiActor->GetVisibility())
	{
		double* spacing = reslicer->GetOutput()->GetSpacing();

		// apparently, image actor starts output at -0,5spacing, -0.5spacing (probably a side effect of BorderOn)
		// That's why we have to subtract 0.5 from the coordinates!
		if (m_mode == iASlicerMode::YZ)
		{
			roiSource->SetOrigin((roi[1]-0.5)*spacing[1]                  , (roi[2]-0.5)*spacing[2]                  , 0);
			roiSource->SetPoint1((roi[1]-0.5)*spacing[1]+roi[4]*spacing[0], (roi[2]-0.5)*spacing[2]                  , 0);
			roiSource->SetPoint2((roi[1]-0.5)*spacing[1]                  , (roi[2]-0.5)*spacing[2]+roi[5]*spacing[2], 0);
		}
		else if (m_mode == iASlicerMode::XY)
		{
			roiSource->SetOrigin((roi[0]-0.5)*spacing[0]                  , (roi[1]-0.5)*spacing[1]                  , 0);
			roiSource->SetPoint1((roi[0]-0.5)*spacing[0]+roi[3]*spacing[0], (roi[1]-0.5)*spacing[1]                  , 0);
			roiSource->SetPoint2((roi[0]-0.5)*spacing[0]                  , (roi[1]-0.5)*spacing[1]+roi[4]*spacing[1], 0);
		}
		else if (m_mode == iASlicerMode::XZ)
		{
			roiSource->SetOrigin((roi[0]-0.5)*spacing[0]                  , (roi[2]-0.5)*spacing[2]                  , 0);
			roiSource->SetPoint1((roi[0]-0.5)*spacing[0]+roi[3]*spacing[0], (roi[2]-0.5)*spacing[2]                  , 0);
			roiSource->SetPoint2((roi[0]-0.5)*spacing[0]                  , (roi[2]-0.5)*spacing[2]+roi[5]*spacing[2], 0);
		}

		roiMapper->Update();
		interactor->Render();
	}
}


void iASlicerData::SetROIVisibility(bool visible)
{
	roiActor->SetVisibility(visible);
}


void iASlicerData::setup(iASingleSlicerSettings const & settings)
{
	imageActor->SetInterpolate(settings.LinearInterpolation);
	QList<iAChannelID> idList = m_channels.keys();
	for (QList<iAChannelID>::const_iterator it = idList.begin(); it != idList.end(); ++it)
	{
		m_channels[*it]->imageActor->SetInterpolate(settings.LinearInterpolation);
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
		axisTextActor[0]->SetVisibility(settings.ShowAxesCaption);
		axisTextActor[1]->SetVisibility(settings.ShowAxesCaption);
	}
}


void iASlicerData::setResliceAxesOrigin( double x, double y, double z )
{
	if (interactor->GetEnabled())
	{
		reslicer->SetResliceAxesOrigin( x, y, z );
		UpdateReslicer();
		interactor->Render(); 
	}
}


void iASlicerData::update()
{
	if( !imageData )
		return;
	
	colormapper->Update();
	foreach( QSharedPointer<iAChannelSlicerData> ch, m_channels )
		ch->updateMapper();
	UpdateReslicer();
	reslicer->UpdateWholeExtent();
	interactor->ReInitialize();
	interactor->Render();
	ren->Render();


	emit updateSignal();
}


void iASlicerData::setPositionMarkerCenter(double x, double y)
{
	if (!m_decorations)
	{
		return;
	}
	if (interactor->GetEnabled() && m_showPositionMarker)
	{
		m_positionMarkerActor->SetVisibility(true);
		m_positionMarkerSrc->SetCenter(x, y, 0);
		m_positionMarkerMapper->Update();
		update();
	}
};


void iASlicerData::showIsolines(bool s) 
{
	if (!m_decorations)
	{
		return;
	}
	cActor->SetVisibility(s); 
}


void iASlicerData::showPosition(bool s) 
{
	if (!m_decorations)
	{
		return;
	}
	m_showPositionMarker = s;
}


void iASlicerData::saveMovie( QString& fileName, int qual /*= 2*/ )
{ 
	QString movie_file_types = GetAvailableMovieFormats();

	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if(!mdi_parent)
		return;
	// If VTK was built without video support, display error message and quit.
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(mdi_parent, "Movie Export", "Sorry, but movie export support is disabled.");
		return;
	}

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
	double* spacing = imageData->GetSpacing();

	emit msg(tr("%1  MOVIE export started. Output: %2").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat), fileName));

	if (m_mode == iASlicerMode::YZ)      // YZ
	{					
		for ( i = extent[0]; i < extent[1]; i++ )
		{
			reslicer->SetResliceAxesOrigin( i * spacing[0], 0, 0 ); 
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
		for ( i = extent[4]; i < extent[5]; i++ )
		{
			reslicer->SetResliceAxesOrigin( 0, 0, i * spacing[2] ); 
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
		for ( i = extent[2]; i < extent[3]; i++ )
		{
			reslicer->SetResliceAxesOrigin( 0, i * spacing[1], 0 ); 
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

	movieWriter->End(); 
	movieWriter->ReleaseDataFlagOn();
	w2if->ReleaseDataFlagOn();

	interactor->Enable();

	if (movieWriter->GetError()) emit msg(tr("  MOVIE export failed."));
	else emit msg(tr("  MOVIE export completed."));

	return;
}


void iASlicerData::changeImageData( vtkImageData *idata )
{
	imageData = idata;

	reslicer->SetInputData( imageData );
	reslicer->SetInformationInput( imageData );

	pointPicker->SetTolerance(PickTolerance);

	if (m_decorations)
	{
		cFilter->SetInputConnection(reslicer->GetOutputPort());
		cMapper->SetInputConnection( cFilter->GetOutputPort() );
		cActor->SetMapper( cMapper );
	}

	interactor->Initialize( );
}


void iASlicerData::saveAsImage() const
{
	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if (!mdi_parent)
		return;
	QString fileName = QFileDialog::getSaveFileName(mdi_parent, tr("Save Image"),
		"", iAIOProvider::GetSupportedImageFormats());
	if (fileName.isEmpty())
		return;
	vtkImageData *img = reslicer->GetOutput();
	bool saveNative = false;
	if (img->GetScalarType() != VTK_DOUBLE &&
		img->GetScalarType() != VTK_FLOAT)
	{
		QStringList inList = (QStringList() << tr("$Save native image"));
		QList<QVariant> inPara = (QList<QVariant>() << (saveNative ? tr("true") : tr("false")));
		dlg_commoninput *dlg = new dlg_commoninput(mdi_parent, "Save options", 1, inList, inPara, NULL);
		if (dlg->exec() != QDialog::Accepted)
		{
			return;
		}
		saveNative = dlg->getCheckValues()[0];
	}
	vtkSmartPointer<vtkImageCast> castfilter = vtkSmartPointer<vtkImageCast>::New();
	vtkSmartPointer<vtkWindowToImageFilter> wtif = vtkSmartPointer<vtkWindowToImageFilter>::New();
	if (saveNative)
	{
		QFileInfo fi(fileName);
		if ((QString::compare(fi.suffix(), "TIF", Qt::CaseInsensitive) != 0) &&
			(QString::compare(fi.suffix(), "TIFF", Qt::CaseInsensitive) != 0))
		{ // TODO: rescale intensity
			castfilter->SetInputData(reslicer->GetOutput());
			castfilter->SetOutputScalarTypeToUnsignedChar();
			castfilter->Update();
			img = castfilter->GetOutput();
		}
	}
	else
	{
		wtif->SetInput(renWin);
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
		"", iAIOProvider::GetSupportedImageFormats() );
	if (file.isEmpty())
		return;

	std::string current_file = file.toLocal8Bit().constData();
	size_t dotpos = current_file.find_last_of(".");
	assert( dotpos != std::string::npos );
	if ( dotpos == std::string::npos )
	{
		return;
	}
	current_file.erase(dotpos,4);

	//Get number of slice at each axis
	int arr[3]; 
	imageData->GetDimensions(arr); 

	//Determine index of number of slice in array
	int nums[3] = {0, 2, 1};
	int num = nums[m_mode];

	//Set slice range
	int sliceFirst = 0, sliceLast = arr[num]-1;

	bool saveNative = true;
	QStringList inList = ( QStringList() << tr("$Save native image") << tr("#From Slice Number:") <<  tr("#To Slice Number:") );
	QList<QVariant> inPara = ( QList<QVariant>() << (saveNative ? tr("true") : tr("false"))<<tr("%1").arg(sliceFirst) <<tr("%1").arg(sliceLast)  );
	dlg_commoninput *dlg = new dlg_commoninput (mdi_parent, "Save options", 3, inList, inPara, NULL);

	if (dlg->exec() != QDialog::Accepted)
	{
		return;
	}
	saveNative = dlg->getCheckValues()[0];
	sliceFirst = dlg->getValues()[1];
	sliceLast = dlg->getValues()[2];

	if(sliceFirst<0 || sliceFirst>sliceLast || sliceLast>arr[num]){
		QMessageBox msgBox;
		msgBox.setText("Invalid Input.");
		msgBox.exec();
		return;
	}
	//Determine extension
	QFileInfo pars(file);
	int ext;
	if ((QString::compare(pars.suffix(), "TIF", Qt::CaseInsensitive) == 0) || (QString::compare(pars.suffix(), "TIFF", Qt::CaseInsensitive) == 0)){
		ext = 1;
	} else if (QString::compare(pars.suffix(), "PNG", Qt::CaseInsensitive) == 0) {
		ext = 2;
	} else if ((QString::compare(pars.suffix(), "JPG", Qt::CaseInsensitive) == 0) || (QString::compare(pars.suffix(), "JPEG", Qt::CaseInsensitive) == 0)){
		ext = 3;
	} else if (QString::compare(pars.suffix(), "BMP", Qt::CaseInsensitive) == 0) {
		ext = 4;
	}

	//Determine condition of saving
	bool valid = false;
	valid = (imageData->GetScalarType() == VTK_UNSIGNED_CHAR || imageData->GetScalarType() == VTK_CHAR)
		 || ((imageData->GetScalarType() == VTK_UNSIGNED_SHORT || imageData->GetScalarType() == VTK_SHORT ) &&  (ext == 1));
	if (!valid){
		QMessageBox msgBox;
		msgBox.setText("Invalid data type or format selected.");
		msgBox.exec();
		return;
	}

	interactor->Disable();
	vtkImageData* img;
	for(int slice=sliceFirst; slice<=sliceLast; slice++)
	{
		//Determine which axis
		if(m_mode==0){ //yz
			setResliceAxesOrigin(slice, 0, 0);
		} else if(m_mode==1){  //xy
			setResliceAxesOrigin(0, 0, slice);
		} else if(m_mode==2){  //xz
			setResliceAxesOrigin(0, slice, 0);
		}
		update();

		vtkSmartPointer<vtkWindowToImageFilter> wtif = vtkSmartPointer<vtkWindowToImageFilter>::New();
		vtkSmartPointer<vtkImageCast> castfilter = vtkSmartPointer<vtkImageCast>::New();
		//Determine Native
		if (saveNative)
		{
			/*
			if (ext != 1)
			{
			castfilter->SetInputData(img);
			castfilter->SetOutputScalarTypeToUnsignedChar();
			castfilter->Update();
			img = castfilter->GetOutput();
			}
			else
			{
			*/
				img = reslicer->GetOutput();
			//}
		}
		else
		{
			wtif->SetInput(renWin);
			wtif->ReadFrontBufferOff();
			wtif->Update();
			img = wtif->GetOutput();
		}

		//append slice number to filename
		std::stringstream ss;
		ss << slice;
		std::string appendFile(ss.str());
		std::string newFileName(current_file);
		newFileName.append(appendFile);

		//Determine extension
		if (ext==1){
			newFileName.append(".tif");
		} else if (ext==2) {
			newFileName.append(".png");
		} else if (ext==3){
			newFileName.append(".jpg");
		} else if (ext==4) {
			newFileName.append(".bmp");
		}
		WriteSingleSliceImage(newFileName.c_str(), img);
	}
	interactor->Enable();
	QMessageBox msgBox;
	msgBox.setText("Saving image stack completed.");
	msgBox.exec();
}

void iASlicerData::UpdatePositionMarkerExtent()
{
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
	if (m_positionMarkerSrc && imageData)
	{
		double center[3];
		m_positionMarkerSrc->GetCenter(center);
		UpdatePositionMarkerExtent();
		m_positionMarkerSrc->SetCenter(center);
	}
}


void iASlicerData::setMode( const iASlicerMode mode )
{
	m_mode = mode;
	UpdateResliceAxesDirectionCosines();
	reslicer->Update();
	colormapper->Update();
	UpdateBackground();
}


void iASlicerData::UpdateResliceAxesDirectionCosines()
{
	foreach( QSharedPointer<iAChannelSlicerData> ch, m_channels )
		ch->UpdateResliceAxesDirectionCosines( m_mode );
	switch(m_mode)
	{
	case iASlicerMode::YZ:
		reslicer->SetResliceAxesDirectionCosines(0,1,0,  0,0,1,  1,0,0);  //yz 
		break;
	case iASlicerMode::XY:
		reslicer->SetResliceAxesDirectionCosines(1,0,0,  0,1,0,  0,0,1);  //xy
		break;
	case iASlicerMode::XZ:
		reslicer->SetResliceAxesDirectionCosines(1,0,0,  0,0,1,  0,-1,0); //xz
		break;
	default:
		break;
	}
}


void iASlicerData::UpdateBackground()
{
	if (m_userSetBackground)
	{
		ren->SetBackground(rgb);
		return;
	}
	switch(m_mode)
	{
	case iASlicerMode::YZ:
		ren->SetBackground(0.2,0.2,0.2); break;
	case iASlicerMode::XY:
		ren->SetBackground(0.3,0.3,0.3); break;
	case iASlicerMode::XZ:
		ren->SetBackground(0.6,0.6,0.6); break;
	default:
		break;
	}
}

void iASlicerData::SetManualBackground(double r, double g, double b)
{
	m_userSetBackground = true;
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
	UpdateBackground();
}


void iASlicerData::Execute( vtkObject * caller, unsigned long eventId, void * callData )
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
	int * epos = interactor->GetEventPosition();
	if ( !pointPicker->Pick(epos[0], epos[1], 0, ren) ) // z is always zero.
	{
		defaultOutput();
		return;
	}

	// Get the mapped position of the mouse using the picker.
	pointPicker->GetPickedPositions()->GetPoint(0, m_ptMapped);
	double* spacing = imageData->GetSpacing();
	m_ptMapped[0] += 0.5*spacing[0];
	m_ptMapped[1] += 0.5*spacing[1];
	m_ptMapped[2] += 0.5*spacing[2];

/*
	// ORIENTATION / ROTATION FIX:
	// make orientation the same as in other image viewers:
	m_ptMapped[1] = -m_ptMapped[1];
*/
	switch(eventId)
	{
	case vtkCommand::LeftButtonPressEvent:
	{
		double result[4];
		int x, y, z;
		GetMouseCoord(x, y, z, result);
		emit clicked(x, y, z);
		emit UserInteraction();
		break;
	}
	case vtkCommand::LeftButtonReleaseEvent:
	{
		double result[4];
		int x, y, z;
		GetMouseCoord(x, y, z, result);
		emit released(x, y, z);
		emit UserInteraction();
		break;
	}
	case vtkCommand::RightButtonPressEvent:
	{
		double result[4];
		int x, y, z;
		GetMouseCoord(x, y, z, result);
		emit rightClicked(x, y, z);
	}
	case vtkCommand::MouseMoveEvent:
	{
		m_positionMarkerActor->SetVisibility(false);
		double result[4];
		int xCoord, yCoord, zCoord;
		GetMouseCoord(xCoord, yCoord, zCoord, result);
		if (m_decorations)
		{
			printVoxelInformation(xCoord, yCoord, zCoord, result);
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
		if (interactor->GetKeyCode() == 'p')
			emit Pick( );
		break;
	default:
		if (interactor->GetKeyCode() == 'p')
			emit Pick( );
		break;
	}

	interactor->Render();
}

void iASlicerData::GetMouseCoord(int & xCoord, int & yCoord, int & zCoord, double* result)
{
	result[0] = result[1] = result[2] = result[3] = 0;
	double point[4] = { m_ptMapped[0], m_ptMapped[1], m_ptMapped[2], 1 };

	// get a shortcut to the pixel data.
	vtkMatrix4x4 *resliceAxes = vtkMatrix4x4::New();
	resliceAxes->DeepCopy(reslicer->GetResliceAxes());
	resliceAxes->MultiplyPoint(point, result);
	resliceAxes->Delete();

	double * imageSpacing = imageData->GetSpacing();
	xCoord = (int)(result[0] / imageSpacing[0]);
	yCoord = (int)(result[1] / imageSpacing[1]);
	zCoord = (int)(result[2] / imageSpacing[2]);

	// TODO: check for negative origin images!
	int* extent = imageData->GetExtent();
	xCoord = clamp(extent[0], extent[1], xCoord);
	yCoord = clamp(extent[2], extent[3], yCoord);
	zCoord = clamp(extent[4], extent[5], zCoord);
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
		vtkImageData* img = data->GetReslicer()->GetOutput();
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

void iASlicerData::printVoxelInformation(int xCoord, int yCoord, int zCoord, double* result)
{
	if (!m_decorations || 0 == m_ptMapped) return;

	vtkImageData * reslicerOutput = reslicer->GetOutput();
	double const * const slicerSpacing = reslicerOutput->GetSpacing();
	double const * const slicerOrigin = reslicerOutput->GetOrigin();
	int const * const slicerExtent = reslicerOutput->GetExtent();
	int const * const slicerDims = reslicerOutput->GetDimensions();
	double const * const slicerBounds = reslicerOutput->GetBounds();

	// We have to manually set the physical z-coordinate which requires us to get the volume spacing.
	m_ptMapped[2] = 0; 

	int cX = static_cast<int>(floor((m_ptMapped[0] - slicerBounds[0]) / slicerSpacing[0]));
	int cY = static_cast<int>(floor((m_ptMapped[1] - slicerBounds[2]) / slicerSpacing[1]));

	// check image extent; if outside ==> default output
	if ( cX < slicerExtent[0] || cX > slicerExtent[1]    ||    cY < slicerExtent[2] || cY > slicerExtent[3] ) {
		defaultOutput(); return;
	}

	// get index, coords and value to display
	QString strDetails(QString("index     [ %1, %2, %3 ]\ndatavalue [")
		.arg(xCoord).arg(yCoord).arg(zCoord));

	for (int i = 0; i < reslicer->GetOutput()->GetNumberOfScalarComponents(); i++) {
		double Pix = reslicer->GetOutput()->GetScalarComponentAsDouble(cX, cY, 0, i);
		strDetails += " " + QString::number(Pix);
	}
	strDetails += " ]\n";
	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if (mdi_parent &&
		mdi_parent->getLinkedMDIs())
	{
		QList<QMdiSubWindow *> mdiwindows = mdi_parent->getMainWnd()->MdiChildList();
		for (int i = 0; i < mdiwindows.size(); i++) {
			MdiChild *tmpChild = qobject_cast<MdiChild *>(mdiwindows.at(i)->widget());
			if (tmpChild != mdi_parent) {
				double * const tmpSpacing = tmpChild->getImagePointer()->GetSpacing();
				double const * const origImgSpacing = imageData->GetSpacing();
				// TODO: calculate proper coords here for images with finer resolution
				// should go something like this (for xCoord:)
				// static_cast<int>(((m_mode == XY || m_mode == XZ)
				//		? ((m_ptMapped[0] - slicerBounds[0]) / slicerSpacing[0])
				//		: xCoord) * (origImgSpacing[0] / tmpSpacing[0]));
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

	// if requested calculate distance and show actor
	if (pLineActor->GetVisibility()) {
		double distance = sqrt(
						pow((m_startMeasurePoint[0] - m_ptMapped[0]), 2) 
					  + pow((m_startMeasurePoint[1] - m_ptMapped[1]), 2)
					);
		pLineSource->SetPoint2(m_ptMapped[0]-(0.5*slicerSpacing[0]), m_ptMapped[1] - (0.5*slicerSpacing[1]), 0.0);	// ORIENTATION / ROTATION FIX: pLineSource->SetPoint2(m_ptMapped[0], -m_ptMapped[1]);
		pDiskSource->SetOuterRadius(distance);
		pDiskSource->SetInnerRadius(distance);
		strDetails += QString("distance [ %1 ]\n").arg(distance);

	}

	// Update the info text with pixel coordinates/value if requested.
	textInfo->GetActor()->SetPosition(interactor->GetEventPosition()[0]+10, interactor->GetEventPosition()[1]+10);
	textInfo->GetTextMapper()->SetInput(strDetails.toStdString().c_str());

/*
	// ORIENTATION / ROTATION FIX:
	// make orientation the same as in other image viewers:
	setPositionMarkerCenter(m_ptMapped[0], -m_ptMapped[1], 1);//m_planeSrc->SetCenter(m_ptMapped[0], m_ptMapped[1]);
*/
	m_positionMarkerMapper->Update();
}

void iASlicerData::executeKeyPressEvent()
{
	switch(interactor->GetKeyCode())
	{
	case 'm':
		m_startMeasurePoint[0] = m_ptMapped[0];
		m_startMeasurePoint[1] = m_ptMapped[1];
		// does not work reliably (often snaps to positions not the highest gradient close to the current position)
		// and causes access to pixels outside of the image:
		//snapToHighGradient(m_startMeasurePoint[0], m_startMeasurePoint[1]);

		if (m_decorations && pLineSource != NULL)
		{
			double * slicerSpacing = reslicer->GetOutput()->GetSpacing();
			pLineSource->SetPoint1(m_startMeasurePoint[0]-(0.5*slicerSpacing[0]), m_startMeasurePoint[1]-(0.5*slicerSpacing[1]), 0.0);
			pDiskActor->SetPosition(m_startMeasurePoint[0]-(0.5*slicerSpacing[0]), m_startMeasurePoint[1]-(0.5*slicerSpacing[1]), 1.0);
			pLineActor->SetVisibility(true);
			pDiskActor->SetVisibility(true);
			double result[4];
			int xCoord, yCoord, zCoord;
			GetMouseCoord(xCoord, yCoord, zCoord, result);
			printVoxelInformation(xCoord, yCoord, zCoord, result);
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
	{
		return;
	}
	QString strDetails(" ");
	textInfo->GetActor()->SetPosition(20, 20);
	textInfo->GetTextMapper()->SetInput(strDetails.toStdString().c_str());
	m_positionMarkerActor->SetVisibility(false);
	interactor->ReInitialize();
	interactor->Render();
}


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

void iASlicerData::InitReslicerWithImageData()
{
	reslicer->SetInputData( imageData );
	reslicer->SetInformationInput( imageData );
	reslicer->SetResliceTransform( transform );

	reslicer->SetOutputDimensionality( 2 );
	reslicer->SetInterpolationModeToCubic();
	reslicer->InterpolateOn();
	reslicer->AutoCropOutputOn();
	reslicer->SetNumberOfThreads(QThread::idealThreadCount());

	reslicer->UpdateWholeExtent();
	UpdateReslicer();
}

void iASlicerData::UpdateReslicer()
{
	reslicer->Update();
	foreach( QSharedPointer<iAChannelSlicerData> ch, m_channels )
		ch->updateReslicer();
}

void iASlicerData::setShowText( bool isVisible )
{
	if (!m_decorations)
	{
		return;
	}
	textInfo->Show(isVisible);
}

void iASlicerData::enableChannel(iAChannelID id, bool enabled, double x, double y, double z)
{
	if (enabled)
		setResliceChannelAxesOrigin(id, x, y, z);
	enableChannel( id, enabled );
}

void iASlicerData::enableChannel( iAChannelID id, bool enabled )
{
	// TODO: move into channeldata!
	if( enabled )
	{
		ren->AddActor(GetOrCreateChannel(id).imageActor );
		if (m_decorations)
		{
			ren->AddActor(GetOrCreateChannel(id).cActor);
		}
	}
	else
	{
		ren->RemoveActor(GetOrCreateChannel(id).imageActor);
		if (m_decorations)
		{
			ren->RemoveActor(GetOrCreateChannel(id).cActor);
		}
	}
}

void iASlicerData::reInitializeChannel( iAChannelID id, iAChannelVisualizationData * chData )
{
	GetOrCreateChannel(id).ReInit(chData);
}


void iASlicerData::initializeChannel( iAChannelID id, iAChannelVisualizationData * chData )
{
	GetOrCreateChannel(id).Init(chData, m_mode);
}


void iASlicerData::removeChannel(iAChannelID id)
{
	m_channels.remove(id);
}


void iASlicerData::setResliceChannelAxesOrigin( iAChannelID id, double x, double y, double z )
{
	if (interactor->GetEnabled())
	{
		iAChannelSlicerData & chSD = GetOrCreateChannel(id);
		chSD.SetResliceAxesOrigin(x, y, z);
	}
}

void iASlicerData::setChannelOpacity(iAChannelID id, double opacity )
{
	 GetOrCreateChannel(id).imageActor->SetOpacity(opacity);
}

void iASlicerData::setSliceNumber( int sliceNumber )
{
	if (!imageData)
	{
		return;
	}
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
	double * spacing = imageData->GetSpacing();
	//also apply to enabled channels
	foreach( QSharedPointer<iAChannelSlicerData> ch, m_channels )
		ch->SetResliceAxesOrigin( xyz[0] * spacing[0], xyz[1] * spacing[1], xyz[2] * spacing[2] );
	setResliceAxesOrigin( xyz[0] * spacing[0], xyz[1] * spacing[1], xyz[2] * spacing[2] );
}

iAChannelSlicerData & iASlicerData::GetOrCreateChannel(iAChannelID id)
{
	if (!m_channels.contains(id))
	{
		QSharedPointer<iAChannelSlicerData> newData(new iAChannelSlicerData);
		newData->imageActor->SetInterpolate(imageActor->GetInterpolate());
		newData->assignTransform( transform );
		m_channels.insert(id, newData);
	}
	return **m_channels.find(id);
}

iAChannelSlicerData * iASlicerData::GetChannel(iAChannelID id)
{
	if (!m_channels.contains(id))
	{
		return 0;
	}
	return m_channels.find(id)->data();
}

size_t iASlicerData::GetEnabledChannels()
{
	return m_channels.size();
}

void iASlicerData::setMagicLensInput(iAChannelID id)
{
	iAChannelSlicerData * data = GetChannel(id);
	assert ( data );
	if (!data)
	{
		return;
	}
	if (m_magicLensExternal)
	{
		m_magicLensExternal->AddInput(data->reslicer, data->GetLookupTable(),
			reslicer, colorTransferFunction, data->GetName());
	}
}


vtkGenericOpenGLRenderWindow* iASlicerData::GetRenderWindow()
{
	return renWin.GetPointer();
}

vtkRenderer* iASlicerData::GetRenderer()
{
	return ren.GetPointer();
}

vtkCamera* iASlicerData::GetCamera()
{
	return m_camera;
}

void iASlicerData::SetCamera(vtkCamera* cam, bool camOwner /*=true*/)
{
	ren->SetActiveCamera(cam);
	if (m_camera && m_cameraOwner)
	{
		m_camera->Delete();
	}
	m_cameraOwner = camOwner;
	m_camera = cam;
}

void iASlicerData::updateChannelMappers()
{
	QList<iAChannelID> keys = m_channels.keys();
	for (QList<iAChannelID>::iterator it = keys.begin();
		it != keys.end();
		++it)
	{
		iAChannelID key = *it;
		iAChannelSlicerData * chData = m_channels.value(key).data();
		chData->UpdateLUT();
		chData->updateMapper();
	}
}

void iASlicerData::rotateSlice( double angle )
{
	if( !imageData )
		return;
	switch( m_mode )
	{
		case iASlicerMode::XY://XY
			angleZ = angle;
			break;
		case iASlicerMode::YZ://YZ
			angleX = angle;
			break;
		case iASlicerMode::XZ://XZ
			angleY = angle;
			break;
		default://ERROR
			break;
	}

	double t[3];
	double* spacing = imageData->GetSpacing();
	int* ext = imageData->GetExtent();

	t[0] = (ext[1]-ext[0])/2 * spacing[0];
	t[1] = (ext[3]-ext[2])/2 * spacing[1];
	t[2] = (ext[5]-ext[4])/2 * spacing[2];

	vtkTransform *t1 = vtkTransform::New();
	t1->Translate(-t[0], -t[1], -t[2]);

	vtkTransform *t2 = vtkTransform::New();
	t2->RotateWXYZ(angleX, 1, 0, 0);
	t2->RotateWXYZ(angleY, 0, 1, 0);
	t2->RotateWXYZ(angleZ, 0, 0, 1);

	vtkTransform *t3 = vtkTransform::New();
	t3->Translate(t[0], t[1], t[2]);

	transform->Identity();
	transform->PostMultiply();
	transform->Concatenate(t1);
	transform->Concatenate(t2);
	transform->Concatenate(t3);
	
	update();
}

void iASlicerData::switchContourSourceToChannel( iAChannelID id )
{
	if (!m_decorations)
	{
		return;
	}
	iAChannelSlicerData & chan =  GetOrCreateChannel( id );
	cFilter->SetInputConnection( chan.reslicer->GetOutputPort() );
}

void iASlicerData::setContours( int n, double mi, double ma )
{
	if (!m_decorations)
	{
		return;
	}
	no = n;  contourMin = mi; contourMax = ma;
	cFilter->GenerateValues( no, contourMin, contourMax );
}

void iASlicerData::setContours( int n, double * contourValues )
{
	if (!m_decorations)
	{
		return;
	}
	cFilter->SetNumberOfContours( n );
	for( int i = 0; i < n; ++i )
		cFilter->SetValue( i, contourValues[i] );
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

vtkScalarBarWidget * iASlicerData::GetScalarWidget()
{
	return scalarWidget;
}

vtkImageActor* iASlicerData::GetImageActor()
{
	return imageActor;
}

QCursor iASlicerData::getMouseCursor()
{
	return m_mouseCursor;
}