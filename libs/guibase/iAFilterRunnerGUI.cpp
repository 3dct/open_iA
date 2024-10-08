// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFilterRunnerGUI.h"

// base
#include "iAAttributeDescriptor.h"
#include "iAImageData.h"
#include "iAFileUtils.h"
#include "iAFilter.h"
#include "iALog.h"
#include "iAProgress.h"
#include "iASettings.h"    // for storeSettings

// guibase
#include "iAConnector.h"
#include "iAJobListView.h"
#include "iAMainWindow.h"
#include "iAMdiChild.h"
#include "iAParameterDlg.h"
#include "iAPerformanceHelper.h"    // for formatDuration
#include "iAPreferences.h"
#include "iAValueTypeVectorHelpers.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QStatusBar>
#include <QString>
#include <QVariant>

class iAFilter;

iAFilterRunnerGUIThread::iAFilterRunnerGUIThread(std::shared_ptr<iAFilter> filter, QVariantMap paramValues, iAMdiChild* sourceMDI) :
	m_filter(filter),
	m_paramValues(paramValues),
	m_sourceMDI(sourceMDI),
	m_aborted(false)
{
	connect(m_sourceMDI, &iAMdiChild::closed, this, [this] { m_sourceMDI = nullptr;  });   // to avoid accessing a closed window
}

void iAFilterRunnerGUIThread::run()
{
	QElapsedTimer time;
	time.start();
	try
	{
		if (!m_filter->run(m_paramValues))
		{
			LOG(lvlError, "Running filter failed!");
			return;
		}
		if (m_aborted)
		{
			return;
		}
	}
	catch (std::exception const & e)
	{
		LOG(lvlError, tr("%1 terminated unexpectedly. Error: %2. Elapsed time: %3")
				   .arg(m_filter->name())
				   .arg(e.what())
				.arg(formatDuration(time.elapsed() / 1000.0, true, false)));
		return;
	}
	LOG(lvlInfo,
		tr("%1 finished. Elapsed time: %2")
			.arg(m_filter->name())
			.arg(formatDuration(time.elapsed() / 1000.0, true, false)));
}

void iAFilterRunnerGUIThread::abort()
{
	m_aborted = true;
	m_filter->abort();
}

std::shared_ptr<iAFilter> iAFilterRunnerGUIThread::filter()
{
	return m_filter;
}

void iAFilterRunnerGUIThread::addInput(std::shared_ptr<iADataSet> dataSet)
{
	m_filter->addInput(dataSet);
}

size_t iAFilterRunnerGUIThread::inputCount() const
{
	return m_filter->inputCount();
}

iAMdiChild* iAFilterRunnerGUIThread::sourceMDI()
{
	return m_sourceMDI;
}


namespace
{
	QString filterSettingGroup(std::shared_ptr<iAFilter> filter)
	{
		QString filterNameShort(filter->name());
		filterNameShort.replace(" ", "");
		return QString("Filters/%1/%2").arg(filter->category()).arg(filterNameShort);
	}
}


// iAFilterRunnerGUI


std::shared_ptr<iAFilterRunnerGUI> iAFilterRunnerGUI::create()
{
	return std::make_shared<iAFilterRunnerGUI>();
}

QVariantMap iAFilterRunnerGUI::loadParameters(std::shared_ptr<iAFilter> filter, iAMdiChild* sourceMdi)
{
	auto params = filter->parameters();
	QVariantMap result;
	QSettings settings;
	settings.beginGroup(filterSettingGroup(filter));
	for (auto param : params)
	{
		QVariant defaultValue = (param->valueType() == iAValueType::Categorical) ? "" : param->defaultValue();
		QVariant value = (param->valueType() == iAValueType::FileNameSave && sourceMdi)
			? pathFileBaseName(sourceMdi->fileInfo()) + param->defaultValue().toString()
			: settings.value(param->name(), defaultValue);
		if ((param->valueType() == iAValueType::Vector2  && variantToVector<double>(value).size() != 2) ||
			(param->valueType() == iAValueType::Vector2i && variantToVector<int   >(value).size() != 2) ||
			(param->valueType() == iAValueType::Vector3  && variantToVector<double>(value).size() != 3) ||
			(param->valueType() == iAValueType::Vector3i && variantToVector<int   >(value).size() != 3))
		{
			value = defaultValue;
		}
		result.insert(param->name(), value);
	}
	return result;
}

