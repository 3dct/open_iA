/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
 
#include "pch.h"
#include "dlg_datatypeconversion.h"

#include "iAConnector.h"
#include "iAHistogramWidget.h"
#include "iAToolsVTK.h"
#include "iATypedCallHelper.h"

#include <itkChangeInformationImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkImageSliceConstIteratorWithIndex.h>
#include <itkNormalizeImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>

#include <QVTKWidget.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageAccumulate.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageReslice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>
#include <vtkMetaImageWriter.h>
#include <vtkPlaneSource.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVersion.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QStringList>
#include <QVariant>


template<class T> int DataTypeConversion_template(string m_filename, double* b, iAAbstractDiagramData::DataType * histptr, float* m_min, float* m_max, float* m_dis, iAConnector* xyconvertimage, iAConnector* xzconvertimage, iAConnector* yzconvertimage)
{
	typedef itk::Image< T, 3 >   InputImageType;

	FILE * pFile;
	pFile = fopen( m_filename.c_str(), "rb" );
	if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

	typename InputImageType::Pointer itkimage = InputImageType::New();

	// create itk image
	float itkz = floor((float)((b[3]-1)/b[0])+1);
	typename InputImageType::SpacingType itkspacing;	itkspacing[0] = b[4]; itkspacing[1] = b[5];	itkspacing[2] = b[6];
	typename InputImageType::SizeType itksize;	itksize[0] = b[1]; itksize[1] = b[2]; itksize[2] = (int)itkz;
	typename InputImageType::IndexType itkindex;	itkindex.Fill(0);
	typename InputImageType::RegionType itkregion;	itkregion.SetSize(itksize);	itkregion.SetIndex(itkindex);

	itkimage->SetSpacing(itkspacing);	itkimage->SetRegions(itkregion);	itkimage->Allocate();	itkimage->FillBuffer(0);

	//create itk image iterator
	typedef itk::ImageRegionIterator<InputImageType> iteratortype;
	iteratortype iter (itkimage, itkimage->GetRequestedRegion());
	iter.GoToBegin();

	typename InputImageType::PixelType buffer;
	unsigned long datatypesize = sizeof(buffer);
	long slicesize = b[1]*b[2];
	int slicecounter = 0;
	size_t result;
	int numsliceread = 0;

	long totalsize = b[1]*b[2]*b[3]*datatypesize;
	double min = std::numeric_limits<double>::max();
	double max = std::numeric_limits<double>::lowest();

	//calculation of minimum and maximum
	fseek ( pFile , 0 , SEEK_SET );
	while (result = fread (reinterpret_cast<char*>(&buffer),datatypesize,1,pFile))
	{
		if (buffer < min)
		{	min = buffer;	}
		if (buffer > max)
		{	max = buffer;	}
	}

	float discretization = (float)((max-min)/(b[7]));

	// copy the file into the buffer:
	bool loop = 1; 
	fseek ( pFile , 0 , SEEK_SET );
	std::fill(histptr, histptr + static_cast<size_t>(b[7]), 0);
	while (loop)
	{
		result = fread (reinterpret_cast<char*>(&buffer),datatypesize,1,pFile);
		size_t binIdx = ((buffer-min)/discretization);
		iter.Set(buffer);
		++iter;
		histptr[binIdx] += 1;
		slicecounter++;
		if ( slicecounter == slicesize )
		{
			slicecounter = 0;
			numsliceread++;
			long skipmemory = slicesize*datatypesize*b[0]*numsliceread;
			if ( skipmemory < totalsize )
			{	fseek ( pFile , skipmemory , SEEK_SET );	}
			else
			{	loop = 0;	}
		}
	}
	fclose(pFile);

	*m_min = min;	*m_max = max;	*m_dis = discretization;

	// creating projection image
	typedef typename itk::Image<double,3> TwoDInputImageType;
	typename TwoDInputImageType::Pointer xytwodimage = TwoDInputImageType::New();

	typename TwoDInputImageType::RegionType extractregion;
	typename TwoDInputImageType::IndexType extractindex; extractindex.Fill(0);	extractregion.SetIndex(extractindex);
	typename TwoDInputImageType::PointType extractpoint; extractpoint.Fill(0);
	typename TwoDInputImageType::SpacingType extractspacing; extractspacing[0] = b[4];	extractspacing[1] = b[5];	extractspacing[2] = b[6];
	// along z axis - xy plane
	typename TwoDInputImageType::SizeType extractsize;	extractsize[0] = b[1]; extractsize[1] = b[2];	extractsize[2] = 1;
	extractregion.SetSize(extractsize);

	xytwodimage->SetRegions(extractregion);
	xytwodimage->SetSpacing(extractspacing);
	xytwodimage->SetOrigin(extractpoint);
	xytwodimage->Allocate();
	xytwodimage->FillBuffer(0);

	typedef itk::ImageRegionIterator<TwoDInputImageType> twoditeratortype;
	twoditeratortype xyiter (xytwodimage, xytwodimage->GetRequestedRegion());

	//create slice iterator
	typedef itk::ImageSliceConstIteratorWithIndex<InputImageType> SliceIteratorType;
	SliceIteratorType SliceIter ( itkimage, itkimage->GetRequestedRegion() );

	//set direction
	SliceIter.SetFirstDirection(0);
	SliceIter.SetSecondDirection(1);

	SliceIter.GoToBegin();
	while ( !SliceIter.IsAtEnd() )
	{
		xyiter.GoToBegin();
		while ( !SliceIter.IsAtEndOfSlice() )
		{
			while ( !SliceIter.IsAtEndOfLine() )
			{
				typename InputImageType::PixelType value = SliceIter.Get();
				typename InputImageType::IndexType index = SliceIter.GetIndex();
				typename TwoDInputImageType::PixelType xypix = xyiter.Get() + value;
				xyiter.Set(xypix);
				++xyiter;
				++SliceIter;
			}//while
			SliceIter.NextLine();
		}//while
		SliceIter.NextSlice();
	}//while

	typedef itk::NormalizeImageFilter<TwoDInputImageType,TwoDInputImageType> NIFTYpe;
	typename NIFTYpe::Pointer xynormalizefilter = NIFTYpe::New();
	xynormalizefilter->SetInput(xytwodimage);
	xynormalizefilter->Update();

	typedef itk::RescaleIntensityImageFilter<TwoDInputImageType,TwoDInputImageType> RIIFType;
	typename RIIFType::Pointer xyrescalefilter = RIIFType::New();
	xyrescalefilter->SetInput(xynormalizefilter->GetOutput());
	xyrescalefilter->SetOutputMinimum(0);
	xyrescalefilter->SetOutputMaximum(65535);
	xyrescalefilter->Update();

	xyconvertimage->SetImage(xyrescalefilter->GetOutput());
	xyconvertimage->Modified();
	vtkMetaImageWriter* metaImageWriter = vtkMetaImageWriter::New();
	metaImageWriter->SetFileName("C:/LIBS/vtkxyimage.mhd");
	metaImageWriter->SetInputData(xyconvertimage->GetVTKImage());

	metaImageWriter->SetCompression(0);
	metaImageWriter->Write();

	//xz plane - along y axis
	extractsize[0] = b[1]; extractsize[1] = 1;	extractsize[2] = b[3];
	extractregion.SetSize(extractsize);

	typename TwoDInputImageType::Pointer xztwodimage = TwoDInputImageType::New();
	xztwodimage->SetRegions(extractregion);
	xztwodimage->SetSpacing(extractspacing);
	xztwodimage->SetOrigin(extractpoint);
	xztwodimage->Allocate();
	xztwodimage->FillBuffer(0);

	twoditeratortype xziter (xztwodimage, xztwodimage->GetRequestedRegion());

	//set direction
	SliceIter.SetFirstDirection(0);
	SliceIter.SetSecondDirection(2);

	SliceIter.GoToBegin();
	while ( !SliceIter.IsAtEnd() )
	{
		xziter.GoToBegin();
		while ( !SliceIter.IsAtEndOfSlice() )
		{
			while ( !SliceIter.IsAtEndOfLine() )
			{
				typename InputImageType::PixelType value = SliceIter.Get();
				typename InputImageType::IndexType index = SliceIter.GetIndex();
				TwoDInputImageType::PixelType xypix = xziter.Get() + value;
				xziter.Set(xypix);
				++xziter;
				++SliceIter;
			}//while
			SliceIter.NextLine();
		}//while
		SliceIter.NextSlice();
	}//while

	typename NIFTYpe::Pointer xznormalizefilter = NIFTYpe::New();
	xznormalizefilter->SetInput(xztwodimage);
	xznormalizefilter->Update();

	typename RIIFType::Pointer xzrescalefilter = RIIFType::New();
	xzrescalefilter->SetInput(xznormalizefilter->GetOutput());
	xzrescalefilter->SetOutputMinimum(0);
	xzrescalefilter->SetOutputMaximum(65535);
	xzrescalefilter->Update();

	xzconvertimage->SetImage(xzrescalefilter->GetOutput());
	xzconvertimage->Modified();

	//yz plane - along x axis
	extractsize[0] = 1; extractsize[1] = b[2];	extractsize[2] = b[3];
	extractregion.SetSize(extractsize);

	typename TwoDInputImageType::Pointer yztwodimage = TwoDInputImageType::New();
	yztwodimage->SetRegions(extractregion);
	yztwodimage->SetSpacing(extractspacing);
	yztwodimage->SetOrigin(extractpoint);
	yztwodimage->Allocate();
	yztwodimage->FillBuffer(0);

	twoditeratortype yziter (yztwodimage, yztwodimage->GetRequestedRegion());

	//set direction
	SliceIter.SetFirstDirection(1);
	SliceIter.SetSecondDirection(2);

	SliceIter.GoToBegin();
	while ( !SliceIter.IsAtEnd() )
	{
		yziter.GoToBegin();
		while ( !SliceIter.IsAtEndOfSlice() )
		{
			while ( !SliceIter.IsAtEndOfLine() )
			{
				typename InputImageType::PixelType value = SliceIter.Get();
				typename InputImageType::IndexType index = SliceIter.GetIndex();
				typename TwoDInputImageType::PixelType xypix = yziter.Get() + value;
				yziter.Set(xypix);
				++yziter;
				++SliceIter;
			}//while
			SliceIter.NextLine();
		}//while
		SliceIter.NextSlice();
	}//while

	typename NIFTYpe::Pointer yznormalizefilter = NIFTYpe::New();
	yznormalizefilter->SetInput(yztwodimage);
	yznormalizefilter->Update();

	typename RIIFType::Pointer yzrescalefilter = RIIFType::New();
	yzrescalefilter->SetInput(yznormalizefilter->GetOutput());
	yzrescalefilter->SetOutputMinimum(0);
	yzrescalefilter->SetOutputMaximum(65535);
	yzrescalefilter->Update();

	yzconvertimage->SetImage(yznormalizefilter->GetOutput());
	yzconvertimage->Modified();

	return EXIT_SUCCESS;
}

