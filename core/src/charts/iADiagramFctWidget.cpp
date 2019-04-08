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
	m_TFTable(nullptr),
	m_allowTrfReset(true),
	m_enableAdditionalFunctions(true),
	m_showFunctions(false),
	m_selectedFunction(0),
	m_activeChild(mdiChild)
{
	dlg_transfer *transferFunction = new dlg_transfer(this, QColor(0, 0, 0, 255));
	m_functions.push_back(transferFunction);
}

iADiagramFctWidget::~iADiagramFctWidget()
{
	for (auto fct: m_functions)
		delete fct;
}

int iADiagramFctWidget::getSelectedFuncPoint() const
{
	auto it = m_functions.begin();
	if (it == m_functions.end())
		return -1;
	dlg_function *func = *(it + m_selectedFunction);
	return func->getSelectedPoint();
}

bool iADiagramFctWidget::isFuncEndPoint(int index) const
{
	auto it = m_functions.begin();
	if (it == m_functions.end())
		return false;
	dlg_function *func = *(it + m_selectedFunction);
	return func->isEndPoint(index);
}

void iADiagramFctWidget::drawAfterPlots(QPainter & painter)
{
	if (m_showFunctions)
		drawFunctions(painter);
}

void iADiagramFctWidget::drawFunctions(QPainter &painter)
{
	int counter = 0;
	std::vector<dlg_function*>::iterator it = m_functions.begin();
	while (it != m_functions.end())
	{
		dlg_function *func = (*it);

		if (counter == m_selectedFunction)
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

	it = m_functions.begin();
	while (it != m_functions.end())
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
				m_zoomYPos = event->y();
				changeMode(Y_ZOOM_MODE, event);
			}
			else if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
				((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier))
			{
				m_zoomXPos = event->x();
				m_zoomYPos = event->y();
				m_xZoomStart = m_xZoom;
				changeMode(X_ZOOM_MODE, event);
			}
			else if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
			{
				m_translationStartX = m_translationX;
				changeMode(MOVE_VIEW_MODE, event);
			}
			else if (!isContextMenuVisible())
				changeMode(MOVE_POINT_MODE, event);
			break;
		case Qt::RightButton:
		{
			if (!m_showFunctions)
				return;
			std::vector<dlg_function*>::iterator it = m_functions.begin();
			dlg_function *func = *(it + m_selectedFunction);
			int selectedPoint = func->selectPoint(event);
			if (selectedPoint == -1)
				emit noPointSelected();
			update();
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
	std::vector<dlg_function*>::iterator it = m_functions.begin();
	dlg_function *func = *(it + m_selectedFunction);
	if (event->button() == Qt::LeftButton)
	{
		if (m_mode == MOVE_NEW_POINT_MODE)
			func->mouseReleaseEventAfterNewPoint(event);
		update();
		emit updateTFTable();
		emit updateViews();
	}
	m_mode = NO_MODE;
	m_contextMenuVisible = false;
	func->mouseReleaseEvent(event);
}

void iADiagramFctWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		changeColor(event);
	}
	iAChartWidget::mouseDoubleClickEvent(event);
}

