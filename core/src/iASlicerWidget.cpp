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
#include "iASlicerWidget.h"

#include "iAArbitraryProfileOnSlicer.h"
#include "iAChannelData.h"
#include "iAChannelSlicerData.h"
#include "iAConsole.h"
#include "iAMagicLens.h"
#include "iAMathUtility.h"
#include "iAPieChartGlyph.h"
#include "iASlicer.h"
#include "iASlicerData.h"
#include "iASlicerProfile.h"
#include "iASnakeSpline.h"
#include "iAMagicLens.h"
#include "iAPieChartGlyph.h"

#include <QVTKInteractorAdapter.h>
#include <QVTKInteractor.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkImageResample.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkPointPicker.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkThinPlateSplineTransform.h>

#include <QBitmap>
#include <QErrorMessage>
#include <QGridLayout>
#include <QIcon>
#include <QKeyEvent>
#include <qmath.h>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

#define EPSILON 0.0015

struct PickedData
{
	double pos[3];
	double res[3];
	int ind[3];
};
PickedData	pickedData;


iASlicerWidget::iASlicerWidget( iASlicer * slicerMaster, QWidget * widget_container, bool decorations)
	: iAVtkWidget(widget_container),
	m_magicLensExternal(slicerMaster->magicLens()),
	m_slicerMode(slicerMaster->getMode()),
	m_slicerDataExternal(slicerMaster->data()),
	m_decorations(decorations)
{
	setFocusPolicy(Qt::StrongFocus);		// to receive the KeyPress Event!
	setMouseTracking(true);					// to receive the Mouse Move Event
	m_viewMode = NORMAL; // standard m_viewMode
	m_xInd = m_yInd = m_zInd = 0;
	m_isInitialized = false;

	m_isSliceProfEnabled = false;
	m_isArbProfEnabled = false;
	m_pieGlyphsEnabled = false;

	//fisheye lens activated
	fisheyeLensActivated = false;

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
	m_layout->setSpacing(0);
	showBorder(false);
	m_layout->addWidget(this);
	widget_container->setLayout( m_layout );

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
	}

	setAutoFillBackground(false);
}


void iASlicerWidget::showBorder(bool show)
{
	int borderWidth = show ? BorderWidth : 0;
	m_layout->setContentsMargins(borderWidth, borderWidth, borderWidth, borderWidth);
}


