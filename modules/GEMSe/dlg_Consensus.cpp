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
#include "dlg_Consensus.h"

#include "dlg_GEMSe.h"
#include "dlg_samplings.h"
#include "iAImageTreeNode.h"
#include "iASamplingResults.h"
#include "iASingleResult.h"

// LabelVoting:
#include "MaskingLabelOverlapMeasuresImageFilter.h"
#include "ParametrizableLabelVotingImageFilter.h"
#include "ProbabilisticVotingImageFilter.h"
#include "UndecidedPixelClassifierImageFilter.h"

#include "dlg_commoninput.h"
#include "iAColorTheme.h"
#include "iAConsole.h"
#include "iADockWidgetWrapper.h"
#include "iALookupTable.h"
#include "iAToolsITK.h"
#include "io/iAFileUtils.h"
#include "io/iAIOProvider.h"
#include "mdichild.h"

#include <itkCastImageFilter.h>
#include <itkMultiLabelSTAPLEImageFilter.h>
#include <itkSTAPLEImageFilter.h>
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
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QVector>


// Where to show (temporary?) output of consensus algorithms:

// currently chosen option:
//  * detail view
//      + prominent display
//      + close to current analysis
//      - lots to rewrite, as it expects node with linked values
//      - volatile - will be gone after next cluster / example image selection

// possibly best future option:
//  * integrate into clustering
//      + image is then part of rest of analysis
//      ~ follow-up decision required:
//          - consider MV separate algorithm?
//          - how to preserve creation "parameters"?
//		- have to re-run whole clustering? or integrate it somehow faked?
//	* intermediate step: add as separate result (e.g. in favorite view)
//      // (with a chance to include in clustering after renewed clustering)

// other options:
//  * separate list of  majority-voted results
//		- separate from other analysis
//  * new dock widget in same mdichild
//		+ closer to current analysis than separate mdi child
//		- lots of new implementation required
//		- no clear benefit
//      - could get pretty crowded
//  * new window (mdichild?)
//      + completely independent of other implementation (should continue working if anything else changes)
//      - completely independent of other implementation (not integrated into current analysis)
//      - detached from current design
//      +/- theoretically easier to do/practically probably also not little work to make it happen

struct ChartWidgetData
{
	QVTKWidget2* vtkWidget;
	vtkSmartPointer<vtkChartXY> chart;
};

ChartWidgetData CreateChartWidget(const char * xTitle, const char * yTitle,
		MdiChild* mdiChild)
{
	ChartWidgetData result;
	result.vtkWidget = new QVTKWidget2();
	auto contextView = vtkSmartPointer<vtkContextView>::New();
	contextView->SetRenderWindow(result.vtkWidget->GetRenderWindow());
	result.chart = vtkSmartPointer<vtkChartXY>::New();
	result.chart->SetSelectionMode(vtkContextScene::SELECTION_NONE);
	auto xAxis1 = result.chart->GetAxis(vtkAxis::BOTTOM);
	auto yAxis1 = result.chart->GetAxis(vtkAxis::LEFT);
	xAxis1->SetTitle(xTitle);
	xAxis1->SetLogScale(false);
	yAxis1->SetTitle(yTitle);
	yAxis1->SetLogScale(false);
	contextView->GetScene()->AddItem(result.chart);
	iADockWidgetWrapper * w(new iADockWidgetWrapper(result.vtkWidget,
			QString("%1 vs. %2").arg(xTitle).arg(yTitle),
			QString("%1%2").arg(xTitle).arg(yTitle).replace(" ", "") ));
	mdiChild->SplitDockWidget(mdiChild->logs, w, Qt::Vertical);
	return result;
}

dlg_Consensus::dlg_Consensus(MdiChild* mdiChild, dlg_GEMSe* dlgGEMSe, int labelCount, QString const & folder, dlg_samplings* dlgSamplings) :
	m_mdiChild(mdiChild),
	m_dlgGEMSe(dlgGEMSe),
	m_labelCount(labelCount),
	m_folder(folder),
	m_dlgSamplings(dlgSamplings),
	m_dlgProgress(nullptr),
	m_comparisonWeightType(Equal)
{
	QString defaultTheme("Brewer Paired (max. 12)");
	m_colorTheme = iAColorThemeManager::GetInstance().GetTheme(defaultTheme);

	m_consensusCharts.push_back(CreateChartWidget("Undecided Pixels", "Mean Dice", mdiChild));
	m_consensusCharts.push_back(CreateChartWidget("Consensus Method Parameter", "Mean Dice", mdiChild));
	m_consensusCharts.push_back(CreateChartWidget("Consensus Method Parameter", "Undecided Pixels", mdiChild));
	m_consensusCharts.push_back(CreateChartWidget("Consensus Method Parameter", "Label Dice", mdiChild));

	QSharedPointer<iAImageTreeNode> root = dlgGEMSe->GetRoot();
	int ensembleSize = root->GetClusterSize();
	slMinRatio->setMaximum(ensembleSize*100);
	slLabelVoters->setMaximum(ensembleSize);
	twSampleResults->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	twSampleResults->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	connect(twSampleResults, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(SampledItemClicked(QTableWidgetItem *)));
}

dlg_Consensus::~dlg_Consensus()
{}

int dlg_Consensus::GetWeightType()
{
	if (rbWeightLabelDice->isChecked())  return LabelBased;
	if (rbWeightCertainty->isChecked())  return Certainty;
	if (rbWeightDiffFBGSBG->isChecked()) return FBGSBGDiff;
	else return Equal;
}

void dlg_Consensus::EnableUI()
{
	pbMajorityVoting->setEnabled(true);
	pbSample->setEnabled(true);
	pbMinAbsPercent_Plot->setEnabled(true);
	pbMinDiffPercent_Plot->setEnabled(true);
	pbMinRatio_Plot->setEnabled(true);
	pbMaxPixelEntropy_Plot->setEnabled(true);
	pbClusterUncertaintyDice->setEnabled(true);
	pbStore->setEnabled(true);
	pbStoreConfig->setEnabled(true);
	pbLoadConfig->setEnabled(true);
	pbSTAPLE->setEnabled(true);
	slAbsMinPercent->setEnabled(true);
	slMinDiffPercent->setEnabled(true);
	slMinRatio->setEnabled(true);
	slMaxPixelEntropy->setEnabled(true);
	slLabelVoters->setEnabled(true);
	connect(pbSample, SIGNAL(clicked()), this, SLOT(Sample()));
	connect(pbMinAbsPercent_Plot, SIGNAL(clicked()), this, SLOT(MinAbsPlot()));
	connect(pbMinDiffPercent_Plot, SIGNAL(clicked()), this, SLOT(MinDiffPlot()));
	connect(pbMinRatio_Plot, SIGNAL(clicked()), this, SLOT(RatioPlot()));
	connect(pbMaxPixelEntropy_Plot, SIGNAL(clicked()), this, SLOT(MaxPixelEntropyPlot()));
	connect(pbClusterUncertaintyDice, SIGNAL(clicked()), this, SLOT(ClusterUncertaintyDice()));
	connect(pbStore, SIGNAL(clicked()), this, SLOT(StoreResult()));
	connect(pbStoreConfig, SIGNAL(clicked()), this, SLOT(StoreConfig()));
	connect(pbLoadConfig, SIGNAL(clicked()), this, SLOT(LoadConfig()));
	connect(pbSTAPLE, SIGNAL(clicked()), this, SLOT(CalcSTAPLE()));
	connect(pbMajorityVoting, SIGNAL(clicked()), this, SLOT(CalcMajorityVote()));
	connect(pbProbRuleVote, SIGNAL(clicked()), this, SLOT(CalcProbRuleVote()));
	connect(slAbsMinPercent, SIGNAL(valueChanged(int)), this, SLOT(AbsMinPercentSlider(int)));
	connect(slMinDiffPercent, SIGNAL(valueChanged(int)), this, SLOT(MinDiffPercentSlider(int)));
	connect(slMinRatio, SIGNAL(valueChanged(int)), this, SLOT(MinRatioSlider(int)));
	connect(slMaxPixelEntropy, SIGNAL(valueChanged(int)), this, SLOT(MaxPixelEntropySlider(int)));
	connect(slLabelVoters, SIGNAL(valueChanged(int)), this, SLOT(LabelVoters(int)));
}

