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
	
	
	

	
	
	
	const void getConfigParameters(csvConfig::configPararams &params_out) const;
	
	 
	
private slots: 
	void FileBtnClicked(); 



private: 
	void initParameters();
	void resetDefault(); 

	bool validateParameters();
	
	void setError(const QString &ParamName, const QString & Param_value);
	
	
	QString m_fPath; 
	QString m_colSeparator;


	QString m_Error_Parameter; 
	//QFile
	bool m_VG_File_Selected;
	bool m_fmt_Engl;
	unsigned long m_startLine; 
	bool m_paramsValid; 
	csvConfig::csv_FileFormat m_csvFileFormat;
	//vtkSmartPointer<vtkTable> ioTable; 
	
	//QSharedPointer<iACsvIO> FileIO; 

};
