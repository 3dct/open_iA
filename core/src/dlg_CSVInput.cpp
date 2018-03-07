#include "dlg_CSVInput.h"
#include "qfiledialog.h"
#include "qmessagebox.h"

dlg_CSVInput::dlg_CSVInput(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/) : QDialog(parent, f) {

	this->setupUi(this);
	this->initParameters();
	this->buttonBox->setVisible(false);
	this->buttonBox->setDisabled(true);

	//connect the component btn load csv with clicked and Action FileBtnClicked, immer da wo es ausgerufen werden soll 
	connect(btn_loadCSV, SIGNAL(clicked()), this, SLOT(FileBtnClicked()));


}


void dlg_CSVInput::FileBtnClicked()
{
	
	/*bool param_HeaderLines_ok = false;*/
	bool param_seperator_ok = false; 

	//read out text
	QString CSVFileFormat = this->cmb_box_FileFormat->currentText();
	
	if (CSVFileFormat ==  "VolumeGraphics") {
		this->m_csvFileFormat = csvConfig::csv_FileFormat::VolumeGraphics;


	}else if (CSVFileFormat == "MAVI") {
		this->m_csvFileFormat = csvConfig::csv_FileFormat::MAVI;

	}
	else if (CSVFileFormat == "open_iA") {
		this->m_csvFileFormat = csvConfig::csv_FileFormat::open_IA_FeatureScout;
	}
	else {
		this->m_csvFileFormat = csvConfig::csv_FileFormat::Default;
	}




	


	QString tmp_seperator = this->cmb_box_separator->currentText(); ///*this->cmb_box */this->ed_CSV_Separator->text();
	//TODO either remove this is wrong!!
	if ((tmp_seperator.contains(";") ) || tmp_seperator.contains(",")) {

		this->m_colSeparator = tmp_seperator; 
		param_seperator_ok = true; 
	}
	else {
		
		this->setError("Separator", tmp_seperator); 
	}

	
	
	
	
	this->m_fmt_Engl = this->cb_fmtEnglish->isChecked();
	

	this->m_paramsValid =/* param_HeaderLines_ok &&*/ param_seperator_ok; 

	if (!this->m_paramsValid) {
		
		this->resetDefault(); 
		QMessageBox::information(this, tr("Wrong parameters assigned:"),
			this->m_Error_Parameter);
		
	}

	//load file with predefined path
	QString fileName = QFileDialog::getOpenFileName(
		this, tr("Open Files"), "D:/OpenIa_TestDaten/Pores/", tr("csv spreadsheet (*.csv),.csv")
	);

	
	if (fileName.isEmpty())
	{
		this->m_paramsValid = false;

		return;
	}
	else {
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::information(this, tr("Unable to open file"),
				file.errorString());
			this->setError(QString("unable to open file"), file.errorString()); 
			this->m_paramsValid = false; 
			return;
		}
		else {
			this->m_paramsValid = (this->m_paramsValid && true);
			
			if (!this->m_paramsValid) {
				QMessageBox::information(this, tr("Wrong parameters assigned:"),
					this->m_Error_Parameter);
			}
			else {
				this->m_fPath = fileName;
				this->btn_loadCSV->setEnabled(false);
				this->btn_loadCSV->setVisible(false);
			}
		}



		file.close();
		
		
	}

	this->buttonBox->setEnabled(true);
	this->buttonBox->setVisible(true);

}

const void dlg_CSVInput::getConfigParameters(csvConfig::configPararams & params_out) const
{
	params_out.colSeparator = this->m_colSeparator; 
	params_out.fileName = this->m_fPath;
	params_out.startLine = this->m_startLine;
	
	params_out.fmt_ENG = this->m_fmt_Engl;
	params_out.paramsValid = this->m_paramsValid;
	params_out.file_fmt = this->m_csvFileFormat;
}

void dlg_CSVInput::initParameters(){
	this->m_colSeparator = ";";
	this->m_fmt_Engl = false;
	this->m_VG_File_Selected = false;
	this->m_paramsValid = false; 
	this->m_startLine = 1;
	this->m_csvFileFormat = csvConfig::csv_FileFormat::Default; 


}

void dlg_CSVInput::resetDefault()
{
	this->m_startLine = 1;
	this->m_colSeparator = ";";
	
}

bool dlg_CSVInput::validateParameters()
{

	return true; 
}

void dlg_CSVInput::setError(const QString &ParamName,const QString & Param_value )
{
	
	this->m_Error_Parameter.append("Error" + ParamName + "\t" + Param_value + "\n");
	this->m_paramsValid = false; 
}

