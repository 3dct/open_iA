/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iANModalProgressWidget.h"

#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QDialog>
#include <QPushButton>

iANModalProgressWidget::iANModalProgressWidget(QWidget *parent) :
	QWidget(parent)
{
	m_layout = new QGridLayout(this);
	m_layout->setColumnStretch(0, 0);
	m_layout->setColumnStretch(1, 1);
}

int iANModalProgressWidget::addProgressBar(int max, QString title, bool autoUpdateText/*=true*/, QString name/*=QString()*/) {
	return addProgressBar(max, new QLabel(title), autoUpdateText, name);
}

int iANModalProgressWidget::addProgressBar(int max, QLabel *label, bool autoUpdateText/*=false*/, QString name/*=QString()*/) {
	QProgressBar *pb = new QProgressBar(this);
	addWidget(pb, label, 0);

	int id = m_bars.size();

	m_bars.append(pb);
	m_barLabels.append(label);
	m_barTexts.append(label->text());
	m_barAutoUpdateText.append(autoUpdateText);

	pb->setMinimum(0);
	pb->setValue(0);
	setMax(id, max);

	if (!name.isNull()) {
		m_name2id.insert(name, id);
	}

	return id;
}

void iANModalProgressWidget::setMax(int pbid, int max) {
	if (exists(pbid)) {
		auto bar = m_bars[pbid];
		int value = bar->value() > max ? max : bar->value();
		m_bars[pbid]->setMaximum(max);
		setValue(pbid, value);
	}
}

void iANModalProgressWidget::setMax(QString name, int max) {
	setMax(m_name2id.value(name, -1), max);
}

void iANModalProgressWidget::setAutoUpdateText(int pbid, bool autoUpdateText) {
	if (exists(pbid)) {
		m_barAutoUpdateText[pbid] = autoUpdateText;
		update(pbid);
	}
}

void iANModalProgressWidget::setAutoUpdateText(QString name, bool autoUpdateText) {
	setAutoUpdateText(m_name2id.value(name, -1), autoUpdateText);
}

void iANModalProgressWidget::setText(int pbid, QString text) {
	if (exists(pbid)) {
		m_barTexts[pbid] = text;
		update(pbid);
	}
}

void iANModalProgressWidget::setText(QString name, QString text) {
	setText(m_name2id.value(name, -1), text);
}

void iANModalProgressWidget::addWidget(QWidget *widget, QString title) {
	addWidget(widget, new QLabel(title));
}

void iANModalProgressWidget::addWidget(QWidget *widget, QLabel *label/*=nullptr*/, int rowStretch/*=1*/) {
	int row = m_nextRow;
	m_nextRow++;

	if (label) {
		m_layout->addWidget(label, row, 0);
		m_layout->addWidget(widget, row, 1);
	} else {
		m_layout->addWidget(widget, row, 0, 1, 2);
	}

	m_layout->setRowStretch(row, rowStretch);
}

void iANModalProgressWidget::addSeparator() {
	QFrame *sep = new QFrame();
	sep->setFrameShape(QFrame::HLine);
	sep->setFrameShadow(QFrame::Sunken);
	addWidget(sep);
}

bool iANModalProgressWidget::isFinished() {
	for (auto bar : m_bars) {
		if (bar->value() < bar->maximum()) {
			return false;
		}
	}
	return true;
}

bool iANModalProgressWidget::isCanceled() {
	return m_canceled;
}

void iANModalProgressWidget::setCanceled(bool canceled) {
	if (!m_canceled && canceled) {
		cancel();
	} else {
		m_canceled = canceled;
	}
}

bool iANModalProgressWidget::exists(int pbid) {
	return pbid >= 0 && pbid < m_bars.size();
}

