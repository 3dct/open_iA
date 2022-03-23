/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iACsvConfig.h"
#include "iACsvQTableCreator.h"
#include "iACsvVectorTableCreator.h"
#include "ui_CsvInput.h"

#include <iALog.h>

#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

namespace
{
	//Names for registry
	static const QString DefaultFormat = "DefaultFormat";
	static const QString AdvancedMode = "AdvancedMode";
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

	const char* NotMapped = "Not mapped";

	static const QString IniFormatName = "FormatName";
}

dlg_CSVInput::dlg_CSVInput(bool volumeDataAvailable, QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/)
	: QDialog(parent, f),
	m_columnMappingChoiceSet(false),
	m_volumeDataAvailable(volumeDataAvailable),
	m_ui(new Ui_CsvInput())
{
	m_ui->setupUi(this);
	// combo boxes involved in column mapping (in the same order as in iACsvConfig::MappedColumn):
	m_mappingBoxes.push_back(m_ui->cmbbox_col_PosStartX);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_PosStartY);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_PosStartZ);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_PosEndX);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_PosEndY);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_PosEndZ);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_PosCenterX);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_PosCenterY);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_PosCenterZ);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_Length);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_Diameter);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_Phi);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_Theta);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_DimensionX);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_DimensionY);
	m_mappingBoxes.push_back(m_ui->cmbbox_col_DimensionZ);
	m_ui->cb_AdvancedMode->setChecked(loadGeneralSetting(AdvancedMode).toBool());
	for (int i = 0; i < iACsvConfig::VisTypeCount; ++i)
	{
		m_ui->cmbbox_VisualizeAs->addItem(MapVisType2Str(static_cast<iACsvConfig::VisualizationType>(i)));
	}
	advancedModeToggled();
	initParameters();
	connectSignals();
	m_ui->list_ColumnSelection->installEventFilter(this);
}

void dlg_CSVInput::setPath(QString const & path)
{
	m_path = path;
}

void dlg_CSVInput::setFileName(QString const & fileName)
{
	m_ui->ed_FileName->setText(fileName);
	updatePreview();
}

void dlg_CSVInput::setFormat(QString const & formatName)
{
	if (m_ui->cmbbox_Format->findText(formatName))
	{
		m_ui->cmbbox_Format->setCurrentText(formatName);
	}
}

iACsvConfig const & dlg_CSVInput::getConfig() const
{
	return m_confParams;
}

void dlg_CSVInput::initParameters()
{
	m_ui->ed_FormatName->setValidator(new QRegularExpressionValidator(
		QRegularExpression("[A-Za-z0-9_]{0,30}"), this));  // limit input to format names
	QStringList formatEntries = iACsvConfig::getListFromRegistry();
	if (!formatEntries.contains(iACsvConfig::FCPFiberFormat))
	{
		formatEntries.append(iACsvConfig::FCPFiberFormat);
	}
	if (!formatEntries.contains(iACsvConfig::FCVoidFormat))
	{
		formatEntries.append(iACsvConfig::FCVoidFormat);
	}
	m_ui->cmbbox_Format->addItems(formatEntries);
	// load default format, and if that fails, load first format if available:
	if (!loadFormatFromRegistry(loadGeneralSetting(DefaultFormat).toString()) && formatEntries.length() > 0)
	{
		loadFormatFromRegistry(formatEntries[0]);
	}
	showConfigParams();
}

