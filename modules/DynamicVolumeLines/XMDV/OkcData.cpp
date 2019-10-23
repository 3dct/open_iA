#include "OkcData.h"

 #include <QtWidgets/QMessageBox>
#include <QFileInfo>
#include <cstring>

//#include "main/XmdvTool.h"
#include "multidim/OkcDataModifierManager.h"
//#include "data/storage/CardStorage.h"
#include "util/exception/ReadOkcException.h"

#include <cstdio>//remove
#include <sys/stat.h>// get file info
#include <ctime>
#include <iostream>
#include <string>
#include <cassert>
using namespace std;


OkcData::OkcData() {
	data_size = 0;
	dims = 0;
	data_buf = new std::vector<double>();
	// Initialize the filename to a null string
	filepath[0] = 0;
	filename[0] = 0;
	m_okcDataModifierManager = new OkcDataModifierManager(this);
}

OkcData::OkcData(OkcData* okcdata) {
	m_okcDataModifierManager = new OkcDataModifierManager();
	copyFrom(okcdata);
}

OkcData::~OkcData()
{
	int i;
	for (i=0; i<(int)names.size(); i++)
	{
		SAFE_DELETE_ARR(names[i]);
	}
	if (isBaseOkcData()) {
		SAFE_DELETE(data_buf);
	}

	SAFE_DELETE(m_okcDataModifierManager);
}

void OkcData::copyFrom(const OkcData* okcData){
	this->data_size = okcData->data_size;
	this->dims = okcData->dims;
	this->data_buf = okcData->data_buf;
	setBaseFlag(false);
	if (this->dims>0) {
		dim_min.resize(this->dims);
		dim_max.resize(this->dims);
		cardinality.resize(this->dims);
		names.resize(this->dims);

		int i;
		for (i=0; i<this->dims; i++) {
			dim_min[i] = okcData->dim_min[i];
			dim_max[i] = okcData->dim_max[i];
			cardinality[i] = okcData->cardinality[i];
			if (okcData->names[i]==0) {
				// For some virtual OkcData, such as brush storage, names[i]=0
				names[i] = 0;
			} else {
				names[i] = new char[strlen(okcData->names[i])+1];
				strcpy(names[i], okcData->names[i]);
			}
		}
	}

	// the operator "=" of OkcDataModifierManager will
	// copy all of modifiers.
	*m_okcDataModifierManager = *(okcData->m_okcDataModifierManager);
}

void OkcData::copyDataBufFrom(const OkcData* okcData) {
	if (isBaseOkcData()) {
		delete data_buf;
	}
	data_buf = new std::vector<double>();
	*(data_buf) = *(okcData->data_buf);
	setBaseFlag(true);
}

QStringList OkcData::toStringList() {
	QStringList result;
	result << "Multi-dimensional Dataset:";
	result << QString("-- File name : %1").arg( QString(filename) );
	result << QString("-- Full path : %1").arg( QString(filepath) );
	result << QString("-- The number of records : %1").arg( data_size );
	result << QString("-- The number of dimensions : %1").arg( dims );
	result << QString("-- All dimensions: (name, min, max)");

	int i;
	for ( i=0; i<(int)names.size(); i++) {
		result << QString("----%1. %2(%3,%4)")
			.arg( i )
			.arg( QString( names[i] ) )
			.arg( dim_min[i] )
			.arg( dim_max[i] );
	}

	return result;
}

bool OkcData::isBaseOkcData() {
	return m_isBaseOkcData;
}

void OkcData::setBaseFlag(bool baseFlag) {
	m_isBaseOkcData = baseFlag;
}

const char * OkcData::read_till(FILE *fp,
		const char *delimiters = "\n",
		char *which_delimiter = 0)
{
	static char str[500];
	int k = 0;
	while (1) {
		char c = fgetc(fp);
		const char *ptr = strchr(delimiters, c);
		if (ptr) {
			str[k] = '\0';
			if (which_delimiter)
				*which_delimiter = *ptr;
			break;
		}
		str[k++] = c;
	}

	return str;
}
/*
bool OkcData::read(const char *fname){
	string ext = strrchr(fname, '.');

	// open and read the data file based on the file extension name
	if(ext==".okc"){
		OkcData::ReadOkcResult result = readOKC(fname);
		if ( result.success && result.outOfRange ) {
	        QMessageBox::critical(0, QString("Values Out of Range!"),
					QString("The file %1 has been read successfully, "
							"but some values are out of range.  "
							"XmdvTool has adjusted the ranges specified in Okc data "
							"and wrote a new Okc file to %2. "
							"Please refer to log file %3 for details.")
							.arg(fname)
							.arg( QString(result.newOkcFileName.c_str() ) )
							.arg( QString(result.logFileName.c_str() )) );

		}
		return true;
	} else if(ext==".csv"){
		return readCSV(fname);
	}
	return false;
}
*/

