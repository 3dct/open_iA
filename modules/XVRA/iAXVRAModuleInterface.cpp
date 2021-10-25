#include "iAXVRAModuleInterface.h"

#include "iAMainWindow.h"
#include "iAModuleDispatcher.h"

#include "dlg_FeatureScout.h"
#include "iA3DObjectFactory.h"
#include "dlg_CSVInput.h"
#include "iACsvConfig.h"
#include "iACsvVtkTableCreator.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>

void iAXVRAModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	{
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	}
	QAction* actionXVRAInfo = new QAction(tr("Info"), m_mainWnd);
	connect(actionXVRAInfo, &QAction::triggered, this, &iAXVRAModuleInterface::info);

	QAction * actionXVRAStart = new QAction(tr("Start XVRA"), m_mainWnd);
	connect(actionXVRAStart, &QAction::triggered, this, &iAXVRAModuleInterface::startXVRA);

	QMenu* vrMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("XVRA"), false);
	vrMenu->addAction(actionXVRAStart);
	vrMenu->addAction(actionXVRAInfo);

	// m_mainWnd->makeActionChildDependent(actionTest);   // uncomment this to enable action only if child window is open
	//m_mainWnd->moduleDispatcher().module<dlg_FeatureScout>();
}

void iAXVRAModuleInterface::info()
{
	QMessageBox::information(m_mainWnd, "XVRA Module", "Cross-Virtuality Analysis of Rich X-Ray Computed Tomography Data for Materials Science Applications");
}

void iAXVRAModuleInterface::startXVRA()
{
	/***** Start csv dialog (-> PolyObject) *****/

	dlg_CSVInput dlg(false);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	iACsvConfig csvConfig = dlg.getConfig();

	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
	{
		return;
	}

	std::map<size_t, std::vector<iAVec3f> > curvedFiberInfo;

	if (csvConfig.visType == iACsvConfig::Cylinders || csvConfig.visType == iACsvConfig::Lines)
	{
		if (!readCurvedFiberInfo(csvConfig.curvedFiberFileName, curvedFiberInfo))
		{
			curvedFiberInfo = std::map<size_t, std::vector<iAVec3f>>();
		}
	}

	m_objectTable = creator.table();

	//Create PolyObject
	m_polyObject = create3DObjectVis(
		csvConfig.visType, m_objectTable, io.getOutputMapping(), QColor(140, 140, 140, 255), curvedFiberInfo)
		.dynamicCast<iA3DColoredPolyObjectVis>();
	if (!m_polyObject)
	{
		LOG(lvlError, "Invalid 3D object visualization!");
	}


	/***** Start Featurescout *****/
	//m_fsMain = new iAFeatureScoutAttachment(m_mainWnd, m_mdiChild);
	//m_fsMain->init(csvConfig.objectType, csvConfig.fileName, creator.table(), csvConfig.visType, io.getOutputMapping(),
	//	curvedFiberInfo, csvConfig.cylinderQuality, csvConfig.segmentSkip);

	m_fsMain = new dlg_FeatureScout(m_mainWnd->createMdiChild(false), csvConfig.objectType, csvConfig.fileName, creator.table(),
		csvConfig.visType, io.getOutputMapping(), m_polyObject);


	/***** Start VR *****/

	//if (!m_vrEnv)
	//	m_vrEnv.reset(new iAVREnvironment());

	//if (m_vrEnv->isRunning())
	//{
	//	m_vrEnv->stop();
	//	return;
	//}

	////if (!vrAvailable())
	////{
	////	return;
	////}

	////Create InteractorStyle
	//m_style = vtkSmartPointer<iAVRInteractorStyle>::New();
	//m_vrMain = new iAVRMain(m_vrEnv.data(), m_style, m_polyObject.data(), m_objectTable, io, csvConfig);

	//// Start Render Loop HERE!
	//m_vrEnv->start();
}