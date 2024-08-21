// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARawFileParamDlg.h"

#include "iAAttributeDescriptor.h"    // for selectOption
#include "iARawFileIO.h"
#include "iAVtkDraw.h"    // for iAvtkImageData
#include "iALog.h"
#include "iAParameterDlg.h"
#include "iARawFileParameters.h"
#include "iAToolsVTK.h"    // for mapVTKTypeToReadableDataType, readableDataTypes, ...
#include "iAValueTypeVectorHelpers.h"

#include "iAVectorInput.h"

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QToolButton>

iARawFileParamDlg::iARawFileParamDlg(QString const& fileName, QWidget* parent, QString const& title, QVariantMap & paramValues, bool brightTheme) :
	m_brightTheme(brightTheme),
	m_fileName(fileName)
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
	m_inputDlg = new iAParameterDlg(parent, title, params);

	auto fileNameLabel = new QLabel(QString("File Name: %1").arg(QFileInfo(fileName).fileName()));
	fileNameLabel->setToolTip(fileName);
	auto fileNamePlusPreview = new QWidget();
	fileNamePlusPreview->setContentsMargins(0, 0, 0, 0);
	fileNamePlusPreview->setLayout(new QHBoxLayout);
	fileNamePlusPreview->layout()->setSpacing(4);
	fileNamePlusPreview->layout()->addWidget(fileNameLabel);
	m_inputDlg->layout()->addWidget(fileNamePlusPreview);

	auto guessFromFileNameButton = new QPushButton("Guess parameters from filename");
	m_inputDlg->layout()->addWidget(guessFromFileNameButton);
	connect(guessFromFileNameButton, &QAbstractButton::clicked, this, [this, fileName]{
		guessParameters(fileName);
	});

	auto actualSizeLabel = new QLabel("Actual file size: " + QString::number(m_fileSize) + " bytes");
	actualSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->layout()->addWidget(actualSizeLabel);

	m_proposedSizeLabel = new QLabel("Predicted file size: ");
	m_proposedSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->layout()->addWidget(m_proposedSizeLabel);

	connect(qobject_cast<iAVectorInput*>(m_inputDlg->paramWidget(iARawFileIO::SizeStr)), &iAVectorInput::valueChanged, this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QSpinBox*>(m_inputDlg->paramWidget(iARawFileIO::HeadersizeStr)), QOverload<int>::of(&QSpinBox::valueChanged), this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QComboBox*>(m_inputDlg->paramWidget(iARawFileIO::DataTypeStr)), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iARawFileParamDlg::checkFileSize);

	auto previewButton = new QToolButton(m_inputDlg);
	previewButton->setText("Preview >>");
	previewButton->setCheckable(true);
	fileNamePlusPreview->layout()->addWidget(previewButton);
	connect(previewButton, &QToolButton::pressed, this, &iARawFileParamDlg::togglePreview);

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

//maybe use iASlicer?

#include <iAQVTKWidget.h>
#include <iARunAsync.h>
#include <iASlicerMode.h>
#include <iATransferFunction.h>
#include <iATypedCallHelper.h>

#include <vtkActor.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageActor.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper3D.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <QProgressBar>

