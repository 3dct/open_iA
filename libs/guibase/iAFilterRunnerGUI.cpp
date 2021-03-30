/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

// base
#include "iAAttributeDescriptor.h"
#include "iAFileUtils.h"
#include "iALogger.h"

#include "dlg_modalities.h"
#include "iAConnector.h"
#include "iALog.h"
#include "iAFilter.h"
#include "iAJobListView.h"
#include "iAMainWindow.h"
#include "iAMdiChild.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAParameterDlg.h"
#include "iAPreferences.h"

#include <vtkImageData.h>
#include <vtkPolyData.h>

#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QSharedPointer>
#include <QStatusBar>
#include <QString>
#include <QVariant>

class iAFilter;

class vtkImageData;


// iAFilterRunnerGUIThread

iAFilterRunnerGUIThread::iAFilterRunnerGUIThread(QSharedPointer<iAFilter> filter,
	QMap<QString, QVariant> paramValues, iAMdiChild* mdiChild, QString const & fileName) :
	iAAlgorithm(filter->name(), mdiChild->imagePointer(), mdiChild->polyData(), iALog::get(), mdiChild),
	m_filter(filter),
	m_paramValues(paramValues),
	m_aborted(false)
{
	m_fileNames.push_back(fileName);
}

void iAFilterRunnerGUIThread::performWork()
{
	m_filter->setProgress(ProgressObserver());
	assert(Connectors().size() == m_fileNames.size());
	for (int i = 0; i < Connectors().size(); ++i)
	{
		m_filter->addInput(Connectors()[i], m_fileNames[i]);
	}
	if (!m_filter->run(m_paramValues))
	{
		m_filter->logger()->log(lvlError, "Running filter failed!");
		return;
	}
	if (m_aborted)
	{
		return;
	}
	allocConnectors(m_filter->output().size());
	for (int i = 0; i < m_filter->output().size(); ++i)
	{
		Connectors()[i]->setImage(m_filter->output()[i]->itkImage());
	}
}

void iAFilterRunnerGUIThread::abort()
{
	m_aborted = true;
	m_filter->abort();
}

QSharedPointer<iAFilter> iAFilterRunnerGUIThread::filter()
{
	return m_filter;
}

void iAFilterRunnerGUIThread::addInput(vtkImageData* img, QString const& fileName)
{
	AddImage(img);
	m_fileNames.push_back(fileName);
}


namespace
{
	QString SettingName(QSharedPointer<iAFilter> filter, QString paramName)
	{
		QString filterNameShort(filter->name());
		filterNameShort.replace(" ", "");
		return QString("Filters/%1/%2/%3").arg(filter->category()).arg(filterNameShort).arg(paramName);
	}
}


// iAFilterRunnerGUI


QSharedPointer<iAFilterRunnerGUI> iAFilterRunnerGUI::create()
{
	return QSharedPointer<iAFilterRunnerGUI>::create();
}

QMap<QString, QVariant> iAFilterRunnerGUI::loadParameters(QSharedPointer<iAFilter> filter, iAMdiChild* sourceMdi)
{
	auto params = filter->parameters();
	QMap<QString, QVariant> result;
	QSettings settings;
	for (auto param : params)
	{
		QVariant defaultValue = (param->valueType() == iAValueType::Categorical) ? "" : param->defaultValue();
		QVariant value = (param->valueType() == iAValueType::FileNameSave) ?
			pathFileBaseName(sourceMdi->fileInfo()) + param->defaultValue().toString() :
			settings.value(SettingName(filter, param->name()), defaultValue);
		result.insert(param->name(), value);
	}
	return result;
}

void iAFilterRunnerGUI::storeParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & paramValues)
{
	auto params = filter->parameters();
	QSettings settings;
	for (QString key : paramValues.keys())
	{
		settings.setValue(SettingName(filter, key), paramValues[key]);
	}
	// just some checking whether there are values for all parameters:
	for (auto param : params)
	{
		if (!paramValues.contains(param->name()))
		{
			LOG(lvlError, QString("No value for parameter '%1'").arg(param->name()));
		}
	}
}

bool iAFilterRunnerGUI::askForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & paramValues,
	iAMdiChild* sourceMdi, iAMainWindow* mainWnd, bool askForAdditionalInput)
{
	iAAttributes dlgParams;
	bool showROI = false;	// TODO: find better way to check this?
	for (auto filterParam : filter->parameters())
	{
		QSharedPointer<iAAttributeDescriptor> p(filterParam->clone());
		if (p->valueType() == iAValueType::Categorical)
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
	QVector<iAMdiChild*> otherMdis;
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
				QString("%1").arg(filter->inputName(i)), iAValueType::Categorical, mdiChildrenNames));
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
				m_additionalFileNames.push_back(otherMdis[mdiIdx]->modality(m)->fileName());
			}
		}
	}
	return true;
}

void iAFilterRunnerGUI::filterGUIPreparations(QSharedPointer<iAFilter> /*filter*/,
	iAMdiChild* /*mdiChild*/, iAMainWindow* /*mainWnd*/, QMap<QString, QVariant> const & /*params*/)
{
}

