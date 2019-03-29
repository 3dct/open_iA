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
#include "iAConsole.h"
#include "iAMagicLens.h"
#include "iAMovieHelper.h"
#include "iASlicerData.h"
#include "iASlicerWidget.h"
#include "mdichild.h"

#include <vtkActor.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <QFileDialog>
#include <QMessageBox>

#include <cassert>

iASlicer::iASlicer( QWidget * parent, const iASlicerMode mode,
	bool decorations /*= true*/, bool magicLensAvailable /*= true*/, vtkAbstractTransform *tr, vtkPoints* snakeSlicerPoints) :
	QObject(parent),
	m_mode(mode),
	m_magicLensInput(NotExistingChannel)
{
	assert(m_widget);
	if (!m_widget)
	{
		DEBUG_LOG("Slicer: Could not allocate iASlicerWidget!");
		return;
	}

	if (magicLensAvailable)
		m_magicLens = QSharedPointer<iAMagicLens>(new iAMagicLens());
	// widget and data creation both access magic lens created above, so don't move that up to init list (unless restructuring slicer)
	m_data.reset(new iASlicerData(this, parent, decorations));
	// widget requires data
	m_widget = new iASlicerWidget(this, decorations);
	m_widget->SetRenderWindow(m_data->getRenderWindow());
	if (magicLensAvailable)
		m_magicLens->SetRenderWindow(m_data->getRenderWindow());
	connect(m_data.data(), SIGNAL(updateSignal()), m_widget, SLOT(slicerUpdatedSlot()));
	m_data->initialize(tr);
	m_widget->initialize(snakeSlicerPoints);
}

iASlicerData * iASlicer::data()
{
	return m_data.data();
}

iASlicerWidget * iASlicer::widget()
{
	return m_widget;
}

bool iASlicer::changeInteractorState()
{
	if (m_data->getInteractor()->GetEnabled())
		m_data->disableInteractor();
	else
		m_data->enableInteractor();
	return m_data->getInteractor()->GetEnabled();
}

void iASlicer::changeMode( const iASlicerMode mode )
{
	m_mode = mode;
	m_data->setMode(m_mode);
	m_widget->setMode(m_mode);
}

void iASlicer::disableInteractor()
{
	m_data->disableInteractor();
}

void iASlicer::enableInteractor()
{
	m_data->enableInteractor();
	m_widget->update();
}

/*
void iASlicer::setResliceChannelAxesOrigin(uint id, double x, double y, double z )
{
	m_data->setResliceChannelAxesOrigin(id, x, y, z);
	if (m_magicLens)
		m_magicLens->UpdateColors();
}
*/

void iASlicer::setPositionMarkerCenter(double x, double y)
{
	m_data->setPositionMarkerCenter(x, y);
}

void iASlicer::update()
{
	if (!m_widget->isVisible())
		return;
	m_data->update();
	if (m_magicLens)
		m_magicLens->Render();
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

//vtkImageReslice * iASlicer::GetReslicer() const
//{
//	return m_data->GetReslicer();
//}

void iASlicer::setResliceAxesOrigin( double x, double y, double z )
{
	m_data->setResliceAxesOrigin(x, y, z);
}

void iASlicer::setSliceNumber( int sliceNumber )
{
	m_data->setSliceNumber(sliceNumber);
	updateMagicLensColors();
	m_widget->computeGlyphs();
	update();
}

void iASlicer::setSlabThickness(int thickness)
{
	m_data->setSlabThickness(thickness);
}

void iASlicer::setSlabCompositeMode(int compositeMode)
{
	m_data->setSlabCompositeMode(compositeMode);
}

vtkRenderer * iASlicer::getRenderer() const
{
	return m_data->getRenderer();
}

void iASlicer::updateROI(int const roi[6])
{
	m_data->updateROI(roi);
}

void iASlicer::setROIVisible(bool isVisible)
{
	m_data->setROIVisible(isVisible);
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
	m_widget->GetRenderWindow()->Render();
}

void iASlicer::show()
{
	m_widget->show();
}

void iASlicer::setStatisticalExtent( int statExt )
{
	m_data->setStatisticalExtent(statExt);
}

void iASlicer::setMagicLensEnabled( bool isEnabled )
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensEnabled called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->SetEnabled(isEnabled);
	m_data->setRightButtonDragZoomEnabled(!isEnabled);
	m_data->setShowText(!isEnabled);
	widget()->updateMagicLens();
}

iAMagicLens * iASlicer::magicLens()
{
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
	widget()->updateMagicLens();
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
	widget()->updateMagicLens();
}

void iASlicer::setMagicLensCount(int count)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensCount called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->SetLensCount(count);
	widget()->updateMagicLens();
}

void iASlicer::setMagicLensInput(uint id)
{
	if (!m_magicLens)
	{
		DEBUG_LOG("SetMagicLensInput called on slicer which doesn't have a magic lens!");
		return;
	}
	m_data->setMagicLensInput(id);
	m_magicLensInput = id;
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

vtkGenericOpenGLRenderWindow * iASlicer::getRenderWindow() const
{
	return m_data->getRenderWindow();
}

iASlicerMode iASlicer::getMode() const
{
	return m_mode;
}

void iASlicer::addChannel(uint id, iAChannelData & chData)
{
	m_data->addChannel(id, chData);
}

void iASlicer::removeChannel(uint id)
{
	m_data->removeChannel(id);
}

void iASlicer::updateChannel(uint id, iAChannelData & chData)
{
	m_data->updateChannel(id, chData);
}

void iASlicer::addImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_data->addImageActor(imgActor);
}

void iASlicer::removeImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_data->removeImageActor(imgActor);
}

void iASlicer::updateMagicLensColors()
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

void iASlicer::setChannelOpacity(uint id, double opacity )
{
	m_data->setChannelOpacity( id, opacity );
}

void iASlicer::enableChannel( uint id, bool enabled )
{
	m_data->enableChannel( id, enabled );
}

/*
void iASlicer::switchContourSourceToChannel( uint id )
{
	m_data->switchContourSourceToChannel( id );
}
*/

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

iAChannelSlicerData * iASlicer::getChannel( uint id )
{
	return m_data->getChannel( id );
}

void iASlicer::setBackground(double r, double g, double b)
{
	m_data->setManualBackground(r, g, b);
}

vtkCamera* iASlicer::getCamera()
{
	return m_data->getCamera();
}

void iASlicer::setCamera(vtkCamera* camera, bool owner /*=true*/ )
{
	m_data->setCamera(camera, owner);
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
