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

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QSettings>

namespace csvRegKeys
{
	//Names for registry
	static const QString str_settingsName = "FeatureScout";
	static const QString str_formatName = "CSVFormats";
	static const QString str_headerName = "HeaderEntries";
	static const QString str_allHeaders = "AllHeaders";
	static const QString str_reg_skipLinesStart = "SkipLinesStart";
	static const QString str_reg_skipLinesEnd = "SkipLinesEnd";
	static const QString str_reg_colSeparator = "ColumnSeparator";
	static const QString str_reg_decimalSeparator = "DecimalSeparator";
	static const QString str_reg_Spacing = "Spacing";
	static const QString str_reg_Units = "Unit";
	static const QString str_reg_FiberPoreData = "InputObjectType";
}

dlg_CSVInput::dlg_CSVInput(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/) : QDialog(parent, f)
{
	setupUi(this);
	initParameters();
	UpdateColumnMappingInputs();
	connectSignals();
}

void dlg_CSVInput::setPath(QString const & path)
{
	m_fPath = path;
}

void dlg_CSVInput::saveHeaderEntriesToReg(const QStringList& HeaderEntries, const QString &HeaderName, const QString &formatName)
{
	QSettings settings;
	QString settingsName = csvRegKeys::str_settingsName + "/" + csvRegKeys::str_formatName + "/" + formatName;
	settings.beginGroup(settingsName);
	settings.setValue(csvRegKeys::str_headerName,*this->m_selHeaders);
	settings.endGroup();
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

void dlg_CSVInput::LoadHeaderEntriesFromReg(QStringList &HeaderEntries, const QString &HeaderNames, const QString &formatName)
{
	QSettings settings;
	QString settingsName = csvRegKeys::str_settingsName + "/" + csvRegKeys::str_formatName + "/" + formatName;
	settings.beginGroup(settingsName);
	HeaderEntries = settings.value(csvRegKeys::str_headerName).value<QStringList>();
	settings.endGroup();
}

void dlg_CSVInput::connectSignals()
{
	connect(btn_LoadCSVData, SIGNAL(clicked()), this, SLOT(LoadCSVPreviewClicked()));
	connect(btn_SaveFormat, SIGNAL(clicked()), this, SLOT(SaveFormatBtnClicked()));
	connect(btn_UpdatePreview, SIGNAL(clicked()), this, SLOT(UpdateCSVPreview()));
	connect(cmbbox_FormatName, &QComboBox::currentTextChanged, this, &dlg_CSVInput::LoadSelectedFormatSettings);
	connect(cmbbox_ColSeparator, &QComboBox::currentTextChanged, this, &dlg_CSVInput::UpdateCSVPreview);  // switch separator
	connect(cmbbox_ObjectType, &QComboBox::currentTextChanged, this, &dlg_CSVInput::switchCTInputObjectType); // switch between fiber and pores / voids
	connect(cmbbox_Encoding, &QComboBox::currentTextChanged, this, &dlg_CSVInput::UpdateCSVPreview);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(OKButtonClicked()));
	//connect(cmb_box_FileFormat, SIGNAL(currentTextChanged(const QString&)), this, SLOT(LoadFormatSettings(QString)));
	connect(ed_SkipLinesStart, SIGNAL(valueChanged(int)), this, SLOT(UpdateCSVPreview()));
	connect(ed_SkipLinesEnd, SIGNAL(valueChanged(int)), this, SLOT(UpdateCSVPreview()));
	connect(sb_PreviewLines, SIGNAL(valueChanged(int)), this, SLOT(UpdateCSVPreview()));
	connect(cmbbox_col_Selection, &QComboBox::currentTextChanged, this, &dlg_CSVInput::UpdateColumnMappingInputs);
	connect(cb_ComputeLength, &QCheckBox::stateChanged, this, &dlg_CSVInput::UpdateLengthEditVisibility);
	connect(cb_ComputeAngles, &QCheckBox::stateChanged, this, &dlg_CSVInput::UpdateAngleEditVisibility);

}

void dlg_CSVInput::OKButtonClicked()
{
	if (!setSelectedEntries(true))
		return;
	this->accept();
}

