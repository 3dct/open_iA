// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAChartWithFunctionsWidget.h"

#include "iAChartFunctionBezier.h"
#include "iAChartFunctionGaussian.h"
#include "iAChartFunctionTransfer.h"
#include "iATFTableDlg.h"
#include "iAThemeHelper.h"

#include "iALog.h"
#include "iAMapper.h"
#include "iAMathUtility.h"
#include "iAXmlSettings.h"

#include <vtkColorTransferFunction.h>

#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

#include <numbers>

QColor* FunctionColors()
{
	static QColor FunctColors[7] = {
		QColor(0, 0, 0),
		QColor(0, 255, 0),
		QColor(255, 0, 0),
		QColor(255, 255, 0),
		QColor(0, 255, 255),
		QColor(255, 0, 255),
		QColor(255, 255, 255)
	};
	return FunctColors;
}


iAChartWithFunctionsWidget::iAChartWithFunctionsWidget(QWidget *parent,
	QString const & xLabel, QString const & yLabel) :
	iAChartWidget(parent, xLabel, yLabel),
	m_selectedFunction(0),
	m_showFunctions(false),
	m_allowTFReset(true),
	m_enableAdditionalFunctions(true),
	m_TFTable(nullptr)
{
	iAChartTransferFunction *transferFunction = new iAChartTransferFunction(this, QColor(0, 0, 0, 255));
	connect(transferFunction, &iAChartTransferFunction::changed,
		this, &iAChartWithFunctionsWidget::transferFunctionChanged);
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
	for (auto func: m_functions)
	{
		if (counter == m_selectedFunction)
		{
			func->draw(painter, iAChartFunction::DefaultColor, iAChartFunction::LineWidthSelected);
		}
		else
		{
			func->draw(painter);
		}
		++counter;
	}
	for (auto func: m_functions)
	{
		func->drawOnTop(painter);
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
				m_zoomYPos = event->position().y();
				changeMode(Y_ZOOM_MODE, event);
			}
			else if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
				((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier))
			{
				m_zoomXPos = event->position().x();
				m_zoomYPos = event->position().y();
				m_xZoomStart = m_xZoom;
				changeMode(X_ZOOM_MODE, event);
			}
			else if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
			{
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
			int selectedPoint = m_functions[m_selectedFunction]->selectPoint(event->position().x() - leftMargin(), chartHeight() - event->position().y());
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

void iAChartWithFunctionsWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (!m_showFunctions)
	{
		iAChartWidget::mouseReleaseEvent(event);
		return;
	}
	auto func = m_functions[m_selectedFunction];
	if (event->button() == Qt::LeftButton)
	{
		if (m_mode == MOVE_NEW_POINT_MODE)
		{
			func->mouseReleaseEventAfterNewPoint(event);
		}
		update();
		emit updateTFTable();
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
			int mouseX = event->position().x() - leftMargin();
			int mouseY = geometry().height() - event->position().y() - bottomMargin();
			if (m_showFunctions)
			{
				m_functions[m_selectedFunction]->moveSelectedPoint(mouseX, mouseY);
				update();
				emit updateTFTable();
			}
		}
		break;
		default:
			iAChartWidget::mouseMoveEvent(event);
	}
}

