// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_datatypeconversion.h"

// guibase
#include <iAQVTKWidget.h>
#include <iARawFileParameters.h>

// charts
#include <iAChartWidget.h>
#include <iAHistogramData.h>
#include <iAPlotTypes.h>

// base
#include <iAConnector.h>
#include <iAMathUtility.h>
#include <iAToolsITK.h>
#include <iAToolsVTK.h>
#include <iATransferFunction.h>    // for GetDefault... functions
#include <iATypedCallHelper.h>

#include <itkChangeInformationImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkImageSliceConstIteratorWithIndex.h>
#include <itkNormalizeImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>

#include <vtkColorTransferFunction.h>    // required for Linux build
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageMapToColors.h>
#include <vtkInteractorStyleImage.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QStringList>
#include <QVariant>
#include <QVBoxLayout>

namespace
{
	FILE* openFile(QString const & filename)
	{
		FILE * pFile = fopen(filename.toStdString().c_str(), "rb");
		if (pFile == nullptr)
		{
			QString msg(QString("Failed to open file %1!").arg(filename));
			throw std::runtime_error(msg.toStdString());
		}
		return pFile;
	}
}

template<class T> void getFileMinMax(FILE* pFile, double& minVal, double& maxVal)
{
	minVal = std::numeric_limits<double>::max();
	maxVal = std::numeric_limits<double>::lowest();
	//calculation of minimum and maximum
	fseek(pFile, 0, SEEK_SET);
	const int elemCount = 1;
	T buffer;
	size_t result;
	while ((result = fread(reinterpret_cast<char*>(&buffer), sizeof(buffer), elemCount, pFile)) == elemCount)
	{
		if (buffer < minVal)
			minVal = buffer;
		if (buffer > maxVal)
			maxVal = buffer;
	}
}

template <class T> void extractSliceImage(typename itk::Image<T, 3>::Pointer itkimage, unsigned int firstDir, unsigned int secondDir, /*int sliceNr, projectionMethod[=SUM], */iAConnector* image)
{
	using InputImageType = itk::Image<T, 3>;
	using TwoDInputImageType = itk::Image<double, 3>;

	typename TwoDInputImageType::RegionType extractregion;
	typename TwoDInputImageType::IndexType extractindex; extractindex.Fill(0);	extractregion.SetIndex(extractindex);
	typename TwoDInputImageType::PointType extractpoint; extractpoint.Fill(0);
	typename TwoDInputImageType::SpacingType extractspacing;
	extractspacing[0] = itkimage->GetSpacing()[firstDir];
	extractspacing[1] = itkimage->GetSpacing()[secondDir];
	extractspacing[2] = 1;

	typename TwoDInputImageType::SizeType extractsize;
	extractsize[0] = itkimage->GetLargestPossibleRegion().GetSize()[firstDir];
	extractsize[1] = itkimage->GetLargestPossibleRegion().GetSize()[secondDir];
	extractsize[2] = 1;
	extractregion.SetSize(extractsize);

	auto twodimage = TwoDInputImageType::New();
	twodimage->SetRegions(extractregion);
	twodimage->SetSpacing(extractspacing);
	twodimage->SetOrigin(extractpoint);
	twodimage->Allocate();
	twodimage->FillBuffer(0);

	itk::ImageRegionIterator<TwoDInputImageType> iter(twodimage, twodimage->GetRequestedRegion());

	//create slice iterator
	itk::ImageSliceConstIteratorWithIndex<InputImageType> SliceIter(itkimage, itkimage->GetRequestedRegion());

	//set direction
	SliceIter.SetFirstDirection(firstDir);
	SliceIter.SetSecondDirection(secondDir);

	SliceIter.GoToBegin();
	//int curSlice = 0;
	while (!SliceIter.IsAtEnd())
	{
		//if (curSlice == sliceNr)
		//{
			iter.GoToBegin();
			while (!SliceIter.IsAtEndOfSlice())
			{
				while (!SliceIter.IsAtEndOfLine())
				{
					typename InputImageType::PixelType value = SliceIter.Get();
					typename TwoDInputImageType::PixelType pix = iter.Get() + value;  // sum projection
					iter.Set(pix);
					++iter;
					++SliceIter;
				}
				SliceIter.NextLine();
			}
		//	break;
		//}
		//++curSlice;
		SliceIter.NextSlice();
	}

	auto normalizefilter = itk::NormalizeImageFilter<TwoDInputImageType, TwoDInputImageType>::New();
	normalizefilter->SetInput(twodimage);
	normalizefilter->Update();

	auto rescalefilter = itk::RescaleIntensityImageFilter<TwoDInputImageType, TwoDInputImageType>::New();
	rescalefilter->SetInput(normalizefilter->GetOutput());
	rescalefilter->SetOutputMinimum(0);
	rescalefilter->SetOutputMaximum(65535);
	rescalefilter->Update();

	image->setImage(rescalefilter->GetOutput());
	image->modified();
}

