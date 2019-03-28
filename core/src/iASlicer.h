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
#include "iASlicerMode.h"

#include <vtkSmartPointer.h>

#include <QObject>
#include <QSharedPointer>

class QWidget;
class QFrame;

class vtkAbstractTransform;
class vtkCamera;
class vtkGenericOpenGLRenderWindow;
class vtkImageActor;
class vtkPoints;
class vtkRenderer;

class iAChannelData;
class iAChannelSlicerData;
class iAMagicLens;
class iAMultiChannelVisualization;
class iASingleSlicerSettings;
class iASlicerData;
class iASlicerWidget;

static const int MODE_TO_X_IND[3]	= { 1, 0, 0 };
static const int MODE_TO_Y_IND[3]	= { 2, 1, 2 };
static const int MODE_TO_Z_IND[3]	= { 0, 2, 1 };

//! A container which combines widget and vtk classes for a slicer.
//!
//! This class keeps all the functionality of the slicer
//! and is responsible for linking all the parts together
//! and managing all the communication and dependencies.
class open_iA_Core_API iASlicer : public QObject
{
	Q_OBJECT
public:
	iASlicer(QWidget * parent, const iASlicerMode mode, QWidget * widget_container, bool decorations = true, bool magicLensAvailable = true, vtkAbstractTransform *tr = nullptr, vtkPoints* snakeSlicerPoints = nullptr);

	void update();
	iASlicerWidget * widget();
	iASlicerData * data();
	void changeMode(const iASlicerMode mode);
	iASlicerMode getMode() const;

	//! @{ Magic Lens methods
	void setMagicLensEnabled( bool isEnabled );
	void setMagicLensSize(int newSize);
	int  getMagicLensSize() const;
	void setMagicLensFrameWidth(int newWidth);
	void setMagicLensCount(int count);
	void setMagicLensInput(uint id);
	void addMagicLensInput(uint id);
	uint getMagicLensInput() const;
	void setMagicLensOpacity(double opacity);
	double getMagicLensOpacity() const;
	void updateMagicLensColors();
	iAMagicLens * magicLens();
	//! @}

	bool changeInteractorState();
	//iASlicerData: wrapping methods--------------------------
	void disableInteractor();
	void enableInteractor(); //also updates widget

	//! @{ management of channels - each channel represents one "layer"
	void addChannel(uint id, iAChannelData & chData );
	void removeChannel(uint id);
	void updateChannel(uint id, iAChannelData & chData );
	iAChannelSlicerData * getChannel(uint id);
	void setResliceChannelAxesOrigin(uint id, double x, double y, double z);
	//! @}

	// { TODO: check whether these can be removed somehow!
	void addImageActor(vtkSmartPointer<vtkImageActor> imgActor);
	void removeImageActor(vtkSmartPointer<vtkImageActor> imgActor);
	// }

	//! @{ ROI rectangle
	void setROIVisible(bool isVisible);
	void updateROI(int const roi[6]);
	//! @}

	void setPositionMarkerCenter(double x, double y);
	void saveMovie(QString& fileName, int qual = 2);
	void saveImageStack();
	void setResliceAxesOrigin(double x, double y, double z);
	void setup(iASingleSlicerSettings const & settings);
	vtkRenderer * getRenderer() const;
	vtkGenericOpenGLRenderWindow * getRenderWindow() const;
	void setStatisticalExtent(int statExt);

	//iASlicerWidget: wrapping methods-----------------------
	void setIndex( int x, int y, int z );
	//void initializeWidget(vtkImageData *imageData, vtkPoints *points = 0);//also connects to mdichild slots
	void show();

	// { TODO: Move to XRF
	void setPieGlyphsOn(bool isOn);
	void setPieGlyphParameters(double opacity, double spacing, double magFactor );
	// }

	void setChannelOpacity( uint id, double opacity );
	void enableChannel( uint id, bool enabled, double x, double y, double z );
	void enableChannel( uint id, bool enabled );
	//void switchContourSourceToChannel( uint id );
	void showIsolines( bool s );
	void setContours( int n, double mi, double ma );
	void setContours( int n, double * contourValues );
	vtkCamera* getCamera();
	void setCamera(vtkCamera*, bool owner=true);
	void setBackground(double r, double g, double b);

public slots:
	void saveAsImage() const;
	void setSliceNumber( int sliceNumber );
	void saveMovie();
	void rotateSlice( double angle );
	void setSlabThickness(int thickness);
	void setSlabCompositeMode(int compositeMode);

private:
	QSharedPointer<iASlicerData> m_data;
	QSharedPointer<iASlicerWidget> m_widget;
	iASlicerMode m_mode;

	uint m_magicLensInput;
	QSharedPointer<iAMagicLens> m_magicLens;
};

//Get index of slicer X screen coordinate in global 3D coordinate system
inline const int SlicerXInd(const iASlicerMode & slicerMode)
{
	return MODE_TO_X_IND[slicerMode];
}
//Get index of slicer Y screen coordinate in global 3D coordinate system
inline const int SlicerYInd(const iASlicerMode & slicerMode)
{
	return MODE_TO_Y_IND[slicerMode];
}

//Get index of slicer Z coordinate in global 3D coordinate system
inline const int SlicerZInd(const iASlicerMode & slicerMode)
{
	return MODE_TO_Z_IND[slicerMode];
}
