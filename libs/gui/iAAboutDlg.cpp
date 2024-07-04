// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAAboutDlg.h"

#include <QDialog>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QBoxLayout>

void iAAboutDlg::show(QWidget* parent, QPixmap const & aboutImg, QString const & buildInfo, QString const & gitVersion, int screenHeight)
{
	QDialog dlg(parent);
	dlg.setWindowTitle("About open_iA");
	dlg.setLayout(new QVBoxLayout());

	auto imgLabel = new QLabel();
	imgLabel->setPixmap(aboutImg);
	// to center image:
	auto imgWidget = new QWidget();
	imgWidget->setLayout(new QHBoxLayout());
	imgWidget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	imgWidget->layout()->addWidget(imgLabel);
	imgWidget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));

	dlg.layout()->addWidget(imgWidget);

	auto linkLabel = new QLabel("<a href=\"https://3dct.github.io/open_iA/\">3dct.github.io/open_iA</a>");
	linkLabel->setTextFormat(Qt::RichText);
	linkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
	linkLabel->setOpenExternalLinks(true);
	linkLabel->setAlignment(Qt::AlignRight);
	dlg.layout()->addWidget(linkLabel);

	auto buildInfoLabel = new QLabel("Build information:");
	buildInfoLabel->setIndent(0);
	buildInfoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	dlg.layout()->addWidget(buildInfoLabel);

	const int ExtraRows = 2;
	const int TotalRows = static_cast<int>(buildInfo.count('\n')) + ExtraRows;
	auto table = new QTableWidget(TotalRows, 2, &dlg);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setItem(0, 0, new QTableWidgetItem("Version"));
	table->setItem(0, 1, new QTableWidgetItem(gitVersion));
	table->setItem(1, 0, new QTableWidgetItem("Configuration"));
	table->setItem(1, 1, new QTableWidgetItem(
#ifdef NDEBUG
		"Release"
#else
		"Debug"
#endif
	));
	auto lines = buildInfo.split("\n");
	int row = ExtraRows;
	for (auto line : lines)
	{
		auto tokens = line.split("\t");
		if (tokens.size() != 2)
		{
			continue;
		}
		table->setItem(row, 0, new QTableWidgetItem(tokens[0]));
		table->setItem(row, 1, new QTableWidgetItem(tokens[1]));
		++row;
	}
	table->resizeColumnsToContents();
	table->verticalHeader()->hide();
	table->horizontalHeader()->hide();
	// set fixed table height:
	auto tableHeight = 0;
	for (int r = 0; r < table->rowCount(); ++r)
	{
		tableHeight += table->rowHeight(r);
	}
	// +2 to avoid minor scrolling when clicking on the left/right- up/bottom-most cell in the table:
	tableHeight += 2;
	auto tableWidth = 0;
	for (int c = 0; c < table->columnCount(); ++c)
	{
		tableWidth += table->columnWidth(c);
	}
	auto screenHeightThird = screenHeight / 3;
	if (aboutImg.height() > screenHeightThird)
	{
		imgLabel->setFixedSize(
			screenHeightThird * static_cast<double>(aboutImg.width()) / aboutImg.height(),
			screenHeightThird);
	}

	imgLabel->setScaledContents(true);
	// make sure about dialog isn't higher than roughly 2/3 the screen size:
	tableWidth = std::max(tableWidth, aboutImg.width());
	const int MinTableHeight = 50;
	auto newTableHeight = std::max(MinTableHeight, std::min(tableHeight, screenHeightThird));
	table->setMinimumWidth(tableWidth + 20); // + 20 for approximation of scrollbar width; verticalScrollBar()->height is wildly inaccurate before first show (100 or so)
	table->setMinimumHeight(newTableHeight);
	//table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table->setAlternatingRowColors(true);
	table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	table->horizontalHeader()->setStretchLastSection(true);
	dlg.layout()->addWidget(table);

	auto okBtn = new QPushButton("Ok");
	QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
	dlg.layout()->addWidget(okBtn);

	dlg.exec();
}
