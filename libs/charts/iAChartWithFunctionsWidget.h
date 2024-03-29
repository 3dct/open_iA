// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAChartWidget.h"

#include "iAcharts_export.h"

#include <QDomNode>
#include <QString>

#include <vector>

class QColor;
class QMenu;
class QPaintEvent;
class QPainter;

class iAChartFunction;
class iATFTableDlg;
class iATransferFunction;
class iAXmlSettings;

//! Preset colors for functions (TODO: maybe use iAColorTheme instead?)
iAcharts_API QColor* FunctionColors();

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
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	void applyTFForAll();
	void updateTFTable();
	void transferFunctionChanged();

public slots:
	int deletePoint();
	void changeColor(QMouseEvent *event = nullptr);
	void resetTrf();
	void loadTransferFunction();
	void loadTransferFunction(QDomNode functionsNode);
	void saveTransferFunction();
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
