/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "pch.h"
#include "mainwindow.h"

#include "defines.h"

#include "dlg_bezier.h"
#include "dlg_commoninput.h"
#include "dlg_datatypeconversion.h"
#include "dlg_gaussian.h"
#include "dlg_transfer.h"
#include "iAConsole.h"
#include "charts/iADiagramFctWidget.h"
#include "io/iAIOProvider.h"
#include "iALogger.h"
#include "iAMathUtility.h"
#include "iAModuleDispatcher.h"
#include "iARenderer.h"
#include "iASlicerData.h"
#include "iAToolsVTK.h"
#include "io/iATLGICTLoader.h"
#include "mdichild.h"

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVersion.h>

#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QSettings>
#include <QSignalMapper>
#include <QSplashScreen>
#include <QTextStream>
#include <QTimer>
#include <QtXml/QDomDocument>

MainWindow::MainWindow(QString const & appName, QString const & version, QString const & splashImage )
:
	QMainWindow(),
	m_moduleDispatcher( new iAModuleDispatcher( this ) ),
	m_gitVersion(version)
{
	setupUi(this);

	// restore geometry and state
	QCoreApplication::setOrganizationName("FHW");
	QCoreApplication::setOrganizationDomain("3dct.at");
	QCoreApplication::setApplicationName(appName);
	setWindowTitle(appName + " " + m_gitVersion);
	QSettings settings;
	path = settings.value("Path").toString();
	restoreGeometry(settings.value("geometry", saveGeometry()).toByteArray());
	restoreState(settings.value("state", saveState()).toByteArray());

	QPixmap pixmap( splashImage );
	splashScreen = new QSplashScreen(pixmap);
	splashScreen->setWindowFlags(splashScreen->windowFlags() | Qt::WindowStaysOnTopHint);
	splashScreen->show();

	splashScreen->showMessage("\n      Reading settings...", Qt::AlignTop, QColor(255, 255, 255));
	readSettings();

	timer = new QTimer();
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
	timer->start(2000);

	splashScreen->showMessage("\n      Setup UI...", Qt::AlignTop, QColor(255, 255, 255));
	applyQSS();
	actionLink_views->setChecked(defaultSlicerSettings.LinkViews);//removed from readSettings, if is needed at all?
	actionLink_mdis->setChecked(defaultSlicerSettings.LinkMDIs);
	setCentralWidget(mdiArea);

	windowMapper = new QSignalMapper(this);

	createRecentFileActions();
	connectSignalsToSlots();
	updateMenus();
	slicerToolsGroup = new QActionGroup(this);
	slicerToolsGroup->setExclusive(false);
	slicerToolsGroup->addAction(actionSnake_Slicer);
	slicerToolsGroup->addAction(actionRawProfile);

	actionDelete_point->setEnabled(false);
	actionChange_color->setEnabled(false);

	splashScreen->showMessage(tr("\n      Version: %1").arg (m_gitVersion), Qt::AlignTop, QColor(255, 255, 255));

	layout = new QComboBox(this);
	for (int i=0; i<layoutNames.size(); ++i)
	{
		layout->addItem(layoutNames[i]);
		if (layoutNames[i] == defaultLayout)
		{
			layout->setCurrentIndex(i);
		}
	}
	this->layout->setStyleSheet("padding: 0");
	this->layout->resize(this->layout->geometry().width(), 100);
	this->layout->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	this->layoutToolbar->insertWidget(this->actionSave_Layout, layout);

	m_moduleDispatcher->InitializeModules(iAConsoleLogger::Get());
	SetModuleActionsEnabled( false );
	statusBar()->showMessage(tr("Ready"));
}


MainWindow::~MainWindow()
{
	// save geometry and state
	QSettings settings;
	settings.setValue("geometry", saveGeometry());
	settings.setValue("state", saveState());

	m_moduleDispatcher->SaveModulesSettings();
	delete windowMapper;
	windowMapper = 0;
}


void MainWindow::timeout()
{
	splashScreen->finish(this);
	delete timer;
}


bool MainWindow::KeepOpen()
{
	bool childHasChanges = false;
	for (MdiChild* mdiChild: MdiChildList())
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
			for (MdiChild* mdiChild: MdiChildList())
				mdiChild->setWindowModified(false);
		}
	}
	return false;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
	if (KeepOpen())
	{
		event->ignore();
		return;
	}
	mdiArea->closeAllSubWindows();
	if (activeMdiChild()) {
		event->ignore();
	} else {
		writeSettings();
		iAConsole::Close();
		event->accept();
	}
}


void MainWindow::CloseAllSubWindows()
{
	if (!KeepOpen())
	{
		mdiArea->closeAllSubWindows();
	}
}


void MainWindow::Open()
{
	LoadFiles(
		QFileDialog::getOpenFileNames(
			this,
			tr("Open Files"),
			path,
			iAIOProvider::GetSupportedLoadFormats()
		)
	);
}

void MainWindow::OpenCSV()
{   /* code to load CSV-data*/
	/*IAOCSV FReader(); */
	//typedef iAQTtoUIConnector<QDialog, Ui_CsvInput>   dlg_csvInput;
	/*csvConfig::configPararams fileConfParams; 

	dlg_CSVInput dlg;
	if (dlg.exec() != QDialog::Accepted) {
	
		return; 
	}

	dlg.getConfigParameters(fileConfParams); 


	iACsvIO io;
	if (!io.loadCSVCustom(fileConfParams)) {
		return; 
	}*/
	

}

void MainWindow::OpenRaw()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		tr("Open Raw File"),
		path,
		"Raw File (*)"
	);
	MdiChild *child = createMdiChild(false);
	QString t; t = fileName; t.truncate(t.lastIndexOf('/'));
	path = t;
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


void MainWindow::OpenImageStack()
{
	LoadFile(
		QFileDialog::getOpenFileName(
			this,
			tr("Open File"),
			path,
			iAIOProvider::GetSupportedImageStackFormats()
		), true
	);
}


void MainWindow::OpenVolumeStack()
{
	LoadFile(
		QFileDialog::getOpenFileName(
			this,
			tr("Open File"),
			path,
			iAIOProvider::GetSupportedVolumeStackFormats()
		), true
	);
}


void MainWindow::OpenRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	QString fileName = action->data().toString();
	LoadFile(fileName);
}


void MainWindow::LoadFile(QString const & fileName)
{
	if (fileName.endsWith(iAIOProvider::ProjectFileExtension))
	{
		LoadProject(fileName);
	}
	else
	{
		QFileInfo fi(fileName);
		if (fi.isDir())
		{
			LoadTLGICTData(fileName);
		}
		else
		{
			LoadFile(fileName, fileName.endsWith(".volstack"));
		}
	}
}


void MainWindow::LoadFile(QString fileName, bool isStack)
{
	if (fileName.isEmpty())
		return;
	statusBar()->showMessage(tr("Loading data..."), 5000);
	QString t; t = fileName; t.truncate(t.lastIndexOf('/'));
	path = t;
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


void MainWindow::LoadFiles(QStringList fileNames)
{
	for (int i = 0; i < fileNames.length(); i++)
	{
		LoadFile(fileNames[i]);
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


QDomDocument MainWindow::loadSettingsFile(QString filename)
{
	QDomDocument doc;

	QFile file(filename);
	if (file.open(QIODevice::ReadOnly))
	{
		if (!doc.setContent(&file)) {
			QMessageBox msgBox;
			msgBox.setText("An error occurred during xml parsing!");
			msgBox.exec();

			return doc;
		}

		if (!doc.hasChildNodes() || doc.documentElement().tagName() != "settings")
		{
			QDomElement root = doc.createElement("settings");
			doc.appendChild(root);
		}

	}
	else
	{
		QDomElement root = doc.createElement("settings");
		doc.appendChild(root);
	}

	file.deleteLater();
	file.close();

	return doc;
}


void MainWindow::saveSettingsFile(QDomDocument &doc, QString filename)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly);
	QTextStream ts(&file);
	ts << doc.toString();

	file.close();
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
		inPara << tr("%1").arg(spCamera ? tr("true") : tr("false"))
			<< tr("%1").arg(spSliceViews ? tr("true") : tr("false"))
			<< tr("%1").arg(spTransferFunction ? tr("true") : tr("false"))
			<< tr("%1").arg(spProbabilityFunctions ? tr("true") : tr("false"))
			<< tr("%1").arg(spPreferences ? tr("true") : tr("false"))
			<< tr("%1").arg(spRenderSettings ? tr("true") : tr("false"))
			<< tr("%1").arg(spSlicerSettings ? tr("true") : tr("false"));

		dlg_commoninput dlg(this, "Save Settings", inList, inPara, NULL);

		if (dlg.exec() == QDialog::Accepted)
		{
			dlg.getCheckValue(0) == 0 ? spCamera = false               : spCamera = true;
			dlg.getCheckValue(1) == 0 ? spSliceViews = false           : spSliceViews = true;
			dlg.getCheckValue(2) == 0 ? spTransferFunction = false     : spTransferFunction = true;
			dlg.getCheckValue(3) == 0 ? spProbabilityFunctions = false : spProbabilityFunctions = true;
			dlg.getCheckValue(4) == 0 ? spPreferences = false          : spPreferences = true;
			dlg.getCheckValue(5) == 0 ? spRenderSettings = false       : spRenderSettings = true;
			dlg.getCheckValue(6) == 0 ? spSlicerSettings = false        : spSlicerSettings = true;

			QDomDocument doc = loadSettingsFile(fileName);

			if (spCamera) saveCamera(doc);
			if (spSliceViews) saveSliceViews(doc);
			if (spTransferFunction) saveTransferFunction(doc, (dlg_transfer*)activeMdiChild()->getFunctions()[0]);
			if (spProbabilityFunctions) saveProbabilityFunctions(doc);
			if (spPreferences) savePreferences(doc);
			if (spRenderSettings) saveRenderSettings(doc);
			if (spSlicerSettings) saveSlicerSettings(doc);

			saveSettingsFile(doc, fileName);
		}
	}

	return true;
}


