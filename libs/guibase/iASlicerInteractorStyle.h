#pragma once

#include <vtkInteractorStyleImage.h>
#include <vtkUnsignedCharArray.h>
#include <vtkRenderWindow.h>


//! Custom interactor style for slicers, disabling some interactions from vtkInteractorStyleImage
//! (e.g. rotation via ctrl+drag, window/level adjustments if not explicitly enabled), and adding
//! a transfer function by region mode.
class iASlicerInteractorStyle : public vtkInteractorStyleImage
{
public:
	enum InteractionMode
	{
		imNormal,
		imRegionSelect,
		imWindowLevelAdjust
	};
	static iASlicerInteractorStyle* New();
	vtkTypeMacro(iASlicerInteractorStyle, vtkInteractorStyleImage);

	iASlicerInteractorStyle() :
		m_rightButtonDragZoomEnabled(true),
		m_leftButtonDown(false),
		m_interactionMode(imNormal),
		m_dragStartImage(vtkSmartPointer<vtkUnsignedCharArray>::New())
	{
		m_dragStart[0] = m_dragStart[1] = 0;
	}

	void OnLeftButtonDown() override
	{
		// TODO: find more reliable way to determine whether left button currently pressed!
		//! With m_leftButtonDown it does not work properly if release happens outside!
		m_leftButtonDown = true;

		// if no modifier key pressed:
		if (!Interactor->GetShiftKey() && !Interactor->GetControlKey() && !Interactor->GetAltKey())
		{
			// if enabled, start "window-level" (click+drag) interaction:
			switch (m_interactionMode)
			{
			case imWindowLevelAdjust:
			{	// mostly copied from base class; but we don't want the "GrabFocus" call there,
				// that prevents the listeners to be notified of mouse move calls
				int x = this->Interactor->GetEventPosition()[0];
				int y = this->Interactor->GetEventPosition()[1];

				this->FindPokedRenderer(x, y);
				if (this->CurrentRenderer == nullptr)
				{
					return;
				}
				if (!this->Interactor->GetShiftKey() && !this->Interactor->GetControlKey())
				{
					this->WindowLevelStartPosition[0] = x;
					this->WindowLevelStartPosition[1] = y;
					this->StartWindowLevel();
				}
			}
			case imRegionSelect:
			{
				auto renWin = this->Interactor->GetRenderWindow();
				m_dragStart[0] = this->Interactor->GetEventPosition()[0];
				m_dragStart[1] = this->Interactor->GetEventPosition()[1];
				m_dragEnd[0] = m_dragStart[0];
				m_dragEnd[1] = m_dragStart[1];

				m_dragStartImage->Initialize();
				m_dragStartImage->SetNumberOfComponents(3);
				int const* size = renWin->GetSize();
				m_dragStartImage->SetNumberOfTuples(size[0] * size[1]);

				renWin->GetPixelData(0, 0, size[0] - 1, size[1] - 1, 1, m_dragStartImage);
			}
			}
		}

		// from the interaction possibilities in vtkInteractorStyleImage, only allow moving the slice (Shift+Drag):
		if (!this->Interactor->GetShiftKey())
		{
			return;
		}
		vtkInteractorStyleImage::OnLeftButtonDown();
	}
	void OnMouseMove() override
	{
		if (m_leftButtonDown && m_interactionMode == imRegionSelect &&
			!Interactor->GetShiftKey() && !Interactor->GetControlKey() && !Interactor->GetAltKey())
		{
			int const* size = this->Interactor->GetRenderWindow()->GetSize();
			m_dragEnd[0] = clamp(0, size[0] - 1, this->Interactor->GetEventPosition()[0]);
			m_dragEnd[1] = clamp(0, size[1] - 1, this->Interactor->GetEventPosition()[1]);

			vtkUnsignedCharArray* tmpPixelArray = vtkUnsignedCharArray::New();
			tmpPixelArray->DeepCopy(m_dragStartImage);
			unsigned char* pixels = tmpPixelArray->GetPointer(0);

			int minVal[2], maxVal[2];
			computeMinMax(minVal, maxVal, m_dragStart, m_dragEnd, size, 2);
			LOG(lvlInfo, QString("Drawing box from %1, %2, to %3, %4")
				.arg(minVal[0]).arg(minVal[1])
				.arg(maxVal[0]).arg(maxVal[1]));
			for (int i = minVal[0]; i <= maxVal[0]; ++i)
			{
				for (int c = 0; c < 3; ++c)
				{
					int minIdx = 3 * (minVal[1] * size[0] + i) + c;
					int maxIdx = 3 * (maxVal[1] * size[0] + i) + c;
					pixels[minIdx] = 255 ^ pixels[minIdx];
					pixels[maxIdx] = 255 ^ pixels[maxIdx];
				}
			}
			for (int i = minVal[1] + 1; i < maxVal[1]; ++i)
			{
				for (int c = 0; c < 3; ++c)
				{
					int minIdx = 3 * (i * size[0] + minVal[0]) + c;
					int maxIdx = 3 * (i * size[0] + maxVal[0]) + c;
					pixels[minIdx] = 255 ^ pixels[minIdx];
					pixels[maxIdx] = 255 ^ pixels[maxIdx];
				}
			}

			this->Interactor->GetRenderWindow()->SetPixelData(0, 0, size[0] - 1, size[1] - 1, pixels, 1);
			this->Interactor->GetRenderWindow()->SetPixelData(1, 0, size[0] - 1, size[1] - 1, pixels, 1);
			this->Interactor->GetRenderWindow()->Frame();

			tmpPixelArray->Delete();
		}
		else
		{
			vtkInteractorStyleImage::OnMouseMove();
		}
	}
	void OnLeftButtonUp() override
	{
		m_leftButtonDown = false;
		if (this->State == VTKIS_WINDOW_LEVEL)
		{
			this->EndWindowLevel();
		}
		else if (m_leftButtonDown && m_interactionMode == imRegionSelect &&
			!Interactor->GetShiftKey() && !Interactor->GetControlKey() && !Interactor->GetAltKey())
		{
			int const* size = this->Interactor->GetRenderWindow()->GetSize();
			m_dragEnd[0] = clamp(0, size[0] - 1, this->Interactor->GetEventPosition()[0]);
			m_dragEnd[1] = clamp(0, size[1] - 1, this->Interactor->GetEventPosition()[1]);
			// hide rectangle - reset image to what it was before starting dragging:
			//unsigned char* pixels = m_dragStartImage->GetPointer(0);
			//this->Interactor->GetRenderWindow()->SetPixelData(0, 0, size[0] - 1, size[1] - 1, pixels, 1);
			// ... handle actual event with given rectangle...
		}
		vtkInteractorStyleImage::OnLeftButtonUp();
	}
	//! @{ shift and control + mousewheel are used differently - don't use them for zooming!
	void OnMouseWheelForward() override
	{
		if (this->Interactor->GetControlKey() || this->Interactor->GetShiftKey())
		{
			return;
		}
		vtkInteractorStyleImage::OnMouseWheelForward();
	}
	void OnMouseWheelBackward() override
	{
		if (this->Interactor->GetControlKey() || this->Interactor->GetShiftKey())
		{
			return;
		}
		vtkInteractorStyleImage::OnMouseWheelBackward();
	}
	//! @}
	//! @{ Conditionally disable zooming via right button dragging
	void OnRightButtonDown() override
	{
		if (!m_rightButtonDragZoomEnabled)
		{
			return;
		}
		vtkInteractorStyleImage::OnRightButtonDown();
	}
	void setRightButtonDragZoomEnabled(bool enabled)
	{
		m_rightButtonDragZoomEnabled = enabled;
	}
	void setInteractionMode(InteractionMode mode)
	{
		m_interactionMode = mode;
	}
	bool leftButtonDown() const
	{
		return m_leftButtonDown;
	}
	InteractionMode interactionMode() const
	{
		return m_interactionMode;
	}

	//! @}
	/*
	virtual void OnChar()
	{
		vtkRenderWindowInteractor *rwi = this->Interactor;
		switch (rwi->GetKeyCode())
		{ // disable 'picking' action on p
		case 'P':
		case 'p':
			break;
		default:
			vtkInteractorStyleImage::OnChar();
		}
	}
	*/

private:
	bool m_rightButtonDragZoomEnabled;
	bool m_leftButtonDown;
	InteractionMode m_interactionMode;
	int m_dragStart[2], m_dragEnd[2];
	vtkSmartPointer<vtkUnsignedCharArray> m_dragStartImage;
};

vtkStandardNewMacro(iASlicerInteractorStyle);