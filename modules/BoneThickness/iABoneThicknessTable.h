// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkType.h>    // for vtkIdType

#include <QTableView>

class iABoneThickness;
class iABoneThicknessChartBar;

class iABoneThicknessTable : public QTableView
{
	Q_OBJECT
public:
	explicit iABoneThicknessTable(QWidget* _pParent = nullptr);
	int selected() const;
	void set(iABoneThickness* _pBoneThickness, iABoneThicknessChartBar* _pBoneThicknessChartBar);
	void setSelected(const vtkIdType& _idSelected);

private:
	iABoneThickness* m_pBoneThickness = nullptr;
	iABoneThicknessChartBar* m_pBoneThicknessChartBar = nullptr;

protected:
	QSize minimumSizeHint() const override;
	void mousePressEvent(QMouseEvent* e) override;
	void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
};
