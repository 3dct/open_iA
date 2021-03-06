/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iADataFolderDialog.h"

#include <defines.h>
#include <mainwindow.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

iADataFolderDialog::iADataFolderDialog( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ ) : QDialog( parent, f )
{
	setupUi( this );

	QSettings settings( organisationName, applicationName );
	dataFolder->setText( settings.value( "FeatureAnalyzer/GUI/resultsFolder", "" ).toString() );
	datasetsFolder->setText( settings.value( "FeatureAnalyzer/GUI/datasetsFolder", "" ).toString() );

	connect(tbOpenDataFolder, &QToolButton::clicked, this, &iADataFolderDialog::browseDataFolder);
	connect(tbOpenDatasetsFolder, &QToolButton::clicked, this, &iADataFolderDialog::browseDatasetsFolder);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &iADataFolderDialog::okBtnClicked );
}

QString iADataFolderDialog::ResultsFolderName()
{
	return dataFolder->text();
}

QString iADataFolderDialog::DatasetsFolderName()
{
	return datasetsFolder->text();
}

iADataFolderDialog::~iADataFolderDialog()
{
	QSettings settings( organisationName, applicationName );
	settings.setValue( "FeatureAnalyzer/GUI/resultsFolder", dataFolder->text() );
	settings.setValue( "FeatureAnalyzer/GUI/datasetsFolder", datasetsFolder->text() );
}

void iADataFolderDialog::browseDataFolder()
{
	QString dir = QFileDialog::getExistingDirectory( this, tr( "Results folder" ), dataFolder->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	if( dir == "" )
		return;

	dataFolder->setText( dir );
	QSettings settings( organisationName, applicationName );
	settings.setValue( "FeatureAnalyzer/GUI/resultsFolder", dataFolder->text() );
}

void iADataFolderDialog::browseDatasetsFolder()
{
	QString dir = QFileDialog::getExistingDirectory( this, tr( "Datasets folder" ), datasetsFolder->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	if( dir == "" )
		return;

	datasetsFolder->setText( dir );
	QSettings settings( organisationName, applicationName );
	settings.setValue( "FeatureAnalyzer/GUI/datasetsFolder", datasetsFolder->text() );
}

void iADataFolderDialog::okBtnClicked()
{
	QFileInfo dataInfo(dataFolder->text());
	if (!dataInfo.exists() || !dataInfo.isDir())
	{
		QMessageBox::warning(this, "Porosity Analyzer", "'Results Folder' does not point to a valid directory!");
		return;
	}
	QFileInfo datasetsInfo(datasetsFolder->text());
	if (!datasetsInfo.exists() || !datasetsInfo.isDir())
	{
		QMessageBox::warning(this, "Porosity Analyzer", "'Datasets Folder' does not point to a valid directory!");
		return;
	}
	accept();
}
