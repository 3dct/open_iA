/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iANModalPreprocessor.h"

#include "iANModalBackgroundRemover.h"
#include "iANModalDilationBackgroundRemover.h"
#include "iANModalDisplay.h"
#include "iANModalDataSetReducer.h"
#include "iANModalPCADataSetReducer.h"

#include <iADataSet.h>
#include <iAMdiChild.h>
#include <iAToolsVTK.h>

#include <vtkImageData.h>
#include <vtkImageMask.h>

#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QTextEdit>

namespace
{
	iANModalBackgroundRemover::Mask MASK_NONE = {nullptr, iANModalBackgroundRemover::MaskMode::NONE};
	class PassthroughReducer : public iANModalDataSetReducer
	{
		QList<std::shared_ptr<iAImageData>> reduce(const QList<std::shared_ptr<iAImageData>>& inputModalities)
		{
			return inputModalities;
		}
	};
	class PassthroughBackgroundRemover : public iANModalBackgroundRemover
	{
		iANModalBackgroundRemover::Mask removeBackground(const QList<std::shared_ptr<iAImageData>>&)
		{
			return MASK_NONE;
		}
	};
}

iANModalPreprocessor::iANModalPreprocessor(iAMdiChild* mdiChild) : m_mdiChild(mdiChild)
{
}

inline QList<std::shared_ptr<iAImageData>> applyMask(
	const vtkSmartPointer<vtkImageData>& mask, const QList<std::shared_ptr<iAImageData>>& dataSets);

iANModalPreprocessor::Output iANModalPreprocessor::preprocess(const QList<std::shared_ptr<iAImageData>>& dataSets_in)
{
	Output output;

	QList<DataSetGroup> groups;
	groupDataSets(dataSets_in, groups);
	auto dataSets = chooseGroup(groups);

	Pipeline pipeline = choosePipeline();

	if (pipeline == MR_BGR || pipeline == MR)
	{
		// Dimensionality reduction
		dataSets = chooseDataSetReducer()->reduce(dataSets);
		if (dataSets.empty())
		{
			// TODO
			return Output(false);
		}
	}

	// Extract mask for background removal
	iANModalBackgroundRemover::Mask mask;
	if (pipeline == BGR || pipeline == BGR_MR || pipeline == MR_BGR)
	{
		mask = chooseBackgroundRemover()->removeBackground(dataSets);
		if (mask.maskMode == iANModalBackgroundRemover::MaskMode::INVALID)
		{
			// TODO return Output(false)?
			output.maskMode = IGNORE_MASK;
		}
		else if (mask.maskMode == iANModalBackgroundRemover::MaskMode::REMOVE)
		{
			output.maskMode = IGNORE_MASK;

			// Perform background removal
			if (pipeline != BGR_MR)
			{
				dataSets = applyMask(mask.mask, dataSets);
			}
		}
		else if (mask.maskMode == iANModalBackgroundRemover::MaskMode::HIDE)
		{
			output.maskMode = HIDE_ON_RENDER;
		}
		else
		{
			output.maskMode = IGNORE_MASK;
		}
	}
	else
	{
		mask = MASK_NONE;
		output.maskMode = IGNORE_MASK;
	}

	if (pipeline == BGR_MR)
	{
		// Dimensionality reduction
		dataSets = chooseDataSetReducer()->reduce(dataSets);
		if (dataSets.empty())
		{
			// TODO
			return Output(false);
		}

		// Perform background removal
		if (mask.maskMode == iANModalBackgroundRemover::MaskMode::REMOVE)
		{
			dataSets = applyMask(mask.mask, dataSets);
		}
	}

	output.dataSets = dataSets;
	output.mask = mask.mask;

	auto newDataSets = extractNewDataSets(dataSets);
	addDataSetsToMdiChild(newDataSets);

	return output;
}

