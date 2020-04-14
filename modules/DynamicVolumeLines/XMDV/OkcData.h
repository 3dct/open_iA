/*
 * this class contains the information for an okc dataset
 * and the functions to manipulate it
 */

#ifndef OKCDATA_H_
#define OKCDATA_H_

#include "data/Data.h"

//#include "datatype/XmdvType.h"
#include "main/XmdvTool.h"

#include <vector>
#include <string>

class OkcDataModifierManager;
//class CardStorage;

class OkcData : public Data
{
	// make the class Brush as the friend class of OkcData
	// to allow a brush be converted to an OkcData storage
	friend class Brush;

public:
	class ReadOkcResult {
	public:
		ReadOkcResult(bool s, bool out, std::string newOkc, std::string log)
			: success(s), outOfRange(out),
			newOkcFileName(newOkc), logFileName(log) {
		}
		// If success==true, this instance of OkcData is a valid one
		bool success;
		// If outOfRange==true, there are some values out of the range.
		// The dim_min and dim_max have been automatically adjusted.
		// NOTE: This instance of OkcData is still valid
		bool outOfRange;
		// If outOfRange==true, XmdvTool will create a new Okc file.
		// This is its file name.
		std::string newOkcFileName;
		// The file name for the log.  Currently, it is only used to store
		// all values of range.  If no value is out of range, this is an empty
		// string.
		std::string logFileName;
	};

public:
	OkcData();
	OkcData(OkcData* okcdata);
	~OkcData();

private:
	static const char * read_till(FILE *fp,
			const char *delimiters,char *which_delimiter);

public:
	// copy all of data from another OkcData structure.
	// Please note that only the pointer for data_buf will
	// be copied to the current instance for efficiency reason.
	// The contents of dim attributes will be deeply copied, including
	// dim_min, dim_max, names and cardinality.
	void copyFrom(const OkcData* okcData);
	// copy the data_buf from another okcData.
	// The function copyFrom() only copy a pointer of data_buf,
	// But this function will recreate the whole buffer.
	// It is called when operator need create the copy of the input data
	void copyDataBufFrom(const OkcData* okcData);

public:
	OkcDataModifierManager* getOkcDataModifierManager();

	// m_isBaseOkcData = true if the data in data_buf
	// in this OkcData is created from scratch;
	// m_isBaseOkcData = false if the data_buf
	// in this OkcData points to a data_buf in other OkcData;
	bool m_isBaseOkcData;

	//path of the okc file, from user input or default
	// For example: c:\foo\cars.okc
	char filepath[FILENAME_MAXLEN];
	// the name of the okc file, from user input or default.
	// It does not include the path
	// For example: cars.okc
	char filename[FILENAME_MAXLEN];

	////////////////////////////////////////////////////
	// Original data read from file.
	//The order of the dimensions could be different from what's
	//in the file since if reorder is performed,
	//the file will be re-read according to the new order.
	//

	//number of data points contained in the okc file
	int	data_size;
	//number of original dimensions contained in the okc file
	int	dims;

	//size of each of these two arrays is the above int dims.
	//The values of these two arrays are loaded from Okc files.
	//cg_dim_min[k] is the minimum of the kth dimensional values
	//of all the data points.
	//cg_dim_max[k] is the maximum of the kth dimensional values
	//of all the data points.
	//We use these two arrays during clustering, so
	//we use the prefix cg.
	std::vector<double> cg_dim_min, cg_dim_max;

	//dim_min and dim_max contain adjusted values
	//adapted from cg_dim_min and cg_dim_max.
	//In general, dim_min[i] is a little smaller than cg_dim_min[i]
	//and dim_max[i] is a little larger than cg_dim_max[i].
	//These two arrays are used in all visualizations,
	//so the final pictures has some reasonable margins.
	std::vector<double> dim_min, dim_max;

