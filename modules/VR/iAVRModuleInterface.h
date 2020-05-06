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
#pragma once

#include <iAModuleInterface.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iA3DCylinderObjectVis;
class iAVREnvironment;
class iAVR3DObjectVis; //TEST
class iAVROctree; //TEST

class vtkTable;

class QAction;

class iAVRModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private:
	iAModuleAttachmentToChild * CreateAttachment( MainWindow* mainWnd, MdiChild* child ) override;
	bool vrAvailable();

	QSharedPointer<iA3DCylinderObjectVis> m_cylinderVis;
	QSharedPointer<iAVR3DObjectVis> m_ObjectVis; //TEST
	QSharedPointer<iAVROctree> m_Octree; //TEST
	QSharedPointer<iAVREnvironment> m_vrEnv;
	vtkSmartPointer<vtkTable> m_objectTable;
	QAction* m_actionVRShowFibers;
private slots:
	void info();
	void render();
	void showFibers();
	void vrDone();
};
