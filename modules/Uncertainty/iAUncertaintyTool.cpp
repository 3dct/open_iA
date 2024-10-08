// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAUncertaintyTool.h"

#include "iAEnsembleDescriptorFile.h"
#include "iAEnsemble.h"
#include "iAEnsembleView.h"
#include "iAHistogramView.h"
#include "iAMemberView.h"
#include "iAScatterPlotView.h"
#include "iASingleResult.h"
#include "iASpatialView.h"
#include "iAUncertaintyColors.h"

#include <iASlicerMode.h>
#include <iAMdiChild.h>
#include <iAMainWindow.h>

#include <iADockWidgetWrapper.h>

#include <iAHistogramData.h>

#include <iAConnector.h>
#include <iALog.h>
#include <iALookupTable.h>
#include <iAParameterDlg.h>
#include <iAStringHelper.h>

#include <vtkLookupTable.h>

#include <QDir>
#include <QFileDialog>
#include <QGuiApplication>

const int EntropyBinCount = 100;

iAUncertaintyTool::iAUncertaintyTool(iAMainWindow * mainWnd, iAMdiChild * child):
	iATool(mainWnd, child),
	m_labelLut(vtkSmartPointer<vtkLookupTable>::New()),
	m_newSubEnsembleID(1)
{
	m_scatterplotView = new iAScatterPlotView();
	m_memberView = new iAMemberView();
	m_spatialView = new iASpatialView();
	m_labelDistributionView = new iAHistogramView();
	m_uncertaintyDistributionView = new iAHistogramView();
	m_ensembleView = new iAEnsembleView();
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_spatialView, "Spatial View", "UncSpatialView", "https://github.com/3dct/open_iA/wiki/Uncertainty"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_memberView, "Member View", "UncMemberView", "https://github.com/3dct/open_iA/wiki/Uncertainty"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_scatterplotView, "Scatterplot View", "UncScatterplotView", "https://github.com/3dct/open_iA/wiki/Uncertainty"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_labelDistributionView, "Label Distribution", "UncLabelDistrView", "https://github.com/3dct/open_iA/wiki/Uncertainty"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_uncertaintyDistributionView, "Uncertainty Distribution", "UncUncertaintyDistrView", "https://github.com/3dct/open_iA/wiki/Uncertainty"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_ensembleView, "Ensemble View", "UncEnsembleView", "https://github.com/3dct/open_iA/wiki/Uncertainty"));
	connect(mainWnd, &iAMainWindow::styleChanged, m_spatialView, &iASpatialView::StyleChanged);
	connect(mainWnd, &iAMainWindow::styleChanged, m_memberView, &iAMemberView::StyleChanged);
	connect(m_scatterplotView, &iAScatterPlotView::SelectionChanged, m_spatialView, &iASpatialView::UpdateSelection);
	connect(m_memberView, &iAMemberView::MemberSelected, this, &iAUncertaintyTool::memberSelected);
	connect(m_ensembleView, &iAEnsembleView::EnsembleSelected, this, &iAUncertaintyTool::ensembleSelected);

	m_mainWindow->showMaximized();
	m_child->showMaximized();
	m_child->splitDockWidget(m_child->slicerDockWidget(iASlicerMode::XY), m_dockWidgets[0], Qt::Horizontal);	// Spatial View
	m_child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[2], Qt::Horizontal);	// ScatterPlot View
	m_child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[5], Qt::Vertical);	// Ensemble View
	m_child->splitDockWidget(m_dockWidgets[2], m_dockWidgets[3], Qt::Vertical);	// Label Distribution View
	m_child->splitDockWidget(m_dockWidgets[3], m_dockWidgets[4], Qt::Vertical);	// Uncertainty Distribution View
	m_child->splitDockWidget(m_dockWidgets[5], m_dockWidgets[1], Qt::Horizontal);	// Member View
	m_child->slicerDockWidget(iASlicerMode::XY)->hide();
	m_child->dataInfoDockWidget()->hide();
}

void iAUncertaintyTool::toggleDockWidgetTitleBars()
{
	for (int i = 0; i < m_dockWidgets.size(); ++i)
	{
		m_dockWidgets[i]->toggleTitleBar();
	}
}

void iAUncertaintyTool::toggleSettings()
{
	m_spatialView->ToggleSettings();
	m_scatterplotView->ToggleSettings();
}

void iAUncertaintyTool::loadState(QSettings& projectFile, QString const& fileName)
{
	m_ensembleFile = std::make_shared<iAEnsembleDescriptorFile>(projectFile, fileName);
	if (!m_ensembleFile->good())
	{
		LOG(lvlError, "Ensemble: Given data file could not be read.");
		return;
	}
	auto ensemble = iAEnsemble::Create(EntropyBinCount, m_ensembleFile);
	if (ensemble)
	{
		m_ensembleView->AddEnsemble("Full Ensemble", ensemble);
		for (auto subEnsemble : ensemble->SubEnsembles())
		{
			if (subEnsemble->ID() > m_newSubEnsembleID)
				m_newSubEnsembleID = subEnsemble->ID();
			m_ensembleView->AddEnsemble(QString("SubEnsemble %1").arg(subEnsemble->ID()), subEnsemble);
		}
		ensembleSelected(ensemble);
		m_spatialView->SetupSelection(m_scatterplotView->GetSelectionImage());
	}
	if (!m_ensembleFile->layoutName().isEmpty())
	{
		m_child->loadLayout(m_ensembleFile->layoutName());
	}
}

