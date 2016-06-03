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
#include "dlg_samplingSettings.h"

#include "iAMMSegParameterRange.h"
#include "iAMMSegParameterGeneratorImpl.h"
#include "iAModality.h"
#include "iASpectraDistanceImpl.h"
#include "QxtSpanSlider"

#include <QDropEvent>
#include <QFileDialog>
#include <QMimeData>
#include <QShortcut>
#include <QStandardItemModel>

namespace
{
	const int MaxFactor = 1000;
};

class MyListWidget: public QListWidget
{
protected:

	bool containsItem(QString title)
	{
		for (int i = 0; i < count(); ++i) {
			if (item(i)->text() == title)
			{
				return true;
			}
		}
		return false;
	}

	virtual void dropEvent ( QDropEvent * event )
	{
		QStringList formats = event->mimeData()->formats();
		if (!formats.contains("application/x-qabstractitemmodeldatalist"))
		{
			event->setDropAction(Qt::IgnoreAction);
			event->accept();
			return;
		}
		QStandardItemModel model;
		model.dropMimeData(event->mimeData(), Qt::CopyAction, 0,0, QModelIndex());
		for (int i=0; i<model.rowCount(); ++i)
		{
			if (containsItem(model.item(i)->text()))
			{
				event->setDropAction(Qt::IgnoreAction);
				event->accept();
				return;
			}
		}
		QListWidget::dropEvent(event);
	}
};

dlg_samplingSettings::dlg_samplingSettings(QWidget *parentWidget, QSharedPointer<iAModalityList const> modalities):
	dlg_samplingSettingsUI(parentWidget)
{
	assert(modalities->size() > 0);
	QSharedPointer<iAModality const> mod0 = modalities->Get(0);
	m_imagePixelCount = mod0->GetHeight() * mod0->GetWidth() * mod0->GetDepth();

	// assemble modality parameter input on the fly:

	QGridLayout* gridLay = dynamic_cast<QGridLayout*>(layout());
	int startLine = gridLay->rowCount()-2;

	size_t channelCount = 0;
	for (int i=0; i<modalities->size(); ++i)
	{		
		ModalityControls modCtrl;
		
		modCtrl.weightSlider = new QxtSpanSlider(Qt::Horizontal);
		modCtrl.weightSlider->setMinimum(0);
		modCtrl.weightSlider->setMaximum(MaxFactor);
		modCtrl.weightSlider->setLowerValue(0);
		modCtrl.weightSlider->setUpperValue(MaxFactor);
		connect(modCtrl.weightSlider, SIGNAL(lowerPositionChanged(int)), this, SLOT(ModalityMinWeightChanged(int)));
		connect(modCtrl.weightSlider, SIGNAL(upperPositionChanged(int)), this, SLOT(ModalityMaxWeightChanged(int)));
		
		modCtrl.distanceMeasures = new MyListWidget();
		modCtrl.distanceMeasures->setAcceptDrops(true);

		modCtrl.distanceDeleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), modCtrl.distanceMeasures);
		modCtrl.distanceDeleteShortcut->setContext(Qt::WidgetShortcut);
		connect(modCtrl.distanceDeleteShortcut, SIGNAL(activated()), this, SLOT(DeleteItem()));

		QLabel* lbModName = new QLabel(modalities->Get(i)->GetName());
		QLabel* lbWeight = new QLabel("Weight");
		QLabel* lbDistanceFunc = new QLabel("Distance Metric");

		gridLay->addWidget(lbModName, startLine, 0);

		gridLay->addWidget(lbWeight, startLine, 1);
		gridLay->addWidget(modCtrl.weightSlider, startLine, 2, 1, 2);

		gridLay->addWidget(lbDistanceFunc, startLine+1, 1);
		gridLay->addWidget(modCtrl.distanceMeasures, startLine+1, 2, 1, 2);

		channelCount += modalities->Get(i)->GetData()->channelCount();
		if (modalities->Get(i)->GetData()->channelCount() > 1)
		{
			QLabel* lbPCA = new QLabel("PCA");
			modCtrl.pcaMin = new QLineEdit("1");
			modCtrl.pcaMax = new QLineEdit(QString::number(modalities->Get(i)->GetData()->channelCount()));

			gridLay->addWidget(lbPCA, startLine+2, 1);
			gridLay->addWidget(modCtrl.pcaMin, startLine+2, 2);
			gridLay->addWidget(modCtrl.pcaMax, startLine+2, 3);
			startLine += 3;

			modCtrl.distanceMeasures->addItem("Squared");
		}
		else
		{
			// add default distance measure:
			modCtrl.distanceMeasures->addItem("Squared");

			modCtrl.pcaMin = 0;
			modCtrl.pcaMax = 0;
			startLine += 2;
		}
		gridLay->addWidget(lbAvailableMetrics, startLine, 0);
		gridLay->addWidget(lwAvailableMetrics, startLine, 1);
		gridLay->addWidget(lbAvailableMetricsDescription, startLine, 2, 1, 2);
		m_modalityControls.push_back(modCtrl);
	}
	gridLay->addWidget(wdButtonBar, startLine+1, 0, 1, 4);

	leSVM_Channels_To->setText(QString::number(channelCount));

	for (int i=0; i<GetDistanceMeasureCount(); ++i)
	{
		lwAvailableMetrics->addItem(GetDistanceMeasureNames()[i]);
	}
	
	cbSamplingMethod->clear();
	auto & paramGens = GetParameterGenerators();
	for (QSharedPointer<iAMMSegParameterGenerator> paramGen : paramGens)
	{
		cbSamplingMethod->addItem(paramGen->GetName());
	}
	cbSamplingMethod->setCurrentIndex(1);

	connect(sbNumberOfSamples, SIGNAL(valueChanged(int)), this, SLOT(UpdateEstimate(int)));
	connect(pbChooseOutputFolder, SIGNAL(clicked()), this, SLOT(ChooseOutputFolder()));

	connect (pbRun, SIGNAL(clicked()), this, SLOT(accept()));
	connect (pbCancel, SIGNAL(clicked()), this, SLOT(reject()));

	UpdateEstimate(10000);
}

