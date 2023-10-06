// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class iAEnsemble;
class iASingleResult;

class QCustomPlot;
class QCPBars;
class QCPRange;
class QCPDataSelection;

class QWheelEvent;

class iAMemberView: public QWidget
{
	Q_OBJECT
public:
	iAMemberView();
	void SetEnsemble(std::shared_ptr<iAEnsemble> ensemble);
	QVector<int > SelectedMemberIDs() const;
signals:
	void MemberSelected(int memberIdx);
public slots:
	void StyleChanged();
	void mouseWheel(QWheelEvent*);
private:
	std::shared_ptr<iAEnsemble> m_ensemble;
	QCustomPlot* m_plot;
	std::vector<size_t> m_sortedIndices;
	QCPBars * mean;
private slots:
	void ChangedRange(QCPRange const & newRange);
	void SelectionChanged(QCPDataSelection const & selection);
	void ChartMousePress(QMouseEvent *);
};
