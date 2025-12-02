#include "NNNode.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void NNNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_num"), &NNNode::get_num);
    ClassDB::bind_method(D_METHOD("set_num", "p_num"), &NNNode::set_num);
    ClassDB::bind_method(D_METHOD("add_one"), &NNNode::add_one);
    ClassDB::add_property("NNNode", PropertyInfo(Variant::FLOAT, "num"), "set_num", "get_num");
}

NNNode::NNNode() {
    NNNode::set_num(1.0);
    UtilityFunctions::print("num value: ", num);
}

NNNode::~NNNode() {
    
}

void NNNode::_process(double delta) {
    if (!Engine::get_singleton()->is_editor_hint()) {
        NNNode::set_num(num + delta);
        UtilityFunctions::print("num value: ", num);
    }
}

void NNNode::set_num(const double p_num) {
    num = p_num;
}

double NNNode::get_num() const {
    return num;
}

void NNNode::add_one() {
    num += 1.0;
}
