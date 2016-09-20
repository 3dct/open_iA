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
#include "dlg_samplingSettings.h"

#include "iAAttributes.h"
#include "iAAttributeDescriptor.h"
#include "iAConsole.h"
#include "iAModality.h"
#include "iAParameterGeneratorImpl.h"

#include <QCheckBox>
#include <QDropEvent>
#include <QFileDialog>
#include <QMimeData>
#include <QShortcut>
#include <QTextStream>
#include <QStandardItemModel>

#include <cassert>



dlg_samplingSettings::dlg_samplingSettings(QWidget *parentWidget,
	QSharedPointer<iAModalityList const> modalities):
	dlg_samplingSettingsUI(parentWidget)
{
	assert(modalities->size() > 0);
	QSharedPointer<iAModality const> mod0 = modalities->Get(0);
	m_modalityCount = modalities->size();
	m_imagePixelCount = mod0->GetHeight() * mod0->GetWidth() * mod0->GetDepth();

	// assemble modality parameter input on the fly:

	QGridLayout* gridLay = dynamic_cast<QGridLayout*>(layout());
	m_startLine = gridLay->rowCount()-2;

	gridLay->addWidget(wdButtonBar, m_startLine+1, 0, 1, 4);
	
	cbSamplingMethod->clear();
	auto & paramGens = GetParameterGenerators();
	for (QSharedPointer<iAParameterGenerator> paramGen : paramGens)
	{
		cbSamplingMethod->addItem(paramGen->GetName());
	}
	cbSamplingMethod->setCurrentIndex(1);

	connect(leParamDescriptor, SIGNAL(editingFinished()), this, SLOT(ParameterDescriptorChanged()));
	connect(pbChooseOutputFolder, SIGNAL(clicked()), this, SLOT(ChooseOutputFolder()));
	connect(pbChooseParameterDescriptor, SIGNAL(clicked()), this, SLOT(ChooseParameterDescriptor()));
	connect(pbChooseExecutable, SIGNAL(clicked()), this, SLOT(ChooseExecutable()));
	connect (pbRun, SIGNAL(clicked()), this, SLOT(accept()));
	connect (pbCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

QSharedPointer<iAParameterGenerator> dlg_samplingSettings::GetGenerator()
{
	return GetParameterGenerators()[cbSamplingMethod->currentIndex()];
}


void dlg_samplingSettings::ChooseParameterDescriptor()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Parameter Descriptor"),
		QString(), // TODO get directory of current file
		tr("Parameter Descriptor Text File (*.txt);;All Files (*);;"));
	if (!fileName.isEmpty())
	{
		leParamDescriptor->setText(fileName);
	}
	LoadDescriptor(leParamDescriptor->text());
}


void dlg_samplingSettings::ChooseExecutable()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Executable"),
		QString(), // TODO get directory of current file
		tr("Windows Executable (*.exe);;Batch Script (*.bat);;Shell Script (*.sh);;Any Executable (*);;"));
	if (!fileName.isEmpty())
	{
		leExecutable->setText(fileName);
	}
}

void dlg_samplingSettings::ParameterDescriptorChanged()
{
	// load parameter descriptor from file
	LoadDescriptor(leParamDescriptor->text());
}

ParameterInputs CreateParameterLine(
	QString const & pName,
	QSharedPointer<iAAttributeDescriptor> descriptor,
	QGridLayout* gridLay,
	int curGridLine)
{
	ParameterInputs result;
	result.label = new QLabel(pName);
	result.from = new QLineEdit(QString::number(descriptor->GetMin(),
		descriptor->GetValueType() != Continuous? 'd' : 'g',
		descriptor->GetValueType() != Continuous ? 0 : 6));
	result.to = new QLineEdit(QString::number(descriptor->GetMax(),
		descriptor->GetValueType() != Continuous ? 'd' : 'g',
		descriptor->GetValueType() != Continuous ? 0 : 6));
	gridLay->addWidget(result.label, curGridLine, 0);
	gridLay->addWidget(result.from, curGridLine, 1);
	gridLay->addWidget(result.to, curGridLine, 2);
	if (descriptor->GetValueType() != Categorical)
	{
		result.logScale = new QCheckBox("Log Scale");
		result.logScale->setChecked(descriptor->IsLogScale());
		gridLay->addWidget(result.logScale, curGridLine, 3);
	}
	result.descriptor = descriptor;
	return result;
}

