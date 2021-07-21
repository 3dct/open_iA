/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iANModalDilationBackgroundRemover.h"

#include "iANModalDisplay.h"
#include "iANModalProgressWidget.h"

#include <iAConnector.h>
#include <iAModality.h>
#include <iAProgress.h>
#include <iASlicer.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkMultiplyImageFilter.h>

#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <QDialog>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QVBoxLayout>

template <class T>
void iANModalDilationBackgroundRemover::itkBinaryThreshold(iAConnector& conn, int loThresh, int upThresh)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<unsigned short, DIM> OutputImageType;
	typedef itk::BinaryThresholdImageFilter<InputImageType, OutputImageType> BTIFType;

	auto binThreshFilter = BTIFType::New();
	binThreshFilter->SetLowerThreshold(loThresh);
	binThreshFilter->SetUpperThreshold(upThresh);
	binThreshFilter->SetOutsideValue(BACKGROUND);
	binThreshFilter->SetInsideValue(FOREGROUND);
	binThreshFilter->SetInput(dynamic_cast<InputImageType*>(conn.itkImage()));
	//filter->progress()->observe(binThreshFilter);
	binThreshFilter->Update();

	m_itkTempImg = binThreshFilter->GetOutput();
	conn.setImage(m_itkTempImg);
}

void iANModalIterativeDilationThread::itkDilateAndCountConnectedComponents(
	ImagePointer itkImgPtr, int& connectedComponentsOut, bool dilate /*= true*/)
{
	typedef itk::Image<unsigned short, DIM> ImageType;
	typedef itk::ConnectedComponentImageFilter<ImageType, ImageType> CCIFType;

	typedef itk::FlatStructuringElement<DIM> StructuringElementType;
	typename StructuringElementType::RadiusType elementRadius;
	elementRadius.Fill(1);
	auto structuringElement = StructuringElementType::Ball(elementRadius);
	typedef itk::BinaryDilateImageFilter<ImageType, ImageType, StructuringElementType> BDIFType;

	typename BDIFType::Pointer dilationFilter;

	auto connCompFilter = CCIFType::New();
	connCompFilter->SetBackgroundValue(iANModalBackgroundRemover::BACKGROUND);
	m_progCc->observe(connCompFilter);

	auto input = dynamic_cast<ImageType*>(itkImgPtr.GetPointer());
	if (dilate)
	{
		// dilate the background (region of higher intensity)...
		dilationFilter = BDIFType::New();
		dilationFilter->SetDilateValue(iANModalBackgroundRemover::BACKGROUND);
		dilationFilter->SetKernel(structuringElement);
		dilationFilter->SetInput(input);
		m_progDil->observe(dilationFilter);

		connCompFilter->SetInput(dilationFilter->GetOutput());
	}
	else
	{
		connCompFilter->SetInput(input);
	}

	// ...until the number of foreground components is equal to connectedComponents
	connCompFilter->Update();

	//filter->addOutput(binThreshFilter->GetOutput());
	//conn.setImage(binThreshFilter->GetOutput());
	m_mask = connCompFilter->GetOutput();
	//m_mask = dilationFilter->GetOutput();

	connectedComponentsOut = connCompFilter->GetObjectCount();
}

void iANModalIterativeDilationThread::itkCountConnectedComponents(ImagePointer itkImgPtr, int& connectedComponentsOut)
{
	itkDilateAndCountConnectedComponents(itkImgPtr, connectedComponentsOut, false);
	return;
	/*
	typedef itk::Image<unsigned short, DIM> ImageType;
	typedef itk::ConnectedComponentImageFilter<ImageType, ImageType> CCIFType;

	auto connCompFilter = CCIFType::New();
	connCompFilter->SetBackgroundValue(iANModalBackgroundRemover::BACKGROUND);
	m_progCc->observe(connCompFilter);

	auto input = dynamic_cast<ImageType *>(itkImgPtr.GetPointer());
	connCompFilter->SetInput(input);

	connCompFilter->Update();

	//m_mask = connCompFilter->GetOutput();
	connectedComponentsOut = connCompFilter->GetObjectCount();
	*/
}

