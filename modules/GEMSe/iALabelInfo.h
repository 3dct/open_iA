// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class QColor;
class QString;

class iALabelInfo
{
public:
	virtual int count() const =0;
	virtual QString name(int idx) const =0;
	virtual QColor color(int idx) const =0;
};