namespace
{

	QString GetWeightName(int weightType)
	{
		switch (weightType)
		{
		case LabelBased: return "LabelDice";
		case Certainty:  return "Certainty";
		case FBGSBGDiff: return "FBG-SBG";
		default:         return "Equal";
		}
	}

	int GetWeightType(QString const & name)
	{
		if (name == "LabelDice") return LabelBased;
		else if (name == "Certainty") return Certainty;
		else if (name == "FBG-SBG") return FBGSBGDiff;
		else if (name == "Equal")  return Equal;
		else return -1;
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

}

void dlg_Consensus::SetGroundTruthImage(LabelImagePointer groundTruthImage)
{
	m_groundTruthImage = groundTruthImage;
}

#include "iAAttributes.h"

void dlg_Consensus::ClusterUncertaintyDice()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	QSharedPointer<iAImageTreeNode> node = m_dlgGEMSe->GetSelectedCluster();
	//"Cluster (id=" + QString::number(node->GetID()) + ")";
}

void dlg_Consensus::SelectionUncertaintyDice(
	QVector<QSharedPointer<iASingleResult> > const & selection,
	QString const & name)
{
	if (!m_groundTruthImage)
	{
		QMessageBox::warning(this, "GEMSe", "Please load a reference image first!");
		return;
	}
	// gather Avg. Uncertainty vs. Accuracy, make it entry in row 1!
	QVector<QString> columns;
	columns.push_back("Average Uncertainty");
	columns.push_back("Mean Dice");

	auto table = CreateVTKTable(selection.size(), columns);

	for (int i = 0; i < selection.size(); ++i)
	{
		int avgUncIdx = selection[i]->GetAttributes()->Find("Average Uncertainty");
		int diceIdx = selection[i]->GetAttributes()->Find("Dice");
		double unc = selection[i]->GetAttribute(avgUncIdx);
		double dice = selection[i]->GetAttribute(diceIdx);
		table->SetValue(i, 0, unc);
		table->SetValue(i, 1, dice);
	}
	AddResult(table, QString("%1 Avg. Unc. vs. Mean Dice").arg(name));
}


typedef ParametrizableLabelVotingImageFilter<LabelImageType> LabelVotingType;

LabelVotingType::Pointer GetLabelVotingFilter(
	QVector<QSharedPointer<iASingleResult> > selection,
	double minAbsPercentage, double minDiffPercentage, double minRatio, double maxPixelEntropy,
	int labelVoters, int weightType, int labelCount)
{
	LabelVotingType::Pointer labelVotingFilter;
	labelVotingFilter = LabelVotingType::New();
	labelVotingFilter->SetAbsoluteMinimumPercentage(minAbsPercentage);
	labelVotingFilter->SetMinimumDifferencePercentage(minDiffPercentage);
	labelVotingFilter->SetMinimumRatio(minRatio);
	labelVotingFilter->SetMaxPixelEntropy(maxPixelEntropy);
	labelVotingFilter->SetWeightType(static_cast<WeightType>(weightType));
	if (labelVoters > 0)
	{
		labelVoters = std::min(selection.size(), labelVoters);
		typedef std::pair<int, double> InputDice;
		std::set<std::pair<int, int> > inputLabelVotersSet;
		for (int l = 0; l<labelCount; ++l)
		{
			std::vector<InputDice> memberDice;
			for (int m = 0; m < selection.size(); ++m)
			{
				int attributeID = selection[m]->GetAttributes()->Find(QString("Dice %1").arg(l));
				if (attributeID == -1)
				{
					DEBUG_LOG(QString("Attribute 'Dice %1' not found, aborting!").arg(l));
					return LabelVotingType::Pointer();
				}
				memberDice.push_back(std::make_pair(m, selection[m]->GetAttribute(attributeID)));
			}
			// sort in descending order by metric
			sort(memberDice.begin(), memberDice.end(), [](InputDice const & a, InputDice const & b)
			{
				return a.second > b.second; // > because we want to order descending
			});
			for (int m = 0; m < labelVoters; ++m)
			{
				inputLabelVotersSet.insert(std::make_pair(l, memberDice[m].first));
			}
		}
		labelVotingFilter->SetInputLabelVotersSet(inputLabelVotersSet);
	}

	if (weightType == LabelBased)
	{
		std::map<std::pair<int, int>, double> inputLabelWeightMap;

		for (int l = 0; l < labelCount; ++l)
		{
			for (int m = 0; m < selection.size(); ++m)
			{
				int attributeID = selection[m]->GetAttributes()->Find(QString("Dice %1").arg(l));
				if (attributeID == -1)
				{
					DEBUG_LOG(QString("Attribute 'Dice %1' not found, aborting!").arg(l));
					return LabelVotingType::Pointer();
				}
				double labelDice = selection[m]->GetAttribute(attributeID);
				inputLabelWeightMap.insert(
					std::make_pair(std::make_pair(l, m), labelDice));
			}
		}
		labelVotingFilter->SetInputLabelWeightMap(inputLabelWeightMap);
	}

	for (unsigned int i = 0; i < static_cast<unsigned int>(selection.size()); ++i)
	{
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(selection[i]->GetLabelledImage().GetPointer());
		labelVotingFilter->SetInput(i, lblImg);
		if (maxPixelEntropy >= 0 || weightType == Certainty || weightType == FBGSBGDiff)
		{
			typedef LabelVotingType::DoubleImg DblImg;
			typedef DblImg::Pointer DblImgPtr;
			std::vector<DblImgPtr> probImgs;
			for (int l = 0; l < labelCount; ++l)
			{
				iAITKIO::ImagePointer p = selection[i]->GetProbabilityImg(l);
				DblImgPtr dp = dynamic_cast<DblImg *>(p.GetPointer());
				probImgs.push_back(dp);
			}
			labelVotingFilter->SetProbabilityImages(i, probImgs);
		}
	}
	labelVotingFilter->Update();
	return labelVotingFilter;
}

