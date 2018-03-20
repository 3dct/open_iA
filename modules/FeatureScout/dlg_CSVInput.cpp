#include "dlg_CSVInput.h"
#include "qfiledialog.h"
#include "qmessagebox.h"

dlg_CSVInput::dlg_CSVInput(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/) : QDialog(parent, f) {

	this->setupUi(this);
	this->initParameters();
	this->buttonBox->setVisible(false);
	this->buttonBox->setDisabled(true);
	this->m_confParams = QSharedPointer<csvConfig::configPararams>(new csvConfig::configPararams());
	
	connect(btn_loadCSV, SIGNAL(clicked()), this, SLOT(FileBtnClicked()));
	connect(btn_LoadConfig, SIGNAL(clicked()), this, SLOT(LoadFormatBtnClicked())); 


}


void dlg_CSVInput::FileBtnClicked()
{
	
	/*bool param_HeaderLines_ok = false;*/
	

	//read out text of combo box
	
	this->assignFileFormat(); 
	this->assignSeparator();

	if (cb_fmtEnglish->isChecked()) {
		this->m_confParams->csv_Inputlanguage = csvConfig::inputLang::EN; 
	
	}else this->m_confParams->csv_Inputlanguage = csvConfig::inputLang::GER;
	
	checkFileExist(); 
	this->showConfigParams(*this->m_confParams);

	this->buttonBox->setEnabled(true);
	this->buttonBox->setVisible(true);

}


const csvConfig::configPararams& dlg_CSVInput::getConfigParameters() const {
	return *this->m_confParams; 
}


//shows configuration parameters to gui
void dlg_CSVInput::showConfigParams(const csvConfig::configPararams & params)
{
	this->ed_startLine->setText(QString("%1").arg(params.endLine));
	this->ed_endLine->setText(QString("%1").arg(params.startLine));
	this->cmb_box_separator->setCurrentIndex(0);
	this->ed_language->setText("EN");
	this->ed_Spacing->setText(QString("%1").arg(params.spacing));
	this->ed_Units->setText(QString("1").arg(params.csv_units)); 
}

void dlg_CSVInput::LoadFormatBtnClicked()
{
}

void dlg_CSVInput::initParameters(){
	
	if (!m_confParams)
		this->m_confParams = QSharedPointer<csvConfig::configPararams>(new csvConfig::configPararams());


	this->m_confParams->file_seperator = csvConfig::csvSeparator::Colunm;
	this->m_confParams->csv_Inputlanguage = csvConfig::inputLang::EN;
	this->m_confParams->file_fmt = csvConfig::csv_FileFormat::Default; 
	this->m_confParams->startLine = 1;
	this->m_confParams->endLine = 0; 
	this->m_confParams->spacing = 10.5f;
	this->m_confParams->csv_units = "microns";
	this->m_confParams->paramsValid = false;
	


}

void dlg_CSVInput::resetDefault()
{
	this->initParameters(); 
	/*this->m_startLine = 1;
	this->m_colSeparator = ";";*/
	
}

bool dlg_CSVInput::validateParameters()
{

	return true; 
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
		this->m_confParams->file_seperator = csvConfig::csvSeparator::Colunm;
		param_seperator_ok = true; 
	
	}else if (tmp_seperator.contains(",")) {
		this->m_confParams->file_seperator = csvConfig::csvSeparator::Comma;
		param_seperator_ok = true;
	}
	else {

		this->setError("Separator", tmp_seperator);
	}

	this->m_confParams->paramsValid = param_seperator_ok; 
	
	
}

void dlg_CSVInput::checkFileExist() {
	QString fileName = QFileDialog::getOpenFileName(
		this, tr("Open Files"), "D:/OpenIa_TestDaten/Pores/", tr("csv spreadsheet (*.csv),.csv")
	);

	if (fileName.isEmpty())
	{
		this->m_confParams->paramsValid= false;

		return;
	}
	else {
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::information(this, tr("Unable to open file"),
				file.errorString());
			this->setError(QString("unable to open file"), file.errorString());
			this->m_confParams->paramsValid = false;
			return;
		}
		else {
			this->m_confParams->paramsValid = (this->m_confParams->paramsValid && true);

			if (!this->m_confParams->paramsValid) {
				QMessageBox::information(this, tr("Wrong parameters assigned:"),
					this->m_Error_Parameter);
			}
			else {
				
				this->m_confParams->fileName = fileName; 
				/*this->m_fPath = fileName;*/
				this->btn_loadCSV->setEnabled(false);
				this->btn_loadCSV->setVisible(false);
			}
		}

	}
}