template<class T> void DataTypeConversion_template(QString const & filename, iARawFileParameters const & p, unsigned int zSkip, size_t numBins,
	iAPlotData::DataType * histptr, double & minVal, double & maxVal, iAConnector* xyimage, iAConnector* xzimage, iAConnector* yzimage)
{
	// TODO: use itk methods instead?
	using InputImageType = itk::Image<T, 3>;

	LOG(lvlInfo, QString("Reading file '%1':").arg(filename));
	FILE * pFile = openFile(filename);

	auto itkimage = InputImageType::New();

	// create itk image
	typename InputImageType::SpacingType itkspacing;
	itkspacing[0] = p.m_spacing[0]; itkspacing[1] = p.m_spacing[1];	itkspacing[2] = p.m_spacing[2];
	typename itk::Size<3> itksize;
	itksize[0] = p.m_size[0]; itksize[1] = p.m_size[1];
	itksize[2] = static_cast<itk::Size<3>::SizeValueType>(std::floor(static_cast<float>(p.m_size[2]-1) / zSkip + 1));
	typename InputImageType::IndexType itkindex;	itkindex.Fill(0);
	typename InputImageType::RegionType itkregion;	itkregion.SetSize(itksize);	itkregion.SetIndex(itkindex);

	itkimage->SetSpacing(itkspacing);	itkimage->SetRegions(itkregion);	itkimage->Allocate();	itkimage->FillBuffer(0);

	typename InputImageType::PixelType buffer;
	size_t datatypesize = sizeof(buffer);
	size_t slicesize = itksize[0] * itksize[1];
	size_t slicecounter = 0;
	size_t numsliceread = 0;
	size_t totalsize = slicesize * p.m_size[2] * datatypesize;

	getFileMinMax<typename InputImageType::PixelType>(pFile, minVal, maxVal);
	double discretization = (maxVal - minVal) / numBins;

	// copy the file into the buffer and create histogram:
	itk::ImageRegionIterator<InputImageType> iter(itkimage, itkimage->GetRequestedRegion());
	iter.GoToBegin();
	bool loop = true;
	const int elemCount = 1;
	fseek ( pFile , 0 , SEEK_SET );
	std::fill(histptr, histptr + static_cast<size_t>(numBins), 0);
	while (loop)
	{
		size_t result = fread (reinterpret_cast<char*>(&buffer),datatypesize, elemCount, pFile);
		if (result != elemCount)
		{
			if (feof(pFile))
			{
				LOG(lvlError, QString("Unexpected end of file '%1'!").arg(filename));
			}
			else if (ferror(pFile))
			{
				LOG(lvlError, QString("Error while reading file '%1'!").arg(filename));
			}
			else
			{
				LOG(lvlError, QString("Could not read a full chunk of size %1 (bytes) while reading file '%2'!")
						  .arg(datatypesize*elemCount)
						  .arg(filename));
			}
			loop = false;
		}
		size_t binIdx = clamp(static_cast<size_t>(0), numBins-1, static_cast<size_t>((buffer-minVal)/discretization));
		iter.Set(buffer);
		++iter;
		++histptr[binIdx];
		++slicecounter;
		if ( slicecounter == slicesize )
		{	// TO CHECK: does that really skip anything?
			slicecounter = 0;
			numsliceread++;
			size_t skipmemory = slicesize*datatypesize * zSkip * numsliceread;
			if ( skipmemory < totalsize )
			{
				LOG(lvlDebug, QString("Skipping ahead to %1.").arg(skipmemory));
				fseek ( pFile , static_cast<long>(skipmemory) , SEEK_SET );
			}
			else
			{
				loop = false;
			}
		}
	}
	fclose(pFile);

	extractSliceImage<typename InputImageType::PixelType>(itkimage, 0, 1/*, itkimage->GetLargestPossibleRegion().GetSize()[2] / 2*/, xyimage); // XY plane - along z axis
	extractSliceImage<typename InputImageType::PixelType>(itkimage, 0, 2/*, itkimage->GetLargestPossibleRegion().GetSize()[1] / 2*/, xzimage); // XZ plane - along y axis
	extractSliceImage<typename InputImageType::PixelType>(itkimage, 1, 2/*, itkimage->GetLargestPossibleRegion().GetSize()[0] / 2*/, yzimage); // YZ plane - along x axis
}

