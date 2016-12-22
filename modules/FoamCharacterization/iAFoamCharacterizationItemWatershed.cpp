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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#include "iAFoamCharacterizationItemWatershed.h"

#include <QApplication>
#include <QPainter>

#include "iAFoamCharacterizationDialogWatershed.h"

iAFoamCharacterizationItemWatershed::iAFoamCharacterizationItemWatershed()
	                                                         : iAFoamCharacterizationItem(iAFoamCharacterizationItem::itWatershed)
{
	QScopedPointer<QImage> pImage(new QImage(32, 32, QImage::Format_ARGB32));
	pImage->fill(0);

	QScopedPointer<QPainter> pPainter(new QPainter(pImage.data()));
	pPainter->setBrush(Qt::blue);
	pPainter->setPen(Qt::blue);
	pPainter->drawEllipse(pImage->rect().adjusted(0, 0, -1, -1));

	setIcon(QIcon(QPixmap::fromImage(*pImage.data())));

	setText("Watershed");
}

iAFoamCharacterizationItemWatershed::iAFoamCharacterizationItemWatershed(iAFoamCharacterizationItemWatershed* _pWatershed)
	                                                         : iAFoamCharacterizationItem(iAFoamCharacterizationItem::itWatershed)
{
	QScopedPointer<QImage> pImage(new QImage(32, 32, QImage::Format_ARGB32));
	pImage->fill(0);

	QScopedPointer<QPainter> pPainter(new QPainter(pImage.data()));
	pPainter->setBrush(Qt::blue);
	pPainter->setPen(Qt::blue);
	pPainter->drawEllipse(pImage->rect().adjusted(0, 0, -1, -1));

	setIcon(QIcon(QPixmap::fromImage(*pImage.data())));

	setText(_pWatershed->text());
}

void iAFoamCharacterizationItemWatershed::dialog()
{
	QScopedPointer<iAFoamCharacterizationDialogWatershed> pDialog
	                                                      (new iAFoamCharacterizationDialogWatershed(this, qApp->focusWidget()));
	pDialog->exec();
}

void iAFoamCharacterizationItemWatershed::execute()
{

}
