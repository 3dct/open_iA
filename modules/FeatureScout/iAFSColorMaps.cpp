// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFSColorMaps.h"

#include <vtkMath.h>

#include <QColor>

namespace
{
	void CheckBounds(double color_out[3])
	{
		for (int i = 0; i < 3; ++i)
		{
			if (color_out[i] < 0)
			{
				color_out[i] = 0;
			}
			if (color_out[i] > 1.0)
			{
				color_out[i] = 1.0;
			}
		}
	}

	void ColormapRGB(const double normal[3], double color_out[3])
	{
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = 0.5 + 0.5 * normal[i];
		}
		CheckBounds(color_out);
	}

	void ColormapCMY(const double normal[3], double color_out[3])
	{
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = 0.5 + 0.5 * normal[i];
		}
		CheckBounds(color_out);
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = 1 - color_out[i];
		}
	}

	void ColormapCMYNormalized(const double normal[3], double color_out[3])
	{
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = 0.5 + 0.5 * normal[i];
		}
		CheckBounds(color_out);
		for (int i = 0; i < 3; ++i) color_out[i] = 1 - color_out[i];
		vtkMath::Normalize(color_out);
	}

	void ColormapRGBNormalized(const double normal[3], double color_out[3])
	{
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = 0.5 + 0.5 * normal[i];
		}
		CheckBounds(color_out);
		vtkMath::Normalize(color_out);
	}

	void ColormapCMYAbsolute(const double normal[3], double color_out[3])
	{
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = fabs(normal[i]);
		}
		CheckBounds(color_out);
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = 1 - color_out[i];
		}
	}

	void ColormapRGBAbsolute(const double normal[3], double color_out[3])
	{
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = fabs(normal[i]);
		}
		CheckBounds(color_out);
	}

	void ColormapCMYAbsoluteNormalized(const double normal[3], double color_out[3])
	{
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = fabs(normal[i]);
		}
		CheckBounds(color_out);
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = 1 - color_out[i];
		}
		vtkMath::Normalize(color_out);
	}

	void ColormapRGBAbsoluteNormalized(const double normal[3], double color_out[3])
	{
		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = fabs(normal[i]);
		}
		CheckBounds(color_out);
		vtkMath::Normalize(color_out);
	}

	void ColormapRGBHalfSphere(const double normal[3], double color_out[3])
	{
		double longest = std::max(normal[0], normal[1]);
		longest = std::max(normal[2], longest);
		if (!longest)
		{
			longest = 0.00000000001;
		}

		double oneVec[3] = {1.0, 1.0, 1.0};
		vtkMath::Normalize(oneVec);
		int sign = 1;
		if (vtkMath::Dot(oneVec, normal) < 0)
		{
			sign = -1;
		}

		for (int i = 0; i < 3; ++i)
		{
			color_out[i] = 0.5 + 0.5 * sign * normal[i];  ///longest;
		}
		CheckBounds(color_out);
	}
}

ColormapFuncPtr getColorMap(int index)
{
	static ColormapFuncPtr colormapsIndex[] = {
		ColormapRGB,
		ColormapRGBNormalized,
		ColormapRGBAbsolute,
		ColormapRGBAbsoluteNormalized,

		ColormapCMY,
		ColormapCMYNormalized,
		ColormapCMYAbsolute,
		ColormapCMYAbsoluteNormalized,

		ColormapRGBHalfSphere,
	};
	return colormapsIndex[index];
}

QColor getClassColor(int cid)
{
	// automatically select a predefined color
	// (from the list of colors defined in the list of SVG
	// color keyword names provided by the World Wide Web Consortium).
	//http://www.w3.org/TR/SVG/types.html#ColorKeywords
	if (cid > 7) { cid = 1; }
	switch (cid)
	{
	default:
	case 1: return QColor("cornflowerblue");
	case 2: return QColor("darkorange");
	case 3: return QColor("chartreuse");
	case 4: return QColor("yellow");
	case 5: return QColor("mediumvioletred");
	case 6: return QColor("blue");
	case 7: return QColor("green");
	}
}