bool MainWindow::loadSettings()
{
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		QFile file(fileName);
		file.open(QIODevice::ReadWrite);

		QDomDocument doc;
		if (!doc.setContent(&file)) {
			QMessageBox msgBox;
			msgBox.setText("An error occurred during xml parsing!");
			msgBox.exec();

			file.close();
			return false;
		}

		file.close();

		bool camera = false, sliceViews = false, transferFunction = false, probabilityFunctions = false;
		bool preferences = false, renderSettings = false, slicerSettings = false;

		QDomElement root = doc.documentElement();
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
		if (camera)               { inList << tr("$Camera");                inPara << tr("%1").arg(lpCamera ? tr("true") : tr("false")); }
		if (sliceViews)           { inList << tr("$Slice Views");           inPara << tr("%1").arg(lpSliceViews ? tr("true") : tr("false")); }
		if (transferFunction)     { inList << tr("$Transfer Function");     inPara << tr("%1").arg(lpTransferFunction ? tr("true") : tr("false")); }
		if (probabilityFunctions) { inList << tr("$Probability Functions"); inPara << tr("%1").arg(lpProbabilityFunctions ? tr("true") : tr("false")); }
		if (preferences)          { inList << tr("$Preferences");           inPara << tr("%1").arg(lpPreferences ? tr("true") : tr("false")); }
		if (renderSettings)       { inList << tr("$Render Settings");       inPara << tr("%1").arg(lpRenderSettings ? tr("true") : tr("false")); }
		if (slicerSettings)       { inList << tr("$Slice Settings");        inPara << tr("%1").arg(lpSlicerSettings ? tr("true") : tr("false")); }

		dlg_commoninput dlg(this, "Load Settings", inList, inPara, NULL);

		if (dlg.exec() == QDialog::Accepted)
		{
			int index = 0;
			if (camera)               { dlg.getCheckValue(index++) == 0 ? lpCamera = false               : lpCamera = true; }
			if (sliceViews)           { dlg.getCheckValue(index++) == 0 ? lpSliceViews = false           : lpSliceViews = true; }
			if (transferFunction)     { dlg.getCheckValue(index++) == 0 ? lpTransferFunction = false     : lpTransferFunction = true; }
			if (probabilityFunctions) { dlg.getCheckValue(index++) == 0 ? lpProbabilityFunctions = false : lpProbabilityFunctions = true; }
			if (preferences)          { dlg.getCheckValue(index++) == 0 ? lpPreferences = false          : lpPreferences = true; }
			if (renderSettings)       { dlg.getCheckValue(index++) == 0 ? lpRenderSettings = false       : lpRenderSettings = true; }
			if (slicerSettings)       { dlg.getCheckValue(index++) == 0 ? lpSlicerSettings = false       : lpSlicerSettings = true; }

			if (lpProbabilityFunctions)
			{
				std::vector<dlg_function*> &functions = activeMdiChild()->getFunctions();
				for (unsigned int i = 1; i < functions.size(); i++)
				{
					delete functions.back();
					functions.pop_back();
				}
			}

			QDomElement root = doc.documentElement();

			QDomNodeList list = root.childNodes();
			for (int n = 0; n < int(list.length()); n++)
			{
				QDomNode node = list.item(n);
				if (node.nodeName() == "camera" && lpCamera) loadCamera(node);
				if (node.nodeName() == "sliceViews" && lpSliceViews) loadSliceViews(node);
				if (node.nodeName() == "functions" && lpTransferFunction)
				{
					activeMdiChild()->getHistogram()->loadTransferFunction(node);
					activeMdiChild()->redrawHistogram();
				}
				if (node.nodeName() == "functions" && lpProbabilityFunctions) loadProbabilityFunctions(node);
				if (node.nodeName() == "preferences" && lpPreferences) loadPreferences(node);
				if (node.nodeName() == "renderSettings" && lpRenderSettings) loadRenderSettings(node);
				if (node.nodeName() == "slicerSettings" && lpSlicerSettings) loadSlicerSettings(node);
			}
		}
	}

	return true;
}


void MainWindow::saveCamera(QDomDocument &doc)
{
	vtkCamera *camera = activeMdiChild()->getRenderer()->GetRenderer()->GetActiveCamera();
	QDomNode node = doc.documentElement();
	removeNode(node, "camera");
	QDomElement cameraElement = doc.createElement("camera");
	saveCamera(cameraElement, camera);
	doc.documentElement().appendChild(cameraElement);
}


void MainWindow::loadCamera(QDomNode &cameraNode)
{
	vtkCamera *camera = activeMdiChild()->getRenderer()->GetRenderer()->GetActiveCamera();
	loadCamera(cameraNode, camera);

	double allBounds[6];
	activeMdiChild()->getRenderer()->GetRenderer()->ComputeVisiblePropBounds( allBounds );
	activeMdiChild()->getRenderer()->GetRenderer()->ResetCameraClippingRange( allBounds );
}


void MainWindow::saveSliceViews(QDomDocument &doc)
{
	QDomNode sliceViewsNode;

	// find slice view node
	bool found = false;
	QDomElement root = doc.documentElement();
	QDomNodeList list = root.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		if (node.nodeName() == "sliceViews")
		{
			sliceViewsNode = node;
			found = true;
			break;
		}
	}

	if (!found)
	{
		sliceViewsNode = doc.createElement("sliceViews");
		root.appendChild(sliceViewsNode);
	}

	saveSliceView(doc, sliceViewsNode, activeMdiChild()->getSlicerDataXY()->GetRenderer(), "XY");
	saveSliceView(doc, sliceViewsNode, activeMdiChild()->getSlicerDataYZ()->GetRenderer(), "YZ");
	saveSliceView(doc, sliceViewsNode, activeMdiChild()->getSlicerDataXZ()->GetRenderer(), "XZ");
}


void MainWindow::saveSliceView(QDomDocument &doc, QDomNode &sliceViewsNode, vtkRenderer *ren, char const *elemStr)
{
	// get parameters of slice views
	vtkCamera *camera = ren->GetActiveCamera();
	// remove views node if there is one
	removeNode(sliceViewsNode, elemStr);
	// add new slice view node
	QDomElement cameraElement = doc.createElement(elemStr);

	saveCamera(cameraElement, camera);

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

void MainWindow::loadSliceViews(QDomNode &sliceViewsNode)
{

	QDomNodeList list = sliceViewsNode.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		vtkCamera *camera;
		if (node.nodeName() == "XY") camera = activeMdiChild()->getSlicerDataXY()->GetCamera();
		else if (node.nodeName() == "YZ") camera = activeMdiChild()->getSlicerDataYZ()->GetCamera();
		else camera = activeMdiChild()->getSlicerDataXZ()->GetCamera();
		loadCamera(node, camera);
	}
}


