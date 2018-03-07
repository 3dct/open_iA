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
#include "iADiagramFctWidget.h"

#include "dlg_bezier.h"
#include "dlg_function.h"
#include "dlg_gaussian.h"
#include "dlg_TFTable.h"
#include "dlg_transfer.h"
#include "iAPlotData.h"
#include "iAMathUtility.h"
#include "iASettings.h"
#include "mainwindow.h"		// TODO: get rid of this inclusion!
#include "mdichild.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QtXml/QDomDocument>
#include <QXmlStreamWriter>

#include <cassert>

iADiagramFctWidget::iADiagramFctWidget(QWidget *parent, MdiChild *mdiChild,
	QString const & xLabel, QString const & yLabel) :
	iAChartWidget(parent, xLabel, yLabel),
	TFTable(0),
	m_allowTrfReset(true),
	m_enableAdditionalFunctions(true),
	m_showFunctions(false),
	selectedFunction(0),
	activeChild(mdiChild),
	updateAutomatically(true)
{
	dlg_transfer *transferFunction = new dlg_transfer(this, QColor(0, 0, 0, 255));
	functions.push_back(transferFunction);
}

iADiagramFctWidget::~iADiagramFctWidget()
{
	for (auto fct: functions)
		delete fct;
}

int iADiagramFctWidget::getSelectedFuncPoint() const
{
	auto it = functions.begin();
	if (it == functions.end())
		return -1;
	dlg_function *func = *(it + selectedFunction);
	return func->getSelectedPoint();
}

bool iADiagramFctWidget::isFuncEndPoint(int index) const
{
	auto it = functions.begin();
	if (it == functions.end())
		return false;
	dlg_function *func = *(it + selectedFunction);
	return func->isEndPoint(index);
}

bool iADiagramFctWidget::isUpdateAutomatically() const
{
	return updateAutomatically;
}

void iADiagramFctWidget::DrawAfterPlots(QPainter & painter)
{
	if (m_showFunctions)
		drawFunctions(painter);
}

void iADiagramFctWidget::drawFunctions(QPainter &painter)
{
	int counter = 0;
	std::vector<dlg_function*>::iterator it = functions.begin();
	while (it != functions.end())
	{
		dlg_function *func = (*it);
		
		if (counter == selectedFunction)
		{
			func->draw(painter, QColor(255,128,0,255), 2);
		}
		else
		{
			func->draw(painter);
		}
		++it;
		counter++;
	}

	it = functions.begin();
	while (it != functions.end())
	{
		dlg_function *func = (*it);
		func->drawOnTop(painter);
		++it;
	}
}

void iADiagramFctWidget::mousePressEvent(QMouseEvent *event)  
{
	switch(event->button())
	{
		case Qt::LeftButton:
			if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
				((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) &&
				((event->modifiers() & Qt::AltModifier) == Qt::AltModifier))
			{
				zoomY = event->y();
				changeMode(Y_ZOOM_MODE, event);
			}
			else if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
				((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier))
			{
				zoomX = event->x();
				zoomY = event->y();
				xZoomStart = xZoom;
				changeMode(X_ZOOM_MODE, event);
			}
			else if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
			{
				translationStartX = translationX;
				changeMode(MOVE_VIEW_MODE, event);
			}
			else if (!IsContextMenuVisible())
				changeMode(MOVE_POINT_MODE, event);
			break;
		case Qt::RightButton:
		{
			if (!m_showFunctions)
				return;
			std::vector<dlg_function*>::iterator it = functions.begin();
			dlg_function *func = *(it + selectedFunction);
			int selectedPoint = func->selectPoint(event);
			if (selectedPoint == -1)
				emit noPointSelected();
			redraw();
			break;
		}
		default:
			iAChartWidget::mousePressEvent(event);
			break;
	}
}

void iADiagramFctWidget::mouseReleaseEvent(QMouseEvent *event)  
{
	if (!m_showFunctions)
	{
		iAChartWidget::mouseReleaseEvent(event);
		return;
	}
	std::vector<dlg_function*>::iterator it = functions.begin();
	dlg_function *func = *(it + selectedFunction);
	if (event->button() == Qt::LeftButton)
	{
		if (mode == MOVE_NEW_POINT_MODE)
			func->mouseReleaseEventAfterNewPoint(event);
		redraw();
		emit updateTFTable();

		if (isUpdateAutomatically())
			emit updateViews();
	}
	mode = NO_MODE;
	m_contextMenuVisible = false;
	func->mouseReleaseEvent(event);
}

