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
#include "iAFiberScoutScatterPlotMatrix.h"
#include "iAObjectAnalysisType.h"

#include <itkDecisionRule.h>
#include <itkDistanceToCentroidMembershipFunction.h>
#include <itkEuclideanDistanceMetric.h>
#include <itkKdTree.h>
#include <itkKdTreeBasedKmeansEstimator.h>
#include <itkListSample.h>
#include <itkMinimumDecisionRule.h>
#include <itkNormalVariateGenerator.h>
#include <itkSampleClassifierFilter.h>
#include <itkVector.h>
#include <itkWeightedCentroidKdTreeGenerator.h>

#include <vtkAnnotationLink.h>
#include <vtkAxis.h>
#include <vtkBrush.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkCommand.h>
#include <vtkContextMouseEvent.h>
#include <vtkContextScene.h>
#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPen.h>
#include <vtkPlot.h>
#include <vtkPlotPoints.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkStdString.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>
#include <vtkVariantArray.h>
#include <vtkVersion.h>

#include <QStandardItemModel>

// STL includes
#include <map>
#include <cassert>
#include <cmath>
#include <float.h>
#include <vector>

namespace FiberScout
{

#define VTK_CREATE(type, name) \
	vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

class iAScatterPlotMatrix::PIMPL
{
public:
	PIMPL() : VisibleColumnsModified( true ), BigChart( NULL ), ClassNumber2Plot( -1 )
	{
		pimplChartSetting* scatterplotSettings = new pimplChartSetting();
		scatterplotSettings->BackgroundBrush->SetColor( 0, 0, 0, 20 );
		this->ChartSettings[iAScatterPlotMatrix::SCATTERPLOT] = scatterplotSettings;
		scatterplotSettings->MarkerSize = 2.0;

		pimplChartSetting* histogramSettings = new pimplChartSetting();
		histogramSettings->BackgroundBrush->SetColor( 127, 127, 127, 102 );
		histogramSettings->PlotPen->SetColor( 255, 255, 255, 255 );
		this->ChartSettings[iAScatterPlotMatrix::HISTOGRAM] = histogramSettings;

		pimplChartSetting* activeplotSettings = new pimplChartSetting();
		activeplotSettings->BackgroundBrush->SetColor( 255, 255, 255, 255 );
		this->ChartSettings[iAScatterPlotMatrix::ACTIVEPLOT] = activeplotSettings;
		activeplotSettings->MarkerSize = 4.0;
		this->SelectedChartBGBrush->SetColor( 0, 204, 0, 102 );
		this->SelectedRowColumnBGBrush->SetColor( 204, 0, 0, 102 );

		this->kMeansClustering = false;
		this->VisibleHistograms = true;
	}

	~PIMPL()
	{
		delete this->ChartSettings[iAScatterPlotMatrix::SCATTERPLOT];
		delete this->ChartSettings[iAScatterPlotMatrix::HISTOGRAM];
		delete this->ChartSettings[iAScatterPlotMatrix::ACTIVEPLOT];
	}

	class pimplChartSetting
	{
	public:
		pimplChartSetting()
		{
			this->PlotPen->SetColor( 0, 0, 0, 255 );
			this->MarkerStyle = vtkPlotPoints::CIRCLE;
			this->MarkerSize = 2.0;
			this->AxisColor.Set( 0, 0, 0, 1 );
			this->GridColor.Set( 242, 242, 242, 255 );
			this->LabelNotation = vtkAxis::STANDARD_NOTATION;
			this->LabelPrecision = 2;
			this->TooltipNotation = vtkAxis::STANDARD_NOTATION;
			this->TooltipPrecision = 1;
			this->ShowGrid = true;
			this->ShowAxisLabels = true;
			this->LabelFont = vtkSmartPointer<vtkTextProperty>::New();
			this->LabelFont->SetFontFamilyToArial();
			this->LabelFont->SetFontSize( 12 );
			this->LabelFont->SetColor( 0.0, 0.0, 0.0 );
			this->LabelFont->SetOpacity( 1.0 );
		}
		~pimplChartSetting() {}

		int MarkerStyle;
		float MarkerSize;
		vtkColor4ub AxisColor;
		vtkColor4ub GridColor;
		int LabelNotation;
		int LabelPrecision;
		int TooltipNotation;
		int TooltipPrecision;
		bool ShowGrid;
		bool ShowAxisLabels;
		vtkSmartPointer<vtkTextProperty> LabelFont;
		vtkNew<vtkBrush> BackgroundBrush;
		vtkNew<vtkPen> PlotPen;
		vtkNew<vtkBrush> PlotBrush;
	};

	void UpdateAxis( vtkAxis* axis, pimplChartSetting* setting,
					 bool updateLabel = true )
	{
		if ( axis && setting )
		{
			axis->GetPen()->SetColor( setting->AxisColor );
			axis->GetGridPen()->SetColor( setting->GridColor );
			axis->SetGridVisible( setting->ShowGrid );
			if ( updateLabel )
			{
				vtkTextProperty *prop = setting->LabelFont.GetPointer();
				axis->SetNotation( setting->LabelNotation );
				axis->SetPrecision( setting->LabelPrecision );
				axis->SetLabelsVisible( setting->ShowAxisLabels );
				axis->GetLabelProperties()->SetFontSize( prop->GetFontSize() );
				axis->GetLabelProperties()->SetColor( prop->GetColor() );
				axis->GetLabelProperties()->SetOpacity( prop->GetOpacity() );
				axis->GetLabelProperties()->SetFontFamilyAsString(
					prop->GetFontFamilyAsString() );
			}
		}
	}

	void UpdateChart( vtkChart* chart, pimplChartSetting* setting )
	{
		if ( chart && setting )
		{
			vtkPlot *plot = chart->GetPlot( 0 );
			if ( plot )
			{
				plot->SetTooltipNotation( setting->TooltipNotation );
				plot->SetTooltipPrecision( setting->TooltipPrecision );
			}
		}
	}

	vtkNew<vtkTable> Histogram;
	bool VisibleColumnsModified;
	vtkWeakPointer<vtkChart> BigChart;
	vtkNew<vtkAnnotationLink> Link;

	std::map<int, pimplChartSetting*> ChartSettings;
	typedef std::map<int, pimplChartSetting*>::iterator chartIterator;

	vtkNew<vtkBrush> SelectedRowColumnBGBrush;
	vtkNew<vtkBrush> SelectedChartBGBrush;
	std::vector< vtkVector2i > AnimationPath;

	unsigned short NbOfClasses;
	vtkNew<vtkVariantArray> NbOfColoredFibersPerClass;
	vtkNew<vtkVariantArray> NbOfColoredFibersPerCluster;
	bool VisibleHistograms;
	bool kMeansClustering;
	short ClassNumber2Plot;
};


namespace
{

	// This is just here for now - quick and dirty historgram calculations...
	bool PopulateHistograms( vtkTable *input, vtkTable *output, vtkStringArray *s,
							 int NumberOfBins )
	{
		// The output table will have the twice the number of columns, they will be
		// the x and y for input column. This is the bin centers, and the population.
		for ( vtkIdType i = 0; i < s->GetNumberOfTuples(); ++i )
		{
			double minmax[2] = { 0.0, 0.0 };
			vtkStdString name( s->GetValue( i ) );
			vtkDataArray *in =
				vtkDataArray::SafeDownCast( input->GetColumnByName( name.c_str() ) );
			if ( in )
			{
				// The bin values are the centers, extending +/- half an inc either side
				in->GetRange( minmax );
				if ( minmax[0] == minmax[1] )
					minmax[1] = minmax[0] + 1.0;

				double inc = ( minmax[1] - minmax[0] ) / (NumberOfBins) * 1.001;
				double halfInc = inc / 2.0;
				vtkSmartPointer<vtkFloatArray> extents =
					vtkFloatArray::SafeDownCast(
					output->GetColumnByName( vtkStdString( name + "_extents" ).c_str() ) );
				
				if ( !extents )
				{
					extents = vtkSmartPointer<vtkFloatArray>::New();
					extents->SetName( vtkStdString( name + "_extents" ).c_str() );
				}

				extents->SetNumberOfTuples( NumberOfBins );
				float *centers = static_cast<float *>( extents->GetVoidPointer( 0 ) );
				double min = minmax[0] - 0.0005 * inc + halfInc;
				
				for ( int j = 0; j < NumberOfBins; ++j )
					extents->SetValue( j, min + j * inc );

				vtkSmartPointer<vtkIntArray> populations =
					vtkIntArray::SafeDownCast(
					output->GetColumnByName( vtkStdString( name + "_pops" ).c_str() ) );
				
				if ( !populations )
				{
					populations = vtkSmartPointer<vtkIntArray>::New();
					populations->SetName( vtkStdString( name + "_pops" ).c_str() );
				}

				populations->SetNumberOfTuples( NumberOfBins );
				int *pops = static_cast<int *>( populations->GetVoidPointer( 0 ) );
				
				for ( int k = 0; k < NumberOfBins; ++k )
					pops[k] = 0;

				for ( vtkIdType j = 0; j < in->GetNumberOfTuples(); ++j )
				{
					double v( 0.0 );
					in->GetTuple( j, &v );
					for ( int k = 0; k < NumberOfBins; ++k )
					{
						if ( vtkMathUtilities::FuzzyCompare( v, double( centers[k] ), halfInc ) )
						{
							++pops[k];
							break;
						}
					}
				}
				output->AddColumn( extents.GetPointer() );
				output->AddColumn( populations.GetPointer() );
			}
		}
		return true;
	}

