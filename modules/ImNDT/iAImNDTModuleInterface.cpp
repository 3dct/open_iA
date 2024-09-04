// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImNDTModuleInterface.h"

#include "iAImNDTMain.h"
#include "iAVREnvironment.h"

// 3D object visualization
#include <iACsvInputDlg.h>
#include <iAColoredPolyObjectVis.h>
#include <iACsvConfig.h>
#include <iACsvVtkTableCreator.h>
#include <iAObjectsData.h>
#include <iAObjectVisFactory.h>

#include <iADataSetRenderer.h>
#include <iADataSetViewer.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAParameterDlg.h>

#include <iALog.h>

#ifdef OPENVR_AVAILABLE
#include <openvr.h>
#endif

#ifdef OPENXR_AVAILABLE
#include <openxr/openxr.h>
#endif

#include <vtkImageData.h>
#include <vtkTable.h>

#include <QAction>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>


iAImNDTModuleInterface::~iAImNDTModuleInterface()
{
	if (m_vrEnv)
	{
		LOG(lvlInfo, "Application shutdown while VR environment still active; stopping!");
		m_vrEnv->stop();
	}
}

void iAImNDTModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	Q_INIT_RESOURCE(ImNDT);

	QAction * actionIMNDTInfo = new QAction(tr("ImNDT Info"), m_mainWnd);
	connect(actionIMNDTInfo, &QAction::triggered, this, &iAImNDTModuleInterface::info);

	m_actionVRStartAnalysis = new QAction(tr("Start Analysis"), m_mainWnd);
	connect(m_actionVRStartAnalysis, &QAction::triggered, this, &iAImNDTModuleInterface::startAnalysis);

	QMenu* vrMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("ImNDT"), false);
	vrMenu->addAction(actionIMNDTInfo);

#ifdef OPENVR_AVAILABLE
	QAction* actionOpenVRInfo = new QAction(tr("OpenVR Info"), m_mainWnd);
	connect(actionOpenVRInfo, &QAction::triggered, this, &iAImNDTModuleInterface::openVRInfo);
	vrMenu->addAction(actionOpenVRInfo);
#endif
#ifdef OPENXR_AVAILABLE
	QAction* actionOpenXRInfo = new QAction(tr("OpenXR Info"), m_mainWnd);
	connect(actionOpenXRInfo, &QAction::triggered, this, &iAImNDTModuleInterface::openXRInfo);
	vrMenu->addAction(actionOpenXRInfo);
#endif
	vrMenu->addAction(m_actionVRStartAnalysis);

	auto removeRenderer = [this](iAMdiChild* child, size_t dataSetIdx)
	{
		auto key = std::make_pair(child, dataSetIdx);
		auto vrRen = m_vrRenderers.at(key);
		m_vrRenderers.erase(key);
		m_vrEnv->queueTask([vrRen] { vrRen->setVisible(false); });
		checkStopVR();
	};

	// on every child window, listen to new datasets, and if one becomes available add action to add VR renderer
	connect(m_mainWnd, &iAMainWindow::childCreated, this, [this, removeRenderer](iAMdiChild* child)
	{
		connect(child, &iAMdiChild::dataSetRendered, this, [this, removeRenderer, child](size_t dataSetIdx)
		{
			auto viewer = child->dataSetViewer(dataSetIdx);
			if (viewer && viewer->renderer())    // if dataset has a renderer, add a button to view it in VR:
			{
				auto action = viewer->addViewAction("VR", "VR", false);
				connect(action, &QAction::triggered, this, [this, removeRenderer, child, viewer, dataSetIdx, action](bool checked)
				{
					if (!checked)
					{
						removeRenderer(child, dataSetIdx);
						return;
					}
					if (!ensureVREnvironment())
					{
						QSignalBlocker blockAction(action);
						action->setChecked(false);
						return;
					}
					auto vrRen = viewer->createRenderer(m_vrEnv->renderer());
					vrRen->setAttributes(viewer->attributeValues());
					m_vrRenderers.insert(std::make_pair(std::make_pair(child, dataSetIdx), vrRen));
					vrRen->setVisible(true);
					if (!m_vrEnv->isRunning())
					{
						m_vrEnv->start();
					}
				});
				m_vrActions.push_back(action);
				connect(viewer, &iADataSetViewer::dataSetChanged, this, [this, child, viewer](size_t dataSetIdx)
				{
					auto key = std::make_pair(child, dataSetIdx);
					if (!m_vrRenderers.contains(key))
					{
						return;
					}
					auto const& vrRen = m_vrRenderers.at(key);
					vrRen->setAttributes(viewer->attributeValues());
				});
				connect(viewer, &iADataSetViewer::removeDataSet, this, [child, removeRenderer](size_t dataSetIdx)
				{
					removeRenderer(child, dataSetIdx);
				});
			}
		});
	});
}

