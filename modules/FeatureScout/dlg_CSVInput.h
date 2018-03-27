#pragma once

#include "ui_CsvInput.h"
#include "io/csv_config.h"
#include "io/DataTable.h"

#include "vtkTable.h"
#include <vtkSmartPointer.h>



class /*open_iA_Core_API*/ dlg_CSVInput : public QDialog, public Ui_CsvInput
{
	Q_OBJECT
		typedef DataIO::DataTable dataTable; 
		typedef dataTable* csvTable_ptr; 

public:
	dlg_CSVInput(QWidget * parent = 0, Qt::WindowFlags f = 0); /*: QDialog(parent, f)*/
	~dlg_CSVInput();

	void connectSignals();
	
	void AssignFormatLanguage();

	const csvConfig::configPararams & getConfigParameters() const;
	void showConfigParams(const csvConfig::configPararams &params);
	inline void setFilePath(const QString& FPath) {
			if(!FPath.isEmpty()) {
			this->m_fPath = FPath; 
		}
	}
	
	const QVector<uint> &getSelectedEntries();
	inline const QSharedPointer<QStringList> getHeaders() {
		return this->m_currentHeaders; 
	};

	
private slots: 
	void FileBtnClicked(); 
	void LoadFormatBtnClicked(); 
	void CustomFormatBtnClicked(); 
	void LoadColsBtnClicked(); 


private: 

	//pointer initialization
	void initParameters();
	void resetDefault(); 

	bool validateParameters();
	void setError(const QString &ParamName, const QString & Param_value);
	void assignFileFormat();
	void assignSeparator();
	void loadFilePreview(const int rowCount);
	bool checkFile();
	bool loadEntries(const QString & fileName, const unsigned int nrPreviewElements);
	void showPreviewTable();
	void assignHeaderLine();
	
	
	void readHeaderLine(const uint headerStartRow); 

private:


	QSharedPointer<csvConfig::configPararams> m_confParams; 
	QString m_fPath; 
	QString m_Error_Parameter; 
	csvConfig::csv_FileFormat m_csvFileFormat;
	csvTable_ptr m_entriesPreviewTable = nullptr; 

	csvTable_ptr m_DataTableSelected = nullptr; 


	//current headers of the table
	QSharedPointer<QStringList> m_currentHeaders; 
	QSharedPointer<QStringList> m_selHeaders = nullptr; 
	QList<QListWidgetItem*> m_selectedHeadersList; 

	QHash<QString, uint> m_hashEntries;
	QVector<uint> m_selColIdx;
	

};
