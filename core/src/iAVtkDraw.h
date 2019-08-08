/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>

#include "open_iA_Core_export.h"


class open_iA_Core_API iAvtkImageData : public vtkImageData
{
public:
	static iAvtkImageData *New();
	vtkTypeMacro(iAvtkImageData, vtkImageData);
	void SetScalarRange(int min, int max)
	{
		ScalarRangeComputeTime.Modified();
		ScalarRange[0] = min;
		ScalarRange[1] = max;
	}
};

/**
 * change a single pixel in the image
 * don't forget to call Modified() on the image,
 * and if the scalar range changed, to update the scalar range!
 * (unfortunately that seems to require a dirty hack at the moment,
 * see iAvtkImageData above)
 */
template <typename T>
void drawPixel(vtkImageData* img, int x, int y, int z, T c);

/**
 * draw a line in the image
 * see the notes for drawPixel
 */
template <typename T>
void drawLine(vtkImageData* img, int x1, int y1, int x2, int y2, T c);

/**
* set all pixels in the image to the given value
* see the notes for drawPixel
*/
template <typename T>
void clearImage(vtkImageData* img, T c);




template <typename T>
void drawPixel(vtkImageData* img, int x, int y, int z, T c)
{
	T* pixel = static_cast<T*>(img->GetScalarPointer(x, y, z));
	*pixel = c;
}

// TODO: find better way to draw a line in a vtkImageData!!!!!
// source: http://www.roguebasin.com/index.php?title=Bresenham%27s_Line_Algorithm
template <typename T>
void drawLine(vtkImageData* img, int x1, int y1, int x2, int y2, T c)
{
	int delta_x(x2 - x1);
	// if x1 == x2, then it does not matter what we set here
	signed char const ix((delta_x > 0) - (delta_x < 0));
	delta_x = std::abs(delta_x) << 1;
	int delta_y(y2 - y1);
	// if y1 == y2, then it does not matter what we set here
	signed char const iy((delta_y > 0) - (delta_y < 0));
	delta_y = std::abs(delta_y) << 1;
	drawPixel(img, x1, y1, 0, c);
	if (delta_x >= delta_y)
	{
		// error may go below zero
		int error(delta_y - (delta_x >> 1));
		while (x1 != x2)
		{
			if ((error >= 0) && (error || (ix > 0)))
			{
				error -= delta_x;
				y1 += iy;
			}
			// else do nothing
			error += delta_y;
			x1 += ix;
			drawPixel(img, x1, y1, 0, c);
		}
	}
	else
	{
		// error may go below zero
		int error(delta_x - (delta_y >> 1));
		while (y1 != y2)
		{
			if ((error >= 0) && (error || (iy > 0)))
			{
				error -= delta_y;
				x1 += ix;
			}
			// else do nothing
			error += delta_x;
			y1 += iy;
			drawPixel(img, x1, y1, 0, c);
		}
	}
}

template <typename T>
void clearImage(vtkImageData* img, T c)
{
	int const * dim = img->GetDimensions();
	int * rawDataPtr = static_cast<T*>(img->GetScalarPointer());
	size_t fullSize = static_cast<size_t>(dim[0]) * dim[1] * dim[2];
	std::fill(rawDataPtr, rawDataPtr + fullSize, c);
	/*
	for (int x=extent[0]; x<=extent[1]; ++x)
	{
		for (int y=extent[2]; y<=extent[3]; ++y)
		{
			for (int z=extent[4]; z<=extent[5]; ++z)
			{
				drawPixel(img, x, y, z, c);
			}
		}
	}
	*/
}
