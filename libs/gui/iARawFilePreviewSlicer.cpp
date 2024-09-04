// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARawFilePreviewSlicer.h"

#include <iALog.h>
#include <iAQVTKWidget.h>
#include <iAProgress.h>
#include <iARunAsync.h>
#include <iASlicerMode.h>
#include <iATypedCallHelper.h>
#include <iAVtkDraw.h>    // for iAvtkImageData

#include <vtkImageActor.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper3D.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <QCheckBox>
#include <QLabel>
#include <QObject>
#include <QProgressBar>
#include <QSpinBox>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

// source https://stackoverflow.com/a/4956493
template <typename T>
T swap_endian(T u)
{
	//static_assert (CHAR_BIT == 8, "CHAR_BIT != 8"); // can be assumed to be true on all systems we are building
	union
	{
		T u;
		unsigned char u8[sizeof(T)];
	} source, dest;

	source.u = u;

	for (size_t k = 0; k < sizeof(T); k++)
	{
		dest.u8[k] = source.u8[sizeof(T) - k - 1];
	}

	return dest.u;
}

namespace
{
	const QString StatusOutOfDate("Out of date");
	const QString StatusAborted("Aborted");
	const QString StatusDisabled("Disabled");
	const QString StatusUpToDate("Up to date");
	const int SliceNrInit = -1;
}

//! Internal data storage for the iARawFilePreviewSlicer class (pimpl-Idiom)
struct iARawFilePreviewSlicerImpl
{
	iASlicerMode mode;
	QString fileName;
	iAQVTKWidget* slicerWidget = new iAQVTKWidget();
	QProgressBar* progressBar = new QProgressBar();
	QVBoxLayout* boxLayout = new QVBoxLayout();
	QSpinBox* sliceSB = new QSpinBox();
	QStackedWidget* statusWidget = new QStackedWidget();
	QLabel* statusLabel = new QLabel();
	QCheckBox* label;
	iARawFileParameters params;
	vtkSmartPointer<vtkRenderer> imageRenderer = vtkSmartPointer<vtkRenderer>::New();
	vtkSmartPointer<vtkImageMapToColors> color = vtkSmartPointer<vtkImageMapToColors>::New();
	vtkScalarsToColors* tf;
	int sliceNr = SliceNrInit;
	bool validParams = false;
	bool enabled = false;
	QFutureWatcher<void>* fw = nullptr;
	vtkSmartPointer<iAvtkImageData> image;
	std::atomic_bool cancellationToken = false;
	QMetaObject::Connection showImgConn;
};

iARawFilePreviewSlicer::iARawFilePreviewSlicer(iASlicerMode mode, QString const& fileName, vtkScalarsToColors* tf) :
	m(std::make_shared<iARawFilePreviewSlicerImpl>())
{
	m->mode = mode;
	m->fileName = fileName;
	m->tf = tf;
	m->label = new QCheckBox(QString("%1").arg(slicerModeString(mode)));
	m->enabled = m->mode != iASlicerMode::YZ;
	m->label->setChecked(m->enabled);
	m->label->setMinimumWidth(50);
	m->label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m->sliceSB->setMinimum(0);
	QObject::connect(m->sliceSB, &QSpinBox::valueChanged, m->sliceSB, [this]
	{
		m->sliceNr = m->sliceSB->value();
		updateImage();
	});
	QObject::connect(m->label, &QCheckBox::toggled, this, [this]
	{
		m->enabled = m->label->isChecked();
		if (!m->enabled)
		{
			stopUpdate();
			setStatus(StatusDisabled);
			return;
		}
		updateImage();
	});
	m->slicerWidget->setMinimumHeight(50);
	auto headerLayout = new QHBoxLayout();
	headerLayout->addWidget(m->label);
	headerLayout->addWidget(m->sliceSB);

	auto window = m->slicerWidget->renderWindow();
	window->SetNumberOfLayers(2);
	auto imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
	window->GetInteractor()->SetInteractorStyle(imageStyle);

	auto abortButton = new QToolButton();
	abortButton->setProperty("qssClass", "tbAbort");
	connect(abortButton, &QToolButton::clicked, this, [this]
	{
		stopUpdate();
		setStatus(StatusAborted);
	});
	auto progressControls = new QWidget();
	progressControls->setLayout(createLayout<QHBoxLayout>());
	progressControls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	progressControls->layout()->addWidget(m->progressBar);
	progressControls->layout()->addWidget(abortButton);

	m->statusLabel->setAlignment(Qt::AlignHCenter);
	m->statusWidget->addWidget(m->statusLabel);
	m->statusWidget->addWidget(progressControls);
	m->statusWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	m->boxLayout->addLayout(headerLayout);
	m->boxLayout->addWidget(m->slicerWidget);
	m->boxLayout->addWidget(m->statusWidget);
	setStatus(m->enabled ? StatusOutOfDate : StatusDisabled);
}

