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
#include "iASlicerData.h"

#include "dlg_commoninput.h"
#include "iAMagicLens.h"
#include "iAMathUtility.h"
#include "iAMovieHelper.h"
#include "iAObserverRedirect.h"
#include "iAPieChartGlyph.h"
#include "iARulerWidget.h"
#include "iARulerRepresentation.h"
#include "iASlicer.h"
#include "iASlicerSettings.h"
#include "mdichild.h"

#include <vtkAlgorithmOutput.h>
#include <vtkAxisActor2D.h>
#include <vtkBMPWriter.h>
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
#include <vtkJPEGWriter.h>
#include <vtkLineSource.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
#include <vtkLookupTable.h>
#include <vtkMarchingContourFilter.h>
#include <vtkPlaneSource.h>
#include <vtkPNGWriter.h>
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
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkTIFFWriter.h>
#include <vtkTransform.h>
#include <vtkVersion.h>
#include <vtkWindowToImageFilter.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QString>

#include <string>
#include <sstream>


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
	logoWidget(0),
	rep(0),
	image1(0),
	m_planeSrc(0),
	m_planeMapper(0),
	m_planeActor(0),
	cFilter(0),
	cMapper(0),
	cActor(0),
	pLineSource(0),
	pLineMapper(0),
	pLineActor(0),
	pDiskSource(0),
	pDiskMapper(0),
	pDiskActor(0),
	roiSource(0),
	roiMapper(0),
	roiActor(0),
	textInfo(0),
	rulerWidget(0),
	interactor(0)
{
	observerMouseMove = iAObserverRedirect::New(this);
	renWin->AlphaBitPlanesOn();
	renWin->LineSmoothingOn();
	renWin->PointSmoothingOn();
	interactorStyle = vtkInteractorStyleImage::New();
	m_camera = vtkCamera::New();

	reslicer = vtkImageReslice::New();
	colormapper = vtkImageMapToColors::New();
	imageActor = vtkImageActor::New();

	pointPicker = vtkPointPicker::New();

	if (decorations)
	{
		scalarWidget = vtkScalarBarWidget::New();
		textProperty = vtkTextProperty::New();
		logoWidget = vtkLogoWidget::New();
		rep = vtkLogoRepresentation::New();
		image1 = vtkQImageToImageSource::New();

		m_planeSrc = vtkPlaneSource::New();
		m_planeSrc->SetCenter(0, 0, -10000); // to initially hide the green rectangle - use actor visibility instead maybe?
		m_planeMapper = vtkPolyDataMapper::New();
		m_planeActor = vtkActor::New();

		cFilter = vtkMarchingContourFilter::New();
		cMapper = vtkPolyDataMapper::New();
		cActor = vtkActor::New();

		pLineSource = vtkLineSource::New();
		pLineMapper = vtkPolyDataMapper::New();
		pLineActor = vtkActor::New();

		pDiskSource = vtkDiskSource::New();
		pDiskMapper = vtkPolyDataMapper::New();
		pDiskActor = vtkActor::New();
		
		roiSource = vtkPlaneSource::New();
		roiMapper = vtkPolyDataMapper::New();
		roiActor = vtkActor::New();
		
		textInfo = iAWrapperText::New();
		
		rulerWidget = iARulerWidget::New();
	}
	transform = vtkTransform::New();
}


