/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iAVRFrontCamera.h"

#include <qstring.h>

#include <iALog.h>

#include <vtkImageData.h>
#include <vtkOpenVRRenderer.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>;
#include <vtkTexture.h>;

iAVRFrontCamera::iAVRFrontCamera(vtkRenderer* renderer,vtkOpenVRRenderWindow* renderWindow): m_renderWindow(renderWindow), m_renderer(renderer)
{
	m_pHMD = nullptr;
	m_VRTrackedCamera = 0;
	m_VRTrackedCameraHandle = 0;
	m_sourceImage = 0;

	m_cameraFrameWidth = 0;
	m_cameraFrameHeight = 0;
	m_cameraFrameBufferSize = 0;
	m_cameraFrameBuffer = nullptr;

	m_lastFrameSequence = 0;

	m_frameType = vr::VRTrackedCameraFrameType_Undistorted;
}

iAVRFrontCamera::~iAVRFrontCamera()
{
	m_pHMD = nullptr;
	m_VRTrackedCamera = nullptr;
}

void iAVRFrontCamera::initialize()
{
	m_pHMD = m_renderWindow->GetHMD();
	m_VRTrackedCamera = vr::VRTrackedCamera();
	if (!m_VRTrackedCamera)
	{
		LOG(lvlError, "Unable to get tracked camera interface!");
		return;
	}

	bool bHasCamera = false;
	vr::EVRTrackedCameraError nCameraError = m_VRTrackedCamera->HasCamera(vr::k_unTrackedDeviceIndex_Hmd, &bHasCamera);
	if (nCameraError != vr::VRTrackedCameraError_None || !bHasCamera)
	{
		LOG(lvlError, QString("No Tracked Camera Available %1").arg(m_VRTrackedCamera->GetCameraErrorNameFromEnum(nCameraError)));
		return;
	}

	// Accessing the FW description is just a further check to ensure camera communication
	// is valid as expected
	vr::ETrackedPropertyError propertyError;
	char buffer[1024];

	m_pHMD->GetStringTrackedDeviceProperty(
		vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_CameraFirmwareDescription_String,
		buffer, sizeof(buffer), &propertyError);
	if (propertyError != vr::TrackedProp_Success)
	{
		LOG(lvlError, "Failed to get tracked camera firmware description");
		return;
	}

	m_pHMD->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd,
		vr::Prop_CameraFirmwareDescription_String, buffer, sizeof(buffer), &propertyError);
	if (propertyError != vr::TrackedProp_Success)
	{
		LOG(lvlError, "Error Initializing the front camera");
		return;
	}

	LOG(lvlInfo, "Front Camera initialized");
	LOG(lvlInfo, QString("Camera Firmware %1").arg(buffer));
	//m_renderWindow->MakeCurrent();
}

void iAVRFrontCamera::buildRepresentation()
{
	m_renderWindow->MakeCurrent();

	getFrameSize();
	allocateImage();
	loadVideoStream();
	createImage();

	m_sourceTexture = vtkSmartPointer<vtkTexture>::New();;
	m_sourceTexture->SetInputData(m_sourceImage);
	m_sourceTexture->Update();

	vtkNew<vtkPlaneSource> plane;

	vtkNew<vtkPolyDataMapper> planeMapper;
	planeMapper->SetInputConnection(plane->GetOutputPort());

	m_actor = vtkSmartPointer<vtkActor>::New();
	m_actor->SetMapper(planeMapper);
	m_actor->SetTexture(m_sourceTexture);
	m_actor->SetScale(800);
	m_renderer->AddActor(m_actor);
}

void iAVRFrontCamera::refreshImage()
{
	if (!m_VRTrackedCamera || !m_VRTrackedCameraHandle)
	{
		return;
	}

	//get the frame header only
	vr::CameraVideoStreamFrameHeader_t frameHeader;
	// provide null for the framebuffer or frameheader to check frame status
	vr::EVRTrackedCameraError nCameraError = m_VRTrackedCamera->GetVideoStreamFrameBuffer(m_VRTrackedCameraHandle, m_frameType, nullptr, 0, &frameHeader, sizeof(frameHeader));
	if (nCameraError != vr::VRTrackedCameraError_None)
	{
		LOG(lvlError, QString("No Tracked Camera Available %1").arg(m_VRTrackedCamera->GetCameraErrorNameFromEnum(nCameraError)));
		return;
	}

	if (frameHeader.nFrameSequence == m_lastFrameSequence)
	{
		// frame hasn't changed yet, nothing to do
		return;
	}
	else
	{
		loadVideoStream();
		m_sourceTexture->Modified();
	}
}

