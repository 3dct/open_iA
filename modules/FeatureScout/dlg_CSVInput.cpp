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

dlg_CSVInput::dlg_CSVInput(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/) : QDialog(parent, f)
{
	this->setupUi(this);
	this->initParameters();
	this->myLayout->addWidget(this->m_entriesPreviewTable);
	this->m_confParams = QSharedPointer<csvConfig::configPararams>(new csvConfig::configPararams());
	connectSignals();
}

void dlg_CSVInput::saveHeaderEntriesToReg(const QStringList& HeaderEntries, const QString &HeaderName, const QString &LayoutName)
{
	QSettings settings;
	QString settingsName = "";

	settingsName = this->m_regEntries->str_settingsName + "/" + this->m_regEntries->str_formatName + "/" + LayoutName;
	settings.beginGroup(settingsName);
	settings.setValue(this->m_regEntries->str_headerName,*this->m_selHeaders);
	settings.endGroup();
}

void dlg_CSVInput::clearTextControl()
{
	while (this->textControl_list->count()>0)
	{
		QListWidgetItem* item = this->textControl_list->takeItem(0);
		if (item)
		{
			delete item;
			item = nullptr;
		}
	}
}

void dlg_CSVInput::LoadHeaderEntriesFromReg(QStringList &HeaderEntries, const QString &HeaderNames, const QString &LayoutName)
{
	QSettings settings;
	QString settingsName = "";
	settingsName = this->m_regEntries->str_settingsName + "/" + this->m_regEntries->str_formatName + "/" + LayoutName;
	settings.beginGroup(settingsName);
	HeaderEntries = settings.value(this->m_regEntries->str_headerName).value<QStringList>();
	settings.endGroup();
}

void dlg_CSVInput::connectSignals()
{
	connect(btn_LoadCSVData, SIGNAL(clicked()), this, SLOT(LoadCSVPreviewClicked()));
	connect(btn_SaveFormat, SIGNAL(clicked()), this, SLOT(SaveFormatBtnClicked()));
	connect(btn_updatePreview, SIGNAL(clicked()), this, SLOT(UpdateCSVPreview()));
	connect(cmb_box_FileFormat, &QComboBox::currentTextChanged, this, &dlg_CSVInput::LoadSelectedFormatSettings);
	connect(cmb_box_separator, &QComboBox::currentTextChanged, this, &dlg_CSVInput::UpdateCSVPreview);  // switch separator
	connect(cmb_box_InputObject, &QComboBox::currentTextChanged, this, &dlg_CSVInput::switchCTInputObjectType); // switch between fiber and pores / voids
	connect(cmb_box_Encoding, &QComboBox::currentTextChanged, this, &dlg_CSVInput::UpdateCSVPreview);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(OKButtonClicked()));
	//connect(cmb_box_FileFormat, SIGNAL(currentTextChanged(const QString&)), this, SLOT(LoadFormatSettings(QString)));
	connect(ed_startLine, SIGNAL(valueChanged(int)), this, SLOT(UpdateCSVPreview()));
	connect(ed_endLine, SIGNAL(valueChanged(int)), this, SLOT(UpdateCSVPreview()));
}

void dlg_CSVInput::OKButtonClicked()
{
	if (!setSelectedEntries(true))
		return;
	this->accept();
}

