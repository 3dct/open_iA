/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
#pragma once

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageIOBase.h>

#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkMathUtilities.h>
#include <vtkSmartPointer.h>

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTableWidget>

#include <numeric>

const int imgDim = 3;
typedef itk::ImageBase< imgDim > ImageBaseType;
typedef ImageBaseType::Pointer ImagePointer;
typedef itk::Image< char, imgDim >   MaskImageType;
typedef MaskImageType::Pointer   MaskImagePointer;
typedef itk::ImageIOBase::IOComponentType ScalarPixelType;

const int algNameColInd = 2;
const int datasetColInd = 3;
const int groupColInd = 4;
const int outputSPMParamNum = 2;
const int paramsOffsetInRunsCSV = 14;
const int porosityOffsetInRuns = 2;
const int maskOffsetInRuns = 4;
const int gtDatasetColInd = 0;
const int gtPorosityColInd = 1;
const int gtGTSegmColumnIndex = 3;
const int maskPathColumnIndex = 11;

const QStringList computerCSVHeader = QStringList()\
<< "Computer Name"\
<< "CPU Spec"\
<< "Algorithm Name"\
<< "Dataset Name"\
<< "Batches CSV";

const QStringList runsCSVHeader = QStringList()\
<< "Start Time"\
<< "Elapsed Time"\
<< "Porosity"\
<< "Threshold"\
<< "Mask MHD"\
<< "False Positive Error"\
<< "False Negative Error"\
<< "Dice"\
<< "FeatureCnt"\
<< "AvgFeatureVol"\
<< "avgFeaturePhi"\
<< "avgFeatureTheta"\
<< "avgFeatureRoundness"\
<< "avgFeatureLength";

const QStringList filterNames = QStringList()\
<< "Binary Threshold"\
<< "Rats Threshold"\
<< "Morph Watershed Meyer"\
<< "Morph Watershed Beucher"\
<< "Otsu Threshold"\
<< "Connected Threshold"\
<< "Confidence Connected"\
<< "Neighborhood Connected"\
<< "IsoData Threshold"\
<< "MaxEntropy Threshold"\
<< "Moments Threshold" \
<< "Yen Threshold"\
<< "Renyi Threshold"\
<< "Shanbhag Threshold"\
<< "Intermodes Threshold"\
<< "Minimum Threshold"\
<< "Multiple Otsu"\
<< "Remove Surrounding"\
<< "Gradient Anisotropic Diffusion Smoothing"\
<< "Curvature Anisotropic Diffusion Smoothing"\
<< "Recursive Gaussian Smoothing"\
<< "Bilateral Smoothing"\
<< "Curvature Flow Smoothing"\
<< "Median Smoothing"\
<< "IsoX Threshold"\
<< "Fhw Threshold"\
<< "Create Surrounding"\
<< "Huang Threshold"\
<< "Li Threshold"\
<< "KittlerIllingworth Threshold"\
<< "Triangle Threshold"\
<< "General Threshold";

const QString contextMenuStyle(
	"QMenu{"
	"font-size: 11px;"
	"background-color: #9B9B9B;"
	"border: 1px solid black;}"
	"QMenu::separator{"
	"height: 1px;"
	"margin: 0px 2px 0px 2px;"
	"background: gray}" );

