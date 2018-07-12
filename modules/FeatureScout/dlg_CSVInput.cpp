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

#include "iACsvIO.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QSettings>

namespace csvRegKeys
{
	//Names for registry
	static const QString SettingsName = "FeatureScout";
	static const QString FormatName = "CSVFormats";
	static const QString SelectedHeaders = "SelectedHeaders";
	static const QString AllHeaders = "AllHeaders";
	static const QString DefaultFormat = "DefaultFormat";
	static const QString SkipLinesStart = "SkipLinesStart";
	static const QString SkipLinesEnd = "SkipLinesEnd";
	static const QString ColSeparator = "ColumnSeparator";
	static const QString DecimalSeparator = "DecimalSeparator";
	static const QString Spacing = "Spacing";
	static const QString Unit = "Unit";
	static const QString ObjectType = "ObjectType";
	static const QString AddAutoID = "AddAutoID";
	static const QString Encoding = "Encoding";
}
namespace
{
	QStringList const & ColumnSeparators()
	{
		static QStringList colSeparators;
		if (colSeparators.isEmpty())
		{                 // has to match order of the entries in cmbbox_ColSeparator!
			colSeparators << "," << ";" << "\t" ;
		}
		return colSeparators;
	}

	QString getFormatRegistryKey(QString const & formatName)
	{
		return csvRegKeys::SettingsName + "/" + csvRegKeys::FormatName + "/" + formatName;
	}
}

dlg_CSVInput::dlg_CSVInput(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/)
	: QDialog(parent, f),
	io(new iACsvIO())
{
	setupUi(this);
	initParameters();
	connectSignals();

	// combo boxes involved in column mapping (in the same order as in iACsvConfig::MappedColumn):
	mappingBoxes.push_back(cmbbox_col_ID);
	mappingBoxes.push_back(cmbbox_col_PosStartX);
	mappingBoxes.push_back(cmbbox_col_PosStartY);
	mappingBoxes.push_back(cmbbox_col_PosStartZ);
	mappingBoxes.push_back(cmbbox_col_PosEndX);
	mappingBoxes.push_back(cmbbox_col_PosEndY);
	mappingBoxes.push_back(cmbbox_col_PosEndZ);
	mappingBoxes.push_back(cmbbox_col_PosCenterX);
	mappingBoxes.push_back(cmbbox_col_PosCenterY);
	mappingBoxes.push_back(cmbbox_col_PosCenterZ);
	mappingBoxes.push_back(cmbbox_col_Length);
	mappingBoxes.push_back(cmbbox_col_Diameter);
	mappingBoxes.push_back(cmbbox_col_Phi);
	mappingBoxes.push_back(cmbbox_col_Theta);
}

void dlg_CSVInput::initParameters()
{
	ed_FormatName->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9_]{0,30}"), this)); // limit input to format names
	QStringList formatEntries = getFormatListFromRegistry();
	cmbbox_Format->addItems(formatEntries);
	// load default format, and if that fails, load first format if available:
	if (!loadFormatFromRegistry(getDefaultFormat()) && formatEntries.length() > 0)
		loadFormatFromRegistry(formatEntries[0]);
	showConfigParams(m_confParams);
}