void dlg_CSVInput::LoadSelectedFormatSettings(const QString &LayoutName)
{
	bool layoutAvaiable = false;
	if (LayoutName.isEmpty())
		return;
	QSettings mySettings;
	QStringList feat_Groups;
	layoutAvaiable = CheckFeatureInRegistry(mySettings, &LayoutName, feat_Groups, true);
	this->m_formatSelected = true;

	if (!layoutAvaiable)
	{
		this->resetTable();
		QMessageBox::warning(this, tr("FeatureScoutCSV"), tr("Layout option not yet defined"));
		this->resetTable();
		clearTextControl();
		return;
	}

	this->m_LayoutName = LayoutName;
	layoutAvaiable = loadEntriesFromRegistry(mySettings, LayoutName);
	if (!layoutAvaiable)
	{
		this->resetTable();
		clearTextControl();
		return;
	}
	//load preview

	//if file is not good -> show empty table but selection
	if (this->loadFilePreview(15, true)) {
		this->LoadHeaderEntriesFromReg(*this->m_currentHeaders, this->m_regEntries->str_allHeaders, LayoutName);
	}
	else
	{
		this->resetTable();
		return;
	}

	this->LoadHeaderEntriesFromReg(*this->m_selHeaders,this->m_regEntries->str_headerName, LayoutName);
	setSelectedHeaderToTextControl(*this->m_selHeaders); //load all headers
	showConfigParams(*this->m_confParams, true);
}

void dlg_CSVInput::UpdateCSVPreview()
{
	this->m_PreviewUpdated = true;
	this->LoadCSVPreviewClicked();
	this->m_PreviewUpdated = false ;
}

void dlg_CSVInput::switchCTInputObjectType(const QString &ObjectInputType)
{
	this->assignInputObjectTypes();
	if (m_confParams->inputObjectType == FiberPoreType::Fiber)
	{
		if (this->textControl_list->count() > 0)
		{
			this->textControl_list->selectAll();
			this->textControl_list->setSelectionMode(QAbstractItemView::NoSelection);
		}
	}
	else
	{
		this->textControl_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	}
}

void dlg_CSVInput::resetTable()
{
	this->m_entriesPreviewTable->clearTable();
	this->m_entriesPreviewTable->update();
	this->m_entriesPreviewTable->resetIndizes();
}

void dlg_CSVInput::selectAllFromTextControl()
{
	for (int i = 0; i < this->textControl_list->count(); ++i)
	{
		 this-> textControl_list->item(i)->setSelected(true);
	}
}

void dlg_CSVInput::SaveFormatBtnClicked()
{
	csvConfig::configPararams params;
	QString layoutName = this->ed_CSVFormat_Name->text();
	if (layoutName.trimmed().isEmpty())
	{
		QMessageBox::warning(this, tr("FeatureScoutCSV"), tr("Please enter layout name"));
		return;
	}

	this->assignSeparator();
	this->assignStartEndLine();
	this->assignFormatLanguage();
	this->assignSpacingUnits();
	//Input Type Fiber or Pores
	this->assignInputObjectTypes();

	//header Entries from selection in control list
	this->setSelectedEntries(false);
	params = *this->m_confParams;

	QStringList OtherFormatEntries;
	QSettings FormatSettings;

	bool writeSettings = false;
	CheckFeatureInRegistry(FormatSettings, nullptr, OtherFormatEntries, false);

	if (OtherFormatEntries.contains(layoutName, Qt::CaseSensitivity::CaseSensitive)) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::warning(this, "Overwrite current format", "Sure to overwrite old format?",
			QMessageBox::Yes | QMessageBox::No);
		writeSettings = (reply == QMessageBox::Yes);
	}
	else
	{ // not in registry add anyway
		writeSettings = true;
		this->cmb_box_FileFormat->addItem(layoutName);
	}

	if (writeSettings)
	{
		saveParamsToRegistry(params, layoutName);
		this->saveHeaderEntriesToReg(*this->m_selHeaders, this->m_regEntries->str_headerName, layoutName);

		//save all entries in order to make sure if file is not available  one still can see the headers??
		this->saveHeaderEntriesToReg(*this->m_currentHeaders, this->m_regEntries->str_allHeaders, layoutName);
	}

}

