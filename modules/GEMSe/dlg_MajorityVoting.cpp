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
#include "iAIOProvider.h"
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
#include <QFileDialog>
#include <QMessageBox>


// Where to put temporary output

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


dlg_MajorityVoting::dlg_MajorityVoting(MdiChild* mdiChild, dlg_GEMSe* dlgGEMSe, int labelCount, QString const & folder) :
	m_mdiChild(mdiChild),
	m_dlgGEMSe(dlgGEMSe),
	m_labelCount(labelCount),
	m_chartDiceVsUndec(vtkSmartPointer<vtkChartXY>::New()),
	m_chartValueVsDice(vtkSmartPointer<vtkChartXY>::New()),
	m_chartValueVsUndec(vtkSmartPointer<vtkChartXY>::New()),
	m_folder(folder)
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
	yAxis1->SetTitle("Mean Dice");
	yAxis1->SetLogScale(false);
	contextView->GetScene()->AddItem(m_chartDiceVsUndec);
	iADockWidgetWrapper * w(new iADockWidgetWrapper(vtkWidget, "Mean Dice vs. Undecided", "ChartDiceVsUndec"));
	mdiChild->splitDockWidget(this, w, Qt::Vertical);

	auto vtkWidget2 = new QVTKWidget2();
	auto contextView2 = vtkSmartPointer<vtkContextView>::New();
	contextView2->SetRenderWindow(vtkWidget2->GetRenderWindow());
	m_chartValueVsDice->SetSelectionMode(vtkContextScene::SELECTION_NONE);
	auto xAxis2 = m_chartValueVsDice->GetAxis(vtkAxis::BOTTOM);
	auto yAxis2 = m_chartValueVsDice->GetAxis(vtkAxis::LEFT);
	xAxis2->SetTitle("Value");
	xAxis2->SetLogScale(false);
	yAxis2->SetTitle("Mean Dice");
	yAxis2->SetLogScale(false);
	contextView2->GetScene()->AddItem(m_chartValueVsDice);
	iADockWidgetWrapper * w2(new iADockWidgetWrapper(vtkWidget2, "Value vs. Dice", "ChartValueVsDice"));
	mdiChild->splitDockWidget(this, w2, Qt::Vertical);

	auto vtkWidget3 = new QVTKWidget2();
	auto contextView3 = vtkSmartPointer<vtkContextView>::New();
	contextView3->SetRenderWindow(vtkWidget3->GetRenderWindow());
	m_chartValueVsUndec->SetSelectionMode(vtkContextScene::SELECTION_NONE);
	auto xAxis3 = m_chartValueVsUndec->GetAxis(vtkAxis::BOTTOM);
	auto yAxis3 = m_chartValueVsUndec->GetAxis(vtkAxis::LEFT);
	xAxis3->SetTitle("Value");
	xAxis3->SetLogScale(false);
	yAxis3->SetTitle("Undecided Pixels");
	yAxis3->SetLogScale(false);
	contextView3->GetScene()->AddItem(m_chartValueVsUndec);
	iADockWidgetWrapper * w3(new iADockWidgetWrapper(vtkWidget3, "Value vs. Undecided", "ChartValueVsUndec"));
	mdiChild->splitDockWidget(this, w3, Qt::Vertical);

	QSharedPointer<iAImageTreeNode> root = dlgGEMSe->GetRoot();
	int ensembleSize = root->GetClusterSize();
	slMinRatio->setMaximum(ensembleSize*100);
	slLabelVoters->setMaximum(ensembleSize);

	connect(pbSample, SIGNAL(clicked()), this, SLOT(Sample()));
	connect(pbMinAbsPercent_Plot, SIGNAL(clicked()), this, SLOT(MinAbsPlot()));
	connect(pbMinDiffPercent_Plot, SIGNAL(clicked()), this, SLOT(MinDiffPlot()));
	connect(pbMinRatio_Plot, SIGNAL(clicked()), this, SLOT(RatioPlot()));
	connect(pbMaxPixelEntropy_Plot, SIGNAL(clicked()), this, SLOT(MaxPixelEntropyPlot()));
	connect(pbClusterUncertaintyDice, SIGNAL(clicked()), this, SLOT(ClusterUncertaintyDice()));
	connect(pbStore, SIGNAL(clicked()), this, SLOT(StoreResult()));
	connect(pbStoreConfig, SIGNAL(clicked()), this, SLOT(StoreConfig()));
	connect(pbLoadConfig, SIGNAL(clicked()), this, SLOT(LoadConfig()));
	connect(slAbsMinPercent, SIGNAL(valueChanged(int)), this, SLOT(AbsMinPercentSlider(int)));
	connect(slMinDiffPercent, SIGNAL(valueChanged(int)), this, SLOT(MinDiffPercentSlider(int)));
	connect(slMinRatio, SIGNAL(valueChanged(int)), this, SLOT(MinRatioSlider(int)));
	connect(slMaxPixelEntropy, SIGNAL(valueChanged(int)), this, SLOT(MaxPixelEntropySlider(int)));
	connect(slLabelVoters, SIGNAL(valueChanged(int)), this, SLOT(LabelVoters(int)));
}