enum PorosityFilterID
{
	P_BINARY_THRESHOLD,
	P_RATS_THRESHOLD,
	P_MORPH_WATERSHED_MEYER,
	P_MORPH_WATERSHED_BEUCHER,
	P_OTSU_THRESHOLD,
	P_CONNECTED_THRESHOLD,
	P_CONFIDENCE_CONNECTED,
	P_NEIGHBORHOOD_CONNECTED,
	P_ISODATA_THRESHOLD,
	P_MAXENTROPY_THRESHOLD,
	P_MOMENTS_THRESHOLD,
	P_YEN_THRESHOLD,
	P_RENYI_THRESHOLD,
	P_SHANBHAG_THRESHOLD,
	P_INTERMODES_THRESHOLD,
	P_MINIMUM_THRESHOLD,
	P_MULTIPLE_OTSU,
	P_REMOVE_SURROUNDING,
	P_GRAD_ANISO_DIFF_SMOOTH,
	P_CURV_ANISO_DIFF_SMOOTH,
	P_RECURSIVE_GAUSS_SMOOTH,
	P_BILATERAL_SMOOTH,
	P_CURV_FLOW_SMOOTH,
	P_MEDIAN_SMOOTH,
	P_ISOX_THRESHOLD,
	P_FHW_THRESHOLD,
	P_CREATE_SURROUNDING,
	P_HUANG_THRESHOLD,
	P_LI_THRESHOLD,
	P_KITTLERILLINGWORTH_THRESHOLD,
	P_TRIANGLE_THRESHOLD,
	P_GENERAL_THRESHOLD
};

typedef QMap<QString, PorosityFilterID> MapQString2PorosityId;
static MapQString2PorosityId fill_FilterNameToId()
{
	MapQString2PorosityId m;
	m[filterNames.at( 0 )] = P_BINARY_THRESHOLD;
	m[filterNames.at( 1 )] = P_RATS_THRESHOLD;
	m[filterNames.at( 2 )] = P_MORPH_WATERSHED_MEYER;
	m[filterNames.at( 3 )] = P_MORPH_WATERSHED_BEUCHER;
	m[filterNames.at( 4 )] = P_OTSU_THRESHOLD;
	m[filterNames.at( 5 )] = P_CONNECTED_THRESHOLD;
	m[filterNames.at( 6 )] = P_CONFIDENCE_CONNECTED;
	m[filterNames.at( 7 )] = P_NEIGHBORHOOD_CONNECTED;
	m[filterNames.at( 8 )] = P_ISODATA_THRESHOLD;
	m[filterNames.at( 9 )] = P_MAXENTROPY_THRESHOLD;
	m[filterNames.at( 10 )] = P_MOMENTS_THRESHOLD;
	m[filterNames.at( 11 )] = P_YEN_THRESHOLD;
	m[filterNames.at( 12 )] = P_RENYI_THRESHOLD;
	m[filterNames.at( 13 )] = P_SHANBHAG_THRESHOLD;
	m[filterNames.at( 14 )] = P_INTERMODES_THRESHOLD;
	m[filterNames.at( 15 )] = P_MINIMUM_THRESHOLD;
	m[filterNames.at( 16 )] = P_MULTIPLE_OTSU;
	m[filterNames.at( 17 )] = P_REMOVE_SURROUNDING;
	m[filterNames.at( 18 )] = P_GRAD_ANISO_DIFF_SMOOTH;
	m[filterNames.at( 19 )] = P_CURV_ANISO_DIFF_SMOOTH;
	m[filterNames.at( 20 )] = P_RECURSIVE_GAUSS_SMOOTH;
	m[filterNames.at( 21 )] = P_BILATERAL_SMOOTH;
	m[filterNames.at( 22 )] = P_CURV_FLOW_SMOOTH;
	m[filterNames.at( 23 )] = P_MEDIAN_SMOOTH;
	m[filterNames.at( 24 )] = P_ISOX_THRESHOLD;
	m[filterNames.at( 25 )] = P_FHW_THRESHOLD;
	m[filterNames.at( 26 )] = P_CREATE_SURROUNDING;
	m[filterNames.at( 27 )] = P_HUANG_THRESHOLD;
	m[filterNames.at( 28 )] = P_LI_THRESHOLD;
	m[filterNames.at( 29 )] = P_KITTLERILLINGWORTH_THRESHOLD;
	m[filterNames.at( 30 )] = P_TRIANGLE_THRESHOLD;
	m[filterNames.at(31)] = P_GENERAL_THRESHOLD;
	return m;
}
const MapQString2PorosityId FilterNameToId = fill_FilterNameToId();

