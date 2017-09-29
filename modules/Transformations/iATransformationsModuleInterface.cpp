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
#include "pch.h"
#include "iATransformationsModuleInterface.h"

#include "dlg_commoninput.h"
#include "dlg_gaussian.h"
#include "dlg_bezier.h"
#include "dlg_function.h"
#include "iAHistogramWidget.h"
#include "iATransformations.h"
#include "mainwindow.h"
#include "mdichild.h"

void iATransformationsModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuTransformations = getMenuWithTitle(filtersMenu, QString( "Transformations" ) );

	//permute axis type
	const QString order[] = { "XZY", "YXZ", "YZX", "ZXY", "ZYX" };
	const int Npermute = sizeof(order) / sizeof(QString);
	for (int k = 0; k < Npermute; k++)
	{
		QAction * actionPermute = new QAction(QApplication::translate("MainWindow", "Coordinate ", 0) + order[k], m_mainWnd);
		actionPermute->setData(order[k]);
		menuTransformations->addAction(actionPermute);

		connect(actionPermute, SIGNAL(triggered()), this, SLOT(permute()));
	}
	menuTransformations->addSeparator();

	//flip axis
	const QChar flipLabel[] = { 'X', 'Y', 'Z' };
	const int NAxes = sizeof(flipLabel) / sizeof(QChar);
	for (int k = 0; k < NAxes; k++)
	{
		QAction * actionFlip = new QAction(QApplication::translate("MainWindow", "Flip axes ", 0) + flipLabel[k], m_mainWnd);
		actionFlip->setData(flipLabel[k]);
		menuTransformations->addAction(actionFlip);

		connect(actionFlip, SIGNAL(triggered()), this, SLOT(flip()));
	}
	menuTransformations->addSeparator();

	QAction * action_transforms_rotate = new QAction(QApplication::translate("MainWindow", "Rotate", 0), m_mainWnd);
	QAction * action_transforms_translate = new QAction(QApplication::translate("MainWindow", "Translate", 0), m_mainWnd);
	
	menuTransformations->addAction( action_transforms_rotate );
	menuTransformations->addAction( action_transforms_translate );
	
	connect( action_transforms_rotate, SIGNAL( triggered() ), this, SLOT( rotate() ) );
	connect( action_transforms_translate, SIGNAL( triggered() ), this, SLOT( translate() ) );
}

vtkImageData * iATransformationsModuleInterface::prepare(const QString & caption)
{
	MdiChild * actChild = m_mainWnd->activeMdiChild();
	if (actChild == NULL)
		return NULL;
	PrepareResultChild(caption);
	m_mdiChild->addStatusMsg(caption);
	m_mainWnd->statusBar()->showMessage(caption, 5000);
	return m_childData.imgData;
}

void iATransformationsModuleInterface::rotate()
{
	const iATransformations::RotationAxesType axes[] = { iATransformations::RotateAlongX, iATransformations::RotateAlongY, iATransformations::RotateAlongZ };
	const iATransformations::RotationCenterType center[] = { iATransformations::RCCenter, iATransformations::RCOrigin, iATransformations::RCCustom };
	QStringList rotAxes = QStringList() << tr("Rotation along X") << tr("Rotation along Y") << tr("Rotation along Z");
	QStringList rotCenter = QStringList() << tr("Image center") << tr("Origin") << tr("Specify coordinate");
	QStringList inList = (QStringList()
		<< tr("#Rotation angle (deg)")) << tr("+Rotation axes") << tr("+Rotation center")
		<< tr("#Center X") << tr("#Center Y") << tr("#Center Z");
	QList<QVariant> inPara; 	inPara
		<< 0 << rotAxes << rotCenter << 0 << 0 << 0;

	dlg_commoninput dlg(m_mainWnd, tr("Rotation parameters"), inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	//get rot axes
	int k = 0;
	QList<qreal> values = dlg.getValues();

	qreal rotAngle = values[k++];
	int rotAxesIdx = dlg.getComboBoxIndex(k++);
	int rotCenterIdx = dlg.getComboBoxIndex(k++);
	qreal cx = values[k++];
	qreal cy = values[k++];
	qreal cz = values[k++];

	QString filterName = "Rotated";
	vtkImageData * inpImage = prepare(filterName);
	if (inpImage == NULL)
	{
		return;
	}
	iATransformations * thread = new iATransformations(filterName,
		inpImage, NULL, m_mdiChild->getLogger(), m_mdiChild);
	thread->setTransformationType(iATransformations::Rotation);
	thread->setRotationCenterCoordinate(cx, cy, cz);
	thread->setRotationAngle(rotAngle);
	thread->setRotationAxes(axes[rotAxesIdx]);
	thread->setRotationCenter(center[rotCenterIdx]);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->start();
}

void iATransformationsModuleInterface::translate()
{
	QStringList inList = QStringList()
		<< tr("#Translate X") << tr("#Translate Y") << tr("#Translate Z");
	QList<QVariant> inPara; 	
	inPara << 0 << 0 << 0;

	dlg_commoninput dlg(m_mainWnd, tr("Translation parameters"), inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	//get translation
	//here set to be minus, because itk translate the origin
	//of the coordinate system, hence opposite to the image translation
	int k = 0;
	QList<qreal> values = dlg.getValues();
	qreal tx = -values[k++];
	qreal ty = -values[k++];
	qreal tz = -values[k++];


	QString filterName = "Translated";
	vtkImageData * inpImage = prepare(filterName);
	if (inpImage == NULL)
	{
		return;
	}
	iATransformations * thread = new iATransformations(filterName,
		inpImage, NULL, m_mdiChild->getLogger(), m_mdiChild);
	thread->setTransformationType(iATransformations::Translation);
	thread->setTranslation(tx, ty, tz);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->start();
}

void iATransformationsModuleInterface::flip()
{
	QAction * action = qobject_cast<QAction *>(QObject::sender());
	if (action == NULL)
		return;
	QChar flipAxes = action->data().toChar();

	QString filterName = "Flipped axis " + QString(flipAxes);
	vtkImageData * inpImage = prepare(filterName);
	if (inpImage == NULL)
	{
		return;
	}
	iATransformations * thread = new iATransformations(filterName,
		inpImage, NULL, m_mdiChild->getLogger(), m_mdiChild);
	thread->setFlipAxes(flipAxes);
	thread->setTransformationType(iATransformations::Flip);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->start();
}

void iATransformationsModuleInterface::permute()
{
	QAction * action = qobject_cast<QAction *>(QObject::sender());
	if (action == NULL)
		return;
	QString order = action->data().toString();
	QString filterName = "Changed coordinate " + order;
	vtkImageData * inpImage = prepare(filterName);
	if (inpImage == NULL)
	{
		return;
	}
	iATransformations * thread = new iATransformations(filterName,
		inpImage, NULL, m_mdiChild->getLogger(), m_mdiChild);
	thread->setPermuteAxesOrder(order);
	thread->setTransformationType(iATransformations::PermuteAxes);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->start();
}