// count the number of appearing of one substing in the string
int OkcData::strstr_cnt(const char *string, const char *substring) {
	int i, j, k, count = 0;
	for (i = 0; string[i]; i++) {
		for (j = i, k = 0; (string[j] == substring[k] && (j < (int)strlen(string))); j++, k++) {
			if (!substring[k + 1]) {
				count++;
			}
		}
	}
	return count;
}


// get the position of a substring in a string
int OkcData::substring_index(const char *s1,const char *s2, int pos){
    int i,j,k;
    for( i = pos ; s1[i] ; i++ ) {
        for( j = i, k = 0 ; s1[j] == s2[k]; j++,k++ ){
            if (! s2[k + 1]) {
                return i;
            }
        }
    }
    return -1;
}


char* OkcData::fgetcsvline(vector<string> &csv_databuf, FILE *fhead) {
    char  *ret_stat;
    char  data_buf[1024];
    string stringbuf;
    ret_stat = fgets( data_buf, 1024, fhead );

    if (ret_stat != nullptr) {
        int len = strstr_cnt(data_buf,",");
        if (len > 0){
            int pos = substring_index(data_buf,",",0);
            int startpos = 0;
            string csv_buf;
            while (pos > 0) {
                stringbuf = (string)data_buf;
                csv_buf = stringbuf.substr(startpos, pos - startpos);
                char* name_ = new char[csv_buf.length() + 4];
                strcpy(name_, csv_buf.c_str());
                //printf("%s \n", csv_buf);
                csv_databuf.push_back(csv_buf);
                startpos = pos + 1;
                pos = substring_index(data_buf,",",pos+1);
                SAFE_DELETE(name_);
            }
            if ((substring_index(data_buf, "\n", 0)) > 0) {
            	int length = stringbuf.length();
				csv_buf = stringbuf.substr(startpos, length - startpos - 1);
				char* name_ = new char[csv_buf.length() + 4];
				strcpy(name_, csv_buf.c_str());
				SAFE_DELETE(name_);
			} else {
				csv_buf = stringbuf.substr(startpos, stringbuf.length()- startpos - 1);
			}
            csv_databuf.push_back(csv_buf);
        }
    }

    return ret_stat;
}
/*
bool OkcData::readCSV(const char *fname){
	char str_temp[500];
	FILE *fp = fopen(fname, "r");
	if (fp == nullptr) {
		sprintf(str_temp, "Data file %s doesn't exist\n", fname);
		throw ReadOkcException(str_temp);
	}

	strcpy( filepath, fname );
	// filename does not contain the path
	strcpy( filename, QFileInfo(fname).fileName().toStdString().c_str() );
	// title does not contain the path and suffix
	setTitle ( QFileInfo(fname).baseName () );

	char *ret_stat;
	vector<string> csv_data;
	ret_stat = fgetcsvline(csv_data, fp);

	// get the dimenison number
	dims = csv_data.size();

	names.resize(dims);

	int i;
	string dname;
	for (i = 0; i < dims; i++) {
		dname = csv_data[i];

		char* name_ = new char[dname.length() + 4];
		strcpy(name_, dname.c_str());

		SAFE_DELETE(name_);
		names[i] = new char[dname.length() + 4];
		strcpy(names[i], dname.c_str());
	}

	dim_min.resize(dims);
	dim_max.resize(dims);
	cg_dim_min.resize(dims);
	cg_dim_max.resize(dims);
	cardinality.resize(dims);

	string sValue;

	int index = 0;

	vector <vector < double > > all_data_values;
	vector<double> one_data;

	int loop = 2;
	csv_data.clear();
	ret_stat = fgetcsvline(csv_data, fp);
	while (ret_stat != nullptr && strlen(ret_stat)!=0) {
		index+=dims;
		for (i = 0; i < dims; i++) {
			sValue =  csv_data[i];
			char* value_ = new char[sValue.length() + 4];
			strcpy(value_, sValue.c_str());
			double v = atof(value_);

			one_data.push_back (v);
			SAFE_DELETE(value_);
		}

		all_data_values.push_back (one_data);
		one_data.clear();

		loop++;
		csv_data.clear();
		ret_stat = fgetcsvline(csv_data, fp);
	}
	fclose(fp);

	data_size = all_data_values.size();

	data_buf->resize(dims * data_size);

	for (int j = 0; j < dims; j++) {
		dim_min[j] = all_data_values[0][j];
		dim_max[j] = all_data_values[0][j];
	}

	int base = 0;

	for (i = 0; i < data_size; i++) {
		for (int j = 0; j < dims; j++) {
			double v = all_data_values[i][j];
			//double max = dim_max[j];
			//double min = dim_min[j];
			if(v>dim_max[j]){
				dim_max[j] = v;
			}else if(v<dim_min[j]){
				dim_min[j] = v;
			}
			(*data_buf)[base++] = v;
		}
	}

	for (i = 0; i < dims; i++) {
		cg_dim_min[i] = dim_min[i];
		cg_dim_max[i] = dim_max[i];
		// Need to handle case when min = max (separate them a bit)
		//
		if (dim_min[i] == dim_max[i]) {
			if (dim_min[i] == 0.0) {
				dim_min[i] = -1.0;
				dim_max[i] = 1.0;
			} else {
				if (dim_min[i] < 0.0) {
					dim_min[i] = 2. * dim_min[i];
					dim_max[i] = 0.0;
				} else {
					dim_max[i] = 2. * dim_min[i];
					dim_min[i] = 0.0;
				}
			}
		} else {
			// Add a bit of space to the bounds to avoid boundary problems
			//
			const double SPACE = .05; // extra space for bounds to avoid landing on edges

			double dim_diff = (dim_max[i] - dim_min[i]) * SPACE;
			dim_max[i] += dim_diff;
			dim_min[i] -= dim_diff;
		}

	}
	for (i = 0; i < data_size; i++) {
		all_data_values[i].clear();
	}
	all_data_values.clear();

	createOKC(fname);

	return true;
}

bool OkcData::createOKC(const char *fname){
	char okc_filename[FILENAME_MAXLEN];
	char csv_filename[FILENAME_MAXLEN];
	strcpy(okc_filename, fname);
	strcpy(csv_filename, fname);

	char * ext;
	ext = strrchr(okc_filename,'s');
	string okc_ext = ".okc";

	int size_t = strlen(okc_filename);

	okc_filename[size_t-4] = 0;

	strcat(okc_filename, ".okc");

	struct stat okc_buf, csv_buf;
	FILE *okc_fp = fopen(okc_filename, "r");

	if (okc_fp == nullptr) {
		// doesn't exist the corresponding okc file create one here and write
		writeOKC(okc_filename);
	}else{
		// does exist the corresponding okc file, compare the creation time of okc file and the modification time csv file
		stat(okc_filename, &okc_buf);
		stat(csv_filename, &csv_buf);

		char csv_datetime[100] = {0};
		struct tm* csv_time;
		struct tm* okc_time;

		struct tm csv_time2;

		csv_time = localtime(&csv_buf.st_mtime);
		csv_time2 = *csv_time;
		csv_time = &csv_time2;
		strftime(csv_datetime, 100, "%c", csv_time);


		char okc_datetime[100] = {0};
		okc_time = localtime(&okc_buf.st_mtime);
		strftime(okc_datetime, 100, "%c", okc_time);

		int a = cmpTime(csv_time, okc_time);

		// change this to ( a > 0 )
		if( a > 0 ){
			// close the opened file for opening with another mode
			fclose(okc_fp);
			writeOKC(okc_filename);
		}else{
			return true;
		}
	}

	return true;
}

//
bool OkcData::writeOKC(const char *fname){
	// open with "w" mode, this means clean all the contents in this file for re-writing
	// if doesn't exist the file, create a new empty file
	FILE* fp = fopen(fname, "w");

	fprintf(fp, "%d %d\n", dims, data_size);

	// write all dim names
	for(int i=0; i<dims; i++){
		fprintf(fp, "%s\n", names[i]);
	}

	int cardinality = 4;

	// write all dim min and max
	for(int i=0; i<dims; i++){
		fprintf(fp, "%.6lf %.6lf %d \n", cg_dim_min[i], cg_dim_max[i], cardinality);
	}

	int k = 0;

	// write all data

	// print the first row
	for(int j=0; j<dims-1; j++){
		fprintf(fp, "%.6lf ", (*data_buf)[k++]);
	}
	// print the last value in a row and go to the next row
	fprintf(fp, "%.6lf", (*data_buf)[k++]);

	for(int i=1; i<data_size; i++){
		fprintf(fp, "\n");
		for(int j=0; j<dims-1; j++){
			fprintf(fp, "%.6lf ", (*data_buf)[k++]);
		}
		// print the last value in a row
		fprintf(fp, "%.6lf", (*data_buf)[k++]);
	}

	fclose(fp);

	return true;
}

*/