void dlg_datatypeconversion::loadPreview(QString const & filename, iARawFileParameters const & p, unsigned int zSkip, size_t numBins)
{
	VTK_TYPED_CALL(DataTypeConversion_template, p.m_scalarType, filename, p, zSkip, numBins, m_histbinlist, m_min, m_max, m_xyimage, m_xzimage, m_yzimage);
}

//roi conversion
template<class T> void DataTypeConversionROI_template(QString const & filename, iARawFileParameters const & p, double* roi, double & minVal, double & maxVal, iAConnector* roiimage)
{
	using InputImageType = itk::Image<T, 3>;

	FILE * pFile = openFile(filename);

	auto itkimage = InputImageType::New();

	// create itk image
	//float itkz = std::floor((float)((p.m_size[2]-1)/zSkip)+1);
	typename InputImageType::SpacingType itkspacing;
	itkspacing[0] = p.m_spacing[0]; itkspacing[1] = p.m_spacing[1];	itkspacing[2] = p.m_spacing[2];
	typename InputImageType::SizeType itksize;
	itksize[0] = p.m_size[0]; itksize[1] = p.m_size[1]; itksize[2] = p.m_size[2];
	typename InputImageType::IndexType itkindex;
	itkindex.Fill(0);
	typename InputImageType::RegionType itkregion;
	itkregion.SetSize(itksize);
	itkregion.SetIndex(itkindex);

	itkimage->SetSpacing(itkspacing);
	itkimage->SetRegions(itkregion);
	itkimage->Allocate();
	itkimage->FillBuffer(0);

	//create itk image iterator
	itk::ImageRegionIterator<InputImageType> iter(itkimage, itkimage->GetRequestedRegion());
	iter.GoToBegin();

	getFileMinMax<typename InputImageType::PixelType>(pFile, minVal, maxVal);

	bool loop = true;
	typename InputImageType::PixelType buffer;
	const int elemCount = 1;
	// copy the file into the buffer:
	fseek ( pFile , 0 , SEEK_SET );
	while (loop)
	{
		size_t result = fread (reinterpret_cast<char*>(&buffer), sizeof(buffer), elemCount, pFile);
		if ( result == elemCount )
		{
			iter.Set(buffer);
			++iter;
		}
		else
			loop = false;
	}
	fclose(pFile);

	using EIFType = itk::ExtractImageFilter<InputImageType, InputImageType>;
	auto filter = EIFType::New();

	typename EIFType::InputImageRegionType::SizeType size; size[0] = roi[1]; size[1] = roi[3]; size[2] = roi[5];
	typename EIFType::InputImageRegionType::IndexType index; index[0] = roi[0]; index[1] = roi[2]; index[2] = roi[4];
	typename EIFType::InputImageRegionType region; region.SetIndex(index); region.SetSize(size);

	filter->SetInput( itkimage );
	filter->SetExtractionRegion(region);
	filter->Update();

	roiimage->setImage( setIndexOffsetToZero<T>(filter->GetOutput()) );
	roiimage->modified();
}

void dlg_datatypeconversion::DataTypeConversionROI(QString const & filename, iARawFileParameters const & p, double *roi)
{
	VTK_TYPED_CALL(DataTypeConversionROI_template, p.m_scalarType, filename, p, roi, m_min, m_max, m_roiimage);
}

