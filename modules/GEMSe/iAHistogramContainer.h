#pragma once

#include <QMap>
#include <QVector>
#include <QSharedPointer>
#include <QSplitter>

class iAAttributes;
class iAChartAttributeMapper;
class iAChartFilter;
class iAChartSpanSlider;
class iAImageClusterNode;

class QGridLayout;
class QSplitter;

class iAHistogramContainer: public QSplitter
{
	Q_OBJECT
public:
	iAHistogramContainer();
	bool ChartExists(int chartID) const;

	void CreateCharts(
		QSharedPointer<iAAttributes> m_chartAttributes,
		iAChartAttributeMapper const & m_chartAttributeMapper,
		iAImageClusterNode* rootNode);

	void UpdateClusterChartData(
		QSharedPointer<iAAttributes> m_chartAttributes,
		iAChartAttributeMapper const & m_chartAttributeMapper,
		QVector<QSharedPointer<iAImageClusterNode> > const & selection);

	void UpdateFilteredChartData(
		QSharedPointer<iAAttributes> m_chartAttributes,
		iAChartAttributeMapper const & m_chartAttributeMapper,
		iAImageClusterNode const * rootNode,
		iAChartFilter const & m_chartFilter);

	void UpdateClusterFilteredChartData(
		QSharedPointer<iAAttributes> m_chartAttributes,
		iAChartAttributeMapper const & m_chartAttributeMapper,
		iAImageClusterNode const * selectedNode,
		iAChartFilter const & m_chartFilter);

	void UpdateAttributeRangeAttitude(
		QSharedPointer<iAAttributes> m_chartAttributes,
		iAChartAttributeMapper const & m_chartAttributeMapper,
		iAImageClusterNode const * root);

	void ResetFilters(QSharedPointer<iAAttributes> m_chartAttributes);
	void ExportAttributeRangeRanking(
		QString const & fileName,
		QSharedPointer<iAAttributes> m_chartAttributes);
	int GetSelectedCount();
	int GetSelectedChartID(int selectionIdx);
	void SetMarker(int chartID, double value);
	void SetSpanValues(int chartID, double min, double max);
signals:
	void ChartSelectionUpdated();
	void ChartDblClicked(int chartID);
	void FilterChanged(int chartID, double min, double max);
private slots:
	void ChartSelected(bool selected);
	void ChartDblClicked();
	void FilterChanged(double min, double max);
private:
	void AddDiagramSubWidgetsWithProperStretch(QSharedPointer<iAAttributes> m_chartAttributes);
	void RemoveAllCharts(QSharedPointer<iAAttributes> m_chartAttributes);
	QWidget * m_paramChartWidget, *m_derivedOutputChartWidget;
	QWidget * m_paramChartContainer, *m_derivedOutputChartContainer;
	QGridLayout* m_paramChartLayout;
	QSplitter* m_chartContainer;
	QMap<int, iAChartSpanSlider*> m_charts;
	QVector< QVector<float> > m_attitudes;
	QVector<int> m_selected;
};