void dlg_CSVInput::LoadSelectedFormatSettings(const QString &formatName)
{
	if (formatName.isEmpty())
		return;
	QSettings mySettings;
	QStringList feat_Groups;
	this->m_formatSelected = true;
	bool formatAvailable = CheckFeatureInRegistry(mySettings, &formatName, feat_Groups, true);
	if (!formatAvailable)
	{
		m_previewTable->clearTable();
		clearTextControl();
		QMessageBox::warning(this, tr("FeatureScout"), tr("Format '%1' is not yet defined!").arg(formatName));
		return;
	}
	this->m_formatName = formatName;
	formatAvailable = loadEntriesFromRegistry(mySettings, formatName);
	if (!formatAvailable)
	{
		m_previewTable->clearTable();
		clearTextControl();
		return;
	}

	//if file is not good -> show empty table but selection
	if (this->loadFilePreview(sb_PreviewLines->value(), true))
	{
		this->LoadHeaderEntriesFromReg(*this->m_currentHeaders, csvRegKeys::str_allHeaders, formatName);
	}
	else
	{
		m_previewTable->clearTable();
		return;
	}

	this->LoadHeaderEntriesFromReg(*this->m_selHeaders, csvRegKeys::str_headerName, formatName);
	setSelectedHeaderToTextControl(*this->m_selHeaders); //load all headers
	showConfigParams(this->m_confParams);
}

void dlg_CSVInput::UpdateCSVPreview()
{
	this->m_PreviewUpdated = true;
	this->LoadCSVPreviewClicked();
	this->m_PreviewUpdated = false;
}

void dlg_CSVInput::switchCTInputObjectType(const QString &ObjectInputType)
{
	this->assignInputObjectTypes();
	if (m_confParams.inputObjectType == FiberPoreType::Fiber)
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
	QString formatName = cmbbox_FormatName->currentText();
	if (formatName.trimmed().isEmpty())
	{
		QMessageBox::warning(this, tr("FeatureScout"), tr("Please enter a format name!"));
		return;
	}

	this->assignSeparator();
	this->assignStartEndLine();
	this->assignFormatLanguage();
	this->assignSpacingUnits();
	this->assignInputObjectTypes();

	//header Entries from selection in control list
	this->setSelectedEntries(false);

	QStringList OtherFormatEntries;
	QSettings FormatSettings;

	bool writeSettings = true;
	CheckFeatureInRegistry(FormatSettings, nullptr, OtherFormatEntries, false);

	if (OtherFormatEntries.contains(formatName, Qt::CaseSensitivity::CaseSensitive))
	{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::warning(this, tr("FeatureScout"), tr("Format '%1' already exists. Do you want to overwrite it?").arg(formatName),
			QMessageBox::Yes | QMessageBox::No);
		writeSettings = (reply == QMessageBox::Yes);
	}
	else
	{ // not yet in registry, add
		cmbbox_FormatName->addItem(formatName);
	}

	if (writeSettings)
	{
		saveParamsToRegistry(this->m_confParams, formatName);
		this->saveHeaderEntriesToReg(*this->m_selHeaders, csvRegKeys::str_headerName, formatName);

		//save all entries in order to make sure if file is not available  one still can see the headers??
		this->saveHeaderEntriesToReg(*this->m_currentHeaders, csvRegKeys::str_allHeaders, formatName);
	}
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
	this->assignSeparator();
	this->assignFormatLanguage();
	this->assignInputObjectTypes();
	this->assignStartEndLine();

	if (!this->loadFilePreview(sb_PreviewLines->value(), this->m_PreviewUpdated))
		return;

	if (this->m_formatSelected)
		this->LoadHeaderEntriesFromReg(*this->m_currentHeaders, csvRegKeys::str_allHeaders, this->m_formatName);

	if (m_confParams.inputObjectType == csvConfig::CTInputObjectType::Fiber)		// for fibers use all headers; TODO: find out why!
		setAllHeaders(m_currentHeaders);
	else
		this->setSelectedHeaderToTextControl(*this->m_currentHeaders);
	m_formatSelected = false;
}

void dlg_CSVInput::setAllHeaders(QSharedPointer<QStringList> &allHeaders)
{
	m_confParams.tableWidth = allHeaders->size();
	setSelectedHeaderToTextControl(*allHeaders);
	//ensure that there are values in textcontrol list
	if (list_ColumnSelection->count() > 0)
	{
		list_ColumnSelection->selectAll();
	}
}

