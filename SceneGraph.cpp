#include "SceneGraph.hpp"

SceneGraph::SceneGraph() {
    auto group = std::make_shared<Group>("root");
    group->add(std::make_shared<Drawable>("cube"));
    root = group;
}

void SceneGraph::traverse(RenderContext& ctx) {
    if (root) root->traverse(ctx);
}