void dlg_datatypeconversion::DataTypeConversion(string m_filename, double* b)
{
	VTK_TYPED_CALL(DataTypeConversion_template, m_intype, m_filename, b, m_histbinlist, &m_min, &m_max, &m_dis, xyconvertimage, xzconvertimage, yzconvertimage);
	m_testxyimage = xyconvertimage->GetVTKImage();
	m_testxzimage = xzconvertimage->GetVTKImage();
	m_testyzimage = yzconvertimage->GetVTKImage();
}

//roi conversion
template<class T> int DataTypeConversionROI_template( string m_filename, double* b, double* roi, float* m_min, float* m_max, float* m_dis, iAConnector* m_roiconvertimage)
{
	typedef itk::Image< T, 3 >   InputImageType;

	FILE * pFile;
	pFile = fopen( m_filename.c_str(), "rb" );
	if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

	typename InputImageType::Pointer itkimage = InputImageType::New();

	// create itk image
	//float itkz = floor((float)((b[3]-1)/b[0])+1);
	typename InputImageType::SpacingType itkspacing;	itkspacing[0] = b[4]; itkspacing[1] = b[5];	itkspacing[2] = b[6];
	typename InputImageType::SizeType itksize;
	itksize[0] = b[1]; itksize[1] = b[2]; itksize[2] = b[3];
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

	typename std::vector<unsigned int> m_VolumeCount ( (b[7]+1), 0 );

	unsigned long datatypesize;
	long slicesize; 
	size_t result;
	//int numsliceread = 0;

	typename InputImageType::PixelType buffer;
	datatypesize = sizeof(buffer);
	slicesize = b[1]*b[2];
	//int slicecounter = 0;

	//long totalsize = b[1]*b[2]*b[3]*datatypesize;
	float min = std::numeric_limits<float>::max();
	float max = std::numeric_limits<float>::lowest();

	//calculation of minimum and maximum
	fseek ( pFile , 0 , SEEK_SET );
	while (result = (fread (reinterpret_cast<char*>(&buffer),sizeof(buffer),1,pFile)))
	{
		if (buffer < min)
		{	min = buffer;	}
		if (buffer > max)
		{	max = buffer;	}
	}

	//float discretization = (float)((max-min)/(b[7]));

	//create the container for the histogram
	bool loop = 1; 
	// copy the file into the buffer:
	fseek ( pFile , 0 , SEEK_SET );
	while (loop)
	{
		result = fread (reinterpret_cast<char*>(&buffer),datatypesize,1,pFile);

		if ( result )
		{
			//unsigned short Pixel_test = ((buffer-min)/discretization); //static_cast<PixelType> 
			iter.Set(buffer);
			++iter;
		}
		else
		{	loop = 0;	}
	}//while
	fclose(pFile);


	typedef itk::ExtractImageFilter< InputImageType, InputImageType > EIFType;
	typename EIFType::Pointer filter = EIFType::New();

	typename EIFType::InputImageRegionType::SizeType size; size[0] = roi[1]; size[1] = roi[3]; size[2] = roi[5];
	typename EIFType::InputImageRegionType::IndexType index; index[0] = roi[0]; index[1] = roi[2]; index[2] = roi[4];
	typename EIFType::InputImageRegionType region; region.SetIndex(index); region.SetSize(size);

	filter->SetInput( itkimage );
	filter->SetExtractionRegion(region);

	filter->Update();

	//change the output image information - offset change to zero
	typename InputImageType::IndexType idx; idx.Fill(0);
	typename InputImageType::PointType origin; origin.Fill(0);
	typename InputImageType::SizeType outsize; outsize[0] = roi[1];	outsize[1] = roi[3];	outsize[2] = roi[5];
	typename InputImageType::RegionType outreg;
	outreg.SetIndex(idx); 
	outreg.SetSize(outsize);
	typename InputImageType::Pointer refimage = InputImageType::New();
	refimage->SetRegions(outreg);
	refimage->SetOrigin(origin);
	refimage->SetSpacing(filter->GetOutput()->GetSpacing());
	refimage->Allocate();

	typedef itk::ChangeInformationImageFilter<InputImageType> CIIFType;
	typename CIIFType::Pointer changefilter = CIIFType::New();
	changefilter->SetInput(filter->GetOutput());
	changefilter->UseReferenceImageOn();
	changefilter->SetReferenceImage(refimage);
	changefilter->SetChangeRegion(1);
	changefilter->Update( );

	m_roiconvertimage->SetImage( changefilter->GetOutput() );
	m_roiconvertimage->Modified();

	return EXIT_SUCCESS;
}