class iASlicerUIData
{
private:
	iAQVTKWidget* slicerWidget = new iAQVTKWidget();
	QProgressBar* progressBar = new QProgressBar();
	QVBoxLayout* boxLayout = new QVBoxLayout();
	iASlicerMode m_mode;
	iARawFileParameters m_lastParams;
	vtkSmartPointer<vtkRenderer> m_imageRenderer;
	QLabel* m_label;
	int m_sliceNr;
public:
	iASlicerUIData(iASlicerMode mode):
		m_mode(mode),
		m_label(new QLabel(QString("%1").arg(slicerModeString(mode))))
	{
		m_label->setMinimumWidth(50);
		m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		slicerWidget->setMinimumHeight(50);
		//slicerWidget->setWindowTitle(QString("%1").arg(name));

		auto window = slicerWidget->renderWindow();
		window->SetNumberOfLayers(2);
		auto imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
		window->GetInteractor()->SetInteractorStyle(imageStyle);

		boxLayout->addWidget(m_label);
		boxLayout->addWidget(slicerWidget);
		boxLayout->addWidget(progressBar);
	}
	QVBoxLayout* layout()
	{
		return boxLayout;
	}
	template <typename T>
	void readImageSlice(vtkSmartPointer<iAvtkImageData>& image, QString const& fileName, iARawFileParameters const& p, iAProgress& progress)
	{
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
		{
			QString msg(QString("Failed to open file %1!").arg(fileName));
			throw std::runtime_error(msg.toStdString());
		}
		bool readSuccess = true;
		size_t curIdx = 0;
		int sliceXAxis = mapSliceToGlobalAxis(m_mode, 0);
		int sliceYAxis = mapSliceToGlobalAxis(m_mode, 1);
		int sliceZAxis = mapSliceToGlobalAxis(m_mode, 2);
		size_t totalValues = p.size[sliceXAxis] * p.size[sliceYAxis];
		int size[3] = { static_cast<int>(p.size[sliceXAxis]), static_cast<int>(p.size[sliceYAxis]), 1 };
		image = allocateiAImage(p.scalarType, size, p.spacing.data(), 1);
		m_sliceNr = p.size[sliceZAxis] / 2; // for now read "middle" slice
		const qint64 chunkSize = (m_mode == iASlicerMode::XY) ?
			p.size[sliceXAxis] * p.size[sliceYAxis] :  // read all in one chunk...
			(m_mode == iASlicerMode::XZ ? p.size[sliceXAxis] : 1);
		const qint64 bytesToRead = p.voxelBytes() * chunkSize;
		T* imgData = static_cast<T*>(image->GetScalarPointer());
		std::array<qint64, 3> curCoords;
		curCoords.fill(0);
		curCoords[sliceZAxis] = m_sliceNr;
		LOG(lvlDebug, QString("Mode %1: chunkSize: %2:")
			.arg(slicerModeString(m_mode)).arg(chunkSize));
		int readCnt = 0;
		while (readSuccess && curIdx < totalValues)
		{
			qint64 seekPos = p.headerSize + curCoords[0] + curCoords[1] * p.size[0] + curCoords[2] * p.size[0] * p.size[1];
			LOG(lvlDebug, QString("    Mode %1: curIdx: %2; curCoords: %3; seekPos: %4; readCnt: %5")
				.arg(slicerModeString(m_mode)).arg(curIdx).arg(arrayToString(curCoords)).arg(seekPos).arg(readCnt));
			if (!file.seek(seekPos))
			{
				LOG(lvlError, QString("Seeking to position %1 failed while reading element %2 from file '%3' (error: %4)!")
					.arg(seekPos).arg(curIdx).arg(fileName).arg(file.errorString()));
				readSuccess = false;
			}
			auto result = file.read(reinterpret_cast<char*>(imgData + curIdx), bytesToRead);
			if (result != bytesToRead)
			{
				LOG(lvlError, QString("Could not read a full chunk of %1 bytes (only read %2 bytes) while reading element %3 from file '%4' (error: %5)!")
					.arg(bytesToRead).arg(result).arg(curIdx).arg(fileName).arg(file.errorString()));
				readSuccess = false;
			}
			curIdx += chunkSize;
			if (m_mode == iASlicerMode::XZ)
			{
				curCoords[2] += 1;
			}
			else if (m_mode == iASlicerMode::YZ)
			{
				curCoords[1] += 1;
				if (curCoords[1] >= p.size[1])
				{
					curCoords[1] = 0;
					curCoords[2] += 1;
				}
			}
			readCnt++;
			progress.emitProgress(100 * static_cast<double>(curIdx) / totalValues);
		}
		auto minmax = std::minmax_element(imgData, imgData + totalValues);
		image->SetScalarRange(*minmax.first, *minmax.second);
		file.close();
	}

