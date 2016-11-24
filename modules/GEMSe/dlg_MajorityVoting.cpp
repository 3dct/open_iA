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
#include "dlg_MajorityVoting.h"

#include "dlg_GEMSe.h"
#include "iAColorTheme.h"
#include "iAConsole.h"
#include "iADockWidgetWrapper.h"
#include "iAImageTreeNode.h"
#include "iALookupTable.h"
#include "iAQSplom.h"
#include "iASingleResult.h"
#include "mdichild.h"

#include "ParametrizableLabelVotingImageFilter.h"
#include "FilteringLabelOverlapMeasuresImageFilter.h"

#include <itkLabelStatisticsImageFilter.h>

#include <vtkAxis.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkFloatArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPlot.h>
#include <vtkPlotLine.h>
#include <vtkTable.h>
#include <QVTKWidget2.h>

#include <QCheckBox>
#include <QMessageBox>

dlg_MajorityVoting::dlg_MajorityVoting(MdiChild* mdiChild, dlg_GEMSe* dlgGEMSe, int labelCount) :
	m_mdiChild(mdiChild),
	m_dlgGEMSe(dlgGEMSe),
	m_labelCount(labelCount),
	m_chartDiceVsUndec(vtkSmartPointer<vtkChartXY>::New()),
	m_chartValueVsDice(vtkSmartPointer<vtkChartXY>::New()),
	m_chartValueVsUndec(vtkSmartPointer<vtkChartXY>::New())
{
	QString defaultTheme("Brewer Set3 (max. 12)");
	m_colorTheme = iAColorThemeManager::GetInstance().GetTheme(defaultTheme);

	auto vtkWidget = new QVTKWidget2();
	auto contextView = vtkSmartPointer<vtkContextView>::New();
	contextView->SetRenderWindow(vtkWidget->GetRenderWindow());
	m_chartDiceVsUndec->SetSelectionMode(vtkContextScene::SELECTION_NONE);
	auto xAxis1 = m_chartDiceVsUndec->GetAxis(vtkAxis::BOTTOM);
	auto yAxis1 = m_chartDiceVsUndec->GetAxis(vtkAxis::LEFT);
	xAxis1->SetTitle("Undecided Pixels");
	xAxis1->SetLogScale(false);
	yAxis1->SetTitle("Accuracy (Dice)");
	yAxis1->SetLogScale(false);
	contextView->GetScene()->AddItem(m_chartDiceVsUndec);
	iADockWidgetWrapper * w(new iADockWidgetWrapper(vtkWidget, "Chart Dice vs. Undecided", "ChartDiceVsUndec"));
	mdiChild->splitDockWidget(this, w, Qt::Vertical);

	auto vtkWidget2 = new QVTKWidget2();
	auto contextView2 = vtkSmartPointer<vtkContextView>::New();
	contextView2->SetRenderWindow(vtkWidget2->GetRenderWindow());
	m_chartValueVsDice->SetSelectionMode(vtkContextScene::SELECTION_NONE);
	auto xAxis2 = m_chartValueVsDice->GetAxis(vtkAxis::BOTTOM);
	auto yAxis2 = m_chartValueVsDice->GetAxis(vtkAxis::LEFT);
	xAxis2->SetTitle("Threshold");
	xAxis2->SetLogScale(false);
	yAxis2->SetTitle("Accuracy(Dice)");
	yAxis2->SetLogScale(false);
	contextView2->GetScene()->AddItem(m_chartValueVsDice);
	iADockWidgetWrapper * w2(new iADockWidgetWrapper(vtkWidget2, "Chart Threshold vs. Dice", "ChartValueVsDice"));
	mdiChild->splitDockWidget(this, w2, Qt::Vertical);

	auto vtkWidget3 = new QVTKWidget2();
	auto contextView3 = vtkSmartPointer<vtkContextView>::New();
	contextView3->SetRenderWindow(vtkWidget3->GetRenderWindow());
	m_chartValueVsUndec->SetSelectionMode(vtkContextScene::SELECTION_NONE);
	auto xAxis3 = m_chartValueVsUndec->GetAxis(vtkAxis::BOTTOM);
	auto yAxis3 = m_chartValueVsUndec->GetAxis(vtkAxis::LEFT);
	xAxis3->SetTitle("Threshold");
	xAxis3->SetLogScale(false);
	yAxis3->SetTitle("Undecided Pixels");
	yAxis3->SetLogScale(false);
	contextView3->GetScene()->AddItem(m_chartValueVsUndec);
	iADockWidgetWrapper * w3(new iADockWidgetWrapper(vtkWidget3, "Chart Threshold vs. Undecided", "ChartValueVsUndec"));
	mdiChild->splitDockWidget(this, w3, Qt::Vertical);

	// repeat  for m_chartValueVsDice, m_chartValueVsUndec

	connect(pbSample, SIGNAL(clicked()), this, SLOT(Sample()));
	connect(pbMinAbsPercent_Plot, SIGNAL(clicked()), this, SLOT(MinAbsPlot()));
	connect(pbMinDiffPercent_Plot, SIGNAL(clicked()), this, SLOT(MinDiffPlot()));
	connect(pbMinRatio_Plot, SIGNAL(clicked()), this, SLOT(RatioPlot()));
	connect(pbMaxPixelEntropy_Plot, SIGNAL(clicked()), this, SLOT(MaxPixelEntropyPlot()));
	connect(slAbsMinPercent, SIGNAL(valueChanged(int)), this, SLOT(AbsMinPercentSlider(int)));
	connect(slMinDiffPercent, SIGNAL(valueChanged(int)), this, SLOT(MinDiffPercentSlider(int)));
	connect(slMinRatio, SIGNAL(valueChanged(int)), this, SLOT(MinRatioSlider(int)));
	connect(slMaxPixelEntropy, SIGNAL(valueChanged(int)), this, SLOT(MaxPixelEntropySlider(int)));
}