enum ParamType
{
	PT_INT,
	PT_FLOAT,
	PT_DOUBLE
};

typedef QPair<QString, ParamType> NameTypePair;
struct ParamNameType : public NameTypePair
{
	ParamNameType() : NameTypePair() {}
	ParamNameType( const NameTypePair & p ) : NameTypePair( p ) {}
	inline QString name() const { return this->first; }
	inline ParamType type() const { return this->second; }
};

typedef QMap<PorosityFilterID, QList<ParamNameType> > MapId2Params;
static MapId2Params fill_FilterIdToParamList()
{
	MapId2Params m;
	m[P_BINARY_THRESHOLD] = QList<ParamNameType>()\
		<< NameTypePair( "BinaryThr", PT_FLOAT );
	m[P_RATS_THRESHOLD] = QList<ParamNameType>()\
		<< NameTypePair( "Pow", PT_FLOAT );
	m[P_MORPH_WATERSHED_MEYER] = QList<ParamNameType>()\
		<< NameTypePair( "Level", PT_FLOAT )\
		<< NameTypePair( "FullyConnected", PT_INT );
	m[P_MORPH_WATERSHED_BEUCHER] = m[P_MORPH_WATERSHED_MEYER];
	m[P_OTSU_THRESHOLD] = QList<ParamNameType>();
	m[P_CONNECTED_THRESHOLD] = QList<ParamNameType>()\
		<< NameTypePair( "LoConnThr", PT_INT )\
		<< NameTypePair( "UpConnThr", PT_INT );
	m[P_CONFIDENCE_CONNECTED] = QList<ParamNameType>()\
		<< NameTypePair( "InitNeighbRadius", PT_INT )\
		<< NameTypePair( "Multip", PT_FLOAT )\
		<< NameTypePair( "NumbIter", PT_INT );
	m[P_NEIGHBORHOOD_CONNECTED] = QList<ParamNameType>()\
		<< NameTypePair( "LoConnThr", PT_INT )\
		<< NameTypePair( "UpConnThr", PT_INT )\
		<< NameTypePair( "NeighbRadius", PT_INT );
	m[P_ISODATA_THRESHOLD] = QList<ParamNameType>();
	m[P_MAXENTROPY_THRESHOLD] = QList<ParamNameType>();
	m[P_MOMENTS_THRESHOLD] = QList<ParamNameType>();
	m[P_YEN_THRESHOLD] = QList<ParamNameType>();
	m[P_RENYI_THRESHOLD] = QList<ParamNameType>();
	m[P_SHANBHAG_THRESHOLD] = QList<ParamNameType>();
	m[P_INTERMODES_THRESHOLD] = QList<ParamNameType>();
	m[P_MINIMUM_THRESHOLD] = QList<ParamNameType>();
	m[P_MULTIPLE_OTSU] = QList<ParamNameType>()\
		<< NameTypePair( "NbOfThr", PT_INT )\
		<< NameTypePair( "ValleyEmbhasis", PT_INT );
	m[P_REMOVE_SURROUNDING] = QList<ParamNameType>();
	m[P_GRAD_ANISO_DIFF_SMOOTH] = QList<ParamNameType>()\
		<< NameTypePair( "NumberOfIterations", PT_INT )\
		<< NameTypePair( "TimeStep", PT_FLOAT )\
		<< NameTypePair( "ConductanceParameter", PT_FLOAT );
	m[P_CURV_ANISO_DIFF_SMOOTH] = QList<ParamNameType>()\
		<< NameTypePair( "NumberOfIterations", PT_INT )\
		<< NameTypePair( "TimeStep", PT_FLOAT )\
		<< NameTypePair( "ConductanceParameter", PT_FLOAT );
	m[P_RECURSIVE_GAUSS_SMOOTH] = QList<ParamNameType>()\
		<< NameTypePair( "Sigma", PT_FLOAT );
	m[P_BILATERAL_SMOOTH] = QList<ParamNameType>()\
		<< NameTypePair( "DomainSigma", PT_FLOAT )\
		<< NameTypePair( "RangeSigma", PT_FLOAT );
	m[P_CURV_FLOW_SMOOTH] = QList<ParamNameType>()\
		<< NameTypePair( "NumberOfIterations", PT_INT)\
		<< NameTypePair( "TimeStep", PT_FLOAT );
	m[P_MEDIAN_SMOOTH] = QList<ParamNameType>()\
		<< NameTypePair( "Radius", PT_INT );
	m[P_ISOX_THRESHOLD] = QList<ParamNameType>()\
		<< NameTypePair( "IsoX", PT_INT );
	m[P_FHW_THRESHOLD] = QList<ParamNameType>()\
		<< NameTypePair( "Air/Pore", PT_INT )\
		<< NameTypePair( "FhwWeight", PT_INT );
	m[P_CREATE_SURROUNDING] = QList<ParamNameType>()\
		<< NameTypePair( "UpSurrThr", PT_FLOAT );
	m[P_HUANG_THRESHOLD] = QList<ParamNameType>();
	m[P_LI_THRESHOLD] = QList<ParamNameType>();
	m[P_KITTLERILLINGWORTH_THRESHOLD] = QList<ParamNameType>();
	m[P_TRIANGLE_THRESHOLD] = QList<ParamNameType>();
	m[P_GENERAL_THRESHOLD] = QList<ParamNameType>()\
		<< NameTypePair("LowerThr", PT_FLOAT)\
		<< NameTypePair("UpperThr", PT_FLOAT);
	return m;
}
const MapId2Params FilterIdToParamList = fill_FilterIdToParamList();