// if return value > 0, means time_a is later than time_b
int OkcData::cmpTime(const struct tm* time_a, const struct tm* time_b){
	if (time_a->tm_year - time_b->tm_year != 0) {
		return (time_a->tm_year - time_b->tm_year);
	}
	if (time_a->tm_yday - time_b->tm_yday != 0) {
		return (time_a->tm_yday - time_b->tm_yday);
	}
	if (time_a->tm_hour - time_b->tm_hour != 0) {
		return (time_a->tm_hour - time_b->tm_hour);
	}
	if (time_a->tm_min - time_b->tm_min != 0) {
		return (time_a->tm_min - time_b->tm_min);
	}
	if (time_a->tm_sec - time_b->tm_sec != 0) {
		return (time_a->tm_sec - time_b->tm_sec);
	}

	return 0;
}

////////////////////////////////////////
// Read in data file: Format is:
// number_of_dimensions number_of_samples (int int)
// dimension_name for each dimension (string)
// minimum maximum cardinality for each dimension (double double int)
// data_samples (doubles)
//
/*
OkcData::ReadOkcResult OkcData::readOKC(const char *fname)
{
	bool success = true;
	bool outOfRange = false;
	std::string newOkcFileName = std::string();
	std::string logFileName = std::string();

	char str_temp[500];
	FILE *fp = fopen(fname, "r");
	if (fp == nullptr) {
		sprintf(str_temp, "Data file %s doesn't exist\n", fname);
		throw ReadOkcException(str_temp);
	}

	strcpy(filepath, fname);
	// filename does not contain the path
	strcpy( filename, QFileInfo(fname).fileName().toStdString().c_str() );
	// title does not contain the path and suffix
	setTitle (QFileInfo(fname).baseName () );

	int ret_size;
	if ((ret_size=fscanf(fp, "%d %d\n", &dims, &data_size)) <= 0) {
		sprintf(str_temp, "Error reading first line of file %s! Please try loading another file!\n", fname);
		throw ReadOkcException(str_temp);
	}

	if (data_size <= 0) {
		sprintf(str_temp, "Number of data items is negative or zero (%d)! Try loading another file!\n", data_size);
		throw ReadOkcException(str_temp);
	}

	if (dims <= 0) {
		sprintf(str_temp, "Number of dimensions is negative or zero (%d)! Try loading another file!\n", data_size);
		throw ReadOkcException(str_temp);
	}

	// Read in names of dimensions, and if necessary, nominal
	// encodings of values.
	//
	names.resize(dims);

	int i;
	for (i = 0; i < dims; i++)
	{
		char endchar;
		const char *dname = read_till(fp, "(\n", &endchar);
		names[i] = new char[strlen(dname) + 4];
		strcpy(names[i], dname);

		if (endchar == '(') {
			read_till(fp, "\n");
		}
	}

	dim_min.resize(dims);
	dim_max.resize(dims);
	cg_dim_min.resize(dims);
	cg_dim_max.resize(dims);
	cardinality.resize(dims);

	// origDimMin, origDimMax are the original values
	// for max and min values for each dimension.
	// This is used in the log file because the values
	// in dim_min and dim_max might be adjusted.
	vector<double> origDimMin, origDimMax;
	origDimMin.resize(dims);
	origDimMax.resize(dims);
	for (i = 0; i < dims; i++)
	{
		if (fscanf(fp,
				"%lf %lf %d",
				&dim_min[i],
				&dim_max[i],
				&cardinality[i]) != 3) {
			sprintf(str_temp, "failed in reading dimension ranges for dim %d.\n", i);
			throw ReadOkcException(str_temp);
		}
		if ( dim_min[i] > dim_max[i] ) {
			sprintf(str_temp, "The range for dimension %d is invalid. "
					"The first number (min) should smaller than "
					"or equal to the second number (max).\n", i);
			throw ReadOkcException(str_temp);
		}
		cg_dim_min[i] = dim_min[i];
		cg_dim_max[i] = dim_max[i];
		origDimMin[i] = dim_min[i];
		origDimMax[i] = dim_max[i];
	}

	data_buf->resize(dims * data_size);

	// outOfRangeLine, outOfRangeCol are used to store the
	// position for those values out of range
	std::vector<int> outOfRangeLine, outOfRangeCol;
	int k = 0;

	for (i = 0; i < data_size; i++) {
		for (int j = 0; j < dims; j++,k++) {
			if (fscanf(fp, "%lf", &(*data_buf)[k]) != 1) {
				sprintf(str_temp, "failed in reading data, line %d, item %d.\n", i, j);
				throw ReadOkcException(str_temp);
			}
			// Judge whether all values are within the defined range.
			// If not, adjust the range and notify users.
			if ( (*data_buf)[k] < dim_min[j] ) {
				outOfRangeLine.push_back(i);
				outOfRangeCol.push_back(j);
				dim_min[j] = (*data_buf)[k];
				cg_dim_min[j] = dim_min[j];
			}
			if ( (*data_buf)[k] > dim_max[j] ) {
				outOfRangeLine.push_back(i);
				outOfRangeCol.push_back(j);
				dim_max[j] = (*data_buf)[k];
				cg_dim_max[j] = dim_max[j];
			}
		}
	}

	fclose(fp);

	// If there are any values out of range,
	// write a log file
	if ( outOfRangeLine.size()>0 ) {
		// Create a new Okc File to store data with fixed range.
		// If the data is cars.okc, this Okc file is cars_fix.okc
		newOkcFileName = fname;
		newOkcFileName.insert( newOkcFileName.find_last_of ('.'), "_fix");
		writeOKC( newOkcFileName.c_str() );

		outOfRange = true;
		// outOfRangeLine should have the same size as outOfRangeCol
		assert ( outOfRangeLine.size() == outOfRangeCol.size() );
		logFileName = fname;
		logFileName.append(".log");
		FILE *fp_log = fopen(logFileName.c_str(), "w");
		fprintf (fp_log, "There are some values which are out of range.\n");
		fprintf (fp_log, "XmdvTool has adjusted the ranged specified in Okc data ");
		fprintf (fp_log, "and wrote a new Okc file to %s. \n", newOkcFileName.c_str() );
		fprintf (fp_log, "The values out of range are as below:\n");
		int i;
		for (i=0; i<(int)outOfRangeLine.size(); i++) {
			int line = outOfRangeLine[i];
			int col = outOfRangeCol[i];
			fprintf (fp_log, "The value %lf at line %d col %d is out of range(%lf, %lf)\n",
					(*data_buf)[  line * dims + col ],
					line,
					col,
					origDimMin[col],
					origDimMax[col]);
		}
		fclose(fp_log);
	} else {
		outOfRange = false;
	}

	// Adjust dim_min and dim_max when dim_min[i]==dim_max[i].
	// Add a bit of space to the bounds to avoid boundary problems
	for (i = 0; i < dims; i++)
	{
		// Need to handle case when min = max (separate them a bit)
		//
		if (dim_min[i] == dim_max[i]) {
			if (dim_min[i] == 0.0) {
				dim_min[i] = -1.0;
				dim_max[i] = 1.0;
			} else {
				if (dim_min[i] < 0.0) {
					dim_min[i] = 2. * dim_min[i];
					dim_max[i] = 0.0;
				} else {
					dim_max[i] = 2. * dim_min[i];
					dim_min[i] = 0.0;
				}
			}
		} else {
			// Add a bit of space to the bounds to avoid boundary problems
			//
			const double SPACE = .05; // extra space for bounds to avoid landing on edges

			double dim_diff = (dim_max[i] - dim_min[i]) * SPACE;
			dim_max[i] += dim_diff;
			dim_min[i] -= dim_diff;
		}

	}

	return OkcData::ReadOkcResult( success, outOfRange, newOkcFileName, logFileName );
}
*/

