#pragma once
#include "iACompTableInteractorStyle.h"

//vtk
#include <vtkSmartPointer.h>

//CompVis
class iACompVariableTable;

//vtk
class vtkPropPicker;

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

		void resetVariableTable();


		//VariableTable Visualization
		iACompVariableTable* m_visualization;

		//stores the picker to pick individual bins and datasets
		vtkSmartPointer<vtkPropPicker> m_picker;


};

