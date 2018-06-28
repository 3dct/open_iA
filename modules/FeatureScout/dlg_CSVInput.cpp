/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "dlg_CSVInput.h"

#include "DataTable.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QSettings>

namespace csvRegKeys
{
	//Names for registry
	static const QString SettingsName = "FeatureScout";
	static const QString FormatName = "CSVFormats";
	static const QString HeaderName = "HeaderEntries";
	static const QString AllHeaders = "AllHeaders";
	static const QString SkipLinesStart = "SkipLinesStart";
	static const QString SkipLinesEnd = "SkipLinesEnd";
	static const QString ColSeparator = "ColumnSeparator";
	static const QString DecimalSeparator = "DecimalSeparator";
	static const QString Spacing = "Spacing";
	static const QString Unit = "Unit";
	static const QString ObjectType = "ObjectType";
	static const QString AddAutoID = "AddAutoID";
}
namespace
{
	QStringList const & ColumnSeparators()
	{
		static QStringList colSeparators;
		if (colSeparators.isEmpty())
		{                 // has to match order of the entries in cmbbox_ColSeparator!
			colSeparators << ";" << "," << "\t" ;
		}
		return colSeparators;
	}

	QString getFormatRegistryKey(QString const & formatName)
	{
		return csvRegKeys::SettingsName + "/" + csvRegKeys::FormatName + "/" + formatName;
	}
}

dlg_CSVInput::dlg_CSVInput(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/) : QDialog(parent, f)
{
	setupUi(this);
	initParameters();
	UpdateColumnMappingInputs();
	connectSignals();
}

void dlg_CSVInput::connectSignals()
{
	connect(btn_LoadCSVData, SIGNAL(clicked()), this, SLOT(LoadCSVPreviewClicked()));
	connect(btn_SaveFormat, SIGNAL(clicked()), this, SLOT(SaveFormatBtnClicked()));
	connect(btn_DeleteFormat, SIGNAL(clicked()), this, SLOT(DeleteFormatBtnClicked()));
	connect(btn_UpdatePreview, SIGNAL(clicked()), this, SLOT(UpdateCSVPreview()));
	connect(cmbbox_Format, &QComboBox::currentTextChanged, this, &dlg_CSVInput::LoadSelectedFormatSettings);
	connect(cmbbox_ColSeparator, &QComboBox::currentTextChanged, this, &dlg_CSVInput::UpdateCSVPreview);  // switch separator
	connect(cmbbox_ObjectType, &QComboBox::currentTextChanged, this, &dlg_CSVInput::switchObjectType); // switch between fiber and pores / voids
	connect(cmbbox_Encoding, &QComboBox::currentTextChanged, this, &dlg_CSVInput::UpdateCSVPreview);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(OKButtonClicked()));
	connect(ed_SkipLinesStart, SIGNAL(valueChanged(int)), this, SLOT(UpdateCSVPreview()));
	connect(ed_SkipLinesEnd, SIGNAL(valueChanged(int)), this, SLOT(UpdateCSVPreview()));
	connect(sb_PreviewLines, SIGNAL(valueChanged(int)), this, SLOT(UpdateCSVPreview()));
	connect(cmbbox_col_Selection, &QComboBox::currentTextChanged, this, &dlg_CSVInput::UpdateColumnMappingInputs);
	connect(cb_ComputeLength, &QCheckBox::stateChanged, this, &dlg_CSVInput::UpdateLengthEditVisibility);
	connect(cb_ComputeAngles, &QCheckBox::stateChanged, this, &dlg_CSVInput::UpdateAngleEditVisibility);
}

void dlg_CSVInput::initParameters()
{
	m_previewTable = new DataTable();
	myLayout->addWidget(m_previewTable);
	m_headersCount = 0;
	ed_FormatName->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9_]{0,30}"), this)); // limit input to format names
	loadFormatEntriesOnStartUp();
	m_formatName = "";
}

void dlg_CSVInput::setPath(QString const & path)
{
	m_fPath = path;
}

void dlg_CSVInput::saveHeaderEntriesToReg(const QStringList& HeaderEntries, const QString &HeaderName, const QString &formatName)
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(formatName));
	settings.setValue(csvRegKeys::HeaderName, m_selHeaders);
}

void dlg_CSVInput::clearTextControl()
{
	while (list_ColumnSelection->count()>0)
	{
		QListWidgetItem* item = list_ColumnSelection->takeItem(0);
		if (item)
		{
			delete item;
			item = nullptr;
		}
	}
}

