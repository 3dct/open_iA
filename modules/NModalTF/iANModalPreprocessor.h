// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QObject>
#include <QList>
#include <QMap>
#include <QSharedPointer>

class iANModalDataSetReducer;
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

	Output preprocess(const QList<std::shared_ptr<iAImageData>>&);

private:
	iAMdiChild* m_mdiChild;

	struct DataSetGroup
	{
		QList<std::shared_ptr<iAImageData>> dataSets;
		int dimx, dimy, dimz;
	};

	enum Pipeline
	{
		NONE,
		BGR,  // Background removal
		DR,   // Dataset reduction
		BGR_DR,
		DR_BGR
	};

	Pipeline choosePipeline();
	QSharedPointer<iANModalDataSetReducer> chooseDataSetReducer();
	QSharedPointer<iANModalBackgroundRemover> chooseBackgroundRemover();

	void groupDataSets(const QList<std::shared_ptr<iAImageData>>&, QList<DataSetGroup>& output);
	QList<std::shared_ptr<iAImageData>> chooseGroup(const QList<DataSetGroup>&);

	QList<std::shared_ptr<iAImageData>> extractNewDataSets(const QList<std::shared_ptr<iAImageData>>&);
	void addDataSetsToMdiChild(const QList<std::shared_ptr<iAImageData>>&);
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
