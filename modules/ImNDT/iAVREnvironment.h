// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QObject>

class iAVRMainThread;

class vtkOpenVRRenderer;
class vtkRenderer;
class vtkOpenVRRenderWindow;
class vtkOpenVRRenderWindowInteractor;
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
	iAVREnvironment();
	vtkRenderer* renderer();
	vtkOpenVRRenderWindowInteractor* interactor();
	vtkOpenVRRenderWindow* renderWindow();
	void update();
	void start();
	void stop();
	void showSkybox();
	void hideSkybox();
	void showFloor();
	void hideFloor();
	double getInitialWorldScale();
	bool isRunning() const;
private slots:
	void vrDone();
private:
	iAVRMainThread* m_vrMainThread = nullptr;
	vtkSmartPointer<vtkOpenVRRenderer> m_renderer;
	vtkSmartPointer<vtkOpenVRRenderWindow> m_renderWindow;
	vtkSmartPointer<vtkOpenVRRenderWindowInteractor> m_interactor;
	vtkSmartPointer<vtkSkybox> m_skyboxActor;
	//Stores the world scale at start
	double m_worldScale;
	bool m_skyBoxVisible = false;
	bool m_floorVisible = false;

	void storeInitialWorldScale();
	void createLightKit();
	void createSkybox(int skyboxImage);
	vtkSmartPointer<vtkTexture> ReadCubeMap(std::string const& folderPath,
		std::string const& fileRoot,
		std::string const& ext, int const& key);

signals:
	void finished();
};