static MapId2Params fill_FilterIdToOutputParamList()
{
	MapId2Params m;
	m[P_BINARY_THRESHOLD] = QList<ParamNameType>();
	m[P_RATS_THRESHOLD] = QList<ParamNameType>()\
		<< NameTypePair( "Resulting Threshold", PT_FLOAT );
	m[P_MORPH_WATERSHED_MEYER] = QList<ParamNameType>();
	m[P_MORPH_WATERSHED_BEUCHER] = m[P_MORPH_WATERSHED_MEYER];
	m[P_OTSU_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_CONNECTED_THRESHOLD] = QList<ParamNameType>();
	m[P_CONFIDENCE_CONNECTED] = QList<ParamNameType>();
	m[P_NEIGHBORHOOD_CONNECTED] = QList<ParamNameType>();
	m[P_ISODATA_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_MAXENTROPY_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_MOMENTS_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_YEN_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_RENYI_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_SHANBHAG_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_INTERMODES_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_MINIMUM_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_MULTIPLE_OTSU] = m[P_RATS_THRESHOLD];
	m[P_REMOVE_SURROUNDING] = QList<ParamNameType>();
	m[P_GRAD_ANISO_DIFF_SMOOTH] = QList<ParamNameType>();
	m[P_CURV_ANISO_DIFF_SMOOTH] = QList<ParamNameType>();
	m[P_RECURSIVE_GAUSS_SMOOTH] = QList<ParamNameType>();
	m[P_BILATERAL_SMOOTH] = QList<ParamNameType>();
	m[P_CURV_FLOW_SMOOTH] = QList<ParamNameType>();
	m[P_MEDIAN_SMOOTH] = QList<ParamNameType>();
	m[P_ISOX_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_FHW_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_CREATE_SURROUNDING] = QList<ParamNameType>();
	m[P_HUANG_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_LI_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_KITTLERILLINGWORTH_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_TRIANGLE_THRESHOLD] = m[P_RATS_THRESHOLD];
	m[P_GENERAL_THRESHOLD] = QList<ParamNameType>();
	return m;
}
const MapId2Params FilterIdToOutputParamList = fill_FilterIdToOutputParamList();

