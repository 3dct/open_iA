// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaio_export.h"

#include <itkImage.h>

#include <vtkSmartPointer.h>

#include <vector>

class iAConnector;

class vtkImageData;

class QString;


//! Helper class for reading Olympus image files (.oif). Adapted from FluoRenderer code.
class iAOIFReaderHelper
{
protected:
	std::wstring m_id_string;  //!< the path and file name used to read files
	int m_resize_type;         //!< 0: no resizing; 1: padding; 2: resampling
	int m_resample_type;       //!< 0: nearest neighbour; 1: linear
	int m_alignment;           //!< padding alignment

	//! @{ 3D Batch
	bool m_batch;
	std::vector<std::wstring> m_batch_list;
	int m_cur_batch;
	//! @}

	std::wstring m_path_name;
public:
	using TiffImgType = itk::Image<unsigned short, 2>;
	using TiffImgPtr = TiffImgType::Pointer;
	using ResultImgType = itk::Image<unsigned short, 3>;
	using ResultImgPtr = ResultImgType::Pointer ;

	iAOIFReaderHelper();

	void SetFile(std::string& file);
	void SetFile(std::wstring& file);
	void SetTimeId(std::wstring& id);
	std::wstring GetTimeId();
	void Preprocess();
	void SetBatch(bool batch);
	int LoadBatch(int index);
	std::wstring GetCurName(int t, int c);

	std::wstring GetPathName() { return m_path_name; }
	std::wstring GetDataName() { return m_data_name; }
	int GetTimeNum() { return m_time_num; }
	int GetCurTime() { return m_cur_time; }
	int GetChanNum() { return m_chan_num; }
	double GetExcitationWavelength(int chan);
	int GetSliceNum() { return m_slice_num; }
	int GetXSize() { return m_x_size; }
	int GetYSize() { return m_y_size; }
	bool IsSpcInfoValid() { return m_valid_spc; }
	double GetXSpc() { return m_xspc; }
	double GetYSpc() { return m_yspc; }
	double GetZSpc() { return m_zspc; }
	double GetMaxValue() { return m_max_value; }
	double GetScalarScale() { return m_scalar_scale; }
	bool GetBatch() { return m_batch; }
	int GetBatchNum() { return (int)m_batch_list.size(); }
	int GetCurBatch() { return m_cur_batch; }
	void Load();
	ResultImgPtr GetResult(int chanIdx);

private:
	void Read(int t, int c, bool get_max);
	TiffImgPtr ReadTiffImage(const QString& file_name);

	std::vector<ResultImgPtr> m_result;

	std::wstring m_data_name;
	std::wstring m_subdir_name;

	int m_type;	//!< 0-time data in a single file; 1-time data in a file sequence
	typedef std::vector<std::wstring> ChannelInfo;  //!< slices form a channel
	typedef std::vector<ChannelInfo> DatasetInfo;   //!< channels form dataset
	struct TimeDataInfo
	{
		int filenumber;                    //!< if type is 1, file number for time data
		std::wstring filename;             //!< if type is 1, file name for current time data
		std::wstring subdirname;           //!< subdirectory name
		DatasetInfo dataset;               //!< a list of the channels
	};
	std::vector<TimeDataInfo> m_oif_info;  //!< time data form the complete oif dataset
	int m_oif_t;                           //!< current time point in oib info for reading

	int m_time_num;
	int m_cur_time;
	int m_chan_num;
	struct WavelengthInfo
	{
		int chan_num;
		double wavelength;
	};
	std::vector<WavelengthInfo> m_excitation_wavelength_list;
	int m_slice_num;
	int m_x_size;
	int m_y_size;
	bool m_valid_spc;
	double m_xspc;
	double m_yspc;
	double m_zspc;
	double m_max_value;
	double m_scalar_scale;

	static bool oif_sort(const TimeDataInfo& info1, const TimeDataInfo& info2);
	void ReadSingleOif();
	void ReadSequenceOif();
	void ReadTifSequence(std::wstring file_name, int t = 0);
	void ReadOif();
	void ReadOifLine(std::wstring oneline);
	void ReadTiff(char* pbyData, unsigned short* val, int z);

	//axis count
	int axis_num;
	int cur_axis;
	//channel count
	int chan_num;
	int cur_chan;
	//axis info
	std::wstring axis_code;
	std::wstring pix_unit;
	std::wstring max_size;
	std::wstring start_pos;
	std::wstring end_pos;
	std::wstring light_type;

	std::wstring m_time_id; //! time sequence id
};


//! Reads an .oif file
iAio_API void readOIF(QString const & filename, iAConnector* con, int channel,
		std::vector<vtkSmartPointer<vtkImageData> > * volumes);
