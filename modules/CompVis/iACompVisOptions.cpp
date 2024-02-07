// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompVisOptions.h"

//vtk
#include <vtkActor.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkMapper.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTexture.h>

//Qt
#include <QString>

namespace iACompVisOptions
{
	void getColorArray3(double colors[3], unsigned char result[3])
	{
		for (size_t j = 0; j < 3; ++j)
		{
			result[j] = static_cast<unsigned char>(colors[j] * 255);
		}
	}

	void getColorArray4(double colors[4], unsigned char result[4])
	{
		for (size_t j = 0; j < 4; ++j)
		{
			result[j] = static_cast<unsigned char>(colors[j] * 255);
		}
	}

	QColor getQColor(const unsigned char colors[3])
	{
		int r = colors[0];
		int g = colors[1];
		int b = colors[2];
		QColor result = QColor(r, g, b);

		return result;
	}

	double histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax)
	{
		//double result = ((newMax - newMin) * ((value - oldMin) / (oldMax - oldMin))) + newMin;
		double result = newMin + ((value - oldMin) * ((newMax - newMin) / (oldMax - oldMin)));
		return result;
	}

	std::vector<double> calculateBinBoundaries(double minVal, double maxVal, int numberOfBins)
	{
		double length = computeIntervalLength(minVal, maxVal);
		double binLength = length / numberOfBins;

		std::vector<double> bins = std::vector<double>();

		for (size_t b = 0; b < static_cast<size_t>(numberOfBins); b++)
		{
			double lowerBound = minVal + (binLength * b);
			bins.push_back(lowerBound);
		}

		return bins;
	}

	double computeIntervalLength(double minVal, double maxVal)
	{
		double length = 0.0;

		if (minVal < 0 || maxVal >= 0)
		{
			length = std::abs(minVal - maxVal);
		}
		else if (minVal < 0 && maxVal < 0)
		{
			length = std::abs(minVal) - std::abs(maxVal);
		}
		else if (minVal >= 0.0 && maxVal >= 0)
		{
			length = std::abs(maxVal) - std::abs(minVal);
		}

		return length;
	}

	 void getDoubleArray(const unsigned char colors[3], double result[3])
	{
		double help[3];
		help[0] = static_cast<double>(colors[0]);
		help[1] = static_cast<double>(colors[1]);
		help[2] = static_cast<double>(colors[2]);

		result[0] = iACompVisOptions::histogramNormalization(help[0], 0.0, 1.0, 0.0, 255);
		result[1] = iACompVisOptions::histogramNormalization(help[1], 0.0, 1.0, 0.0, 255);
		result[2] = iACompVisOptions::histogramNormalization(help[2], 0.0, 1.0, 0.0, 255);
	}

	double round_up(double value, int decimal_places)
	{
		const double multiplier = std::pow(10.0, decimal_places);
		return std::ceil(value * multiplier) / multiplier;
	}

	std::string cutStringAfterNDecimal(std::string input, int decimal_places)
	{
		std::size_t pos = input.find(".");
		std::string result = input.substr(0, (pos + 1) + decimal_places);
		return result;
	}

	void copyVector(std::vector<int>* toCopy, std::vector<int>* copied)
	{
		for (int i = 0; i < ((int)toCopy->size()); i++)
		{
			copied->at(i) = toCopy->at(i);
		}
	}

	void copyVector(std::vector<double>* toCopy, std::vector<double>* copied)
	{
		for (int i = 0; i < ((int)toCopy->size()); i++)
		{
			copied->at(i) = toCopy->at(i);
		}
	}

	void stippledLine(vtkSmartPointer<vtkActor> actor, int lineStipplePattern, int lineStippleRepeat)
	{
		vtkSmartPointer<vtkDoubleArray> tcoords = vtkSmartPointer<vtkDoubleArray>::New();
		vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
		vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();

		// Create texture
		int dimension = 16 * lineStippleRepeat;

		image->SetDimensions(dimension, 1, 1);
		image->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
		image->SetExtent(0, dimension - 1, 0, 0, 0, 0);
		unsigned char* pixel;
		pixel = static_cast<unsigned char*>(image->GetScalarPointer());
		unsigned char on = 255;
		unsigned char off = 0;
		for (int i = 0; i < 16; ++i)
		{
			unsigned int mask = (1 << i);
			unsigned int bit = (lineStipplePattern & mask) >> i;
			unsigned char value = static_cast<unsigned char>(bit);
			if (value == 0)
			{
				for (int j = 0; j < lineStippleRepeat; ++j)
				{
					*pixel = on;
					*(pixel + 1) = on;
					*(pixel + 2) = on;
					*(pixel + 3) = off;
					pixel += 4;
				}
			}
			else
			{
				for (int j = 0; j < lineStippleRepeat; ++j)
				{
					*pixel = on;
					*(pixel + 1) = on;
					*(pixel + 2) = on;
					*(pixel + 3) = on;
					pixel += 4;
				}
			}
		}
		vtkPolyData* polyData = dynamic_cast<vtkPolyData*>(actor->GetMapper()->GetInput());

		// Create texture coordnates
		tcoords->SetNumberOfComponents(1);
		tcoords->SetNumberOfTuples(polyData->GetNumberOfPoints());
		for (int i = 0; i < polyData->GetNumberOfPoints(); ++i)
		{
			double value = static_cast<double>(i) * .5;
			tcoords->SetTypedTuple(i, &value);
		}

		polyData->GetPointData()->SetTCoords(tcoords);
		texture->SetInputData(image);
		texture->InterpolateOff();
		texture->RepeatOn();

		actor->SetTexture(texture);
	}

	//calculates the percentage of a point in any range interval (with positive and negative values)
	double calculatePercentofRange(double value, double min, double max)
	{
		return (value - min) / (max - min);
	}

	//calculates a value at a specific percentage in any range interval (with positive and negative values)
	double calculateValueAccordingToPercent(double min, double max, double percent)
	{
		return percent * (max - min) + min;
	}

	QString getLabel(QString input)
	{
		int ind = input.lastIndexOf('/');
		QString newLow = input.mid(ind+1,input.size());

		QString result = newLow.replace(".csv", "");
		return result;
	}

	/*************** Initialization of Computation & GUI Options ****************************/
	namespace
	{
		bool computeMDS; //initialized in dlg_CSVReader, since depending on user input at beginning
		bool show3DViews; //initialized in dlg_CSVReader, since depending on user input at beginning
	}

	void setComputeMDS(bool val)
	{
		computeMDS = val;
	}
	void setShow3DViews(bool val)
	{
		show3DViews = val;
	}

	bool getComputeMDS()
	{
		return computeMDS;
	}
	bool getShow3DViews()
	{
		return show3DViews; 
	}
}
