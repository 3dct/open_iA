/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "mainwindow.h"

#include "charts/iADiagramFctWidget.h"
#include "defines.h"
#include "dlg_commoninput.h"
#include "dlg_datatypeconversion.h"
#include "dlg_openfile_sizecheck.h"
#include "iAChartFunctionBezier.h"
#include "iAChartFunctionGaussian.h"
#include "iAChartFunctionTransfer.h"
#include "iACheckOpenGL.h"
#include "iAConsole.h"
#include "iALogger.h"
#include "iAMathUtility.h"
#include "iAModuleDispatcher.h"
#include "iAProjectBase.h"
#include "iAProjectRegistry.h"
#include "iARenderer.h"
#include "iASavableProject.h"
#include "iASlicer.h"
#include "iAToolsVTK.h"
#include "iAXmlSettings.h"
#include "io/iAFileUtils.h"    // for fileNameOnly
#include "io/iAIOProvider.h"
#include "io/iATLGICTLoader.h"
#include "mdichild.h"

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVersion.h>

#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QMdiSubWindow>
#include <QSettings>
#include <QSignalMapper>
#include <QSplashScreen>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QtXml/QDomDocument>
#include <QDesktopServices>

MainWindow::MainWindow(QString const & appName, QString const & version, QString const & splashImage )
:
	QMainWindow(),
	m_moduleDispatcher( new iAModuleDispatcher( this ) ),
	m_gitVersion(version)
{
	setupUi(this);
	setAcceptDrops(true);

	// restore geometry and state
	QCoreApplication::setOrganizationName("FHW");
	QCoreApplication::setOrganizationDomain("3dct.at");
	QCoreApplication::setApplicationName(appName);
	setWindowTitle(appName + " " + m_gitVersion);
	QSettings settings;
	m_path = settings.value("Path").toString();
	restoreGeometry(settings.value("geometry", saveGeometry()).toByteArray());
	restoreState(settings.value("state", saveState()).toByteArray());

	QPixmap pixmap( splashImage );
	m_splashScreen = new QSplashScreen(pixmap);
	m_splashScreen->setWindowFlags(m_splashScreen->windowFlags() | Qt::WindowStaysOnTopHint);
	m_splashScreen->show();
	m_splashScreen->showMessage("\n      Reading settings...", Qt::AlignTop, QColor(255, 255, 255));
	readSettings();

	m_splashTimer = new QTimer();
	m_splashTimer->setSingleShot(true);
	connect(m_splashTimer, SIGNAL(timeout()), this, SLOT(hideSplashSlot()));
	m_splashTimer->start(2000);

	m_splashScreen->showMessage("\n      Setup UI...", Qt::AlignTop, QColor(255, 255, 255));
	applyQSS();
	actionLinkViews->setChecked(m_defaultSlicerSettings.LinkViews);//removed from readSettings, if is needed at all?
	actionLinkMdis->setChecked(m_defaultSlicerSettings.LinkMDIs);
	setCentralWidget(mdiArea);

	m_windowMapper = new QSignalMapper(this);

	createRecentFileActions();
	connectSignalsToSlots();
	updateMenus();
	m_slicerToolsGroup = new QActionGroup(this);
	m_slicerToolsGroup->setExclusive(false);
	m_slicerToolsGroup->addAction(actionSnakeSlicer);
	m_slicerToolsGroup->addAction(actionRawProfile);

	actionDeletePoint->setEnabled(false);
	actionChangeColor->setEnabled(false);

	m_splashScreen->showMessage(tr("\n      Version: %1").arg (m_gitVersion), Qt::AlignTop, QColor(255, 255, 255));

	m_layout = new QComboBox(this);
	for (int i=0; i< m_layoutNames.size(); ++i)
	{
		m_layout->addItem(m_layoutNames[i]);
		if (m_layoutNames[i] == m_defaultLayout)
		{
			m_layout->setCurrentIndex(i);
		}
	}
	m_layout->setStyleSheet("padding: 0");
	m_layout->resize(m_layout->geometry().width(), 100);
	m_layout->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	this->layoutToolbar->insertWidget(this->actionSaveLayout, m_layout);

	// why do we use iAConsoleLogger::get here and not iAConsole::instance()?
	m_moduleDispatcher->InitializeModules(iAConsoleLogger::get());
	setModuleActionsEnabled( false );
	statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow()
{
	// save geometry and state
	QSettings settings;
	settings.setValue("geometry", saveGeometry());
	settings.setValue("state", saveState());

	m_moduleDispatcher->SaveModulesSettings();
	delete m_windowMapper;
	m_windowMapper = nullptr;
}

void MainWindow::hideSplashSlot()
{
	m_splashScreen->finish(this);
	delete m_splashTimer;
}

void MainWindow::quitTimerSlot()
{
	delete m_quitTimer;
	qApp->closeAllWindows();
}

bool MainWindow::keepOpen()
{
	bool childHasChanges = false;
	for (MdiChild* mdiChild: mdiChildList())
		childHasChanges |= mdiChild->isWindowModified();
	if (childHasChanges)
	{
		auto reply = QMessageBox::question(this, "Unsaved changes",
			"One or more windows have unsaved changes. Are you sure you want to close?",
			QMessageBox::Yes | QMessageBox::No);
		if (reply != QMessageBox::Yes)
		{
			return true;
		}
		else
		{ // avoid individual questions for each window
			for (MdiChild* mdiChild: mdiChildList())
				mdiChild->setWindowModified(false);
		}
	}
	return false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (keepOpen())
	{
		event->ignore();
		return;
	}
	mdiArea->closeAllSubWindows();
	if (activeMdiChild())
	{
		event->ignore();
	}
	else
	{
		writeSettings();
		iAConsole::closeInstance();
		event->accept();
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
	if (e->mimeData()->hasUrls())
	{
		e->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent *e)
{
	for(const QUrl &url: e->mimeData()->urls())
	{
		QString fileName = url.toLocalFile();
		loadFile(fileName);
	}
}

void MainWindow::closeAllSubWindows()
{
	if (!keepOpen())
	{
		mdiArea->closeAllSubWindows();
	}
}

void MainWindow::open()
{
	loadFiles(
		QFileDialog::getOpenFileNames(
			this,
			tr("Open Files"),
			m_path,
			iAIOProvider::GetSupportedLoadFormats()
		)
	);
}

void MainWindow::openRaw()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		tr("Open Raw File"),
		m_path,
		"Raw File (*)"
	);

	if (fileName.isEmpty())
		return;

	MdiChild *child = createMdiChild(false);
	QString t; t = fileName; t.truncate(t.lastIndexOf('/'));
	m_path = t;
	if (child->loadRaw(fileName))
	{
		child->show();
	}
	else
	{
		statusBar()->showMessage(tr("FILE LOADING FAILED!"), 10000);
		child->close();
	}
}

void MainWindow::openImageStack()
{
	loadFile(
		QFileDialog::getOpenFileName(
			this,
			tr("Open File"),
			m_path,
			iAIOProvider::GetSupportedImageStackFormats()
		), true
	);
}

void MainWindow::openVolumeStack()
{
	loadFile(
		QFileDialog::getOpenFileName(
			this,
			tr("Open File"),
			m_path,
			iAIOProvider::GetSupportedVolumeStackFormats()
		), true
	);
}

void MainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	QString fileName = action->data().toString();
	loadFile(fileName);
}

void MainWindow::loadFile(QString const & fileName)
{
	QFileInfo fi(fileName);
	if (fi.isDir())
	{
		loadTLGICTData(fileName);
	}
	else
	{
		loadFile(fileName, fileName.toLower().endsWith(".volstack"));
	}
}

void MainWindow::loadFile(QString fileName, bool isStack)
{
	if (fileName.isEmpty())
		return;
	statusBar()->showMessage(tr("Loading data..."), 5000);
	QString t; t = fileName; t.truncate(t.lastIndexOf('/'));
	m_path = t;
	if (QString::compare(QFileInfo(fileName).suffix(), "STL", Qt::CaseInsensitive) == 0)
	{
		if (activeMdiChild())
		{
			QMessageBox msgBox;
			msgBox.setText("Active window detected.");
			msgBox.setInformativeText("Load polydata in the active window?");
			msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
			msgBox.setDefaultButton(QMessageBox::Yes);

			int ret = msgBox.exec();
			if (ret == QMessageBox::Yes)
			{
				activeMdiChild()->loadFile(fileName, false);
			}
			else if (ret == QMessageBox::No)
			{
				MdiChild *child = createMdiChild(false);
				if (child->loadFile(fileName, false)) {
					child->show();
				} else {
					statusBar()->showMessage(tr("FILE LOADING FAILED!"), 10000);
					child->close();
				}
			}
			return;
		}
	}
	if (fileName.toLower().endsWith(iAIOProvider::NewProjectFileExtension))
	{
		QSettings projectFile(fileName, QSettings::IniFormat);
		projectFile.setIniCodec("UTF-8");
		// TODO: asynchronous loading, merge with mdichild: loadFile project init parts
		if (!projectFile.value("UseMdiChild", false).toBool())
		{
			auto registeredProjects = iAProjectRegistry::projectKeys();
			auto projectFileGroups = projectFile.childGroups();
			for (auto projectKey : registeredProjects)
			{
				if (projectFileGroups.contains(projectKey))
				{
					auto project = iAProjectRegistry::createProject(projectKey);
					project->setMainWindow(this);
					projectFile.beginGroup(projectKey);
					project->loadProject(projectFile, fileName);
					projectFile.endGroup();
				}
			}
			return;
		}
	}
	// Todo: hook for plugins?
	MdiChild *child = createMdiChild(false);
	if (child->loadFile(fileName, isStack)) {
		child->show();
	}
	else
	{
		statusBar()->showMessage(tr("FILE LOADING FAILED!"), 10000);
		child->close();
	}
}

void MainWindow::loadFiles(QStringList fileNames)
{
	for (int i = 0; i < fileNames.length(); i++)
	{
		loadFile(fileNames[i]);
	}
}

void MainWindow::save()
{
	if (activeMdiChild())
		activeMdiChild()->save();
}

void MainWindow::saveAs()
{
	if (activeMdiChild())
		activeMdiChild()->saveAs();
}

bool MainWindow::saveSettings()
{
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		QStringList inList = (QStringList() << tr("$Camera")
			<< tr("$Slice Views")
			<< tr("$Transfer Function")
			<< tr("$Probability Functions")
			<< tr("$Preferences")
			<< tr("$Render Settings")
			<< tr("$Slice Settings"));
		QList<QVariant> inPara;
		inPara
			<< m_spCamera
			<< m_spSliceViews
			<< m_spTransferFunction
			<< m_spProbabilityFunctions
			<< m_spPreferences
			<< m_spRenderSettings
			<< m_spSlicerSettings;

		dlg_commoninput dlg(this, "Save Settings", inList, inPara, nullptr);

		if (dlg.exec() == QDialog::Accepted)
		{
			m_spCamera = dlg.getCheckValue(0) != 0;
			m_spSliceViews = dlg.getCheckValue(1) != 0;
			m_spTransferFunction = dlg.getCheckValue(2) != 0;
			m_spProbabilityFunctions = dlg.getCheckValue(3) != 0;
			m_spPreferences = dlg.getCheckValue(4) != 0;
			m_spRenderSettings = dlg.getCheckValue(5) != 0;
			m_spSlicerSettings = dlg.getCheckValue(6) != 0;

			iAXmlSettings xml;

			if (m_spCamera)
				saveCamera(xml);
			if (m_spSliceViews)
				saveSliceViews(xml);
			if (m_spTransferFunction && activeMdiChild()->histogram())
				xml.saveTransferFunction((iAChartTransferFunction*)activeMdiChild()->functions()[0]);
			if (m_spProbabilityFunctions && activeMdiChild()->histogram())
				activeMdiChild()->histogram()->saveProbabilityFunctions(xml);
			if (m_spPreferences)
				savePreferences(xml);
			if (m_spRenderSettings)
				saveRenderSettings(xml);
			if (m_spSlicerSettings)
				saveSlicerSettings(xml);

			xml.save(fileName);
		}
	}
	return true;
}

