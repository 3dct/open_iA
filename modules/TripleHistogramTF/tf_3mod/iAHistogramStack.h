// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAHistogramAbstract.h"

#include <QVector>
#include <QWidget>

class iAHistogramStackGrid;
class iATripleModalityWidget;

class iAMdiChild;

class QLabel;
class QSplitter;

class iAHistogramStack : public iAHistogramAbstract
{
public:
	iAHistogramStack(iATripleModalityWidget *tripleModalityWidget);

	// OVERRIDES
	void initialize(std::array<QString, 3> names) override;
	bool isSlicerInteractionEnabled() override { return true; }
	void updateDataSetNames(std::array<QString, 3> names) override;

private:
	QSplitter *m_splitter;
	iAHistogramStackGrid *m_grid;
	QVector<QLabel*> m_labels;

	iATripleModalityWidget* m_tmw;
};