void dlg_CSVInput::connectSignals()
{
	connect(btn_SelectFile, &QPushButton::clicked, this, &dlg_CSVInput::selectFileBtnClicked);
	connect(btn_SaveFormat, &QPushButton::clicked, this, &dlg_CSVInput::saveFormatBtnClicked);
	connect(btn_DeleteFormat, &QPushButton::clicked, this, &dlg_CSVInput::deleteFormatBtnClicked);
	connect(btn_UpdatePreview, &QPushButton::clicked, this, &dlg_CSVInput::updatePreview);
	connect(btn_ApplyFormatColumnSelection, &QPushButton::clicked, this, &dlg_CSVInput::applyFormatColumnSelection);
	connect(cmbbox_Format, &QComboBox::currentTextChanged, this, &dlg_CSVInput::loadSelectedFormatSettings);
	connect(cmbbox_ColSeparator, &QComboBox::currentTextChanged, this, &dlg_CSVInput::updatePreview);
	connect(cmbbox_ObjectType, &QComboBox::currentTextChanged, this, &dlg_CSVInput::switchObjectType);
	connect(cmbbox_Encoding, &QComboBox::currentTextChanged, this, &dlg_CSVInput::updatePreview);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &dlg_CSVInput::okBtnClicked);
	connect(ed_SkipLinesStart, SIGNAL(valueChanged(int)), this, SLOT(updatePreview()));
	connect(ed_SkipLinesEnd, SIGNAL(valueChanged(int)), this, SLOT(updatePreview()));
	connect(sb_PreviewLines, SIGNAL(valueChanged(int)), this, SLOT(updatePreview()));
	connect(cmbbox_col_Selection, &QComboBox::currentTextChanged, this, &dlg_CSVInput::updateColumnMappingInputs);
	connect(cb_ComputeLength, &QCheckBox::stateChanged, this, &dlg_CSVInput::updateLengthEditEnabled);
	connect(cb_ComputeAngles, &QCheckBox::stateChanged, this, &dlg_CSVInput::updateAngleEditEnabled);
	connect(cb_addAutoID, &QCheckBox::stateChanged, this, &dlg_CSVInput::updateCreateIDEnabled);
	connect(list_ColumnSelection, &QListWidget::itemSelectionChanged, this, &dlg_CSVInput::selectedColsChanged);
}

void dlg_CSVInput::setPath(QString const & path)
{
	m_fPath = path;
}

void dlg_CSVInput::saveHeadersToReg(const QString &formatName, const QString& entryName, QStringList const & headers)
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(formatName));
	settings.setValue(entryName, headers);
}

QStringList dlg_CSVInput::loadHeadersFromReg(const QString &formatName, const QString& entryName)
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(formatName));
	return settings.value(entryName).value<QStringList>();
}

bool dlg_CSVInput::checkFile()
{
	if (m_confParams.fileName.isEmpty())
	{
		QMessageBox::information(this, tr("FeatureScout"), tr("Please specify a filename!"));
		return false;
	}
	QFile file(m_confParams.fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::information(this, tr("FeatureScout"), tr("Unable to open file: %1!").arg(file.errorString()));
		return false;
	}
	file.close();
	return true;
}

void dlg_CSVInput::okBtnClicked()
{
	assignFormatSettings();
	if (!checkFile())
		return;
	assignSelectedCols();
	if (m_confParams.selHeaders.size() < 2)
	{
		QMessageBox::warning(this, tr("FeatureScout"), "Please select at least 2 columns to load!");
		return;
	}
	if (!cmbbox_Format->currentText().isEmpty())
		saveDefaultFormat(cmbbox_Format->currentText());
	accept();
}

void dlg_CSVInput::loadSelectedFormatSettings(const QString &formatName)
{
	if (!loadFormatFromRegistry(formatName))
	{
		QMessageBox::warning(this, tr("FeatureScout"), tr("Format not available (or name empty)!"));
		return;
	}
	showConfigParams(m_confParams);
	if (!loadFilePreview())
		return;
	showSelectedCols();
}

void dlg_CSVInput::updatePreview()
{
	assignFormatSettings();
	if (!loadFilePreview())
		return;
	showSelectedCols();
}

void dlg_CSVInput::switchObjectType(const QString &ObjectInputType)
{
	assignObjectTypes();
	updateColumnMappingInputs();
}

void dlg_CSVInput::saveFormatBtnClicked()
{
	QString formatName = ed_FormatName->text();
	if (formatName.trimmed().isEmpty())
	{
		QMessageBox::warning(this, tr("FeatureScout"), tr("Please enter a format name!"));
		return;
	}
	assignFormatSettings();
	assignSelectedCols();
	QStringList formatEntries = getFormatListFromRegistry();
	QSignalBlocker fmtBlocker(cmbbox_Format);
	if (formatEntries.contains(formatName, Qt::CaseSensitivity::CaseInsensitive))
	{
		auto reply = QMessageBox::warning(this, tr("FeatureScout"),
			tr("Format '%1' already exists. Do you want to overwrite it?").arg(formatName),
			QMessageBox::Yes | QMessageBox::No);
		if (reply != QMessageBox::Yes)
			return;
		else // to be sure to have the same lower/upper case string in the combo-box, delete existing entry:
			deleteFormatFromReg(formatName);
	}
	saveFormatToRegistry(m_confParams, formatName);
	saveHeadersToReg(formatName, csvRegKeys::SelectedHeaders, m_confParams.selHeaders);
	saveHeadersToReg(formatName, csvRegKeys::AllHeaders, m_confParams.currentHeaders);
	cmbbox_Format->addItem(formatName);
	cmbbox_Format->model()->sort(0);
	cmbbox_Format->setCurrentText(formatName);
}