QVBoxLayout* setupSliceWidget(iAQVTKWidget* &widget, vtkSmartPointer<vtkPlaneSource> & roiSource, iAConnector* image, QString const & name)
{
	QVBoxLayout *boxlayout = new QVBoxLayout();
	QLabel *label = new QLabel(QString("%1 IMAGE").arg(name));
	label->setMinimumWidth(50);
	label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	widget = new iAQVTKWidget();
	widget->setMinimumHeight(50);
	widget->setWindowTitle(QString("%1 Plane").arg(name));

	auto color = vtkSmartPointer<vtkImageMapToColors>::New();
	auto table = defaultColorTF(image->vtkImage()->GetScalarRange());
	color->SetLookupTable(table);
	color->SetInputData(image->vtkImage());
	color->Update();

	roiSource = vtkSmartPointer<vtkPlaneSource>::New();
	roiSource->SetCenter(0, 0, 1);
	roiSource->SetOrigin(0, 0, 0);
	roiSource->SetPoint1(3, 0, 0);
	roiSource->SetPoint2(0, 3, 0);

	auto imageActor = vtkSmartPointer<vtkImageActor>::New();
	imageActor->SetInputData(color->GetOutput());
	imageActor->GetMapper()->BorderOn();
	auto roiMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	roiMapper->SetInputConnection(roiSource->GetOutputPort());
	auto roiActor = vtkSmartPointer<vtkActor>::New();
	roiActor->SetVisibility(true);
	roiActor->SetMapper(roiMapper);
	roiActor->GetProperty()->SetColor(1, 0, 0);
	roiActor->GetProperty()->SetOpacity(1);
	roiActor->GetProperty()->SetRepresentation(VTK_WIREFRAME);
	roiMapper->Update();

	auto imageRenderer = vtkSmartPointer<vtkRenderer>::New();
	imageRenderer->SetLayer(0);
	imageRenderer->AddActor(imageActor);
	auto roiRenderer = vtkSmartPointer<vtkRenderer>::New();
	roiRenderer->SetLayer(1);
	roiRenderer->AddActor(roiActor);
	roiRenderer->SetInteractive(false);
	roiRenderer->SetActiveCamera(imageRenderer->GetActiveCamera());
	imageRenderer->ResetCamera();

	auto window = widget->renderWindow();
	window->SetNumberOfLayers(2);
	window->AddRenderer(imageRenderer);
	window->AddRenderer(roiRenderer);
	auto imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
	window->GetInteractor()->SetInteractorStyle(imageStyle);

	boxlayout->addWidget(label);
	boxlayout->addWidget(widget);
	return boxlayout;
}

