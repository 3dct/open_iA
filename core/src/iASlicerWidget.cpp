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
#include "iASlicerWidget.h"

#include "iAArbitraryProfileOnSlicer.h"
#include "iAChannelVisualizationData.h"
#include "iAFramedQVTKWidget2.h"
#include "iAMagicLens.h"
#include "iAPieChartGlyph.h"
#include "iASlicer.h"
#include "iASlicerData.h"
#include "iASlicerProfile.h"
#include "iASnakeSpline.h"

#include <QVTKInteractorAdapter.h>
#include <QVTKInteractor.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkImageResample.h>
#include <vtkMatrix4x4.h>
#include <vtkPointPicker.h>
#include <vtkRendererCollection.h>

#include <QErrorMessage>
#include <QMenu>
#include <QIcon>
#include <QGridLayout>
#include <QMouseEvent>
#include <QKeyEvent>

#define EPSILON 0.0015

struct PickedData
{
	double pos[3];
	double res[3];
	int ind[3];
};
PickedData	pickedData;


iASlicerWidget::iASlicerWidget( iASlicer const * slicerMaster, QWidget * parent, const QGLWidget * shareWidget, Qt::WindowFlags f,
	bool decorations)
	: QVTKWidget2(parent, shareWidget, f),
	m_magicLensExternal(slicerMaster->magicLens()),
	m_slicerMode(slicerMaster->m_mode),
	m_slicerDataExternal(slicerMaster->m_data),
	m_decorations(decorations)
{
	setFocusPolicy(Qt::StrongFocus);		// to receive the KeyPress Event!
	setCursor(QCursor(Qt::CrossCursor));

	m_imageData = NULL;
	m_viewMode = NORMAL; // standard m_viewMode
	m_xInd = m_yInd = m_zInd = 0;
	m_isInitialized = false;


	m_isSliceProfEnabled = false;
	m_isArbProfEnabled = false;
	m_pieGlyphsEnabled = false;

	if (decorations)
	{
		m_snakeSpline = new iASnakeSpline;

		m_contextMenu = new QMenu(this);
		m_contextMenu->addAction(QIcon(":/images/loadtrf.png"), tr("Delete Snake Line"), this, SLOT(menuDeleteSnakeLine()));
		m_sliceProfile = new iASlicerProfile();
		m_sliceProfile->SetVisibility(false);

		m_arbProfile = new iAArbitraryProfileOnSlicer();
		m_arbProfile->SetVisibility(false);
	}
	
	m_layout = new QGridLayout();
	m_layout->setContentsMargins(0,0,0,0);
	m_layout->setSpacing(0);
	m_layout->addWidget(this);
	parent->setLayout( m_layout );

	//setup context menu for the magic lens view options
	if (m_magicLensExternal)
	{
		m_magicLensContextMenu = new QMenu(this);
		QActionGroup * actionGr; QAction * addedAction;
		actionGr = new QActionGroup( this );
		addedAction = m_magicLensContextMenu->addAction(tr("Centered Magic Lens"), this, SLOT(menuCenteredMagicLens()));
		addedAction->setCheckable( true );
		addedAction->setChecked( true );
		actionGr->addAction( addedAction );
		addedAction = m_magicLensContextMenu->addAction(tr("Offseted Magic Lens"), this, SLOT(menuOffsetMagicLens()));
		addedAction->setCheckable( true );
		actionGr->addAction( addedAction );
		addedAction = m_magicLensContextMenu->addAction(tr("Side by Side"), this, SLOT(menuSideBySideMagicLens()));
		addedAction->setCheckable( true );
		actionGr->addAction( addedAction );
	}

	setAutoFillBackground(false);
}


void iASlicerWidget::initialize( vtkImageData *imageData, vtkPoints *points )
{
	this->m_worldSnakePointsExternal = points;
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();
	ren->GetActiveCamera()->SetParallelProjection(true);

	if (m_decorations)
	{
		m_snakeSpline->initialize(ren, imageData->GetSpacing()[0]);
		m_sliceProfile->initialize(ren);
		m_arbProfile->initialize(ren);
	}

	changeImageData(imageData);
	m_isInitialized = true;
}


iASlicerWidget::~iASlicerWidget()
{
	if (m_decorations)
	{
		delete m_snakeSpline;
		delete m_contextMenu;
		delete m_sliceProfile;
		delete m_arbProfile;
	}
}