void dlg_MajorityVoting::SetGroundTruthImage(LabelImagePointer groundTruthImage)
{
	m_groundTruthImage = groundTruthImage;
}

iAITKIO::ImagePointer GetMajorityVotingImage(QVector<QSharedPointer<iASingleResult> > selection,
	double minAbsPercentage, double minDiffPercentage, double minRatio, double maxPixelEntropy,
	int labelCount)
{
	typedef ParametrizableLabelVotingImageFilter<LabelImageType> LabelVotingType;
	LabelVotingType::Pointer labelVotingFilter;
	labelVotingFilter = LabelVotingType::New();
	labelVotingFilter->SetAbsoluteMinimumPercentage(minAbsPercentage);
	labelVotingFilter->SetMinimumDifferencePercentage(minDiffPercentage);
	labelVotingFilter->SetMinimumRatio(minRatio);
	labelVotingFilter->SetMaxPixelEntropy(maxPixelEntropy);

	for (unsigned int i = 0; i < static_cast<unsigned int>(selection.size()); ++i)
	{
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(selection[i]->GetLabelledImage().GetPointer());
		labelVotingFilter->SetInput(i, lblImg);
		if (maxPixelEntropy >= 0)
		{
			typedef LabelVotingType::DoubleImg::Pointer DblImgPtr;
			std::vector<DblImgPtr> probImgs;
			for (int l = 0; l < labelCount; ++l)
			{
				iAITKIO::ImagePointer p = selection[i]->GetProbabilityImg(l);
				DblImgPtr dp = dynamic_cast<typename LabelVotingType::DoubleImg*>(p.GetPointer());
				probImgs.push_back(dp);
			}
			labelVotingFilter->SetProbabilityImages(i, probImgs);
		}
	}

	labelVotingFilter->Update();
	LabelImagePointer labelResult = labelVotingFilter->GetOutput();
	// according to https://stackoverflow.com/questions/27016173/pointer-casts-for-itksmartpointer,
	// the following does not leave a "dangling" smart pointer:
	iAITKIO::ImagePointer result = dynamic_cast<iAITKIO::ImageBaseType *>(labelResult.GetPointer());
	return result;
}


