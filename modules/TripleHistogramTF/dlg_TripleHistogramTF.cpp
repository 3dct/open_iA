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

// TODO: why tf do I need this?
#include "iAModalityList.h"

dlg_TripleHistogramTF::dlg_TripleHistogramTF(MdiChild * mdiChild /*= 0*/, Qt::WindowFlags f /*= 0 */) :
	TripleHistogramTFConnector(mdiChild, f)
{
	QWidget *histogramStackContainer = new QWidget();
	QVBoxLayout *histogramStackContainerLayout = new QVBoxLayout(histogramStackContainer);

	iAModalityWidget *modality1, *modality2, *modality3;
	// TODO: load 3 DIFFERENT modalities
	if (mdiChild->GetModalities()->size() > 0)
	{
		iAModalityWidget *modalities[3];
		int i = 0;
		for (int j = 0; j < 3/*mdiChild->GetModalities()->size()*/; ++j)
		{
			modalities[j] = new iAModalityWidget(histogramStackContainer, mdiChild->GetModality(i), mdiChild);
			histogramStackContainerLayout->addWidget(modalities[j]);
		}
		modality1 = modalities[0];
		modality2 = modalities[1];
		modality3 = modalities[2];
	}

	iABarycentricTriangleWidget *triangle = new iABarycentricTriangleWidget(dockWidgetContents);
	
	// TODO: I'm confused here with the references and pointers
	connect(
		triangle, &iABarycentricTriangleWidget::weightChanged,
		[=](const BCoord bCoord)
		{
			modality1->setWeight(bCoord.getAlpha());
			modality2->setWeight(bCoord.getBeta());
			modality3->setWeight(bCoord.getGamma());
		}
	);

	QLayout *mainLayout = dockWidgetContents->layout();
	mainLayout->addWidget(histogramStackContainer);
	mainLayout->addWidget(triangle);
}

dlg_TripleHistogramTF::~dlg_TripleHistogramTF()
{}