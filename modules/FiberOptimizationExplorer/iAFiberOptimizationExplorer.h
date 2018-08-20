/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

#include <vtkSmartPointer.h>

#include <QMainWindow>
#include <QMap>
#include <QSharedPointer>

#include <vector>

class InteractorStyle;

class iA3DCylinderObjectVis;
class iAColorTheme;
class iARendererManager;
class iAResultData;
class MainWindow;

class QVTKOpenGLWidget;
class vtkTable;

class QLabel;

class iAFiberOptimizationExplorer : public QMainWindow
{
public:
	iAFiberOptimizationExplorer(QString const & path, MainWindow* mainWnd);
	~iAFiberOptimizationExplorer();
	void loadStateAndShow();
private slots:
	void toggleVis(int);
	void miniMouseEvent(QMouseEvent* ev);
	void timeSliderChanged(int);
	void selectionChanged(std::vector<size_t> const & selection);
private:
	std::vector<iAResultData> m_resultData;
	QSharedPointer<iARendererManager> m_renderManager;
	QSharedPointer<iA3DCylinderObjectVis> m_lastMain3DVis;
	int m_lastResultID;
	vtkSmartPointer<InteractorStyle> m_style;
	QVTKOpenGLWidget* m_mainRenderer;
	iAColorTheme const * m_colorTheme;
	MainWindow* m_mainWnd;
	int m_timeStepCount;
	QLabel* m_currentTimeStepLabel;
};
