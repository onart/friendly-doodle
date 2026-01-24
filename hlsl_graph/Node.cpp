#include "Node.h"
#include <vector>
#include "imgui.h"
#include "GraphicsPipelineNode.h"

enum SerialTree {
	IN = 0,
	OUT = 1,
};

size_t Node::getBinSize() {
	size_t s = 4 + getBinSizeDetails();
	if (predecessors.size()) {
		s += 8;
		for (auto& pr : predecessors) {
			s += pr->getBinSize();
		}
	}
	return s;
}

bool Node::serialize(stream& s) {
	s.write(type());
	serializeDetails(s);
	if (predecessors.size()) {
		s.write(SerialTree::IN);
		for (auto& pr : predecessors) {
			pr->serialize(s);
			if (s.hadFault()) return false;
		}
		s.write(SerialTree::OUT);
	}
	return !s.hadFault();
}

std::shared_ptr<Node> Node::create(uint32_t t) {
	switch (t)
	{
	case 0: return create<Node>();
	case 1: return create<GraphicsPipelineNode>();
	default:
		break;
	}
	return {};
}

std::shared_ptr<Node> Node::deserialize(stream& s) {
	uint32_t type = s.read<uint32_t>();
	std::shared_ptr<Node> ret = create(type);
	if (!ret) return {};
	ret->deserializeDetails(s);
	std::vector<std::shared_ptr<Node>> stack;
	stack.push_back(ret);
	while (SerialTree io = s.read<SerialTree>()) {
		switch (io)
		{
		case IN:
		{
			if (stack.size() == 0) return {};
			uint32_t type = s.read<uint32_t>();
			auto pred = create(type);
			stack.back()->addPredecessor(pred);
			stack.push_back(pred);
			if (!pred->deserializeDetails(s)) return {};
			break;
		}
		case OUT:
		{
			if (stack.size() == 0) return {};
			stack.pop_back();
			break;
		}
		default:
			return {};
		}
	}
	return ret;
}

void Node::draw() {
	ImGui::PushID(this);
	if (ImGui::TreeNode(name())) {
		drawDetails();
		for (auto& pr : predecessors) {
			pr->draw();
		}

		ImGui::Checkbox("Run per frame", &runPerFrame);
		if (runPerFrame) {
			run();
		}
		else if (ImGui::Button("Run")) {
			run();
		}
		if (auto pred = drawAdd()) {
			addPredecessor(pred);
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

std::shared_ptr<Node> Node::drawAdd(char* nodeNameBuffer, size_t size) {
	const char* NODE_TYPES[] = { "Plain Node", "Graphics Pipeline", };
	if (nodeNameBuffer) {
		ImGui::InputText("New Node Name", nodeNameBuffer, size);
	}
	if (ImGui::BeginCombo(nodeNameBuffer ? "new predecessor type" : "new node type", "select")) {
		for (auto& type : NODE_TYPES) {
			if (ImGui::Selectable(type, false)) {
				ImGui::EndCombo();
				return create(&type - NODE_TYPES);
			}
		}
		ImGui::EndCombo();
	}
	return {};
}