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
#include "iAParameterDlg.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkImageData.h>
#include <vtkPolyData.h>

#include <QMessageBox>
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QVariant>

class iAFilter;

class vtkImageData;


// iAFilterRunnerGUIThread

iAFilterRunnerGUIThread::iAFilterRunnerGUIThread(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> paramValues, MdiChild* mdiChild) :
	iAAlgorithm(filter->name(), mdiChild->imagePointer(), mdiChild->polyData(), mdiChild->logger(), mdiChild),
	m_filter(filter),
	m_paramValues(paramValues)
{}

void iAFilterRunnerGUIThread::performWork()
{
	m_filter->setProgress(ProgressObserver());
	for (iAConnector* con : Connectors())
		m_filter->addInput(con);
	if (!m_filter->run(m_paramValues))
	{
		m_filter->logger()->log("Running filter failed!");
		return;
	}
	allocConnectors(m_filter->output().size());
	for (int i = 0; i < m_filter->output().size(); ++i)
	{
		Connectors()[i]->setImage(m_filter->output()[i]->itkImage());
	}
}

QSharedPointer<iAFilter> iAFilterRunnerGUIThread::filter()
{
	return m_filter;
}


namespace
{
	QString SettingName(QSharedPointer<iAFilter> filter, QSharedPointer<iAAttributeDescriptor> param)
	{
		QString filterNameShort(filter->name());
		filterNameShort.replace(" ", "");
		return QString("Filters/%1/%2/%3").arg(filter->category()).arg(filterNameShort).arg(param->name());
	}
}


// iAFilterRunnerGUI


QSharedPointer<iAFilterRunnerGUI> iAFilterRunnerGUI::create()
{
	return QSharedPointer<iAFilterRunnerGUI>(new iAFilterRunnerGUI());
}

QMap<QString, QVariant> iAFilterRunnerGUI::loadParameters(QSharedPointer<iAFilter> filter, MdiChild* /*sourceMdi*/)
{
	auto params = filter->parameters();
	QMap<QString, QVariant> result;
	QSettings settings;
	for (auto param : params)
	{
		QVariant defaultValue = (param->valueType() == Categorical) ? "" : param->defaultValue();
		result.insert(param->name(), settings.value(SettingName(filter, param), defaultValue));
	}
	return result;
}

void iAFilterRunnerGUI::storeParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & paramValues)
{
	auto params = filter->parameters();
	QSettings settings;
	for (auto param : params)
	{
		settings.setValue(SettingName(filter, param), paramValues[param->name()]);
	}
}

bool iAFilterRunnerGUI::askForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & paramValues,
	MdiChild* sourceMdi, MainWindow* mainWnd, bool askForAdditionalInput)
{
	QVector<pParameter> dlgParams;
	bool showROI = false;	// TODO: find better way to check this?
	for (auto filterParam : filter->parameters())
	{
		pParameter p(filterParam->clone());
		if (p->valueType() == Categorical)
		{
			QStringList comboValues = p->defaultValue().toStringList();
			QString storedValue = paramValues[p->name()].toString();
			for (int i = 0; i < comboValues.size(); ++i)
			{
				if (comboValues[i] == storedValue)
				{
					comboValues[i] = "!" + comboValues[i];
				}
			}
			p->setDefaultValue(comboValues);
		}
		else
		{
			p->setDefaultValue(paramValues[p->name()]);
		}
		if (p->name() == "Index X")
		{
			showROI = true;
		}
		dlgParams.push_back(p);
	}
	if (filter->requiredInputs() == 1 && dlgParams.empty())
	{
		return true;
	}
	QVector<MdiChild*> otherMdis;
	for (auto mdi : mainWnd->mdiChildList())
	{
		if (mdi != sourceMdi)
		{
			otherMdis.push_back(mdi);
		}
	}
	if (askForAdditionalInput && filter->requiredInputs() > (otherMdis.size()+1) )
	{
		QMessageBox::warning(mainWnd, filter->name(),
			QString("This filter requires %1 datasets, only %2 open file(s)!")
			.arg(filter->requiredInputs()).arg(otherMdis.size()+1));
		return false;
	}
	QStringList mdiChildrenNames;
	if (askForAdditionalInput && filter->requiredInputs() > 1)
	{
		for (auto mdi: otherMdis)
		{
			mdiChildrenNames << mdi->windowTitle().replace("[*]", "");
		}
		for (int i = 1; i < filter->requiredInputs(); ++i)
		{
			dlgParams.push_back(iAAttributeDescriptor::createParam(
				QString("%1").arg(filter->inputName(i)), Categorical, mdiChildrenNames));
		}
	}
	iAParameterDlg dlg(mainWnd, filter->name(), dlgParams, filter->description());
	dlg.setModal(false);
	dlg.hide();	dlg.show(); // required to apply change in modality!
	dlg.setSourceMdi(sourceMdi, mainWnd);
	if (showROI)
	{
		dlg.showROI();
	}
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	paramValues = dlg.parameterValues();
	if (askForAdditionalInput && filter->requiredInputs() > 1)
	{
		for (int i = 1; i < filter->requiredInputs(); ++i)
		{
			QString selectedFile = paramValues[QString("%1").arg(filter->inputName(i))].toString();
			int mdiIdx = mdiChildrenNames.indexOf(selectedFile);
			for (int m = 0; m < otherMdis[mdiIdx]->modalities()->size(); ++m)
			{
				m_additionalInput.push_back(otherMdis[mdiIdx]->modality(m)->image());
			}
		}
	}
	return true;
}