void iASlicerWidget::keyPressEvent(QKeyEvent *event)
{
	if(!m_isInitialized)
	{
		QVTKWidget2::keyPressEvent(event);
		return;
	}
	// if not in snake m_viewMode
	if (m_viewMode != SHOW)
	{
		if (event->key() == Qt::Key_Tab && m_viewMode == NORMAL)
			m_viewMode = DEFINE_SPLINE;

		else
			m_viewMode = NORMAL;
		
		switchMode(m_viewMode);

		// let other slice views know that slice view m_viewMode changed
		emit switchedMode(m_viewMode);
	}

	QVTKWidget2::keyPressEvent(event);
}


void iASlicerWidget::mousePressEvent(QMouseEvent *event)
{
	if(!m_isInitialized)
	{
		QVTKWidget2::mousePressEvent(event);
		return;
	}

	double pos[3];
	double result[4];
	int indxs[3];
	if( !pickPoint(pos, result, indxs) )
	{
		QVTKWidget2::mousePressEvent(event);
		return;
	}

	double spacing[3];
	m_imageData->GetSpacing(spacing);

	if(m_isSliceProfEnabled
		&& (event->modifiers() == Qt::NoModifier)
		&& event->button() == Qt::LeftButton)//if slice profile m_viewMode is enabled do all the necessary operations
	{
		setSliceProfile(result);
	}

	if(m_isArbProfEnabled
		&& (event->modifiers() == Qt::NoModifier)
		&& event->button() == Qt::LeftButton)//if arbitrary profile m_viewMode is enabled do all the necessary operations
	{
		m_arbProfile->FindSelectedPntInd(result[SlicerXInd(m_slicerMode)], result[SlicerYInd(m_slicerMode)]);
	}

	// only changed mouse press behaviour if m_viewMode is in drawing m_viewMode
	// and left mouse button is pressed
	if (m_decorations && m_viewMode == DEFINE_SPLINE && event->button() == Qt::LeftButton)
	{	
		const double x = result[SlicerXInd(m_slicerMode)];
		const double y = result[SlicerYInd(m_slicerMode)];
		
		// if no point is found at picked position add a new one
		if (m_snakeSpline->CalculateSelectedPoint(x,y) == -1)
		{
			m_snakeSpline->addPoint(x, y);
			
			// add the point to the world point list only once because it is a member of MdiChild
			m_worldSnakePointsExternal->InsertNextPoint(result[0], result[1], result[2]);

			// let other slices views know that a new point was created
			emit addedPoint(result[0], result[1], result[2]);
		}
		// call mousePressEvent function of super class without button press information to avoid standard VTK functionality
		QVTKWidget2::mousePressEvent(new QMouseEvent(QEvent::MouseButtonPress, event->pos(), Qt::NoButton, Qt::NoButton, event->modifiers()));
	}
	else
	{
		if (m_decorations && event->modifiers() != Qt::NoModifier)
		// disable brightness/contrast change happening if no modifier pressed
		{
			QVTKWidget2::mousePressEvent(new QMouseEvent(QEvent::MouseButtonPress, event->pos(), Qt::NoButton, Qt::NoButton, event->modifiers()));
		}
		else
		{
			QVTKWidget2::mousePressEvent(event);
		}
		emit Clicked();
	}
}