void dlg_datatypeconversion::DataTypeConversionROI(string m_filename, double* b, double *roi)
{
	VTK_TYPED_CALL(DataTypeConversionROI_template, m_intype, m_filename, b, roi, &m_min, &m_max, &m_dis, m_roiconvertimage);
	m_roiimage = m_roiconvertimage->GetVTKImage();
}

dlg_datatypeconversion::dlg_datatypeconversion(QWidget *parent, vtkImageData* input, const char* filename, int intype, double* b, double* c, double* inPara) : QDialog (parent)
{
	setupUi(this);

	imageData = vtkImageData::New();
	m_testxyimage =  vtkImageData::New();
	m_testxzimage =  vtkImageData::New();
	m_testyzimage =  vtkImageData::New();
	m_roiimage =  vtkImageData::New();

	m_roiconvertimage = new iAConnector();
	xyconvertimage = new iAConnector();
	xzconvertimage = new iAConnector();
	yzconvertimage = new iAConnector();

	xyroiSource = vtkPlaneSource::New();	xzroiSource = vtkPlaneSource::New();
	xyroiMapper = vtkPolyDataMapper::New();	xzroiMapper = vtkPolyDataMapper::New();
	xyroiActor = vtkActor::New();	xzroiActor = vtkActor::New();
	xyrenderer = vtkRenderer::New();
	xzrenderer = vtkRenderer::New();
	xyinteractor = vtkRenderWindowInteractor::New();
	xzinteractor = vtkRenderWindowInteractor::New();

	this->setWindowTitle("DatatypeConversion");
	this->setMinimumWidth(c[0]*0.5);
	this->setMinimumHeight(c[0]*0.5);

	//read raw file
	m_bptr = b;
	m_sliceskip = b[0]; 
	m_insizex = b[1];	m_insizey = b[2]; m_insizez = b[3];
	m_spacing[0] = b[4]; m_spacing[1] = b[5];	m_spacing[2] = b[6];
	m_bins = b[7];
	m_intype = intype;
	m_filename = filename;
	m_min = 0; m_max = 0; m_dis = 0;
	m_roi[0]= 0; m_roi[1] = 0; m_roi[2]= 0; m_roi[3]= m_insizex; m_roi[4] = m_insizey; m_roi[5] = m_insizez;

	m_histbinlist = new iAAbstractDiagramData::DataType[m_bins];

	DataTypeConversion(m_filename, b);

	vtkMetaImageWriter* metaImageWriter = vtkMetaImageWriter::New();
	metaImageWriter->SetFileName("C:/LIBS/testimage3.mhd");
	metaImageWriter->SetInputData(m_testyzimage);
	metaImageWriter->SetCompression(0);
	metaImageWriter->Write();

	TabWidget = new QTabWidget(this);
	verticalLayout->addWidget(TabWidget);

	histogramdrawing (m_histbinlist, m_min, m_max, m_bins, m_dis);

	QVBoxLayout *xyboxlayout = new QVBoxLayout();
	QLabel *xylabel = new QLabel(this, 0);
	xylabel->setMinimumWidth(50);
	xylabel->setText("XY IMAGE");

	vtkWidgetXY = new QVTKWidget(this);
	vtkWidgetXY->setMinimumHeight(c[0]*0.1);
	vtkWidgetXY->setMinimumWidth(c[1]*0.1);
	vtkWidgetXY->setWindowTitle("XY Plane");
	xyboxlayout->addWidget(xylabel);
	xyboxlayout->addWidget(vtkWidgetXY);
	//call the projection slices function
	xyprojectslices ( );
	vtkWidgetXY->show();

	QVBoxLayout *xzboxlayout = new QVBoxLayout();
	QLabel *xzlabel = new QLabel(this, 0);
	xzlabel->setMinimumWidth(50);
	xzlabel->setText("XZ IMAGE");

	vtkWidgetXZ = new QVTKWidget(this);
	vtkWidgetXZ->setMinimumHeight(c[0]*0.1);
	vtkWidgetXZ->setMinimumWidth(c[1]*0.1);
	vtkWidgetXZ->setWindowTitle("XZ Plane");
	xzboxlayout->addWidget(xzlabel);
	xzboxlayout->addWidget(vtkWidgetXZ);
	//call the projection slices function
	xzprojectslices ( );
	vtkWidgetXZ->show();

	QHBoxLayout *hboxlayout = new QHBoxLayout();
	hboxlayout->addLayout(xyboxlayout);
	hboxlayout->addLayout(xzboxlayout);

	verticalLayout->addLayout(hboxlayout);

	//data entry
	QLabel *label5 = new QLabel(this, 0);
	label5->setMinimumWidth(50);
	label5->setText("Output Datatype");
	QStringList datatypecon = (QStringList() <<  tr("VTK_SIGNED_CHAR") <<  tr("VTK_UNSIGNED_CHAR") <<  tr("VTK_SHORT")
		<<  tr("VTK_UNSIGNED_SHORT") <<  tr("VTK_INT") <<  tr("VTK_UNSIGNED_INT") <<  tr("VTK_FLOAT") <<  tr("VTK_DOUBLE") );
	cbDataType = new QComboBox(this);
	cbDataType->insertItems(0,datatypecon);

	chConvertROI = new QCheckBox(" Data Conversion of ROI ", this);
	chUseMaxDatatypeRange = new QCheckBox(" Use Maximum Datatype Range ", this);

	QHBoxLayout *hbox0 = new QHBoxLayout();
	hbox0->addWidget(label5);
	hbox0->addWidget(cbDataType);
	hbox0->addWidget(chConvertROI);
	hbox0->addWidget(chUseMaxDatatypeRange);

	verticalLayout->addLayout(hbox0);

	QLabel *label1 = new QLabel(this, 0);
	label1->setMinimumWidth(50);
	label1->setText("Lower Range");
	lineEdit1 = new QLineEdit(this);
	lineEdit1->setMinimumWidth(50);

	QLabel *label2 = new QLabel(this, 0);
	label2->setMinimumWidth(50);
	label2->setText("Upper Range");
	lineEdit2 = new QLineEdit(this);
	lineEdit2->setMinimumWidth(50);

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->addWidget(label1);
	hbox1->addWidget(lineEdit1);
	hbox1->addWidget(label2);
	hbox1->addWidget(lineEdit2);
	verticalLayout->addLayout(hbox1);

	QLabel *label3 = new QLabel(this, 0);
	label3->setMinimumWidth(50);
	label3->setText("Minimum Output Value");
	lineEdit3 = new QLineEdit(this);
	lineEdit3->setMinimumWidth(50);

	QLabel *label4 = new QLabel(this, 0);
	label4->setMinimumWidth(50);
	label4->setText("Maximum Output Value");
	lineEdit4 = new QLineEdit(this);
	lineEdit4->setMinimumWidth(50);

	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addWidget(label3);
	hbox2->addWidget(lineEdit3);
	hbox2->addWidget(label4);
	hbox2->addWidget(lineEdit4);
	verticalLayout->addLayout(hbox2);

	QLabel *label6 = new QLabel(this, 0);
	label6->setMinimumWidth(50);
	label6->setText("X Origin");
	lineEdit5 = new QLineEdit(this);
	lineEdit5->setMinimumWidth(50);
	lineEdit5->setObjectName("labelWidget6");

	QLabel *label7 = new QLabel(this, 0);
	label7->setMinimumWidth(50);
	label7->setText("X Size");
	lineEdit6 = new QLineEdit(this);
	lineEdit6->setMinimumWidth(50);
	lineEdit6->setObjectName("labelWidget7");

	QHBoxLayout *hbox3 = new QHBoxLayout();
	hbox3->addWidget(label6);
	hbox3->addWidget(lineEdit5);
	hbox3->addWidget(label7);
	hbox3->addWidget(lineEdit6);
	verticalLayout->addLayout(hbox3);

	QLabel *label8 = new QLabel(this, 0);
	label8->setMinimumWidth(50);
	label8->setText("Y Origin");
	lineEdit7 = new QLineEdit(this);
	lineEdit7->setMinimumWidth(50);
	lineEdit7->setObjectName("labelWidget8");

	QLabel *label9 = new QLabel(this, 0);
	label9->setMinimumWidth(50);
	label9->setText("Y Size");
	lineEdit8 = new QLineEdit(this);
	lineEdit8->setMinimumWidth(50);
	lineEdit8->setObjectName("labelWidget9");

	QHBoxLayout *hbox4 = new QHBoxLayout();
	hbox4->addWidget(label8);
	hbox4->addWidget(lineEdit7);
	hbox4->addWidget(label9);
	hbox4->addWidget(lineEdit8);
	verticalLayout->addLayout(hbox4);

	QLabel *label10 = new QLabel(this, 0);
	label10->setMinimumWidth(50);
	label10->setText("Z Origin");
	lineEdit9 = new QLineEdit(this);
	lineEdit9->setMinimumWidth(50);
	lineEdit9->setObjectName("labelWidget10");

	QLabel *label11 = new QLabel(this, 0);
	label11->setMinimumWidth(50);
	label11->setText("Z Size");
	lineEdit10 = new QLineEdit(this);
	lineEdit10->setMinimumWidth(50);
	lineEdit10->setObjectName("labelWidget11");

	QHBoxLayout *hbox5 = new QHBoxLayout();
	hbox5->addWidget(label10);
	hbox5->addWidget(lineEdit9);
	hbox5->addWidget(label11);
	hbox5->addWidget(lineEdit10);
	verticalLayout->addLayout(hbox5);

	updatevalues( inPara );

	//add the ok and cancel button to the verticalLayout
	verticalLayout->addWidget(buttonBox);

	connect(lineEdit5, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(lineEdit6, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(lineEdit7, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(lineEdit8, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(lineEdit9, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
	connect(lineEdit10, SIGNAL(textChanged(QString)), this, SLOT(update(QString)));
}

void dlg_datatypeconversion::changeEvent()
{
	//emit labelWidget11->textChanged();
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
	lineEdit1->setText(str[0].toString());
	lineEdit2->setText(str[1].toString());
	lineEdit3->setText(str[2].toString());
	lineEdit4->setText(str[3].toString());
	chConvertROI->setChecked(str[4].toBool());
	lineEdit5->setText(str[5].toString());
	lineEdit6->setText(str[6].toString());
	lineEdit7->setText(str[7].toString());
	lineEdit8->setText(str[8].toString());
	lineEdit9->setText(str[9].toString());
	lineEdit10->setText(str[10].toString());
}

void dlg_datatypeconversion::histogramdrawing(iAAbstractDiagramData::DataType* histbinlist, float min, float max, int m_bins, double discretization )
{
	vtkImageAccumulate* imageAccumulate = vtkImageAccumulate::New();
	vtkPiecewiseFunction* piecewiseFunction = vtkPiecewiseFunction::New();
	vtkColorTransferFunction* colorTransferFunction = vtkColorTransferFunction::New();

	iAHistogramWidget *imgHistogram = new iAHistogramWidget(TabWidget, (MdiChild*)parent(), imageAccumulate, piecewiseFunction, colorTransferFunction,
		histbinlist, min, max , m_bins, discretization);
	imgHistogram->updateTrf();
	imgHistogram->redraw();
	TabWidget->addTab(imgHistogram, QString("Histogram"));
}

void dlg_datatypeconversion::xyprojectslices()
{
	// Create a greyscale lookup table
	vtkSmartPointer<vtkLookupTable> table = vtkSmartPointer<vtkLookupTable>::New();

	vtkImageAccumulate* imageAccumulate = vtkImageAccumulate::New();
	imageAccumulate->SetInputData(m_testxyimage);
	imageAccumulate->Update();
	table->SetRange(imageAccumulate->GetMin()[0], imageAccumulate->GetMax()[0]); // image intensity range

	table->SetValueRange(0.0, 1.0); // from black to white
	table->SetSaturationRange(0.0, 0.0); // no color saturation
	table->SetRampToLinear();
	table->Build();

	// Map the image through the lookup table
	vtkSmartPointer<vtkImageMapToColors> color = vtkSmartPointer<vtkImageMapToColors>::New();
	color->SetLookupTable(table);
	color->SetInputData(m_testxyimage);

	// Display the image
	vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
	actor->SetInputData(color->GetOutput());

	xywindow =	vtkRenderWindow::New();
	xywindow->AddRenderer(xyrenderer);

	// Set up the interaction
	vtkSmartPointer<vtkInteractorStyleImage> imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
	xyinteractor->SetInteractorStyle(imageStyle);
	xywindow->SetInteractor(xyinteractor);

	xyroiMapper->SetInputConnection( xyroiSource->GetOutputPort() );
	xyroiActor->SetVisibility( true );
	xyroiActor->SetMapper( xyroiMapper );
	xyroiActor->GetProperty()->SetColor( 1, 0, 0 );
	xyroiActor->GetProperty()->SetOpacity( 1 );

	xyroiSource->SetCenter( 0, 0, 1 );
	xyroiActor->GetProperty()->SetRepresentation( VTK_WIREFRAME );

	xyroiSource->SetOrigin(0, 0, 0);
	xyroiSource->SetPoint1(3, 0 , 0); 
	xyroiSource->SetPoint2(0 , 3, 0);
	xyroiMapper->Update( );

	xyrenderer->AddActor(xyroiActor);
	xyrenderer->AddActor(actor);
	xywindow->Render();

	vtkWidgetXY->SetRenderWindow(xywindow);
	vtkWidgetXY->update();
}

void dlg_datatypeconversion::xzprojectslices()
{
	double center[3];
	center[0] = 0; 
	center[1] = 0; 
	center[2] = 0; 

	static double coronalElements[16] = {
		1, 0, 0, 0,
		0, 0, 1, 0,
		0,-1, 0, 0,
		0, 0, 0, 1
	};

	// Set the slice orientation
	vtkSmartPointer<vtkMatrix4x4> resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
	resliceAxes->DeepCopy(coronalElements);
	// Set the point through which to slice
	resliceAxes->SetElement(0, 3, center[0]);
	resliceAxes->SetElement(1, 3, center[1]);
	resliceAxes->SetElement(2, 3, center[2]);

	// Extract a slice in the desired orientation
	vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
	reslice->SetInputData(m_testxzimage);
	reslice->SetOutputDimensionality(2);
	reslice->SetResliceAxes(resliceAxes);
	reslice->SetInterpolationModeToLinear();

	// Create a greyscale lookup table
	vtkSmartPointer<vtkLookupTable> table =	vtkSmartPointer<vtkLookupTable>::New();

	vtkImageAccumulate* imageAccumulate = vtkImageAccumulate::New();
	imageAccumulate->SetInputData(m_testxzimage);
	imageAccumulate->Update();
	table->SetRange(imageAccumulate->GetMin()[0], imageAccumulate->GetMax()[0]); // image intensity range

	table->SetValueRange(0.0, 1.0); // from black to white
	table->SetSaturationRange(0.0, 0.0); // no color saturation
	table->SetRampToLinear();
	table->Build();

	// Map the image through the lookup table
	vtkSmartPointer<vtkImageMapToColors> color = vtkSmartPointer<vtkImageMapToColors>::New();
	color->SetLookupTable(table);
	color->SetInputConnection(reslice->GetOutputPort());

	// Display the image
	vtkSmartPointer<vtkImageActor> xzactor = vtkSmartPointer<vtkImageActor>::New();
	xzactor->SetInputData(color->GetOutput());

	vtkSmartPointer<vtkRenderer> xzrenderer = vtkSmartPointer<vtkRenderer>::New();
	xzrenderer->AddActor(xzactor);

	vtkSmartPointer<vtkRenderWindow> xzwindow =	vtkSmartPointer<vtkRenderWindow>::New();
	xzwindow->AddRenderer(xzrenderer);

	// Set up the interaction
	vtkSmartPointer<vtkInteractorStyleImage> xzimageStyle =	vtkSmartPointer<vtkInteractorStyleImage>::New();
	xzinteractor->SetInteractorStyle(xzimageStyle);
	xzwindow->SetInteractor(xzinteractor);

	xzroiMapper->SetInputConnection( xzroiSource->GetOutputPort() );
	xzroiActor->SetVisibility( true );
	xzroiActor->SetMapper( xzroiMapper );
	xzroiActor->GetProperty()->SetColor( 1, 0, 0 );
	xzroiActor->GetProperty()->SetOpacity( 1 );

	xzroiSource->SetCenter( 0, 0, 1 );
	xzroiActor->GetProperty()->SetRepresentation( VTK_WIREFRAME );

	xzroiSource->SetOrigin(0, -m_insizez, 0);
	xzroiSource->SetPoint1(10, -m_insizez , 0);
	xzroiSource->SetPoint2(0 , 2, 0);
	xzroiMapper->Update( );

	xzrenderer->AddActor(xzroiActor);
	xzwindow->Render();

	vtkWidgetXZ->SetRenderWindow(xzwindow);
	vtkWidgetXZ->update();
}


void dlg_datatypeconversion::yzprojectslices()
{
	double center[3];
	center[0] = 0; 
	center[1] = 0; 
	center[2] = 0; 

	static double obliqueElements[16] = {
		1, 0, 0, 0,
		0, 0.866025, -0.5, 0,
		0, 0.5, 0.866025, 0,
		0, 0, 0, 1
	};

	// Set the slice orientation
	vtkSmartPointer<vtkMatrix4x4> resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
	resliceAxes->DeepCopy(obliqueElements);
	// Set the point through which to slice
	resliceAxes->SetElement(0, 3, center[0]);
	resliceAxes->SetElement(1, 3, center[1]);
	resliceAxes->SetElement(2, 3, center[2]);

	// Extract a slice in the desired orientation
	vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
	reslice->SetInputData(m_testyzimage);
	reslice->SetOutputDimensionality(2);
	reslice->SetResliceAxes(resliceAxes);
	reslice->SetInterpolationModeToLinear();

	// Create a greyscale lookup table
	vtkSmartPointer<vtkLookupTable> table =	vtkSmartPointer<vtkLookupTable>::New();
	table->SetRange(0, 65535); // image intensity range
	table->SetValueRange(0.0, 1.0); // from black to white
	table->SetSaturationRange(0.0, 0.0); // no color saturation
	table->SetRampToLinear();
	table->Build();

	// Map the image through the lookup table
	vtkSmartPointer<vtkImageMapToColors> color = vtkSmartPointer<vtkImageMapToColors>::New();
	color->SetLookupTable(table);
	color->SetInputConnection(reslice->GetOutputPort());

	// Display the image
	vtkSmartPointer<vtkImageActor> actor =	vtkSmartPointer<vtkImageActor>::New();
	actor->SetInputData(color->GetOutput());

	vtkSmartPointer<vtkRenderer> renderer =	vtkSmartPointer<vtkRenderer>::New();
	renderer->AddActor(actor);

	vtkSmartPointer<vtkRenderWindow> window = vtkSmartPointer<vtkRenderWindow>::New();
	window->AddRenderer(renderer);

	// Set up the interaction
	vtkSmartPointer<vtkInteractorStyleImage> imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
	vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	interactor->SetInteractorStyle(imageStyle);
	window->SetInteractor(interactor);
	window->Render();
	vtkWidgetYZ->SetRenderWindow(window);
	vtkWidgetYZ->update();
}

template <typename T>
void loadBinary(FILE* pFile, vtkImageData* imageData, float shift, float scale, double &minout, double &maxout)
{
	T buffer;
	fseek(pFile, 0, SEEK_SET);
	// Fill every entry of the image data with "2.0"
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

QString dlg_datatypeconversion::coreconversionfunction( QString filename, QString & finalfilename, double* para, int indatatype, int outdatatype, double minrange, double maxrange, double minout, double maxout, int check )
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
	FILE * pFile;
	pFile = fopen ( filename.toStdString().c_str(), "rb" );
	if (pFile==NULL) {fputs ("File error",stderr); exit (1);}
	vtkImageData* imageData = vtkImageData::New();
	// Setup the image
	imageData->SetDimensions(para[1], para[2], para[3]);
	imageData->AllocateScalars(outdatatype, 1);
	//loading of datatype 
	VTK_TYPED_CALL(loadBinary, indatatype, pFile, imageData, shift, scale, minout, maxout);
	fclose(pFile);
	filename.chop(4);
	filename.append("-DT.mhd");
	vtkMetaImageWriter* metaImageWriter = vtkMetaImageWriter::New();
	metaImageWriter->SetFileName(filename.toStdString().c_str());
	metaImageWriter->SetInputData(imageData);
	metaImageWriter->Write();
	finalfilename = filename;
	return filename;
}

QString dlg_datatypeconversion::coreconversionfunctionforroi(QString filename, QString & finalfilename, double* para, int outdatatype, double minrange, double maxrange, double minout, double maxout, int check, double* roi)
{

	DataTypeConversionROI(filename.toStdString(), m_bptr, roi);
	float m_Scale = 0;
	//scale and shift calculator
	if ( minrange != maxrange )
	{   m_Scale = ( maxout - minout ) / ( maxrange - minrange );}
	else if ( maxrange != 0 )
	{	//m_Scale = ( maxout - minout ) / minrange );	
	}
	else
	{	m_Scale = 0.0;	}

	float m_Shift = ( minout - minrange ) * m_Scale;

	FILE * pFile;
	pFile = fopen( filename.toStdString().c_str(), "rb" );
	if (pFile==NULL) {fputs ("File error",stderr); exit (1);}
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
				double buffer = m_roiimage->GetScalarComponentAsDouble(x,y,z,0);
				double value  =  buffer * m_Scale + m_Shift;
				value = ( value > maxout ) ? maxout : value;
				value = ( value < minout ) ? minout : value;
				imageData->SetScalarComponentFromDouble(x,y,z,0,value);
			}// for x
		}// for y
	}//for z

	filename.chop(4);
	filename.append("-DT-roi.mhd");
	vtkMetaImageWriter* metaImageWriter = vtkMetaImageWriter::New();
	metaImageWriter->SetFileName(filename.toStdString().c_str());
	metaImageWriter->SetInputData(imageData);
	metaImageWriter->Write();
	finalfilename = filename;
	return filename;
}


void dlg_datatypeconversion::update(QString a ) 
{
	QString senderName = QObject::sender()->objectName();
	if (senderName.compare("labelWidget11") == 0)
	{	m_roi[5] = a.toDouble();}
	if (senderName.compare("labelWidget10") == 0)
	{	m_roi[2] = a.toDouble();}
	if (senderName.compare("labelWidget9") == 0)
	{	m_roi[4] = a.toDouble();}
	if (senderName.compare("labelWidget8") == 0)
	{	m_roi[1] = a.toDouble();}
	if (senderName.compare("labelWidget7") == 0)
	{	m_roi[3] = a.toDouble();}
	if (senderName.compare("labelWidget6") == 0)
	{	m_roi[0] = a.toDouble();}
	updateroi();
}

void dlg_datatypeconversion::updateroi( )
{
	xyroiSource->SetOrigin(m_roi[0]*m_spacing[0], m_roi[1]*m_spacing[1], 0);
	xyroiSource->SetPoint1(m_roi[0]*m_spacing[0]+m_roi[3]*m_spacing[0], m_roi[1]*m_spacing[1]  , 0); 
	xyroiSource->SetPoint2(m_roi[0]*m_spacing[0] , m_roi[1]*m_spacing[1]+m_roi[4]*m_spacing[1], 0);

	xzroiSource->SetOrigin(m_roi[0]*m_spacing[0], -m_roi[5]*m_spacing[2], 0);
	xzroiSource->SetPoint1(m_roi[0]*m_spacing[0]+m_roi[3]*m_spacing[0], -m_roi[5]*m_spacing[2], 0); 
	xzroiSource->SetPoint2(m_roi[0]*m_spacing[0] , m_roi[2]+m_roi[5], 0);

	vtkWidgetXY->update();
	vtkWidgetXZ->update();
}

double dlg_datatypeconversion::getlabelWidget1 (){return lineEdit1->text().toDouble();};
double dlg_datatypeconversion::getlabelWidget2 (){return lineEdit2->text().toDouble();};
double dlg_datatypeconversion::getlabelWidget3 (){return lineEdit3->text().toDouble();};
double dlg_datatypeconversion::getlabelWidget4 (){return lineEdit4->text().toDouble();};
double dlg_datatypeconversion::getlabelWidget6 (){return lineEdit5->text().toDouble();};
double dlg_datatypeconversion::getlabelWidget7 (){return lineEdit6->text().toDouble();};
double dlg_datatypeconversion::getlabelWidget8 (){return lineEdit7->text().toDouble();};
double dlg_datatypeconversion::getlabelWidget9 (){return lineEdit8->text().toDouble();};
double dlg_datatypeconversion::getlabelWidget10 (){return lineEdit9->text().toDouble();};
double dlg_datatypeconversion::getlabelWidget11 (){return lineEdit10->text().toDouble();};

QString dlg_datatypeconversion::getDataType() { return cbDataType->currentText(); };
int dlg_datatypeconversion::getConvertROI(){return chConvertROI->checkState();}