int dlg_MajorityVoting::GetWeightType()
{
	if (rbWeightLabelDice->isChecked())  return LabelBased;
	if (rbWeightCertainty->isChecked())  return Certainty;
	if (rbWeightDiffFBGSBG->isChecked()) return FBGSBGDiff;
	else return Equal;
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

void dlg_MajorityVoting::SetGroundTruthImage(LabelImagePointer groundTruthImage)
{
	m_groundTruthImage = groundTruthImage;
}

#include "iAAttributes.h"

void dlg_MajorityVoting::ClusterUncertaintyDice()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	QSharedPointer<iAImageTreeNode> node = m_dlgGEMSe->GetSelectedCluster();
	"Cluster (id=" + QString::number(node->GetID()) + ")";
}

void dlg_MajorityVoting::SelectionUncertaintyDice(
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

iAITKIO::ImagePointer GetMajorityVotingImage(QVector<QSharedPointer<iASingleResult> > selection,
	double minAbsPercentage, double minDiffPercentage, double minRatio, double maxPixelEntropy,
	int labelVoters, int weightType, int labelCount)
{
	auto labelVotingFilter = GetLabelVotingFilter(
		selection, minAbsPercentage, minDiffPercentage, minRatio, maxPixelEntropy, labelVoters, weightType, labelCount);
	if (!labelVotingFilter)
		return iAITKIO::ImagePointer();
	LabelImagePointer labelResult = labelVotingFilter->GetOutput();
	iAITKIO::ImagePointer result = dynamic_cast<iAITKIO::ImageBaseType *>(labelResult.GetPointer());
	return result;
}

iAITKIO::ImagePointer GetMajorityVotingNumbers(QVector<QSharedPointer<iASingleResult> > selection,
	double minAbsPercentage, double minDiffPercentage, double minRatio, double maxPixelEntropy,
	int labelVoters, int weightType, int labelCount, int mode)
{
	auto labelVotingFilter = GetLabelVotingFilter(
		selection, minAbsPercentage, minDiffPercentage, minRatio, maxPixelEntropy, labelVoters, weightType, labelCount);
	if (!labelVotingFilter)
		return iAITKIO::ImagePointer();
	typedef LabelVotingType::DoubleImg::Pointer DblImgPtr;
	DblImgPtr labelResult = labelVotingFilter->GetNumbers(mode);
	iAITKIO::ImagePointer result = dynamic_cast<iAITKIO::ImageBaseType *>(labelResult.GetPointer());
	return result;
}


void dlg_MajorityVoting::AbsMinPercentSlider(int)
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	double minAbs = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	lbValue->setText("Value: Abs. Min % = "+QString::number(minAbs * 100, 'f', 2) + " %");
	UpdateWeightPlot();
	m_lastMVResult = GetMajorityVotingImage(selection, minAbs, -1, -1, -1, -1, GetWeightType(), m_labelCount);
	m_dlgGEMSe->AddMajorityVotingImage(m_lastMVResult);
}