void dlg_CSVInput::loadHeaderEntriesFromReg(QStringList & HeaderEntries, const QString &HeaderNames, const QString &formatName)
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(formatName));
	HeaderEntries = settings.value(csvRegKeys::HeaderName).value<QStringList>();
}

void dlg_CSVInput::OKButtonClicked()
{
	if (!setSelectedEntries(true))
		return;
	accept();
}

void dlg_CSVInput::LoadSelectedFormatSettings(const QString &formatName)
{
	if (formatName.isEmpty())
		return;
	m_formatSelected = true;
	m_formatName = formatName;
	bool formatAvailable = loadFormatFromRegistry(formatName);
	if (!formatAvailable)
	{
		m_previewTable->clearTable();
		clearTextControl();
		return;
	}
	showConfigParams(m_confParams);
	//if file is not good -> show empty table but selection
	if (loadFilePreview(sb_PreviewLines->value(), true))
	{
		loadHeaderEntriesFromReg(m_currentHeaders, csvRegKeys::AllHeaders, formatName);
	}
	else
	{
		m_previewTable->clearTable();
		return;
	}
	loadHeaderEntriesFromReg(m_selHeaders, csvRegKeys::HeaderName, formatName);
	setSelectedHeaderToTextControl(m_selHeaders); //load all headers
}

void dlg_CSVInput::UpdateCSVPreview()
{
	m_PreviewUpdated = true;
	LoadCSVPreviewClicked();
	m_PreviewUpdated = false;
}

void dlg_CSVInput::switchObjectType(const QString &ObjectInputType)
{
	assignObjectTypes();
	if (m_confParams.objectType == iAFeatureScoutObjectType::Fibers)
	{
		if (list_ColumnSelection->count() > 0)
		{
			list_ColumnSelection->selectAll();
			list_ColumnSelection->setSelectionMode(QAbstractItemView::NoSelection);
		}
	}
	else
	{
		list_ColumnSelection->setSelectionMode(QAbstractItemView::ExtendedSelection);
	}
}

void dlg_CSVInput::selectAllFromTextControl()
{
	for (int i = 0; i < list_ColumnSelection->count(); ++i)
	{
		list_ColumnSelection->item(i)->setSelected(true);
	}
}

void dlg_CSVInput::SaveFormatBtnClicked()
{
	QString formatName = ed_FormatName->text();
	if (formatName.trimmed().isEmpty())
	{
		QMessageBox::warning(this, tr("FeatureScout"), tr("Please enter a format name!"));
		return;
	}

	assignFormatSettings();

	//header Entries from selection in control list
	setSelectedEntries(false);

	bool writeSettings = true;
	QStringList OtherFormatEntries = GetFormatListFromRegistry();
	if (OtherFormatEntries.contains(formatName, Qt::CaseSensitivity::CaseSensitive))
	{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::warning(this, tr("FeatureScout"),
			tr("Format '%1' already exists. Do you want to overwrite it?").arg(formatName),
			QMessageBox::Yes | QMessageBox::No);
		writeSettings = (reply == QMessageBox::Yes);
	}
	else
	{ // not yet in registry, add
		cmbbox_Format->addItem(formatName);
		//cmbbox_Format->setCurrentText(formatName);
	}

	if (writeSettings)
	{
		saveParamsToRegistry(m_confParams, formatName);
		saveHeaderEntriesToReg(m_selHeaders, csvRegKeys::HeaderName, formatName);

		//save all entries in order to make sure if file is not available  one still can see the headers??
		saveHeaderEntriesToReg(m_currentHeaders, csvRegKeys::AllHeaders, formatName);
	}
}

void dlg_CSVInput::DeleteFormatBtnClicked()
{
	QString formatName = cmbbox_Format->currentText();
	QMessageBox::StandardButton reply;
	reply = QMessageBox::warning(this, tr("FeatureScout"),
		tr("Format '%1' will be deleted permanently. Do you want to proceed?").arg(formatName),
		QMessageBox::Yes | QMessageBox::No);
	if (reply != QMessageBox::Yes)
		return;

	QSettings settings;
	settings.remove(getFormatRegistryKey(formatName));
	cmbbox_Format->removeItem(cmbbox_Format->currentIndex());
}