void iANModalIterativeDilationThread::itkDilate(ImagePointer itkImgPtr)
{
	typedef itk::Image<unsigned short, DIM> ImageType;

	typedef itk::FlatStructuringElement<DIM> StructuringElementType;
	typename StructuringElementType::RadiusType elementRadius;
	elementRadius.Fill(1);
	auto structuringElement = StructuringElementType::Ball(elementRadius);

	typedef itk::BinaryDilateImageFilter<ImageType, ImageType, StructuringElementType> BDIFType;
	typename BDIFType::Pointer dilationFilter;

	auto input = dynamic_cast<ImageType*>(itkImgPtr.GetPointer());
	dilationFilter = BDIFType::New();
	dilationFilter->SetDilateValue(iANModalBackgroundRemover::BACKGROUND);
	dilationFilter->SetKernel(structuringElement);
	dilationFilter->SetInput(input);
	m_progDil->observe(dilationFilter);

	dilationFilter->Update();
	m_mask = dilationFilter->GetOutput();
}

void iANModalIterativeDilationThread::itkErodeAndInvert(ImagePointer itkImgPtr, int count)
{
	typedef itk::FlatStructuringElement<DIM> StructuringElementType;
	typedef itk::Image<unsigned short, DIM> ImageType;
	typedef itk::BinaryErodeImageFilter<ImageType, ImageType, StructuringElementType> BDIFType;

	typename StructuringElementType::RadiusType elementRadius;
	elementRadius.Fill(1);
	auto structuringElement = StructuringElementType::Ball(elementRadius);

	if (count <= 0)
	{
		m_mask = itkImgPtr;
		return;
	}

	std::vector<typename BDIFType::Pointer> erosionFilters(count);
	auto input = dynamic_cast<ImageType*>(itkImgPtr.GetPointer());
	for (int i = 0; i < count; i++)
	{
		erosionFilters[i] = BDIFType::New();
		erosionFilters[i]->SetKernel(structuringElement);
		erosionFilters[i]->SetErodeValue(iANModalBackgroundRemover::BACKGROUND);
		erosionFilters[i]->SetBackgroundValue(iANModalBackgroundRemover::FOREGROUND);
		erosionFilters[i]->SetInput(i == 0 ? input : erosionFilters[i - 1]->GetOutput());
		m_progEro->observe(erosionFilters[i]);
	}

	typedef itk::InvertIntensityImageFilter<ImageType> IIIFType;
	auto invertFilter = IIIFType::New();
	invertFilter->SetMaximum(iANModalBackgroundRemover::FOREGROUND);
	invertFilter->SetInput(erosionFilters[count - 1]->GetOutput());

	invertFilter->Update();

	m_mask = invertFilter->GetOutput();

	//erosionFilters[count - 1]->Update();
	//m_mask = erosionFilters[count - 1]->GetOutput();
}

#ifndef NDEBUG
void iANModalDilationBackgroundRemover::showMask(QSharedPointer<iAModality> mod, vtkSmartPointer<vtkImageData> mask)
{
	QList<QSharedPointer<iAModality>> mods;
	mods.append(mod);
	auto display = new iANModalDisplay(new QWidget(), m_mdiChild, mods);
	uint cid = display->createChannel();
	auto cd = iAChannelData("Binary mask for " + mod->name(), mask, m_colorTf, m_opacityTf);
	display->setChannelData(cid, cd);
	iANModalDisplay::selectModalities(display);
}
void iANModalDilationBackgroundRemover::showMask(ImagePointer itkImgPtr)
{
	auto c = iAConnector();
	c.setImage(itkImgPtr);
	auto vtkImg = c.vtkImage();

	QList<QSharedPointer<iAModality>> mods;
	mods.append(QSharedPointer<iAModality>(new iAModality("Binary mask", "", -1, vtkImg, iAModality::NoRenderer)));
	auto display = new iANModalDisplay(new QWidget(), m_mdiChild, mods);
	auto cd = iAChannelData("temp", vtkImg, m_colorTf, m_opacityTf);
	display->setChannelData(iANModalDisplay::MAIN_CHANNEL_ID, cd);
	iANModalDisplay::selectModalities(display);
}
#endif