void dlg_CSVInput::LoadCSVPreviewClicked()
{
	this->assignSeparator();
	this->assignFormatLanguage();
	this->assignInputObjectTypes();
	this->assignStartEndLine();
	this->m_entriesPreviewTable->resetIndizes();
	if (!this->loadFilePreview(15, this->m_PreviewUpdated))
	{
		return;
	}

	//for fibers use all headers;
	if (this->m_confParams->inputObjectType == csvConfig::CTInputObjectType::Fiber)
	{
		if (this->m_formatSelected)
		{
			this->LoadHeaderEntriesFromReg(*this->m_currentHeaders, this->m_regEntries->str_allHeaders, this->m_LayoutName);
		}
		setAllHeaders(m_currentHeaders);
	}
	else
	{
		if (this->m_formatSelected)
		{
			//output m_currentHeaders
			this->LoadHeaderEntriesFromReg(*this->m_currentHeaders, this->m_regEntries->str_allHeaders, this->m_LayoutName);
		}
		this->setSelectedHeaderToTextControl(*this->m_currentHeaders);
	}

	// show fileName
	this->showConfigParams(*this->m_confParams, false);
	this->m_formatSelected = false;
}


void dlg_CSVInput::setAllHeaders(QSharedPointer<QStringList> &allHeaders)
{
	//clear data
	/*for (int i = 0; i < this->textControl_list->model()->rowCount(); i++) {
		this->textControl_list->takeItem(i);
	}*/
	this->m_confParams->tableWidth = allHeaders->size();
	this->setSelectedHeaderToTextControl(*allHeaders);

	//ensure that there are values in textcontrol list
	if (textControl_list->count() > 0)
	{
		this->textControl_list->selectAll();
	}

}

void dlg_CSVInput::assignFormatLanguage()
{
	m_confParams->csv_Inputlanguage = (cb_fmtEnglish->isChecked()) ? csvConfig::inputLang::EN : csvConfig::inputLang::DE;
}

const csvConfig::configPararams& dlg_CSVInput::getConfigParameters() const
{
	return *this->m_confParams;
}

void dlg_CSVInput::showConfigParams(const csvConfig::configPararams &params, const bool paramsLoaded)
{
	if (paramsLoaded)
	{
		QString FileSeparator = "";
		if (params.file_seperator == csvColSeparator::Comma)
			FileSeparator = "COMMA (,)";
		else
			FileSeparator = "COLUMN (;)";
		int currIdx = cmb_box_separator->findText(FileSeparator, Qt::MatchContains);
		this->cmb_box_separator->setCurrentIndex(currIdx);

		QString ObjInputType = "";
		if (params.inputObjectType == csvConfig::CTInputObjectType::Fiber)
			ObjInputType = "Fiber";
		else
			ObjInputType = "Pores";
		int index = cmb_box_InputObject->findText(ObjInputType, Qt::MatchContains);
		this->cmb_box_InputObject->setCurrentIndex(index);

		this->ed_startLine->setValue(params.skipLinesStart);
		this->ed_endLine->setValue(params.skipLinesEnd);
		this->ed_Spacing->setText(QString("%1").arg(params.spacing));
		this->ed_Units->setText(QString("1").arg(params.csv_units));
		this->cb_fmtEnglish->setChecked(m_confParams->csv_Inputlanguage == csvLang::EN);
	}
	this->txt_ed_fileName->setText(params.fileName);
}

void dlg_CSVInput::initParameters()
{
	if (!this->m_confParams)
		this->m_confParams = QSharedPointer<csvConfig::configPararams>(new csvConfig::configPararams());

	if(!this->m_regEntries)
	{
		this->m_regEntries = cvsRegSettings_ShrdPtr(new csv_reg());
	}
	initBasicFormatParameters(csvLang::EN, csvColSeparator::Colunm, csvFormat::Default);
	initLineSkip(5, 0);
	this->m_confParams->spacing = 10.5f;
	this->m_confParams->csv_units = "microns";
	this->m_confParams->paramsValid = false;
	this->m_fPath = "C:/TestData/";

	this->m_entriesPreviewTable = new dataTable();
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
	this->ed_CSVFormat_Name->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9_]{0,30}"), this));
	this->m_regEntries->initParam();
	LoadFormatEntriesOnStartUp();
	this->m_LayoutName = "";

}

