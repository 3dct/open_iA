// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifndef OPENXR_AVAILABLE

#include "iAvtkVR.h"

#include <openvr.h>

#include <vtkSmartPointer.h>

class vtkActor;
class vtkExtractVOI;
class vtkFloatArray;
class vtkImageData;
class vtkOpenVRRenderer;
class vtkRenderer;
class vtkTexture;

class iAVRFrontCamera
{
public:
	iAVRFrontCamera(vtkRenderer* renderer, iAvtkVRRenderWindow* renderWindow);
	~iAVRFrontCamera();

	/** Initialize the tracked camera */
	bool initialize();

	/*Build representation */
	bool buildRepresentation();

	//void switch Background ?

	bool refreshImage();

private:

	/** Camera frame parameters */
	uint32_t				m_cameraFrameWidth;
	uint32_t				m_cameraFrameHeight;
	uint32_t				m_cameraFrameBufferSize;
	uint8_t*				m_cameraFrameBuffer;

	uint32_t				m_lastFrameSequence;

	iAvtkVRRenderWindow* m_renderWindow;
	/** Vive System */
	vr::IVRSystem* m_pHMD;

	/** Tracked Camera (or front camera) */
	vr::IVRTrackedCamera* m_VRTrackedCamera;

	/*Type of frame from the Tracked Camera : distorted/Undistorted/MaximumUndistorted*/
	vr::EVRTrackedCameraFrameType m_frameType;

	//get the frame header only
	vr::CameraVideoStreamFrameHeader_t m_frameHeader;

	/*The tracked Camera has a unique TrackedCameraHandle_t
	* This handle is used to set attributes, receive events, (and render?).
	* These are several circumstances where the tracked camera isn't detected or invalid.
	* In those case the handle will be equal to INVALID_TRACKED_CAMERA_HANDLE */
	vr::TrackedCameraHandle_t m_VRTrackedCameraHandle;

	vtkSmartPointer<vtkOpenVRRenderer> m_backgroundRenderer;

	/** Image source to fill and return for display */
	vtkSmartPointer<vtkImageData> m_sourceImage;
	vtkSmartPointer<vtkImageData> m_leftImage;
	vtkSmartPointer<vtkImageData> m_rightImage;

	vtkSmartPointer<vtkTexture> m_sourceTexture;
	vtkSmartPointer<vtkTexture> m_leftTexture;
	vtkSmartPointer<vtkTexture> m_rightTexture;
	vtkSmartPointer<vtkFloatArray> m_textureCoordinates;

	vtkSmartPointer<vtkRenderer> m_renderer;

	bool getFrameSize();
	void allocateImages();
	void loadVideoStream();
	void createImage();
	void createLeftAndRightEyeImage();

	//void saveImageAsPNG(int type);
};
#endif