void iAImNDTModuleInterface::info()
{
	QString infoTxt("ImNDT: Immersive Workspace for the Analysis of Multidimensional Material Data From Non-Destructive Testing"
		"\n\n"
		"Actions with Right Controller:\n"
		" (1) Picking is detected at the bottom inner edge of the controller and activated by pressing the trigger inside cube in the MiM or the fiber model.\n"
		" (2) Multi-selection is made possible by holding one grip and then picking by triggering the right controller. To confirm the selection release the grip. Deselection by selecting the already selected cube again.\n"
		" (3) Pressing the trigger outside an object resets any highlighting.\n"
		" (4) Pressing the Application Button switches between similarity network and fiber model (only possible if MiM is present).\n"
		" (5) Picking two cubes (Multi-selection) in the similarity network opens the Histo-book through which the user can switch between the distributions by holding the right trigger - swiping left or right and releasing it.\n"
		" (6) The Trackpad changes the octree level or feature (both only possible if MiM is present) and resets the Fiber Model. Octree Level can be adjusted by pressing up (higher/detailed level) and down (lower/coarser level) on the trackpad. Feature can be changed by pressing right (next feature) and left (previous feature) on the trackpad."
		"\n\n"
		"Actions with Left Controller:\n"
		" (1) Pressing the Application Button shows or hides the MiM.\n"
		" (2) Pressing the trigger changes the displacement mode.\n"
		" (3) The Trackpad changes the applied displacement (only possible if MiM is present) or the Jaccard index (only possible if similarity network is shown). Displacement can be adjusted by pressing up (shift away from center) and down (merge together) on the trackpad. Jaccard index can be changed by pressing right (shows higher similarity) and left (shows lower similarity) on the trackpad.\n"
		" (4) By holding one grip and then pressing the trigger on the right controller the AR View can be turned on/off."
		"\n\n"
		"Actions using Both Controllers:\n"
		" (1) Pressing a grid button on both controllers zooms or translates the world. The zoom can be controlled by pulling controllers apart (zoom in) or together (zoom out). To translate the world both controllers pull in the same direction (grab and pull closer or away).\n");
	LOG(lvlInfo, infoTxt);
	QMessageBox::information(m_mainWnd, "ImNDT Module", infoTxt);
}

