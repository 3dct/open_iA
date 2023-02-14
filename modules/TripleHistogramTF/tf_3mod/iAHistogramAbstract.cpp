// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iATripleModalityWidget.h"
#include "iAHistogramStack.h"
#include "iAHistogramTriangle.h"

iAHistogramAbstract* iAHistogramAbstract::buildHistogramAbstract(iAHistogramAbstractType type, iATripleModalityWidget *tmw)
{
	switch (type)
	{
	case STACK:
		return new iAHistogramStack(tmw);

	case TRIANGLE:
		return new iAHistogramTriangle(tmw);
	}

	throw "Unexpected type";
}

void iAHistogramAbstract::updateDataSetNames(std::array<QString, 3> names)
{
	Q_UNUSED(names);
}
