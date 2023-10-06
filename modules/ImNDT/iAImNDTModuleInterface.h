// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "imndt_export.h"

#include "iACsvIO.h"

#include <iAGUIModuleInterface.h>

#include <vtkSmartPointer.h>

#include <memory>

class iAImNDTMain;
class iAVREnvironment;

class iAColoredPolyObjectVis;

class iADataSetRenderer;

class vtkRenderer;
class vtkTable;

class QAction;

class ImNDT_API iAImNDTModuleInterface : public iAGUIModuleInterface{
	Q_OBJECT
public:
	~iAImNDTModuleInterface();
	void Initialize() override;
	bool ImNDT(std::shared_ptr<iAColoredPolyObjectVis> polyObject, vtkSmartPointer<vtkTable> objectTable, iACsvIO io,
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
	//! @{ for ImNDT
	std::shared_ptr<iAColoredPolyObjectVis> m_polyObject;
	std::shared_ptr<iAImNDTMain> m_vrMain;
	iACsvConfig m_csvConfig;
	iACsvIO m_io;
	vtkSmartPointer<vtkTable> m_objectTable;
	//! @}
	QAction *m_actionVRStartAnalysis;

	std::map<std::pair<iAMdiChild*, size_t>, std::shared_ptr<iADataSetRenderer> >  m_vrRenderers;
	std::vector<QAction*> m_vrActions;

private slots:
	void info();
	void startAnalysis();
#ifdef OPENVR_AVAILABLE
	void openVRInfo();
#endif
#ifdef OPENXR_AVAILABLE
	void openXRInfo();
#endif
};