void dlg_MajorityVoting::AbsMinPercentSlider(int)
{
	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);
	double minAbs = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	lbMinAbsPercent->setText(QString::number(minAbs * 100, 'f', 2) + " %");
	iAITKIO::ImagePointer result = GetMajorityVotingImage(m_selection, minAbs, -1, -1, -1, m_labelCount);
	m_dlgGEMSe->AddMajorityVotingImage(result);
}

void dlg_MajorityVoting::MinDiffPercentSlider(int)
{
	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);
	double minDiff = static_cast<double>(slMinDiffPercent->value()) / slMinDiffPercent->maximum();
	lbMinDiffPercent->setText(QString::number(minDiff * 100, 'f', 2) + " %");
	iAITKIO::ImagePointer result = GetMajorityVotingImage(m_selection, -1, minDiff, -1, -1, m_labelCount);
	m_dlgGEMSe->AddMajorityVotingImage(result);
}

void dlg_MajorityVoting::MinRatioSlider(int)
{
	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);
	double minRatio = static_cast<double>(slMinRatio->value()) / 100;
	lbMinRatio->setText(QString::number(minRatio, 'f', 2));
	iAITKIO::ImagePointer result = GetMajorityVotingImage(m_selection, -1, -1, minRatio, -1, m_labelCount);
	m_dlgGEMSe->AddMajorityVotingImage(result);
}

void dlg_MajorityVoting::MaxPixelEntropySlider(int)
{
	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);
	double maxPixelEntropy = static_cast<double>(slMaxPixelEntropy->value()) / slMaxPixelEntropy->maximum();
	lbMaxPixelEntropy->setText(QString::number(maxPixelEntropy*100, 'f', 2)+" %");
	iAITKIO::ImagePointer result = GetMajorityVotingImage(m_selection, -1, -1, -1, maxPixelEntropy, m_labelCount);
	m_dlgGEMSe->AddMajorityVotingImage(result);
}

iAITKIO::ImagePointer GetMajorityVotingNumbers(QVector<QSharedPointer<iASingleResult> > selection,
	double minAbsPercentage, double minDiffPercentage, double minRatio, double maxPixelEntropy, int mode)
{
	typedef ParametrizableLabelVotingImageFilter<LabelImageType> LabelVotingType;
	LabelVotingType::Pointer labelVotingFilter;
	labelVotingFilter = LabelVotingType::New();
	labelVotingFilter->SetAbsoluteMinimumPercentage(minAbsPercentage);
	labelVotingFilter->SetMinimumDifferencePercentage(minDiffPercentage);
	labelVotingFilter->SetMinimumRatio(minRatio);
	labelVotingFilter->SetMaxPixelEntropy(maxPixelEntropy);

	for (unsigned int i = 0; i < static_cast<unsigned int>(selection.size()); ++i)
	{
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(selection[i]->GetLabelledImage().GetPointer());
		labelVotingFilter->SetInput(i, lblImg);
	}
	labelVotingFilter->Update();
	typename LabelVotingType::DoubleImg::Pointer labelResult = labelVotingFilter->GetNumbers(mode);
	// according to https://stackoverflow.com/questions/27016173/pointer-casts-for-itksmartpointer,
	// the following does not leave a "dangling" smart pointer:
	iAITKIO::ImagePointer result = dynamic_cast<iAITKIO::ImageBaseType *>(labelResult.GetPointer());
	return result;
}

