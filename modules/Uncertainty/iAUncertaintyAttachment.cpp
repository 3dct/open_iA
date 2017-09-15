/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "pch.h"
#include "iAUncertaintyAttachment.h"

#include "iAEnsembleDescriptorFile.h"
#include "iAEnsemble.h"
#include "iAEnsembleView.h"
#include "iAHistogramView.h"
#include "iAMember.h"
#include "iAMemberView.h"
#include "iAScatterPlotView.h"
#include "iASpatialView.h"

#include "iAChildData.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iADockWidgetWrapper.h"
#include "mdichild.h"
#include "mainwindow.h"

#include <QDir>

const int EntropyBinCount = 100;

template <typename T>
QString Join(QVector<T> const & vec, QString const & joinStr)
{
	QString result;
	bool first = true;
	for (T elem : vec)
	{
		if (!first)
			result += joinStr;
		else
			first = false;
		result += QString::number(elem);
	}
	return result;
}

iAUncertaintyAttachment::iAUncertaintyAttachment(MainWindow * mainWnd, iAChildData childData):
	iAModuleAttachmentToChild(mainWnd, childData)
{
	m_scatterplotView = new iAScatterPlotView();
	m_memberView = new iAMemberView();
	m_spatialView = new iASpatialView();
	m_histogramView = new iAHistogramView();
	m_ensembleView = new iAEnsembleView();
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_spatialView, "Spatial View", "UncSpatialView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_memberView, "Member View", "UncMemberView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_scatterplotView, "Scatterplot View", "UncScatterplotView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_histogramView, "Histogram View", "UncHistogramView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_ensembleView, "Ensemble View", "UncEnsembleView"));
	QDockWidget* splitAnchor = childData.child->getRendererDlg();
	for (auto dockWidget : m_dockWidgets)
	{
		childData.child->splitDockWidget(splitAnchor, dockWidget, Qt::Horizontal);
		splitAnchor = dockWidget;
	}
	connect(mainWnd, SIGNAL(StyleChanged()), m_spatialView, SLOT(StyleChanged()));
	connect(m_scatterplotView, SIGNAL(SelectionChanged()), this, SLOT(ChartSelectionChanged()));
	connect(m_memberView, SIGNAL(MemberSelected(int)), this, SLOT(MemberSelected(int)));
	connect(m_ensembleView, SIGNAL(EnsembleSelected(QSharedPointer<iAEnsemble>)), this, SLOT(EnsembleSelected(QSharedPointer<iAEnsemble>)));
}

iAUncertaintyAttachment* iAUncertaintyAttachment::Create(MainWindow * mainWnd, iAChildData childData)
{
	MdiChild * mdiChild = childData.child;
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
	iAEnsembleDescriptorFile ensembleFile(fileName);
	if (!ensembleFile.good())
	{
		DEBUG_LOG("Ensemble: Given data file could not be read.");
		return false;
	}
	if (!GetMdiChild()->LoadProject(ensembleFile.GetModalityFileName()))
	{
		DEBUG_LOG(QString("Ensemble: Failed loading project '%1'").arg(ensembleFile.GetModalityFileName()));
		return false;
	}
	auto ensemble = iAEnsemble::Create(EntropyBinCount, fileName, ensembleFile);
	if (ensemble)
	{
		m_ensembleView->AddEnsemble("Full Ensemble", ensemble);
		EnsembleSelected(ensemble);
	}
	return ensemble;
}

void iAUncertaintyAttachment::CalculateNewSubEnsemble()
{
	auto members = m_memberView->SelectedMembers();
	auto memberIDs = m_memberView->SelectedMemberIDs();
	if (members.empty())
	{
		DEBUG_LOG("No members selected!");
		return;
	}
	static int newEnsembleID = 1;
	QString cachePath;
	do
	{
		cachePath = m_currentEnsemble->CachePath() + QString("/sub%1").arg(newEnsembleID);
		++newEnsembleID;
	} while (QDir(cachePath).exists());
	auto newEnsemble = iAEnsemble::Create(EntropyBinCount, members, m_currentEnsemble->Sampling(0),
		m_currentEnsemble->LabelCount(), cachePath, newEnsembleID);
	m_ensembleView->AddEnsemble(QString("Subset: Members %1").arg(Join(memberIDs, ",")), newEnsemble);
}

void iAUncertaintyAttachment::ChartSelectionChanged()
{
	m_spatialView->ShowSelection(m_scatterplotView->GetSelectionImage());
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
	m_spatialView->SetDatasets(ensemble);
	m_scatterplotView->SetDatasets(ensemble);
	m_memberView->SetEnsemble(ensemble);
	m_histogramView->SetEnsemble(ensemble);
}