void dlg_MajorityVoting::MinDiffPercentSlider(int)
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	double minDiff = static_cast<double>(slMinDiffPercent->value()) / slMinDiffPercent->maximum();
	lbValue->setText("Value: Min. Diff. % = "+QString::number(minDiff * 100, 'f', 2) + " %");
	UpdateWeightPlot();
	m_lastMVResult = GetMajorityVotingImage(selection, -1, minDiff, -1, -1, -1, GetWeightType(), m_labelCount);
	m_dlgGEMSe->AddMajorityVotingImage(m_lastMVResult);
}

void dlg_MajorityVoting::MinRatioSlider(int)
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	double minRatio = static_cast<double>(slMinRatio->value()) / 100;
	lbValue->setText("Value: Ratio = "+QString::number(minRatio, 'f', 2));
	UpdateWeightPlot();
	m_lastMVResult = GetMajorityVotingImage(selection, -1, -1, minRatio, -1, -1, GetWeightType(), m_labelCount);
	m_dlgGEMSe->AddMajorityVotingImage(m_lastMVResult);
}

void dlg_MajorityVoting::MaxPixelEntropySlider(int)
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	double maxPixelEntropy = static_cast<double>(slMaxPixelEntropy->value()) / slMaxPixelEntropy->maximum();
	lbValue->setText("Value: Max. Pixel Ent. = " + QString::number(maxPixelEntropy*100, 'f', 2)+" %");
	UpdateWeightPlot();
	m_lastMVResult = GetMajorityVotingImage(selection, -1, -1, -1, maxPixelEntropy, -1, GetWeightType(), m_labelCount);
	m_dlgGEMSe->AddMajorityVotingImage(m_lastMVResult);
}

void dlg_MajorityVoting::LabelVoters(int)
{
	if (!m_groundTruthImage)
	{
		DEBUG_LOG("Please load a reference image first!");
		return;
	}
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	int labelVoters = slLabelVoters->value();
	lbValue->setText("Value: Label Voters = " + QString::number(labelVoters));
	UpdateWeightPlot();
	m_lastMVResult = GetMajorityVotingImage(selection, -1, -1, -1, -1, labelVoters, GetWeightType(), m_labelCount);
	m_dlgGEMSe->AddMajorityVotingImage(m_lastMVResult);
}

void dlg_MajorityVoting::MinAbsPlot()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	double minAbs = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	m_lastMVResult = GetMajorityVotingNumbers(selection, minAbs, -1, -1, -1, -1, GetWeightType(), m_labelCount, AbsolutePercentage);
	m_dlgGEMSe->AddMajorityVotingNumbers(m_lastMVResult);
}

void dlg_MajorityVoting::MinDiffPlot()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	double minDiff = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	m_lastMVResult = GetMajorityVotingNumbers(selection, -1, minDiff, -1, -1, -1, GetWeightType(), m_labelCount, DiffPercentage);
	m_dlgGEMSe->AddMajorityVotingNumbers(m_lastMVResult);
}

void dlg_MajorityVoting::RatioPlot()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	double minRatio = static_cast<double>(slAbsMinPercent->value()) / slAbsMinPercent->maximum();
	m_lastMVResult = GetMajorityVotingNumbers(selection, -1, -1, minRatio, -1, -1, GetWeightType(), m_labelCount, Ratio);
	m_dlgGEMSe->AddMajorityVotingNumbers(m_lastMVResult);
}


void dlg_MajorityVoting::MaxPixelEntropyPlot()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	double maxPixelEntropy = static_cast<double>(slMaxPixelEntropy->value()) / slMaxPixelEntropy->maximum();
	m_lastMVResult = GetMajorityVotingNumbers(selection, -1, -1, -1, maxPixelEntropy, -1, GetWeightType(), m_labelCount, PixelEntropy);
	m_dlgGEMSe->AddMajorityVotingNumbers(m_lastMVResult);
}

