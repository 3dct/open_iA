/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVREnvironment.h"

#include "iAVRInteractor.h"

#include "iAConsole.h"

#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRCamera.h>
#include <vtkPNGReader.h>
#include <vtkJPEGReader.h>
#include <vtkEquirectangularToCubeMapTexture.h>
#include <vtkImageFlip.h>
#include <vtkLight.h>
#include <vtkLightKit.h>

iAVREnvironment::iAVREnvironment():	m_renderer(vtkSmartPointer<vtkOpenVRRenderer>::New()), m_interactor(vtkSmartPointer<iAVRInteractor>::New()), 
m_renderWindow(vtkSmartPointer<vtkOpenVRRenderWindow>::New())
{	
	createSkybox(1);
	createLightKit();
	m_renderer->SetShowFloor(true);
}

vtkRenderer* iAVREnvironment::renderer()
{
	return m_renderer;
}

iAVRInteractor* iAVREnvironment::interactor()
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
	static int runningInstances = 0;
	// "poor man's" check for trying to run two VR sessions in parallel:
	if (runningInstances >= 1)
	{
		DEBUG_LOG("Cannot start more than one VR session in parallel!");
		emit finished();
		return;
	}
	++runningInstances;
	m_renderWindow->AddRenderer(m_renderer);
	// MultiSamples needs to be set to 0 to make Volume Rendering work:
	// http://vtk.1045678.n5.nabble.com/Problems-in-rendering-volume-with-vtkOpenVR-td5739143.html
	m_renderWindow->SetMultiSamples(0);
	m_interactor->SetRenderWindow(m_renderWindow);
	auto camera = vtkSmartPointer<vtkOpenVRCamera>::New();

	m_renderer->SetActiveCamera(camera);
	m_renderer->ResetCamera();
	m_renderer->ResetCameraClippingRange();
	m_renderWindow->Render();
	m_interactor->Start();
	--runningInstances;
	emit finished();
}

void iAVREnvironment::stop()
{
	if (m_interactor)
		m_interactor->stop();
}

void iAVREnvironment::createLightKit()
{
	vtkSmartPointer<vtkLightKit> light = vtkSmartPointer<vtkLightKit>::New();
	light->SetKeyLightIntensity(0.85);
	light->AddLightsToRenderer(m_renderer);
}

void iAVREnvironment::createSkybox(int skyboxImage)
{
	const std::string chosenSkybox = QString("/skybox%1/").arg(skyboxImage).toUtf8();
	//auto cubemap = ReadCubeMap("C:/FHTools/open_iA/src/modules/VR/images", chosenSkybox, ".png", 0);

	// Load the skybox
	// Read it again as there is no deep copy for vtkTexture
	auto skybox = ReadCubeMap("C:/FHTools/open_iA/src/modules/VR/images", chosenSkybox, ".png", 0);
	skybox->InterpolateOn();
	skybox->RepeatOff();
	skybox->EdgeClampOn();

	//m_renderer->UseImageBasedLightingOn();
	//m_renderer->SetEnvironmentTexture(cubemap);

	skyboxActor = vtkSmartPointer<vtkSkybox>::New();
	skyboxActor->SetTexture(skybox);
	m_renderer->AddActor(skyboxActor);
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