iARawFilePreviewSlicer::~iARawFilePreviewSlicer()
{
	stopUpdate();
}

void iARawFilePreviewSlicer::setOutOfDate()
{
	if (m->enabled)
	{
		setStatus(StatusOutOfDate);
	}
}

void iARawFilePreviewSlicer::setStatus(QString const& status)
{
	m->statusLabel->setText(status);
	m->statusWidget->setCurrentIndex(0);
}

QVBoxLayout* iARawFilePreviewSlicer::layout()
{
	return m->boxLayout;
}

void iARawFilePreviewSlicer::loadImage(iARawFileParameters const& params)
{
	if (params == m->params)
	{
		return;
	}
	auto sliceAxisSize = params.size[mapSliceToGlobalAxis(m->mode, iAAxisIndex::Z)];
	QSignalBlocker sb(m->sliceSB);  // setMaximum might already trigger a changed value (if maximum is reduced)...
	m->sliceSB->setMaximum(sliceAxisSize - 1);
	if (m->sliceNr == SliceNrInit || static_cast<unsigned int>(m->sliceNr) >= sliceAxisSize)
	{
		m->sliceNr = sliceAxisSize / 2; // start with "middle" slice
		m->sliceSB->setValue(m->sliceNr);
	}
	m->params = params;
	m->validParams = true;
	updateImage();
}

void iARawFilePreviewSlicer::stopUpdate()
{
	if (!m->fw)
	{
		return;
	}
	bool disconnectAndWait = m->showImgConn;  // we want to evaluate operator bool on the connection, not copy it!
	if (disconnectAndWait)
	{
		QObject::disconnect(m->showImgConn);
	}
	m->cancellationToken = true;
	if (disconnectAndWait)
	{
		m->fw->waitForFinished();
		m->fw = nullptr;
	}
}


