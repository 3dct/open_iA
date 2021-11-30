#include "iAXVRAModuleInterface.h"

#include "iAMainWindow.h"
#include "iAMdiChild.h"
#include "iARenderer.h"
#include "vtkOpenglRenderer.h"
#include "vtkCamera.h"

#include "dlg_FeatureScout.h"
#include "iA3DObjectFactory.h"
#include "dlg_CSVInput.h"
#include "iACsvConfig.h"
#include "iACsvVtkTableCreator.h"

#include "iAImNDTModuleInterface.h"

#include "iAFrustumActor.h"

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

	vtkSmartPointer<vtkTable> m_objectTable = creator.table();

	//Create PolyObject
	m_polyObject = create3DObjectVis(
		csvConfig.visType, m_objectTable, io.getOutputMapping(), QColor(140, 140, 140, 255), curvedFiberInfo)
		.dynamicCast<iA3DColoredPolyObjectVis>();
	if (!m_polyObject)
	{
		LOG(lvlError, "Invalid 3D object visualization!");
	}


	/***** Start Featurescout *****/
	m_fsMain = new dlg_FeatureScout(m_mainWnd->createMdiChild(false), csvConfig.objectType, csvConfig.fileName, creator.table(),
		csvConfig.visType, io.getOutputMapping(), m_polyObject);

	/***** Start VR *****/

	m_vrMain = new iAImNDTModuleInterface();
	m_vrMain->ImNDT(m_polyObject, m_objectTable, io, csvConfig);

	/***** Add Camera Frustum *****/

	//Get camera from featureScout
	vtkSmartPointer<vtkCamera> fsCam = m_mainWnd->activeMdiChild()->renderer()->camera(); //m_mainWnd->activeMdiChild()->renderer()->renderer()->GetActiveCamera();

	//Get camera from featureScout
	vtkSmartPointer<vtkCamera> vrCam = m_vrMain->getRenderer()->GetActiveCamera();

	//vrFrustum = new iAFrustumActor(m_vrMain->getRenderer(), fsCam); // frustum of featurescout shown in vr
	fsFrustum = new iAFrustumActor(m_mainWnd->activeMdiChild()->renderer()->renderer(), vrCam); // frustum of vr shown in featurescout

	//vrFrustum->show();
	fsFrustum->show();
}