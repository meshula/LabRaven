//
//  NodeGraphActivity.cpp
//  LabRaven
//
//  Created by Nick Porcino on 8/5/25.
//  Copyright © 2025 Nick Porcino. All rights reserved.
//

#include "NodeGraphActivity.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "Lab/lab_imgui_ext.hpp"
#include "Lab/lab_noodle.h"
#include <mutex>

namespace lab {

using std::map;
using std::shared_ptr;
using std::unique_ptr;
using std::string;
using std::vector;

//--------------------------------------------------------------

struct LandruSetting {
    std::string name;
    std::string short_name;
    lab::noodle::NoodlePin::DataType type;
    float f32_value;
    uint32_t ui32_value;
    std::string str_value;
};

struct LandruPinData
{
    int output_index = 0;
    ln_Node node_id;
    LandruSetting setting;
};

struct LandruNodeData
{
};

struct NodePins
{
    map<string, ln_Pin> input_pin_map;   // name the inputs
    map<string, ln_Pin> output_pin_map;  // name the outputs
    map<string, ln_Pin> param_pin_map;   // name the parameters
    map<string, ln_Pin> setting_pin_map; // name the settings
};

class LandruNoodleProvider final : public lab::noodle::Provider
{
    std::map<ln_Pin,  LandruPinData,     cmp_ln_Pin>  _landruPins;
    std::map<ln_Node, LandruNodeData,    cmp_ln_Node> _landruNodes;
    std::map<ln_Node, NodePins, cmp_ln_Node> _node_reverse_lookups;

public:
    virtual ~LandruNoodleProvider() override = default;

    virtual ln_Context create_runtime_context(ln_Node id) override;

    // node creation and deletion
    virtual char const* const* node_names() const override;
    virtual ln_Node node_create(const std::string& name, ln_Node id) override;
    virtual void node_delete(ln_Node node) override;
    virtual bool node_rename(ln_Node node, const std::string& name) override { return true; }

    // node access
    virtual float node_get_timing(ln_Node node) override;      // in seconds
    virtual float node_get_self_timing(ln_Node node) override; // in seconds
    virtual void  node_start_stop(ln_Node node, float when) override;
    virtual void  node_bang(ln_Node node) override;

    virtual ln_Pin node_input_with_index(ln_Node node, int output) override;
    virtual ln_Pin node_output_named(ln_Node node, const std::string& output_name) override;
    virtual ln_Pin node_output_with_index(ln_Node node, int output) override;
    virtual ln_Pin node_param_named(ln_Node node, const std::string& output_name) override;

    // pins
    virtual void  pin_set_param_value(const std::string& node_name, const std::string& param_name, float) override;
    virtual void  pin_set_setting_float_value(const std::string& node_name, const std::string& setting_name, float) override;
    virtual void  pin_set_float_value(ln_Pin pin, float) override;
    virtual float pin_float_value(ln_Pin pin) override;
    virtual void  pin_set_setting_int_value(const std::string& node_name, const std::string& setting_name, int) override;
    virtual void  pin_set_int_value(ln_Pin pin, int) override;
    virtual int   pin_int_value(ln_Pin pin) override;
    virtual void  pin_set_setting_bool_value(const std::string& node_name, const std::string& setting_name, bool) override;
    virtual void  pin_set_bool_value(ln_Pin pin, bool) override;
    virtual bool  pin_bool_value(ln_Pin pin) override;
    virtual void  pin_set_setting_string_value(const std::string& node_name, const std::string& setting_name, const std::string&) override;
    virtual void  pin_set_string_value(ln_Pin pin, const std::string&) override;
    virtual std::string  pin_string_value(ln_Pin pin) override;
    virtual void  pin_set_setting_bus_value(const std::string& node_name, const std::string& setting_name, const std::string& path) override;
    virtual void  pin_set_bus_from_file(ln_Pin pin, const std::string& path) override;
    virtual void  pin_set_enumeration_value(ln_Pin pin, const std::string& value) override;
    virtual void  pin_set_setting_enumeration_value(const std::string& node_name, const std::string& setting_name, const std::string& value) override;

    // string based interfaces
    virtual void pin_create_output(const std::string& node_name, const std::string& output_name, int channels) override;

