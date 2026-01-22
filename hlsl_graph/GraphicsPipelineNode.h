#ifndef __GRAPHICS_PIPELINE_NODE_H__
#define __GRAPHICS_PIPELINE_NODE_H__

#include "Node.h"

class GraphicsPipelineNode : public Node {
public:
	GraphicsPipelineNode();
	virtual ~GraphicsPipelineNode();
protected:
	virtual void run() override;
	virtual bool serializeDetails(stream& s) override;
	virtual void drawDetails() override;
private:
	void* _pipeline = nullptr;
};

#endif // !__GRAPHICS_PIPELINE_NODE_H__