void dlg_CSVInput::connectSignals()
{
	connect(m_ui->btn_SelectFile, &QPushButton::clicked, this, &dlg_CSVInput::selectFileBtnClicked);
	connect(m_ui->cb_CurvedFiberInfo, &QCheckBox::stateChanged, this, &dlg_CSVInput::curvedFiberInfoChanged);
	connect(m_ui->btn_SelectCurvedFile, &QPushButton::clicked, this, &dlg_CSVInput::selectCurvedFileBtnClicked);
	connect(m_ui->btn_SaveFormat, &QPushButton::clicked, this, &dlg_CSVInput::saveFormatBtnClicked);
	connect(m_ui->btn_DeleteFormat, &QPushButton::clicked, this, &dlg_CSVInput::deleteFormatBtnClicked);
	connect(m_ui->btn_UpdatePreview, &QPushButton::clicked, this, &dlg_CSVInput::updatePreview);
	connect(m_ui->btn_ExportTable, &QPushButton::clicked, this, &dlg_CSVInput::exportTable);
	connect(m_ui->btn_ApplyFormatColumnSelection, &QPushButton::clicked, this, &dlg_CSVInput::applyFormatColumnSelection);
	connect(m_ui->btn_ExportFormat, &QPushButton::clicked, this, &dlg_CSVInput::exportButtonClicked);
	connect(m_ui->btn_ImportFormat, &QPushButton::clicked, this, &dlg_CSVInput::importButtonClicked);
	connect(m_ui->cmbbox_Format, &QComboBox::currentTextChanged, this, &dlg_CSVInput::loadSelectedFormatSettings);
	connect(m_ui->cmbbox_ColSeparator, &QComboBox::currentTextChanged, this, &dlg_CSVInput::updatePreview);
	connect(m_ui->cmbbox_ObjectType, &QComboBox::currentTextChanged, this, &dlg_CSVInput::switchObjectType);
	connect(m_ui->cmbbox_Encoding, &QComboBox::currentTextChanged, this, &dlg_CSVInput::updatePreview);
	connect(m_ui->cmbbox_VisualizeAs, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_CSVInput::visualizationTypeChanged);
	connect(m_ui->btn_OK, &QPushButton::clicked, this, &dlg_CSVInput::okBtnClicked);
	connect(m_ui->ed_SkipLinesStart, QOverload<int>::of(&QSpinBox::valueChanged), this, &dlg_CSVInput::updatePreview);
	connect(m_ui->ed_SkipLinesEnd,   QOverload<int>::of(&QSpinBox::valueChanged), this, &dlg_CSVInput::updatePreview);
	connect(m_ui->sb_PreviewLines,   QOverload<int>::of(&QSpinBox::valueChanged), this, &dlg_CSVInput::updatePreview);
	connect(m_ui->cb_ComputeStartEnd, &QCheckBox::stateChanged, this, &dlg_CSVInput::computeStartEndChanged);
	connect(m_ui->cb_ComputeLength, &QCheckBox::stateChanged, this, &dlg_CSVInput::computeLengthChanged);
	connect(m_ui->cb_ComputeAngles, &QCheckBox::stateChanged, this, &dlg_CSVInput::computeAngleChanged);
	connect(m_ui->cb_ComputeTensors, &QCheckBox::stateChanged, this, &dlg_CSVInput::updatePreview);
	connect(m_ui->cb_ComputeCenter, &QCheckBox::stateChanged, this, &dlg_CSVInput::computeCenterChanged);
	connect(m_ui->cb_AddAutoID, &QCheckBox::stateChanged, this, &dlg_CSVInput::updatePreview);
	connect(m_ui->cb_ContainsHeader, &QCheckBox::stateChanged, this, &dlg_CSVInput::updatePreview);
	connect(m_ui->cb_AdvancedMode, &QCheckBox::stateChanged, this, &dlg_CSVInput::advancedModeToggled);
	connect(m_ui->list_ColumnSelection, &QListWidget::itemSelectionChanged, this, &dlg_CSVInput::selectedColsChanged);
	connect(m_ui->cb_FixedDiameter, &QCheckBox::stateChanged, this, &dlg_CSVInput::fixedDiameterChanged);
	connect(m_ui->sb_FixedDiameter, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &dlg_CSVInput::updatePreview);
}

void dlg_CSVInput::okBtnClicked()
{
	assignFormatSettings();
	assignSelectedCols();
	QString errorMsg;
	if (!m_confParams.isValid(errorMsg))
	{
		QMessageBox::warning(this, tr("FeatureScout"), errorMsg);
		return;
	}
	if (m_confParams.visType == iACsvConfig::UseVolume && !m_volumeDataAvailable)
	{
		QMessageBox::information(this, "FeatureScout", "You have selected to use the 'Labelled Volume' Visualization. "
			"This requires a volume dataset to be loaded which contains the labelled objects, "
			"yet there is either no open window or the active window does not contain volume data!");
		return;
	}
	if (!m_ui->cmbbox_Format->currentText().isEmpty())
	{
		saveGeneralSetting(DefaultFormat, m_ui->cmbbox_Format->currentText());
	}
	saveGeneralSetting(AdvancedMode, m_ui->cb_AdvancedMode->isChecked());
	accept();
}

void dlg_CSVInput::loadSelectedFormatSettings(const QString &formatName)
{
	if (!loadFormatFromRegistry(formatName))
	{
		QMessageBox::warning(this, tr("FeatureScout"), tr("Format not available (or name empty)!"));
		return;
	}
	showConfigParams();
	if (!loadFilePreview())
	{
		return;
	}
	showSelectedCols();
}

