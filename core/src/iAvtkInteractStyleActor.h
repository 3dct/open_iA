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

#include <vtkInteractorStyleTrackballActor.h>
#include <vtkSmartPointer.h>
//#include <vtkTransform.h>

#include <QObject>

class iAChannelSlicerData;
class iAVolumeRenderer;
class MdiChild;

class vtkImageData;
class vtkTransform; 

class vtkTransformFilter;

class iAvtkInteractStyleActor : public QObject, public vtkInteractorStyleTrackballActor
{
	Q_OBJECT
public:

	static iAvtkInteractStyleActor *New();
	vtkTypeMacro(iAvtkInteractStyleActor, vtkInteractorStyleTrackballActor);

	// override the mouse move, we add some behavior here
	void OnMouseMove() override;

	//! @{ Conditionally disable zooming via right button dragging
	void Rotate() override;
	void Spin() override;
	//! @}

	void initialize(vtkImageData *img, iAVolumeRenderer* volRend, iAChannelSlicerData *slicerChannel[4],
		int currentMode, MdiChild *mdiChild);
	
	//update interactors of slicers, for translation
	void updateInteractors(); 
	
	//rotates 2d slicer/ interactor
	void rotate2D(); 

	void UpdateReslicerTransform2D(double *const Rendposition, const double *orientation, const double *imageCenter);
	

	//probably take a transform set to origin, then translate based on slice mode
	
	//void translateSlicerActor(const double *pos, const int sliceMode);
	void translateSlicerActor(const double *origin, const double *pos, double *posOut, const int sliceMode);
	//updates interactor for 3d volume according to angle and axis
	void Update3DTransform(const double * imageCenter, const double * spacing, double relativeAngle);

	void TransformReslicerExperimental(double * obj_center, double rotationAngle);


	// transformation of the reslicer rotation based on a slice mode
	void updateReslicerRotationTransformation(const int sliceMode, double * ofs, const int sliceNumber);
	

signals:
	void actorsUpdated();

private:
	iAvtkInteractStyleActor();

	MdiChild *m_mdiChild; 
	iAVolumeRenderer* m_volumeRenderer;
	bool enable3D;
	vtkImageData *m_image;
	iAChannelSlicerData* m_slicerChannel[3];
	vtkSmartPointer<vtkTransform> m_transform3D;
	vtkSmartPointer<vtkTransform> m_sliceTranslationTransform[3]; 

	vtkSmartPointer<vtkTransformFilter> m_transformFilter;

	//probably used to update the slicer
	vtkSmartPointer<vtkTransform> m_SliceRotateTransform;

	int m_currentSliceMode;
	bool m_rightButtonDragZoomEnabled = false;
	bool m_rotationEnabled; 


	//! @{ disable copying
	void operator=(const iAvtkInteractStyleActor&) = delete;
	iAvtkInteractStyleActor(const iAvtkInteractStyleActor &) = delete;
	//! @}
};