	void loadImage(QString const& fileName, iARawFileParameters const & params)
	{
		if (params == m_lastParams)
		{
			return;
		}
		m_lastParams = params;
		auto image = std::make_shared<vtkSmartPointer<iAvtkImageData>>();  // additional level of indirection required to be able to modify smartpointer
		auto progress = std::make_shared<iAProgress>();
		QObject::connect(progress.get(), &iAProgress::progress, progressBar, &QProgressBar::setValue);
		auto fw = runAsync([this, fileName, params, image, progress]()
		{
			VTK_TYPED_CALL(readImageSlice, params.scalarType, *image, fileName, params, *progress.get());
		}, [this, image]
		{
			progressBar->hide();
			auto color = vtkSmartPointer<vtkImageMapToColors>::New();
			auto range = (*image)->GetScalarRange();
			LOG(lvlDebug, QString("Slicer %1, range: %2").arg(slicerModeString(m_mode)).arg(arrayToString(range, 2)));
			auto table = defaultColorTF(range);
			color->SetLookupTable(table);
			color->SetInputData(*image);
			color->Update();
			auto imageActor = vtkSmartPointer<vtkImageActor>::New();
			imageActor->SetInputData(color->GetOutput());
			imageActor->GetMapper()->BorderOn();
			imageActor->SetInterpolate(false);

			auto window = slicerWidget->renderWindow();
			if (m_imageRenderer)
			{
				window->RemoveRenderer(m_imageRenderer);
			}
			m_imageRenderer = vtkSmartPointer<vtkRenderer>::New();
			m_imageRenderer->SetLayer(0);
			m_imageRenderer->AddActor(imageActor);
			m_imageRenderer->ResetCamera();

			m_label->setText(QString("%1 - # %2").arg(slicerModeString(m_mode)).arg(m_sliceNr));

			window->AddRenderer(m_imageRenderer);
			slicerWidget->updateAll();
		}, progressBar);

		/*
		auto roiMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		roiMapper->SetInputConnection(roiSource->GetOutputPort());
		auto roiActor = vtkSmartPointer<vtkActor>::New();
		roiActor->SetVisibility(true);
		roiActor->SetMapper(roiMapper);
		roiActor->GetProperty()->SetColor(1, 0, 0);
		roiActor->GetProperty()->SetOpacity(1);
		roiActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);
		roiMapper->Update();

		auto roiRenderer = vtkSmartPointer<vtkRenderer>::New();
		roiRenderer->SetLayer(1);
		roiRenderer->AddActor(roiActor);
		roiRenderer->SetInteractive(false);
		roiRenderer->SetActiveCamera(imageRenderer->GetActiveCamera());

		window->AddRenderer(roiRenderer);
		*/
	}
};

void iARawFileParamDlg::togglePreview()
{
	m_previewShown = !m_previewShown;
	if (!m_previewShown)
	{
		m_previewWidth = m_previewContainer->width();
		m_previewContainer->hide();
		auto newWidth = m_inputDlg->width() - m_previewWidth;
		m_inputDlg->resize(newWidth, m_inputDlg->height());
	}
	else
	{
		if (!m_previewContainer)
		{
			m_previewContainer = new QWidget();
			m_previewContainer->setContentsMargins(0, 0, 0, 0);
			auto layout = new QHBoxLayout;
			layout->setSpacing(4);
			m_previewContainer->setLayout(layout);
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
			{
				m_slicer.push_back(std::make_shared<iASlicerUIData>(static_cast<iASlicerMode>(i)));
				layout->addLayout(m_slicer[i]->layout());
			}
			m_inputDlg->mainLayout()->addWidget(m_previewContainer, 0, 1, m_inputDlg->formLayout()->rowCount(), 1);
		}
		m_previewContainer->show();
		m_inputDlg->resize(m_inputDlg->width() + m_previewWidth, m_inputDlg->height());
		updatePreview();
	}
}

void iARawFileParamDlg::updatePreview()
{
	// TODO: clear slice view if invalid params now but valid before?
	if (!m_previewShown || !m_params || static_cast<qint64>(m_params->fileSize()) != m_fileSize)
	{
		return;
	}
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		m_slicer[i]->loadImage(m_fileName, *m_params);
	}
}