void iAChartWithFunctionsWidget::enterEvent(QEnterEvent*)
{   // to get keyboard events;
	setFocus(Qt::OtherFocusReason);
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
		auto func = m_functions[m_selectedFunction];
		if (func->getSelectedPoint() != -1)
		{
			if (func->isColored())
			{
				QAction *changeColorAction = new QAction(iAThemeHelper::icon("color-wheel"), tr("Change Color"), this);
				contextMenu->setDefaultAction(changeColorAction);
				connect(changeColorAction, &QAction::triggered, [this] { changeColor(nullptr); });
				contextMenu->addAction(changeColorAction);
			}

			if (func->isDeletable(func->getSelectedPoint()))
			{
				contextMenu->addAction(iAThemeHelper::icon("function-point-remove"), tr("Delete"), this, &iAChartWithFunctionsWidget::deletePoint);
			}
			contextMenu->addSeparator();
		}
		contextMenu->addAction(iAThemeHelper::icon("table"), tr("Transfer Function Table View"), this, &iAChartWithFunctionsWidget::showTFTable);
		contextMenu->addAction(iAThemeHelper::icon("tf-load"), tr("Load transfer function"), this, QOverload<>::of(&iAChartWithFunctionsWidget::loadTransferFunction));
		contextMenu->addAction(iAThemeHelper::icon("tf-save"), tr("Save transfer function"), this, &iAChartWithFunctionsWidget::saveTransferFunction);
		contextMenu->addAction(iAThemeHelper::icon("tf-copy"), tr("Copy transfer function"), this, &iAChartWithFunctionsWidget::copyTransferFunction);
		// maybe disable pasting if no proper XML in clipboard?
		contextMenu->addAction(iAThemeHelper::icon("tf-paste"), tr("Paste transfer function"), this, &iAChartWithFunctionsWidget::pasteTransferFunction);
		if (m_allowTFReset)
		{
			contextMenu->addAction(iAThemeHelper::icon("tf-reset"), tr("Reset transfer function"), this, &iAChartWithFunctionsWidget::resetTF);
		}
		contextMenu->addSeparator();
	}
	if (m_enableAdditionalFunctions)
	{
		contextMenu->addAction(iAThemeHelper::icon("bezier"), tr("Add bezier function"), this, &iAChartWithFunctionsWidget::addBezierFunction);
		contextMenu->addAction(iAThemeHelper::icon("gaussian"), tr("Add gaussian function"), this, QOverload<>::of(&iAChartWithFunctionsWidget::addGaussianFunction));
		contextMenu->addAction(iAThemeHelper::icon("function-load"), tr("Load functions"), this, &iAChartWithFunctionsWidget::loadFunctions);
		if (m_functions.size() > 1)
		{   // it only makes sense to copy / save functions if there are some defined!
			contextMenu->addAction(iAThemeHelper::icon("function-save"), tr("Save functions"), this, &iAChartWithFunctionsWidget::saveFunctions);
			contextMenu->addAction(iAThemeHelper::icon("function-copy"), tr("Copy functions"), this, &iAChartWithFunctionsWidget::copyProbabilityFunctions);
		}
		// maybe disable pasting if no proper XML in clipboard?
		contextMenu->addAction(iAThemeHelper::icon("function-paste"), tr("Paste functions"), this, &iAChartWithFunctionsWidget::pasteProbabilityFunctions);

		if (m_selectedFunction != 0)
		{
			contextMenu->addAction(iAThemeHelper::icon("function-remove"), tr("Remove selected function"), this, &iAChartWithFunctionsWidget::removeFunction);
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
			auto func = m_functions[m_selectedFunction];
			int mouseX = event->position().x() - leftMargin();
			int mouseY = chartHeight() - event->position().y();
			int selectedPoint = func->selectPoint(mouseX, mouseY);
			// don't do anything if outside of diagram region:
			if (selectedPoint == -1 && mouseX < 0)
			{
				return;
			}
			bool added = false;
			if (selectedPoint == -1)
			{
				// disallow removal and reinsertion of first/last point
				mouseX = clamp(1, chartWidth() - 1, mouseX);
				mouseY = clamp(0, chartHeight(), mouseY);
				size_t numPointsBefore = func->numPoints();
				// if point's x is the same as for an existing point, that point will be selected, instead of a new one created:
				selectedPoint = func->addPoint(mouseX, mouseY);
				if (selectedPoint == -1)
				{
					return;
				}
				// to know whether really a point was added, we need to check whether the number of points has increased:
				added = numPointsBefore < func->numPoints();
			}
			if (added)
			{
				func->addColorPoint(mouseX);
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
	auto func = m_functions[m_selectedFunction];
	if (!func->isDeletable(func->getSelectedPoint()))
	{
		return -1;
	}
	int selectedPoint = func->getSelectedPoint();
	func->removePoint(selectedPoint);
	update();
	emit updateTFTable();

	return selectedPoint;
}

void iAChartWithFunctionsWidget::changeColor(QMouseEvent *event)
{
	if (!m_showFunctions)
	{
		return;
	}
	auto func = m_functions[m_selectedFunction];
	if (event != nullptr)
	{
		func->selectPoint(event->position().x() - leftMargin(), chartHeight() - event->position().y());
	}
	func->changeColor();

	update();
	emit updateTFTable();
}

void iAChartWithFunctionsWidget::resetTF()
{
	m_functions[m_selectedFunction]->reset();
	update();
	emit updateTFTable();
}

void iAChartWithFunctionsWidget::newTransferFunction()
{
	update();
	emit noPointSelected();
	emit updateTFTable();
	dynamic_cast<iAChartTransferFunction*>(m_functions[0])->triggerOnChange();
}

void iAChartWithFunctionsWidget::loadTransferFunction()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	auto tf = dynamic_cast<iAChartTransferFunction*>(m_functions[0])->tf();
	double oldRange[2];
	tf->colorTF()->GetRange(oldRange);
	iAXmlSettings s;
	if (!s.read(fileName))
	{
		LOG(lvlError, QString("Failed to read transfer function from file %1").arg(fileName));
		return;
	}
	s.loadTransferFunction(tf);
	tf->ensureValidity(oldRange);
	newTransferFunction();
}

void iAChartWithFunctionsWidget::loadTransferFunction(QDomNode functionsNode)
{
	auto tf = dynamic_cast<iAChartTransferFunction*>(m_functions[0])->tf();
	double oldRange[2];
	tf->colorTF()->GetRange(oldRange);
	iAXmlSettings::loadTransferFunction(functionsNode, tf);
	tf->ensureValidity(oldRange);
	newTransferFunction();
}

void iAChartWithFunctionsWidget::saveTransferFunction()
{
	iATransferFunction* tf = dynamic_cast<iAChartTransferFunction*>(m_functions[0])->tf();
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::currentPath(), tr("XML (*.xml)"));
	if (!fileName.isEmpty())
	{
		iAXmlSettings s;
		s.saveTransferFunction(tf);
		s.save(fileName);
		LOG(lvlInfo, QString("Transfer function saved to file %1.").arg(fileName));
	}
}

