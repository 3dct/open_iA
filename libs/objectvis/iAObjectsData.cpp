// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectsData.h"

#include <vtkTable.h>

iAObjectsData::iAObjectsData(QString const & name, iAObjectVisType visType, vtkSmartPointer<vtkTable> table, std::shared_ptr<QMap<uint, uint>> colMapping) :
	iADataSet(iADataSetType::Objects),
	m_table(table),
	m_colMapping(colMapping),
	m_visType(visType)
{
	setMetaData(iADataSet::NameKey, name);
}

QString iAObjectsData::info() const
{
	QString infoStr = "Object type: ";
	switch (m_visType) {
		case iAObjectVisType::UseVolume: infoStr += "Labeled Volume"; break;
		case iAObjectVisType::Line:      infoStr += "Lines"; break;
		case iAObjectVisType::Cylinder:  infoStr += "Cylinders"; break;
		case iAObjectVisType::Ellipsoid: infoStr += "Ellipsoids"; break;
		default:        infoStr += "Unknown"; break;
	}
	infoStr += "\nNumber of objects: " + m_table->GetNumberOfRows();
	infoStr += "\nNumber of characteristics: " + m_table->GetNumberOfColumns();
	// maybe column mapping?
	return infoStr;
}