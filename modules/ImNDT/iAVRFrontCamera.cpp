// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRFrontCamera.h"

#ifdef OPENVR_AVAILABLE

#include "iAVtkVR.h"
#include "iAVREnvironment.h"

#include <iALog.h>

#include <openvr.h>

#include <vtkExtractVOI.h>
#include <vtkImageData.h>
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkPNGWriter.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTexture.h>

#include <QString>

class vtkFloatArray;

// dummy implementations so that we can use iAVRFrontCamera as "null object":
iAVRFrontCamera::~iAVRFrontCamera()
{}

bool iAVRFrontCamera::refreshImage()
{
	return true;
}

bool iAVRFrontCamera::setEnabled(bool enabled)
{
	Q_UNUSED(enabled);
	return true;
}

class iAOpenVRFrontCamera: public iAVRFrontCamera
{
public:
	iAOpenVRFrontCamera(iAVREnvironment* env);  // vtkRenderer* renderer, iAvtkVRRenderWindow* renderWindow
	~iAOpenVRFrontCamera();

	/** Initialize the tracked camera */
	bool initialize();

	/*Build representation */
	bool buildRepresentation();

	//void switch Background ?

	bool refreshImage() override;

	bool setEnabled(bool enabled) override;

private:

	/** Camera frame parameters */
	uint32_t m_cameraFrameWidth;
	uint32_t m_cameraFrameHeight;
	uint32_t m_cameraFrameBufferSize;
	uint8_t* m_cameraFrameBuffer;
	uint32_t m_lastFrameSequence;

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

	bool getFrameSize();
	void allocateImages();
	void loadVideoStream();
	void createImage();
	void createLeftAndRightEyeImage();

	//void saveImageAsPNG(int type);
	iAVREnvironment* m_vrEnv;
};

iAOpenVRFrontCamera::iAOpenVRFrontCamera(iAVREnvironment* env)
	: m_vrEnv(env)
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

	m_sourceTexture = vtkSmartPointer<vtkTexture>::New();
	m_rightTexture = vtkSmartPointer<vtkTexture>::New();
	m_leftTexture = vtkSmartPointer<vtkTexture>::New();

	m_frameType = vr::VRTrackedCameraFrameType_Distorted;
}

iAOpenVRFrontCamera::~iAOpenVRFrontCamera()
{
	m_pHMD = nullptr;
	m_VRTrackedCamera = nullptr;
}

bool iAOpenVRFrontCamera::initialize()
{
	auto openVRRenWin = vtkOpenVRRenderWindow::SafeDownCast(m_vrEnv->renderWindow());
	m_pHMD = openVRRenWin->GetHMD();
	m_VRTrackedCamera = vr::VRTrackedCamera();
	if (!m_VRTrackedCamera)
	{
		LOG(lvlError, "Unable to get tracked camera interface!");
		return false;
	}

	bool bHasCamera = false;
	vr::EVRTrackedCameraError nCameraError = m_VRTrackedCamera->HasCamera(vr::k_unTrackedDeviceIndex_Hmd, &bHasCamera);
	if (nCameraError != vr::VRTrackedCameraError_None || !bHasCamera)
	{
		LOG(lvlError, QString("No tracked camera available! Error: %1").arg(m_VRTrackedCamera->GetCameraErrorNameFromEnum(nCameraError)));
		return false;
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
		return false;
	}

	m_pHMD->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd,
		vr::Prop_CameraFirmwareDescription_String, buffer, sizeof(buffer), &propertyError);
	if (propertyError != vr::TrackedProp_Success)
	{
		LOG(lvlError, "Error initializing the front camera");
		return false;
	}

	LOG(lvlInfo, "Front Camera initialized");
	LOG(lvlInfo, QString("Camera Firmware %1").arg(buffer));
	//m_renderWindow->MakeCurrent();
	if (!buildRepresentation() ||
		!refreshImage())
	{
		return false;
	}
	return true;
}

bool iAOpenVRFrontCamera::buildRepresentation()
{
	m_vrEnv->renderWindow()->MakeCurrent();

	if (!getFrameSize())
	{
		return false;
	}
	allocateImages();
	loadVideoStream();
	createLeftAndRightEyeImage();

	m_backgroundRenderer = vtkSmartPointer<vtkOpenVRRenderer>::New();
	m_backgroundRenderer->TexturedBackgroundOn();
	//m_backgroundRenderer->SetBackgroundTexture(m_sourceTexture);
	m_backgroundRenderer->SetRightBackgroundTexture(m_rightTexture);
	m_backgroundRenderer->SetLeftBackgroundTexture(m_leftTexture);

	m_vrEnv->renderWindow()->SetNumberOfLayers(2);
	m_vrEnv->renderWindow()->AddRenderer(m_backgroundRenderer);
	m_backgroundRenderer->SetLayer(0);
	m_backgroundRenderer->InteractiveOff();
	m_vrEnv->renderer()->SetLayer(1);

	m_vrEnv->renderWindow()->Render();
	return true;
}