void dlg_CSVInput::updatePreview()
{
	assignFormatSettings();
	if (!loadFilePreview())
	{
		return;
	}
	showSelectedCols();
}

void dlg_CSVInput::visualizationTypeChanged(int newType)
{
	m_ui->sb_CylinderQuality->setEnabled(newType == iACsvConfig::Cylinders);
	m_ui->sb_SegmentSkip->setEnabled(newType == iACsvConfig::Cylinders || newType == iACsvConfig::Lines);
	if (newType != iACsvConfig::Cylinders && newType != iACsvConfig::Lines)
	{
		m_ui->cb_CurvedFiberInfo->setChecked(false);
	}
	m_ui->cb_CurvedFiberInfo->setEnabled(newType == iACsvConfig::Cylinders || newType == iACsvConfig::Lines);
}

void dlg_CSVInput::exportTable()
{
	iACsvIO io;
	iACsvVectorTableCreator creator;
	if (!io.loadCSV(creator, m_confParams, std::numeric_limits<size_t>::max()))
	{
		LOG(lvlError, QString("Error loading CSV file '%1'.").arg(m_confParams.fileName));
		return;
	}

	QString origCSVFileName = m_confParams.fileName;
	QFile origCSV(origCSVFileName);
	if (!origCSV.open(QIODevice::ReadOnly))
	{
		LOG(lvlError, QString("Could not open CSV file '%1' for reading! "
			"It probably does not exist!").arg(origCSVFileName));
		return;
	}
	QStringList origCSVInfo;
	QTextStream in(&origCSV);
	//TODO: Skip Header problem for arbitrary file format (see getOutputHeaders below)
	for (size_t r = 0; r < m_confParams.skipLinesStart - 1; ++r)
	{
		origCSVInfo.append(in.readLine());
	}
	QString exportCSVFileName = QFileDialog::getSaveFileName(this, tr("Export CSV file"),
		m_path, "CSV file (*.csv);;All files (*)");
	if (exportCSVFileName.isEmpty())
	{
		LOG(lvlInfo, "Selected file name is empty or selection cancelled, aborting!");
		return;
	}

	QFile csvExport(exportCSVFileName);
	if (!csvExport.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Could not open file '%1' for writing; "
			"maybe it is locked by another program or the folder does not exist yet?").arg(exportCSVFileName));
		return;
	}

	QTextStream ts(&csvExport);
	for (int i = 0; i < origCSVInfo.size(); ++i)
	{
		ts << origCSVInfo[i] + "\n";
	}
	QStringList outputHeaders = io.getOutputHeaders();
	outputHeaders.removeLast();	// without ClassID
	ts << outputHeaders.join(",") + ",\n";
	for (size_t r=0; r < creator.table()[0].size(); ++r)
	{   // values are stored in col-row order
		QStringList strList;
		for (size_t c = 0; c < creator.table().size()-1; ++c)
		{
			strList << QString::number(creator.table()[c][r]);
		}
		ts << strList.join(",") + (r == creator.table().size() - 1 ? "," : ",\n");
	}
	csvExport.close();

	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Icon::Information);
	msgBox.setText(QString("CSV file successfully saved under: %1").arg(exportCSVFileName));
	msgBox.setWindowTitle("FeatureScout");
	msgBox.exec();
	return;
}

void dlg_CSVInput::switchObjectType(const QString & /*ObjectInputType*/)
{
	assignObjectTypes();
	updateColumnMappingInputs();
}

void dlg_CSVInput::saveFormatBtnClicked()
{
	QString formatName = m_ui->ed_FormatName->text();
	if (formatName.trimmed().isEmpty())
	{
		QMessageBox::warning(this, tr("FeatureScout"), tr("Please enter a format name!"));
		return;
	}
	assignFormatSettings();
	assignSelectedCols();
	saveFormatToRegistry(formatName);
}

void dlg_CSVInput::deleteFormatBtnClicked()
{
	QString formatName = m_ui->cmbbox_Format->currentText();
	QMessageBox::StandardButton reply;
	reply = QMessageBox::warning(this, tr("FeatureScout"),
		tr("Format '%1' will be deleted permanently. Do you want to proceed?").arg(formatName),
		QMessageBox::Yes | QMessageBox::No);
	if (reply != QMessageBox::Yes)
	{
		return;
	}
	deleteFormatFromReg(formatName);
}