void dlg_samplingSettings::LoadDescriptor(QString const & fileName)
{
	if (fileName == m_descriptorFileName)
	{
		// same filename as before, we don't need to load again
		return;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Couldn't open parameter descriptor file '%1'\n").arg(fileName));
		return;
	}
	QTextStream in(&file);
	m_descriptor = iAAttributes::Create(in);

	// TODO: store values from previous descriptor?
	for (int i = 0; i < m_paramInputs.size(); ++i)
	{
		delete m_paramInputs[i].label;
		delete m_paramInputs[i].from;
		delete m_paramInputs[i].to;
		delete m_paramInputs[i].logScale;
	}
	if (m_descriptor->size() == 0)
	{
		DEBUG_LOG("Invalid descriptor file!");
		return;
	}
	m_paramInputs.clear();
	int curGridLine = m_startLine+1;
	QGridLayout* gridLay = dynamic_cast<QGridLayout*>(layout());
	for (int i = 0; i < m_descriptor->size(); ++i)
	{
		QString pName(m_descriptor->at(i)->GetName());
		if (pName.startsWith("Mod "))
		{
			for (int m = 0; m < m_modalityCount; ++m)
			{
				ParameterInputs pInput = CreateParameterLine(QString("Mod %1 ").arg(m) +
					pName.right(pName.length() - 4),
					m_descriptor->at(i),
					gridLay,
					curGridLine);
				curGridLine++;
				m_paramInputs.push_back(pInput);
			}
		}
		else
		{
			ParameterInputs pInput = CreateParameterLine(
				pName,
				m_descriptor->at(i),
				gridLay,
				curGridLine);
			curGridLine++;
			m_paramInputs.push_back(pInput);
		}
	}
	gridLay->addWidget(wdButtonBar, curGridLine, 0, 1, 4);
	m_descriptorFileName = fileName;
}


void SetMinMax(QSharedPointer<iAAttributeDescriptor> desc, QString valueText)
{
	bool ok;
	double value = valueText.toDouble(&ok);
	if (desc->GetValueType() == Categorical ||
		desc->GetValueType() == Discrete)
	{
		int value = valueText.toInt(&ok);
	}
	if (!ok)
	{
		DEBUG_LOG(QString("Value '%1' for parameter %2 is not valid!").arg(valueText).arg(desc->GetName()));
		return;
	}
	desc->AdjustMinMax(value);
}

QSharedPointer<iAAttributes> dlg_samplingSettings::GetAttributes()
{
	QSharedPointer<iAAttributes> result(new iAAttributes);
	for (int l = 0; l < m_paramInputs.size(); ++l)
	{
		QString pName(m_paramInputs[l].label->text());
		QSharedPointer<iAAttributeDescriptor> desc(new iAAttributeDescriptor(
			pName,
			iAAttributeDescriptor::Parameter,
			m_paramInputs[l].descriptor->GetValueType()));
		desc->SetNameMapper(m_paramInputs[l].descriptor->GetNameMapper());
		SetMinMax(desc, m_paramInputs[l].from->text());
		SetMinMax(desc, m_paramInputs[l].to->text());
		if (m_paramInputs[l].logScale)
		{
			assert(desc->GetValueType() != Categorical);
			desc->SetLogScale(m_paramInputs[l].logScale->isChecked());
		}
		result->Add(desc);
	}
	return result;
}


void dlg_samplingSettings::ChooseOutputFolder()
{
	QFileDialog dialog;
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly);
	QString outFolder = dialog.getExistingDirectory(this, "Choose Output Folder");
	if (outFolder != "")
	{
		leOutputFolder->setText(outFolder);
	}
}


QString dlg_samplingSettings::GetOutputFolder() const
{
	QString outDir = leOutputFolder->text();
	return outDir.replace("\\", "/");
}


QString dlg_samplingSettings::GetAdditionalArguments() const
{
	return leAdditionalArguments->text();
}

QString dlg_samplingSettings::GetExecutable() const
{
	return leExecutable->text();
}

int dlg_samplingSettings::GetSampleCount() const
{
	return sbNumberOfSamples->value();
}

int dlg_samplingSettings::GetLabelCount() const
{
	return sbLabelCount->value();
}