void dlg_MajorityVoting::MinAbsPlot()
{
	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);
	double minAbs = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	iAITKIO::ImagePointer result = GetMajorityVotingNumbers(m_selection, minAbs, -1, -1, -1, AbsolutePercentage);
	m_dlgGEMSe->AddMajorityVotingNumbers(result);
}

void dlg_MajorityVoting::MinDiffPlot()
{
	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);
	double minDiff = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	iAITKIO::ImagePointer result = GetMajorityVotingNumbers(m_selection, -1, minDiff, -1, -1, DiffPercentage);
	m_dlgGEMSe->AddMajorityVotingNumbers(result);
}

void dlg_MajorityVoting::RatioPlot()
{
	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);
	double minRatio = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	iAITKIO::ImagePointer result = GetMajorityVotingNumbers(m_selection, -1, -1, minRatio, -1, Ratio);
	m_dlgGEMSe->AddMajorityVotingNumbers(result);
}


void dlg_MajorityVoting::MaxPixelEntropyPlot()
{
	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);
	double maxPixelEntropy = static_cast<double>(slMaxPixelEntropy->value()) / slMaxPixelEntropy->maximum();
	iAITKIO::ImagePointer result = GetMajorityVotingNumbers(m_selection, -1, -1, -1, maxPixelEntropy, PixelEntropy);
	m_dlgGEMSe->AddMajorityVotingNumbers(result);
}

/*
void dlg_MajorityVoting::MinAbsPercentStore()
{

static int majorityVotingID = 0;
QVector<QSharedPointer<iASingleResult> > m_selection;
m_dlgGEMSe->GetSelection(m_selection);
double percentage = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
iAITKIO::ImagePointer result = GetMajorityVotingImage(m_selection, percentage, -1, -1, -1, -1);
// Put output somewhere!
// Options / Design considerations:
//  * integrate into clustering
//      + image is then part of rest of analysis
//      ~ follow-up decision required:
//          - consider MJ separate algorithm?
//          - how to preserve creation "parameters"?
//		- have to re-run whole clustering? or integrate it somehow faked?
//	* intermediate step: add as separate result (e.g. in favorite view)
//      // (chance to include in clustering after renewed clustering)
//  * separate list of  majority-voted results
//		- separate from
//  * detail view
//      + prominent display
//      + close to current analysis
//      - lots to rewrite, as it expects node with linked values
//      - volatile - will be gone after next cluster / example image selection
//  * new dock widget in same mdichild
//		+ closer to current analysis than separate mdi child
//		- lots of new implementation required
//		- no clear benefit - if each
//  * new window (mdichild?)
//      - detached from current design
//      + completely independent of other implementation (should continue working if anything else changes)
//      - completely independent of other implementation (not integrated into current analysis)
//      +/- theoretically easier to do/practically probably also not little work to make it happen

// store image to disk
QString mvOutFolder = m_outputFolder + "/majorityVoting";
iAITKIO::ScalarPixelType pixelType = itk::ImageIOBase::INT;
QString mvResultFolder = mvOutFolder + "/sample" + QString::number(majorityVotingID);
QDir qdir;
if (!qdir.mkpath(mvResultFolder))
{
DEBUG_LOG(QString("Can't create output directory %1!").arg(mvResultFolder));
return;
}
iAITKIO::writeFile(mvResultFolder+"/label.mhd", result, pixelType);
m_dlgGEMSe->AddMajorityVotingImage(mvOutFolder, majorityVotingID, percentage);
majorityVotingID++;
//m_dlgSamplings->Add(majoritySamplingResults);
}
*/