void dlg_CSVInput::assignFormatLanguage()
{
	m_confParams.decimalSeparator = cmbbox_DecimalSeparator->currentText();
}

const csvConfig::configPararams& dlg_CSVInput::getConfigParameters() const
{
	return this->m_confParams;
}

void dlg_CSVInput::showConfigParams(const csvConfig::configPararams &params)
{
	QString ObjInputType = (params.inputObjectType == csvConfig::CTInputObjectType::Fiber) ? "Fibers": "Voids";
	int index = cmbbox_ObjectType->findText(ObjInputType, Qt::MatchContains);
	cmbbox_ObjectType->setCurrentIndex(index);

	cmbbox_ColSeparator->setCurrentText(params.colSeparator);
	cmbbox_DecimalSeparator->setCurrentText(params.decimalSeparator);
	ed_SkipLinesStart->setValue(params.skipLinesStart);
	ed_SkipLinesEnd->setValue(params.skipLinesEnd);
	ed_Spacing->setText(QString("%1").arg(params.spacing));
	cmbbox_Unit->setCurrentText(params.unit);
}

void dlg_CSVInput::initParameters()
{
	this->m_previewTable = new dataTable();
	this->myLayout->addWidget(m_previewTable);
	this->m_headersCount = 0;

	if (!this->m_currentHeaders)
	{
		this->m_currentHeaders = QSharedPointer<QStringList>(new QStringList());
	}

	if (!this->m_selHeaders)
	{
		this->m_selHeaders = QSharedPointer < QStringList>(new QStringList());
	}

	//limit input
	this->cmbbox_FormatName->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9_]{0,30}"), this));
	LoadFormatEntriesOnStartUp();
	this->m_formatName = "";
}

void dlg_CSVInput::assignStartEndLine()
{
	m_confParams.skipLinesStart = ed_SkipLinesStart->value();
	m_confParams.skipLinesEnd = ed_SkipLinesEnd->value();
}

void dlg_CSVInput::setError(const QString &ParamName,const QString &Param_value)
{
	this->m_Error_Parameter.append("Error" + ParamName + "\t" + Param_value + "\n");
	m_confParams.paramsValid = false;
}

void dlg_CSVInput::assignInputObjectTypes()
{
	QString InputType = this->cmbbox_ObjectType->currentText();
	if (InputType == "Fibers")
		m_confParams.inputObjectType = csvConfig::CTInputObjectType::Fiber;
	else // if (InputType == "Voids")
		m_confParams.inputObjectType = csvConfig::CTInputObjectType::Voids;
	//else
	//	QMessageBox::warning()
}

void dlg_CSVInput::assignSeparator()
{
	m_confParams.colSeparator = cmbbox_ColSeparator->currentText();
}

void dlg_CSVInput::assignSpacingUnits()
{
	m_confParams.spacing = this->ed_Spacing->text().toDouble();
	m_confParams.unit = this->cmbbox_Unit->currentText();
}

bool dlg_CSVInput::loadFilePreview(const int rowCount, const bool formatLoaded)
{
	isFileNameValid = this->checkFile(formatLoaded);
	if (!isFileNameValid)
		return false;
	QString encoding;
	if (formatLoaded)
		encoding = cmbbox_Encoding->currentText();
	loadEntries(m_confParams.fileName, rowCount, encoding);
	if (!formatLoaded)
		cmbbox_Encoding->setCurrentText(m_previewTable->getLastEncoding());
	txt_ed_fileName->setText(m_confParams.fileName);
	showPreviewTable();
	return true;
}

bool dlg_CSVInput::checkFile(bool LayoutLoaded)
{
	QString fileName = "";
	if (!LayoutLoaded)
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
			this->setError(QString("unable to open file"), file.errorString());
			m_confParams.paramsValid = false;
			return false;
		}
		else
		{
			if (!m_confParams.paramsValid)
			{
				QMessageBox::information(this, tr("FeatureScout"), tr("Wrong parameters assigned: %1").arg(this->m_Error_Parameter));
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
		m_confParams.skipLinesStart, true, false, encoding);
	this->assignHeaderLine();
}

void dlg_CSVInput::showPreviewTable()
{
	m_previewTable->setAutoScroll(true);
	m_previewTable->setEnabled(true);
	m_previewTable->setVisible(true);
}

