/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#pragma once

#include "iAChartWidget.h"
#include "iAPlotData.h"

#include "iAcharts_export.h"

#include <QDomNode>
#include <QPoint>
#include <QString>

#include <vector>

class QMenu;
class QPaintEvent;
class QPainter;

class iAChartFunction;
class iATFTableDlg;
class iATransferFunction;
class iAXmlSettings;

//! A chart widget that can also show functions overlaid over the chart area (transfer function, Gaussian and Bezier curves)
class iAcharts_API iAChartWithFunctionsWidget : public iAChartWidget
{
	Q_OBJECT

public:
	enum AdditionalMode { MOVE_NEW_POINT_MODE=Y_ZOOM_MODE+1, MOVE_POINT_MODE};

	iAChartWithFunctionsWidget(QWidget *parent,
		QString const & label = "Greyvalue", QString const & yLabel = "");
	virtual ~iAChartWithFunctionsWidget();

	//! Get the index of the selected point in the selected function.
	int selectedFuncPoint() const;
	bool isFuncEndPoint(int index) const;
	//! Set the transfer functions to be displayed on top of the chart.
	void setTransferFunction(iATransferFunction* f);
	//! Get the currently selected function.
	iAChartFunction * selectedFunction();
	//! Get all functions currently defined in this chart.
	std::vector<iAChartFunction*> &functions();
	//! Set whether the user is allowed to reset the transfer function.
	void setAllowTrfReset(bool allow);
	//! Set whether the user can add additional functions (Bezier and Gaussian curves), in addition to the standard transfer function.
	void setEnableAdditionalFunctions(bool enable);

	//! Add a Gaussian function with the given parameters.
	void addGaussianFunction(double mean, double sigma, double multiplier);

	bool loadProbabilityFunctions(iAXmlSettings & xml);
	void saveProbabilityFunctions(iAXmlSettings &xml);
protected:
	//! @{ Events overrided from Qt.
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;

#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	void enterEvent(QEvent* event) override;
#else
	void enterEvent(QEnterEvent* event) override;
#endif
	void keyPressEvent(QKeyEvent *event) override;
	//! @}

	//! @{ Methods overrided from iAChartWidget
	void addContextMenuEntries(QMenu* contextMenu) override;
	void changeMode(int newMode, QMouseEvent *event) override;
	void drawAfterPlots(QPainter& painter) override;
	//! @}

	virtual void drawFunctions(QPainter &painter);

	std::vector<iAChartFunction*> m_functions;
	size_t m_selectedFunction;
	bool m_showFunctions;

signals:
	void updateViews();  // TODO: this "knowledge" of when some other views should be updated should probably be somewhere else (mdichild?)
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	void active(); // TODO: check use of this signal
	void applyTFForAll();
	void updateTFTable();

public slots:
	int deletePoint();
	void changeColor(QMouseEvent *event = nullptr);
	void resetTrf();
	void loadTransferFunction();
	void loadTransferFunction(QDomNode functionsNode);
	void saveTransferFunction();
	void applyTransferFunctionForAll();
	void addBezierFunction();
	void addGaussianFunction();
	void loadFunctions();
	void saveFunctions();
	void removeFunction();
	void showTFTable();
	void tfTableIsFinished();

private slots:
	void selectFunction(size_t functionIndex);

private:
	bool m_allowTrfReset;
	bool m_enableAdditionalFunctions;
	iATFTableDlg* m_TFTable;

	void newTransferFunction();
};