void dlg_MajorityVoting::StoreResult()
{
	if (!m_lastMVResult)
	{
		DEBUG_LOG("You need to perform Majority Voting at least once, before last Majority Voting result can be stored!");
		return;
	}
	iAITKIO::ScalarPixelType pixelType = itk::ImageIOBase::INT;
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Store Last Majority Voting Result"),
		m_folder,
		iAIOProvider::GetSupportedSaveFormats()
	);
	if (fileName.isEmpty())
	{
		return;
	}
	iAITKIO::writeFile(fileName, m_lastMVResult, pixelType);
}

#include "iAFileUtils.h"
#include "iASamplingResults.h"

#include <QSettings>

namespace
{
	static const QString FileFormatKey("FileFormat");
	static const QString FileVersion("v1");
	static const QString LabelsKey("Labels");
	static const QString DerivedOutputName = "Dice";
}

void dlg_MajorityVoting::StoreConfig()
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

	// fetch config for (last?) majority voting (sampling?)
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	QStringList parameterSets;
	for (int i = 0; i < selection.size(); ++i)
	{
		parameterSets.append(QString::number(selection[i]->GetDatasetID()) + "-"+QString::number(selection[i]->GetID()));
	}
	int weightType = GetWeightType();
	s.setValue("MajorityVoting/ParameterSets", parameterSets.join(","));
	s.setValue("MajorityVoting/WeightType", GetWeightName(weightType));
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
			s.setValue(QString("MajorityVoting/InputWeightLabel%1").arg(l), inputWeights.join(","));
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

void dlg_MajorityVoting::LoadConfig()
{
	if (!m_groundTruthImage)
	{
		QMessageBox::warning(this, "GEMSe", "Please load a reference image first!");
		return;
	}
	// load config
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

	QStringList mvParamSetsList = s.value("MajorityVoting/ParameterSets").toString().split(",");
	m_comparisonWeightType = ::GetWeightType(s.value("MajorityVoting/WeightType").toString());

	std::map<std::pair<int, int>, double> inputLabelWeightMap;
	if (m_comparisonWeightType == LabelBased)
	{
		bool ok;
		for (int l = 0; l < m_labelCount; ++l)
		{
			QStringList inputWeights = s.value(QString("MajorityVoting/InputWeightLabel%1").arg(l)).toString().split(",");
			for (int m = 0; m < mvParamSetsList.size(); ++m)
			{
				double labelWeight = inputWeights[m].toDouble(&ok);
				if (!ok)
				{
					DEBUG_LOG(QString("Error in label weights for label %1, entry %2('%3')").arg(l).arg(m).arg(inputWeights[m]));
				}
				inputLabelWeightMap.insert(
					std::make_pair(std::make_pair(l, m), labelWeight));
			}
		}
	}
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

		auto samplingResults = iASamplingResults::Load(smpFileName, s);
		//m_comparisonSamplingInput.push_back(samplingResults);
		// after sampling, we shouldn't need these anymore
		// and as long as we don't access the images, they shouldn't get loaded...

		ParameterSetsPointer parameterSets(new QVector<ParameterSet>());
		for (int i = 0; i < samplingResults->size(); ++i)
		{
			if (bestParameterSetIDs[s].contains(i) || mvParameterSetIDs[s].contains(i))
			{
				if (bestParameterSetIDs[s].contains(i))
				{
					m_comparisonBestIDs[s].push_back(parameterSets->size());
				}
				if (mvParameterSetIDs[s].contains(i))
				{
					//assert(mvParameterSetIDs[s].contains(i))
					m_comparisonMVIDs[s].push_back(parameterSets->size());
				}
				QVector<double> singleParameterSet;
				for (int p = 0; p < samplingResults->GetAttributes()->size(); ++p)
				{
					if (samplingResults->GetAttributes()->at(p)->GetAttribType() == iAAttributeDescriptor::Parameter)
					{
						singleParameterSet.push_back(samplingResults->Get(i)->GetAttribute(p));
					}
				}
				parameterSets->push_back(singleParameterSet);
			}
		}

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
			samplingResults->GetExecutable(),
			samplingResults->GetAdditionalArguments()
		));
		if (s == 0)
		{
			m_dlgProgress = new dlg_progress(this, sampler, sampler, "Sampling Progress");
			m_mdiChild->splitDockWidget(this, m_dlgProgress, Qt::Vertical);
			connect(sampler.data(), SIGNAL(finished()), this, SLOT(SamplerFinished()));
			connect(sampler.data(), SIGNAL(Progress(int)), m_dlgProgress, SLOT(SetProgress(int)));
			connect(sampler.data(), SIGNAL(Status(QString const &)), m_dlgProgress, SLOT(SetStatus(QString const &)));
			sampler->start();
		}
		else
		{
			m_sampler.push_back(sampler);
		}
	}
}