void dlg_CSVInput::deleteFormatFromReg(QString const & formatName)
{
	QSettings settings;
	settings.remove(iACsvConfig::getFormatKey(formatName));
	m_ui->cmbbox_Format->removeItem(m_ui->cmbbox_Format->findText(formatName, Qt::MatchFixedString));
}

void dlg_CSVInput::applyFormatColumnSelection()
{
	QString formatName = m_ui->cmbbox_Format->currentText();
	iACsvConfig tmp;
	loadFormatFromRegistry(formatName, tmp);
	m_confParams.selectedHeaders = tmp.selectedHeaders;
	showSelectedCols();
}

void dlg_CSVInput::computeStartEndChanged()
{
	updateColumnMappingInputs();
	updatePreview();
}

void dlg_CSVInput::updateDiameterInputEnabled()
{
	m_ui->sb_FixedDiameter->setEnabled(m_ui->cb_FixedDiameter->isChecked());
	m_ui->lbl_col_diameter->setEnabled(!m_ui->cb_FixedDiameter->isChecked());
	m_ui->cmbbox_col_Diameter->setEnabled(!m_ui->cb_FixedDiameter->isChecked());
}

void dlg_CSVInput::fixedDiameterChanged()
{
	updateDiameterInputEnabled();
	updatePreview();
}

void dlg_CSVInput::updateColumnMappingInputs()
{
	m_ui->lbl_col_dimensionX->setEnabled(m_confParams.objectType == iAObjectType::Voids);
	m_ui->lbl_col_dimensionY->setEnabled(m_confParams.objectType == iAObjectType::Voids);
	m_ui->lbl_col_dimensionZ->setEnabled(m_confParams.objectType == iAObjectType::Voids);
	m_ui->cmbbox_col_DimensionX->setEnabled( m_confParams.objectType == iAObjectType::Voids );
	m_ui->cmbbox_col_DimensionY->setEnabled( m_confParams.objectType == iAObjectType::Voids );
	m_ui->cmbbox_col_DimensionZ->setEnabled( m_confParams.objectType == iAObjectType::Voids );

	bool computeStartEnd = m_ui->cb_ComputeStartEnd->isChecked();
	m_ui->cmbbox_col_PosStartX->setEnabled(!computeStartEnd);
	m_ui->cmbbox_col_PosStartY->setEnabled(!computeStartEnd);
	m_ui->cmbbox_col_PosStartZ->setEnabled(!computeStartEnd);
	m_ui->lbl_col_posStartX->setEnabled   (!computeStartEnd);
	m_ui->lbl_col_posStartY->setEnabled   (!computeStartEnd);
	m_ui->lbl_col_posStartZ->setEnabled   (!computeStartEnd);
	m_ui->cmbbox_col_PosEndX->setEnabled  (!computeStartEnd);
	m_ui->cmbbox_col_PosEndY->setEnabled  (!computeStartEnd);
	m_ui->cmbbox_col_PosEndZ->setEnabled  (!computeStartEnd);
	m_ui->lbl_col_posEndX->setEnabled     (!computeStartEnd);
	m_ui->lbl_col_posEndY->setEnabled     (!computeStartEnd);
	m_ui->lbl_col_posEndZ->setEnabled     (!computeStartEnd);

	m_ui->cb_ComputeLength->setEnabled    (!computeStartEnd);
	m_ui->cb_ComputeAngles->setEnabled    (!computeStartEnd);
	m_ui->cb_ComputeCenter->setEnabled    (!computeStartEnd);
	if (computeStartEnd)
	{
		m_ui->cb_ComputeLength->setChecked(false);
		m_ui->cb_ComputeAngles->setChecked(false);
		m_ui->cb_ComputeCenter->setChecked(false);
	}
	updateAngleEditEnabled();
	updateLengthEditEnabled();
	updateCenterEditEnabled();
	updateDiameterInputEnabled();
}

void dlg_CSVInput::updateLengthEditEnabled()
{
	m_ui->cmbbox_col_Length->setEnabled(!m_ui->cb_ComputeLength->isChecked());
	m_ui->lbl_col_length   ->setEnabled(!m_ui->cb_ComputeLength->isChecked());
}

void dlg_CSVInput::computeLengthChanged()
{
	updateLengthEditEnabled();
	updatePreview();
}