void iADiagramFctWidget::mouseMoveEvent(QMouseEvent *event)
{
	switch (m_mode)
	{
		case MOVE_POINT_MODE:
		case MOVE_NEW_POINT_MODE:
		{
			int viewX = event->x() - leftMargin();
			int viewY = geometry().height() -event->y() -bottomMargin();
			if (m_showFunctions)
			{
				std::vector<dlg_function*>::iterator it = m_functions.begin();
				dlg_function *func = *(it + m_selectedFunction);
				func->moveSelectedPoint(viewX, viewY);
				update();
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
		m_selectedFunction++;
		if (m_selectedFunction >= m_functions.size())
			m_selectedFunction = 0;

		update();
	}
	else if (event->key() == Qt::Key_Up)
	{
		if (m_selectedFunction > 0)
			m_selectedFunction--;
		else
			m_selectedFunction = (int)(m_functions.size()-1);

		update();
	}
}

void iADiagramFctWidget::addContextMenuEntries(QMenu* contextMenu)
{
	if (m_showFunctions)
	{
		std::vector<dlg_function*>::iterator it = m_functions.begin();
		dlg_function *func = *(it + m_selectedFunction);

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
		contextMenu->addSeparator();
	}
	if (m_enableAdditionalFunctions)
	{
		contextMenu->addAction(QIcon(":/images/addBezier.png"), tr("Add bezier function"), this, SLOT(addBezierFunction()));
		contextMenu->addAction(QIcon(":/images/addGaussian.png"), tr("Add gaussian function"), this, SLOT(addGaussianFunction()));
		contextMenu->addAction(QIcon(":/images/openFkt.png"), tr("Load functions"), this, SLOT(loadFunctions()));
		contextMenu->addAction(QIcon(":/images/saveFkt.png"), tr("Save functions"), this, SLOT(saveFunctions()));

		if (m_selectedFunction != 0)
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
			std::vector<dlg_function*>::iterator it = m_functions.begin();
			dlg_function *func = *(it + m_selectedFunction);
			int x = event->x() - leftMargin();
			int y = geometry().height() - event->y() -bottomMargin();
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

				m_mode = MOVE_NEW_POINT_MODE;
			}
			else
				m_mode = MOVE_POINT_MODE;

			update();

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
	std::vector<dlg_function*>::iterator it = m_functions.begin();
	dlg_function *func = *(it + m_selectedFunction);

	int selectedPoint = func->getSelectedPoint();
	func->removePoint(selectedPoint);
	update();
	emit updateTFTable();
	emit updateViews();

	return selectedPoint;
}

void iADiagramFctWidget::changeColor(QMouseEvent *event)
{
	if (!m_showFunctions)
		return;
	std::vector<dlg_function*>::iterator it = m_functions.begin();
	dlg_function *func = *(it + m_selectedFunction);

	func->changeColor(event);

	update();
	emit updateTFTable();
	emit updateViews();
}

void iADiagramFctWidget::resetTrf()
{
	std::vector<dlg_function*>::iterator it = m_functions.begin();
	dlg_function *func = *(it + m_selectedFunction);

	func->reset();
	update();
	emit updateTFTable();
	emit updateViews();
}

void iADiagramFctWidget::updateTrf()
{
	((dlg_transfer*)m_functions[0])->TranslateToNewRange(xBounds());
	update();
}

void iADiagramFctWidget::newTransferFunction()
{
	update();
	emit noPointSelected();
	emit updateTFTable();
	dynamic_cast<dlg_transfer*>(m_functions[0])->triggerOnChange();
	emit updateViews();
}

void iADiagramFctWidget::loadTransferFunction()
{
	QString filePath = (m_activeChild) ? m_activeChild->filePath() : "";
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		iASettings s(fileName);
		s.LoadTransferFunction((dlg_transfer*)m_functions[0]);
		newTransferFunction();
	}
}

void iADiagramFctWidget::loadTransferFunction(QDomNode &functionsNode)
{
	iASettings::LoadTransferFunction(functionsNode, (dlg_transfer*)m_functions[0]);
	newTransferFunction();
}

void iADiagramFctWidget::saveTransferFunction()
{
	dlg_transfer *transferFunction = (dlg_transfer*)m_functions[0];
	QString filePath = (m_activeChild) ? m_activeChild->filePath() : "";
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
	dlg_bezier *bezier = new dlg_bezier(this, PredefinedColors()[m_functions.size() % 7]);
	bezier->addPoint(contextMenuPos().x(), activeHeight()- contextMenuPos().y());
	m_selectedFunction = (unsigned int)m_functions.size();
	m_functions.push_back(bezier);

	update();
	emit updateTFTable();
	emit updateViews();
}

void iADiagramFctWidget::addGaussianFunction()
{
	addGaussianFunction(contextMenuPos().x(), geometry().width() / 6, (int)((activeHeight() - contextMenuPos().y())*yZoom()));
}

void iADiagramFctWidget::addGaussianFunction(double mean, double sigma, double multiplier)
{
	dlg_gaussian *gaussian = new dlg_gaussian(this, PredefinedColors()[m_functions.size() % 7]);
	gaussian->setMean(mean);
	gaussian->setSigma(sigma);
	gaussian->setMultiplier(multiplier);
	m_selectedFunction = (unsigned int)m_functions.size();
	m_functions.push_back(gaussian);

	update();
	emit updateTFTable();
	emit updateViews();
}

bool iADiagramFctWidget::loadFunctions()
{
	if (!m_activeChild)
	{
		return false;
	}
	QString filePath = m_activeChild->filePath();
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		MdiChild* child = m_activeChild;
		MainWindow *mw = (MainWindow*)child->window();
		QDomDocument doc = mw->loadSettingsFile(fileName);
		QDomElement root = doc.documentElement();

		for (unsigned int i = 1; i < m_functions.size(); i++)
		{
			delete m_functions.back();
			m_functions.pop_back();
		}

		QDomNode functionsNode = root.namedItem("functions");
		if (functionsNode.isElement())
				mw->loadProbabilityFunctions(functionsNode);

		emit noPointSelected();
		emit updateViews();
		update();
	}

	return true;
}