void iADiagramFctWidget::mouseDoubleClickEvent(QMouseEvent *event)  
{
	if (event->button() == Qt::LeftButton)
	{
		changeColor(event);
	}
	emit DblClicked();
}

void iADiagramFctWidget::mouseMoveEvent(QMouseEvent *event)
{
	switch (mode)
	{
		case MOVE_POINT_MODE:
		case MOVE_NEW_POINT_MODE:
		{
			int viewX = event->x() - LeftMargin();
			int viewY = geometry().height() -event->y() -BottomMargin();
			if (m_showFunctions)
			{
				std::vector<dlg_function*>::iterator it = functions.begin();
				dlg_function *func = *(it + selectedFunction);
				func->moveSelectedPoint(viewX, viewY);
				redraw();
				emit updateTFTable();
			}
			showDataTooltip(event);
		}
		break;
		default:
			iAChartWidget::mouseMoveEvent(event);
	}
}

void iADiagramFctWidget::enterEvent(QEvent*)
{
	emit active();
}

void iADiagramFctWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Down)
	{
		selectedFunction++;
		if (selectedFunction >= functions.size())
			selectedFunction = 0;

		redraw();
	}
	else if (event->key() == Qt::Key_Up)
	{
		if(selectedFunction > 0)
			selectedFunction--;
		else
			selectedFunction = (int)(functions.size()-1);

		redraw();
	}
}

void iADiagramFctWidget::AddContextMenuEntries(QMenu* contextMenu)
{
	if (m_showFunctions)
	{
		std::vector<dlg_function*>::iterator it = functions.begin();
		dlg_function *func = *(it + selectedFunction);

		if (func->getSelectedPoint() != -1)
		{
			if (func->isColored())
			{
				QAction *changeColorAction = new QAction(QIcon(":/images/changeColor.png"), tr("Change Color"), this);
				contextMenu->setDefaultAction(changeColorAction);
				connect(changeColorAction, SIGNAL(triggered()), this, SLOT(changeColor()));
				contextMenu->addAction(changeColorAction);
			}

			if (func->isDeletable(func->getSelectedPoint()))
				contextMenu->addAction(QIcon(":/images/deletePoint.png"), tr("Delete"), this, SLOT(deletePoint()));
			contextMenu->addSeparator();
		}
		contextMenu->addAction(QIcon(":/images/TFTableView.png"), tr("Transfer Function Table View"), this, SLOT(showTFTable()));
		contextMenu->addAction(QIcon(":/images/loadtrf.png"), tr("Load transfer function"), this, SLOT(loadTransferFunction()));
		contextMenu->addAction(QIcon(":/images/savetrf.png"), tr("Save transfer function"), this, SLOT(saveTransferFunction()));
		contextMenu->addAction(QIcon(":/images/savetrf.png"), tr("Apply transfer function for all"), this, SLOT(applyTransferFunctionForAll()));
		if (m_allowTrfReset)
			contextMenu->addAction(QIcon(":/images/resetTrf.png"), tr("Reset transfer function"), this, SLOT(resetTrf()));

		QAction *autoUpdateAction = new QAction(tr("Update automatically"), this);
		autoUpdateAction->setCheckable(true);
		autoUpdateAction->setChecked(updateAutomatically);
		connect(autoUpdateAction, SIGNAL(toggled(bool)), this, SLOT(autoUpdate(bool)));
		contextMenu->addAction(autoUpdateAction);
		contextMenu->addAction(QIcon(":/images/update.png"), tr("Update views"), this, SIGNAL(updateViews()));
		contextMenu->addSeparator();
	}
	if (m_enableAdditionalFunctions)
	{
		contextMenu->addAction(QIcon(":/images/addBezier.png"), tr("Add bezier function"), this, SLOT(addBezierFunction()));
		contextMenu->addAction(QIcon(":/images/addGaussian.png"), tr("Add gaussian function"), this, SLOT(addGaussianFunction()));
		contextMenu->addAction(QIcon(":/images/openFkt.png"), tr("Load functions"), this, SLOT(loadFunctions()));
		contextMenu->addAction(QIcon(":/images/saveFkt.png"), tr("Save functions"), this, SLOT(saveFunctions()));

		if (selectedFunction != 0)
			contextMenu->addAction(QIcon(":/images/removeFkt.png"), tr("Remove selected function"), this, SLOT(removeFunction()));
	}
}

