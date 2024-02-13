// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "imndt_export.h"

#include <iACsvConfig.h>

#include <iAGUIModuleInterface.h>

#include <memory>

class iAImNDTMain;
class iAVREnvironment;

class iAColoredPolyObjectVis;
class iAObjectsData;

class iADataSetRenderer;

class vtkRenderer;

class QAction;

class ImNDT_API iAImNDTModuleInterface : public iAGUIModuleInterface{
	Q_OBJECT
public:
	~iAImNDTModuleInterface();
	void Initialize() override;
	//! start ImNDT (the VR objects visualization) tool with pre-loaded data
	bool startImNDT(std::shared_ptr<iAObjectsData> objData, iACsvConfig csvConfig);
	//! stop the ImNDT (VR objects visualization) tool
	void stopImNDT();
	//! retrieve the VTK renderer used for rendering stuff in VR
	vtkRenderer* getRenderer();
	//! queue a task to be executed in the VR main thread
	void queueTask(std::function<void()> fn);
	//! whether currently the VR environment is running
	bool isVRRunning() const;
	//! checks whether anyone is still using the VR environment, and if not, closes it
	void checkStopVR();
	//! set a selection from the outside
	void setSelection(std::vector<size_t> selection);
	//! retrieve the current selected objects
	std::vector<size_t> selection();

signals:
	//! fires whenever the selection of the associated 3D object visualization is changed
	void selectionChanged();
	//! fires immediately before the ImNDT analysis is stopped
	void analysisStopped();

private:
	bool vrAvailable();
	bool setupVREnvironment();
	//! The VR environment. Currently deleted every time when the environment is stopped.
	//! Could be re-used, but that would require all features using it to cleanly remove
	//! all elements from the VR renderer before exiting!
	std::shared_ptr<iAVREnvironment> m_vrEnv;
	//! @{ for ImNDT
	std::shared_ptr<iAColoredPolyObjectVis> m_polyObject;
	std::shared_ptr<iAObjectsData> m_objData;
	std::shared_ptr<iAImNDTMain> m_vrMain;
	//! @}
	QAction *m_actionVRStartAnalysis;

	std::map<std::pair<iAMdiChild*, size_t>, std::shared_ptr<iADataSetRenderer> >  m_vrRenderers;
	std::vector<QAction*> m_vrActions;

private slots:
	void info();
	//! start a new ImNDT analysis (including selecting an objects file)
	void startAnalysis();
#ifdef OPENVR_AVAILABLE
	void openVRInfo();
#endif
#ifdef OPENXR_AVAILABLE
	void openXRInfo();
#endif
};