void iASlicerWidget::initialize(vtkPoints *snakeSlicerPoints )
{
	m_worldSnakePointsExternal = snakeSlicerPoints;
	GetRenderWindow()->SetNumberOfLayers(3);
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();
	ren->GetActiveCamera()->SetParallelProjection(true);

	if (m_decorations)
	{
		// TODO: fix snake spline with non-fixed slicer images
		//m_snakeSpline->initialize(ren, imageData->GetSpacing()[0]);
		m_sliceProfile->initialize(ren);
		m_arbProfile->initialize(ren);
	}
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
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();

	if (event->key() == Qt::Key_R)
	{
		ren->ResetCamera();
	}
	if (event->key() == Qt::Key_O)
	{
		pickPoint( pickedData.pos, pickedData.res, pickedData.ind );
		// TODO: fisheye lens on all channels???
		if (!m_slicerDataExternal->getChannel(0))
			return;
		auto reslicer = m_slicerDataExternal->getChannel(0)->reslicer;
		if (!fisheyeLensActivated)
		{
			fisheyeLensActivated = true;
			reslicer->SetAutoCropOutput	( !reslicer->GetAutoCropOutput() );
			ren->SetWorldPoint( pickedData.res[SlicerXInd( m_slicerMode )], pickedData.res[SlicerYInd( m_slicerMode )], 0, 1 );

			initializeFisheyeLens(reslicer);

			updateFisheyeTransform( ren->GetWorldPoint(), m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);
		}
		else
		{
			fisheyeLensActivated = false;
			reslicer->SetAutoCropOutput( !reslicer->GetAutoCropOutput() );

			// Clear outdated circles and actors (not needed for final version)
			for ( int i = 0; i < circle1ActList.length(); ++i )
				ren->RemoveActor( circle1ActList.at( i ) );
			//circle1List.clear();
			circle1ActList.clear();

			for ( int i = 0; i < circle2ActList.length(); ++i )
				ren->RemoveActor( circle2ActList.at( i ) );
			circle2List.clear();
			circle2ActList.clear(); //*/

			ren->RemoveActor( fisheyeActor );

			// No fisheye transform
			double bounds[6];
			reslicer->GetInformationInput()->GetBounds( bounds );
			p_target->SetNumberOfPoints( 4 );
			p_source->SetNumberOfPoints( 4 );
			p_target->SetPoint( 0, bounds[0], bounds[2], 0 ); //x_min, y_min, bottom left
			p_target->SetPoint( 1, bounds[0], bounds[3], 0 ); //x_min, y_max, top left
			p_target->SetPoint( 2, bounds[1], bounds[3], 0 ); //x_max, y_max, top right
			p_target->SetPoint( 3, bounds[1], bounds[2], 0 ); //x_max, y_min, bottom right
			p_source->SetPoint( 0, bounds[0], bounds[2], 0 ); //x_min, y_min, bottom left
			p_source->SetPoint( 1, bounds[0], bounds[3], 0 ); //x_min, y_max, top left
			p_source->SetPoint( 2, bounds[1], bounds[3], 0 ); //x_max, y_max, top right
			p_source->SetPoint( 3, bounds[1], bounds[2], 0 ); //x_max, y_min, bottom right

			fisheyeTransform->SetSourceLandmarks( p_source );
			fisheyeTransform->SetTargetLandmarks( p_target );
			reslicer->SetResliceTransform( fisheyeTransform );
			m_slicerDataExternal->update();
		}
	}

	// magnify and unmagnify fisheye lens and distortion radius
	if (fisheyeLensActivated) {

		pickPoint(pickedData.pos, pickedData.res, pickedData.ind);
		ren->SetWorldPoint(pickedData.res[SlicerXInd(m_slicerMode)], pickedData.res[SlicerYInd(m_slicerMode)], 0, 1);
		if (!m_slicerDataExternal->getChannel(0))
			return;
		// TODO: fisheye lens on all channels???
		auto reslicer = m_slicerDataExternal->getChannel(0)->reslicer;
		if (event->modifiers().testFlag(Qt::ControlModifier)) {
			if (event->key() == Qt::Key_Minus) {

				if (fisheyeRadius <= 20 && innerFisheyeRadius + 1.0 <= 20.0) {
					innerFisheyeRadius = innerFisheyeRadius + 1.0;
					updateFisheyeTransform(ren->GetWorldPoint(), m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);

				}

				else {
					if ((innerFisheyeRadius + 2.0) <= fisheyeRadius) {
						innerFisheyeRadius = innerFisheyeRadius + 2.0;
						updateFisheyeTransform(ren->GetWorldPoint(), m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);
					}

				}
			}
			if (event->key() == Qt::Key_Plus) {

				if (fisheyeRadius <= 20 && innerFisheyeRadius - 1.0 >= 1.0) {
					innerFisheyeRadius = innerFisheyeRadius - 1.0;
					updateFisheyeTransform(ren->GetWorldPoint(), m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);

				}
				else {
					if ((innerFisheyeRadius - 2.0) >= (innerFisheyeMinRadius)) {
						innerFisheyeRadius = innerFisheyeRadius - 2.0;
						updateFisheyeTransform(ren->GetWorldPoint(), m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);

				}
				}
			}
		}
		else if (!(event->modifiers().testFlag(Qt::ControlModifier))) {
			if (event->key() == Qt::Key_Plus) {

				if (fisheyeRadius + 1.0 <= 20.0) {
					fisheyeRadius = fisheyeRadius + 1.0;
					innerFisheyeRadius = innerFisheyeRadius + 1.0;
					updateFisheyeTransform(ren->GetWorldPoint(), m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);

				}
				else {
					if (!(fisheyeRadius + 10.0 > maxFisheyeRadius)) {
						fisheyeRadius = fisheyeRadius + 10.0;
						innerFisheyeRadius = innerFisheyeRadius + 10.0;
						innerFisheyeMinRadius = innerFisheyeMinRadius + 8;
						updateFisheyeTransform(ren->GetWorldPoint(), m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);

					}
				}
			}
			if (event->key() == Qt::Key_Minus) {

				if (fisheyeRadius - 1.0 < 20.0 && fisheyeRadius - 1.0 >= minFisheyeRadius) {
					fisheyeRadius = fisheyeRadius - 1.0;
					innerFisheyeRadius = innerFisheyeRadius - 1.0;
					if (innerFisheyeMinRadius > 1.0)
						innerFisheyeMinRadius = 1.0;

					updateFisheyeTransform(ren->GetWorldPoint(), m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);

				}
				else {

					if (!(fisheyeRadius - 10.0 < minFisheyeRadius)) {
						fisheyeRadius = fisheyeRadius - 10.0;
						innerFisheyeRadius = innerFisheyeRadius - 10.0;
						innerFisheyeMinRadius = innerFisheyeMinRadius - 8;

						if (innerFisheyeRadius < innerFisheyeMinRadius)
							innerFisheyeRadius = innerFisheyeMinRadius;

						updateFisheyeTransform(ren->GetWorldPoint() /*pickedData.pos*/, m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);
					}
				}
			}
		}
	}

	if(!m_isInitialized && !fisheyeLensActivated)
	{
		iAVtkWidget::keyPressEvent(event);
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
	iAVtkWidget::keyPressEvent(event);
}


void iASlicerWidget::mousePressEvent(QMouseEvent *event)
{
	if(!m_isInitialized && !fisheyeLensActivated)
	{
		iAVtkWidget::mousePressEvent(event);
		return;
	}

	double pos[3];
	double result[4];
	int indxs[3];
	if( !pickPoint(pos, result, indxs) )
	{
		iAVtkWidget::mousePressEvent(event);
		return;
	}

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
	}
	else
	{
		emit clicked();
	}
	iAVtkWidget::mousePressEvent(event);
}