	//size of this array is int all_dims.
	//all_cardinality[k] is the cardinality of the kth dimension
	//related to number of bins in dimension stacking.
	std::vector<int> cardinality;
	//size of this array is int all_dims.
	//all_names[k] is the name of the kth dimension.
	std::vector<char*> names;
	//buffer in main memory for storing all the data points in full dimensions
	std::vector<double>* m_data_buf;

private:
	// an object to manage all OkcData modifiers.
	OkcDataModifierManager* m_okcDataModifierManager;

	int cmpTime(const struct tm* time_a, const struct tm* time_b);

public:
	// This function overrides the virtual function in the base class
	QStringList toStringList();

	/*
	// apparently these functions are not used!
	// read the data file and write to data member
	bool read(const char *fname);
	// read csv file format data file
	bool readCSV(const char *fname);
	// read okc file format data file
	OkcData::ReadOkcResult readOKC(const char *fname);
	// create a new okc file when reading csv file
	bool createOKC(const char *fname);
	// write the okc data into an okc fie; if the okc file exists, erase all contents and write
	bool writeOKC(const char *fname);
	*/

	// Get the number of data points in the original data_buf
	int getOrigDataSize();
	// Get the number of data points if no modifier exist
	// or this number in the view if a modifier changes it.
	int getDataSize();
	// Get the number of dimensions in the original data_buf
	int getOrigDimSize();
	// Get the number of dimensions if no modifier exist
	// or this number in the view if a modifier changes it.
	int getDimSize();

	// Get all the data values for a single dimension
	void GetDimensionData(std::vector<double> &data, int dimIdx);

	// Get a data point if no modifier exist
	// or its view if a modifier changes it.
	void getData(std::vector<double> &data, int line);
	// Assign the data to a data point in a specific line
	// if no modifier exist
	// or regard data as a view to do assignment
	void setData(std::vector<double> &data, int line);

	// Get a data point ignoring the modifier.
	// This should be used sparely because it ignore modifiers.
	void getOrigData(std::vector<double> &data, int line);
	// Assign the data to a data point in a specific line
	// This should be used sparely because it ignore modifiers.
	void setOrigData(std::vector<double> &data, int line);

	// Get the array to store dimension min values if no modifier exist
	// or its view if a modifier changes it.
	void getDimMinArr(std::vector<double> &buf);
	// Get the array to store dimension max values if no modifier exist
	// or its view if a modifier changes it.
	void getDimMaxArr(std::vector<double> &buf);
	// Get the original array to store dimension min values,
	// ignoring any modifiers.  This should be used rarely.
	void getOrigDimMinArr(std::vector<double> &buf);
	// Get the original array to store dimension max values,
	// ignoring any modifiers.  This should be used rarely.
	void getOrigDimMaxArr(std::vector<double> &buf);
	// Get the array to store dimension names if no modifier exist
	// or its view if a modifier changes it.
	void getDimNameArr(std::vector<char*> &buf);
	// Get the original array to store dimension names,
	// ignoring any modifiers.  This should be used rarely.
	void getOrigDimNameArr(std::vector<char*> &buf);

	// Get the array to store dimension cg_min values  if no modifier exist
	// or its view if a modifier changes it.
	void getCgDimMinArr(std::vector<double> &buf);
	// Get the array to store dimension cg_max values if no modifier exist
	// or its view if a modifier changes it.
	void getCgDimMaxArr(std::vector<double> &buf);

	void getCardinality(std::vector<int> &buf);

	// return true if the data_buf in this OkcData
	// is created from scratch instead of from another class
	bool isBaseOkcData();
	void setBaseFlag(bool baseFlag);

	// count the number of appearing of one substing in the string
	int strstr_cnt(const char *string, const char *substring);
	// read a single line in csv file
	char *fgetcsvline(vector<string> &csv_databuf, FILE *fhead);
	// get the position of a substring in a string
	int substring_index(const char *s1,const char *s2, int pos);

	// get the one attribute value (dimIdx) of one data line
	double getSingleDataAttribute(int line, int dimIdx);

	//void updateCardinalityByStorage(CardStorage* storage);
};

#endif /*OKCDATA_H_*/
