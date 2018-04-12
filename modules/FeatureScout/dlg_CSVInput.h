#pragma once

#include "ui_CsvInput.h"
#include "io/csv_config.h"
#include "io/DataTable.h"

#include <QMetaType>

#include "vtkTable.h"
#include <vtkSmartPointer.h>

class QSettings; 

namespace FeatureScoutCSV{

	struct csvRegKeys {
		
		csvRegKeys() {
			initParam(); 
		}

		void initParam() {
			str_settingsName = "FeatureScoutCSV";
			str_formatName = "FormatName";
			str_headerName = "HeaderEntries"; 
			str_allHeaders = "AllHeaders"; 
			str_fileName = "FileName";
			str_reg_useEndline = "useEndLine";
			str_reg_startLine = "StartLine";
			str_reg_EndLine = "Endline";
			str_reg_colSeparator = "ColumnSeperator";
			str_reg_languageFormat = "LanguageFormat";
			str_reg_Spacing = "Spacing";
			str_reg_Units = "Microns";
		}

		/*void reset() {
			initParam(); 
		}*/

		
		QVariant v_startLine;
		QVariant v_useEndline;
		QVariant v_endLine;
		QVariant v_colSeparator;
		QVariant v_languageFormat;
		QVariant v_Spacing; 
		QVariant v_Units; 
		QVariant v_FiberPoreObject; //for fibers or pores
		QVariant v_fileName; 
		QVariant v_allHeaders; 


		QString str_settingsName;
		QString str_headerName;
		QString str_allHeaders;
		QString str_fileName; 
		QString str_formatName; 
		QString str_reg_useEndline;
		QString str_reg_startLine;
		QString str_reg_EndLine;
		QString str_reg_colSeparator;
		QString str_reg_languageFormat;
		QString str_reg_Spacing;
		QString str_reg_Units; 

		//for fibers or pores
		QString str_reg_FiberPoreData; 
	};

};

//Q_DECLARE_METATYPE(QStringList);

class /*open_iA_Core_API*/ dlg_CSVInput : public QDialog, public Ui_CsvInput
{
	Q_OBJECT
		typedef DataIO::DataTable dataTable; 
		typedef dataTable* csvTable_ptr; 
		typedef FeatureScoutCSV::csvRegKeys csv_reg;
		typedef QSharedPointer<csv_reg> cvsRegSettings_ShrdPtr; 

		typedef csvConfig::csvSeparator csvColSeparator; 
		typedef csvConfig::inputLang csvLang; 
		typedef csvConfig::csv_FileFormat csvFormat; 
		

		
		

public:
	dlg_CSVInput(QWidget * parent = 0, Qt::WindowFlags f = 0); /*: QDialog(parent, f)*/
	
	//load Headers saved in registry
	void LoadHeaderEntriesFromReg(QStringList &HeaderEntries, const QString &LayoutName);

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
	void addSingleHeaderToList(uint &currItemIdx, QString &listEntry);
	const QVector<uint>& getEntriesSelInd();

	void selectSingleHeader(uint & currItemIdx, QString & listEntry);

	void setSelectedHeaderToTextControl(QStringList & sel_headers);


	
	
	inline const QSharedPointer<QStringList> getHeaders() {
		return this->m_selHeaders;
	};

	inline const ulong getTableWidth() {
		return this->m_confParams->tableWidth; 
	}

	
private slots: 
	void LoadCSVPreviewClicked();

	//TODO TBD
	void ImportRegSettings();

	//custom file format 
	void CustomFormatBtnClicked();
	void showFormatComponents();

	//load format based on selected input format (ex. mavi/ vg, ...) 
	void LoadFormatSettings(const QString &LayoutName);

	void LoadColsBtnClicked(); 
	void SaveLayoutBtnClicked(); 

private: 

	bool CheckFeatureInRegistry(QSettings & anySetting, const QString * LayoutName, QStringList & groups, bool useSubGroup);

	void saveParamsToRegistry(csvConfig::configPararams & csv_params, const QString & LayoutName);
	void loadEntriesFromRegistry(QSettings & anySetting, const QString & LayoutName);

	//load initial settings
	void LoadFormatEntriesOnStartUp();
	
	void saveSettings(QSettings & anySetting, const QString & LayoutName, const QString & FeatureName, const QVariant & feat_value);

	void createSettingsName(QString &fullSettingsName, const QString & LayoutName, const QString & FeatureName, bool useSubGroup);

	//pointer initialization
	//saving headers from registry entry 
	void initParameters();
	void initBasicFormatParameters(csvLang Language, csvColSeparator FileSeparator, csvFormat FileFormat);
	void initStartEndline(unsigned long startLine, unsigned long EndLine, const bool useEndline); 

	void resetDefault();
	void assignStartEndLine();

	void connectSignals();


	//bool validateParameters();
	void setError(const QString &ParamName, const QString & Param_value);
	void assignFileFormat();
	void assignSeparator();
	void assignSpacingUnits();
	void loadFilePreview(const int rowCount, const bool formatLoaded);
	bool checkFile(bool Layoutloaded);
	bool loadEntries(const QString & fileName, const unsigned int nrPreviewElements);
	void showPreviewTable();
	void assignHeaderLine();
	void readHeaderLine(const uint headerStartRow); 

	void hideCoordinateInputs();
	void disableFormatComponents();

	void saveHeaderEntriesToReg(const QStringList & HeaderEntries, const QString & HeaderName, const QString & LayoutName);

	

private:

	bool useCustomformat; 

	QSharedPointer<csvConfig::configPararams> m_confParams; 
	QString m_fPath; 
	QString m_Error_Parameter; 
	csvConfig::csv_FileFormat m_csvFileFormat;
	csvTable_ptr m_entriesPreviewTable = nullptr; 
	//csvTable_ptr m_DataTableSelected = nullptr; 
	cvsRegSettings_ShrdPtr m_regEntries = nullptr;

	bool isFileNameValid = false; 
	bool isFilledWithData = false; 
	bool m_formatSelected = false; 
	ulong m_headersCount; 

	//current headers of the table
	QSharedPointer<QStringList> m_currentHeaders; 
	QSharedPointer<QStringList> m_selHeaders = nullptr; 
	QList<QListWidgetItem*> m_selectedHeadersList; 

	QHash<QString, uint> m_hashEntries;
	QVector<uint> m_selColIdx;
	

};