bool iADiagramFctWidget::saveFunctions()
{
	if (!m_activeChild)
	{
		return false;
	}
	QString filePath = m_activeChild->filePath();
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		MdiChild* child = m_activeChild;
		MainWindow *mw = (MainWindow*)child->window();

		QDomDocument doc = mw->loadSettingsFile(fileName);
		mw->saveProbabilityFunctions(doc);
		mw->saveSettingsFile(doc, fileName);
	}
	return true;
}

void iADiagramFctWidget::removeFunction()
{
	std::vector<dlg_function*>::iterator it = m_functions.begin()+ m_selectedFunction;
	dlg_function *function = *it;
	m_functions.erase(it);
	delete function;
	m_selectedFunction--;
	update();
	emit updateViews();
}

void iADiagramFctWidget::setTransferFunctions(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* pwf)
{
	m_showFunctions = ctf && pwf;
	if (!m_showFunctions)
		return;
	((dlg_transfer*)m_functions[0])->setColorFunction(ctf);
	((dlg_transfer*)m_functions[0])->setOpacityFunction(pwf);
	newTransferFunction();
}

dlg_function *iADiagramFctWidget::getSelectedFunction()
{
	return m_functions[m_selectedFunction];
}

int iADiagramFctWidget::chartHeight() const
{
	return geometry().height() - bottomMargin();
}

std::vector<dlg_function*> &iADiagramFctWidget::functions()
{
	return m_functions;
}

void iADiagramFctWidget::setAllowTrfReset(bool allow)
{
	m_allowTrfReset = allow;
}

void iADiagramFctWidget::setEnableAdditionalFunctions(bool enable)
{
	m_enableAdditionalFunctions = enable;
}

void iADiagramFctWidget::showTFTable()
{
	if ( !m_TFTable )
	{
		std::vector<dlg_function*>::iterator it = m_functions.begin();
		dlg_function* func = *( it + m_selectedFunction );

		m_TFTable = new dlg_TFTable( this, func );
		m_TFTable->setWindowTitle( "Transfer Function Table View" );
		m_TFTable->setWindowFlags( Qt::Dialog |Qt::WindowMinimizeButtonHint |Qt::WindowCloseButtonHint );
		m_TFTable->setAttribute( Qt::WA_DeleteOnClose, true);
		m_TFTable->show();

		connect(m_TFTable, SIGNAL( destroyed( QObject* ) ), this, SLOT( tfTableIsFinished() ) );
		connect( this, SIGNAL( updateTFTable() ), m_TFTable, SLOT( updateTable() ) );
	}
}

QPoint iADiagramFctWidget::getTFTablePos() const
{
	return m_TFTable->pos();
}

void iADiagramFctWidget::setTFTablePos( QPoint pos )
{
	m_TFTable->move( pos );
}

bool iADiagramFctWidget::isTFTableCreated() const
{
	bool created;
	m_TFTable ? created = true : created = false;
	return created;
}

void iADiagramFctWidget::closeTFTable()
{
	m_TFTable->close();
}

void iADiagramFctWidget::tfTableIsFinished()
{
	m_TFTable = nullptr;
}