void dlg_CSVInput::assignHeaderLine()
{
	list_ColumnSelection->clear();
	m_hashEntries.clear();

	if (!m_currentHeaders)
		return;
	*m_currentHeaders = m_previewTable->getHeaders();
	if (m_currentHeaders->isEmpty())
		return;
	m_confParams.tableWidth = m_currentHeaders->length();
	
	int autoIdxCol = 0;
	for (const auto &currItem:*this->m_currentHeaders)
	{
		if (!currItem.trimmed().isEmpty())
		{
			list_ColumnSelection->addItem(currItem);
			m_hashEntries.insert(currItem, autoIdxCol);
		}
		++autoIdxCol;
	}
	list_ColumnSelection->update();
}

bool dlg_CSVInput::setSelectedEntries(const bool EnableMessageBox)
{
	m_selectedHeadersList = list_ColumnSelection->selectedItems();
	if (EnableMessageBox && (m_selectedHeadersList.length() < 2))
	{
		QMessageBox::warning(this, tr("FeatureScout"), "Please select at least 2 columns to load!");
		return false;
	}
	uint currItemIdx;
	QString listEntry;
	//no selection use all entries
	if (this->m_selectedHeadersList.length() != 0)
	{
		if (m_selHeaders->length() > 0)
		{
			this->m_selHeaders->clear();
			this->m_selColIdx.clear();
		}

		this->m_selColIdx.capacity();
		for (const auto &selEntry : m_selectedHeadersList)
		{

			listEntry = selEntry->text();
			addSingleHeaderToList(currItemIdx, listEntry);
		}

		qSort(this->m_selColIdx.begin(), this->m_selColIdx.end(), qLess<uint>());
		this->addSelectedHeaders(this->m_selColIdx);
	}

	m_confParams.colCount = this->m_selColIdx.length();
	return true;
}

//ensure correct order of header!
void dlg_CSVInput::addSelectedHeaders(QVector<uint> &data)
{
	QString curHeader = "";
	this->m_selHeaders->clear();
	for (const auto &HeaderIdx : data) {
		curHeader = this->m_hashEntries.key(HeaderIdx);
		this->m_selHeaders->push_back(curHeader);
	}
}


void dlg_CSVInput::addSingleHeaderToList(uint &currItemIdx, QString &listEntry)
{
	currItemIdx = this->m_hashEntries.value(listEntry);
	this->m_selColIdx.push_back(currItemIdx);

}

const QVector<uint>& dlg_CSVInput::getEntriesSelInd()
{
	return this->m_selColIdx;
}

void dlg_CSVInput::selectSingleHeader(uint & currItemIdx, QString & listEntry)
{

	if (this->m_hashEntries.contains(listEntry))
	{
		currItemIdx = this->m_hashEntries.value(listEntry);
		//set selected if item exists
		if (list_ColumnSelection->item(currItemIdx))
		{
			list_ColumnSelection->item(currItemIdx)->setSelected(true);
		}
	}
}

void dlg_CSVInput::setSelectedHeaderToTextControl(QStringList &sel_headers)
{
	uint itemIDx = 0;
	QSharedPointer<QListWidgetItem> myItem = QSharedPointer<QListWidgetItem>(new QListWidgetItem());

	if ((sel_headers.length() > m_currentHeaders->length()) || sel_headers.length() == 0 )
	{
		QMessageBox::warning(this, tr("FeatureScout"),
			tr("Size of selected headers does not match with headers in file!"));
		return;
	}

	for ( auto &h_entry: sel_headers)
	{
		selectSingleHeader(itemIDx, h_entry);
	}

}

