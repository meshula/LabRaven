# Roped Activity: Spatial Code Editor using lab_noodle

## Vision
Implement the Roped spatial code editor concept using the mature lab_noodle framework. Lab_noodle already provides complete node graph infrastructure - we need to extend it with a specialized text node type and integrate with LabDb9 for persistence and graph analysis.

## Current Architecture Status

### ✅ Completed Core Infrastructure
- **Three-panel layout**: Already implemented in RopedActivity
- **lab_noodle integration**: Full node graph system operational
- **Scaling system**: Canvas scaling with ImDrawList rendering works
- **Focus system**: Node selection and editing framework in place
- **LabDb9 integration**: Database persistence and graph analysis ready

### 🎯 Current Challenge: Scalable Long-Form Text Rendering

**The Problem**: Need rich markdown text rendering that scales with the lab_noodle canvas while maintaining readability and performance.

**Technical Constraint**: ImGui widgets don't scale, but lab_noodle's canvas does. We need custom text rendering that integrates with the existing `node.render()` callback system.

## Implementation Strategy: Custom Markdown Node

### Why Custom Implementation over imgui_md Adaptation

**imgui_md Architecture Assessment**:
- Deeply coupled to ImGui's layout system (`ImGui::TextUnformatted`, `ImGui::NewLine`)
- Uses ImGui cursor positioning for complex elements (tables, lists)
- State-driven rendering with ImGui font/color stacks
- Image handling requires ImGui content region calculations

**lab_noodle Text Rendering Pattern**:
- Direct `ImDrawList->AddText()` calls with coordinate positioning
- Scaling applied: `font_size * root.canvas.scale`
- Custom render callbacks: `node.second.render.render(id, ul_ws, lr_ws, scale, drawList)`
- No ImGui layout dependency

**Technical Decision**: Build custom markdown renderer using lab_noodle's established text rendering patterns rather than adapting imgui_md's ImGui-centric architecture.

### Phase 1: MarkdownTextNode Foundation (Week 1)

```cpp
class MarkdownTextNode : public RopedNode {
    std::string markdown_content;
    std::vector<RenderedLine> rendered_lines;  // Pre-computed layout
    bool layout_dirty = true;
    
    struct RenderedLine {
        std::string text;
        float x_offset, y_offset;
        float font_size;
        ImU32 color;
        FontStyle style;  // normal, bold, italic
    };
    
    void update_layout() {
        if (!layout_dirty) return;
        // Parse markdown and compute line positions
        // Store in rendered_lines for efficient scaling
        layout_dirty = false;
    }
    
    void render(ln_Node id, vec2 ul_ws, vec2 lr_ws, float scale, ImDrawList* draw_list) override {
        update_layout();
        
        for (auto& line : rendered_lines) {
            ImVec2 pos = ImVec2(ul_ws.x + line.x_offset * scale, 
                               ul_ws.y + line.y_offset * scale);
            float scaled_font_size = line.font_size * scale;
            
            draw_list->AddText(get_font(line.style), scaled_font_size, pos, 
                              line.color, line.text.c_str(), 
                              line.text.c_str() + line.text.size());
        }
    }
};
```

### Phase 2: Markdown Parser Subset (Week 1-2)

**Initial Feature Set** (focusing on common documentation needs):
- **Headers**: `# ## ###` with size scaling
- **Emphasis**: `*italic*` and `**bold**` text
- **Code spans**: `` `inline code` ``
- **Code blocks**: ` ``` ` with syntax highlighting
- **Lists**: Basic `- item` and `1. item`
- **Paragraphs**: Automatic line wrapping

**Deferred Features** (Phase 4+):
- Tables (complex layout)
- Images (require texture management)
- Links (need interaction handling)
- Complex nesting

### Phase 3: Three-Panel Integration (Week 2)

**Focus System Enhancement**:
```cpp
class RopedActivity {
    ln_Node focused_markdown_node = ln_Node_null();
    std::unique_ptr<ImGuiColorTextEdit> markdown_editor;
    
    void focus_markdown_node(ln_Node node) {
        auto* text_node = get_markdown_node(node);
        if (text_node) {
            focused_markdown_node = node;
            markdown_editor->SetText(text_node->markdown_content);
            // Gray out the node preview
            text_node->set_focus_mode(true);
        }
    }
    