iAITKIO::ImagePointer GetVotingImage(QVector<QSharedPointer<iASingleResult> > selection,
	double minAbsPercentage, double minDiffPercentage, double minRatio, double maxPixelEntropy,
	int labelVoters, int weightType, int labelCount, bool undecidedPixels, double & undecided)
{
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return iAITKIO::ImagePointer();
	}
	auto labelVotingFilter = GetLabelVotingFilter(
		selection, minAbsPercentage, minDiffPercentage, minRatio, maxPixelEntropy, labelVoters, weightType, labelCount);
	if (!labelVotingFilter)
		return iAITKIO::ImagePointer();
	LabelImagePointer labelResult = labelVotingFilter->GetOutput();
	undecided = labelVotingFilter->GetUndecided();
	iAITKIO::ImagePointer result;
	if (undecidedPixels)
	{
		auto undec = UndecidedPixelClassifierImageFilter<LabelImageType>::New();
		typedef LabelVotingType::DoubleImg DblImg;
		typedef DblImg::Pointer DblImgPtr;
		for (unsigned int i = 0; i < static_cast<unsigned int>(selection.size()); ++i)
		{
			std::vector<DblImgPtr> probImgs;
			for (int l = 0; l < labelCount; ++l)
			{
				iAITKIO::ImagePointer p = selection[i]->GetProbabilityImg(l);
				DblImgPtr dp = dynamic_cast<DblImg *>(p.GetPointer());
				probImgs.push_back(dp);
			}
			undec->SetProbabilityImages(i, probImgs);
		}
		undec->SetInput(labelResult);
		undec->SetUndecidedPixelLabel(labelCount);
		undec->Update();
		LabelImagePointer undecResult = undec->GetOutput();
		result = dynamic_cast<iAITKIO::ImageBaseType *>(undecResult.GetPointer());
	}
	else
	{
		result = dynamic_cast<iAITKIO::ImageBaseType *>(labelResult.GetPointer());
	}
	return result;
}


iAITKIO::ImagePointer GetProbVotingImage(QVector<QSharedPointer<iASingleResult> > selection,
	double threshold, VotingRule rule, int labelCount, bool undecidedPixels, double & undecided,
	QVector<double> & diceMV, QVector<double> & diceUndecided, LabelImagePointer groundTruth)
{
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return iAITKIO::ImagePointer();
	}
	auto filter = ProbabilisticVotingImageFilter<LabelImageType>::New();
	filter->SetVotingRule(rule);
	filter->SetUndecidedUncertaintyThreshold(threshold);
	// set one "alibi" input to automatically create output:
	filter->SetInput(0, dynamic_cast<LabelImageType*>(selection[0]->GetLabelledImage().GetPointer()));
	typedef LabelVotingType::DoubleImg DblImg;
	typedef DblImg::Pointer DblImgPtr;
	for (unsigned int i = 0; i < static_cast<unsigned int>(selection.size()); ++i)
	{
		std::vector<DblImgPtr> probImgs;
		for (int l = 0; l < labelCount; ++l)
		{
			iAITKIO::ImagePointer p = selection[i]->GetProbabilityImg(l);
			DblImgPtr dp = dynamic_cast<DblImg *>(p.GetPointer());
			probImgs.push_back(dp);
		}
		filter->SetProbabilityImages(i, probImgs);
	}
	// TODO: consider weights? Though according to Al-Taie et al., weighted voting doesn't have benefits
	// filter->SetWeights()
	filter->Update();
	LabelImagePointer labelResult = filter->GetOutput();
	undecided = filter->GetUndecided();

	// calculate dice for those voxels decided by the Prob. Vote:
	auto pvdicefilter = fhw::MaskingLabelOverlapMeasuresImageFilter<LabelImageType>::New() ;
	pvdicefilter->SetSourceImage(groundTruth);
	pvdicefilter->SetTargetImage(labelResult);
	pvdicefilter->SetIgnoredLabel(labelCount);
	pvdicefilter->Update();
	diceMV.push_back(pvdicefilter->GetMeanOverlap());
	for (int l = 0; l < labelCount; ++l)
	{
		diceMV.push_back(pvdicefilter->GetMeanOverlap(l));
	}
	auto undecidedPixelIndices = pvdicefilter->IgnoredIndices();


	iAITKIO::ImagePointer result;
	if (undecidedPixels)
	{
		auto undec = UndecidedPixelClassifierImageFilter<LabelImageType>::New();
		typedef LabelVotingType::DoubleImg DblImg;
		typedef DblImg::Pointer DblImgPtr;
		for (unsigned int i = 0; i < static_cast<unsigned int>(selection.size()); ++i)
		{
			std::vector<DblImgPtr> probImgs;
			for (int l = 0; l < labelCount; ++l)
			{
				iAITKIO::ImagePointer p = selection[i]->GetProbabilityImg(l);
				DblImgPtr dp = dynamic_cast<DblImg *>(p.GetPointer());
				probImgs.push_back(dp);
			}
			undec->SetProbabilityImages(i, probImgs);
		}
		undec->SetInput(labelResult);
		undec->SetUndecidedPixelLabel(labelCount);
		undec->Update();
		LabelImagePointer undecResult = undec->GetOutput();
		result = dynamic_cast<iAITKIO::ImageBaseType *>(undecResult.GetPointer());
		
		// calculate dice for undecided pixels:
		auto undicefilter = fhw::MaskingLabelOverlapMeasuresImageFilter<LabelImageType>::New();
		undicefilter->SetSourceImage(groundTruth);
		undicefilter->SetTargetImage(undecResult);
		undicefilter->SetIgnoredIndices(undecidedPixelIndices);
		undicefilter->Update();
		diceUndecided.push_back(undicefilter->GetMeanOverlap());
		for (int l = 0; l < labelCount; ++l)
		{
			diceUndecided.push_back(undicefilter->GetMeanOverlap(l));
		}
	}
	else
	{
		result = dynamic_cast<iAITKIO::ImageBaseType *>(labelResult.GetPointer());
	}
	return result;
}


iAITKIO::ImagePointer GetVotingNumbers(QVector<QSharedPointer<iASingleResult> > selection,
	double minAbsPercentage, double minDiffPercentage, double minRatio, double maxPixelEntropy,
	int labelVoters, int weightType, int labelCount, int mode)
{
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return iAITKIO::ImagePointer();
	}
	auto labelVotingFilter = GetLabelVotingFilter(
		selection, minAbsPercentage, minDiffPercentage, minRatio, maxPixelEntropy, labelVoters, weightType, labelCount);
	if (!labelVotingFilter)
		return iAITKIO::ImagePointer();
	typedef LabelVotingType::DoubleImg::Pointer DblImgPtr;
	DblImgPtr labelResult = labelVotingFilter->GetNumbers(mode);
	iAITKIO::ImagePointer result = dynamic_cast<iAITKIO::ImageBaseType *>(labelResult.GetPointer());
	return result;
}


