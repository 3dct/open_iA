#pragma once
#include "iACompTableInteractorStyle.h"

//vtk
#include <vtkSmartPointer.h>

//CompVis
class iACompVariableTable;

//vtk
class vtkCellPicker;
class vtkPropPicker;

//C++
#include <vector>

class iACompVariableTableInteractorStyle : public iACompTableInteractorStyle
{
	public:
		static iACompVariableTableInteractorStyle* New();
		vtkTypeMacro(iACompVariableTableInteractorStyle, iACompTableInteractorStyle);

		//initialize the visualization for which the interactionStyle will be applicable
		void setVariableTableVisualization(iACompVariableTable* visualization);

		virtual void OnLeftButtonDown();
		virtual void OnLeftButtonUp();

		virtual void OnMouseMove();

		virtual void OnMiddleButtonDown();
		virtual void OnRightButtonDown();
		virtual void OnMouseWheelForward();
		virtual void OnMouseWheelBackward();
		virtual void OnKeyPress();
		virtual void OnKeyRelease();

		virtual void Pan();

	protected:
		
		iACompVariableTableInteractorStyle();

	

	private:

		/*** Interaction Picking ***/

		void setPickList(std::vector<vtkSmartPointer<vtkActor>>* originalRowActors);

		/**
		 * @brief The bar chart, showing the number of objects for each dataset, is removed from the table visualization, if one exists.
		 * @return bool that is true when the bar chart was removed, false when no bar chart was present beforehand
		*/
		bool removeBarChart();


		//VariableTable Visualization
		iACompVariableTable* m_visualization;


		//stores the picker to pick individual bins
		vtkSmartPointer<vtkCellPicker> m_picker;


};

