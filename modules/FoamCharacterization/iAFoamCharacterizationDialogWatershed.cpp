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
#include "iAFoamCharacterizationDialogWatershed.h"

#include "iAFoamCharacterizationItemWatershed.h"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>

iAFoamCharacterizationDialogWatershed::iAFoamCharacterizationDialogWatershed
                                                         (iAFoamCharacterizationItemWatershed* _pItemWatershed, QWidget* _pParent)
	                                                                     : iAFoamCharacterizationDialog(_pItemWatershed, _pParent)
																		 , m_pItemWatershed(_pItemWatershed)
{
	m_pGroupBox2 = new QGroupBox(this);

	QLabel* pLabelLevel (new QLabel("Level [%]:", m_pGroupBox2));
	m_pDoubleSpinBoxLevel = new QDoubleSpinBox(m_pGroupBox2);
	m_pDoubleSpinBoxLevel->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxLevel->setRange(0.0, 1.0);
	m_pDoubleSpinBoxLevel->setSingleStep(0.1);
	m_pDoubleSpinBoxLevel->setValue(m_pItemWatershed->level());
	m_pDoubleSpinBoxLevel->setWhatsThis("The Level parameter controls the depth of metaphorical flooding of the image. "
		"That is, it sets the maximum saliency value of interest in the result. Raising "
		"and lowering the Level influences the number of segments in the basic segmentation "
		"that are merged to produce the final output. A level of 1.0 is analogous to "
		"flooding the image up to a depth that is 100 percent of the maximum value in the "
		"image. A level of 0.0 produces the basic segmentation, which will typically be "
		"very oversegmented. Level values of interest are typically low (i.e. less than "
		"about 0.40 or 40% ), since higher values quickly start to undersegment the image."
		"The Level parameter can be used to create a hierarchy of output images in "
		"constant time once an initial segmentation is done.A typical scenario might go "
		"like this: For the initial execution of the filter, set the Level to the maximum "
		"saliency value that you anticipate might be of interest."
	);

	QLabel* pLabelThreshold(new QLabel("Threshold [%]:", m_pGroupBox2));
	m_pDoubleSpinBoxTreshold = new QDoubleSpinBox(m_pGroupBox2);
	m_pDoubleSpinBoxTreshold->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxTreshold->setRange(0.0, 1.0);
	m_pDoubleSpinBoxTreshold->setSingleStep(0.1);
	m_pDoubleSpinBoxTreshold->setValue(m_pItemWatershed->threshold());
	m_pDoubleSpinBoxTreshold->setWhatsThis ( "Threshold is used to set the absolute minimum height value used during processing. "
		                                     "Raising this threshold percentage effectively decreases the number of local minima "
		                                     "in the input, resulting in an initial segmentation with fewer regions. The "
		                                     "assumption is that the shallow regions that thresholding removes are of of less "
		                                     "interest."
	                                       );

	QGridLayout* pGridLayout2(new QGridLayout(m_pGroupBox2));
	pGridLayout2->addWidget(pLabelLevel, 0, 0);
	pGridLayout2->addWidget(m_pDoubleSpinBoxLevel, 0, 1);
	pGridLayout2->addWidget(pLabelThreshold, 1, 0);
	pGridLayout2->addWidget(m_pDoubleSpinBoxTreshold, 1, 1);

	setLayout();
}

void iAFoamCharacterizationDialogWatershed::slotPushButtonOk()
{
	m_pItemWatershed->setLevel(m_pDoubleSpinBoxLevel->value());
	m_pItemWatershed->setThreshold(m_pDoubleSpinBoxTreshold->value());

	iAFoamCharacterizationDialog::slotPushButtonOk();
}