void iAFilterRunnerGUI::filterGUIPreparations(QSharedPointer<iAFilter> /*filter*/, MdiChild* /*mdiChild*/, MainWindow* /*mainWnd*/)
{
}

void iAFilterRunnerGUI::run(QSharedPointer<iAFilter> filter, MainWindow* mainWnd)
{
	MdiChild* sourceMdi = mainWnd->activeMdiChild();
	if (filter->requiredInputs() > 0 && (!sourceMdi || !sourceMdi->isFullyLoaded()))
	{
		mainWnd->statusBar()->showMessage("Please wait until file is fully loaded!");
		return;
	}

	filter->setLogger(sourceMdi->logger());
	QMap<QString, QVariant> paramValues = loadParameters(filter, sourceMdi);

	if (!askForParameters(filter, paramValues, sourceMdi, mainWnd, true))
	{
		return;
	}
	storeParameters(filter, paramValues);

	//! TODO: find way to check parameters already in dlg_commoninput (before closing)
	if (!filter->checkParameters(paramValues))
	{
		return;
	}

	QString oldTitle(sourceMdi->windowTitle());
	oldTitle = oldTitle.replace("[*]", "").trimmed();
	auto mdiChild = filter->outputCount() > 0 ?
		mainWnd->resultChild(sourceMdi, filter->outputName(0, filter->name() + " " + oldTitle)) :
		sourceMdi;
	filter->setLogger(mdiChild->logger());
	if (!mdiChild)
	{
		mainWnd->statusBar()->showMessage("Cannot create result child!", 5000);
		return;
	}
	filterGUIPreparations(filter, mdiChild, mainWnd);
	iAFilterRunnerGUIThread* thread = new iAFilterRunnerGUIThread(filter, paramValues, mdiChild);
	if (!thread)
	{
		mainWnd->statusBar()->showMessage("Cannot create result calculation thread!", 5000);
		return;
	}
	// TODO: move all image adding here?
	for (int m = 1; m < sourceMdi->modalities()->size(); ++m)
	{
		thread->AddImage(sourceMdi->modality(m)->image());
	}
	filter->setFirstInputChannels(sourceMdi->modalities()->size());
	for (auto img : m_additionalInput)
	{
		thread->AddImage(img);
	}
	if (thread->Connectors().size() < filter->requiredInputs())
	{
		mdiChild->addMsg(QString("Not enough inputs specified, filter %1 requires %2 input images!")
			.arg(filter->name()).arg(filter->requiredInputs()));
		return;
	}
	if (mdiChild->preferences().PrintParameters && !filter->parameters().isEmpty())
	{
		mdiChild->addMsg(QString("Starting %1 filter with parameters:").arg(thread->filter()->name()));
		for (int p = 0; p < thread->filter()->parameters().size(); ++p)
		{
			auto paramDescriptor = thread->filter()->parameters()[p];
			QString paramName = paramDescriptor->name();
			QString paramValue = paramDescriptor->valueType() == Boolean ?
				(paramValues[paramName].toBool() ? "yes" : "no")
				: paramValues[paramName].toString();
			mdiChild->addMsg(QString("    %1 = %2").arg(paramName).arg(paramValue));
		}
	}
	else
	{
		mdiChild->addMsg(QString("Starting %1 filter.").arg(thread->filter()->name()));
	}
	connectThreadSignals(mdiChild, thread);
	mdiChild->addStatusMsg(filter->name());
	mainWnd->statusBar()->showMessage(filter->name(), 5000);
	thread->start();
}

void iAFilterRunnerGUI::connectThreadSignals(MdiChild* mdiChild, iAFilterRunnerGUIThread* thread)
{
	connect(thread, &QThread::finished, this, &iAFilterRunnerGUI::filterFinished);
	mdiChild->connectThreadSignalsToChildSlots(thread);
}

void iAFilterRunnerGUI::filterFinished()
{
	auto thread = qobject_cast<iAFilterRunnerGUIThread*>(sender());
	// add additional output as additional modalities here
	// "default" output 0 is handled elsewhere (via obscure MdiChild::rendererDeactivated / iAAlgorithm::updateVtkImageData)
	auto mdiChild = qobject_cast<MdiChild*>(thread->parent());
	if (thread->filter()->polyOutput())
	{
		mdiChild->polyData()->DeepCopy(thread->filter()->polyOutput());
	}
	if (thread->filter()->output().size() > 1)
	{
		for (int p = 1; p < thread->filter()->output().size(); ++p)
		{
			auto img = vtkSmartPointer<vtkImageData>::New();
			// some filters apparently clean up the result image
			// (disregarding that a smart pointer still points to it...)
			// so let's copy it to be on the safe side!
			img->DeepCopy(thread->filter()->output()[p]->vtkImage());
			QSharedPointer<iAModality> mod(new iAModality(thread->filter()->outputName(p, QString("Extra Out %1").arg(p)), "", -1, img, 0));
			mdiChild->modalities()->add(mod);
			// signal to add it to list automatically is created to late to be effective here, we have to add it to list ourselves:
			mdiChild->modalitiesDockWidget()->modalityAdded(mod);
		}
	}
	for (auto outputValue : thread->filter()->outputValues())
	{
		mdiChild->addMsg(QString("%1: %2").arg(outputValue.first).arg(outputValue.second.toString()));
	}

	emit finished();
}