void dlg_MajorityVoting::SamplerFinished()
{
	// insert result in sampling list?
	iAImageSampler* sender = qobject_cast<iAImageSampler*> (QObject::sender());
	if (!sender)
	{
		DEBUG_LOG("Invalid SamplingFinished: No iAImageSampler sender!");
		return;
	}
	auto results = sender->GetResults();
	m_comparisonSamplingResults.push_back(results);
	emit SamplingAdded(results);
	if (m_sampler.size() > 0)
	{
		auto sampler = m_sampler.takeFirst();
		connect(sampler.data(), SIGNAL(finished()), this, SLOT(SamplerFinished()));
		connect(sampler.data(), SIGNAL(Progress(int)), m_dlgProgress, SLOT(SetProgress(int)));
		connect(sampler.data(), SIGNAL(Status(QString const &)), m_dlgProgress, SLOT(SetStatus(QString const &)));
		sampler->start();
	}
	else
	{
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
					m_labelCount, measureValues);
				for (int i = 0; i<measures.size(); ++i)
				{
					int attributeID = attributes->Find(measures[i]->GetName());
					m_comparisonSamplingResults[s]->Get(m)->SetAttribute(attributeID, measureValues[i]);
					attributes->at(attributeID)->AdjustMinMax(measureValues[i]);
				}
			}
		}

		// create charts for these selection:
		SelectionUncertaintyDice(m_comparisonBestSelection, "Best Parameter Sets from Comparison dataset");
		Sample(m_comparisonMVSelection, m_comparisonWeightType);

		// ignore label based input weight for now
		// labelVotingFilter->SetInputLabelWeightMap(inputLabelWeightMap);

		// perform majority voting (sampling?) & do ref img comparisons

		delete m_dlgProgress;
		m_dlgProgress = 0;
	}
}

vtkIdType AddPlot(int plotType,
	vtkSmartPointer<vtkChartXY> chart,
	vtkSmartPointer<vtkTable> table,
	int col1, int col2,
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
	plot->SetWidth(1.0);
	plot->SetInputData(table, col1, col2);
	vtkIdType plotID = chart->AddPlot(plot);
	return plotID;
}

void dlg_MajorityVoting::AddResult(vtkSmartPointer<vtkTable> table, QString const & title)
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

void dlg_MajorityVoting::UpdateWeightPlot()
{
	lbWeight->setText("Weight: " + GetWeightName(GetWeightType()));
}

void dlg_MajorityVoting::Sample()
{
	QVector<QSharedPointer<iASingleResult> > selection;
	m_dlgGEMSe->GetSelection(selection);
	Sample(selection, GetWeightType());
}

