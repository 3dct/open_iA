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

/*
Originally distributed under the MIT License:


For more information, please see: http://software.sci.utah.edu

The MIT License

Copyright (c) 2014 Scientific Computing and Imaging Institute,
University of Utah.


Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <string>
#include <vector>

#include <itkImage.h>

class OIFReader
{
public:
/*
	bool operator==(BaseReader& reader)
	{
		return m_id_string == reader.m_id_string;
	}
	void SetResize(int type)
	{
		m_resize_type = type;
	}
	int GetResize()
	{
		return m_resize_type;
	}
	void SetResample(int type)
	{
		m_resample_type = type;
	}
	int GetResample()
	{
		return m_resample_type;
	}
	void SetAlignment(int alignment)
	{
		m_alignment = alignment;
	}
	int GetAlignment()
	{
		return m_alignment;
	}
*/

protected:
	std::wstring m_id_string;	//the path and file name used to read files
	//resizing
	int m_resize_type;	//0: no resizing; 1: padding; 2: resampling
	int m_resample_type;	//0: nearest neighbour; 1: linear
	int m_alignment;	//padding alignment

	//3d batch
	bool m_batch;
	std::vector<std::wstring> m_batch_list;
	int m_cur_batch;

	std::wstring m_path_name;
public:
	typedef itk::Image<unsigned short, 2> TiffImgType;
	typedef TiffImgType::Pointer TiffImgPtr;
	typedef itk::Image<unsigned short, 3> ResultImgType;
	typedef ResultImgType::Pointer ResultImgPtr;

	OIFReader();

	void SetFile(std::string &file);
	void SetFile(std::wstring &file);
	void SetSliceSeq(bool ss);
	bool GetSliceSeq();
	void SetTimeId(std::wstring &id);
	std::wstring GetTimeId();
	void Preprocess();
	void SetBatch(bool batch);
	int LoadBatch(int index);
	std::wstring GetCurName(int t, int c);

	std::wstring GetPathName() {return m_path_name;}
	std::wstring GetDataName() {return m_data_name;}
	int GetTimeNum() {return m_time_num;}
	int GetCurTime() {return m_cur_time;}
	int GetChanNum() {return m_chan_num;}
	double GetExcitationWavelength(int chan);
	int GetSliceNum() {return m_slice_num;}
	int GetXSize() {return m_x_size;}
	int GetYSize() {return m_y_size;}
	bool IsSpcInfoValid() {return m_valid_spc;}
	double GetXSpc() {return m_xspc;}
	double GetYSpc() {return m_yspc;}
	double GetZSpc() {return m_zspc;}
	double GetMaxValue() {return m_max_value;}
	double GetScalarScale() {return m_scalar_scale;}
	bool GetBatch() {return m_batch;}
	int GetBatchNum() {return (int)m_batch_list.size();}
	int GetCurBatch() {return m_cur_batch;}
	void Load();
	ResultImgPtr GetResult(int chanIdx);
private:
	void Read(int t, int c, bool get_max);
	TiffImgPtr ReadTiffImage(std::string file_name);

	std::vector<ResultImgPtr> m_result;

	std::wstring m_data_name;
	std::wstring m_subdir_name;

	int m_type;	//0-time data in a single file; 1-time data in a file sequence
	typedef std::vector<std::wstring> ChannelInfo;	//slices form a channel
	typedef std::vector<ChannelInfo> DatasetInfo;//channels form dataset
	struct TimeDataInfo
	{
		int filenumber;		//if type is 1, file number for time data
		std::wstring filename;	//if type is 1, file name for current time data
		std::wstring subdirname;	//subdirectory name
		DatasetInfo dataset;//a list of the channels
	};
	std::vector<TimeDataInfo> m_oif_info;		//time data form the complete oif dataset
	int m_oif_t;	//current time point in oib info for reading

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

private:
	static bool oif_sort(const TimeDataInfo& info1, const TimeDataInfo& info2);
	void ReadSingleOif();
	void ReadSequenceOif();
	void ReadTifSequence(std::wstring file_name, int t=0);
	void ReadOif();
	void ReadOifLine(std::wstring oneline);
	void ReadTiff(char* pbyData, unsigned short *val, int z);

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

	//time sequence id
	std::wstring m_time_id;
};
