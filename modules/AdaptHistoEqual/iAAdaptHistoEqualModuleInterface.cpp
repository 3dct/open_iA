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
#include "iAAdaptHistoEqualModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAAdaptiveHistogramEqualization.h"
#include "mainwindow.h"
#include "mdichild.h"

void iAAdaptHistoEqualModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QAction * actionAdaptive_Histogram_Equalization = new QAction( m_mainWnd );
	actionAdaptive_Histogram_Equalization->setText( QApplication::translate( "MainWindow", "Adaptive Histogram Equalization", 0 ) );
	AddActionToMenuAlphabeticallySorted(filtersMenu,  actionAdaptive_Histogram_Equalization );
	connect( actionAdaptive_Histogram_Equalization, SIGNAL( triggered() ), this, SLOT( adaptive_Histogram_Equalization() ) );
}

void iAAdaptHistoEqualModuleInterface::adaptive_Histogram_Equalization()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Alpha" ) << tr( "#Beta" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( aheAlpha ) << tr( "%1" ).arg( aheBeta );
	QTextDocument *fDescr = new QTextDocument( 0 );
	fDescr->setHtml(
		"<p><font size=+1>Histogram equalization modifies the contrast in an image.</font></p>"
		"<p>The AdaptiveHistogramEqualizationImageFilter is a superset of many contrast enhancing filters. By modifying its parameters"
		"(alpha, beta), the AdaptiveHistogramEqualizationImageFilter can produce an adaptively equalized histogram or a version of"
		"unsharp mask (local mean subtraction).</p>"
		"<p>The parameter alpha controls how much the filter acts like the classical histogram equalization method (alpha=0)"
		"to how much the filter acts like an unsharp mask (alpha=1).</p>"
		"<p>The parameter beta controls how much the filter acts like an unsharp mask (beta=0) to much the filter acts like pass"
		"through (beta=1, with alpha=1).</p>" );
	dlg_commoninput dlg( m_mainWnd, "Parameters for the Adaptive Histogram Equalization", 2, inList, inPara, fDescr );
	if( dlg.exec() != QDialog::Accepted )
		return;
	aheAlpha = dlg.getValues()[0];
	aheBeta = dlg.getValues()[1];
	//prepare
	QString filterName = "Adaptive Histogram Equalization";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	m_mdiChild->addStatusMsg( filterName );
	iAAdaptiveHistogramEqualization* thread = new iAAdaptiveHistogramEqualization( filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setCParameters( aheAlpha, aheBeta );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}
