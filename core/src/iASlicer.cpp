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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "iASlicer.h"

#include "iAConsole.h"
#include "iAMagicLens.h"
#include "iAMovieHelper.h"
#include "iASlicerData.h"
#include "iASlicerWidget.h"
#include "mdichild.h"

#include <vtkActor.h>

#include <QFileDialog>
#include <QMessageBox>

iASlicer::iASlicer( QWidget * parent, const iASlicerMode mode, QWidget * widget_container, const QGLWidget * shareWidget /*= 0*/, Qt::WindowFlags f /*= 0*/,
	bool decorations /*= true*/, bool magicLensAvailable /*= true*/) :

		QObject(parent),
		m_mode(mode),
		m_magicLensInput(ch_None)
{
	if (magicLensAvailable)
	{
		m_magicLens = QSharedPointer<iAMagicLens>(new iAMagicLens());
	}
	m_data		= new iASlicerData(this, parent, decorations);
	m_widget	= new iASlicerWidget(this, widget_container, shareWidget, f, decorations);

	assert(m_widget);
	if (!m_widget)
	{
		DEBUG_LOG("Slicer: Could not allocate iASlicerWidget!");
		return;
	}
	ConnectWidgetAndData();
	ConnectToMdiChildSlots();

	if (m_magicLens)
	{
		m_magicLens->InitWidget(m_widget, shareWidget, f);
		m_magicLens->SetScaleCoefficient(static_cast<double>(m_magicLens->GetSize()) / m_widget->height());
	}
}

iASlicer::~iASlicer()
{
	delete m_data;
	delete m_widget;
}

void iASlicer::ConnectWidgetAndData()
{
	m_widget->SetRenderWindow(m_data->GetRenderWindow());
	connect(m_data, SIGNAL(updateSignal()), m_widget, SLOT(slicerUpdatedSlot()));
}

void iASlicer::ConnectToMdiChildSlots()
{
	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(this->parent());
	if(!mdi_parent)
		return;//TODO: exceptions?
	// enable linked view (similarity rendering for metadata visualization
	connect( m_data, SIGNAL( oslicerPos(int, int, int, int) ),	mdi_parent,	SLOT( updateRenderers(int, int, int, int) ) );
	
	connect( m_data, SIGNAL(msg(QString)),  mdi_parent, SLOT(addMsg(QString)));
	connect( m_data, SIGNAL(progress(int)), mdi_parent, SLOT(updateProgressBar(int))) ;
}

iASlicerData * iASlicer::GetSlicerData()
{
	return m_data;
}

bool iASlicer::changeInteractorState()
{
	if (m_data->GetInteractor()->GetEnabled())
		m_data->disableInteractor();
	else
		m_data->enableInteractor();
	return m_data->GetInteractor()->GetEnabled();
}

iASlicerWidget * iASlicer::widget() const
{
	return m_widget;
}

void iASlicer::ChangeMode( const iASlicerMode mode )
{
	m_mode = mode;
	m_data->setMode(m_mode);
	m_widget->setMode(m_mode);
}


//iASlicerData: wrapping methods
void iASlicer::disableInteractor()
{
	m_data->disableInteractor();
}

void iASlicer::enableInteractor()
{
	m_data->enableInteractor();
	m_widget->update();
}

void iASlicer::reInitialize( vtkImageData *ds, vtkTransform *tr, vtkColorTransferFunction* ctf, bool sil /*= false*/, bool sp /*= false */ )
{
	m_data->reInitialize(ds, tr, ctf, sil, sp);
}

void iASlicer::reInitializeChannel(iAChannelID id, iAChannelVisualizationData * chData )
{
	m_data->reInitializeChannel(id, chData);
}

void iASlicer::setResliceChannelAxesOrigin(iAChannelID id, double x, double y, double z )
{
	m_data->setResliceChannelAxesOrigin(id, x, y, z);
	if (m_magicLens)
	{
		m_magicLens->UpdateColors();
	}
}

