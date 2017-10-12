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
#include "iAConsole.h"
#include "iASCIFIOCheck.h"
#include "mainwindow.h"
#include "version.h"

#include <QApplication>
#include <QDate>

int main(int argc, char *argv[])
{
	MainWindow::InitResources();
	QApplication app(argc, argv);
	app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

	MainWindow mainWin("open_iA", VERSION, ":/images/splashscreen.png");

	iAConsole::GetInstance();				// (workaround) for binding log instance to GUI thread

	CheckSCIFIO(QCoreApplication::applicationDirPath());

	mainWin.LoadArguments(argc, argv);
	// TODO: unify with logo in slicer/renderer!
	app.setWindowIcon(QIcon(QPixmap(":/images/ia.png")));
	mainWin.setWindowIcon(QIcon(QPixmap(":/images/ia.png")));

	if( QDate::currentDate().dayOfYear() >= 340 ) {
		mainWin.setWindowTitle("Merry X-Mas and a happy new year!");
		mainWin.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
		app.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
	}
	mainWin.show();

	return app.exec();
}