void MainWindow::loadSettings()
{
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath ,tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	if (!xml.read(fileName))
	{
		QMessageBox::warning(this, "Loading settings", "An error occurred during xml parsing!");
		return;
	}
	bool camera = false, sliceViews = false, transferFunction = false, probabilityFunctions = false;
	bool preferences = false, renderSettings = false, slicerSettings = false;

	QDomElement root = xml.documentElement();
	QDomNodeList list = root.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		QString nodeName = node.nodeName();
		if (nodeName == "camera") camera = true;
		else if (nodeName == "sliceViews") sliceViews = true;
		else if (nodeName == "functions")
		{
			if (node.namedItem("transfer").isElement()) transferFunction = true;
			if (node.namedItem("bezier").isElement() || node.namedItem("gaussian").isElement())	probabilityFunctions = true;
		}
		else if (nodeName == "preferences") preferences = true;
		else if (nodeName == "renderSettings") renderSettings = true;
		else if (nodeName == "slicerSettings") slicerSettings = true;
	}

	QStringList inList = QStringList();
	QList<QVariant> inPara;
	if (camera)               { inList << tr("$Camera");                inPara << tr("%1").arg(m_lpCamera ? tr("true") : tr("false")); }
	if (sliceViews)           { inList << tr("$Slice Views");           inPara << tr("%1").arg(m_lpSliceViews ? tr("true") : tr("false")); }
	if (transferFunction)     { inList << tr("$Transfer Function");     inPara << tr("%1").arg(m_lpTransferFunction ? tr("true") : tr("false")); }
	if (probabilityFunctions) { inList << tr("$Probability Functions"); inPara << tr("%1").arg(m_lpProbabilityFunctions ? tr("true") : tr("false")); }
	if (preferences)          { inList << tr("$Preferences");           inPara << tr("%1").arg(m_lpPreferences ? tr("true") : tr("false")); }
	if (renderSettings)       { inList << tr("$Render Settings");       inPara << tr("%1").arg(m_lpRenderSettings ? tr("true") : tr("false")); }
	if (slicerSettings)       { inList << tr("$Slice Settings");        inPara << tr("%1").arg(m_lpSlicerSettings ? tr("true") : tr("false")); }

	dlg_commoninput dlg(this, "Load Settings", inList, inPara, NULL);

	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	int index = 0;
	if (camera)               { dlg.getCheckValue(index++) == 0 ? m_lpCamera = false               : m_lpCamera = true; }
	if (sliceViews)           { dlg.getCheckValue(index++) == 0 ? m_lpSliceViews = false           : m_lpSliceViews = true; }
	if (transferFunction)     { dlg.getCheckValue(index++) == 0 ? m_lpTransferFunction = false     : m_lpTransferFunction = true; }
	if (probabilityFunctions) { dlg.getCheckValue(index++) == 0 ? m_lpProbabilityFunctions = false : m_lpProbabilityFunctions = true; }
	if (preferences)          { dlg.getCheckValue(index++) == 0 ? m_lpPreferences = false          : m_lpPreferences = true; }
	if (renderSettings)       { dlg.getCheckValue(index++) == 0 ? m_lpRenderSettings = false       : m_lpRenderSettings = true; }
	if (slicerSettings)       { dlg.getCheckValue(index++) == 0 ? m_lpSlicerSettings = false       : m_lpSlicerSettings = true; }

	if (m_lpProbabilityFunctions)
	{
		std::vector<iAChartFunction*> &functions = activeMdiChild()->functions();
		for (unsigned int i = 1; i < functions.size(); i++)
		{
			delete functions.back();
			functions.pop_back();
		}
	}

	if (m_lpCamera)
		loadCamera(xml);
	if (m_lpSliceViews && xml.hasElement("sliceViews"))
		loadSliceViews(xml.node("sliceViews"));
	if (activeMdiChild()->histogram())
	{
		if (m_lpTransferFunction)
			activeMdiChild()->histogram()->loadTransferFunction(xml.documentElement().namedItem("functions"));
		if (m_lpProbabilityFunctions)
			activeMdiChild()->histogram()->loadProbabilityFunctions(xml);
		activeMdiChild()->redrawHistogram();
	}
	if (m_lpPreferences && xml.hasElement("preferences"))
		loadPreferences(xml.node("preferences"));
	if (m_lpRenderSettings && xml.hasElement("renderSettings"))
		loadRenderSettings(xml.node("renderSettings"));
	if (m_lpSlicerSettings && xml.hasElement("slicerSettings"))
		loadSlicerSettings(xml.node("slicerSettings"));
}

void MainWindow::saveCamera(iAXmlSettings & xml)
{
	vtkCamera *camera = activeMdiChild()->renderer()->renderer()->GetActiveCamera();
	QDomElement cameraElement = xml.createElement("camera");
	saveCamera(cameraElement, camera);
}

bool MainWindow::loadCamera(iAXmlSettings & xml)
{
	vtkCamera *camera = activeMdiChild()->renderer()->renderer()->GetActiveCamera();
	if (!xml.hasElement("camera"))
		return false;
	loadCamera(xml.node("camera"), camera);

	double allBounds[6];
	activeMdiChild()->renderer()->renderer()->ComputeVisiblePropBounds( allBounds );
	activeMdiChild()->renderer()->renderer()->ResetCameraClippingRange( allBounds );
	return true;
}

void MainWindow::saveSliceViews(iAXmlSettings & xml)
{
	QDomNode sliceViewsNode = xml.createElement("sliceViews");
	for (int i=0; i<iASlicerMode::SlicerCount; ++i)
		saveSliceView(xml.document(), sliceViewsNode, activeMdiChild()->slicer(i)->camera(), slicerModeString(i));
}

void MainWindow::saveSliceView(QDomDocument &doc, QDomNode &sliceViewsNode, vtkCamera *cam, QString const & elemStr)
{
	// add new slice view node
	QDomElement cameraElement = doc.createElement(elemStr);

	saveCamera(cameraElement, cam);

	sliceViewsNode.appendChild(cameraElement);
}

void MainWindow::loadCamera(QDomNode const & node, vtkCamera* camera)
{
	QDomNamedNodeMap attributes = node.attributes();
	double position[4], focalPoint[4], viewUp[4];
	position[0] = attributes.namedItem("positionX").nodeValue().toDouble();
	position[1] = attributes.namedItem("positionY").nodeValue().toDouble();
	position[2] = attributes.namedItem("positionZ").nodeValue().toDouble();
	position[3] = attributes.namedItem("positionW").nodeValue().toDouble();
	focalPoint[0] = attributes.namedItem("focalPointX").nodeValue().toDouble();
	focalPoint[1] = attributes.namedItem("focalPointY").nodeValue().toDouble();
	focalPoint[2] = attributes.namedItem("focalPointZ").nodeValue().toDouble();
	focalPoint[3] = attributes.namedItem("focalPointW").nodeValue().toDouble();
	viewUp[0] = attributes.namedItem("viewUpX").nodeValue().toDouble();
	viewUp[1] = attributes.namedItem("viewUpY").nodeValue().toDouble();
	viewUp[2] = attributes.namedItem("viewUpZ").nodeValue().toDouble();
	viewUp[3] = attributes.namedItem("viewUpW").nodeValue().toDouble();

	camera->SetPosition(position);
	camera->SetFocalPoint(focalPoint);
	camera->SetViewUp(viewUp);
	if (attributes.contains("scale"))
	{
		double scale = attributes.namedItem("scale").nodeValue().toDouble();
		camera->SetParallelScale(scale);
	}
}

void MainWindow::saveCamera(QDomElement &cameraElement, vtkCamera* camera)
{
	double position[4], focalPoint[4], viewUp[4];
	camera->GetPosition(position);
	camera->GetFocalPoint(focalPoint);
	camera->GetViewUp(viewUp);
	position[3] = 1.0; focalPoint[3] = 1.0; viewUp[3] = 1.0;

	cameraElement.setAttribute("positionX", tr("%1").arg(position[0]));
	cameraElement.setAttribute("positionY", tr("%1").arg(position[1]));
	cameraElement.setAttribute("positionZ", tr("%1").arg(position[2]));
	cameraElement.setAttribute("positionW", tr("%1").arg(position[3]));
	cameraElement.setAttribute("focalPointX", tr("%1").arg(focalPoint[0]));
	cameraElement.setAttribute("focalPointY", tr("%1").arg(focalPoint[1]));
	cameraElement.setAttribute("focalPointZ", tr("%1").arg(focalPoint[2]));
	cameraElement.setAttribute("focalPointW", tr("%1").arg(focalPoint[3]));
	cameraElement.setAttribute("viewUpX", tr("%1").arg(viewUp[0]));
	cameraElement.setAttribute("viewUpY", tr("%1").arg(viewUp[1]));
	cameraElement.setAttribute("viewUpZ", tr("%1").arg(viewUp[2]));
	cameraElement.setAttribute("viewUpW", tr("%1").arg(viewUp[3]));

	if (camera->GetParallelProjection())
	{
		double scale = camera->GetParallelScale();
		cameraElement.setAttribute("scale", tr("%1").arg(scale));
	}
}

void MainWindow::loadSliceViews(QDomNode sliceViewsNode)
{
	QDomNodeList list = sliceViewsNode.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		vtkCamera *camera;
		if      (node.nodeName() == "XY") camera = activeMdiChild()->slicer(iASlicerMode::XY)->camera();
		else if (node.nodeName() == "YZ") camera = activeMdiChild()->slicer(iASlicerMode::YZ)->camera();
		else                              camera = activeMdiChild()->slicer(iASlicerMode::XZ)->camera();
		loadCamera(node, camera);
	}
}

void MainWindow::saveTransferFunction(QDomDocument &doc, iAChartTransferFunction* transferFunction)
{
	// does functions node exist
	QDomNode functionsNode = doc.documentElement().namedItem("functions");
	if (!functionsNode.isElement())
	{
		functionsNode = doc.createElement("functions");
		doc.documentElement().appendChild(functionsNode);
	}

	// add new function node
	QDomElement transferElement = doc.createElement("transfer");

	for (int i = 0; i < transferFunction->opacityTF()->GetSize(); i++)
	{
		double opacityTFValue[4];
		double colorTFValue[6];
		transferFunction->opacityTF()->GetNodeValue(i, opacityTFValue);
		transferFunction->colorTF()->GetNodeValue(i, colorTFValue);

		QDomElement nodeElement = doc.createElement("node");
		nodeElement.setAttribute("value",   tr("%1").arg(opacityTFValue[0]));
		nodeElement.setAttribute("opacity", tr("%1").arg(opacityTFValue[1]));
		nodeElement.setAttribute("red",     tr("%1").arg(colorTFValue[1]));
		nodeElement.setAttribute("green",   tr("%1").arg(colorTFValue[2]));
		nodeElement.setAttribute("blue",    tr("%1").arg(colorTFValue[3]));
		transferElement.appendChild(nodeElement);
	}

	functionsNode.appendChild(transferElement);
}

void MainWindow::savePreferences(iAXmlSettings &xml)
{
	QDomElement preferencesElement = xml.createElement("preferences");
	preferencesElement.setAttribute("histogramBins", tr("%1").arg(m_defaultPreferences.HistogramBins));
	preferencesElement.setAttribute("statisticalExtent", tr("%1").arg(m_defaultPreferences.StatisticalExtent));
	preferencesElement.setAttribute("compression", tr("%1").arg(m_defaultPreferences.Compression));
	preferencesElement.setAttribute("printParameters", tr("%1").arg(m_defaultPreferences.PrintParameters));
	preferencesElement.setAttribute("resultsInNewWindow", tr("%1").arg(m_defaultPreferences.ResultInNewWindow));
	preferencesElement.setAttribute("magicLensSize", tr("%1").arg(m_defaultPreferences.MagicLensSize));
	preferencesElement.setAttribute("magicLensFrameWidth", tr("%1").arg(m_defaultPreferences.MagicLensFrameWidth));
	preferencesElement.setAttribute("logToFile", tr("%1").arg(iAConsole::instance()->isLogToFileOn()));
}

void MainWindow::loadPreferences(QDomNode preferencesNode)
{
	QDomNamedNodeMap attributes = preferencesNode.attributes();
	m_defaultPreferences.HistogramBins = attributes.namedItem("histogramBins").nodeValue().toInt();
	m_defaultPreferences.StatisticalExtent = attributes.namedItem("statisticalExtent").nodeValue().toDouble();
	m_defaultPreferences.Compression = attributes.namedItem("compression").nodeValue() == "1";
	m_defaultPreferences.PrintParameters = attributes.namedItem("printParameters").nodeValue() == "1";
	m_defaultPreferences.ResultInNewWindow = attributes.namedItem("resultsInNewWindow").nodeValue() == "1";
	m_defaultPreferences.MagicLensSize = attributes.namedItem("magicLensSize").nodeValue().toInt();
	m_defaultPreferences.MagicLensFrameWidth = attributes.namedItem("magicLensFrameWidth").nodeValue().toInt();
	bool prefLogToFile = attributes.namedItem("logToFile").nodeValue() == "1";
	QString logFileName = attributes.namedItem("logFile").nodeValue();

	iAConsole::instance()->setLogToFile(prefLogToFile, logFileName);

	activeMdiChild()->editPrefs(m_defaultPreferences);
}

