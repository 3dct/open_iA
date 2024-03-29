// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVREnvironment.h"

#include "iAVRMainThread.h"

#include <iALog.h>

#include <vtkVersion.h>

#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
#include <vtkCullerCollection.h>
#endif
#include <vtkImageFlip.h>
#include <vtkLight.h>
#include <vtkLightKit.h>
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRCamera.h>
#include <vtkPickingManager.h>
#include <vtkPNGReader.h>
#include <vtkVersion.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QString>


iAVREnvironment::iAVREnvironment():
	m_renderer(vtkSmartPointer<vtkOpenVRRenderer>::New()),
	m_renderWindow(vtkSmartPointer<vtkOpenVRRenderWindow>::New()),
	m_interactor(vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New()),
	m_worldScale(-1.0)
{
	createLightKit();
	createSkybox(0);
	showSkybox();
	//m_renderer->SetShowFloor(true);
}

vtkRenderer* iAVREnvironment::renderer()
{
	return m_renderer;
}

vtkOpenVRRenderWindowInteractor* iAVREnvironment::interactor()
{
	return m_interactor;
}

vtkOpenVRRenderWindow* iAVREnvironment::renderWindow()
{
	return m_renderWindow;
}

void iAVREnvironment::update()
{
	m_renderWindow->Render();
}

void iAVREnvironment::start()
{
	if (m_vrMainThread)
	{
		LOG(lvlWarn, "Cannot start more than one VR session in parallel!");
		emit finished();
		return;
	}
	m_renderWindow->AddRenderer(m_renderer);
	// MultiSamples needs to be set to 0 to make Volume Rendering work:
	// http://vtk.1045678.n5.nabble.com/Problems-in-rendering-volume-with-vtkOpenVR-td5739143.html
	//m_renderWindow->SetMultiSamples(0);
	m_interactor->SetRenderWindow(m_renderWindow);
	auto camera = vtkSmartPointer<vtkOpenVRCamera>::New();

	m_renderer->SetActiveCamera(camera);
	m_renderer->ResetCamera();
	m_renderer->ResetCameraClippingRange();
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
	// workaround for dysfunctional culler culling right eye in VTK >= 9.2:
	m_renderer->RemoveCuller(m_renderer->GetCullers()->GetLastItem());
#endif
	m_interactor->GetPickingManager()->EnabledOn();

	m_vrMainThread = new iAVRMainThread(m_renderWindow, m_interactor);
	connect(m_vrMainThread, &QThread::finished, this, &iAVREnvironment::vrDone);
	m_vrMainThread->setObjectName("ImNDTRenderThread");
	m_vrMainThread->start();

	//TODO: Wait for thread to finish or the rendering might not have started yet
	storeInitialWorldScale();
}

void iAVREnvironment::stop()
{
	if (!m_vrMainThread)
	{
		LOG(lvlWarn, "VR Environment not running!");
		return;
	}
	m_vrMainThread->stop();
}

void iAVREnvironment::showSkybox()
{
	if (m_skyBoxVisible)
	{
		return;
	}
	m_renderer->AddActor(m_skyboxActor);
	m_skyBoxVisible = true;
}

void iAVREnvironment::hideSkybox()
{
	if (!m_skyBoxVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_skyboxActor);
	m_skyBoxVisible = false;
}

void iAVREnvironment::showFloor()
{
	if (m_floorVisible)
	{
		return;
	}
	m_renderer->SetShowFloor(true);
	m_floorVisible = true;
}

void iAVREnvironment::hideFloor()
{
	if (!m_floorVisible)
	{
		return;
	}
	m_renderer->SetShowFloor(false);
	m_floorVisible = false;
}

void iAVREnvironment::createLightKit()
{
	vtkSmartPointer<vtkLightKit> light = vtkSmartPointer<vtkLightKit>::New();
	light->SetKeyLightIntensity(0.88);
	light->AddLightsToRenderer(m_renderer);
}

double iAVREnvironment::getInitialWorldScale()
{
	if (m_worldScale == -1.0) storeInitialWorldScale(); // if not set
	return m_worldScale;
}

void iAVREnvironment::storeInitialWorldScale()
{
	m_worldScale = m_interactor->GetPhysicalScale();
}

void iAVREnvironment::createSkybox(int skyboxImage)
{
	//const std::string chosenSkybox = QString("/skybox%1/").arg(skyboxImage).toUtf8();
	const std::string chosenSkybox = "/skybox" + std::to_string(skyboxImage) + "/";

	// Load the skybox
	// Read it again as there is no deep copy for vtkTexture
	QString path = QCoreApplication::applicationDirPath() + "/VR-skybox";
	auto skybox = ReadCubeMap(path.toStdString(), chosenSkybox, ".png", 0);
	skybox->InterpolateOn();
	skybox->RepeatOff();
	skybox->EdgeClampOn();

	//m_renderer->UseImageBasedLightingOn();
	//m_renderer->SetEnvironmentTexture(cubemap);

	m_skyboxActor = vtkSmartPointer<vtkSkybox>::New();
	m_skyboxActor->SetTexture(skybox);
}

vtkSmartPointer<vtkTexture> iAVREnvironment::ReadCubeMap(std::string const& folderPath,
	std::string const& fileRoot,
	std::string const& ext, int const& key)
{
	// A map of cube map naming conventions and the corresponding file name
	// components.
	std::map<int, std::vector<std::string>> fileNames{
		{0, {"right", "left", "top", "bottom", "front", "back"}},
		{1, {"posx", "negx", "posy", "negy", "posz", "negz"}},
		{2, {"-px", "-nx", "-py", "-ny", "-pz", "-nz"}},
		{3, {"0", "1", "2", "3", "4", "5"}} };
	std::vector<std::string> fns;
	if (fileNames.count(key))
	{
		fns = fileNames.at(key);
	}
	else
	{
		std::cerr << "ReadCubeMap(): invalid key, unable to continue." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	auto texture = vtkSmartPointer<vtkTexture>::New();
	texture->CubeMapOn();
	// Build the file names.
	std::for_each(fns.begin(), fns.end(),
		[&folderPath, &fileRoot, &ext](std::string& f) {
			f = folderPath + fileRoot + f + ext;
		});
	auto i = 0;
	for (auto const& fn : fns)
	{
		auto imgReader = vtkSmartPointer<vtkPNGReader>::New();
		imgReader->SetFileName(fn.c_str());

		auto flip = vtkSmartPointer<vtkImageFlip>::New();
		flip->SetInputConnection(imgReader->GetOutputPort());
		flip->SetFilteredAxis(1); // flip y axis
		texture->SetInputConnection(i, flip->GetOutputPort(0));
		++i;
	}
	return texture;
}

bool iAVREnvironment::isRunning() const
{
	return m_vrMainThread;
}

void iAVREnvironment::vrDone()
{
	if (!m_vrMainThread->message().isEmpty())
	{
		QMessageBox::warning(nullptr, "VR Problems", m_vrMainThread->message());
	}
	delete m_vrMainThread;
	m_vrMainThread = nullptr;
	emit finished();
}
