/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFoamCharacterizationDialogFilter.h"

#include "iAFoamCharacterizationItemFilter.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

iAFoamCharacterizationDialogFilter::iAFoamCharacterizationDialogFilter
                                                               (iAFoamCharacterizationItemFilter* _pItemFilter, QWidget* _pParent)
	                                                                        : iAFoamCharacterizationDialog(_pItemFilter, _pParent)
	                                                                        , m_pItemFilter(_pItemFilter)
{
	m_pGroupBox2 = new QGroupBox(this);

	QLabel* pLabel2(new QLabel("Type:", m_pGroupBox2));

	m_pComboBox2 = new QComboBox(m_pGroupBox2);
	m_pComboBox2->addItem("Anisotropic diffusion", 0);
	m_pComboBox2->addItem("Gaussian", 1);
	m_pComboBox2->addItem("Median", 2);
	m_pComboBox2->addItem("Non-local means", 3);
	m_pComboBox2->setWhatsThis("Choose the filter type.");
	connect(m_pComboBox2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAFoamCharacterizationDialogFilter::slotComboBox2);

	m_pWidgetAnisotropic = new QWidget(m_pGroupBox2);

	QLabel* pLabelAnisotropicIteration (new QLabel("Iteration:", m_pWidgetAnisotropic));
	m_pSpinBoxAnisotropicIteration = new QSpinBox(m_pWidgetAnisotropic);
	m_pSpinBoxAnisotropicIteration->setAlignment(Qt::AlignRight);
	m_pSpinBoxAnisotropicIteration->setRange(1, 1000);
	m_pSpinBoxAnisotropicIteration->setValue(m_pItemFilter->anisotropicIteration());
	m_pSpinBoxAnisotropicIteration->setWhatsThis ( "Specifies the number of iterations "
		                                           "(time-step updates) that the solver will perform to produce a solution image."
		                                           " The appropriate number of iterations is dependent on the application and the"
		                                           " image being processed. As a general rule, the more iterations performed, the"
		                                           " more diffused the image will become."
	                                             );

	QLabel* pLabelAnisotropicTimeStep(new QLabel("Time step:", m_pWidgetAnisotropic));
	m_pDoubleSpinBoxAnisotropicTimeStep = new QDoubleSpinBox(m_pWidgetAnisotropic);
	m_pDoubleSpinBoxAnisotropicTimeStep->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxAnisotropicTimeStep->setRange(0.0, 100.0);
	m_pDoubleSpinBoxAnisotropicTimeStep->setSingleStep(0.1);
	m_pDoubleSpinBoxAnisotropicTimeStep->setValue(m_pItemFilter->anisotropicTimeStep());
	m_pDoubleSpinBoxAnisotropicTimeStep->setWhatsThis ( "Sets the time step to be used for each iteration "
		                                                "(update). This parameter is described in detail in "
		                                                "itkAnisotropicDiffusionFunction. The time step is constrained at "
		                                                "run-time to keep the solution stable. In general, the time step should "
		                                                "be at or below $(PixelSpacing)/2^{N+1}$, where $N$ is the dimensionality"
		                                                " of the image."
	                                                  );

	QLabel* pLabelAnisotropicConductance(new QLabel("Conductance:", m_pWidgetAnisotropic));
	m_pDoubleSpinBoxAnisotropicConductance = new QDoubleSpinBox(m_pWidgetAnisotropic);
	m_pDoubleSpinBoxAnisotropicConductance->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxAnisotropicConductance->setRange(0.0, 1000.0);
	m_pDoubleSpinBoxAnisotropicConductance->setValue(m_pItemFilter->anisotropicConductance());
	m_pDoubleSpinBoxAnisotropicConductance->setWhatsThis ( "Set a common parameter used by subclasses"
		                                                   " of itkAnisotropicDiffusionFunction. See "
		                                                   "itkAnisotropicDiffusionFunction for detailed information."
														 );

	QGridLayout* pGridLayoutAnisotropic(new QGridLayout(m_pWidgetAnisotropic));
	pGridLayoutAnisotropic->addWidget(pLabelAnisotropicIteration, 0, 0);
	pGridLayoutAnisotropic->addWidget(m_pSpinBoxAnisotropicIteration, 0, 1);
	pGridLayoutAnisotropic->addWidget(pLabelAnisotropicTimeStep, 1, 0);
	pGridLayoutAnisotropic->addWidget(m_pDoubleSpinBoxAnisotropicTimeStep, 1, 1);
	pGridLayoutAnisotropic->addWidget(pLabelAnisotropicConductance, 2, 0);
	pGridLayoutAnisotropic->addWidget(m_pDoubleSpinBoxAnisotropicConductance, 2, 1);

	m_pWidgetGauss = new QWidget(m_pGroupBox2);
	m_pWidgetGauss->setVisible(false);

	QLabel* pLabelGaussVariance(new QLabel("Variance:", m_pWidgetGauss));
	m_pDoubleSpinBoxGaussVariance = new QDoubleSpinBox(m_pWidgetMedian);
	m_pDoubleSpinBoxGaussVariance->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxGaussVariance->setRange(0.0, 100.0);
	m_pDoubleSpinBoxGaussVariance->setValue(m_pItemFilter->gaussianVariance());
	m_pDoubleSpinBoxGaussVariance->setWhatsThis ( "The variance or standard deviation (sigma) will be evaluated as pixel units "
		                                          "if use image spacing is off (false) or as physical units if "
		                                          "use image spacing is on (true, default). The variance can be set "
		                                          "independently in each dimension."
												);

	m_pCheckBoxImageSpacing = new QCheckBox("Use image spacing", m_pWidgetGauss);
	m_pCheckBoxImageSpacing->setChecked(m_pItemFilter->gaussianImageSpacing());

	QGridLayout* pGridLayoutGauss(new QGridLayout(m_pWidgetGauss));
	pGridLayoutGauss->addWidget(pLabelGaussVariance, 0, 0);
	pGridLayoutGauss->addWidget(m_pDoubleSpinBoxGaussVariance, 0, 1);
	pGridLayoutGauss->addWidget(m_pCheckBoxImageSpacing, 1, 0);

	m_pWidgetMedian = new QWidget(m_pGroupBox2);
	m_pWidgetMedian->setVisible(false);

	QLabel* pLabelMedianBoxRadius(new QLabel("Radius:", m_pWidgetMedian));
	m_pSpinBoxMedianBoxRadius = new QSpinBox(m_pWidgetMedian);
	m_pSpinBoxMedianBoxRadius->setAlignment(Qt::AlignRight);
	m_pSpinBoxMedianBoxRadius->setRange(1, 100);
	m_pSpinBoxMedianBoxRadius->setValue(m_pItemFilter->medianRadius());
	m_pSpinBoxMedianBoxRadius->setWhatsThis("Dimensional Kernel radius.");

	QGridLayout* pGridLayoutMedian(new QGridLayout(m_pWidgetMedian));
	pGridLayoutMedian->addWidget(pLabelMedianBoxRadius, 0, 0);
	pGridLayoutMedian->addWidget(m_pSpinBoxMedianBoxRadius, 0, 1);

	m_pWidgetNonLocalMeans = new QWidget(m_pGroupBox2);
	m_pWidgetNonLocalMeans->setVisible(false);

	QLabel* pLabelNonLocalMeansIteration(new QLabel("Iteration:", m_pWidgetNonLocalMeans));
	m_pSpinBoxNonLocalMeansIteration = new QSpinBox(m_pWidgetNonLocalMeans);
	m_pSpinBoxNonLocalMeansIteration->setAlignment(Qt::AlignRight);
	m_pSpinBoxNonLocalMeansIteration->setRange(1, 100);
	m_pSpinBoxNonLocalMeansIteration->setValue(m_pItemFilter->nonLocalMeansIteration());
	m_pSpinBoxNonLocalMeansIteration->setWhatsThis ( "Set the number of denoising iterations to perform. Must be a positive "
		                                             "integer. Defaults to 1."
	                                               );

	QLabel* pLabelNonLocalMeansRadius(new QLabel("Radius:", m_pWidgetNonLocalMeans));
	m_pSpinBoxNonLocalMeansRadius = new QSpinBox(m_pWidgetNonLocalMeans);
	m_pSpinBoxNonLocalMeansRadius->setAlignment(Qt::AlignRight);
	m_pSpinBoxNonLocalMeansRadius->setRange(1, 100);
	m_pSpinBoxNonLocalMeansRadius->setValue(m_pItemFilter->nonLocalMeansRadius());
	m_pSpinBoxNonLocalMeansRadius->setWhatsThis ( "Set the patch radius specified in physical coordinates.Patch radius is "
												  "preferably set to an even number.Currently, only isotropic patches in "
												  "physical space are allowed; patches can be anisotropic in voxel space."
												);

	QGridLayout* pGridLayoutNonLocalMeans(new QGridLayout(m_pWidgetNonLocalMeans));
	pGridLayoutNonLocalMeans->addWidget(pLabelNonLocalMeansIteration, 0, 0);
	pGridLayoutNonLocalMeans->addWidget(m_pSpinBoxNonLocalMeansIteration, 0, 1);
	pGridLayoutNonLocalMeans->addWidget(pLabelNonLocalMeansRadius, 1, 0);
	pGridLayoutNonLocalMeans->addWidget(m_pSpinBoxNonLocalMeansRadius, 1, 1);

	QGridLayout* pGridLayout2(new QGridLayout(m_pGroupBox2));
	pGridLayout2->addWidget(pLabel2, 0, 0);
	pGridLayout2->addWidget(m_pComboBox2, 0, 1);
	pGridLayout2->addWidget(m_pWidgetAnisotropic, 1, 0, 1, 2);
	pGridLayout2->addWidget(m_pWidgetGauss, 1, 0, 1, 2);
	pGridLayout2->addWidget(m_pWidgetMedian, 1, 0, 1, 2);
	pGridLayout2->addWidget(m_pWidgetNonLocalMeans, 1, 0, 1, 2);

	setLayout();

	m_pComboBox2->setCurrentIndex((int)m_pItemFilter->itemFilterType());
}