    void sync_editor_to_node() {
        if (focused_markdown_node.valid) {
            auto* text_node = get_markdown_node(focused_markdown_node);
            text_node->markdown_content = markdown_editor->GetText();
            text_node->invalidate_layout();
        }
    }
};
```

**Panel Layout** (already implemented):
```
┌─────────────┬──────────────────┬─────────────────┐
│             │                  │                 │
│  Node Graph │   Focused Node   │   Inspector     │
│   (Scaled)  │    (1:1 Scale)   │   (Properties)  │
│             │                  │                 │
│  [Node1]    │  # My Document   │  Name: doc.md   │
│   ┌─────┐   │                  │  Lines: 247     │
│   │# My │   │  This is a       │  Language: MD   │
│   │Doc  │   │  **markdown**    │                 │
│   │...  │   │  document with   │  Connections:   │
│   └─────┘   │  `code` and      │  ──────────────  │
│     ↓       │  - lists         │  Input: none    │
│  [Node2]    │  - more items    │  Output: text   │
│   ┌─────┐◄──┤                  │                 │
│   │proc │   │  ```python       │  Actions:       │
│   │...  │   │  print("hello")  │  ──────────────  │
│   └─────┘   │  ```             │  [Export HTML]  │
│             │                  │  [Export MD]    │
└─────────────┴──────────────────┴─────────────────┘
  Canvas View    Focused Editor      Inspector
  (scaled)       (1:1 scale)        (1:1 scale)
```

### Phase 4: Performance Optimization (Week 3)

**Layout Caching Strategy**:
- Pre-compute text positions at reference scale
- Cache glyph metrics and line breaks
- Invalidate only on content change, not scale change
- Target: 100+ nodes with smooth 10%-200% scaling

**Memory Management**:
- Lazy layout computation (only when visible)
- Preview truncation for very long documents
- Efficient string handling with move semantics

### Phase 5: Advanced Features (Week 4)

**Enhanced Markdown Support**:
- Nested lists with proper indentation
- Block quotes with visual indicators
- Horizontal rules
- Better code syntax highlighting

**Interaction Features**:
- Click-to-edit inline
- Drag-and-drop text between nodes
- Search within markdown content
- Folding/expansion of sections

### Phase 6: LabDb9 Deep Integration (Week 5)

**Graph Analysis**:
- Store document structure as triples
- Cross-reference detection between nodes
- Semantic relationships between text blocks
- Query system for content discovery

**Version Control**:
- Content history tracking
- Diff visualization
- Branch/merge support for collaborative editing

## Technical Implementation Details

### Custom Markdown Parser

**Design Principles**:
1. **Single-pass parsing** for performance
2. **Incremental updates** - only reparse changed sections
3. **Scale-independent layout** - positions in logical units
4. **Memory efficient** - minimal allocations during render

```cpp
class MarkdownParser {
    struct Token {
        enum Type { Text, Header, Emphasis, Code, ListItem, LineBreak };
        Type type;
        std::string content;
        int level;  // for headers, list depth
        FontStyle style;
    };
    
    std::vector<Token> tokenize(const std::string& markdown);
    std::vector<RenderedLine> layout(const std::vector<Token>& tokens, float width);
};
```

### Font Management

**Font Stack Integration**:
- Leverage ImGui's font system
- Support for regular, bold, italic variants
- Code font for monospace sections
- Size scaling for headers

### Color Scheme

**Syntax Highlighting**:
- Headers: Scaled font size + accent color
- Code: Monospace font + muted background
- Emphasis: Font style changes
- Lists: Bullet/number styling

## Success Metrics

- [ ] **Performance**: 100+ markdown nodes with smooth scaling
- [ ] **Usability**: Sub-100ms focus switching
- [ ] **Functionality**: Core markdown features render correctly
- [ ] **Integration**: Seamless three-panel workflow
- [ ] **Memory**: <2KB per text node for layout cache
- [ ] **Scalability**: Readable text from 25% to 200% scale

## Migration Path

**From Current State**:
1. Create MarkdownTextNode class
2. Implement basic text rendering
3. Add to node creation menu
4. Test with simple markdown content
5. Iterate on parser and layout
6. Enhance focus system integration

**Compatibility**:
- Existing lab_noodle nodes continue to work
- RopedActivity three-panel layout unchanged
- LabDb9 integration builds on current foundation

---

*"Custom implementation leverages lab_noodle's architecture rather than fighting against it - resulting in better performance and maintainability than adapting imgui_md."*
