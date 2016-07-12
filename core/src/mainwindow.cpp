/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
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
#include "iAHistogramWidget.h"
#include "iAIOProvider.h"
#include "iALogger.h"
#include "iAModuleDispatcher.h"
#include "iARenderer.h"
#include "iASlicerData.h"
#include "mdichild.h"

#include <vtkCamera.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVersion.h>

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


class ConsoleLogger : public iALogger
{
public:
	void log(QString const & msg)
	{
		iAConsole::GetInstance().Log(msg.toStdString());
	}
};

ConsoleLogger GlobalConsoleLogger;

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
	actionLink_views->setChecked(ssLinkViews);//removed from readSettings, if is needed at all?
	actionLink_mdis->setChecked(ssLinkMDIs);
	setCentralWidget(mdiArea);

	windowMapper = new QSignalMapper(this);

	createRecentFileActions();
	connectSignalsToSlots();
	//setupToolBars();
	setupStatusBar();
	updateMenus();
	slicerToolsGroup = new QActionGroup(this);
	groupActions();

	actionDelete_point->setEnabled(false);
	actionChange_color->setEnabled(false);

	std::string git_version(m_gitVersion.toStdString());
	git_version = git_version.substr(0, git_version.length() - 9);
	std::replace( git_version.begin(), git_version.end(), '-', '.');
	splashScreen->showMessage(tr("\n      Version: %1").arg ( git_version.c_str() ), Qt::AlignTop, QColor(255, 255, 255));

	// define preset colors
	colors[0] = QColor(0, 0, 0);     colors[1] = QColor(0, 255, 0);   colors[2] = QColor(255, 0, 0); colors[3] = QColor(255, 255, 0);
	colors[4] = QColor(0, 255, 255); colors[5] = QColor(255, 0, 255); colors[6] = QColor(255, 255, 255);

	isStack=false;

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
	this->layoutToolbar->insertWidget(this->actionSave_Layout, layout);

	m_moduleDispatcher->InitializeModules(&GlobalConsoleLogger);
	SetModuleActionsEnabled( false );

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

	delete timer;
	timer = 0;
}


void MainWindow::timeout()
{
	splashScreen->finish(this);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
	mdiArea->closeAllSubWindows();
	if (activeMdiChild()) {
		event->ignore();
	} else {
		writeSettings();
		iAConsole::Close();
		event->accept();
	}
}


void MainWindow::newFile()
{
	MdiChild *child = createMdiChild();
	child->newFile();
	child->show();
}


void MainWindow::open()
{
	isStack=false;
	loadFiles(
		QFileDialog::getOpenFileNames(
			this,
			tr("Open Files"),
			path,
			iAIOProvider::GetSupportedLoadFormats()
		)
	);
}


void MainWindow::loadFiles(QStringList fileNames){

	statusBar()->showMessage(tr("Loading data..."), 5000);
	QString fileName;

	for (int i = 0; i < fileNames.length(); i++)
	{
		loadFileInternal(fileNames[i]);
	}
}


void MainWindow::openImageStack()
{
	isStack=false;
	loadFile(
		QFileDialog::getOpenFileName(
			this,
			tr("Open File"),
			path,
			tr(
				"All supported types (*.mhd *.mha *.tif *.png *.jpg *.bmp);;"
				"MetaImages (*.mhd *.mha);;"
				"TIFF stacks (*.tif);;"
				"PNG stacks (*.png);;"
				"BMP stacks (*.bmp);;"
				"JPEG stacks (*.jpg)"
			)
		)
	);
}


void MainWindow::openVolumeStack()
{
	isStack = true;
	loadFile(
		QFileDialog::getOpenFileName(
			this,
			tr("Open File"),
			path,
			tr(
				"All supported types (*.mhd *.raw *.volstack);;"
				"MetaImages (*.mhd *.mha);;"
				"RAW files (*.raw);;"
				"Volume Stack (*.volstack)"
			)
		)
	);
	isStack = false;
}


void MainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) loadFile(action->data().toString());
}

void MainWindow::loadFileInternal(QString fileName)
{
	if (!fileName.isEmpty()) {

		QString t; t = fileName; t.truncate(t.lastIndexOf('/'));
		path = t;

		QMdiSubWindow *existing = findMdiChild(fileName);
		if (existing) {
			mdiArea->setActiveSubWindow(existing);
			return;
		}
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
					MdiChild *child = createMdiChild();
					if (child->loadFile(fileName, false)) {
						child->show();
						child->showMaximized();
					} else {
						statusBar()->showMessage(tr("FILE LOADING FAILED!"), 10000);
						child->close();
					}
				}
				return;
			}
		}
		// Todo: hook for plugins?
		MdiChild *child = createMdiChild();
		if (child->loadFile(fileName, isStack)) {
			child->show();
			child->showMaximized();
		}
		else {
			statusBar()->showMessage(tr("FILE LOADING FAILED!"), 10000);
			child->close();
		}
	}
}

void MainWindow::loadFile(QString fileName)
{
	statusBar()->showMessage(tr("Loading data..."), 5000);
	loadFileInternal(fileName);
}


void MainWindow::save()
{
	if (activeMdiChild() && activeMdiChild()->save())
		statusBar()->showMessage(tr("File saved"), 5000);
}


void MainWindow::saveAs()
{
	if (activeMdiChild() && activeMdiChild()->saveAs())
		statusBar()->showMessage(tr("File saved"), 5000);
}

void MainWindow::saveAsImageStack()
{
	if (activeMdiChild() && activeMdiChild()->saveAsImageStack())
		statusBar()->showMessage(tr("Image stack saved"), 5000);
}


