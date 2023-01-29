// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iABoneThicknessSplitter.h"

iABoneThicknessSplitter::iABoneThicknessSplitter(QWidget* _pParent) : QSplitter(_pParent)
{
	const QColor cColor(palette().color(QPalette::WindowText));
	setStyleSheet("QSplitter::handle{background: " + cColor.name() + ";}");
}