void dlg_CSVInput::initBasicFormatParameters(csvLang Language, csvColSeparator FileSeparator, csvFormat FileFormat)
{
	this->m_confParams->file_seperator = FileSeparator;
	this->m_confParams->csv_Inputlanguage = Language;
	this->m_confParams->file_fmt = FileFormat;
}

void dlg_CSVInput::initLineSkip(uint skipLinesStart, uint skipLinesEnd)
{
	this->m_confParams->skipLinesStart = skipLinesStart;
	this->m_confParams->headerStartLine = skipLinesStart;
	this->m_confParams->skipLinesEnd = skipLinesEnd;
}

//todo validate data format
void dlg_CSVInput::assignStartEndLine()
{
	this->initLineSkip(ed_startLine->value(), ed_endLine->value());
}

void dlg_CSVInput::setError(const QString &ParamName,const QString &Param_value)
{
	this->m_Error_Parameter.append("Error" + ParamName + "\t" + Param_value + "\n");
	this->m_confParams->paramsValid = false;
}

void dlg_CSVInput::assignInputObjectTypes(){
	QString InputType = this->cmb_box_InputObject->currentText();
	if (InputType == "Fiber")
	{
		this->m_confParams->inputObjectType = csvConfig::CTInputObjectType::Fiber;

	}
	else
	{
		if (InputType == "Pores")
		{
			this->m_confParams->inputObjectType = csvConfig::CTInputObjectType::Voids;
		}
	}
}

void dlg_CSVInput::assignSeparator()
{
	bool param_seperator_ok = false;
	QString tmp_seperator = this->cmb_box_separator->currentText();

	if (tmp_seperator.contains(";"))
	{
		this->m_confParams->file_seperator = csvColSeparator::Colunm;
		param_seperator_ok = true;
	}
	else if (tmp_seperator.contains(","))
	{
		this->m_confParams->file_seperator = csvColSeparator::Comma;
		param_seperator_ok = true;
	}
	else
	{
		this->setError("Separator", tmp_seperator);
	}
	this->m_confParams->paramsValid = param_seperator_ok;
}

void dlg_CSVInput::assignSpacingUnits()
{
	this->m_confParams->spacing = this->ed_Spacing->text().toDouble();
	this->m_confParams->csv_units = this->ed_Units->text();
}

bool dlg_CSVInput::loadFilePreview(const int rowCount, const bool formatLoaded)
{
	m_entriesPreviewTable->setColSeparator(this->m_confParams->file_seperator);
	isFileNameValid = this->checkFile(formatLoaded);
	if (!isFileNameValid)
	{
			return false;
	}
	this->m_entriesPreviewTable->prepareTable(rowCount, this->m_confParams->colCount, this->m_confParams->headerStartLine);
	QString encoding;
	if (formatLoaded)
		encoding = cmb_box_Encoding->currentText();
	this->loadEntries(this->m_confParams->fileName, rowCount, encoding);
	if (!formatLoaded)
		cmb_box_Encoding->setCurrentText(m_entriesPreviewTable->getLastEncoding());
	this->txt_ed_fileName->setText(this->m_confParams->fileName);

	//adding text to label
	this->showPreviewTable();

	return true;
}