void dlg_samplingSettings::DeleteItem()
{
	QShortcut* shortCut = dynamic_cast<QShortcut*>(sender());
	assert(shortCut);
	if (!shortCut)
	{
		return;
	}
	foreach(ModalityControls modCtrl, m_modalityControls)
	{
		if (shortCut == modCtrl.distanceDeleteShortcut)
		{
			delete modCtrl.distanceMeasures->currentItem();
		}
	}
}

void dlg_samplingSettings::ModalityMinWeightChanged(int min)
{
	QVector<QSignalBlocker*> blockers;
	int otherMaxSum = 0;
	
	QxtSpanSlider* span = dynamic_cast<QxtSpanSlider*>(sender());

	foreach(ModalityControls modCtrl, m_modalityControls)
	{
		blockers.push_back(new QSignalBlocker(modCtrl.weightSlider));
		if (span != modCtrl.weightSlider)
		{
			otherMaxSum += (MaxFactor - modCtrl.weightSlider->upperValue());
		}
	}

	foreach(ModalityControls modCtrl, m_modalityControls)
	{
		if (span != modCtrl.weightSlider)
		{
			// if need be, adapt max of other modalities in the same ratio as they currently have to each other:
			double ratio =
				(otherMaxSum == 0) ? 1 :
				static_cast<double>(MaxFactor - modCtrl.weightSlider->upperValue()) / otherMaxSum;

			// TODO: make sure min - other maxes add up to MaxFactor?
			int newMax = MaxFactor - static_cast<int>(min * ratio);
			modCtrl.weightSlider->setUpperValue( newMax );
		}
	}


	foreach(QSignalBlocker* block, blockers)
	{
		delete block;
	}
}

void dlg_samplingSettings::ModalityMaxWeightChanged(int max)
{
	QVector<QSignalBlocker*> blockers;
	int otherMinSum = 0;
	
	QxtSpanSlider* span = dynamic_cast<QxtSpanSlider*>(sender());

	foreach(ModalityControls modCtrl, m_modalityControls)
	{
		blockers.push_back(new QSignalBlocker(modCtrl.weightSlider));
		if (span != modCtrl.weightSlider)
		{
			otherMinSum += modCtrl.weightSlider->lowerValue();
		}
	}

	foreach(ModalityControls modCtrl, m_modalityControls)
	{
		if (span != modCtrl.weightSlider)
		{
			// if need be, adapt min of other modalities in the same ratio as they currently have to each other:
			double ratio =
				(otherMinSum == 0) ? 1 :
				static_cast<double>(modCtrl.weightSlider->lowerValue()) / otherMinSum;

			// TODO: make sure min - other maxes add up to MaxFactor?
			int newMin = static_cast<int>((MaxFactor-max) * ratio);
			modCtrl.weightSlider->setLowerValue( newMin );
		}
	}


	foreach(QSignalBlocker* block, blockers)
	{
		delete block;
	}
}