QString CollectedIDs(QVector<QSharedPointer<iASingleResult> > selection)
{
	QStringList ids;
	for (int i = 0; i < selection.size(); ++i)
	{
		ids.append(QString("%1-%2").arg(selection[i]->GetDatasetID()).arg(selection[i]->GetID()));
	}
	return ids.join(",");
}

void dlg_Consensus::AbsMinPercentSlider(int)
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	double minAbs = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	QString name = QString("Voting FBG > %1 % (%2)").arg(QString::number(minAbs * 100, 'f', 2).arg(CollectedIDs(selection)));
	lbValue->setText(name);
	UpdateWeightPlot();
	double undecided;
	m_lastMVResult = GetVotingImage(selection, minAbs, -1, -1, -1, -1, GetWeightType(), m_labelCount, cbUndecidedPixels->isChecked(), undecided);
	m_dlgGEMSe->AddConsensusImage(m_lastMVResult, name );
}

void dlg_Consensus::MinDiffPercentSlider(int)
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	double minDiff = static_cast<double>(slMinDiffPercent->value()) / slMinDiffPercent->maximum();
	QString name = QString("Voting FBG-SBG > %1 % (%2)").arg(QString::number(minDiff * 100, 'f', 2).arg(CollectedIDs(selection)));
	lbValue->setText(name);
	UpdateWeightPlot();
	double undecided;
	m_lastMVResult = GetVotingImage(selection, -1, minDiff, -1, -1, -1, GetWeightType(), m_labelCount, cbUndecidedPixels->isChecked(), undecided);
	m_dlgGEMSe->AddConsensusImage(m_lastMVResult, name);
}

void dlg_Consensus::MinRatioSlider(int)
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	double minRatio = static_cast<double>(slMinRatio->value()) / 100;
	QString name = QString("Voting FBG/SBG > %1 (%2)").arg(QString::number(minRatio, 'f', 2).arg(CollectedIDs(selection)));
	lbValue->setText(name);
	UpdateWeightPlot();
	double undecided;
	m_lastMVResult = GetVotingImage(selection, -1, -1, minRatio, -1, -1, GetWeightType(), m_labelCount, cbUndecidedPixels->isChecked(), undecided);
	m_dlgGEMSe->AddConsensusImage(m_lastMVResult, name);
}

void dlg_Consensus::MaxPixelEntropySlider(int)
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	double maxPixelEntropy = static_cast<double>(slMaxPixelEntropy->value()) / slMaxPixelEntropy->maximum();
	QString name = QString("Voting Entropy < %1 (%2)").arg(QString::number(maxPixelEntropy * 100, 'f', 2).arg(CollectedIDs(selection)));
	lbValue->setText(name);
	UpdateWeightPlot();
	double undecided;
	m_lastMVResult = GetVotingImage(selection, -1, -1, -1, maxPixelEntropy, -1, GetWeightType(), m_labelCount, cbUndecidedPixels->isChecked(), undecided);
	m_dlgGEMSe->AddConsensusImage(m_lastMVResult, name);
}

void dlg_Consensus::LabelVoters(int)
{
	if (!m_groundTruthImage)
	{
		DEBUG_LOG("Please load a reference image first!");
		return;
	}
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	int labelVoters = slLabelVoters->value();
	QString name = QString("Voting Best %1 of label (%2)").arg(labelVoters).arg(CollectedIDs(selection));
	lbValue->setText(name);
	lbValue->setMinimumWidth(10);
	UpdateWeightPlot();
	double undecided;
	m_lastMVResult = GetVotingImage(selection, -1, -1, -1, -1, labelVoters, GetWeightType(), m_labelCount, cbUndecidedPixels->isChecked(), undecided);
	m_dlgGEMSe->AddConsensusImage(m_lastMVResult, name);
}

void dlg_Consensus::MinAbsPlot()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	double minAbs = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	m_lastMVResult = GetVotingNumbers(selection, minAbs, -1, -1, -1, -1, GetWeightType(), m_labelCount, AbsolutePercentage);
	m_dlgGEMSe->AddConsensusNumbersImage(m_lastMVResult, QString("FBG (%1)").arg(CollectedIDs(selection)));
}

void dlg_Consensus::MinDiffPlot()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	double minDiff = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	m_lastMVResult = GetVotingNumbers(selection, -1, minDiff, -1, -1, -1, GetWeightType(), m_labelCount, DiffPercentage);
	m_dlgGEMSe->AddConsensusNumbersImage(m_lastMVResult, QString("FBG-SBG (%1)").arg(CollectedIDs(selection)));
}

void dlg_Consensus::RatioPlot()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	double minRatio = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	m_lastMVResult = GetVotingNumbers(selection, -1, -1, minRatio, -1, -1, GetWeightType(), m_labelCount, Ratio);
	m_dlgGEMSe->AddConsensusNumbersImage(m_lastMVResult, QString("FBG/SBG (%1)").arg(CollectedIDs(selection)));
}


void dlg_Consensus::MaxPixelEntropyPlot()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	double maxPixelEntropy = static_cast<double>(slMaxPixelEntropy->value()) / slMaxPixelEntropy->maximum();
	m_lastMVResult = GetVotingNumbers(selection, -1, -1, -1, maxPixelEntropy, -1, GetWeightType(), m_labelCount, PixelEntropy);
	m_dlgGEMSe->AddConsensusNumbersImage(m_lastMVResult, QString("Entropy (%1)").arg(CollectedIDs(selection)));
}

void dlg_Consensus::StoreResult()
{
	if (!m_lastMVResult)
	{
		DEBUG_LOG("You need to perform Voting at least once, before last Consensus result can be stored!");
		return;
	}
	iAITKIO::ScalarPixelType pixelType = itk::ImageIOBase::INT;
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Store Last Voting Result"),
		m_folder,
		iAIOProvider::GetSupportedSaveFormats()
	);
	if (fileName.isEmpty())
	{
		return;
	}
	iAITKIO::writeFile(fileName, m_lastMVResult, pixelType);
}

namespace
{
	static const QString FileFormatKey("FileFormat");
	static const QString FileVersion("v1");
	static const QString LabelsKey("Labels");
	static const QString DerivedOutputName = "Dice";
}

