//
//  RopedActivity.cpp
//  LabRaven
//
//  Created by Nick Porcino on 8/5/25.
//  Copyright © 2025 Nick Porcino. All rights reserved.
//

#include "RopedActivity.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "Lab/ImguiExt.hpp"
#include "Lab/lab_noodle.h"
#include "Lab/imgui_md/imgui_md.h"
//#include "Lab/imgui_markdown.h"
#include <mutex>

namespace lab {

using std::map;
using std::shared_ptr;
using std::unique_ptr;
using std::string;
using std::vector;

//--------------------------------------------------------------
// Custom markdown renderer for imgui_md testing
//--------------------------------------------------------------

struct roped_markdown : public imgui_md 
{
    // Track style state to ensure balanced push/pop
    bool highlight_style_pushed = false;
    
    ImFont* get_font() const override
    {
        if (m_is_table_header) {
            return ImGui::GetFont(); // Use default bold if available
        }

        switch (m_hlevel)
        {
        case 0:
            return ImGui::GetFont(); // Regular font
        case 1:
            return ImGui::GetFont(); // Header font (could be larger)
        default:
            return ImGui::GetFont();
        }
    }

    void open_url() const override
    {
        // For testing, just print to console
        // In real implementation, would open URL in browser
        printf("Opening URL: %s\n", m_href.c_str());
    }

    bool get_image(image_info& nfo) const override
    {
        // For testing, just return false (no images)
        // In real implementation, would load images based on m_href
        return false;
    }
    
    void html_div(const std::string& dclass, bool e) override
    {
        // Handle custom div classes with proper state tracking
        // Following the pattern from imgui_md README example
        if (dclass == "highlight") {
            if (e) {
                // Starting div - push style
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 200, 255, 255));
                m_table_border = false;
                highlight_style_pushed = true;
            } else {
                // Ending div - pop style
                ImGui::PopStyleColor();
                m_table_border = true; // Re-enable table border
                highlight_style_pushed = false;
            }
        }
    }
};


//--------------------------------------------------------------

struct RopedSetting {
    std::string name;
    std::string short_name;
    lab::noodle::NoodlePin::DataType type;
    float f32_value;
    uint32_t ui32_value;
    std::string str_value;
};

struct RopedPinData
{
    int output_index = 0;
    ln_Node node_id;
    RopedSetting setting;
};

struct RopedNodeData
{
};

struct NodePins
{
    map<string, ln_Pin> input_pin_map;   // name the inputs
    map<string, ln_Pin> output_pin_map;  // name the outputs
    map<string, ln_Pin> param_pin_map;   // name the parameters
    map<string, ln_Pin> setting_pin_map; // name the settings
};

class RopedNoodleProvider final : public lab::noodle::Provider
{
    std::map<ln_Pin,  RopedPinData,     cmp_ln_Pin>  _landruPins;
    std::map<ln_Node, RopedNodeData,    cmp_ln_Node> _landruNodes;
    std::map<ln_Node, NodePins, cmp_ln_Node> _node_reverse_lookups;

public:
    virtual ~RopedNoodleProvider() override = default;

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
    
    // Roped-specific rendering
    std::string renderFromOutputNode();
    std::string getNodeContent(ln_Node node_id);
    void traverseAndConcatenate(ln_Node node_id, std::vector<ln_Node>& visited, std::string& result);
};

// Implementation methods - simplified versions
void RopedNoodleProvider::pin_set_setting_bus_value(const std::string& node_name, const std::string& setting_name, const std::string& path) {}
void RopedNoodleProvider::pin_set_bus_from_file(ln_Pin pin_id, const std::string& path) {}
void RopedNoodleProvider::connect_bus_out_to_bus_in(ln_Node output_node_id, ln_Pin output_pin_id, ln_Node input_node_id) {}
void RopedNoodleProvider::connect_bus_out_to_param_in(ln_Node output_node_id, ln_Pin output_pin_id, ln_Pin param_pin_id) {}
void RopedNoodleProvider::disconnect(ln_Connection connection_id_) {}

ln_Context RopedNoodleProvider::create_runtime_context(ln_Node id) {
    return ln_Context{id.id};
}

void RopedNoodleProvider::node_start_stop(ln_Node node_id, float when) {}
void RopedNoodleProvider::node_bang(ln_Node node_id) {}