iANModalPreprocessor::Pipeline iANModalPreprocessor::choosePipeline()
{
	const QString NONE = "Passthrough";
	const QString BGR = "Background Removal (BGR)";
	const QString MR = "Modality Reducion (MR)";
	const QString BGR_MR = "Get BGR mask, perform MR, apply BGR mask";
	const QString MR_BGR = "MR then BGR";

	QMap<QString, Pipeline> map;
	map.insert(NONE, Pipeline::NONE);
	map.insert(BGR, Pipeline::BGR);
	map.insert(MR, Pipeline::MR);
	map.insert(BGR_MR, Pipeline::BGR_MR);
	map.insert(MR_BGR, Pipeline::MR_BGR);

	auto sel = new iANModalPreprocessorSelector("n-Modal Transfer Function preprocessing: choose steps");
	sel->addOption(NONE, {NONE, "Do not perform any preprocessing: use modalities as-is"});
	sel->addOption(BGR, {BGR, "Perform background removal.\n\nYou will be able to choose a method later"});
	sel->addOption(MR, {MR, "Perform modality reduction.\n\nYou will be able to choose a method later"});
	sel->addOption(BGR_MR,
		{BGR_MR,
			"The following steps will be performed:\n"
			"1) Calculate a binary mask for background removal from the input modalities;\n"
			"2) Perform modality reduction;\n"
			"3) Apply the binary mask from step (1) onto the reduced modalities.\n\n"
			"You will be able to choose the background removal and modality reduction methods later"});
	sel->addOption(MR_BGR,
		{MR_BGR,
			"Perform modality reduction on the input modalities, then perform background removal on the reduced "
			"modalities\n\n"
			"You will be able to choose the background removal and modality reduction methods later"});
	QString selection = sel->exec();
	sel->deleteLater();

	return map.value(selection);
}

QSharedPointer<iANModalDataSetReducer> iANModalPreprocessor::chooseDataSetReducer()
{
	const QString PCA = "PCA";
	const QString NONE = "Skip";

	auto sel = new iANModalPreprocessorSelector("n-Modal Transfer Function preprocessing: choose Modality Reducer");
	sel->addOption(PCA,
		{PCA,
			"Perform Principal Component Analysis on the input modalities and use the principal components with "
			"largest variance"});
	sel->addOption(NONE, {NONE, "Skip modality reduction and use any four modalities"});
	QString selection = sel->exec();
	sel->deleteLater();

	if (selection == NONE)
	{
		return QSharedPointer<PassthroughReducer>(new PassthroughReducer());
	}
	else if (selection == PCA)
	{
		return QSharedPointer<iANModalDataSetReducer>(new iANModalPCADataSetReducer());
	}
	else
	{
		return nullptr;
	}
}

QSharedPointer<iANModalBackgroundRemover> iANModalPreprocessor::chooseBackgroundRemover()
{
	const QString CLOSING = "Morphological Closing";
	const QString NONE = "Skip";

	auto sel = new iANModalPreprocessorSelector("n-Modal Transfer Function preprocessing: choose Background Remover");
	sel->addOption(CLOSING,
		{
			CLOSING,
			"The following operations will be performed:\n"
			"1) Make an initial rough background selection (with simple thresholding), resulting in a binary mask;\n"
			"2) Then perform N morphological dilations (until the mask has only one region);\n"
			"3) Then perform N morphological erosions.\n\n"
			"This method only works if the input specimen has only one foreground region"  // However, it can later be adapted to support more regions! TODO
		});
	sel->addOption(NONE, {NONE, "Skip background removal (do not remove background)"});
	QString selection = sel->exec();
	sel->deleteLater();

	if (selection == NONE)
	{
		return QSharedPointer<PassthroughBackgroundRemover>(new PassthroughBackgroundRemover());
	}
	else if (selection == CLOSING)
	{
		return QSharedPointer<iANModalBackgroundRemover>(new iANModalDilationBackgroundRemover(m_mdiChild));
	}
	else
	{
		return nullptr;
	}
}

/*
bool iANModalPreprocessor::areModalitiesCompatible(std::shared_ptr<iAImageData> m1, std::shared_ptr<iAImageData> m2) {
	return true; // TODO
}
*/

void iANModalPreprocessor::groupDataSets(
	const QList<std::shared_ptr<iAImageData>>& dataSetsToGroup, QList<DataSetGroup>& output)
{
	// TODO
	// Currently returning same list as in input
	auto dims = dataSetsToGroup[0]->vtkImage()->GetDimensions();
	DataSetGroup group;
	group.dimx = dims[0];
	group.dimy = dims[1];
	group.dimz = dims[2];
	group.dataSets = dataSetsToGroup;
	output.append(group);
}

QList<std::shared_ptr<iAImageData>> iANModalPreprocessor::chooseGroup(const QList<DataSetGroup>& groups)
{
	// TODO
	return groups[0].dataSets;
}

