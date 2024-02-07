// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

#include <memory>

QStringList const& DistributionDifferenceMeasureNames();

class iAFiberResultsCollection;
class Ui_CharacteristicsMeasureDialog;

class QStandardItemModel;

class iACharacteristicsMeasureDlg : public QDialog
{
	Q_OBJECT
public:
	iACharacteristicsMeasureDlg(std::shared_ptr<iAFiberResultsCollection> data);
	QVector<int> selectedCharacteristics() const;
	QVector<int> selectedDiffMeasures() const;
private:
	QStandardItemModel* m_characteristicsModel, * m_diffMeasuresModel;
	std::shared_ptr<Ui_CharacteristicsMeasureDialog> m_ui;
};