void MainWindow::saveTransferFunction(QDomDocument &doc, dlg_transfer* transferFunction)
{
	// does functions node exist
	QDomNode functionsNode = doc.documentElement().namedItem("functions");
	if (!functionsNode.isElement())
	{
		functionsNode = doc.createElement("functions");
		doc.documentElement().appendChild(functionsNode);
	}

	// remove function node if there is one
	removeNode(functionsNode, "transfer");

	// add new function node
	QDomElement transferElement = doc.createElement("transfer");

	for (int i = 0; i < transferFunction->GetOpacityFunction()->GetSize(); i++)
	{
		double opacityTFValue[4];
		double colorTFValue[6];
		transferFunction->GetOpacityFunction()->GetNodeValue(i, opacityTFValue);
		transferFunction->GetColorFunction()->GetNodeValue(i, colorTFValue);

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


void MainWindow::saveProbabilityFunctions(QDomDocument &doc)
{
	// does functions node exist
	QDomNode functionsNode = doc.documentElement().namedItem("functions");
	if (!functionsNode.isElement())
	{
		functionsNode = doc.createElement("functions");
		doc.documentElement().appendChild(functionsNode);
	}

	// remove existing function nodes except the transfer function
	int n = 0;
	while (n < functionsNode.childNodes().length())
	{
		QDomNode node = functionsNode.childNodes().item(n);
		if (node.nodeName() == "bezier" || node.nodeName() == "gaussian")
			functionsNode.removeChild(node);
		else
			n++;
	}

	// add new function nodes
	std::vector<dlg_function*> functions = activeMdiChild()->getFunctions();

	for (unsigned int f = 1; f < functions.size(); f++)
	{
		switch(functions[f]->getType())
		{
		case dlg_function::BEZIER:
			{
				QDomElement bezierElement = doc.createElement("bezier");

				std::vector<QPointF> points = ((dlg_bezier*)functions[f])->getPoints();

				std::vector<QPointF>::iterator it = points.begin();
				while(it != points.end())
				{
					QPointF point = *it;

					QDomElement nodeElement = doc.createElement("node");
					nodeElement.setAttribute("value", tr("%1").arg(point.x()));
					nodeElement.setAttribute("fktValue", tr("%1").arg(point.y()));
					bezierElement.appendChild(nodeElement);

					functionsNode.appendChild(bezierElement);

					++it;
				}
			}
			break;
		case dlg_function::GAUSSIAN:
			{
				QDomElement gaussianElement = doc.createElement("gaussian");

				dlg_gaussian *gaussian = (dlg_gaussian*)functions[f];

				gaussianElement.setAttribute("mean", tr("%1").arg(gaussian->getMean()));
				gaussianElement.setAttribute("sigma", tr("%1").arg(gaussian->getSigma()));
				gaussianElement.setAttribute("multiplier", tr("%1").arg(gaussian->getMultiplier()));

				functionsNode.appendChild(gaussianElement);
			}
			break;
		default:
			// unknown function type, do nothing
			break;
		}
	}
}


void MainWindow::loadProbabilityFunctions(QDomNode &functionsNode)
{
	double value, fktValue, sigma, mean, multiplier;

	int colorIndex = 1;
	QDomNodeList list = functionsNode.childNodes();
	for (int n = 0; n < list.size(); n++)
	{
		QDomNode functionNode = list.item(n);
		if (functionNode.nodeName() == "bezier")
		{
			dlg_bezier *bezier = new dlg_bezier(activeMdiChild()->getHistogram(), PredefinedColors()[colorIndex % 7], false);
			QDomNodeList innerList = functionNode.childNodes();
			for (int in = 0; in < innerList.length(); in++)
			{
				QDomNode node = innerList.item(in);
				QDomNamedNodeMap attributes = node.attributes();

				value = attributes.namedItem("value").nodeValue().toDouble();
				fktValue = attributes.namedItem("fktValue").nodeValue().toDouble();

				bezier->push_back(value, fktValue);
			}
			activeMdiChild()->getFunctions().push_back(bezier);
			colorIndex++;
		}
		else if (functionNode.nodeName() == "gaussian")
		{
			dlg_gaussian *gaussian = new dlg_gaussian(activeMdiChild()->getHistogram(), PredefinedColors()[colorIndex % 7], false);

			mean = functionNode.attributes().namedItem("mean").nodeValue().toDouble();
			sigma = functionNode.attributes().namedItem("sigma").nodeValue().toDouble();
			multiplier = functionNode.attributes().namedItem("multiplier").nodeValue().toDouble();

			gaussian->setMean(mean);
			gaussian->setSigma(sigma);
			gaussian->setMultiplier(multiplier);

			activeMdiChild()->getFunctions().push_back(gaussian);
			colorIndex++;
		}
	}
}


void MainWindow::savePreferences(QDomDocument &doc)
{
	// remove preferences node if there is one
	QDomNode node = doc.documentElement();
	removeNode(node, "preferences");
	// add new camera node
	QDomElement preferencesElement = doc.createElement("preferences");
	preferencesElement.setAttribute("histogramBins", tr("%1").arg(defaultPreferences.HistogramBins));
	preferencesElement.setAttribute("statisticalExtent", tr("%1").arg(defaultPreferences.StatisticalExtent));
	preferencesElement.setAttribute("compression", tr("%1").arg(defaultPreferences.Compression));
	preferencesElement.setAttribute("resultsInNewWindow", tr("%1").arg(defaultPreferences.ResultInNewWindow));
	preferencesElement.setAttribute("magicLensSize", tr("%1").arg(defaultPreferences.MagicLensSize));
	preferencesElement.setAttribute("magicLensFrameWidth", tr("%1").arg(defaultPreferences.MagicLensFrameWidth));
	preferencesElement.setAttribute("logToFile", tr("%1").arg(iAConsole::GetInstance()->IsLogToFileOn()));

	doc.documentElement().appendChild(preferencesElement);
}


void MainWindow::loadPreferences(QDomNode &preferencesNode)
{
	QDomNamedNodeMap attributes = preferencesNode.attributes();
	defaultPreferences.HistogramBins = attributes.namedItem("histogramBins").nodeValue().toInt();
	defaultPreferences.StatisticalExtent = attributes.namedItem("statisticalExtent").nodeValue().toDouble();
	defaultPreferences.Compression = attributes.namedItem("compression").nodeValue() == "1";
	defaultPreferences.ResultInNewWindow = attributes.namedItem("resultsInNewWindow").nodeValue() == "1";
	defaultPreferences.MagicLensSize = attributes.namedItem("magicLensSize").nodeValue().toInt();
	defaultPreferences.MagicLensFrameWidth = attributes.namedItem("magicLensFrameWidth").nodeValue().toInt();
	bool prefLogToFile = attributes.namedItem("logToFile").nodeValue() == "1";
	QString logFileName = attributes.namedItem("logFile").nodeValue();

	iAConsole::GetInstance()->SetLogToFile(prefLogToFile, logFileName);

	activeMdiChild()->editPrefs(defaultPreferences);
}


void MainWindow::saveRenderSettings(QDomDocument &doc)
{
	// remove renderSettings node if there is one
	QDomNode node = doc.documentElement();
	removeNode(node, "renderSettings");

	// add new camera node
	QDomElement renderSettingsElement = doc.createElement("renderSettings");
	renderSettingsElement.setAttribute("showSlicers", tr("%1").arg(defaultRenderSettings.ShowSlicers));
	renderSettingsElement.setAttribute("showHelpers", tr("%1").arg(defaultRenderSettings.ShowHelpers));
	renderSettingsElement.setAttribute("showRPosition", tr("%1").arg(defaultRenderSettings.ShowRPosition));
	renderSettingsElement.setAttribute("linearInterpolation", tr("%1").arg(defaultVolumeSettings.LinearInterpolation));
	renderSettingsElement.setAttribute("shading", tr("%1").arg(defaultVolumeSettings.Shading));
	renderSettingsElement.setAttribute("parallelProjection", tr("%1").arg(defaultRenderSettings.ParallelProjection));
	renderSettingsElement.setAttribute("sampleDistance", tr("%1").arg(defaultVolumeSettings.SampleDistance));
	renderSettingsElement.setAttribute("ambientLighting", tr("%1").arg(defaultVolumeSettings.AmbientLighting));
	renderSettingsElement.setAttribute("diffuseLighting", tr("%1").arg(defaultVolumeSettings.DiffuseLighting));
	renderSettingsElement.setAttribute("specularLighting", tr("%1").arg(defaultVolumeSettings.SpecularLighting));
	renderSettingsElement.setAttribute("specularPower", tr("%1").arg(defaultVolumeSettings.SpecularPower));
	renderSettingsElement.setAttribute("backgroundTop", tr("%1").arg(defaultRenderSettings.BackgroundTop.toLatin1().constData()));
	renderSettingsElement.setAttribute("backgroundBottom", tr("%1").arg(defaultRenderSettings.BackgroundBottom.toLatin1().constData()));
	renderSettingsElement.setAttribute("renderMode", tr("%1").arg(defaultVolumeSettings.Mode));

	doc.documentElement().appendChild(renderSettingsElement);
}


void MainWindow::loadRenderSettings(QDomNode &renderSettingsNode)
{
	QDomNamedNodeMap attributes = renderSettingsNode.attributes();

	defaultRenderSettings.ShowSlicers = attributes.namedItem("showSlicers").nodeValue() == "1";
	defaultRenderSettings.ShowHelpers = attributes.namedItem("showHelpers").nodeValue() == "1";
	defaultRenderSettings.ShowRPosition = attributes.namedItem("showRPosition").nodeValue() == "1";
	defaultVolumeSettings.LinearInterpolation = attributes.namedItem("linearInterpolation").nodeValue() == "1";
	defaultVolumeSettings.Shading = attributes.namedItem("shading").nodeValue() == "1";
	defaultRenderSettings.ParallelProjection = attributes.namedItem("parallelProjection").nodeValue() == "1";

	defaultVolumeSettings.SampleDistance = attributes.namedItem("sampleDistance").nodeValue().toDouble();
	defaultVolumeSettings.AmbientLighting = attributes.namedItem("ambientLighting").nodeValue().toDouble();
	defaultVolumeSettings.DiffuseLighting = attributes.namedItem("diffuseLighting").nodeValue().toDouble();
	defaultVolumeSettings.SpecularLighting = attributes.namedItem("specularLighting").nodeValue().toDouble();
	defaultVolumeSettings.SpecularPower = attributes.namedItem("specularPower").nodeValue().toDouble();
	defaultRenderSettings.BackgroundTop = attributes.namedItem("backgroundTop").nodeValue();
	defaultRenderSettings.BackgroundBottom = attributes.namedItem("backgroundBottom").nodeValue();
	defaultVolumeSettings.Mode = attributes.namedItem("renderMode").nodeValue().toInt();

	activeMdiChild()->editRendererSettings(defaultRenderSettings, defaultVolumeSettings);
}


void MainWindow::saveSlicerSettings(QDomDocument &doc)
{
	// remove slicerSettings node if there is one
	QDomNode node = doc.documentElement();
	removeNode(node, "slicerSettings");

	// add new camera node
	QDomElement slicerSettingsElement = doc.createElement("slicerSettings");
	slicerSettingsElement.setAttribute("linkViews", tr("%1").arg(defaultSlicerSettings.LinkViews));
	slicerSettingsElement.setAttribute("showIsolines", tr("%1").arg(defaultSlicerSettings.SingleSlicer.ShowIsoLines));
	slicerSettingsElement.setAttribute("showPosition", tr("%1").arg(defaultSlicerSettings.SingleSlicer.ShowPosition));
	slicerSettingsElement.setAttribute("showAxesCaption", tr("%1").arg(defaultSlicerSettings.SingleSlicer.ShowAxesCaption));
	slicerSettingsElement.setAttribute("numberOfIsolines", tr("%1").arg(defaultSlicerSettings.SingleSlicer.NumberOfIsoLines));
	slicerSettingsElement.setAttribute("minIsovalue", tr("%1").arg(defaultSlicerSettings.SingleSlicer.MinIsoValue));
	slicerSettingsElement.setAttribute("maxIsovalue", tr("%1").arg(defaultSlicerSettings.SingleSlicer.MaxIsoValue));
	slicerSettingsElement.setAttribute("linearInterpolation", tr("%1").arg(defaultSlicerSettings.SingleSlicer.LinearInterpolation));
	slicerSettingsElement.setAttribute("snakeSlices", tr("%1").arg(defaultSlicerSettings.SnakeSlices));
	slicerSettingsElement.setAttribute("linkMDIs", tr("%1").arg(defaultSlicerSettings.LinkMDIs));
	slicerSettingsElement.setAttribute("cursorMode", tr( "%1" ).arg( defaultSlicerSettings.SingleSlicer.CursorMode));
	slicerSettingsElement.setAttribute("toolTipFontSize", tr("%1").arg(defaultSlicerSettings.SingleSlicer.ToolTipFontSize));

	doc.documentElement().appendChild(slicerSettingsElement);
}


void MainWindow::loadSlicerSettings(QDomNode &slicerSettingsNode)
{
	QDomNamedNodeMap attributes = slicerSettingsNode.attributes();

	defaultSlicerSettings.LinkViews = attributes.namedItem("linkViews").nodeValue() == "1";
	defaultSlicerSettings.SingleSlicer.ShowIsoLines = attributes.namedItem("showIsolines").nodeValue() == "1";
	defaultSlicerSettings.SingleSlicer.ShowPosition = attributes.namedItem("showPosition").nodeValue() == "1";
	defaultSlicerSettings.SingleSlicer.ShowAxesCaption = attributes.namedItem("showAxesCaption").nodeValue() == "1";
	defaultSlicerSettings.SingleSlicer.NumberOfIsoLines = attributes.namedItem("numberOfIsolines").nodeValue().toInt();
	defaultSlicerSettings.SingleSlicer.MinIsoValue = attributes.namedItem("minIsovalue").nodeValue().toDouble();
	defaultSlicerSettings.SingleSlicer.MaxIsoValue = attributes.namedItem("maxIsovalue").nodeValue().toDouble();
	defaultSlicerSettings.SingleSlicer.LinearInterpolation = attributes.namedItem("linearInterpolation").nodeValue().toDouble();
	defaultSlicerSettings.SnakeSlices = attributes.namedItem("snakeSlices").nodeValue().toDouble();
	defaultSlicerSettings.LinkMDIs = attributes.namedItem("linkMDIs").nodeValue() == "1";
	defaultSlicerSettings.SingleSlicer.CursorMode = attributes.namedItem("cursorMode").nodeValue().toStdString().c_str();
	defaultSlicerSettings.SingleSlicer.ToolTipFontSize = attributes.namedItem("toolTipFontSize").nodeValue().toInt();

	activeMdiChild()->editSlicerSettings(defaultSlicerSettings);
}


void MainWindow::removeNode(QDomNode &rootNode, char const *str)
{
	QDomNodeList list = rootNode.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		if (node.nodeName() == str)
		{
			rootNode.removeChild(node);
			break;
		}
	}
}


void MainWindow::maxXY()
{
	if (activeMdiChild() && activeMdiChild()->xyview())
		statusBar()->showMessage(tr("XY View"), 5000);
}


QList<QString> MainWindow::mdiWindowTitles()
{
	QList<QString> windowTitles;
	for (MdiChild* mdiChild: MdiChildList())
		windowTitles.append(mdiChild->windowTitle());
	return windowTitles;
}


void MainWindow::maxXZ()
{
	if (activeMdiChild() && activeMdiChild()->xzview())
		statusBar()->showMessage(tr("XZ View"), 5000);
}


void MainWindow::maxYZ()
{
	if (activeMdiChild() && activeMdiChild()->yzview())
		statusBar()->showMessage(tr("YZ View"), 5000);
}


void MainWindow::maxRC()
{
	if (activeMdiChild() && activeMdiChild()->rcview())
		statusBar()->showMessage(tr("Raycasting"), 5000);
}


void MainWindow::multi()
{
	if (activeMdiChild() && activeMdiChild()->multiview())
		statusBar()->showMessage(tr("Multiple Views"), 5000);
}


void MainWindow::linkViews()
{
	if (activeMdiChild())
	{
		defaultSlicerSettings.LinkViews = actionLink_views->isChecked();
		activeMdiChild()->linkViews(defaultSlicerSettings.LinkViews);

		if (defaultSlicerSettings.LinkViews)
			statusBar()->showMessage(tr("Link Views"), 5000);
	}
}

void MainWindow::linkMDIs()
{
	if (activeMdiChild())
	{
		defaultSlicerSettings.LinkMDIs = actionLink_mdis->isChecked();
		activeMdiChild()->linkMDIs(defaultSlicerSettings.LinkMDIs);

		if (defaultSlicerSettings.LinkViews)
			statusBar()->showMessage(tr("Link MDIs"), 5000);
	}
}


void MainWindow::enableInteraction()
{
	if (activeMdiChild())
	{
		defaultSlicerSettings.InteractorsEnabled = actionEnableInteraction->isChecked();
		activeMdiChild()->enableInteraction(defaultSlicerSettings.InteractorsEnabled);

		if (defaultSlicerSettings.InteractorsEnabled)
			statusBar()->showMessage(tr("Interaction Enabled"), 5000);
		else
			statusBar()->showMessage(tr("Interaction Disabled"), 5000);
	}
}


void MainWindow::prefs()
{
	MdiChild *child = activeMdiChild();

	QStringList inList = (QStringList() << tr("#Histogram Bins")
		<< tr("#Statistical extent")
		<< tr("$Compression")
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
		if (qssName == styleNames[key])
		{
			looks.append(QString("!") + key);
		}
		else
		{
			looks.append(key);
		}
	}
	iAPreferences p = child ? child->GetPreferences() : defaultPreferences;
	QTextDocument *fDescr = nullptr;
	if (iAConsole::GetInstance()->IsFileLogError())
	{
		fDescr = new QTextDocument();
		fDescr->setHtml("Could not write to the specified logfile, logging to file was therefore disabled."
			" Please check file permissions and/or whether the path to the file exists, before re-enabling the option!.");
	}
	QList<QVariant> inPara; 	inPara << tr("%1").arg(p.HistogramBins)
		<< tr("%1").arg(p.StatisticalExtent)
		<< (p.Compression ? tr("true") : tr("false"))
		<< (p.ResultInNewWindow ? tr("true") : tr("false"))
		<< (iAConsole::GetInstance()->IsLogToFileOn() ? tr("true") : tr("false"))
		<< iAConsole::GetInstance()->GetLogFileName()
		<< looks
		<< tr("%1").arg(p.MagicLensSize)
		<< tr("%1").arg(p.MagicLensFrameWidth);

	dlg_commoninput dlg(this, "Preferences", inList, inPara, fDescr);

	if (dlg.exec() == QDialog::Accepted)
	{
		defaultPreferences.HistogramBins = (int)dlg.getDblValue(0);
		defaultPreferences.StatisticalExtent = (int)dlg.getDblValue(1);
		defaultPreferences.Compression = dlg.getCheckValue(2) != 0;
		defaultPreferences.ResultInNewWindow = dlg.getCheckValue(3) != 0;
		bool logToFile = dlg.getCheckValue(4) != 0;
		QString logFileName = dlg.getText(5);
		QString looksStr = dlg.getComboBoxValue(6);
		qssName = styleNames[looksStr];
		applyQSS();

		defaultPreferences.MagicLensSize = clamp(MinimumMagicLensSize, MaximumMagicLensSize,
			static_cast<int>(dlg.getDblValue(7)));
		defaultPreferences.MagicLensFrameWidth = std::max(0, static_cast<int>(dlg.getDblValue(8)));

		if (activeMdiChild() && activeMdiChild()->editPrefs(defaultPreferences))
			statusBar()->showMessage(tr("Edit preferences"), 5000);

		iAConsole::GetInstance()->SetLogToFile(logToFile, logFileName, true);
	}
}

#include "vtkSmartVolumeMapper.h"

void MainWindow::renderSettings()
{
	MdiChild *child = activeMdiChild();

	QString t = tr("true");
	QString f = tr("false");

	QMap<int, QString> renderModes;
	renderModes.insert(vtkSmartVolumeMapper::DefaultRenderMode, tr("DefaultRenderMode"));
	renderModes.insert(vtkSmartVolumeMapper::RayCastRenderMode, tr("RayCastRenderMode"));
	renderModes.insert(vtkSmartVolumeMapper::GPURenderMode, tr("GPURenderMode"));

	int currentRenderMode = child->GetRenderMode();

	QStringList renderTypes;
	for (int mode : renderModes.keys())
	{
		renderTypes << ((mode == currentRenderMode) ? QString("!") : QString()) + renderModes[mode];
	}

	QStringList inList;
	inList
		<< tr("$Show slicers")
		<< tr("$Show helpers")
		<< tr("$Show position")
		<< tr("$Linear interpolation")
		<< tr("$Shading")
		<< tr("$Parallel projection")
		<< tr("#Sample distance")
		<< tr("#Ambient lighting")
		<< tr("#Diffuse lighting")
		<< tr("#Specular lighting")
		<< tr("#Specular power")
		<< tr("#Background top")
		<< tr("#Background bottom")
		<< tr("+Renderer Type");
	QList<QVariant> inPara;
	iARenderSettings const & renderSettings = child->GetRenderSettings();
	iAVolumeSettings const & volumeSettings = child->GetVolumeSettings();
	inPara << (renderSettings.ShowSlicers ? t : f)
		<< (renderSettings.ShowHelpers ? t : f)
		<< (renderSettings.ShowRPosition ? t : f)
		<< (volumeSettings.LinearInterpolation ? t : f)
		<< (volumeSettings.Shading ? t : f)
		<< (renderSettings.ParallelProjection ? t : f)

		<< tr("%1").arg(volumeSettings.SampleDistance)
		<< tr("%1").arg(volumeSettings.AmbientLighting)
		<< tr("%1").arg(volumeSettings.DiffuseLighting)
		<< tr("%1").arg(volumeSettings.SpecularLighting)
		<< tr("%1").arg(volumeSettings.SpecularPower)
		<< tr("%1").arg(renderSettings.BackgroundTop)
		<< tr("%1").arg(renderSettings.BackgroundBottom)
		<< renderTypes;

	dlg_commoninput dlg(this, "Renderer settings", inList, inPara, NULL);

	if (dlg.exec() == QDialog::Accepted)
	{
		defaultRenderSettings.ShowSlicers = dlg.getCheckValue(0) != 0;
		defaultRenderSettings.ShowHelpers = dlg.getCheckValue(1) != 0;
		defaultRenderSettings.ShowRPosition = dlg.getCheckValue(2) != 0;
		
		defaultVolumeSettings.LinearInterpolation = dlg.getCheckValue(3) != 0;
		defaultVolumeSettings.Shading = dlg.getCheckValue(4) != 0;
		defaultRenderSettings.ParallelProjection = dlg.getCheckValue(5) != 0;

		defaultVolumeSettings.SampleDistance = dlg.getDblValue(6);
		defaultVolumeSettings.AmbientLighting = dlg.getDblValue(7);
		defaultVolumeSettings.DiffuseLighting = dlg.getDblValue(8);
		defaultVolumeSettings.SpecularLighting = dlg.getDblValue(9);
		defaultVolumeSettings.SpecularPower = dlg.getDblValue(10);
		defaultRenderSettings.BackgroundTop = dlg.getText(11);
		defaultRenderSettings.BackgroundBottom = dlg.getText(12);

		QString renderType = dlg.getComboBoxValue(13);

		// TODO: use renderModes / reverse mapping ?
		defaultVolumeSettings.Mode = vtkSmartVolumeMapper::DefaultRenderMode;
		if (renderType == tr("DefaultRenderMode"))
		{
			defaultVolumeSettings.Mode = vtkSmartVolumeMapper::DefaultRenderMode;
		}
		else if (renderType == tr("RayCastRenderMode"))
		{
			defaultVolumeSettings.Mode = vtkSmartVolumeMapper::RayCastRenderMode;
		}
		else if (renderType == tr("GPURenderMode"))
		{
			defaultVolumeSettings.Mode = vtkSmartVolumeMapper::GPURenderMode;
		}

		if (activeMdiChild() && activeMdiChild()->editRendererSettings(
			defaultRenderSettings,
			defaultVolumeSettings))
		{
			statusBar()->showMessage(tr("Changed renderer settings"), 5000);
		}
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
	
	iASlicerSettings const & slicerSettings = child->GetSlicerSettings();
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
		<< (child->getLinkedMDIs() ? tr("true") : tr("false"))
		<< mouseCursorTypes
		<< (slicerSettings.SingleSlicer.ShowAxesCaption ? tr("true") : tr("false"))
		<< QString("%1").arg(slicerSettings.SingleSlicer.ToolTipFontSize)
		<< (slicerSettings.SingleSlicer.ShowTooltip ? tr("true") : tr("false"));

	dlg_commoninput dlg(this, "Slicer settings", inList, inPara, NULL);

	if (dlg.exec() == QDialog::Accepted)
	{
		defaultSlicerSettings.LinkViews = dlg.getCheckValue(0) != 0;
		defaultSlicerSettings.SingleSlicer.ShowPosition = dlg.getCheckValue(1) != 0;
		defaultSlicerSettings.SingleSlicer.ShowIsoLines = dlg.getCheckValue(2) != 0;
		defaultSlicerSettings.SingleSlicer.LinearInterpolation = dlg.getCheckValue(3) != 0;
		defaultSlicerSettings.SingleSlicer.NumberOfIsoLines = dlg.getIntValue(4);
		defaultSlicerSettings.SingleSlicer.MinIsoValue = dlg.getDblValue(5);
		defaultSlicerSettings.SingleSlicer.MaxIsoValue = dlg.getDblValue(6);
		defaultSlicerSettings.SnakeSlices = dlg.getIntValue(7);
		defaultSlicerSettings.LinkMDIs = dlg.getCheckValue(8) != 0;
		defaultSlicerSettings.SingleSlicer.CursorMode = dlg.getComboBoxValue(9);
		defaultSlicerSettings.SingleSlicer.ShowAxesCaption = dlg.getCheckValue(10) != 0;
		defaultSlicerSettings.SingleSlicer.ToolTipFontSize = dlg.getIntValue(11);
		defaultSlicerSettings.SingleSlicer.ShowTooltip = dlg.getCheckValue(12) != 0;

		if (activeMdiChild() && activeMdiChild()->editSlicerSettings(defaultSlicerSettings))
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


void MainWindow::saveTransferFunction()
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


void MainWindow::autoUpdate(bool toggled)
{
	if (activeMdiChild() && activeMdiChild()->isUpdateAutomatically() != toggled)
		activeMdiChild()->autoUpdate(toggled);
}


void MainWindow::updateViews()
{
	if (activeMdiChild())
		activeMdiChild()->updateViews();
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

void MainWindow::raycasterCamPX()
{
	if (activeMdiChild()) activeMdiChild()->camPX();
}
void MainWindow::raycasterCamPY()
{
	if (activeMdiChild()) activeMdiChild()->camPY();
}
void MainWindow::raycasterCamPZ()
{
	if (activeMdiChild()) activeMdiChild()->camPZ();
}
void MainWindow::raycasterCamMX()
{
	if (activeMdiChild()) activeMdiChild()->camMX();
}
void MainWindow::raycasterCamMY()
{
	if (activeMdiChild()) activeMdiChild()->camMY();
}
void MainWindow::raycasterCamMZ()
{
	if (activeMdiChild()) activeMdiChild()->camMZ();
}
void MainWindow::raycasterCamIso()
{
	if (activeMdiChild()) activeMdiChild()->camIso();
}


void MainWindow::raycasterAssignIso()
{
	QList<MdiChild *> mdiwindows = MdiChildList();
	int sizeMdi = mdiwindows.size();
	if (sizeMdi > 1)
	{
		double camOptions[10] = {0};
		if (activeMdiChild())  activeMdiChild()->getCamPosition(camOptions);
		for(int i = 0; i < sizeMdi; i++)
		{
			MdiChild *tmpChild = mdiwindows.at(i);

			// check dimension and spacing here, if not the same with active mdichild, skip.
			tmpChild->setCamPosition(camOptions, defaultRenderSettings.ParallelProjection);
		}
	}
}


void MainWindow::raycasterSaveCameraSettings()
{
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		QDomDocument doc = loadSettingsFile(fileName);
		saveCamera(doc);
		saveSettingsFile(doc, fileName);
	}
}


void MainWindow::raycasterLoadCameraSettings()
{
	// load camera settings
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		QFile file(fileName);
		file.open(QIODevice::ReadWrite);

		QDomDocument doc;
		if (!doc.setContent(&file))
		{
			QMessageBox msgBox;
			msgBox.setText("An error occurred during xml parsing!");
			msgBox.exec();

			file.close();
			return;
		}

		file.close();

		QDomElement root = doc.documentElement();
		QDomNodeList list = root.childNodes();
		for (int n = 0; n < int(list.length()); n++)
		{
			QDomNode node = list.item(n);
			if (node.nodeName() == "camera") loadCamera(node);
		}
	}

	// apply this camera settings to all the MdiChild.
	raycasterAssignIso();
}


MdiChild* MainWindow::GetResultChild(MdiChild* oldChild, QString const & title)
{
	if (oldChild->getResultInNewWindow())
	{
		// TODO: copy all modality images, or don't copy anything here and use image from old child directly,
		// or nothing at all until new image available!
		// Note that filters currently get their input from this child already!
		vtkSmartPointer<vtkImageData> imageData = oldChild->getImagePointer();
		MdiChild* newChild = createMdiChild(true);
		newChild->show();
		newChild->displayResult(title, imageData);
		copyFunctions(oldChild, newChild);
		return newChild;
	}
	oldChild->PrepareForResult();
	return oldChild;
}


void MainWindow::copyFunctions(MdiChild* oldChild, MdiChild* newChild)
{
	std::vector<dlg_function*> oldChildFunctions = oldChild->getFunctions();
	for (unsigned int i = 1; i < oldChildFunctions.size(); ++i)
	{
		dlg_function *curFunc = oldChildFunctions[i];
		switch (curFunc->getType())
		{
		case dlg_function::GAUSSIAN:
		{
			dlg_gaussian * oldGaussian = (dlg_gaussian*)curFunc;
			dlg_gaussian * newGaussian = new dlg_gaussian(newChild->getHistogram(), PredefinedColors()[i % 7]);
			newGaussian->setMean(oldGaussian->getMean());
			newGaussian->setMultiplier(oldGaussian->getMultiplier());
			newGaussian->setSigma(oldGaussian->getSigma());
			newChild->getFunctions().push_back(newGaussian);
		}
		break;
		case dlg_function::BEZIER:
		{
			dlg_bezier * oldBezier = (dlg_bezier*)curFunc;
			dlg_bezier * newBezier = new dlg_bezier(newChild->getHistogram(), PredefinedColors()[i % 7]);
			for (unsigned int j = 0; j < oldBezier->getPoints().size(); ++j)
				newBezier->addPoint(oldBezier->getPoints()[j].x(), oldBezier->getPoints()[j].y());
			newChild->getFunctions().push_back(newBezier);
		}
		break;
		default:	// unknown function type, do nothing
			break;
		}
	}
}


MdiChild * MainWindow::GetResultChild( QString const & title )
{
	return GetResultChild(activeMdiChild(), title);
}


MdiChild * MainWindow::GetResultChild( int childInd, QString const & f )
{
	return GetResultChild(MdiChildList().at(childInd), f);
}


void MainWindow::about()
{
	splashScreen->show();
	splashScreen->showMessage(tr("\n      Version: %1").arg (m_gitVersion), Qt::AlignTop, QColor(255, 255, 255));
}


void MainWindow::createRecentFileActions()
{
	separatorAct = menu_File->addSeparator();
	for (int i = 0; i < MaxRecentFiles; ++i) {
		recentFileActs[i] = new QAction(this);
		recentFileActs[i]->setVisible(false);
		menu_File->addAction(recentFileActs[i]);
	}
	updateRecentFileActions();
}


void MainWindow::updateMenus()
{
	bool hasMdiChild = (activeMdiChild() != 0);

	saveAct->setEnabled(hasMdiChild);
	saveAsAct->setEnabled(hasMdiChild);
	actionSave_Image_Stack->setEnabled(hasMdiChild);
	loadSettingsAct->setEnabled(hasMdiChild);
	saveSettingsAct->setEnabled(hasMdiChild);
	closeAct->setEnabled(hasMdiChild);
	closeAllAct->setEnabled(hasMdiChild);
	tileAct->setEnabled(hasMdiChild);
	cascadeAct->setEnabled(hasMdiChild);
	nextAct->setEnabled(hasMdiChild);
	previousAct->setEnabled(hasMdiChild);
	actionSave_Project->setEnabled(hasMdiChild);

	xyAct->setEnabled(hasMdiChild);
	xzAct->setEnabled(hasMdiChild);
	yzAct->setEnabled(hasMdiChild);
	rcAct->setEnabled(hasMdiChild);
	multiAct->setEnabled(hasMdiChild);
	tabAct->setEnabled(hasMdiChild);
	actionLink_views->setEnabled(hasMdiChild);
	actionLink_mdis->setEnabled(hasMdiChild);
	actionEnableInteraction->setEnabled(hasMdiChild);
	actionRendererSettings->setEnabled(hasMdiChild);
	actionSlicerSettings->setEnabled(hasMdiChild);
	actionLoad_transfer_function->setEnabled(hasMdiChild);
	actionSave_transfer_function->setEnabled(hasMdiChild);
	actionUpdate_automatically->setEnabled(hasMdiChild);
	actionUpdateViews->setEnabled(hasMdiChild);
	actionSnake_Slicer->setEnabled(hasMdiChild);
	actionMagicLens->setEnabled(hasMdiChild);
	actionView_X_direction_in_raycaster->setEnabled(hasMdiChild);
	actionView_mX_direction_in_raycaster->setEnabled(hasMdiChild);
	actionView_Y_direction_in_raycaster->setEnabled(hasMdiChild);
	actionView_mY_direction_in_raycaster->setEnabled(hasMdiChild);
	actionView_Z_direction_in_raycaster->setEnabled(hasMdiChild);
	actionView_mZ_direction_in_raycaster->setEnabled(hasMdiChild);
	actionIsometric_view_in_raycaster->setEnabled(hasMdiChild);
	actionAssignView->setEnabled(hasMdiChild);
	actionLoad_Camera_Settings->setEnabled(hasMdiChild);
	actionSave_Camera_Settings->setEnabled(hasMdiChild);
	actionReset_view->setEnabled(hasMdiChild);
	actionReset_function->setEnabled(hasMdiChild);
	actionRawProfile->setEnabled(hasMdiChild);
	actionLoad_Layout->setEnabled(hasMdiChild);
	actionSave_Layout->setEnabled(hasMdiChild);
	actionResetLayout->setEnabled(hasMdiChild);
	actionDelete_Layout->setEnabled(hasMdiChild);
	action_ChildStatusBar->setEnabled(hasMdiChild);

	updateRecentFileActions();

	if (activeMdiChild())
	{
		int selectedFuncPoint = activeMdiChild()->getSelectedFuncPoint();
		if (selectedFuncPoint == -1)
		{
			actionDelete_point->setEnabled(false);
			actionChange_color->setEnabled(false);
		}
		else if (activeMdiChild()->isFuncEndPoint(selectedFuncPoint))
		{
			actionDelete_point->setEnabled(false);
			actionChange_color->setEnabled(true);
		}
		else
		{
			actionDelete_point->setEnabled(true);
			actionChange_color->setEnabled(true);
		}

		actionUpdate_automatically->setChecked(activeMdiChild()->isUpdateAutomatically());

		//??if (activeMdiChild())
		//	histogramToolbar->setEnabled(activeMdiChild()->getTabIndex() == 1 && !activeMdiChild()->isMaximized());
	}
	else
	{
		actionDelete_point->setEnabled(false);
		actionChange_color->setEnabled(false);
	}
}


void MainWindow::updateWindowMenu()
{
	QList<MdiChild *> windows = MdiChildList();

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
		QAction *action  = menu_Window->addAction(text);
		action->setCheckable(true);
		action ->setChecked(child == activeMdiChild());
		connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
		windowMapper->setMapping(action, windows.at(i));
	}
}


MdiChild* MainWindow::createMdiChild(bool unsavedChanges)
{
	MdiChild *child = new MdiChild(this, defaultPreferences, unsavedChanges);
	QMdiSubWindow* subWin = mdiArea->addSubWindow(child);
	subWin->setOption(QMdiSubWindow::RubberBandResize);
	subWin->setOption(QMdiSubWindow::RubberBandMove);

	child->setRenderSettings(defaultRenderSettings, defaultVolumeSettings);
	child->setupSlicers(defaultSlicerSettings, false);

	connect( child, SIGNAL( pointSelected() ), this, SLOT( pointSelected() ) );
	connect( child, SIGNAL( noPointSelected() ), this, SLOT( noPointSelected() ) );
	connect( child, SIGNAL( endPointSelected() ), this, SLOT( endPointSelected() ) );
	connect( child, SIGNAL( active() ), this, SLOT( setHistogramFocus() ) );
	connect( child, SIGNAL( autoUpdateChanged( bool ) ), actionUpdate_automatically, SLOT( setChecked( bool ) ) );
	connect( child, SIGNAL( closed() ), this, SLOT( childClosed() ) );

	SetModuleActionsEnabled( true );

	m_moduleDispatcher->ChildCreated(child);
	return child;
}


void MainWindow::connectSignalsToSlots()
{
	connect(openAct, SIGNAL(triggered()), this, SLOT(Open()));
	connect(actionOpen_CSV, SIGNAL(triggered()), this, SLOT(OpenCSV()));
	connect(actionOpen_Raw, SIGNAL(triggered()), this, SLOT(OpenRaw()));
	connect(actionOpen_Image_Stack, SIGNAL(triggered()), this, SLOT(OpenImageStack()));
	connect(actionOpen_Volume_Stack, SIGNAL(triggered()), this, SLOT(OpenVolumeStack()));
	connect(actionOpen_With_DataTypeConversion, SIGNAL(triggered()), this, SLOT(OpenWithDataTypeConversion()));
	connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));
	connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(loadSettingsAct, SIGNAL(triggered()), this, SLOT(loadSettings()));
	connect(saveSettingsAct, SIGNAL(triggered()), this, SLOT(saveSettings()));
	connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
	connect(closeAct, SIGNAL(triggered()), mdiArea, SLOT(closeActiveSubWindow()));
	connect(closeAllAct, SIGNAL(triggered()), this, SLOT(CloseAllSubWindows()));
	connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));
	connect(cascadeAct, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));
	connect(nextAct, SIGNAL(triggered()), mdiArea, SLOT(activateNextSubWindow()));
	connect(previousAct, SIGNAL(triggered()), mdiArea, SLOT(activatePreviousSubWindow()));
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
	connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	connect(xyAct, SIGNAL(triggered()), this, SLOT(maxXY()));
	connect(xzAct, SIGNAL(triggered()), this, SLOT(maxXZ()));
	connect(yzAct, SIGNAL(triggered()), this, SLOT(maxYZ()));
	connect(rcAct, SIGNAL(triggered()), this, SLOT(maxRC()));
	connect(multiAct, SIGNAL(triggered()), this, SLOT(multi()));
	connect(actionLink_views, SIGNAL(triggered()), this, SLOT(linkViews()));
	connect(actionLink_mdis, SIGNAL(triggered()), this, SLOT(linkMDIs()));
	connect(actionEnableInteraction, SIGNAL(triggered()), this, SLOT(enableInteraction()));
	connect(actionPreferences, SIGNAL(triggered()), this, SLOT(prefs()));
	connect(actionRendererSettings, SIGNAL(triggered()), this, SLOT(renderSettings()));
	connect(actionSlicerSettings, SIGNAL(triggered()), this, SLOT(slicerSettings()));
	connect(actionLoad_transfer_function, SIGNAL(triggered()), this, SLOT(loadTransferFunction()));
	connect(actionSave_transfer_function, SIGNAL(triggered()), this, SLOT(saveTransferFunction()));
	connect(actionDelete_point, SIGNAL(triggered()), this, SLOT(deletePoint()));
	connect(actionChange_color, SIGNAL(triggered()), this, SLOT(changeColor()));
	connect(actionUpdate_automatically, SIGNAL(toggled(bool)), this, SLOT(autoUpdate(bool)));
	//connect(actionUpdate_3D_view, SIGNAL(triggered()), this, SLOT(update3DView()));
	connect(actionUpdateViews, SIGNAL(triggered()), this, SLOT(updateViews()));
	connect(actionSnake_Slicer, SIGNAL(toggled(bool)), this, SLOT(toggleSnakeSlicer(bool)));
	connect(actionMagicLens, SIGNAL(toggled(bool)), this, SLOT(toggleMagicLens(bool)));
	connect(actionView_X_direction_in_raycaster, SIGNAL(triggered()), this, SLOT(raycasterCamPX()));
	connect(actionView_mX_direction_in_raycaster, SIGNAL(triggered()), this, SLOT(raycasterCamMX()));
	connect(actionView_Y_direction_in_raycaster, SIGNAL(triggered()), this, SLOT(raycasterCamPY()));
	connect(actionView_mY_direction_in_raycaster, SIGNAL(triggered()), this, SLOT(raycasterCamMY()));
	connect(actionView_Z_direction_in_raycaster, SIGNAL(triggered()), this, SLOT(raycasterCamPZ()));
	connect(actionView_mZ_direction_in_raycaster, SIGNAL(triggered()), this, SLOT(raycasterCamMZ()));
	connect(actionIsometric_view_in_raycaster, SIGNAL(triggered()), this, SLOT(raycasterCamIso()));
	connect(actionAssignView, SIGNAL(triggered()), this, SLOT(raycasterAssignIso()));
	connect(actionSave_Camera_Settings, SIGNAL(triggered()), this, SLOT(raycasterSaveCameraSettings()));
	connect(actionLoad_Camera_Settings, SIGNAL(triggered()), this, SLOT(raycasterLoadCameraSettings()));
	connect(actionReset_view, SIGNAL(triggered()), this, SLOT(resetView()));
	connect(actionReset_function, SIGNAL(triggered()), this, SLOT(resetTrf()));
	connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(updateMenus()));
	connect(windowMapper, SIGNAL(mapped(QWidget *)), this, SLOT(setActiveSubWindow(QWidget *)));

	connect(actionRawProfile, SIGNAL(toggled(bool)), this, SLOT(toggleSliceProfile(bool)));

	connect(actionSave_Layout, SIGNAL(triggered()), this, SLOT(saveLayout()));
	connect(actionLoad_Layout, SIGNAL(triggered()), this, SLOT(loadLayout()));
	connect(actionDelete_Layout, SIGNAL(triggered()), this, SLOT(deleteLayout()));
	connect(actionResetLayout, SIGNAL(triggered()), this, SLOT(resetLayout()));
	connect(actionShowToolbar, SIGNAL(triggered()), this, SLOT(ToggleToolbar()));
	connect(action_MainWindowStatusBar, SIGNAL(triggered()), this, SLOT(ToggleMainWindowStatusBar()));
	connect(action_ChildStatusBar, SIGNAL(triggered()), this, SLOT(ToggleChildStatusBar()));
	
	connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(childActivatedSlot(QMdiSubWindow*)));
	for (int i = 0; i < MaxRecentFiles; ++i) {
		connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(OpenRecentFile()));
	}

	connect(actionOpen_Project, SIGNAL(triggered()), this, SLOT(LoadProject()));
	connect(actionSave_Project, SIGNAL(triggered()), this, SLOT(SaveProject()));
	connect(actionOpen_TLGI_CT_Data, SIGNAL(triggered()), this, SLOT(OpenTLGICTData()));
}