void iASlicerWidget::mouseMoveEvent(QMouseEvent *event)
{
	iAVtkWidget::mouseMoveEvent(event);
	if(!m_isInitialized && !fisheyeLensActivated)
		return;

	pickPoint(pickedData.pos, pickedData.res, pickedData.ind);

	if ( fisheyeLensActivated )
	{
		if (!m_slicerDataExternal->getChannel(0))
			return;
		// TODO: fisheye lens on all channels???
		auto reslicer = m_slicerDataExternal->getChannel(0)->reslicer;
		vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();
		ren->SetWorldPoint( pickedData.res[SlicerXInd( m_slicerMode )], pickedData.res[SlicerYInd( m_slicerMode )], 0, 1 );
		updateFisheyeTransform( ren->GetWorldPoint(), m_slicerDataExternal, reslicer, fisheyeRadius, innerFisheyeRadius);
	}

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

				if( setArbitraryProfile(arbProfPtInd, result, true) )
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
	if(!m_isInitialized && !fisheyeLensActivated)
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

void iASlicerWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		emit dblClicked();
	}
}

void iASlicerWidget::contextMenuEvent(QContextMenuEvent *event)
{
	if(!m_isInitialized)
	{
		iAVtkWidget::contextMenuEvent(event);
		return;
	}
	// is m_viewMode spline drawing m_viewMode?
	if (m_decorations && m_viewMode == DEFINE_SPLINE)
		m_contextMenu->exec(event->globalPos());

	if(m_magicLensExternal && m_magicLensExternal->IsEnabled())
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
	if (!m_slicerDataExternal->getChannel(0))
		return;
	// TODO: slice profile on selected/current channel
	auto reslicer = m_slicerDataExternal->getChannel(0)->reslicer;
	vtkImageData * reslicedImgData = reslicer->GetOutput();
	double PosY = Pos[SlicerYInd(m_slicerMode)];
	if (!m_sliceProfile->updatePosition(PosY, reslicedImgData))
		return;
	// render slice view
	GetRenderWindow()->GetInteractor()->Render();
}