void iASlicerWidget::mouseMoveEvent(QMouseEvent *event)
{
	QVTKWidget2::mouseMoveEvent(event);
	if(!m_isInitialized)
		return;

	pickPoint(pickedData.pos, pickedData.res, pickedData.ind);
	
	if (!event->modifiers().testFlag(Qt::ShiftModifier))
	{
		updateMagicLens();
	}
	
	// only do something in spline drawing m_viewMode and if a point is selected
	if (m_decorations && m_viewMode == DEFINE_SPLINE && m_snakeSpline->selectedPointIndex() != -1)
	{
		// Do a pick. It will return a non-zero value if we intersected the image.
		vtkPointPicker* pointPicker = (vtkPointPicker*)this->GetInteractor()->GetPicker();

		if (!pointPicker->Pick(GetInteractor()->GetEventPosition()[0], 
			GetInteractor()->GetEventPosition()[1], 
			0,  // always zero.
			GetRenderWindow()->GetRenderers()->GetFirstRenderer())) 
		{
			return;
		}

		// Get the first point of found intersections
		double *ptMapped = pointPicker->GetPickedPositions()->GetPoint(0);

		// Move world and slice view points
		double *point = m_worldSnakePointsExternal->GetPoint(m_snakeSpline->selectedPointIndex());

		double pos[3];
		int indxs[3] = {SlicerXInd(m_slicerMode), SlicerYInd(m_slicerMode), SlicerZInd(m_slicerMode)};
		
		for(int i=0; i<2; ++i)				//2d: x, y
			pos[indxs[i]] = ptMapped[i];
		pos[indxs[2]] = point[indxs[2]];	//z

		movePoint(m_snakeSpline->selectedPointIndex(), pos[0], pos[1], pos[2]);
		
		// update world point list only once because it is a member of MdiChild
		m_worldSnakePointsExternal->SetPoint(m_snakeSpline->selectedPointIndex(), pos[0], pos[1], pos[2]);

		// let other slice views know that a point was moved
		emit movedPoint(m_snakeSpline->selectedPointIndex(), pos[0], pos[1], pos[2]);
	}

	if(m_isSliceProfEnabled 
		&& (event->modifiers() == Qt::NoModifier) )//if profile m_viewMode is enabled do all the necessary operations
	{
		if( event->buttons()&Qt::LeftButton)
		{
			double xPos, yPos, zPos;
			double result[4];
			int x,y,z;
			if( !pickPoint(xPos, yPos, zPos, result, x, y, z) )
				return;
			setSliceProfile(result);
		}
	}

	if(m_isArbProfEnabled)
	{
		int arbProfPtInd = m_arbProfile->GetPntInd();
		if (event->modifiers() == Qt::NoModifier && arbProfPtInd >= 0)//if profile m_viewMode is enabled do all the necessary operations
		{
			if( event->buttons()&Qt::LeftButton)
			{
				double xPos, yPos, zPos;
				double result[4];
				int x,y,z;
				if( !pickPoint(xPos, yPos, zPos, result, x, y, z) )
					return;

				double *ptPos = m_arbProfile->GetPosition(arbProfPtInd);
				const int zind = SlicerZInd(m_slicerMode);
				result[zind] = ptPos[zind];

				if( setArbitraryProfile(arbProfPtInd, result) )
					emit arbitraryProfileChanged(arbProfPtInd, result);
			}
		}
	}
}


void iASlicerWidget::deselectPoint()
{
	if (!m_decorations)
	{
		return;
	}
	m_snakeSpline->deselectPoint();
}


void iASlicerWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if(!m_isInitialized)
	{
		QVTKWidget2::mouseReleaseEvent(event);
		return;
	}
	
	if (m_decorations)
	{
		// deselect points
		m_snakeSpline->deselectPoint();
	}

	// let other slice views now that the point was deselected
	emit deselectedPoint();

	QVTKWidget2::mouseReleaseEvent(event);
}

void iASlicerWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		emit DblClicked();
	}
}

void iASlicerWidget::contextMenuEvent(QContextMenuEvent *event)
{
	if(!m_isInitialized)
	{
		QVTKWidget2::contextMenuEvent(event);
		return;
	}
	// is m_viewMode spline drawing m_viewMode?
	if (m_decorations && m_viewMode == DEFINE_SPLINE)
		m_contextMenu->exec(event->globalPos());

	if(m_magicLensExternal && m_magicLensExternal->Enabled())
		m_magicLensContextMenu->exec(event->globalPos());
}


void iASlicerWidget::switchMode(int mode)
{
	if (!m_decorations)
	{
		return;
	}
	this->m_viewMode = static_cast<viewModeEnum>(mode);
	switch(mode)
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

	// render interactor
	GetRenderWindow()->GetInteractor()->Render();
}


void iASlicerWidget::addPoint(double xPos, double yPos, double zPos)
{
	if (!m_decorations)
	{
		return;
	}
	double pos[3] = {xPos, yPos, zPos};
	double x = pos[ SlicerXInd(m_slicerMode) ];
	double y = pos[ SlicerYInd(m_slicerMode) ];

	//add point to the snake slicer spline
	m_snakeSpline->addPoint(x, y);

	// render slice view
	GetRenderWindow()->GetInteractor()->Render();
}


