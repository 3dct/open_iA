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
#pragma once

#include "iAChartWidget.h"
#include "iAPlotData.h"
#include "open_iA_Core_export.h"

#include <QSharedPointer>
#include <QPoint>
#include <QString>

#include <vector>

class QDomNode;
class QMenu;
class QPaintEvent;
class QPainter;

class vtkPiecewiseFunction;
class vtkColorTransferFunction;

class dlg_function;
class dlg_TFTable;
class MdiChild;

//! A chart widget that can also show functions overlaid over the chart area (transfer function, Gaussian and Bezier curves)
class open_iA_Core_API iADiagramFctWidget : public iAChartWidget
{
	Q_OBJECT

public:
	enum AdditionalMode { MOVE_NEW_POINT_MODE=Y_ZOOM_MODE+1, MOVE_POINT_MODE};

	static const int SELECTED_POINT_RADIUS = 10;
	static const int SELECTED_POINT_SIZE = 2*SELECTED_POINT_RADIUS;
	static const int POINT_RADIUS = 4;
	static const int POINT_SIZE = 2*POINT_RADIUS;

	static const int SELECTED_PIE_RADIUS = 16;
	static const int SELECTED_PIE_SIZE = 2 * SELECTED_PIE_RADIUS;
	static const int PIE_RADIUS = 16;
	static const int PIE_SIZE = 2 * PIE_RADIUS;

	iADiagramFctWidget(QWidget *parent, MdiChild *mdiChild,
		QString const & label = "Greyvalue", QString const & yLabel = "");
	virtual ~iADiagramFctWidget();

	//! Get the index of the selected point in the selected function.
	int selectedFuncPoint() const;
	bool isFuncEndPoint(int index) const;
	//! The height of the chart area itself (in pixels), without bottom margin (where the x axis, captions and labels are drawn).
	int chartHeight() const;
	//! Set the transfer functions to be displayed on top of the chart.
	void setTransferFunctions(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* pwf);
	//! Get the currently selected function.
	dlg_function * selectedFunction();
	//! Get all functions currently defined in this chart.
	std::vector<dlg_function*> &functions();
	//! Set whether the user is allowed to reset the transfer function.
	void setAllowTrfReset(bool allow);
	//! Set whether the user can add additional functions (Bezier and Gaussian curves), in addition to the standard transfer function.
	void setEnableAdditionalFunctions(bool enable);

	//! @{ Transfer Function Table dialog related.
	bool isTFTableCreated() const;
	void closeTFTable();
	QPoint getTFTablePos() const;
	void setTFTablePos(QPoint pos);
	//! @}

	//! Add a Gaussian function with the given parameters.
	void addGaussianFunction(double mean, double sigma, double multiplier);

protected:
	//! @{ Events overrided from Qt.
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void enterEvent(QEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	//! @}

	//! @{ Methods overrided from iAChartWidget
	void addContextMenuEntries(QMenu* contextMenu) override;
	void changeMode(int newMode, QMouseEvent *event) override;
	//! @}

	virtual void drawFunctions(QPainter &painter);

	MdiChild* m_activeChild;
	std::vector<dlg_function*> m_functions;
	unsigned int m_selectedFunction;
	bool m_showFunctions;

signals:
	void updateViews();
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	void active();
	void applyTFForAll();
	void updateTFTable();

public slots:
	int deletePoint();
	void changeColor(QMouseEvent *event = NULL);
	void resetTrf();
	void updateTrf();
	void loadTransferFunction();
	void loadTransferFunction(QDomNode &functionsNode);
	void saveTransferFunction();
	void applyTransferFunctionForAll();
	void addBezierFunction();
	void addGaussianFunction();
	bool loadFunctions();
	bool saveFunctions();
	void removeFunction();
	void showTFTable();
	void tfTableIsFinished();

private:
	bool m_allowTrfReset;
	bool m_enableAdditionalFunctions;
	dlg_TFTable* m_TFTable;

	void newTransferFunction();
	void drawAfterPlots(QPainter& painter) override;
};