vtkIdType AddPlot(int plotType, vtkSmartPointer<vtkChartXY> chart, vtkSmartPointer<vtkTable> table, int col1, int col2,
	QColor const & color)
{
	vtkSmartPointer<vtkPlot> plot;
	switch (plotType)
	{
		default: // intentional fall-through
		case vtkChart::POINTS: plot = vtkSmartPointer<vtkPlotPoints>::New(); break;
		case vtkChart::LINE: plot = vtkSmartPointer<vtkPlotLine>::New(); break;
	}
	plot->SetColor(
		static_cast<unsigned char>(color.red()),
		static_cast<unsigned char>(color.green()),
		static_cast<unsigned char>(color.blue()),
		static_cast<unsigned char>(color.alpha())
	);
	plot->SetWidth(1.0);
	plot->SetInputData(table, col1, col2);
	vtkIdType plotID = chart->AddPlot(plot);
	return plotID;
}

vtkSmartPointer<vtkTable> CreateVTKTable(int rowCount, QVector<QString> const & columnNames)
{
	auto result = vtkSmartPointer<vtkTable>::New();
	for (int i = 0; i < columnNames.size(); ++i)
	{
		vtkSmartPointer<vtkFloatArray> arr(vtkSmartPointer<vtkFloatArray>::New());
		arr->SetName(columnNames[i].toStdString().c_str());
		result->AddColumn(arr);
	}
	result->SetNumberOfRows(rowCount);
	return result;
}

void dlg_MajorityVoting::Sample()
{
	if (!m_groundTruthImage)
	{
		QMessageBox::warning(this, "GEMSe", "Please load a reference image first!");
		return;
	}

	QVector<QString> columnNames;
	columnNames.push_back("Value");
	columnNames.push_back("Undecided Pixels");
	columnNames.push_back("Dice");

	const int SampleCount = 100;
	const int ResultCount = 4;
	const int UndecidedLabel = m_labelCount;

	vtkSmartPointer<vtkTable> tables[ResultCount];
	QString titles[ResultCount] =
	{
		QString("Minimum Absolute Percentage"),
		QString("Minimum Percentage Difference"),
		QString("Ratio"),
		QString("Maximum Pixel Uncertainty")
	};
	for (int r = 0; r < ResultCount; ++r)
	{
		tables[r] = CreateVTKTable(SampleCount, columnNames);
	}

	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);

	double absPercMin = 1.0 / m_labelCount;
	double absPercMax = 1;

	double ratioMin = 1;
	double ratioMax = m_selection.size();
	DEBUG_LOG(QString("Majority Voting evaluation for a selection of %1 images").arg(m_selection.size()));

	typedef fhw::FilteringLabelOverlapMeasuresImageFilter<LabelImageType> DiceFilter;
	typedef itk::LabelStatisticsImageFilter<LabelImageType, LabelImageType> StatFilter;

	auto region = m_groundTruthImage->GetLargestPossibleRegion();
	auto size = region.GetSize();
	double pixelCount = size[0] * size[1] * size[2];

	for (int i = 0; i < SampleCount; ++i)
	{
		// calculate current value:
		double norm = mapToNorm(0, SampleCount, i);

		double value[4] = {
			mapNormTo(absPercMin, absPercMax, norm),		// minimum absolute percentage
			norm,											// minimum relative percentage
			mapNormTo(ratioMin, ratioMax, norm),			// ratio
			norm											// maximum pixel uncertainty
		};

		// calculate majority voting using these values:
		iAITKIO::ImagePointer result[ResultCount];

		result[0] = GetMajorityVotingImage(m_selection, value[0], -1, -1, -1, m_labelCount);
		result[1] = GetMajorityVotingImage(m_selection, -1, value[1], -1, -1, m_labelCount);
		result[2] = GetMajorityVotingImage(m_selection, -1, -1, value[2], -1, m_labelCount);
		result[3] = GetMajorityVotingImage(m_selection, -1, -1, -1, value[3], m_labelCount);

		//QString out(QString("absPerc=%1, relPerc=%2, ratio=%3, pixelUnc=%4\t").arg(absPerc).arg(relPerc).arg(ratio).arg(pixelUnc));
		// calculate dice coefficient and percentage of undetermined pixels
		// (percentage of voxels with label = difference marker = max. label + 1)
		for (int r = 0; r < ResultCount; ++r)
		{
			LabelImageType* labelImg = dynamic_cast<LabelImageType*>(result[r].GetPointer());

			auto diceFilter = DiceFilter::New();
			diceFilter->SetSourceImage(m_groundTruthImage);
			diceFilter->SetTargetImage(labelImg);
			diceFilter->SetIgnoredLabel(UndecidedLabel);
			diceFilter->Update();
			auto statFilter = StatFilter::New();
			statFilter->SetInput(labelImg);
			statFilter->SetLabelInput(labelImg);
			statFilter->Update();

			double meanDice = diceFilter->GetMeanOverlap();

			double undefinedPerc =
				statFilter->HasLabel(UndecidedLabel)
				? static_cast<double>(statFilter->GetCount(UndecidedLabel)) / pixelCount
				: 0;
			//out += QString("%1 %2\t").arg(meanDice).arg(undefinedPerc);

			// add values to table
			tables[r]->SetValue(i, 0, value[r]);
			tables[r]->SetValue(i, 1, undefinedPerc);
			tables[r]->SetValue(i, 2, meanDice);
		}
		//DEBUG_LOG(QString::number(i) + ": " + out);
	}

	QString ids;
	for (int s = 0; s < m_selection.size(); ++s)
	{
		ids += QString::number(m_selection[s]->GetDatasetID()) + "-" + QString::number(m_selection[s]->GetID());
		if (s < m_selection.size() - 1)
		{
			ids += ", ";
		}
	}

	int startIdx = twSampleResults->rowCount();
	twSampleResults->setRowCount(startIdx + ResultCount);
	for (int i = 0; i < ResultCount; ++i)
	{
		QCheckBox * checkBox = new QCheckBox;
		//if (i == 3) checkBox->setChecked(true);
		twSampleResults->setCellWidget(startIdx + i, 0, checkBox);
		connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(CheckBoxStateChanged(int)));
		twSampleResults->setItem(startIdx + i, 1, new QTableWidgetItem("Maj. Vote Sampling/" + titles[i] + "/" + ids));
		m_checkBoxResultIDMap.insert(checkBox, startIdx + i);
		if (m_results.size() != startIdx + i)
		{
			DEBUG_LOG("Results vector and table are out of sync!");
		}
		m_results.push_back(tables[i]);
	}

}

