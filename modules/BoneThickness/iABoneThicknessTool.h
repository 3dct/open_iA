// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iABoneThickness.h"

#include <iATool.h>

#include <QDoubleSpinBox>
#include <QScopedPointer>
#include <QLabel>

class iABoneThicknessChartBar;
class iABoneThicknessTable;

class iABoneThicknessTool : public QObject, public iATool
{
	Q_OBJECT

public:
	iABoneThicknessTool(iAMainWindow* mainWnd, iAMdiChild* child);
	void setStatistics();

private:
	iABoneThicknessTable* m_pBoneThicknessTable = nullptr;
	iABoneThicknessChartBar* m_pBoneThicknessChartBar = nullptr;
	QDoubleSpinBox* m_pDoubleSpinBoxSphereRadius = nullptr;
	QDoubleSpinBox* m_pDoubleSpinBoxThicknessMaximum = nullptr;
	QDoubleSpinBox* m_pDoubleSpinBoxSurfaceDistanceMaximum = nullptr;
	QLabel* pLabelMeanTh = nullptr;
	QLabel* pLabelStdTh = nullptr;
	QLabel* pLabelMeanSDi = nullptr;
	QLabel* pLabelStdSDi = nullptr;
	QScopedPointer<iABoneThickness> m_pBoneThickness;

private slots:
	void slotDoubleSpinBoxSphereRadius();
	void slotDoubleSpinBoxThicknessMaximum();
	void slotDoubleSpinBoxSurfaceDistanceMaximum();
	void slotPushButtonOpen();
	void slotPushButtonSave();
	void slotCheckBoxShowThickness(const bool& _bChecked);
	void slotCheckBoxTransparency(const bool& _bChecked);
};
