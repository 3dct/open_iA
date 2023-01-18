/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iARawFileParameters.h"

iARawFileParameters::iARawFileParameters():
	m_headersize(0),
	m_scalarType(VTK_UNSIGNED_SHORT),
	m_byteOrder(VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN)
{
	std::fill(m_size, m_size+3, 1);
	std::fill(m_spacing, m_spacing+3, 1.0);
	std::fill(m_origin, m_origin+3, 0.0);
}


QVariantMap rawParamsToMap(iARawFileParameters const& p)
{
	QVariantMap result;
	result["Size"] = variantVector<int>({ static_cast<int>(p.m_size[0]), static_cast<int>(p.m_size[1]), static_cast<int>(p.m_size[2]) });
	result["Spacing"] = variantVector<double>({ p.m_spacing[0], p.m_spacing[1], p.m_spacing[2] });
	result["Origin"] = variantVector<double>({ p.m_origin[0], p.m_origin[1], p.m_origin[2] });
	result["Headersize"] = p.m_headersize;
	result["DataType"] = mapVTKTypeToReadableDataType(p.m_scalarType);
	result["ByteOrder"] = ByteOrder::mapVTKTypeToString(p.m_byteOrder);
	return result;
}

iARawFileParameters rawParamsFromMap(QVariantMap const& map)
{
	iARawFileParameters result;
	setFromVectorVariant<int>(result.m_size, map["Size"]);
	setFromVectorVariant<double>(result.m_spacing, map["Spacing"]);
	setFromVectorVariant<double>(result.m_origin, map["Origin"]);
	result.m_headersize = map["Headersize"].toULongLong();
	result.m_scalarType = mapReadableDataTypeToVTKType(map["Data Type"].toString());
	result.m_byteOrder = ByteOrder::mapStringToVTKType(map["Byte Order"].toString());
	return result;
}
