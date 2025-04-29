// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class iAMdiChild;
class iATripleModalityWidget;

enum iAHistogramAbstractType
{
	STACK, TRIANGLE
};

class iAHistogramAbstract : public QWidget
{
public:
	virtual void initialize(std::array<QString, 3> names) = 0;
	virtual bool isSlicerInteractionEnabled() = 0;
	virtual void updateDataSetNames(std::array<QString, 3> names);

	static iAHistogramAbstract* buildHistogramAbstract(iAHistogramAbstractType type, iATripleModalityWidget *tmw);
};
