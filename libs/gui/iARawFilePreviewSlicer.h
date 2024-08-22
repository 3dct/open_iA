#pragma once

//maybe use iASlicer?

#include <iALog.h>
#include <iAQVTKWidget.h>
#include <iAProgress.h>
#include <iARunAsync.h>
#include <iASlicerMode.h>
#include <iATransferFunction.h>
#include <iATypedCallHelper.h>
#include <iAVtkDraw.h>    // for iAvtkImageData

#include <vtkActor.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper3D.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <QLabel>
#include <QObject>
#include <QProgressBar>
#include <QSpinBox>
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

template <typename LayoutType>
LayoutType* createLayout()
{
	auto layout = new LayoutType();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);
	return layout;
}

class iARawFilePreviewSlicer: public QObject
{
	Q_OBJECT
private:
	iASlicerMode m_mode;
	QString m_fileName;
	iAQVTKWidget* slicerWidget = new iAQVTKWidget();
	QProgressBar* progressBar = new QProgressBar();
	QVBoxLayout* boxLayout = new QVBoxLayout();
	QSpinBox* m_sliceSB = new QSpinBox();
	QWidget* m_progressControls = new QWidget();
	QLabel* m_label;
	iARawFileParameters m_params;
	vtkSmartPointer<vtkRenderer> m_imageRenderer;
	const int SliceNrInit = -1;
	int m_sliceNr = SliceNrInit;
	bool m_validParams = false;
	QFutureWatcher<void>* m_fw = nullptr;
	vtkSmartPointer<iAvtkImageData> m_image;
	std::atomic_bool cancellation_token = ATOMIC_VAR_INIT(false);
public:
	iARawFilePreviewSlicer(iASlicerMode mode, QString const& fileName) :
		m_mode(mode),
		m_fileName(fileName),
		m_label(new QLabel(QString("%1").arg(slicerModeString(mode))))
	{
		m_label->setMinimumWidth(50);
		m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		m_sliceSB->setMinimum(0);
		QObject::connect(m_sliceSB, &QSpinBox::valueChanged, m_sliceSB, [this]
			{
				m_sliceNr = m_sliceSB->value();
				updateImage();
			});
		slicerWidget->setMinimumHeight(50);
		auto headerLayout = new QHBoxLayout();
		headerLayout->addWidget(m_label);
		headerLayout->addWidget(m_sliceSB);

		auto window = slicerWidget->renderWindow();
		window->SetNumberOfLayers(2);
		auto imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
		window->GetInteractor()->SetInteractorStyle(imageStyle);

		auto abortButton = new QToolButton();
		abortButton->setProperty("qssClass", "tbAbort");
		connect(abortButton, &QToolButton::clicked, this, [this]
		{
			stopUpdate();
			m_progressControls->hide();
		});
		m_progressControls->setLayout(createLayout<QHBoxLayout>());
		m_progressControls->layout()->addWidget(progressBar);
		m_progressControls->layout()->addWidget(abortButton);

		boxLayout->addLayout(headerLayout);
		boxLayout->addWidget(slicerWidget);
		boxLayout->addWidget(m_progressControls);
	}
	~iARawFilePreviewSlicer()
	{
		stopUpdate();
	}
	QVBoxLayout* layout()
	{
		return boxLayout;
	}
	template <typename T>
	void readImageSlice(iAProgress& progress)
	{
		QFile file(m_fileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
		{
			QString msg(QString("Failed to open file %1!").arg(m_fileName));
			throw std::runtime_error(msg.toStdString());
		}
		bool readSuccess = true;
		size_t curIdx = 0;
		int sliceXAxis = mapSliceToGlobalAxis(m_mode, 0);
		int sliceYAxis = mapSliceToGlobalAxis(m_mode, 1);
		int sliceZAxis = mapSliceToGlobalAxis(m_mode, 2);
		size_t totalValues = m_params.size[sliceXAxis] * m_params.size[sliceYAxis];
		int size[3] = { static_cast<int>(m_params.size[sliceXAxis]), static_cast<int>(m_params.size[sliceYAxis]), 1 };
		m_image = allocateiAImage(m_params.scalarType, size, m_params.spacing.data(), 1);
		const qint64 chunkSize = (m_mode == iASlicerMode::XY) ?
			m_params.size[sliceXAxis] * m_params.size[sliceYAxis] :  // read all in one chunk...
			(m_mode == iASlicerMode::XZ ? m_params.size[sliceXAxis] : 1);
		const qint64 bytesToRead = m_params.voxelBytes() * chunkSize;
		T* imgData = static_cast<T*>(m_image->GetScalarPointer());
		std::array<qint64, 3> curCoords;
		curCoords.fill(0);
		curCoords[sliceZAxis] = m_sliceNr;
		int readCnt = 0;
		while (readSuccess && curIdx < totalValues && !cancellation_token)
		{
			QThread::sleep(1);
			qint64 idx = curCoords[0] + curCoords[1] * m_params.size[0] + curCoords[2] * m_params.size[0] * m_params.size[1];  // instead of recomputation each time, instead compute stride between chunks?
			qint64 seekPos = m_params.headerSize + (idx * m_params.voxelBytes());
			if (!file.seek(seekPos))
			{
				LOG(lvlError, QString("Seeking to position %1 failed while reading element %2 from file '%3' (error: %4)!")
					.arg(seekPos).arg(curIdx).arg(m_fileName).arg(file.errorString()));
				readSuccess = false;
			}
			auto result = file.read(reinterpret_cast<char*>(imgData + curIdx), bytesToRead);
			if (result != bytesToRead)
			{
				LOG(lvlError, QString("Could not read a full chunk of %1 bytes (only read %2 bytes) while reading element %3 from file '%4' (error: %5)!")
					.arg(bytesToRead).arg(result).arg(curIdx).arg(m_fileName).arg(file.errorString()));
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
				if (curCoords[1] >= m_params.size[1])
				{
					curCoords[1] = 0;
					curCoords[2] += 1;
				}
			}
			readCnt++;
			progress.emitProgress(100 * static_cast<double>(curIdx) / totalValues);
		}
		if (m_params.byteOrder == itk::CommonEnums::IOByteOrder::BigEndian)
		{
			for (size_t i = 0; i < totalValues; ++i)
			{
				imgData[i] = swap_endian(imgData[i]);
			}
		}
		auto minmax = std::minmax_element(imgData, imgData + totalValues);
		m_image->SetScalarRange(*minmax.first, *minmax.second);
		file.close();
	}

	void loadImage(iARawFileParameters const& params)
	{
		if (params == m_params)
		{
			return;
		}
		auto sliceAxisSize = params.size[mapSliceToGlobalAxis(m_mode, 2)];
		QSignalBlocker sb(m_sliceSB);  // setMaximum might already trigger a changed value (if maximum is reduced)...
		m_sliceSB->setMaximum(sliceAxisSize - 1);
		if (m_sliceNr == SliceNrInit || m_sliceNr >= sliceAxisSize)
		{
			m_sliceNr = sliceAxisSize / 2; // start with "middle" slice
			m_sliceSB->setValue(m_sliceNr);
		}
		m_params = params;
		m_validParams = true;
		updateImage();
	}
	void stopUpdate()
	{
		if (!m_fw)
		{
			return;
		}
		QObject::disconnect(m_fw, &QFutureWatcher<void>::finished, this, &iARawFilePreviewSlicer::showImage);
		cancellation_token = true;
		m_fw->waitForFinished();
		m_fw = nullptr;
	}
	void updateImage()
	{	// maybe protect method by a mutex to avoid the (slim chance of) starting two computations in parallel if triggered within a very short time?
		auto progress = std::make_shared<iAProgress>();
		QObject::connect(progress.get(), &iAProgress::progress, progressBar, &QProgressBar::setValue);
		stopUpdate();
		progressBar->setValue(0);
		m_progressControls->show();
		cancellation_token = false;
		m_fw = new QFutureWatcher<void>(progressBar);
		QObject::connect(m_fw, &QFutureWatcher<void>::finished, this, &iARawFilePreviewSlicer::showImage);
		QObject::connect(m_fw, &QFutureWatcher<void>::finished, m_fw, &QFutureWatcher<void>::deleteLater);
		auto future = QtConcurrent::run([this, progress]()
		{
			VTK_TYPED_CALL(readImageSlice, m_params.scalarType, *progress.get());
		});
		m_fw->setFuture(future);
	}
	void showImage()
	{
		m_fw = nullptr;
		m_progressControls->hide();
		auto color = vtkSmartPointer<vtkImageMapToColors>::New();
		auto range = m_image->GetScalarRange();
		auto table = defaultColorTF(range);
		color->SetLookupTable(table);
		color->SetInputData(m_image);
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

		m_label->setText(QString("%1 - #%2").arg(slicerModeString(m_mode)).arg(m_sliceNr));

		window->AddRenderer(m_imageRenderer);
		slicerWidget->updateAll();

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