#ifdef OPENVR_AVAILABLE
void iAImNDTModuleInterface::openVRInfo()
{
	LOG(lvlInfo, QString("OpenVR Information:"));
	LOG(lvlInfo, QString("    Is Runtime installed: %1").arg(vr::VR_IsRuntimeInstalled() ? "yes" : "no"));
	const uint32_t MaxRuntimePathLength = 1024;
	uint32_t actualLength;
#if OPENVR_VERSION_MAJOR > 1 || (OPENVR_VERSION_MAJOR == 1 && OPENVR_VERSION_MINOR > 3)
	char runtimePath[MaxRuntimePathLength];
	vr::VR_GetRuntimePath(runtimePath, MaxRuntimePathLength, &actualLength);
#else // OpenVR <= 1.3.22:
	char const * runtimePath = vr::VR_RuntimePath();
#endif
	LOG(lvlInfo, QString("    OpenVR runtime path: %1").arg(runtimePath));
	LOG(lvlInfo, QString("    Head-mounted display present: %1").arg(vr::VR_IsHmdPresent() ? "yes" : "no"));
	vr::EVRInitError eError = vr::VRInitError_None;
	auto pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if (eError != vr::VRInitError_None)
	{
		LOG(lvlError, QString("    Unable to initialize OpenVR: %1").arg(vr::VR_GetVRInitErrorAsEnglishDescription(eError)));
	}
	else
	{
		if (pHMD)
		{
			uint32_t width, height;
			pHMD->GetRecommendedRenderTargetSize(&width, &height);
			LOG(lvlInfo, QString("    Head-mounted display present, recommended render target size: %1x%2 ").arg(width).arg(height));
		}
		else
		{
			LOG(lvlInfo, QString("    Head-mounted display could not be initialized: %1").arg(vr::VR_GetVRInitErrorAsEnglishDescription(eError)));
		}
	}
	vr::VR_Shutdown();
}
#endif