void dlg_CSVInput::updateAngleEditEnabled()
{
	m_ui->cmbbox_col_Phi  ->setEnabled(!m_ui->cb_ComputeAngles->isChecked());
	m_ui->cmbbox_col_Theta->setEnabled(!m_ui->cb_ComputeAngles->isChecked());
	m_ui->lbl_col_phi     ->setEnabled(!m_ui->cb_ComputeAngles->isChecked());
	m_ui->lbl_col_theta   ->setEnabled(!m_ui->cb_ComputeAngles->isChecked());
}

void dlg_CSVInput::computeAngleChanged()
{
	updateAngleEditEnabled();
	updatePreview();
}

void dlg_CSVInput::updateCenterEditEnabled()
{
	bool centerEditEnabled = !m_ui->cb_ComputeCenter->isChecked();
	m_ui->cmbbox_col_PosCenterX->setEnabled(centerEditEnabled);
	m_ui->cmbbox_col_PosCenterY->setEnabled(centerEditEnabled);
	m_ui->cmbbox_col_PosCenterZ->setEnabled(centerEditEnabled);
	m_ui->lbl_col_posCenterX->setEnabled(centerEditEnabled);
	m_ui->lbl_col_posCenterY->setEnabled(centerEditEnabled);
	m_ui->lbl_col_posCenterZ->setEnabled(centerEditEnabled);
}

void dlg_CSVInput::computeCenterChanged()
{
	updateCenterEditEnabled();
	updatePreview();
}

void dlg_CSVInput::selectedColsChanged()
{
	assignSelectedCols();
	updatePreview();
}

void dlg_CSVInput::advancedModeToggled()
{
	m_ui->wd_Advanced->setVisible(m_ui->cb_AdvancedMode->isChecked());
}

void dlg_CSVInput::selectFileBtnClicked()
{
	QString fileName =
		QFileDialog::getOpenFileName(this, tr("Open Files"), m_path, tr("Comma-separated values (*.csv);;All files (*)"));
	if (fileName.isEmpty())
	{
		return;
	}
	m_ui->ed_FileName->setText(fileName);
	updatePreview();
}

void dlg_CSVInput::selectCurvedFileBtnClicked()
{
	QString fileName = QFileDialog::getOpenFileName(
		this, tr("Open Files"), m_path, tr("Curved Fiber Information (*.csv);;All files (*)")
	);
	if (fileName.isEmpty())
	{
		return;
	}
	m_ui->ed_CurvedFileName->setText(fileName);
}

void dlg_CSVInput::curvedFiberInfoChanged()
{
	m_ui->ed_CurvedFileName->setEnabled(m_ui->cb_CurvedFiberInfo->isChecked());
	m_ui->btn_SelectCurvedFile->setEnabled(m_ui->cb_CurvedFiberInfo->isChecked());
}

void dlg_CSVInput::showConfigParams()
{
	// prevent signals to update config and preview:
	QSignalBlocker slsblock(m_ui->ed_SkipLinesStart), sleblock(m_ui->ed_SkipLinesEnd),
		csblock(m_ui->cmbbox_ColSeparator), aiblock(m_ui->cb_AddAutoID), eblock(m_ui->cmbbox_Encoding),
		otblock(m_ui->cmbbox_ObjectType), clblock(m_ui->cb_ComputeLength), cablock(m_ui->cb_ComputeAngles),
		ctblock(m_ui->cb_ComputeTensors), ccblock(m_ui->cb_ComputeCenter), chblock(m_ui->cb_ContainsHeader),
		cseblock(m_ui->cb_ComputeStartEnd), cfdblock(m_ui->cb_FixedDiameter), dsblock(m_ui->sb_FixedDiameter);
	if (m_confParams.skipLinesStart > std::numeric_limits<int>::max() ||
		m_confParams.skipLinesEnd > std::numeric_limits<int>::max() ||
		m_confParams.segmentSkip > std::numeric_limits<int>::max())
	{
		LOG(lvlWarn, "Skip Line start/end or segment skip number is too high to be displayed in this dialog!");
	}
	int index = m_ui->cmbbox_ObjectType->findText(MapObjectTypeToString(m_confParams.objectType), Qt::MatchContains);
	m_ui->cmbbox_ObjectType->setCurrentIndex(index);
	m_ui->cmbbox_ColSeparator->setCurrentIndex(ColumnSeparators().indexOf(m_confParams.columnSeparator));
	m_ui->cmbbox_DecimalSeparator->setCurrentText(m_confParams.decimalSeparator);
	m_ui->ed_SkipLinesStart->setValue(static_cast<int>(m_confParams.skipLinesStart));
	m_ui->ed_SkipLinesEnd->setValue(static_cast<int>(m_confParams.skipLinesEnd));
	m_ui->ed_Spacing->setText(QString("%1").arg(m_confParams.spacing));
	m_ui->cmbbox_Unit->setCurrentText(m_confParams.unit);
	m_ui->cmbbox_Encoding->setCurrentText(m_confParams.encoding);
	m_ui->cb_AddAutoID->setChecked(m_confParams.addAutoID);
	m_ui->cb_ComputeLength->setChecked(m_confParams.computeLength);
	m_ui->cb_ComputeAngles->setChecked(m_confParams.computeAngles);
	m_ui->cb_ComputeTensors->setChecked(m_confParams.computeTensors);
	m_ui->cb_ComputeCenter->setChecked(m_confParams.computeCenter);
	m_ui->cb_ComputeStartEnd->setChecked(m_confParams.computeStartEnd);
	m_ui->cb_ContainsHeader->setChecked(m_confParams.containsHeader);
	m_ui->sb_OfsX->setValue(m_confParams.offset[0]);
	m_ui->sb_OfsY->setValue(m_confParams.offset[1]);
	m_ui->sb_OfsZ->setValue(m_confParams.offset[2]);
	m_ui->cmbbox_VisualizeAs->setCurrentText(MapVisType2Str(m_confParams.visType));
	visualizationTypeChanged(m_confParams.visType);
	m_ui->sb_SegmentSkip->setValue(static_cast<int>(m_confParams.segmentSkip));
	m_ui->sb_CylinderQuality->setValue(m_confParams.cylinderQuality);
	m_ui->cb_FixedDiameter->setChecked(m_confParams.isDiameterFixed);
	m_ui->sb_FixedDiameter->setValue(m_confParams.fixedDiameterValue);
	updateColumnMappingInputs();
}

