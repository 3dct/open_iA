// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARawFileParamDlg.h"

#include "iARawFilePreviewSlicer.h"
#include <iAVectorInput.h>

#include <iAAttributeDescriptor.h>    // for selectOption
#include <iAChartWithFunctionsWidget.h>
#include <iAHistogramData.h>
#include <iAPlotTypes.h>
#include <iARawFileIO.h>
#include <iALog.h>
#include <iAParameterDlg.h>
#include <iARawFileParameters.h>
#include <iAToolsVTK.h>    // for mapVTKTypeToReadableDataType, readableDataTypes, ...
#include <iATransferFunctionOwner.h>    // for defaultColorTF
#include <iAValueTypeVectorHelpers.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>

#include <QApplication>
#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>

struct iASliceMergedValues
{
	std::vector<double> values;
	std::set<std::pair<iASlicerMode, int>> slices;
	void clear()
	{
		values.clear();
		slices.clear();
	}
	void addValues(vtkImageData * i, iASlicerMode mode, int sliceNr)
	{
		if (slices.contains(std::make_pair(mode, sliceNr)))
		{
			return;
		}
		//LOG(lvlDebug, QString("%1: values: %2; slices: %3").arg(slicerModeString(mode))
		//	.arg(values.size()).arg(slices.size()));
		//int skipped = 0;
		FOR_VTKIMG_PIXELS(i, x, y, z)
		{
			// avoid adding values twice:
			int coord[2] = { x, y };
			bool skip = false;
			for (auto s : slices)
			{
				if (mode == s.first)  // if same slice mode, only skip if same slice - already checked at top of method
				{
					continue;
				}
				auto thisSliceAxis = mapGlobalToSliceAxis(mode, mapSliceToGlobalAxis(s.first, iAAxisIndex::Z));
				if (coord[thisSliceAxis] == s.second)
				{
					skip = true;
					break;
				}
			}
			if (skip)
			{
				//++skipped;
				continue;
			}
			auto data = i->GetScalarComponentAsDouble(x, y, z, 0);
			values.push_back(data);
		}
		//LOG(lvlDebug, QString("%1: skipped %2").arg(slicerModeString(mode)).arg(skipped));
		slices.insert(std::make_pair(mode, sliceNr));
	}
};

iARawFileParamDlg::iARawFileParamDlg(QString const& fileName, QWidget* parent, QString const& title, QVariantMap & paramValues, bool brightTheme) :
	m_brightTheme(brightTheme),
	m_fileName(fileName),
	m_dataValues(std::make_unique<iASliceMergedValues>()),
	m_tf(std::make_unique<iATransferFunctionOwner>())
{
	QFileInfo info1(fileName);
	m_fileSize = info1.size();

	QStringList datatypeList(readableDataTypeList(false));
	selectOption(datatypeList, paramValues[iARawFileIO::DataTypeStr].toString());
	QStringList byteOrderList(iAByteOrder::stringList());
	selectOption(byteOrderList, paramValues[iARawFileIO::ByteOrderStr].toString());
	iAAttributes params;
	// duplication to iARawFileIO!
	addAttr(params, iARawFileIO::SizeStr, iAValueType::Vector3i, paramValues[iARawFileIO::SizeStr]);
	addAttr(params, iARawFileIO::SpacingStr, iAValueType::Vector3, paramValues[iARawFileIO::SpacingStr]);
	addAttr(params, iARawFileIO::OriginStr, iAValueType::Vector3, paramValues[iARawFileIO::OriginStr]);
	addAttr(params, iARawFileIO::HeadersizeStr, iAValueType::Discrete, paramValues[iARawFileIO::HeadersizeStr].toInt(), 0);
	addAttr(params, iARawFileIO::DataTypeStr, iAValueType::Categorical, datatypeList);
	addAttr(params, iARawFileIO::ByteOrderStr, iAValueType::Categorical, byteOrderList);

	auto fileNameLabel = new QLabel(QString("File Name: %1").arg(QFileInfo(fileName).fileName()));
	fileNameLabel->setToolTip(fileName);

	auto guessFromFileNameButton = new QPushButton("Guess parameters from filename");
	connect(guessFromFileNameButton, &QAbstractButton::clicked, this, [this, fileName] {
		guessParameters(fileName);
	});

	auto previewButton = new QPushButton(m_inputDlg);
	previewButton->setText("Preview >>");
	previewButton->setCheckable(true);
	connect(previewButton, &QPushButton::clicked, this, &iARawFileParamDlg::togglePreview);

	auto fileNameAndButtons = new QWidget();
	fileNameAndButtons->setLayout(createLayout<QHBoxLayout>());
	fileNameAndButtons->layout()->addWidget(fileNameLabel);
	fileNameAndButtons->layout()->addWidget(guessFromFileNameButton);
	fileNameAndButtons->layout()->addWidget(previewButton);

	auto actualSizeLabel = new QLabel("Actual file size: " + QString::number(m_fileSize) + " bytes");
	actualSizeLabel->setAlignment(Qt::AlignRight);

	m_proposedSizeLabel = new QLabel("Predicted file size: ");
	m_proposedSizeLabel->setAlignment(Qt::AlignRight);

	m_inputDlg = new iAParameterDlg(parent, title, params);
	m_inputDlg->mainWidget()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	m_inputDlg->layout()->addWidget(fileNameAndButtons);
	m_inputDlg->layout()->addWidget(actualSizeLabel);
	m_inputDlg->layout()->addWidget(m_proposedSizeLabel);

	connect(qobject_cast<iAVectorInput*>(m_inputDlg->paramWidget(iARawFileIO::SizeStr)), &iAVectorInput::valueChanged, this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QSpinBox*>(m_inputDlg->paramWidget(iARawFileIO::HeadersizeStr)), QOverload<int>::of(&QSpinBox::valueChanged), this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QComboBox*>(m_inputDlg->paramWidget(iARawFileIO::DataTypeStr)), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iARawFileParamDlg::checkFileSize);
	// only need to updatePreview, but also the parameter reading currently only contained in checkFileSize:
	connect(qobject_cast<QComboBox*>(m_inputDlg->paramWidget(iARawFileIO::ByteOrderStr)), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iARawFileParamDlg::checkFileSize);

	checkFileSize();

	if (m_inputDlg->exec() != QDialog::Accepted)
	{
		return;
	}
	auto newValues = m_inputDlg->parameterValues(); // maybe we can directly assign to paramValues?
	paramValues[iARawFileIO::SizeStr] = newValues[iARawFileIO::SizeStr];
	paramValues[iARawFileIO::SpacingStr] = newValues[iARawFileIO::SpacingStr];
	paramValues[iARawFileIO::OriginStr] = newValues[iARawFileIO::OriginStr];
	paramValues[iARawFileIO::HeadersizeStr] = newValues[iARawFileIO::HeadersizeStr];
	paramValues[iARawFileIO::DataTypeStr] = newValues[iARawFileIO::DataTypeStr];
	paramValues[iARawFileIO::ByteOrderStr] = newValues[iARawFileIO::ByteOrderStr];
	m_accepted = true;
}