void MainWindow::readSettings()
{
	QSettings settings;
	path = settings.value("Path").toString();

	qssName = settings.value("qssName", ":/bright.qss").toString();

	defaultLayout = settings.value("Preferences/defaultLayout", "").toString();
	defaultPreferences.HistogramBins = settings.value("Preferences/prefHistogramBins", DefaultHistogramBins).toInt();
	defaultPreferences.StatisticalExtent = settings.value("Preferences/prefStatExt", 3).toInt();
	defaultPreferences.Compression = settings.value("Preferences/prefCompression", true).toBool();
	defaultPreferences.ResultInNewWindow = settings.value("Preferences/prefResultInNewWindow", true).toBool();
	defaultPreferences.MagicLensSize = settings.value("Preferences/prefMagicLensSize", DefaultMagicLensSize).toInt();
	defaultPreferences.MagicLensFrameWidth = settings.value("Preferences/prefMagicLensFrameWidth", 3).toInt();
	bool prefLogToFile = settings.value("Preferences/prefLogToFile", false).toBool();
	QString logFileName = settings.value("Preferences/prefLogFile", "debug.log").toString();
	iAConsole::GetInstance()->SetLogToFile(prefLogToFile, logFileName);

	defaultRenderSettings.ShowSlicers = settings.value("Renderer/rsShowSlicers", false).toBool();
	defaultRenderSettings.ShowHelpers = settings.value("Renderer/rsShowHelpers", true).toBool();
	defaultRenderSettings.ShowRPosition = settings.value("Renderer/rsShowRPosition", true).toBool();
	defaultVolumeSettings.LinearInterpolation = settings.value("Renderer/rsLinearInterpolation", true).toBool();
	defaultVolumeSettings.Shading = settings.value("Renderer/rsShading", true).toBool();
	defaultRenderSettings.ParallelProjection = settings.value("Renderer/rsParallelProjection", false).toBool();
	defaultVolumeSettings.SampleDistance = settings.value("Renderer/rsSampleDistance", 1).toDouble();
	defaultVolumeSettings.AmbientLighting = settings.value("Renderer/rsAmbientLighting", 0.2).toDouble();
	defaultVolumeSettings.DiffuseLighting = settings.value("Renderer/rsDiffuseLighting", 0.5).toDouble();
	defaultVolumeSettings.SpecularLighting = settings.value("Renderer/rsSpecularLighting", 0.7).toDouble();
	defaultVolumeSettings.SpecularPower = settings.value("Renderer/rsSpecularPower", 10).toDouble();
	defaultRenderSettings.BackgroundTop = settings.value("Renderer/rsBackgroundTop", "#7FAAFF").toString();
	defaultRenderSettings.BackgroundBottom = settings.value("Renderer/rsBackgroundBottom", "#FFFFFF").toString();
	defaultVolumeSettings.Mode = settings.value("Renderer/rsRenderMode", 0).toInt();

	defaultSlicerSettings.LinkViews = settings.value("Slicer/ssLinkViews", false).toBool();
	defaultSlicerSettings.SingleSlicer.ShowPosition = settings.value("Slicer/ssShowPosition", true).toBool();
	defaultSlicerSettings.SingleSlicer.ShowAxesCaption = settings.value("Slicer/ssShowAxesCaption", false).toBool();
	defaultSlicerSettings.SingleSlicer.ShowIsoLines = settings.value("Slicer/ssShowIsolines", false).toBool();
	defaultSlicerSettings.LinkMDIs = settings.value("Slicer/ssLinkMDIs", false).toBool();
	defaultSlicerSettings.SingleSlicer.NumberOfIsoLines = settings.value("Slicer/ssNumberOfIsolines", 5).toDouble();
	defaultSlicerSettings.SingleSlicer.MinIsoValue = settings.value("Slicer/ssMinIsovalue", 20000).toDouble();
	defaultSlicerSettings.SingleSlicer.MaxIsoValue = settings.value("Slicer/ssMaxIsovalue", 40000).toDouble();
	defaultSlicerSettings.SingleSlicer.LinearInterpolation = settings.value("Slicer/ssImageActorUseInterpolation", true).toBool();
	defaultSlicerSettings.SingleSlicer.CursorMode = settings.value( "Slicer/ssCursorMode", "Crosshair default").toString();
	defaultSlicerSettings.SnakeSlices = settings.value("Slicer/ssSnakeSlices", 100).toInt();
	defaultSlicerSettings.SingleSlicer.ToolTipFontSize = settings.value("Slicer/toolTipFontSize", 12).toInt();

	lpCamera = settings.value("Parameters/lpCamera").toBool();
	lpSliceViews = settings.value("Parameters/lpSliceViews").toBool();
	lpTransferFunction = settings.value("Parameters/lpTransferFunction").toBool();
	lpProbabilityFunctions = settings.value("Parameters/lpProbabilityFunctions").toBool();
	lpPreferences = settings.value("Parameters/lpPreferences").toBool();
	lpRenderSettings = settings.value("Parameters/lpRenderSettings").toBool();
	lpSlicerSettings = settings.value("Parameters/lpSlicerSettings").toBool();

	spCamera = settings.value("Parameters/spCamera").toBool();
	spSliceViews = settings.value("Parameters/spSliceViews").toBool();
	spTransferFunction = settings.value("Parameters/spTransferFunction").toBool();
	spProbabilityFunctions = settings.value("Parameters/spProbabilityFunctions").toBool();
	spPreferences = settings.value("Parameters/spPreferences").toBool();
	spRenderSettings = settings.value("Parameters/spRenderSettings").toBool();
	spSlicerSettings = settings.value("Parameters/spSlicerSettings").toBool();

	owdtcs = settings.value("OpenWithDataTypeConversion/owdtcs").toInt();
	owdtcx = settings.value("OpenWithDataTypeConversion/owdtcx").toInt();
	owdtcy = settings.value("OpenWithDataTypeConversion/owdtcy").toInt();
	owdtcz = settings.value("OpenWithDataTypeConversion/owdtcz").toInt();
	owdtcsx = settings.value("OpenWithDataTypeConversion/owdtcsx").toDouble();
	owdtcsy = settings.value("OpenWithDataTypeConversion/owdtcsy").toDouble();
	owdtcsz = settings.value("OpenWithDataTypeConversion/owdtcsz").toDouble();
	owdtcmin = settings.value("OpenWithDataTypeConversion/owdtcmin").toDouble();
	owdtcmax = settings.value("OpenWithDataTypeConversion/owdtcmax").toDouble();
	owdtcoutmin = settings.value("OpenWithDataTypeConversion/owdtcoutmin").toDouble();
	owdtcoutmax = settings.value("OpenWithDataTypeConversion/owdtcoutmax").toDouble();
	owdtcdov = settings.value("OpenWithDataTypeConversion/owdtcdov").toInt();
	owdtcxori = settings.value("OpenWithDataTypeConversion/owdtcxori").toInt();
	owdtcyori = settings.value("OpenWithDataTypeConversion/owdtcyori").toInt();
	owdtczori = settings.value("OpenWithDataTypeConversion/owdtczori").toInt();
	owdtcxsize = settings.value("OpenWithDataTypeConversion/owdtcxsize").toInt();
	owdtcysize = settings.value("OpenWithDataTypeConversion/owdtcysize").toInt();
	owdtczsize = settings.value("OpenWithDataTypeConversion/owdtczsize").toInt();

	settings.beginGroup("Layout");
	layoutNames = settings.allKeys();
	layoutNames = layoutNames.filter(QRegularExpression("^state"));
	layoutNames.replaceInStrings(QRegularExpression("^state"), "");
	settings.endGroup();
	if (layoutNames.size() == 0)
	{
		layoutNames.push_back("1");
		layoutNames.push_back("2");
		layoutNames.push_back("3");
	}
}


void MainWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("Path", path);
	settings.setValue("qssName", qssName);

	settings.setValue("Preferences/defaultLayout", layout->currentText());
	settings.setValue("Preferences/prefHistogramBins", defaultPreferences.HistogramBins);
	settings.setValue("Preferences/prefStatExt", defaultPreferences.StatisticalExtent);
	settings.setValue("Preferences/prefCompression", defaultPreferences.Compression);
	settings.setValue("Preferences/prefResultInNewWindow", defaultPreferences.ResultInNewWindow);
	settings.setValue("Preferences/prefMagicLensSize", defaultPreferences.MagicLensSize);
	settings.setValue("Preferences/prefMagicLensFrameWidth", defaultPreferences.MagicLensFrameWidth);
	settings.setValue("Preferences/prefLogToFile", iAConsole::GetInstance()->IsLogToFileOn());
	settings.setValue("Preferences/prefLogFile", iAConsole::GetInstance()->GetLogFileName());

	settings.setValue("Renderer/rsShowSlicers", defaultRenderSettings.ShowSlicers);
	settings.setValue("Renderer/rsLinearInterpolation", defaultVolumeSettings.LinearInterpolation);
	settings.setValue("Renderer/rsShading", defaultVolumeSettings.Shading);
	settings.setValue("Renderer/rsParallelProjection", defaultRenderSettings.ParallelProjection);
	settings.setValue("Renderer/rsShowHelpers", defaultRenderSettings.ShowHelpers);
	settings.setValue("Renderer/rsShowRPosition", defaultRenderSettings.ShowRPosition);
	settings.setValue("Renderer/rsSampleDistance", defaultVolumeSettings.SampleDistance);
	settings.setValue("Renderer/rsAmbientLighting", defaultVolumeSettings.AmbientLighting);
	settings.setValue("Renderer/rsDiffuseLighting", defaultVolumeSettings.DiffuseLighting);
	settings.setValue("Renderer/rsSpecularLighting", defaultVolumeSettings.SpecularLighting);
	settings.setValue("Renderer/rsSpecularPower", defaultVolumeSettings.SpecularPower);
	settings.setValue("Renderer/rsBackgroundTop", defaultRenderSettings.BackgroundTop);
	settings.setValue("Renderer/rsBackgroundBottom", defaultRenderSettings.BackgroundBottom);
	settings.setValue("Renderer/rsRenderMode", defaultVolumeSettings.Mode);

	settings.setValue("Slicer/ssLinkViews", defaultSlicerSettings.LinkViews);
	settings.setValue("Slicer/ssShowPosition", defaultSlicerSettings.SingleSlicer.ShowPosition);
	settings.setValue("Slicer/ssShowAxesCaption", defaultSlicerSettings.SingleSlicer.ShowAxesCaption);
	settings.setValue("Slicer/ssShowIsolines", defaultSlicerSettings.SingleSlicer.ShowIsoLines);
	settings.setValue("Slicer/ssLinkMDIs", defaultSlicerSettings.LinkMDIs);
	settings.setValue("Slicer/ssNumberOfIsolines", defaultSlicerSettings.SingleSlicer.NumberOfIsoLines);
	settings.setValue("Slicer/ssMinIsovalue", defaultSlicerSettings.SingleSlicer.MinIsoValue);
	settings.setValue("Slicer/ssMaxIsovalue", defaultSlicerSettings.SingleSlicer.MaxIsoValue);
	settings.setValue("Slicer/ssImageActorUseInterpolation", defaultSlicerSettings.SingleSlicer.LinearInterpolation);
	settings.setValue("Slicer/ssSnakeSlices", defaultSlicerSettings.SnakeSlices);
	settings.setValue("Slicer/ssCursorMode", defaultSlicerSettings.SingleSlicer.CursorMode);
	settings.setValue("Slicer/toolTipFontSize", defaultSlicerSettings.SingleSlicer.ToolTipFontSize);

	settings.setValue("Parameters/lpCamera", lpCamera);
	settings.setValue("Parameters/lpSliceViews", lpSliceViews);
	settings.setValue("Parameters/lpTransferFunction", lpTransferFunction);
	settings.setValue("Parameters/lpProbabilityFunctions", lpProbabilityFunctions);
	settings.setValue("Parameters/lpPreferences", lpPreferences);
	settings.setValue("Parameters/lpRenderSettings", lpRenderSettings);
	settings.setValue("Parameters/lpSlicerSettings", lpSlicerSettings);

	settings.setValue("Parameters/spCamera", spCamera);
	settings.setValue("Parameters/spSliceViews", spSliceViews);
	settings.setValue("Parameters/spTransferFunction", spTransferFunction);
	settings.setValue("Parameters/spProbabilityFunctions", spProbabilityFunctions);
	settings.setValue("Parameters/spPreferences", spPreferences);
	settings.setValue("Parameters/spRenderSettings", spRenderSettings);
	settings.setValue("Parameters/spSlicerSettings", spSlicerSettings);

	settings.setValue("OpenWithDataTypeConversion/owdtcs", owdtcs);
	settings.setValue("OpenWithDataTypeConversion/owdtcx", owdtcx);
	settings.setValue("OpenWithDataTypeConversion/owdtcy", owdtcy);
	settings.setValue("OpenWithDataTypeConversion/owdtcz", owdtcz);
	settings.setValue("OpenWithDataTypeConversion/owdtcsx", owdtcsx);
	settings.setValue("OpenWithDataTypeConversion/owdtcsy", owdtcsy);
	settings.setValue("OpenWithDataTypeConversion/owdtcsz", owdtcsz);
	settings.setValue("OpenWithDataTypeConversion/owdtcmin", owdtcmin);
	settings.setValue("OpenWithDataTypeConversion/owdtcmax", owdtcmax);
	settings.setValue("OpenWithDataTypeConversion/owdtcoutmin", owdtcoutmin);
	settings.setValue("OpenWithDataTypeConversion/owdtcoutmax", owdtcoutmax);
	settings.setValue("OpenWithDataTypeConversion/owdtcdov", owdtcdov);
	settings.setValue("OpenWithDataTypeConversion/owdtcxori", owdtcxori);
	settings.setValue("OpenWithDataTypeConversion/owdtcyori", owdtcyori);
	settings.setValue("OpenWithDataTypeConversion/owdtczori", owdtczori);
	settings.setValue("OpenWithDataTypeConversion/owdtcxsize", owdtcxsize);
	settings.setValue("OpenWithDataTypeConversion/owdtcysize", owdtcysize);
	settings.setValue("OpenWithDataTypeConversion/owdtczsize", owdtczsize);
}