dlg_datatypeconversion::dlg_datatypeconversion(QWidget *parent, QString const & filename, iARawFileParameters const & p,
	unsigned int zSkip, double* inPara) : QDialog (parent)
{
	setLayout(new QVBoxLayout());

	m_roiimage = new iAConnector();
	m_xyimage = new iAConnector();
	m_xzimage = new iAConnector();
	m_yzimage = new iAConnector();

	//read raw file
	m_spacing[0] = p.m_spacing[0]; m_spacing[1] = p.m_spacing[1];	m_spacing[2] = p.m_spacing[2];
	m_min = 0; m_max = 0;
	m_roi[0] = m_roi[1] = m_roi[2]= 0;
	m_roi[3] = p.m_size[0];
	m_roi[4] = p.m_size[1];
	m_roi[5] = p.m_size[2];

	const size_t numBins = 2048;
	m_histbinlist = new iAPlotData::DataType[numBins];

	loadPreview(filename, p, zSkip, numBins);

	createHistogram(m_histbinlist, m_min, m_max, numBins);

	auto xyboxlayout = setupSliceWidget(m_xyWidget, m_xyroiSource, m_xyimage, "XY");
	auto xzboxlayout = setupSliceWidget(m_xzWidget, m_xzroiSource, m_xzimage, "XZ");
	auto yzboxlayout = setupSliceWidget(m_yzWidget, m_yzroiSource, m_yzimage, "YZ");

	updateROI();

	QHBoxLayout *hboxlayout = new QHBoxLayout();
	hboxlayout->addLayout(xyboxlayout);
	hboxlayout->addLayout(xzboxlayout);
	hboxlayout->addLayout(yzboxlayout);

	qobject_cast<QVBoxLayout*>(layout())->addLayout(hboxlayout);

	//data entry
	QLabel *label5 = new QLabel("Output Datatype", this);
	label5->setMinimumWidth(50);
	QStringList dataTypes(readableDataTypeList(false));
	cbDataType = new QComboBox(this);
	cbDataType->insertItems(0, dataTypes);

	chConvertROI = new QCheckBox(" Data Conversion of ROI ", this);
	//chUseMaxDatatypeRange = new QCheckBox(" Use Maximum Datatype Range ", this);
	QHBoxLayout *hbox0 = new QHBoxLayout();
	hbox0->addWidget(label5);
	hbox0->addWidget(cbDataType);
	hbox0->addWidget(chConvertROI);
	//hbox0->addWidget(chUseMaxDatatypeRange);
	qobject_cast<QVBoxLayout*>(layout())->addLayout(hbox0);

	QLabel *label1 = new QLabel("Lower Range", this);
	label1->setMinimumWidth(50);
	leRangeLower = new QLineEdit(this);
	leRangeLower->setMinimumWidth(50);

	QLabel *label2 = new QLabel("Upper Range", this);
	label2->setMinimumWidth(50);
	leRangeUpper = new QLineEdit(this);
	leRangeUpper->setMinimumWidth(50);

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->addWidget(label1);
	hbox1->addWidget(leRangeLower);
	hbox1->addWidget(label2);
	hbox1->addWidget(leRangeUpper);
	qobject_cast<QVBoxLayout*>(layout())->addLayout(hbox1);

	QLabel *label3 = new QLabel("Minimum Output Value", this);
	label3->setMinimumWidth(50);
	leOutputMin = new QLineEdit(this);
	leOutputMin->setMinimumWidth(50);

	QLabel *label4 = new QLabel("Maximum Output Value", this);
	label4->setMinimumWidth(50);
	leOutputMax = new QLineEdit(this);
	leOutputMax->setMinimumWidth(50);

	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addWidget(label3);
	hbox2->addWidget(leOutputMin);
	hbox2->addWidget(label4);
	hbox2->addWidget(leOutputMax);
	qobject_cast<QVBoxLayout*>(layout())->addLayout(hbox2);

	QLabel *label6 = new QLabel("X Origin", this);
	label6->setMinimumWidth(50);
	leXOrigin = new QLineEdit(this);
	leXOrigin->setMinimumWidth(50);
	leXOrigin->setObjectName("XOrigin");

	QLabel *label7 = new QLabel("X Size", this);
	label7->setMinimumWidth(50);
	leXSize = new QLineEdit(this);
	leXSize->setMinimumWidth(50);
	leXSize->setObjectName("XSize");

	QHBoxLayout *hbox3 = new QHBoxLayout();
	hbox3->addWidget(label6);
	hbox3->addWidget(leXOrigin);
	hbox3->addWidget(label7);
	hbox3->addWidget(leXSize);
	qobject_cast<QVBoxLayout*>(layout())->addLayout(hbox3);

	QLabel *label8 = new QLabel("Y Origin", this);
	label8->setMinimumWidth(50);
	leYOrigin = new QLineEdit(this);
	leYOrigin->setMinimumWidth(50);
	leYOrigin->setObjectName("YOrigin");

	QLabel *label9 = new QLabel("Y Size", this);
	label9->setMinimumWidth(50);
	leYSize = new QLineEdit(this);
	leYSize->setMinimumWidth(50);
	leYSize->setObjectName("YSize");

	QHBoxLayout *hbox4 = new QHBoxLayout();
	hbox4->addWidget(label8);
	hbox4->addWidget(leYOrigin);
	hbox4->addWidget(label9);
	hbox4->addWidget(leYSize);
	qobject_cast<QVBoxLayout*>(layout())->addLayout(hbox4);

	QLabel *label10 = new QLabel("Z Origin", this);
	label10->setMinimumWidth(50);
	leZOrigin = new QLineEdit(this);
	leZOrigin->setMinimumWidth(50);
	leZOrigin->setObjectName("ZOrigin");

	QLabel *label11 = new QLabel("Z Size", this);
	label11->setMinimumWidth(50);
	leZSize = new QLineEdit(this);
	leZSize->setMinimumWidth(50);
	leZSize->setObjectName("ZSize");

	QHBoxLayout *hbox5 = new QHBoxLayout();
	hbox5->addWidget(label10);
	hbox5->addWidget(leZOrigin);
	hbox5->addWidget(label11);
	hbox5->addWidget(leZSize);
	qobject_cast<QVBoxLayout*>(layout())->addLayout(hbox5);

	updatevalues( inPara );

	auto buttonBox = new QDialogButtonBox();
	buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	qobject_cast<QVBoxLayout*>(layout())->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(leXOrigin, &QLineEdit::textChanged, this, &dlg_datatypeconversion::update);
	connect(leXSize,   &QLineEdit::textChanged, this, &dlg_datatypeconversion::update);
	connect(leYOrigin, &QLineEdit::textChanged, this, &dlg_datatypeconversion::update);
	connect(leYSize,   &QLineEdit::textChanged, this, &dlg_datatypeconversion::update);
	connect(leZOrigin, &QLineEdit::textChanged, this, &dlg_datatypeconversion::update);
	connect(leZSize,   &QLineEdit::textChanged, this, &dlg_datatypeconversion::update);

	setWindowTitle("Open with datatype conversion");
}

