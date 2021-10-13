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
#pragma once

#include <iAGUIModuleInterface.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>

#include "iAVRMain.h"
#include "iA3DColoredPolyObjectVis.h"
#include "iAVRInteractorStyle.h"

class iAVREnvironment;

class vtkTable;

class QAction;

class iAVRModuleInterface : public iAGUIModuleInterface{
	Q_OBJECT
public:
	void Initialize() override;
	void ImNDT();
	void ImNDT(iA3DColoredPolyObjectVis* polyObject, vtkTable* objectTable, iACsvIO io, iACsvConfig csvConfig);
private:
	iAModuleAttachmentToChild* CreateAttachment(iAMainWindow* mainWnd, iAMdiChild* child) override;
	bool vrAvailable();
	bool loadImNDT();
	bool create3DPolyObjectVis(vtkTable* objectTable, iACsvIO io, iACsvConfig csvConfig, std::map<size_t, std::vector<iAVec3f> > curvedFiberInfo);
	QSharedPointer<iAVREnvironment> m_vrEnv;
	QSharedPointer<iA3DColoredPolyObjectVis> m_polyObject;
	iAVRMain* m_vrMain;
	vtkSmartPointer<iAVRInteractorStyle> m_style;
	vtkSmartPointer<vtkTable> m_objectTable;
	QAction* m_actionVRStartAnalysis;
private slots:
	void info();
	void render();
	void startAnalysis();
	void vrDone();
};