void iAFoamCharacterizationDialogFilter::slotComboBox2(const int& _iIndex)
{
	if (_iIndex == 0)
	{
		m_pWidgetAnisotropic->setVisible(true);
		m_pWidgetGauss->setVisible(false);
		m_pWidgetMedian->setVisible(false);
		m_pWidgetNonLocalMeans->setVisible(false);
	}
	else if (_iIndex == 1)
	{
		m_pWidgetAnisotropic->setVisible(false);
		m_pWidgetGauss->setVisible(true);
		m_pWidgetMedian->setVisible(false);
		m_pWidgetNonLocalMeans->setVisible(false);
	}
	else if (_iIndex == 2)
	{
		m_pWidgetAnisotropic->setVisible(false);
		m_pWidgetGauss->setVisible(false);
		m_pWidgetMedian->setVisible(true);
		m_pWidgetNonLocalMeans->setVisible(false);
	}
	else
	{
		m_pWidgetAnisotropic->setVisible(false);
		m_pWidgetGauss->setVisible(false);
		m_pWidgetMedian->setVisible(false);
		m_pWidgetNonLocalMeans->setVisible(true);
	}
}

void iAFoamCharacterizationDialogFilter::slotPushButtonOk()
{
	m_pItemFilter->setItemFilterType((iAFoamCharacterizationItemFilter::EItemFilterType) m_pComboBox2->currentIndex());

	m_pItemFilter->setAnisotropicConductance(m_pDoubleSpinBoxAnisotropicConductance->value());
	m_pItemFilter->setAnisotropicIteration(m_pSpinBoxAnisotropicIteration->value());
	m_pItemFilter->setAnisotropicTimeStep(m_pDoubleSpinBoxAnisotropicTimeStep->value());

	m_pItemFilter->setGaussianImageSpacing(m_pCheckBoxImageSpacing->isChecked());
	m_pItemFilter->setGaussianVariance(m_pDoubleSpinBoxGaussVariance->value());

	m_pItemFilter->setMedianRadius(m_pSpinBoxMedianBoxRadius->value());

	m_pItemFilter->setNonLocalMeansIteration(m_pSpinBoxNonLocalMeansIteration->value());
	m_pItemFilter->setNonLocalMeansRadius(m_pSpinBoxNonLocalMeansRadius->value());

	iAFoamCharacterizationDialog::slotPushButtonOk();
}