void iAUncertaintyTool::writeFullDataFile()
{
	QString fileName = QFileDialog::getSaveFileName(m_mainWindow,
		tr("Save Full Data file"),
		m_mainWindow->activeMdiChild() ? m_child->filePath() : QString(),
		tr("SVM file format (*.svm);;All files (*)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAAttributes params;
	addAttr(params, "Write original data values", iAValueType::Boolean, true);
	addAttr(params, "Write Member Labels", iAValueType::Boolean, true);
	addAttr(params, "Write Member Probabilities", iAValueType::Boolean, true);
	addAttr(params, "Write Ensemble Uncertainties", iAValueType::Boolean, true);
	iAParameterDlg dlg(m_mainWindow, "Write parameters", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto val = dlg.parameterValues();
	m_currentEnsemble->writeFullDataFile(fileName, val["Write original data values"].toBool(), val["Write Member Labels"].toBool(),
		val["Write Member Probabilities"].toBool(), val["Write Ensemble Uncertainties"].toBool(), m_child->dataSetMap());
}

void iAUncertaintyTool::saveState(QSettings& projectFile, QString const& fileName)
{
	m_ensembleFile->store(projectFile, fileName);
}

void iAUncertaintyTool::calculateNewSubEnsemble()
{
	auto memberIDs = m_memberView->SelectedMemberIDs();
	if (memberIDs.empty())
	{
		LOG(lvlError, "No members selected!");
		return;
	}
	std::shared_ptr<iAEnsemble> mainEnsemble = m_ensembleView->Ensembles()[0];
	QString cachePath;
	int subEnsembleID;
	do
	{
		subEnsembleID = m_newSubEnsembleID;
		cachePath = mainEnsemble->CachePath() + QString("/sub%1").arg(subEnsembleID);
		++m_newSubEnsembleID;
	} while (QDir(cachePath).exists());
	std::shared_ptr<iAEnsemble> newEnsemble = mainEnsemble->AddSubEnsemble(memberIDs, subEnsembleID);
	mainEnsemble->EnsembleFile()->addSubEnsemble(subEnsembleID, memberIDs);
	m_ensembleView->AddEnsemble(QString("Subset: Members %1").arg(joinNumbersAsString(memberIDs, ",")), newEnsemble);
}

void iAUncertaintyTool::memberSelected(int memberIdx)
{
	iAITKIO::ImagePointer itkImg = m_currentEnsemble->Member(memberIdx)->labelImage();
	iAConnector con;
	con.setImage(itkImg);
	bool keep = QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
	m_spatialView->AddMemberImage(QString("Member #%1").arg(memberIdx), con.vtkImage(), keep);
	if (!keep)
	{
		m_shownMembers.clear();
	}
	m_shownMembers.push_back(itkImg);
}


void iAUncertaintyTool::ensembleSelected(std::shared_ptr<iAEnsemble> ensemble)
{
	m_currentEnsemble = ensemble;
	m_scatterplotView->SetDatasets(ensemble);
	m_memberView->SetEnsemble(ensemble);
	m_labelDistributionView->Clear();
	auto labelDistributionHistogram = createHistogramData<int>("Label Frequency", iAValueType::Discrete, ensemble->GetLabelDistribution(),
		ensemble->LabelCount(), 0, ensemble->LabelCount() - 1);
	double lutRange[2];
	lutRange[0] = 0;
	lutRange[1] = m_currentEnsemble->LabelCount();
	m_labelLut->SetRange(lutRange);
	m_labelLut->SetTableRange(lutRange);
	m_labelLut->SetNumberOfTableValues(m_currentEnsemble->LabelCount());
	double rgba[4];
	QColor c(iAUncertaintyColors::LabelDistributionBase);
	QColor curColor;
	for (vtkIdType i = 0; i < m_currentEnsemble->LabelCount(); i++)
	{
		curColor.setHslF(c.hueF(), c.saturationF(), (static_cast<double>(m_currentEnsemble->LabelCount()-i) / (m_currentEnsemble->LabelCount()+1)));
		rgba[0] = curColor.redF();
		rgba[1] = curColor.greenF();
		rgba[2] = curColor.blueF();
		rgba[3] = curColor.alphaF();
		m_labelLut->SetTableValue(i, rgba);
	}
	m_labelLut->Build();
	auto labelLookup = std::make_shared<iALookupTable>(m_labelLut);
	m_labelDistributionView->AddChart("Label", labelDistributionHistogram, iAUncertaintyColors::LabelDistributionBase, labelLookup);
	m_uncertaintyDistributionView->Clear();
	auto entropyHistogram = iAHistogramData::create("Frequency (Pixels)", iAValueType::Continuous,
		0, 1, ensemble->EntropyBinCount(), ensemble->EntropyHistogram());
	m_uncertaintyDistributionView->AddChart("Algorithm Uncertainty", entropyHistogram, iAUncertaintyColors::UncertaintyDistribution);
	m_spatialView->SetDatasets(ensemble, m_labelLut);
}
