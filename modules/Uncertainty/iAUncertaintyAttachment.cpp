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
#include "iAEnsembleDescriptorFile.h"
#include "iAUncertaintyAttachment.h"

#include "iAChartView.h"
#include "iAChildData.h"
#include "iAConsole.h"
#include "iADockWidgetWrapper.h"
#include "iAMemberView.h"
#include "iASamplingResults.h"
#include "iASpatialView.h"
#include "mdichild.h"

iAUncertaintyAttachment::iAUncertaintyAttachment(MainWindow * mainWnd, iAChildData childData):
	iAModuleAttachmentToChild(mainWnd, childData)
{
	m_chartView = new iAChartView();
	m_memberView = new iAMemberView();
	m_spatialView = new iASpatialView();
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_spatialView, "Spatial View", "UncSpatialView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_memberView, "Member View", "UncMemberView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_chartView, "Chart View", "UncChartView"));
	childData.child->splitDockWidget(childData.child->getRendererDlg(), m_dockWidgets[0], Qt::Horizontal);
	childData.child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[1], Qt::Vertical);
	childData.child->splitDockWidget(m_dockWidgets[1], m_dockWidgets[2], Qt::Horizontal);
}

iAUncertaintyAttachment* iAUncertaintyAttachment::create(MainWindow * mainWnd, iAChildData childData)
{
	MdiChild * mdiChild = childData.child;
	iAUncertaintyAttachment * newAttachment = new iAUncertaintyAttachment(mainWnd, childData);
	return newAttachment;
}


void iAUncertaintyAttachment::toggleDockWidgetTitleBars()
{
	for (int i = 0; i < m_dockWidgets.size(); ++i)
	{
		m_dockWidgets[i]->toggleTitleBar();
	}
}

bool iAUncertaintyAttachment::loadEnsemble(QString const & fileName)
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
	// load sampling data:
	QMap<int, QString> const & samplings = ensembleFile.GetSamplings();
	for (int key : samplings.keys())
	{
		if (!loadSampling(samplings[key], ensembleFile.GetLabelCount(), key))
		{
			DEBUG_LOG(QString("Ensemble: Could not load sampling '%1'!").arg(samplings[key]));
			return false;
		}
	}
	return true;
}


bool iAUncertaintyAttachment::loadSampling(QString const & fileName, int labelCount, int id)
{
	//m_simpleLabelInfo->SetLabelCount(labelCount);
	if (fileName.isEmpty())
	{
		DEBUG_LOG("No filename given, not loading.");
		return false;
	}
	QSharedPointer<iASamplingResults> samplingResults = iASamplingResults::Load(fileName, id);
	if (!samplingResults)
	{
		DEBUG_LOG("Loading Sampling failed.");
		return false;
	}
	QFileInfo fi(fileName);
	// load all ensemble members into member view
	// update spatial view to show representative of all
	// enable probability probing in chart view?
	return true;
}
