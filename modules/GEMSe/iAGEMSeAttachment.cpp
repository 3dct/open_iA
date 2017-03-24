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
#include "iAGEMSeAttachment.h"

#include "dlg_GEMSeControl.h"
#include "dlg_GEMSe.h"
#include "dlg_labels.h"
#include "dlg_modalities.h"
#include "dlg_samplings.h"
#include "iAChildData.h"
#include "iAConsole.h"
#include "iAColorTheme.h"
#include "iALogger.h"
#include "iAModality.h"
#include "iARenderer.h"
#include "iASlicer.h"
#include "iASlicerData.h"
#include "iASlicerWidget.h"
#include "iAWidgetAddHelper.h"
#include "mdichild.h"
#include "mainwindow.h"


iAGEMSeAttachment::iAGEMSeAttachment(MainWindow * mainWnd, iAChildData childData):
	iAModuleAttachmentToChild(mainWnd, childData),
	m_dummyTitleWidget(new QWidget())
{
}

iAGEMSeAttachment* iAGEMSeAttachment::create(MainWindow * mainWnd, iAChildData childData)
{
	MdiChild * mdiChild = childData.child;
	iAGEMSeAttachment * newAttachment = new iAGEMSeAttachment(mainWnd, childData);

	newAttachment->m_widgetAddHelper = QSharedPointer<iAWidgetAddHelper>(new iAWidgetAddHelper(mdiChild, childData.logs));
	
	QString defaultThemeName("Brewer Set3 (max. 12)");
	iAColorTheme const * colorTheme = iAColorThemeManager::GetInstance().GetTheme(defaultThemeName);
	
	newAttachment->m_dlgGEMSe = new dlg_GEMSe(mdiChild, mdiChild->getLogger(), colorTheme);
	
	newAttachment->m_dlgLabels = new dlg_labels(mdiChild, colorTheme);
	newAttachment->m_dlgSamplings = new dlg_samplings();
	newAttachment->m_dlgGEMSeControl = new dlg_GEMSeControl(
		childData.child,
		newAttachment->m_dlgGEMSe,
		childData.child->GetModalitiesDlg(),
		newAttachment->m_dlgLabels,
		newAttachment->m_dlgSamplings,
		colorTheme
	);
	mdiChild->splitDockWidget(childData.logs, newAttachment->m_dlgGEMSe, Qt::Vertical);
	mdiChild->splitDockWidget(childData.logs, newAttachment->m_dlgGEMSeControl, Qt::Horizontal);
	mdiChild->splitDockWidget(newAttachment->m_dlgGEMSeControl, newAttachment->m_dlgLabels, Qt::Vertical);
	mdiChild->splitDockWidget(newAttachment->m_dlgGEMSeControl, newAttachment->m_dlgSamplings, Qt::Vertical);

	connect(mdiChild->getRenderer(),     SIGNAL(Clicked(int, int, int)), newAttachment->m_dlgLabels, SLOT(RendererClicked(int, int, int)));
	connect(mdiChild->getSlicerDataXY(), SIGNAL(clicked(int, int, int)), newAttachment->m_dlgLabels, SLOT(SlicerClicked(int, int, int)));
	connect(mdiChild->getSlicerDataXZ(), SIGNAL(clicked(int, int, int)), newAttachment->m_dlgLabels, SLOT(SlicerClicked(int, int, int)));
	connect(mdiChild->getSlicerDataYZ(), SIGNAL(clicked(int, int, int)), newAttachment->m_dlgLabels, SLOT(SlicerClicked(int, int, int)));


	return newAttachment;
}

bool iAGEMSeAttachment::LoadSampling(QString const & smpFileName, int labelCount, int datasetID)
{
	return m_dlgGEMSeControl->LoadSampling(smpFileName, labelCount, datasetID);
}

bool iAGEMSeAttachment::LoadClustering(QString const & fileName)
{
	return m_dlgGEMSeControl->LoadClustering(fileName);
}

bool iAGEMSeAttachment::LoadSeeds(QString const & seedsFileName)
{
	return m_dlgLabels->Load(seedsFileName);
}

bool iAGEMSeAttachment::LoadReferenceImage(QString const & referenceImageName)
{
	return m_dlgGEMSeControl->LoadReferenceImage(referenceImageName);
}

void iAGEMSeAttachment::SetSerializedHiddenCharts(QString const & hiddenCharts)
{
	return m_dlgGEMSeControl->SetSerializedHiddenCharts(hiddenCharts);
}

void iAGEMSeAttachment::ResetFilter()
{
	m_dlgGEMSe->ResetFilters();
}

void iAGEMSeAttachment::ToggleAutoShrink()
{
	m_dlgGEMSe->ToggleAutoShrink();
}

void iAGEMSeAttachment::ToggleDockWidgetTitleBar()
{
	QWidget* titleBar = m_dlgGEMSe->titleBarWidget();
	if (titleBar == m_dummyTitleWidget)
	{
		m_dlgGEMSe->setTitleBarWidget(nullptr);
	}
	else
	{
		m_dlgGEMSe->setTitleBarWidget(m_dummyTitleWidget);
	}
}


void iAGEMSeAttachment::ExportClusterIDs()
{
	m_dlgGEMSeControl->ExportIDs();
}


void iAGEMSeAttachment::ExportAttributeRangeRanking()
{
	m_dlgGEMSeControl->ExportAttributeRangeRanking();
}

void iAGEMSeAttachment::ExportRankings()
{
	m_dlgGEMSeControl->ExportRankings();
}

void iAGEMSeAttachment::ImportRankings()
{
	m_dlgGEMSeControl->ImportRankings();
}

void iAGEMSeAttachment::SetLabelInfo(QString const & colorTheme, QString const & labelNames)
{
	m_dlgGEMSeControl->SetLabelInfo(colorTheme, labelNames);
}
