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
#include "dlg_datatypeconversion.h"

#include "charts/iAChartWidget.h"
#include "charts/iAHistogramData.h"
#include "charts/iAPlotTypes.h"
#include "iAConnector.h"
#include "iAToolsITK.h"
#include "iAToolsVTK.h"
#include "iATransferFunction.h"    // for GetDefault... functions
#include "iATypedCallHelper.h"
#include "iAVtkWidget.h"
#include "io/iARawFileParameters.h"

#include <itkChangeInformationImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkImageSliceConstIteratorWithIndex.h>
#include <itkNormalizeImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>

#include <vtkColorTransferFunction.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageMapToColors.h>
#include <vtkInteractorStyleImage.h>
#include <vtkMatrix4x4.h>
#include <vtkMetaImageWriter.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QStringList>
#include <QVariant>

namespace
{
	FILE* openFile(QString const & filename)
	{
		FILE * pFile = fopen(getLocalEncodingFileName(filename).c_str(), "rb");
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
	typedef typename itk::Image<T, 3> InputImageType;
	typedef typename itk::Image<double, 3> TwoDInputImageType;

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

	typename TwoDInputImageType::Pointer twodimage = TwoDInputImageType::New();
	twodimage->SetRegions(extractregion);
	twodimage->SetSpacing(extractspacing);
	twodimage->SetOrigin(extractpoint);
	twodimage->Allocate();
	twodimage->FillBuffer(0);

	typedef itk::ImageRegionIterator<TwoDInputImageType> twoditeratortype;
	twoditeratortype iter(twodimage, twodimage->GetRequestedRegion());

	//create slice iterator
	typedef itk::ImageSliceConstIteratorWithIndex<InputImageType> SliceIteratorType;
	SliceIteratorType SliceIter(itkimage, itkimage->GetRequestedRegion());

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

	typedef itk::NormalizeImageFilter<TwoDInputImageType, TwoDInputImageType> NIFTYpe;
	typename NIFTYpe::Pointer normalizefilter = NIFTYpe::New();
	normalizefilter->SetInput(twodimage);
	normalizefilter->Update();

	typedef itk::RescaleIntensityImageFilter<TwoDInputImageType, TwoDInputImageType> RIIFType;
	typename RIIFType::Pointer rescalefilter = RIIFType::New();
	rescalefilter->SetInput(normalizefilter->GetOutput());
	rescalefilter->SetOutputMinimum(0);
	rescalefilter->SetOutputMaximum(65535);
	rescalefilter->Update();

	image->setImage(rescalefilter->GetOutput());
	image->modified();

	// DEBUG:
	//vtkSmartPointer<vtkMetaImageWriter> metaImageWriter = vtkSmartPointer<vtkMetaImageWriter>::New();
	//metaImageWriter->SetFileName("xyimage.mhd");
	//metaImageWriter->SetInputData(xyconvertimage->vtkImage());
	//metaImageWriter->SetCompression(0);
	//metaImageWriter->Write();
}

template<class T> void DataTypeConversion_template(QString const & filename, iARawFileParameters const & p, unsigned int zSkip, int numBins,
	iAPlotData::DataType * histptr, double & minVal, double & maxVal, double & discretization, iAConnector* xyimage, iAConnector* xzimage, iAConnector* yzimage)
{
	// TODO: use itk methods instead?
	typedef itk::Image< T, 3 >   InputImageType;

	FILE * pFile = openFile(filename);

	typename InputImageType::Pointer itkimage = InputImageType::New();

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
	discretization = (maxVal - minVal) / numBins;

	// copy the file into the buffer and create histogram:
	typedef itk::ImageRegionIterator<InputImageType> iteratortype;
	iteratortype iter(itkimage, itkimage->GetRequestedRegion());
	iter.GoToBegin();
	bool loop = true;
	const int elemCount = 1;
	fseek ( pFile , 0 , SEEK_SET );
	std::fill(histptr, histptr + static_cast<size_t>(numBins), 0);
	size_t result;
	while (loop)
	{
		result = fread (reinterpret_cast<char*>(&buffer),datatypesize, elemCount, pFile);
		// TODO: check result!
		size_t binIdx = ((buffer-minVal)/discretization);
		iter.Set(buffer);
		++iter;
		histptr[binIdx] += 1;
		slicecounter++;
		if ( slicecounter == slicesize )
		{	// TO CHECK: does that really skip anything?
			slicecounter = 0;
			numsliceread++;
			size_t skipmemory = slicesize*datatypesize * zSkip * numsliceread;
			if ( skipmemory < totalsize )
				fseek ( pFile , static_cast<long>(skipmemory) , SEEK_SET );
			else
				loop = false;
		}
	}
	fclose(pFile);

	extractSliceImage<typename InputImageType::PixelType>(itkimage, 0, 1/*, itkimage->GetLargestPossibleRegion().GetSize()[2] / 2*/, xyimage); // XY plane - along z axis
	extractSliceImage<typename InputImageType::PixelType>(itkimage, 0, 2/*, itkimage->GetLargestPossibleRegion().GetSize()[1] / 2*/, xzimage); // XZ plane - along y axis
	extractSliceImage<typename InputImageType::PixelType>(itkimage, 1, 2/*, itkimage->GetLargestPossibleRegion().GetSize()[0] / 2*/, yzimage); // YZ plane - along x axis
}

void dlg_datatypeconversion::loadPreview(QString const & filename, iARawFileParameters const & p, unsigned int zSkip, int numBins)
{
	VTK_TYPED_CALL(DataTypeConversion_template, p.m_scalarType, filename, p, zSkip, numBins, m_histbinlist, m_min, m_max, m_dis, m_xyimage, m_xzimage, m_yzimage);
}

//roi conversion
template<class T> void DataTypeConversionROI_template(QString const & filename, iARawFileParameters const & p, double* roi, double & minVal, double & maxVal, iAConnector* roiimage)
{
	typedef itk::Image< T, 3 >   InputImageType;

	FILE * pFile = openFile(filename);

	typename InputImageType::Pointer itkimage = InputImageType::New();

	// create itk image
	//float itkz = floor((float)((p.m_size[2]-1)/zSkip)+1);
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
	typedef itk::ImageRegionIterator<InputImageType> iteratortype;
	iteratortype iter (itkimage, itkimage->GetRequestedRegion());
	iter.GoToBegin();

	getFileMinMax<typename InputImageType::PixelType>(pFile, minVal, maxVal);

	bool loop = true;
	typename InputImageType::PixelType buffer;
	size_t result;
	const int elemCount = 1;
	// copy the file into the buffer:
	fseek ( pFile , 0 , SEEK_SET );
	while (loop)
	{
		result = fread (reinterpret_cast<char*>(&buffer), sizeof(buffer), elemCount, pFile);
		if ( result == elemCount )
		{
			iter.Set(buffer);
			++iter;
		}
		else
			loop = false;
	}
	fclose(pFile);

	typedef itk::ExtractImageFilter< InputImageType, InputImageType > EIFType;
	typename EIFType::Pointer filter = EIFType::New();

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

QVBoxLayout* setupSliceWidget(iAVtkWidget* &widget, vtkSmartPointer<vtkPlaneSource> & roiSource, iAConnector* image, QString const & name)
{
	QVBoxLayout *boxlayout = new QVBoxLayout();
	QLabel *label = new QLabel(QString("%1 IMAGE").arg(name));
	label->setMinimumWidth(50);
	label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	widget = new iAVtkWidget();
	widget->setMinimumHeight(50);
	widget->setWindowTitle(QString("%1 Plane").arg(name));
	boxlayout->addWidget(label);
	boxlayout->addWidget(widget);

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

	auto window = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	window->SetNumberOfLayers(2);
	window->AddRenderer(imageRenderer);
	window->AddRenderer(roiRenderer);
	widget->SetRenderWindow(window);
	auto imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
	window->GetInteractor()->SetInteractorStyle(imageStyle);
	widget->update();
	window->Render();

	widget->show();
	return boxlayout;
}

dlg_datatypeconversion::dlg_datatypeconversion(QWidget *parent, QString const & filename, iARawFileParameters const & p,
	unsigned int zSkip, int numBins, double* c, double* inPara) : QDialog (parent)
{
	setupUi(this);

	m_roiimage = new iAConnector();
	m_xyimage = new iAConnector();
	m_xzimage = new iAConnector();
	m_yzimage = new iAConnector();

	//read raw file
	m_spacing[0] = p.m_spacing[0]; m_spacing[1] = p.m_spacing[1];	m_spacing[2] = p.m_spacing[2];
	m_min = 0; m_max = 0; m_dis = 0;
	m_roi[0] = m_roi[1] = m_roi[2]= 0;
	m_roi[3] = p.m_size[0];
	m_roi[4] = p.m_size[1];
	m_roi[5] = p.m_size[2];

	m_histbinlist = new iAPlotData::DataType[numBins];

	loadPreview(filename, p, zSkip, numBins);

	createHistogram(m_histbinlist, m_min, m_max, numBins, m_dis);

	auto xyboxlayout = setupSliceWidget(m_xyWidget, m_xyroiSource, m_xyimage, "XY");
	auto xzboxlayout = setupSliceWidget(m_xzWidget, m_xzroiSource, m_xzimage, "XZ");
	auto yzboxlayout = setupSliceWidget(m_yzWidget, m_yzroiSource, m_yzimage, "YZ");

	updateROI();

	QHBoxLayout *hboxlayout = new QHBoxLayout();
	hboxlayout->addLayout(xyboxlayout);
	hboxlayout->addLayout(xzboxlayout);
	hboxlayout->addLayout(yzboxlayout);

	verticalLayout->addLayout(hboxlayout);

	//data entry
	QLabel *label5 = new QLabel("Output Datatype", this);
	label5->setMinimumWidth(50);
	QStringList datatypecon = (QStringList() <<  tr("VTK_SIGNED_CHAR") <<  tr("VTK_UNSIGNED_CHAR") <<  tr("VTK_SHORT")
		<<  tr("VTK_UNSIGNED_SHORT") <<  tr("VTK_INT") <<  tr("VTK_UNSIGNED_INT") <<  tr("VTK_FLOAT") <<  tr("VTK_DOUBLE") );
	cbDataType = new QComboBox(this);
	cbDataType->insertItems(0,datatypecon);

	chConvertROI = new QCheckBox(" Data Conversion of ROI ", this);
	//chUseMaxDatatypeRange = new QCheckBox(" Use Maximum Datatype Range ", this);
	QHBoxLayout *hbox0 = new QHBoxLayout();
	hbox0->addWidget(label5);
	hbox0->addWidget(cbDataType);
	hbox0->addWidget(chConvertROI);
	//hbox0->addWidget(chUseMaxDatatypeRange);
	verticalLayout->addLayout(hbox0);

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
	verticalLayout->addLayout(hbox1);

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
	verticalLayout->addLayout(hbox2);

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
	verticalLayout->addLayout(hbox3);

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
	verticalLayout->addLayout(hbox4);

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
	verticalLayout->addLayout(hbox5);

	updatevalues( inPara );

	verticalLayout->addWidget(buttonBox);

	connect(leXOrigin, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(leXSize, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(leYOrigin, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(leYSize, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(leZOrigin, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(leZSize, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
}

dlg_datatypeconversion::~dlg_datatypeconversion()
{
	//delete[] m_histbinlist;  // gets deleted in iAHistogramData !
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

void dlg_datatypeconversion::createHistogram(iAPlotData::DataType* histbinlist, double minVal, double maxVal, int bins, double discretization )
{
	iAChartWidget* chart = new iAChartWidget(nullptr, "Histogram (Intensities)", "Frequency");
	auto data = iAHistogramData::create(histbinlist, bins, discretization, minVal, maxVal);
	chart->addPlot(QSharedPointer<iAPlot>(new iABarGraphPlot(data, QColor(70, 70, 70, 255))));
	chart->update();
	chart->setMinimumHeight(80);
	verticalLayout->addWidget(chart);
}

// read binary and perform shift+scale
// TODO: implement with itk filters - benefits: parallelized; probably better reading than in chunks of 1 voxel
template <typename T>
void loadBinary(FILE* pFile, vtkImageData* imageData, float shift, float scale, double &minout, double &maxout)
{
	T buffer;
	fseek(pFile, 0, SEEK_SET);
	int* dims = imageData->GetDimensions();
	for (int z = 0; z < dims[2]; z++)
	{
		for (int y = 0; y<dims[1]; y++)
		{
			for (int x = 0; x<dims[0]; x++)
			{
				size_t result = fread(reinterpret_cast<char*>(&buffer), sizeof(buffer), 1, pFile);
				double value = buffer * scale + shift;
				value = (value > maxout) ? maxout : value;
				value = (value < minout) ? minout : value;
				imageData->SetScalarComponentFromDouble(x, y, z, 0, value);
			}
		}
	}
}

QString dlg_datatypeconversion::convert( QString const & filename, iARawFileParameters const & p, int outdatatype, double minrange, double maxrange, double minout, double maxout, int check )
{
	float scale = 0;
	//scale and shift calculator
	if ( minrange != maxrange )
	{   scale = ( maxout - minout ) / ( maxrange - minrange );	}
	else if ( maxrange != 0 )
	{	//m_Scale = ( maxout - minout ) / minrange );
	}
	else
	{
		scale = 0.0;
	}
	float shift = ( minout - minrange ) * scale;
	FILE * pFile = openFile(filename);
	vtkImageData* imageData = vtkImageData::New();
	// Setup the image
	imageData->SetDimensions(p.m_size[0], p.m_size[1], p.m_size[2]);
	imageData->AllocateScalars(outdatatype, 1);
	//loading of datatype
	VTK_TYPED_CALL(loadBinary, p.m_scalarType, pFile, imageData, shift, scale, minout, maxout);
	fclose(pFile);

	QString outputFileName(filename);
	outputFileName.chop(4);
	outputFileName.append("-DT.mhd");
	vtkMetaImageWriter* metaImageWriter = vtkMetaImageWriter::New();
	metaImageWriter->SetFileName(getLocalEncodingFileName(outputFileName).c_str());
	metaImageWriter->SetInputData(imageData);
	metaImageWriter->Write();
	return outputFileName;
}

QString dlg_datatypeconversion::convertROI(QString const & filename, iARawFileParameters const & p, int outdatatype, double minrange, double maxrange, double minout, double maxout, int check, double* roi)
{
	DataTypeConversionROI(filename, p, roi);
	double scale = 0;
	//scale and shift calculator
	if ( minrange != maxrange )
		scale = ( maxout - minout ) / ( maxrange - minrange );
	else if ( maxrange != 0 )
	{	//m_Scale = ( maxout - minout ) / minrange );
	}
	else
		scale = 0.0;

	double shift = ( minout - minrange ) * scale;

	FILE * pFile = openFile(filename);
	fclose(pFile);

	vtkImageData* imageData = vtkImageData::New();
	// Setup the image
	imageData->SetDimensions(roi[1], roi[3], roi[5]);
	imageData->AllocateScalars(outdatatype, 1);
	int* dims = imageData->GetDimensions();
	for (int z=0; z<dims[2]; z++)
	{
		for (int y=0; y<dims[1]; y++)
		{
			for (int x=0; x<dims[0]; x++)
			{
				double buffer = m_roiimage->vtkImage()->GetScalarComponentAsDouble(x,y,z,0);
				double value  =  buffer * scale + shift;
				value = ( value > maxout ) ? maxout : value;
				value = ( value < minout ) ? minout : value;
				imageData->SetScalarComponentFromDouble(x,y,z,0,value);
			}// for x
		}// for y
	}//for z

	QString outputFileName(filename);
	outputFileName.chop(4);
	outputFileName.append("-DT-roi.mhd");
	vtkMetaImageWriter* metaImageWriter = vtkMetaImageWriter::New();
	metaImageWriter->SetFileName( getLocalEncodingFileName(outputFileName).c_str());
	metaImageWriter->SetInputData(imageData);
	metaImageWriter->Write();
	return outputFileName;
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

	m_xyWidget->GetRenderWindow()->Render();
	m_xyWidget->update();
	m_xzWidget->GetRenderWindow()->Render();
	m_xzWidget->update();
	m_yzWidget->GetRenderWindow()->Render();
	m_yzWidget->update();
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
