/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <iATool.h>

#include <iAITKIO.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QObject>
#include <QVector>

class iADockWidgetWrapper;
class iAEnsemble;
class iAEnsembleDescriptorFile;
class iAEnsembleView;
class iAHistogramView;
class iAMemberView;
class iAScatterPlotView;
class iASpatialView;

class vtkLookupTable;

class iAUncertaintyTool: public QObject, public iATool
{
	Q_OBJECT
public:
	iAUncertaintyTool(iAMainWindow* mainWnd, iAMdiChild* child);
	void toggleDockWidgetTitleBars();
	void toggleSettings();
	void calculateNewSubEnsemble();
	bool loadEnsemble(QString const & fileName);
	void writeFullDataFile(QString const & fileName, bool writeIntensities, bool writeMemberLabels, bool writeMemberProbabilities, bool writeEnsembleUncertainties);
private slots:
	void memberSelected(int memberIdx);
	void ensembleSelected(QSharedPointer<iAEnsemble> ensemble);
	void continueEnsembleLoading();
private:
	iAHistogramView * m_labelDistributionView, * m_uncertaintyDistributionView;
	iAMemberView* m_memberView;
	iAScatterPlotView* m_scatterplotView;
	iASpatialView* m_spatialView;
	iAEnsembleView* m_ensembleView;
	QVector<iADockWidgetWrapper*> m_dockWidgets;
	QVector<iAITKIO::ImagePointer> m_shownMembers;
	QSharedPointer<iAEnsemble> m_currentEnsemble;
	vtkSmartPointer<vtkLookupTable> m_labelLut;
	int m_newSubEnsembleID;
	// cache for ensemble loading:
	QSharedPointer<iAEnsembleDescriptorFile> m_ensembleFile;
};