QList<std::shared_ptr<iAImageData>> iANModalPreprocessor::extractNewDataSets(
	const QList<std::shared_ptr<iAImageData>>& dataSets)
{
	auto list = m_mdiChild->dataSets();
	QList<std::shared_ptr<iAImageData>> currentDataSets;
	for (int i = 0; i < list.size(); ++i)
	{
		auto imgDS = std::dynamic_pointer_cast<iAImageData>(list[i]);
		if (!imgDS)
		{
			continue;
		}
		currentDataSets.append(imgDS);
	}
	QList<std::shared_ptr<iAImageData>> newModalities;
	for (auto potentiallyNewModality: dataSets)
	{
		if (!currentDataSets.contains(potentiallyNewModality))
		{
			newModalities.append(potentiallyNewModality);
		}
	}
	return newModalities;
}

void iANModalPreprocessor::addDataSetsToMdiChild(const QList<std::shared_ptr<iAImageData>>& dataSets)
{
	for (auto ds : dataSets)
	{
		//m_mdiChild->dataDockWidget()->addModality(mod->image(), mod->name());
		m_mdiChild->addDataSet(ds);
	}
}

inline QList<std::shared_ptr<iAImageData>> applyMask(
	const vtkSmartPointer<vtkImageData>& mask, const QList<std::shared_ptr<iAImageData>>& dataSets)
{
	/* TODO (28th July 2020)
	As of now, the way the mask works does not support dynamic addition of dataSets.

	If setDataSets() is called with modality A
	Then applyMask() is called (=> modality A is changed to reserve 0)
	Then setDataSets() is called again with modality A

	That will result in modality A being changed again.

	That's ok for now. In the future, if dynamic addition of dataSets is to be supported, this must be fixed!
	*/

#ifndef NDEBUG
	//storeImage2(output, "pca_output_before_conversion_" + QString::number(out_i) + ".mhd", true);
	//storeImage(mask, "mask.mhd", true);
#endif

	QList<std::shared_ptr<iAImageData>> newDataSets;

	QString nameSuffix = "_noBackground";

	for (auto dsOld : dataSets)
	{
		auto maskFilter = vtkSmartPointer<vtkImageMask>::New();
		maskFilter->SetInput1Data(dsOld->vtkImage());
		maskFilter->SetInput2Data(mask);
		//maskFilter->SetMaskedOutputValue(0, 1, 0);
		//maskFilter->NotMaskOn();
		maskFilter->Update();

		QString name = dsOld->name() + nameSuffix;
		auto dataSet = std::make_shared<iAImageData>(maskFilter->GetOutput());
		dataSet->setMetaData(iADataSet::NameKey, name);
		newDataSets.append(dataSet);
	}

	return newDataSets;
}

// iANModalPreprocessorSelector ------------------------------------------------------------

iANModalPreprocessorSelector::iANModalPreprocessorSelector(QString dialogTitle)
{
	m_dialog = new QDialog();
	m_dialog->setWindowTitle(dialogTitle);
	QVBoxLayout* layout = new QVBoxLayout(m_dialog);

	m_comboBox = new QComboBox(m_dialog);
	connect(m_comboBox, &QComboBox::currentTextChanged, this, &iANModalPreprocessorSelector::updateText);

	m_label = new QLabel(m_dialog);

	m_textEdit = new QTextEdit(m_dialog);
	m_textEdit->setEnabled(false);

	QWidget* footer = iANModalDisplay::createOkCancelFooter(m_dialog);

	layout->addWidget(m_comboBox);
	layout->addWidget(m_label);
	layout->addWidget(m_textEdit);
	layout->addWidget(footer);
}

void iANModalPreprocessorSelector::addOption(QString displayName, iANModalPreprocessorSelector::Option option)
{
	m_comboBox->addItem(displayName);
	m_options.insert(displayName, option);
	if (m_comboBox->count() == 1)
	{
		m_comboBox->setCurrentIndex(0);
		m_label->setText(option.name);
		m_textEdit->setPlainText(option.description);
	}
}

QString iANModalPreprocessorSelector::exec()
{
	auto code = m_dialog->exec();
	if (code == QDialog::Rejected)
	{
		return QString();
	}
	return m_comboBox->currentText();
}

void iANModalPreprocessorSelector::updateText()
{
	QString selected = m_comboBox->currentText();
	iANModalPreprocessorSelector::Option option = m_options.value(selected);
	m_label->setText(option.name);
	m_textEdit->setText(option.description);
}