iASlicerData::~iASlicerData(void)
{
	disconnect();
	if(observerMouseMove)
		observerMouseMove->Delete();

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
		rep->Delete();
		logoWidget->Delete();
		image1->Delete();

		m_planeSrc->Delete();
		m_planeMapper->Delete();
		m_planeActor->Delete();

		cFilter->Delete();
		cMapper->Delete();
		cActor->Delete();

		pLineSource->Delete();
		pLineMapper->Delete();
		pLineActor->Delete();

		pDiskSource->Delete();
		pDiskMapper->Delete();
		pDiskActor->Delete();

		roiSource->Delete();
		roiMapper->Delete();
		roiActor->Delete();
		roiActor = NULL;
		rulerWidget->Delete();
	}

	if (m_cameraOwner)
	{
		m_camera->Delete();
	}
}


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

	interactor->AddObserver( vtkCommand::LeftButtonPressEvent, observerMouseMove );
	interactor->AddObserver( vtkCommand::MouseMoveEvent, observerMouseMove );
	interactor->AddObserver( vtkCommand::KeyReleaseEvent, observerMouseMove );
	interactor->AddObserver( vtkCommand::MouseWheelBackwardEvent, observerMouseMove );
	interactor->AddObserver( vtkCommand::MouseWheelForwardEvent, observerMouseMove );

	InitReslicerWithImageData();
	
	UpdateResliceAxesDirectionCosines();
	UpdateBackground();
	pointPicker->SetTolerance(imageData->GetSpacing()[0]/3);

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

		// may be uncomment to test and remove logo for video
		image1->SetQImage(&img);
		image1->Update();
		rep->SetImage(image1->GetOutput( ));
		logoWidget->SetInteractor( interactor );
		logoWidget->SetRepresentation( rep );
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

		m_planeMapper->SetInputConnection( m_planeSrc->GetOutputPort() );
		m_planeActor->SetMapper( m_planeMapper );
		m_planeActor->GetProperty()->SetColor( 0,1,0 );
		m_planeActor->GetProperty()->SetEdgeColor(0,0,1);
		m_planeActor->GetProperty()->SetOpacity( 1 );
		m_planeActor->SetVisibility( poly );

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

		pDiskSource->SetOuterRadius( 0.5 );
		pDiskSource->SetInnerRadius( 0.0 );
		pDiskSource->SetCircumferentialResolution( 20 );
		pDiskSource->Update( );

		pDiskMapper->SetInputConnection( pDiskSource->GetOutputPort() );
		pDiskActor->SetMapper( pDiskMapper );
		pDiskActor->GetProperty()->SetColor( 1.0, 1.0, 1.0 );
		pDiskActor->GetProperty()->SetOpacity( 1 );
		pDiskActor->SetVisibility( false );

		roiMapper->SetInputConnection( roiSource->GetOutputPort() );
		roiActor->SetVisibility( false );
		roiActor->SetMapper( roiMapper );
		roiActor->GetProperty()->SetColor( 1, 0, 0 );
		roiActor->GetProperty()->SetOpacity( 1 );

		roiSource->SetCenter( 0, 0, 1 );
		roiMapper->Update( );
		roiActor->GetProperty()->SetRepresentation( VTK_WIREFRAME );
		
		rulerWidget->SetInteractor(interactor);
		rulerWidget->SetEnabled( true );
		rulerWidget->SetRepositionable( true );
		rulerWidget->SetResizable( true );
		rulerWidget->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue(0.333,0.05);
		rulerWidget->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue(0.333,0.051);
		
		ren->AddActor(m_planeActor);
		ren->AddActor(cActor);
		ren->AddActor(pLineActor);
		ren->AddActor(pDiskActor);
		ren->AddActor(roiActor);
	}

	colormapper->SetLookupTable(colorTransferFunction);
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
	pointPicker->SetTolerance(imgBlender->GetOutput()->GetSpacing()[0]/3);
	//scalarWidget->GetScalarBarActor()->SetLookupTable( lut );

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

	pointPicker->SetTolerance(imageData->GetSpacing()[0]/3);

	if (m_decorations)
	{
		scalarWidget->GetScalarBarActor()->SetLookupTable( colorTransferFunction );
	}

	colormapper->SetLookupTable(colorTransferFunction);

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

		if (m_mode == iASlicerMode::YZ)
		{
			roiSource->SetOrigin(roi[1]*spacing[1]                  , roi[2]*spacing[2]                  , 0);
			roiSource->SetPoint1(roi[1]*spacing[1]+roi[4]*spacing[0], roi[2]*spacing[2]                  , 0);
			roiSource->SetPoint2(roi[1]*spacing[1]                  , roi[2]*spacing[2]+roi[5]*spacing[2], 0);
		}
		else if (m_mode == iASlicerMode::XY)
		{
			roiSource->SetOrigin(roi[0]*spacing[0]                  , roi[1]*spacing[1]                  , 0);
			roiSource->SetPoint1(roi[0]*spacing[0]+roi[3]*spacing[0], roi[1]*spacing[1]                  , 0);
			roiSource->SetPoint2(roi[0]*spacing[0]                  , roi[1]*spacing[1]+roi[4]*spacing[1], 0);
		}
		else if (m_mode == iASlicerMode::XZ)
		{
			roiSource->SetOrigin(roi[0]*spacing[0]                  , roi[2]*spacing[2]                  , 0);
			roiSource->SetPoint1(roi[0]*spacing[0]+roi[3]*spacing[0], roi[2]*spacing[2]                  , 0);
			roiSource->SetPoint2(roi[0]*spacing[0]                  , roi[2]*spacing[2]+roi[5]*spacing[2], 0);
		}

		roiMapper->Update();
		interactor->Render();
	}
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
	setContours(settings.NumberOfIsoLines, settings.MinIsoValue, settings.MaxIsoValue);
	showIsolines(settings.ShowIsoLines);
	showPosition(settings.ShowPosition);
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
	
	if (m_decorations)
	{
		cFilter->Update();
	}
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


