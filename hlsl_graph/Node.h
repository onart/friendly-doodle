#ifndef __NODE_H__
#define __NODE_H__

#include <set>
#include <memory>

#include "stream.hpp"

struct Node {
	template<class Derived>
	static std::shared_ptr<Node> create() {
		static_assert(std::is_base_of_v<Node, Derived>);
		return std::shared_ptr<Derived>(new Derived);
	}
	void trigger() {
		for (auto it = predecessors.begin(); it != predecessors.end();) {
			auto& node = *it;
			if (node->alive) {
				node->trigger();
				++it;
			}
			else {
				it = predecessors.erase(it);
			}
		}
		run();
	}
	void addPredecessor(const std::shared_ptr<Node>& node) {
		predecessors.insert(node);
	}
	void removePredecessor(const std::shared_ptr<Node>& node) {
		predecessors.erase(node);
	}
	virtual ~Node() = default;
	size_t getBinSize();
	bool serialize(stream& s);
	static std::shared_ptr<Node> deserialize(stream& s);
protected:
	Node() = default;
	std::set<std::shared_ptr<Node>> predecessors;
	bool alive = true;
	static std::shared_ptr<Node> create(uint32_t type);
	virtual uint32_t type() { return 0; }
	virtual void run() {}
	virtual bool serializeDetails(stream& s) { return !s.hadFault(); }
	virtual bool deserializeDetails(stream& s) { return !s.hadFault(); }
	virtual size_t getBinSizeDetails() { return 0; }
};

#endif // !__NODE_H__