void iAChartWithFunctionsWidget::copyTransferFunction()
{
	iAXmlSettings s;
	iATransferFunction* tf = dynamic_cast<iAChartTransferFunction*>(m_functions[0])->tf();
	s.saveTransferFunction(tf);
	QGuiApplication::clipboard()->setText(s.toString());
}

void iAChartWithFunctionsWidget::pasteTransferFunction()
{
	auto tf = dynamic_cast<iAChartTransferFunction*>(m_functions[0])->tf();
	double oldRange[2];
	tf->colorTF()->GetRange(oldRange);
	iAXmlSettings xml;
	if (!xml.fromString(QGuiApplication::clipboard()->text()) ||
		!xml.loadTransferFunction(dynamic_cast<iAChartTransferFunction*>(m_functions[0])->tf()))
	{
		LOG(lvlWarn, "Inserting transfer function from clipboard failed; probably there is currently no valid transfer function definition in clipboard!");
	}
	else
	{
		tf->ensureValidity(oldRange);
		newTransferFunction();
	}
}

void iAChartWithFunctionsWidget::addBezierFunction()
{
	iAChartFunctionBezier *bezier = new iAChartFunctionBezier(this, FunctionColors()[m_functions.size() % 7]);
	bezier->addPoint(contextMenuPos().x(), chartHeight()- contextMenuPos().y());
	m_selectedFunction = m_functions.size();
	m_functions.push_back(bezier);
	update();
}

void iAChartWithFunctionsWidget::addGaussianFunction()
{
	double mean = mouse2DataX(contextMenuPos().x() - leftMargin());
	double sigma = m_xMapper->dstToSrc(geometry().width() / 20) - xBounds()[0];
	int contextYHeight = chartHeight() - contextMenuPos().y();
	double multiplier = yMapper().dstToSrc(contextYHeight) * (sigma * std::sqrt(2 * std::numbers::pi));
	addGaussianFunction(mean, sigma, multiplier);
}

void iAChartWithFunctionsWidget::addGaussianFunction(double mean, double sigma, double multiplier)
{
	iAChartFunctionGaussian *gaussian = new iAChartFunctionGaussian(this, FunctionColors()[m_functions.size() % 7]);
	gaussian->setMean(mean);
	gaussian->setSigma(sigma);
	gaussian->setMultiplier(multiplier);
	m_selectedFunction = m_functions.size();
	m_functions.push_back(gaussian);

	update();
}

