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

#include "dlg_imageproperty.h"
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
#include "iAStringHelper.h"
#include "mdichild.h"
#include "mainwindow.h"

#include <QDir>

const int EntropyBinCount = 100;

iAUncertaintyAttachment::iAUncertaintyAttachment(MainWindow * mainWnd, iAChildData childData):
	iAModuleAttachmentToChild(mainWnd, childData),
	m_newSubEnsembleID(1)
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
	connect(mainWnd, SIGNAL(StyleChanged()), m_spatialView, SLOT(StyleChanged()));
	connect(m_scatterplotView, SIGNAL(SelectionChanged()), m_spatialView, SLOT(UpdateSelection()));
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
	QSharedPointer<iAEnsembleDescriptorFile> ensembleFile(new iAEnsembleDescriptorFile(fileName));
	if (!ensembleFile->good())
	{
		DEBUG_LOG("Ensemble: Given data file could not be read.");
		return false;
	}
	if (!GetMdiChild()->LoadProject(ensembleFile->ModalityFileName()))
	{
		DEBUG_LOG(QString("Ensemble: Failed loading project '%1'").arg(ensembleFile->ModalityFileName()));
		return false;
	}
	auto ensemble = iAEnsemble::Create(EntropyBinCount, ensembleFile);
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
	m_childData.child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[4], Qt::Vertical);	// Ensemble View
	m_childData.child->splitDockWidget(m_dockWidgets[2], m_dockWidgets[3], Qt::Vertical);	// Histogram View
	m_childData.child->splitDockWidget(m_dockWidgets[4], m_dockWidgets[1], Qt::Horizontal);	// Member View
	m_childData.child->getSlicerDlgXY()->hide();
	m_childData.child->getImagePropertyDlg()->hide();
	m_mainWnd->resizeDocks({ m_dockWidgets[2], m_dockWidgets[3] }, { 400 , 200 }, Qt::Vertical);
	m_mainWnd->resizeDocks({ m_dockWidgets[4], m_dockWidgets[1] }, { 100 , 200 }, Qt::Horizontal);
	m_mainWnd->resizeDocks({ m_dockWidgets[4], m_dockWidgets[1] }, { 100 , 200 }, Qt::Vertical);
	if (!ensembleFile->LayoutName().isEmpty())
	{
		m_childData.child->LoadLayout(ensembleFile->LayoutName());
	}
	return ensemble;
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
	m_spatialView->SetDatasets(ensemble);
	m_scatterplotView->SetDatasets(ensemble);
	m_memberView->SetEnsemble(ensemble);
	m_histogramView->SetEnsemble(ensemble);
}
