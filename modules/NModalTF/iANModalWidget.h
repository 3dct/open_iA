// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iALabellingObjects.h"
#include "iANModalObjects.h"

#include <QList>
#include <QMap>
#include <QWidget>

class iANModalController;
class iANModalPreprocessor;
class iANModalLabelsWidget;
class iASlicer;
class iAMdiChild;

class QLabel;
class QGridLayout;

class iANModalWidget : public QWidget
{
	Q_OBJECT

public:
	iANModalWidget(iAMdiChild* mdiChild);

private:
	iANModalController* m_c;
	QSharedPointer<iANModalPreprocessor> m_preprocessor;
	iAMdiChild* m_mdiChild;

	QGridLayout* m_layoutSlicersGrid;
	iANModalLabelsWidget* m_labelsWidget;

	QLabel* m_labelModalityCount;

	QMap<int, iANModalLabel> m_labels;

private slots:
	void onAllSlicersInitialized();
	void onAllSlicersReinitialized();
	void onHistogramInitialized(int index);

	void onSeedsAdded(const QList<iASeed>&);
	void onSeedsRemoved(const QList<iASeed>&);
	void onAllSeedsRemoved();
	void onLabelAdded(const iALabel&);
	void onLabelRemoved(const iALabel&);
	void onLabelsColorChanged(const QList<iALabel>&);

	void onLabelOpacityChanged(int labelId);
	void onLabelRemoverStateChanged(int labelId);
};
