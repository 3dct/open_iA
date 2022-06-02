/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
*                 A. Gall															  *
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

#include <openvr.h>

#include <vtkOpenVRRenderWindow.h>
#include <vtkSmartPointer.h>
#include <openvr.h>

class vtkExtractVOI;
class vtkOpenVRRenderer;
class vtkActor;
class vtkImageData;
class vtkRenderer;

class iAVRFrontCamera
{
public:
	iAVRFrontCamera(vtkRenderer* renderer, vtkOpenVRRenderWindow* renderWindow);
	virtual ~iAVRFrontCamera();

	/** Initialize the tracked camera */
	virtual void initialize();

	/*Build representation */
	void buildRepresentation();

	/*Show AR View */
	void show();
	void hide();

	//void switch Background ?

	void refreshImage();


private:
	bool m_visible;

	/** Vive System */
	vr::IVRSystem* m_pHMD;

	/** Tracked Camera (or front camera) */
	vr::IVRTrackedCamera* m_VRTrackedCamera;

	/*The tracked Camera has a unique TrackedCameraHandle_t
	* This handle is used to set attributes, receive events, (and render?).
	* These are several circumstances where the tracked camera isn't detected or invalid.
	* In those case the handle will be equal to INVALID_TRACKED_CAMERA_HANDLE */
	vr::TrackedCameraHandle_t m_VRTrackedCameraHandle;


	/** Camera frame parameters */
	uint32_t				m_cameraFrameWidth;
	uint32_t				m_cameraFrameHeight;
	uint32_t				m_cameraFrameBufferSize;
	uint8_t*				m_cameraFrameBuffer;

	uint32_t				m_lastFrameSequence;

	/*Type of frame from the Tracked Camera : distorted/Undistorted/MaximumUndistorted*/
	vr::EVRTrackedCameraFrameType m_frameType;

	//get the frame header only
	vr::CameraVideoStreamFrameHeader_t m_frameHeader;

	/** Image source to fill and return for display */
	vtkSmartPointer<vtkImageData> m_sourceImage;
	vtkSmartPointer<vtkImageData> m_leftImage;
	vtkSmartPointer<vtkImageData> m_rightImage;

	vtkSmartPointer<vtkTexture> m_sourceTexture;
	vtkSmartPointer<vtkTexture> m_leftTexture;
	vtkSmartPointer<vtkTexture> m_rightTexture;
	vtkSmartPointer<vtkFloatArray> m_textureCoordinates;

	vtkSmartPointer<vtkOpenVRRenderWindow> m_renderWindow;
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkOpenVRRenderer> m_backgroundRenderer;
	vtkSmartPointer<vtkActor> m_cameraActor;

	void getFrameSize();
	void allocateImages();
	void loadVideoStream();
	void createImage();
	void createLeftAndRightEyeImage();

	void saveImageAsPNG(int type);
};

