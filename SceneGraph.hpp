#pragma once
#include <memory>
#include <vector>
#include <iostream>
#include <vulkan/vulkan.h>

struct RenderContext {
    VkCommandBuffer cmdBuf;
};

struct Node {
    virtual void traverse(RenderContext& ctx) = 0;
    virtual ~Node() = default;
};

using NodePtr = std::shared_ptr<Node>;

struct Drawable : Node {
    std::string name;
    Drawable(const std::string& n) : name(n) {}
    void traverse(RenderContext& ctx) override {
        std::cout << "Drawing: " << name << "\n";
    }
};
struct Geometry : public Node {
    std::string name;
    Geometry(const std::string& n) : name(n) {}
    void traverse(RenderContext& ctx) override {
        std::cout << "Drawing: " << name << std::endl;
    }
};

struct Group : Node {
    std::string name;
    std::vector<NodePtr> children;
    Group(const std::string& n) : name(n) {}
    void add(const NodePtr& c) { children.push_back(c); }
    void traverse(RenderContext& ctx) override {
        std::cout << "Group: " << name << "\n";
        for (auto& c : children) c->traverse(ctx);
    }
    virtual void traverse() {
        std::cout << "Node traversed\n";
    }
};

class SceneGraph {
public:
    SceneGraph();
    void traverse(RenderContext& ctx);
private:
    NodePtr root;
};
