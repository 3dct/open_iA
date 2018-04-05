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
		typedef csvConfig::csvSeparator colSeparator; 
		typedef csvConfig::inputLang csvLang; 
		typedef csvConfig::csv_FileFormat csvFormat; 
		

public:
	dlg_CSVInput(QWidget * parent = 0, Qt::WindowFlags f = 0); /*: QDialog(parent, f)*/
	
	~dlg_CSVInput();

	
	void AssignFormatLanguage();

	const csvConfig::configPararams & getConfigParameters() const;
	void showConfigParams(const csvConfig::configPararams &params);
	inline void setFilePath(const QString& FPath) {
			if(!FPath.isEmpty()) {
			this->m_fPath = FPath; 
		}
	}
	
	void setSelectedEntries();
	const QVector<uint>& getEntriesSelInd(); 

	inline const QSharedPointer<QStringList> getHeaders() {
		return this->m_selHeaders;
	};

	inline const uint getTableWidth() {
		return this->m_confParams->tableWidth; 
	}

	
private slots: 
	void FileBtnClicked(); 
	void LoadFormatBtnClicked();

	//custom file format 
	void CustomFormatBtnClicked();
	void showFormatComponents();

	void LoadColsBtnClicked(); 


private: 

	//pointer initialization
	void initParameters();
	void initBasicFormatParameters(csvLang Language, colSeparator FileSeparator, csvFormat FileFormat);
	void initStartEndline(unsigned long startLine, unsigned long EndLine, const bool useEndline); 

	void resetDefault();
	void assignStartEndLine();

	void connectSignals();


	//bool validateParameters();
	void setError(const QString &ParamName, const QString & Param_value);
	void assignFileFormat();
	void assignSeparator();
	void loadFilePreview(const int rowCount);
	bool checkFile();
	bool loadEntries(const QString & fileName, const unsigned int nrPreviewElements);
	void showPreviewTable();
	void assignHeaderLine();
	
	
	void readHeaderLine(const uint headerStartRow); 

	void hideCoordinateInputs();
	void disableFormatComponents();

private:
	bool useCustomformat; 

	QSharedPointer<csvConfig::configPararams> m_confParams; 
	QString m_fPath; 
	QString m_Error_Parameter; 
	csvConfig::csv_FileFormat m_csvFileFormat;
	csvTable_ptr m_entriesPreviewTable = nullptr; 
	csvTable_ptr m_DataTableSelected = nullptr; 

	bool isFileNameValid = false; 
	bool isFilledWithData = false; 


	//current headers of the table
	QSharedPointer<QStringList> m_currentHeaders; 
	QSharedPointer<QStringList> m_selHeaders = nullptr; 
	QList<QListWidgetItem*> m_selectedHeadersList; 

	QHash<QString, uint> m_hashEntries;
	QVector<uint> m_selColIdx;
	

};
