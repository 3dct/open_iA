/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
 
#include "dlg_TripleHistogramTF.h"

// TODO: should I include this here?
#include "iAModalityWidget.h"
#include "iABarycentricTriangleWidget.h"

dlg_TripleHistogramTF::dlg_TripleHistogramTF(MdiChild * mdiChild /*= 0*/, Qt::WindowFlags f /*= 0 */) :
	TripleHistogramTFConnector(mdiChild, f)
{
	QWidget *histogramStackContainer = new QWidget();
	
	/*QLabel *modality1 = new QLabel(histogramStackContainer);
	modality1->setText("Modality 1");

	QLabel *modality2 = new QLabel(histogramStackContainer);
	modality2->setText("Modality 2");

	QLabel *modality3 = new QLabel(histogramStackContainer);
	modality3->setText("Modality 3");*/

	iAModalityWidget *modality1 = new iAModalityWidget(histogramStackContainer, mdiChild);
	iAModalityWidget *modality2 = new iAModalityWidget(histogramStackContainer, mdiChild);
	iAModalityWidget *modality3 = new iAModalityWidget(histogramStackContainer, mdiChild);

	QVBoxLayout *histogramStackContainerLayout = new QVBoxLayout(histogramStackContainer);
	histogramStackContainerLayout->addWidget(modality1);
	histogramStackContainerLayout->addWidget(modality2);
	histogramStackContainerLayout->addWidget(modality3);

	iABarycentricTriangleWidget *triangle = new iABarycentricTriangleWidget(dockWidgetContents);

	QLayout *mainLayout = dockWidgetContents->layout();
	mainLayout->addWidget(histogramStackContainer);
	mainLayout->addWidget(triangle);
}

dlg_TripleHistogramTF::~dlg_TripleHistogramTF()
{}