bool dlg_CSVInput::checkFile(bool LayoutLoaded)
{
	bool fileOK = false;
	QString fileName = "";
	if (m_fPath.isEmpty())
	{
		this->m_confParams->paramsValid = false;
		return fileOK;
	}

	if (!LayoutLoaded)
	{
		fileName = QFileDialog::getOpenFileName(
			this, tr("Open Files"), m_fPath, tr("csv spreadsheet (*.csv),.csv")
		);
	}
	else
	{
		fileName = this->m_confParams->fileName;
	}

	if (fileName.isEmpty())
	{
		this->m_confParams->paramsValid = false;
		return fileOK;
	}
	else
	{
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::information(this, tr("Unable to open file"),
				file.errorString());
			this->setError(QString("unable to open file"), file.errorString());
			this->m_confParams->paramsValid = false;
			return fileOK;
		}
		else
		{
			this->m_confParams->paramsValid = (this->m_confParams->paramsValid && true);

			if (!this->m_confParams->paramsValid)
			{
				QMessageBox::information(this, tr("Wrong parameters assigned:"),
					this->m_Error_Parameter);
			}
			else
			{
				fileOK = true;
				this->m_confParams->fileName = fileName;
			}
		}
		if (file.isOpen()) {
			file.close();
		}
	}

	return fileOK;
}

bool dlg_CSVInput::loadEntries(const QString& fileName, const unsigned int nrPreviewElements, QString const & encoding)
{
	bool dataLoaded = false;
	if (isFilledWithData)
		this->m_entriesPreviewTable->clearTable();
	dataLoaded = this->m_entriesPreviewTable->readTableEntries(fileName, nrPreviewElements, this->m_confParams->colCount,
		this->m_confParams->headerStartLine, this->m_confParams->skipLinesStart, true, false, encoding);
	this->m_entriesPreviewTable->update();
	this->assignHeaderLine();
	isFilledWithData = true;
	return dataLoaded;
}

void dlg_CSVInput::showPreviewTable()
{
	this->m_entriesPreviewTable->setAutoScroll(true);
	this->m_entriesPreviewTable->setEnabled(true);
	this->m_entriesPreviewTable->setVisible(true);
	this->m_entriesPreviewTable->update();
}

void dlg_CSVInput::assignHeaderLine()
{
	this->textControl_list->clear();
	this->textControl_list->update();
	this->m_hashEntries.clear();

	int autoIdxCol = 0;
	if (!this->m_currentHeaders) return;
	*this->m_currentHeaders = m_entriesPreviewTable->getHeaders();
	if (this->m_currentHeaders->isEmpty()) return;
	this->m_confParams->tableWidth = this->m_currentHeaders->length();

	for (const auto &currItem:*this->m_currentHeaders){
		if (!currItem.trimmed().isEmpty())
		{
				this->textControl_list->addItem(currItem);
				this->m_hashEntries.insert(currItem, autoIdxCol);
		}
			autoIdxCol++;
	}
	this->textControl_list->update();
}

bool dlg_CSVInput::setSelectedEntries(const bool EnableMessageBox)
{
	this->m_selectedHeadersList = this->textControl_list->selectedItems();
	if (EnableMessageBox && (this->m_selectedHeadersList.length() < 2))
	{
		QMessageBox::warning(this, tr("FeatureScoutCSV"), "Please select at least 2 columns to load");
		return false;
	}


	uint currItemIdx;
	QString listEntry;
	//no selection use all entries
	if (!(this->m_selectedHeadersList.length() == 0))
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

	this->m_confParams->colCount = this->m_selColIdx.length();
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
		if (this->textControl_list->item(currItemIdx))
		{
			this->textControl_list->item(currItemIdx)->setSelected(true);
		}
	}
}

void dlg_CSVInput::setSelectedHeaderToTextControl(QStringList &sel_headers)
{
	uint itemIDx = 0;
	QSharedPointer<QListWidgetItem> myItem = QSharedPointer<QListWidgetItem>(new QListWidgetItem());

	if ((sel_headers.length() > this->m_currentHeaders->length())|| sel_headers.length() == 0 )
	{
		QMessageBox::warning(this, tr("Data Preview"),
			tr("Size of selected headers does not match with headers in file.\n"));
		return;
	}

	for ( auto &h_entry: sel_headers)
	{
		selectSingleHeader(itemIDx, h_entry);
	}

}