template <typename T>
void readImageSlice(iAProgress& progress, std::shared_ptr<iARawFilePreviewSlicerImpl> m)
{
	QFile file(m->fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
	{
		QString msg(QString("Failed to open file %1!").arg(m->fileName));
		throw std::runtime_error(msg.toStdString());
	}
	bool readSuccess = true;
	size_t curIdx = 0;
	int sliceXAxis = mapSliceToGlobalAxis(m->mode, iAAxisIndex::X);
	int sliceYAxis = mapSliceToGlobalAxis(m->mode, iAAxisIndex::Y);
	int sliceZAxis = mapSliceToGlobalAxis(m->mode, iAAxisIndex::Z);
	size_t totalValues = m->params.size[sliceXAxis] * m->params.size[sliceYAxis];
	int size[3] = { static_cast<int>(m->params.size[sliceXAxis]), static_cast<int>(m->params.size[sliceYAxis]), 1 };
	m->image = allocateiAImage(m->params.scalarType, size, m->params.spacing.data(), 1);
	const qint64 chunkSize = (m->mode == iASlicerMode::XY) ?
		m->params.size[sliceXAxis] * m->params.size[sliceYAxis] :  // read all in one chunk...
		(m->mode == iASlicerMode::XZ ? m->params.size[sliceXAxis] : 1);
	const qint64 bytesToRead = m->params.voxelBytes() * chunkSize;
	T* imgData = static_cast<T*>(m->image->GetScalarPointer());
	std::array<qint64, 3> curCoords;
	curCoords.fill(0);
	curCoords[sliceZAxis] = m->sliceNr;
	while (readSuccess && curIdx < totalValues && !m->cancellationToken)
	{
		qint64 idx = curCoords[0] + curCoords[1] * m->params.size[0] + curCoords[2] * m->params.size[0] * m->params.size[1];  // instead of recomputation each time, instead compute stride between chunks?
		qint64 seekPos = m->params.headerSize + (idx * m->params.voxelBytes());
		if (!file.seek(seekPos))
		{
			LOG(lvlError, QString("Seeking to position %1 failed while reading element %2 from file '%3' (error: %4)!")
				.arg(seekPos).arg(curIdx).arg(m->fileName).arg(file.errorString()));
			readSuccess = false;
		}
		auto result = file.read(reinterpret_cast<char*>(imgData + curIdx), bytesToRead);
		if (result != bytesToRead)
		{
			LOG(lvlError, QString("Could not read a full chunk of %1 bytes (only read %2 bytes) while reading element %3 from file '%4' (error: %5)!")
				.arg(bytesToRead).arg(result).arg(curIdx).arg(m->fileName).arg(file.errorString()));
			readSuccess = false;
		}
		curIdx += chunkSize;
		if (m->mode == iASlicerMode::XZ)
		{
			curCoords[2] += 1;
		}
		else if (m->mode == iASlicerMode::YZ)
		{
			curCoords[1] += 1;
			if (curCoords[1] >= m->params.size[1])
			{
				curCoords[1] = 0;
				curCoords[2] += 1;
			}
		}
		progress.emitProgress(100 * static_cast<double>(curIdx) / totalValues);
	}
	if (m->params.byteOrder == itk::CommonEnums::IOByteOrder::BigEndian)
	{
		for (size_t i = 0; i < totalValues; ++i)
		{
			imgData[i] = swap_endian(imgData[i]);
		}
	}
	auto minmax = std::minmax_element(imgData, imgData + totalValues);
	m->image->SetScalarRange(*minmax.first, *minmax.second);
	file.close();
}

void iARawFilePreviewSlicer::updateImage()
{
	if (!m->enabled)
	{
		return;
	}
	stopUpdate();
	// maybe protect method by a mutex to avoid (the slim chance of) starting two computations in parallel if triggered within a very short time?
	auto progress = std::make_shared<iAProgress>();
	QObject::connect(progress.get(), &iAProgress::progress, m->progressBar, &QProgressBar::setValue);
	m->progressBar->setValue(0);
	m->statusWidget->setCurrentIndex(1);
	m->cancellationToken = false;
	m->fw = new QFutureWatcher<void>(m->progressBar);
	m->showImgConn = QObject::connect(m->fw, &QFutureWatcher<void>::finished, this, &iARawFilePreviewSlicer::showImage);
	QObject::connect(m->fw, &QFutureWatcher<void>::finished, m->fw, &QFutureWatcher<void>::deleteLater);
	auto future = QtConcurrent::run([this, progress]()
	{
		VTK_TYPED_CALL(readImageSlice, m->params.scalarType, *progress.get(), m);
	});
	m->fw->setFuture(future);
}

void iARawFilePreviewSlicer::showImage()
{
	emit loadDone();
	m->fw = nullptr;
	setStatus(StatusUpToDate);
	m->color->SetLookupTable(m->tf);
	m->color->SetInputData(m->image);
	m->color->Update();
	auto imageActor = vtkSmartPointer<vtkImageActor>::New();
	imageActor->SetInputData(m->color->GetOutput());
	imageActor->GetMapper()->BorderOn();
	imageActor->SetInterpolate(false);

	auto window = m->slicerWidget->renderWindow();
	if (m->imageRenderer)
	{
		window->RemoveRenderer(m->imageRenderer);
	}
	m->imageRenderer->SetLayer(0);
	m->imageRenderer->AddActor(imageActor);
	m->imageRenderer->ResetCamera();

	m->label->setText(QString("%1 - #%2").arg(slicerModeString(m->mode)).arg(m->sliceNr));

	window->AddRenderer(m->imageRenderer);
	m->slicerWidget->updateAll();
}

vtkImageData * iARawFilePreviewSlicer::image() const
{
	return m->image;
}

int iARawFilePreviewSlicer::sliceNr() const
{
	return m->sliceNr;
}

void iARawFilePreviewSlicer::updateColors()
{
	if (!m->enabled || !m->color->GetInput())
	{
		return;
	}
	m->color->Update();
	m->slicerWidget->updateAll();
}