void iAChartWithFunctionsWidget::loadFunctions()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), tr("XML (*.xml)"));
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
		LOG(lvlError, QString("Failed to read xml for functions from file %&1").arg(fileName));
		return;
	}
	if (!loadProbabilityFunctions(xml))
	{
		LOG(lvlError, QString("Failed to load functions from file %&1").arg(fileName));
		return;
	}
	emit noPointSelected();
	update();
}

void iAChartWithFunctionsWidget::copyProbabilityFunctions()
{
	iAXmlSettings xml;
	saveProbabilityFunctions(xml);
	QGuiApplication::clipboard()->setText(xml.toString());
}

void iAChartWithFunctionsWidget::pasteProbabilityFunctions()
{
	iAXmlSettings xml;
	if (!xml.fromString(QGuiApplication::clipboard()->text()) ||
		!loadProbabilityFunctions(xml))
	{
		LOG(lvlWarn, "Inserting functions from clipboard failed; probably there is currently no valid function definition in clipboard!");
	}
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
			iAChartFunctionBezier *bezier = new iAChartFunctionBezier(this, FunctionColors()[colorIndex % 7], false);
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
			iAChartFunctionGaussian *gaussian = new iAChartFunctionGaussian(this, FunctionColors()[colorIndex % 7], false);

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
		if (dynamic_cast<iAChartFunctionBezier*>(m_functions[f]))
		{
			QDomElement bezierElement = xml.createElement("bezier", functionsNode);
			auto bezier = dynamic_cast<iAChartFunctionBezier*>(m_functions[f]);
			for (auto const & point: bezier->getPoints())
			{
				QDomElement nodeElement = xml.createElement("node", bezierElement);
				nodeElement.setAttribute("value", tr("%1").arg(point.x()));
				nodeElement.setAttribute("fktValue", tr("%1").arg(point.y()));
			}
		}
		else if (dynamic_cast<iAChartFunctionGaussian*>(m_functions[f]))
		{
			QDomElement gaussianElement = xml.createElement("gaussian", functionsNode);
			auto gaussian = dynamic_cast<iAChartFunctionGaussian*>(m_functions[f]);
			gaussianElement.setAttribute("mean", tr("%1").arg(gaussian->getMean()));
			gaussianElement.setAttribute("sigma", tr("%1").arg(gaussian->getSigma()));
			gaussianElement.setAttribute("multiplier", tr("%1").arg(gaussian->getMultiplier()));
		}
		// otherwise: unknown function type, do nothing
	}
}

void iAChartWithFunctionsWidget::saveFunctions()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::currentPath() ,tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	saveProbabilityFunctions(xml);
	xml.save(fileName);
	LOG(lvlInfo, QString("Functions saved to file %1.").arg(fileName));
}

void iAChartWithFunctionsWidget::removeFunction()
{
	assert(m_selectedFunction != 0);
	if (m_selectedFunction == 0)
	{
		return;
	}
	auto it = m_functions.begin() + m_selectedFunction;
	auto function = *it;
	m_functions.erase(it);
	delete function;
	m_selectedFunction--;
	update();
}

void iAChartWithFunctionsWidget::setTransferFunction(iATransferFunction* f)
{
	m_showFunctions = f;
	if (!m_showFunctions)
	{
		return;
	}
	dynamic_cast<iAChartTransferFunction*>(m_functions[0])->setTF(f);
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

void iAChartWithFunctionsWidget::setAllowTFReset(bool allow)
{
	m_allowTFReset = allow;
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

		m_TFTable = new iATFTableDlg( this, func );
		m_TFTable->setWindowTitle("Transfer Function Table View");
		m_TFTable->setWindowFlags( Qt::Dialog |Qt::WindowMinimizeButtonHint |Qt::WindowCloseButtonHint );
		m_TFTable->setAttribute( Qt::WA_DeleteOnClose, true);
		m_TFTable->show();

		connect(m_TFTable, &iATFTableDlg::destroyed, this, &iAChartWithFunctionsWidget::tfTableIsFinished);
		connect(m_TFTable, &iATFTableDlg::transferFunctionChanged, this, [this] { update(); });
		connect(m_TFTable, &iATFTableDlg::transferFunctionChanged, this, &iAChartWithFunctionsWidget::transferFunctionChanged);
		connect(this, &iAChartWithFunctionsWidget::updateTFTable, m_TFTable, &iATFTableDlg::updateTable);
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
