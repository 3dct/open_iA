/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
 
#include "pch.h"
#include "iADataFolderDialog.h"
#include "mainwindow.h"
#include "defines.h"

#include <QFileDialog>
#include <QSettings>

iADataFolderDialog::iADataFolderDialog( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ ) : QDialog( parent, f )
{
	setupUi( this );
	
	QSettings settings( organisationName, applicationName );
	dataFolder->setText( settings.value( "PorosityAnalyser/GUI/resultsFolder", "" ).toString() );
	datasetsFolder->setText( settings.value( "PorosityAnalyser/GUI/datasetsFolder", "" ).toString() );

	connect( tbOpenDataFolder, SIGNAL( clicked() ), this, SLOT( browseDataFolder() ) );
	connect( tbOpenDatasetsFolder, SIGNAL( clicked() ), this, SLOT( browseDatasetsFolder() ) );
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
	settings.setValue( "PorosityAnalyser/GUI/resultsFolder", dataFolder->text() );
	settings.setValue( "PorosityAnalyser/GUI/datasetsFolder", datasetsFolder->text() );
}

void iADataFolderDialog::browseDataFolder()
{
	QString dir = QFileDialog::getExistingDirectory( this, tr( "Results folder" ), dataFolder->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	if( dir == "" )
		return;

	dataFolder->setText( dir );
	QSettings settings( organisationName, applicationName );
	settings.setValue( "PorosityAnalyser/GUI/resultsFolder", dataFolder->text() );
}

void iADataFolderDialog::browseDatasetsFolder()
{
	QString dir = QFileDialog::getExistingDirectory( this, tr( "Datasets folder" ), datasetsFolder->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	if( dir == "" )
		return;

	datasetsFolder->setText( dir );
	QSettings settings( organisationName, applicationName );
	settings.setValue( "PorosityAnalyser/GUI/datasetsFolder", datasetsFolder->text() );
}