bool iAOpenVRFrontCamera::refreshImage()
{
	m_vrEnv->renderWindow()->MakeCurrent();

	if (!m_VRTrackedCamera || !m_VRTrackedCameraHandle)
	{
		return false;
	}

	//get the frame header only
	vr::CameraVideoStreamFrameHeader_t frameHeader;
	// provide null for the framebuffer or frameheader to check frame status
	vr::EVRTrackedCameraError nCameraError = m_VRTrackedCamera->GetVideoStreamFrameBuffer(m_VRTrackedCameraHandle, m_frameType, nullptr, 0, &frameHeader, sizeof(frameHeader));
	if (nCameraError != vr::VRTrackedCameraError_None)
	{
		LOG(lvlError, QString("No tracked camera available! Error: %1").arg(m_VRTrackedCamera->GetCameraErrorNameFromEnum(nCameraError)));
		return false;
	}

	if (frameHeader.nFrameSequence == m_lastFrameSequence)
	{
		// frame hasn't changed yet, nothing to do
	}
	else
	{
		loadVideoStream();
		createLeftAndRightEyeImage();
		m_sourceTexture->Modified();
		m_rightTexture->Modified();
		m_leftTexture->Modified();

		m_vrEnv->renderWindow()->Render();
	}
	return true;
}

//! Get FrameWidth and FrameHeight from tracked camera
bool iAOpenVRFrontCamera::getFrameSize()
{
	// Allocate for camera frame buffer requirements
	uint32_t nCameraFrameBufferSize = 0;

	auto nCameraError = m_VRTrackedCamera->GetCameraFrameSize(vr::k_unTrackedDeviceIndex_Hmd, m_frameType,
		&m_cameraFrameWidth, &m_cameraFrameHeight, &nCameraFrameBufferSize);
	if (nCameraError != vr::VRTrackedCameraError_None)
	{
		LOG(lvlError, QString("GetCameraFrameBounds() failed! Error: %1!").arg(m_VRTrackedCamera->GetCameraErrorNameFromEnum(nCameraError)));
		return false;
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
		return false;
	}
	return true;
}

//! Allocate dimensions to the source image and the image for left and right eye
void iAOpenVRFrontCamera::allocateImages()
{
	//m_sourceImage = vtkImageData::New();
	m_sourceImage = vtkSmartPointer<vtkImageData>::New();
	m_sourceImage->SetDimensions((int)m_cameraFrameWidth, (int)m_cameraFrameHeight, 1);
	m_sourceImage->SetSpacing(1.0, 1.0, 1.0);
	m_sourceImage->SetOrigin(0.0, 0.0, 0.0);
	m_sourceImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

	/*Images for each eye */
	m_leftImage = vtkSmartPointer<vtkImageData>::New();
	//m_leftImage->SetDimensions((int)m_cameraFrameWidth, (int)m_cameraFrameHeight/2.0, 1); //half height
	//m_leftImage->SetSpacing(1.0, 1.0, 1.0);
	//m_leftImage->SetOrigin(0.0, 0.0, 0.0);
	//m_leftImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

	m_rightImage = vtkSmartPointer<vtkImageData>::New();
	//m_rightImage->SetDimensions((int)m_cameraFrameWidth, (int)m_cameraFrameHeight/2.0, 1);
	//m_rightImage->SetSpacing(1.0, 1.0, 1.0);
	//m_rightImage->SetOrigin(0.0, 0.0, 0.0);
	//m_rightImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
}

//! Copies the frame buffer of the openVR device and creates the image
void iAOpenVRFrontCamera::loadVideoStream()
{
	if (!m_VRTrackedCamera || !m_VRTrackedCameraHandle)
	{
		return;
	}

	// Frame has changed, do the more expensive frame buffer copy
	vr::EVRTrackedCameraError nCameraError = m_VRTrackedCamera->GetVideoStreamFrameBuffer(m_VRTrackedCameraHandle, m_frameType, m_cameraFrameBuffer, m_cameraFrameBufferSize, &m_frameHeader, sizeof(m_frameHeader));
	if (nCameraError != vr::VRTrackedCameraError_None)
	{
		LOG(lvlError, QString("No tracked camera available! Error: %1").arg(m_VRTrackedCamera->GetCameraErrorNameFromEnum(nCameraError)));
		return;
	}

	m_lastFrameSequence = m_frameHeader.nFrameSequence;

	createImage();
	m_sourceImage->Modified();
	m_sourceTexture->SetInputData(m_sourceImage);
	m_sourceTexture->Modified();
}

