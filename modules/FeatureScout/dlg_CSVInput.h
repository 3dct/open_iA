#pragma once

#include "ui_CsvInput.h"
#include "io/csv_config.h"


#include "vtkTable.h"
#include <vtkSmartPointer.h>



class /*open_iA_Core_API*/ dlg_CSVInput : public QDialog, public Ui_CsvInput
{
	Q_OBJECT

public:
	dlg_CSVInput(QWidget * parent = 0, Qt::WindowFlags f = 0); /*: QDialog(parent, f)*/
	
	const csvConfig::configPararams & getConfigParameters() const;
	void showConfigParams(const csvConfig::configPararams &params);
	
	 
	
private slots: 
	void FileBtnClicked(); 
	void LoadFormatBtnClicked(); 



private: 
	void initParameters();
	void resetDefault(); 

	bool validateParameters();
	
	void setError(const QString &ParamName, const QString & Param_value);

	void assignFileFormat();

	void assignSeparator();

	void checkFileExist();
	
private:


	QSharedPointer<csvConfig::configPararams> m_confParams; 
	QString m_fPath; 
	QString m_Error_Parameter; 


	csvConfig::csv_FileFormat m_csvFileFormat;
	

};
