/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include <vtkSmartPointer.h>
#include <vtkTexture.h>
#include <vtkActor.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkSkybox.h>

#include <QObject>

class iAVRMainThread;

class vtkOpenVRRenderer;
class vtkRenderer;
class vtkOpenVRRenderWindow;

class iAVRInteractor;

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
	iAVRMainThread* m_vrMainThread;
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
