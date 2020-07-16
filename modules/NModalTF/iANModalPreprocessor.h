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
#pragma once

#include "mdichild.h"

#include <vtkSmartPointer.h>

#include <QList>
#include <QSharedPointer>

class iAModality;
class iANModalModalityReducer;
class iANModalBackgroundRemover;

class vtkImageData;

class QComboBox;
class QTextEdit;

class iANModalPreprocessor {

public:
	iANModalPreprocessor(MdiChild *mdiChild);

	struct Output {
		Output() {}
		Output(bool valid) : valid(valid) {}
		QList<QSharedPointer<iAModality>> modalities;
		//bool hasMask = false;
		vtkSmartPointer<vtkImageData> mask;
		bool valid = true;
	};
	
	Output preprocess(QList<QSharedPointer<iAModality>>);

private:

	MdiChild *m_mdiChild;

	struct ModalitiesGroup {
		QList<QSharedPointer<iAModality>> modalities;
		int dimx, dimy, dimz;
	};

	QSharedPointer<iANModalModalityReducer> chooseModalityReducer();
	QSharedPointer<iANModalBackgroundRemover> chooseBackgroundRemover();

	bool areModalitiesCompatible(QSharedPointer<iAModality>, QSharedPointer <iAModality>);
	void groupModalities(QList<QSharedPointer<iAModality>>, QList<ModalitiesGroup> &output);
	QList<QSharedPointer<iAModality>> chooseGroup(QList<ModalitiesGroup>);
};

class iANModalPreprocessorSelector : public QObject {
	Q_OBJECT

public:
	struct Option {
		QString name;
		QString description;
	};

	iANModalPreprocessorSelector();
	void addOption(QString displayName, Option option);
	QString exec();

private:
	QDialog *m_dialog;
	QComboBox *m_comboBox;
	QLabel *m_label;
	QTextEdit *m_textEdit;
	QMap<QString, Option> m_options;

private slots:
	void updateText();
};