void MainWindow::saveRenderSettings(iAXmlSettings &xml)
{
	QDomElement renderSettingsElement = xml.createElement("renderSettings");
	renderSettingsElement.setAttribute("showSlicers", m_defaultRenderSettings.ShowSlicers);
	renderSettingsElement.setAttribute("showSlicePlanes", m_defaultRenderSettings.ShowSlicePlanes);
	renderSettingsElement.setAttribute("showHelpers", m_defaultRenderSettings.ShowHelpers);
	renderSettingsElement.setAttribute("showRPosition", m_defaultRenderSettings.ShowRPosition);
	renderSettingsElement.setAttribute("parallelProjection", m_defaultRenderSettings.ParallelProjection);
	renderSettingsElement.setAttribute("backgroundTop", m_defaultRenderSettings.BackgroundTop);
	renderSettingsElement.setAttribute("backgroundBottom", m_defaultRenderSettings.BackgroundBottom);
	renderSettingsElement.setAttribute("linearInterpolation", m_defaultVolumeSettings.LinearInterpolation);
	renderSettingsElement.setAttribute("shading", m_defaultVolumeSettings.Shading);
	renderSettingsElement.setAttribute("sampleDistance", m_defaultVolumeSettings.SampleDistance);
	renderSettingsElement.setAttribute("ambientLighting", m_defaultVolumeSettings.AmbientLighting);
	renderSettingsElement.setAttribute("diffuseLighting", m_defaultVolumeSettings.DiffuseLighting);
	renderSettingsElement.setAttribute("specularLighting", m_defaultVolumeSettings.SpecularLighting);
	renderSettingsElement.setAttribute("specularPower", m_defaultVolumeSettings.SpecularPower);
	renderSettingsElement.setAttribute("renderMode", m_defaultVolumeSettings.RenderMode);
}

void MainWindow::loadRenderSettings(QDomNode renderSettingsNode)
{
	QDomNamedNodeMap attributes = renderSettingsNode.attributes();

	m_defaultRenderSettings.ShowSlicers = attributes.namedItem("showSlicers").nodeValue() == "1";
	m_defaultRenderSettings.ShowSlicePlanes = attributes.namedItem("showSlicePlanes").nodeValue() == "1";
	m_defaultRenderSettings.ShowHelpers = attributes.namedItem("showHelpers").nodeValue() == "1";
	m_defaultRenderSettings.ShowRPosition = attributes.namedItem("showRPosition").nodeValue() == "1";
	m_defaultRenderSettings.ParallelProjection = attributes.namedItem("parallelProjection").nodeValue() == "1";
	m_defaultRenderSettings.BackgroundTop = attributes.namedItem("backgroundTop").nodeValue();
	m_defaultRenderSettings.BackgroundBottom = attributes.namedItem("backgroundBottom").nodeValue();

	m_defaultVolumeSettings.LinearInterpolation = attributes.namedItem("linearInterpolation").nodeValue() == "1";
	m_defaultVolumeSettings.Shading = attributes.namedItem("shading").nodeValue() == "1";
	m_defaultVolumeSettings.SampleDistance = attributes.namedItem("sampleDistance").nodeValue().toDouble();
	m_defaultVolumeSettings.AmbientLighting = attributes.namedItem("ambientLighting").nodeValue().toDouble();
	m_defaultVolumeSettings.DiffuseLighting = attributes.namedItem("diffuseLighting").nodeValue().toDouble();
	m_defaultVolumeSettings.SpecularLighting = attributes.namedItem("specularLighting").nodeValue().toDouble();
	m_defaultVolumeSettings.SpecularPower = attributes.namedItem("specularPower").nodeValue().toDouble();
	m_defaultVolumeSettings.RenderMode = attributes.namedItem("renderMode").nodeValue().toInt();

	activeMdiChild()->editRendererSettings(m_defaultRenderSettings, m_defaultVolumeSettings);
}

void MainWindow::saveSlicerSettings(iAXmlSettings &xml)
{
	QDomElement slicerSettingsElement = xml.createElement("slicerSettings");
	slicerSettingsElement.setAttribute("linkViews", m_defaultSlicerSettings.LinkViews);
	slicerSettingsElement.setAttribute("showIsolines", m_defaultSlicerSettings.SingleSlicer.ShowIsoLines);
	slicerSettingsElement.setAttribute("showPosition", m_defaultSlicerSettings.SingleSlicer.ShowPosition);
	slicerSettingsElement.setAttribute("showAxesCaption", m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption);
	slicerSettingsElement.setAttribute("numberOfIsolines", m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines);
	slicerSettingsElement.setAttribute("minIsovalue", m_defaultSlicerSettings.SingleSlicer.MinIsoValue);
	slicerSettingsElement.setAttribute("maxIsovalue", m_defaultSlicerSettings.SingleSlicer.MaxIsoValue);
	slicerSettingsElement.setAttribute("linearInterpolation", m_defaultSlicerSettings.SingleSlicer.LinearInterpolation);
	slicerSettingsElement.setAttribute("snakeSlices", m_defaultSlicerSettings.SnakeSlices);
	slicerSettingsElement.setAttribute("linkMDIs", m_defaultSlicerSettings.LinkMDIs);
	slicerSettingsElement.setAttribute("cursorMode", m_defaultSlicerSettings.SingleSlicer.CursorMode);
	slicerSettingsElement.setAttribute("toolTipFontSize", m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize);
}

void MainWindow::loadSlicerSettings(QDomNode slicerSettingsNode)
{
	QDomNamedNodeMap attributes = slicerSettingsNode.attributes();

	m_defaultSlicerSettings.LinkViews = attributes.namedItem("linkViews").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.ShowIsoLines = attributes.namedItem("showIsolines").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.ShowPosition = attributes.namedItem("showPosition").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption = attributes.namedItem("showAxesCaption").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines = attributes.namedItem("numberOfIsolines").nodeValue().toInt();
	m_defaultSlicerSettings.SingleSlicer.MinIsoValue = attributes.namedItem("minIsovalue").nodeValue().toDouble();
	m_defaultSlicerSettings.SingleSlicer.MaxIsoValue = attributes.namedItem("maxIsovalue").nodeValue().toDouble();
	m_defaultSlicerSettings.SingleSlicer.LinearInterpolation = attributes.namedItem("linearInterpolation").nodeValue().toDouble();
	m_defaultSlicerSettings.SnakeSlices = attributes.namedItem("snakeSlices").nodeValue().toDouble();
	m_defaultSlicerSettings.LinkMDIs = attributes.namedItem("linkMDIs").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.CursorMode = attributes.namedItem("cursorMode").nodeValue().toStdString().c_str();
	m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize = attributes.namedItem("toolTipFontSize").nodeValue().toInt();

	activeMdiChild()->editSlicerSettings(m_defaultSlicerSettings);
}

QList<QString> MainWindow::mdiWindowTitles()
{
	QList<QString> windowTitles;
	for (MdiChild* mdiChild: mdiChildList())
		windowTitles.append(mdiChild->windowTitle());
	return windowTitles;
}

void MainWindow::maxXY()
{
	if (activeMdiChild())
		activeMdiChild()->maximizeXY();
}

void MainWindow::maxXZ()
{
	if (activeMdiChild())
		activeMdiChild()->maximizeXZ();
}

void MainWindow::maxYZ()
{
	if (activeMdiChild())
		activeMdiChild()->maximizeYZ();
}

void MainWindow::maxRC()
{
	if (activeMdiChild())
		activeMdiChild()->maximizeRC();
}

void MainWindow::multi()
{
	if (activeMdiChild())
		activeMdiChild()->multiview();
}

void MainWindow::linkViews()
{
	if (activeMdiChild())
	{
		m_defaultSlicerSettings.LinkViews = actionLinkViews->isChecked();
		activeMdiChild()->linkViews(m_defaultSlicerSettings.LinkViews);

		if (m_defaultSlicerSettings.LinkViews)
			statusBar()->showMessage(tr("Link Views"), 5000);
	}
}

void MainWindow::linkMDIs()
{
	if (activeMdiChild())
	{
		m_defaultSlicerSettings.LinkMDIs = actionLinkMdis->isChecked();
		activeMdiChild()->linkMDIs(m_defaultSlicerSettings.LinkMDIs);

		if (m_defaultSlicerSettings.LinkViews)
			statusBar()->showMessage(tr("Link MDIs"), 5000);
	}
}

void MainWindow::enableInteraction()
{
	if (activeMdiChild())
	{
		m_defaultSlicerSettings.InteractorsEnabled = actionEnableInteraction->isChecked();
		activeMdiChild()->enableInteraction(m_defaultSlicerSettings.InteractorsEnabled);

		if (m_defaultSlicerSettings.InteractorsEnabled)
			statusBar()->showMessage(tr("Interaction Enabled"), 5000);
		else
			statusBar()->showMessage(tr("Interaction Disabled"), 5000);
	}
}

void MainWindow::toggleConsole()
{
	iAConsole::instance()->setVisible(actionShowConsole->isChecked());
}

void MainWindow::toggleFullScreen()
{
	bool fullScreen = actionFullScreenMode->isChecked();
	if (fullScreen)
		showFullScreen();
	else
		showNormal();
	emit fullScreenToggled();
}

void MainWindow::toggleMenu()
{
	bool showMenu = actionShowMenu->isChecked();
	if (showMenu)
		menubar->show();
	else
		menubar->hide();
}

void MainWindow::prefs()
{
	MdiChild *child = activeMdiChild();

	QStringList inList = (QStringList() << tr("#Histogram Bins")
		<< tr("#Statistical extent")
		<< tr("$Use Compression when storing .mhd files")
		<< tr("$Print Parameters")
		<< tr("$Results in new window")
		<< tr("$Log to file")
		<< tr("#Log File Name")
		<< tr("+Looks")
		<< tr("#Magic lens size")
		<< tr("#Magic lens frame width"));
	QStringList looks;
	QMap<QString, QString> styleNames;
	styleNames.insert(tr("Dark")      , ":/dark.qss");
	styleNames.insert(tr("Bright")    , ":/bright.qss");
	styleNames.insert(tr("Dark New")  , ":/dark_2.qss");
	styleNames.insert(tr("Bright New"), ":/bright_2.qss");

	for (QString key: styleNames.keys())
	{
		if (m_qssName == styleNames[key])
		{
			looks.append(QString("!") + key);
		}
		else
		{
			looks.append(key);
		}
	}
	iAPreferences p = child ? child->preferences() : m_defaultPreferences;
	QTextDocument *fDescr = nullptr;
	if (iAConsole::instance()->isFileLogError())
	{
		fDescr = new QTextDocument();
		fDescr->setHtml("Could not write to the specified logfile, logging to file was therefore disabled."
			" Please check file permissions and/or whether the path to the file exists, before re-enabling the option!.");
	}
	QList<QVariant> inPara; 	inPara << tr("%1").arg(p.HistogramBins)
		<< tr("%1").arg(p.StatisticalExtent)
		<< (p.Compression ? tr("true") : tr("false"))
		<< (p.PrintParameters ? tr("true") : tr("false"))
		<< (p.ResultInNewWindow ? tr("true") : tr("false"))
		<< (iAConsole::instance()->isLogToFileOn() ? tr("true") : tr("false"))
		<< iAConsole::instance()->logFileName()
		<< looks
		<< tr("%1").arg(p.MagicLensSize)
		<< tr("%1").arg(p.MagicLensFrameWidth);

	dlg_commoninput dlg(this, "Preferences", inList, inPara, fDescr);

	if (dlg.exec() == QDialog::Accepted)
	{
		m_defaultPreferences.HistogramBins = (int)dlg.getDblValue(0);
		m_defaultPreferences.StatisticalExtent = (int)dlg.getDblValue(1);
		m_defaultPreferences.Compression = dlg.getCheckValue(2) != 0;
		m_defaultPreferences.PrintParameters = dlg.getCheckValue(3) != 0;
		m_defaultPreferences.ResultInNewWindow = dlg.getCheckValue(4) != 0;
		bool logToFile = dlg.getCheckValue(5) != 0;
		QString logFileName = dlg.getText(6);
		QString looksStr = dlg.getComboBoxValue(7);
		if (m_qssName != styleNames[looksStr])
		{
			m_qssName = styleNames[looksStr];
			applyQSS();
		}

		m_defaultPreferences.MagicLensSize = clamp(MinimumMagicLensSize, MaximumMagicLensSize,
			static_cast<int>(dlg.getDblValue(8)));
		m_defaultPreferences.MagicLensFrameWidth = std::max(0, static_cast<int>(dlg.getDblValue(9)));

		if (activeMdiChild() && activeMdiChild()->editPrefs(m_defaultPreferences))
			statusBar()->showMessage(tr("Edit preferences"), 5000);

		iAConsole::instance()->setLogToFile(logToFile, logFileName, true);
	}
	delete fDescr;
}

