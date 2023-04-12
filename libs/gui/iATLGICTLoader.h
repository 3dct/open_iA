// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QFileInfoList>
#include <QSharedPointer>
#include <QThread>

#include <memory>

class iADataCollection;
class iAMultiStepProgressObserver;
class MdiChild;

//! Helper for loading Talbot-Lau Grating Interferometry (TLGI) Computed Tomography (CT) image stacks.
class iATLGICTLoader : public QThread
{
	Q_OBJECT
public:
	iATLGICTLoader();
	bool setup(QString const & baseDirectory, QWidget* parent);
	void start(MdiChild* child);
protected:
	~iATLGICTLoader();	// destructur private to make sure we can only be constructed on the heap
						// and will destruct ourselves in finishUp()
private:

	QString m_baseDirectory;
	MdiChild* m_child;
	
	std::shared_ptr<iADataCollection> m_result;
	double m_spacing[3];
	double m_origin[3];
	QFileInfoList m_subDirs;
	iAMultiStepProgressObserver* m_multiStepObserver;

	virtual void run();
private slots:
	void finishUp();
};