bool iASlicerWidget::setArbitraryProfile(int pointInd, double * Pos, bool doClamp)
{
	if (!m_decorations)
		return false;
	if (!m_slicerDataExternal->getChannel(0))
		return false;
	// TODO: slice profile on selected/current channel
	auto reslicer  = m_slicerDataExternal->getChannel(0)->reslicer;
	auto imageData = m_slicerDataExternal->getChannel(0)->image;
	if (doClamp)
	{
		double * spacing = imageData->GetSpacing();
		double * origin  = imageData->GetOrigin();
		int * dimensions = imageData->GetDimensions();
		for (int i = 0; i < 3; ++i)
		{
			Pos[i] = clamp(origin[i], origin[i] + (dimensions[i] - 1) * spacing[i], Pos[i]);
		}
	}
	double profileCoord2d[2] = {Pos[ SlicerXInd(m_slicerMode) ], Pos[ SlicerYInd(m_slicerMode) ]};
	if( !m_arbProfile->setup(pointInd, Pos, profileCoord2d, reslicer->GetOutput()) )
		return false;
	GetRenderWindow()->GetInteractor()->Render();
	return true;
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
	double *result_out,	int &xInd_out, int &yInd_out, int &zInd_out)
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

	if (!m_slicerDataExternal->getChannel(0))
		return 0;
	// TODO: slice profile on selected/current channel
	auto reslicer = m_slicerDataExternal->getChannel(0)->reslicer;
	auto imageData = m_slicerDataExternal->getChannel(0)->image;

	// get image spacing to be able to select a point independent of zoom level
	double spacing[3];
	imageData->GetSpacing(spacing);

	vtkMatrix4x4 * resliceAxes = reslicer->GetResliceAxes();
	double point[4] = { ptMapped[0], ptMapped[1], 0, 1 }; // We have to set the physical z-coordinate which requires us to get the volume spacing.
	resliceAxes->MultiplyPoint(point, result_out);

	xInd_out = (int)(result_out[0] / spacing[0] + 0.5);
	yInd_out = (int)(result_out[1] / spacing[1] + 0.5);
	zInd_out = (int)(result_out[2] / spacing[2] + 0.5);

	// initialize m_positions depending on slice view
	switch(m_slicerMode)
	{
	default:
	case iASlicerMode::YZ: { xPos_out = m_xInd * spacing[0]; yPos_out = ptMapped[0];         zPos_out = ptMapped[1];         } break;
	case iASlicerMode::XY: { xPos_out = ptMapped[0];         yPos_out = ptMapped[1];         zPos_out = m_zInd * spacing[2]; } break;
	case iASlicerMode::XZ: { xPos_out = ptMapped[0];         yPos_out = m_yInd * spacing[1]; zPos_out = ptMapped[1];         } break;
	}

	return 1;
}


void iASlicerWidget::slicerUpdatedSlot()
{
	setCursor( m_slicerDataExternal->getMouseCursor() );
	if (m_isSliceProfEnabled)
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
	updateMagicLens();
	iAVtkWidget::resizeEvent(event);
}

void iASlicerWidget::wheelEvent(QWheelEvent* event)
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

void iASlicerWidget::setMode( iASlicerMode slicerMode )
{
	m_slicerMode = slicerMode;
}

void iASlicerWidget::menuCenteredMagicLens()
{
	if (!m_magicLensExternal) return;
	m_magicLensExternal->SetViewMode(iAMagicLens::CENTERED);
	updateMagicLens();
}

