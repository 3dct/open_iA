#include "iAXVRAModuleInterface.h"

#include "iAMainWindow.h"
#include "iAMdiChild.h"
#include "iARenderer.h"
#include "vtkOpenglRenderer.h"
#include "vtkCamera.h"

// objectvis
#include "iA3DObjectFactory.h"
#include "dlg_CSVInput.h"
#include "iACsvConfig.h"
#include "iACsvVtkTableCreator.h"

// FeatureScout
#include "dlg_FeatureScout.h"
#include "iAFeatureScoutToolbar.h"

// ImNDT
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

	m_actionXVRA_ARView = new QAction(tr("Start AR Environment"), m_mainWnd);
	connect(m_actionXVRA_ARView, &QAction::triggered, this, &iAXVRAModuleInterface::startARView);
	m_actionXVRA_ARView->setDisabled(true);

	QMenu* vrMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("XVRA"), false);
	vrMenu->addAction(actionXVRAInfo);
	vrMenu->addAction(actionXVRAStart);
	vrMenu->addAction(m_actionXVRA_ARView);

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
	auto child = m_mainWnd->createMdiChild(false);
	m_fsMain = new dlg_FeatureScout(child, csvConfig.objectType, csvConfig.fileName, creator.table(),
		csvConfig.visType, io.getOutputMapping(), m_polyObject);
	iAFeatureScoutToolbar::addForChild(m_mainWnd, child);

	/***** Start VR *****/

	m_vrMain = new iAImNDTModuleInterface();
	m_vrMain->ImNDT(m_polyObject, m_objectTable, io, csvConfig);

	/***** Add Camera Frustum *****/

	//Get camera from featureScout
	vtkSmartPointer<vtkCamera> fsCam = m_mainWnd->activeMdiChild()->renderer()->camera(); //m_mainWnd->activeMdiChild()->renderer()->renderer()->GetActiveCamera();

	//Get camera from featureScout
	vtkSmartPointer<vtkCamera> vrCam = m_vrMain->getRenderer()->GetActiveCamera();

	double const* bounds = m_polyObject->bounds();
	double maxSize = std::max({bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]});
	vrFrustum = new iAFrustumActor(m_vrMain->getRenderer(), fsCam, maxSize/10);  // frustum of featurescout shown in vr
	fsFrustum = new iAFrustumActor(m_mainWnd->activeMdiChild()->renderer()->labelRenderer(), vrCam, maxSize / 10);  // frustum of vr shown in featurescout

	//causes invalid access; maybe mutex required?
	//connect(fsFrustum, &iAFrustumActor::updateRequired, m_mainWnd->activeMdiChild(), &iAMdiChild::updateRenderer);

	vrFrustum->show();
	fsFrustum->show();

	//Enable AR Mode
	m_actionXVRA_ARView->setEnabled(true);

	connect(m_vrMain, &iAImNDTModuleInterface::selectionChanged, m_fsMain, &dlg_FeatureScout::selectionChanged3D);
}

void iAXVRAModuleInterface::startARView()
{
	if (m_vrMain->toggleARView())
	{
		m_actionXVRA_ARView->setText("Stop AR Environment");
	}
	else
	{
		m_actionXVRA_ARView->setText("Start AR Environment");
	}

}