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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "ui_MajorityVoting.h"

#include "iAImageTreeNode.h"    // for LabelImagePointer
#include "iAQTtoUIConnector.h"

#include <vtkSmartPointer.h>

typedef iAQTtoUIConnector<QDockWidget, Ui_MajorityVoting>   dlg_MajorityVotingUI;

class dlg_GEMSe;
class iAColorTheme;
class iALookupTable;
class iAQSplom;
class MdiChild;

class vtkChartXY;
class vtkPlot;
class vtkTable;

class QCheckBox;

class dlg_MajorityVoting : public dlg_MajorityVotingUI
{
	Q_OBJECT
public:
	dlg_MajorityVoting(MdiChild* mdiChild, dlg_GEMSe* dlgGEMSe, int labelCount);
	void SetGroundTruthImage(LabelImagePointer groundTruthImage);
	void EnableUI();
private slots:
	void MinAbsPlot();
	void MinDiffPlot();
	void RatioPlot();
	void MaxPixelEntropyPlot();
	void AbsMinPercentSlider(int);
	void MinDiffPercentSlider(int);
	void MinRatioSlider(int);
	void MaxPixelEntropySlider(int);
	void LabelVoters(int);
	void Sample();
	void CheckBoxStateChanged(int state);
	void ClusterUncertaintyDice();
private:
	void AddResult(vtkSmartPointer<vtkTable> table, QString const & title);

	MdiChild*  m_mdiChild;
	dlg_GEMSe* m_dlgGEMSe;
	LabelImagePointer m_groundTruthImage;
	int m_labelCount;
	QVector<vtkSmartPointer<vtkTable> > m_results;
	QMap<QCheckBox*, int> m_checkBoxResultIDMap;
	QMap<int, QVector<vtkIdType> > m_plotMap;
	vtkSmartPointer<vtkChartXY> m_chartDiceVsUndec;
	vtkSmartPointer<vtkChartXY> m_chartValueVsDice;
	vtkSmartPointer<vtkChartXY> m_chartValueVsUndec;

	iAColorTheme const * m_colorTheme;
};
