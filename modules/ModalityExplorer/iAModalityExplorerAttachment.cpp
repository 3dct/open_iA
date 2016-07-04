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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAModalityExplorerAttachment.h"

#include "dlg_modalities.h"
#include "dlg_modalityRenderer.h"
#include "dlg_planeSlicer.h"
#include "iAChannelVisualizationData.h"
#include "iAChildData.h"
#include "iAConsole.h"
#include "iALogger.h"
#include "iAModality.h"
#include "iARenderSettings.h"
#include "iASlicer.h"
#include "iASlicerWidget.h"
#include "iAWidgetAddHelper.h"
#include "mdichild.h"
#include "mainwindow.h"

#include <QFileDialog>

#include <fstream>
#include <sstream>
#include <string>


iAModalityExplorerAttachment::iAModalityExplorerAttachment(MainWindow * mainWnd, iAChildData childData):
	iAModuleAttachmentToChild(mainWnd, childData),
	m_currentModality(0)
{
}

iAModalityExplorerAttachment* iAModalityExplorerAttachment::create(MainWindow * mainWnd, iAChildData childData)
{
	MdiChild * mdiChild = childData.child;
	iAModalityExplorerAttachment * newAttachment = new iAModalityExplorerAttachment(mainWnd, childData);

	dlg_modalityRenderer * renderWidget = new dlg_modalityRenderer();
	mdiChild->splitDockWidget(mdiChild->logs, renderWidget, Qt::Horizontal);
	renderWidget->hide();

	dlg_planeSlicer* planeSlicer = new dlg_planeSlicer();
	mdiChild->splitDockWidget(renderWidget, planeSlicer, Qt::Horizontal);
	planeSlicer->hide();
		
	return newAttachment;
}

void iAModalityExplorerAttachment::ChangeModality(int chg)
{
	SetCurrentModality(
		(GetCurrentModality() + chg + GetModalitiesDlg()->GetModalities()->size())
		% (GetModalitiesDlg()->GetModalities()->size())
	);
	int curModIdx = GetCurrentModality();
	if (curModIdx < 0 || curModIdx >= GetModalitiesDlg()->GetModalities()->size())
	{
		DEBUG_LOG("Invalid modality index!");
		return;
	}
	ChangeImage(GetModalitiesDlg()->GetModalities()->Get(curModIdx)->GetImage(),
		GetModalitiesDlg()->GetModalities()->Get(curModIdx)->GetName().toStdString());
}

void iAModalityExplorerAttachment::ChangeMagicLensOpacity(int chg)
{
	iASlicerWidget * sliceWidget = dynamic_cast<iASlicerWidget *>(sender());
	if (!sliceWidget)
	{
		DEBUG_LOG("Invalid slice widget sender!");
		return;
	}
	sliceWidget->SetMagicLensOpacity(sliceWidget->GetMagicLensOpacity() + (chg*0.05));
}

int iAModalityExplorerAttachment::GetCurrentModality() const
{
	return m_currentModality;
}

void iAModalityExplorerAttachment::SetCurrentModality(int modality)
{
	m_currentModality = modality;
}

void iAModalityExplorerAttachment::ChangeImage(vtkSmartPointer<vtkImageData> img)
{
	int selected = m_dlgModalities->GetSelected();
	if (selected != -1)
	{
		m_currentModality = selected;
		ChangeImage(img, m_dlgModalities->GetModalities()->Get(selected)->GetName().toStdString());
	}
}

void iAModalityExplorerAttachment::ChangeImage(vtkSmartPointer<vtkImageData> img, std::string const & caption)
{
	if (!m_childData.child->isMagicLensToggled())
	{
		return;
	}
	m_childData.child->reInitMagicLens(ch_ModalityLens, img,
		m_dlgModalities->GetCTF(m_currentModality),
		m_dlgModalities->GetOTF(m_currentModality), caption);
}

void iAModalityExplorerAttachment::SetModalities(QSharedPointer<iAModalityList> modList)
{
	return m_dlgModalities->SetModalities(modList);
}

void iAModalityExplorerAttachment::MagicLensToggled(bool isOn)
{
	if (isOn)
	{
		iAChannelVisualizationData * chData = m_childData.child->GetChannelData( ch_ModalityLens );
		if( !chData )
		{
			chData = new iAChannelVisualizationData();
			m_childData.child->InsertChannelData( ch_ModalityLens, chData );
		}
		m_currentModality = m_dlgModalities->GetSelected();
		vtkSmartPointer<vtkImageData> img = m_dlgModalities->GetModalities()->Get(m_currentModality)->GetImage();
		chData->SetActiveImage(img);
		chData->SetColorTF( m_dlgModalities->GetCTF(m_currentModality) );
		chData->SetOpacityTF(m_dlgModalities->GetOTF(m_currentModality));
		chData->SetOpacity(0.5);
		m_childData.child->InitChannelRenderer( ch_ModalityLens, false, false );
		m_childData.child->SetMagicLensInput(ch_ModalityLens, true);
		m_childData.child->SetMagicLensCaption(m_dlgModalities->GetModalities()->Get(m_currentModality)->GetName().toStdString());
	}
	m_childData.child->SetMagicLensEnabled(isOn);
	
	m_childData.child->updateSlicers();
}

void iAModalityExplorerAttachment::RenderSettingsChanged()
{
	m_dlgModalities->ChangeRenderSettings(m_childData.child->GetRenderSettings());
}

void iAModalityExplorerAttachment::preferencesChanged()
{
	m_dlgModalities->ChangeMagicLensSize(m_childData.child->GetMagicLensSize());
}

dlg_modalities* iAModalityExplorerAttachment::GetModalitiesDlg()
{
	return m_dlgModalities;
}

bool iAModalityExplorerAttachment::LoadModalities()
{
	m_dlgModalities->Load();
	return (m_dlgModalities->GetModalities()->size() > 0);
}
