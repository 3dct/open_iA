// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACompVisOptions.h"
#include "iACorrelationCoefficient.h"
#include "ui_CompHistogramTable.h"

//vtk
#include <vtkGraphLayoutStrategy.h>
#include <vtkInteractorStyleRubberBand2D.h>
#include <vtkSmartPointer.h>

//Qt
#include <QDockWidget>

#include <cmath>    // for abs (cpp), atan

//CompVis
class iACompVisMain;
class iACorrelationCoefficient;
class iACorrelationGraphLayout;
class iACsvDataStorage;
class iAGraphInteractorStyle;

class iAMainWindow;
class iAQVTKWidget;

//vtk
class vtkActor2D;
class vtkActor;
class vtkBalloonWidget;
class vtkDoubleArray;
class vtkGraphLayoutView;
class vtkHoverWidget;
class vtkLookupTable;
class vtkMutableUndirectedGraph;
class vtkPoints;
class vtkPropPicker;
class vtkRenderer;
class vtkScalarBarWidget;
class vtkTextActor;
class vtkUnsignedCharArray;
class vtkViewTheme;

class iACompCorrelationMap : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT
   public:
	iACompCorrelationMap(iAMainWindow* parent, iACorrelationCoefficient* corrCalculation, iACsvDataStorage* dataStorage, iACompVisMain* main);
	void showEvent(QShowEvent* event);

	void updateCorrelationMap(std::map<QString, Correlation::CorrelationStore>* correlations, std::map<int, std::vector<double>>* pickStatistic);
	void resetCorrelationMap();

	std::vector<vtkSmartPointer<vtkActor>>* getArcActors();
	std::map<vtkSmartPointer<vtkActor>, double>* getArcPercentPairs();
	std::map<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkTextActor>>* getOuterArcsWithLegends();

private:

	void initializeCorrelationMap();

	void initializeLutForVertices();
	void initializeVertices(QStringList attrNames);

	void initializeLutForEdges();
	void initializeLegend(vtkScalarBarWidget* widget);
	void initializeEdges();
	double colorEdges(vtkIdType startVertex, vtkIdType endVertex, std::map<QString, Correlation::CorrelationStore>* correlations, std::map<vtkIdType, QString>* vertices);

	void initializeArcs();
	void initializeLutForArcs();
	void initializeArcLegend();

	//draw the arc with on a specific start position (startPos), with a defined color and line width for a specified length in degree
	//the arc can be dotted if the stippled variable is true, then the variables lineStipplePattern & lineStippleRepeat have to be set, otherwise they can be set to 0
	void drawArc(double lengthInDegree, double* startPos, double* color, double lineWidth, bool stippled, int lineStipplePattern, int lineStippleRepeat);
	void drawGlyphs(vtkSmartPointer<vtkPoints> positions, vtkSmartPointer<vtkDoubleArray> colors, vtkSmartPointer<vtkDoubleArray> scales);
	void drawLegend(vtkSmartPointer<vtkPoints> positions, QStringList names);
	void drawInnerArc(std::vector<double> dataPoints, double* parentPosition, double parentTheta, double parentPhi,
		double parentAngle, double parentArcLength, int dataIndex);

	void calculateLabelPosition(vtkSmartPointer<vtkPoints> labelPositions, double theta, double arcLength, double phi, double radiusOffset);

	void renderWidget();

	void updateEdges(std::map<QString, Correlation::CorrelationStore>* correlations);
	void updateArcs(std::map<int, std::vector<double>>* pickStatistic);
	void removeOldActors();

	iACorrelationCoefficient* m_corrCalculation;
	iACsvDataStorage* m_dataStorage;

	iACompVisMain* m_main;

	iAQVTKWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;

	//stores for every vertex its name
	std::map<vtkIdType, QString>* m_vertices;

	QStringList m_attrNames;
	double m_radius = 0.75;
	double m_PI = std::atan(1) * 4;

	vtkSmartPointer<iAGraphInteractorStyle> style;
	vtkSmartPointer<iACorrelationGraphLayout> m_graphLayout;
	vtkSmartPointer<vtkGraphLayoutView> m_graphLayoutView;
	vtkSmartPointer<vtkViewTheme> m_theme;
	vtkSmartPointer<vtkMutableUndirectedGraph> m_graph;

	vtkSmartPointer<vtkLookupTable> m_lutForEdges;
	vtkSmartPointer<vtkLookupTable> m_lutForVertices;
	vtkSmartPointer<vtkLookupTable> m_lutForArcs;

	std::vector<vtkSmartPointer<vtkActor>>* arcActors;
	std::map<vtkSmartPointer<vtkActor>, double>* arcPercentPair;
	std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>>* outerArcWithInnerArcs;
	std::map<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkTextActor>>* outerArcWithLegend;

	//for each arc actor store to which dataset it belong by storing the dataIndex and whether it is
	//a selected inner-arc, non-selected inner-arc or an outer arc
	//outer-arc = 0; selected inner arc = 1; not-selected inner arc = 2,
	std::map< vtkSmartPointer<vtkActor>, std::map<int, double>*>* m_arcDataIndxTypePair;

	std::vector<vtkSmartPointer<vtkActor>>* glyphActors;
	std::vector<vtkSmartPointer<vtkTextActor>>* legendActors;

	//stores the last interaction that was performed to make a reinitialization after minimizing etc. possible
	iACompVisOptions::lastState m_lastState;

	friend class iAGraphInteractorStyle;    //!< to allow iAGraphInteractorStyle access to private members
};
