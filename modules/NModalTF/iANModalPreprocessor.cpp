/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iANModalPreprocessor.h"
#include "iANModalModalityReducer.h"
#include "iANModalBackgroundRemover.h"

// Background removers
#include "iANModalDilationBackgroundRemover.h"

// Modality reducers
#include "iANModalPCAModalityReducer.h"

#include "iANModalDisplay.h"
#include "iAModality.h"

#include <vtkImageData.h>

#include <QComboBox>
#include <QTextEdit>

namespace {
	class PassthroughReducer : public iANModalModalityReducer {
		QList<QSharedPointer<iAModality>> reduce(QList<QSharedPointer<iAModality>> inputModalities) {
			return inputModalities;
		}
	};
}

iANModalPreprocessor::iANModalPreprocessor(MdiChild *mdiChild) :
	m_mdiChild(mdiChild)
{

}

iANModalPreprocessor::Output iANModalPreprocessor::preprocess(QList<QSharedPointer<iAModality>> modalities) {

	Output output;

	QList<ModalitiesGroup> groups;
	groupModalities(modalities, groups);
	modalities = chooseGroup(groups);

	// Step 1: Dimensionality reduction 1
	modalities = chooseModalityReducer()->reduce(modalities);
	if (modalities.empty()) {
		// TODO
		return Output(false);
	}

	// Step 2: Remove background
	auto mask = chooseBackgroundRemover()->removeBackground(modalities);
	if (mask == nullptr) {
		// TODO
		return Output(false);
	}

	// TODO set modality voxel to zero everywhere where mask is 1

	// Step 3: Dimensionality reduction 2
	// TODO

	output.modalities = modalities;
	output.mask = mask;

	return output;
}

QSharedPointer<iANModalModalityReducer> iANModalPreprocessor::chooseModalityReducer() {
	
	const QString PCA = "PCA";
	const QString NONE = "None";

	auto sel = new iANModalPreprocessorSelector();
	sel->addOption(PCA, {PCA, "todo"});
	sel->addOption(NONE, {NONE, "Skip modality reduction and use any four modalities" });
	QString selection = sel->exec();
	sel->deleteLater();

	if (selection == NONE) {
		return QSharedPointer<PassthroughReducer>(new PassthroughReducer());
	} else if (selection == PCA) {
		return QSharedPointer<iANModalModalityReducer>(new iANModalPCAModalityReducer());
	} else {
		return nullptr;
	}
}

QSharedPointer<iANModalBackgroundRemover> iANModalPreprocessor::chooseBackgroundRemover() {
	// TODO
	return QSharedPointer<iANModalBackgroundRemover>(new iANModalDilationBackgroundRemover(m_mdiChild));
}

bool iANModalPreprocessor::areModalitiesCompatible(QSharedPointer<iAModality> m1, QSharedPointer<iAModality> m2) {
	return true; // TODO
}

void iANModalPreprocessor::groupModalities(QList<QSharedPointer<iAModality>> modalitiesToGroup, QList<ModalitiesGroup> &output) {
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

QList<QSharedPointer<iAModality>> iANModalPreprocessor::chooseGroup(QList<ModalitiesGroup> groups) {
	// TODO
	return groups[0].modalities;
}


// iANModalPreprocessorSelector ------------------------------------------------------------

iANModalPreprocessorSelector::iANModalPreprocessorSelector() {
	m_dialog = new QDialog();
	QVBoxLayout *layout = new QVBoxLayout(m_dialog);

	m_comboBox = new QComboBox(m_dialog);
	//QObject::connect(m_comboBox, &QComboBox::currentTextChanged, this, &PreprocessorSelector::updateText);
	connect(m_comboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(updateText()));

	m_label = new QLabel(m_dialog);

	m_textEdit = new QTextEdit(m_dialog);
	m_textEdit->setEnabled(false);

	QWidget *footer = iANModalDisplay::createOkCancelFooter(m_dialog);

	layout->addWidget(m_comboBox);
	layout->addWidget(m_label);
	layout->addWidget(m_textEdit);
	layout->addWidget(footer);
}

void iANModalPreprocessorSelector::addOption(QString displayName, iANModalPreprocessorSelector::Option option) {
	m_comboBox->addItem(displayName);
	m_options.insert(displayName, option);
	if (m_comboBox->count() == 1) {
		m_comboBox->setCurrentIndex(0);
		m_label->setText(option.name);
		m_textEdit->setText(option.description);
	}
}

QString iANModalPreprocessorSelector::exec() {
	auto code = m_dialog->exec();
	if (code == QDialog::Rejected) {
		return QString();
	}
	return m_comboBox->currentText();
}

void iANModalPreprocessorSelector::updateText() {
	QString selected = m_comboBox->currentText();
	iANModalPreprocessorSelector::Option option = m_options.value(selected);
	m_label->setText(option.name);
	m_textEdit->setText(option.description);
}
