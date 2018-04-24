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
#include "iAFilterRunnerGUI.h"

#include "dlg_commoninput.h"
#include "dlg_modalities.h"
#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAFilter.h"
#include "iALogger.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkImageData.h>

#include <QMessageBox>
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QTextDocument>
#include <QVariant>

class iAFilter;

class vtkImageData;


// iAFilterRunnerGUIThread


iAFilterRunnerGUIThread::iAFilterRunnerGUIThread(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> paramValues, MdiChild* mdiChild) :
	iAAlgorithm(filter->Name(), mdiChild->getImagePointer(), mdiChild->getPolyData(), mdiChild->getLogger(), mdiChild),
	m_filter(filter),
	m_paramValues(paramValues)
{}


void iAFilterRunnerGUIThread::performWork()
{
	m_filter->SetProgress(ProgressObserver());
	for (iAConnector* con : Connectors())
		m_filter->AddInput(con);
	if (!m_filter->Run(m_paramValues))
	{
		m_filter->Logger()->Log("Running filter failed!");
		return;
	}
	allocConnectors(m_filter->Output().size());
	for (int i = 0; i < m_filter->Output().size(); ++i)
	{
		Connectors()[i]->SetImage(m_filter->Output()[i]->GetITKImage());
	}
}


QSharedPointer<iAFilter> iAFilterRunnerGUIThread::Filter()
{
	return m_filter;
}


namespace
{
	QString SettingName(QSharedPointer<iAFilter> filter, QSharedPointer<iAAttributeDescriptor> param)
	{
		QString filterNameShort(filter->Name());
		filterNameShort.replace(" ", "");
		return QString("Filters/%1/%2/%3").arg(filter->Category()).arg(filterNameShort).arg(param->Name());
	}

	QString ValueTypePrefix(iAValueType val)
	{
		switch (val)
		{
		case Continuous : return "#"; // potentially ^ for DoubleSpinBox?
		case Discrete   : return "*";
		case Boolean    : return "$";
		case Categorical: return "+";
		case Text       : return "=";
		case FilterName : return "&";
		case FilterParameters: return ".";
		case FileNameOpen: return "<";
		case FileNamesOpen: return "{";
		case FileNameSave: return ">";
		case Folder:       return ";";
		default:
		case String     : return "#";
		}
	}
}


// iAFilterRunnerGUI


QSharedPointer<iAFilterRunnerGUI> iAFilterRunnerGUI::Create()
{
	return QSharedPointer<iAFilterRunnerGUI>(new iAFilterRunnerGUI());
}


QMap<QString, QVariant> iAFilterRunnerGUI::LoadParameters(QSharedPointer<iAFilter> filter, MdiChild* sourceMdi)
{
	auto params = filter->Parameters();
	QMap<QString, QVariant> result;
	QSettings settings;
	for (auto param : params)
	{
		QVariant defaultValue = (param->ValueType() == Categorical) ? "" : param->DefaultValue();
		result.insert(param->Name(), settings.value(SettingName(filter, param), defaultValue));
	}
	return result;
}


void iAFilterRunnerGUI::StoreParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & paramValues)
{
	auto params = filter->Parameters();
	QSettings settings;
	for (auto param : params)
	{
		settings.setValue(SettingName(filter, param), paramValues[param->Name()]);
	}
}


bool iAFilterRunnerGUI::AskForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & paramValues,
	MdiChild* sourceMdi, MainWindow* mainWnd, bool askForAdditionalInput)
{
	auto params = filter->Parameters();
	if (filter->RequiredInputs() == 1 && params.empty())
		return true;
	QStringList dlgParamNames;
	QList<QVariant> dlgParamValues;
	QVector<MdiChild*> otherMdis;
	for (auto mdi : mainWnd->MdiChildList())
	{
		if (mdi != sourceMdi)
			otherMdis.push_back(mdi);
	}
	if (askForAdditionalInput && filter->RequiredInputs() > (otherMdis.size()+1) )
	{
		QMessageBox::warning(mainWnd, filter->Name(),
			QString("This filter requires %1 datasets, only %2 open file(s)!")
			.arg(filter->RequiredInputs()).arg(otherMdis.size()+1));
		return false;
	}
	bool showROI = false;
	for (auto param : params)
	{
		dlgParamNames << (ValueTypePrefix(param->ValueType()) + param->Name());
		if (param->Name() == "Index X")	// TODO: find better way to check this?
			showROI = true;
		if (param->ValueType() == Categorical)
		{
			QStringList comboValues = param->DefaultValue().toStringList();
			QString storedValue = paramValues[param->Name()].toString();
			for (int i = 0; i < comboValues.size(); ++i)
				if (comboValues[i] == storedValue)
					comboValues[i] = "!" + comboValues[i];
			dlgParamValues << comboValues;
		}
		else
		{
			dlgParamValues << paramValues[param->Name()];
		}
	}
	if (askForAdditionalInput && filter->RequiredInputs() > 1)
	{
		QStringList mdiChildrenNames;
		for (auto mdi: otherMdis)
		{
			mdiChildrenNames << mdi->windowTitle().replace("[*]", "");
		}
		for (int i = 0; i < filter->RequiredInputs()-1; ++i)
		{
			dlgParamNames << QString("+Additional Input %1").arg(i+1);
			dlgParamValues << mdiChildrenNames;
		}
	}
	QTextDocument *fDescr = new QTextDocument(0);
	fDescr->setHtml(filter->Description());
	dlg_commoninput dlg(mainWnd, filter->Name(), dlgParamNames, dlgParamValues, fDescr);
	dlg.setModal(false);
	dlg.hide();	dlg.show(); // required to apply change in modality!
	dlg.setSourceMdi(sourceMdi, mainWnd);
	if (showROI)
		dlg.showROI();
	if (dlg.exec() != QDialog::Accepted)
		return false;
	
	int idx = 0;
	for (auto param : params)
	{
		QVariant value;
		switch (param->ValueType())
		{
		case Continuous:  value = dlg.getDblValue(idx);      break;
		case Discrete:    value = dlg.getIntValue(idx);      break;
		case FilterName:
		case FilterParameters:
		case Text:
		case Folder:
		case FileNameSave:
		case FileNameOpen:
		case FileNamesOpen:
		case String:      value = dlg.getText(idx);          break;
		case Boolean:     value = dlg.getCheckValue(idx);    break;
		case Categorical: value = dlg.getComboBoxValue(idx); break;
		}
		paramValues[param->Name()] = value;
		++idx;
	}
	if (askForAdditionalInput && filter->RequiredInputs() > 1)
	{
		for (int i = 0; i < filter->RequiredInputs()-1; ++i)
		{
			int mdiIdx = dlg.getComboBoxIndex(idx);
			for (int m = 0; m < otherMdis[mdiIdx]->GetModalities()->size(); ++m)
			{
				m_additionalInput.push_back(otherMdis[mdiIdx]->GetModality(m)->GetImage());
			}
			++idx;

		}
	}
	return true;
}