inline bool existsBatchesRecord( QTableWidget const * computerCSV, QString algName, QString datasetName )
{
	for( int row = 1; row < computerCSV->rowCount(); ++row )
	{
		if( computerCSV->item( row, 2 )->text() == algName && computerCSV->item( row, 3 )->text() == datasetName )
			return true;
	}
	return false;
}

inline QString dirFromAlgAndDataset( QString algName, QString datasetName )
{
	QString strippedAlgName( algName.replace( " ", "" ) );
	QString strippedDatasetName( datasetName.replace( " ", "" ).replace( QRegularExpression( "\\.[^.]*$" ), "" ) ); //remove spaces and extension
	return strippedAlgName + "_" + strippedDatasetName;
}

inline void getAlgorithmAndDatasetNames( QTableWidget * settingsCSV, int row, QString * algName, QString * datasetName, int colOffset = 0 )
{
	*algName = settingsCSV->item( row, colOffset )->text();
	*datasetName = settingsCSV->item( row, colOffset + 1 )->text();
}

inline QString dirFromAlgAndDataset( QTableWidget * settingsCSV, int row )
{
	QString algName, datasetName;
	getAlgorithmAndDatasetNames( settingsCSV, row, &algName, &datasetName );
	return dirFromAlgAndDataset( algName, datasetName );
}

inline void getBatchesDirectoryAndFilename( QTableWidget * settingsCSV, int row, QString resultsFolder, QString * dirName, QString * batchesFile )
{
	QString algName, datasetName;
	getAlgorithmAndDatasetNames( settingsCSV, row, &algName, &datasetName );
	*dirName = dirFromAlgAndDataset( algName, datasetName );
	*batchesFile = resultsFolder + "/" + *dirName + "/batches.csv";
}

inline bool existsInBatches( QTableWidget &settingsCSV, int settings_row, QTableWidget &batchesData )
{
	// data
	for( int row = 1; row < batchesData.rowCount(); ++row )
	{
		bool exists = true;
		for( int col = 0; col < batchesData.columnCount(); ++col )
		{
			if( batchesData.item( row, col )->text() != settingsCSV.item( settings_row, col + 2 )->text() )
			{
				exists = false;
				continue;
			}
		}
		if( exists )
			return true;
	}
	return false;
}

inline bool isRandomSampling( QTableWidget *settingsCSV, int row )
{
	return (settingsCSV->item( row, 2 )->text().toInt() > 0);
}

inline int getNumRandomSamples( QTableWidget *settingsCSV, int row )
{
	return settingsCSV->item( row, 2 )->text().toInt();
}

inline int getRegularNumSamples( QTableWidget *settingsCSV, int row )
{
	return  settingsCSV->item( row, 3 )->text().section( ' ', -1 ).toInt();
}

template<typename T, typename U>
struct sameType { static const bool value = false; };
template<typename T>
struct sameType < T, T > { static const bool value = true; };  //specialization

struct IParameterInfo
{
	IParameterInfo( QString parameterName, int numberOfSamples ) : name( parameterName ), numSamples( numberOfSamples ), incr(0), sampleId(0) {}
	virtual ~IParameterInfo() {}

	virtual void reset() = 0;
	virtual void randomInRange() = 0;
	virtual bool increment() = 0;
	virtual int asInt() const = 0;
	virtual float asFloat() const = 0;
	virtual double asDouble() const = 0;
	virtual QString asString() const = 0;

	int numSamples;
	int sampleId;
	double incr;
	QString name;
};

template<typename T>
struct ParameterInfo : public IParameterInfo
{
	ParameterInfo( QString parameterName = "", int numberOfSamples = 0 ) : IParameterInfo( parameterName, numberOfSamples )
	{
		range[0] = range[1] = T( 0 );
	}
	ParameterInfo( QString parameterName, T min_val, T max_val, int numberOfSamples ) : IParameterInfo( parameterName, numberOfSamples )
	{
		range[0] = min_val;
		range[1] = max_val;
	}