ln_Pin RopedNoodleProvider::node_output_named(ln_Node node_id, const std::string& output_name) {
    if (!node_id.valid) return ln_Pin_null();
    return ln_Pin_null(); // Simplified
}

ln_Pin RopedNoodleProvider::node_input_with_index(ln_Node node_id, int output) {
    if (!node_id.valid) return ln_Pin_null();
    return ln_Pin_null(); // Simplified
}

ln_Pin RopedNoodleProvider::node_output_with_index(ln_Node node_id, int output) {
    if (!node_id.valid) return ln_Pin_null();
    return ln_Pin_null(); // Simplified
}

ln_Pin RopedNoodleProvider::node_param_named(ln_Node node_id, const std::string& param_name) {
    if (!node_id.valid) return ln_Pin_null();
    return ln_Pin_null(); // Simplified
}

char const* const* RopedNoodleProvider::node_names() const {
    static const char* names[] = {
        "TextEditor",
        "Comment", 
        "Output",
        nullptr
    };
    return names;
}

ln_Node RopedNoodleProvider::node_create(const std::string& kind, ln_Node id) {
    lab::noodle::NoodleNode* const node = find_node(id);
    if (!node) {
        return ln_Node_null();  // this should never happen
    }

    vector<string> inputs;
    vector<string> outputs;
    vector<RopedSetting> settings;
    
    if (kind == "TextEditor") {
        // TextEditor has BusIn and BusOut for rope connections
        inputs.push_back("input");
        outputs.push_back("output");
        settings.push_back({
            "content",
            "content",
            lab::noodle::NoodlePin::DataType::MultilineString,
            0.f,
            0,
            "# TextEditor Node\n\nEnter your markdown content here...\n"
        });
    }
    else if (kind == "Comment") {
        // Comment has BusIn and BusOut for rope connections
        inputs.push_back("input");
        outputs.push_back("output");
        settings.push_back({
            "content",
            "content", 
            lab::noodle::NoodlePin::DataType::MultilineString,
            0.f,
            0,
            "<!-- Comment Node -->\n\nThis is a comment that will be included in the final output.\n"
        });
    }
    else if (kind == "Output") {
        // Output only has BusIn - it's the end of the chain
        inputs.push_back("input");
        settings.push_back({
            "rendered_content",
            "rendered",
            lab::noodle::NoodlePin::DataType::MultilineString,
            0.f,
            0,
            "<!-- Rendered output will appear here -->"
        });
    }
    else {
        return ln_Node_null();  // couldn't create node
    }

    _landruNodes[id] = RopedNodeData{};
    
    // prep the reverse table
    auto reverse_it = _node_reverse_lookups.find(node->id);
    if (reverse_it == _node_reverse_lookups.end()) {
        _node_reverse_lookups[node->id] = NodePins{};
        reverse_it = _node_reverse_lookups.find(node->id);
    }
    auto& reverse = reverse_it->second;

    //---------- inputs (BusIn pins for rope connections)
    for (auto& name : inputs) {
        ln_Pin pin_id = { create_entity(), true };
        node->pins.push_back(pin_id);
        reverse.input_pin_map[name] = pin_id;
        add_pin(pin_id, lab::noodle::NoodlePin {
            lab::noodle::NoodlePin::Kind::BusIn,
            lab::noodle::NoodlePin::DataType::Bus,
            name,
            "",
            pin_id, node->id,
            "", nullptr
        });
        _landruPins[pin_id] = RopedPinData{ 0, node->id };
    }

    //---------- outputs (BusOut pins for rope connections)
    for (auto& name : outputs) {
        ln_Pin pin_id = { create_entity(), true };
        node->pins.push_back(pin_id);
        reverse.output_pin_map[name] = ln_Pin{ pin_id };
        add_pin(pin_id, lab::noodle::NoodlePin {
            lab::noodle::NoodlePin::Kind::BusOut,
            lab::noodle::NoodlePin::DataType::Bus,
            name,
            "",
            pin_id, node->id,
        });
        _landruPins[pin_id] = RopedPinData{ 0, node->id };
    }

    //---------- settings (MultilineString for content editing)
    for (auto& setting : settings) {
        ln_Pin pin_id = { create_entity(), true };
        node->pins.push_back(pin_id);
        reverse.setting_pin_map[setting.name] = pin_id;
        
        add_pin(pin_id, lab::noodle::NoodlePin {
            lab::noodle::NoodlePin::Kind::Setting,
            setting.type,
            setting.name,
            setting.short_name,
            pin_id, node->id,
            setting.str_value, nullptr
        });
        
        _landruPins[pin_id] = RopedPinData{ 0, node->id, setting };
    }
    
    return id;
}

