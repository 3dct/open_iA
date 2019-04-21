/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iATripleModalityWidget.h"

#include "iAHistogramAbstract.h"
#include "iABarycentricContextRenderer.h"
#include "iABarycentricTriangleWidget.h"
#include "iASimpleSlicerWidget.h"
#include "RightBorderLayout.h"

#include <charts/iADiagramFctWidget.h>
#include <iAModality.h>

#include <QComboBox>
#include <QSharedPointer>

iATripleModalityWidget::iATripleModalityWidget(QWidget * parent, MdiChild *mdiChild, Qt::WindowFlags f /*= 0 */) :
	iAMultimodalWidget(parent, mdiChild, THREE)
{
	m_triangleRenderer = new iABarycentricContextRenderer();
	m_triangleWidget = new iABarycentricTriangleWidget();

	m_triangleWidget->setTriangleRenderer(m_triangleRenderer);
	m_layoutComboBox = new QComboBox();
	m_layoutComboBox->addItem("Stack", iAHistogramAbstractType::STACK);
	m_layoutComboBox->addItem("Triangle", iAHistogramAbstractType::TRIANGLE);

	// Initialize the inner widget {
	m_mainLayout = new QHBoxLayout(this);
	setHistogramAbstractType(iAHistogramAbstractType::STACK);
	// }

	connect(m_layoutComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(layoutComboBoxIndexChanged(int)));
	connect(m_triangleWidget, SIGNAL(weightsChanged(BCoord)), this, SLOT(triangleWeightChanged(BCoord)));
	connect(m_triangleWidget, SIGNAL(weightsChanged(BCoord)), this, SLOT(weightsChangedSlot(BCoord)));

	connect(this, SIGNAL(modalitiesLoaded_beforeUpdate()), this, SLOT(modalitiesLoaded_beforeUpdateSlot()));
}

iATripleModalityWidget::~iATripleModalityWidget()
{
	delete m_triangleRenderer;
}

void iATripleModalityWidget::layoutComboBoxIndexChanged(int newIndex)
{
	setLayoutTypePrivate(getLayoutTypeAt(newIndex));
}

iAHistogramAbstractType iATripleModalityWidget::getLayoutTypeAt(int comboBoxIndex) {
	return (iAHistogramAbstractType)m_layoutComboBox->itemData(comboBoxIndex).toInt();
}

// SLOT
void iATripleModalityWidget::triangleWeightChanged(BCoord newWeight)
{
	setWeightsProtected(newWeight);
}

// SLOT
void iATripleModalityWidget::weightsChangedSlot(BCoord bCoord)
{
	if (bCoord != getWeights()) {
		m_triangleWidget->setWeight(bCoord);
	}
}

// SLOT
void iATripleModalityWidget::modalitiesLoaded_beforeUpdateSlot() {
	m_triangleWidget->setModalities(getModality(0)->GetImage(), getModality(1)->GetImage(), getModality(2)->GetImage());
	m_triangleWidget->update();
	m_histogramAbstract->initialize();
}

// PUBLIC SETTERS { -------------------------------------------------------------------------
void iATripleModalityWidget::setHistogramAbstractType(iAHistogramAbstractType type) {
	setLayoutTypePrivate(type);
	m_layoutComboBox->setCurrentIndex(m_layoutComboBox->findData(type));
}
// } ----------------------------------------------------------------------------------------

// PRIVATE SETTERS { ------------------------------------------------------------------------
void iATripleModalityWidget::setLayoutTypePrivate(iAHistogramAbstractType type) {
	if (m_histogramAbstract && type == m_histogramAbstractType) {
		return;
	}

	iAHistogramAbstract *histogramAbstract_new = iAHistogramAbstract::buildHistogramAbstract(type, this, m_mdiChild);

	if (m_histogramAbstract) {
		for (int i = 0; i < 3; i++) {
			w_histogram(i)->setParent(NULL);
			w_slicer(i)->setParent(NULL);
			resetSlicer(i);
		}
		m_triangleWidget->setParent(NULL);
		m_layoutComboBox->setParent(NULL);
		w_slicerModeComboBox()->setParent(NULL);
		w_sliceNumberSlider()->setParent(NULL);

		//delete m_histogramAbstract;
		m_mainLayout->replaceWidget(m_histogramAbstract, histogramAbstract_new, Qt::FindDirectChildrenOnly);

		delete m_histogramAbstract;
	} else {
		m_mainLayout->addWidget(histogramAbstract_new);
	}
	m_histogramAbstract = histogramAbstract_new;

	if (isReady()) {
		m_histogramAbstract->initialize();
	}
}
// } ----------------------------------------------------------------------------------------