void MainWindow::saveScreen()
{
	QWidget *child = (QWidget*)activeMdiChild();
	if (child != NULL) {
		QString file = QFileDialog::getSaveFileName(this, tr("Save"), path, tr("PNG (*.png  )" ) );

		if (file.isEmpty()) return;

		QPixmap pixmap(child->size());
		child->render(&pixmap);

		pixmap.save(file.toLatin1(), "PNG", 100);
	} else
		QMessageBox::warning(this, tr("Screenshot"), tr("No window selected!"));
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

		dlg_commoninput dlg(this, "Save Settings", 7, inList, inPara, NULL);

		if (dlg.exec() == QDialog::Accepted)
		{
			dlg.getCheckValues()[0] == 0 ? spCamera = false               : spCamera = true;
			dlg.getCheckValues()[1] == 0 ? spSliceViews = false           : spSliceViews = true;
			dlg.getCheckValues()[2] == 0 ? spTransferFunction = false     : spTransferFunction = true;
			dlg.getCheckValues()[3] == 0 ? spProbabilityFunctions = false : spProbabilityFunctions = true;
			dlg.getCheckValues()[4] == 0 ? spPreferences = false          : spPreferences = true;
			dlg.getCheckValues()[5] == 0 ? spRenderSettings = false       : spRenderSettings = true;
			dlg.getCheckValues()[6] == 0 ? spSlicerSettings = false        : spSlicerSettings = true;

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

		dlg_commoninput dlg(this, "Load Settings", inList.size(), inList, inPara, NULL);

		if (dlg.exec() == QDialog::Accepted)
		{
			int index = 0;
			if (camera)               { dlg.getCheckValues()[index++] == 0 ? lpCamera = false               : lpCamera = true; }
			if (sliceViews)           { dlg.getCheckValues()[index++] == 0 ? lpSliceViews = false           : lpSliceViews = true; }
			if (transferFunction)     { dlg.getCheckValues()[index++] == 0 ? lpTransferFunction = false     : lpTransferFunction = true; }
			if (probabilityFunctions) { dlg.getCheckValues()[index++] == 0 ? lpProbabilityFunctions = false : lpProbabilityFunctions = true; }
			if (preferences)          { dlg.getCheckValues()[index++] == 0 ? lpPreferences = false          : lpPreferences = true; }
			if (renderSettings)       { dlg.getCheckValues()[index++] == 0 ? lpRenderSettings = false       : lpRenderSettings = true; }
			if (slicerSettings)       { dlg.getCheckValues()[index++] == 0 ? lpSlicerSettings = false       : lpSlicerSettings = true; }

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

	for (int i = 0; i < transferFunction->getOpacityFunction()->GetSize(); i++)
	{
		double opacityTFValue[4];
		double colorTFValue[6];
		transferFunction->getOpacityFunction()->GetNodeValue(i, opacityTFValue);
		transferFunction->getColorFunction()->GetNodeValue(i, colorTFValue);

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
			dlg_bezier *bezier = new dlg_bezier(activeMdiChild()->getHistogram(), colors[colorIndex % 7], false);
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
			dlg_gaussian *gaussian = new dlg_gaussian(activeMdiChild()->getHistogram(), colors[colorIndex % 7], false);

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
	preferencesElement.setAttribute("histogramBins", tr("%1").arg(prefHistogramBins));
	preferencesElement.setAttribute("statisticalExtent", tr("%1").arg(prefStatExt));
	preferencesElement.setAttribute("compression", tr("%1").arg(prefCompression));
	preferencesElement.setAttribute("resultsInNewWindow", tr("%1").arg(prefResultInNewWindow));
	preferencesElement.setAttribute("removePeaks", tr("%1").arg(prefMedianFilterFistogram));
	preferencesElement.setAttribute("magicLensSize", tr("%1").arg(prefMagicLensSize));
	preferencesElement.setAttribute("magicLensFrameWidth", tr("%1").arg(prefMagicLensFrameWidth));

	doc.documentElement().appendChild(preferencesElement);
}


void MainWindow::loadPreferences(QDomNode &preferencesNode)
{
	QDomNamedNodeMap attributes = preferencesNode.attributes();
	prefHistogramBins = attributes.namedItem("histogramBins").nodeValue().toInt();
	prefStatExt = attributes.namedItem("statisticalExtent").nodeValue().toDouble();
	prefCompression = attributes.namedItem("compression").nodeValue() == "1";
	prefResultInNewWindow = attributes.namedItem("resultsInNewWindow").nodeValue() == "1";
	prefMedianFilterFistogram = attributes.namedItem("removePeaks").nodeValue() == "1";
	prefMagicLensSize = attributes.namedItem("magicLensSize").nodeValue().toInt();
	prefMagicLensFrameWidth = attributes.namedItem("magicLensFrameWidth").nodeValue().toInt();

	activeMdiChild()->editPrefs(prefHistogramBins, prefMagicLensSize, prefMagicLensFrameWidth, prefStatExt, prefCompression, prefMedianFilterFistogram, prefResultInNewWindow, false);
}


void MainWindow::saveRenderSettings(QDomDocument &doc)
{
	// remove renderSettings node if there is one
	QDomNode node = doc.documentElement();
	removeNode(node, "renderSettings");

	// add new camera node
	QDomElement renderSettingsElement = doc.createElement("renderSettings");
	renderSettingsElement.setAttribute("showVolume", tr("%1").arg(rsShowVolume));
	renderSettingsElement.setAttribute("showSlicers", tr("%1").arg(rsShowSlicers));
	renderSettingsElement.setAttribute("showHelpers", tr("%1").arg(rsShowHelpers));
	renderSettingsElement.setAttribute("showRPosition", tr("%1").arg(rsShowRPosition));
	renderSettingsElement.setAttribute("linearInterpolation", tr("%1").arg(rsLinearInterpolation));
	renderSettingsElement.setAttribute("shading", tr("%1").arg(rsShading));
	renderSettingsElement.setAttribute("boundingBox", tr("%1").arg(rsBoundingBox));
	renderSettingsElement.setAttribute("parallelProjection", tr("%1").arg(rsParallelProjection));
	renderSettingsElement.setAttribute("imageSampleDistance", tr("%1").arg(rsImageSampleDistance));
	renderSettingsElement.setAttribute("sampleDistance", tr("%1").arg(rsSampleDistance));
	renderSettingsElement.setAttribute("ambientLighting", tr("%1").arg(rsAmbientLighting));
	renderSettingsElement.setAttribute("diffuseLighting", tr("%1").arg(rsDiffuseLighting));
	renderSettingsElement.setAttribute("specularLighting", tr("%1").arg(rsSpecularLighting));
	renderSettingsElement.setAttribute("specularPower", tr("%1").arg(rsSpecularPower));
	renderSettingsElement.setAttribute("backgroundTop", tr("%1").arg(rsBackgroundTop.toLatin1().constData()));
	renderSettingsElement.setAttribute("backgroundBottom", tr("%1").arg(rsBackgroundBottom.toLatin1().constData()));
	renderSettingsElement.setAttribute("renderMode", tr("%1").arg(rsRenderMode));

	doc.documentElement().appendChild(renderSettingsElement);
}


void MainWindow::loadRenderSettings(QDomNode &renderSettingsNode)
{
	QDomNamedNodeMap attributes = renderSettingsNode.attributes();

	rsShowVolume = attributes.namedItem("showVolume").nodeValue() == "1";
	rsShowSlicers = attributes.namedItem("showSlicers").nodeValue() == "1";
	rsShowHelpers = attributes.namedItem("showHelpers").nodeValue() == "1";
	rsShowRPosition = attributes.namedItem("showRPosition").nodeValue() == "1";
	rsLinearInterpolation = attributes.namedItem("linearInterpolation").nodeValue() == "1";
	rsShading = attributes.namedItem("shading").nodeValue() == "1";
	rsBoundingBox = attributes.namedItem("boundingBox").nodeValue() == "1";
	rsParallelProjection = attributes.namedItem("parallelProjection").nodeValue() == "1";

	rsImageSampleDistance = attributes.namedItem("imageSampleDistance").nodeValue().toDouble();
	rsSampleDistance = attributes.namedItem("sampleDistance").nodeValue().toDouble();
	rsAmbientLighting = attributes.namedItem("ambientLighting").nodeValue().toDouble();
	rsDiffuseLighting = attributes.namedItem("diffuseLighting").nodeValue().toDouble();
	rsSpecularLighting = attributes.namedItem("specularLighting").nodeValue().toDouble();
	rsSpecularPower = attributes.namedItem("specularPower").nodeValue().toDouble();
	rsBackgroundTop = attributes.namedItem("backgroundTop").nodeValue();
	rsBackgroundBottom = attributes.namedItem("backgroundBottom").nodeValue();
	rsRenderMode = attributes.namedItem("renderMode").nodeValue().toInt();

	activeMdiChild()->editRendererSettings(
		rsShowVolume, rsShowSlicers, rsShowHelpers, rsShowRPosition, rsLinearInterpolation, rsShading, rsBoundingBox,
		rsParallelProjection, rsImageSampleDistance, rsSampleDistance, rsAmbientLighting,
		rsDiffuseLighting, rsSpecularLighting, rsSpecularPower, rsBackgroundTop, rsBackgroundBottom, rsRenderMode);
}


void MainWindow::saveSlicerSettings(QDomDocument &doc)
{
	// remove slicerSettings node if there is one
	QDomNode node = doc.documentElement();
	removeNode(node, "slicerSettings");

	// add new camera node
	QDomElement slicerSettingsElement = doc.createElement("slicerSettings");
	slicerSettingsElement.setAttribute("linkViews", tr("%1").arg(ssLinkViews));
	slicerSettingsElement.setAttribute("showIsolines", tr("%1").arg(ssShowIsolines));
	slicerSettingsElement.setAttribute("showPosition", tr("%1").arg(ssShowPosition));

	slicerSettingsElement.setAttribute("numberOfIsolines", tr("%1").arg(ssNumberOfIsolines));
	slicerSettingsElement.setAttribute("minIsovalue", tr("%1").arg(ssMinIsovalue));
	slicerSettingsElement.setAttribute("maxIsovalue", tr("%1").arg(ssMaxIsovalue));
	slicerSettingsElement.setAttribute("linearInterpolation", tr("%1").arg(ssImageActorUseInterpolation));
	slicerSettingsElement.setAttribute("snakeSlices", tr("%1").arg(ssSnakeSlices));
	slicerSettingsElement.setAttribute("linkMDIs", tr("%1").arg(ssLinkMDIs));

	doc.documentElement().appendChild(slicerSettingsElement);
}


void MainWindow::loadSlicerSettings(QDomNode &slicerSettingsNode)
{
	QDomNamedNodeMap attributes = slicerSettingsNode.attributes();

	ssLinkViews = attributes.namedItem("linkViews").nodeValue() == "1";
	ssShowIsolines = attributes.namedItem("showIsolines").nodeValue() == "1";
	ssShowPosition = attributes.namedItem("showPosition").nodeValue() == "1";
	ssNumberOfIsolines = attributes.namedItem("numberOfIsolines").nodeValue().toInt();
	ssMinIsovalue = attributes.namedItem("minIsovalue").nodeValue().toDouble();
	ssMaxIsovalue = attributes.namedItem("maxIsovalue").nodeValue().toDouble();
	ssImageActorUseInterpolation = attributes.namedItem("linearInterpolation").nodeValue().toDouble();
	ssSnakeSlices = attributes.namedItem("snakeSlices").nodeValue().toDouble();
	ssLinkMDIs = attributes.namedItem("linkMDIs").nodeValue() == "1";

	activeMdiChild()->editSlicerSettings(ssLinkViews, ssShowIsolines, ssShowPosition, ssNumberOfIsolines, ssMinIsovalue, ssMaxIsovalue, ssImageActorUseInterpolation, ssSnakeSlices, ssLinkMDIs);
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
	QList<QMdiSubWindow *> mdiwindows = MdiChildList();

	for (int i = 0; i < mdiwindows.size(); ++i)
	{
		windowTitles.append(mdiwindows.at(i)->widget()->windowTitle());
	}
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
		ssLinkViews = actionLink_views->isChecked();
		activeMdiChild()->linkViews(ssLinkViews);

		if (ssLinkViews)
			statusBar()->showMessage(tr("Link Views"), 5000);
	}
}

void MainWindow::linkMDIs()
{
	if (activeMdiChild())
	{
		ssLinkMDIs = actionLink_mdis->isChecked();
		activeMdiChild()->linkMDIs(ssLinkMDIs);

		if (ssLinkViews)
			statusBar()->showMessage(tr("Link MDIs"), 5000);
	}
}


void MainWindow::enableInteraction()
{
	if (activeMdiChild())
	{
		ssInteractionEnabled = actionEnableInteraction->isChecked();
		activeMdiChild()->enableInteraction(ssInteractionEnabled);

		if (ssInteractionEnabled)
			statusBar()->showMessage(tr("Interaction Enabled"), 5000);
		else
			statusBar()->showMessage(tr("Interaction Disabled"), 5000);
	}
}


void MainWindow::prefs()
{
	/////////////////////////////
	// # -> switch on line edit
	// $ -> switch on check box
	// + -> switch on combo box
	/////////////////////////////
	MdiChild *child = activeMdiChild();

	QStringList inList = (QStringList() << tr("#Histogram Bins")
		<< tr("#Statistical extent")
		<< tr("$Compression")
		<< tr("$Results in new window")
		<< tr("$Remove Peaks from Histogram"))
		<< tr("+Looks")
		<< tr("#Magic lens frame width");
	QStringList looks;
	//if(qssName == ":/dark.qss")
	//	looks = ( QStringList() <<  tr("Dark") <<  tr("Bright") );
	//else
	//	looks = ( QStringList() <<  tr("Bright") <<  tr("Dark") );
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
	QList<QVariant> inPara; 	inPara << tr("%1").arg(child->getNumberOfHistogramBins())
		<< tr("%1").arg(child->getStatExtent())
		<< (child->getCompression() ? tr("true") : tr("false"))
		<< (child->getResultInNewWindow() ? tr("true") : tr("false"))
		<< (child->getMedianFilterHistogram() ? tr("true") : tr("false"))
		<< looks
		<< tr("%1").arg(child->GetMagicLensFrameWidth());

	dlg_commoninput dlg(this, "Preferences", 7, inList, inPara, NULL);

	if (dlg.exec() == QDialog::Accepted)
	{
		prefHistogramBins = (int)dlg.getValues()[0];
		prefStatExt = (int)dlg.getValues()[1];
		dlg.getCheckValues()[2] == 0 ? prefCompression = false : prefCompression = true;
		dlg.getCheckValues()[3] == 0 ? prefResultInNewWindow = false : prefResultInNewWindow = true;
		dlg.getCheckValues()[4] == 0 ? prefMedianFilterFistogram = false : prefMedianFilterFistogram = true;

		QString looksStr = dlg.getComboBoxValues()[5];
		qssName = styleNames[looksStr];
		applyQSS();

		prefMagicLensFrameWidth = dlg.getValues()[6];
		prefMagicLensSize = child->GetMagicLensSize();

		if (activeMdiChild() && activeMdiChild()->editPrefs(prefHistogramBins, prefMagicLensSize, prefMagicLensFrameWidth, prefStatExt, prefCompression, prefMedianFilterFistogram, prefResultInNewWindow, false))
			statusBar()->showMessage(tr("Edit preferences"), 5000);
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
	//renderModes.insert(vtkSmartVolumeMapper::RayCastAndTextureRenderMode, tr("RayCastAndTextureRenderMode")); -> deprecated
	//renderModes.insert(vtkSmartVolumeMapper::TextureRenderMode, tr("TextureRenderMode")); -> deprecated
	renderModes.insert(vtkSmartVolumeMapper::GPURenderMode, tr("GPURenderMode"));

	int currentRenderMode = child->GetRenderMode();

	QStringList renderTypes;
	for (int mode : renderModes.keys())
	{
		renderTypes << ((mode == currentRenderMode) ? QString("!") : QString()) + renderModes[mode];
	}

	QStringList inList;
	inList 
		<< tr("$Show volume")
		<< tr("$Show slicers")
		<< tr("$Show helpers")
		<< tr("$Show position")
		<< tr("$Linear interpolation")
		<< tr("$Shading")
		<< tr("$Bounding box")
		<< tr("$Parallel projection")
		<< tr("#Image sample distance")
		<< tr("#Sample distance")
		<< tr("#Ambient lighting")
		<< tr("#Diffuse lighting")
		<< tr("#Specular lighting")
		<< tr("#Specular power")
		<< tr("#Background top")
		<< tr("#Background bottom")
		<< tr("+Renderer Type");
	QList<QVariant> inPara;
	inPara << (child->getShowVolume() ? t : f)
		<< (child->getShowSlicers() ? t : f)
		<< (child->getShowHelpers() ? t : f)
		<< (child->getShowRPosition() ? t : f)
		<< (child->getLinearInterpolation() ? t : f)
		<< (child->getShading() ? t : f)
		<< (child->getBoundingBox() ? t : f)
		<< (child->getParallelProjection() ? t : f)

		<< tr("%1").arg(child->getImageSampleDistance())
		<< tr("%1").arg(child->getSampleDistance())
		<< tr("%1").arg(child->getAmbientLighting())
		<< tr("%1").arg(child->getDiffuseLighting())
		<< tr("%1").arg(child->getSpecularLighting())
		<< tr("%1").arg(child->getSpecularPower())
		<< tr("%1").arg(child->getBackgroundTop())
		<< tr("%1").arg(child->getBackgroundBottom())
		<< renderTypes;

	dlg_commoninput dlg(this, "Renderer settings", 17, inList, inPara, NULL);

	if (dlg.exec() == QDialog::Accepted)
	{
		dlg.getCheckValues()[0] == 0 ? rsShowVolume = false : rsShowVolume = true;
		dlg.getCheckValues()[1] == 0 ? rsShowSlicers = false : rsShowSlicers = true;
		dlg.getCheckValues()[2] == 0 ? rsShowHelpers = false : rsShowHelpers = true;
		dlg.getCheckValues()[3] == 0 ? rsShowRPosition = false : rsShowRPosition = true;
		dlg.getCheckValues()[4] == 0 ? rsLinearInterpolation = false : rsLinearInterpolation = true;
		dlg.getCheckValues()[5] == 0 ? rsShading = false : rsShading = true;
		dlg.getCheckValues()[6] == 0 ? rsBoundingBox = false : rsBoundingBox = true;
		dlg.getCheckValues()[7] == 0 ? rsParallelProjection = false : rsParallelProjection = true;

		rsImageSampleDistance = dlg.getValues()[8];
		rsSampleDistance = dlg.getValues()[9];
		rsAmbientLighting = dlg.getValues()[10];
		rsDiffuseLighting = dlg.getValues()[11];
		rsSpecularLighting = dlg.getValues()[12];
		rsSpecularPower = dlg.getValues()[13];
		rsBackgroundTop = dlg.getText()[14];
		rsBackgroundBottom = dlg.getText()[15];

		QString renderType = dlg.getComboBoxValues()[16];

		// TODO: use renderModes / reverse mapping ?
		rsRenderMode = vtkSmartVolumeMapper::DefaultRenderMode;
		if (renderType == tr("DefaultRenderMode"))
		{
			rsRenderMode = vtkSmartVolumeMapper::DefaultRenderMode;
		}
		else if (renderType == tr("RayCastAndTextureRenderMode"))
		{
			rsRenderMode = vtkSmartVolumeMapper::RayCastAndTextureRenderMode;
		}
		else if (renderType == tr("RayCastRenderMode"))
		{
			rsRenderMode = vtkSmartVolumeMapper::RayCastRenderMode;
		}
		else if (renderType == tr("TextureRenderMode"))
		{
			rsRenderMode = vtkSmartVolumeMapper::TextureRenderMode;
		}
		else if (renderType == tr("GPURenderMode"))
		{
			rsRenderMode = vtkSmartVolumeMapper::GPURenderMode;
		}

		if (activeMdiChild() && activeMdiChild()->editRendererSettings(
			rsShowVolume, rsShowSlicers, rsShowHelpers, rsShowRPosition, rsLinearInterpolation, rsShading, rsBoundingBox,
			rsParallelProjection, rsImageSampleDistance, rsSampleDistance, rsAmbientLighting,
			rsDiffuseLighting, rsSpecularLighting, rsSpecularPower, rsBackgroundTop, rsBackgroundBottom, rsRenderMode))
		{
			statusBar()->showMessage(tr("Changed renderer settings"), 5000);
		}
	}
}


void MainWindow::slicerSettings()
{
	MdiChild *child = activeMdiChild();

	QStringList inList = (QStringList() << tr("$Link Views")
		<< tr("$Show Position")
		<< tr("$Show Isolines")
		<< tr("$Linear Interpolation")
		<< tr("#Number of Isolines")
		<< tr("#Min Isovalue")
		<< tr("#Max Isovalue")
		<< tr("#Snake Slices")
		<< tr("$Link MDIs"));
	QList<QVariant> inPara; 	inPara  << (child->getLinkedViews() ? tr("true") : tr("false"))
		<< (child->getShowPosition() ? tr("true") : tr("false"))
		<< (child->getShowIsolines() ? tr("true") : tr("false"))
		<< (child->getImageActorUseInterpolation() ? tr("true") : tr("false"))
		<< tr("%1").arg(child->getNumberOfIsolines())
		<< tr("%1").arg(child->getMinIsovalue())
		<< tr("%1").arg(child->getMaxIsovalue())
		<< tr("%1").arg(child->getSnakeSlices())
		<< (child->getLinkedMDIs() ? tr("true") : tr("false"));

	dlg_commoninput *dlg = new dlg_commoninput (this, "Slicer settings", 9, inList, inPara, NULL);

	if (dlg->exec() == QDialog::Accepted)
	{
		dlg->getCheckValues()[0] == 0 ? ssLinkViews = false : ssLinkViews = true;
		dlg->getCheckValues()[1] == 0 ? ssShowPosition = false : ssShowPosition = true;
		dlg->getCheckValues()[2] == 0 ? ssShowIsolines = false : ssShowIsolines = true;
		dlg->getCheckValues()[3] == 0 ? ssImageActorUseInterpolation = false : ssImageActorUseInterpolation = true;
		ssNumberOfIsolines = dlg->getValues()[4];
		ssMinIsovalue = dlg->getValues()[5];
		ssMaxIsovalue = dlg->getValues()[6];
		ssSnakeSlices = dlg->getValues()[7];
		dlg->getCheckValues()[9] == 0 ? ssLinkMDIs = false : ssLinkMDIs = true;

		if (activeMdiChild() && activeMdiChild()->editSlicerSettings(ssLinkViews, ssShowIsolines, ssShowPosition, ssNumberOfIsolines, ssMinIsovalue, ssMaxIsovalue, ssImageActorUseInterpolation, ssSnakeSlices, ssLinkMDIs))
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
	QList<QMdiSubWindow *> mdiwindows = MdiChildList();
	int sizeMdi = mdiwindows.size();
	if (sizeMdi > 1)
	{
		double camOptions[10] = {0};
		if (activeMdiChild())  activeMdiChild()->getCamPosition(camOptions);
		for(int i = 0; i < sizeMdi; i++)
		{
			MdiChild *tmpChild = qobject_cast<MdiChild *>(mdiwindows.at(i)->widget());

			// check dimension and spacing here, if not the same with active mdichild, skip.
			tmpChild->setCamPosition(camOptions, rsParallelProjection);
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


MdiChild * MainWindow::GetResultChild( QString const & title )
{
	MdiChild *child = activeMdiChild();

	QString filename =  child->currentFile();

	if (child->getResultInNewWindow())
	{
		vtkImageData* imageData = activeMdiChild()->getImageData();
		std::vector<dlg_function*> activeChildFunctions = activeMdiChild()->getFunctions();
		child = createMdiChild();
		//child->setCurrentFile(filename);
		child->show();
		child->displayResult(title, imageData );

		for ( unsigned int i = 1; i < activeChildFunctions.size(); ++i )
		{
			dlg_function *curFunc = activeChildFunctions[i];

			switch(curFunc->getType())
			{
			case dlg_function::GAUSSIAN:
				{
					dlg_gaussian * oldGaussian = (dlg_gaussian*)curFunc;
					dlg_gaussian * newGaussian = new dlg_gaussian( child->getHistogram(), getColors()[i%7] );

					newGaussian->setMean(oldGaussian->getMean());
					newGaussian->setMultiplier(oldGaussian->getMultiplier());
					newGaussian->setSigma(oldGaussian->getSigma());

					child->getFunctions().push_back(newGaussian);
				}
				break;
			case dlg_function::BEZIER:
				{
					dlg_bezier * oldBezier = (dlg_bezier*)curFunc;
					dlg_bezier * newBezier = new dlg_bezier( child->getHistogram(), getColors()[i%7] );

					for( unsigned int j=0; j<oldBezier->getPoints().size(); ++j )
						newBezier->addPoint(oldBezier->getPoints()[j].x(), oldBezier->getPoints()[j].y());

					child->getFunctions().push_back(newBezier);
				}
				break;
			default:
				// unknown function type, do nothing
				break;
			}
		}
	}

	return child;
}


MdiChild * MainWindow::GetResultChild( int childInd, QString const & f )
{
	QList<QMdiSubWindow *> mdiwindows = MdiChildList();
	MdiChild *child = qobject_cast<MdiChild *>(mdiwindows.at(childInd)->widget());
	QString filename =  child->currentFile();

	if (child->getResultInNewWindow())
	{
		vtkImageData* imageData = child->getImageData();
		child = createMdiChild();
		//child->setCurrentFile(filename);
		child->show();
		child->displayResult(f, imageData );
	}

	return child;
}


double MainWindow::neighborhood(vtkImageData *imageData, int x0, int y0, int z0)
{
	int extents[6];
	imageData->GetExtent(extents);

	int startX = x0-1;
	int startY = y0-1;
	int startZ = z0-1;
	int endX = x0+1;
	int endY = y0+1;
	int endZ = z0+1;

	if (startX < extents[0]) startX = extents[0];
	if (startY < extents[2]) startY = extents[2];
	if (startZ < extents[4]) startZ = extents[4];
	if (endX > extents[1]) endX = extents[1];
	if (endY > extents[3]) endY = extents[3];
	if (endZ > extents[5]) endZ = extents[5];

	double n = 0;

	for (int x = startX; x <= endX; x++)
		for (int y = startY; y <= endY; y++)
			for (int z = startZ; z <= endZ; z++)
			{
				double value = imageData->GetScalarComponentAsDouble(x,y,z,0);
				if (1.0-value < value)
					n += 1.0-value;
				else
					n += value;
			}

			return n;
}


void MainWindow::about()
{
	splashScreen->show();
	string git_version(m_gitVersion.toStdString());
	git_version = git_version.substr(0, git_version.length() - 9);
	std::replace( git_version.begin(), git_version.end(), '-', '.');
	splashScreen->showMessage(tr("\n      Version: %1").arg ( git_version.c_str() ), Qt::AlignTop, QColor(255, 255, 255));
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
	saveScreenAct->setEnabled(hasMdiChild);
	loadSettingsAct->setEnabled(hasMdiChild);
	saveSettingsAct->setEnabled(hasMdiChild);
	closeAct->setEnabled(hasMdiChild);
	closeAllAct->setEnabled(hasMdiChild);
	tileAct->setEnabled(hasMdiChild);
	cascadeAct->setEnabled(hasMdiChild);
	nextAct->setEnabled(hasMdiChild);
	previousAct->setEnabled(hasMdiChild);

	xyAct->setEnabled(hasMdiChild);
	xzAct->setEnabled(hasMdiChild);
	yzAct->setEnabled(hasMdiChild);
	rcAct->setEnabled(hasMdiChild);
	multiAct->setEnabled(hasMdiChild);
	tabAct->setEnabled(hasMdiChild);
	actionLink_views->setEnabled(hasMdiChild);
	actionLink_mdis->setEnabled(hasMdiChild);
	actionEnableInteraction->setEnabled(hasMdiChild);
	actionPreferences->setEnabled(hasMdiChild);
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
	QList<QMdiSubWindow *> windows = MdiChildList();

	for (int i = 0; i < windows.size(); ++i) {
		MdiChild *child = qobject_cast<MdiChild *>(windows.at(i)->widget());

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


MdiChild* MainWindow::createMdiChild()
{
	MdiChild *child = new MdiChild(this);
	mdiArea->addSubWindow(child);

	child->setupRaycaster( rsShowVolume, rsShowSlicers, rsShowHelpers, rsShowRPosition,
		rsLinearInterpolation, rsShading, rsBoundingBox, rsParallelProjection,
		rsImageSampleDistance, rsSampleDistance, rsAmbientLighting, rsDiffuseLighting,
		rsSpecularLighting, rsSpecularPower, rsBackgroundTop, rsBackgroundBottom,
		rsRenderMode, true );
	child->setupSlicers( ssLinkViews, ssShowIsolines, ssShowPosition, ssNumberOfIsolines, ssMinIsovalue, ssMaxIsovalue, ssImageActorUseInterpolation, ssSnakeSlices);
	child->editPrefs(prefHistogramBins, prefMagicLensSize, prefMagicLensFrameWidth, prefStatExt, prefCompression, prefMedianFilterFistogram, prefResultInNewWindow, true);

	connect( child, SIGNAL( pointSelected() ), this, SLOT( pointSelected() ) );
	connect( child, SIGNAL( noPointSelected() ), this, SLOT( noPointSelected() ) );
	connect( child, SIGNAL( endPointSelected() ), this, SLOT( endPointSelected() ) );
	connect( child, SIGNAL( active() ), this, SLOT( setHistogramFocus() ) );
	connect( child, SIGNAL( autoUpdateChanged( bool ) ), actionUpdate_automatically, SLOT( setChecked( bool ) ) );
	connect( child, SIGNAL( currentChanged( int ) ), this, SLOT( tabChanged( int ) ) );
	connect( child, SIGNAL( closed() ), this, SLOT( childClosed() ) );

	SetModuleActionsEnabled( true );

	m_moduleDispatcher->ChildCreated(child);
	return child;
}


void MainWindow::connectSignalsToSlots()
{
	connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));
	connect(openAct, SIGNAL(triggered()), this, SLOT(open()));
	connect(actionOpen_Image_Stack, SIGNAL(triggered()), this, SLOT(openImageStack()));
	connect(actionOpen_Volume_Stack, SIGNAL(triggered()), this, SLOT(openVolumeStack()));
	connect(actionOpen_With_DataTypeConversion, SIGNAL(triggered()), this, SLOT(OpenWithDataTypeConversion()));
	connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));
	connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(actionSave_Image_Stack, SIGNAL(triggered()), this, SLOT(saveAsImageStack()));
	connect(saveScreenAct, SIGNAL(triggered()), this, SLOT(saveScreen()));
	connect(loadSettingsAct, SIGNAL(triggered()), this, SLOT(loadSettings()));
	connect(saveSettingsAct, SIGNAL(triggered()), this, SLOT(saveSettings()));
	connect(switchAct, SIGNAL(triggered()), this, SLOT(switchLayoutDirection()));
	connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
	connect(closeAct, SIGNAL(triggered()), mdiArea, SLOT(closeActiveSubWindow()));
	connect(closeAllAct, SIGNAL(triggered()), mdiArea, SLOT(closeAllSubWindows()));
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

	connect(action_MainWindowStatusBar, SIGNAL(triggered()), this, SLOT(ToggleMainWindowStatusBar()));
	connect(action_ChildStatusBar, SIGNAL(triggered()), this, SLOT(ToggleChildStatusBar()));
	
	connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(childActivatedSlot(QMdiSubWindow*)));
	for (int i = 0; i < MaxRecentFiles; ++i) {
		connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}
}


void MainWindow::setupStatusBar()
{
	//statusBar()->addWidget(progress);
	statusBar()->showMessage(tr("Ready"));

	//stateLabel = new QLabel(" ready ", this);
	//stateLabel->setAlignment(Qt::AlignLeft);
	//stateLabel->setMinimumSize(locationLabel->sizeHint());

	//modLabel = new QLabel(" MOD ", this);
	//modLabel->setAlignment(Qt::AlignHCenter);
	//modLabel->setMinimumSize(modLabel->sizeHint());

	//statusBar()->addWidget(locationLabel);
	//statusBar()->addWidget(stateLabel,1);
	//statusBar()->addWidget(modLabel);
}


void MainWindow::readSettings()
{
	QSettings settings;
	path = settings.value("Path").toString();

	qssName = settings.value("qssName", ":/dark.qss").toString();

	defaultLayout = settings.value("Preferences/defaultLayout", "").toString();
	prefHistogramBins = settings.value("Preferences/prefHistogramBins", 2048).toInt();
	prefStatExt = settings.value("Preferences/prefStatExt", 3).toInt();
	prefCompression = settings.value("Preferences/prefCompression", true).toBool();
	prefResultInNewWindow = settings.value("Preferences/prefResultInNewWindow", true).toBool();
	prefMedianFilterFistogram = settings.value("Preferences/prefMedianFilterFistogram", true).toBool();
	prefMagicLensFrameWidth = settings.value("Preferences/prefMagicLensFrameWidth", 3).toInt();

	rsShowVolume = settings.value("Renderer/rsShowVolume", true).toBool();;
	rsShowSlicers = settings.value("Renderer/rsShowSlicers", false).toBool();
	rsShowHelpers = settings.value("Renderer/rsShowHelpers", true).toBool();
	rsShowRPosition = settings.value("Renderer/rsShowRPosition", true).toBool();
	rsLinearInterpolation = settings.value("Renderer/rsLinearInterpolation", true).toBool();
	rsShading = settings.value("Renderer/rsShading", true).toBool();
	rsBoundingBox = settings.value("Renderer/rsBoundingBox", true).toBool();
	rsParallelProjection = settings.value("Renderer/rsParallelProjection", false).toBool();
	rsImageSampleDistance = settings.value("Renderer/rsImageSampleDistance", 1).toDouble();
	rsSampleDistance = settings.value("Renderer/rsSampleDistance", 1).toDouble();
	rsAmbientLighting = settings.value("Renderer/rsAmbientLighting", 0.2).toDouble();
	rsDiffuseLighting = settings.value("Renderer/rsDiffuseLighting", 0.5).toDouble();
	rsSpecularLighting = settings.value("Renderer/rsSpecularLighting", 0.7).toDouble();
	rsSpecularPower = settings.value("Renderer/rsSpecularPower", 10).toDouble();
	rsBackgroundTop = settings.value("Renderer/rsBackgroundTop", "7FAAFF").toString();
	rsBackgroundBottom = settings.value("Renderer/rsBackgroundBottom", "FFFFFF").toString();
	rsRenderMode = settings.value("Renderer/rsRenderMode", 0).toInt();

	ssLinkViews = settings.value("Slicer/ssLinkViews", false).toBool();
	ssShowPosition = settings.value("Slicer/ssShowPosition", true).toBool();
	ssShowIsolines = settings.value("Slicer/ssShowIsolines", false).toBool();
	ssNumberOfIsolines = settings.value("Slicer/ssNumberOfIsolines", 5).toDouble();
	ssMinIsovalue = settings.value("Slicer/ssMinIsovalue", 20000).toDouble();
	ssMaxIsovalue = settings.value("Slicer/ssMaxIsovalue", 40000).toDouble();
	ssImageActorUseInterpolation = settings.value("Slicer/ssImageActorUseInterpolation", true).toBool();
	ssSnakeSlices = settings.value("Slicer/ssSnakeSlices", 100).toInt();
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

	dtcmin = settings.value("Filters/DataTypeConversion/dtcmin").toFloat();
	dtcmax = settings.value("Filters/DataTypeConversion/dtcmax").toFloat();
	dtcoutmin = settings.value("Filters/DataTypeConversion/dtcoutmin").toDouble();
	dtcoutmax = settings.value("Filters/DataTypeConversion/dtcoutmax").toDouble();
	dtcdov = settings.value("Filters/DataTypeConversion/dtcdov").toInt();

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
	settings.setValue("Preferences/prefHistogramBins", prefHistogramBins);
	settings.setValue("Preferences/prefStatExt", prefStatExt);
	settings.setValue("Preferences/prefCompression", prefCompression);
	settings.setValue("Preferences/prefResultInNewWindow", prefResultInNewWindow);
	settings.setValue("Preferences/prefMedianFilterFistogram", prefMedianFilterFistogram);
	settings.setValue("Preferences/prefMagicLensSize", prefMagicLensSize);
	settings.setValue("Preferences/prefMagicLensFrameWidth", prefMagicLensFrameWidth);

	settings.setValue("Renderer/rsShowVolume", rsShowVolume);
	settings.setValue("Renderer/rsShowSlicers", rsShowSlicers);
	settings.setValue("Renderer/rsLinearInterpolation", rsLinearInterpolation);
	settings.setValue("Renderer/rsShading", rsShading);
	settings.setValue("Renderer/rsBoundingBox", rsBoundingBox);
	settings.setValue("Renderer/rsParallelProjection", rsParallelProjection);
	settings.setValue("Renderer/rsShowHelpers", rsShowHelpers);
	settings.setValue("Renderer/rsShowRPosition", rsShowRPosition);
	settings.setValue("Renderer/rsImageSampleDistance", rsImageSampleDistance);
	settings.setValue("Renderer/rsSampleDistance", rsSampleDistance);
	settings.setValue("Renderer/rsAmbientLighting", rsAmbientLighting);
	settings.setValue("Renderer/rsDiffuseLighting", rsDiffuseLighting);
	settings.setValue("Renderer/rsSpecularLighting", rsSpecularLighting);
	settings.setValue("Renderer/rsSpecularPower", rsSpecularPower);
	settings.setValue("Renderer/rsBackgroundTop", rsBackgroundTop);
	settings.setValue("Renderer/rsBackgroundBottom", rsBackgroundBottom);
	settings.setValue("Renderer/rsRenderMode", rsRenderMode);

	settings.setValue("Slicer/ssLinkViews", ssLinkViews);
	settings.setValue("Slicer/ssShowPosition", ssShowPosition);
	settings.setValue("Slicer/ssShowIsolines", ssShowIsolines);
	settings.setValue("Slicer/ssNumberOfIsolines", ssNumberOfIsolines);
	settings.setValue("Slicer/ssMinIsovalue", ssMinIsovalue);
	settings.setValue("Slicer/ssMaxIsovalue", ssMaxIsovalue);
	settings.setValue("Slicer/ssImageActorUseInterpolation", ssImageActorUseInterpolation);
	settings.setValue("Slicer/ssSnakeSlices", ssSnakeSlices);

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
	curFile = fileName;
	if (curFile.isEmpty())
		setWindowTitle(tr("Recent Files"));
	else
		setWindowTitle(tr("%1 - %2").arg(strippedName(curFile))
		.arg(tr("Recent Files")));

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
		return qobject_cast<MdiChild *>(MdiChildList(QMdiArea::ActivationHistoryOrder).last()->widget());
	}

	return 0;
}


QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

	foreach (QMdiSubWindow *window, MdiChildList()) {
		MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
		if (mdiChild->currentFile() == canonicalFilePath)
			return window;
	}
	return 0;
}


void MainWindow::switchLayoutDirection()
{
	if (layoutDirection() == Qt::LeftToRight)
		qApp->setLayoutDirection(Qt::RightToLeft);
	else
		qApp->setLayoutDirection(Qt::LeftToRight);
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


void MainWindow::tabChanged(int index)
{
	histogramToolbar->setEnabled(index == 1);
}


QString MainWindow::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}


QList<QMdiSubWindow*> MainWindow::MdiChildList(QMdiArea::WindowOrder order)
{
	QList<QMdiSubWindow*> res;

	foreach(QMdiSubWindow *window, mdiArea->subWindowList(order))
	{
		MdiChild * child = qobject_cast<MdiChild*>(window->widget());
		if (child)
			res.append(window);
	}
	return res;
}


int MainWindow::SelectInputs( QString winTitel, int n, QStringList inList, int * out_inputIndxs, bool modal /*= true*/ )
{
	QList<QMdiSubWindow *> mdiwindows = MdiChildList();
	//check if sufficient number of datasets are opened
	if (mdiwindows.size() == 0)
	{
		QMessageBox::warning(this, winTitel, tr("At least one dataset should be opened. Please open the dataset."));
		return QDialog::Rejected;
	}
	QList<QVariant> inPara;
	QStringList mdiChildrenNames;
	for (int childInd=0; childInd<mdiwindows.size(); childInd++)
		mdiChildrenNames << mdiwindows.at(childInd)->windowTitle();
	for (int inputInd = 0; inputInd<n; inputInd++)
		inPara << mdiChildrenNames;

	dlg_commoninput inputs(this, winTitel + ": Inputs Selection", n, inList, inPara, NULL);
	if( QDialog::Accepted == inputs.exec() )
	{
		for (int inputInd = 0; inputInd<n; inputInd++)
			out_inputIndxs[inputInd] = inputs.getComboBoxIndices()[inputInd];
		return QDialog::Accepted;
	}

	return QDialog::Rejected;
}


void MainWindow::groupActions()
{
	slicerToolsGroup->setExclusive(false);
	slicerToolsGroup->addAction(actionSnake_Slicer);
	slicerToolsGroup->addAction(actionRawProfile);
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


void MainWindow::OpenWithDataTypeConversion()
{
	// Create an image data
	vtkImageData* imageData = vtkImageData::New();
	QString finalfilename;
	QString testfinalfilename;

	QString file = QFileDialog::getOpenFileName(this,tr("Open File"),path,
		tr("All supported types (*.mhd *.mha *.stl *.vgi *.raw *.rec *.vol *.pro *.pars);;"
			"MetaImages (*.mhd *.mha);;"
			"STL files (*.stl);;"
			"VGI files (*.vgi);;"
			"RAW files (*.raw);;"
			"REC files (*.rec);;"
			"VOL files (*.vol);;"
			"PRO files (*.pro);;"
			"PARS files (*.pars);;"));

	QStringList datatype = (QStringList()
		<<  tr("VTK_SIGNED_CHAR") <<  tr("VTK_UNSIGNED_CHAR")
		<<  tr("VTK_SHORT")	      <<  tr("VTK_UNSIGNED_SHORT")
		<<  tr("VTK_INT")         <<  tr("VTK_UNSIGNED_INT")
		<<  tr("VTK_FLOAT")
		<<  tr("VTK_DOUBLE") );

	QStringList inList = (QStringList()
		<< tr("+Data Type")
		<< tr("#Slice sample rate")
		<< tr("# Dim X")
		<< tr("# Dim Y")
		<< tr("# Dim Z")
		<< tr("# Space X")
		<< tr("# Space Y")
		<< tr("# Space Z"));
	QList<QVariant> inPara;
	inPara << datatype
		<< tr("%1").arg(owdtcs)
		<< tr("%1").arg(owdtcx)
		<< tr("%1").arg(owdtcy)
		<< tr("%1").arg(owdtcz)
		<< tr("%1").arg(owdtcsx)
		<< tr("%1").arg(owdtcsy)
		<< tr("%1").arg(owdtcsz);

	dlg_commoninput dlg(this, "Open With DataType Conversion", 8, inList, inPara, NULL);

	if (dlg.exec() == QDialog::Accepted)
	{
		owdtcs = dlg.getValues()[1];
		owdtcx = dlg.getValues()[2];	owdtcy = dlg.getValues()[3]; owdtcz = dlg.getValues()[4];
		owdtcsx = dlg.getValues()[5]; owdtcsy = dlg.getValues()[6];	owdtcsz = dlg.getValues()[7];

		QString owdtcintype = dlg.getComboBoxValues()[0];

		double para[8];
		para[0] = dlg.getValues()[1];
		para[1] = dlg.getValues()[2];	para[2] = dlg.getValues()[3]; para[3] = dlg.getValues()[4];
		para[4] = dlg.getValues()[5]; para[5] = dlg.getValues()[6];	para[6] = dlg.getValues()[7];
		para[7] = this->getPrefHistoBinCnt();

		QSize qwinsize = this->size();
		double winsize[2];
		winsize[0] = qwinsize.width();	winsize[1] = qwinsize.height();

		double inPara[11];
		inPara[0] = owdtcmin;	inPara[1] = owdtcmax; inPara[2] = owdtcoutmin; inPara[3] = owdtcoutmax; inPara[4] = owdtcdov; inPara[5] = owdtcxori; inPara[6] = owdtcxsize;
		inPara[7] = owdtcyori; inPara[8] = owdtcysize; inPara[9] = owdtczori; inPara[10] = owdtczsize;
		dlg_datatypeconversion* conversionwidget = new dlg_datatypeconversion( this, imageData, file.toStdString().c_str(), owdtcintype.toStdString().c_str(), para, winsize, inPara );

		if(conversionwidget->exec() == QDialog::Accepted)
		{
			string owdtctype = conversionwidget->getcombobox();
			owdtcmin = conversionwidget->getlabelWidget1(); owdtcmax = conversionwidget->getlabelWidget2();
			owdtcoutmin = conversionwidget->getlabelWidget3(); owdtcoutmax = conversionwidget->getlabelWidget4();
			owdtcdov = conversionwidget->getcheckbox1();
			owdtcxori = conversionwidget->getlabelWidget6();	owdtcxsize = conversionwidget->getlabelWidget7();
			owdtcyori = conversionwidget->getlabelWidget8();	owdtcysize = conversionwidget->getlabelWidget9();
			owdtczori = conversionwidget->getlabelWidget10(); owdtczsize = conversionwidget->getlabelWidget11();

			double roi[6];
			roi[0] = conversionwidget->getlabelWidget6();	roi[1] = conversionwidget->getlabelWidget7();
			roi[2] = conversionwidget->getlabelWidget8();	roi[3] = conversionwidget->getlabelWidget9();
			roi[4] = conversionwidget->getlabelWidget10(); roi[5] = conversionwidget->getlabelWidget11();

			if ( owdtcdov == 0 )
			{
				testfinalfilename = conversionwidget->coreconversionfunction(file, finalfilename, para, owdtcintype.toStdString(), owdtctype, owdtcmin, owdtcmax, owdtcoutmin, owdtcoutmax, owdtcdov  );
			}
			else
			{
				testfinalfilename = conversionwidget->coreconversionfunctionforroi(file, finalfilename, para, owdtcintype.toStdString(), owdtctype, owdtcmin, owdtcmax, owdtcoutmin, owdtcoutmax, owdtcdov, roi  );
			}
		}
	}

	loadFile(testfinalfilename);
	//int test1 = 0;
}

void MainWindow::applyQSS()
{
	// Load an application style
	QFile styleFile(qssName);
	if (styleFile.open( QFile::ReadOnly ))
	{
		//QSS does not allow styling QMdiArea >_<, so we do it manually
// 		if(qssName == ":/bright.qss")
// 			mdiArea->setBackground(QColor(150,150,150));
// 		if(qssName == ":/dark.qss")
// 			mdiArea->setBackground(QColor(15,15,15));
		QTextStream styleIn(&styleFile);
		QString style = styleIn.readAll();
		styleFile.close();
		qApp->setStyleSheet(style);
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
		dlg_commoninput dlg(this, "Layout Name", 1, inList, inPara, NULL);
		if (dlg.exec() == QDialog::Accepted)
		{
			layoutName =  dlg.getText()[0];
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
	loadLayout(child, this->layout->currentText());
}

void MainWindow::loadLayout(MdiChild* child, QString const & layout)
{
	assert(child);
	if(!child)
	{
		return;
	}
	QSettings settings;
	QByteArray state = settings.value( "Layout/state" + layout ).value<QByteArray>();
	child->hide();
	child->restoreState(state, 0);
	child->show();
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

void MainWindow::addSubWindow( QWidget * child )
{
	mdiArea->addSubWindow( child );
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
	prefMagicLensSize = sender->GetMagicLensSize();
	if( mdiArea->subWindowList().size() == 1 )
	{
		MdiChild * child = dynamic_cast<MdiChild*> ( mdiArea->subWindowList().at( 0 )->widget() );
		if(!child)
			return;
		if( child == sender )
			SetModuleActionsEnabled( false );
	}
}


void MainWindow::InitResources()
{
	Q_INIT_RESOURCE(open_iA);
}