void dlg_CSVInput::UpdateColumnMappingInputs()
{
	bool useStartEnd = cmbbox_col_Selection->currentIndex() == 0;
	cmbbox_col_PosStartX->setVisible(useStartEnd);
	cmbbox_col_PosStartY->setVisible(useStartEnd);
	cmbbox_col_PosStartZ->setVisible(useStartEnd);
	lbl_col_posStartX->setVisible(useStartEnd);
	lbl_col_posStartY->setVisible(useStartEnd);
	lbl_col_posStartZ->setVisible(useStartEnd);
	cmbbox_col_PosEndX->setVisible(useStartEnd);
	cmbbox_col_PosEndY->setVisible(useStartEnd);
	cmbbox_col_PosEndZ->setVisible(useStartEnd);
	lbl_col_posEndX->setVisible(useStartEnd);
	lbl_col_posEndY->setVisible(useStartEnd);
	lbl_col_posEndZ->setVisible(useStartEnd);

	cmbbox_col_PosCenterX->setVisible(!useStartEnd);
	cmbbox_col_PosCenterY->setVisible(!useStartEnd);
	cmbbox_col_PosCenterZ->setVisible(!useStartEnd);
	lbl_col_posCenterX->setVisible(!useStartEnd);
	lbl_col_posCenterY->setVisible(!useStartEnd);
	lbl_col_posCenterZ->setVisible(!useStartEnd);

	cb_ComputeLength->setEnabled(useStartEnd);
	cb_ComputeAngles->setEnabled(useStartEnd);
	if (!useStartEnd)
	{
		cb_ComputeLength->setChecked(false);
		cb_ComputeAngles->setChecked(false);
	}
	UpdateAngleEditVisibility();
	UpdateLengthEditVisibility();
}

void dlg_CSVInput::UpdateLengthEditVisibility()
{
	cmbbox_col_Length->setVisible(!cb_ComputeLength->isChecked());
	lbl_col_length->setVisible(!cb_ComputeLength->isChecked());
}

void dlg_CSVInput::UpdateAngleEditVisibility()
{
	cmbbox_col_Phi->setVisible(!cb_ComputeAngles->isChecked());
	cmbbox_col_Theta->setVisible(!cb_ComputeAngles->isChecked());
	lbl_col_phi->setVisible(!cb_ComputeAngles->isChecked());
	lbl_col_theta->setVisible(!cb_ComputeAngles->isChecked());
}

void dlg_CSVInput::LoadCSVPreviewClicked()
{
	assignFormatSettings();

	if (!loadFilePreview(sb_PreviewLines->value(), m_PreviewUpdated))
		return;

	if (m_formatSelected)
		loadHeaderEntriesFromReg(m_currentHeaders, csvRegKeys::AllHeaders, m_formatName);

	if (m_confParams.objectType == iAFeatureScoutObjectType::Fibers)		// for fibers use all headers; TODO: find out why!
		setAllHeaders(m_currentHeaders);
	else
		setSelectedHeaderToTextControl(m_currentHeaders);
	m_formatSelected = false;
}

void dlg_CSVInput::setAllHeaders(QStringList const & allHeaders)
{
	m_confParams.tableWidth = allHeaders.size();
	setSelectedHeaderToTextControl(allHeaders);
	//ensure that there are values in textcontrol list
	if (list_ColumnSelection->count() > 0)
	{
		list_ColumnSelection->selectAll();
	}
}

iACsvConfig const & dlg_CSVInput::getConfigParameters() const
{
	return m_confParams;
}

void dlg_CSVInput::showConfigParams(iACsvConfig const &params)
{
	QString ObjInputType = MapObjectTypeToString(params.objectType);
	int index = cmbbox_ObjectType->findText(ObjInputType, Qt::MatchContains);
	cmbbox_ObjectType->setCurrentIndex(index);

	cmbbox_ColSeparator->setCurrentIndex(ColumnSeparators().indexOf(params.colSeparator));
	cmbbox_DecimalSeparator->setCurrentText(params.decimalSeparator);
	ed_SkipLinesStart->setValue(params.skipLinesStart);
	ed_SkipLinesEnd->setValue(params.skipLinesEnd);
	ed_Spacing->setText(QString("%1").arg(params.spacing));
	cmbbox_Unit->setCurrentText(params.unit);
	ed_FormatName->setText(params.formatName);
}

void dlg_CSVInput::setError(const QString &ParamName, const QString &Param_value)
{
	m_Error_Parameter.append("Error" + ParamName + "\t" + Param_value + "\n");
	m_confParams.paramsValid = false;
}