void dlg_CSVInput::assignFormatSettings()
{
	m_confParams.fileName = m_ui->ed_FileName->text();
	m_confParams.curvedFiberFileName = m_ui->cb_CurvedFiberInfo->isChecked() ? m_ui->ed_CurvedFileName->text() : "";
	assignObjectTypes();
	m_confParams.columnSeparator = ColumnSeparators()[m_ui->cmbbox_ColSeparator->currentIndex()];
	m_confParams.decimalSeparator = m_ui->cmbbox_DecimalSeparator->currentText();
	m_confParams.skipLinesStart = m_ui->ed_SkipLinesStart->value();
	m_confParams.skipLinesEnd = m_ui->ed_SkipLinesEnd->value();
	m_confParams.spacing = m_ui->ed_Spacing->text().toDouble();
	m_confParams.unit = m_ui->cmbbox_Unit->currentText();
	m_confParams.encoding = m_ui->cmbbox_Encoding->currentText();
	m_confParams.addAutoID = m_ui->cb_AddAutoID->isChecked();
	m_confParams.computeLength = m_ui->cb_ComputeLength->isChecked();
	m_confParams.computeAngles = m_ui->cb_ComputeAngles->isChecked();
	m_confParams.computeTensors = m_ui->cb_ComputeTensors->isChecked();
	m_confParams.computeCenter = m_ui->cb_ComputeCenter->isChecked();
	m_confParams.computeStartEnd = m_ui->cb_ComputeStartEnd->isChecked();
	m_confParams.offset[0] = m_ui->sb_OfsX->value();
	m_confParams.offset[1] = m_ui->sb_OfsY->value();
	m_confParams.offset[2] = m_ui->sb_OfsZ->value();
	m_confParams.containsHeader = m_ui->cb_ContainsHeader->isChecked();
	m_confParams.visType = static_cast<iACsvConfig::VisualizationType>(m_ui->cmbbox_VisualizeAs->currentIndex());
	m_confParams.cylinderQuality = m_ui->sb_CylinderQuality->value();
	m_confParams.segmentSkip = m_ui->sb_SegmentSkip->value();
	m_confParams.isDiameterFixed = m_ui->cb_FixedDiameter->isChecked();
	m_confParams.fixedDiameterValue = m_ui->sb_FixedDiameter->value();
	if (!m_columnMappingChoiceSet)
	{
		return;
	}
	m_confParams.columnMapping.clear();
	QSet<QString> usedColumns;
	for (int i = 0; i < iACsvConfig::MappedCount; ++i)
	{
		if (!m_mappingBoxes[i]->currentText().isEmpty() &&  m_mappingBoxes[i]->currentText() != NotMapped)
		{
			if (usedColumns.contains(m_mappingBoxes[i]->currentText()))
			{
				LOG(lvlWarn, QString("Invalid column mapping: Column '%1' is used more than once!").arg(m_mappingBoxes[i]->currentText()));
			}
			else
			{
				usedColumns.insert(m_mappingBoxes[i]->currentText());
			}
			m_confParams.columnMapping.insert(i, m_mappingBoxes[i]->currentIndex()-1);
		}
	}
}

