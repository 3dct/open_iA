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
#include "iAUncertaintyAttachment.h"

#include "iAEnsembleDescriptorFile.h"
#include "iAEnsemble.h"
#include "iAEnsembleView.h"
#include "iAHistogramView.h"
#include "iAMember.h"
#include "iAMemberView.h"
#include "iAScatterPlotView.h"
#include "iASpatialView.h"
#include "iAUncertaintyColors.h"

#include <charts/iASimpleHistogramData.h>
#include <dlg_imageproperty.h>
#include <dlg_slicer.h>
#include <iAConnector.h>
#include <iAConsole.h>
#include <iALookupTable.h>
#include <iASlicerMode.h>
#include <iAStringHelper.h>
#include <mdichild.h>
#include <mainwindow.h>
#include <qthelper/iADockWidgetWrapper.h>

#include <vtkLookupTable.h>

#include <QDir>

const int EntropyBinCount = 100;

iAUncertaintyAttachment::iAUncertaintyAttachment(MainWindow * mainWnd, MdiChild * child):
	iAModuleAttachmentToChild(mainWnd, child),
	m_labelLut(vtkSmartPointer<vtkLookupTable>::New()),
	m_newSubEnsembleID(1)
{
	m_scatterplotView = new iAScatterPlotView();
	m_memberView = new iAMemberView();
	m_spatialView = new iASpatialView();
	m_labelDistributionView = new iAHistogramView();
	m_uncertaintyDistributionView = new iAHistogramView();
	m_ensembleView = new iAEnsembleView();
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_spatialView, "Spatial View", "UncSpatialView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_memberView, "Member View", "UncMemberView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_scatterplotView, "Scatterplot View", "UncScatterplotView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_labelDistributionView, "Label Distribution", "UncLabelDistrView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_uncertaintyDistributionView, "Uncertainty Distribution", "UncUncertaintyDistrView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_ensembleView, "Ensemble View", "UncEnsembleView"));
	connect(mainWnd, &MainWindow::styleChanged, m_spatialView, &iASpatialView::StyleChanged);
	connect(mainWnd, &MainWindow::styleChanged, m_memberView, &iAMemberView::StyleChanged);
	connect(m_scatterplotView, &iAScatterPlotView::SelectionChanged, m_spatialView, &iASpatialView::UpdateSelection);
	connect(m_memberView, &iAMemberView::MemberSelected, this, &iAUncertaintyAttachment::MemberSelected);
	connect(m_ensembleView, &iAEnsembleView::EnsembleSelected, this, &iAUncertaintyAttachment::EnsembleSelected);
}


iAUncertaintyAttachment* iAUncertaintyAttachment::Create(MainWindow * mainWnd, MdiChild * child)
{
	iAUncertaintyAttachment * newAttachment = new iAUncertaintyAttachment(mainWnd, child);
	return newAttachment;
}


void iAUncertaintyAttachment::ToggleDockWidgetTitleBars()
{
	for (int i = 0; i < m_dockWidgets.size(); ++i)
	{
		m_dockWidgets[i]->toggleTitleBar();
	}
}


void iAUncertaintyAttachment::ToggleSettings()
{
	m_spatialView->ToggleSettings();
	m_scatterplotView->ToggleSettings();
}


bool iAUncertaintyAttachment::LoadEnsemble(QString const & fileName)
{
	m_ensembleFile = QSharedPointer<iAEnsembleDescriptorFile>(new iAEnsembleDescriptorFile(fileName));
	if (!m_ensembleFile->good())
	{
		DEBUG_LOG("Ensemble: Given data file could not be read.");
		return false;
	}
	connect(m_child, &MdiChild::fileLoaded, this, &iAUncertaintyAttachment::ContinueEnsembleLoading);
	if (!m_child->loadFile(m_ensembleFile->ModalityFileName(), false))
	{
		DEBUG_LOG(QString("Failed to load project '%1'").arg(m_ensembleFile->ModalityFileName()));
		return false;
	}
	return true;
}

