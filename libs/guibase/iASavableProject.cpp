// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASavableProject.h"

#include <iAFileTypeRegistry.h>

#include <QApplication>
#include <QFileDialog>

bool iASavableProject::saveProject(QString const & basePath)
{
	QString defaultFilter(iAFileTypeRegistry::defaultExtFilterString(iADataSetType::Collection));
	QString projectFileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		QCoreApplication::translate("iAMainWindow", "Select Output File"),
		basePath,
		iAFileTypeRegistry::registeredFileTypes(iAFileIO::Save, iADataSetType::Collection),
		&defaultFilter);
	if (projectFileName.isEmpty())
	{
		return false;
	}
	m_fileName = projectFileName;
	return doSaveProject(projectFileName);
}

QString const& iASavableProject::fileName() const
{
	return m_fileName;
}

iASavableProject::~iASavableProject()
{}