    // connections
    virtual void connect_bus_out_to_bus_in(ln_Node node_out_id, ln_Pin output_pin_id, ln_Node node_in_id) override;
    virtual void connect_bus_out_to_param_in(ln_Node output_node_id, ln_Pin output_pin_id, ln_Pin pin_id) override;
    virtual void disconnect(ln_Connection connection_id) override;
};

// Implementation methods - simplified versions
void LandruNoodleProvider::pin_set_setting_bus_value(const std::string& node_name, const std::string& setting_name, const std::string& path) {}
void LandruNoodleProvider::pin_set_bus_from_file(ln_Pin pin_id, const std::string& path) {}
void LandruNoodleProvider::connect_bus_out_to_bus_in(ln_Node output_node_id, ln_Pin output_pin_id, ln_Node input_node_id) {}
void LandruNoodleProvider::connect_bus_out_to_param_in(ln_Node output_node_id, ln_Pin output_pin_id, ln_Pin param_pin_id) {}
void LandruNoodleProvider::disconnect(ln_Connection connection_id_) {}

ln_Context LandruNoodleProvider::create_runtime_context(ln_Node id) {
    return ln_Context{id.id};
}

void LandruNoodleProvider::node_start_stop(ln_Node node_id, float when) {}
void LandruNoodleProvider::node_bang(ln_Node node_id) {}

ln_Pin LandruNoodleProvider::node_output_named(ln_Node node_id, const std::string& output_name) {
    if (!node_id.valid) return ln_Pin_null();
    return ln_Pin_null(); // Simplified
}

ln_Pin LandruNoodleProvider::node_input_with_index(ln_Node node_id, int output) {
    if (!node_id.valid) return ln_Pin_null();
    return ln_Pin_null(); // Simplified
}

ln_Pin LandruNoodleProvider::node_output_with_index(ln_Node node_id, int output) {
    if (!node_id.valid) return ln_Pin_null();
    return ln_Pin_null(); // Simplified
}

ln_Pin LandruNoodleProvider::node_param_named(ln_Node node_id, const std::string& param_name) {
    if (!node_id.valid) return ln_Pin_null();
    return ln_Pin_null(); // Simplified
}

char const* const* LandruNoodleProvider::node_names() const {
    static const char* names[] = {
        "TextEditor",
        "Comment", 
        "Output",
        nullptr
    };
    return names;
}

ln_Node LandruNoodleProvider::node_create(const std::string& name, ln_Node id) {
    _landruNodes[id] = LandruNodeData{};
    _node_reverse_lookups[id] = NodePins{};
    return id;
}

void LandruNoodleProvider::node_delete(ln_Node node) {
    _landruNodes.erase(node);
    _node_reverse_lookups.erase(node);
}

float LandruNoodleProvider::node_get_timing(ln_Node node) { return 0; }
float LandruNoodleProvider::node_get_self_timing(ln_Node node) { return 0; }

// Pin value methods - simplified
void LandruNoodleProvider::pin_set_param_value(const std::string& node_name, const std::string& param_name, float) {}
void LandruNoodleProvider::pin_set_setting_float_value(const std::string& node_name, const std::string& setting_name, float) {}
void LandruNoodleProvider::pin_set_float_value(ln_Pin pin, float) {}
float LandruNoodleProvider::pin_float_value(ln_Pin pin) { return 0; }
void LandruNoodleProvider::pin_set_setting_int_value(const std::string& node_name, const std::string& setting_name, int) {}
void LandruNoodleProvider::pin_set_int_value(ln_Pin pin, int) {}
int LandruNoodleProvider::pin_int_value(ln_Pin pin) { return 0; }
void LandruNoodleProvider::pin_set_setting_bool_value(const std::string& node_name, const std::string& setting_name, bool) {}
void LandruNoodleProvider::pin_set_bool_value(ln_Pin pin, bool) {}
bool LandruNoodleProvider::pin_bool_value(ln_Pin pin) { return false; }
void LandruNoodleProvider::pin_set_setting_string_value(const std::string& node_name, const std::string& setting_name, const std::string&) {}
void LandruNoodleProvider::pin_set_string_value(ln_Pin pin, const std::string&) {}
std::string LandruNoodleProvider::pin_string_value(ln_Pin pin) { return ""; }
void LandruNoodleProvider::pin_set_enumeration_value(ln_Pin pin, const std::string& value) {}
void LandruNoodleProvider::pin_set_setting_enumeration_value(const std::string& node_name, const std::string& setting_name, const std::string& value) {}
void LandruNoodleProvider::pin_create_output(const std::string& node_name, const std::string& output_name, int channels) {}

struct NodeGraphActivity::data {
    std::once_flag must_init;
    std::unique_ptr<lab::noodle::ProviderHarness> noodleHarness;
    std::unique_ptr<LandruNoodleProvider> noodleProvider;
};

NodeGraphActivity::NodeGraphActivity()
: Activity(NodeGraphActivity::sname())
, _self(new NodeGraphActivity::data) {
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<NodeGraphActivity*>(instance)->RunUI(*vi);
    };
}

NodeGraphActivity::~NodeGraphActivity() {
    delete _self;
}

void NodeGraphActivity::_activate() {
    // Activity lifecycle - activated
}

void NodeGraphActivity::_deactivate() {
    // Activity lifecycle - deactivated  
}

void NodeGraphActivity::RunUI(const LabViewInteraction&) {
    std::call_once(_self->must_init, [this]() {
        _self->noodleProvider = std::unique_ptr<LandruNoodleProvider>(new LandruNoodleProvider());
        _self->noodleHarness = std::unique_ptr<lab::noodle::ProviderHarness>(
            new lab::noodle::ProviderHarness(*_self->noodleProvider));
    });
    
    ImGui::Begin("Node Graph##NodeGraph");
    _self->noodleHarness->run();
    ImGui::End();
}

} // namespace lab