void MainWindow::renderSettings()
{
	MdiChild *child = activeMdiChild();

	QString t = tr("true");
	QString f = tr("false");

	iARenderSettings const & renderSettings = child->renderSettings();
	iAVolumeSettings const & volumeSettings = child->volumeSettings();

	QStringList renderTypes;
	for (int mode : RenderModeMap().keys())
		renderTypes << ((mode == volumeSettings.RenderMode) ? QString("!") : QString()) + RenderModeMap().value(mode);

	QStringList inList;
	inList
		<< tr("$Show slicers")
		<< tr("$Show slice planes")
		<< tr("$Show helpers")
		<< tr("$Show position")
		<< tr("$Parallel projection")
		<< tr("#Background top")
		<< tr("#Background bottom")
		<< tr("$Use FXAA")
		<< tr("$Linear interpolation")
		<< tr("$Shading")
		<< tr("#Sample distance")
		<< tr("#Ambient lighting")
		<< tr("#Diffuse lighting")
		<< tr("#Specular lighting")
		<< tr("#Specular power")
		<< tr("+Renderer type")
		<< tr("#Slice plane opacity");

	QList<QVariant> inPara;
	inPara << (renderSettings.ShowSlicers ? t : f)
		<< (renderSettings.ShowSlicePlanes ? t : f)
		<< (renderSettings.ShowHelpers ? t : f)
		<< (renderSettings.ShowRPosition ? t : f)
		<< (renderSettings.ParallelProjection ? t : f)
		<< tr("%1").arg(renderSettings.BackgroundTop)
		<< tr("%1").arg(renderSettings.BackgroundBottom)
		<< (renderSettings.UseFXAA ? t : f)
		<< (volumeSettings.LinearInterpolation ? t : f)
		<< (volumeSettings.Shading ? t : f)
		<< tr("%1").arg(volumeSettings.SampleDistance)
		<< tr("%1").arg(volumeSettings.AmbientLighting)
		<< tr("%1").arg(volumeSettings.DiffuseLighting)
		<< tr("%1").arg(volumeSettings.SpecularLighting)
		<< tr("%1").arg(volumeSettings.SpecularPower)
		<< renderTypes
		<< tr("%1").arg(renderSettings.PlaneOpacity);

	dlg_commoninput dlg(this, "Renderer settings", inList, inPara, NULL);

	if (dlg.exec() != QDialog::Accepted)
		return;

	m_defaultRenderSettings.ShowSlicers = dlg.getCheckValue(0) != 0;
	m_defaultRenderSettings.ShowSlicePlanes = dlg.getCheckValue(1) != 0;
	m_defaultRenderSettings.ShowHelpers = dlg.getCheckValue(2) != 0;
	m_defaultRenderSettings.ShowRPosition = dlg.getCheckValue(3) != 0;
	m_defaultRenderSettings.ParallelProjection = dlg.getCheckValue(4) != 0;
	m_defaultRenderSettings.BackgroundTop = dlg.getText(5);
	m_defaultRenderSettings.BackgroundBottom = dlg.getText(6);
	m_defaultRenderSettings.UseFXAA = dlg.getCheckValue(7) !=0;

	QColor bgTop(m_defaultRenderSettings.BackgroundTop);
	QColor bgBottom(m_defaultRenderSettings.BackgroundBottom);
	if (!bgTop.isValid())
	{
		bgTop.setRgbF(0.5, 0.666666666666666667, 1.0);
		m_defaultRenderSettings.BackgroundTop = bgTop.name();
	}
	if (!bgBottom.isValid())
	{
		bgBottom.setRgbF(1.0, 1.0, 1.0);
		m_defaultRenderSettings.BackgroundBottom = bgTop.name();
	}

	m_defaultVolumeSettings.LinearInterpolation = dlg.getCheckValue(8) != 0;
	m_defaultVolumeSettings.Shading = dlg.getCheckValue(9) != 0;
	m_defaultVolumeSettings.SampleDistance = dlg.getDblValue(10);
	m_defaultVolumeSettings.AmbientLighting = dlg.getDblValue(11);
	m_defaultVolumeSettings.DiffuseLighting = dlg.getDblValue(12);
	m_defaultVolumeSettings.SpecularLighting = dlg.getDblValue(13);
	m_defaultVolumeSettings.SpecularPower = dlg.getDblValue(14);
	m_defaultVolumeSettings.RenderMode = mapRenderModeToEnum(dlg.getComboBoxValue(15));

	m_defaultRenderSettings.PlaneOpacity = dlg.getDblValue(16);

	if (activeMdiChild() && activeMdiChild()->editRendererSettings(
		m_defaultRenderSettings,
		m_defaultVolumeSettings))
	{
		statusBar()->showMessage(tr("Changed renderer settings"), 5000);
	}
}

void MainWindow::slicerSettings()
{
	MdiChild *child = activeMdiChild();

	const QStringList mouseCursorModes = QStringList()\
		<< "Crosshair default" \
		<< "Crosshair thick red"	<< "Crosshair thin red" \
		<< "Crosshair thick orange"	<< "Crosshair thin orange" \
		<< "Crosshair thick yellow"	<< "Crosshair thin yellow" \
		<< "Crosshair thick blue"	<< "Crosshair thin blue" \
		<< "Crosshair thick cyan"	<< "Crosshair thin cyan";

	QStringList inList = (QStringList() << tr("$Link Views")
		<< tr("$Show Position")
		<< tr("$Show Isolines")
		<< tr("$Linear Interpolation")
		<< tr("#Number of Isolines")
		<< tr("#Min Isovalue")
		<< tr("#Max Isovalue")
		<< tr("#Snake Slices")
		<< tr("$Link MDIs")
		<< tr("+Mouse Coursor Types")
		<< tr("$Show Axes Caption")
		<< tr("#Tooltip Font Size (pt)")
		<< tr("$Show Tooltip")
		);

	iASlicerSettings const & slicerSettings = child->slicerSettings();
	QStringList mouseCursorTypes;
	foreach( QString mode, mouseCursorModes )
		mouseCursorTypes << ( ( mode == slicerSettings.SingleSlicer.CursorMode ) ? QString( "!" ) : QString() ) + mode;

	QList<QVariant> inPara; 	inPara  << (slicerSettings.LinkViews ? tr("true") : tr("false"))
		<< (slicerSettings.SingleSlicer.ShowPosition ? tr("true") : tr("false"))
		<< (slicerSettings.SingleSlicer.ShowIsoLines ? tr("true") : tr("false"))
		<< (slicerSettings.SingleSlicer.LinearInterpolation ? tr("true") : tr("false"))
		<< tr("%1").arg(slicerSettings.SingleSlicer.NumberOfIsoLines)
		<< tr("%1").arg(slicerSettings.SingleSlicer.MinIsoValue)
		<< tr("%1").arg(slicerSettings.SingleSlicer.MaxIsoValue)
		<< tr("%1").arg(slicerSettings.SnakeSlices)
		<< (slicerSettings.LinkMDIs ? tr("true") : tr("false"))
		<< mouseCursorTypes
		<< (slicerSettings.SingleSlicer.ShowAxesCaption ? tr("true") : tr("false"))
		<< QString("%1").arg(slicerSettings.SingleSlicer.ToolTipFontSize)
		<< (slicerSettings.SingleSlicer.ShowTooltip ? tr("true") : tr("false"));

	dlg_commoninput dlg(this, "Slicer settings", inList, inPara, NULL);

	if (dlg.exec() == QDialog::Accepted)
	{
		m_defaultSlicerSettings.LinkViews = dlg.getCheckValue(0) != 0;
		m_defaultSlicerSettings.SingleSlicer.ShowPosition = dlg.getCheckValue(1) != 0;
		m_defaultSlicerSettings.SingleSlicer.ShowIsoLines = dlg.getCheckValue(2) != 0;
		m_defaultSlicerSettings.SingleSlicer.LinearInterpolation = dlg.getCheckValue(3) != 0;
		m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines = dlg.getIntValue(4);
		m_defaultSlicerSettings.SingleSlicer.MinIsoValue = dlg.getDblValue(5);
		m_defaultSlicerSettings.SingleSlicer.MaxIsoValue = dlg.getDblValue(6);
		m_defaultSlicerSettings.SnakeSlices = dlg.getIntValue(7);
		m_defaultSlicerSettings.LinkMDIs = dlg.getCheckValue(8) != 0;
		m_defaultSlicerSettings.SingleSlicer.CursorMode = dlg.getComboBoxValue(9);
		m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption = dlg.getCheckValue(10) != 0;
		m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize = dlg.getIntValue(11);
		m_defaultSlicerSettings.SingleSlicer.ShowTooltip = dlg.getCheckValue(12) != 0;

		if (activeMdiChild() && activeMdiChild()->editSlicerSettings(m_defaultSlicerSettings))
			statusBar()->showMessage(tr("Edit slicer settings"), 5000);
	}
}

void MainWindow::loadTransferFunction()
{
	if (activeMdiChild())
	{
		if (activeMdiChild()->loadTransferFunction())
			statusBar()->showMessage(tr("Loaded transfer function successfully"), 5000);
		else
			statusBar()->showMessage(tr("Loading transfer function failed"), 5000);
	}
}

void MainWindow::saveTransferFunctionSlot()
{
	if (activeMdiChild())
	{
		if (activeMdiChild()->saveTransferFunction())
			statusBar()->showMessage(tr("Saved transfer function successfully"), 5000);
		else
			statusBar()->showMessage(tr("Saving transfer function failed"), 5000);
	}
}

void MainWindow::deletePoint()
{
	if (activeMdiChild())
	{
		int point = activeMdiChild()->deletePoint();
		statusBar()->showMessage(tr("Deleted point %1").arg(point), 5000);
	}
}

void MainWindow::changeColor()
{
	if (activeMdiChild())
		activeMdiChild()->changeColor();
}

void MainWindow::resetView()
{
	if (activeMdiChild())
		activeMdiChild()->resetView();
}

void MainWindow::resetTrf()
{
	if (activeMdiChild())
		activeMdiChild()->resetTrf();
}

void MainWindow::toggleSnakeSlicer(bool isChecked)
{
	if (activeMdiChild())
		activeMdiChild()->toggleSnakeSlicer(isChecked);
}

void MainWindow::toggleSliceProfile(bool isChecked)
{
	if (activeMdiChild())
		activeMdiChild()->toggleSliceProfile(isChecked);
}

void MainWindow::toggleMagicLens( bool isChecked )
{
	if (activeMdiChild())
		activeMdiChild()->toggleMagicLens(isChecked);
}

void MainWindow::rendererCamPosition()
{
	int pos = sender()->property("camPosition").toInt();
	if (activeChild<iAChangeableCameraWidget>())
		activeChild<iAChangeableCameraWidget>()->setCamPosition(pos);
}

void MainWindow::raycasterAssignIso()
{
	QList<MdiChild *> mdiwindows = mdiChildList();
	int sizeMdi = mdiwindows.size();
	if (sizeMdi > 1)
	{
		double camOptions[10] = {0};
		if (activeMdiChild())
			activeMdiChild()->camPosition(camOptions);
		for(int i = 0; i < sizeMdi; i++)
		{
			MdiChild *tmpChild = mdiwindows.at(i);

			// check dimension and spacing here, if not the same with active mdichild, skip.
			tmpChild->setCamPosition(camOptions, m_defaultRenderSettings.ParallelProjection);
		}
	}
}