//! creates the source image out of the buffer
void iAOpenVRFrontCamera::createImage()
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
			allocateImages();
		}
		for (int y = (int)m_cameraFrameHeight - 1; y >= 0; y--)
		{
			for (int x = 0; x < (int)m_cameraFrameWidth; x++)
			{
				//int* dims = m_sourceImage->GetDimensions();
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

void iAOpenVRFrontCamera::createLeftAndRightEyeImage()
{
	const int* dims = m_sourceImage->GetDimensions();

	vtkSmartPointer<vtkExtractVOI> extractLeftImage = vtkSmartPointer<vtkExtractVOI>::New();
	extractLeftImage->SetInputData(m_sourceImage);
	extractLeftImage->SetVOI(0, dims[0] - 1, 0, std::floor(((double)dims[1] - 1.0) / 2.0), 0, dims[2] - 1);
	extractLeftImage->Update();
	m_leftImage = extractLeftImage->GetOutput();

	//auto yr = std::floor(((double)dims[1] - 1.0) / 2.0);
	//m_rightImage->CopyAndCastFrom(m_sourceImage, 0, dims[0] -1, 0, std::floor(((double)dims[1]-1.0) / 2.0), 0, dims[2]-1);

	vtkSmartPointer<vtkExtractVOI> extractRightImage = vtkSmartPointer<vtkExtractVOI>::New();
	extractRightImage->SetInputData(m_sourceImage);
	extractRightImage->SetVOI(0, dims[0] - 1, std::ceil(((double)dims[1] - 1.0) / 2.0), dims[1] - 1, 0, dims[2] - 1);
	extractRightImage->Update();
	m_rightImage = extractRightImage->GetOutput();

	//auto yl = std::ceil(((double)dims[1] - 1.0) / 2.0);
	//m_leftImage->CopyAndCastFrom(m_sourceImage, 0, dims[0] - 1, std::ceil(((double)dims[1] - 1.0) / 2.0), dims[1] - 1, 0, dims[2] - 1);

	m_leftImage->Modified();
	m_rightImage->Modified();

	m_leftTexture->SetInputData(m_leftImage);
	m_rightTexture->Modified();
	m_rightTexture->SetInputData(m_rightImage);
	m_rightTexture->Modified();
}

/*
//! Saves the camera image as PNG
//! 0: Source Image
//! 1: Right Eye Image
//! 2: Left Eye Image
void iAVRFrontCamera::saveImageAsPNG(int type)
{
	vtkNew<vtkPNGWriter> writer;
	switch (type)
	{
	case 0:
		writer->SetFileName("SourceImage.png");
		writer->SetInputData(m_sourceImage);
		writer->Write();
		break;
	case 1:
		writer->SetFileName("RightImage.png");
		writer->SetInputData(m_rightImage);
		writer->Write();
		break;
	case 2:
		writer->SetFileName("LeftImage.png");
		writer->SetInputData(m_leftImage);
		writer->Write();
		break;
	default:
		writer->SetFileName("SourceImage.png");
		writer->SetInputData(m_sourceImage);
		writer->Write();
	}
}
*/

bool iAOpenVRFrontCamera::setEnabled(bool enabled)
{
	if (enabled)
	{
		m_vrEnv->hideSkybox();
		if (!initialize())
		{
			m_vrEnv->showSkybox();
			LOG(lvlWarn, "Initializing AR view failed; maybe your headset doesn't have a camera? If your headset does have a camera, make sure you have Camera enabled in the SteamVR settings!");
			return false;
		}
	}
	else
	{
		m_vrEnv->showSkybox();
	}
	return true;
}

#endif


#ifdef OPENXR_AVAILABLE

#include <openxr.h>
#include <vtkOpenXRManager.h>

class iAOpenXRFrontCamera : public iAVRFrontCamera
{
	bool refreshImage() override
	{
		return true;
	}
	bool setEnabled(bool enabled) override
	{
		if (enabled)
		{
			// Currently requires additional setup: https://github.com/Rectus/openxr-steamvr-passthrough/blob/main/readme.md
			// and even then, it doesn't seem to work - probably due to "OpenGL applications are not currently supported."
			vtkOpenXRManager::GetInstance().SetBlendMode(XrEnvironmentBlendMode::XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND);
		}
		else
		{
			vtkOpenXRManager::GetInstance().SetBlendMode(XrEnvironmentBlendMode::XR_ENVIRONMENT_BLEND_MODE_OPAQUE);
		}
		return true;
	}
};
#endif


std::unique_ptr<iAVRFrontCamera> createARViewer(iAVREnvironment* env)
{
#ifdef OPENXR_AVAILABLE
	if (env->backend()  == iAvtkVR::OpenVR)
	{
		return std::make_unique<iAOpenVRFrontCamera>(env);
	}
#endif
#ifdef OPENVR_AVAILABLE
	if (env->backend() == iAvtkVR::OpenXR)
	{
		return std::make_unique<iAOpenXRFrontCamera>();
	}
#endif
	LOG(lvlError, "Could not create AR viewer for given environment, using dummy!");
	return std::make_unique<iAVRFrontCamera>();
}