int OkcData::getOrigDataSize() {
	return data_size;
}

int OkcData::getDataSize() {
	return m_okcDataModifierManager->getDataSize(data_size);
}

int OkcData::getOrigDimSize() {
	return dims;
}

int OkcData::getDimSize() {
	return m_okcDataModifierManager->getDimSize(dims);
}

void OkcData::GetDimensionData(std::vector<double> &data, int dimIdx){
	int i;
	data.resize(data_size);

	for (i = 0; i < data_size; i++)
		data[i] = (*data_buf)[ i * dims + dimIdx];
}

void OkcData::getData(std::vector<double> &buf, int line)
{
	int i;
	std::vector<double> temp_buf;
	temp_buf.resize(dims);
	// if the order for rows has been changed,
	// or sampling has been applied to the dataset,
	// we need call getOrigLine() to get the actual line no
	// in the original data
	int origLine = m_okcDataModifierManager->getOrigLine(line);
	int base = dims * origLine;
	for (i = 0; i < dims; i++)
		temp_buf[i] = (*data_buf)[base + i];
	m_okcDataModifierManager->mapData(temp_buf, buf);
}

void OkcData::setData(std::vector<double> &data, int line) {
	std::vector<double> origData;
	//obtain the original data
	getOrigData(origData, line);
	// invMapData() will change only those dimensions in current view
	m_okcDataModifierManager->invMapData(data, origData);
	setOrigData(origData, line);
}

