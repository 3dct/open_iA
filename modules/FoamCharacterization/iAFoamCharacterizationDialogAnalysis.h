// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

class QLabel;

class iAFoamCharacterizationTableAnalysis;
class iAImageData;

class iAFoamCharacterizationDialogAnalysis : public QDialog
{
	Q_OBJECT

public:
	explicit iAFoamCharacterizationDialogAnalysis(iAImageData const * dataSet, QWidget* _pParent = nullptr);

private:
	QLabel* m_pLabel12 = nullptr;
	iAFoamCharacterizationTableAnalysis* m_pTable = nullptr;
	void analyse(iAImageData const* dataSet);

private slots:
	void slotPushButtonOk();

protected:
	virtual QSize sizeHint() const override;
};