void dlg_CSVInput::deleteFormatBtnClicked()
{
	QString formatName = cmbbox_Format->currentText();
	QMessageBox::StandardButton reply;
	reply = QMessageBox::warning(this, tr("FeatureScout"),
		tr("Format '%1' will be deleted permanently. Do you want to proceed?").arg(formatName),
		QMessageBox::Yes | QMessageBox::No);
	if (reply != QMessageBox::Yes)
		return;
	deleteFormatFromReg(formatName);
}

void dlg_CSVInput::deleteFormatFromReg(QString const & formatName)
{
	QSettings settings;
	settings.remove(getFormatRegistryKey(formatName));
	cmbbox_Format->removeItem(cmbbox_Format->findText(formatName, Qt::MatchFixedString));
}

void dlg_CSVInput::applyFormatColumnSelection()
{
	QString formatName = cmbbox_Format->currentText();
	m_confParams.selHeaders = loadHeadersFromReg(formatName, csvRegKeys::SelectedHeaders);
	showSelectedCols();
}

void dlg_CSVInput::updateColumnMappingInputs()
{
	// overall, enable column mapping only for analysis type fiber for now
	grpbox_ColMapping->setEnabled(m_confParams.objectType == iAFeatureScoutObjectType::Fibers);

	bool useStartEnd = cmbbox_col_Selection->currentIndex() == 0;
	cmbbox_col_PosStartX->setEnabled(useStartEnd);
	cmbbox_col_PosStartY->setEnabled(useStartEnd);
	cmbbox_col_PosStartZ->setEnabled(useStartEnd);
	lbl_col_posStartX->setEnabled(useStartEnd);
	lbl_col_posStartY->setEnabled(useStartEnd);
	lbl_col_posStartZ->setEnabled(useStartEnd);
	cmbbox_col_PosEndX->setEnabled(useStartEnd);
	cmbbox_col_PosEndY->setEnabled(useStartEnd);
	cmbbox_col_PosEndZ->setEnabled(useStartEnd);
	lbl_col_posEndX->setEnabled(useStartEnd);
	lbl_col_posEndY->setEnabled(useStartEnd);
	lbl_col_posEndZ->setEnabled(useStartEnd);

	cmbbox_col_PosCenterX->setEnabled(!useStartEnd);
	cmbbox_col_PosCenterY->setEnabled(!useStartEnd);
	cmbbox_col_PosCenterZ->setEnabled(!useStartEnd);
	lbl_col_posCenterX->setEnabled(!useStartEnd);
	lbl_col_posCenterY->setEnabled(!useStartEnd);
	lbl_col_posCenterZ->setEnabled(!useStartEnd);

	cb_ComputeLength->setEnabled(useStartEnd);
	cb_ComputeAngles->setEnabled(useStartEnd);
	if (!useStartEnd)
	{
		cb_ComputeLength->setChecked(false);
		cb_ComputeAngles->setChecked(false);
	}
	updateAngleEditEnabled();
	updateLengthEditEnabled();
}

void dlg_CSVInput::updateLengthEditEnabled()
{
	cmbbox_col_Length->setEnabled(!cb_ComputeLength->isChecked());
	lbl_col_length   ->setEnabled(!cb_ComputeLength->isChecked());
}

void dlg_CSVInput::updateAngleEditEnabled()
{
	cmbbox_col_Phi  ->setEnabled(!cb_ComputeAngles->isChecked());
	cmbbox_col_Theta->setEnabled(!cb_ComputeAngles->isChecked());
	lbl_col_phi     ->setEnabled(!cb_ComputeAngles->isChecked());
	lbl_col_theta   ->setEnabled(!cb_ComputeAngles->isChecked());
}

void dlg_CSVInput::updateCreateIDEnabled()
{
	lbl_col_ID->setEnabled(!cb_addAutoID->isChecked());
	cmbbox_col_ID->setEnabled(!cb_addAutoID->isChecked());
	updatePreview();
}