void dlg_MajorityVoting::Sample(QVector<QSharedPointer<iASingleResult> > const & selection, int weightType)
{
	if (!m_groundTruthImage)
	{
		QMessageBox::warning(this, "GEMSe", "Please load a reference image first!");
		return;
	}

	QVector<QString> columnNames;
	columnNames.push_back("Value");
	columnNames.push_back("Undecided Pixels");
	columnNames.push_back("Mean Dice");

	const int SampleCount = sbSampleCount->value();
	const int ResultCount = 5;
	const int UndecidedLabel = m_labelCount;

	vtkSmartPointer<vtkTable> tables[ResultCount];
	QString titles[ResultCount] =
	{
		QString("Min. Absolute Percentage"),
		QString("Min. Percentage Difference"),
		QString("Ratio"),
		QString("Max. Pixel Uncertainty"),
		QString("Max. Label Voters")
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

	typedef fhw::FilteringLabelOverlapMeasuresImageFilter<LabelImageType> DiceFilter;
	typedef itk::LabelStatisticsImageFilter<LabelImageType, LabelImageType> StatFilter;

	auto region = m_groundTruthImage->GetLargestPossibleRegion();
	auto size = region.GetSize();
	double pixelCount = size[0] * size[1] * size[2];

	for (int i = 0; i < SampleCount; ++i)
	{
		// calculate current value:
		double norm = mapToNorm(0, SampleCount, i);

		double value[ResultCount] = {
			mapNormTo(absPercMin, absPercMax, norm),		// minimum absolute percentage
			norm,											// minimum relative percentage
			mapNormTo(ratioMin, ratioMax, norm),			// ratio
			norm,											// maximum pixel uncertainty
			mapNormTo(labelVoterMin, labelVoterMax, norm)
		};

		// calculate majority voting using these values:
		iAITKIO::ImagePointer result[ResultCount];

		result[0] = GetMajorityVotingImage(selection, value[0], -1, -1, -1, -1, weightType, m_labelCount);
		result[1] = GetMajorityVotingImage(selection, -1, value[1], -1, -1, -1, weightType, m_labelCount);
		result[2] = GetMajorityVotingImage(selection, -1, -1, value[2], -1, -1, weightType, m_labelCount);
		result[3] = GetMajorityVotingImage(selection, -1, -1, -1, value[3], -1, weightType, m_labelCount);
		result[4] = GetMajorityVotingImage(selection, -1, -1, -1, -1, value[4], weightType, m_labelCount);

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
	}

	QString ids;
	for (int s = 0; s < selection.size(); ++s)
	{
		ids += QString::number(selection[s]->GetDatasetID()) + "-" + QString::number(selection[s]->GetID());
		if (s < selection.size() - 1)
		{
			ids += ", ";
		}
	}

	int startIdx = twSampleResults->rowCount();
	for (int i = 0; i < ResultCount; ++i)
	{
		AddResult(tables[i], "Sampling(w=" + GetWeightName(weightType) + ",value=" + titles[i] + ",ids=" + ids);
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

		QVector<vtkIdType> plots;
		if (m_results[id]->GetNumberOfColumns() == 3)
		{
			vtkIdType plot1 = AddPlot(vtkChart::POINTS, m_chartDiceVsUndec, m_results[id], 1, 2, plotColor);
			vtkIdType plot2 = AddPlot(vtkChart::LINE, m_chartValueVsDice, m_results[id], 0, 2, plotColor);
			vtkIdType plot3 = AddPlot(vtkChart::LINE, m_chartValueVsUndec, m_results[id], 0, 1, plotColor);
			plots.push_back(plot1);
			plots.push_back(plot2);
			plots.push_back(plot3);
		}
		else
		{
			vtkIdType plotID = AddPlot(vtkChart::POINTS, m_chartValueVsDice, m_results[id], 0, 1, plotColor);
			plots.push_back(plotID);
		}
		m_plotMap.insert(id, plots);
		twSampleResults->item(id, 1)->setBackgroundColor(plotColor);
	}
	else
	{
		twSampleResults->item(id, 1)->setBackgroundColor(Qt::white);
		QVector<vtkIdType> plots = m_plotMap[id];
		if (m_results[id]->GetNumberOfColumns() == 3)
		{
			m_chartDiceVsUndec->RemovePlot(plots[0]);
			m_chartValueVsDice->RemovePlot(plots[1]);
			m_chartValueVsUndec->RemovePlot(plots[2]);
		}
		else
		{
			m_chartValueVsDice->RemovePlot(plots[0]);
		}
		m_plotMap.remove(id);
	}
}
