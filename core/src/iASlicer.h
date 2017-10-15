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
#pragma once

#include "iAChannelID.h"
#include "open_iA_Core_export.h"
#include "iASlicerMode.h"

#include <vtkSmartPointer.h>

#include <QObject>
#include <QSharedPointer>

class QWidget;
class QFrame;
class QGLWidget;

class vtkCamera;
class vtkColorTransferFunction;
class vtkImageActor;
class vtkImageData;
class vtkImageReslice;
class vtkGenericOpenGLRenderWindow;
class vtkPoints;
class vtkRenderer;
class vtkScalarsToColors;
class vtkTransform;

class iAChannelVisualizationData;
class iAChannelSlicerData;
class iAMagicLens;
class iAMultiChannelVisualization;
class iASingleSlicerSettings;
class iASlicerData;
class iASlicerWidget;

static const int MODE_TO_X_IND[3]	= { 1, 0, 0 }; 
static const int MODE_TO_Y_IND[3]	= { 2, 1, 2 }; 
static const int MODE_TO_Z_IND[3]	= { 0, 2, 1 }; 

/**
 * \brief	A container which combines classes for a slicer. Combines a special widget and vtk classes.
 * 
 * This class keeps all the functionality of the slicer 
 * and is responsible for linking all the parts together 
 * and managing all the communication and dependencies.
 */
class open_iA_Core_API iASlicer : public QObject
{
	Q_OBJECT
	friend class iASlicerData;
	friend class iASlicerWidget;
public:
	iASlicer(QWidget * parent, const iASlicerMode mode, QWidget * widget_container, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0,
		bool decorations = true, bool magicLensAvailable = true);
	~iASlicer();
	bool changeInteractorState();
	iASlicerWidget * widget() const;
	void ChangeMode(const iASlicerMode mode);
	iASlicerMode GetMode() const;
	void changeImageData(vtkImageData *idata);
	void SetMagicLensEnabled( bool isEnabled );
	void SetMagicLensSize(int newSize);
	void SetMagicLensFrameWidth(int newWidth);
	void SetMagicLensCount(int count);
	void SetMagicLensInput( iAChannelID id );
	void AddMagicLensInput(iAChannelID id);
	void SetMagicLensOpacity(double opacity);
	void UpdateMagicLensColors();
	
	//iASlicerData: wrapping methods--------------------------
	void disableInteractor(); 
	void enableInteractor(); //also updates widget
	void initializeData( vtkImageData *ds, vtkTransform *tr, vtkColorTransferFunction* ctf, bool sil = false, bool sp = false);
	void reInitialize(	vtkImageData *ds, 
						vtkTransform *tr, 
						vtkColorTransferFunction* ctf, 
						bool sil = false, 
						bool sp = false );

	void initializeChannel( iAChannelID id, iAChannelVisualizationData * chData );
	void removeChannel(iAChannelID id);
	void reInitializeChannel( iAChannelID id, iAChannelVisualizationData * chData );	
	void setResliceChannelAxesOrigin( iAChannelID id, double x, double y, double z);
	iAChannelID getMagicLensInput() const;

	void AddImageActor(vtkSmartPointer<vtkImageActor> imgActor);
	void RemoveImageActor(vtkSmartPointer<vtkImageActor> imgActor);

	void setPositionMarkerCenter(double x, double y);
	void setROI(int r[6]);
	void setROIVisible(bool isVisible);
	void updateROI();
	void update();
	void saveMovie(QString& fileName, int qual = 2);
	void saveImageStack();
	vtkImageReslice * GetReslicer() const;
	void setResliceAxesOrigin(double x, double y, double z);
	void setup(iASingleSlicerSettings const & settings);
	vtkRenderer * GetRenderer() const;
	vtkGenericOpenGLRenderWindow * GetRenderWindow() const;
	vtkImageData* GetImageData() const;
	void setStatisticalExtent(int statExt);

	//iASlicerWidget: wrapping methods-----------------------
	void setIndex( int x, int y, int z );
	void initializeWidget(vtkImageData *imageData, vtkPoints *points = 0);//also connects to mdichild slots
	void show();
	void setSliceProfileOn(bool isOn);
	void setArbitraryProfileOn(bool isOn);
	void setPieGlyphsOn(bool isOn);
	void setPieGlyphParameters( double opacity, double spacing, double magFactor );
	void setChannelOpacity( iAChannelID id, double opacity );
	void enableChannel( iAChannelID id, bool enabled, double x, double y, double z );
	void enableChannel( iAChannelID id, bool enabled );
	void switchContourSourceToChannel( iAChannelID id );
	void showIsolines( bool s );
	void setContours( int n, double mi, double ma );
	void setContours( int n, double * contourValues );
	iAChannelSlicerData * GetChannel( iAChannelID id );
	vtkCamera* GetCamera();
	void SetCamera(vtkCamera*, bool owner=true);

	iASlicerData * GetSlicerData();

	void SetBackground(double r, double g, double b);

public slots:
	void saveAsImage() const;
	void setSliceNumber( int sliceNumber );
	void saveMovie();
	void rotateSlice( double angle );

protected:
	iASlicerData * m_data;
	iASlicerWidget * m_widget;
	
	iASlicerMode m_mode;
	iAChannelID m_magicLensInput;

private:
	QSharedPointer<iAMagicLens> m_magicLens;
protected:
	iAMagicLens * magicLens() const	{	return m_magicLens.data();	}

protected:
	void ConnectWidgetAndData();
	void ConnectToMdiChildSlots();
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
