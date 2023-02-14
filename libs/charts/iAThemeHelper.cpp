// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAThemeHelper.h"

#include <QIcon>
#include <QString>

namespace
{
	bool& brightModeVar()
	{
		static bool brightMode = true;
		return brightMode;
	}
}

QIcon iAThemeHelper::icon(QString const& name)
{
	return QIcon(QString(":/images/%1%2.svg").arg(name).arg(brightMode() ? "" : "_light"));
}

void iAThemeHelper::setBrightMode(bool enabled)
{
	brightModeVar() = enabled;
}

bool  iAThemeHelper::brightMode()
{
	return brightModeVar();
}