	T range[2], val;

	inline double getIncrement()
	{
		if( numSamples > 1 )
			return (range[1] - range[0]) / (numSamples - 1);
		return 0.0;
	}
	inline virtual void reset()
	{
		val = range[0];
		sampleId = 0;
		incr = getIncrement();
	}
	inline virtual bool increment()
	{
		if( sampleId == (numSamples - 1) )
			return false;
		val += incr;
		++sampleId;
		return true;
	}
	inline virtual void randomInRange()
	{
		double randVal = qrand() / (double)RAND_MAX;
		randVal = range[0] + (range[1] - range[0]) * randVal;
		val = randVal;
	}
	inline virtual int asInt() const
	{
		if( sameType<T, int>::value )
			return val;
		throw itk::ExceptionObject( __FILE__, __LINE__, "Error: wrong parameter type is used!" );
		return 0;
	}
	inline virtual float asFloat() const
	{
		if( sameType<T, float>::value )
			return val;
		throw itk::ExceptionObject( __FILE__, __LINE__, "Error: wrong parameter type is used!" );
	}
	inline virtual double asDouble() const
	{
		if( sameType<T, double>::value )
			return val;
		throw itk::ExceptionObject( __FILE__, __LINE__, "Error: wrong parameter type is used!" );
	}
	inline virtual QString asString() const
	{
		return QString::number( val );
	}
};

inline bool incrementParameterSet( QList<IParameterInfo*> & parameters )
{
	if( parameters.isEmpty() )
		return false;
	IParameterInfo * parameter = parameters.first();
	int i = 0;
	while( true )
	{
		if( !parameter->increment() )
		{
			if( parameter == parameters.last() )
				return false;
			parameter->reset();
			parameter = parameters[++i];
		}
		else
			return true;
	}
}

inline void randomlySampleParameters( QList<IParameterInfo*> & parameters )
{
	foreach( IParameterInfo * p, parameters )
		p->randomInRange();
}

template<typename T>
inline ParameterInfo<T> * getParameterInfoType( QString paramName, QTableWidget *settingsCSV, int row, int col )
{
	QString thrStr = settingsCSV->item( row, col )->text();
	ParameterInfo<T> * paramInfo = new ParameterInfo < T > ;
	paramInfo->name = paramName;
	for( int i = 0; i < 2; ++i )
	{
		if( sameType<T, int>::value )
			paramInfo->range[i] = thrStr.section( ' ', i, i ).toInt();
		if( sameType<T, float>::value )
			paramInfo->range[i] = thrStr.section( ' ', i, i ).toFloat();
		if( sameType<T, double>::value )
			paramInfo->range[i] = thrStr.section( ' ', i, i ).toDouble();
	}
	if( thrStr.count( ' ' ) == 2 )
		paramInfo->numSamples = thrStr.section( ' ', -1 ).toInt();	//last Parameter = numSamples (no algo specific param)
	paramInfo->reset();
	return paramInfo;
}

inline IParameterInfo * getParameterInfo( const ParamNameType & paramNameType, QTableWidget *settingsCSV, int row, int col )
{
	switch( paramNameType.type() )
	{
		case PT_INT:
			return getParameterInfoType<int>( paramNameType.name(), settingsCSV, row, col );
			break;
		case PT_FLOAT:
			return getParameterInfoType<float>( paramNameType.name(), settingsCSV, row, col );
			break;
		case PT_DOUBLE:
			return getParameterInfoType<double>( paramNameType.name(), settingsCSV, row, col );
			break;
		default:
			throw itk::ExceptionObject( __FILE__, __LINE__, "Error: wrong parameter type is used in getParameterInfoType!" );
			return 0;
			break;
	}
}