iANModalDilationBackgroundRemover::iANModalDilationBackgroundRemover(iAMdiChild* mdiChild) : m_mdiChild(mdiChild)
{
	m_colorTf = vtkSmartPointer<vtkLookupTable>::New();
	m_colorTf->SetNumberOfTableValues(2);
	m_colorTf->SetRange(0, 1);
	m_colorTf->SetTableValue(0.0, 0.0, 0.0, 0.0, 0.0);
	m_colorTf->SetTableValue(1.0, 1.0, 0.0, 0.0, 0.5);
	m_colorTf->Build();
}

iANModalBackgroundRemover::Mask iANModalDilationBackgroundRemover::removeBackground(
	const QList<QSharedPointer<iAModality>>& modalities)
{
	QSharedPointer<iAModality> selectedMod;
	iANModalBackgroundRemover::MaskMode maskMode;
	int upThresh;
	//int loThresh = 0;
	int regionCountGoal = 1;

	bool skipped = !selectModalityAndThreshold(nullptr, modalities, upThresh, selectedMod, maskMode);
	if (skipped)
	{
		return {nullptr, INVALID};
	}

	iAConnector conn;
	//conn.setImage(mod->image());
	conn.setImage(m_itkTempImg);
	//ImagePointer itkImgPtr = conn.itkImage();

	if (conn.itkPixelType() == itk::ImageIOBase::SCALAR)
	{
		// TODO
	}
	else
	{  // example if == itk::ImageIOBase::RGBA
		return {nullptr, INVALID};
	}

	bool success = iterativeDilation(m_itkTempImg, regionCountGoal);
	if (success)
	{
		conn.setImage(m_itkTempImg);

		auto castFilter = vtkSmartPointer<vtkImageCast>::New();
		castFilter->SetInputData(conn.vtkImage());
		castFilter->SetOutputScalarTypeToUnsignedChar();
		castFilter->Update();
		auto output = vtkSmartPointer<vtkImageData>(castFilter->GetOutput());

		//return conn.vtkImage(); // If doesn't work, keep iAConnector alive (member variable)
		return {output, maskMode};
	}
	else
	{
		return {nullptr, INVALID};
	}
}

// return - true if a modality and a threshold were successfully chosen
//        - false otherwise
bool iANModalDilationBackgroundRemover::selectModalityAndThreshold(QWidget* parent,
	const QList<QSharedPointer<iAModality>>& modalities, int& out_threshold, QSharedPointer<iAModality>& out_modality,
	iANModalBackgroundRemover::MaskMode& out_maskMode)
{
	QDialog* dialog = new QDialog(parent);
	// TODO: set dialog title
	dialog->setModal(true);

	auto layout = new QVBoxLayout(dialog);

	auto displayWidget = new QWidget(dialog);
	auto displayLayout = new QVBoxLayout(displayWidget);
	{
		auto displayLabel = new QLabel(
			"Select modality for the thresholding step of the dilation-based background removal", displayWidget);

		m_display = new iANModalDisplay(displayWidget, m_mdiChild, modalities, 1, 1);
		m_threholdingMaskChannelId = m_display->createChannel();
		//connect(m_display, SIGNAL(selectionChanged()), this, SLOT(updateThreshold()));
		connect(m_display, &iANModalDisplay::selectionChanged, this,
			&iANModalDilationBackgroundRemover::updateModalitySelected);

		displayLayout->addWidget(displayLabel);
		displayLayout->addWidget(m_display);
		displayLayout->setSpacing(0);

		//displayWidget->setStyleSheet("border: 1px solid black");
		displayWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	}

	auto thresholdWidget = new QWidget(dialog);
	auto thresholdLayout = new QVBoxLayout(thresholdWidget);
	{
		auto thresholdLabel = new QLabel("Set threshold", thresholdWidget);

		m_threshold = new iANModalThresholdingWidget(thresholdWidget);
		connect(m_threshold, &iANModalThresholdingWidget::thresholdChanged, this,
			&iANModalDilationBackgroundRemover::updateThreshold);

		thresholdLayout->addWidget(thresholdLabel);
		thresholdLayout->addWidget(m_threshold);
		thresholdLayout->setSpacing(0);

		thresholdWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	}

	layout->addWidget(displayWidget);
	layout->setStretchFactor(displayWidget, 1);

	layout->addWidget(thresholdWidget);
	layout->setStretchFactor(thresholdWidget, 0);

	const QString REMOVE = "Remove background";
	const QString HIDE = "Hide backgrond";
	const QString SKIP = "Skip";

	//QWidget *footerWidget = iANModalDisplay::createOkSkipFooter(dialog);
	iANModalDisplay::Footer* footerWidget = iANModalDisplay::createFooter(dialog, {REMOVE, HIDE}, {SKIP});
	layout->addWidget(footerWidget);
	layout->setStretchFactor(footerWidget, 0);

	auto dialogCode = dialog->exec();
	if (dialogCode == QDialog::Rejected)
	{
		return false;
	}

	const QString t = footerWidget->m_textOfButtonClicked;
	out_maskMode = t == REMOVE ? MaskMode::REMOVE : (t == HIDE ? MaskMode::HIDE : MaskMode::INVALID);

	LOG(lvlDebug, QString("Option chosen: ") + t + QString("\n"));

	out_threshold = m_threshold->threshold();
	out_modality = m_display->singleSelection();

	return true;
}