void MainWindow::setCurrentFile(const QString &fileName)
{
	if (fileName.isEmpty())
	{
		DEBUG_LOG("Can't use empty filename as current!");
		return;
	}
	curFile = fileName;
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);

	while (files.size() > MaxRecentFiles)
		files.removeLast();

	settings.setValue("recentFileList", files);

	foreach (QWidget *widget, QApplication::topLevelWidgets()) {
		MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
		if (mainWin)
			mainWin->updateRecentFileActions();
	}
}


void MainWindow::updateRecentFileActions()
{
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();

	int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
		recentFileActs[j]->setVisible(false);

	separatorAct->setVisible(numRecentFiles > 0);
}


MdiChild* MainWindow::activeMdiChild()
{
	int subWndCnt = MdiChildList().size();
	if(subWndCnt>0)
	{
		return MdiChildList(QMdiArea::ActivationHistoryOrder).last();
	}

	return 0;
}


MdiChild* MainWindow::findMdiChild(const QString &fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

	for (MdiChild* mdiChild: MdiChildList())
		if (mdiChild->currentFile() == canonicalFilePath)
			return mdiChild;
	return 0;
}


void MainWindow::setActiveSubWindow(QWidget *window)
{
	if (!window)
		return;
	mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}


void MainWindow::pointSelected()
{
	actionChange_color->setEnabled(true);
	actionDelete_point->setEnabled(true);
}


