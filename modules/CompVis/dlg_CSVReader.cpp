#include "dlg_CSVReader.h"

//Debug
#include "iALog.h"

#include <QDirIterator>
#include <QFileDialog>
#include <QStringListModel>
#include <QPushButton>


dlg_CSVReader::dlg_CSVReader() : QDialog(), m_computeMDSFlag(true)
{
	setupUi(this);
	connectSignals();
}

void dlg_CSVReader::connectSignals()
{
	connect(btnAddFiles, &QPushButton::clicked, this, &dlg_CSVReader::btnAddFilesClicked);
	connect(btnDeleteFile, &QPushButton::clicked, this, &dlg_CSVReader::btnDeleteFileClicked);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &dlg_CSVReader::okBtnClicked);
}

void dlg_CSVReader::btnAddFilesClicked()
{
	m_filenames.append(QFileDialog::getOpenFileNames(this, tr("Open Files"), m_path,
		tr("Comma-seperated values (*.csv);;All files (*)"), nullptr, QFileDialog::DontConfirmOverwrite));

	if (m_filenames.isEmpty())
	{
		return;
	}

	QStringListModel *model = new QStringListModel(this);
	model->setStringList(m_filenames);
	listView->setModel(model);
}

void dlg_CSVReader::btnDeleteFileClicked()
{
	foreach (const QModelIndex &index, listView->selectionModel()->selectedIndexes())
	{
		QString name = listView->model()->data(index).toString();
		if (m_filenames.contains(name))
		{
			int ind = m_filenames.indexOf(name);
			m_filenames.removeAt(ind);
		}
	}

	QVector<QItemSelectionRange> ranges = listView->selectionModel()->selection().toVector();
	foreach (const QItemSelectionRange &range, ranges)
	{
		listView->model()->removeRows(range.top(), range.height());
	}
}

void dlg_CSVReader::okBtnClicked()
{
	m_dataStorage = new iACsvDataStorage(&m_filenames);

	if (noMDSCheckBox->isChecked())
	{
		noMDSChecked();
	}

	this->accept();
}

void dlg_CSVReader::noMDSChecked()
{
	m_computeMDSFlag = false;
}

iACsvDataStorage* dlg_CSVReader::getCsvDataStorage()
{
	return m_dataStorage;
}

bool dlg_CSVReader::getMDSState()
{
	return m_computeMDSFlag;
}