void dlg_Consensus::StoreConfig()
{
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Store Algorithm Comparison Configuration"),
		m_folder,
		"Algorithm Comparison Configuration (*.acc)"
	);
	if (fileName.isEmpty())
	{
		return;
	}
	QFileInfo fi(fileName);
	QString basePath(fi.absolutePath());
	QSettings s(fileName, QSettings::IniFormat);
	s.setValue(FileFormatKey, FileVersion);
	s.setValue(LabelsKey, m_labelCount);

	// fetch best n results by dice
	auto samplings = m_dlgGEMSe->GetSamplings();
	typedef std::tuple<int, int, double> OneRunDice;
	std::vector<OneRunDice> runs;

	for (int d = 0; d < samplings->size(); ++d)
	{
		auto sampling = samplings->at(d);
		s.setValue(QString("SamplingData%1").arg(d), MakeRelative(basePath, sampling->GetFileName()) );
		for (int s = 0; s < sampling->size(); ++s)
		{
			auto r = sampling->Get(s);
			int derivedOutID = r->GetAttributes()->Find(DerivedOutputName);
			runs.push_back(std::make_tuple(r->GetDatasetID(), r->GetID(), r->GetAttribute(derivedOutID)));
		}
	}
	std::sort(runs.begin(), runs.end(), [](const OneRunDice & a, const OneRunDice & b)
	{
		return std::get<2>(a) > std::get<2>(b); // > because we want to order descending
	});

	const size_t NumberOfBestSets = 10;
	QStringList bestParameterSets, bestDice;
	for (int b = 0; b < std::min(runs.size(), NumberOfBestSets); ++b)
	{
		bestParameterSets.append(QString::number(std::get<0>(runs[b])) + "-" + QString::number(std::get<1>(runs[b])));
		bestDice.append(QString::number(std::get<2>(runs[b])));
	}
	s.setValue("BestSingle/ParameterSets", bestParameterSets.join(","));
	s.setValue(QString("BestSingle/%1").arg(DerivedOutputName), bestDice.join(","));

	// fetch config for (last?) consensus (sampling?)
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	QStringList parameterSets;
	for (int i = 0; i < selection.size(); ++i)
	{
		parameterSets.append(QString::number(selection[i]->GetDatasetID()) + "-"+QString::number(selection[i]->GetID()));
	}
	int weightType = GetWeightType();
	s.setValue("Voting/ParameterSets", parameterSets.join(","));
	s.setValue("Voting/WeightType", GetWeightName(weightType));
	if (weightType == LabelBased)
	{
		for (int l = 0; l < m_labelCount; ++l)
		{
			QStringList inputWeights;
			for (int m = 0; m < selection.size(); ++m)
			{
				QString derivedOutName(QString("%1 %2").arg(DerivedOutputName).arg(l));
				int attributeID = selection[m]->GetAttributes()->Find(derivedOutName);
				if (attributeID == -1)
				{
					DEBUG_LOG(QString("Attribute '%1' not found!").arg(derivedOutName));
				}
				double labelDice = selection[m]->GetAttribute(attributeID);
				inputWeights.append(QString::number(labelDice));
			}
			s.setValue(QString("Voting/InputWeightLabel%1").arg(l), inputWeights.join(","));
		}
	}
}

namespace
{
	void AddParameterSets(QVector<QSet<int> > & ids, QStringList const & fullIDStrList)
	{
		bool sizeOK, conv1ok, conv2ok;
		int datasetID, parameterSetID;
		for (QString fullID: fullIDStrList)
		{
			QStringList spl = fullID.split("-");
			sizeOK = spl.size() == 2;
			if (sizeOK)
			{
				datasetID = spl[0].toInt(&conv1ok);
				parameterSetID = spl[1].toInt(&conv2ok);
			}
			if (sizeOK && conv1ok && conv2ok)
			{
				ids[datasetID].insert(parameterSetID);
			}
			else
			{
				DEBUG_LOG(QString("Error in converting full ID '%1'!").arg(fullID));
			}
		}
	}
}

#include "dlg_progress.h"
#include "iAAttributeDescriptor.h"
#include "iAImageSampler.h"
#include "iAMeasures.h"
#include "iAParameterGeneratorImpl.h"
#include "iASEAFile.h"

void dlg_Consensus::LoadConfig()
{
	if (!m_groundTruthImage)
	{
		QMessageBox::warning(this, "GEMSe", "Please load a reference image first!");
		return;
	}
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Load Algorithm Comparison Configuration"),
		m_folder,
		"Algorithm Comparison Configuration (*.acc)"
	);
	if (fileName.isEmpty())
	{
		return;
	}
	QFileInfo fi(fileName);
	QSettings s(fileName, QSettings::IniFormat);
	if (s.value(FileFormatKey) != FileVersion)
	{
		QMessageBox::warning(this, "GEMSe",
			QString("Loaded File has the wrong file format, expected %1, got %2 as format identifier!")
			.arg(FileVersion).arg(s.value(FileFormatKey).toString()));
		return;
	}
	/*
	// only applicable if weight set by label
	if (s.value(LabelsKey).toInt() != m_labelCount)
	{
		QMessageBox::warning(this, "GEMSe",
			QString("Label count does not match: expected %1, got %2 as number of labels!")
			.arg(m_labelCount).arg(s.value(LabelsKey).toInt()));
		return;
	}
	*/
	// load datasets:
	QStringList samplings;
	int curSamplingIdx = 0;
	while (s.contains(QString("SamplingData%1").arg(curSamplingIdx)))
	{
		samplings.push_back(
			MakeAbsolute(fi.absolutePath(),
			s.value(QString("SamplingData%1").arg(curSamplingIdx)).toString()
		));
		++curSamplingIdx;
	}
	QStringList bestParameterSetsList = s.value("BestSingle/ParameterSets").toString().split(",");
	QStringList derivedOuts = s.value(QString("BestSingle/%1").arg(DerivedOutputName)).toString().split(",");
	QStringList mvParamSetsList = s.value("Voting/ParameterSets").toString().split(",");
	m_comparisonWeightType = ::GetWeightType(s.value("Voting/WeightType").toString());
	m_queuedSamplers.clear();
	std::map<std::pair<int, int>, double> inputLabelWeightMap;
	if (m_comparisonWeightType == LabelBased)
	{
		bool ok;
		for (int l = 0; l < m_labelCount; ++l)
		{
			QStringList inputWeights = s.value(QString("Voting/InputWeightLabel%1").arg(l)).toString().split(",");
			for (int m = 0; m < mvParamSetsList.size(); ++m)
			{
				double labelWeight = inputWeights[m].toDouble(&ok);
				if (!ok)
				{
					DEBUG_LOG(QString("Error in label weights for label %1, entry %2('%3')").arg(l).arg(m).arg(inputWeights[m]));
					return;
				}
				inputLabelWeightMap.insert(
					std::make_pair(std::make_pair(l, m), labelWeight));
			}
		}
	}
	int lastSamplingID = m_dlgSamplings->GetSamplings()->size();
	QVector<QSet<int> > bestParameterSetIDs(samplings.size()),
		mvParameterSetIDs(samplings.size());
	AddParameterSets(bestParameterSetIDs, bestParameterSetsList);
	AddParameterSets(mvParameterSetIDs, mvParamSetsList);
	// run segmentations with loaded parameter sets
	m_comparisonSamplingResults.clear();
	m_comparisonBestIDs.clear();
	m_comparisonMVIDs.clear();
	for (int s = 0; s < samplings.size(); ++s)
	{
		m_comparisonBestIDs.push_back(QVector<int>());
		m_comparisonMVIDs.push_back(QVector<int>());
		// find all IDs from this data that need to be sampled:
		QString smpFileName = samplings[s];
		QString outputFolder = QFileDialog::getExistingDirectory(this,
			QString("Output folder for Algorithm %1").arg(s),
			m_folder);
		if (outputFolder.isEmpty())
		{
			return;
		}
		auto samplingResults = iASamplingResults::Load(smpFileName, s);
		ParameterSetsPointer parameterSets(new QVector<ParameterSet>());
		for (int i = 0; i < samplingResults->size(); ++i)
		{
			if (bestParameterSetIDs[s].contains(i) || mvParameterSetIDs[s].contains(i))
			{
				// ToDo: check if parameters already exist in current samplings!
				if (bestParameterSetIDs[s].contains(i))
				{
					m_comparisonBestIDs[s].push_back(parameterSets->size());
				}
				if (mvParameterSetIDs[s].contains(i))
				{
					m_comparisonMVIDs[s].push_back(parameterSets->size());
				}
				QVector<double> singleParameterSet;
				for (int p = 0; p < samplingResults->GetAttributes()->size(); ++p)
				{
					if (samplingResults->GetAttributes()->at(p)->AttribType() == iAAttributeDescriptor::Parameter)
					{
						singleParameterSet.push_back(samplingResults->Get(i)->GetAttribute(p));
					}
				}
				parameterSets->push_back(singleParameterSet);
			}
		}
		QStringList parameters;	parameters
			<< "#Executable"
			<< "#Additional Parameters";
		QList<QVariant> values; values
			<< samplingResults->GetExecutable()
			<< samplingResults->GetAdditionalArguments();
		dlg_commoninput checkAlgoParams(m_mdiChild, "Check/Correct Algorithm Parameters", parameters, values, NULL);
		if (checkAlgoParams.exec() != QDialog::Accepted)
		{
			return;
		}
		QString executable = checkAlgoParams.getText(0);
		QString additionalParameters = checkAlgoParams.getText(1);
		QSharedPointer<iASelectionParameterGenerator> generator(
			new iASelectionParameterGenerator(QString("Holdout Comparison, Algorithm %1").arg(s),
				parameterSets));
		auto sampler = QSharedPointer<iAImageSampler>(new iAImageSampler(
			m_mdiChild->GetModalities(),
			samplingResults->GetAttributes(),
			generator,
			0,
			m_labelCount,
			outputFolder,
			iASEAFile::DefaultSMPFileName,
			iASEAFile::DefaultSPSFileName,
			iASEAFile::DefaultCHRFileName,
			executable,
			additionalParameters,
			samplingResults->GetName(),
			"label.mhd",
			true,
			true,
			lastSamplingID+s
		));
		m_queuedSamplers.push_back(sampler);
	}
	StartNextSampler();
}


