// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkInteractorStyleTrackballActor.h>
#include <vtkSmartPointer.h>

#include <QObject>

class iAChannelSlicerData;
class iADataSetRenderer;

class vtkImageData;
class vtkImageReslice;
class vtkProp3D;
class vtkTransform;

enum transformationMode
{
	x,
	y,
	z
};

//! interactor style enabling the manual registration of objects in slicer and 3D renderer.
class iAvtkInteractStyleActor : public QObject, public vtkInteractorStyleTrackballActor
{
	Q_OBJECT
public:
	static iAvtkInteractStyleActor* New();
	vtkTypeMacro(iAvtkInteractStyleActor, vtkInteractorStyleTrackballActor);

	//! override the mouse move, we add some behavior here
	void OnMouseMove() override;

	//! @{ Conditionally disable zooming via right button dragging
	void Rotate() override;
	void Spin() override;
	//! @}

	void initialize(vtkImageData* img, iADataSetRenderer* dataSetRenderer, iAChannelSlicerData* slicerChannel[3], int currentMode);

signals:
	void actorsUpdated();

private:
	//! Default constructor - only callable via ::New().
	iAvtkInteractStyleActor();

	Q_DISABLE_COPY_MOVE(iAvtkInteractStyleActor);

	iADataSetRenderer* m_dataSetRenderer;
	bool m_is3D;  //!< true if style assigned to 3D renderer, false if assigned to a slicer
	vtkImageData* m_image;
	iAChannelSlicerData* m_slicerChannel[3];
	vtkSmartPointer<vtkTransform> m_transform3D;

	vtkSmartPointer<vtkTransform> m_sliceInteractorTransform[3];  //!< transform for each interactor of slicer
	vtkSmartPointer<vtkTransform> m_reslicerTransform[3];         //!< transform for each reslicer

	double m_imageSpacing[3];
	double m_imageRefOrientation[3];
	int m_currentSliceMode;
	bool m_rotationEnabled;
	double m_currentSliceActorPosition[3];   //!< position of the currentActor of slicer
	double m_currentVolRendererPosition[3];  //!< position of the currentActor of displayed volume

	void setPreviousSlicesActorPosition(double const* pos);
	void setPreviousVolActorPosition(double const* pos);

	//! calculates relative rotation angle in xy of specified slicer
	void computeDisplayRotationAngle(
		double* sliceProbCenter, double* disp_obj_center, vtkRenderWindowInteractor* rwi, double& relativeAngle);

	//! set slicer translation
	void set2DTranslation(
		vtkSmartPointer<vtkTransform>& transform, vtkImageReslice* reslice, double const* position, double* spacing);

	//! set translation of 3D prop
	void set3DTranslation(double const* relMovementXYZ, double const* sliceActorPos);

	//! set slicer orientation
	void set2DOrientation(vtkSmartPointer<vtkTransform>& transform, vtkImageReslice* reslicer, transformationMode mode,
		double const* center, double angle, double const* spacing);

	//! set orientation of 3D prop
	void set3DOrientation(
		vtkSmartPointer<vtkTransform>& transform, double const* center, double angle, vtkProp3D* prop, uint mode);

	void prepareMovementCoords(double* movement, double const* sliceActorPos, bool relativeMovement);

	//! update reslicer
	void prepareReslicer(vtkImageReslice* reslicer, vtkSmartPointer<vtkTransform> transform, double const* spacing);

	//! rotate 3d in hope first Rotate x then rotate y then rotate Z
	//! mode 0 -> x Rotation
	//! mode 1 x and y Rotation
	//! mode 2 x, y, z Rotation, -> first rotate x, then y, then z
	void rotateReslicerXYZ(vtkSmartPointer<vtkTransform> transform, vtkImageReslice* reslcier, double const* rotXYZ,
		uint rotationMode, double const* center, double const* spacing);

	//! perform rotation of transform around an axis by angle
	void rotateAroundAxis(
		vtkSmartPointer<vtkTransform>& transform, double const* center, transformationMode mode, double angle);

	void setRefOrientation(double const* orientation);

	//! @param mode -> iASlicerMode
	vtkImageReslice* reslicer(uint mode);

	//! @param mode -> iASlicerMode
	vtkSmartPointer<vtkTransform> resliceTransform(uint mode);

	//! called when a rotation is performed in the 2D slicer
	void rotate2D();

	//! called when a rotation is performed in the 3D renderer
	void rotate3D();

	//! called when a translation is performed
	void translate();
};
