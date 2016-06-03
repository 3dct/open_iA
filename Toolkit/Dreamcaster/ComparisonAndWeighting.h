#ifndef COMPARISONANDWEIGHTING_H
#define COMPARISONANDWEIGHTING_H

class QWidget;
class QPixmap;
class PaintWidget;
/**	\class ParamWidget.
\brief Class representing widget that shows height map of one parameter.

Contains: PaintWidget, pixmap that it uses, memory buffer used to fill pixmap and dimensions of that buffer.	
*/
struct ParamWidget
{
public:
	//methods
	ParamWidget();
	void Init(int pxmpWidth, int pxmpHeight, QWidget *widget);
	int AllocateBuffer(int width, int height);
	~ParamWidget();
	//props
	QPixmap *pxmp;
	PaintWidget *paintWidget;
	unsigned int * buffer;
	int bufferWidth, bufferHeight;
protected:
	bool initialized;
};
/**	\struct ParametersView.
\brief struct that uses all parameters.

Contains: ParamWidget for every parameter.
*/
struct ParametersView
{
	//meth
	ParametersView(int width, int height, QWidget *w1, QWidget *w2, QWidget *w3);
	virtual void Update();
	//props
	ParamWidget paramWidgets[3];
};
/**	\struct WeightingView.
\brief struct that uses all parameters and combination of all parameters.

Contains: ParamWidget for every parameter and one for combination results
*/
struct CombinedParametersView
{
	//meth
	CombinedParametersView(QWidget *resultsWidget, int width, int height);
	virtual void Update();
	//props
	ParamWidget results;
};
#endif//COMPARISONANDWEIGHTING_H