void OkcData::getOrigData(std::vector<double> &data, int line) {
	int i;
	data.resize(dims);
	int base = dims *line;
	for (i = 0; i < dims; i++)
		data[i] = (*data_buf)[base + i];
}

void OkcData::setOrigData(std::vector<double> &data, int line) {
	assert( (int)data.size() == dims );
	int i, base = dims *line;
	for (i = 0; i < dims; i++)
		(*data_buf)[base + i] = data[i];
}


void OkcData::getDimMinArr(std::vector<double> &buf) {
	m_okcDataModifierManager->mapData(dim_min, buf);
}

void OkcData::getDimMaxArr(std::vector<double> &buf) {
	m_okcDataModifierManager->mapData(dim_max, buf);
}

void OkcData::getOrigDimMinArr(std::vector<double> &buf){
	int dim_size = getOrigDimSize();
	buf.resize(dim_size);
	for(int i=0; i<dim_size; i++){
		buf[i] = dim_min[i];
	}

}
void OkcData::getOrigDimMaxArr(std::vector<double> &buf){
	int dim_size = getOrigDimSize();
	buf.resize(dim_size);
	for(int i=0; i<dim_size; i++){
		buf[i] = dim_max[i];
	}

}

void OkcData::getCgDimMinArr(std::vector<double> &buf) {
	m_okcDataModifierManager->mapData(cg_dim_min, buf);
}

