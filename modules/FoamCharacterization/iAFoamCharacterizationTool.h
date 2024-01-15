// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <QObject>

#include <memory>

class QPushButton;

class iAFoamCharacterizationTable;
class iADataSet;

class iAFoamCharacterizationTool : public QObject, public iATool
{
public:
	iAFoamCharacterizationTool(iAMainWindow* mainWnd, iAMdiChild * child);

private:
	std::shared_ptr<iADataSet> m_origDataSet;
	iAFoamCharacterizationTable* m_pTable = nullptr;
	QPushButton* m_pPushButtonAnalysis = nullptr;

private slots:
	void slotPushButtonAnalysis();
	void slotPushButtonBinarization();
	void slotPushButtonClear();
	void slotPushButtonDistanceTransform();
	void slotPushButtonExecute();
	void slotPushButtonFilter();
	void slotPushButtonOpen();
	void slotPushButtonRestore();
	void slotPushButtonSave();
	void slotPushButtonWatershed();
};