void iASlicerWidget::setSliceProfile(double Pos[3])
{		
	vtkImageData * reslicedImgData = m_slicerDataExternal->GetReslicer()->GetOutput();
	double PosY = Pos[SlicerYInd(m_slicerMode)];
	if(!m_sliceProfile->setup(PosY, reslicedImgData))
		return;
	// render slice view
	GetRenderWindow()->GetInteractor()->Render();
}


int iASlicerWidget::setArbitraryProfile(int pointInd, double * Pos)
{
	if (!m_decorations)
	{
		return 1;
	}
	double profileCoord2d[2] = {Pos[ SlicerXInd(m_slicerMode) ], Pos[ SlicerYInd(m_slicerMode) ]};
	if( !m_arbProfile->setup(pointInd, Pos, profileCoord2d, m_slicerDataExternal->GetReslicer()->GetOutput()) )
		return 0;
	// render slice view
	GetRenderWindow()->GetInteractor()->Render();
	return 1;
}


void iASlicerWidget::movePoint(size_t selectedPointIndex, double xPos, double yPos, double zPos)
{
	if (!m_decorations)
	{
		return;
	}
	double pos[3] = {xPos, yPos, zPos};
	double x = pos[ SlicerXInd(m_slicerMode) ];
	double y = pos[ SlicerYInd(m_slicerMode) ];

	// move only if a point is selected
	if (selectedPointIndex != -1)
	{
		// move only if a point is selected
		m_snakeSpline->movePoint(selectedPointIndex, x, y);

		// render slice view
		GetRenderWindow()->GetInteractor()->Render();
	}
}


void iASlicerWidget::menuDeleteSnakeLine()
{
	deleteSnakeLine();
	emit deletedSnakeLine();
}


void iASlicerWidget::deleteSnakeLine()
{
	if (!m_decorations)
	{
		return;
	}
	//delete all points from snake slicer spline
	m_snakeSpline->deleteAllPoints();

	// reset point lists
	m_worldSnakePointsExternal->Reset();

	// render slice view
	GetRenderWindow()->GetInteractor()->Render();
}


void iASlicerWidget::setSliceProfileOn( bool isOn )
{
	m_isSliceProfEnabled = isOn;
	m_sliceProfile->SetVisibility(m_isSliceProfEnabled);
	GetRenderWindow()->GetInteractor()->Render();
}


void iASlicerWidget::clearProfileData()
{

}


int iASlicerWidget::pickPoint( double *pos_out, double *result_out, int * ind_out )
{
	return pickPoint(pos_out[0], pos_out[1], pos_out[2], result_out, ind_out[0], ind_out[1], ind_out[2]);
}


int iASlicerWidget::pickPoint(double &xPos_out, double &yPos_out, double &zPos_out, 
	double *result_out,
	int &xInd_out, int &yInd_out, int &zInd_out)
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
	pointPicker->GetPickedPositions()->GetPoint(total_points-1, ptMapped );

	// get image spacing to be able to select a point independent of zoom level
	double spacing[3];
	m_imageData->GetSpacing(spacing);

	vtkMatrix4x4 * resliceAxes = m_slicerDataExternal->GetReslicer()->GetResliceAxes();
	double point[4] = { ptMapped[0], ptMapped[1], 0, 1 }; // We have to set the physical z-coordinate which requires us to get the volume spacing.
	resliceAxes->MultiplyPoint(point, result_out);

	xInd_out = (int)(result_out[0] / spacing[0] + 0.5);
	yInd_out = (int)(result_out[1] / spacing[1] + 0.5);
	zInd_out = (int)(result_out[2] / spacing[2] + 0.5);

	// initialize m_positions depending on slice view
	switch(m_slicerMode)
	{
	case 0: { xPos_out = m_xInd * spacing[0];	yPos_out = ptMapped[0];			zPos_out = ptMapped[1];			} break; //yz
	case 1: { xPos_out = ptMapped[0];		yPos_out = ptMapped[1];			zPos_out = m_zInd * spacing[2];	} break; //xy
	case 2: { xPos_out = ptMapped[0];		yPos_out = m_yInd * spacing[1];	zPos_out = ptMapped[1];			} break; //xz
	}

	return 1;
}


void iASlicerWidget::slicerUpdatedSlot()
{
	if(m_isSliceProfEnabled)
		updateProfile();
}


