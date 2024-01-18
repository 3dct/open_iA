// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAvtkVR.h"

#include <vtkSmartPointer.h>

#include <QObject>

class iAVRMainThread;

class vtkRenderer;
class vtkSkybox;
class vtkTexture;

//! The VR environment. Currently deleted every time when the environment is stopped.
//! Could be re-used, but that would require all features using it to cleanly remove
//! all elements from the VR renderer before exiting!
//! The **only** instance of it should be the one in the ImNDT module interface,
//! so that we can keep track of whether it's currently running.
//! TODO: Enforce this programmatically
class iAVREnvironment: public QObject
{
	Q_OBJECT
public:
	iAVREnvironment(iAvtkVR::Backend backend);
	vtkRenderer* renderer();
	iAvtkVRRenderWindowInteractor* interactor();
	iAvtkVRRenderWindow* renderWindow();
	void update();
	void start();
	void stop();
	void showSkybox();
	void hideSkybox();
	void showFloor();
	void hideFloor();
	//! retrieve the scale of the world that was set when this function was first called
	double getInitialWorldScale();
	//! @return whether the environment's main event loop is set (=currently running)
	bool isRunning() const;
	//! queue a task to be executed within the main VR thread
	void queueTask(std::function<void()> task);
	iAvtkVR::Backend backend() const;
private slots:
	void vrDone();
private:
	iAVRMainThread* m_vrMainThread = nullptr;
	vtkSmartPointer<iAvtkVRRenderer> m_renderer;
	vtkSmartPointer<iAvtkVRRenderWindow> m_renderWindow;
	vtkSmartPointer<iAvtkVRRenderWindowInteractor> m_interactor;
	vtkSmartPointer<vtkSkybox> m_skyboxActor;
	//Stores the world scale at start
	double m_worldScale;
	bool m_skyBoxVisible = false;
	bool m_floorVisible = false;
	iAvtkVR::Backend m_backend;

	void storeInitialWorldScale();
	void createLightKit();
	void createSkybox(int skyboxImage);
	vtkSmartPointer<vtkTexture> ReadCubeMap(std::string const& folderPath,
		std::string const& fileRoot,
		std::string const& ext, int const& key);

signals:
	void finished();
};