void dlg_Consensus::StartNextSampler()
{

	m_currentSampler = m_queuedSamplers.takeFirst();
	connect(m_currentSampler.data(), SIGNAL(finished()), this, SLOT(SamplerFinished()));

	m_dlgProgress = new dlg_progress(this, m_currentSampler, m_currentSampler, "Sampling Progress");
	connect(m_currentSampler.data(), SIGNAL(Progress(int)), m_dlgProgress, SLOT(SetProgress(int)));
	connect(m_currentSampler.data(), SIGNAL(Status(QString const &)), m_dlgProgress, SLOT(SetStatus(QString const &)));
	m_mdiChild->tabifyDockWidget(this, m_dlgProgress);

	m_currentSampler->start();
}


void dlg_Consensus::SamplerFinished()
{
	delete m_dlgProgress;
	m_dlgProgress = 0;
	// insert result in sampling list?
	iAImageSampler* sender = qobject_cast<iAImageSampler*> (QObject::sender());
	if (!sender)
	{
		DEBUG_LOG("Invalid SamplingFinished: No iAImageSampler sender!");
		return;
	}
	if (sender->IsAborted())
	{
		DEBUG_LOG("Parameter sampling was aborted, aborting further configuration loading steps!");
		return;
	}
	auto results = sender->GetResults();
	m_comparisonSamplingResults.push_back(results);
	m_dlgSamplings->Add(results);
	m_currentSampler.clear();
	{
		if (m_queuedSamplers.size() > 0)
		{
			StartNextSampler();
			return;
		}
	}
	
	DEBUG_LOG("Measures for loaded configuration:");
	m_comparisonMVSelection.clear();
	m_comparisonBestSelection.clear();
	for (int s = 0; s < m_comparisonSamplingResults.size(); ++s)
	{
		auto attributes = m_comparisonSamplingResults[s]->GetAttributes();
		// do ref img comparison / measure calculation for the new samplings:
		// TODO: remove duplication between here and dlg_GEMSe::CalcRefImgComp
		QVector<QSharedPointer<iAAttributeDescriptor> > measures;
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Dice", iAAttributeDescriptor::DerivedOutput, Continuous)));
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Kappa", iAAttributeDescriptor::DerivedOutput, Continuous)));
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Overall Accuracy", iAAttributeDescriptor::DerivedOutput, Continuous)));
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Precision", iAAttributeDescriptor::DerivedOutput, Continuous)));
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Recall", iAAttributeDescriptor::DerivedOutput, Continuous)));
		for (int i = 0; i<m_labelCount; ++i)
		{
			measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
				QString("Dice %1").arg(i), iAAttributeDescriptor::DerivedOutput, Continuous)));
		}
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Undecided Pixels", iAAttributeDescriptor::DerivedOutput, Discrete)));
		for (QSharedPointer<iAAttributeDescriptor> measure : measures)
		{
			measure->ResetMinMax();
			attributes->Add(measure);
		}
		for (int m = 0; m < m_comparisonSamplingResults[s]->size(); ++m)
		{
			// create selections:
			if (m_comparisonBestIDs[s].contains(m))
			{
				m_comparisonBestSelection.push_back(m_comparisonSamplingResults[s]->Get(m));
			}
			if (m_comparisonMVIDs[s].contains(m))
			{
				m_comparisonMVSelection.push_back(m_comparisonSamplingResults[s]->Get(m));
			}

			QVector<double> measureValues;
			CalculateMeasures(m_groundTruthImage,
				dynamic_cast<LabelImageType*>(m_comparisonSamplingResults[s]->Get(m)->GetLabelledImage().GetPointer()),
				m_labelCount, measureValues, true);
			// {
			// write measures and parameters to debug out:
			QString debugOut = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
				.arg(m_comparisonSamplingResults[s]->Get(m)->GetDatasetID())
				.arg(m_comparisonSamplingResults[s]->Get(m)->GetID())
				.arg(measureValues[0]) // dice
				.arg(measureValues[2]) // accuracy
				.arg(measureValues[3]) // precision
				.arg(measureValues[4]) // recall
				.arg(measureValues[measureValues.size() - 1]); // undecided
			for (int i = 0; i < m_comparisonSamplingResults[s]->Get(m)->GetAttributes()->size(); ++i)
			{
				if (m_comparisonSamplingResults[s]->Get(m)->GetAttributes()->at(i)->AttribType() == iAAttributeDescriptor::Parameter)
				{
					debugOut += QString("\t%1").arg(m_comparisonSamplingResults[s]->Get(m)->GetAttribute(i));
				}
			}
			DEBUG_LOG(debugOut);
			// }
			for (int i = 0; i<measures.size(); ++i)
			{
				int attributeID = attributes->Find(measures[i]->Name());
				m_comparisonSamplingResults[s]->Get(m)->SetAttribute(attributeID, measureValues[i]);
				attributes->at(attributeID)->AdjustMinMax(measureValues[i]);
			}
		}
	}

	// create charts for these selection:
	SelectionUncertaintyDice(m_comparisonBestSelection, "Best Parameter Sets from Comparison dataset");
	Sample(m_comparisonMVSelection, -1, m_comparisonWeightType);

	// ignore label based input weight for now
	// labelVotingFilter->SetInputLabelWeightMap(inputLabelWeightMap);

	// perform voting (sampling?) & do ref img comparisons
}

