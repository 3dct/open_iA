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
#include "iANModalModalityReducer.h"
#include "iANModalPCAModalityReducer.h"

#include <dlg_modalities.h>
#include <iAModality.h>
#include <iAModalityList.h>
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
	class PassthroughReducer : public iANModalModalityReducer
	{
		QList<QSharedPointer<iAModality>> reduce(const QList<QSharedPointer<iAModality>>& inputModalities)
		{
			return inputModalities;
		}
	};
	class PassthroughBackgroundRemover : public iANModalBackgroundRemover
	{
		iANModalBackgroundRemover::Mask removeBackground(const QList<QSharedPointer<iAModality>>&)
		{
			return MASK_NONE;
		}
	};
}

iANModalPreprocessor::iANModalPreprocessor(iAMdiChild* mdiChild) : m_mdiChild(mdiChild)
{
}

inline QList<QSharedPointer<iAModality>> applyMask(
	const vtkSmartPointer<vtkImageData>& mask, const QList<QSharedPointer<iAModality>>& modalities);

iANModalPreprocessor::Output iANModalPreprocessor::preprocess(const QList<QSharedPointer<iAModality>>& modalities_in)
{
	Output output;

	QList<ModalitiesGroup> groups;
	groupModalities(modalities_in, groups);
	auto modalities = chooseGroup(groups);

	Pipeline pipeline = choosePipeline();

	if (pipeline == MR_BGR || pipeline == MR)
	{
		// Dimensionality reduction
		modalities = chooseModalityReducer()->reduce(modalities);
		if (modalities.empty())
		{
			// TODO
			return Output(false);
		}
	}

	// Extract mask for background removal
	iANModalBackgroundRemover::Mask mask;
	if (pipeline == BGR || pipeline == BGR_MR || pipeline == MR_BGR)
	{
		mask = chooseBackgroundRemover()->removeBackground(modalities);
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
				modalities = applyMask(mask.mask, modalities);
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
		modalities = chooseModalityReducer()->reduce(modalities);
		if (modalities.empty())
		{
			// TODO
			return Output(false);
		}

		// Perform background removal
		if (mask.maskMode == iANModalBackgroundRemover::MaskMode::REMOVE)
		{
			modalities = applyMask(mask.mask, modalities);
		}
	}

	output.modalities = modalities;
	output.mask = mask.mask;

	auto newModalities = extractNewModalities(modalities);
	addModalitiesToMdiChild(newModalities);

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

QSharedPointer<iANModalModalityReducer> iANModalPreprocessor::chooseModalityReducer()
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
		return QSharedPointer<iANModalModalityReducer>(new iANModalPCAModalityReducer());
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
bool iANModalPreprocessor::areModalitiesCompatible(QSharedPointer<iAModality> m1, QSharedPointer<iAModality> m2) {
	return true; // TODO
}
*/

void iANModalPreprocessor::groupModalities(
	const QList<QSharedPointer<iAModality>>& modalitiesToGroup, QList<ModalitiesGroup>& output)
{
	// TODO
	// Currently returning same list as in input
	auto dims = modalitiesToGroup[0]->image()->GetDimensions();
	auto group = ModalitiesGroup();
	group.dimx = dims[0];
	group.dimy = dims[1];
	group.dimz = dims[2];
	group.modalities = modalitiesToGroup;
	output.append(group);
}

QList<QSharedPointer<iAModality>> iANModalPreprocessor::chooseGroup(const QList<ModalitiesGroup>& groups)
{
	// TODO
	return groups[0].modalities;
}

QList<QSharedPointer<iAModality>> iANModalPreprocessor::extractNewModalities(
	const QList<QSharedPointer<iAModality>>& modalities)
{
	auto list = m_mdiChild->dataDockWidget()->modalities();
	auto currentModalities = QList<QSharedPointer<iAModality>>();
	for (int i = 0; i < list->size(); ++i)
	{
		auto modality = list->get(i);
		currentModalities.append(modality);
	}

	auto newModalities = QList<QSharedPointer<iAModality>>();
	for (auto potentiallyNewModality : modalities)
	{
		if (!currentModalities.contains(potentiallyNewModality))
		{
			newModalities.append(potentiallyNewModality);
		}
	}

	return newModalities;
}

void iANModalPreprocessor::addModalitiesToMdiChild(const QList<QSharedPointer<iAModality>>& modalities)
{
	for (auto mod : modalities)
	{
		//m_mdiChild->dataDockWidget()->addModality(mod->image(), mod->name());
		m_mdiChild->dataDockWidget()->addModality(mod);
	}
}

inline QList<QSharedPointer<iAModality>> applyMask(
	const vtkSmartPointer<vtkImageData>& mask, const QList<QSharedPointer<iAModality>>& modalities)
{
	/* TODO (28th July 2020)
	As of now, the way the mask works does not support dynamic addition of modalities.

	If setModalities() is called with modality A
	Then applyMask() is called (=> modality A is changed to reserve 0)
	Then setModalities() is called again with modality A

	That will result in modality A being changed again.

	That's ok for now. In the future, if dynamic addition of modalities is to be supported, this must be fixed!
	*/

#ifndef NDEBUG
	//storeImage2(output, "pca_output_before_conversion_" + QString::number(out_i) + ".mhd", true);
	//storeImage(mask, "mask.mhd", true);
#endif

	auto newModalities = QList<QSharedPointer<iAModality>>();

	QString nameSuffix = "_noBackground";

	for (auto modOld : modalities)
	{
		auto maskFilter = vtkSmartPointer<vtkImageMask>::New();
		maskFilter->SetInput1Data(modOld->image());
		maskFilter->SetInput2Data(mask);
		//maskFilter->SetMaskedOutputValue(0, 1, 0);
		//maskFilter->NotMaskOn();
		maskFilter->Update();

		QString name = modOld->name() + nameSuffix;
		auto modNew = new iAModality(name, "", -1, maskFilter->GetOutput(), iAModality::NoRenderer);
		newModalities.append(QSharedPointer<iAModality>(modNew));
	}

	return newModalities;
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