void iADiagramFctWidget::changeMode(int newMode, QMouseEvent *event)
{
	switch(newMode)
	{
		case MOVE_POINT_MODE:
		{
			if (!m_showFunctions)
				return;
			std::vector<dlg_function*>::iterator it = functions.begin();
			dlg_function *func = *(it + selectedFunction);
			int x = event->x() - LeftMargin();
			int y = geometry().height() - event->y() -BottomMargin();
			int selectedPoint = func->selectPoint(event, &x);

			// don't do anything if outside of diagram region:
			if (selectedPoint == -1 && x < 0)
				return;
			// disallow removal and reinsertion of first point; instead, insert a point after it:
			if (selectedPoint == -1 && x == 0)
				x = 1;
			if (selectedPoint == -1)
			{
				if (y < 0) y = 0;
				selectedPoint = func->addPoint(x, y);
				func->addColorPoint(x);
				
				mode = MOVE_NEW_POINT_MODE;
			}
			else
				mode = MOVE_POINT_MODE;

			redraw();

			bool endPoint = func->isEndPoint(selectedPoint);//selectedPoint == 0 || selectedPoint == opacityTF->GetSize()-1;
			if (endPoint)
				emit endPointSelected();
			else
				emit pointSelected();
		}
			break;
		default:
			iAChartWidget::changeMode(newMode, event);
			break;
	}
}

int iADiagramFctWidget::deletePoint()
{
	std::vector<dlg_function*>::iterator it = functions.begin();
	dlg_function *func = *(it + selectedFunction);

	int selectedPoint = func->getSelectedPoint();
	func->removePoint(selectedPoint);
	redraw();
	emit updateTFTable();
	if (isUpdateAutomatically())
		emit updateViews();

	return selectedPoint;
}

void iADiagramFctWidget::changeColor(QMouseEvent *event)
{
	if (!m_showFunctions)
		return;
	std::vector<dlg_function*>::iterator it = functions.begin();
	dlg_function *func = *(it + selectedFunction);

	func->changeColor(event);

	redraw();
	emit updateTFTable();
	if (isUpdateAutomatically())
		emit updateViews();
}

void iADiagramFctWidget::autoUpdate(bool toggled)
{
	updateAutomatically = toggled;
	emit autoUpdateChanged(toggled);
}

void iADiagramFctWidget::resetTrf()
{
	std::vector<dlg_function*>::iterator it = functions.begin();
	dlg_function *func = *(it + selectedFunction);

	func->reset();
	redraw();
	emit updateTFTable();
	emit updateViews();
}

void iADiagramFctWidget::updateTrf()
{
	((dlg_transfer*)functions[0])->TranslateToNewRange(XBounds());
	redraw();
}

void iADiagramFctWidget::NewTransferFunction()
{
	redraw();
	emit noPointSelected();
	emit updateTFTable();
	dynamic_cast<dlg_transfer*>(functions[0])->triggerOnChange();
	emit updateViews();
}

void iADiagramFctWidget::loadTransferFunction()
{
	QString filePath = (activeChild) ? activeChild->getFilePath() : "";
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		iASettings s(fileName);
		s.LoadTransferFunction((dlg_transfer*)functions[0]);
		NewTransferFunction();
	}
}

void iADiagramFctWidget::loadTransferFunction(QDomNode &functionsNode)
{
	iASettings::LoadTransferFunction(functionsNode, (dlg_transfer*)functions[0]);
	NewTransferFunction();
}

void iADiagramFctWidget::saveTransferFunction()
{
	dlg_transfer *transferFunction = (dlg_transfer*)functions[0];
	QString filePath = (activeChild) ? activeChild->getFilePath() : "";
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		iASettings s;
		s.StoreTransferFunction(transferFunction);
		s.Save(fileName);
	}
}

void iADiagramFctWidget::applyTransferFunctionForAll()
{
	emit applyTFForAll();
}

void iADiagramFctWidget::addBezierFunction()
{
	dlg_bezier *bezier = new dlg_bezier(this, PredefinedColors()[functions.size() % 7]);
	bezier->addPoint(ContextMenuPos().x(), ActiveHeight()- ContextMenuPos().y());
	selectedFunction = (unsigned int)functions.size();
	functions.push_back(bezier);

	redraw();
	emit updateTFTable();
	emit updateViews();
}

