// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iARawFileParameters.h>
#include <iASlicerMode.h>

#include <QObject>

class vtkImageData;
class vtkScalarsToColors;

struct iARawFilePreviewSlicerImpl;

class QVBoxLayout;

//! Loads preview slices of a raw image from disk
class iARawFilePreviewSlicer: public QObject
{
	Q_OBJECT
private:
	std::shared_ptr<iARawFilePreviewSlicerImpl> m;    // shared in order to keep it available in background worker if this class is deleted while worker is still running
	void stopUpdate();
	void showImage();
	void setStatus(QString const& status);

public:
	iARawFilePreviewSlicer(iASlicerMode mode, QString const& fileName, vtkScalarsToColors* tf);
	~iARawFilePreviewSlicer();
	QVBoxLayout* layout();
	void loadImage(iARawFileParameters const& params);
	void updateImage();
	void setOutOfDate();
	vtkImageData * image() const;
	int sliceNr() const;
	void updateColors();

signals:
	void loadDone();
};