vtkIdType AddPlot(int plotType,
	vtkSmartPointer<vtkChartXY> chart,
	vtkSmartPointer<vtkTable> table,
	int xcol, int ycol,
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
	plot->SetTooltipLabelFormat("%x, %l: %y");
	plot->SetWidth(2.0);
	plot->SetInputData(table, xcol, ycol);
	vtkIdType plotID = chart->AddPlot(plot);
	return plotID;
}

void dlg_Consensus::AddResult(vtkSmartPointer<vtkTable> table, QString const & title)
{
	int idx = twSampleResults->rowCount();
	twSampleResults->setRowCount(idx + 1);
	QCheckBox * checkBox = new QCheckBox;
	//if (i == 3) checkBox->setChecked(true);
	twSampleResults->setCellWidget(idx, 0, checkBox);
	connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(CheckBoxStateChanged(int)));
	twSampleResults->setItem(idx, 1, new QTableWidgetItem(title));
	m_checkBoxResultIDMap.insert(checkBox, idx);
	if (m_results.size() != idx)
	{
		DEBUG_LOG("Results vector and table are out of sync!");
		return;
	}
	m_results.push_back(table);
}

void dlg_Consensus::UpdateWeightPlot()
{
	lbWeight->setText("Weight: " + GetWeightName(GetWeightType()));
}

void dlg_Consensus::Sample()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	Sample(selection, m_dlgGEMSe->GetSelectedCluster()->GetID(), GetWeightType());
}

void dlg_Consensus::Sample(QVector<QSharedPointer<iASingleResult> > const & selection, int selectedClusterID, int weightType)
{
	try
	{
		if (!m_groundTruthImage)
		{
			QMessageBox::warning(this, "GEMSe", "Please load a reference image first!");
			return;
		}

		if (m_cachePath.isEmpty())
		{
			m_cachePath = QFileDialog::getExistingDirectory(this, "Consensus Sampling Cache Folder", "");
			if (m_cachePath.isEmpty())
				return;
		}

		QVector<QString> columnNames;
		columnNames.push_back("Value");
		columnNames.push_back("Undecided Pixels");
		columnNames.push_back("Mean Dice");
		for (int l = 0; l < m_labelCount; ++l)
		{
			columnNames.push_back(QString("Dice Label %1").arg(l));
		}

		const int SampleCount = sbSampleCount->value();
		const int ResultCount = 10;
		const int UndecidedLabel = m_labelCount;


		vtkSmartPointer<vtkTable> tables[ResultCount];
		// TODO: sample all for different undecided pixel types:
		QString titles[ResultCount] =
		{
			// TODO: sample first five for each weight type:
			QString("Min. Absolute Percentage"),
			QString("Min. Percentage Difference"),
			QString("Ratio"),
			QString("Max. Pixel Uncertainty"),
			QString("Max. Label Voters"),

			QString("Prob.Voting/Sum Rule"),
			QString("Prob.Voting/Max Rule"),
			QString("Prob.Voting/Min Rule"),
			QString("Prob.Voting/Median Rule"),
			QString("Prob.Voting/Majority Rule"),
		};
		for (int r = 0; r < ResultCount; ++r)
		{
			tables[r] = CreateVTKTable(SampleCount, columnNames);
		}

		double absPercMin = 1.0 / m_labelCount;
		double absPercMax = 1;
		double ratioMin = 1;
		double ratioMax = selection.size();
		double labelVoterMin = 1;
		double labelVoterMax = selection.size();

		typedef itk::LabelStatisticsImageFilter<LabelImageType, LabelImageType> StatFilter;

		auto region = m_groundTruthImage->GetLargestPossibleRegion();
		auto size = region.GetSize();
		double pixelCount = size[0] * size[1] * size[2];
		
		// DEBUG_LOG("Measures for SAMPLING:");

		// TODO:
		/*
		- also sample over weight types...
		- also test with different undecided pixel classification schemes!
		*/
		for (int i = 0; i < SampleCount; ++i)
		{
			// calculate current value:
			double norm = mapToNorm(0, SampleCount, i);

			double value[ResultCount] = {
				mapNormTo(absPercMin, absPercMax, norm),		// minimum absolute percentage
				norm,											// minimum relative percentage
				mapNormTo(ratioMin, ratioMax, norm),			// ratio
				norm,											// maximum pixel uncertainty
				mapNormTo(labelVoterMin, labelVoterMax, norm),
				norm,
				norm,
				norm,
				norm,
				norm,
			};

			// calculate voting using these values:
			iAITKIO::ImagePointer result[ResultCount];
			QVector<double> undecided(ResultCount);
			const int ProbVoteCount = 5;
			result[0] = GetVotingImage(selection, value[0], -1, -1, -1, -1, weightType, m_labelCount, true, undecided[0]);
			result[1] = GetVotingImage(selection, -1, value[1], -1, -1, -1, weightType, m_labelCount, true, undecided[1]);
			result[2] = GetVotingImage(selection, -1, -1, value[2], -1, -1, weightType, m_labelCount, true, undecided[2]);
			result[3] = GetVotingImage(selection, -1, -1, -1, value[3], -1, weightType, m_labelCount, true, undecided[3]);
			result[4] = GetVotingImage(selection, -1, -1, -1, -1, value[4], weightType, m_labelCount, true, undecided[4]);
			QVector<QVector<double>>  probVoteDice;
			QVector<QVector<double>> undecidedDice;
			for (int i = 0; i < ProbVoteCount; ++i)
			{
				probVoteDice.push_back(QVector<double>());
				undecidedDice.push_back(QVector<double>());
				result[i + 5] = GetProbVotingImage(selection, value[i + 5],
					static_cast<VotingRule>(i), m_labelCount, true, undecided[i+5],
					probVoteDice[i], undecidedDice[i], m_groundTruthImage);
			}
			for (int r = 0; r < ResultCount; ++r)
			{
				LabelImageType* labelImg = dynamic_cast<LabelImageType*>(result[r].GetPointer());
				QString filename(m_cachePath + QString("/sample-method%1-sample%2.mhd").arg(r).arg(i));
				StoreImage(labelImg, filename, true);
				auto statFilter = StatFilter::New();
				statFilter->SetInput(labelImg);
				statFilter->SetLabelInput(labelImg);
				statFilter->Update();

				QVector<double> measures;
				CalculateMeasures(m_groundTruthImage, labelImg, m_labelCount, measures, true);

				double meanDice = measures[0];
				double undecidedPerc = undecided[r] / pixelCount;

				tables[r]->SetValue(i, 0, value[r]);
				tables[r]->SetValue(i, 1, undecidedPerc);
				tables[r]->SetValue(i, 2, meanDice);
				for (int l = 0; l < m_labelCount; ++l)
				{                                // hacky workaround for label 0 having "wrong" dice values"
					tables[r]->SetValue(i, 3 + l, (l==0)? measures[4+l]/ m_labelCount : measures[4 + l]);
				}
				/*
				DEBUG_LOG(QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
					.arg(r)
					.arg(i)
					.arg(measures[0]) // dice
					.arg(measures[2]) // accuracy
					.arg(measures[3]) // precision
					.arg(measures[4]) // recall
					.arg(undecidedPerc) // undecided
				);
				*/
			}
		}
		for (int i = 0; i < ResultCount; ++i)
		{
			AddResult(tables[i], "Sampling(method=" + titles[i] + ", weight=" + GetWeightName(weightType) + ", cluster=" + selectedClusterID);
		}
	}
	catch (std::exception & e)
	{
		DEBUG_LOG(QString("Exception occured while sampling loaded config: %1").arg(e.what()));
	}
}

