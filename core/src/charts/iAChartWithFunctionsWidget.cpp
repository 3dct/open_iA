/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAChartWithFunctionsWidget.h"

#include "dlg_TFTable.h"
#include "iAChartFunctionBezier.h"
#include "iAChartFunctionGaussian.h"
#include "iAChartFunctionTransfer.h"
#include "iAConsole.h"
#include "iAMapper.h"
#include "iAMathUtility.h"
#include "iAPlotData.h"
#include "iAXmlSettings.h"
#include "mdichild.h"

#include <vtkColorTransferFunction.h>
#include <vtkMath.h>
#include <vtkPiecewiseFunction.h>

#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>

#include <cassert>

iAChartWithFunctionsWidget::iAChartWithFunctionsWidget(QWidget *parent, MdiChild *mdiChild,
	QString const & xLabel, QString const & yLabel) :
	iAChartWidget(parent, xLabel, yLabel),
	m_activeChild(mdiChild),
	m_selectedFunction(0),
	m_showFunctions(false),
	m_allowTrfReset(true),
	m_enableAdditionalFunctions(true),
	m_TFTable(nullptr)
{
	iAChartTransferFunction *transferFunction = new iAChartTransferFunction(this, QColor(0, 0, 0, 255));
	m_functions.push_back(transferFunction);
}

iAChartWithFunctionsWidget::~iAChartWithFunctionsWidget()
{
	for (auto fct : m_functions)
	{
		delete fct;
	}
}

int iAChartWithFunctionsWidget::selectedFuncPoint() const
{
	auto it = m_functions.begin();
	if (it == m_functions.end())
	{
		return -1;
	}
	iAChartFunction *func = *(it + m_selectedFunction);
	return func->getSelectedPoint();
}

bool iAChartWithFunctionsWidget::isFuncEndPoint(int index) const
{
	auto it = m_functions.begin();
	if (it == m_functions.end())
	{
		return false;
	}
	iAChartFunction *func = *(it + m_selectedFunction);
	return func->isEndPoint(index);
}

void iAChartWithFunctionsWidget::drawAfterPlots(QPainter & painter)
{
	if (m_showFunctions)
	{
		drawFunctions(painter);
	}
}

void iAChartWithFunctionsWidget::drawFunctions(QPainter &painter)
{
	size_t counter = 0;
	std::vector<iAChartFunction*>::iterator it = m_functions.begin();
	while (it != m_functions.end())
	{
		iAChartFunction *func = (*it);

		if (counter == m_selectedFunction)
		{
			func->draw(painter, QColor(255,128,0,255), iAChartFunction::LineWidthSelected);
		}
		else
		{
			func->draw(painter);
		}
		++it;
		++counter;
	}
	it = m_functions.begin();
	while (it != m_functions.end())
	{
		iAChartFunction *func = (*it);
		func->drawOnTop(painter);
		++it;
	}
}

void iAChartWithFunctionsWidget::mousePressEvent(QMouseEvent *event)
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
			else
			{
				changeMode(MOVE_POINT_MODE, event);
			}
			break;
		case Qt::RightButton:
		{
			if (!m_showFunctions)
			{
				return;
			}
			std::vector<iAChartFunction*>::iterator it = m_functions.begin();
			iAChartFunction *func = *(it + m_selectedFunction);
			int selectedPoint = func->selectPoint(event);
			if (selectedPoint == -1)
			{
				emit noPointSelected();
			}
			update();
			break;
		}
		default:
			iAChartWidget::mousePressEvent(event);
			break;
	}
}

void iAChartWithFunctionsWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (!m_showFunctions)
	{
		iAChartWidget::mouseReleaseEvent(event);
		return;
	}
	std::vector<iAChartFunction*>::iterator it = m_functions.begin();
	iAChartFunction *func = *(it + m_selectedFunction);
	if (event->button() == Qt::LeftButton)
	{
		if (m_mode == MOVE_NEW_POINT_MODE)
		{
			func->mouseReleaseEventAfterNewPoint(event);
		}
		update();
		emit updateTFTable();
		emit updateViews();
	}
	m_mode = NO_MODE;
	func->mouseReleaseEvent(event);
}

void iAChartWithFunctionsWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		changeColor(event);
	}
	iAChartWidget::mouseDoubleClickEvent(event);
}