dlg_datatypeconversion::~dlg_datatypeconversion()
{
	delete[] m_histbinlist;
}

void dlg_datatypeconversion::updatevalues(double* inPara)
{
	QList<QVariant> str;
	str <<  tr("%1").arg(inPara[0]) <<  tr("%1").arg(inPara[1]) <<  tr("%1").arg(inPara[2]) <<  tr("%1").arg(inPara[3])
		<<  tr("%1").arg(inPara[4]) <<  tr("%1").arg(inPara[5])	<<  tr("%1").arg(inPara[6]) <<  tr("%1").arg(inPara[7])
		<<  tr("%1").arg(inPara[8]) <<  tr("%1").arg(inPara[9]) <<  tr("%1").arg(inPara[10]);
	leRangeLower->setText(str[0].toString());
	leRangeUpper->setText(str[1].toString());
	leOutputMin->setText(str[2].toString());
	leOutputMax->setText(str[3].toString());
	chConvertROI->setChecked(str[4].toBool());
	leXOrigin->setText(str[5].toString());
	leXSize->setText(str[6].toString());
	leYOrigin->setText(str[7].toString());
	leYSize->setText(str[8].toString());
	leZOrigin->setText(str[9].toString());
	leZSize->setText(str[10].toString());
}

void dlg_datatypeconversion::createHistogram(iAPlotData::DataType* histbinlist, double minVal, double maxVal, int bins)
{
	iAChartWidget* chart = new iAChartWidget(nullptr, "Histogram (Intensities)", "Frequency");
	auto histogramData = iAHistogramData::create("Frequency", iAValueType::Continuous, minVal, maxVal, bins, histbinlist);
	chart->addPlot(std::make_shared<iABarGraphPlot>(histogramData, QColor(70, 70, 70, 255)));
	chart->update();
	chart->setMinimumHeight(80);
	qobject_cast<QVBoxLayout*>(layout())->addWidget(chart);
}

// unify with iAIntensity shiftScale?

void convertRange(vtkImageData* inImg, vtkImageData* outImg, double minrange, double maxrange, double minout, double maxout)
{
	int* dims = inImg->GetDimensions();
	for (int z = 0; z < dims[2]; z++)
	{
		for (int y = 0; y<dims[1]; y++)
		{
			for (int x = 0; x<dims[0]; x++)
			{
				double inValue = inImg->GetScalarComponentAsDouble(x, y, z, 0);
				double outValue = mapValue(minrange, maxrange, minout, maxout, inValue);
				outImg->SetScalarComponentFromDouble(x, y, z, 0, outValue);
			}
		}
	}
}

QString writeScaledImage(QString const& filename, QString const & suffix,
	vtkImageData* inImg, int outdatatype, double minrange,
	double maxrange, double minout, double maxout)
{
	auto outImg = allocateImage(outdatatype, inImg->GetDimensions(), inImg->GetSpacing());
	convertRange(inImg, outImg, minrange, maxrange, minout, maxout);
	QString outputFileName(filename);
	outputFileName.chop(4); // assuming current name has 3 characters...?
	outputFileName.append(suffix);
	iAConnector outCon;
	outCon.setImage(outImg);
	iAITKIO::writeFile(outputFileName, outCon.itkImage(), outCon.itkScalarType());
	return outputFileName;
}

QString dlg_datatypeconversion::convert( QString const & filename,
	int outdatatype, double minrange,
	double maxrange, double minout, double maxout)
{
	iAITKIO::PixelType pixelType;
	iAITKIO::ScalarType scalarType;
	auto inImg = iAITKIO::readFile(filename, pixelType, scalarType, true);
	assert(pixelType == iAITKIO::PixelType::SCALAR);
	iAConnector con;
	con.setImage(inImg);
	return writeScaledImage(filename, "-DT.mhd", con.vtkImage(), outdatatype, minrange, maxrange, minout, maxout);
}