void iASlicerWidget::menuOffsetMagicLens()
{
	if (!m_magicLensExternal) return;
	m_magicLensExternal->SetViewMode(iAMagicLens::OFFSET);
	updateMagicLens();
}

void iASlicerWidget::initializeFisheyeLens(vtkImageReslice* reslicer)
{
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();

	fisheyeTransform = vtkSmartPointer<vtkThinPlateSplineTransform>::New();
	fisheyeTransform->SetBasisToR2LogR();
	p_source = vtkSmartPointer<vtkPoints>::New();
	p_target = vtkSmartPointer<vtkPoints>::New();
	p_source->SetNumberOfPoints( 32 );
	p_target->SetNumberOfPoints( 32 );
	reslicer->SetInterpolationModeToLinear(); // added while testing

	fisheye = vtkSmartPointer<vtkRegularPolygonSource>::New();
	fisheye->SetNumberOfSides( 60 );
	fisheye->GeneratePolygonOff(); // just outlines;
	fisheye->SetRadius(50.0);
	fisheyeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	fisheyeMapper->SetInputConnection( fisheye->GetOutputPort() );
	fisheyeActor = vtkSmartPointer<vtkActor>::New();
	fisheyeActor->GetProperty()->SetColor( 1.00000, 0.50196, 0.00000 );
	fisheyeActor->GetProperty()->SetOpacity( 1.0 );
	fisheyeActor->SetMapper( fisheyeMapper );
	ren->AddActor(fisheyeActor);

	// Create circle actors (green and red) to show the transform landmarks
	for ( int i = 0; i < p_target->GetNumberOfPoints(); ++i )
	{
		// Create a sphere and its associated mapper and actor.
		vtkSmartPointer<vtkRegularPolygonSource> circle = vtkSmartPointer<vtkRegularPolygonSource>::New();
		circle->GeneratePolygonOff(); // Uncomment to generate only the outline of the circle
		circle->SetNumberOfSides( 50 );
		circle->SetRadius( 1.0 * reslicer->GetOutput()->GetSpacing()[0] );

		vtkSmartPointer<vtkPolyDataMapper> circleMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		circleMapper->SetInputConnection( circle->GetOutputPort() );

		vtkSmartPointer<vtkActor> circleActor = vtkSmartPointer<vtkActor>::New();
		circleActor->GetProperty()->SetColor( 0.0, 1.0, 0.0 );
		circleActor->GetProperty()->SetOpacity( 1.0 );
		circleActor->SetMapper( circleMapper );
		circleActor->VisibilityOff(); // comment to show landmarks

		circle1List.append( circle );
		circle1ActList.append( circleActor );
		ren->AddActor( circleActor );
	}

	for ( int i = 0; i < p_source->GetNumberOfPoints(); ++i )
	{
		vtkSmartPointer<vtkRegularPolygonSource> circle = vtkSmartPointer<vtkRegularPolygonSource>::New();
		circle->GeneratePolygonOff(); // Uncomment to generate only the outline of the circle
		circle->SetNumberOfSides( 50 );
		circle->SetRadius( 3.0 * reslicer->GetOutput()->GetSpacing()[0] );

		vtkSmartPointer<vtkPolyDataMapper> circleMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		circleMapper->SetInputConnection( circle->GetOutputPort() );

		vtkSmartPointer<vtkActor> circleActor = vtkSmartPointer<vtkActor>::New();
		circleActor->GetProperty()->SetColor( 1.0, 0.0, 0.0 );
		circleActor->GetProperty()->SetOpacity( 1.0 );
		circleActor->SetMapper( circleMapper );
		circleActor->VisibilityOff(); // comment to show landmarks

		circle2List.append( circle );
		circle2ActList.append( circleActor );
		ren->AddActor( circleActor );
	}
}

