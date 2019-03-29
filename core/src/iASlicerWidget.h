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
#pragma once

#include "open_iA_Core_export.h"
#include "iASlicer.h"
#include "iAVtkWidget.h"

#include <QSharedPointer>
#include <QGridLayout>

class QMenu;
struct iASlicerProfile;
struct iAArbitraryProfileOnSlicer;
struct PickedData;
class iASnakeSpline;
class iAMagicLens;
class iAPieChartGlyph;

class vtkActor;
class vtkImageReslice;
class vtkParametricFunctionSource;
class vtkPoints;
class vtkPolyDataMapper;
class vtkRegularPolygonSource;
class vtkThinPlateSplineTransform;

class open_iA_Core_API iASlicerWidget : public iAVtkWidget
{
	Q_OBJECT
public:
	enum viewModeEnum{
		NORMAL = 0,
		DEFINE_SPLINE = 1,
		SHOW = 2
	};

protected:
	iAMagicLens					* m_magicLensExternal;
	QMenu						* m_magicLensContextMenu;
	//QImage img;

	QMenu						* m_contextMenu;

	bool						m_isInitialized;
	viewModeEnum				m_viewMode;						//!< current edit mode
	bool						m_isSliceProfEnabled;			//!< if slice profile mode is enabled
	bool						m_isArbProfEnabled;				//!< if arbitrary profile mode is enabled
	iASlicerMode				m_slicerMode;					//!< which slice viewer
	int							m_xInd, m_yInd, m_zInd;

	iASnakeSpline				* m_snakeSpline;
	vtkPoints					* m_worldSnakePointsExternal;
	iASlicerProfile				* m_sliceProfile;				//!< necessary vtk classes for the slice profile
	iAArbitraryProfileOnSlicer	* m_arbProfile;
	iASlicerData				* m_slicerDataExternal;

	// { TODO: move to XRF module
	bool						m_pieGlyphsEnabled;				//!< if slice pie glyphs for xrf are enabled
	QVector<QSharedPointer<iAPieChartGlyph> >	m_pieGlyphs;
	double										m_pieGlyphMagFactor;
	double										m_pieGlyphSpacing;
	double										m_pieGlyphOpacity;
	// }

	static const int			RADIUS = 5;
	QGridLayout * m_layout;

public:
	iASlicerWidget(iASlicer * slicerMaster, bool decorations = true);
	~iASlicerWidget();
	
	void	initialize(vtkPoints * snakeSlicerPoints = nullptr);
	void	setIndex( int x, int y, int z ) { m_xInd = x; m_yInd = y; m_zInd = z; };
	void	setMode(iASlicerMode slicerMode);
	void	updateMagicLens();
	// {
	// TODO: Move to XRF module!
	void	computeGlyphs();
	void	setPieGlyphParameters( double opacity, double spacing, double magFactor );
	// }
protected:
	void	updateProfile();
	int		pickPoint( double * pos_out, double * result_out, int * ind_out);
	int		pickPoint( double & xPos_out, double & yPos_out, double & zPos_out,
					   double * result_out,
					   int & xInd_out, int &yInd_out, int &zInd_out);

protected:			//overloaded events of QWidget
	virtual void keyPressEvent ( QKeyEvent * event );
	virtual void mousePressEvent ( QMouseEvent * event );
	virtual void mouseMoveEvent ( QMouseEvent * event );
	virtual void mouseReleaseEvent ( QMouseEvent * event );
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void contextMenuEvent ( QContextMenuEvent * event );
	virtual void resizeEvent ( QResizeEvent * event );
	virtual void wheelEvent(QWheelEvent*);

public slots:

	//! Sets a profile line.
	void setSliceProfile(double Pos[3]);

	//! Sets coordinates for line profile
	bool setArbitraryProfile(int pointInd, double * Pos, bool doClamp=false);

	//! Moves a point of the snake slicer to a new position.
	void movePoint(size_t selectedPointIndex, double xPos, double yPos, double zPos);

	//! Function to deselect points in snake slicer (necessary to avoid endless loops with signals and slots).
	void deselectPoint();

	//! Function to switch slice view modi and to change visibility of spline snakeDisks.
	//! @param	mode	Mode which should be switched to.
	void switchMode(int mode);
	void setSliceProfileOn(bool isOn);
	void setArbitraryProfileOn(bool isOn);

	// TODO: Move to XRF module!
	void setPieGlyphsOn(bool isOn);

	//! Adds a new spline point to the end of the spline curve.
	void addPoint(double x, double y, double z);
	//! Deletes the current spline curve.
	void deleteSnakeLine();
	//! Called when the delete snake line menu is clicked.
	void menuDeleteSnakeLine();

	void clearProfileData();
	void slicerUpdatedSlot();
	void menuCenteredMagicLens();
	void menuOffsetMagicLens();

signals:
	void addedPoint(double x, double y, double z);
	void movedPoint(size_t selectedPointIndex, double xPos, double yPos, double zPos);
	void arbitraryProfileChanged(int pointInd, double * Pos);
	void deselectedPoint();
	void switchedMode(int mode);
	void deletedSnakeLine();
	void shiftMouseWheel(int angle);
	void altMouseWheel(int angle);
	void ctrlMouseWheel(int angle);
	void clicked();
	void dblClicked();

private:
	bool m_decorations;

	//! @{ fish-eye lens
	void initializeFisheyeLens(vtkImageReslice* reslicer);
	void updateFisheyeTransform(double focalPt[3], iASlicerData *slicerData, vtkImageReslice* reslicer, double lensRadius, double innerLensRadius);

	bool fisheyeLensActivated;
	double fisheyeRadius = 80.0; // 110.0;
	double fisheyeRadiusDefault = 80.0;
	double minFisheyeRadius = 2.0;
	double maxFisheyeRadius = 220.0;
	double innerFisheyeRadius =  70.0; // 86
	double innerFisheyeRadiusDefault = 70.0;
	double innerFisheyeMinRadius = 58; // for default radius 70.0

	// variables for transformation
	vtkSmartPointer<vtkThinPlateSplineTransform> fisheyeTransform;
	vtkSmartPointer<vtkPoints> p_source;
	vtkSmartPointer<vtkPoints> p_target;
	// variables for lens appearance
	vtkSmartPointer<vtkRegularPolygonSource> fisheye;
	vtkSmartPointer<vtkPolyDataMapper> fisheyeMapper;
	vtkSmartPointer<vtkActor> fisheyeActor;

	QList<vtkSmartPointer<vtkRegularPolygonSource>> circle1List;
	QList<vtkSmartPointer<vtkActor>> circle1ActList;
	QList<vtkSmartPointer<vtkRegularPolygonSource>> circle2List;
	QList<vtkSmartPointer<vtkActor>> circle2ActList;
	//! @}
};
