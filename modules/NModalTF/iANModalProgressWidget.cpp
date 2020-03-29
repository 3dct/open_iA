/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

int iANModalProgressWidget::addProgressBar(int max, QString title) {
	return addProgressBar(max, new QLabel(title));
}

int iANModalProgressWidget::addProgressBar(int max, QLabel *label) {
	QProgressBar *pb = new QProgressBar(this);
	pb->setMinimum(0);
	pb->setMaximum(max);
	pb->setValue(0);

	addWidget(pb, label, 0);

	int id = m_bars.size();
	m_bars.append(pb);
	return id;
}

void iANModalProgressWidget::addWidget(QWidget *widget, QString title, int rowStretch/*=1*/) {
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

void iANModalProgressWidget::showDialog(QWidget *widget/*=nullptr*/) {
	QDialog *dialog = new QDialog(widget);
	dialog->setModal(false);

	QPushButton *buttonCancel = new QPushButton("Cancel");
	buttonCancel->setEnabled(false); // TODO enable

	QGridLayout *layout = new QGridLayout(dialog);
	layout->addWidget(this, 0, 0, 1, 2);
	layout->addWidget(buttonCancel, 1, 1);
	layout->setRowStretch(0, 1);
	layout->setRowStretch(1, 0);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 0);

	connect(this, SIGNAL(finished()), dialog, SLOT(accept()));
	connect(buttonCancel, SIGNAL(clicked()), dialog, SLOT(reject()));

	dialog->exec();
}

void iANModalProgressWidget::updateFirstProgressBar(int value) {
	updateProgressBar(0, value);
}

void iANModalProgressWidget::updateProgressBar(int pbid, int value) {
	if (pbid < m_bars.size()) {
		m_bars[pbid]->setValue(value);
		if (isFinished()) {
			emit finished();
		}
	}
}