void MainWindow::raycasterSaveCameraSettings()
{
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	saveCamera(xml);
	xml.save(fileName);
}

void MainWindow::raycasterLoadCameraSettings()
{
	// load camera settings
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath, tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	if (!xml.read(fileName) ||
		!loadCamera(xml))
	{
		return;
	}

	// apply this camera settings to all the MdiChild.
	raycasterAssignIso();
}

MdiChild* MainWindow::resultChild(MdiChild* oldChild, QString const & title)
{
	if (oldChild->resultInNewWindow())
	{
		// TODO: copy all modality images, or don't copy anything here and use image from old child directly,
		// or nothing at all until new image available!
		// Note that filters currently get their input from this child already!
		vtkSmartPointer<vtkImageData> imageData = oldChild->imagePointer();
		MdiChild* newChild = createMdiChild(true);
		newChild->show();
		newChild->displayResult(title, imageData);
		copyFunctions(oldChild, newChild);
		return newChild;
	}
	oldChild->prepareForResult();
	return oldChild;
}

MdiChild * MainWindow::resultChild(QString const & title)
{
	return resultChild(activeMdiChild(), title);
}

MdiChild * MainWindow::resultChild(int childInd, QString const & f)
{
	return resultChild(mdiChildList().at(childInd), f);
}

void MainWindow::copyFunctions(MdiChild* oldChild, MdiChild* newChild)
{
	std::vector<iAChartFunction*> const & oldChildFunctions = oldChild->functions();
	for (unsigned int i = 1; i < oldChildFunctions.size(); ++i)
	{
		iAChartFunction *curFunc = oldChildFunctions[i];
		switch (curFunc->getType())
		{
		case iAChartFunction::GAUSSIAN:
		{
			iAChartFunctionGaussian * oldGaussian = (iAChartFunctionGaussian*)curFunc;
			iAChartFunctionGaussian * newGaussian = new iAChartFunctionGaussian(newChild->histogram(), PredefinedColors()[i % 7]);
			newGaussian->setMean(oldGaussian->getMean());
			newGaussian->setMultiplier(oldGaussian->getMultiplier());
			newGaussian->setSigma(oldGaussian->getSigma());
			newChild->functions().push_back(newGaussian);
		}
		break;
		case iAChartFunction::BEZIER:
		{
			iAChartFunctionBezier * oldBezier = (iAChartFunctionBezier*)curFunc;
			iAChartFunctionBezier * newBezier = new iAChartFunctionBezier(newChild->histogram(), PredefinedColors()[i % 7]);
			for (unsigned int j = 0; j < oldBezier->getPoints().size(); ++j)
				newBezier->addPoint(oldBezier->getPoints()[j].x(), oldBezier->getPoints()[j].y());
			newChild->functions().push_back(newBezier);
		}
		break;
		default:	// unknown function type, do nothing
			break;
		}
	}
}

void MainWindow::about()
{
	m_splashScreen->show();
	m_splashScreen->showMessage(tr("\n      Version: %1").arg (m_gitVersion), Qt::AlignTop, QColor(255, 255, 255));
}

void MainWindow::wiki()
{
	QAction* act = qobject_cast<QAction*>(QObject::sender());
	if (act->text().contains("Core"))
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/wiki/Core"));
	else if(act->text().contains("Filters"))
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/wiki/Filters"));
	else if (act->text().contains("Tools"))
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/wiki/Tools"));
	else if (act->text().contains("releases"))
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/releases"));
	else if (act->text().contains("bug"))
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/issues"));
}

void MainWindow::createRecentFileActions()
{
	m_separatorAct = menuFile->addSeparator();
	for (int i = 0; i < MaxRecentFiles; ++i)
	{
		m_recentFileActs[i] = new QAction(this);
		m_recentFileActs[i]->setVisible(false);
		menuFile->addAction(m_recentFileActs[i]);
	}
	updateRecentFileActions();
}

void MainWindow::updateMenus()
{
	bool hasMdiChild = activeMdiChild();

	actionSave->setEnabled(hasMdiChild);
	actionSaveAs->setEnabled(hasMdiChild);
	actionSaveImageStack->setEnabled(hasMdiChild);
	actionSaveProject->setEnabled(activeChild<iASavableProject>());
	actionLoadSettings->setEnabled(hasMdiChild);
	actionSaveSettings->setEnabled(hasMdiChild);
	actionClose->setEnabled(hasMdiChild);
	actionCloseAll->setEnabled(hasMdiChild);

	actionTile->setEnabled(hasMdiChild);
	actionCascade->setEnabled(hasMdiChild);
	actionNextWindow->setEnabled(hasMdiChild);
	actionPrevWindow->setEnabled(hasMdiChild);

	actionXY->setEnabled(hasMdiChild);
	actionXZ->setEnabled(hasMdiChild);
	actionYZ->setEnabled(hasMdiChild);
	action3D->setEnabled(hasMdiChild);
	actionMultiViews->setEnabled(hasMdiChild);
	actionLinkViews->setEnabled(hasMdiChild);
	actionLinkMdis->setEnabled(hasMdiChild);
	actionEnableInteraction->setEnabled(hasMdiChild);
	actionRendererSettings->setEnabled(hasMdiChild);
	actionSlicerSettings->setEnabled(hasMdiChild);
	actionLoadTransferFunction->setEnabled(hasMdiChild);
	actionSaveTransferFunction->setEnabled(hasMdiChild);
	actionSnakeSlicer->setEnabled(hasMdiChild);
	actionMagicLens->setEnabled(hasMdiChild);

	bool hasChangeableRenderer = activeChild<iAChangeableCameraWidget>();
	actionViewXDirectionInRaycaster->setEnabled(hasChangeableRenderer);
	actionViewmXDirectionInRaycaster->setEnabled(hasChangeableRenderer);
	actionViewYDirectionInRaycaster->setEnabled(hasChangeableRenderer);
	actionViewmYDirectionInRaycaster->setEnabled(hasChangeableRenderer);
	actionViewZDirectionInRaycaster->setEnabled(hasChangeableRenderer);
	actionViewmZDirectionInRaycaster->setEnabled(hasChangeableRenderer);
	actionIsometricViewInRaycaster->setEnabled(hasChangeableRenderer);
	actionAssignView->setEnabled(hasMdiChild);
	actionLoadCameraSettings->setEnabled(hasMdiChild);
	actionSaveCameraSettings->setEnabled(hasMdiChild);
	actionResetView->setEnabled(hasMdiChild);
	actionResetFunction->setEnabled(hasMdiChild);
	actionRawProfile->setEnabled(hasMdiChild);
	actionLoadLayout->setEnabled(hasMdiChild);
	actionSaveLayout->setEnabled(hasMdiChild);
	actionResetLayout->setEnabled(hasMdiChild);
	actionDeleteLayout->setEnabled(hasMdiChild);
	actionChildStatusBar->setEnabled(hasMdiChild);

	updateRecentFileActions();

	if (activeMdiChild())
	{
		int selectedFuncPoint = activeMdiChild()->selectedFuncPoint();
		if (selectedFuncPoint == -1)
		{
			actionDeletePoint->setEnabled(false);
			actionChangeColor->setEnabled(false);
		}
		else if (activeMdiChild()->isFuncEndPoint(selectedFuncPoint))
		{
			actionDeletePoint->setEnabled(false);
			actionChangeColor->setEnabled(true);
		}
		else
		{
			actionDeletePoint->setEnabled(true);
			actionChangeColor->setEnabled(true);
		}
		// set current application working directory to the one where the file is in (as default directory, e.g. for file open)
		// see also MdiChild::setCurrentFile
		if (!activeMdiChild()->filePath().isEmpty())
			QDir::setCurrent(activeMdiChild()->filePath());
		//??if (activeMdiChild())
		//	histogramToolbar->setEnabled(activeMdiChild()->getTabIndex() == 1 && !activeMdiChild()->isMaximized());
	}
	else
	{
		actionDeletePoint->setEnabled(false);
		actionChangeColor->setEnabled(false);
	}
}

void MainWindow::updateWindowMenu()
{
	QList<MdiChild *> windows = mdiChildList();

	for (int i = 0; i < windows.size(); ++i) {
		MdiChild *child = windows.at(i);

		QString text;
		if (i < 9) {
			text = tr("&%1 %2").arg(i + 1)
				.arg(child->userFriendlyCurrentFile());
		} else {
			text = tr("%1 %2").arg(i + 1)
				.arg(child->userFriendlyCurrentFile());
		}
		QAction *action  = menuWindow->addAction(text);
		action->setCheckable(true);
		action->setChecked(child == activeMdiChild());
		connect(action, SIGNAL(triggered()), m_windowMapper, SLOT(map()));
		m_windowMapper->setMapping(action, windows.at(i));
	}
}

MdiChild* MainWindow::createMdiChild(bool unsavedChanges)
{
	MdiChild *child = new MdiChild(this, m_defaultPreferences, unsavedChanges);
	QMdiSubWindow* subWin = mdiArea->addSubWindow(child);
	subWin->setOption(QMdiSubWindow::RubberBandResize);
	subWin->setOption(QMdiSubWindow::RubberBandMove);

	if (mdiArea->subWindowList().size() < 2)
		child->showMaximized();

	child->setRenderSettings(m_defaultRenderSettings, m_defaultVolumeSettings);
	child->setupSlicers(m_defaultSlicerSettings, false);

	connect( child, SIGNAL( pointSelected() ), this, SLOT( pointSelected() ) );
	connect( child, SIGNAL( noPointSelected() ), this, SLOT( noPointSelected() ) );
	connect( child, SIGNAL( endPointSelected() ), this, SLOT( endPointSelected() ) );
	connect( child, SIGNAL( active() ), this, SLOT( setHistogramFocus() ) );
	connect( child, SIGNAL( closed() ), this, SLOT( childClosed() ) );

	setModuleActionsEnabled( true );

	m_moduleDispatcher->ChildCreated(child);
	return child;
}

void MainWindow::closeMdiChild(MdiChild* child)
{
	if (!child)
		return;
	QMdiSubWindow* subWin = qobject_cast<QMdiSubWindow*>(child->parent());
	delete subWin;
}