void iARawFileParamDlg::checkFileSize()
{
	m_params = iARawFileParameters::fromMap(m_inputDlg->parameterValues());
	bool valid = m_params.has_value() && static_cast<qint64>(m_params->fileSize()) == m_fileSize;
	m_proposedSizeLabel->setText(m_params.has_value() ?
		"Predicted file size: " + QString::number(m_params->fileSize()) + " bytes" :
		"Invalid number(s) in size, data type or header size (too large/negative)?");
	m_proposedSizeLabel->setStyleSheet(
		QString("QLabel { background-color : %1; }").arg(valid ?
			(m_brightTheme ? "#BFB" : "#070"):
			(m_brightTheme ? "#FBB" : "#700") ));
	m_inputDlg->setOKEnabled(valid);
	updatePreview();
}

void iARawFileParamDlg::guessParameters(QString fileName)
{
	QRegularExpression sizeRegEx("(\\d+)x(\\d+)x(\\d+)");
	// TODO: also recognize trailing k/K for 1000
	auto sizeMatch = sizeRegEx.match(fileName);
	if (sizeMatch.hasMatch())
	{
		auto size = { sizeMatch.captured(1).toInt(), sizeMatch.captured(2).toInt(), sizeMatch.captured(3).toInt() };
		m_inputDlg->setValue(iARawFileIO::SizeStr, variantVector<int>(size));
	}
	QString spcGrp("(\\d+(?:[.-]\\d+)?)(um|mm)");
	QRegularExpression spcRegEx1(QString("%1x%2x%3").arg(spcGrp).arg(spcGrp).arg(spcGrp));
	auto spc1Match = spcRegEx1.match(fileName);
	if (spc1Match.hasMatch())
	{
		auto spc = { spc1Match.captured(1).replace("-", ".").toDouble(), spc1Match.captured(3).replace("-", ".").toDouble(), spc1Match.captured(5).replace("-", ".").toDouble() };
		m_inputDlg->setValue(iARawFileIO::SpacingStr, variantVector<double>(spc));
	}
	else
	{
		QRegularExpression spcRegEx2(spcGrp);
		auto spc2Match = spcRegEx2.match(fileName);
		if (spc2Match.hasMatch())
		{

			auto spc = { spc2Match.captured(1).replace("-", ".").toDouble(), spc2Match.captured(1).replace("-", ".").toDouble(), spc2Match.captured(1).replace("-", ".").toDouble() };
			m_inputDlg->setValue(iARawFileIO::SpacingStr, variantVector<double>(spc));
		}
	}
	QRegularExpression scalarTypeRegEx("(\\d+)bit");
	auto scalarTypeMatch = scalarTypeRegEx.match(fileName);
	if (scalarTypeMatch.hasMatch())
	{
		auto bits = scalarTypeMatch.captured(1);
		int vtkTypeID = (bits == "8") ? VTK_UNSIGNED_CHAR
			: (bits == "16")          ? VTK_UNSIGNED_SHORT
			: (bits == "32")          ? VTK_UNSIGNED_INT
			: (bits == "64")          ? VTK_UNSIGNED_LONG_LONG
									  : -1;
		if (vtkTypeID == -1)
		{
			LOG(lvlWarn, QString("Invalid bits string %1 cannot be resolved to a type").arg(bits));
		}
		else
		{
			QString scalarTypeStr = mapVTKTypeToReadableDataType(vtkTypeID);
			m_inputDlg->setValue(iARawFileIO::DataTypeStr, scalarTypeStr);
		}
	}
}