void iASlicerWidget::updateProfile()
{
	double oldPos[3];
	m_sliceProfile->GetPoint(0, oldPos);
	double pos[3] = {oldPos[1], oldPos[1], oldPos[1]};
	setSliceProfile(pos);
}


void iASlicerWidget::setArbitraryProfileOn( bool isOn )
{
	if (!m_decorations)
	{
		return;
	}
	m_isArbProfEnabled = isOn;
	m_arbProfile->SetVisibility(m_isArbProfEnabled);
	GetRenderWindow()->GetInteractor()->Render();
}


void iASlicerWidget::setPieGlyphsOn( bool isOn )
{
	m_pieGlyphsEnabled = isOn;
	computeGlyphs();
}


void iASlicerWidget::resizeEvent( QResizeEvent * event )
{
	if (m_magicLensExternal)
	{
		const double lenSz = m_magicLensExternal->GetSize();
		m_magicLensExternal->SetScaleCoefficient( lenSz / this->height() );
	}
	QVTKWidget2::resizeEvent(event);
}


void iASlicerWidget::wheelEvent(QWheelEvent* event)
{
	if (m_magicLensExternal && m_magicLensExternal->Enabled() &&
		event->modifiers().testFlag(Qt::ControlModifier))
	{
		double sizeFactor = 1.1 * (std::abs(event->angleDelta().y() / 120.0));
		if (event->angleDelta().y() < 0)
		{
			sizeFactor = 1 / sizeFactor;
		}
		m_magicLensExternal->SetSize(m_magicLensExternal->GetSize() * sizeFactor);
		m_magicLensExternal->SetScaleCoefficient(static_cast<double>(m_magicLensExternal->GetSize()) / height());
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
		QVTKWidget2::wheelEvent(event);
		pickPoint(pickedData.pos, pickedData.res, pickedData.ind);
	}
	updateMagicLens();
}

void iASlicerWidget::Frame()
{
	if(!m_isInitialized)
	{
		QVTKWidget2::Frame();
		return;
	}

	if(	m_magicLensExternal && m_magicLensExternal->Enabled() &&
		(m_magicLensExternal->GetViewMode() == iAMagicLens::SIDE_BY_SIDE ||
		m_magicLensExternal->GetViewMode() == iAMagicLens::OFFSET) &&
		m_magicLensExternal->GetFrameWidth() > 0)
	{
		QPainter painter(this);
		QPen pen( QColor(255, 255, 255, 255) );
		qreal magicLensFrameWidth = m_magicLensExternal->GetFrameWidth();
		pen.setWidthF(magicLensFrameWidth);
		painter.setPen(pen);
		QRect vr = m_magicLensExternal->GetViewRect();
		qreal hw = pen.widthF() * 0.5;
		int penWidth = static_cast<int>(pen.widthF());
		const double UpperLeftFix = penWidth % 2;
		const double HeightWidthFix = (penWidth == 1) ? 1 : 0;
		QPointF points[4] = {
			vr.topLeft()     + QPoint(UpperLeftFix + hw, UpperLeftFix + hw),
			vr.topRight()    + QPoint(HeightWidthFix+1  -hw, UpperLeftFix + hw),
			vr.bottomRight() + QPoint(HeightWidthFix + 1  -hw, HeightWidthFix + 1  -hw),
			vr.bottomLeft()  + QPoint(UpperLeftFix + hw, HeightWidthFix + 1 -hw)
		};
		drawBorderRectangle(painter, points, magicLensFrameWidth);
		if (m_magicLensExternal->GetViewMode() == iAMagicLens::OFFSET)
		{
			painter.drawLine(points[1] + QPoint(magicLensFrameWidth - hw, vr.height()*0.35),
				points[0] + QPoint(m_magicLensExternal->GetOffset() + 10, +10));
			painter.drawLine(points[2] - QPoint(-magicLensFrameWidth + hw, vr.height()*0.35),
				points[3] + QPoint(m_magicLensExternal->GetOffset() + 10, -10));
		}
	}
	QVTKWidget2::Frame();
}


void iASlicerWidget::setMode( iASlicerMode slicerMode )
{
	m_slicerMode = slicerMode;
}


void iASlicerWidget::changeImageData( vtkImageData * imageData )
{
	m_imageData = imageData;
	
	double	* origin	= m_imageData->GetOrigin();
	int		* dim		= m_imageData->GetDimensions();
	double	* spacing	= m_imageData->GetSpacing();
	double end[3];
	for (int i=0; i<3; i++)
		end[i] = origin[i] + (dim[i]-1)*spacing[i];
	setArbitraryProfile(0, origin); setArbitraryProfile(1, end);
	m_isInitialized = true;
}


