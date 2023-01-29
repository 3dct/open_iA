// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASlicerMode.h>

#include <vtkTransform.h>

#include <QWidget>

class iASingleSlicerSettings;
class iASlicer;
class iATransferFunction;

class vtkCamera;
class vtkImageData;

class iASimpleSlicerWidget : public QWidget
{
	Q_OBJECT

public:
	iASimpleSlicerWidget(QWidget* parent = nullptr, bool enableInteraction = false, Qt::WindowFlags f = Qt::WindowFlags());
	~iASimpleSlicerWidget();

	void setSlicerMode(iASlicerMode slicerMode);
	iASlicerMode getSlicerMode();

	void setSliceNumber(int sliceNumber);
	int getSliceNumber();

	bool hasHeightForWidth() const override;
	int heightForWidth(int width) const override;

	void applySettings(iASingleSlicerSettings const & settings);
	void changeData(vtkImageData* imageData, iATransferFunction* tf, QString const& name);

	void setCamera(vtkCamera* camera);

	iASlicer* getSlicer()
	{
		return m_slicer;
	}

public slots:
	void update();

private:
	bool m_enableInteraction;
	vtkTransform *m_slicerTransform;
	iASlicer *m_slicer;
};
