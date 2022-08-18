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

#include <vtkImageReader.h>  // for VTK_FILE_BYTE_ORDER_... constants
#include <vtkType.h>         // For types, e.g. VTK_UNSIGNED_SHORT

#include "iARawFileIO.h"     // for constants used in mapStructToMap
#include "iAToolsVTK.h"      // for mapVTKTypeToReadableDataType


//! Contains all metadata required to load a raw data file.
//! @deprecated new developments should use iARawFileIO and its parameters instead
struct iARawFileParameters
{
	iARawFileParameters():
		m_headersize(0),
		m_scalarType(VTK_UNSIGNED_SHORT),
		m_byteOrder(VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN)
	{
		std::fill(m_size, m_size+3, 1);
		std::fill(m_spacing, m_spacing+3, 1.0);
		std::fill(m_origin, m_origin+3, 0.0);
	}
	//! Size of the dataset in voxels in all 3 dimensions.
	unsigned int m_size[3];
	//! Voxel spacing of the dataset (dimension of a single voxel in  given unit).
	double m_spacing[3];
	//! Default origin of the dataset, typically at the global origin (0, 0, 0).
	double m_origin[3];
	//! Size of an optional header (which must be skipped), typically 0.
	quint64 m_headersize;
	//! The scalar type used for storing single voxel values, as VTK type specifier.
	//! See VTK_UNSIGNED_SHORT and other defines at the same place in vtkType.h
	int  m_scalarType;
	//! The byte order (little endian or big endian) in VTK type constants.
	//! See VTK_FILE_BYTE_ORDER_ defines in include file <vtkImageReader.h>
	//! this type is mapped to list index in raw file dialog in mapVTKByteOrderToIdx in iARawFileParamDlg
	int m_byteOrder;

	inline QVariantMap toMap() const
	{
		QMap<QString, QVariant> result;
		QVector<int> sizeVec{ static_cast<int>(m_size[0]), static_cast<int>(m_size[1]), static_cast<int>(m_size[2]) };
		result[iARawFileIO::SizeStr] = QVariant::fromValue(sizeVec);
		QVector<double> spcVec{ m_spacing[0], m_spacing[1], m_spacing[2] };
		result[iARawFileIO::SpacingStr] = QVariant::fromValue(spcVec);
		QVector<double> oriVec{ m_origin[0], m_origin[1], m_origin[2] };
		result[iARawFileIO::OriginStr] = QVariant::fromValue(oriVec);
		result[iARawFileIO::HeadersizeStr] = m_headersize;
		result[iARawFileIO::DataTypeStr] = mapVTKTypeToReadableDataType(m_scalarType);
		result[iARawFileIO::ByteOrderStr] = ByteOrder::mapVTKTypeToString(m_byteOrder);
		return result;
	}

	static inline iARawFileParameters fromMap(QVariantMap const& map)
	{
		iARawFileParameters result;
		auto sizeVec = map["Size"].value<QVector<int>>();
		auto spcVec = map["Spacing"].value<QVector<double>>();
		auto oriVec = map["Origin"].value<QVector<double>>();
		for (int c = 0; c < 3; ++c)
		{
			result.m_size[c] = sizeVec[c];
			result.m_spacing[c] = spcVec[c];
			result.m_origin[c] = oriVec[c];
		}
		result.m_headersize = map["Headersize"].toULongLong();
		result.m_scalarType = mapReadableDataTypeToVTKType(map["Data Type"].toString());
		result.m_byteOrder = ByteOrder::mapStringToVTKType(map["Byte Order"].toString());
		return result;
	}
};
