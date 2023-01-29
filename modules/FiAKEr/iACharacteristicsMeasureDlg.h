// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>
#include <QSharedPointer>

QStringList const& DistributionDifferenceMeasureNames();

class iAFiberResultsCollection;
class Ui_CharacteristicsMeasureDialog;

class QStandardItemModel;

class iACharacteristicsMeasureDlg : public QDialog
{
	Q_OBJECT
public:
	iACharacteristicsMeasureDlg(QSharedPointer<iAFiberResultsCollection> data);
	QVector<int> selectedCharacteristics() const;
	QVector<int> selectedDiffMeasures() const;
private:
	QStandardItemModel* m_characteristicsModel, * m_diffMeasuresModel;
	QSharedPointer<Ui_CharacteristicsMeasureDialog> m_ui;
};
