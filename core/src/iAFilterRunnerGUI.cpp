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
#include "iAFilterRunnerGUI.h"

#include "iAFilter.h"

#include <QSettings>

#include "dlg_commoninput.h"
#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkImageData.h>

#include <QSharedPointer>
#include <QString>
#include <QTextDocument>

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
	m_filter->SetUp(getConnector(), qobject_cast<MdiChild*>(parent())->getLogger(), getItkProgress());
	m_filter->Run(m_paramValues);
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
		default:
		case String: return "#";
		}
	}
}


// iAFilterRunnerGUI


QSharedPointer<iAFilterRunnerGUI> iAFilterRunnerGUI::Create()
{
	return QSharedPointer<iAFilterRunnerGUI>(new iAFilterRunnerGUI());
}


QMap<QString, QVariant> iAFilterRunnerGUI::LoadParameters(QSharedPointer<iAFilter> filter)
{
	auto params = filter->Parameters();
	QMap<QString, QVariant> result;
	QSettings settings;
	for (auto param : params)
	{
		QVariant default = (param->ValueType() == Categorical) ? "" : param->DefaultValue();
		result.insert(param->Name(), settings.value(SettingName(filter, param), default));
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


bool iAFilterRunnerGUI::AskForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & paramValues, MainWindow* mainWnd)
{
	auto params = filter->Parameters();
	QStringList dlgParamNames;
	QList<QVariant> dlgParamValues;
	for (auto param : params)
	{
		dlgParamNames << (ValueTypePrefix(param->ValueType()) + param->Name());
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
	QTextDocument *fDescr = new QTextDocument(0);
	fDescr->setHtml(filter->Description());
	dlg_commoninput dlg(mainWnd, filter->Name(), dlgParamNames, dlgParamValues, fDescr);
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
		case String:      value = dlg.getText(idx);          break;
		case Boolean:     value = dlg.getCheckValue(idx);    break;
		case Categorical: value = dlg.getComboBoxValue(idx); break;
		}
		paramValues[param->Name()] = value;
		++idx;
	}
	return true;
}


void iAFilterRunnerGUI::Run(QSharedPointer<iAFilter> filter, MainWindow* mainWnd)
{
	QMap<QString, QVariant> paramValues = LoadParameters(filter);
	if (!AskForParameters(filter, paramValues, mainWnd))
		return;
	StoreParameters(filter, paramValues);
	
	//! TODO: find way to check parameters already in dlg_commoninput (before closing)
	if (!filter->CheckParameters(paramValues))
		return;

	auto mdiChild = mainWnd->GetResultChild(filter->Name() + " " + mainWnd->activeMdiChild()->windowTitle());
	if (!mdiChild)
	{
		mainWnd->statusBar()->showMessage("Cannot get result child from main window!", 5000);
		return;
	}
	iAFilterRunnerGUIThread* thread = new iAFilterRunnerGUIThread(filter, paramValues, mdiChild);
	if (!thread)
	{
		mainWnd->statusBar()->showMessage("Cannot create result calculation thread!", 5000);
		return;
	}
	ConnectThreadSignals(mdiChild, thread);
	mdiChild->addStatusMsg(filter->Name());
	mainWnd->statusBar()->showMessage(filter->Name(), 5000);
	thread->start();
}

void iAFilterRunnerGUI::ConnectThreadSignals(MdiChild* mdiChild, iAFilterRunnerGUIThread* thread)
{
	mdiChild->connectThreadSignalsToChildSlots(thread);
	connect(thread, SIGNAL(finished()), this, SIGNAL(finished()));
}