void RopedNoodleProvider::node_delete(ln_Node node) {
    _landruNodes.erase(node);
    _node_reverse_lookups.erase(node);
}

float RopedNoodleProvider::node_get_timing(ln_Node node) { return 0; }
float RopedNoodleProvider::node_get_self_timing(ln_Node node) { return 0; }

// Pin value methods - simplified
void RopedNoodleProvider::pin_set_param_value(const std::string& node_name, const std::string& param_name, float) {}
void RopedNoodleProvider::pin_set_setting_float_value(const std::string& node_name, const std::string& setting_name, float) {}
void RopedNoodleProvider::pin_set_float_value(ln_Pin pin, float) {}
float RopedNoodleProvider::pin_float_value(ln_Pin pin) { return 0; }
void RopedNoodleProvider::pin_set_setting_int_value(const std::string& node_name, const std::string& setting_name, int) {}
void RopedNoodleProvider::pin_set_int_value(ln_Pin pin, int) {}
int RopedNoodleProvider::pin_int_value(ln_Pin pin) { return 0; }
void RopedNoodleProvider::pin_set_setting_bool_value(const std::string& node_name, const std::string& setting_name, bool) {}
void RopedNoodleProvider::pin_set_bool_value(ln_Pin pin, bool) {}
bool RopedNoodleProvider::pin_bool_value(ln_Pin pin) { return false; }
void RopedNoodleProvider::pin_set_setting_string_value(const std::string& node_name, const std::string& setting_name, const std::string&) {}
void RopedNoodleProvider::pin_set_string_value(ln_Pin pin, const std::string&) {}
std::string RopedNoodleProvider::pin_string_value(ln_Pin pin) { return ""; }
void RopedNoodleProvider::pin_set_enumeration_value(ln_Pin pin, const std::string& value) {}
void RopedNoodleProvider::pin_set_setting_enumeration_value(const std::string& node_name, const std::string& setting_name, const std::string& value) {}
void RopedNoodleProvider::pin_create_output(const std::string& node_name, const std::string& output_name, int channels) {}

// Roped-specific rendering implementation
std::string RopedNoodleProvider::renderFromOutputNode() {
    // Find Output node
    ln_Node output_node = ln_Node_null();
    for (auto& [node_id, node_data] : _landruNodes) {
        // Check if this is an Output node by looking for rendered_content setting
        auto reverse_it = _node_reverse_lookups.find(node_id);
        if (reverse_it != _node_reverse_lookups.end()) {
            auto& reverse = reverse_it->second;
            if (reverse.setting_pin_map.find("rendered_content") != reverse.setting_pin_map.end()) {
                output_node = node_id;
                break;
            }
        }
    }
    
    if (!output_node.valid) {
        return "<!-- No Output node found -->";
    }
    
    std::vector<ln_Node> visited;
    std::string result;
    traverseAndConcatenate(output_node, visited, result);
    
    // Update the Output node's rendered_content setting
    auto reverse_it = _node_reverse_lookups.find(output_node);
    if (reverse_it != _node_reverse_lookups.end()) {
        auto& reverse = reverse_it->second;
        auto setting_it = reverse.setting_pin_map.find("rendered_content");
        if (setting_it != reverse.setting_pin_map.end()) {
            auto pin_it = _landruPins.find(setting_it->second);
            if (pin_it != _landruPins.end()) {
                pin_it->second.setting.str_value = result;
            }
        }
    }
    
    return result;
}

std::string RopedNoodleProvider::getNodeContent(ln_Node node_id) {
    auto reverse_it = _node_reverse_lookups.find(node_id);
    if (reverse_it == _node_reverse_lookups.end()) {
        return "";
    }
    
    auto& reverse = reverse_it->second;
    auto setting_it = reverse.setting_pin_map.find("content");
    if (setting_it == reverse.setting_pin_map.end()) {
        return "";
    }
    
    auto pin_it = _landruPins.find(setting_it->second);
    if (pin_it == _landruPins.end()) {
        return "";
    }
    
    return pin_it->second.setting.str_value;
}