void dlg_CSVInput::selectedColsChanged()
{
	assignSelectedCols();
	updatePreview();
}

void dlg_CSVInput::selectFileBtnClicked()
{
	QString fileName = QFileDialog::getOpenFileName(
		this, tr("Open Files"), m_fPath, tr("Comma-separated values (*.csv),.csv")
	);
	if (fileName.isEmpty())
		return;
	ed_FileName->setText(fileName);
	updatePreview();
}

iACsvConfig const & dlg_CSVInput::getConfigParameters() const
{
	return m_confParams;
}

void dlg_CSVInput::showConfigParams(iACsvConfig const &params)
{
	// prevent signals to update config and preview:
	QSignalBlocker slsblock(ed_SkipLinesStart), sleblock(ed_SkipLinesEnd),
		csblock(cmbbox_ColSeparator), aiblock(cb_addAutoID),
		eblock(cmbbox_Encoding), otblock(cmbbox_ObjectType);
	int index = cmbbox_ObjectType->findText(MapObjectTypeToString(params.objectType), Qt::MatchContains);
	cmbbox_ObjectType->setCurrentIndex(index);
	cmbbox_ColSeparator->setCurrentIndex(ColumnSeparators().indexOf(params.colSeparator));
	cmbbox_DecimalSeparator->setCurrentText(params.decimalSeparator);
	ed_SkipLinesStart->setValue(params.skipLinesStart);
	ed_SkipLinesEnd->setValue(params.skipLinesEnd);
	ed_Spacing->setText(QString("%1").arg(params.spacing));
	cmbbox_Unit->setCurrentText(params.unit);
	cb_addAutoID->setChecked(params.addAutoID);
	cmbbox_Encoding->setCurrentText(params.encoding);
	updateColumnMappingInputs();
	updateCreateIDEnabled();
}

void dlg_CSVInput::assignFormatSettings()
{
	m_confParams.colSeparator = ColumnSeparators()[cmbbox_ColSeparator->currentIndex()];
	m_confParams.decimalSeparator = cmbbox_DecimalSeparator->currentText();
	m_confParams.skipLinesStart = ed_SkipLinesStart->value();
	m_confParams.skipLinesEnd = ed_SkipLinesEnd->value();
	m_confParams.spacing = ed_Spacing->text().toDouble();
	m_confParams.unit = cmbbox_Unit->currentText();
	m_confParams.addAutoID = cb_addAutoID->isChecked();
	m_confParams.encoding = cmbbox_Encoding->currentText();
	m_confParams.fileName = ed_FileName->text();
	assignObjectTypes();
}

void dlg_CSVInput::assignObjectTypes()
{
	m_confParams.objectType = MapStringToObjectType(cmbbox_ObjectType->currentText());
}

void dlg_CSVInput::showColumnHeaders()
{
	QSignalBlocker listSignalBlock(list_ColumnSelection);
	list_ColumnSelection->clear();
	m_confParams.currentHeaders = io->getHeaders();
	if (m_confParams.currentHeaders.isEmpty())
		return;
	list_ColumnSelection->addItems(m_confParams.currentHeaders);
	QStringList selectibleColumns = m_confParams.currentHeaders;
	selectibleColumns.insert(0, "Not mapped");
	for (auto cmbbox : mappingBoxes)
	{
		cmbbox->clear();
		cmbbox->addItems(m_confParams.currentHeaders);
	}
}

bool dlg_CSVInput::loadFilePreview()
{
	clearPreviewTable();
	int previewLines = sb_PreviewLines->value();
	if (!io->readTableEntries(tbl_preview, m_confParams, previewLines))
		return false;
	showColumnHeaders();
	return true;
}

void dlg_CSVInput::clearPreviewTable()
{
	tbl_preview->clear();
	tbl_preview->setRowCount(0);
}

void dlg_CSVInput::assignSelectedCols()
{
	auto selectedColModelIndices = list_ColumnSelection->selectionModel()->selectedIndexes();
	m_confParams.selColIdx.clear();
	for (auto selColModelIdx : selectedColModelIndices)
		m_confParams.selColIdx.push_back(selColModelIdx.row());
	qSort(m_confParams.selColIdx.begin(), m_confParams.selColIdx.end(), qLess<uint>());
	m_confParams.selHeaders.clear();
	for (auto selColIdx: m_confParams.selColIdx)
		m_confParams.selHeaders.push_back(m_confParams.currentHeaders[selColIdx]);
}

