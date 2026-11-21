#pragma once

#include "../layout_manager.h"

// Painted primitives and controls built from XML subtrees.
//
// Paint-only elements (painted by LayoutManager::paintChildren, in order):
//   Rectangle, Ellipse, Line, AutoLabel  (existing)
//   Group         x y: offset of its children; scale: zoom of its children
//                 (about the group origin); rotation (deg, clockwise) about
//                 pivotX pivotY (child units); rotateWithValue="true" adds the
//                 owning control's value angle; opacity applies to the group.
//   Ticks         radial tick marks around cx cy: count, startAngle, endAngle
//                 (deg, 0 = up, clockwise), outerRadius, length, majorEvery,
//                 majorLength, strokeColour, strokeWeight.
//   ConicStripes  alternating wedges filling the ellipse x y width height:
//                 segments, colourA, colourB, startAngle.
// Any painted element may carry state="on" (or "!on", or "on|over"): it is
// painted only when the paint context's states match.
//
// Line also takes dashPattern="6,4" (layout units).
// Rectangle and Ellipse also take fill-gradient-angle (CSS linear-gradient
// angle in degrees) in place of fill-gradient-start / -end.

class LayoutManager;

// A rotary slider drawn entirely from its element's children (LayeredKnob).
// Attributes: startAngle endAngle (deg, 0 = up, clockwise), dragMode
// ("circular" | "vertical"), dragSensitivity (pixels for the full range),
// hitRadius pivotX pivotY (accepts clicks within hitRadius of the pivot).
class LMLayeredKnob : public juce::Slider
{
public:
    LMLayeredKnob();
    void setStyle (LayoutManager&, const juce::ValueTree&);
    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    float getValueAngle(); // radians, clockwise from up

private:
    LayoutManager* layout = nullptr;
    juce::ValueTree node;
    juce::Point<float> pivot;
    float hitRadius = 0.0f;
};

// A button drawn from its element's children (StateButton). Children with
// state="on" / "off" follow the toggle state; "over", "down", "disabled"
// follow the mouse and enablement. toggles="false" makes it momentary;
// interactive="false" makes it a display only (an LED).
class LMStateButton : public juce::Button
{
public:
    LMStateButton();
    void setStyle (LayoutManager&, const juce::ValueTree&);
    void paintButton (juce::Graphics&, bool isOver, bool isDown) override;

private:
    LayoutManager* layout = nullptr;
    juce::ValueTree node;
};

// Development hot reload: polls the layout file and reloads it on change,
// then calls onReload (re-apply bounds, repaint, re-read custom styles).
class LMLayoutFileWatcher : private juce::Timer
{
public:
    LMLayoutFileWatcher (LayoutManager&, const juce::File&, std::function<void()> onReload, int intervalMs = 400);
    ~LMLayoutFileWatcher() override;

private:
    void timerCallback() override;
    LayoutManager& layout;
    juce::File file;
    juce::Time lastModified;
    std::function<void()> onReload;
};
