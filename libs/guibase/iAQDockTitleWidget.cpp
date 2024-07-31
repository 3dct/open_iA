// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAQDockTitleWidget.h"

#include <QDockWidget>
#include <QFrame>
#include <QLabel>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <QDesktopServices>
#include <QPainter>



// iAVerticalLabel


//! a label with vertical text orientation (from bottom to top)
//! currently only used in iAQDockTitleWidget
class iAVerticalLabel : public QLabel
{
public:
	explicit iAVerticalLabel(const QString& text);

protected:
	void paintEvent(QPaintEvent*) override;
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;
};

iAVerticalLabel::iAVerticalLabel(const QString& text): QLabel(text)
{}

void iAVerticalLabel::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.translate(0, sizeHint().height());
	painter.rotate(270);
	QFont f(painter.font());
	f.setBold(true);
	painter.setFont(f);
	painter.drawText(QRect(QPoint(0, 0), QLabel::sizeHint()), Qt::AlignLeft | Qt::AlignTop, text());
	painter.drawText(0, 0, text());
}

QSize iAVerticalLabel::minimumSizeHint() const
{
	QSize s = QLabel::minimumSizeHint();
	return QSize(s.height(), 0);
}

QSize iAVerticalLabel::sizeHint() const
{
	QSize s = QLabel::sizeHint();
	return QSize(s.height(), s.width());
}



// iAQDockTitleWidget

iAQDockTitleWidget::iAQDockTitleWidget(QDockWidget* parent, QString infoLink) : QFrame(parent)
{
	setProperty("qssClass", "iAQDockTitleWidget");
	setLayout(new QVBoxLayout());
	int mw = parent->style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, nullptr, parent);
	layout()->setContentsMargins(mw, mw, mw, mw);
	layout()->setSpacing(2);
	auto closeButton = new QToolButton();
	closeButton->setProperty("qssClass", "dockwidget-close");
	closeButton->setMinimumSize(0, 10);
	connect(closeButton, &QToolButton::clicked, this, [this]()
	{
		QDockWidget* q = qobject_cast<QDockWidget*>(parentWidget());
		q->close();
	});
	auto floatButton = new QToolButton();
	floatButton->setProperty("qssClass", "dockwidget-float");
	floatButton->setMinimumSize(0, 10);
	connect(floatButton, &QToolButton::clicked, this, [this]()
	{
		QDockWidget* q = qobject_cast<QDockWidget*>(parentWidget());
		q->setFloating(!q->isFloating());
	});
	auto infoButton = new QToolButton();
	infoButton->setProperty("qssClass", "dockwidget-info");
	infoButton->setMinimumSize(0, 10);
	connect(infoButton, &QToolButton::clicked, this, [infoLink]() { QDesktopServices::openUrl(QUrl(infoLink)); });
	layout()->addWidget(closeButton);
	layout()->addWidget(floatButton);
	layout()->addWidget(infoButton);
	layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
	layout()->addWidget(new iAVerticalLabel(parent->windowTitle()));
}

QSize iAQDockTitleWidget::sizeHint() const
{
	QDockWidget* q = qobject_cast<QDockWidget*>(parentWidget());
	QFontMetrics titleFontMetrics = q->fontMetrics();
	int mw = q->style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, nullptr, q);
	auto h = geometry().height();
	auto w = titleFontMetrics.height() + 2 * mw;
	return QSize(w, h);
}
