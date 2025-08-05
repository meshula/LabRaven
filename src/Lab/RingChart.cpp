// ImGuiRingChartExample.cpp

#include "RingChart.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <cmath>
// Helper to convert screen ImVec2 to local Vec2
inline Vec2 ToVec2(const ImVec2& v) { return {v.x, v.y}; }

inline Vec2 Sub(const Vec2& a, const Vec2& b) { return {a.x - b.x, a.y - b.y}; }

void DrawRingChartInteractive(RingChart& chart, ChartMode mode, float morphT, float scale) {
    chart.SetMorph(morphT);

    ImGui::Text("Ring Chart (%s mode)", mode == ChartMode::Ring ? "Ring" : "Flame");

    // Get draw list and cursor position
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    const auto& slices = chart.GetRings();
    Vec2 mouseLocal = Sub(ToVec2(ImGui::GetIO().MousePos), ToVec2(origin));

    RingSlice* hovered = chart.HitTest(mouseLocal, mode);

    // Draw each slice
    for (const auto& slice : slices) {
        for (const auto& e : slice.elements) {
            Vec2 quad[4];
            e.GetInterpolatedQuad(quad);
            ImVec2 points[4];
            for (int i = 0; i < 4; ++i)
                points[i] = ImVec2(origin.x + quad[i].x * scale, origin.y + quad[i].y * scale);
            
            ImU32 color = e.color;
            if (&e == hovered)
                color = (color & 0xffffff00) | 0x88; // fade alpha for hover highlight
            
            drawList->AddConvexPolyFilled(points, 4, color);
        }
    }

    // Optionally display debug text for hovered slice
    if (hovered) {
        ImGui::Text("Hovered slice a0=%.2f a1=%.2f", hovered->a0, hovered->a1);
    }

    // Add dummy space to avoid overlapping UI
    ImGui::Dummy(ImVec2(300, 300));
}

void ShowRingChartWindow() {
    static RingChart chart;
    static bool initialized = false;
    if (!initialized) {
        chart.BuildFromData({{1, 2, 3}, {2, 1}}, 30.0f, 20.0f);
        initialized = true;
    }

    ImGui::Begin("Ring Chart");
    ImVec2 center = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin();
    center.x += ImGui::GetContentRegionAvail().x * 0.5f;
    center.y += ImGui::GetContentRegionAvail().y * 0.5f;
    ImGui::SetCursorScreenPos(center);
    DrawRingChartInteractive(chart, ChartMode::Ring, 0.f, 1.0f);
    ImGui::End();
}

#if 0
// Qt version
// QtRingChartWidget.hpp

#include "RingChart.hpp"
#include <QWidget>
#include <QColor>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>

class RingChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit RingChartWidget(QWidget* parent = nullptr);

    void setChartData(const std::vector<std::vector<float>>& hierarchy);
    void setMorphT(float t);

signals:
    void sliceClicked(RingSlice* slice);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    RingChart chart;
    ChartMode currentMode = ChartMode::Ring;
    float morphT = 0.0f;
    float ringThickness = 30.0f;
};


RingChartWidget::RingChartWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(400, 400);
}

void RingChartWidget::setChartData(const std::vector<std::vector<float>>& hierarchy) {
    Vec2 center = { width() / 2.0f, height() / 2.0f };
    chart.Build(hierarchy, 40.0f, ringThickness, 0.0f, 2.0f * 3.14159265f, center);
    update();
}

void RingChartWidget::setMorphT(float t) {
    morphT = std::clamp(t, 0.0f, 1.0f);
    chart.SetMorph(morphT);
    update();
}

void RingChartWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    for (const RingSlice& slice : chart.GetSlices()) {
        Vec2 quad[4];
        slice.GetInterpolatedQuad(quad);

        QPolygonF polygon;
        for (int i = 0; i < 4; ++i)
            polygon << QPointF(quad[i].x, quad[i].y);

        QColor color = QColor::fromRgba(slice.color);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(polygon);
    }
}

void RingChartWidget::mousePressEvent(QMouseEvent* event) {
    Vec2 p = { static_cast<float>(event->pos().x()), static_cast<float>(event->pos().y()) };
    RingSlice* hit = chart.HitTest(p, currentMode);
    if (hit) {
        emit sliceClicked(hit);
    }
}
#endif