	bool MoveColumn( vtkStringArray* visCols, int fromCol, int toCol )
	{
		if ( !visCols || visCols->GetNumberOfTuples() == 0
			 || fromCol == toCol || fromCol == ( toCol - 1 ) || fromCol < 0 || toCol < 0 )
			return false;
		
		int numCols = visCols->GetNumberOfTuples();
		if ( fromCol >= numCols || toCol > numCols )
			return false;
		
		std::vector<vtkStdString> newVisCols;
		vtkIdType c;
		if ( toCol == numCols )
		{
			for ( c = 0; c < numCols; c++ )
			{
				if ( c != fromCol )
					newVisCols.push_back( visCols->GetValue( c ) );
			}
			// move the fromCol to the end
			newVisCols.push_back( visCols->GetValue( fromCol ) );
		}
		// insert the fromCol before toCol
		else if ( fromCol < toCol )
		{
			// move Cols in the middle up
			for ( c = 0; c < fromCol; c++ )
				newVisCols.push_back( visCols->GetValue( c ) );
			
			for ( c = fromCol + 1; c < numCols; c++ )
			{
				if ( c == toCol )
					newVisCols.push_back( visCols->GetValue( fromCol ) );
				
				newVisCols.push_back( visCols->GetValue( c ) );
			}
		}
		else
		{
			for ( c = 0; c < toCol; c++ )
				newVisCols.push_back( visCols->GetValue( c ) );
			
			newVisCols.push_back( visCols->GetValue( fromCol ) );
			for ( c = toCol; c < numCols; c++ )
			{
				if ( c != fromCol )
					newVisCols.push_back( visCols->GetValue( c ) );
			}
		}

		// repopulate the visCols
		vtkIdType visId = 0;
		std::vector<vtkStdString>::iterator arrayIt;
		for ( arrayIt = newVisCols.begin(); arrayIt != newVisCols.end(); ++arrayIt )
			visCols->SetValue( visId++, *arrayIt );
		
		return true;
	}
}

vtkStandardNewMacro( iAScatterPlotMatrix )

//----------------------------------------------------------------------------
iAScatterPlotMatrix::iAScatterPlotMatrix() : NumberOfBins( 8 )
{
	this->activeChild = NULL;
	this->Private = new PIMPL;
	this->TitleProperties = vtkSmartPointer<vtkTextProperty>::New();
	this->TitleProperties->SetFontSize( 12 );
	this->SelectionMode = vtkContextScene::SELECTION_NONE;
	this->m_selectionTool = vtkChart::SELECT_RECTANGLE;
}

//----------------------------------------------------------------------------
iAScatterPlotMatrix::~iAScatterPlotMatrix()
{
	this->activeChild = NULL;
	delete this->Private;
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::Update()
{
	if ( this->Private->VisibleColumnsModified )
	{
		// We need to handle layout changes due to modified visibility.
		// Build up our histograms data before updating the layout.
		PopulateHistograms( this->Input.GetPointer(),
							this->Private->Histogram.GetPointer(),
							this->VisibleColumns.GetPointer(),
							this->NumberOfBins );
		this->UpdateLayout();
		this->Private->VisibleColumnsModified = false;
	}
}

//----------------------------------------------------------------------------
bool iAScatterPlotMatrix::Paint( vtkContext2D *painter )
{
	this->Update();
	return Superclass::Paint( painter );
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetkMeansMode( bool kMeans )
{
	this->Private->kMeansClustering = kMeans;
	this->SetActivePlot( GetActivePlot() );
}

//----------------------------------------------------------------------------
bool iAScatterPlotMatrix::SetActivePlot( const vtkVector2i &pos )
{
	if ( pos.GetX() + pos.GetY() + 1 < this->Size.GetX() && pos.GetX() < this->Size.GetX() &&
		 pos.GetY() < this->Size.GetY() )
	{
		// The supplied index is valid (in the lower quadrant).
		this->ActivePlot = pos;

		// Set background colors for plots
		if ( this->GetChart( this->ActivePlot )->GetPlot( 0 ) )
		{
			int plotCount = this->GetSize().GetX();

			for ( int i = 0; i < plotCount; ++i )
			{
				for ( int j = 0; j < plotCount; ++j )
				{
					if ( this->GetPlotType( i, j ) == SCATTERPLOT )
					{
						vtkChart *chart =
							( this->GetChart( vtkVector2i( i, j ) ) );

						if ( pos[0] == i && pos[1] == j )
						{
							// Set the new active chart background color to light green
							chart->SetBackgroundBrush(
								this->Private->SelectedChartBGBrush.GetPointer() );

							// Forces BigChart axis labeling.
							this->Private->BigChart->GetAxis( vtkAxis::TOP )->SetTitle
								( removeUnit( this->VisibleColumns->GetValue( i ) ) );
							this->Private->BigChart->GetAxis( vtkAxis::RIGHT )->SetTitle
								( removeUnit( this->VisibleColumns->GetValue( plotCount - j - 1 ) ) );
							this->Private->BigChart->GetAxis( vtkAxis::TOP )->GetTitleProperties()->SetFontSize( 11 );
							this->Private->BigChart->GetAxis( vtkAxis::TOP )->GetTitleProperties()->BoldOn();
							this->Private->BigChart->GetAxis( vtkAxis::RIGHT )->GetTitleProperties()->SetFontSize( 11 );
							this->Private->BigChart->GetAxis( vtkAxis::RIGHT )->GetTitleProperties()->BoldOn();

							//this->Private->BigChart->GetAxis(vtkAxis::RIGHT)->SetTickLabelAlgorithm(vtkAxis::TICK_WILKINSON_EXTENDED);
							//this->Private->BigChart->GetAxis(vtkAxis::TOP)->SetTickLabelAlgorithm(vtkAxis::TICK_WILKINSON_EXTENDED);

							// Forces BigChart grid visibility
							this->Private->BigChart->GetAxis( vtkAxis::RIGHT )->SetGridVisible( true );
							this->Private->BigChart->GetAxis( vtkAxis::TOP )->SetGridVisible( true );
						}
						else if ( pos[0] == i || pos[1] == j )
						{
							// Set background color for all other charts in the selected
							// chart's row and column to light red
							chart->SetBackgroundBrush(
								this->Private->SelectedRowColumnBGBrush.GetPointer() );
						}
						else
						{
							// set all else to white
							chart->SetBackgroundBrush(
								this->Private->ChartSettings[SCATTERPLOT]
								->BackgroundBrush.GetPointer() );
						}
					}
				}
			}
		}
		if ( this->Private->BigChart )
		{

			// Clear all plots to get an empty BigChart.
			this->Private->BigChart->ClearPlots();

			// Changes mouse button configuration to...
			this->Private->BigChart->SetActionToButton( vtkChart::PAN, vtkContextMouseEvent::RIGHT_BUTTON );
			this->Private->BigChart->SetActionToButton( vtkChart::ZOOM, vtkContextMouseEvent::MIDDLE_BUTTON );

			// The selected or active class in Parallel Coordinates is shown in BigChart.
			if ( this->Private->ClassNumber2Plot > -1 && !this->Private->kMeansClustering )
			{
				// Changes BigChart mouse button configuration to...
				this->Private->BigChart->SetActionToButton( m_selectionTool, vtkContextMouseEvent::LEFT_BUTTON );

				// Add a plot to the BigChart.
				vtkPlot *plot = this->Private->BigChart->AddPlot( vtkChart::POINTS );

				// Added since VTK 6.1; to enable multi selection with modifiers (SHIFT,...)
				// Sets the higligthed selection for the BigChart
				if ( this->Private->Link->GetCurrentSelection()->GetNumberOfNodes() > 0 )
				{
					plot->SetSelection(
						vtkIdTypeArray::SafeDownCast(
						this->Private->Link->GetCurrentSelection()->GetNode( 0 )->GetSelectionList() ) );
				}

				// BigChart is drawn in the upper right corner. 
				vtkChartXY *xy = vtkChartXY::SafeDownCast( this->Private->BigChart );

				if(xy)
					xy->SetPlotCorner(plot, 2); 

				// Fill up plot with data from the tmptable and set color. 
				plot->SetInputData( CurrentClassTable.GetPointer(),
									this->VisibleColumns->GetValue( pos.GetX() ),
									this->VisibleColumns->GetValue( this->Size.GetX() - pos.GetY() - 1 ) );
				plot->SetColor( 0, 0, 0 );
				plot->GetPen()->SetWidth( 0 );
				plot->GetPen()->SetOpacity( 150 );

				// Set marker size and style.
				vtkPlotPoints *plotPoints = vtkPlotPoints::SafeDownCast( plot );
				plotPoints->SetMarkerSize( this->Private->ChartSettings[ACTIVEPLOT]->MarkerSize );
				plotPoints->SetMarkerStyle( this->Private->ChartSettings[ACTIVEPLOT]->MarkerStyle );

				// If option Multi Rendering is selected all classes will be plotted. 
			}
			else if ( this->Private->ClassNumber2Plot == -1 && !this->Private->kMeansClustering )
			{
				// Changes mouse button configuration to...
				this->Private->BigChart->SetActionToButton( m_selectionTool, -1 );

				// Start to plot one class after another with their given color information.
				unsigned short currcl = 0;

				while ( currcl < this->Private->NbOfClasses )
				{
					// Set up a temporary table with objects belonging to a specific calss only. 
					vtkSmartPointer<vtkTable> tmpTable = vtkSmartPointer<vtkTable>::New();
					tmpTable->DeepCopy( Input );

					// tmpTable holds the values for one single class
					for ( int del = tmpTable->GetNumberOfRows(); del >= 0; --del )
					{
						if ( tmpTable->GetValueByName( del, "Class_ID" ).ToInt() != currcl )
							tmpTable->RemoveRow( del );
					}

					// Add a plot to the BigChart.
					vtkPlot *plot = this->Private->BigChart->AddPlot( vtkChart::POINTS );

					// BigChart is drawn in the upper right corner. 
					vtkChartXY *xy = vtkChartXY::SafeDownCast( this->Private->BigChart );

					if(xy)
						xy->SetPlotCorner(plot, 2);	

					// Fill up plot with data from the tmptable and set to its class color. 
					plot->SetInputData( tmpTable,
										this->VisibleColumns->GetValue( pos.GetX() ),
										this->VisibleColumns->GetValue( this->Size.GetX() - pos.GetY() - 1 ) );
					plot->SetColor( tmpTable->GetValueByName( 0, "cColorR" ).ToChar(),
									tmpTable->GetValueByName( 0, "cColorG" ).ToChar(),
									tmpTable->GetValueByName( 0, "cColorB" ).ToChar(), 255 );
					plot->GetPen()->SetOpacity( 144 );

					// Set marker size and style.
					vtkPlotPoints *plotPoints = vtkPlotPoints::SafeDownCast( plot );
					plotPoints->SetMarkerSize( this->Private->ChartSettings[ACTIVEPLOT]->MarkerSize );
					plotPoints->SetMarkerStyle( this->Private->ChartSettings[ACTIVEPLOT]->MarkerStyle );

					currcl++;
				}
				currcl = 0;

			}
			else if ( this->Private->kMeansClustering )
			{

				typedef itk::Vector< double, 2 > MeasurementVectorType;
				typedef itk::Statistics::ListSample< MeasurementVectorType > SampleType;
				SampleType::Pointer sample = SampleType::New();

				MeasurementVectorType mv;

				kMeansTable = vtkSmartPointer<vtkTable>::New();
				kMeansTable->DeepCopy( Input );

				// kMeansTable holds the values for one single class
				for ( int del = kMeansTable->GetNumberOfRows(); del >= 0; --del )
				{
					if ( kMeansTable->GetValueByName( del, "Class_ID" ).ToInt() != this->Private->ClassNumber2Plot )
						kMeansTable->RemoveRow( del );
				}

				double tValueXMax = DBL_MIN, tValueYMax = DBL_MIN, rXHMark = DBL_MIN,
					rXLMark = DBL_MAX, rYHMark = DBL_MIN, rYLMark = DBL_MAX,
					tValueXMin = DBL_MAX, tValueYMin = DBL_MAX, tValueX = 0.0, tValueY = 0.0;

				//  Get column min, max value ranges
				for ( int i = 0; i < kMeansTable->GetNumberOfRows(); ++i )
				{
					tValueX = kMeansTable->GetValueByName( i, this->VisibleColumns->GetValue( pos.GetX() ) ).ToDouble();
					tValueY = kMeansTable->GetValueByName( i, this->VisibleColumns->GetValue( this->Size.GetX() - pos.GetY() - 1 ) ).ToDouble();

					if ( tValueX > tValueXMax ) tValueXMax = tValueX;
					if ( tValueY > tValueYMax ) tValueYMax = tValueY;
					if ( tValueX < tValueXMin ) tValueXMin = tValueX;
					if ( tValueY < tValueYMin ) tValueYMin = tValueY;
				}

				//  Get Input for kMeans and normailize values (FIX negative values)
				if ( tValueXMax > 1 && tValueYMax <= 1 )
				{
					for ( unsigned int Idx = 0; Idx < kMeansTable->GetNumberOfRows(); ++Idx )
					{
						mv[0] = kMeansTable->GetValueByName( 
							Idx, this->VisibleColumns->GetValue( pos.GetX() ) ).ToDouble() / ( tValueXMax );

						mv[1] = kMeansTable->GetValueByName( 
							Idx, this->VisibleColumns->GetValue( this->Size.GetX() - pos.GetY() - 1 ) ).ToDouble();

						sample->PushBack( mv );
					}
				}
				else if ( tValueYMax > 1 && tValueXMax <= 1 )
				{
					for ( unsigned int Idx = 0; Idx < kMeansTable->GetNumberOfRows(); ++Idx )
					{
						mv[0] = kMeansTable->GetValueByName( 
							Idx, this->VisibleColumns->GetValue( pos.GetX() ) ).ToDouble();

						mv[1] = kMeansTable->GetValueByName( 
							Idx, this->VisibleColumns->GetValue( this->Size.GetX() - pos.GetY() - 1 ) ).ToDouble() / ( tValueYMax );

						sample->PushBack( mv );
					}
				}
				else if ( tValueYMax > 1 && tValueXMax > 1 )
				{
					for ( unsigned int Idx = 0; Idx < kMeansTable->GetNumberOfRows(); ++Idx )
					{
						mv[0] = kMeansTable->GetValueByName( 
							Idx, this->VisibleColumns->GetValue( pos.GetX() ) ).ToDouble() / ( tValueXMax );

						mv[1] = kMeansTable->GetValueByName( 
							Idx, this->VisibleColumns->GetValue( this->Size.GetX() - pos.GetY() - 1 ) ).ToDouble() / ( tValueYMax );

						sample->PushBack( mv );
					}
				}
				else
				{
					for ( unsigned int Idx = 0; Idx < kMeansTable->GetNumberOfRows(); ++Idx )
					{
						mv[0] = kMeansTable->GetValueByName( 
							Idx, this->VisibleColumns->GetValue( pos.GetX() ) ).ToDouble() / tValueXMax;

						mv[1] = kMeansTable->GetValueByName( 
							Idx, this->VisibleColumns->GetValue( this->Size.GetX() - pos.GetY() - 1 ) ).ToDouble() / tValueYMax;

						sample->PushBack( mv );
					}
				}

				//  Get (normailzed) min, max value ranges of sample 
				SampleType::Iterator it = sample->Begin();
				MeasurementVectorType sv;

				while ( it != sample->End() )
				{
					sv = it.GetMeasurementVector();
					if ( sv[0] > rXHMark ) rXHMark = sv[0]; //XMax
					if ( sv[0] < rXLMark ) rXLMark = sv[0]; //XMin
					if ( sv[1] > rYHMark ) rYHMark = sv[1]; //YMax
					if ( sv[1] < rYLMark ) rYLMark = sv[1]; //YMin
					++it;
				}

				typedef itk::Statistics::WeightedCentroidKdTreeGenerator< SampleType > TreeGeneratorType;
				TreeGeneratorType::Pointer treeGenerator = TreeGeneratorType::New();

				treeGenerator->SetSample( sample );
				treeGenerator->SetBucketSize( 16 );
				treeGenerator->Update();

				typedef TreeGeneratorType::KdTreeType TreeType;
				typedef itk::Statistics::KdTreeBasedKmeansEstimator<TreeType> EstimatorType;
				EstimatorType::Pointer estimator = EstimatorType::New();

				EstimatorType::ParametersType initialMeans( NumberOfClusters * 2 );

				//  Calculates random initial centroids (within himark and lowmark)
				for ( int i = 0; i <= ( NumberOfClusters * 2 ) - 2; i += 2 )
				{
					initialMeans[i] = ( ( rXHMark - rXLMark ) * ( (float) rand() / RAND_MAX ) ) + rXLMark;
					initialMeans[i + 1] = ( rYHMark - rYLMark ) * ( (float) rand() / RAND_MAX ) + rYLMark;
				}

				estimator->SetParameters( initialMeans );
				estimator->SetKdTree( treeGenerator->GetOutput() );
				estimator->SetMaximumIteration( 200 );
				estimator->SetCentroidPositionChangesThreshold( 0.0 );
				estimator->StartOptimization();

				EstimatorType::ParametersType estimatedMeans = estimator->GetParameters();

				typedef itk::Statistics::DistanceToCentroidMembershipFunction< MeasurementVectorType > MembershipFunctionType;
				typedef MembershipFunctionType::Pointer	MembershipFunctionPointer;

#if ITK_VERSION_MAJOR < 4
				typedef itk::Statistics::MinimumDecisionRule2 DecisionRuleType;
#else
				typedef itk::Statistics::MinimumDecisionRule DecisionRuleType;
#endif
				DecisionRuleType::Pointer decisionRule = DecisionRuleType::New();

				typedef itk::Statistics::SampleClassifierFilter< SampleType > ClassifierType;
				ClassifierType::Pointer classifier = ClassifierType::New();

				classifier->SetDecisionRule( decisionRule );
				classifier->SetInput( sample );
				classifier->SetNumberOfClasses( NumberOfClusters );

				typedef ClassifierType::ClassLabelVectorObjectType               ClassLabelVectorObjectType;
				typedef ClassifierType::ClassLabelVectorType                     ClassLabelVectorType;
				typedef ClassifierType::MembershipFunctionVectorObjectType       MembershipFunctionVectorObjectType;
				typedef ClassifierType::MembershipFunctionVectorType             MembershipFunctionVectorType;

				ClassLabelVectorObjectType::Pointer  classLabelsObject = ClassLabelVectorObjectType::New();
				classifier->SetClassLabels( classLabelsObject );

				ClassLabelVectorType &  classLabelsVector = classLabelsObject->Get();
				for ( int i = 1; i <= NumberOfClusters; ++i )
					classLabelsVector.push_back( i * 100 );
				
				MembershipFunctionVectorObjectType::Pointer membershipFunctionsObject = 
					MembershipFunctionVectorObjectType::New();
				classifier->SetMembershipFunctions( membershipFunctionsObject );

				MembershipFunctionVectorType &  membershipFunctionsVector = membershipFunctionsObject->Get();

				MembershipFunctionType::CentroidType origin( sample->GetMeasurementVectorSize() );

				int index = 0;
				for ( unsigned int i = 0; i < NumberOfClusters; i++ )
				{
					MembershipFunctionPointer membershipFunction = MembershipFunctionType::New();
					for ( unsigned int j = 0; j < sample->GetMeasurementVectorSize(); j++ )
						origin[j] = estimatedMeans[index++];
				
					membershipFunction->SetCentroid( origin );
					membershipFunctionsVector.push_back( membershipFunction.GetPointer() );
				}

				classifier->Update();

				const ClassifierType::MembershipSampleType* membershipSample = classifier->GetOutput();
				ClassifierType::MembershipSampleType::ConstIterator iter = membershipSample->Begin();

				// Changes mouse button configuration to no selection
				this->Private->BigChart->SetActionToButton( m_selectionTool, -1 );

				// Start to plot one class after another with their given color information.
				unsigned short currcl = 1;
				vtkPlot *plot;

				ClassifierType::MembershipSampleType::ConstIterator kIter = membershipSample->Begin();

				// Set Class_IDs to the new Class_IDs (found by kMeans)
				int kRowIdx = 0;
				while ( kIter != membershipSample->End() )
				{
					kMeansTable->SetValueByName( kRowIdx, "Class_ID", kIter.GetClassLabel() / 100 );
					++kIter;
					++kRowIdx;
				}

				// Sets the nubmer of elements for the NbOfColoredFiberPerCluster array.
				this->Private->NbOfColoredFibersPerCluster->SetNumberOfValues( NumberOfClusters );

				while ( currcl <= NumberOfClusters )
				{
					// Set up a temporary table with objects belonging to a specific calss only. 
					vtkSmartPointer<vtkTable> tmpTable = vtkSmartPointer<vtkTable>::New();
					tmpTable->DeepCopy( kMeansTable );

					// tmpTable holds the values for one single class
					for ( int del = tmpTable->GetNumberOfRows(); del >= 0; --del )
					{
						if ( tmpTable->GetValueByName( del, "Class_ID" ).ToInt() != currcl )
							tmpTable->RemoveRow( del );
					}

					//Sets number of fibers to the corresponding cluster
					this->Private->NbOfColoredFibersPerCluster->SetValue( currcl - 1, tmpTable->GetNumberOfRows() );

					// Add a plot to the BigChart.
					plot = this->Private->BigChart->AddPlot( vtkChart::POINTS );

					// BigChart is drawn in the upper right corner. 
					vtkChartXY *xy = vtkChartXY::SafeDownCast( this->Private->BigChart );

					if(xy){ xy->SetPlotCorner(plot, 2); }

					// Fill up plot with data from the tmptable and set to its class color. 
					plot->SetInputData( tmpTable,
										this->VisibleColumns->GetValue( pos.GetX() ),
										this->VisibleColumns->GetValue( this->Size.GetX() - pos.GetY() - 1 ) );

					QColor cColor;	/* http://www.w3.org/TR/SVG/types.html#ColorKeywords */

					switch ( currcl )
					{
						case 1:
							cColor.setNamedColor( "cornflowerblue" ); break;
						case 2:
							cColor.setNamedColor( "darkorange" ); break;
						case 3:
							cColor.setNamedColor( "chartreuse" ); break;
						case 4:
							cColor.setNamedColor( "yellow" ); break;
						case 5:
							cColor.setNamedColor( "mediumvioletred" ); break;
						case 6:
							cColor.setNamedColor( "blue" ); break;
						case 7:
							cColor.setNamedColor( "green" ); break;
					}

					plot->GetPen()->SetColor( cColor.red(), cColor.green(), cColor.blue() );
					plot->GetPen()->SetOpacity( 144 );

					// Set marker size and style.
					vtkPlotPoints *plotPoints = vtkPlotPoints::SafeDownCast( plot );
					plotPoints->SetMarkerSize( this->Private->ChartSettings[ACTIVEPLOT]->MarkerSize );
					plotPoints->SetMarkerStyle( this->Private->ChartSettings[ACTIVEPLOT]->MarkerStyle );

					currcl++;
				}
				currcl = 0;
			}

			// Adds custom legend.
			this->UpdateCustomLegend();

			//Sets ToolTipLabelFormat to "X-Value, Y-Value" 
			this->Private->BigChart->GetPlot( 0 )->SetTooltipLabelFormat( vtkStdString( "%x,  %y" ) );

			// Set BigChart ground color.
			vtkSmartPointer<vtkBrush> BigChartBackroundBrush = vtkSmartPointer<vtkBrush>::New();
			BigChartBackroundBrush->SetColor( 232, 232, 232 );
			this->Private->BigChart->SetBackgroundBrush( BigChartBackroundBrush );

			// Calculate the ideal range.
			this->Private->BigChart->RecalculateBounds();
			this->Private->kMeansClustering = false;
		}
		return true;
	}
	else
	{
		return false;
	}
}

//----------------------------------------------------------------------------
vtkStdString iAScatterPlotMatrix::removeUnit( vtkStdString str )
{
	//Removes "unit" from str ( "unit" consists of [...] )

	QString rUStr = QString::fromStdString( str );

	if ( rUStr.indexOf( "[" ) > -1 )
		rUStr.replace( rUStr.indexOf( "[" ), rUStr.indexOf( "]" ) - rUStr.indexOf( "[" ) + 1, "" );
	
	return rUStr.toStdString();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::AddColorColumns2Table()
{
	// Add RGB color info to Table and initialize each row with a value of 0.0f

	VTK_CREATE( vtkFloatArray, cColorR );
	cColorR->SetName( "cColorR" );
	cColorR->SetNumberOfTuples( Input->GetNumberOfRows() );
	Input->AddColumn( cColorR );
	VTK_CREATE( vtkFloatArray, cColorG );
	cColorG->SetName( "cColorG" );
	cColorG->SetNumberOfTuples( Input->GetNumberOfRows() );
	Input->AddColumn( cColorG );
	VTK_CREATE( vtkFloatArray, cColorB );
	cColorB->SetName( "cColorB" );
	cColorB->SetNumberOfTuples( Input->GetNumberOfRows() );
	Input->AddColumn( cColorB );

	for ( int i = 0; i < Input->GetNumberOfRows(); ++i )
	{
		Input->SetValueByName( i, "cColorR", 0.0f );
		Input->SetValueByName( i, "cColorG", 0.0f );
		Input->SetValueByName( i, "cColorB", 0.0f );
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::UpdateColorInfo( QStandardItemModel *classTree,
										   QList<QColor> colorList )
{
	// Collects object IDs and color information from the 'Parallel Coordinates Class Tree'
	// and adds the information to the Input (table).

	//Number of Classes in the PC tree 
	int hmcl = classTree->invisibleRootItem()->rowCount();

	if ( hmcl > 0 )
	{
		for ( int cclIndex = 0; cclIndex < hmcl; ++cclIndex )
		{
			// Number of objects in this class
			int iclNbOfObj = classTree->invisibleRootItem()->child( cclIndex, 0 )->rowCount();

			// Sets the class color to the corresponding object Ids 
			for ( int iclIndex = 0; iclIndex < iclNbOfObj; ++iclIndex )
			{
				Input->SetValueByName( classTree->invisibleRootItem()->child( cclIndex, 0 )->
									   child( iclIndex, 0 )->text().toInt() - 1, "Class_ID", cclIndex );
				Input->SetValueByName( classTree->invisibleRootItem()->child( cclIndex, 0 )->
									   child( iclIndex, 0 )->text().toInt() - 1, "cColorR", colorList.at( cclIndex ).red() );
				Input->SetValueByName( classTree->invisibleRootItem()->child( cclIndex, 0 )->
									   child( iclIndex, 0 )->text().toInt() - 1, "cColorG", colorList.at( cclIndex ).green() );
				Input->SetValueByName( classTree->invisibleRootItem()->child( cclIndex, 0 )->
									   child( iclIndex, 0 )->text().toInt() - 1, "cColorB", colorList.at( cclIndex ).blue() );
			}
		}
	}
	this->pcClassTree = classTree;
	this->pcColorList = colorList;
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetClass2Plot( short classNumber )
{
	this->Private->ClassNumber2Plot = classNumber;
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::ShowHideHistograms()
{
	this->Private->VisibleHistograms = !this->Private->VisibleHistograms;
	this->UpdateLayout();
}

//----------------------------------------------------------------------------
int iAScatterPlotMatrix::GetkMeansClusterCount()
{
	//  Class_ID at the end of the table gives the number of clusters
	vtkVariant v = this->Private->NbOfColoredFibersPerCluster->GetNumberOfTuples();
	return v.ToInt();
}

//----------------------------------------------------------------------------
vtkSelection * iAScatterPlotMatrix::GetkMeansCluster( int clusterNumber )
{
	vtkVariantArray *selArr = vtkVariantArray::New();

	for ( int i = 0; i < kMeansTable->GetNumberOfRows(); ++i )
	{
		if ( kMeansTable->GetValueByName( i, "Class_ID" ) == clusterNumber )
			selArr->InsertNextValue( kMeansTable->GetValueByName( i, "Label" ) );
	}

	// Create a PedigreeIds selection node
	vtkSelection *s = vtkSelection::New();
	vtkSelectionNode *n = vtkSelectionNode::New();
	s->AddNode( n );
	n->SetFieldType( vtkSelectionNode::POINT );
	n->SetContentType( vtkSelectionNode::INDICES );
	n->SetSelectionList( selArr );
	return s;
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::UpdateCustomLegend()
{
	// Creates an custom legend left to the Big Chart
	// Smallest possible scatter plot matrix with custom legend 
	//
	//  3 H L bbb   
	//  2 S H Bbb
	//  1 S S H
	//  0 S S S H 
	//    0 1 2 3 

	if ( this->Size.GetX() > 2 )
	{
		QString segmentedObjects;
		if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
			segmentedObjects = "Fibers";
		else
			segmentedObjects = "Objects";

		// Gets the starting position of the Big Chart to set the right position of the legend.
		float plotCount = this->GetSize().GetX();
		vtkVector2i bigChartPos;
		int bCP = ceil( plotCount / 2 );
		bigChartPos.Set( bCP, bCP );

		// Legend background color
		vtkBrush *brsh = vtkBrush::New();
		brsh->SetColor( 0, 0, 0, 0 );

		// Creates a vtkChartXY left to the BigChart
		vtkChartXY *legend = vtkChartXY::SafeDownCast( this->GetChart( vtkVector2i(
			this->Size.GetX() - bigChartPos.GetX() - 1,
			this->Size.GetX() - 1 ) ) );
		legend->SetBackgroundBrush( brsh );

		// Gets the BigChart's legend and shows it.
		legend->GetLegend()->SetChart( this->Private->BigChart );
		legend->SetShowLegend( true );

		// Removes any axis from the legend.
		legend->SetAutoAxes( false );
		legend->GetAxis( vtkAxis::BOTTOM )->SetGridVisible( false );
		legend->GetAxis( vtkAxis::BOTTOM )->SetVisible( 0 );
		legend->GetAxis( vtkAxis::LEFT )->SetGridVisible( false );
		legend->GetAxis( vtkAxis::LEFT )->SetVisible( 0 );

		// A plot to hold the legend within.
		vtkPlot	*holder = legend->AddPlot( vtkChart::POINTS );

		QString	text;
		// Legend for all classes (when ClassNumber2Plot = -1).
		if ( this->Private->ClassNumber2Plot == -1 && !this->Private->kMeansClustering )
		{
			for ( int i = 1; i <= this->pcClassTree->invisibleRootItem()->rowCount(); ++i )
			{
				text.clear();

				text.append( QString( "%1 '%2' %3" )
							 .arg( this->Private->NbOfColoredFibersPerClass->GetValue( i - 1 ).ToInt() )
							 .arg( this->pcClassTree->invisibleRootItem()->child( i - 1 )->text() )
							 .arg( segmentedObjects ) );

				this->Private->BigChart->GetPlot( i - 1 )->SetLabel( text.toStdString() );
			}
		}
		// Legend for the active class.
		else if ( this->Private->ClassNumber2Plot > -1 && !this->Private->kMeansClustering )
		{
			int NbOfCurrSel = 0;

			if ( this->Private->Link->GetCurrentSelection()->GetNumberOfNodes() )
			{
				NbOfCurrSel = this->Private->Link->GetCurrentSelection()->GetNode( 0 )
					->GetSelectionList()->GetNumberOfTuples();
			}

			text.clear();
			text.append( QString( "%1 of %2 '%3' %4 selected" )
						 .arg( NbOfCurrSel )
						 .arg( this->Private->NbOfColoredFibersPerClass->GetValue( this->Private->ClassNumber2Plot ).ToInt() )
						 .arg( this->pcClassTree->invisibleRootItem()->child( this->Private->ClassNumber2Plot )->text() )
						 .arg( segmentedObjects ) );

			this->Private->BigChart->GetPlot( 0 )->SetLabel( text.toStdString() );
		}
		// Legend for kMenas classes.
		else if ( this->Private->kMeansClustering )
		{
			for ( int i = 0; i < NumberOfClusters; ++i )
			{
				text.clear();
				text.append( QString( "Cluster %1: %2 %3" )
							 .arg( i + 1 )
							 .arg( this->Private->NbOfColoredFibersPerCluster->GetVariantValue( i ).ToInt() )
							 .arg( segmentedObjects ) );
				this->Private->BigChart->GetPlot( i )->SetLabel( text.toStdString() );
			}
		}
	}
}

//----------------------------------------------------------------------------
vtkVector2i iAScatterPlotMatrix::GetActivePlot()
{
	return this->ActivePlot;
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::UpdateAnimationPath(
	const vtkVector2i& newActivePos )
{
	this->Private->AnimationPath.clear();
	if ( newActivePos[0] != this->ActivePlot[0] ||
		 newActivePos[1] != this->ActivePlot[1] )
	{
		if ( newActivePos[1] >= this->ActivePlot[1] )
		{
			// x direction first
			if ( this->ActivePlot[0] > newActivePos[0] )
			{
				for ( int r = this->ActivePlot[0] - 1; r >= newActivePos[0]; r-- )
					this->Private->AnimationPath.push_back(
					vtkVector2i( r, this->ActivePlot[1] ) );
			}
			else
			{
				for ( int r = this->ActivePlot[0] + 1; r <= newActivePos[0]; r++ )
					this->Private->AnimationPath.push_back(
					vtkVector2i( r, this->ActivePlot[1] ) );
			}
			// then y direction
			for ( int c = this->ActivePlot[1] + 1; c <= newActivePos[1]; c++ )
				this->Private->AnimationPath.push_back(
				vtkVector2i( newActivePos[0], c ) );
		}
		else
		{
			// y direction first
			for ( int c = this->ActivePlot[1] - 1; c >= newActivePos[1]; c-- )
				this->Private->AnimationPath.push_back(
				vtkVector2i( this->ActivePlot[0], c ) );
			// then x direction
			if ( this->ActivePlot[0] > newActivePos[0] )
			{
				for ( int r = this->ActivePlot[0] - 1; r >= newActivePos[0]; r-- )
					this->Private->AnimationPath.push_back(
					vtkVector2i( r, newActivePos[1] ) );
			}
			else
			{
				for ( int r = this->ActivePlot[0] + 1; r <= newActivePos[0]; r++ )
					this->Private->AnimationPath.push_back(
					vtkVector2i( r, newActivePos[1] ) );
			}
		}
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::StartAnimation(
	vtkRenderWindowInteractor* interactor )
{
	for ( std::vector<vtkVector2i>::iterator iter =
		  this->Private->AnimationPath.begin();
		  iter != this->Private->AnimationPath.end(); iter++ )
	{
		this->SetActivePlot( *iter );
		this->GetScene()->SetDirty( true );
		interactor->Render();
	}
}

//----------------------------------------------------------------------------
vtkAnnotationLink* iAScatterPlotMatrix::GetAnnotationLink()
{
	return this->Private->Link.GetPointer();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetSelection( vtkSelection *sel )
{
	this->Private->Link->SetCurrentSelection( sel );
	this->UpdateLayout();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetInput( vtkTable *table, int fid )
{
	filterID = fid;

	// do nothing if the table is emtpy
	if ( table && table->GetNumberOfRows() == 0 )
		return;

	if ( this->Input != table )
	{

		// Set the input, then update the size of the scatter plot matrix, set
		// their inputs and all the other stuff needed.
		this->Input = table;

		//Adds three columns (R G B) to the Input.
		AddColorColumns2Table();

		this->SetSize( vtkVector2i( 0, 0 ) );
		this->Modified();

		if ( table == NULL )
		{
			this->SetColumnVisibilityAll( true );
			return;
		}
		int n = static_cast<int>( this->Input->GetNumberOfColumns() );
		this->SetColumnVisibilityAll( true );
		this->SetSize( vtkVector2i( n, n ) );
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetColumnVisibility( const vtkStdString &name,
											   bool visible )
{
	if ( visible )
	{
		for ( vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i )
		{
			// Already there, nothing more needs to be done
			if ( this->VisibleColumns->GetValue( i ) == name )
				return;
		}
		// Add the column to the end of the list
		this->VisibleColumns->InsertNextValue( name );
		this->Private->VisibleColumnsModified = true;
		this->SetSize( vtkVector2i( 0, 0 ) );
		this->SetSize( vtkVector2i( this->VisibleColumns->GetNumberOfTuples(),
		this->VisibleColumns->GetNumberOfTuples() ) );
		this->Modified();
	}
	else
	{
		// Remove the value if present
		for ( vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i )
		{
			if ( this->VisibleColumns->GetValue( i ) == name )
			{
				// Move all the later elements down by one, and reduce the size
				while ( i < this->VisibleColumns->GetNumberOfTuples() - 1 )
				{
					this->VisibleColumns->SetValue( i, this->VisibleColumns->GetValue( i + 1 ) );
					++i;
				}

				this->VisibleColumns->SetNumberOfTuples( this->VisibleColumns->GetNumberOfTuples() - 1 );
				this->SetSize( vtkVector2i( 0, 0 ) );
				this->SetSize( 
					vtkVector2i( this->VisibleColumns->GetNumberOfTuples(),
					this->VisibleColumns->GetNumberOfTuples() ) );

				if ( this->ActivePlot.GetX() + this->ActivePlot.GetY() + 1 >=
					 this->VisibleColumns->GetNumberOfTuples() )
					this->ActivePlot.Set( 0, this->VisibleColumns->GetNumberOfTuples() - 1 );
				
				this->Private->VisibleColumnsModified = true;
				this->Modified();
				return;
			}
		}
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::InsertVisibleColumn( const vtkStdString &name,
											   int index )
{
	if ( !this->Input || !this->Input->GetColumnByName( name.c_str() ) )
		return;
	
	// Check if the column is already in the list. If yes,
	// we may need to rearrange the order of the columns.
	vtkIdType currIdx = -1;
	vtkIdType numCols = this->VisibleColumns->GetNumberOfTuples();
	for ( vtkIdType i = 0; i < numCols; ++i )
	{
		if ( this->VisibleColumns->GetValue( i ) == name )
		{
			currIdx = i;
			break;
		}
	}

	//This column is already there.
	if ( currIdx > 0 && currIdx == index )
		return;
	
	if ( currIdx < 0 )
	{
		this->VisibleColumns->SetNumberOfTuples( numCols + 1 );
		if ( index >= numCols )
		{
			this->VisibleColumns->SetValue( numCols, name );
		}
		else // move all the values after index down 1
		{
			vtkIdType startidx = numCols;
			vtkIdType idx = ( index < 0 ) ? 0 : index;
			while ( startidx > idx )
			{
				this->VisibleColumns->SetValue( startidx,
												this->VisibleColumns->GetValue( startidx - 1 ) );
				startidx--;
			}
			this->VisibleColumns->SetValue( idx, name );
		}
		this->Private->VisibleColumnsModified = true;
	}
	else // need to rearrange table columns
	{
		vtkIdType toIdx = ( index < 0 ) ? 0 : index;
		toIdx = toIdx>numCols ? numCols : toIdx;
		this->Private->VisibleColumnsModified =
			MoveColumn( this->VisibleColumns.GetPointer(), currIdx, toIdx );
	}

	this->Update();
}

//----------------------------------------------------------------------------
bool iAScatterPlotMatrix::GetColumnVisibility( const vtkStdString &name )
{
	for ( vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i )
	{
		if ( this->VisibleColumns->GetValue( i ) == name )
			return true;
	}
	return false;
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetColumnVisibilityAll( bool visible )
{
	if ( visible && this->Input )
	{
		vtkIdType n = this->Input->GetNumberOfColumns();
		this->VisibleColumns->SetNumberOfTuples( n );
		for ( vtkIdType i = 0; i < n; ++i )
			this->VisibleColumns->SetValue( i, this->Input->GetColumnName( i ) );
	}
	else
	{
		this->SetSize( vtkVector2i( 0, 0 ) );
		this->VisibleColumns->SetNumberOfTuples( 0 );
	}
	this->Private->VisibleColumnsModified = true;
}

vtkStringArray* iAScatterPlotMatrix::GetVisibleColumns()
{
	return this->VisibleColumns.GetPointer();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetVisibleColumns( vtkStringArray* visColumns )
{
	if ( !visColumns || visColumns->GetNumberOfTuples() == 0 )
	{
		this->SetSize( vtkVector2i( 0, 0 ) );
		this->VisibleColumns->SetNumberOfTuples( 0 );
	}
	else
	{
		this->VisibleColumns->SetNumberOfTuples( visColumns->GetNumberOfTuples() );
		this->VisibleColumns->DeepCopy( visColumns );
	}
	this->Private->VisibleColumnsModified = true;
	this->Update();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetNumberOfBins( int numberOfBins )
{
	if ( this->NumberOfBins != numberOfBins )
	{
		this->NumberOfBins = numberOfBins;
		if ( this->Input )
		{
			PopulateHistograms( this->Input.GetPointer(),
								this->Private->Histogram.GetPointer(),
								this->VisibleColumns.GetPointer(),
								this->NumberOfBins );
		}
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetPlotColor( int plotType, const vtkColor4ub& color )
{
	if ( plotType >= 0 && plotType < NOPLOT )
	{
		if ( plotType == ACTIVEPLOT || plotType == SCATTERPLOT )
			this->Private->ChartSettings[plotType]->PlotPen->SetColor( color );
		else
			this->Private->ChartSettings[HISTOGRAM]->PlotBrush->SetColor( color );
		
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetPlotMarkerStyle( int plotType, int style )
{
	if ( plotType >= 0 && plotType < NOPLOT &&
		 style != this->Private->ChartSettings[plotType]->MarkerStyle )
	{
		this->Private->ChartSettings[plotType]->MarkerStyle = style;

		if ( plotType == ACTIVEPLOT )
		{
			vtkChart *chart = this->Private->BigChart;
			if ( chart )
			{
				vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast( chart->GetPlot( 0 ) );
				if ( plot )
					plot->SetMarkerStyle( style );
			}
			this->Modified();
		}
		else if ( plotType == SCATTERPLOT )
		{
			int plotCount = this->GetSize().GetX();
			for ( int i = 0; i < plotCount - 1; ++i )
			{
				for ( int j = 0; j < plotCount - 1; ++j )
				{
					if ( this->GetPlotType( i, j ) == SCATTERPLOT &&
						 this->GetChart( vtkVector2i( i, j ) ) )
					{
						vtkChart *chart = this->GetChart( vtkVector2i( i, j ) );
						vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast( chart->GetPlot( 0 ) );
						if ( plot )
							plot->SetMarkerStyle( style );
					}
				}
			}
			this->Modified();
		}
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetPlotMarkerSize( int plotType, float size )
{
	if ( plotType >= 0 && plotType < NOPLOT &&
		 size != this->Private->ChartSettings[plotType]->MarkerSize )
	{
		this->Private->ChartSettings[plotType]->MarkerSize = size;

		if ( plotType == ACTIVEPLOT )
		{
			// update marker size on current active plot
			vtkChart *chart = this->Private->BigChart;
			if ( chart )
			{
				vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast( chart->GetPlot( 0 ) );
				if ( plot )
					plot->SetMarkerSize( size );
			}
			this->Modified();
		}
		else if ( plotType == SCATTERPLOT )
		{
			int plotCount = this->GetSize().GetX();

			for ( int i = 0; i < plotCount - 1; i++ )
			{
				for ( int j = 0; j < plotCount - 1; j++ )
				{
					if ( this->GetPlotType( i, j ) == SCATTERPLOT &&
						 this->GetChart( vtkVector2i( i, j ) ) )
					{
						vtkChart *chart = this->GetChart( vtkVector2i( i, j ) );
						vtkPlotPoints *plot = vtkPlotPoints::SafeDownCast( chart
																		   ->GetPlot( 0 ) );
						if ( plot )
							plot->SetMarkerSize( size );
					}
				}
			}
			this->Modified();
		}
	}
}

//----------------------------------------------------------------------------
bool iAScatterPlotMatrix::Hit( const vtkContextMouseEvent & )
{
	return true;
}

//----------------------------------------------------------------------------
bool iAScatterPlotMatrix::MouseMoveEvent( const vtkContextMouseEvent & )
{
	// Eat the event, don't do anything for now...
	return true;
}

//----------------------------------------------------------------------------
bool iAScatterPlotMatrix::MouseButtonPressEvent( const vtkContextMouseEvent & )
{
	return true;
}

//----------------------------------------------------------------------------
bool iAScatterPlotMatrix::MouseButtonReleaseEvent(
	const vtkContextMouseEvent &mouse )
{
	// Work out which scatter plot was clicked 
	// Make that one the active plot (no animation).

	int n = this->GetSize().GetX();

	for ( int i = 0; i < n; ++i )
	{
		for ( int j = 0; j < n; ++j )
		{
			if ( i + j + 1 < n && this->GetChart( vtkVector2i( i, j ) )->Hit( mouse ) )
			{
				vtkVector2i pos( i, j );
				this->SetActivePlot( pos );
				return true;
			}
		}
	}

	return false;
}

//----------------------------------------------------------------------------
int iAScatterPlotMatrix::GetPlotType( const vtkVector2i &pos )
{
	int plotCount = this->GetSize().GetX();

	if ( pos.GetX() + pos.GetY() + 1 < plotCount )
		return SCATTERPLOT;
	else if ( pos.GetX() + pos.GetY() + 1 == plotCount )
		return HISTOGRAM;
	else if ( pos.GetX() == pos.GetY() && pos.GetX() == static_cast<int>( plotCount / 2.0 ) + plotCount % 2 )
		return ACTIVEPLOT;
	else
		return NOPLOT;
}

//----------------------------------------------------------------------------
int iAScatterPlotMatrix::GetPlotType( int row, int column )
{
	return this->GetPlotType( vtkVector2i( row, column ) );
}


//----------------------------------------------------------------------------
void iAScatterPlotMatrix::UpdateLayout()
{
	// We want scatter plots on the lower-left triangle, then histograms along
	// the diagonal and a big plot in the top-right. The basic layout is,
	//
	// 3 H   +++
	// 2 S H +++
	// 1 S S H
	// 0 S S S H
	//   0 1 2 3

	unsigned short currcl = 0;

	////map holds class tables when multi-view rendering
	//if(this->Private->ClassNumber2Plot == -1)
	//{				
	//	while(currcl < this->Private->NbOfClasses)
	//	{
	//		tablelist[currcl] = vtkTable::New();
	//		tablelist[currcl]->DeepCopy(Input);
	//	
	//		for(int del=tablelist[currcl]->GetNumberOfRows(); del>=0; --del)
	//		{
	//			if(tablelist[currcl]->GetValueByName(del, "Class_ID").ToInt() != currcl )
	//				tablelist[currcl]->RemoveRow(del);
	//			
	//			currcl++;
	//		}
	//	}
	//}


	// Get the numbers of classes from PC-ClassTree.
	this->Private->NbOfClasses = this->pcClassTree->invisibleRootItem()->rowCount();

	// Sets the number of elements for the NbOfColoredFiberPerClass array.
	this->Private->NbOfColoredFibersPerClass->SetNumberOfValues( this->Private->NbOfClasses );

	// Where the indices are those of the columns. The indices of the charts
	// originate in the bottom-left.
	int n = this->Size.GetX();

	// currentClassTable holds the values for one single class only (all other classes will be deleted temporary).
	CurrentClassTable = vtkSmartPointer<vtkTable>::New();
	CurrentClassTable->DeepCopy( Input );

	for ( int del = CurrentClassTable->GetNumberOfRows(); del >= 0; --del )
	{
		if ( CurrentClassTable->GetValueByName( del, "Class_ID" ).ToInt() != this->Private->ClassNumber2Plot )
			CurrentClassTable->RemoveRow( del );
	}

	for ( int i = 0; i < n; ++i )
	{
		for ( int j = 0; j < n; ++j )
		{
			vtkVector2i pos( i, j );


			if ( this->GetPlotType( pos ) == SCATTERPLOT )
			{
				vtkChart* chart = this->GetChart( pos );
				chart->ClearPlots();
				chart->SetAnnotationLink( this->Private->Link.GetPointer() );

				// Added since VTK 6.1; needed to enable multi selection with modifiers (SHIFT,...)
				chart->SetSelectionMethod( vtkChart::SELECTION_ROWS );

				//chart->AddObserver(vtkCommand::SelectionChangedEvent, this,
				//						&iAScatterPlotMatrix::BigChartSelectionCallback);
				// Lower-left triangle - scatter plots.
				chart->SetActionToButton( vtkChart::PAN, -1 );
				chart->SetActionToButton( vtkChart::ZOOM, -1 );
				chart->SetActionToButton( vtkChart::SELECT, -1 );

				// The selected (active) class in Parallel Coordinates is shown.
				if ( this->Private->ClassNumber2Plot > -1 )
				{
					// Sets the nubmer of elements for the NbOfColoredFiberPerClass array.
					this->Private->NbOfColoredFibersPerClass
						->SetValue(this->Private->ClassNumber2Plot, CurrentClassTable->GetNumberOfRows());

					// Fill up plot with data from the tmptable and set color. 
					vtkPlot	*points = chart->AddPlot( vtkChart::POINTS );
					points->SetInputData( CurrentClassTable.GetPointer(),
										  this->VisibleColumns->GetValue( i ),
										  this->VisibleColumns->GetValue( n - j - 1 ) );
					points->SetColor( 0, 0, 0 );
					points->GetPen()->SetWidth( 0 );
					points->GetPen()->SetOpacity( 100 );

					// Set plot marker size and style
					vtkPlotPoints *plotPoints = vtkPlotPoints::SafeDownCast( points );
					plotPoints->SetMarkerSize( this->Private->ChartSettings[SCATTERPLOT]->MarkerSize );
					plotPoints->SetMarkerStyle( this->Private->ChartSettings[SCATTERPLOT]->MarkerStyle );

					// If option 'Multi Rendering' is selected all classes will be plotted.
					//TODO: optimize
				}
				else if ( this->Private->ClassNumber2Plot == -1 )
				{
					// Creates a plot (with its color information) for each class in the tree
					while ( currcl < this->Private->NbOfClasses )
					{
						//Constructing specific class table
						vtkSmartPointer<vtkTable> tmpTable = vtkSmartPointer<vtkTable>::New();
						tmpTable->DeepCopy( Input );

						// tmpTable holds the values for one single class only.
						// (all other classes will be deleted)
						for ( int del = tmpTable->GetNumberOfRows(); del >= 0; --del )
						{
							if ( tmpTable->GetValueByName( del, "Class_ID" ).ToInt() != currcl )
								tmpTable->RemoveRow( del );
						}

						// Sets the nubmer of elements for the NbOfColoredFiberPerClass array.
						this->Private->NbOfColoredFibersPerClass->SetValue(currcl, tmpTable->GetNumberOfRows());

						// Fill up plot with data from the tmptable and set color. 
						vtkPlot	*points = chart->AddPlot( vtkChart::POINTS );

						points->SetInputData( tmpTable,
											  this->VisibleColumns->GetValue( i ),
											  this->VisibleColumns->GetValue( n - j - 1 ) );
						points->GetPen()->SetColor( tmpTable->GetValueByName( 0, "cColorR" ).ToChar(),
													tmpTable->GetValueByName( 0, "cColorG" ).ToChar(),
													tmpTable->GetValueByName( 0, "cColorB" ).ToChar() );
						points->GetPen()->SetOpacity( 144 );

						// set plot marker size and style
						vtkPlotPoints *plotPoints = vtkPlotPoints::SafeDownCast( points );
						plotPoints->SetMarkerSize( this->Private->ChartSettings[SCATTERPLOT]->MarkerSize );
						plotPoints->SetMarkerStyle( this->Private->ChartSettings[SCATTERPLOT]->MarkerStyle );
						currcl++;
					}
					currcl = 0;
				}

				//Sets ToolTipLabelFormat to "X-Value, Y-Value" 
				chart->GetPlot( 0 )->SetTooltipLabelFormat( vtkStdString( "%x, %y" ) );
			}
			else if ( this->GetPlotType( pos ) == HISTOGRAM )
			{
				if ( this->Private->VisibleHistograms )
				{
					// We are on the diagonal - need a histogram plot.
					vtkChart* chart = this->GetChart( pos );
					chart->ClearPlots();

					// No mouse interaction on histograms
					chart->SetActionToButton( vtkChart::PAN, -1 );
					chart->SetActionToButton( vtkChart::ZOOM, -1 );
					chart->SetActionToButton( vtkChart::SELECT, -1 );

					vtkPlot *plot = chart->AddPlot( vtkChart::BAR );
					plot->SetPen(this->Private->ChartSettings[HISTOGRAM]
								  ->PlotPen.GetPointer());
					plot->SetBrush( this->Private->ChartSettings[HISTOGRAM]
									->PlotBrush.GetPointer() );
					vtkStdString name( this->VisibleColumns->GetValue( i ) );

					plot->SetInputData( this->Private->Histogram.GetPointer(), 
										name + "_extents", name + "_pops" );

					vtkAxis *axis = chart->GetAxis( vtkAxis::TOP );
					axis->SetTitle( removeUnit( name ) );
					axis->GetTitleProperties()->SetFontSize( 10 );
					if ( i != n - 1 )
						axis->SetBehavior( vtkAxis::FIXED );
					
					// Set the plot corner to the top-right
					vtkChartXY *xy = vtkChartXY::SafeDownCast( chart );

					if ( xy )
					{
						xy->SetBarWidthFraction( 1.0 );
						xy->SetPlotCorner( plot, 2 );
					}

					// set background color to light gray
					xy->SetBackgroundBrush( this->Private->ChartSettings[HISTOGRAM]
											->BackgroundBrush.GetPointer() );
				}
				else
				{
					// We are on the diagonal - need a histogram plot.
					vtkChart* chart = this->GetChart( pos );
					chart->ClearPlots();
				}
			}
			else if ( this->GetPlotType( pos ) == ACTIVEPLOT )
			{
				// This big plot in the top-right
				this->Private->BigChart = this->GetChart( pos );
				this->Private->BigChart->SetAnnotationLink( this->Private->Link.GetPointer() );

				// Added since VTK 6.1; needed to enable multi selection with modifiers (SHIFT,...)
				this->Private->BigChart->SetSelectionMethod( vtkChart::SELECTION_PLOTS );

				//Bigchart gets observed by one observer only 
				this->Private->BigChart->RemoveAllObservers();
				this->Private->BigChart->AddObserver( vtkCommand::SelectionChangedEvent, this,
													  &iAScatterPlotMatrix::BigChartSelectionCallback, 2.0 );

				this->SetChartSpan( pos, vtkVector2i( n - i, n - j ) );
				this->SetActivePlot( vtkVector2i( 0, n - 2 ) );
			}

			//// Only show bottom axis label for bottom plots
			if ( j > 0 )
			{
				vtkAxis *axis = this->GetChart( pos )->GetAxis( vtkAxis::BOTTOM );
				axis->SetTitle( "" );
				axis->SetLabelsVisible( false );
				axis->SetGridVisible( false );
			}
			else
			{
				vtkAxis *axis = this->GetChart( pos )->GetAxis( vtkAxis::BOTTOM );
				axis->SetTitle( removeUnit( this->VisibleColumns->GetValue( i ) ) );
				axis->GetTitleProperties()->SetFontSize( 10 );
				axis->SetLabelsVisible( false );
				axis->SetGridVisible( false );
				axis->GetTitleProperties()->SetJustificationToCentered();
				axis->GetTitleProperties()->SetVerticalJustificationToCentered();
				this->AttachAxisRangeListener( axis );
			}
			// Only show the left axis labels for left-most plots
			if ( i > 0 )
			{
				vtkAxis *axis = this->GetChart( pos )->GetAxis( vtkAxis::LEFT );
				axis->SetTitle( "" );
				axis->SetLabelsVisible( false );
				axis->SetGridVisible( false );
			}
			else
			{
				vtkAxis *axis = this->GetChart( pos )->GetAxis( vtkAxis::LEFT );
				axis->SetLabelsVisible( false );
				axis->SetGridVisible( false );
				axis->SetTitle( removeUnit( this->VisibleColumns->GetValue( n - j - 1 ) ) );
				axis->GetTitleProperties()->SetFontSize( 10 );
				axis->GetTitleProperties()->SetOrientation( 90.0 );
				axis->GetTitleProperties()->SetJustificationToCentered();
				axis->GetTitleProperties()->SetVerticalJustificationToCentered();
				this->AttachAxisRangeListener( axis );
			}
		}
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::AttachAxisRangeListener( vtkAxis* axis )
{
	axis->AddObserver( vtkChart::UpdateRange, this,
					   &iAScatterPlotMatrix::AxisRangeForwarderCallback );
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::AxisRangeForwarderCallback( vtkObject*,
													  unsigned long, void* )
{
	// Only set on the end axes, and propagated to all other matching axes.
	double r[2];
	int n = this->GetSize().GetX() - 1;
	for ( int i = 0; i < n; ++i )
	{
		this->GetChart( vtkVector2i( i, 0 ) )->GetAxis( vtkAxis::BOTTOM )->GetRange( r );
		for ( int j = 1; j < n - i; ++j )
			this->GetChart( vtkVector2i( i, j ) )->GetAxis( vtkAxis::BOTTOM )->SetRange( r );
		
		this->GetChart( vtkVector2i( i, n - i ) )->GetAxis( vtkAxis::TOP )->SetRange( r );
		this->GetChart( vtkVector2i( 0, i ) )->GetAxis( vtkAxis::LEFT )->GetRange( r );
		
		for ( int j = 1; j < n - i; ++j )
			this->GetChart( vtkVector2i( j, i ) )->GetAxis( vtkAxis::LEFT )->SetRange( r );
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::BigChartSelectionCallback( vtkObject*,
													 unsigned long event, void* )
{
	// forward the SelectionChangedEvent from the Big Chart plot
	this->InvokeEvent( event );

	// Added since VTK 6.1; to enable multi selection with modifiers (SHIFT,...)
	// Sets the higligthed selection for small scatter plots
	this->Private->Link->GetCurrentSelection()->GetNode( 0 )->SetSelectionList(
		this->Private->BigChart->GetPlot( 0 )->GetSelection() );

	// Set up custom made legend
	this->UpdateCustomLegend();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetTitle( const vtkStdString& title )
{
	if ( this->Title != title )
	{
		this->Title = title;
		this->Modified();
	}
}

//----------------------------------------------------------------------------
vtkStdString iAScatterPlotMatrix::GetTitle()
{
	return this->Title;
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetTitleProperties( vtkTextProperty *prop )
{
	if ( this->TitleProperties != prop )
	{
		this->TitleProperties = prop;
		this->Modified();
	}
}

//----------------------------------------------------------------------------
vtkTextProperty* iAScatterPlotMatrix::GetTitleProperties()
{
	return this->TitleProperties.GetPointer();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetAxisLabelProperties( int plotType,
												  vtkTextProperty *prop )
{
	if ( plotType >= 0 && plotType < iAScatterPlotMatrix::NOPLOT &&
		 this->Private->ChartSettings[plotType]->LabelFont != prop )
	{
		this->Private->ChartSettings[plotType]->LabelFont = prop;
		this->Modified();
	}
}

//----------------------------------------------------------------------------
vtkTextProperty* iAScatterPlotMatrix::GetAxisLabelProperties( int plotType )
{
	if ( plotType >= 0 && plotType < iAScatterPlotMatrix::NOPLOT )
		return this->Private->ChartSettings[plotType]->LabelFont.GetPointer();
	
	return NULL;
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetBackgroundColor( int plotType,
											  const vtkColor4ub& color )
{
	if ( plotType >= 0 && plotType < iAScatterPlotMatrix::NOPLOT )
	{
		this->Private->ChartSettings[plotType]->BackgroundBrush->SetColor( color );
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetAxisColor( int plotType,
										const vtkColor4ub& color )
{
	if ( plotType >= 0 && plotType < iAScatterPlotMatrix::NOPLOT )
	{
		this->Private->ChartSettings[plotType]->AxisColor = color;
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetGridVisibility( int plotType, bool visible )
{
	if ( plotType != NOPLOT )
	{
		this->Private->ChartSettings[plotType]->ShowGrid = visible;
		// How to update
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetGridColor( int plotType,
										const vtkColor4ub& color )
{
	if ( plotType >= 0 && plotType < iAScatterPlotMatrix::NOPLOT )
	{
		this->Private->ChartSettings[plotType]->GridColor = color;
		// How to update
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetAxisLabelVisibility( int plotType, bool visible )
{
	if ( plotType != NOPLOT )
	{
		this->Private->ChartSettings[plotType]->ShowAxisLabels = visible;
		// How to update
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetAxisLabelNotation( int plotType, int notation )
{
	if ( plotType != NOPLOT )
	{
		this->Private->ChartSettings[plotType]->LabelNotation = notation;
		// How to update
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetAxisLabelPrecision( int plotType, int precision )
{
	if ( plotType != NOPLOT )
	{
		this->Private->ChartSettings[plotType]->LabelPrecision = precision;
		// How to update
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetTooltipNotation( int plotType, int notation )
{
	if ( plotType != NOPLOT )
	{
		this->Private->ChartSettings[plotType]->TooltipNotation = notation;
		// How to update
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetTooltipPrecision( int plotType, int precision )
{
	if ( plotType != NOPLOT )
	{
		this->Private->ChartSettings[plotType]->TooltipPrecision = precision;
		// How to update
		this->Modified();
	}
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetScatterPlotSelectedRowColumnColor(
	const vtkColor4ub& color )
{
	this->Private->SelectedRowColumnBGBrush->SetColor( color );
	this->Modified();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetScatterPlotSelectedActiveColor(
	const vtkColor4ub& color )
{
	this->Private->SelectedChartBGBrush->SetColor( color );
	this->Modified();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::UpdateChartSettings( int plotType )
{
	if ( plotType == HISTOGRAM )
	{
		int plotCount = this->GetSize().GetX();

		for ( int i = 0; i < plotCount; i++ )
		{
			vtkChart *chart = this->GetChart( vtkVector2i( i, plotCount - i - 1 ) );
			this->Private->UpdateAxis( chart->GetAxis( vtkAxis::TOP ),
									   this->Private->ChartSettings[HISTOGRAM] );
			this->Private->UpdateAxis( chart->GetAxis( vtkAxis::RIGHT ),
									   this->Private->ChartSettings[HISTOGRAM] );
			this->Private->UpdateChart( chart,
										this->Private->ChartSettings[HISTOGRAM] );
		}
	}
	else if ( plotType == SCATTERPLOT )
	{
		int plotCount = this->GetSize().GetX();

		for ( int i = 0; i < plotCount - 1; i++ )
		{
			for ( int j = 0; j < plotCount - 1; j++ )
			{
				if ( this->GetPlotType( i, j ) == SCATTERPLOT )
				{
					vtkChart *chart = this->GetChart( vtkVector2i( i, j ) );
					bool updateleft = i == 0 ? true : false;
					bool updatebottom = j == 0 ? true : false;
					this->Private->UpdateAxis( chart->GetAxis( vtkAxis::LEFT ),
											   this->Private->ChartSettings[SCATTERPLOT], updateleft );
					this->Private->UpdateAxis( chart->GetAxis( vtkAxis::BOTTOM ),
											   this->Private->ChartSettings[SCATTERPLOT], updatebottom );
				}
			}
		}
	}
	else if ( plotType == ACTIVEPLOT && this->Private->BigChart )
	{
		this->Private->UpdateAxis( this->Private->BigChart->GetAxis(
			vtkAxis::TOP ), this->Private->ChartSettings[ACTIVEPLOT] );
		this->Private->UpdateAxis( this->Private->BigChart->GetAxis(
			vtkAxis::RIGHT ), this->Private->ChartSettings[ACTIVEPLOT] );
		this->Private->UpdateChart( this->Private->BigChart,
									this->Private->ChartSettings[ACTIVEPLOT] );
		this->Private->BigChart->SetSelectionMode( this->SelectionMode );
	}
}

//-----------------------------------------------------------------------------
void iAScatterPlotMatrix::SetSelectionMode( int selMode )
{
	if ( this->SelectionMode == selMode ||
		 selMode < vtkContextScene::SELECTION_NONE ||
		 selMode > vtkContextScene::SELECTION_TOGGLE )
		return;
	
	this->SelectionMode = selMode;
	
	if ( this->Private->BigChart )
		this->Private->BigChart->SetSelectionMode( selMode );

	this->Modified();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::UpdateSettings()
{

	// TODO: Should update the Scatter plot title

	this->UpdateChartSettings( ACTIVEPLOT );
	this->UpdateChartSettings( HISTOGRAM );
	this->UpdateChartSettings( SCATTERPLOT );
}

//----------------------------------------------------------------------------
bool iAScatterPlotMatrix::GetGridVisibility( int plotType )
{
	assert( plotType != NOPLOT );
	return this->Private->ChartSettings[plotType]->ShowGrid;
}

//----------------------------------------------------------------------------
vtkColor4ub iAScatterPlotMatrix::GetBackgroundColor( int plotType )
{
	assert( plotType != NOPLOT );
	return this->Private->ChartSettings[plotType]->BackgroundBrush
		->GetColorObject();
}

//----------------------------------------------------------------------------
vtkColor4ub iAScatterPlotMatrix::GetAxisColor( int plotType )
{
	assert( plotType != NOPLOT );
	return this->Private->ChartSettings[plotType]->AxisColor;
}

//----------------------------------------------------------------------------
vtkColor4ub iAScatterPlotMatrix::GetGridColor( int plotType )
{
	assert( plotType != NOPLOT );
	return this->Private->ChartSettings[plotType]->GridColor;
}

//----------------------------------------------------------------------------
bool iAScatterPlotMatrix::GetAxisLabelVisibility( int plotType )
{
	assert( plotType != NOPLOT );
	return this->Private->ChartSettings[plotType]->ShowAxisLabels;
}

//----------------------------------------------------------------------------
int iAScatterPlotMatrix::GetAxisLabelNotation( int plotType )
{
	assert( plotType != NOPLOT );
	return this->Private->ChartSettings[plotType]->LabelNotation;
}

//----------------------------------------------------------------------------
int iAScatterPlotMatrix::GetAxisLabelPrecision( int plotType )
{
	assert( plotType != NOPLOT );
	return this->Private->ChartSettings[plotType]->LabelPrecision;
}

//----------------------------------------------------------------------------
int iAScatterPlotMatrix::GetTooltipNotation( int plotType )
{
	assert( plotType != NOPLOT );
	return this->Private->ChartSettings[plotType]->TooltipNotation;
}

int iAScatterPlotMatrix::GetTooltipPrecision( int plotType )
{
	assert( plotType != NOPLOT );
	return this->Private->ChartSettings[plotType]->TooltipPrecision;
}

//----------------------------------------------------------------------------
vtkColor4ub iAScatterPlotMatrix::GetScatterPlotSelectedRowColumnColor()
{
	return this->Private->SelectedRowColumnBGBrush->GetColorObject();
}

//----------------------------------------------------------------------------
vtkColor4ub iAScatterPlotMatrix::GetScatterPlotSelectedActiveColor()
{
	return this->Private->SelectedChartBGBrush->GetColorObject();
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::PrintSelf( ostream &os, vtkIndent indent )
{
	Superclass::PrintSelf( os, indent );

	os << indent << "NumberOfBins: " << this->NumberOfBins << endl;
	os << indent << "Title: " << this->Title << endl;
	os << indent << "SelectionMode: " << this->SelectionMode << endl;
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::setRectangleSelectionOn()
{
	m_selectionTool = vtkChart::SELECT_RECTANGLE;
	this->Private->BigChart->SetActionToButton( m_selectionTool, vtkContextMouseEvent::LEFT_BUTTON );
}

//----------------------------------------------------------------------------
void iAScatterPlotMatrix::setPolygonSelectionOn()
{
	m_selectionTool = vtkChart::SELECT_POLYGON;
	this->Private->BigChart->SetActionToButton( m_selectionTool, vtkContextMouseEvent::LEFT_BUTTON );
}

}

