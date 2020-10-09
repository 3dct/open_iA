#pragma once

class iAStackedBarChart;
class iASensitivityInfo;

#include <QWidget>

class iAParameterInfluenceView : public QWidget
{
	Q_OBJECT
public:
	iAParameterInfluenceView(iASensitivityInfo* sensInf);
	void changeMeasure(int newMeasure);
	void changeAggregation(int newAggregation);
	int selectedMeasure() const;
	int selectedAggrType() const;
	int selectedParam() const;
private slots:
	void stackedColSelect();
	void selectMeasure(int measureIdx);
	void paramChangedSlot();
private:
	QVector<int> m_visibleCharacts;
	void updateStackedBars();
	void addStackedBar(int charactIdx);
	void removeStackedBar(int charactIdx);
	//! stacked bar charts (one per parameter)
	QVector<iAStackedBarChart*> m_stackedCharts;
	iAStackedBarChart* m_stackedHeader;
	//QVector<iAChartWidget*> m_chartWidget;
	//! sensitivity information
	iASensitivityInfo* m_sensInf;
	//! measure to use 
	int m_measureIdx;
	//! aggregation type
	int m_aggrType;
	int m_paramIdx;
signals:
	void parameterChanged();
};