QSharedPointer<iAMMSegParameterGenerator> dlg_samplingSettings::GetGenerator()
{
	return GetParameterGenerators()[cbSamplingMethod->currentIndex()];
}

void AssembleDistanceFuncs(QListWidget* listWidget, QVector<QSharedPointer<iASpectraDistance> > & result)
{
	for (int i=0; i<listWidget->count(); ++i)
	{
		QListWidgetItem * item = listWidget->item(i);
		QSharedPointer<iASpectraDistance> dist = GetDistanceMeasure(item->text());
		result.push_back(dist);
	}
}


QSharedPointer<iAMMSegParameterRange> dlg_samplingSettings::GetRange()
{
	QSharedPointer<iAMMSegParameterRange> result(new iAMMSegParameterRange);
	result->erw_beta_From = leBetaMin->text().toDouble();
	result->erw_beta_To = leBetaMax->text().toDouble();
	result->erw_beta_logScale = cbBetaLogScale->isChecked();
	result->erw_gamma_From = leGammaMin->text().toDouble();
	result->erw_gamma_To = leGammaMax->text().toDouble();
	result->erw_gamma_logScale = cbGammaLogScale->isChecked();
	result->erw_maxIter_From = leMaxIterMin->text().toDouble();
	result->erw_maxIter_To = leMaxIterMax->text().toDouble();
	result->erw_maxIter_logScale = cbMaxIterLogScale->isChecked();
	result->svm_C_From = leSVM_C_From->text().toDouble();
	result->svm_C_To = leSVM_C_To->text().toDouble();
	result->svm_C_logScale = cbSVM_C_LogScale->isChecked();
	result->svm_gamma_From = leSVM_gamma_From->text().toDouble();
	result->svm_gamma_To = leSVM_gamma_To->text().toDouble();
	result->svm_gamma_logScale = cbSVM_Gamma_LogScale->isChecked();
	result->svm_channels_From = leSVM_Channels_From->text().toDouble();
	result->svm_channels_To = leSVM_Channels_To->text().toDouble();

	result->sampleCount = sbNumberOfSamples->value();
	result->weightLogScale = cbWeightLogScale->isChecked();
	result->objCountMin = std::numeric_limits<int>::max();
	result->objCountMax = std::numeric_limits<int>::lowest();
	result->durationMin = std::numeric_limits<int>::max();
	result->durationMax = std::numeric_limits<int>::lowest();
	
	for (int i=0; i<m_modalityControls.size(); ++i)
	{
		iAMMSegModalityParamRange modParamRng;
		modParamRng.pcaDimMin  = (m_modalityControls[i].pcaMin) ? m_modalityControls[i].pcaMin->text().toInt() : 1;
		modParamRng.pcaDimMax  = (m_modalityControls[i].pcaMax) ? m_modalityControls[i].pcaMax->text().toInt() : 1;
		modParamRng.weightFrom = static_cast<double>(m_modalityControls[i].weightSlider->lowerValue()) / MaxFactor;
		modParamRng.weightTo   = static_cast<double>(m_modalityControls[i].weightSlider->upperValue()) / MaxFactor;
		AssembleDistanceFuncs(m_modalityControls[i].distanceMeasures, modParamRng.distanceFuncs);
		result->modalityParamRange.push_back(modParamRng);
	}

	return result;
}

double dlg_samplingSettings::GetEstimatedDuration() const
{
	return (0.00007*m_imagePixelCount*m_nbOfSamples )/3600 ;
}

void dlg_samplingSettings::UpdateEstimate(int nbOfSamples)
{
	m_nbOfSamples = nbOfSamples;
	lbEstimate->setText(QString::number( GetEstimatedDuration(), 'g', 3 ));
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
	return leOutputFolder->text();
}


bool dlg_samplingSettings::DoStoreProbabilities() const
{
	return cbStoreProbabilities->isChecked();

}