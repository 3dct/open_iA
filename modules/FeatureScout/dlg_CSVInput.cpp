#include "dlg_CSVInput.h"
#include "qfiledialog.h"
#include "qmessagebox.h"
#include "qstandarditemmodel.h"
#include <QSettings>

dlg_CSVInput::dlg_CSVInput(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/) : QDialog(parent, f) {

	this->setupUi(this);
	this->initParameters();
	this->myLayout->addWidget(this->m_entriesPreviewTable);
	this->m_confParams = QSharedPointer<csvConfig::configPararams>(new csvConfig::configPararams());
	
	disableFormatComponents();
	//hideCoordinateInputs();
	connectSignals();

}

void dlg_CSVInput::hideCoordinateInputs()
{
	this->ed_XStartCol->setVisible(false);
	this->ed_X_EndCol->setVisible(false);

	this->ed_yStart_Col->setVisible(false);
	this->ed_X_EndCol->setVisible(false);
	
}

void dlg_CSVInput::disableFormatComponents()
{
	this->ed_startLine->setVisible(true);
	this->lbl_startLine->setVisible(true);
	this->ed_startLine->setEnabled(false);
	this->ed_endLIne->setEnabled(false);
	this->lbl_endLine->setVisible(true);
	this->ed_decimal->setVisible(false);
	this->lbl_decimal->setVisible(false);
	this->buttonBox->setVisible(false);
	this->buttonBox->setDisabled(true);
}

dlg_CSVInput::~dlg_CSVInput()
{
	if (this->m_DataTableSelected) {
		delete this->m_DataTableSelected;
		this->m_DataTableSelected = nullptr;
	}
}

void dlg_CSVInput::connectSignals()
{
	connect(btn_loadCSV, SIGNAL(clicked()), this, SLOT(FileBtnClicked()));
	//connect(btn_LoadConfig, SIGNAL(clicked()), this, SLOT(LoadFormatBtnClicked()));
	connect(btn_CustomFormat, SIGNAL(clicked()), this, SLOT(CustomFormatBtnClicked()));
	connect(btn_loadCols, SIGNAL(clicked()), this, SLOT(LoadColsBtnClicked()));
	connect(btn_SaveLayout, SIGNAL(clicked()), this, SLOT(SaveLayoutBtnClicked())); 
	connect(cmb_box_FileFormat, &QComboBox::currentTextChanged, this, &dlg_CSVInput::LoadFormatSettings);
	
	//connect(cmb_box_FileFormat, SIGNAL(currentTextChanged(const QString&)), this, SLOT(LoadFormatSettings(QString)));
}

//enabling for custom file format
void dlg_CSVInput::CustomFormatBtnClicked(){
	bool enabled = true; 
	showFormatComponents();
	this->useCustomformat = true; 
}

void dlg_CSVInput::showFormatComponents()
{
	this->groupBox_Config->setEnabled(true);
	this->ed_startLine->setVisible(true);
	this->ed_startLine->setEnabled(true);
	this->lbl_endLine->setVisible(true);
	this->cb_applyEndLine->setEnabled(true);
	this->cmb_box_separator->setEnabled(true);
	this->cmb_box_FileFormat->setEnabled(false);
	this->cb_fmtEnglish->setEnabled(true); 
	this->btn_SaveLayout->setEnabled(true);
	this->ed_endLIne->setEnabled(true);
	this->ed_CSVFormat_Name->setEnabled(true); 
}

void dlg_CSVInput::LoadFormatSettings(const QString &LayoutName)
{
	bool layoutAvaiable = false; 
	if (LayoutName.isEmpty()) { return;}
	QSettings mySettings; 
	layoutAvaiable = CheckFeatureInRegistry(mySettings, &LayoutName);
	if (!layoutAvaiable) {
		QMessageBox::warning(this,tr("FeatureScoutCSV"), tr("Layout option not yet defined"));
		return; 
	}

	this->loadEntriesFromRegistry(mySettings, LayoutName);
	
}

void dlg_CSVInput::LoadColsBtnClicked()
{
	this->setSelectedEntries();
	this->buttonBox->setEnabled(true);
	this->buttonBox->setVisible(true);
}

void dlg_CSVInput::SaveLayoutBtnClicked()
{
	csvConfig::configPararams params;
	QString layoutName = this->ed_CSVFormat_Name->text(); 
	if (layoutName.trimmed().isEmpty()) {

		QMessageBox::warning(this, tr("featureScoutCSV"),
			tr("Please enter layout name"));
		return; 
	}

	this->assignSeparator(); 
	this->assignStartEndLine(); 
	this->AssignFormatLanguage();
	params = *this->m_confParams; 
	saveParamsToRegistry(params, layoutName);
}

