// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASlicerMode.h>

#include <QWidget>

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

	void applySettings(QVariantMap const & settings);
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
	iASlicer *m_slicer;
};