void iANModalProgressWidget::showDialog(QWidget *widget/*=nullptr*/) {
	QDialog *dialog = new QDialog(widget);
	dialog->setModal(false);

	QPushButton *buttonCancel = new QPushButton("Cancel");

	QGridLayout *layout = new QGridLayout(dialog);
	layout->addWidget(this, 0, 0, 1, 2);
	layout->addWidget(buttonCancel, 1, 1);
	layout->setRowStretch(0, 1);
	layout->setRowStretch(1, 0);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 0);
	
	connect(buttonCancel, &QPushButton::clicked, this, &iANModalProgressWidget::cancel);

	connect(this, &iANModalProgressWidget::finished, dialog, &QDialog::accept);
	connect(this, &iANModalProgressWidget::canceled, dialog, &QDialog::reject);

	auto dialogCode = dialog->exec();
	if (dialogCode == QDialog::Rejected) {
		emit canceled();
	}
}

void iANModalProgressWidget::setFirstValue(int value) {
	setValue(0, value);
}

void iANModalProgressWidget::setValue(int pbid, int value) {
	if (exists(pbid) && value >= 0) {
		auto bar = m_bars[pbid];
		bar->setValue(value);
		update(pbid);
	}
}

void iANModalProgressWidget::setValue(QString name, int value) {
	setValue(m_name2id.value(name, -1), value);
}

void iANModalProgressWidget::update(int pbid) {
	if (exists(pbid)) {
		auto bar = m_bars[pbid];
		auto value = bar->value();
		auto max = bar->maximum();
		QString text = m_barTexts[pbid];
		if (m_barAutoUpdateText[pbid]) {
			QString sval = QString::number(value);
			QString smax = QString::number(max);
			m_barLabels[pbid]->setText(text + " (" + sval + "/" + smax + ")");
		} else {
			m_barLabels[pbid]->setText(text);
		}
		emit updated(pbid, value,max);
		if (isFinished()) {
			emit finished();
		}
	}
}

void iANModalProgressWidget::finish() {
	for (int id = 0; id < m_bars.size(); id++) {
		auto bar = m_bars[id];
		setValue(id, bar->maximum());
	}
}

void iANModalProgressWidget::cancel() {
	m_canceled = true;
	emit canceled();
}

iANModalProgressUpdater::iANModalProgressUpdater(iANModalProgressWidget* progress) {
	connect(this, &iANModalProgressUpdater::finish, progress, &iANModalProgressWidget::finish, CONNECTION_TYPE);
	connect(this, &iANModalProgressUpdater::cancel, progress, &iANModalProgressWidget::cancel, CONNECTION_TYPE);

	connect(this, &iANModalProgressUpdater::showDialog, progress, &iANModalProgressWidget::showDialog, CONNECTION_TYPE);

	connect(this, &iANModalProgressUpdater::setFirstValue, progress, &iANModalProgressWidget::setFirstValue, CONNECTION_TYPE);

	connect(this, QOverload<int,int>::of(&iANModalProgressUpdater::setValue), progress, QOverload<int,int>::of(&iANModalProgressWidget::setValue), CONNECTION_TYPE);
	connect(this, QOverload<int,int>::of(&iANModalProgressUpdater::setMax), progress, QOverload<int,int>::of(&iANModalProgressWidget::setMax), CONNECTION_TYPE);
	connect(this, QOverload<int,bool>::of(&iANModalProgressUpdater::setAutoUpdateText), progress, QOverload<int,bool>::of(&iANModalProgressWidget::setAutoUpdateText), CONNECTION_TYPE);
	connect(this, QOverload<int,QString>::of(&iANModalProgressUpdater::setText), progress, QOverload<int,QString>::of(&iANModalProgressWidget::setText), CONNECTION_TYPE);

	connect(this, QOverload<QString,int>::of(&iANModalProgressUpdater::setValue), progress, QOverload<QString,int>::of(&iANModalProgressWidget::setValue), CONNECTION_TYPE);
	connect(this, QOverload<QString,int>::of(&iANModalProgressUpdater::setMax),   progress, QOverload<QString,int>::of(&iANModalProgressWidget::setMax), CONNECTION_TYPE);
	connect(this, QOverload<QString,bool>::of(&iANModalProgressUpdater::setAutoUpdateText), progress, QOverload<QString,bool>::of(&iANModalProgressWidget::setAutoUpdateText), CONNECTION_TYPE);
	connect(this, QOverload<QString,QString>::of(&iANModalProgressUpdater::setText), progress, QOverload<QString,QString>::of(&iANModalProgressWidget::setText), CONNECTION_TYPE);
}