#ifdef OPENXR_AVAILABLE
void iAImNDTModuleInterface::openXRInfo()
{
	// errors encountered in API calls so far:

	// XR_ERROR_RUNTIME_FAILURE = -2,           when no HMD connected / vive connection box not turned on (?)
	// XR_ERROR_API_VERSION_UNSUPPORTED = -4    when type member wasn't properly set in passed-in struct (?) - but xrGetSystemProperty crashed in this case with an access violation!
	// XR_ERROR_SIZE_INSUFFICIENT = -11         when space in data structure not sufficient to store all results of xrEnumerate... call
	// XR_ERROR_RUNTIME_UNAVAILABLE = -51       when SteamVR wasn't installed at all

	// Most important thing to keep in mind: in all structures passed into an API call, set the type field to the respective constant from XrStructureType!"
	// If you don't apparently different things can happen: Either error result (e.g. XR_ERROR_API_VERSION_UNSUPPORTED) or crash (access violation)!

	LOG(lvlInfo, QString("OpenXR Information:"));

	// there seem to be no layers available ... ?
	uint32_t layerCount;
	auto getLayerCountResult = xrEnumerateApiLayerProperties(0, &layerCount, nullptr);
	if (getLayerCountResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Unable to get layer count: %1").arg(getLayerCountResult));
		return;
	}
	LOG(lvlInfo, QString("  Layers (%1):").arg(layerCount));

	uint32_t layersRead;
	std::vector<XrApiLayerProperties> layers(layerCount, { XR_TYPE_API_LAYER_PROPERTIES } );
	auto getLayersResult = xrEnumerateApiLayerProperties(layerCount, &layersRead, layers.data());
	if (getLayersResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Unable to get layer information: %1").arg(getLayersResult));
		return;
	}

	assert(layerCount == layersRead);

	for (uint32_t l = 0; l < layersRead; ++l)
	{
		LOG(lvlInfo, QString("    %1: name=%2, layerVersion=%3, specVersion=%4, description=%5")
			.arg(l)
			.arg(layers[l].layerName)
			.arg(layers[l].layerVersion)
			.arg(layers[l].specVersion)
			.arg(layers[l].description));
	}

	uint32_t extensionCount;
	auto getExtensionCountResult = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
	if (getExtensionCountResult == XR_ERROR_RUNTIME_UNAVAILABLE)
	{
		LOG(lvlError, QString("    Runtime is not available! Please install e.g. SteamVR and try again!"));
		return;
	}
	if (getExtensionCountResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Unable to get extension count: %1").arg(getExtensionCountResult));
		return;
	}
	LOG(lvlInfo, QString("  Extensions (%1):").arg(extensionCount));

	uint32_t extensionRead;
	std::vector<XrExtensionProperties> extensions(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES } );
	auto getExtensionsResult = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionRead, extensions.data());
	if (getExtensionsResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Unable to get extension information: %1").arg(getExtensionsResult));
		return;
	}
	assert(extensionCount == extensionRead);

	for (uint32_t e = 0; e < extensionRead; ++e)
	{
		LOG(lvlInfo, QString("    %1: name=%2, version=%3")
			.arg(e)
			.arg(extensions[e].extensionName)
			.arg(extensions[e].extensionVersion));
	}

	XrInstanceCreateInfo info = { XR_TYPE_INSTANCE_CREATE_INFO };
	std::string appName = "open_iA";
	strncpy(info.applicationInfo.applicationName, appName.c_str(), XR_MAX_APPLICATION_NAME_SIZE);
	info.applicationInfo.applicationVersion = 1;
	strncpy(info.applicationInfo.engineName, appName.c_str(), XR_MAX_ENGINE_NAME_SIZE);
	info.applicationInfo.engineVersion = 1;
	info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	XrInstance instance = nullptr;
	auto createInstanceResult = xrCreateInstance(&info, &instance);
	if (createInstanceResult != XR_SUCCESS || instance == nullptr)
	{
		LOG(lvlError, QString("    Unable to initialize: %1").arg(createInstanceResult));
		return;
	}

	XrInstanceProperties properties;
	auto getPropsResult = xrGetInstanceProperties(instance, &properties);
	if (getPropsResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Unable to get instance properties: %1").arg(getPropsResult));
		return;
	}
	LOG(lvlInfo, QString("    Instance properties: runtimeName=%1, runtimeVersion=%2")
		.arg(properties.runtimeName)
		.arg(properties.runtimeVersion));

	XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
	XrSystemId systemID = XR_NULL_SYSTEM_ID;
	systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	auto getSystemResult = xrGetSystem(instance, &systemInfo, &systemID);
	if (getSystemResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Unable to get HMD system: %1").arg(getSystemResult));
		return;
	}

	XrSystemProperties systemProps = { XR_TYPE_SYSTEM_PROPERTIES };
	auto getSystemPropsResult = xrGetSystemProperties(instance, systemID, &systemProps);
	if (getSystemPropsResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Unable to get system properties: %1").arg(getSystemPropsResult));
		return;
	}
	LOG(lvlInfo, QString("    System properties: name=%1, systemId=%2, vendorId=%3, graphicsProps(max LayerCount=%4, ImageHeight=%5, ImageWidth=%6), trackingProps(orientation=%7, position=%8)")
		.arg(systemProps.systemName)
		.arg(systemProps.systemId)
		.arg(systemProps.vendorId)
		.arg(systemProps.graphicsProperties.maxLayerCount)
		.arg(systemProps.graphicsProperties.maxSwapchainImageHeight)
		.arg(systemProps.graphicsProperties.maxSwapchainImageWidth)
		.arg(systemProps.trackingProperties.orientationTracking)
		.arg(systemProps.trackingProperties.positionTracking));

	XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	uint32_t blendCount = 0;
	auto getBlendModeCountResult = xrEnumerateEnvironmentBlendModes(instance, systemID, app_config_view, 0, &blendCount, nullptr);
	if (getBlendModeCountResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Unable to get count of environment blend modes: %1").arg(getBlendModeCountResult));
		return;
	}

	LOG(lvlInfo, QString("Available environment blend modes (%1):").arg(blendCount));
	uint32_t blendRead;
	std::vector<XrEnvironmentBlendMode> blendModes(blendCount);
	auto enumerateBlendModeResult = xrEnumerateEnvironmentBlendModes(instance, systemID, app_config_view, blendCount, &blendRead, blendModes.data());
	if (enumerateBlendModeResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Unable to get environmet blend mode information: %1").arg(enumerateBlendModeResult));
		return;
	}
	for (auto blendModeID : blendModes)
	{
		QString blendMode;
		switch (blendModeID)
		{
		case XR_ENVIRONMENT_BLEND_MODE_OPAQUE: blendMode = "Opaque"; break;
		case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE: blendMode = "Additive"; break;
		case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND: blendMode = "Alpha blend"; break;
		default: blendMode = QString("unknown (%1)").arg(blendModeID);
		}
		LOG(lvlInfo, QString("    %1: %2").arg(blendModeID).arg(blendMode));
	}

	auto destroyInstanceResult = xrDestroyInstance(instance);
	if (destroyInstanceResult != XR_SUCCESS)
	{
		LOG(lvlError, QString("    Error while destroying instance: %1").arg(destroyInstanceResult));
	}

	// further info possibilities:

	// xrEnumerateViewConfigurations     // potentially most interesting
	// xrEnumerateViewConfigurationViews
	// xrEnumerateSwapchainImages
	// xrEnumerateSwapchainFormats       // requires session
	// xrEnumerateReferenceSpaces        // requires session
	// xrEnumerateBoundSourcesForAction  // requires session

	// from extensions:

	// xrEnumerateDisplayRefreshRatesFB  // requires session
	// xrEnumerateRenderModelPathsFB     // requires session
	// xrEnumerateSpaceSupportedComponentsFB
	// xrEnumeratePerformanceMetricsCounterPathsMETA
	// xrEnumerateReprojectionModesMSFT
	// xrEnumerateSceneComputeFeaturesMSFT
	// xrEnumeratePersistedSpatialAnchorNamesMSFT
}
#endif