void iASlicer::setPlaneCenter( int x, int y, int z )
{
	m_data->setPlaneCenter(x, y, z);
}

void iASlicer::updateROI()
{
	m_data->updateROI();
}

void iASlicer::update()
{
	m_data->update();
	if (m_magicLens)
	{
		m_magicLens->Render();
	}
}

void iASlicer::saveAsImage() const
{
	m_data->saveAsImage();
}

void iASlicer::saveMovie( QString& fileName, int qual /*= 2*/ )
{
	m_data->saveMovie(fileName, qual);
}

void iASlicer::saveMovie()
{
	QString movie_file_types = GetAvailableMovieFormats();

	QWidget * parentWidget = dynamic_cast<QWidget*>( this->parent() );
	if( !parentWidget )
		return;

	// If VTK was built without video support,
	// display error message and quit.
	if( movie_file_types.isEmpty() )
	{
		QMessageBox::information( parentWidget, "Movie Export", "Sorry, but movie export support is disabled." );
		return;
	}

	QString saveDir;
	MdiChild * mdi_parent = dynamic_cast<MdiChild*>( parentWidget );
	if( mdi_parent )
	{
		QFileInfo fileInfo = mdi_parent->getFileInfo();
		saveDir = fileInfo.absolutePath() + "/" + fileInfo.baseName();
	}

	QString fileName = QFileDialog::getSaveFileName( parentWidget,
		tr( "Export as a movie" ),
		saveDir,
		movie_file_types );
	// Show standard save file dialog using available movie file types.
	saveMovie( fileName );
}

void iASlicer::saveImageStack()
{
	m_data->saveImageStack();
}

vtkImageReslice * iASlicer::GetReslicer() const
{
	return m_data->GetReslicer();
}

void iASlicer::setResliceAxesOrigin( double x, double y, double z )
{
	m_data->setResliceAxesOrigin(x, y, z);
}

void iASlicer::setSliceNumber( int sliceNumber )
{
	m_data->setSliceNumber(sliceNumber);
	UpdateMagicLensColors();
	m_widget->computeGlyphs();
	update();
}

vtkRenderer * iASlicer::GetRenderer() const
{
	return m_data->GetRenderer();
}

void iASlicer::initializeData( vtkImageData *ds, vtkTransform *tr, vtkColorTransferFunction* ctf, bool sil /*= false*/, bool sp /*= false*/)
{
	m_data->initialize(ds, tr, ctf, sil, sp);
}

void iASlicer::setROI( int r[6] )
{
	m_data->setROI(r);
	m_data->updateROI();
}

void iASlicer::setROIVisible( bool isVisible )
{
	m_data->getROIActor()->SetVisibility(isVisible);
}

void iASlicer::changeImageData( vtkImageData *idata )
{
	m_data->changeImageData(idata);
	m_widget->changeImageData(idata);
}


//iASlicerWidget: wrapping methods
void iASlicer::setIndex( int x, int y, int z )
{
	m_widget->setIndex(x, y, z);
}

void iASlicer::setup( iASingleSlicerSettings const & settings )
{
	m_data->setup(settings);
	if (m_magicLens)
	{
		m_widget->updateMagicLens();
	}
}

void iASlicer::initializeWidget( vtkImageData *imageData, vtkPoints *points /*= 0*/ )
{
	m_widget->initialize(imageData, points);
}

void iASlicer::show()
{
	m_widget->show();
}

void iASlicer::setSliceProfileOn( bool isOn )
{
	m_widget->setSliceProfileOn(isOn);
}

void iASlicer::setArbitraryProfileOn( bool isOn )
{
	m_widget->setArbitraryProfileOn(isOn);
}

void iASlicer::setStatisticalExtent( int statExt )
{
	m_data->setStatisticalExtent(statExt);
}

void iASlicer::SetMagicLensEnabled( bool isEnabled )
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensEnabled called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->SetEnabled(isEnabled);
	m_data->setShowText(!isEnabled);
	widget()->updateMagicLens();
}

