#ifndef __NODE_H__
#define __NODE_H__

#include <set>
#include <memory>

struct Node {
	template<class Derived>
	static std::shared_ptr<Node> create() {
		static_assert(std::is_base_of_v<Node, Derived>);
		return std::make_shared<Derived>();
	}
	void trigger() {
		for(auto it = predecessors.begin(); it != predecessors.end();) {
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
protected:
	Node() = default;
	std::set<std::shared_ptr<Node>> predecessors;
	bool alive = true;
	virtual void run() {}
};

#endif // !__NODE_H__