const QVector<uint>& dlg_CSVInput::getEntriesSelInd()
{
	return m_confParams.selColIdx;
}

void dlg_CSVInput::showSelectedCols()
{
	QSignalBlocker listSignalBlock(list_ColumnSelection);
	if (m_confParams.selHeaders.length() == 0)
	{
		list_ColumnSelection->selectAll();
		return;
	}
	if (m_confParams.selHeaders.length() > m_confParams.currentHeaders.length())
	{
		QMessageBox::warning(this, tr("FeatureScout"),
			tr("Size of selected headers does not match with headers in file!"));
		return;
	}
	list_ColumnSelection->clearSelection();
	for ( auto &h_entry: m_confParams.selHeaders)
	{
		int idx = m_confParams.currentHeaders.indexOf(h_entry);
		if (list_ColumnSelection->item(idx))
			list_ColumnSelection->item(idx)->setSelected(true);
	}
}

bool dlg_CSVInput::loadFormatFromRegistry(const QString & formatName)
{
	if (formatName.isEmpty())
		return false;
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(formatName));
	QStringList allEntries = settings.allKeys();
	if (allEntries.isEmpty())
		return false;
	m_formatName = formatName;
	ed_FormatName->setText(formatName);
	cmbbox_Format->setCurrentText(formatName);
	iACsvConfig defaultConfig;
	m_confParams.skipLinesStart = settings.value(csvRegKeys::SkipLinesStart, defaultConfig.skipLinesStart).toLongLong();
	m_confParams.skipLinesEnd = settings.value(csvRegKeys::SkipLinesEnd, defaultConfig.skipLinesEnd).toLongLong();
	m_confParams.colSeparator = settings.value(csvRegKeys::ColSeparator, defaultConfig.colSeparator).toString();
	m_confParams.decimalSeparator = settings.value(csvRegKeys::DecimalSeparator, defaultConfig.decimalSeparator).toString();
	m_confParams.objectType = MapStringToObjectType(settings.value(csvRegKeys::ObjectType, defaultConfig.objectType).toString());
	m_confParams.addAutoID = settings.value(csvRegKeys::AddAutoID, defaultConfig.addAutoID).toBool();
	m_confParams.unit = settings.value(csvRegKeys::Unit, defaultConfig.unit).toString();
	m_confParams.spacing = settings.value(csvRegKeys::Spacing, defaultConfig.spacing).toDouble();
	m_confParams.encoding = settings.value(csvRegKeys::Encoding, defaultConfig.encoding).toString();
	m_confParams.selHeaders = loadHeadersFromReg(formatName, csvRegKeys::SelectedHeaders);
	return true;
}

void dlg_CSVInput::saveDefaultFormat(QString const & formatName)
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(""));
	settings.setValue(csvRegKeys::DefaultFormat, formatName);
}

QString dlg_CSVInput::getDefaultFormat() const
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(""));
	return settings.value(csvRegKeys::DefaultFormat, "").toString();
}

QStringList dlg_CSVInput::getFormatListFromRegistry() const
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey("") );
	return settings.childGroups();
}

void dlg_CSVInput::saveFormatToRegistry(iACsvConfig const & csv_params, const QString &formatName)
{
	QSettings settings;
	settings.beginGroup(getFormatRegistryKey(formatName));
	settings.setValue(csvRegKeys::SkipLinesStart, csv_params.skipLinesStart);
	settings.setValue(csvRegKeys::SkipLinesEnd, csv_params.skipLinesEnd);
	settings.setValue(csvRegKeys::ColSeparator, csv_params.colSeparator);
	settings.setValue(csvRegKeys::DecimalSeparator, csv_params.decimalSeparator);
	settings.setValue(csvRegKeys::ObjectType, MapObjectTypeToString(csv_params.objectType));
	settings.setValue(csvRegKeys::AddAutoID, csv_params.addAutoID);
	settings.setValue(csvRegKeys::Unit, csv_params.unit);
	settings.setValue(csvRegKeys::Spacing, csv_params.spacing);
	settings.setValue(csvRegKeys::Encoding, csv_params.encoding);
}
