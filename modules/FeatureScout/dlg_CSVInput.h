/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#pragma once

#include "ui_CsvInput.h"
#include "iACsvConfig.h"
#include "FeatureScout_export.h"

class iACsvIO;

class QSettings;

//! Loads custom csv file with data preview
//! Settings can be adapted and saved under a specified format name
class FeatureScout_API dlg_CSVInput : public QDialog, public Ui_CsvInput
{
Q_OBJECT
public:
	//! Create a new dialog, all parameters are optional
	dlg_CSVInput(bool volumeDataAvailable, QWidget * parent = 0, Qt::WindowFlags f = 0);
	//! Set the internal path (used when choosing a csv file) to the one given as parameter
	void setPath(QString const & path);
	//! Set the file name input to the one given as parameter
	void setFileName(QString const & fileName);
	//! Set the format to be used for loading the file
	void setFormat(QString const & formatName);
	//! Retrieve the configuration currently set in the dialog
	const iACsvConfig & getConfig() const;
private slots:
	//! On button click for selecting CSV file 
	void selectFileBtnClicked();
	//! On button click for selecting curved CSV file 
	void selectCurvedFileBtnClicked();
	//! when checkbox for curved fiber info
	void curvedFiberInfoChanged();
	//! handles a click on the OK button
	void okBtnClicked();
	//! load format based on selected input format (ex. mavi/ vg, ...)
	void loadSelectedFormatSettings(const QString &formatName);
	//! show file preview (triggered on any update to the format settings and when Update Preview button is clicked)
	void updatePreview();
	//! exports complete preview table
	void exportTable();
	//! switch Object Type Fiber / Pores
	void switchObjectType(const QString &inputType);
	//! Add format to the list of known formats
	void saveFormatBtnClicked();
	//! Delete format from the list of known formats
	void deleteFormatBtnClicked();
	//! Apply default format selection
	void applyFormatColumnSelection();
	//! switch the inputs for the column mappings depending on what data is available
	void updateColumnMappingInputs();
	//! called when selected columns change
	void selectedColsChanged();
	//! called when Compute Length checkbox check state changed
	void computeLengthChanged();
	//! called when Compute Angles checkbox check state changed
	void computeAngleChanged();
	//! called when Compute Center checkbox check state changed
	void computeCenterChanged();
	//! called when Compute Start/End checkbox check state changed
	void computeStartEndChanged();
	//! called when Fixed Diameter checkbox state changed
	void fixedDiameterChanged();
	//! called when the Advanced Mode checkbox is checked or unchecked
	void advancedModeToggled();
	//! called when the export button is clicked
	void exportButtonClicked();
	//! called when the export button is clicked
	void importButtonClicked();
	//! called when different visualization type was selected
	void visualizationTypeChanged(int);
private:
	//! switch length mapping choice enabled based on whether to checkbox to automatically compute it is checked or not
	void updateLengthEditEnabled();
	//! switch angle mapping choices enabled based on whether to checkbox to automatically compute it is checked or not
	void updateAngleEditEnabled();
	//! switch center mapping choices enabled if start/end is selected and compute center is not checked
	void updateCenterEditEnabled();
	//! switch diameter choices and fixed diameter spin box enabled based on whether "add fixed diameter" is checked or not
	void updateDiameterInputEnabled();
	//! initialize GUI elements
	void initParameters();
	//! connect signals and slots of all dialog controls
	void connectSignals();
	//! Load a general setting, such as the name of format loaded last time, or whether advanced mode was shown
	QVariant loadGeneralSetting(QString const & settingName) const;
	//! Save a general setting, such as the name of format to be loaded next time dialog is opened or whether advanced mode is shown
	void saveGeneralSetting(QString const & settinName, QVariant value);
	//! Save the currently configured format in registry
	void saveFormatToRegistry(const QString & formatName);
	//! Loads settings from registry for a given format name, into default config object
	bool loadFormatFromRegistry(const QString & formatName);
	//! Loads settings from registry for a given format name, into a given config object
	bool loadFormatFromRegistry(const QString & formatName, iACsvConfig & dest);
	//! Deletes a format from the registry
	void deleteFormatFromReg(QString const & formatName);
	//! Shows configuration parameters in GUI
	void showConfigParams();
	//! Show selected columns from parameters in GUI
	void showSelectedCols();
	//! @{ assign methods take data from GUI and assign it to config object
	//! Assign all format settings from the GUI to the internal configuration object
	//! (except for header lines)
	void assignFormatSettings();
	//! Assign the input object type from the GUI to the internal configuration object
	void assignObjectTypes();
	//! Set entries from a selected List + setting column count information for selection
	void assignSelectedCols();
	//! @}
	//! Show column header in list widget, and prepare map with indexes
	void showColumnHeaders();
	//! Load file into preview table widget
	bool loadFilePreview();
	//! Clears the preview table (so that it doesn't contain any rows or columns
	void clearPreviewTable();

	//! Catches events from list view to be able  to react on delete key event
	bool eventFilter(QObject *obj, QEvent *event) override;

	iACsvConfig m_confParams;
	QString m_path;
	QVector<QComboBox*> m_mappingBoxes;
	bool m_columnMappingChoiceSet; //!< whether we have provided proper choices in the column mapping comboboxes already
	bool m_volumeDataAvailable;    //!< whether a volume dataset was available before opening the dialog
};
