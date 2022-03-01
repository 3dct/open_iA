#pragma once

#include <qcolor.h>
#include <QString>

#include "vtkSmartPointer.h"
class vtkActor;

namespace iACompVisOptions
{
	/*************** Initialization of Computation & GUI Options ****************************/
	static bool computeNoMDS; //initialized in dlg_CSVReader, since depending on user input at beginning
	static bool show3DViews; //initialized in dlg_CSVReader, since depending on user input at beginning

	void setComputeNoMDS(bool val);
	void setShow3DViews(bool val);

	bool getComputeNoMDS();
	bool getShow3DViews();

	/*************** Reinitialization of Charts ****************************/
	enum class lastState
	{
		Undefined, //visualization before it was rendered for the first time
		Defined, //visualization in a defined state - only renderWidget() has to be called
		Changed //something was changed in the visualization and everything has to be drawn again
	};

	/*************** Binning Calculation ****************************/
	enum class binningType
	{
		Undefined,
		Uniform,
		JenksNaturalBreaks,
		BayesianBlocks
	};

	/*************** Active Visualization ****************************/
	enum class activeVisualization 
	{	Undefined,
		UniformTable, 
		VariableTable, 
		CombTable, 
		CurveVisualization 
	};
	
	/*************** Rendering ****************************/
	const unsigned char BACKGROUNDCOLOR_BLACK[3] = {0, 0, 0};
	const unsigned char BACKGROUNDCOLOR_GREY[3] = { 25, 25, 25 };//{128, 128, 128 };
	const unsigned char BACKGROUNDCOLOR_LIGHTGREY[3] = { 115, 115, 115 };
	const unsigned char BACKGROUNDCOLOR_LIGHTERGREY[3] = { 189, 189, 189 };
	const unsigned char BACKGROUNDCOLOR_WHITE[3] = { 255, 255, 255 };

	const unsigned char HIGHLIGHTCOLOR_BLACK[3] = { 0, 0, 0 };
	const unsigned char HIGHLIGHTCOLOR_YELLOW[3] = { 255,237,160 };
	const unsigned char HIGHLIGHTCOLOR_GREEN[3] = { 31,179,81 };

	const unsigned char FONTCOLOR_TITLE[3] = { 239, 239, 239 };//{ 195, 195, 195 };//{255, 255, 255};
	const int FONTSIZE_TITLE = 20;

	const unsigned char FONTCOLOR_TEXT[3] = { 239, 239, 239 };//{ 255, 255, 255 };
	const int FONTSIZE_TEXT = 13;

	const int LINE_WIDTH = 5; //3

	void getColorArray3(double colors[3], unsigned char result[3]);
	void getColorArray4(double colors[4], unsigned char result[4]);

	QColor getQColor(const unsigned char colors[3]);

	double histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax);

	std::vector<double> calculateBinBoundaries(double minVal, double maxVal, int numberOfBins);

	double computeIntervalLength(double minVal, double maxVal);

	void getDoubleArray(const unsigned char colors[3], double result[3]);

	double round_up(double value, int decimal_places);

	std::string cutStringAfterNDecimal(std::string input, int decimal_places);

	void copyVector(std::vector<int>* toCopy, std::vector<int>* copied);

	void copyVector(std::vector<double>* toCopy, std::vector<double>* copied);

	void stippledLine(vtkSmartPointer<vtkActor> actor, int lineStipplePattern, int lineStippleRepeat);

	//calculates the percentage of a point in any range interval (with positive and negative values)
	double calculatePercentofRange(double value, double min, double max);

	//calculates a value at a specific percentage in any range interval (with positive and negative values)
	double calculateValueAccordingToPercent(double min, double max, double percent);

	//return the label of the dataset from the whole path
	QString getLabel(QString input);
};