void iAChartWithFunctionsWidget::mouseMoveEvent(QMouseEvent *event)
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
				std::vector<iAChartFunction*>::iterator it = m_functions.begin();
				iAChartFunction *func = *(it + m_selectedFunction);
				func->moveSelectedPoint(viewX, viewY);
				update();
				emit updateTFTable();
			}
		}
		break;
		default:
			iAChartWidget::mouseMoveEvent(event);
	}
}

void iAChartWithFunctionsWidget::enterEvent(QEvent*)
{
	emit active();
}

void iAChartWithFunctionsWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Down)
	{
		m_selectedFunction++;
		if (m_selectedFunction >= m_functions.size())
		{
			m_selectedFunction = 0;
		}

		update();
	}
	else if (event->key() == Qt::Key_Up)
	{
		if (m_selectedFunction > 0)
		{
			--m_selectedFunction;
		}
		else
		{
			m_selectedFunction = m_functions.size() - 1;
		}

		update();
	}
	else if (event->key() == Qt::Key_Delete)
	{
		deletePoint();
	}
}

void iAChartWithFunctionsWidget::addContextMenuEntries(QMenu* contextMenu)
{
	if (m_showFunctions)
	{
		std::vector<iAChartFunction*>::iterator it = m_functions.begin();
		iAChartFunction *func = *(it + m_selectedFunction);

		if (func->getSelectedPoint() != -1)
		{
			if (func->isColored())
			{
				QAction *changeColorAction = new QAction(QIcon(":/images/changeColor.png"), tr("Change Color"), this);
				contextMenu->setDefaultAction(changeColorAction);
				connect(changeColorAction, &QAction::triggered, [this] { changeColor(nullptr); });
				contextMenu->addAction(changeColorAction);
			}

			if (func->isDeletable(func->getSelectedPoint()))
			{
				contextMenu->addAction(QIcon(":/images/deletePoint.png"), tr("Delete"), this, &iAChartWithFunctionsWidget::deletePoint);
			}
			contextMenu->addSeparator();
		}
		contextMenu->addAction(QIcon(":/images/TFTableView.png"), tr("Transfer Function Table View"), this, &iAChartWithFunctionsWidget::showTFTable);
		contextMenu->addAction(QIcon(":/images/loadtrf.png"), tr("Load transfer function"), this, QOverload<>::of(&iAChartWithFunctionsWidget::loadTransferFunction));
		contextMenu->addAction(QIcon(":/images/savetrf.png"), tr("Save transfer function"), this, &iAChartWithFunctionsWidget::saveTransferFunction);
		contextMenu->addAction(QIcon(":/images/savetrf.png"), tr("Apply transfer function for all"), this, &iAChartWithFunctionsWidget::applyTransferFunctionForAll);
		if (m_allowTrfReset)
		{
			contextMenu->addAction(QIcon(":/images/resetTrf.png"), tr("Reset transfer function"), this, &iAChartWithFunctionsWidget::resetTrf);
		}
		contextMenu->addSeparator();
	}
	if (m_enableAdditionalFunctions)
	{
		contextMenu->addAction(QIcon(":/images/addBezier.png"), tr("Add bezier function"), this, &iAChartWithFunctionsWidget::addBezierFunction);
		contextMenu->addAction(QIcon(":/images/addGaussian.png"), tr("Add gaussian function"), this, QOverload<>::of(&iAChartWithFunctionsWidget::addGaussianFunction));
		contextMenu->addAction(QIcon(":/images/openFkt.png"), tr("Load functions"), this, &iAChartWithFunctionsWidget::loadFunctions);
		contextMenu->addAction(QIcon(":/images/saveFkt.png"), tr("Save functions"), this, &iAChartWithFunctionsWidget::saveFunctions);

		if (m_selectedFunction != 0)
		{
			contextMenu->addAction(QIcon(":/images/removeFkt.png"), tr("Remove selected function"), this, &iAChartWithFunctionsWidget::removeFunction);
		}
		if (m_functions.size() > 1)
		{
			auto selectionMenu = contextMenu->addMenu("Select function");
			for (size_t f = 0; f < m_functions.size(); ++f)
			{
				auto action = selectionMenu->addAction(QString("%1: %2").arg(f)
					.arg(m_functions[f]->name()),
					[this, f]() { selectFunction(f); }
				);
				action->setCheckable(true);
				action->setChecked(m_selectedFunction == f);
			}
		}
	}
}

