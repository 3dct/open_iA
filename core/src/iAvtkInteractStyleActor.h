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
class vtkProp3D; 
class vtkImageReslice; 

class vtkImageData;
class vtkTransform; 
class vtkLineSource;
//TODO REmove
class vtkTransformFilter;
class vtkCubeSource;
//class vtkPlaneSource;
class vtkPolyDataMapper; 
class vtkSphereSource; 
class vtkTransform; 
class vtkImageActor; 
class vtkImageReslice;

enum transformationMode
{
	x, y, z
};

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
	
	void OnLeftButtonDown() override; 

	//update interactors of slicers, for translation
	void updateInteractors();

	//input relative movement of actor -> xyz
	//actor position for debugging
	void update3DUpdateReslicer(double const * relMovementXYZ, double const * sliceActorPos);

	/*void TranslateActor(double * movement, uint mode);*/

	void reset(); 
	

	
	void TranslateActor(double const * movement, uint mode);
	void performTranslationTransform(vtkSmartPointer<vtkTransform> &transform, vtkImageActor *actor, double const *relMovement, uint mode);
	
	//movent = newPos - oldPos
	void prepareMoventCoords(double * movement, double const * sliceActorPos, bool relativeMovement);
	
	//rotates 2d slicer/ interactor
	void rotate2D(); 
	
	void rotate3D(); //3d rotation of the volume actor //TODO 
	//void UpdateReslicerTranslateTransform2D(double *const Rendposition, /*const double *orientation,*/ const double *imageCenter, int sliceMode);
	

	//probably take a transform set to origin, then translate based on slice mode
	//updates interactor for 3d volume according to angle and axis
	void Update3DTransform(const double * imageCenter, const double * spacing, double relativeAngle);

	/*void TransformReslicerExperimental(double const * obj_center, double rotationAngle, double const *spacing, int sliceMode);
*/

	// transformation of the reslicer rotation based on a slice mode
	//void updateReslicerRotationTransformation2d(const int sliceMode, double * ofs, const int sliceNumber);
	

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

	//used to update the slicer
	vtkSmartPointer<vtkTransform> m_SliceInteractorTransform[3];  //transform for each interactor of slicer
	vtkSmartPointer<vtkTransform> m_ReslicerTransform[3]; //transform for each reslicer

	double m_imageSpacing[3];
	//double imageCenter[3]; 

	int m_currentSliceMode;
	bool m_rightButtonDragZoomEnabled = false;
	bool m_rotationEnabled; 
	bool m_rotation3DEnabled = false; 
	double m_currentSliceActorPosition[3]; //< position of the currentActor of slicer
	double m_currentVolRendererPosition[3]; //< position of the currentActor of displayed volume

	inline void setPreviouSlicesActorPosition(double const *pos) {
		m_currentSliceActorPosition[0] = pos[0]; 
		m_currentSliceActorPosition[1] = pos[1];
		m_currentSliceActorPosition[2] = pos[2]; 
 	}

	inline void setPreviousVolActorPosition(double const *pos) {
		m_currentVolRendererPosition[0] = pos[0];
		m_currentVolRendererPosition[1] = pos[1];
		m_currentVolRendererPosition[2] = pos[2];
	}

	//! @{ disable copying
	void operator=(const iAvtkInteractStyleActor&) = delete;
	iAvtkInteractStyleActor(const iAvtkInteractStyleActor &) = delete;
	//! @}

	void computeDisplayRotationAngle(double * sliceProbCenter, double * disp_obj_center, vtkRenderWindowInteractor * rwi, double &relativeAngle);
    
	void TranslateReslicer(vtkSmartPointer<vtkTransform> &transform, vtkImageReslice *reslice, double const *position, double *spacing, double const * imageCenter);
	
		
	/*mode x: RotateX(angle); 
	mode y: RotateY(angle); 
	mode Z: RotateZ(angle);*/
	//void ReslicerRotate(vtkSmartPointer<vtkTransform> &transform, vtkImageReslice *reslicer, uint sliceMode, double const * center, double angle, double const *spacing);
	

	//void ReslicerRotate(vtkSmartPointer<vtkTransform> &transform, vtkImageReslice *reslicer, rotationMode mode /*uint sliceMode*/, double const * center, double angle, double const *spacing);
	void ReslicerRotate(vtkSmartPointer<vtkTransform> &transform, vtkImageReslice *reslicer, transformationMode mode, double const *rotXYZ, double const * center, double angle, double const *spacing);
	//just a cube source for visualisation
	//for debugging / visualisation
	void initializeAndRenderPolyData(uint thickness); 

	//rotates a prop by a vtk transform, works fine	   
	void rotateInterActorProp(vtkSmartPointer<vtkTransform> &transform, double const *center, double angle, vtkProp3D *prop, uint mode);

	
	void translateInterActor(vtkSmartPointer<vtkTransform> &transform, vtkImageActor *actor, double const *position, uint mode);
	void TranslateActorMovement(vtkImageActor * actor, uint mode, vtkSmartPointer<vtkTransform> & transform, double const * position);
	
	//perform rotation of transform around an axis by angle
	//void rotateAroundAxis(vtkSmartPointer<vtkTransform> &transform, double const * center, uint mode, double angle);

	
	void rotateAroundAxis(vtkSmartPointer<vtkTransform> & transform, double const * center, transformationMode mode/*uint mode*/, double angle);
	//reslicer only for 3d rotation
	//void rotateReslicer(vtkSmartPointer<vtkTransform> &transform, vtkImageReslice *reslicer, double const *center, uint mode, double angle);
	/*mode 0: X, mode 1: Y:, mode 2:  z
	* reference object for plane ... 
	*/
	void createReferenceObject(double /*const */* center, double const *spacing, uint thickness, const double *bounds, uint mode);
	
	void rotateXYZ(vtkSmartPointer<vtkTransform> &transform, double const *center, double const *rotationWXYZ);
		
	void createAndInitLines(double const *bounds, double const * center);
	void initLine(vtkSmartPointer<vtkLineSource> &line, vtkSmartPointer<vtkActor>& lineActor, double const * center, double min, double max, uint sliceMode);
	
	void translatePolydata(vtkSmartPointer<vtkTransform> &polTransform, vtkSmartPointer<vtkActor> &polyActor, double X, double Y, double Z);
	
	//mode 0: rotateX, mode 1: rotate Y:, mode 2: rotate z
	//************************************
	// rotating polydata
	// Parameter: vtkSmartPointer<vtkTransform> polTransform
	// Parameter: vtkSmartPointer<vtkActor> & polyActor
	// Parameter: const double * center
	// Parameter: double angle
	// Parameter: uint mode
	//************************************
	void rotatePolydata(vtkSmartPointer<vtkTransform> &polTransform, vtkSmartPointer<vtkActor> &polyActor, const double *center, double angle, uint mode);
	vtkSmartPointer<vtkCubeSource> m_CubeSource_X;
	vtkSmartPointer<vtkPolyDataMapper> m_cubeMapper;
	vtkSmartPointer<vtkActor> m_cubeActor;
	vtkSmartPointer<vtkTransform> m_cubeXTransform; 
	vtkSmartPointer<vtkSphereSource> m_SphereSourceCenter;
	vtkSmartPointer<vtkPolyDataMapper> m_SphereMapper;
	vtkSmartPointer<vtkActor> m_SphereActor;

	vtkSmartPointer<vtkCubeSource> m_RefCubeSource;
	vtkSmartPointer<vtkPolyDataMapper> m_RefCubeMapper;
	vtkSmartPointer<vtkActor> m_RefCubeActor;
	vtkSmartPointer<vtkTransform> m_RefTransform; 

	/*vtkSmartPointer<vtkLineSource> m_RefLine[3]; 
	vtkSmartPointer<vtkActor> m_RefLineActor[3];
	vtkSmartPointer<vtkPolyDataMapper> m_RefLineMapper[3];*/
	

	//end for debugging; 
};