void iANModalDilationBackgroundRemover::updateModalitySelected()
{
	setModalitySelected(m_display->singleSelection());
}

void iANModalDilationBackgroundRemover::setModalitySelected(QSharedPointer<iAModality> modality)
{
	double range[2];
	modality->image()->GetScalarRange(range);
	double min = range[0];
	double max = range[1];
	int value = round(max - min) / 2 + min;

	m_threshold->spinBox()->setRange(min, max);
	m_threshold->spinBox()->setValue(value);

	m_threshold->slider()->setRange(min, max);
	m_threshold->slider()->setValue(value);

	//updateThreshold(); // already happens at spinBox()->setValue(value)
}

void iANModalDilationBackgroundRemover::updateThreshold()
{
	auto modality = m_display->singleSelection();
	int threshold = m_threshold->threshold();

	iAConnector conn;
	conn.setImage(modality->image());
	ITK_TYPED_CALL(itkBinaryThreshold, conn.itkScalarPixelType(), conn, 0, threshold);
	auto mask = conn.vtkImage();

	auto channelData = iAChannelData("Threshold mask", mask, m_colorTf);
	m_display->setChannelData(m_threholdingMaskChannelId, channelData);
}

bool iANModalDilationBackgroundRemover::iterativeDilation(ImagePointer mask, int regionCountGoal)
{
#ifndef NDEBUG
	storeImage(mask, "thresholded.mhd", true);
#endif

	auto pw = new iANModalProgressWidget();

	auto plot = new iANModalIterativeDilationPlot(pw);

	pw->addProgressBar(3, "Total progress", true, "status");  // 3 steps: counts; counts+dilations; erosions
	pw->addProgressBar(100, "Dilation", false, "dil");        // 100 steps, because that's how iAProgress works
	pw->addProgressBar(100, "Count connected components", false, "cc");
	pw->addProgressBar(100, "Erosion", false, "ero");
	pw->addSeparator();
	pw->addWidget(new QLabel("Target number of background regions: " + QString::number(regionCountGoal)), nullptr, 0);
	pw->addWidget(plot);

	constexpr int PROGS = 3;
	iAProgress* progs[PROGS] = {new iAProgress(), new iAProgress(), new iAProgress()};

	auto thread = new iANModalIterativeDilationThread(pw, progs, mask, regionCountGoal);
	connect(thread, &iANModalIterativeDilationThread::addValue, plot, &iANModalIterativeDilationPlot::addValue);
	connect(thread, &QThread::finished, thread, &QObject::deleteLater);
	connect(thread, &QThread::finished, pw, &QObject::deleteLater);
	for (int i = 0; i < PROGS; i++)
	{
		connect(thread, &QThread::finished, progs[i], &QObject::deleteLater);
	}
	connect(progs[0], &iAProgress::progress, pw, [pw](int p) { pw->setValue("dil", p); });
	connect(progs[1], &iAProgress::progress, pw, [pw](int p) { pw->setValue("cc", p); });
	connect(progs[2], &iAProgress::progress, pw, [pw](int p) { pw->setValue("ero", p); });

	// Now begin the heavy computation
	thread->start();

	// Progress dialog will close automatically once everything is finished
	pw->showDialog();
	if (pw->isCanceled())
	{
		thread->setCanceled(true);
	}
	else
	{
		m_itkTempImg = thread->mask();
	}

	return !pw->isCanceled();
}