void iAChartWithFunctionsWidget::changeMode(int newMode, QMouseEvent *event)
{
	switch(newMode)
	{
		case MOVE_POINT_MODE:
		{
			if (!m_showFunctions)
			{
				return;
			}
			std::vector<iAChartFunction*>::iterator it = m_functions.begin();
			iAChartFunction *func = *(it + m_selectedFunction);
			int x = event->x() - leftMargin();
			int y = geometry().height() - event->y() -bottomMargin();
			int selectedPoint = func->selectPoint(event, &x);

			// don't do anything if outside of diagram region:
			if (selectedPoint == -1 && x < 0)
			{
				return;
			}
			// disallow removal and reinsertion of first point; instead, insert a point after it:
			if (selectedPoint == -1 && x == 0)
			{
				x = 1;
			}
			bool added = false;
			if (selectedPoint == -1)
			{
				if (y < 0)
				{
					y = 0;
				}
				size_t numPointsBefore = func->numPoints();
				// if point's x is the same as for an existing point, that point will be selected, instead of a new one created:
				selectedPoint = func->addPoint(x, y);
				// to know whether really a point was added, we need to check whether the number of points has increased:
				added = numPointsBefore < func->numPoints();
			}
			if (added)
			{
				func->addColorPoint(x);
				m_mode = MOVE_NEW_POINT_MODE;
			}
			else
			{
				m_mode = MOVE_POINT_MODE;
			}

			update();

			bool endPoint = func->isEndPoint(selectedPoint);//selectedPoint == 0 || selectedPoint == opacityTF->GetSize()-1;
			if (endPoint)
			{
				emit endPointSelected();
			}
			else
			{
				emit pointSelected();
			}
		}
			break;
		default:
			iAChartWidget::changeMode(newMode, event);
			break;
	}
}

int iAChartWithFunctionsWidget::deletePoint()
{
	std::vector<iAChartFunction*>::iterator it = m_functions.begin();
	iAChartFunction *func = *(it + m_selectedFunction);
	if (!func->isDeletable(func->getSelectedPoint()))
	{
		return -1;
	}
	int selectedPoint = func->getSelectedPoint();
	func->removePoint(selectedPoint);
	update();
	emit updateTFTable();
	emit updateViews();

	return selectedPoint;
}

void iAChartWithFunctionsWidget::changeColor(QMouseEvent *event)
{
	if (!m_showFunctions)
	{
		return;
	}
	std::vector<iAChartFunction*>::iterator it = m_functions.begin();
	iAChartFunction *func = *(it + m_selectedFunction);

	func->changeColor(event);

	update();
	emit updateTFTable();
	emit updateViews();
}

void iAChartWithFunctionsWidget::resetTrf()
{
	std::vector<iAChartFunction*>::iterator it = m_functions.begin();
	iAChartFunction *func = *(it + m_selectedFunction);

	func->reset();
	update();
	emit updateTFTable();
	emit updateViews();
}

void iAChartWithFunctionsWidget::updateTrf()
{
	((iAChartTransferFunction*)m_functions[0])->TranslateToNewRange(xBounds());
	update();
}

void iAChartWithFunctionsWidget::newTransferFunction()
{
	update();
	emit noPointSelected();
	emit updateTFTable();
	dynamic_cast<iAChartTransferFunction*>(m_functions[0])->triggerOnChange();
	emit updateViews();
}

void iAChartWithFunctionsWidget::loadTransferFunction()
{
	QString filePath = (m_activeChild) ? m_activeChild->filePath() : "";
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath ,tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings s;
	if (!s.read(fileName))
	{
		DEBUG_LOG(QString("Failed to read transfer function from file %1").arg(fileName));
		return;
	}
	s.loadTransferFunction((iAChartTransferFunction*)m_functions[0]);
	newTransferFunction();
}

void iAChartWithFunctionsWidget::loadTransferFunction(QDomNode functionsNode)
{
	iAXmlSettings::loadTransferFunction(functionsNode, (iAChartTransferFunction*)m_functions[0]);
	newTransferFunction();
}

void iAChartWithFunctionsWidget::saveTransferFunction()
{
	iAChartTransferFunction *transferFunction = (iAChartTransferFunction*)m_functions[0];
	QString filePath = (m_activeChild) ? m_activeChild->filePath() : "";
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		iAXmlSettings s;
		s.saveTransferFunction(transferFunction);
		s.save(fileName);
	}
}