bool dlg_CSVInput::loadEntriesFromRegistry(QSettings & anySetting, const QString & LayoutName)
{
	QString f_separator = "";
	QString CSV_InputType = "";
	bool useEN_DecimalPoint = false;
	QString fullName = "";
	QString  cnfgSettingsName;
	QStringList allEntries;

	this->m_confParams->resetParams();
	cnfgSettingsName = this->m_regEntries->str_settingsName + "/" + this->m_regEntries->str_formatName + "/" + LayoutName;
	anySetting.beginGroup(cnfgSettingsName);
	allEntries = anySetting.allKeys();

	if (allEntries.isEmpty())
	{
		QMessageBox::warning(this, tr("Error"),
			tr("Format not available"));
		return false;
	}

	this->m_confParams->fileName = anySetting.value(this->m_regEntries->str_fileName).toString();
	this->m_confParams->skipLinesStart = anySetting.value( this->m_regEntries->str_reg_startLine).toLongLong(); //startLine
	this->m_confParams->headerStartLine = this->m_confParams->skipLinesStart;
	this->m_confParams->skipLinesEnd = anySetting.value(this->m_regEntries->str_reg_EndLine).toLongLong() + 1; //endLine Endline +1

	//this->m_confParams->spacing = anySetting.value(this->m_regEntries->str_reg_Spacing).toDouble(); //Spacing TODO TBA
	//this->m_confParams->csv_units = anySetting.value(this->m_regEntries->str_reg_Units).toString(); //Units

	f_separator = anySetting.value(this->m_regEntries->str_reg_colSeparator).toString();//file separator

	if (f_separator.contains("Comma"))
	{
		this->m_confParams->file_seperator = csvColSeparator::Comma;
	}
	else
	{
		if (f_separator.contains("Column"))
		{
			this->m_confParams->file_seperator = csvColSeparator::Colunm;
		}
		//add more if neccessary
	}

	useEN_DecimalPoint = anySetting.value(this->m_regEntries->str_reg_languageFormat).toBool();//inputlang - decimalPoint
	this->m_confParams->csv_Inputlanguage = useEN_DecimalPoint ? csvLang::EN : csvLang::DE;

	//Fiber or Pores as Input
	CSV_InputType = anySetting.value(this->m_regEntries->str_reg_FiberPoreData).toString();
	if (CSV_InputType == "Pores")
	{
		this->m_confParams->inputObjectType =csvConfig::CTInputObjectType::Voids;
	}
	else
	{
		if (CSV_InputType == "Fiber")
		{
			this->m_confParams->inputObjectType = csvConfig::CTInputObjectType::Fiber;
		}
	}
	anySetting.endGroup();
	return true;
}

void dlg_CSVInput::LoadFormatEntriesOnStartUp()
{
	QSettings settings;

	QString MaviOption = "MAVI";
	QString DefaultOption = "Default";
	QString open_iA_Option = "open_iA";
	QString VolumeGraphicsOption = "VolumeGraphics";
	QStringList OtherFormatEntries;

	this->cmb_box_FileFormat->addItem(MaviOption);
	this->cmb_box_FileFormat->addItem(DefaultOption);
	this->cmb_box_FileFormat->addItem(open_iA_Option);
	this->cmb_box_FileFormat->addItem(VolumeGraphicsOption);

	//Output values OtherformatEntries
	CheckFeatureInRegistry(settings, nullptr, OtherFormatEntries,false);

	if (!OtherFormatEntries.isEmpty())
	{
		this->cmb_box_FileFormat->addItems(OtherFormatEntries);
	}
}

