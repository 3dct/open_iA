/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
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
#include "pch.h"
#include "iADatasetInfo.h"

#include "PorosityAnalyserHelpers.h"

#include "io/iAITKIO.h"

#include <itkImageFileWriter.h>
#include <itkExtractImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkImageDuplicator.h>
#include <itkStatisticsImageFilter.h>
#include <itkImageToHistogramFilter.h>

#include <QDir>
#include <QDebug>

template<class T> int iADatasetInfo::generateInfo( QString datasetPath, QString datasetName, 
												   ImagePointer & image, iAPorosityAnalyserModuleInterface * pmi, 
												   int totalFInfoNbToCreate, int currentFInfoNb )
{
	typedef itk::Image<T, DIM>  InputImageType;
	typedef itk::Image<unsigned char, DIM>  uCharInputImageType;
	typedef itk::Image<unsigned char, 2>  OutputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();
	
	//intensity statistics 
	typedef itk::StatisticsImageFilter<InputImageType> StatisticsImageFilterType;
	typename StatisticsImageFilterType::Pointer statisticsImageFilter = StatisticsImageFilterType::New();
	statisticsImageFilter->SetInput( duplicator->GetOutput() );
	statisticsImageFilter->Update();
	int minIntensity = statisticsImageFilter->GetMinimum();
	int maxIntensity = statisticsImageFilter->GetMaximum();

	//intensity histogram
	const unsigned int MeasurementVectorSize = 1; // Grayscale
	const unsigned int binsPerDimension = maxIntensity - minIntensity;
	typedef itk::Statistics::ImageToHistogramFilter< InputImageType > ImageToHistogramFilterType;
	typename ImageToHistogramFilterType::HistogramType::MeasurementVectorType	lowerBound( binsPerDimension );
	lowerBound.Fill( minIntensity );
	typename ImageToHistogramFilterType::HistogramType::MeasurementVectorType	upperBound( binsPerDimension );
	upperBound.Fill( maxIntensity );
	typename ImageToHistogramFilterType::HistogramType::SizeType		h_size( MeasurementVectorSize );
	h_size.Fill( binsPerDimension );
	typename ImageToHistogramFilterType::Pointer imageToHistogramFilter = ImageToHistogramFilterType::New();
	imageToHistogramFilter->SetInput( duplicator->GetOutput() );
	imageToHistogramFilter->SetHistogramBinMinimum( lowerBound );
	imageToHistogramFilter->SetHistogramBinMaximum( upperBound );
	imageToHistogramFilter->SetHistogramSize( h_size );
	imageToHistogramFilter->Update();
	typename ImageToHistogramFilterType::HistogramType* histogram = imageToHistogramFilter->GetOutput();
	
	//Write info to dataset info file
	ofstream fout( QString( datasetPath + "/" + datasetName + ".info" ).toStdString().c_str(), std::ofstream::out );
	fout << "Datasetname:" << QString( datasetName ).toStdString() << '\n'
		<< "Min:" << minIntensity << '\n'
		<< "Max:" << maxIntensity << '\n'
		<< "Std:" << statisticsImageFilter->GetSigma() << '\n'
		<< "Mean:" << statisticsImageFilter->GetMean() << '\n'
		<< "Variance:" << statisticsImageFilter->GetVariance() << '\n';
	// Walking through all of the histogram bins and getting the corresponding frequencies
	typedef typename ImageToHistogramFilterType::HistogramType HistogramType;
	typename HistogramType::ConstIterator itr = histogram->Begin();
	typename HistogramType::ConstIterator end = histogram->End();
	typedef typename HistogramType::AbsoluteFrequencyType AbsoluteFrequencyType;
	fout << "VoxelCount:" << histogram->GetTotalFrequency() << '\n';
	int indexCounter = minIntensity;
	while ( itr != end )
	{
		const AbsoluteFrequencyType frequency = itr.GetFrequency();
		fout << indexCounter << ":" << frequency << "\n";
		indexCounter++;
		++itr;
	}
	fout.close();

	//Create png file from middle Z-slice
	try
	{
		typedef itk::RescaleIntensityImageFilter<InputImageType, uCharInputImageType> RescalerType;
		typename RescalerType::Pointer rescaler = RescalerType::New();
		rescaler->SetOutputMinimum( 0 );
		rescaler->SetOutputMaximum( 255 );
		rescaler->SetInput( duplicator->GetOutput() );
		rescaler->Update();
		uCharInputImageType::Pointer rescaledImg = rescaler->GetOutput();

		const unsigned char * bufferPtr = rescaledImg->GetBufferPointer();
		MaskImageType::SizeType size = rescaledImg->GetLargestPossibleRegion().GetSize();
		unsigned int sliceSize = size[0] * size[1];
		QString datasetFolder = datasetPath + "/" + datasetName;
		QFileInfo maskFI( datasetFolder );
		if( !QDir( maskFI.absoluteDir() ).mkdir( getMaskSliceDirName( datasetFolder ) ) )
			pmi->log( "Could not create directory for slices!" );

		for( unsigned int sliceNumber = 1; sliceNumber <= size[2]; ++sliceNumber )	// int slicerNumber set to 1 cause of emitted progess value
		{
			const unsigned char * sBufferPtr = bufferPtr + sliceSize * (sliceNumber - 1);
			QImage img( size[0], size[1], QImage::Format_Indexed8 );
			for( int y = 0; y < size[1]; y++ )
				memcpy( img.scanLine( size[1] - y - 1 ), sBufferPtr + y*size[0], size[0] );	// we invert Y-axis, because VTK and Qt have different Y axis directions
			for( int i = 0; i < 255; ++i )
				img.setColor( i, qRgb( i, i, i ) );
			QString fileName = getSliceFilename( datasetFolder, (sliceNumber - 1));
			if( !img.save( fileName ) )
				throw itk::ExceptionObject( "Could not save png!" );
			emit progress( sliceNumber * ( 100 / totalFInfoNbToCreate ) / size[2]  +  100 * currentFInfoNb / totalFInfoNbToCreate );
		}
	}
	catch( itk::ExceptionObject & err )
	{
		QString tolog = QString( "  %1 in File %2, Line %3" ).arg( err.GetDescription() )
			.arg( err.GetFile() )
			.arg( err.GetLine() );
		pmi->log( tolog );
	}
// 	typedef itk::ExtractImageFilter< uCharInputImageType, OutputImageType > ExtracterType;
// 	ExtracterType::Pointer extracter = ExtracterType::New();
// 	extracter->InPlaceOn();
// 	extracter->SetDirectionCollapseToIdentity();
// 	MaskImageType::RegionType inputRegion = duplicator->GetOutput()->GetLargestPossibleRegion();
// 	MaskImageType::SizeType m_size = inputRegion.GetSize();
// 	m_size[2] = 0;	//Z slice
// 	MaskImageType::IndexType start = inputRegion.GetIndex();
// 	const unsigned int sliceNumber = inputRegion.GetSize()[2] / 2;
// 	start[2] = sliceNumber;
// 	MaskImageType::RegionType desiredRegion;
// 	desiredRegion.SetSize( m_size );
// 	desiredRegion.SetIndex( start );
// 	extracter->SetExtractionRegion( desiredRegion );
// 	extracter->SetInput( rescaler->GetOutput() );
// 	typedef itk::ImageFileWriter< OutputImageType > WriterType;
// 	WriterType::Pointer writer = WriterType::New();
// 	writer->SetFileName( fileName.toStdString() );
// 	writer->SetInput( extracter->GetOutput() );
// 	writer->Update();

	return EXIT_SUCCESS;
}