void iADiagramFctWidget::addGaussianFunction()
{
	addGaussianFunction(ContextMenuPos().x(), width / 6, (int)((ActiveHeight() - ContextMenuPos().y())*YZoom()));
}

void iADiagramFctWidget::addGaussianFunction(double mean, double sigma, double multiplier)
{
	dlg_gaussian *gaussian = new dlg_gaussian(this, PredefinedColors()[functions.size() % 7]);
	gaussian->setMean(mean);
	gaussian->setSigma(sigma);
	gaussian->setMultiplier(multiplier);
	selectedFunction = (unsigned int)functions.size();
	functions.push_back(gaussian);

	redraw();
	emit updateTFTable();
	emit updateViews();
}

bool iADiagramFctWidget::loadFunctions()
{
	if (!activeChild)
	{
		return false;
	}
	QString filePath = activeChild->getFilePath();
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		MdiChild* child = activeChild;
		MainWindow *mw = (MainWindow*)child->window();
		QDomDocument doc = mw->loadSettingsFile(fileName);
		QDomElement root = doc.documentElement();

		for (unsigned int i = 1; i < functions.size(); i++)
		{
			delete functions.back();
			functions.pop_back();
		}

		QDomNode functionsNode = root.namedItem("functions");
		if (functionsNode.isElement())
				mw->loadProbabilityFunctions(functionsNode);
		
		emit noPointSelected();
		emit updateViews();
		redraw();
	}

	return true;
}

bool iADiagramFctWidget::saveFunctions()
{
	if (!activeChild)
	{
		return false;
	}
	QString filePath = activeChild->getFilePath();
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		MdiChild* child = activeChild;
		MainWindow *mw = (MainWindow*)child->window();

		QDomDocument doc = mw->loadSettingsFile(fileName);
		mw->saveProbabilityFunctions(doc);
		mw->saveSettingsFile(doc, fileName);
	}
	return true;
}

void iADiagramFctWidget::removeFunction()
{
	std::vector<dlg_function*>::iterator it = functions.begin()+selectedFunction;
	dlg_function *function = *it;
	functions.erase(it);
	delete function;
	selectedFunction--;
	redraw();
	emit updateViews();
}

void iADiagramFctWidget::SetTransferFunctions(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* pwf)
{
	m_showFunctions = ctf && pwf;
	if (!m_showFunctions)
		return;
	((dlg_transfer*)functions[0])->setColorFunction(ctf);
	((dlg_transfer*)functions[0])->setOpacityFunction(pwf);
	NewTransferFunction();
}

dlg_function *iADiagramFctWidget::getSelectedFunction()
{
	return functions[selectedFunction];
}

int iADiagramFctWidget::ChartHeight() const
{
	return height - BottomMargin();
}

std::vector<dlg_function*> &iADiagramFctWidget::getFunctions()
{
	return functions;
}

void iADiagramFctWidget::SetAllowTrfReset(bool allow)
{
	m_allowTrfReset = allow;
}

void iADiagramFctWidget::SetEnableAdditionalFunctions(bool enable)
{
	m_enableAdditionalFunctions = enable;
}

void iADiagramFctWidget::showTFTable()
{
	if ( !TFTable )
	{
		std::vector<dlg_function*>::iterator it = functions.begin();
		dlg_function* func = *( it + selectedFunction );

		TFTable = new dlg_TFTable( this, func );
		TFTable->setWindowTitle( "Transfer Function Table View" );
		TFTable->setWindowFlags( Qt::Dialog |Qt::WindowMinimizeButtonHint |Qt::WindowCloseButtonHint );
		TFTable->setAttribute( Qt::WA_DeleteOnClose, true);
		TFTable->show();

		connect( TFTable, SIGNAL( destroyed( QObject* ) ), this, SLOT( TFTableIsFinished() ) );
		connect( this, SIGNAL( updateTFTable() ), TFTable, SLOT( updateTable() ) );
	}
}

QPoint iADiagramFctWidget::getTFTablePos() const
{
	return TFTable->pos();
}

void iADiagramFctWidget::setTFTablePos( QPoint pos )
{
	TFTable->move( pos );
}

bool iADiagramFctWidget::isTFTableCreated() const
{
	bool created;
	TFTable ? created = true : created = false;
	return created;
}

void iADiagramFctWidget::closeTFTable()
{
	TFTable->close();
}

void iADiagramFctWidget::TFTableIsFinished()
{
	TFTable = NULL;
}