//! Get FrameWidth and FrameHeight from tracked camera
void iAVRFrontCamera::getFrameSize()
{
	// Allocate for camera frame buffer requirements
	uint32_t nCameraFrameBufferSize = 0;

	if (m_VRTrackedCamera->GetCameraFrameSize(vr::k_unTrackedDeviceIndex_Hmd, m_frameType, &m_cameraFrameWidth,
		&this->m_cameraFrameHeight, &nCameraFrameBufferSize) != vr::VRTrackedCameraError_None)
	{
		LOG(lvlError, "GetCameraFrameBounds() Failed");
		return;
	}
	LOG(lvlInfo, QString("Size: Width: %1 Height: %2").arg(m_cameraFrameWidth).arg(m_cameraFrameHeight));

	if (nCameraFrameBufferSize && nCameraFrameBufferSize != this->m_cameraFrameBufferSize)
	{
		delete[] this->m_cameraFrameBuffer;
		this->m_cameraFrameBufferSize = nCameraFrameBufferSize;
		this->m_cameraFrameBuffer = new uint8_t[this->m_cameraFrameBufferSize];
		memset(this->m_cameraFrameBuffer, 0, this->m_cameraFrameBufferSize);
	}

	m_VRTrackedCamera->AcquireVideoStreamingService(vr::k_unTrackedDeviceIndex_Hmd, &m_VRTrackedCameraHandle);
	if (m_VRTrackedCameraHandle == INVALID_TRACKED_CAMERA_HANDLE)
	{
		LOG(lvlError, "AcquireVideoStreamingService() Failed");
		return;
	}
}

//! Allocate dimensions to the source image
void iAVRFrontCamera::allocateImage()
{
	m_sourceImage = vtkImageData::New();
	m_sourceImage->SetDimensions((int)m_cameraFrameWidth, (int)m_cameraFrameHeight, 1);
	m_sourceImage->SetSpacing(1.0, 1.0, 1.0);
	m_sourceImage->SetOrigin(0.0, 0.0, 0.0);
	m_sourceImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
}

//! Copies the frame buffer of the openVR device and creates the image
void iAVRFrontCamera::loadVideoStream()
{
	if (!m_VRTrackedCamera || !m_VRTrackedCameraHandle)
	{
		return;
	}

	// Frame has changed, do the more expensive frame buffer copy
	vr::EVRTrackedCameraError nCameraError = m_VRTrackedCamera->GetVideoStreamFrameBuffer(m_VRTrackedCameraHandle, m_frameType, m_cameraFrameBuffer, m_cameraFrameBufferSize, &m_frameHeader, sizeof(m_frameHeader));
	if (nCameraError != vr::VRTrackedCameraError_None)
	{
		LOG(lvlError, QString("No Tracked Camera Available %1").arg(m_VRTrackedCamera->GetCameraErrorNameFromEnum(nCameraError)));
		return;
	}

	m_lastFrameSequence = m_frameHeader.nFrameSequence;

	createImage();
}

//! creates the source image out of the buffer
void iAVRFrontCamera::createImage()
{
	const uint8_t* pFrameImage = m_cameraFrameBuffer;

	if (pFrameImage && m_cameraFrameWidth && m_cameraFrameHeight)
	{
		if (this->m_sourceImage &&
			((uint32_t)this->m_sourceImage->GetDimensions()[0] != m_cameraFrameWidth ||
				(uint32_t)this->m_sourceImage->GetDimensions()[1] != m_cameraFrameHeight))
		{
			//dimension changed
			this->m_sourceImage->Delete();
			this->m_sourceImage = nullptr;
		}

		if (!this->m_sourceImage)
		{
			// allocate to expected dimensions
			allocateImage();
		}
		for (int y = (int)m_cameraFrameHeight - 1; y >= 0; y--)
		{
			for (int x = 0; x < (int)m_cameraFrameWidth; x++)
			{
				int* dims = m_sourceImage->GetDimensions();
				unsigned char* pixel = static_cast<unsigned char*>(this->m_sourceImage->GetScalarPointer(x, y, 0));

				if (!pixel)
				{
					LOG(lvlError, "Pixel Null, check for errors");
					return;
				}
				pixel[0] = pFrameImage[0];
				pixel[1] = pFrameImage[1];
				pixel[2] = pFrameImage[2];
				pFrameImage += 4;
			}
		}
	}
}

