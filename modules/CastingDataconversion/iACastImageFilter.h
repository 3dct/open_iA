/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include <string>

#include "iAAlgorithm.h"

/**
 * An itk cast image filter. Basic filter is itkCastImageFilter.
 * Further details at http://www.itk.org/Doxygen/html/classitk_1_1CastImageFilter.html
 * \remarks	Kana, 01/12/2010.
 */

enum iACastType
{
	FHW_CAST_IMAGE,
	DATATYPE_CONVERSION,
};

class iACastImageFilter : public iAAlgorithm
{
public:
	iACastImageFilter( QString fn, iACastType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );

	/**
	 * Sets an odt parameters.
	 * \param	odt	Output image datatype.
	 */
	void setODTParameters( std::string odt ) { m_odt = odt; };
	void setDTCParameters( std::string type, float min, float max, double outmin, double outmax, int dtcdov ) { m_type = type; m_min = min; m_max = max; m_outmin = outmin; m_outmax = outmax; m_dov = dtcdov; };
protected:
	void run();
private:
	std::string m_odt;
	std::string m_type; float m_min, m_max; int m_dov; double m_outmin, m_outmax;
	iACastType m_castType;
	void fhwCastImage();
	void DataTypeConversion();
};