void dlg_Consensus::CheckBoxStateChanged(int state)
{
	QCheckBox* sender = dynamic_cast<QCheckBox*>(QObject::sender());
	int id = m_checkBoxResultIDMap[sender];
	if (state == Qt::Checked)
	{
		static int colorCnt = 0;
		int colorIdx = (colorCnt++) % 12;
		QColor plotColor = m_colorTheme->GetColor(colorIdx);

		QVector<vtkIdType> plots;
		if (m_results[id]->GetNumberOfColumns() >= 3)
		{
			vtkIdType plot1 = AddPlot(vtkChart::LINE, m_consensusCharts[0].chart, m_results[id], 1, 2, plotColor);
			vtkIdType plot2 = AddPlot(vtkChart::LINE, m_consensusCharts[1].chart, m_results[id], 0, 2, plotColor);
			vtkIdType plot3 = AddPlot(vtkChart::LINE, m_consensusCharts[2].chart, m_results[id], 0, 1, plotColor);
			plots.push_back(plot1);
			plots.push_back(plot2);
			plots.push_back(plot3);
		}
		else
		{
			vtkIdType plotID = AddPlot(vtkChart::LINE, m_consensusCharts[1].chart, m_results[id], 0, 1, plotColor);
			plots.push_back(plotID);
		}
		m_plotMap.insert(id, plots);
		twSampleResults->item(id, 1)->setBackgroundColor(plotColor);
	}
	else
	{
		twSampleResults->item(id, 1)->setBackgroundColor(Qt::white);
		QVector<vtkIdType> plots = m_plotMap[id];
		if (m_results[id]->GetNumberOfColumns() >= 3)
		{
			m_consensusCharts[0].chart->RemovePlot(plots[0]);
			m_consensusCharts[1].chart->RemovePlot(plots[1]);
			m_consensusCharts[2].chart->RemovePlot(plots[2]);
		}
		else
		{
			m_consensusCharts[1].chart->RemovePlot(plots[0]);
		}
		m_plotMap.remove(id);
	}
}


void dlg_Consensus::SampledItemClicked(QTableWidgetItem * item)
{
	m_consensusCharts[3].chart->ClearPlots();
	int row = item->row();
	for (int l = 0; l < m_labelCount; ++l)
	{
		QColor plotColor = m_colorTheme->GetColor(l);
		AddPlot(vtkChart::LINE, m_consensusCharts[3].chart, m_results[row], 0, 3+l, plotColor);
	}
}


typedef itk::Image<unsigned int, 3> UIntImage;
typedef itk::CastImageFilter<LabelImageType, UIntImage> CastIntToUInt;
typedef itk::CastImageFilter<UIntImage, LabelImageType> CastUIntToInt;

void dlg_Consensus::CalcSTAPLE()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	typedef itk::MultiLabelSTAPLEImageFilter<UIntImage, UIntImage> STAPLEFilter;
	STAPLEFilter::Pointer filter = STAPLEFilter::New();
	for (int i = 0; i < selection.size(); ++i)
	{
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(selection[i]->GetLabelledImage().GetPointer());
		CastIntToUInt::Pointer caster = CastIntToUInt::New();
		caster->SetInput(lblImg);
		caster->Update();
		filter->SetInput(i, caster->GetOutput());
	}
	filter->Update();
	CastUIntToInt::Pointer castback = CastUIntToInt::New();
	castback->SetInput(filter->GetOutput());
	castback->Update();
	m_lastMVResult = castback->GetOutput();
	lbValue->setText("Value: STAPLE");
	lbWeight->setText("Weight: EM");
	m_dlgGEMSe->AddConsensusImage(m_lastMVResult, QString("STAPLE (%1)").arg(CollectedIDs(selection)));
}

void dlg_Consensus::CalcMajorityVote()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	itk::LabelVotingImageFilter<UIntImage>::Pointer filter = itk::LabelVotingImageFilter<UIntImage>::New();
	for (int i = 0; i < selection.size(); ++i)
	{
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(selection[i]->GetLabelledImage().GetPointer());
		CastIntToUInt::Pointer caster = CastIntToUInt::New();
		caster->SetInput(lblImg);
		caster->Update();
		filter->SetInput(i, caster->GetOutput());
	}
	filter->Update();
	CastUIntToInt::Pointer castback = CastUIntToInt::New();
	castback->SetInput(filter->GetOutput());
	castback->Update();
	m_lastMVResult = castback->GetOutput();
	m_dlgGEMSe->AddConsensusImage(m_lastMVResult, QString("Majority Vote (%1)").arg(CollectedIDs(selection)));
	lbValue->setText("Value: Majority Vote");
	lbWeight->setText("Weight: Equal");
}


void dlg_Consensus::CalcProbRuleVote()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	if (selection.size() == 0)
	{
		DEBUG_LOG("Please select a cluster from the tree!");
		return;
	}
	double undecided;
	QVector<double> mv, un;
	m_lastMVResult = GetProbVotingImage(selection, sbUndecidedThresh->value(), static_cast<VotingRule>(cbProbRule->currentIndex())
		, m_labelCount, cbUndecidedPixels->isChecked(), undecided,
		mv, un, m_groundTruthImage);
	m_dlgGEMSe->AddConsensusImage(m_lastMVResult, QString("Probability Vote rule=%1, thresh=%2, (%3)")
		.arg(cbProbRule->currentIndex())
		.arg(sbUndecidedThresh->value())
		.arg(CollectedIDs(selection)));
	lbValue->setText("Value: Majority Vote");
	lbWeight->setText("Weight: Equal");
}