bool iARawFileParamDlg::accepted() const
{
	return m_accepted;
}

QVariantMap iARawFileParamDlg::parameterValues() const
{
	return m_inputDlg->parameterValues();
}

iARawFileParamDlg::~iARawFileParamDlg()
{
	delete m_inputDlg;
}

void iARawFileParamDlg::togglePreview()
{
	const size_t NumBins = 2048;
	m_previewShown = !m_previewShown;
	if (!m_previewShown)
	{
		m_previewWidth = m_inputDlg->width();
		m_previewContainer->hide();
		m_inputDlg->adjustSize();  // works better than any resize call
	}
	else
	{
		m_inputDlg->resize(m_previewWidth, m_inputDlg->height());   // restore previous width
		if (!m_previewContainer)
		{
			m_chart = new iAChartWithFunctionsWidget(m_inputDlg, "Greyvalue", "Frequency");
			m_previewContainer = new QWidget();
			auto slicersWidget = new QWidget();
			auto layout = createLayout<QHBoxLayout>();
			slicersWidget->setLayout(layout);
			for (int i = iASlicerMode::SlicerCount-1; i >= 0; --i)
			{
				m_slicer.push_back(std::make_shared<iARawFilePreviewSlicer>(static_cast<iASlicerMode>(i), m_fileName, m_tf->colorTF()));
				connect(m_slicer.back().get(), &iARawFilePreviewSlicer::loadDone, this, [this, i]
				{
					auto slicer = dynamic_cast<iARawFilePreviewSlicer*>(QObject::sender());
					m_dataValues->addValues(slicer->image(), static_cast<iASlicerMode>(i), slicer->sliceNr());
					m_chart->clearPlots();
					auto type = isVtkIntegerImage(slicer->image()) ? iAValueType::Discrete : iAValueType::Continuous;
					auto histogramData = iAHistogramData::create("Greyvalue",
						type, m_dataValues->values, NumBins);
					auto const tfRange = m_tf->colorTF()->GetRange();
					auto histRange = histogramData->xBounds();
					if (tfRange[0] != histRange[0] || tfRange[1] != histRange[1])
					{
						m_tf->resetFunctions(histRange);
					}
					m_chart->addPlot(std::make_shared<iABarGraphPlot>(histogramData, QApplication::palette().color(QPalette::Shadow)));
					m_chart->update();
				});
				connect(m_chart, &iAChartWithFunctionsWidget::transferFunctionChanged, m_slicer.back().get(), &iARawFilePreviewSlicer::updateColors);
				layout->addLayout(m_slicer.back()->layout());
			}
			m_previewContainer->setLayout(createLayout<QVBoxLayout>());
			m_previewContainer->layout()->addWidget(slicersWidget);

			m_chart->setTransferFunction(m_tf.get());
			m_previewContainer->layout()->addWidget(m_chart);
			m_inputDlg->mainLayout()->addWidget(m_previewContainer, 0, 1, m_inputDlg->mainLayout()->rowCount(), 1);
		}
		m_previewContainer->show();
		m_previewContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		updatePreview();
	}
}

void iARawFileParamDlg::updatePreview()
{
	if (!m_previewShown)
	{
		return;
	}
	m_dataValues->clear();
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (!m_params || static_cast<qint64>(m_params->fileSize()) != m_fileSize)
		{
			m_slicer[i]->setOutOfDate();
		}
		else
		{
			m_slicer[i]->loadImage(*m_params);
		}
	}
}