void iASlicerWidget::menuCenteredMagicLens()
{
	if (!m_magicLensExternal) return;
	m_magicLensExternal->SetViewMode(iAMagicLens::CENTERED);
}


void iASlicerWidget::menuOffsetMagicLens()
{
	if (!m_magicLensExternal) return;
	m_magicLensExternal->SetViewMode(iAMagicLens::OFFSET);
}


void iASlicerWidget::menuSideBySideMagicLens()
{
	if (!m_magicLensExternal) return;
	m_magicLensExternal->SetViewMode(iAMagicLens::SIDE_BY_SIDE);
}


void iASlicerWidget::updateMagicLens()
{
	if(!m_magicLensExternal || !m_magicLensExternal->Enabled())
	{
		return;
	}
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();
	ren->SetWorldPoint(pickedData.res[ SlicerXInd(m_slicerMode) ], pickedData.res[ SlicerYInd(m_slicerMode) ], 0, 1);
	ren->WorldToView();
	ren->ViewToDisplay();
	double * dpos = ren->GetDisplayPoint();

	int lensSz = m_magicLensExternal->GetSize();
	// restrict size to size of smallest side
	lensSz = (std::min)(lensSz, (std::min)(geometry().width(), geometry().height()));
	int lensSzHalf = 0.5*lensSz;

	if (dpos[0] < lensSzHalf)
	{
		dpos[0] = lensSzHalf;
	}
	else if (dpos[0] >= (geometry().width() - lensSzHalf))
	{
		dpos[0] = geometry().width() - lensSzHalf -1;
	}
	if (dpos[1] < lensSzHalf)
	{
		dpos[1] = lensSzHalf;
	}
	else if (dpos[1] >= (geometry().height() - lensSzHalf))
	{
		dpos[1] = geometry().height() - lensSzHalf - 1;
	}

	//because pixels are integer we need to round and adjust the world point
	for(int i=0; i<3; ++i)
		dpos[i] = qRound(dpos[i]);
	ren->SetDisplayPoint(dpos);
	ren->DisplayToWorld();

	double pos2d[2];
	pos2d[0] = dpos[0]; pos2d[1] = this->geometry().height() - dpos[1];

	//adjust the focus point for the split offset if in side-by-side mode
	if( m_magicLensExternal->GetViewMode() == iAMagicLens::SIDE_BY_SIDE )
	{
		dpos[0] += m_magicLensExternal->GetCenterSplitOffset();
		ren->SetDisplayPoint(dpos);
		ren->DisplayToWorld();
	}
	
	GetRenderWindow()->Render();
	Frame();
	m_magicLensExternal->UpdateCamera(ren->GetWorldPoint(), ren->GetActiveCamera());
	
	QRect rect = QRect(pos2d[0]-lensSzHalf, pos2d[1]-lensSzHalf, lensSz, lensSz);
	m_magicLensExternal->SetGeometry(rect);
	m_magicLensExternal->Render();
}


void iASlicerWidget::computeGlyphs()
{
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
	
	for (int chan = 0; chan < m_slicerDataExternal->GetEnabledChannels(); ++chan)
	{
		iAChannelSlicerData * chSlicerData = m_slicerDataExternal->GetChannel(static_cast<iAChannelID>(ch_Concentration0 + chan));
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
}

void iASlicerWidget::setPieGlyphParameters( double opacity, double spacing, double magFactor )
{
	m_pieGlyphOpacity = opacity; 
	m_pieGlyphSpacing = spacing;
	m_pieGlyphMagFactor	= magFactor;

	computeGlyphs();
}


void iASlicerWidget::SetMagicLensFrameWidth(qreal width)
{
	if (m_magicLensExternal)
	{
		m_magicLensExternal->SetFrameWidth(width);
	}
}


void iASlicerWidget::SetMagicLensOpacity(double newOpac)
{
	if (m_magicLensExternal)
	{
		m_magicLensExternal->SetOpacity(newOpac);
	}
}


double iASlicerWidget::GetMagicLensOpacity()
{
	return (m_magicLensExternal) ? m_magicLensExternal->GetOpacity() : 0;
}