void MainWindow::noPointSelected()
{
	actionChange_color->setEnabled(false);
	actionDelete_point->setEnabled(false);
}


void MainWindow::endPointSelected()
{
	actionChange_color->setEnabled(true);
	actionDelete_point->setEnabled(false);
}


void MainWindow::setHistogramFocus()
{
	if (activeMdiChild())
		activeMdiChild()->setHistogramFocus();
}


QString MainWindow::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}


QList<MdiChild*> MainWindow::MdiChildList(QMdiArea::WindowOrder order)
{
	QList<MdiChild*> res;
	foreach(QMdiSubWindow *window, mdiArea->subWindowList(order))
	{
		MdiChild * child = qobject_cast<MdiChild*>(window->widget());
		if (child)
			res.append(child);
	}
	return res;
}


void MainWindow::childActivatedSlot(QMdiSubWindow *wnd)
{
	MdiChild * activeChild = activeMdiChild();
	if (activeChild && wnd)
	{
		actionRawProfile->setChecked(activeChild->isSliceProfileToggled());
		//actionSnake_Slicer->setChecked(activeChild->isSnakeSlicerToggled());
		QSignalBlocker blockMagicLensSignal(actionMagicLens);
		actionMagicLens->setChecked(activeChild->isMagicLensToggled());
	}
}


