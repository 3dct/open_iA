#pragma once

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkRenderedGraphRepresentation.h"
#include "iACorrelationCoefficient.h"
#include "vtkGraphLayoutStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkCommand.h"

class iAMainWindow;
class iACorrelationCoefficient;
class iACsvDataStorage;
class QVTKOpenGLNativeWidget;
class vtkRenderer;
class vtkGraphLayoutView;
class vtkMutableUndirectedGraph;
class vtkLookupTable;
class vtkDoubleArray;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkActor;
class vtkActor2D;
class vtkPropPicker;
class vtkTextActor;
class vtkHoverWidget;
class vtkBalloonWidget;
class iACompVisMain;

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

	void reinitializeCorrelationMap(iACorrelationCoefficient* newCorrCalculation);

private:

	void initializeCorrelationMap();

	void initializeLutForVertices();
	void initializeVertices(QStringList attrNames);
	
	void initializeLutForEdges();
	void initializeLegend(vtkScalarBarWidget* widget);
	void initializeEdges(QStringList attrNames);
	double colorEdges(vtkIdType startVertex, vtkIdType endVertex, std::map<QString, Correlation::CorrelationStore>* correlations, std::map<vtkIdType, QString>* vertices);

	void initializeArcs();
	void initializeLutForArcs();
	void initializeArcLegend();

	//draw the arc with on a specific start position (startPos), with a defined color and line width for a specified length in degree
	//the arc can be dotted if the stippled variable is true, then the variables lineStipplePattern & lineStippleRepeat have to be set, otherwise they can be set to 0
	void drawArc(double lengthInDegree, double* startPos, double* color, double lineWidth, bool stippled, int lineStipplePattern, int lineStippleRepeat);
	void drawGlyphs(vtkSmartPointer<vtkPoints> positions, vtkSmartPointer<vtkDoubleArray> colors, vtkSmartPointer<vtkDoubleArray> scales);
	void drawLegend(vtkSmartPointer<vtkPoints> positions, QStringList names);
	void drawInnerArc(std::vector<double> data, double* parentPosition, double parentTheta, double parentPhi, double parentAngle, double parentArcLength, int dataIndex);

	void calculateLabelPosition(vtkSmartPointer<vtkPoints> labelPositions, double theta, double arcLength, double phi, double radiusOffset);

	void renderWidget();

	void updateEdges(std::map<QString, Correlation::CorrelationStore>* correlations);
	void updateArcs(std::map<int, std::vector<double>>* pickStatistic);
	void removeOldActors();

	iACorrelationCoefficient* m_corrCalculation;
	iACsvDataStorage* m_dataStorage;

	iACompVisMain* m_main;

	QVTKOpenGLNativeWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;

	int m_numberOfAttr;
	double m_radius = 0.75;
	double m_PI = std::atan(1) * 4;
	QStringList m_attrNames;
	//stores for every vertex its name
	std::map<vtkIdType, QString>* m_vertices;

	//inner class
	//class copied from vtkForceDirectedLayoutStrategy with changed methods:
	// CoolDown(), forceAttract(), forceRepulse()
	class CorrelationGraphLayout : public vtkGraphLayoutStrategy
	{
		public:
			static CorrelationGraphLayout* New();
			vtkTypeMacro(CorrelationGraphLayout, vtkGraphLayoutStrategy);

			//@{
			/**
			* Seed the random number generator used to jitter point positions.
			* This has a significant effect on their final positions when
			* the layout is complete.
			*/
			vtkSetClampMacro(RandomSeed, int, 0, VTK_INT_MAX);
			vtkGetMacro(RandomSeed, int);
			//@}

			//@{
			/**
			 * Set / get the region in space in which to place the final graph.
			 * The GraphBounds only affects the results if AutomaticBoundsComputation
			 * is off.
			 */
			vtkSetVector6Macro(GraphBounds, double);
			vtkGetVectorMacro(GraphBounds, double, 6);
			//@}

			//@{
			/**
			 * Turn on/off automatic graph bounds calculation. If this
			 * boolean is off, then the manually specified GraphBounds is used.
			 * If on, then the input's bounds us used as the graph bounds.
			 */
			vtkSetMacro(AutomaticBoundsComputation, vtkTypeBool);
			vtkGetMacro(AutomaticBoundsComputation, vtkTypeBool);
			vtkBooleanMacro(AutomaticBoundsComputation, vtkTypeBool);
			//@}

			//@{
			/**
			 * Set/Get the maximum number of iterations to be used.
			 * The higher this number, the more iterations through the algorithm
			 * is possible, and thus, the more the graph gets modified.
			 * The default is '50' for no particular reason
			 */
			vtkSetClampMacro(MaxNumberOfIterations, int, 0, VTK_INT_MAX);
			vtkGetMacro(MaxNumberOfIterations, int);
			//@}

			//@{
			/**
			 * Set/Get the number of iterations per layout.
			 * The only use for this ivar is for the application
			 * to do visualizations of the layout before it's complete.
			 * The default is '50' to match the default 'MaxNumberOfIterations'
			 */
			vtkSetClampMacro(IterationsPerLayout, int, 0, VTK_INT_MAX);
			vtkGetMacro(IterationsPerLayout, int);
			//@}

			//@{
			/**
			 * Set/Get the Cool-down rate.
			 * The higher this number is, the longer it will take to "cool-down",
			 * and thus, the more the graph will be modified.
			 */
			vtkSetClampMacro(CoolDownRate, double, 0.01, VTK_DOUBLE_MAX);
			vtkGetMacro(CoolDownRate, double);
			//@}

			//@{
			/**
			 * Turn on/off layout of graph in three dimensions. If off, graph
			 * layout occurs in two dimensions. By default, three dimensional
			 * layout is off.
			 */
			vtkSetMacro(ThreeDimensionalLayout, vtkTypeBool);
			vtkGetMacro(ThreeDimensionalLayout, vtkTypeBool);
			vtkBooleanMacro(ThreeDimensionalLayout, vtkTypeBool);
			//@}

			//@{
			/**
			 * Turn on/off use of random positions within the graph bounds as initial points.
			 */
			vtkSetMacro(RandomInitialPoints, vtkTypeBool);
			vtkGetMacro(RandomInitialPoints, vtkTypeBool);
			vtkBooleanMacro(RandomInitialPoints, vtkTypeBool);
			//@}

			//@{
			/**
			 * Set the initial temperature.  If zero (the default) , the initial temperature
			 * will be computed automatically.
			 */
			vtkSetClampMacro(InitialTemperature, float, 0.0, VTK_FLOAT_MAX);
			vtkGetMacro(InitialTemperature, float);
			//@}

			/**
			 * This strategy sets up some data structures
			 * for faster processing of each Layout() call
			 */
			void Initialize() override;

			/**
			 * This is the layout method where the graph that was
			 * set in SetGraph() is laid out. The method can either
			 * entirely layout the graph or iteratively lay out the
			 * graph. If you have an iterative layout please implement
			 * the IsLayoutComplete() method.
			 */
			void Layout() override;

			/**
			 * I'm an iterative layout so this method lets the caller
			 * know if I'm done laying out the graph
			 */
			int IsLayoutComplete() override { return this->LayoutComplete; }

			void setCorrelations(std::map<QString, Correlation::CorrelationStore>* correlations);
			void setVertices(std::map<vtkIdType, QString>* vertices);
			double getWeightForEdge(vtkIdType startV, vtkIdType endV);

	protected:
		CorrelationGraphLayout();
		~CorrelationGraphLayout() override;

		double forceRepulse(double x, double k, double weight);
		double forceAttract(double x, double k, double weight);
		double CoolDown(double t, double r);

		double calculateHooksLaw(double displacement);

		double GraphBounds[6];
		vtkTypeBool   AutomaticBoundsComputation;  //Boolean controls automatic bounds calc.
		int   MaxNumberOfIterations;  //Maximum number of iterations.
		double CoolDownRate;  //Cool-down rate.  Note:  Higher # = Slower rate.
		double InitialTemperature;
		vtkTypeBool   ThreeDimensionalLayout;  //Boolean for a third dimension.
		vtkTypeBool RandomInitialPoints; //Boolean for having random points
	private:

		// A vertex contains a position and a displacement.
		typedef struct
		{
			double x[3];
			double d[3];
		} vtkLayoutVertex;

		// An edge consists of two vertices joined together.
		// This struct acts as a "pointer" to those two vertices.
		typedef struct
		{
			int t;
			int u;
		} vtkLayoutEdge;

		int RandomSeed;
		int IterationsPerLayout;
		int TotalIterations;
		int LayoutComplete;
		double Temp;
		double optDist;
		vtkLayoutVertex *v;
		vtkLayoutEdge *e;

		std::map<vtkIdType, QString>* m_vertices;
		std::map<QString, Correlation::CorrelationStore>* m_correlations;

		const int K = 1;
		const double MASS = 1;

		double minDist;
		double maxDist;

	};

	//inner class
	class GraphInteractorStyle : public vtkInteractorStyleRubberBand2D
	{
		public:
			static GraphInteractorStyle* New();
			vtkTypeMacro(GraphInteractorStyle, vtkInteractorStyleRubberBand2D);

			void setGraphLayoutView(vtkSmartPointer<vtkGraphLayoutView> graphLayoutView);
			void setBaseClass(iACompCorrelationMap* baseClass);
			bool setPickList();

			void removeHighlighting();
			void resetEdgeVisualization();

			virtual void OnLeftButtonDown();
			virtual void OnMiddleButtonDown();
			virtual void OnRightButtonDown();
			virtual void OnMouseWheelForward();
			virtual void OnMouseWheelBackward();
			virtual void OnKeyPress();
			virtual void OnKeyRelease();

		protected:
			GraphInteractorStyle();

		private:

			void drawSelectedArc(vtkSmartPointer<vtkActor> arcActor);
			void drawPercentLabel(vtkSmartPointer<vtkActor> arcActor);
			void storePickedActor(vtkSmartPointer<vtkActor> arcActor, double* position, double lineWidth);
			//resets old arcs to get back to their initial position/linewidth,...
			void resetOldPickedActor();
			//adds highlighting arc below picked arc
			void addHighlightingBelow(vtkSmartPointer<vtkActor> arcActor);
			//moves label when according arc was picked
			void moveLabel(vtkSmartPointer<vtkActor> arcActor);

			void initializeLutForEdgesWithLabel();
			

			iACompCorrelationMap* m_baseClass;
			vtkSmartPointer<vtkPropPicker> m_actorPicker;
			vtkSmartPointer<vtkGraphLayoutView> m_graphLayoutView;
			vtkSmartPointer<vtkTextActor> m_percentLegend;
			
			std::vector<vtkSmartPointer<vtkActor>>* m_oldPickedActors;
			//for each oldPickedActor the first 3 positions of the vector are the old position, and the 4th position is the lineWidth
			std::map<vtkSmartPointer<vtkActor>, std::vector<double>>* m_oldAttributesForOldPickedActors;
			//stores the yellow arc for highlighting
			vtkSmartPointer<vtkActor> highlightingActor;
			//stores all moved labels, which were moved when according arc was picked
			std::vector <vtkSmartPointer<vtkTextActor>>* movedLabels;
			//stores for each moved label its old position
			std::map<vtkSmartPointer<vtkTextActor>, std::vector<double>>* m_oldLabelPositionInWorldCoords;


			bool edgeLabelsShown;
			vtkSmartPointer<vtkLookupTable> lutForLabels;
	};

	vtkSmartPointer<GraphInteractorStyle> style;
	vtkSmartPointer<CorrelationGraphLayout> m_graphLayout;
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

};