void iAUncertaintyAttachment::ContinueEnsembleLoading()
{
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
		EnsembleSelected(ensemble);
		m_spatialView->SetupSelection(m_scatterplotView->GetSelectionImage());
	}
	m_mainWnd->showMaximized();
	m_child->showMaximized();
	m_child->splitDockWidget(m_child->slicerDockWidget(iASlicerMode::XY), m_dockWidgets[0], Qt::Horizontal);	// Spatial View
	m_child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[2], Qt::Horizontal);	// ScatterPlot View
	m_child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[5], Qt::Vertical);	// Ensemble View
	m_child->splitDockWidget(m_dockWidgets[2], m_dockWidgets[3], Qt::Vertical);	// Label Distribution View
	m_child->splitDockWidget(m_dockWidgets[3], m_dockWidgets[4], Qt::Vertical);	// Uncertainty Distribution View
	m_child->splitDockWidget(m_dockWidgets[5], m_dockWidgets[1], Qt::Horizontal);	// Member View
	m_child->slicerDockWidget(iASlicerMode::XY)->hide();
	m_child->imagePropertyDockWidget()->hide();
	if (!m_ensembleFile->LayoutName().isEmpty())
	{
		m_child->loadLayout(m_ensembleFile->LayoutName());
	}
}


void iAUncertaintyAttachment::WriteFullDataFile(QString const & fileName, bool writeIntensities, bool writeMemberLabels, bool writeMemberProbabilities, bool writeEnsembleUncertainties)
{
	m_currentEnsemble->WriteFullDataFile(fileName, writeIntensities, writeMemberLabels, writeMemberProbabilities, writeEnsembleUncertainties, m_child->modalities());
}


void iAUncertaintyAttachment::CalculateNewSubEnsemble()
{
	auto memberIDs = m_memberView->SelectedMemberIDs();
	if (memberIDs.empty())
	{
		DEBUG_LOG("No members selected!");
		return;
	}
	QSharedPointer<iAEnsemble> mainEnsemble = m_ensembleView->Ensembles()[0];
	QString cachePath;
	int subEnsembleID;
	do
	{
		subEnsembleID = m_newSubEnsembleID;
		cachePath = mainEnsemble->CachePath() + QString("/sub%1").arg(subEnsembleID);
		++m_newSubEnsembleID;
	} while (QDir(cachePath).exists());
	QSharedPointer<iAEnsemble> newEnsemble = mainEnsemble->AddSubEnsemble(memberIDs, subEnsembleID);
	mainEnsemble->EnsembleFile()->AddSubEnsemble(subEnsembleID, memberIDs);
	m_ensembleView->AddEnsemble(QString("Subset: Members %1").arg(joinAsString(memberIDs, ",")), newEnsemble);
	mainEnsemble->Store();
}


void iAUncertaintyAttachment::MemberSelected(int memberIdx)
{
	iAITKIO::ImagePointer itkImg = m_currentEnsemble->Member(memberIdx)->LabelImage();
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


void iAUncertaintyAttachment::EnsembleSelected(QSharedPointer<iAEnsemble> ensemble)
{
	m_currentEnsemble = ensemble;
	m_scatterplotView->SetDatasets(ensemble);
	m_memberView->SetEnsemble(ensemble);
	m_labelDistributionView->Clear();
	auto labelDistributionHistogram = createHistogram<int>(ensemble->GetLabelDistribution(), ensemble->LabelCount(), 0, ensemble->LabelCount()-1, Discrete);
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
	QSharedPointer<iALookupTable> labelLookup(new iALookupTable(m_labelLut));
	m_labelDistributionView->AddChart("Label", labelDistributionHistogram, iAUncertaintyColors::LabelDistributionBase, labelLookup);
	m_uncertaintyDistributionView->Clear();
	auto entropyHistogram = iASimpleHistogramData::create(0, 1, ensemble->EntropyBinCount(), ensemble->EntropyHistogram(), Continuous);
	m_uncertaintyDistributionView->AddChart("Algorithm Uncertainty", entropyHistogram, iAUncertaintyColors::UncertaintyDistribution);
	m_spatialView->SetDatasets(ensemble, m_labelLut);
}