void dlg_CSVInput::assignFormatSettings()
{
	m_confParams.colSeparator = ColumnSeparators()[cmbbox_ColSeparator->currentIndex()];
	m_confParams.decimalSeparator = cmbbox_DecimalSeparator->currentText();
	m_confParams.skipLinesStart = ed_SkipLinesStart->value();
	m_confParams.skipLinesEnd = ed_SkipLinesEnd->value();
	m_confParams.spacing = ed_Spacing->text().toDouble();
	m_confParams.unit = cmbbox_Unit->currentText();
	m_confParams.addAutoID = cb_addAutoID->isChecked();
	assignObjectTypes();
}

void dlg_CSVInput::assignObjectTypes()
{
	m_confParams.objectType = MapStringToObjectType(cmbbox_ObjectType->currentText());
}

void dlg_CSVInput::assignHeaderLine()
{
	list_ColumnSelection->clear();
	m_hashEntries.clear();

	m_currentHeaders = m_previewTable->getHeaders();
	if (m_currentHeaders.isEmpty())
		return;
	m_confParams.tableWidth = m_currentHeaders.length();

	int autoIdxCol = 0;
	for (const auto &currItem : m_currentHeaders)
	{
		if (!currItem.trimmed().isEmpty())
		{
			list_ColumnSelection->addItem(currItem);
			m_hashEntries.insert(currItem, autoIdxCol);
		}
		++autoIdxCol;
	}
}

bool dlg_CSVInput::loadFilePreview(const int rowCount, const bool formatLoaded)
{
	isFileNameValid = checkFile(formatLoaded);
	if (!isFileNameValid)
		return false;
	QString encoding;
	if (formatLoaded)
		encoding = cmbbox_Encoding->currentText();
	loadEntries(m_confParams.fileName, rowCount, encoding);
	if (!formatLoaded)
	{
		QSignalBlocker blocker(cmbbox_Encoding);
		cmbbox_Encoding->setCurrentText(m_previewTable->getLastEncoding());
	}
	txt_ed_fileName->setText(m_confParams.fileName);
	showPreviewTable();
	return true;
}

bool dlg_CSVInput::checkFile(bool formatLoaded)
{
	QString fileName = "";
	if (!formatLoaded)
	{
		fileName = QFileDialog::getOpenFileName(
			this, tr("Open Files"), m_fPath, tr("csv spreadsheet (*.csv),.csv")
		);
	}
	else
	{
		fileName = m_confParams.fileName;
	}

	bool fileOK = false;
	if (fileName.isEmpty())
	{
		m_confParams.paramsValid = false;
		return false;
	}
	else
	{
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::information(this, tr("FeatureScout"), tr("Unable to open file: %1").arg(file.errorString()));
			setError(QString("unable to open file"), file.errorString());
			m_confParams.paramsValid = false;
			return false;
		}
		else
		{
			if (!m_confParams.paramsValid)
			{
				QMessageBox::information(this, tr("FeatureScout"), tr("Wrong parameters assigned: %1").arg(m_Error_Parameter));
			}
			else
			{
				fileOK = true;
				m_confParams.fileName = fileName;
			}
		}
		if (file.isOpen())
		{
			file.close();
		}
	}
	return fileOK;
}

void dlg_CSVInput::loadEntries(const QString& fileName, const unsigned int nrPreviewElements, QString const & encoding)
{
	m_previewTable->clearTable();
	m_previewTable->readTableEntries(fileName, nrPreviewElements, m_confParams.colSeparator,
		m_confParams.skipLinesStart, true, cb_addAutoID->isChecked(), encoding);
	assignHeaderLine();
}

void dlg_CSVInput::showPreviewTable()
{
	m_previewTable->setAutoScroll(true);
	m_previewTable->setEnabled(true);
	m_previewTable->setVisible(true);
}

bool dlg_CSVInput::setSelectedEntries(const bool EnableMessageBox)
{
	m_selectedHeadersList = list_ColumnSelection->selectedItems();
	if (EnableMessageBox && (m_selectedHeadersList.length() < 2))
	{
		QMessageBox::warning(this, tr("FeatureScout"), "Please select at least 2 columns to load!");
		return false;
	}
	//no selection use all entries
	if (m_selectedHeadersList.length() != 0)
	{
		if (m_selHeaders.length() > 0)
		{
			m_selHeaders.clear();
			m_selColIdx.clear();
		}

		m_selColIdx.capacity();
		for (const auto &selEntry : m_selectedHeadersList)
		{
			QString listEntry = selEntry->text();
			addSingleHeaderToList(listEntry);
		}

		qSort(m_selColIdx.begin(), m_selColIdx.end(), qLess<uint>());
		addSelectedHeaders(m_selColIdx);
	}
	return true;
}