void iASlicerWidget::updateFisheyeTransform(double focalPt[3], iASlicerData* slicerData, vtkImageReslice* reslicer, double lensRadius, double innerLensRadius)
{
	vtkImageData * reslicedImgData = reslicer->GetOutput();
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();

	double * spacing = reslicedImgData->GetSpacing();
	double * origin = reslicedImgData->GetOrigin();

	double bounds[6];
	reslicer->GetInformationInput()->GetBounds( bounds );

	p_target->SetNumberOfPoints( 32 ); // already set above!
	p_source->SetNumberOfPoints( 32 );
	iASlicerMode mode = slicerData->getMode();
	int sn = slicerData->getSliceNumber();

	std::cout << bounds[0] << " " << bounds[1] << " " << bounds[2] << " " << bounds[3] << " " << bounds[4] << " " << bounds[5] << std::endl;
	std::cout << focalPt[0] << " " << focalPt[1] << " " << focalPt[2] << std::endl;

	switch ( slicerData->getMode() )
	{
		case iASlicerMode::YZ:
			p_target->SetPoint( 0, sn * spacing[0], bounds[2], bounds[2] ); //x_min, y_min, bottom left // left border points
			p_target->SetPoint( 1, sn * spacing[0], bounds[2], 0.5  * bounds[5] );
			p_target->SetPoint( 2, sn * spacing[0], bounds[2], bounds[5] ); //x_min, y_max, top left // top border points
			p_target->SetPoint( 3, sn * spacing[0], 0.5  * bounds[3], bounds[5]);
			p_target->SetPoint( 4, sn * spacing[0], bounds[3], bounds[5] ); //x_max, y_max, top right // right border points
			p_target->SetPoint( 5, sn * spacing[0], bounds[3], 0.5  * bounds[5] );
			p_target->SetPoint( 6, sn * spacing[0], bounds[3], bounds[2] ); //x_max, y_min, bottom right // bottom border points
			p_target->SetPoint( 7, sn * spacing[0], 0.5  * bounds[3], bounds[2] );
			break;
		case iASlicerMode::XY:
			p_target->SetPoint( 0, bounds[0], bounds[2], sn * spacing[2] ); //x_min, y_min, bottom left // left border points
			p_target->SetPoint( 1, bounds[0], 0.5  * bounds[3], sn * spacing[2]);
			p_target->SetPoint( 2, bounds[0], bounds[3], sn * spacing[2] ); //x_min, y_max, top left // top border points
			p_target->SetPoint( 3, 0.5  * bounds[1], bounds[3], sn * spacing[2]);
			p_target->SetPoint( 4, bounds[1], bounds[3], sn * spacing[2] ); //x_max, y_max, top right // right border points
			p_target->SetPoint( 5, bounds[1], 0.5  * bounds[3], sn * spacing[2]);
			p_target->SetPoint( 6, bounds[1], bounds[2], sn * spacing[2] ); //x_max, y_min, bottom right // bottom border points
			p_target->SetPoint( 7, 0.5  * bounds[1], bounds[2], sn * spacing[2]);
			break;
		case iASlicerMode::XZ:
			p_target->SetPoint( 0, bounds[0], sn * spacing[1], bounds[4] ); //x_min, y_min, bottom left // left border points
			p_target->SetPoint( 1, bounds[0], sn * spacing[1], 0.5  * bounds[5]);
			p_target->SetPoint( 2, bounds[0], sn * spacing[1], bounds[5] ); //x_min, y_max, top left // top border points
			p_target->SetPoint( 3, 0.5  * bounds[1], sn * spacing[1], bounds[5] );
			p_target->SetPoint( 4, bounds[1], sn * spacing[1], bounds[5] ); //x_max, y_max, top right // right border points
			p_target->SetPoint( 5, bounds[1], sn * spacing[1], 0.5  * bounds[5] );
			p_target->SetPoint( 6, bounds[1], sn * spacing[1],  bounds[4] ); //x_max, y_min, bottom right // bottom border points
			p_target->SetPoint( 7, 0.5  * bounds[1], sn * spacing[1], bounds[4] );
			break;
		default:
			break;
	}

	for ( int i = 0; i < p_target->GetNumberOfPoints()- (p_target->GetNumberOfPoints()-8); ++i )
		p_source->SetPoint( i, p_target->GetPoint( i ) );

	int fixPoints = 8;
	// outer circle 1
	double fixRadiusX;
	double fixRadiusY;
	for (int fix = p_target->GetNumberOfPoints() - 8 - 8 - 8; fix < p_target->GetNumberOfPoints() - 8 - 8; fix++)
	{
			fixRadiusX = (lensRadius + 15.0)* std::cos(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];
			fixRadiusY = (lensRadius + 15.0)* std::sin(fix * (360 / fixPoints) * vtkMath::Pi() / 180) * spacing[0];

		switch (mode)
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

		switch (mode)
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
		double xCoordCircle1 = (innerLensRadius) * std::cos(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];
		double yCoordCircle1 = (innerLensRadius) * std::sin(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];

		double xCoordCircle2 = (lensRadius) * std::cos(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];
		double yCoordCircle2 = (lensRadius) * std::sin(i * (360 / pointsCount) * vtkMath::Pi() / 180) * spacing[0];

		switch (mode)
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
	for ( int i = 0; i < p_target->GetNumberOfPoints(); ++i )
	{
		if ( mode == iASlicerMode::YZ )
		{
			circle1List.at( i )->SetCenter( p_target->GetPoint( i )[1], p_target->GetPoint( i )[2], 0.0 );
			circle2List.at( i )->SetCenter( p_source->GetPoint( i )[1], p_source->GetPoint( i )[2], 0.0 );
		}
		if ( mode == iASlicerMode::XY )
		{
			circle1List.at( i )->SetCenter( p_target->GetPoint( i )[0], p_target->GetPoint( i )[1], 0.0);
			circle2List.at( i )->SetCenter( p_source->GetPoint( i )[0], p_source->GetPoint( i )[1], 0.0);
		}

		if ( mode == iASlicerMode::XZ )
		{
			circle1List.at( i )->SetCenter( p_target->GetPoint( i )[0], p_target->GetPoint( i )[2], 0.0 );
			circle2List.at( i )->SetCenter( p_source->GetPoint( i )[0], p_source->GetPoint( i )[2], 0.0 );
		}
	}

	fisheye->SetCenter( focalPt[0], focalPt[1], 0.0 );
	fisheye->SetRadius( lensRadius * reslicer->GetOutput()->GetSpacing()[0] );

	fisheyeTransform->SetSourceLandmarks(p_source); // red
	fisheyeTransform->SetTargetLandmarks(p_target);  // green

	reslicer->SetResliceTransform( fisheyeTransform );
	reslicer->Update();
	slicerData->update();
}

void iASlicerWidget::updateMagicLens()
{
	if (!m_magicLensExternal || !m_magicLensExternal->IsEnabled())
		return;
	vtkRenderer * ren = GetRenderWindow()->GetRenderers()->GetFirstRenderer();
	ren->SetWorldPoint(pickedData.res[ SlicerXInd(m_slicerMode) ], pickedData.res[ SlicerYInd(m_slicerMode) ], 0, 1);
	ren->WorldToDisplay();
	double dpos[3];
	ren->GetDisplayPoint(dpos);
	int lensSz = m_magicLensExternal->GetSize();
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
	m_magicLensExternal->UpdatePosition(GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera(), worldP, mousePos);
}


void iASlicerWidget::computeGlyphs()
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
	*/
}

void iASlicerWidget::setPieGlyphParameters( double opacity, double spacing, double magFactor )
{
	m_pieGlyphOpacity = opacity;
	m_pieGlyphSpacing = spacing;
	m_pieGlyphMagFactor	= magFactor;
	computeGlyphs();
}