void iAImNDTModuleInterface::startAnalysis()
{
	if (m_imNDT)
	{
		stopImNDT();
		return;
	}
	iACsvInputDlg dlg(false);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto const& csvConfig = dlg.getConfig();
	auto objData = loadObjectsCSV(csvConfig);
	if (!objData)
	{
		return;
	}
	if (!startImNDT(objData, csvConfig))
	{
		return;
	}
}

void iAImNDTModuleInterface::stopImNDT()
{
	if (!m_imNDT)
	{
		return;
	}
	LOG(lvlInfo, "Stopping ImNDT analysis.");
	connect(m_imNDT.get(), &iAImNDTMain::finished, this, [this]()
	{
		m_imNDT.reset();
		checkStopVR();
		emit analysisStopped();
	});
	queueTask([this] { m_imNDT->stop(); });
	m_actionVRStartAnalysis->setText("Start Analysis");
}

namespace
{
	bool backendAvailable(iAvtkVR::Backend backend)
	{
		// only OpenVR currently provides a way to check for runtime/HMD presence without starting the environment:
		if (backend != iAvtkVR::OpenVR)
		{
			return true;
		}
#ifdef OPENVR_AVAILABLE
		if (!vr::VR_IsRuntimeInstalled())
		{
			LOG(lvlWarn, "OpenVR: Runtime not found. Please install Steam and SteamVR!");
			//QMessageBox::warning(m_mainWnd, "VR", "VR runtime not found. Please install Steam and SteamVR!");
			return false;
		}
		if (!vr::VR_IsHmdPresent())
		{
			LOG(lvlWarn, "OpenVR: No VR device found. Make sure your HMD device is plugged in and turned on!");
			//QMessageBox::warning(m_mainWnd, "VR", "No VR device found. Make sure your HMD device is plugged in and turned on!");
			return false;
		}
#endif
		return true;
	}
}