void dlg_CSVInput::assignObjectTypes()
{
	m_confParams.objectType = MapStringToObjectType(m_ui->cmbbox_ObjectType->currentText());
}

void dlg_CSVInput::showColumnHeaders()
{
	QSignalBlocker listSignalBlock(m_ui->list_ColumnSelection);
	m_ui->list_ColumnSelection->clear();
	if (m_confParams.currentHeaders.isEmpty())
	{
		return;
	}
	m_ui->list_ColumnSelection->addItems(m_confParams.currentHeaders);
	for (int row = 0; row < m_ui->list_ColumnSelection->count(); ++row)
	{
		m_ui->list_ColumnSelection->item(row)->setFlags(m_ui->list_ColumnSelection->item(row)->flags() | Qt::ItemIsEditable);
	}
	QStringList selectibleColumns = m_confParams.currentHeaders;
	selectibleColumns.insert(0, NotMapped);
	for (auto cmbbox : m_mappingBoxes)
	{
		auto curText = cmbbox->currentText();
		cmbbox->clear();
		cmbbox->addItems(selectibleColumns);
		cmbbox->setCurrentText(curText);
	}
	for (int i = 0; i < iACsvConfig::MappedCount; ++i)
	{
		if (m_confParams.columnMapping.contains(i))
		{
			m_mappingBoxes[i]->setCurrentIndex(m_confParams.columnMapping[i] + 1);
		}
		else
		{
			m_mappingBoxes[i]->setCurrentIndex(0);
		}
	}
	m_columnMappingChoiceSet = true;
}

bool dlg_CSVInput::loadFilePreview()
{
	if (m_confParams.fileName.isEmpty())
	{
		return false;
	}
	clearPreviewTable();
	int previewLines = m_ui->sb_PreviewLines->value();
	iACsvIO io;
	iACsvQTableCreator creator(m_ui->tbl_preview);
	if (!io.loadCSV(creator, m_confParams, previewLines))
	{
		return false;
	}
	m_confParams.currentHeaders = io.getFileHeaders();
	showColumnHeaders();
	return true;
}

void dlg_CSVInput::clearPreviewTable()
{
	m_ui->tbl_preview->clear();
	m_ui->tbl_preview->setRowCount(0);
}

void dlg_CSVInput::assignSelectedCols()
{
	auto selectedColModelIndices = m_ui->list_ColumnSelection->selectionModel()->selectedIndexes();
	QVector<int> selectedColIDx;
	for (auto selColModelIdx : selectedColModelIndices)
	{
		selectedColIDx.push_back(selColModelIdx.row());
	}
	std::sort(selectedColIDx.begin(), selectedColIDx.end(), std::less<uint>());
	if (m_ui->list_ColumnSelection->count() > 0)
	{
		m_confParams.currentHeaders.clear();
		for (int row = 0; row < m_ui->list_ColumnSelection->count(); ++row)
		{
			m_confParams.currentHeaders.append(m_ui->list_ColumnSelection->item(row)->text());
		}
		m_confParams.selectedHeaders.clear();
		for (auto selColIdx : selectedColIDx)
		{
			m_confParams.selectedHeaders.push_back(m_confParams.currentHeaders[selColIdx]);
		}
	}
}

void dlg_CSVInput::showSelectedCols()
{
	QSignalBlocker listSignalBlock(m_ui->list_ColumnSelection);
	m_ui->list_ColumnSelection->clearSelection();
	for ( auto &selected: m_confParams.selectedHeaders)
	{
		int idx = m_confParams.currentHeaders.indexOf(selected);
		if (idx >= 0 && idx < m_ui->list_ColumnSelection->count())
		{
			m_ui->list_ColumnSelection->item(idx)->setSelected(true);
		}
		else
		{
			LOG(lvlWarn, QString("Header entry '%1' not found, skipping its selection!").arg(selected));
		}
	}
	if (m_confParams.addAutoID)
	{
		m_ui->ed_col_ID->setText(iACsvIO::ColNameAutoID);
	}
	else if (m_confParams.selectedHeaders.size() > 0)
	{
		m_ui->ed_col_ID->setText(m_confParams.selectedHeaders[0]);
	}
	else if (m_confParams.currentHeaders.size() > 0)
	{
		m_ui->ed_col_ID->setText(m_confParams.currentHeaders[0]);
	}
	else
	{
		m_ui->ed_col_ID->setText("NONE");
	}
}