void iAFilterRunnerGUI::run(QSharedPointer<iAFilter> filter, iAMainWindow* mainWnd)
{
	iAMdiChild* sourceMdi = mainWnd->activeMdiChild();
	if (filter->requiredInputs() > 0 && (!sourceMdi || !sourceMdi->isFullyLoaded()))
	{
		mainWnd->statusBar()->showMessage("Please wait until file is fully loaded!");
		emit finished();
		return;
	}
	QMap<QString, QVariant> paramValues = loadParameters(filter, sourceMdi);
	filter->adaptParametersToInput(paramValues, sourceMdi->modality(0)->image());

	if (!askForParameters(filter, paramValues, sourceMdi, mainWnd, true))
	{
		emit finished();
		return;
	}
	storeParameters(filter, paramValues);

	//! TODO: find way to check parameters already in iAParameterDlg (before closing)
	if (!filter->checkParameters(paramValues))
	{
		emit finished();
		return;
	}

	QString oldTitle(sourceMdi->windowTitle());
	oldTitle = oldTitle.replace("[*]", "").trimmed();
	QString newTitle(filter->outputName(0, filter->name()) + " " + oldTitle);
	m_sourceFileName = sourceMdi->modality(0)->fileName();
	auto mdiChild = filter->outputCount() > 0 ? mainWnd->resultChild(sourceMdi, newTitle) :
		sourceMdi;

	if (!mdiChild)
	{
		mainWnd->statusBar()->showMessage("Cannot create result child!", 5000);
		emit finished();
		return;
	}
	filterGUIPreparations(filter, mdiChild, mainWnd, paramValues);
	iAFilterRunnerGUIThread* thread = new iAFilterRunnerGUIThread(filter, paramValues, mdiChild, sourceMdi->modality(0)->fileName());
	if (!thread)
	{
		mainWnd->statusBar()->showMessage("Cannot create result calculation thread!", 5000);
		emit finished();
		return;
	}
	// TODO: move all image adding here?
	for (int m = 1; m < sourceMdi->modalities()->size(); ++m)
	{
		thread->addInput(sourceMdi->modality(m)->image(), sourceMdi->modality(m)->fileName());
	}
	filter->setFirstInputChannels(sourceMdi->modalities()->size());
	for (int a=0; a < m_additionalInput.size(); ++a)
	{
		thread->addInput(m_additionalInput[a], m_additionalFileNames[a]);
	}
	if (thread->Connectors().size() < filter->requiredInputs())
	{
		LOG(lvlError, QString("Not enough inputs specified, filter %1 requires %2 input images!")
			.arg(filter->name()).arg(filter->requiredInputs()));
		emit finished();
		return;
	}
	if (mdiChild->preferences().PrintParameters && !filter->parameters().isEmpty())
	{
		LOG(lvlInfo, QString("Starting %1 filter with parameters:").arg(thread->filter()->name()));
		for (int p = 0; p < thread->filter()->parameters().size(); ++p)
		{
			auto paramDescriptor = thread->filter()->parameters()[p];
			QString paramName = paramDescriptor->name();
			QString paramValue = paramDescriptor->valueType() == iAValueType::Boolean ?
				(paramValues[paramName].toBool() ? "yes" : "no")
				: paramValues[paramName].toString();
			LOG(lvlInfo, QString("    %1 = %2").arg(paramName).arg(paramValue));
		}
	}
	else
	{
		LOG(lvlInfo, QString("Starting %1 filter.").arg(thread->filter()->name()));
	}
	connectThreadSignals(mdiChild, thread);
	mdiChild->addStatusMsg(filter->name());
	iAJobListView::get()->addJob(filter->name(), thread->ProgressObserver(), thread, filter->canAbort() ? thread : nullptr);
	mainWnd->statusBar()->showMessage(filter->name(), 5000);
	thread->start();
}

void iAFilterRunnerGUI::connectThreadSignals(iAMdiChild* mdiChild, iAFilterRunnerGUIThread* thread)
{
	connect(thread, &QThread::finished, this, &iAFilterRunnerGUI::filterFinished);
	mdiChild->connectThreadSignalsToChildSlots(thread);
}

void iAFilterRunnerGUI::filterFinished()
{
	auto thread = qobject_cast<iAFilterRunnerGUIThread*>(sender());
	// add additional output as additional modalities here
	// "default" output 0 is handled elsewhere (via obscure iAMdiChild::rendererDeactivated / iAAlgorithm::updateVtkImageData)
	auto mdiChild = qobject_cast<iAMdiChild*>(thread->parent());
	// set default file name suggestion
	QFileInfo sourceFI(m_sourceFileName);
	QString suggestedFileName = sourceFI.absolutePath() + "/" + sourceFI.completeBaseName() + "-" +
		thread->filter()->name().replace(QRegularExpression("[\\\\/:*?\"<>| ]"), "_") + "." +
		sourceFI.suffix();
	mdiChild->modality(0)->setFileName(suggestedFileName);
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
			auto mod = QSharedPointer<iAModality>::create(thread->filter()->outputName(p, QString("Extra Out %1").arg(p)), "", -1, img, 0);
			mdiChild->modalities()->add(mod);
			// signal to add it to list automatically is created to late to be effective here, we have to add it to list ourselves:
			mdiChild->dataDockWidget()->modalityAdded(mod);
		}
	}
	for (auto outputValue : thread->filter()->outputValues())
	{
		LOG(lvlImportant, QString("%1: %2").arg(outputValue.first).arg(outputValue.second.toString()));
	}

	emit finished();
}