void iAFilterRunnerGUI::storeParameters(std::shared_ptr<iAFilter> filter, QVariantMap & paramValues)
{
	auto params = filter->parameters();
	storeSettings(filterSettingGroup(filter), paramValues);
	// just some checking whether there are values for all parameters:
	for (auto param : params)
	{
		if (!paramValues.contains(param->name()))
		{
			LOG(lvlError, QString("No value for parameter '%1'").arg(param->name()));
		}
	}
}

bool iAFilterRunnerGUI::askForParameters(std::shared_ptr<iAFilter> filter, QVariantMap & paramValues,
	iAMdiChild* sourceMdi, iAMainWindow* mainWnd, bool askForAdditionalInput)
{

	auto dlgParams = combineAttributesWithValues(filter->parameters(), paramValues);
	bool showROI = false;	// TODO: find better way to check this?
	for (auto filterParam : filter->parameters())
	{
		if (filterParam->name() == "Index")
		{
			showROI = true;
		}
	}
	if ( filter->requiredImages() == 1 && dlgParams.empty())
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
	if (askForAdditionalInput && filter->requiredImages() > static_cast<unsigned int>(otherMdis.size()+1) )
	{
		QMessageBox::warning(mainWnd, filter->name(),
			QString("This filter requires %1 datasets, only %2 open file(s)!")
			.arg(filter->requiredImages()).arg(otherMdis.size()+1));
		return false;
	}
	QStringList mdiChildrenNames;
	if (askForAdditionalInput && filter->requiredImages() > 1)
	{
		for (auto mdi: otherMdis)
		{
			mdiChildrenNames << mdi->windowTitle().replace("[*]", "");
		}
		for (unsigned int i = 1; i < filter->requiredImages(); ++i)
		{
			addAttr(dlgParams, QString("%1").arg(filter->inputName(i)), iAValueType::Categorical, mdiChildrenNames);
		}
	}
	iAParameterDlg dlg(mainWnd, filter->name(), dlgParams, filter->description());
	dlg.setModal(false);
	dlg.hide();	dlg.show(); // required to apply change in modality!
	if (sourceMdi)
	{
		dlg.setSourceMdi(sourceMdi, mainWnd);
	}
	if (showROI)
	{
		dlg.showROI();
	}
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	paramValues = dlg.parameterValues();
	if (askForAdditionalInput && filter->requiredImages() > 1)
	{
		for (unsigned int i = 1; i < filter->requiredImages(); ++i)
		{
			QString selectedFile = paramValues[QString("%1").arg(filter->inputName(i))].toString();
			auto mdiIdx = mdiChildrenNames.indexOf(selectedFile);
			for (auto d: otherMdis[mdiIdx]->dataSetMap())
			{
				// TODO: polydata input / ...
				auto imgData = dynamic_cast<iAImageData*>(d.second.get());
				if (imgData)
				{
					m_additionalInput.push_back(d.second);
				}
			}
		}
	}
	return true;
}

void iAFilterRunnerGUI::filterGUIPreparations(std::shared_ptr<iAFilter> /*filter*/,
	iAMdiChild* /*mdiChild*/, iAMainWindow* /*mainWnd*/, QVariantMap const & /*params*/)
{
}

