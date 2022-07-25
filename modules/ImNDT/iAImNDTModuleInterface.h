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

#include "ImNDT_export.h"
#include "iAImNDTMain.h"
#include "iAImNDTInteractorStyle.h"

#include "iA3DColoredPolyObjectVis.h"

#include <iAGUIModuleInterface.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iAVREnvironment;

class iAVolumeRenderer;

class vtkTable;

class QAction;


class ImNDT_API iAImNDTModuleInterface : public iAGUIModuleInterface{
	Q_OBJECT
public:
	~iAImNDTModuleInterface();
	void Initialize() override;
	bool ImNDT(QSharedPointer<iA3DColoredPolyObjectVis> polyObject, vtkSmartPointer<vtkTable> objectTable, iACsvIO io,
		iACsvConfig csvConfig);
	vtkRenderer* getRenderer();

signals:
	void selectionChanged();
	void arViewToggled();

private:
	bool vrAvailable();
	bool loadImNDT();
	bool setupVREnvironment();
	//! The VR environment. Currently deleted every time when the environment is stopped.
	//! Could be re-used, but that would require all features using it to cleanly remove
	//! all elements from the VR renderer before exiting!
	std::shared_ptr<iAVREnvironment> m_vrEnv;
	bool m_vrEnvStartedBefore = false; //!< when started a second time, the VR environment variable flickers strangely. So we show a warning
	//! @{ for ImNDT
	QSharedPointer<iA3DColoredPolyObjectVis> m_polyObject;
	std::shared_ptr<iAImNDTMain> m_vrMain;
	vtkSmartPointer<iAImNDTInteractorStyle> m_style;
	iACsvConfig m_csvConfig;
	iACsvIO m_io;
	vtkSmartPointer<vtkTable> m_objectTable;
	//! @}
	std::shared_ptr<iAVolumeRenderer> m_volumeRenderer;  //! for VR volume rendering

	QAction *m_actionVRStartAnalysis, * m_actionVRVolumeRender;

private slots:
	void info();
	void renderVolume();
	void startAnalysis();
	void vrInfo();
};