//ensure correct order of header!
void dlg_CSVInput::addSelectedHeaders(QVector<uint> &data)
{
	m_selHeaders.clear();
	for (const auto &HeaderIdx : data)
	{
		QString curHeader = m_hashEntries.key(HeaderIdx);
		m_selHeaders.push_back(curHeader);
	}
}

void dlg_CSVInput::addSingleHeaderToList(QString const & listEntry)
{
	uint currItemIdx = m_hashEntries[listEntry];
	m_selColIdx.push_back(currItemIdx);
}

const QVector<uint>& dlg_CSVInput::getEntriesSelInd()
{
	return m_selColIdx;
}

void dlg_CSVInput::selectSingleHeader(QString const & listEntry)
{
	if (m_hashEntries.contains(listEntry))
	{
		uint currItemIdx = m_hashEntries.value(listEntry);
		//set selected if item exists
		if (list_ColumnSelection->item(currItemIdx))
		{
			list_ColumnSelection->item(currItemIdx)->setSelected(true);
		}
	}
}

void dlg_CSVInput::setSelectedHeaderToTextControl(QStringList const &sel_headers)
{
	QSharedPointer<QListWidgetItem> myItem = QSharedPointer<QListWidgetItem>(new QListWidgetItem());

	if ((sel_headers.length() > m_currentHeaders.length()) || sel_headers.length() == 0 )
	{
		QMessageBox::warning(this, tr("FeatureScout"),
			tr("Size of selected headers does not match with headers in file!"));
		return;
	}

	for ( auto &h_entry: sel_headers)
	{
		selectSingleHeader(h_entry);
	}
}

bool dlg_CSVInput::loadFormatFromRegistry(const QString & formatName)
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(formatName));
	QStringList allEntries = settings.allKeys();
	if (allEntries.isEmpty())
	{
		QMessageBox::warning(this, tr("FeatureScout"), tr("Format not available!"));
		return false;
	}
	iACsvConfig defaultConfig;
	m_confParams.skipLinesStart = settings.value(csvRegKeys::SkipLinesStart, defaultConfig.skipLinesStart).toLongLong();
	m_confParams.skipLinesEnd = settings.value(csvRegKeys::SkipLinesEnd, defaultConfig.skipLinesEnd).toLongLong();
	m_confParams.colSeparator = settings.value(csvRegKeys::ColSeparator, defaultConfig.colSeparator).toString();
	m_confParams.decimalSeparator = settings.value(csvRegKeys::DecimalSeparator, defaultConfig.decimalSeparator).toString();
	m_confParams.objectType = MapStringToObjectType(settings.value(csvRegKeys::ObjectType, defaultConfig.objectType).toString());
	m_confParams.addAutoID = settings.value(csvRegKeys::AddAutoID, defaultConfig.addAutoID).toBool();
	m_confParams.unit = settings.value(csvRegKeys::Unit, defaultConfig.unit).toString();
	m_confParams.spacing = settings.value(csvRegKeys::Spacing, defaultConfig.spacing).toDouble();
	return true;
}

void dlg_CSVInput::loadFormatEntriesOnStartUp()
{
	QStringList formatEntries = GetFormatListFromRegistry();
	cmbbox_Format->addItems(formatEntries);
}

QStringList dlg_CSVInput::GetFormatListFromRegistry() const
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey("") );
	return settings.childGroups();
}

void dlg_CSVInput::saveParamsToRegistry(iACsvConfig const & csv_params, const QString &formatName)
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(formatName));
	settings.setValue(csvRegKeys::SkipLinesStart, csv_params.skipLinesStart);
	settings.setValue(csvRegKeys::SkipLinesEnd, csv_params.skipLinesEnd);
	settings.setValue(csvRegKeys::ColSeparator, csv_params.colSeparator);
	settings.setValue(csvRegKeys::DecimalSeparator, csv_params.decimalSeparator);
	settings.setValue(csvRegKeys::ObjectType, MapObjectTypeToString(csv_params.objectType));
	settings.setValue(csvRegKeys::AddAutoID, csv_params.addAutoID);
	settings.setValue(csvRegKeys::Unit, csv_params.unit);
	settings.setValue(csvRegKeys::Spacing, csv_params.spacing);
}