void RopedNoodleProvider::traverseAndConcatenate(ln_Node node_id, std::vector<ln_Node>& visited, std::string& result) {
    // Avoid cycles
    for (auto& v : visited) {
        if (v.id == node_id.id) {
            return;
        }
    }
    visited.push_back(node_id);
    
    auto reverse_it = _node_reverse_lookups.find(node_id);
    if (reverse_it == _node_reverse_lookups.end()) {
        return;
    }
    
    auto& reverse = reverse_it->second;
    
    // Find all input connections and traverse them first (depth-first)
    auto input_it = reverse.input_pin_map.find("input");
    if (input_it != reverse.input_pin_map.end()) {
        ln_Pin input_pin = input_it->second;
        
        // Look for connections to this input pin
        // This is simplified - in real implementation, would traverse the connection graph
        // For now, we'll just get the content of the current node
    }
    
    // After recursion, concatenate this node's content
    std::string content = getNodeContent(node_id);
    if (!content.empty()) {
        if (!result.empty()) {
            result += "\n\n";
        }
        result += content;
    }
}

struct RopedActivity::data {
    std::once_flag must_init;
    std::unique_ptr<lab::noodle::ProviderHarness> noodleHarness;
    std::unique_ptr<RopedNoodleProvider> noodleProvider;
};

RopedActivity::RopedActivity()
: Activity(RopedActivity::sname())
, _self(new RopedActivity::data) {
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<RopedActivity*>(instance)->RunUI(*vi);
    };
}

RopedActivity::~RopedActivity() {
    delete _self;
}

void RopedActivity::_activate() {
    // Activity lifecycle - activated
}

void RopedActivity::_deactivate() {
    // Activity lifecycle - deactivated  
}

void RopedActivity::RunUI(const LabViewInteraction&) {
    std::call_once(_self->must_init, [this]() {
        _self->noodleProvider = std::unique_ptr<RopedNoodleProvider>(new RopedNoodleProvider());
        _self->noodleHarness = std::unique_ptr<lab::noodle::ProviderHarness>(
            new lab::noodle::ProviderHarness(*_self->noodleProvider));
    });
    
    ImGui::Begin("Node Graph##NodeGraph");
    _self->noodleHarness->run();
    ImGui::End();
    
    // Test imgui_md with sample markdown content + Render functionality
    ImGui::Begin("Markdown Test##MarkdownTest");
    
    static roped_markdown s_markdown_renderer;
    static std::string rendered_output = "";
    
    // Add Render button
    if (ImGui::Button("Render")) {
        if (_self->noodleProvider) {
            rendered_output = _self->noodleProvider->renderFromOutputNode();
        } else {
            rendered_output = "<!-- No provider available -->";
        }
    }
    
    ImGui::Separator();
    
    if (!rendered_output.empty()) {
        ImGui::Text("Rendered Output:");
        ImGui::Separator();
        s_markdown_renderer.print(rendered_output.c_str(), rendered_output.c_str() + rendered_output.size());
    } else {
        // Show sample markdown content for testing
        const char* sample_markdown = R"(# Roped Activity Test

This is a **test** of the `imgui_md` integration for the Roped spatial code editor.

## Features Being Tested

1. **Headers** - Different levels
2. **Emphasis** - *italic* and **bold** text  
3. **Code** - Inline `code` and blocks
4. **Lists** - Numbered and bulleted
   - Sub-item one
   - Sub-item two
5. **Links** - [GitHub](https://github.com) (won't open in test)

## Code Block Example

```cpp
class TextNode : public RopedNode {
    std::string content;
    ImTextureId preview_texture;
    bool texture_dirty = true;
    
    void update_preview() {
        // Render markdown to texture
        s_markdown_renderer.print(content.c_str(), 
                                 content.c_str() + content.size());
    }
};
```

## Table Test

| Feature | Status | Notes |
|---------|--------|-------|
| Headers | ✓ | Working |
| **Bold** | ✓ | Working |  
| *Italic* | ✓ | Working |
| `Code` | ✓ | Working |

---

<div class="highlight">
This text should be highlighted in blue!
</div>

> This would be a blockquote if supported

**Success!** If you can see formatted text above, imgui_md integration is working correctly.

Click the **Render** button above to test the rope traversal system.
)";

        s_markdown_renderer.print(sample_markdown, sample_markdown + strlen(sample_markdown));
    }
    
    ImGui::End();
}

} // namespace lab
