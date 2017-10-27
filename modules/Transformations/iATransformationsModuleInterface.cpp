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
#include "iATransformations.h"

#include "iAFilterRegistry.h"

#include "dlg_commoninput.h"
#include "mainwindow.h"
#include "mdichild.h"

void iATransformationsModuleInterface::Initialize()
{
	REGISTER_FILTER(iARotate);
	REGISTER_FILTER(iAPermuteAxes);

	if (!m_mainWnd)
		return;
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuTransformations = getMenuWithTitle(filtersMenu, QString( "Transformations" ) );
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
	QAction * action_transforms_translate = new QAction(QApplication::translate("MainWindow", "Translate", 0), m_mainWnd);
	
	menuTransformations->addAction( action_transforms_translate );
	
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
	qreal tx = -dlg.getDblValue(k++);
	qreal ty = -dlg.getDblValue(k++);
	qreal tz = -dlg.getDblValue(k++);

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