inline void getListOfAlgorithms( QTableWidget & computerCSV, QStringList & algList )
{
	algList.clear();
	for( int row = 1; row < computerCSV.rowCount(); ++row )
		algList << computerCSV.item( row, 2 )->text();
	algList.removeDuplicates();
}

inline void getListOfDatasets( QTableWidget & computerCSV, QStringList & datasetList )
{
	datasetList.clear();
	for( int row = 1; row < computerCSV.rowCount(); ++row )
		datasetList << computerCSV.item( row, 3 )->text();
	datasetList.removeDuplicates();
}

inline QList<PorosityFilterID> parseFiltersFromString( QString algName )
{
	QStringList algs = algName.split( "_" );
	QList<PorosityFilterID> filterIds;
	foreach( QString a, algs )
		filterIds.append( FilterNameToId[a] );
	return filterIds;
}

inline double median( std::vector<double> vec )
{
	typedef std::vector<double>::size_type vec_sz;
	vec_sz size = vec.size();
	sort( vec.begin(), vec.end() );
	vec_sz mid = size / 2;
	return size % 2 == 0 ? ( vec[mid] + vec[mid - 1] ) / 2 : vec[mid];
}

inline double mean( std::vector<double> vec )
{
	return std::accumulate( vec.begin(), vec.end(), 0.0 ) / vec.size();
}