void dlg_CSVInput::FileBtnClicked()
{
	
	this->assignFileFormat(); 
	this->assignSeparator();
	this->AssignFormatLanguage(); 
	
	if (this->useCustomformat) {
		this->assignStartEndLine(); 
	}

	this->loadFilePreview(15); 
	this->showConfigParams(*this->m_confParams);
}

void dlg_CSVInput::AssignFormatLanguage() {
	if (cb_fmtEnglish->isChecked()) {
		this->m_confParams->csv_Inputlanguage = csvConfig::inputLang::EN;
	}else this->m_confParams->csv_Inputlanguage = csvConfig::inputLang::GER;
}

const csvConfig::configPararams& dlg_CSVInput::getConfigParameters() const {
	return *this->m_confParams; 
}


//shows configuration parameters to gui
void dlg_CSVInput::showConfigParams(const csvConfig::configPararams & params)
{
	this->ed_startLine->setText(QString("%1").arg(params.startLine));
	this->cmb_box_separator->setCurrentIndex(0);
	//this->ed_language->setText("EN");
	this->ed_Spacing->setText(QString("%1").arg(params.spacing));
	this->ed_Units->setText(QString("1").arg(params.csv_units)); 
}

//void dlg_CSVInput::LoadFormatBtnClicked()
//{
//	
//}

void dlg_CSVInput::initParameters(){
	
	if (!this->m_confParams)
		this->m_confParams = QSharedPointer<csvConfig::configPararams>(new csvConfig::configPararams());

	if(!this->m_regEntries){
		this->m_regEntries = cvsRegSettings_ShrdPtr(new csv_reg());
	}

	this->useCustomformat = false; 
	initBasicFormatParameters(csvLang::EN, csvColSeparator::Colunm, csvFormat::Default);
	initStartEndline(5, 0, false);
	this->m_confParams->spacing = 10.5f;
	this->m_confParams->csv_units = "microns";
	this->m_confParams->paramsValid = false;
	this->m_fPath = "D:/OpenIa_TestDaten/Pores/";
	this->m_entriesPreviewTable = new dataTable(); 
	
	if (!this->m_currentHeaders) {
		this->m_currentHeaders = QSharedPointer<QStringList>(new QStringList()); 
	}

	if (!this->m_DataTableSelected) {
		this->m_DataTableSelected = new dataTable();
	}

	if (!this->m_selHeaders) {
		this->m_selHeaders = QSharedPointer < QStringList>(new QStringList());
	}

	this->ed_CSVFormat_Name->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9_]{0,255}"), this));
	this->m_regEntries->initParam(); 
	
}

void dlg_CSVInput::initBasicFormatParameters(csvLang Language, csvColSeparator FileSeparator, csvFormat FileFormat)
{
	this->m_confParams->file_seperator = FileSeparator;
	this->m_confParams->csv_Inputlanguage = Language;
	this->m_confParams->file_fmt = FileFormat;
}

void dlg_CSVInput::initStartEndline(unsigned long startLine, unsigned long EndLine, const bool useEndline)
{
	this->m_confParams->startLine = startLine; 
	this->m_confParams->headerStartLine = startLine;
	if (useEndline) {
		//starts with 0 
		this->m_confParams->endLine = EndLine - 1;
		this->m_confParams->useEndline = useEndline; 
	}
}

//todo validate data format
void dlg_CSVInput::assignStartEndLine() {
	QString startLine = "";
	QString endLine = "";
	bool skipEndline = this->cb_applyEndLine->isChecked();
	startLine = this->ed_startLine->text();
	endLine = this->ed_endLIne->text();
	this->initStartEndline(startLine.toLong(), endLine.toLong(), skipEndline);
}

void dlg_CSVInput::resetDefault()
{
	this->initParameters(); 
}


void dlg_CSVInput::setError(const QString &ParamName,const QString & Param_value )
{
	this->m_Error_Parameter.append("Error" + ParamName + "\t" + Param_value + "\n");
	this->m_confParams->paramsValid = false; 
}


void dlg_CSVInput::assignFileFormat() 
{
	//read out text of combo box
	QString CSVFileFormat = this->cmb_box_FileFormat->currentText();

	if (CSVFileFormat == "VolumeGraphics") {
		this->m_csvFileFormat = csvConfig::csv_FileFormat::VolumeGraphics;


	}
	else if (CSVFileFormat == "MAVI") {
		this->m_csvFileFormat = csvConfig::csv_FileFormat::MAVI;

	}
	else if (CSVFileFormat == "open_iA") {
		this->m_csvFileFormat = csvConfig::csv_FileFormat::open_IA_FeatureScout;
	}
	else {
		this->m_csvFileFormat = csvConfig::csv_FileFormat::Default;
	}
	
}

