// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACompTableInteractorStyle.h"

class iACompCombiTable;

class iACompCombiTableInteractionStyle : public iACompTableInteractorStyle
{
public:
	static iACompCombiTableInteractionStyle* New();
	vtkTypeMacro(iACompCombiTableInteractionStyle, iACompTableInteractorStyle);

	//init table visualization
	void setVisualization(iACompCombiTable* visualization);
	virtual void OnMouseWheelForward() override;
	virtual void OnMouseWheelBackward() override;

	virtual void updateCharts() override;
	virtual void updateOtherCharts(QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes) override;

protected:
	iACompCombiTableInteractionStyle();

	virtual iACompTable* getVisualization() override;
	virtual std::map<int, std::vector<double>>* calculatePickedObjects(QList<bin::BinType*>* zoomedRowData) override;

	virtual void resetHistogramTable() override;

private:
	iACompCombiTable* m_visualization;
};