void iADatasetInfo::run()
{
	m_newGeneratedInfoFilesList.clear();
	calculateInfo();
}

void iADatasetInfo::calculateInfo()
{
	QString datasetPath = m_pmi->DatasetFolder();
	QDir datasetsDir( datasetPath );
	datasetsDir.setNameFilters( QStringList( "*.mhd" ) );
	QDir filesInfoDir( datasetPath );
	filesInfoDir.setNameFilters( QStringList( "*.mhd.info" ) );
	QStringList dl = datasetsDir.entryList();
	QStringList fl = filesInfoDir.entryList();
	int totalFInfoNbToCreate = dl.size() - fl.size();
	int currentFInfoNb = 0;
	bool success = false;
	
	for ( int i = 0; i < dl.size(); ++i ) //iterate over datasets
	{
		QString datasetName = dl[i];
		QFile datasetInfoFile( datasetPath + "/" + datasetName + ".info" );
		if( datasetInfoFile.exists() )
			continue;

		// inintialize input datset
		ScalarPixelType pixelType;
		ImagePointer image = iAITKIO::readFile( datasetPath + "/" + datasetName, pixelType, true);
		try
		{
			switch ( pixelType )
			{
				case itk::ImageIOBase::UCHAR:
					generateInfo<unsigned char>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::CHAR:
					generateInfo<char>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::USHORT:
					generateInfo<unsigned short>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::SHORT:
					generateInfo<short>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::UINT:
					generateInfo<unsigned int>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::INT:
					generateInfo<int>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::ULONG:
					generateInfo<unsigned long>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::LONG:
					generateInfo<long>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::FLOAT:
					generateInfo<float>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::DOUBLE:
					generateInfo<double>( datasetPath, datasetName, image, m_pmi, totalFInfoNbToCreate, currentFInfoNb ); break;
				case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
					//
					break;
			}
			currentFInfoNb++;
			success = true;
		}
		catch ( itk::ExceptionObject &excep )
		{
			m_pmi->log( "Filter run terminated unexpectedly." );
			m_pmi->log( QObject::tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
						.arg( excep.GetFile() )
						.arg( excep.GetLine() ) );
			success = false;
		}
		catch ( ... )
		{
			m_pmi->log( "Dataset Preview filter run terminated unexpectedly with unknown exception." );
			success = false;
		}
	}
	if ( success )
	{
		QDir newfilesInfoDir( datasetPath );
		newfilesInfoDir.setNameFilters( QStringList( "*.mhd.info" ) );
		QSet<QString> old_fl = fl.toSet();
		QSet<QString> new_fl = newfilesInfoDir.entryList().toSet();
		m_newGeneratedInfoFilesList = new_fl.subtract( old_fl ).toList();
		m_pmi->log( "Dataset previews successfully created." );
	}
}


QStringList iADatasetInfo::getNewGeneratedInfoFiles()
{
	return m_newGeneratedInfoFilesList;
}