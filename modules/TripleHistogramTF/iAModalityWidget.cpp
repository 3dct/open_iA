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
 
#include "iAModalityWidget.h"

#include <QHBoxLayout>
#include <QLabel>

#include "iAModalityList.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "charts/iAHistogramData.h"
#include "charts/iADiagramFctWidget.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAProfileWidget.h"
#include "iAPreferences.h"

iAModalityWidget::iAModalityWidget(QWidget * parent, MdiChild * mdiChild, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f)
{
	QWidget *rightWidget = new QWidget(this);

	QLabel *slicer = new QLabel(rightWidget);
	slicer->setText("Slicer");

	QLabel *weight = new QLabel(rightWidget);
	weight->setText("Weight");

	QVBoxLayout *rightWidgetLayout = new QVBoxLayout(rightWidget);
	rightWidgetLayout->addWidget(slicer);
	rightWidgetLayout->addWidget(weight);

	//QLabel *histogram = new QLabel(this);
	//histogram->setText("Histogram");

	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	//mainLayout->addWidget(histogram);

	// TODO: move
	if (mdiChild->GetModalities()->size() > 0)
	{
		for (int i = 0; i < mdiChild->GetModalities()->size(); ++i)
		{
			if (!mdiChild->GetModality(i)->GetHistogramData() || mdiChild->GetModality(i)->GetHistogramData()->GetNumBin() != mdiChild->GetPreferences().HistogramBins)
			{
				mdiChild->GetModality(i)->ComputeImageStatistics();
				mdiChild->GetModality(i)->ComputeHistogramData(mdiChild->GetPreferences().HistogramBins);
			}

			iADiagramFctWidget* histogram = new iADiagramFctWidget(0, mdiChild);
			QSharedPointer<iAPlot> histogramPlot = QSharedPointer<iAPlot>(
				new	iABarGraphDrawer(mdiChild->GetModality(i)->GetHistogramData(), QColor(70, 70, 70, 255)));
			histogram->AddPlot(histogramPlot);
			histogram->SetTransferFunctions(mdiChild->GetModality(i)->GetTransfer()->GetColorFunction(),
				mdiChild->GetModality(i)->GetTransfer()->GetOpacityFunction());
			mainLayout->addWidget(histogram);
			histogram->updateTrf();
		}
	}

	mainLayout->addWidget(rightWidget);
}

iAModalityWidget::~iAModalityWidget()
{}