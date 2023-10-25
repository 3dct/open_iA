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
	infoStr += QString("\nNumber of objects: %1").arg(m_table->GetNumberOfRows());
	infoStr += QString("\nNumber of characteristics: %1").arg(m_table->GetNumberOfColumns());
	// maybe column mapping?
	return infoStr;
}


#include <iACsvIO.h>
#include <iACsvVtkTableCreator.h>

#include <QFileInfo>

std::shared_ptr<iAObjectsData> loadObjectsCSV(iACsvConfig const& csvConfig)
{
	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
	{
		return std::shared_ptr<iAObjectsData>();
	}
	auto objData = std::make_shared<iAObjectsData>(QFileInfo(csvConfig.fileName).completeBaseName(), csvConfig.visType, creator.table(), io.outputMapping());
	if (!csvConfig.curvedFiberFileName.isEmpty())
	{
		readCurvedFiberInfo(csvConfig.curvedFiberFileName, objData->m_curvedFiberData);
	}
	return objData;
}