void iAChartWithFunctionsWidget::applyTransferFunctionForAll()
{
	emit applyTFForAll();
}

void iAChartWithFunctionsWidget::addBezierFunction()
{
	iAChartFunctionBezier *bezier = new iAChartFunctionBezier(this, PredefinedColors()[m_functions.size() % 7]);
	bezier->addPoint(contextMenuPos().x(), chartHeight()- contextMenuPos().y());
	m_selectedFunction = m_functions.size();
	m_functions.push_back(bezier);

	update();
	emit updateTFTable();
	emit updateViews();
}

void iAChartWithFunctionsWidget::addGaussianFunction()
{
	double mean = m_xMapper->dstToSrc(contextMenuPos().x() - leftMargin() - xShift());
	double sigma = m_xMapper->dstToSrc(geometry().width() / 20) - xBounds()[0];
	int contextYHeight = chartHeight() - contextMenuPos().y();
	double multiplier = yMapper().dstToSrc(contextYHeight) * (sigma * sqrt(2 * vtkMath::Pi()));
	addGaussianFunction(mean, sigma, multiplier);
}

void iAChartWithFunctionsWidget::addGaussianFunction(double mean, double sigma, double multiplier)
{
	iAChartFunctionGaussian *gaussian = new iAChartFunctionGaussian(this, PredefinedColors()[m_functions.size() % 7]);
	gaussian->setMean(mean);
	gaussian->setSigma(sigma);
	gaussian->setMultiplier(multiplier);
	m_selectedFunction = m_functions.size();
	m_functions.push_back(gaussian);

	update();
	emit updateTFTable();
	emit updateViews();
}

void iAChartWithFunctionsWidget::loadFunctions()
{
	QString filePath = m_activeChild ? m_activeChild->filePath(): "";
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath ,tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	for (unsigned int i = 1; i < m_functions.size(); i++)
	{
		delete m_functions.back();
		m_functions.pop_back();
	}
	iAXmlSettings xml;
	if (!xml.read(fileName))
	{
		DEBUG_LOG(QString("Failed to read xml for functions from file %&1").arg(fileName));
		return;
	}
	if (!loadProbabilityFunctions(xml))
	{
		DEBUG_LOG(QString("Failed to load functions from file %&1").arg(fileName));
		return;
	}
	emit noPointSelected();
	emit updateViews();
	update();
}

bool iAChartWithFunctionsWidget::loadProbabilityFunctions(iAXmlSettings & xml)
{
	if (!xml.hasElement("functions"))
	{
		return false;
	}
	QDomNode functionsNode = xml.node("functions");
	int colorIndex = 1;
	QDomNodeList list = functionsNode.childNodes();
	for (int n = 0; n < list.size(); n++)
	{
		QDomNode functionNode = list.item(n);
		if (functionNode.nodeName() == "bezier")
		{
			iAChartFunctionBezier *bezier = new iAChartFunctionBezier(this, PredefinedColors()[colorIndex % 7], false);
			QDomNodeList innerList = functionNode.childNodes();
			for (int in = 0; in < innerList.length(); in++)
			{
				QDomNode node = innerList.item(in);
				QDomNamedNodeMap attributes = node.attributes();

				double value = attributes.namedItem("value").nodeValue().toDouble();
				double fktValue = attributes.namedItem("fktValue").nodeValue().toDouble();

				bezier->push_back(value, fktValue);
			}
			m_functions.push_back(bezier);
			++colorIndex;
		}
		else if (functionNode.nodeName() == "gaussian")
		{
			iAChartFunctionGaussian *gaussian = new iAChartFunctionGaussian(this, PredefinedColors()[colorIndex % 7], false);

			double mean = functionNode.attributes().namedItem("mean").nodeValue().toDouble();
			double sigma = functionNode.attributes().namedItem("sigma").nodeValue().toDouble();
			double multiplier = functionNode.attributes().namedItem("multiplier").nodeValue().toDouble();

			gaussian->setMean(mean);
			gaussian->setSigma(sigma);
			gaussian->setMultiplier(multiplier);

			m_functions.push_back(gaussian);
			++colorIndex;
		}
	}
	return true;
}