void MainWindow::connectSignalsToSlots()
{
	// "File menu entries:
	connect(actionOpen, &QAction::triggered, this, &MainWindow::open);
	connect(actionOpenRaw, &QAction::triggered, this, &MainWindow::openRaw);
	connect(actionOpenImageStack, &QAction::triggered, this, &MainWindow::openImageStack);
	connect(actionOpenVolumeStack, &QAction::triggered, this, &MainWindow::openVolumeStack);
	connect(actionOpenWithDataTypeConversion, &QAction::triggered, this, &MainWindow::openWithDataTypeConversion);
	connect(actionOpenTLGICTData, &QAction::triggered, this, &MainWindow::openTLGICTData);
	connect(actionSave, &QAction::triggered, this, &MainWindow::save);
	connect(actionSaveAs, &QAction::triggered, this, &MainWindow::saveAs);
	connect(actionSaveProject, &QAction::triggered, this, &MainWindow::saveProject);
	connect(actionLoadSettings, &QAction::triggered, this, &MainWindow::loadSettings);
	connect(actionSaveSettings, &QAction::triggered, this, &MainWindow::saveSettings);
	connect(actionExit, &QAction::triggered, qApp, &QApplication::closeAllWindows);
	for (int i = 0; i < MaxRecentFiles; ++i)
		connect(m_recentFileActs[i], &QAction::triggered, this, &MainWindow::openRecentFile);

	// "Edit" menu entries:
	connect(actionPreferences, &QAction::triggered, this, &MainWindow::prefs);
	connect(actionRendererSettings, &QAction::triggered, this, &MainWindow::renderSettings);
	connect(actionSlicerSettings, &QAction::triggered, this, &MainWindow::slicerSettings);
	connect(actionLoadTransferFunction, &QAction::triggered, this, &MainWindow::loadTransferFunction);
	connect(actionSaveTransferFunction, &QAction::triggered, this, &MainWindow::saveTransferFunctionSlot);
	connect(actionChangeColor, &QAction::triggered, this, &MainWindow::changeColor);
	connect(actionDeletePoint, &QAction::triggered, this, &MainWindow::deletePoint);
	connect(actionResetView, &QAction::triggered, this, &MainWindow::resetView);
	connect(actionResetFunction, &QAction::triggered, this, &MainWindow::resetTrf);

	// "Views" menu entries:
	connect(actionXY, &QAction::triggered, this, &MainWindow::maxXY);
	connect(actionXZ, &QAction::triggered, this, &MainWindow::maxXZ);
	connect(actionYZ, &QAction::triggered, this, &MainWindow::maxYZ);
	connect(action3D, &QAction::triggered, this, &MainWindow::maxRC);
	connect(actionMultiViews, &QAction::triggered, this, &MainWindow::multi);
	connect(actionLinkViews, &QAction::triggered, this, &MainWindow::linkViews);
	connect(actionLinkMdis, &QAction::triggered, this, &MainWindow::linkMDIs);
	connect(actionEnableInteraction, &QAction::triggered, this, &MainWindow::enableInteraction);
	connect(actionShowConsole, &QAction::triggered, this, &MainWindow::toggleConsole);
	connect(actionFullScreenMode, &QAction::triggered, this, &MainWindow::toggleFullScreen);
	connect(actionShowMenu, &QAction::triggered, this, &MainWindow::toggleMenu);
	connect(actionShowToolbar, &QAction::triggered, this, &MainWindow::toggleToolbar);
	connect(actionMainWindowStatusBar, &QAction::triggered, this, &MainWindow::toggleMainWindowStatusBar);
	// Enable these actions also when menu not visible:
	addAction(actionFullScreenMode);
	addAction(actionShowMenu);
	addAction(actionShowToolbar);
	addAction(actionMainWindowStatusBar);

	// "Window" menu entries:
	connect(actionClose, &QAction::triggered, mdiArea, &QMdiArea::closeActiveSubWindow);
	connect(actionCloseAll, &QAction::triggered, this, &MainWindow::closeAllSubWindows);
	connect(actionTile, &QAction::triggered, mdiArea, &QMdiArea::tileSubWindows);
	connect(actionCascade, &QAction::triggered, mdiArea, &QMdiArea::cascadeSubWindows);
	connect(actionNextWindow, &QAction::triggered, mdiArea, &QMdiArea::activateNextSubWindow);
	connect(actionPrevWindow, &QAction::triggered, mdiArea, &QMdiArea::activatePreviousSubWindow);
	connect(actionChildStatusBar, &QAction::triggered, this, &MainWindow::toggleChildStatusBar);

	// "Help" menu entries:
	connect(actionUserGuideCore, &QAction::triggered, this, &MainWindow::wiki);
	connect(actionUserGuideFilters, &QAction::triggered, this, &MainWindow::wiki);
	connect(actionUserGuideTools, &QAction::triggered, this, &MainWindow::wiki);
	connect(actionReleases, &QAction::triggered, this, &MainWindow::wiki);
	connect(actionBug, &QAction::triggered, this, &MainWindow::wiki);
	connect(actionAbout, &QAction::triggered, this, &MainWindow::about);

	// Renderer toolbar:
	connect(actionViewXDirectionInRaycaster,  &QAction::triggered, this, &MainWindow::rendererCamPosition);
	actionViewXDirectionInRaycaster->setProperty("camPosition", iACameraPosition::PX);
	connect(actionViewmXDirectionInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	actionViewmXDirectionInRaycaster->setProperty("camPosition", iACameraPosition::MX);
	connect(actionViewYDirectionInRaycaster,  &QAction::triggered, this, &MainWindow::rendererCamPosition);
	actionViewYDirectionInRaycaster->setProperty("camPosition", iACameraPosition::PY);
	connect(actionViewmYDirectionInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	actionViewmYDirectionInRaycaster->setProperty("camPosition", iACameraPosition::MY);
	connect(actionViewZDirectionInRaycaster,  &QAction::triggered, this, &MainWindow::rendererCamPosition);
	actionViewZDirectionInRaycaster->setProperty("camPosition", iACameraPosition::PZ);
	connect(actionViewmZDirectionInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	actionViewmZDirectionInRaycaster->setProperty("camPosition", iACameraPosition::MZ);
	connect(actionIsometricViewInRaycaster,   &QAction::triggered, this, &MainWindow::rendererCamPosition);
	actionIsometricViewInRaycaster->setProperty("camPosition", iACameraPosition::Iso);

	// Camera toolbar:
	connect(actionAssignView, &QAction::triggered, this, &MainWindow::raycasterAssignIso);
	connect(actionSaveCameraSettings, &QAction::triggered, this, &MainWindow::raycasterSaveCameraSettings);
	connect(actionLoadCameraSettings, &QAction::triggered, this, &MainWindow::raycasterLoadCameraSettings);

	// Snake slicer toolbar
	connect(actionSnakeSlicer, &QAction::toggled, this, &MainWindow::toggleSnakeSlicer);
	connect(actionRawProfile, &QAction::toggled, this, &MainWindow::toggleSliceProfile);
	connect(actionMagicLens, &QAction::toggled, this, &MainWindow::toggleMagicLens);

	// Layout toolbar menu entries
	connect(actionSaveLayout, &QAction::triggered, this, &MainWindow::saveLayout);
	connect(actionLoadLayout, &QAction::triggered, this, &MainWindow::loadLayout);
	connect(actionDeleteLayout, &QAction::triggered, this, &MainWindow::deleteLayout);
	connect(actionResetLayout, &QAction::triggered, this, &MainWindow::resetLayout);

	connect(mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::childActivatedSlot);
	connect(mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::updateMenus);
	connect(m_windowMapper, SIGNAL(mapped(QWidget*)), this, SLOT(setActiveSubWindow(QWidget*)));

	consoleVisibilityChanged(iAConsole::instance()->isVisible());
	connect(iAConsole::instance(), &iAConsole::consoleVisibilityChanged, this, &MainWindow::consoleVisibilityChanged);
}

void MainWindow::readSettings()
{
	QSettings settings;
	m_path = settings.value("Path").toString();

	m_qssName = settings.value("qssName", ":/bright.qss").toString();

	m_defaultLayout = settings.value("Preferences/defaultLayout", "").toString();
	m_defaultPreferences.HistogramBins = settings.value("Preferences/prefHistogramBins", DefaultHistogramBins).toInt();
	m_defaultPreferences.StatisticalExtent = settings.value("Preferences/prefStatExt", 3).toInt();
	m_defaultPreferences.Compression = settings.value("Preferences/prefCompression", true).toBool();
	m_defaultPreferences.ResultInNewWindow = settings.value("Preferences/prefResultInNewWindow", true).toBool();
	m_defaultPreferences.MagicLensSize = settings.value("Preferences/prefMagicLensSize", DefaultMagicLensSize).toInt();
	m_defaultPreferences.MagicLensFrameWidth = settings.value("Preferences/prefMagicLensFrameWidth", 3).toInt();
	bool prefLogToFile = settings.value("Preferences/prefLogToFile", false).toBool();
	QString logFileName = settings.value("Preferences/prefLogFile", "debug.log").toString();
	iAConsole::instance()->setLogToFile(prefLogToFile, logFileName);

	iARenderSettings fallbackRS;
	m_defaultRenderSettings.ShowSlicers = settings.value("Renderer/rsShowSlicers", fallbackRS.ShowSlicers).toBool();
	m_defaultRenderSettings.ShowSlicePlanes = settings.value("Renderer/rsShowSlicePlanes", fallbackRS.ShowSlicePlanes).toBool();
	m_defaultRenderSettings.ShowHelpers = settings.value("Renderer/rsShowHelpers", fallbackRS.ShowHelpers).toBool();
	m_defaultRenderSettings.ShowRPosition = settings.value("Renderer/rsShowRPosition", fallbackRS.ShowRPosition).toBool();
	m_defaultRenderSettings.ParallelProjection = settings.value("Renderer/rsParallelProjection", fallbackRS.ParallelProjection).toBool();
	m_defaultRenderSettings.BackgroundTop = settings.value("Renderer/rsBackgroundTop", fallbackRS.BackgroundTop).toString();
	m_defaultRenderSettings.BackgroundBottom = settings.value("Renderer/rsBackgroundBottom", fallbackRS.BackgroundBottom).toString();

	iAVolumeSettings fallbackVS;
	m_defaultVolumeSettings.LinearInterpolation = settings.value("Renderer/rsLinearInterpolation", fallbackVS.LinearInterpolation).toBool();
	m_defaultVolumeSettings.Shading = settings.value("Renderer/rsShading", fallbackVS.Shading).toBool();
	m_defaultVolumeSettings.SampleDistance = settings.value("Renderer/rsSampleDistance", fallbackVS.SampleDistance).toDouble();
	m_defaultVolumeSettings.AmbientLighting = settings.value("Renderer/rsAmbientLighting", fallbackVS.AmbientLighting).toDouble();
	m_defaultVolumeSettings.DiffuseLighting = settings.value("Renderer/rsDiffuseLighting", fallbackVS.DiffuseLighting).toDouble();
	m_defaultVolumeSettings.SpecularLighting = settings.value("Renderer/rsSpecularLighting", fallbackVS.SpecularLighting).toDouble();
	m_defaultVolumeSettings.SpecularPower = settings.value("Renderer/rsSpecularPower", fallbackVS.SpecularPower).toDouble();
	m_defaultVolumeSettings.RenderMode = settings.value("Renderer/rsRenderMode", fallbackVS.RenderMode).toInt();

	iASlicerSettings fallbackSS;
	m_defaultSlicerSettings.LinkViews = settings.value("Slicer/ssLinkViews", fallbackSS.LinkViews).toBool();
	m_defaultSlicerSettings.LinkMDIs = settings.value("Slicer/ssLinkMDIs", fallbackSS.LinkMDIs).toBool();
	m_defaultSlicerSettings.SnakeSlices = settings.value("Slicer/ssSnakeSlices", fallbackSS.SnakeSlices).toInt();
	m_defaultSlicerSettings.SingleSlicer.ShowPosition = settings.value("Slicer/ssShowPosition", fallbackSS.SingleSlicer.ShowPosition).toBool();
	m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption = settings.value("Slicer/ssShowAxesCaption", fallbackSS.SingleSlicer.ShowAxesCaption).toBool();
	m_defaultSlicerSettings.SingleSlicer.ShowIsoLines = settings.value("Slicer/ssShowIsolines", fallbackSS.SingleSlicer.ShowIsoLines).toBool();
	m_defaultSlicerSettings.SingleSlicer.ShowTooltip = settings.value("Slicer/ssShowTooltip", fallbackSS.SingleSlicer.ShowTooltip).toBool();
	m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines = settings.value("Slicer/ssNumberOfIsolines", fallbackSS.SingleSlicer.NumberOfIsoLines).toDouble();
	m_defaultSlicerSettings.SingleSlicer.MinIsoValue = settings.value("Slicer/ssMinIsovalue", fallbackSS.SingleSlicer.MinIsoValue).toDouble();
	m_defaultSlicerSettings.SingleSlicer.MaxIsoValue = settings.value("Slicer/ssMaxIsovalue", fallbackSS.SingleSlicer.MaxIsoValue).toDouble();
	m_defaultSlicerSettings.SingleSlicer.LinearInterpolation = settings.value("Slicer/ssImageActorUseInterpolation", fallbackSS.SingleSlicer.LinearInterpolation).toBool();
	m_defaultSlicerSettings.SingleSlicer.CursorMode = settings.value( "Slicer/ssCursorMode", fallbackSS.SingleSlicer.CursorMode).toString();
	m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize = settings.value("Slicer/toolTipFontSize", fallbackSS.SingleSlicer.ToolTipFontSize).toInt();

	m_lpCamera = settings.value("Parameters/lpCamera").toBool();
	m_lpSliceViews = settings.value("Parameters/lpSliceViews").toBool();
	m_lpTransferFunction = settings.value("Parameters/lpTransferFunction").toBool();
	m_lpProbabilityFunctions = settings.value("Parameters/lpProbabilityFunctions").toBool();
	m_lpPreferences = settings.value("Parameters/lpPreferences").toBool();
	m_lpRenderSettings = settings.value("Parameters/lpRenderSettings").toBool();
	m_lpSlicerSettings = settings.value("Parameters/lpSlicerSettings").toBool();

	m_spCamera = settings.value("Parameters/spCamera").toBool();
	m_spSliceViews = settings.value("Parameters/spSliceViews").toBool();
	m_spTransferFunction = settings.value("Parameters/spTransferFunction").toBool();
	m_spProbabilityFunctions = settings.value("Parameters/spProbabilityFunctions").toBool();
	m_spPreferences = settings.value("Parameters/spPreferences").toBool();
	m_spRenderSettings = settings.value("Parameters/spRenderSettings").toBool();
	m_spSlicerSettings = settings.value("Parameters/spSlicerSettings").toBool();

	m_owdtcs = settings.value("OpenWithDataTypeConversion/owdtcs", 1).toInt();
	m_rawFileParams.m_size[0] = settings.value("OpenWithDataTypeConversion/owdtcx", 1).toInt();
	m_rawFileParams.m_size[1] = settings.value("OpenWithDataTypeConversion/owdtcy", 1).toInt();
	m_rawFileParams.m_size[2] = settings.value("OpenWithDataTypeConversion/owdtcz", 1).toInt();
	m_rawFileParams.m_spacing[0] = settings.value("OpenWithDataTypeConversion/owdtcsx", 1.0).toDouble();
	m_rawFileParams.m_spacing[1] = settings.value("OpenWithDataTypeConversion/owdtcsy", 1.0).toDouble();
	m_rawFileParams.m_spacing[2] = settings.value("OpenWithDataTypeConversion/owdtcsz", 1.0).toDouble();
	m_owdtcmin = settings.value("OpenWithDataTypeConversion/owdtcmin").toDouble();
	m_owdtcmax = settings.value("OpenWithDataTypeConversion/owdtcmax").toDouble();
	m_owdtcoutmin = settings.value("OpenWithDataTypeConversion/owdtcoutmin").toDouble();
	m_owdtcoutmax = settings.value("OpenWithDataTypeConversion/owdtcoutmax").toDouble();
	m_owdtcdov = settings.value("OpenWithDataTypeConversion/owdtcdov").toInt();
	m_owdtcxori = settings.value("OpenWithDataTypeConversion/owdtcxori").toInt();
	m_owdtcyori = settings.value("OpenWithDataTypeConversion/owdtcyori").toInt();
	m_owdtczori = settings.value("OpenWithDataTypeConversion/owdtczori").toInt();
	m_owdtcxsize = settings.value("OpenWithDataTypeConversion/owdtcxsize").toInt();
	m_owdtcysize = settings.value("OpenWithDataTypeConversion/owdtcysize").toInt();
	m_owdtczsize = settings.value("OpenWithDataTypeConversion/owdtczsize").toInt();

	settings.beginGroup("Layout");
	m_layoutNames = settings.allKeys();
	m_layoutNames = m_layoutNames.filter(QRegularExpression("^state"));
	m_layoutNames.replaceInStrings(QRegularExpression("^state"), "");
	settings.endGroup();
	if (m_layoutNames.size() == 0)
	{
		m_layoutNames.push_back("1");
		m_layoutNames.push_back("2");
		m_layoutNames.push_back("3");
	}
}

void MainWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("Path", m_path);
	settings.setValue("qssName", m_qssName);

	settings.setValue("Preferences/defaultLayout", m_layout->currentText());
	settings.setValue("Preferences/prefHistogramBins", m_defaultPreferences.HistogramBins);
	settings.setValue("Preferences/prefStatExt", m_defaultPreferences.StatisticalExtent);
	settings.setValue("Preferences/prefCompression", m_defaultPreferences.Compression);
	settings.setValue("Preferences/prefResultInNewWindow", m_defaultPreferences.ResultInNewWindow);
	settings.setValue("Preferences/prefMagicLensSize", m_defaultPreferences.MagicLensSize);
	settings.setValue("Preferences/prefMagicLensFrameWidth", m_defaultPreferences.MagicLensFrameWidth);
	settings.setValue("Preferences/prefLogToFile", iAConsole::instance()->isLogToFileOn());
	settings.setValue("Preferences/prefLogFile", iAConsole::instance()->logFileName());

	settings.setValue("Renderer/rsShowSlicers", m_defaultRenderSettings.ShowSlicers);
	settings.setValue("Renderer/rsShowSlicePlanes", m_defaultRenderSettings.ShowSlicePlanes);
	settings.setValue("Renderer/rsParallelProjection", m_defaultRenderSettings.ParallelProjection);
	settings.setValue("Renderer/rsBackgroundTop", m_defaultRenderSettings.BackgroundTop);
	settings.setValue("Renderer/rsBackgroundBottom", m_defaultRenderSettings.BackgroundBottom);
	settings.setValue("Renderer/rsShowHelpers", m_defaultRenderSettings.ShowHelpers);
	settings.setValue("Renderer/rsShowRPosition", m_defaultRenderSettings.ShowRPosition);

	settings.setValue("Renderer/rsLinearInterpolation", m_defaultVolumeSettings.LinearInterpolation);
	settings.setValue("Renderer/rsShading", m_defaultVolumeSettings.Shading);
	settings.setValue("Renderer/rsSampleDistance", m_defaultVolumeSettings.SampleDistance);
	settings.setValue("Renderer/rsAmbientLighting", m_defaultVolumeSettings.AmbientLighting);
	settings.setValue("Renderer/rsDiffuseLighting", m_defaultVolumeSettings.DiffuseLighting);
	settings.setValue("Renderer/rsSpecularLighting", m_defaultVolumeSettings.SpecularLighting);
	settings.setValue("Renderer/rsSpecularPower", m_defaultVolumeSettings.SpecularPower);
	settings.setValue("Renderer/rsRenderMode", m_defaultVolumeSettings.RenderMode);

	settings.setValue("Slicer/ssLinkViews", m_defaultSlicerSettings.LinkViews);
	settings.setValue("Slicer/ssShowPosition", m_defaultSlicerSettings.SingleSlicer.ShowPosition);
	settings.setValue("Slicer/ssShowAxesCaption", m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption);
	settings.setValue("Slicer/ssShowIsolines", m_defaultSlicerSettings.SingleSlicer.ShowIsoLines);
	settings.setValue("Slicer/ssShowTooltip", m_defaultSlicerSettings.SingleSlicer.ShowTooltip);
	settings.setValue("Slicer/ssLinkMDIs", m_defaultSlicerSettings.LinkMDIs);
	settings.setValue("Slicer/ssNumberOfIsolines", m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines);
	settings.setValue("Slicer/ssMinIsovalue", m_defaultSlicerSettings.SingleSlicer.MinIsoValue);
	settings.setValue("Slicer/ssMaxIsovalue", m_defaultSlicerSettings.SingleSlicer.MaxIsoValue);
	settings.setValue("Slicer/ssImageActorUseInterpolation", m_defaultSlicerSettings.SingleSlicer.LinearInterpolation);
	settings.setValue("Slicer/ssSnakeSlices", m_defaultSlicerSettings.SnakeSlices);
	settings.setValue("Slicer/ssCursorMode", m_defaultSlicerSettings.SingleSlicer.CursorMode);
	settings.setValue("Slicer/toolTipFontSize", m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize);

	settings.setValue("Parameters/lpCamera", m_lpCamera);
	settings.setValue("Parameters/lpSliceViews", m_lpSliceViews);
	settings.setValue("Parameters/lpTransferFunction", m_lpTransferFunction);
	settings.setValue("Parameters/lpProbabilityFunctions", m_lpProbabilityFunctions);
	settings.setValue("Parameters/lpPreferences", m_lpPreferences);
	settings.setValue("Parameters/lpRenderSettings", m_lpRenderSettings);
	settings.setValue("Parameters/lpSlicerSettings", m_lpSlicerSettings);

	settings.setValue("Parameters/spCamera", m_spCamera);
	settings.setValue("Parameters/spSliceViews", m_spSliceViews);
	settings.setValue("Parameters/spTransferFunction", m_spTransferFunction);
	settings.setValue("Parameters/spProbabilityFunctions", m_spProbabilityFunctions);
	settings.setValue("Parameters/spPreferences", m_spPreferences);
	settings.setValue("Parameters/spRenderSettings", m_spRenderSettings);
	settings.setValue("Parameters/spSlicerSettings", m_spSlicerSettings);

	settings.setValue("OpenWithDataTypeConversion/owdtcs", m_owdtcs);
	settings.setValue("OpenWithDataTypeConversion/owdtcx", m_rawFileParams.m_size[0]);
	settings.setValue("OpenWithDataTypeConversion/owdtcy", m_rawFileParams.m_size[1]);
	settings.setValue("OpenWithDataTypeConversion/owdtcz", m_rawFileParams.m_size[2]);
	settings.setValue("OpenWithDataTypeConversion/owdtcsx", m_rawFileParams.m_spacing[0]);
	settings.setValue("OpenWithDataTypeConversion/owdtcsy", m_rawFileParams.m_spacing[1]);
	settings.setValue("OpenWithDataTypeConversion/owdtcsz", m_rawFileParams.m_spacing[2]);
	settings.setValue("OpenWithDataTypeConversion/owdtcmin", m_owdtcmin);
	settings.setValue("OpenWithDataTypeConversion/owdtcmax", m_owdtcmax);
	settings.setValue("OpenWithDataTypeConversion/owdtcoutmin", m_owdtcoutmin);
	settings.setValue("OpenWithDataTypeConversion/owdtcoutmax", m_owdtcoutmax);
	settings.setValue("OpenWithDataTypeConversion/owdtcdov", m_owdtcdov);
	settings.setValue("OpenWithDataTypeConversion/owdtcxori", m_owdtcxori);
	settings.setValue("OpenWithDataTypeConversion/owdtcyori", m_owdtcyori);
	settings.setValue("OpenWithDataTypeConversion/owdtczori", m_owdtczori);
	settings.setValue("OpenWithDataTypeConversion/owdtcxsize", m_owdtcxsize);
	settings.setValue("OpenWithDataTypeConversion/owdtcysize", m_owdtcysize);
	settings.setValue("OpenWithDataTypeConversion/owdtczsize", m_owdtczsize);
}

void MainWindow::setCurrentFile(const QString &fileName)
{
	if (fileName.isEmpty())
	{
		DEBUG_LOG("Can't use empty filename as current!");
		return;
	}
	m_curFile = fileName;
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);

	while (files.size() > MaxRecentFiles)
		files.removeLast();

	settings.setValue("recentFileList", files);

	updateRecentFileActions();
}

QString const & MainWindow::currentFile()
{
	return m_curFile;
}

void MainWindow::setPath(QString const & p)
{
	m_path = p;
}

QString const & MainWindow::path()
{
	return m_path;
}

void MainWindow::updateRecentFileActions()
{
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	QMutableStringListIterator it(files);
	while (it.hasNext())
	{
		QString fileName = it.next();
		QFileInfo fi(fileName);
		if (!fi.exists())
			it.remove();
	}
	settings.setValue("recentFileList", files);

	int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = tr("&%1 %2").arg(i + 1).arg(fileNameOnly(files[i]));
		m_recentFileActs[i]->setText(text);
		m_recentFileActs[i]->setData(files[i]);
		m_recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
		m_recentFileActs[j]->setVisible(false);

	m_separatorAct->setVisible(numRecentFiles > 0);
}

MdiChild* MainWindow::activeMdiChild()
{
	return activeChild<MdiChild>();
}

MdiChild * MainWindow::secondNonActiveChild()
{
	QList<MdiChild *> mdiwindows = mdiChildList();
	if (mdiwindows.size() > 2)
	{
		QMessageBox::warning(this, tr("Warning"),
			tr("Only two datasets can be processed at a time! Please close %1 datasets")
			.arg(mdiwindows.size() - 2));
		return nullptr;
	}
	else if (mdiwindows.size() < 2)
	{
		QMessageBox::warning(this, tr("Warning"),
			tr("Only one dataset available. Please load another one!"));
		return nullptr;
	}
	return activeMdiChild() == mdiwindows.at(0) ?
		mdiwindows.at(1) : mdiwindows.at(0);
}

MdiChild* MainWindow::findMdiChild(const QString &fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

	for (MdiChild* mdiChild: mdiChildList())
		if (mdiChild->currentFile() == canonicalFilePath)
			return mdiChild;
	return nullptr;
}

void MainWindow::setActiveSubWindow(QWidget *window)
{
	if (!window)
		return;
	mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}

void MainWindow::pointSelected()
{
	actionChangeColor->setEnabled(true);
	actionDeletePoint->setEnabled(true);
}

void MainWindow::noPointSelected()
{
	actionChangeColor->setEnabled(false);
	actionDeletePoint->setEnabled(false);
}

void MainWindow::endPointSelected()
{
	actionChangeColor->setEnabled(true);
	actionDeletePoint->setEnabled(false);
}

void MainWindow::setHistogramFocus()
{
	if (activeMdiChild())
		activeMdiChild()->setHistogramFocus();
}

void MainWindow::consoleVisibilityChanged(bool newVisibility)
{
	QSignalBlocker block(actionShowConsole);
	actionShowConsole->setChecked(newVisibility);
}

QList<MdiChild*> MainWindow::mdiChildList(QMdiArea::WindowOrder order)
{
	return childList<MdiChild>(order);
}

void MainWindow::childActivatedSlot(QMdiSubWindow *wnd)
{
	MdiChild * activeChild = activeMdiChild();
	if (activeChild && wnd)
	{
		QSignalBlocker blockSliceProfile(actionRawProfile);
		actionRawProfile->setChecked(activeChild->isSliceProfileToggled());
		QSignalBlocker blockSnakeSlicer(actionSnakeSlicer);
		actionSnakeSlicer->setChecked(activeChild->isSnakeSlicerToggled());
		QSignalBlocker blockMagicLens(actionMagicLens);
		actionMagicLens->setChecked(activeChild->isMagicLensToggled());
	}
}

void MainWindow::applyQSS()
{
	// Load an application style
	QFile styleFile(m_qssName);
	if (styleFile.open( QFile::ReadOnly ))
	{
		QTextStream styleIn(&styleFile);
		QString style = styleIn.readAll();
		styleFile.close();
		qApp->setStyleSheet(style);
		emit styleChanged();
	}
}

void MainWindow::saveLayout()
{
	MdiChild *child = activeMdiChild();
	if(child)
	{
		QByteArray state = child->saveState(0);
		QSettings settings;
		QString layoutName(m_layout->currentText());
		QStringList inList = (QStringList() << tr("#Layout Name:") );
		QList<QVariant> inPara;
		inPara << tr("%1").arg(layoutName);
		dlg_commoninput dlg(this, "Layout Name", inList, inPara, NULL);
		if (dlg.exec() == QDialog::Accepted)
		{
			layoutName =  dlg.getText(0);
			if (layoutName == "")
			{
				QMessageBox::warning(this, "Save Layout", "Layout Name cannot be empty!");
				return;
			}
			if (m_layout->findText(layoutName) == -1)
			{
				m_layout->addItem(layoutName);
			}
			else
			{
				if (QMessageBox::question(
					this,
					"Save Layout",
					"Do you want to overwrite the existing layout with this name?"
					) != QMessageBox::Yes)
				{
					return;
				}
			}
			settings.setValue( "Layout/state" + layoutName, state );
			m_layout->setCurrentIndex(m_layout->findText(layoutName));
		}
	}
}

void MainWindow::loadLayout()
{
	MdiChild *child = activeMdiChild();
	assert(child);
	if (!child)
	{
		return;
	}
	child->loadLayout(m_layout->currentText());
}

void MainWindow::deleteLayout()
{
	if (QMessageBox::question(this, "Delete Layout",
		QString("Do you want to delete the layout '")+ m_layout->currentText()+"'?")
		== QMessageBox::Yes)
	{
		QSettings settings;
		settings.remove("Layout/state" + m_layout->currentText());
		m_layout->removeItem(m_layout->currentIndex());
	}
}

void MainWindow::resetLayout()
{
	activeMdiChild()->resetLayout();
}

QMenu * MainWindow::fileMenu()
{
	return this->menuFile;
}

QMenu * MainWindow::filtersMenu()
{
	return this->menuFilters;
}

QMenu * MainWindow::toolsMenu()
{
	return this->menuTools;
}

QMenu * MainWindow::helpMenu()
{
	return this->menuHelp;
}

void MainWindow::toggleMainWindowStatusBar()
{
	statusBar()->setVisible(actionMainWindowStatusBar->isChecked());
}

void MainWindow::toggleToolbar()
{
	bool visible = actionShowToolbar->isChecked();
	QList<QToolBar *> toolbars = findChildren<QToolBar *>();
	for (auto toolbar : toolbars)
	{
		toolbar->setVisible(visible);
	}
}

void MainWindow::toggleChildStatusBar()
{
	if (!activeMdiChild())
	{
		return;
	}
	activeMdiChild()->statusBar()->setVisible(actionChildStatusBar->isChecked());
}

QMdiSubWindow* MainWindow::addSubWindow( QWidget * child )
{
	return mdiArea->addSubWindow( child );
}

void MainWindow::setModuleActionsEnabled( bool isEnabled )
{
	m_moduleDispatcher->SetModuleActionsEnabled(isEnabled);
}

void MainWindow::childClosed()
{
	MdiChild * sender = dynamic_cast<MdiChild*> (QObject::sender());
	if (!sender)
		return;
	// magic lens size can be modified in the slicers as well; make sure to store this change:
	m_defaultPreferences.MagicLensSize = sender->magicLensSize();
	if( mdiArea->subWindowList().size() == 1 )
	{
		MdiChild * child = dynamic_cast<MdiChild*> ( mdiArea->subWindowList().at( 0 )->widget() );
		if(!child)
			return;
		if( child == sender )
			setModuleActionsEnabled( false );
	}
}

void MainWindow::saveProject()
{
	iASavableProject * child = activeChild<iASavableProject>();
	if (!child)
		return;
	child->saveProject();
}

void MainWindow::loadArguments(int argc, char** argv)
{
	QStringList files;
	for (int a = 1; a < argc; ++a)
	{
		if (QString(argv[a]).startsWith("--quit"))
		{
			++a;
			bool ok;
			quint64 ms = QString(argv[a]).toULongLong(&ok);
			if (ok)
			{
				m_quitTimer = new QTimer();
				m_quitTimer->setSingleShot(true);
				connect(m_quitTimer, SIGNAL(timeout()), this, SLOT(quitTimerSlot()));
				m_quitTimer->start(ms);
			}
			else
			{
				DEBUG_LOG("Invalid --quit parameter; must be followed by an integer number (milliseconds) after which to quit, e.g. '--quit 1000'");
			}
		}
		else
			files << QString::fromLocal8Bit(argv[a]);
	}
	loadFiles(files);
}

iAPreferences const & MainWindow::getDefaultPreferences() const
{
	return m_defaultPreferences;
}

iAModuleDispatcher & MainWindow::getModuleDispatcher() const
{
	return *this->m_moduleDispatcher.data();
}


// Move to other places (modules?):

void MainWindow::openWithDataTypeConversion()
{
	QString file = QFileDialog::getOpenFileName(this,
		tr("Open File"),
		m_path,
		iAIOProvider::GetSupportedLoadFormats()
	);
	if (file.isEmpty())
		return;

	QStringList additionalLabels = (QStringList() << tr("#Slice sample rate"));
	QList<QVariant> additionalValues = (QList<QVariant>() << tr("%1").arg(m_owdtcs));

	dlg_openfile_sizecheck dlg(false, file, this, "Open With DataType Conversion", additionalLabels, additionalValues, m_rawFileParams);
	if (!dlg.accepted())
	{
		return;
	}
	m_owdtcs = clamp(1u, m_rawFileParams.m_size[2], static_cast<unsigned int>(dlg.inputDlg()->getIntValue(dlg.fixedParams())));

	QSize qwinsize = this->size();
	double winsize[2];
	winsize[0] = qwinsize.width();	winsize[1] = qwinsize.height();

	double convPara[11];
	convPara[0] = m_owdtcmin;   convPara[1] = m_owdtcmax;  convPara[2] = m_owdtcoutmin; convPara[3] = m_owdtcoutmax; convPara[4] =  m_owdtcdov; convPara[5] = m_owdtcxori;
	convPara[6] = m_owdtcxsize; convPara[7] = m_owdtcyori; convPara[8] = m_owdtcysize;  convPara[9] = m_owdtczori;   convPara[10] = m_owdtczsize;
	try
	{
		dlg_datatypeconversion conversionwidget(this, file, m_rawFileParams,
			m_owdtcs, m_defaultPreferences.HistogramBins, winsize, convPara);
		if (conversionwidget.exec() != QDialog::Accepted)
			return;

		QString outDataType = conversionwidget.getDataType();
		m_owdtcmin = conversionwidget.getRangeLower();   m_owdtcmax = conversionwidget.getRangeUpper();
		m_owdtcoutmin = conversionwidget.getOutputMin(); m_owdtcoutmax = conversionwidget.getOutputMax();
		m_owdtcdov = conversionwidget.getConvertROI();
		m_owdtcxori = conversionwidget.getXOrigin(); m_owdtcxsize = conversionwidget.getXSize();
		m_owdtcyori = conversionwidget.getYOrigin(); m_owdtcysize = conversionwidget.getYSize();
		m_owdtczori = conversionwidget.getZOrigin(); m_owdtczsize = conversionwidget.getZSize();

		double roi[6];
		roi[0] = m_owdtcxori; roi[1] = m_owdtcxsize;
		roi[2] = m_owdtcyori; roi[3] = m_owdtcysize;
		roi[4] = m_owdtczori; roi[5] = m_owdtczsize;

		QString finalfilename;
		if (m_owdtcdov == 0)
		{
			finalfilename = conversionwidget.convert(file, m_rawFileParams,
				mapVTKTypeStringToInt(outDataType),
				m_owdtcmin, m_owdtcmax, m_owdtcoutmin, m_owdtcoutmax, m_owdtcdov);
		}
		else
		{
			finalfilename = conversionwidget.convertROI(file, m_rawFileParams,
				mapVTKTypeStringToInt(outDataType),
				m_owdtcmin, m_owdtcmax, m_owdtcoutmin, m_owdtcoutmax, m_owdtcdov, roi);
		}
		loadFile(finalfilename, false);
	}
	catch (std::exception & e)
	{
		DEBUG_LOG(QString("Open with datatype conversion: %1").arg(e.what()));
	}
}

void MainWindow::openTLGICTData()
{
	QString baseDirectory = QFileDialog::getExistingDirectory(
		this,
		tr("Open Talbot-Lau Grating Interferometer CT Dataset"),
		path(),
		QFileDialog::ShowDirsOnly);
	loadTLGICTData(baseDirectory);
}

void MainWindow::loadTLGICTData(QString const & baseDirectory)
{
	iATLGICTLoader* tlgictLoader = new iATLGICTLoader();
	if (!tlgictLoader->setup(baseDirectory, this))
		return;
	tlgictLoader->start(createMdiChild(false));
	// tlgictLoader will delete itself when finished!
}

#include "iAConsole.h"
#include "iASCIFIOCheck.h"

#include <QApplication>
#include <QDate>
#include <QProxyStyle>

class MyProxyStyle : public QProxyStyle
{
public:
	using QProxyStyle::QProxyStyle;

	int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override
	{
		// disable tooltip delay for iAChartWidget and descendants:
		if (hint == QStyle::SH_ToolTip_WakeUpDelay && widget && widget->inherits(iAChartWidget::staticMetaObject.className()))
		{
			return 0;
		}
		return QProxyStyle::styleHint(hint, option, widget, returnData);
	}
};

void MainWindow::initResources()
{
	Q_INIT_RESOURCE(open_iA);
}

int MainWindow::runGUI(int argc, char * argv[], QString const & appName, QString const & version,
	QString const & splashPath, QString const & iconPath)
{
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
	MainWindow::initResources();
	QApplication app(argc, argv);
	QString msg;
	if (!checkOpenGLVersion(msg))
	{
		bool runningScripted = false;
		for (int a = 1; a < argc; ++a)
		{
			if (QString(argv[a]).startsWith("--quit"))
			{
				runningScripted = true;
				break;
			}
		}
		if (runningScripted)
		{
			DEBUG_LOG(msg);
		}
		else
		{
			QMessageBox::warning(nullptr, appName, msg);
		}
		return 1;
	}
	app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
	app.setAttribute(Qt::AA_ShareOpenGLContexts);
	iAGlobalLogger::setLogger(iAConsole::instance());
	MainWindow mainWin(appName, version, splashPath);
	CheckSCIFIO(QCoreApplication::applicationDirPath());
	mainWin.loadArguments(argc, argv);
	// TODO: unify with logo in slicer/renderer!
	app.setWindowIcon(QIcon(QPixmap(iconPath)));
	qApp->setStyle(new MyProxyStyle(qApp->style()));
	mainWin.setWindowIcon(QIcon(QPixmap(iconPath)));
	if (QDate::currentDate().dayOfYear() >= 350)
	{
		mainWin.setWindowTitle("Merry Christmas and a Happy New Year!");
		mainWin.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
		app.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
	}
	mainWin.show();
	return app.exec();
}