void dlg_CSVInput::saveGeneralSetting(QString const & settingName, QVariant value)
{
	QSettings settings;
	settings.beginGroup(iACsvConfig::getFormatKey(""));
	settings.setValue(settingName, value);
}

QVariant dlg_CSVInput::loadGeneralSetting(QString const & settingName) const
{
	QSettings settings;
	settings.beginGroup(iACsvConfig::getFormatKey(""));
	return settings.value(settingName, "");
}

void dlg_CSVInput::exportButtonClicked()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Settings"),
		m_path, "Settings file (*.ini);;All files (*)");
	if (fileName.isEmpty())
	{
		return;
	}
	assignFormatSettings();
	assignSelectedCols();
	QSettings settings(fileName, QSettings::IniFormat);
	QString formatName = m_ui->ed_FormatName->text();
	settings.setValue(IniFormatName, formatName);
	m_confParams.save(settings, formatName);
}

void dlg_CSVInput::importButtonClicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings"),
		m_path, "Settings file (*.ini);;All files (*)");
	if (fileName.isEmpty())
	{
		return;
	}
	QSettings settings(fileName, QSettings::IniFormat);
	QString formatName = settings.value(IniFormatName).toString();
	if (formatName.isEmpty())
	{
		QMessageBox::information(this, "FeatureScout", "Invalid format settings file, cannot read format name");
		return;
	}
	m_confParams.load(settings, formatName);
	showConfigParams();
	m_ui->ed_FormatName->setText(formatName);
	saveFormatToRegistry(formatName);
}

bool dlg_CSVInput::loadFormatFromRegistry(const QString & formatName)
{
	bool result = loadFormatFromRegistry(formatName, m_confParams);
	if (result)
	{
		m_ui->ed_FormatName->setText(formatName);
		m_ui->cmbbox_Format->setCurrentText(formatName);
	}
	return result;
}

bool dlg_CSVInput::loadFormatFromRegistry(const QString & formatName, iACsvConfig & dest)
{
	if (formatName.isEmpty())
	{
		return false;
	}
	QSettings settings;
	if (!dest.load(settings, formatName))
	{
		if (formatName == iACsvConfig::FCPFiberFormat)
		{
			dest = iACsvConfig::getFCPFiberFormat(dest.fileName);
			return true;
		}
		else if (formatName == iACsvConfig::FCVoidFormat)
		{
			dest = iACsvConfig::getFCVoidFormat(dest.fileName);
			return true;
		}
		return false;
	}
	return true;
}

void dlg_CSVInput::saveFormatToRegistry(const QString &formatName)
{
	QStringList formatEntries = iACsvConfig::getListFromRegistry();
	QSignalBlocker fmtBlocker(m_ui->cmbbox_Format);
	if (formatEntries.contains(formatName, Qt::CaseSensitivity::CaseInsensitive))
	{
		auto reply = QMessageBox::warning(this, tr("FeatureScout"),
			tr("Format '%1' already exists. Do you want to overwrite it?").arg(formatName),
			QMessageBox::Yes | QMessageBox::No);
		if (reply != QMessageBox::Yes)
		{
			return;
		}
		else  // to be sure to have the same lower/upper case string in the combo-box, delete existing entry:
		{
			deleteFormatFromReg(formatName);
		}
	}
	QSettings settings;
	m_confParams.save(settings, formatName);
	m_ui->cmbbox_Format->addItem(formatName);
	m_ui->cmbbox_Format->model()->sort(0);
	m_ui->cmbbox_Format->setCurrentText(formatName);
}

bool dlg_CSVInput::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)
		{
			auto selectedItems = m_ui->list_ColumnSelection->selectedItems();
			//qDeleteAll(selectedItems);
			for (int i = 0; i < selectedItems.size(); i++)
			{
				auto item = m_ui->list_ColumnSelection->takeItem(m_ui->list_ColumnSelection->row(selectedItems[i]));
				delete item;
			}
		}
		return true;
	}
	else  // standard event processing
	{
		return QObject::eventFilter(obj, event);
	}
}