void dlg_MajorityVoting::CheckBoxStateChanged(int state)
{
	QCheckBox* sender = dynamic_cast<QCheckBox*>(QObject::sender());
	int id = m_checkBoxResultIDMap[sender];
	if (state == Qt::Checked)
	{
		static int colorCnt = 0;
		int colorIdx = (colorCnt++) % 12;
		QColor plotColor = m_colorTheme->GetColor(colorIdx);
		vtkIdType plot1 = AddPlot(vtkChart::POINTS, m_chartDiceVsUndec, m_results[id], 1, 2, plotColor);
		vtkIdType plot2 = AddPlot(vtkChart::LINE, m_chartValueVsDice,  m_results[id], 0, 2, plotColor);
		vtkIdType plot3 = AddPlot(vtkChart::LINE, m_chartValueVsUndec, m_results[id], 0, 1, plotColor);
		QVector<vtkIdType> plots;
		plots.push_back(plot1);
		plots.push_back(plot2);
		plots.push_back(plot3);
		m_plotMap.insert(id, plots);
		twSampleResults->item(id, 1)->setBackgroundColor(plotColor);
	}
	else
	{
		twSampleResults->item(id, 1)->setBackgroundColor(Qt::white);
		QVector<vtkIdType> plots = m_plotMap[id];
		m_chartDiceVsUndec->RemovePlot(plots[0]);
		m_chartValueVsDice->RemovePlot(plots[1]);
		m_chartValueVsUndec->RemovePlot(plots[2]);
	}
}