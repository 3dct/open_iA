// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QFileInfoList>
#include <QThread>

#include <memory>

class iADataCollection;
class iAMultiStepProgressObserver;
class iAMdiChild;

//! Helper for loading Talbot-Lau Grating Interferometry (TLGI) Computed Tomography (CT) image stacks.
class iATLGICTLoader : public QThread
{
	Q_OBJECT
public:
	iATLGICTLoader();
	bool setup(QString const & baseDirectory, QWidget* parent);
	void start(iAMdiChild* child);
protected:
	~iATLGICTLoader();	// destructur private to make sure we can only be constructed on the heap
						// and will destruct ourselves in finishUp()
private:

	QString m_baseDirectory;
	iAMdiChild* m_child;

	std::shared_ptr<iADataCollection> m_result;
	double m_spacing[3];
	double m_origin[3];
	QFileInfoList m_subDirs;
	iAMultiStepProgressObserver* m_multiStepObserver;

	void run() override;
private slots:
	void finishUp();
};