// ----------------------------------------------------------------------------------------------
// iANModalIterativeDilationThread
// ----------------------------------------------------------------------------------------------

iANModalIterativeDilationThread::iANModalIterativeDilationThread(
	iANModalProgressWidget* progressWidget, iAProgress* progress[3], ImagePointer mask, int regionCountGoal) :
	iANModalProgressUpdater(progressWidget),
	m_progDil(progress[0]),
	m_progCc(progress[1]),
	m_progEro(progress[2]),
	m_mask(mask),
	m_regionCountGoal(regionCountGoal)
{
}

void iANModalIterativeDilationThread::setCanceled(bool c)
{
	m_canceled = c;
}

#ifndef IANMODAL_REQUIRE_NCANCELED
#define IANMODAL_REQUIRE_NCANCELED() \
	if (m_canceled)                  \
	{                                \
		return;                      \
	}                                \
	while (0)
#endif

void iANModalIterativeDilationThread::run()
{
	int connectedComponents;

	emit setValue("dil", 100);
	emit setValue("ero", 100);

	IANMODAL_REQUIRE_NCANCELED();
	itkCountConnectedComponents(m_mask, connectedComponents);
	//storeImage(m_itkTempImg, "connectedComponents.mhd", true);
	emit addValue(connectedComponents);
	emit setValue("dil", 0);
	emit setValue("cc", 0);

	emit setValue("status", 1);
	int dilationCount = 0;
	while (connectedComponents > m_regionCountGoal)
	{
		emit setValue("dil", 0);
		emit setValue("cc", 0);
		auto d = QString::number(dilationCount + 1);
		auto c = QString::number(connectedComponents);
		auto t = QString::number(m_regionCountGoal);
		IANMODAL_REQUIRE_NCANCELED();
		itkDilateAndCountConnectedComponents(m_mask, connectedComponents);
		//itkDilate(m_mask);
		//itkCountConnectedComponents(m_mask, connectedComponents);
		//storeImage(m_itkTempImg, "connectedComponents" + QString::number(dilationCount) + ".mhd", true);
		emit addValue(connectedComponents);
		dilationCount++;
	}

#ifndef NDEBUG
	//storeImage2(m_mask, "mask_after_dilations", true);
#endif

	emit setValue("ero", 0);

	emit setValue("status", 2);
	emit setText("ero", "Erosion (" + QString::number(dilationCount) + ")");
	IANMODAL_REQUIRE_NCANCELED();
	itkErodeAndInvert(m_mask, dilationCount);

#ifndef NDEBUG
	//storeImage2(m_mask, "mask_after_erosions", true);
#endif

	IANMODAL_REQUIRE_NCANCELED();
	emit finish();
}

// ----------------------------------------------------------------------------------------------
// iANModalThresholdingWidget
// ----------------------------------------------------------------------------------------------

iANModalThresholdingWidget::iANModalThresholdingWidget(QWidget* parent) : QWidget(parent)
{
	int min = 0;
	int max = 0;
	int threshold = round(max - min) / 2 + min;

	//auto slicer = display->createSlicer(m_mod);

	m_spinBox = new QSpinBox(this);
	m_spinBox->setMaximum(max);
	m_spinBox->setMinimum(min);
	m_spinBox->setValue(threshold);

	//auto slider = new QSlider(Qt::Orientation::Vertical, this);
	m_slider = new QSlider(Qt::Orientation::Horizontal, this);
	m_slider->setMaximum(max);
	m_slider->setMinimum(min);
	m_slider->setValue(threshold);

	/*auto layout = new QGridLayout(this);
	layout->addWidget(slicer, 0, 0, 2, 1);
	layout->addWidget(spinBox, 0, 1);
	layout->addWidget(slider, 1, 1);*/
	auto layout = new QHBoxLayout(this);
	layout->addWidget(m_slider);
	layout->setStretchFactor(m_slider, 1);
	layout->addWidget(m_spinBox);
	layout->setStretchFactor(m_spinBox, 0);

	connect(m_slider, &QSlider::sliderMoved, m_spinBox, &QSpinBox::setValue);
	connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_slider, &QSlider::setValue);
	connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &iANModalThresholdingWidget::setThreshold);
	connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &iANModalThresholdingWidget::thresholdChanged);
}

