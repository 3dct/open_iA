// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include <cstddef>    // for size_t
#include <vector>

class QColor;
class vtkLookupTable;

//! Class representing a lookup table for color coding.
//! Used for example in the scatter plot matrix iAQSplom.
//! Has methods for importing an existing VTK lookup table,
//! and for mapping scalar values to a corresponding QColor.
class iAbase_API iALookupTable
{
public:
	iALookupTable();                                           //!< Set up an empty (uninitialized) iALookupTable.
	iALookupTable(QColor color);                               //!< Set up a lookup table with a single color.
	explicit iALookupTable(vtkLookupTable * vtk_lut);          //!< Initialize an iALookupTable from the given vtkLookupTable.
	void copyFromVTK(vtkLookupTable * vtk_lut);                //!< Copies data from the existing VTK lookup table (vtkLookupTable).
	void allocate(size_t numberOfColors);                      //!< Allocate place for a given number of colors and fill with zeros.
	size_t numberOfValues() const;                             //!< Get the number of values in the table.
	void getColor(double val, double * rgba_out) const;        //!< Map a scalar value into an RGBA color.
	QColor getQColor(double val) const;                        //!< Map a scalar value into an QColor object.
	void getTableValue(size_t index, double * rgba_out) const; //!< Get the RGBA color value for a given index in the table.
	void setColor(size_t colInd, double * rgba);               //!< Assign an RGBA color (every component 0..1) to a given index in the table.
	void setColor(size_t colInd, QColor const & col);          //!< Assign a color specified by a QColor to a given index in the table.
	void setData(size_t numberOfColors, double * rgba_data);   //!< Fill the lookup table using provided raw RBGA data for a given number of colors.
	void setOpacity(double alpha);                             //!< Set a given alpha value for every color in the table.
	const double * getRange() const;                           //!< Get the mapped scalar range.
	void setRange(double from_val, double to_val);             //!< Set the mapped scalar range.
	void setRange(double const * range);                       //!< Set the mapped scalar range.
	bool initialized() const;                                  //!< Check if the table has data (initialized).
protected:
	bool m_isInitialized = false;                              //!< flag which is on if lookup table data is set
	std::vector<double> m_data;                                //!< lookup table raw color data, each color is 4 doubles (RGBA)
	double m_range[2];                                         //!< scalar range mapped by the lookup table
	double m_rangeLen;                                         //!< length of the total scalar range that is mapped by the lookup table
	size_t m_numColors;                                        //!< number of colors stored in the lookup table
};