QString dlg_datatypeconversion::convertROI(QString const & filename,
	iARawFileParameters const & p, int outdatatype, double minrange,
	double maxrange, double minout, double maxout, double* roi)
{
	DataTypeConversionROI(filename, p, roi);
	return writeScaledImage(filename, "-DT-roi.mhd", m_roiimage->vtkImage(), outdatatype, minrange, maxrange, minout, maxout);
}


void dlg_datatypeconversion::update(QString a)
{
	QString senderName = QObject::sender()->objectName();
	if (senderName.compare("XOrigin") == 0) { m_roi[0] = a.toDouble(); }
	if (senderName.compare("YOrigin") == 0) { m_roi[1] = a.toDouble(); }
	if (senderName.compare("ZOrigin") == 0) { m_roi[2] = a.toDouble(); }
	if (senderName.compare("XSize") == 0)   { m_roi[3] = a.toDouble(); }
	if (senderName.compare("YSize") == 0)   { m_roi[4] = a.toDouble(); }
	if (senderName.compare("ZSize") == 0)	{ m_roi[5] = a.toDouble(); }
	updateROI();
}

void dlg_datatypeconversion::updateROI( )
{
	m_xyroiSource->SetOrigin(-0.5*m_spacing[0] + m_roi[0]*m_spacing[0], -0.5*m_spacing[1] + m_roi[1]*m_spacing[1], 0);
	m_xyroiSource->SetPoint1(-0.5*m_spacing[0] + m_roi[0]*m_spacing[0]+m_roi[3]*m_spacing[0], -0.5*m_spacing[1] + m_roi[1]*m_spacing[1]  , 0);
	m_xyroiSource->SetPoint2(-0.5*m_spacing[0] + m_roi[0]*m_spacing[0] , -0.5*m_spacing[1] + m_roi[1]*m_spacing[1]+m_roi[4]*m_spacing[1], 0);

	m_xzroiSource->SetOrigin(-0.5*m_spacing[0] + m_roi[0]*m_spacing[0], -0.5*m_spacing[2] + m_roi[2]*m_spacing[2], 0);
	m_xzroiSource->SetPoint1(-0.5*m_spacing[0] + m_roi[0]*m_spacing[0]+m_roi[3]*m_spacing[0], -0.5*m_spacing[2] + m_roi[2]*m_spacing[2], 0);
	m_xzroiSource->SetPoint2(-0.5*m_spacing[0] + m_roi[0]*m_spacing[0] , -0.5*m_spacing[2] + m_roi[2]+m_roi[5], 0);

	m_yzroiSource->SetOrigin(-0.5*m_spacing[1] + m_roi[1]*m_spacing[1], -0.5*m_spacing[2] + m_roi[2]*m_spacing[2], 0);
	m_yzroiSource->SetPoint1(-0.5*m_spacing[1] + m_roi[1]*m_spacing[1]+m_roi[4]*m_spacing[1], -0.5*m_spacing[2] + m_roi[2]*m_spacing[2], 0);
	m_yzroiSource->SetPoint2(-0.5*m_spacing[1] + m_roi[1]*m_spacing[1], -0.5*m_spacing[2] + m_roi[2]+m_roi[5], 0);

	m_xyWidget->updateAll();
	m_xzWidget->updateAll();
	m_yzWidget->updateAll();
}

double dlg_datatypeconversion::getRangeLower() const { return leRangeLower->text().toDouble(); }
double dlg_datatypeconversion::getRangeUpper() const { return leRangeUpper->text().toDouble(); }
double dlg_datatypeconversion::getOutputMin()  const { return leOutputMin->text().toDouble(); }
double dlg_datatypeconversion::getOutputMax()  const { return leOutputMax->text().toDouble(); }
double dlg_datatypeconversion::getXOrigin()    const { return leXOrigin->text().toDouble(); }
double dlg_datatypeconversion::getXSize()      const { return leXSize->text().toDouble(); }
double dlg_datatypeconversion::getYOrigin()    const { return leYOrigin->text().toDouble(); }
double dlg_datatypeconversion::getYSize()      const { return leYSize->text().toDouble(); }
double dlg_datatypeconversion::getZOrigin()    const { return leZOrigin->text().toDouble(); }
double dlg_datatypeconversion::getZSize()      const { return leZSize->text().toDouble(); }
QString dlg_datatypeconversion::getDataType()  const { return cbDataType->currentText(); }
int dlg_datatypeconversion::getConvertROI()    const { return chConvertROI->checkState(); }