void iANModalThresholdingWidget::setThreshold(int threshold)
{
	m_threshold = threshold;
}

int iANModalThresholdingWidget::threshold()
{
	return m_threshold;
}

QSlider* iANModalThresholdingWidget::slider()
{
	return m_slider;
}

QSpinBox* iANModalThresholdingWidget::spinBox()
{
	return m_spinBox;
}

// ----------------------------------------------------------------------------------------------
// iANModalIterativeDilationPlot
// ----------------------------------------------------------------------------------------------

iANModalIterativeDilationPlot::iANModalIterativeDilationPlot(QWidget* parent) : QWidget(parent)
{
}

void iANModalIterativeDilationPlot::addValue(int v)
{
	assert(v > 0);
	m_values.append(v);
	m_max = v > m_max ? v : m_max;
	update();
}

QSize iANModalIterativeDilationPlot::sizeHint() const
{
	return QSize(200, 200);
}

void iANModalIterativeDilationPlot::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	if (m_values.empty())
	{
		return;
	}

	constexpr int sep = 10;           // separation
	constexpr int textSep = sep / 2;  // text separation
	constexpr int mar = sep * 2;      // margin

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	QFontMetrics fm(p.font());
	int textHeight = fm.height();

	int w = size().width();
	int h = size().height();
	int top = mar + textHeight + textSep;
	int left = mar + textHeight + textSep;
	int right = w - mar;
	int bottom = h - mar - (2 * (textHeight - textSep));

	int wAvailable = right - left;                    // available width
	int wSeparators = (sep * (m_values.size() - 1));  // width occupied by separators
	int barWidth = (int)((float)(wAvailable - wSeparators) / (float)m_values.size());

	int hAvailable = bottom - top;  // available height

	for (int i = 0; i < m_values.size(); i++)
	{
		int v = m_values[i];

		int barHeight = v == 0 ? 0 : (int)((float)(hAvailable * v) / (float)(m_max));
		int barLeft = left + ((barWidth + sep) * i);
		int barTop = bottom - barHeight;

		QString vs = QString::number(v);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		int vWidth = fm.horizontalAdvance(vs);
#else
		int vWidth = fm.width(vs);
#endif
		int vLeft = barLeft + (int)((float)(barWidth - vWidth) / 2.0f);

		QString ids = QString::number(i);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		int idWidth = fm.horizontalAdvance(ids);
#else
		int idWidth = fm.width(ids);
#endif
		int idLeft = barLeft + (int)((float)(barWidth - idWidth) / 2.0f);

		p.fillRect(barLeft, barTop, barWidth, barHeight, QColor::fromRgb(0, 114, 189));
		p.drawText(vLeft, barTop - textSep, vs);
		p.drawText(idLeft, bottom + textSep + textHeight, ids);
	}
	p.setPen(QColor::fromRgb(0, 0, 0));
	p.drawLine(left, bottom, right, bottom);

	int centerx = left + (int)((float)wAvailable / 2.0f);
	//int centery = top + (int)((float)hAvailable / 2.0f);

	QString xlabel = "# dilations";
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	int xlabelwidth = fm.horizontalAdvance(xlabel);
#else
	int xlabelwidth = fm.width(xlabel);
#endif
	p.drawText(centerx - (int)((float)xlabelwidth / 2.0f), bottom + (2 * (textSep + textHeight)), xlabel);

	QString ylabel = "# background regions";
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	int ylabelwidth = fm.horizontalAdvance(ylabel);
#else
	int ylabelwidth = fm.width(ylabel);
#endif
	p.translate(mar, top + (int)(((float)hAvailable + (float)ylabelwidth) / 2.0f));
	p.rotate(-90);
	p.drawText(0, 0, ylabel);
}