void dlg_CSVInput::assignSeparator() {
	bool param_seperator_ok = false;
	QString tmp_seperator = this->cmb_box_separator->currentText();
	

	if (tmp_seperator.contains(";"))
	{
		this->m_confParams->file_seperator = csvColSeparator::Colunm;
		param_seperator_ok = true; 
	
	}else if (tmp_seperator.contains(",")) {
		this->m_confParams->file_seperator = csvColSeparator::Comma;
		param_seperator_ok = true;
	}
	else {

		this->setError("Separator", tmp_seperator);
	}

	this->m_confParams->paramsValid = param_seperator_ok; 
	
	
}

void dlg_CSVInput::loadFilePreview(const int rowCount) {
	m_entriesPreviewTable->setColSeparator(this->m_confParams->file_seperator);
	if (!isFileNameValid) {
		
		isFileNameValid = this->checkFile();
		if (!isFileNameValid)
		{
			return;
		}
	}
	
	this->m_entriesPreviewTable->prepareTable(rowCount, this->m_confParams->colCount, this->m_confParams->headerStartLine); 
	this->loadEntries(this->m_confParams->fileName, rowCount);
	this->showPreviewTable(); 
}


//checks if file exists and save it to config params
bool dlg_CSVInput::checkFile() {
	bool fileOK = false; 
	if (m_fPath.isEmpty()) {

		this->m_confParams->paramsValid = false;
		return fileOK;
	}

	QString fileName = QFileDialog::getOpenFileName(
		this, tr("Open Files"), m_fPath, tr("csv spreadsheet (*.csv),.csv")
	);

	if (fileName.isEmpty())
	{
		this->m_confParams->paramsValid = false;
		return fileOK;
	}
	else {
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::information(this, tr("Unable to open file"),
				file.errorString());
			this->setError(QString("unable to open file"), file.errorString());
			this->m_confParams->paramsValid = false;
			return fileOK;
		}
		else {
			this->m_confParams->paramsValid = (this->m_confParams->paramsValid && true);

			if (!this->m_confParams->paramsValid) {
				QMessageBox::information(this, tr("Wrong parameters assigned:"),
					this->m_Error_Parameter);
			}
			else {
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

//loading entries in table widget preview
bool dlg_CSVInput::loadEntries(const QString& fileName, const unsigned int nrPreviewElements) {
 
	bool dataLoaded = false; 
	uint startElLine = (uint)  this->m_confParams->startLine;

	if (isFilledWithData) { this->m_entriesPreviewTable->clearTable(); }
	
	dataLoaded = this->m_entriesPreviewTable->readTableEntries(fileName, nrPreviewElements, this->m_confParams->colCount,this->m_confParams->headerStartLine, &startElLine, true, false); 
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


//assign headers and prepare map with indexes
void dlg_CSVInput::assignHeaderLine() {
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

//setEntries from a selected List;
void dlg_CSVInput::setSelectedEntries() {
	uint currItemIdx; 
	QString listEntry; 
	this->m_selectedHeadersList = this->textControl_list->selectedItems();
				
	//no selection use all entries
	if (!(this->m_selectedHeadersList.length() == 0)) {
		for (const auto &selEntry : m_selectedHeadersList) {
			listEntry = selEntry->text();
			currItemIdx = this->m_hashEntries.value(listEntry);
			this->m_selColIdx.push_back(currItemIdx);
			this->m_selHeaders->append(listEntry);
			
		}
		
		qSort(this->m_selColIdx.begin(), this->m_selColIdx.end(), qLess<uint>());
	}

}

const QVector<uint>& dlg_CSVInput::getEntriesSelInd()
{
	return this->m_selColIdx;
}


void dlg_CSVInput::saveParamsToRegistry(csvConfig::configPararams& csv_params, const QString &LayoutName ) {
	QSettings settings;
	
	QString colSeparator =""; 
	bool useEN_Decimals = false; 

	if (this->m_regEntries) {
		
		switch (csv_params.file_seperator) {
			
			case(csvColSeparator::Colunm): colSeparator = "Column"; break;
			
			case(csvColSeparator::Comma): colSeparator = "Comma"; break; 
			
			default: colSeparator = "Column"; break; 
		}


		switch(csv_params.csv_Inputlanguage) {

			case(csvLang::EN): useEN_Decimals = true; break;
			
			case(csvLang::GER): useEN_Decimals = false; break; 
			
			default: useEN_Decimals = true; 
		
		}


		//settingvalues to variant
		this->m_regEntries->v_colSeparator.setValue(colSeparator);
		this->m_regEntries->v_startLine.setValue(csv_params.startLine); 
		this->m_regEntries->v_useEndline.setValue(csv_params.useEndline); 
		this->m_regEntries->v_endLine.setValue(csv_params.endLine); 
		this->m_regEntries->v_languageFormat.setValue(useEN_Decimals);

		//saveValue to Registry;

		//colSeparator
		this->saveSettings(settings,LayoutName, this->m_regEntries->str_reg_colSeparator, this->m_regEntries->v_colSeparator);
		
		//startLine
		this->saveSettings(settings,LayoutName, this->m_regEntries->str_reg_startLine, this->m_regEntries->v_startLine);
		//useEndline
		this->saveSettings(settings, LayoutName, this->m_regEntries->str_reg_useEndline, this->m_regEntries->v_useEndline);
		
		//EndLine
		this->saveSettings(settings, LayoutName, this->m_regEntries->str_reg_EndLine, this->m_regEntries->v_endLine);

		//LanguageFormat
		this->saveSettings(settings, LayoutName, this->m_regEntries->str_reg_languageFormat, this->m_regEntries->v_languageFormat);

		
		//just for testing
		loadEntriesFromRegistry(settings, LayoutName);
	}
	
}


//in order to load entries from registry
void dlg_CSVInput::loadEntriesFromRegistry(QSettings &anySetting, const QString &LayoutName) {
	QString f_separator = "";
	bool useEN_DecimalPoint = false;
	QString fullName = ""; 

	this->m_confParams->resetParams(); 

	//loadEntriesList(anySetting);

	this->createSettingsName(fullName, LayoutName, this->m_regEntries->str_reg_startLine); 
	this->m_confParams->startLine = anySetting.value(fullName, this->m_regEntries->str_reg_startLine).toLongLong();

	//useEndline
	this->createSettingsName(fullName, LayoutName, this->m_regEntries->str_reg_useEndline);
	this->m_confParams->useEndline = anySetting.value(fullName, this->m_regEntries->str_reg_useEndline).toBool();
	
	//endLine
	this->createSettingsName(fullName, LayoutName, this->m_regEntries->str_reg_EndLine);
	this->m_confParams->endLine = anySetting.value(fullName, this->m_regEntries->str_reg_EndLine).toLongLong(); 
	
	//file separator
	this->createSettingsName(fullName, LayoutName, this->m_regEntries->str_reg_colSeparator);
	f_separator = anySetting.value(fullName, this->m_regEntries->str_reg_colSeparator).toString();

	if (f_separator.contains("Comma")) {
		this->m_confParams->file_seperator = csvColSeparator::Comma;

	}
	else {
		if (f_separator.contains("Column")) {
			this->m_confParams->file_seperator = csvColSeparator::Colunm; 
		}

		//add more if neccessary
	}

	//inputlang - decimalPoint
	this->createSettingsName(fullName, LayoutName, this->m_regEntries->str_reg_languageFormat);
	useEN_DecimalPoint = anySetting.value(fullName, this->m_regEntries->str_reg_languageFormat).toBool();

	if (useEN_DecimalPoint) {
		this->m_confParams->csv_Inputlanguage = csvLang::EN; 
	}
	else {
		this->m_confParams->csv_Inputlanguage = csvLang::GER;
	}

	
}


//loads entries with layout Name or list all entries under FeaturescoutCSV
bool dlg_CSVInput::CheckFeatureInRegistry(QSettings & anySetting, const QString *LayoutName)
{
	QString Layout = "";
	if (LayoutName) {
		Layout = *LayoutName;
		Layout += "/";

	}

	QString myName = this->m_regEntries->str_settingsName + "/" + this->m_regEntries->str_formatName + "/" + Layout;
	anySetting.beginGroup(/*this->m_regEntries->*/myName);
	QStringList groups = anySetting.childGroups();
	
	if (groups.isEmpty()) {
		return false; 
	}
	else return true; 
}

//save single setting
void dlg_CSVInput::saveSettings(QSettings &anySetting, const QString &LayoutName, const QString &FeatureName, const QVariant &feat_value)
{
	QString fullSettingsName = "";
	createSettingsName(fullSettingsName, LayoutName, FeatureName);
	anySetting.setValue(fullSettingsName, feat_value); 
}

void dlg_CSVInput::createSettingsName(QString &fullSettingsName, const QString & LayoutName, const QString & FeatureName)
{
	fullSettingsName = this->m_regEntries->str_settingsName + "/" + this->m_regEntries->str_formatName + "/" + LayoutName + "/" + FeatureName;
}