void iASlicerData::setPlaneCenter( double x, double y, double z )
{
	if (!m_decorations)
	{
		return;
	}
	if (interactor->GetEnabled())
	{
		double spacing[2] = {
			imageData->GetSpacing()[SlicerXInd(m_mode)],
			imageData->GetSpacing()[SlicerYInd(m_mode)]
		};
		m_planeSrc->SetOrigin(0, 0, 0);
		m_planeSrc->SetPoint1(m_ext * spacing[0], 0, 0);
		m_planeSrc->SetPoint2(0, m_ext * spacing[1], 0);
		m_planeSrc->SetCenter( x, y, z );
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
	m_planeActor->SetVisibility(s); 
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

	pointPicker->SetTolerance(imageData->GetSpacing()[0]/3);

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
	QString file = QFileDialog::getSaveFileName(0, tr("Save Image"), 
												"",
												tr("JPEG (*.jpg);;TIFF (*.tif);;PNG (*.png);;BMP (*.bmp)" ) );


	if (file.isEmpty())
		return;

	QStringList inList = ( QStringList() << tr("$Save native image") );
	bool saveNative = true;
	QList<QVariant> inPara = ( QList<QVariant>() << (saveNative ? tr("true") : tr("false")) );

	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if(!mdi_parent)
		return;

	dlg_commoninput *dlg = new dlg_commoninput (mdi_parent, "Save options", 1, inList, inPara, NULL);
	if (dlg->exec() == QDialog::Accepted)
	{
		saveNative = dlg->getCheckValues()[0];

		vtkImageData *imageData;
		vtkWindowToImageFilter *wtif = vtkWindowToImageFilter::New();

		if (saveNative)
		{
			imageData = reslicer->GetOutput();
		}
		else
		{
			wtif->SetInput(renWin);
			wtif->Update();
			imageData = wtif->GetOutput();
		}

		QFileInfo pars(file);
		if ((QString::compare(pars.suffix(), "TIF", Qt::CaseInsensitive) == 0) || (QString::compare(pars.suffix(), "TIFF", Qt::CaseInsensitive) == 0)){
			vtkTIFFWriter *writer = vtkTIFFWriter::New();
			writer->SetFileName(file.toLatin1());
			writer->SetInputData(imageData);
			writer->Write();
			writer->Delete();	
		} else if (QString::compare(pars.suffix(), "PNG", Qt::CaseInsensitive) == 0) {
			vtkPNGWriter *writer = vtkPNGWriter::New();
			writer->SetFileName(file.toLatin1());
			writer->SetInputData(imageData);
			writer->Write();
			writer->Delete();
		} else if ((QString::compare(pars.suffix(), "JPG", Qt::CaseInsensitive) == 0) || (QString::compare(pars.suffix(), "JPEG", Qt::CaseInsensitive) == 0)){
			if ( saveNative )
			{
				vtkImageCast* castfilter = vtkImageCast::New();
				castfilter->SetInputData(imageData);
				castfilter->SetOutputScalarTypeToUnsignedChar();
				castfilter->Update();

				vtkJPEGWriter *writer = vtkJPEGWriter::New();
				writer->SetFileName(file.toLatin1());
				writer->SetInputConnection(castfilter->GetOutputPort());
				writer->Write();
				writer->Delete();
			}
			else
			{
				vtkJPEGWriter *writer = vtkJPEGWriter::New();
				writer->SetFileName(file.toLatin1());
				writer->SetInputData(imageData);
				writer->Write();
				writer->Delete();
			}
		} else if (QString::compare(pars.suffix(), "BMP", Qt::CaseInsensitive) == 0) {
			if ( saveNative )
			{
				vtkImageCast* castfilter = vtkImageCast::New();
				castfilter->SetInputData(imageData);
				castfilter->SetOutputScalarTypeToUnsignedChar();
				castfilter->Update();

				vtkBMPWriter *writer = vtkBMPWriter::New();
				writer->SetFileName(file.toLatin1());
				writer->SetInputConnection(castfilter->GetOutputPort());
				writer->Write();
				writer->Delete();
			}
			else
			{
				vtkBMPWriter *writer = vtkBMPWriter::New();
				writer->SetFileName(file.toLatin1());
				writer->SetInputData(imageData);
				writer->Write();
				writer->Delete();
			}
		}

		wtif->Delete();
	}
}


void iASlicerData::saveImageStack()
{
	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if(!mdi_parent)
		return;
	QString file = QFileDialog::getSaveFileName(mdi_parent, tr("Save Image"), 
		"",
		tr("JPEG (*.jpg);;TIFF (*.tif);;PNG (*.png);;BMP (*.bmp)" ) );

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
	int nums[3] = {0, 1, 2};
	int num = nums[m_mode];

	//Set slice range
	int sliceFirst = 0, sliceLast = arr[num]-1;

	bool saveNative = true;
	QStringList inList = ( QStringList() << tr("$Save native image") << tr("#From Slice Number:") <<  tr("#To Slice Number:") );
	QList<QVariant> inPara = ( QList<QVariant>() << (saveNative ? tr("true") : tr("false"))<<tr("%1").arg(sliceFirst) <<tr("%1").arg(sliceLast)  );
	dlg_commoninput *dlg = new dlg_commoninput (mdi_parent, "Save options", 3, inList, inPara, NULL);

	if (dlg->exec() == QDialog::Accepted)
	{
		saveNative = dlg->getCheckValues()[0];
		sliceFirst = dlg->getValues()[1];
		sliceLast = dlg->getValues()[2];

		if(sliceFirst<0 || sliceFirst>sliceLast || sliceLast>arr[num]){
			QMessageBox msgBox;
			msgBox.setText("Invalid Input.");
			msgBox.exec();
		}

		else{
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
			if (imageData->GetScalarType() == VTK_UNSIGNED_CHAR || imageData->GetScalarType() == VTK_CHAR){
				valid = true;
			}
			else if (imageData->GetScalarType() == VTK_UNSIGNED_SHORT || imageData->GetScalarType() == VTK_SHORT){
				if (ext==1 || ext==2){ valid =true; }
				else if (ext==3 || ext==4){ valid = false; }
			}
			else if (imageData->GetScalarType() == VTK_UNSIGNED_INT || imageData->GetScalarType() == VTK_INT){
				valid = false;
			}
			else if (imageData->GetScalarType() == VTK_FLOAT){
				valid = false;
			}
			else if (imageData->GetScalarType() == VTK_DOUBLE){
				valid = false;
			}


			//Valid Save Check
			if (valid == false){
				QMessageBox msgBox;
				msgBox.setText("Invalid data type or format selected.");
				msgBox.exec();

			}

			else {
				for(int slice=sliceFirst; slice<=sliceLast; slice++){// loop based on the number of slice

					vtkWindowToImageFilter *wtif = vtkWindowToImageFilter::New();
					//Determine which axis
					if(m_mode==0){ //yz
						setResliceAxesOrigin(slice, 0, 0);
					} else if(m_mode==1){  //xy 
						setResliceAxesOrigin(0, 0, slice);
					} else if(m_mode==2){  //xz
						setResliceAxesOrigin(0, slice, 0);
					}

					//Determine Native
					if (saveNative)
					{
						imageData = reslicer->GetOutput();
					}
					else
					{
						wtif->SetInput(renWin);
						wtif->Update();
						imageData = wtif->GetOutput();
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
						vtkTIFFWriter *writer = vtkTIFFWriter::New();
						writer->SetFileName(newFileName.c_str());
						writer->SetInputData(imageData);
						writer->Write();
						writer->Delete();	
					} else if (ext==2) {
						newFileName.append(".png");
						vtkPNGWriter *writer = vtkPNGWriter::New();
						writer->SetFileName(newFileName.c_str());
						writer->SetInputData(imageData);
						writer->Write();
						writer->Delete();
					} else if (ext==3){
						newFileName.append(".jpg");
						if ( saveNative )
						{

							vtkImageCast* castfilter = vtkImageCast::New();
							castfilter->SetInputData(imageData);
							castfilter->SetOutputScalarTypeToUnsignedChar();
							castfilter->Update();

							vtkJPEGWriter *writer = vtkJPEGWriter::New();
							writer->SetFileName(newFileName.c_str());
							writer->SetInputConnection(castfilter->GetOutputPort());
							writer->Write();
							writer->Delete();
						}
						else
						{
							vtkJPEGWriter *writer = vtkJPEGWriter::New();
							writer->SetFileName(newFileName.c_str());
							writer->SetInputData(imageData);
							writer->Write();
							writer->Delete();
						}
					} else if (ext==4) {
						newFileName.append(".bmp");
						if ( saveNative )
						{
							vtkImageCast* castfilter = vtkImageCast::New();
							castfilter->SetInputData(imageData);
							castfilter->SetOutputScalarTypeToUnsignedChar();
							castfilter->Update();

							vtkBMPWriter *writer = vtkBMPWriter::New();
							writer->SetFileName(newFileName.c_str());
							writer->SetInputConnection(castfilter->GetOutputPort());
							writer->Write();
							writer->Delete();
						}
						else
						{
							vtkBMPWriter *writer = vtkBMPWriter::New();
							writer->SetFileName(newFileName.c_str());
							writer->SetInputData(imageData);
							writer->Write();
							writer->Delete();
						}
					}

					wtif->Delete();
				}

				QMessageBox msgBox;
				msgBox.setText("Saving image stack completed.");
				msgBox.exec();
			}
		}
	}
}


void iASlicerData::setStatisticalExtent( int statExt )
{
	m_ext = statExt;
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
	case vtkCommand::MouseMoveEvent:
	{
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

#define FmtStr(str_out, strm) { std::ostringstream oss; oss << strm; str_out = oss.str(); }

void iASlicerData::printVoxelInformation(int xCoord, int yCoord, int zCoord, double* result)
{
	if (!m_decorations || 0 == m_ptMapped) return;

	vtkImageData * reslicerOutput = reslicer->GetOutput();
	double * slicerSpacing = reslicerOutput->GetSpacing();
	double * slicerOrigin = reslicerOutput->GetOrigin();
	int * slicerExtent = reslicerOutput->GetExtent();
	int * slicerDims = reslicerOutput->GetDimensions();
	double * slicerBounds = reslicerOutput->GetBounds();

	// We have to manually set the physical z-coordinate which requires us to get the volume spacing.
	m_ptMapped[2] = 0; 

	int cX, cY;

	cX = static_cast<int>(floor((m_ptMapped[0] - slicerBounds[0]) / slicerSpacing[0]));
	cY = static_cast<int>(floor((m_ptMapped[1] - slicerBounds[2]) / slicerSpacing[1]));

	// check image extent; if outside ==> default output
	if ( cX < slicerExtent[0] || cX > slicerExtent[1]    ||    cY < slicerExtent[2] || cY > slicerExtent[3] ) {
		defaultOutput(); return;
	}


	// get index, coords and value to display
	std::string tmp;

	FmtStr(m_strDetails,
		"index     [ " << xCoord << ", " << yCoord << ", " << zCoord << " ]\n" <<
		"datavalue [" );

	for (int i = 0; i < reslicer->GetOutput()->GetNumberOfScalarComponents(); i++) {
		double Pix = reslicer->GetOutput()->GetScalarComponentAsDouble(cX, cY, 0, i);
		FmtStr(tmp, " " << Pix); m_strDetails += tmp;
	}
	FmtStr(tmp, " ]\n"); m_strDetails += tmp;
	std::ostringstream ss;
	double tmpPix;
	std::string modAbb;
	bool longName = true;
	std::string file;
	std::string path;

	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if (mdi_parent == mdi_parent->getM_mainWnd()->activeMdiChild() && mdi_parent->getLinkedMDIs())
	{
		QList<QMdiSubWindow *> mdiwindows = mdi_parent->getM_mainWnd()->MdiChildList();
		for (int i = 0; i < mdiwindows.size(); i++) {
			MdiChild *tmpChild = qobject_cast<MdiChild *>(mdiwindows.at(i)->widget());
			if (tmpChild != mdi_parent) {
				double spacing[3];
				std::size_t foundSlash;
				tmpChild->getImagePointer()->GetSpacing(spacing);
				switch (m_mode)
				{
				case iASlicerMode::XY://XY
					tmpChild->getSlicerDataXY()->setPlaneCenter(xCoord*spacing[0], yCoord*spacing[1], 1);
					tmpChild->getSlicerXY()->setIndex(xCoord, yCoord, zCoord);
					tmpChild->getSlicerDlgXY()->spinBoxXY->setValue(zCoord);
					
					tmpChild->getSlicerDataXY()->update();
					tmpChild->getSlicerXY()->update();
					tmpChild->getSlicerDlgYZ()->update();

					tmpChild->update();
					
					tmpPix = tmpChild->getSlicerDataXY()->GetReslicer()->GetOutput()->GetScalarComponentAsDouble(cX, cY, 0, 0);
					ss << tmpPix;

					path = tmpChild->getFileInfo().absoluteFilePath().toStdString();
					foundSlash = path.find_last_of("/");
					file = path.substr(foundSlash + 1);

					m_strDetails += file + "\t\t\t, " + ss.str() + "\n";
					ss.str("");
					ss.clear();
					break;
				case iASlicerMode::YZ://YZ
					tmpChild->getSlicerDataYZ()->setPlaneCenter(yCoord*spacing[1], zCoord*spacing[2], 1);
					tmpChild->getSlicerYZ()->setIndex(xCoord, yCoord, zCoord);
					tmpChild->getSlicerDlgYZ()->spinBoxYZ->setValue(xCoord);

					tmpChild->getSlicerDataYZ()->update();
					tmpChild->getSlicerYZ()->update();
					tmpChild->getSlicerDlgYZ()->update();

					tmpChild->update();

					tmpPix = tmpChild->getSlicerDataYZ()->GetReslicer()->GetOutput()->GetScalarComponentAsDouble(cX, cY, 0, 0);
					ss << tmpPix;

					path = tmpChild->getFileInfo().absoluteFilePath().toStdString();
					foundSlash = path.find_last_of("/");
					file = path.substr(foundSlash + 1);

					m_strDetails += file + "\t\t\t, " + ss.str() + "\n";
					ss.str("");
					ss.clear();
					break;
				case iASlicerMode::XZ://XZ
					tmpChild->getSlicerDataXZ()->setPlaneCenter(xCoord*spacing[0], zCoord*spacing[2], 1);
					tmpChild->getSlicerXZ()->setIndex(xCoord, yCoord, zCoord);
					tmpChild->getSlicerDlgXZ()->spinBoxXZ->setValue(yCoord);

					tmpChild->getSlicerDataXZ()->update();
					tmpChild->getSlicerXZ()->update();
					tmpChild->getSlicerDlgXZ()->update();
					
					tmpChild->update();

					tmpPix = tmpChild->getSlicerDataXZ()->GetReslicer()->GetOutput()->GetScalarComponentAsDouble(cX, cY, 0, 0);
					ss << tmpPix;

					path = tmpChild->getFileInfo().absoluteFilePath().toStdString();
					foundSlash = path.find_last_of("/");
					file = path.substr(foundSlash + 1);

					m_strDetails += file + "\t\t\t, " + ss.str() + "\n";
					ss.str("");
					ss.clear();
					break;
				default://ERROR
					break;
				}
			}
		}
	}

	// if requested calculate distance and show actor
	if (pLineActor->GetVisibility()) {
		double distance = sqrt(
						pow((m_startMeasurePoint[0] - m_ptMapped[0]), 2) 
					  + pow((m_startMeasurePoint[1] - m_ptMapped[1]), 2)
					);
		FmtStr(tmp, "distance [" << distance << " ]\n"); m_strDetails += tmp;
	}

	// Update the info text with pixel coordinates/value if requested.
	textInfo->GetActor()->SetPosition(interactor->GetEventPosition()[0]+10, interactor->GetEventPosition()[1]+10);
	textInfo->GetTextMapper()->SetInput(m_strDetails.c_str());

/*
	// ORIENTATION / ROTATION FIX:
	// make orientation the same as in other image viewers:
	setPlaneCenter(m_ptMapped[0], -m_ptMapped[1], 1);//m_planeSrc->SetCenter(m_ptMapped[0], m_ptMapped[1], 1);
*/
	m_planeMapper->Update();

/*
	// ORIENTATION / ROTATION FIX:
	// make orientation the same as in other image viewers:
	if (pLineSource != NULL)	pLineSource->SetPoint2(m_ptMapped[0], -m_ptMapped[1], m_ptMapped[2]);
*/

	//show cross here
}

void iASlicerData::executeKeyPressEvent()
{
	switch(interactor->GetKeyCode())
	{
	case 'm':
		snap(m_ptMapped[0], m_ptMapped[1]);
		m_startMeasurePoint[0] = m_ptMapped[0];
		m_startMeasurePoint[1] = m_ptMapped[1];

		if (m_decorations && pLineSource != NULL){
			pLineSource->SetPoint1(m_startMeasurePoint[0], m_startMeasurePoint[1], 0.0);
			pDiskActor->SetPosition(m_startMeasurePoint[0], m_startMeasurePoint[1], 0.0);
		}

		if (m_decorations && pLineActor != NULL){
			pLineActor->SetVisibility(true);
			pDiskActor->SetVisibility(true);
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
	FmtStr(m_strDetails, " " );
	textInfo->GetActor()->SetPosition(20, 20);
	textInfo->GetTextMapper()->SetInput(m_strDetails.c_str());

	m_planeSrc->SetCenter(0, 0, -10000);
	interactor->ReInitialize();
	interactor->Render();
}


void iASlicerData::snap(double &x, double &y)
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
	if( enabled )
	{
		ren->AddActor( GetOrCreateChannel( id ).imageActor );
		if (m_decorations)
		{
			ren->AddActor( GetOrCreateChannel( id ).cActor );
		}
	}
	else
	{
		ren->RemoveActor( GetOrCreateChannel( id ).imageActor );
		if (m_decorations)
		{
			ren->RemoveActor( GetOrCreateChannel( id ).cActor );
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
		m_magicLensExternal->SetInput(data->reslicer, data->GetLookupTable(),
			reslicer, colorTransferFunction);
	}
}

void iASlicerData::setMagicLensCaption(std::string const & caption)
{
	if (!m_magicLensExternal)
	{
		return;
	}
	m_magicLensExternal->SetCaption(caption);
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
	iAChannelSlicerData * chan =  &GetOrCreateChannel( id );
	cFilter->SetInputConnection( chan->reslicer->GetOutputPort() );
}

void iASlicerData::setContours( int n, double mi, double ma )
{
	if (!m_decorations)
	{
		return;
	}
	no = n;  min = mi; max = ma;
	cFilter->GenerateValues( no, min, max );
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

vtkScalarBarWidget * iASlicerData::GetScalarWidget()
{
	return scalarWidget;
}

vtkImageActor* iASlicerData::GetImageActor()
{
	return imageActor;
}