void MainWindow::applyQSS()
{
	// Load an application style
	QFile styleFile(qssName);
	if (styleFile.open( QFile::ReadOnly ))
	{
		QTextStream styleIn(&styleFile);
		QString style = styleIn.readAll();
		styleFile.close();
		qApp->setStyleSheet(style);
		emit StyleChanged();
	}
}


void MainWindow::saveLayout()
{
	MdiChild *child = activeMdiChild();
	if(child)
	{
		QByteArray state = child->saveState(0);
		QSettings settings;
		QString layoutName(layout->currentText());
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
			if (layout->findText(layoutName) == -1)
			{
				layout->addItem(layoutName);
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
			layout->setCurrentIndex(layout->findText(layoutName));
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
	child->LoadLayout(layout->currentText());
}


void MainWindow::deleteLayout()
{
	if (QMessageBox::question(this, "Delete Layout",
		QString("Do you want to delete the layout '")+layout->currentText()+"'?")
		== QMessageBox::Yes)
	{
		QSettings settings;
		settings.remove("Layout/state" + layout->currentText());
		layout->removeItem(layout->currentIndex());
	}
}


void MainWindow::resetLayout()
{
	activeMdiChild()->resetLayout();
}


QMenu * MainWindow::getToolsMenu()
{
	return this->menu_Tools;
}


void MainWindow::ToggleMainWindowStatusBar()
{
	statusBar()->setVisible(action_MainWindowStatusBar->isChecked());
}


void MainWindow::ToggleToolbar()
{
	bool visible = actionShowToolbar->isChecked();
	QList<QToolBar *> toolbars = findChildren<QToolBar *>();
	for (auto toolbar : toolbars)
	{
		toolbar->setVisible(visible);
	}
}


void MainWindow::ToggleChildStatusBar()
{
	if (!activeMdiChild())
	{
		return;
	}
	activeMdiChild()->statusBar()->setVisible(action_ChildStatusBar->isChecked());
}


QMenu * MainWindow::getFiltersMenu()
{
	return this->menu_Filters;
}


QMdiSubWindow* MainWindow::addSubWindow( QWidget * child )
{
	return mdiArea->addSubWindow( child );
}


QMenu * MainWindow::getHelpMenu()
{
	return this->menu_Help;
}


QMenu * MainWindow::getFileMenu()
{
	return this->menu_File;
}


void MainWindow::SetModuleActionsEnabled( bool isEnabled )
{
	m_moduleDispatcher->SetModuleActionsEnabled(isEnabled);
}


void MainWindow::childClosed()
{
	MdiChild * sender = dynamic_cast<MdiChild*> (QObject::sender());
	if (!sender)
		return;
	// magic lens size can be modified in the slicers as well; make sure to store this change:
	defaultPreferences.MagicLensSize = sender->GetMagicLensSize();
	if( mdiArea->subWindowList().size() == 1 )
	{
		MdiChild * child = dynamic_cast<MdiChild*> ( mdiArea->subWindowList().at( 0 )->widget() );
		if(!child)
			return;
		if( child == sender )
			SetModuleActionsEnabled( false );
	}
}


void MainWindow::LoadProject()
{
	QString fileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr("Open Input File"),
		path,
		iAIOProvider::ProjectFileTypeFilter);
	LoadProject(fileName);
}


void MainWindow::LoadProject(QString const & fileName)
{
	if (fileName.isEmpty())
		return;
	MdiChild* child = createMdiChild(false);
	if (child->LoadProject(fileName))
	{
		child->show();
	}
	else
	{
		delete child;
	}
}


void MainWindow::SaveProject()
{
	MdiChild * activeChild = activeMdiChild();
	if (!activeChild)
		return;
	activeChild->StoreProject();
}


void MainWindow::LoadArguments(int argc, char** argv)
{
	QStringList files;
	for (int a = 1; a < argc; ++a) files << argv[a];
	LoadFiles(files);
}


iAPreferences const & MainWindow::GetDefaultPreferences() const
{
	return defaultPreferences;
}


// Move to other places (modules?):


void MainWindow::OpenWithDataTypeConversion()
{
	// Create an image data
	vtkImageData* imageData = vtkImageData::New();
	QString finalfilename;
	QString testfinalfilename;

	QString file = QFileDialog::getOpenFileName(this,
		tr("Open File"),
		path,
		iAIOProvider::GetSupportedLoadFormats()
	);
	if (file.isEmpty())
	{
		return;
	}
	QStringList inList = (QStringList()
		<< tr("+Data Type")
		<< tr("#Slice sample rate")
		<< tr("# Dim X")   << tr("# Dim Y")   << tr("# Dim Z")
		<< tr("# Space X") << tr("# Space Y") << tr("# Space Z"));
	QList<QVariant> inPara;
	inPara << VTKDataTypeList()
		<< tr("%1").arg(owdtcs)
		<< tr("%1").arg(owdtcx) << tr("%1").arg(owdtcy) << tr("%1").arg(owdtcz)
		<< tr("%1").arg(owdtcsx)<< tr("%1").arg(owdtcsy)<< tr("%1").arg(owdtcsz);

	dlg_commoninput dlg(this, "Open With DataType Conversion", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	owdtcs = dlg.getDblValue(1);
	owdtcx = dlg.getDblValue(2); owdtcy = dlg.getDblValue(3); owdtcz = dlg.getDblValue(4);
	owdtcsx = dlg.getDblValue(5); owdtcsy = dlg.getDblValue(6);	owdtcsz = dlg.getDblValue(7);

	QString owdtcintype = dlg.getComboBoxValue(0);

	double para[8];
	para[0] = dlg.getDblValue(1);
	para[1] = dlg.getDblValue(2); para[2] = dlg.getDblValue(3); para[3] = dlg.getDblValue(4);
	para[4] = dlg.getDblValue(5); para[5] = dlg.getDblValue(6);	para[6] = dlg.getDblValue(7);
	para[7] = defaultPreferences.HistogramBins;

	QSize qwinsize = this->size();
	double winsize[2];
	winsize[0] = qwinsize.width();	winsize[1] = qwinsize.height();

	double convPara[11];
	convPara[0] = owdtcmin;   convPara[1] = owdtcmax;  convPara[2] = owdtcoutmin; convPara[3] = owdtcoutmax; convPara[4] = owdtcdov; convPara[5] = owdtcxori;
	convPara[6] = owdtcxsize; convPara[7] = owdtcyori; convPara[8] = owdtcysize;  convPara[9] = owdtczori;   convPara[10] = owdtczsize;
	dlg_datatypeconversion* conversionwidget = new dlg_datatypeconversion( this, imageData, file.toStdString().c_str(), MapVTKTypeStringToInt(owdtcintype), para, winsize, convPara );
	if (conversionwidget->exec() != QDialog::Accepted)
	{
		return;
	}
	QString outDataType = conversionwidget->getDataType();
	owdtcmin = conversionwidget->getRangeLower(); owdtcmax = conversionwidget->getRangeUpper();
	owdtcoutmin = conversionwidget->getOutputMin(); owdtcoutmax = conversionwidget->getOutputMax();
	owdtcdov = conversionwidget->getConvertROI();
	owdtcxori = conversionwidget->getXOrigin(); owdtcxsize = conversionwidget->getXSize();
	owdtcyori = conversionwidget->getYOrigin(); owdtcysize = conversionwidget->getYSize();
	owdtczori = conversionwidget->getZOrigin(); owdtczsize = conversionwidget->getZSize();

	double roi[6];
	roi[0] = owdtcxori; roi[1] = owdtcxsize;
	roi[2] = owdtcyori; roi[3] = owdtcysize;
	roi[4] = owdtczori; roi[5] = owdtczsize;

	if ( owdtcdov == 0 )
	{
		testfinalfilename = conversionwidget->coreconversionfunction(file, finalfilename, para,
			MapVTKTypeStringToInt(owdtcintype),
			MapVTKTypeStringToInt(outDataType),
			owdtcmin, owdtcmax, owdtcoutmin, owdtcoutmax, owdtcdov  );
	}
	else
	{
		testfinalfilename = conversionwidget->coreconversionfunctionforroi(file, finalfilename, para,
			MapVTKTypeStringToInt(outDataType),
			owdtcmin, owdtcmax, owdtcoutmin, owdtcoutmax, owdtcdov, roi  );
	}
	LoadFile(testfinalfilename, false);
}


void MainWindow::OpenTLGICTData()
{
	QString baseDirectory = QFileDialog::getExistingDirectory(
		this,
		tr("Open Talbot-Lau Grating Interferometer CT Dataset"),
		getPath(),
		QFileDialog::ShowDirsOnly);
	LoadTLGICTData(baseDirectory);
}


void MainWindow::LoadTLGICTData(QString const & baseDirectory)
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


void MainWindow::InitResources()
{
	Q_INIT_RESOURCE(open_iA);
}


int MainWindow::RunGUI(int argc, char * argv[], QString const & appName, QString const & version,
	QString const & splashPath, QString const & iconPath)
{
	MainWindow::InitResources();
	QApplication app(argc, argv);
	app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
	iAGlobalLogger::SetLogger(iAConsole::GetInstance());
	MainWindow mainWin(appName, version, splashPath);
	CheckSCIFIO(QCoreApplication::applicationDirPath());
	mainWin.LoadArguments(argc, argv);
	// TODO: unify with logo in slicer/renderer!
	app.setWindowIcon(QIcon(QPixmap(iconPath)));
	mainWin.setWindowIcon(QIcon(QPixmap(iconPath)));
	if (QDate::currentDate().dayOfYear() >= 340)
	{
		mainWin.setWindowTitle("Merry X-Mas and a happy new year!");
		mainWin.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
		app.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
	}
	mainWin.show();
	return app.exec();
}