void iAChartWithFunctionsWidget::saveProbabilityFunctions(iAXmlSettings &xml)
{
	// does functions node exist
	QDomNode functionsNode;
	if (xml.hasElement("functions"))
	{
		functionsNode = xml.node("functions");
	}
	else
	{
		functionsNode = xml.createElement("functions");
	}

	// remove existing function nodes except the transfer function
	int n = 0;
	while (n < functionsNode.childNodes().length())
	{
		QDomNode node = functionsNode.childNodes().item(n);
		if (node.nodeName() == "bezier" || node.nodeName() == "gaussian")
		{
			functionsNode.removeChild(node);
		}
		else
		{
			++n;
		}
	}
	// add new function nodes
	for (unsigned int f = 1; f < m_functions.size(); ++f)
	{
		switch (m_functions[f]->getType())
		{
			case iAChartFunction::BEZIER:
			{
				QDomElement bezierElement = xml.createElement("bezier", functionsNode);
				std::vector<QPointF> points = ((iAChartFunctionBezier*)m_functions[f])->getPoints();
				std::vector<QPointF>::iterator it = points.begin();
				while (it != points.end())
				{
					QPointF point = *it;
					QDomElement nodeElement = xml.createElement("node", bezierElement);
					nodeElement.setAttribute("value", tr("%1").arg(point.x()));
					nodeElement.setAttribute("fktValue", tr("%1").arg(point.y()));
					++it;
				}
				break;
			}
			case iAChartFunction::GAUSSIAN:
			{
				QDomElement gaussianElement = xml.createElement("gaussian", functionsNode);
				iAChartFunctionGaussian * gaussian = (iAChartFunctionGaussian*)m_functions[f];
				gaussianElement.setAttribute("mean", tr("%1").arg(gaussian->getMean()));
				gaussianElement.setAttribute("sigma", tr("%1").arg(gaussian->getSigma()));
				gaussianElement.setAttribute("multiplier", tr("%1").arg(gaussian->getMultiplier()));
				break;
			}
			default:
			// unknown function type, do nothing
				break;
		}
	}
}

void iAChartWithFunctionsWidget::saveFunctions()
{
	QString filePath = m_activeChild ? m_activeChild->filePath(): "";
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	saveProbabilityFunctions(xml);
	xml.save(fileName);
}

void iAChartWithFunctionsWidget::removeFunction()
{
	std::vector<iAChartFunction*>::iterator it = m_functions.begin()+ m_selectedFunction;
	iAChartFunction *function = *it;
	m_functions.erase(it);
	delete function;
	m_selectedFunction--;
	update();
	emit updateViews();
}

void iAChartWithFunctionsWidget::setTransferFunctions(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* pwf)
{
	m_showFunctions = ctf && pwf;
	if (!m_showFunctions)
	{
		return;
	}
	((iAChartTransferFunction*)m_functions[0])->setColorFunction(ctf);
	((iAChartTransferFunction*)m_functions[0])->setOpacityFunction(pwf);
	newTransferFunction();
}

iAChartFunction *iAChartWithFunctionsWidget::selectedFunction()
{
	return m_functions[m_selectedFunction];
}

std::vector<iAChartFunction*> &iAChartWithFunctionsWidget::functions()
{
	return m_functions;
}

void iAChartWithFunctionsWidget::setAllowTrfReset(bool allow)
{
	m_allowTrfReset = allow;
}

void iAChartWithFunctionsWidget::setEnableAdditionalFunctions(bool enable)
{
	m_enableAdditionalFunctions = enable;
}

void iAChartWithFunctionsWidget::showTFTable()
{
	if ( !m_TFTable )
	{
		iAChartFunction* func = m_functions[0];

		m_TFTable = new dlg_TFTable( this, func );
		m_TFTable->setWindowTitle( "Transfer Function Table View" );
		m_TFTable->setWindowFlags( Qt::Dialog |Qt::WindowMinimizeButtonHint |Qt::WindowCloseButtonHint );
		m_TFTable->setAttribute( Qt::WA_DeleteOnClose, true);
		m_TFTable->show();

		connect(m_TFTable, &dlg_TFTable::destroyed, this, &iAChartWithFunctionsWidget::tfTableIsFinished);
		connect(this, &iAChartWithFunctionsWidget::updateTFTable, m_TFTable, &dlg_TFTable::updateTable);
	}
}

void iAChartWithFunctionsWidget::tfTableIsFinished()
{
	m_TFTable = nullptr;
}

void iAChartWithFunctionsWidget::selectFunction(size_t functionIndex)
{
	m_selectedFunction = functionIndex;
	update();
}