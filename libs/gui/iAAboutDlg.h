// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class QPixmap;
class QString;
class QWidget;

namespace iAAboutDlg
{
	void show(QWidget* parent, QPixmap const& aboutImg, QString const& buildInfo, QString const& gitVersion, int screenHeight);
}