inline QMap<double, QList<double> > calculateHistogram( QList<double> data, double minX = 0, double maxX = 0 )
{
	double minmax[2] = { minX, maxX };
	int NumberOfBins = 1000;

	vtkSmartPointer<vtkDoubleArray> in = vtkSmartPointer<vtkDoubleArray>::New();
	in->SetNumberOfTuples( data.count() );
	for ( int i = 0; i < data.count(); ++i )
		in->SetValue( i, data.at( i ) );

	if ( minmax[0] == minmax[1] )
			minmax[1] = minmax[0] + 1.0;
	
	double inc = ( minmax[1] - minmax[0] ) / (NumberOfBins) * 1.001;
	double halfInc = inc / 2.0;

	vtkSmartPointer<vtkFloatArray> extents = vtkSmartPointer<vtkFloatArray>::New();
	extents->SetName( vtkStdString( "_extents" ).c_str() );

	extents->SetNumberOfTuples( NumberOfBins );
	float *centers = static_cast<float *>( extents->GetVoidPointer( 0 ) );
	double min = minmax[0] - 0.0005 * inc + halfInc;
	for ( int j = 0; j < NumberOfBins; ++j )
		extents->SetValue( j, min + j * inc );

	vtkSmartPointer<vtkIntArray> populations = vtkSmartPointer<vtkIntArray>::New();
	populations->SetName( vtkStdString( "_pops" ).c_str() );

	populations->SetNumberOfTuples( NumberOfBins );
	int *pops = static_cast<int *>( populations->GetVoidPointer( 0 ) );
	for ( int k = 0; k < NumberOfBins; ++k )
		pops[k] = 0;

	QMap<double, QList<double> > map;
	for ( double bin = 0.0; bin < extents->GetSize(); ++bin )
	{
		QList<double> list;
		map.insertMulti( bin, list );
	}

	for ( vtkIdType j = 0; j < in->GetNumberOfTuples(); ++j )
	{
		QList<double> list;
		double v( 0.0 );
		in->GetTuple( j, &v );
		for ( int k = 0; k < NumberOfBins; ++k )
		{
			if ( vtkMathUtilities::FuzzyCompare( v, double( centers[k] ), halfInc ) )
			{
				map.find( k )->append( j + 1 );	// table lines
				++pops[k];
				break;
			}
		}
	}

	return map;
}
inline QStringList getDatasetInfo( QString datasetDir, QString datasetName )
{
	QStringList datasetInfo;

	// Read *.mhd file
	QFile mhdFile( datasetDir + "/" + datasetName );
	if ( !mhdFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
	{
		datasetInfo.append( "<center><br>No dataset info (*.mhd) available.</center><br>" );
	}
	else
	{
		QTextStream tsMhdFilein( &mhdFile );
		while ( !tsMhdFilein.atEnd() )
		{
			QString str = tsMhdFilein.readLine();
			if ( str.section( ' ', 0, 0 ) == "ElementSpacing" )
			{
				// ElementSpacing
				datasetInfo.append(
					str.section( ' ', 0, 0 ) + "(voxel): "
					+ "X:" + str.section( ' ', 2, 2 )
					+ " Y:" + str.section( ' ', 3, 3 )
					+ " Z:" + str.section( ' ', 4, 4 ) );
				// DimSize
				str = tsMhdFilein.readLine();
				datasetInfo.append(
					str.section( ' ', 0, 0 ) + "(microns): "
					+ "X:" + str.section( ' ', 2, 2 )
					+ " Y:" + str.section( ' ', 3, 3 )
					+ " Z:" + str.section( ' ', 4, 4 ) );
				// ElementType
				str = tsMhdFilein.readLine();
				datasetInfo.append( str.section( ' ', 0, 0 ) + ": " + str.section( ' ', 2, 2 ) );
				//ElementDataFile
				str = tsMhdFilein.readLine();
				datasetInfo.insert( 0, "Dataset: " + str.section( ' ', 2, 2 ).section( '.', 0, 0 ) );
				break;
			}
		}
		mhdFile.close();	// done with *.mhd file
	}
		
	// Read *.mhd.info file
	QFile infoFile( datasetDir + "/" + datasetName + ".info" );
	if ( !infoFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
	{
		datasetInfo.append( "<center><br>No dataset intensity info (*.mhd.info) available.</center><br>" );
	}
	else
	{
		QTextStream tsInfoFile( &infoFile );
		QVector<double> bin, freq;
		tsInfoFile.readLine();
		for ( int i = 0; i < 6; ++i )	// first 7 lines (Datasetname, Min, Max...)
		{
			QString str = tsInfoFile.readLine();
			str.insert( str.indexOf( ":" ) + 1, " " );
			datasetInfo.append( str );
		}
		infoFile.close();  // done with *.mhd.info file
	}

	return datasetInfo;
}

inline void getDatasetHistogramValues( QString datasetDir, QString datasetName, QVector<double> &b, QVector<double> &f )
{
	// Read *.mhd.info file
	QString infoFilePath = datasetDir + "/" + datasetName + ".info";
	QFile infoFile( infoFilePath );
	if ( !infoFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
		return;
	QTextStream tsInfoFile( &infoFile );
	for ( int i = 0; i < 7; ++i )	// first 7 lines (Datasetname, Min, Max...)
		QString str = tsInfoFile.readLine();
	while ( !tsInfoFile.atEnd() )
	{
		QString str = tsInfoFile.readLine();
		b.append( str.section( ':', 0, 0 ).toDouble() );
		f.append( str.section( ':', 1, 1 ).toDouble() );
	}
	infoFile.close();  // done with *.mhd.info file
}

inline QString getMaskSliceDirName( const QString & maskFilename )
{
	QFileInfo maskFI( maskFilename );
	return maskFI.baseName() + "_slices";
}

inline QString getMaskSliceDirNameAbsolute( const QString & maskFilename )
{
	QFileInfo maskFI( maskFilename );
	return maskFI.absolutePath() + "/" + maskFI.baseName() + "_slices";
}

inline QString getSliceFilename( const QString &maskFilename, const int & sliceNumber )
{
	QFileInfo maskFI( maskFilename );
	return  maskFI.absolutePath()
		+ "/" + getMaskSliceDirName( maskFilename ) + "/"
		+ maskFI.baseName()
		+ "_Z_" + QString::number( sliceNumber ) + ".png";
}