void OkcData::getCgDimMaxArr(std::vector<double> &buf) {
	m_okcDataModifierManager->mapData(cg_dim_max, buf);
}

void OkcData::getDimNameArr(std::vector<char*> &buf) {
	m_okcDataModifierManager->mapData(names, buf);
}

void OkcData::getOrigDimNameArr(std::vector<char*> &buf) {
	int dim_size = getOrigDimSize();
	buf.resize(dim_size);
	for(int i=0; i<dim_size; i++){
		buf[i] = names[i];
	}
}

void OkcData::getCardinality(std::vector<int> &buf) {
	m_okcDataModifierManager->mapData(cardinality, buf);
}

OkcDataModifierManager* OkcData::getOkcDataModifierManager() {
	return m_okcDataModifierManager;
}

double OkcData::getSingleDataAttribute(int line, int dimIdx){
	int base = dims *line;
	return (*data_buf)[base + dimIdx];
}

//void OkcData::updateCardinalityByStorage(CardStorage* storage) {
//	int i;
//	assert( storage->data_size == 1 );
//	assert( storage->dims == this->dims );
//
//	// Retrieve the cardinality fromt the storage
//	std::vector<double> card;
//	card.resize(dims);
//	storage->getOrigData(card, 0);
//
//	// Assign the cardinality numbers to the cardinality array
//	this->cardinality.resize(dims);
//	for (i=0; i<this->dims; i++) {
//		// We add 0.1 to the card[i] to avoid the float computation
//		this->cardinality[i] = (int)(card[i]+0.1);
//	}
//}