void iAFilterRunnerGUI::run(std::shared_ptr<iAFilter> filter, iAMainWindow* mainWnd)
{
	iAMdiChild* sourceMdi = mainWnd->activeMdiChild();
	if (filter->requiredImages() > 0 && (!sourceMdi || sourceMdi->dataSetMap().empty()))
	{
		LOG(lvlWarn,QString("Filter requires %1 input(s), but %2!")
			.arg(filter->requiredImages())
			.arg(!sourceMdi ? "no source file is available" : "source file is not fully loaded yet"));
		emit finished();
		return;
	}
	QVariantMap paramValues = loadParameters(filter, sourceMdi);
	filter->adaptParametersToInput(paramValues, sourceMdi? sourceMdi->dataSetMap() : std::map<size_t, std::shared_ptr<iADataSet>>());

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

	QString oldTitle(sourceMdi ? sourceMdi->windowTitle() : "");
	oldTitle = oldTitle.replace("[*]", "").trimmed();
	QString newTitle(filter->outputName(0) + " " + oldTitle);
	filterGUIPreparations(filter, sourceMdi, mainWnd, paramValues);
	auto thread = new iAFilterRunnerGUIThread(filter, paramValues, sourceMdi);
	if (sourceMdi)
	{
		auto dataSets = sourceMdi->dataSetMap();
		for (auto d: dataSets)
		{	// check which (type of) datasets the filter expects, and only add these!
			thread->addInput(d.second);
		}
		filter->setFirstInputChannels(static_cast<unsigned int>(dataSets.size()));
	}
	for (size_t a=0; a < m_additionalInput.size(); ++a)
	{
		thread->addInput(m_additionalInput[a]);
	}
	if (thread->inputCount() < filter->requiredImages())
	{
		LOG(lvlError, QString("Not enough inputs specified, filter %1 requires %2 input images!")
			.arg(filter->name()).arg(filter->requiredImages()));
		emit finished();
		return;
	}
	if (mainWnd->defaultPreferences().PrintParameters && !filter->parameters().isEmpty())
	{
		LOG(lvlInfo, QString("Starting %1 filter with parameters:").arg(filter->name()));
		for (int p = 0; p < filter->parameters().size(); ++p)
		{
			auto paramDescriptor = filter->parameters()[p];
			QString paramName = paramDescriptor->name();
			QString paramValue = variantValueToString(paramDescriptor->valueType(), paramValues[paramName]);
			LOG(lvlInfo, QString("    %1 = %2").arg(paramName).arg(paramValue));
		}
	}
	else
	{
		LOG(lvlInfo, QString("Starting %1 filter.").arg(thread->filter()->name()));
	}
	connect(thread, &QThread::finished, this, &iAFilterRunnerGUI::filterFinished);
	iAJobListView::get()->addJob(filter->name(), filter->progress(), thread, filter->canAbort() ? thread : nullptr);
	thread->start();
}

void iAFilterRunnerGUI::filterFinished()
{
	auto thread = qobject_cast<iAFilterRunnerGUIThread*>(sender());
	auto filter = thread->filter();
	if (filter->finalOutputCount() > 0)
	{
		QString baseName = filter->inputCount() > 0 ? (filter->input(0)->metaData(iADataSet::NameKey).toString() + " ") : "";
		iAMdiChild* newChild = nullptr;
		if (!iAMainWindow::get()->defaultPreferences().ResultInNewWindow)
		{
			newChild = thread->sourceMDI();
		}
		if (!newChild)    // if we want a new result child, if there was no source MDI, or if child was closed in the meantime...
		{
			newChild = iAMainWindow::get()->createMdiChild(true);
			newChild->show();
		}
		for (size_t o = 0; o < filter->finalOutputCount(); ++o)
		{
			auto dataSet = filter->output(o);
			dataSet->setMetaData(iADataSet::NameKey, baseName + dataSet->name());
			newChild->addDataSet(dataSet);    // TODO: if it makes sense for this filter, we might want to pass in the viewer parameters (transfer function, etc.) from the old dataset (?)
		}
	}
#if QT_VERSION >= QT_VERSION_CHECK(6,4,0)
	for (auto [name, value] : filter->outputValues().asKeyValueRange())
	{
#else
	for (auto it = map.keyValueBegin(); it != map.keyValueEnd(); ++it)
	{
		auto name = it->first;
		auto value = it->second;
#endif
		LOG(lvlImportant, QString("%1: %2").arg(name).arg(value.toString()));
	}
	thread->deleteLater();
	emit finished();
}
