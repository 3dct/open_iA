// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADataFolderDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

iADataFolderDialog::iADataFolderDialog( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ ) : QDialog( parent, f )
{
	setupUi( this );

	QSettings settings;
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
	QSettings settings;
	settings.setValue( "FeatureAnalyzer/GUI/resultsFolder", dataFolder->text() );
	settings.setValue( "FeatureAnalyzer/GUI/datasetsFolder", datasetsFolder->text() );
}

void iADataFolderDialog::browseDataFolder()
{
	QString dir = QFileDialog::getExistingDirectory( this, tr( "Results folder" ), dataFolder->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	if( dir == "" )
		return;

	dataFolder->setText( dir );
	QSettings settings;
	settings.setValue( "FeatureAnalyzer/GUI/resultsFolder", dataFolder->text() );
}

void iADataFolderDialog::browseDatasetsFolder()
{
	QString dir = QFileDialog::getExistingDirectory( this, tr( "Datasets folder" ), datasetsFolder->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	if( dir == "" )
		return;

	datasetsFolder->setText( dir );
	QSettings settings;
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
