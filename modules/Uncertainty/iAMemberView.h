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
#pragma once

#include <QWidget>

class iAEnsemble;
class iAMember;
class QCustomPlot;
class QCPBars;
class QCPRange;
class QCPDataSelection;

class iAMemberView: public QWidget
{
	Q_OBJECT
public:
	iAMemberView();
	void SetEnsemble(QSharedPointer<iAEnsemble> ensemble);
	QVector<int > SelectedMemberIDs() const;
signals:
	void MemberSelected(int memberIdx);
public slots:
	void StyleChanged();
private:
	QSharedPointer<iAEnsemble> m_ensemble;
	QCustomPlot* m_plot;
	std::vector<size_t> m_sortedIndices;
	QCPBars * mean;
private slots:
	void ChangedRange(QCPRange const & newRange);
	void SelectionChanged(QCPDataSelection const & selection);
	void ChartMousePress(QMouseEvent *);
};
