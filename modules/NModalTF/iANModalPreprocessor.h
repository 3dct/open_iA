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
#pragma once

#include <vtkSmartPointer.h>

#include <QList>
#include <QMap>
#include <QSharedPointer>

class iANModalModalityReducer;
class iANModalBackgroundRemover;

class iAMdiChild;
class iAImageData;

class vtkImageData;

class QComboBox;
class QDialog;
class QLabel;
class QTextEdit;

class iANModalPreprocessor
{
public:
	iANModalPreprocessor(iAMdiChild* mdiChild);

	enum MaskMode
	{
		IGNORE_MASK,
		HIDE_ON_RENDER
	};

	struct Output
	{
		Output()
		{
		}
		Output(bool valid) : valid(valid)
		{
		}
		QList<std::shared_ptr<iAImageData>> dataSets;
		//bool hasMask = false;
		vtkSmartPointer<vtkImageData> mask;
		MaskMode maskMode;
		bool valid = true;
	};

	Output preprocess(const QList<iAImageData*>&);

private:
	iAMdiChild* m_mdiChild;

	struct DataSetGroup
	{
		QList<iAImageData*> dataSets;
		int dimx, dimy, dimz;
	};

	enum Pipeline
	{
		NONE,
		BGR,  // Background removal
		MR,   // Modality reduction
		BGR_MR,
		MR_BGR
	};

	Pipeline choosePipeline();
	QSharedPointer<iANModalModalityReducer> chooseModalityReducer();
	QSharedPointer<iANModalBackgroundRemover> chooseBackgroundRemover();

	void groupDataSets(const QList<iAImageData*>&, QList<DataSetGroup>& output);
	QList<iAImageData*> chooseGroup(const QList<DataSetGroup>&);

	QList<std::shared_ptr<iAImageData>> extractNewModalities(const QList<std::shared_ptr<iAImageData>>&);
	void addModalitiesToMdiChild(const QList<std::shared_ptr<iAImageData>>&);
};

class iANModalPreprocessorSelector : public QObject
{
	Q_OBJECT

public:
	struct Option
	{
		QString name;
		QString description;
	};

	iANModalPreprocessorSelector(QString dialogTitle);
	void addOption(QString displayName, Option option);
	QString exec();

private:
	QDialog* m_dialog;
	QComboBox* m_comboBox;
	QLabel* m_label;
	QTextEdit* m_textEdit;
	QMap<QString, Option> m_options;

private slots:
	void updateText();
};
