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

#include "charts/iASimpleHistogramData.h"
#include "dlg_imageproperty.h"
#include "iAChildData.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "qthelper/iADockWidgetWrapper.h"
#include "iALookupTable.h"
#include "iAStringHelper.h"
#include "mdichild.h"
#include "mainwindow.h"

#include <vtkLookupTable.h>

#include <QDir>

const int EntropyBinCount = 100;

iAUncertaintyAttachment::iAUncertaintyAttachment(MainWindow * mainWnd, iAChildData childData):
	iAModuleAttachmentToChild(mainWnd, childData),
	m_newSubEnsembleID(1),
	m_labelLut(vtkSmartPointer<vtkLookupTable>::New())
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
	connect(mainWnd, SIGNAL(StyleChanged()), m_spatialView, SLOT(StyleChanged()));
	connect(mainWnd, SIGNAL(StyleChanged()), m_memberView, SLOT(StyleChanged()));
	connect(mainWnd, SIGNAL(StyleChanged()), m_scatterplotView, SLOT(StyleChanged()));
	connect(m_scatterplotView, SIGNAL(SelectionChanged()), m_spatialView, SLOT(UpdateSelection()));
	connect(m_memberView, SIGNAL(MemberSelected(int)), this, SLOT(MemberSelected(int)));
	connect(m_ensembleView, SIGNAL(EnsembleSelected(QSharedPointer<iAEnsemble>)), this, SLOT(EnsembleSelected(QSharedPointer<iAEnsemble>)));
}


iAUncertaintyAttachment* iAUncertaintyAttachment::Create(MainWindow * mainWnd, iAChildData childData)
{
	iAUncertaintyAttachment * newAttachment = new iAUncertaintyAttachment(mainWnd, childData);
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
	connect(GetMdiChild(), SIGNAL(fileLoaded()), this, SLOT(ContinueEnsembleLoading()));
	if (!GetMdiChild()->loadFile(m_ensembleFile->ModalityFileName(), false))
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
	m_childData.child->showMaximized();
	m_childData.child->splitDockWidget(m_childData.child->getSlicerDlgXY(), m_dockWidgets[0], Qt::Horizontal);	// Spatial View
	m_childData.child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[2], Qt::Horizontal);	// ScatterPlot View
	m_childData.child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[5], Qt::Vertical);	// Ensemble View
	m_childData.child->splitDockWidget(m_dockWidgets[2], m_dockWidgets[3], Qt::Vertical);	// Label Distribution View
	m_childData.child->splitDockWidget(m_dockWidgets[3], m_dockWidgets[4], Qt::Vertical);	// Uncertainty Distribution View
	m_childData.child->splitDockWidget(m_dockWidgets[5], m_dockWidgets[1], Qt::Horizontal);	// Member View
	m_childData.child->getSlicerDlgXY()->hide();
	m_childData.child->getImagePropertyDlg()->hide();
	if (!m_ensembleFile->LayoutName().isEmpty())
	{
		m_childData.child->LoadLayout(m_ensembleFile->LayoutName());
	}
}


void iAUncertaintyAttachment::WriteFullDataFile(QString const & fileName, bool writeIntensities, bool writeMemberLabels, bool writeMemberProbabilities, bool writeEnsembleUncertainties)
{
	m_currentEnsemble->WriteFullDataFile(fileName, writeIntensities, writeMemberLabels, writeMemberProbabilities, writeEnsembleUncertainties, m_childData.child->GetModalities());
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
	m_ensembleView->AddEnsemble(QString("Subset: Members %1").arg(Join(memberIDs, ",")), newEnsemble);
	mainEnsemble->Store();
}


void iAUncertaintyAttachment::MemberSelected(int memberIdx)
{
	iAITKIO::ImagePointer itkImg = m_currentEnsemble->Member(memberIdx)->LabelImage();
	iAConnector con;
	con.SetImage(itkImg);
	bool keep = QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
	m_spatialView->AddMemberImage(QString("Member #%1").arg(memberIdx), con.GetVTKImage(), keep);
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
	auto labelDistributionHistogram = CreateHistogram<int>(ensemble->GetLabelDistribution(), ensemble->LabelCount(), 0, ensemble->LabelCount()-1, Discrete);
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
	auto entropyHistogram = iASimpleHistogramData::Create(0, 1, ensemble->EntropyBinCount(), ensemble->EntropyHistogram(), Continuous);
	m_uncertaintyDistributionView->AddChart("Algorithm Uncertainty", entropyHistogram, iAUncertaintyColors::UncertaintyDistribution);
	m_spatialView->SetDatasets(ensemble, m_labelLut);
}
