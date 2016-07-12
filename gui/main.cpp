/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/

#include "iAConsole.h"
#include "iASCIFIOCheck.h"
#include "mainwindow.h"
#include "version.h"

#include "iARedirectVtkOutput.h"

#include <QApplication>
#include <QDate>

#include <vtkSmartPointer.h>

int main(int argc, char *argv[])
{
	vtkSmartPointer<iARedirectVtkOutput> myOutputWindow = vtkSmartPointer<iARedirectVtkOutput>::New();
	vtkOutputWindow::SetInstance(myOutputWindow);

	MainWindow::InitResources();
	QApplication app(argc, argv);
	app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

	MainWindow mainWin("open_iA", VERSION, ":/images/splashscreen.png");

	iAConsole::GetInstance();				// (workaround) for binding log instance to GUI thread

	if (argc > 1) mainWin.loadFile(QString(argv[1]));

	app.setWindowIcon(QIcon(QPixmap(":/images/ia.png")));
	mainWin.setWindowIcon(QIcon(QPixmap(":/images/ia.png")));

	if( QDate::currentDate().dayOfYear() >= 340 ) {
		mainWin.setWindowTitle("Merry X-Mas and a happy new year!");
		mainWin.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
		app.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
	}

	mainWin.show();

	CheckSCIFIO(argv[0]);

	return app.exec();
}
