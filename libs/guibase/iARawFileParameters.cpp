// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	result["ByteOrder"] = iAByteOrder::mapVTKTypeToString(p.m_byteOrder);
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
	result.m_byteOrder = iAByteOrder::mapStringToVTKType(map["Byte Order"].toString());
	return result;
}
