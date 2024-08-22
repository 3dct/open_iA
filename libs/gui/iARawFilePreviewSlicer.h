// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iARawFileParameters.h>
#include <iASlicerMode.h>

#include <QObject>

template <typename LayoutType>
LayoutType* createLayout()
{
	auto layout = new LayoutType();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);
	return layout;
}

struct iARawFilePreviewSlicerImpl;

class QVBoxLayout;

class iARawFilePreviewSlicer: public QObject
{
	Q_OBJECT
private:
	std::unique_ptr<iARawFilePreviewSlicerImpl> m;
	void stopUpdate();
	void showImage();
	void setStatus(QString const& status);

public:
	iARawFilePreviewSlicer(iASlicerMode mode, QString const& fileName);
	~iARawFilePreviewSlicer();
	QVBoxLayout* layout();
	void loadImage(iARawFileParameters const& params);
	void updateImage();
	void setOutOfDate();
};