void iAFilterRunnerGUI::FilterGUIPreparations(QSharedPointer<iAFilter> filter, MdiChild* mdiChild, MainWindow* mainWnd)
{
}

void iAFilterRunnerGUI::Run(QSharedPointer<iAFilter> filter, MainWindow* mainWnd)
{
	MdiChild* sourceMdi = mainWnd->activeMdiChild();
	if (filter->RequiredInputs() > 0 && (!sourceMdi || !sourceMdi->IsFullyLoaded()))
	{
		mainWnd->statusBar()->showMessage("Please wait until file is fully loaded!");
		return;
	}

	filter->SetLogger(sourceMdi->getLogger());
	QMap<QString, QVariant> paramValues = LoadParameters(filter, sourceMdi);

	if (!AskForParameters(filter, paramValues, sourceMdi, mainWnd, true))
		return;
	StoreParameters(filter, paramValues);
	
	//! TODO: find way to check parameters already in dlg_commoninput (before closing)
	if (!filter->CheckParameters(paramValues))
		return;

	QString oldTitle(sourceMdi->windowTitle());
	oldTitle = oldTitle.replace("[*]", "").trimmed();
	auto mdiChild = filter->OutputCount() > 0 ?
		mainWnd->GetResultChild(sourceMdi, filter->Name() + " " + oldTitle) :
		sourceMdi;
	if (!mdiChild)
	{
		mainWnd->statusBar()->showMessage("Cannot create result child!", 5000);
		return;
	}
	FilterGUIPreparations(filter, mdiChild, mainWnd);
	iAFilterRunnerGUIThread* thread = new iAFilterRunnerGUIThread(filter, paramValues, mdiChild);
	if (!thread)
	{
		mainWnd->statusBar()->showMessage("Cannot create result calculation thread!", 5000);
		return;
	}
	// TODO: move all image adding here?
	for (int m = 1; m < sourceMdi->GetModalities()->size(); ++m)
	{
		thread->AddImage(sourceMdi->GetModality(m)->GetImage());
	}
	filter->SetFirstInputChannels(sourceMdi->GetModalities()->size());
	for (auto img : m_additionalInput)
	{
		thread->AddImage(img);
	}
	if (thread->Connectors().size() < filter->RequiredInputs())
	{
		mdiChild->addMsg(QString("Not enough inputs specified, filter %1 requires %2 input images!")
			.arg(filter->Name()).arg(filter->RequiredInputs()));
		return;
	}
	ConnectThreadSignals(mdiChild, thread);
	mdiChild->addStatusMsg(filter->Name());
	mainWnd->statusBar()->showMessage(filter->Name(), 5000);
	thread->start();
}

void iAFilterRunnerGUI::ConnectThreadSignals(MdiChild* mdiChild, iAFilterRunnerGUIThread* thread)
{
	connect(thread, SIGNAL(finished()), this, SLOT(FilterFinished()));
	mdiChild->connectThreadSignalsToChildSlots(thread);
}

void iAFilterRunnerGUI::FilterFinished()
{
	auto thread = qobject_cast<iAFilterRunnerGUIThread*>(sender());
	// add additional output as additional modalities here
	// "default" output 0 is handled elsewhere
	auto mdiChild = qobject_cast<MdiChild*>(thread->parent());
	if (thread->Filter()->Output().size() > 1)
	{
		for (int p = 1; p < thread->Filter()->Output().size(); ++p)
		{
			auto img = vtkSmartPointer<vtkImageData>::New();
			// some filters apparently clean up the result image
			// (disregarding that a smart pointer still points to it...)
			// so let's copy it to be on the safe side!
			img->DeepCopy(thread->Filter()->Output()[p]->GetVTKImage());
			QSharedPointer<iAModality> mod(new iAModality(QString("Extra Out %1").arg(p), "", -1, img, 0));
			mdiChild->GetModalities()->Add(mod);
			// signal to add it to list automatically is created to late to be effective here, we have to add it to list ourselves:
			mdiChild->GetModalitiesDlg()->ModalityAdded(mod);
		}
	}
	for (auto outputValue : thread->Filter()->OutputValues())
		mdiChild->addMsg(QString("%1: %2").arg(outputValue.first).arg(outputValue.second.toString()));

	emit finished();
}