bool dlg_CSVInput::CheckFeatureInRegistry(QSettings & anySetting, const QString *LayoutName, QStringList &groups, bool useSubGroup)
{
	QString Layout = "";
	QString subEntry = "";
	bool isValidEntry = false;

	if (LayoutName)
	{
		Layout = *LayoutName;
		Layout += "/";
	}

	QString regName = this->m_regEntries->str_settingsName + "/" + this->m_regEntries->str_formatName + "/" + Layout;
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

void dlg_CSVInput::saveParamsToRegistry(csvConfig::configPararams& csv_params, const QString &LayoutName)
{
	QSettings settings;
	QString settingsName="";
	QString colSeparator = "";
	QString CSVinputObjectType;
	bool useEN_Decimals = false;

	if (this->m_regEntries)
	{
		switch (csv_params.file_seperator)
		{
			default:
			case csvColSeparator::Colunm: colSeparator = "Column"; break;
			case csvColSeparator::Comma : colSeparator = "Comma"; break;
		}
		switch (csv_params.csv_Inputlanguage)
		{
			default:
			case csvLang::EN: useEN_Decimals = true; break;
			case csvLang::DE: useEN_Decimals = false; break;
		}
		switch (csv_params.inputObjectType)
		{
			case csvConfig::CTInputObjectType::Fiber: CSVinputObjectType = "Fiber"; break;
			default:
			case csvConfig::CTInputObjectType::Voids: CSVinputObjectType = "Pores"; break;
		}
		//setting values to variant
		this->m_regEntries->v_FiberPoreObject = CSVinputObjectType;
		this->m_regEntries->v_colSeparator.setValue(colSeparator);
		this->m_regEntries->v_startLine.setValue(csv_params.skipLinesStart);
		this->m_regEntries->v_endLine.setValue(csv_params.skipLinesEnd);
		this->m_regEntries->v_languageFormat.setValue(useEN_Decimals);

		//spacing + units
		this->m_regEntries->v_Spacing = csv_params.spacing;
		this->m_regEntries->v_Units = csv_params.csv_units;
		//this->m_regEntries->v_FiberPoreObject = csv_params.inputObjectType TODO save fiber pores?
		this->m_regEntries->v_fileName = csv_params.fileName;

		//saveValues in registry;
		settingsName = this->m_regEntries->str_settingsName + "/" + this->m_regEntries->str_formatName + "/" + LayoutName;
		settings.beginGroup(settingsName);
		settings.setValue(this->m_regEntries->str_reg_colSeparator, this->m_regEntries->v_colSeparator); // colSeparator
		settings.setValue(this->m_regEntries->str_reg_startLine, this->m_regEntries->v_startLine); //startLine
		settings.setValue(this->m_regEntries->str_reg_useEndline, this->m_regEntries->v_useEndline);//useEndline
		settings.setValue(this->m_regEntries->str_reg_EndLine, this->m_regEntries->v_endLine);//EndLine
		settings.setValue(this->m_regEntries->str_reg_languageFormat, this->m_regEntries->v_languageFormat);//LanguageFormat
		settings.setValue(this->m_regEntries->str_reg_Spacing, this->m_regEntries->v_Spacing); //Spacing
		settings.setValue(this->m_regEntries->str_reg_Units, this->m_regEntries->v_Units); //Units;
		settings.setValue(this->m_regEntries->str_fileName, this->m_regEntries->v_fileName); //FileName;
		settings.setValue(this->m_regEntries->str_reg_FiberPoreData, this->m_regEntries->v_FiberPoreObject); //Fiber or Pores as csv Input
		settings.endGroup();
	}
}

//save single setting
void dlg_CSVInput::saveSettings(QSettings &anySetting, const QString &LayoutName, const QString &FeatureName, const QVariant &feat_value)
{
	QString fullSettingsName = "";
	createSettingsName(fullSettingsName, LayoutName, FeatureName, true);
	anySetting.setValue(fullSettingsName, feat_value);
}

void dlg_CSVInput::createSettingsName(QString &fullSettingsName, const QString & LayoutName, const QString & FeatureName, bool useSubGroup)
{
	QString myFeature = "";
	if (useSubGroup)
	{
		myFeature = FeatureName;
	}
	fullSettingsName = this->m_regEntries->str_settingsName + "/" + this->m_regEntries->str_formatName + "/" + LayoutName + "/" + myFeature;
}
