// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAClassifyDefectsDialog.h"

iAClassifyDefectsDialog::iAClassifyDefectsDialog()
{
	ui.setupUi(this);
	ui.Fibers->setOptions(iASetPathWidget::Mode::openFile, tr("Open Image"), tr("Extracted fibers (*.csv)"), tr("1JURVXF8HZ5K0NE1QYJ9"));
	ui.Defects->setOptions(iASetPathWidget::Mode::openFile, tr("Open Image"), tr("Extracted defects (*.txt)"), tr("IVJO2BSWPQ9NC46ICB6S"));
	ui.Output->setOptions(iASetPathWidget::Mode::directory, tr("Output directory"), tr(""), tr("3HTMBPBWBR4WF4TC32SF"));
}

iAClassifyDefectsDialog::~iAClassifyDefectsDialog()
{ }
