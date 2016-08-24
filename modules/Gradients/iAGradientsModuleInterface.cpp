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
 
#include "pch.h"
#include "iAGradientsModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAGradients.h"
#include "mainwindow.h"
#include "mdichild.h"

HOAccGradientDerrivativeSettings HOAGDSettings;

void iAGradientsModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuGradients = getMenuWithTitle(filtersMenu, QString( "Gradients" ) );
	QAction * actionGradient_Magnitude = new QAction(QApplication::translate("MainWindow", "Gradient Magnitude", 0), m_mainWnd );
	QAction * actionDerivative = new QAction(QApplication::translate("MainWindow", "Derivative", 0), m_mainWnd );
	QAction * actionHgOrderDerivative = new QAction(QApplication::translate("MainWindow", "Higher Order Accurate Derivative", 0), m_mainWnd);
	menuGradients->addAction( actionGradient_Magnitude );
	menuGradients->addAction( actionDerivative );
	menuGradients->addAction( actionHgOrderDerivative );
	connect( actionGradient_Magnitude, SIGNAL( triggered() ), this, SLOT( gradientMagnitude() ) );
	connect( actionDerivative, SIGNAL( triggered() ), this, SLOT( derivative_Filter() ) );
	connect(actionHgOrderDerivative, SIGNAL(triggered()), this, SLOT(higherOrderDerivative()));
}

void iAGradientsModuleInterface::gradientMagnitude()
{
	//prepare
	QString filterName = tr( "Gradient Magnitude" );
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAGradients* thread = new iAGradients( filterName, GRADIENT_MAGNITUDE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAGradientsModuleInterface::derivative_Filter()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Order" ) << tr( "#Direction" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( dfOrder ) << tr( "%1" ).arg( dfDirection );
	dlg_commoninput dlg( m_mainWnd, "Derivative", 2, inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;

	dfOrder = dlg.getValues()[0];
	dfDirection = dlg.getValues()[1];

	//prepare
	QString filterName = tr( "Derivative image filter" );
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAGradients* thread = new iAGradients( filterName, DERIVATIVE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setDParameters( dfOrder, dfDirection );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAGradientsModuleInterface::higherOrderDerivative()
{
	//set parameters
	QStringList inList = (QStringList() << tr("#Order") << tr("#Direction") << tr("#LevelOfAccuracy"));
	QList<QVariant> inPara; 	
	inPara << tr("%1").arg(HOAGDSettings.order)\
		<< tr("%1").arg(HOAGDSettings.direction)\
		<< tr("%1").arg(HOAGDSettings.orderOfAcc);
	QTextDocument *fDescr = new QTextDocument();
	fDescr->setHtml(
		"<p><font size=+1>Calculate Higher Order Accurate Gradient Derivative.</font></p>"
		"<p>Computes the higher order accurate directional derivative of an image. The directional derivative at each pixel location is computed by convolution with a higher order accurate derivative operator of user-specified order.</p>");
	dlg_commoninput dlg(m_mainWnd, "Higher Order Accurate Gradient Derivative", 3, inList, inPara, fDescr);

	if (dlg.exec() != QDialog::Accepted)
		return;

	HOAGDSettings.order = dlg.getValues()[0];
	HOAGDSettings.direction = dlg.getValues()[1];
	HOAGDSettings.orderOfAcc = dlg.getValues()[2];

	//prepare
	QString filterName = tr("Higher Order Accurate Gradient Derivative Filter");
	PrepareResultChild(filterName);
	m_mdiChild->addStatusMsg(filterName);
	//execute
	iAGradients* thread = new iAGradients(filterName, HIGHER_ORDER_ACCURATE_DERIVATIVE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->setHOAGDParameters(&HOAGDSettings);
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}