void iASlicer::SetMagicLensSize(int newSize)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensSize called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->SetSize(newSize);
	m_magicLens->SetScaleCoefficient(static_cast<double>(m_magicLens->GetSize()) / widget()->height());
	widget()->updateMagicLens();
}

void iASlicer::SetMagicLensFrameWidth(int newWidth)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensFrameWidth called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->SetFrameWidth(newWidth);
	widget()->updateMagicLens();
}

void iASlicer::SetMagicLensCount(int count)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensCount called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->SetLensCount(count);
	widget()->updateMagicLens();
}

void iASlicer::SetMagicLensInput(iAChannelID id)
{
	m_data->setMagicLensInput(id);
	m_magicLensInput = id;
}

void iASlicer::SetMagicLensOpacity(double opacity)
{
	m_magicLens->SetOpacity(opacity);
}

vtkGenericOpenGLRenderWindow * iASlicer::GetRenderWindow() const
{
	return m_data->GetRenderWindow();
}

vtkImageData* iASlicer::GetImageData() const
{
	return m_data->GetImageData();
}

iASlicerMode iASlicer::GetMode() const
{
	return m_mode;
}

void iASlicer::initializeChannel(iAChannelID id,  iAChannelVisualizationData * chData)
{
	m_data->initializeChannel(id, chData);
}

void iASlicer::removeChannel(iAChannelID id)
{
	m_data->removeChannel(id);
}

iAChannelID iASlicer::getMagicLensInput() const
{
	return m_magicLensInput;
}

void iASlicer::AddImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_data->AddImageActor(imgActor);
}

void iASlicer::RemoveImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_data->RemoveImageActor(imgActor);
}

void iASlicer::UpdateMagicLensColors()
{
	if (m_magicLens)
	{
		m_magicLens->UpdateColors();
	}
}

void iASlicer::setPieGlyphsOn( bool isOn )
{
	m_widget->setPieGlyphsOn(isOn);
}

void iASlicer::setPieGlyphParameters( double opacity, double spacing, double magFactor )
{
	m_widget->setPieGlyphParameters(opacity, spacing, magFactor);
}

void iASlicer::rotateSlice( double angle )
{
	m_data->rotateSlice( angle );
}

void iASlicer::setChannelOpacity( iAChannelID id, double opacity )
{
	m_data->setChannelOpacity( id, opacity );
}

void iASlicer::enableChannel( iAChannelID id, bool enabled, double x, double y, double z )
{
	m_data->enableChannel( id, enabled, x, y, z );
}

void iASlicer::enableChannel( iAChannelID id, bool enabled )
{
	m_data->enableChannel( id, enabled );
}

void iASlicer::switchContourSourceToChannel( iAChannelID id )
{
	m_data->switchContourSourceToChannel( id );
}

void iASlicer::showIsolines( bool s )
{
	m_data->showIsolines( s );
}

void iASlicer::setContours( int n, double mi, double ma )
{
	m_data->setContours( n, mi, ma );
}

void iASlicer::setContours( int n, double * contourValues )
{
	m_data->setContours( n, contourValues );
}

iAChannelSlicerData * iASlicer::GetChannel( iAChannelID id )
{
	return m_data->GetChannel( id );
}


void iASlicer::SetBackground(double r, double g, double b)
{
	m_data->SetManualBackground(r, g, b);
}

vtkCamera* iASlicer::GetCamera()
{
	return m_data->GetCamera();
}

void iASlicer::SetCamera(vtkCamera* camera, bool owner /*=true*/ )
{
	m_data->SetCamera(camera, owner);
}

// declaration in iASlicerMode.h
const char* GetSlicerModeString(int mode)
{
	return (mode == YZ) ? "YZ"
		: (mode == XY) ? "XY"
		: (mode == XZ) ? "XZ"
		: "??";
}