bool iAImNDTModuleInterface::setupVREnvironment()
{
	if (m_vrEnv && m_vrEnv->isRunning())
	{
		QString msg("VR environment is currently running! Please stop the ongoing VR analysis before starting a new one!");
		LOG(lvlInfo, msg);
		QMessageBox::information(m_mainWnd, "VR", msg);
		return false;
	}
	auto const& backends = iAvtkVR::availableBackends();
	if (backends.size() < 1)
	{
		QString msg("Invalid configuration: VR module built without without VR backends; please report this in our issue tracker (https://github.com/3dct/open_iA/issues)!");
		QMessageBox::information(m_mainWnd, "VR", msg);
		LOG(lvlError, msg);
		return false;
	}
	auto backend = backends[0];
	if (backends.size() > 1)
	{// quick and dirty: if more than one backends available, let user choose each time. TODO: Store user choice (option - store / ask each time?) !
		QStringList backendNames;
		for (auto b : backends)
		{
			backendNames << iAvtkVR::backendName(b);
		}
		iAAttributes a;
		addAttr(a, "Backend", iAValueType::Categorical, backendNames);
		iAParameterDlg dlg(m_mainWnd, "VR Backend", a);
		if (dlg.exec() != QDialog::Accepted)
		{
			return false;
		}
		backend = (dlg.parameterValues()["Backend"].toString() == iAvtkVR::backendName(iAvtkVR::OpenVR)) ? iAvtkVR::OpenVR : iAvtkVR::OpenXR;
	}
	if (!backendAvailable(backend))
	{
		QString msg("Backend is currently unavailable! Please see additional messages in the log for how to fix, or try a different backend.");
		QMessageBox::information(m_mainWnd, "VR", msg);
		LOG(lvlWarn, msg);
		return false;
	}
	m_vrEnv = std::make_shared<iAVREnvironment>(backend);
	return true;
}

bool iAImNDTModuleInterface::ensureVREnvironment()
{
	if (m_vrEnv)
	{
		return true;
	}
	if (!setupVREnvironment())
	{
		return false;
	}
	connect(m_vrEnv.get(), &iAVREnvironment::finished, this, [this]
	{
		m_vrEnv.reset();
		m_imNDT.reset();
		m_vrRenderers.clear();
		for (auto vrAction : m_vrActions)
		{
			vrAction->setChecked(false);
		}
	});
	return true;
}

void iAImNDTModuleInterface::checkStopVR()
{
	if (m_vrRenderers.empty() && !m_imNDT && m_vrEnv)    // check m_vrEnv because VR might already be finished due to errors (e.g. headset currently not available)
	{
		m_vrEnv->stop();  // no more VR renderers and no objects analysis -> stop VR environment
	}
}

bool iAImNDTModuleInterface::startImNDT(std::shared_ptr<iAObjectsData> objData, iACsvConfig csvConfig)
{
	if (csvConfig.visType == iAObjectVisType::UseVolume || csvConfig.visType == iAObjectVisType::None)
	{
		LOG(lvlError, "Invalid visualization type!");
		return false;
	}
	m_objData = objData;
	auto objVis = createObjectVis(m_objData.get(), QColor(140, 140, 140, 255));
	m_polyObject = std::dynamic_pointer_cast<iAColoredPolyObjectVis>(objVis);
	if (!m_polyObject)
	{
		LOG(lvlError, "Invalid 3D object visualization!");
		return false;
	}
	LOG(lvlInfo, QString("Starting ImNDT analysis of %1").arg(csvConfig.fileName));
	if (!ensureVREnvironment())
	{
		return false;
	}
	m_imNDT = std::make_shared<iAImNDTMain>(m_vrEnv.get(), m_polyObject.get(), m_objData.get(), csvConfig);
	connect(m_imNDT.get(), &iAImNDTMain::selectionChanged, this, &iAImNDTModuleInterface::selectionChanged);
	m_actionVRStartAnalysis->setText("Stop Analysis");
	if (!m_vrEnv->isRunning())
	{
		m_vrEnv->start();
	}
	return true;
}

vtkRenderer* iAImNDTModuleInterface::getRenderer()
{
	return m_vrEnv->renderer();
}

void iAImNDTModuleInterface::queueTask(std::function<void()> task)
{
	if (!m_vrEnv)
	{
		return;
	}
	m_vrEnv->queueTask(task);
}

bool iAImNDTModuleInterface::isVRRunning() const
{
	return m_vrEnv && m_vrEnv->isRunning();
}

bool iAImNDTModuleInterface::isImNDTRunning() const
{
	return isVRRunning() && m_imNDT;
}

void iAImNDTModuleInterface::setSelection(std::vector<size_t> selection)
{
	m_polyObject->setSelection(selection, true);
}

std::vector<size_t> iAImNDTModuleInterface::selection()
{
	return m_polyObject->selection();
}
