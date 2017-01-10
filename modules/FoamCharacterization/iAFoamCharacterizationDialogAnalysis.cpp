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

#include "iAFoamCharacterizationDialogAnalysis.h"

#include<QApplication>
#include<QGroupBox>
#include<QGridLayout>
#include<QLabel>
#include<QStyle>
#include<QTime>
#include<QPushButton>

#include "iAConnector.h"

#include "itkLabelGeometryImageFilter.h"
#include "vtkImageData.h"

iAFoamCharacterizationDialogAnalysis::iAFoamCharacterizationDialogAnalysis(vtkImageData* _pImageData, QWidget* _pParent)
	                                                           : QDialog(_pParent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
															   , m_pImageData (_pImageData)
{
	setWindowTitle("Analysis");

	analyse();

	QGroupBox* pGroupBox1(new QGroupBox(this));

	QLabel* pLabel11(new QLabel("Number of bubbles:", pGroupBox1));
	
	m_pLabel12 = new QLabel(QString("%1").arg(m_iBubbles), pGroupBox1);

	QGridLayout* pGridLayout1(new QGridLayout(pGroupBox1));
	pGridLayout1->addWidget(  pLabel11, 0, 0);
	pGridLayout1->addWidget(m_pLabel12, 0, 1);

	QPushButton* pPushButtonOk(new QPushButton("Ok", this));
	pPushButtonOk->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOkButton));
	connect(pPushButtonOk, SIGNAL(clicked()), this, SLOT(slotPushButtonOk()));

	QGridLayout* pGridLayout(new QGridLayout(this));
	pGridLayout->addWidget(pGroupBox1);
	pGridLayout->addWidget(pPushButtonOk);
}

void iAFoamCharacterizationDialogAnalysis::analyse()
{
	qApp->setOverrideCursor(Qt::WaitCursor);
	qApp->processEvents();

	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->SetImage(m_pImageData);

	typedef itk::LabelGeometryImageFilter<itk::Image<unsigned short, 3>> itkLabelGeometryImageFilterType;
	itkLabelGeometryImageFilterType::Pointer pLabelGeometryImageFilter(itkLabelGeometryImageFilterType::New());
	pLabelGeometryImageFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->GetITKImage()));
	pLabelGeometryImageFilter->Update();

	m_iBubbles = pLabelGeometryImageFilter->GetNumberOfLabels();

	qApp->restoreOverrideCursor();
}

void iAFoamCharacterizationDialogAnalysis::slotPushButtonOk()
{
	accept();
}