bool dlg_CSVInput::loadEntriesFromRegistry(QSettings & anySetting, const QString & formatName)
{
	QString CSV_InputType = "";
	QString fullName = "";
	QString  cnfgSettingsName;
	QStringList allEntries;

	m_confParams.initDefaultParams();
	cnfgSettingsName = csvRegKeys::str_settingsName + "/" + csvRegKeys::str_formatName + "/" + formatName;
	anySetting.beginGroup(cnfgSettingsName);
	allEntries = anySetting.allKeys();

	if (allEntries.isEmpty())
	{
		QMessageBox::warning(this, tr("FeatureScout"), tr("Format not available!"));
		return false;
	}

	m_confParams.skipLinesStart = anySetting.value(csvRegKeys::str_reg_skipLinesStart).toLongLong(); //startLine
	m_confParams.skipLinesEnd = anySetting.value(csvRegKeys::str_reg_skipLinesEnd).toLongLong() + 1; //endLine Endline +1

	//this->m_confParams->spacing = anySetting.value(this->m_regEntries->str_reg_Spacing).toDouble(); //Spacing TODO TBA
	//this->m_confParams->csv_units = anySetting.value(this->m_regEntries->str_reg_Units).toString(); //Units

	m_confParams.colSeparator = anySetting.value(csvRegKeys::str_reg_colSeparator).toString();//file separator
	m_confParams.decimalSeparator = anySetting.value(csvRegKeys::str_reg_decimalSeparator).toString();

	//Fiber or Pores as Input
	CSV_InputType = anySetting.value(csvRegKeys::str_reg_FiberPoreData).toString();
	if (CSV_InputType == "Voids")
		m_confParams.inputObjectType =csvConfig::CTInputObjectType::Voids;
	else if (CSV_InputType == "Fibers")
		m_confParams.inputObjectType = csvConfig::CTInputObjectType::Fiber;
	anySetting.endGroup();
	return true;
}

void dlg_CSVInput::LoadFormatEntriesOnStartUp()
{
	QSettings settings;
	QStringList formatEntries;
	CheckFeatureInRegistry(settings, nullptr, formatEntries, false);
	if (!formatEntries.isEmpty())
		this->cmbbox_FormatName->addItems(formatEntries);
}

bool dlg_CSVInput::CheckFeatureInRegistry(QSettings & anySetting, const QString *formatName, QStringList &groups, bool useSubGroup)
{
	QString Layout = "";
	QString subEntry = "";
	bool isValidEntry = false;

	if (formatName)
	{
		Layout = *formatName;
		Layout += "/";
	}

	QString regName = csvRegKeys::str_settingsName + "/" + csvRegKeys::str_formatName + "/" + Layout;
	anySetting.beginGroup(regName);
	groups = anySetting.childGroups();
	subEntry = anySetting.group();

	if (useSubGroup)
	{
		if (!subEntry.isEmpty())
		{
			isValidEntry = true;
		}
	}
	else
	{
		if(!groups.isEmpty())
		{
			isValidEntry = true;
		}
	}

	anySetting.endGroup();
	return isValidEntry;
}

void dlg_CSVInput::saveParamsToRegistry(csvConfig::configPararams& csv_params, const QString &formatName)
{
	QSettings settings;
	QString CSVinputObjectType = (csv_params.inputObjectType == csvConfig::CTInputObjectType::Fiber)? "Fibers": "Voids";
	QString settingsName = csvRegKeys::str_settingsName + "/" + csvRegKeys::str_formatName + "/" + formatName;
	settings.beginGroup(settingsName);
	settings.setValue(csvRegKeys::str_reg_colSeparator, csv_params.colSeparator);
	settings.setValue(csvRegKeys::str_reg_skipLinesStart, csv_params.skipLinesStart);
	settings.setValue(csvRegKeys::str_reg_skipLinesEnd, csv_params.skipLinesEnd);
	settings.setValue(csvRegKeys::str_reg_decimalSeparator, csv_params.decimalSeparator);
	settings.setValue(csvRegKeys::str_reg_Spacing, csv_params.spacing);
	settings.setValue(csvRegKeys::str_reg_Units, csv_params.unit);
	settings.setValue(csvRegKeys::str_reg_FiberPoreData, CSVinputObjectType);
	settings.endGroup();
}

//save single setting
void dlg_CSVInput::saveSettings(QSettings &anySetting, const QString &formatName, const QString &FeatureName, const QVariant &feat_value)
{
	QString fullSettingsName = "";
	createSettingsName(fullSettingsName, formatName, FeatureName, true);
	anySetting.setValue(fullSettingsName, feat_value);
}

void dlg_CSVInput::createSettingsName(QString &fullSettingsName, const QString & FormatName, const QString & FeatureName, bool useSubGroup)
{
	QString myFeature = "";
	if (useSubGroup)
	{
		myFeature = FeatureName;
	}
	fullSettingsName = csvRegKeys::str_settingsName + "/" + csvRegKeys::str_formatName + "/" + FormatName + "/" + myFeature;
}
