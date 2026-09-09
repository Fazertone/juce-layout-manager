#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur/melatonin_blur.h>
#include <vector>
#include <memory>

// Dimensions are unscaled layout units. Colour alpha and opacity multiply.
struct LMShadowStyle
{
    float x = 0.0f, y = 0.0f, blur = 0.0f, spread = 0.0f;
    juce::Colour colour = juce::Colours::black;
    float opacity = 1.0f;
};

// Keep one renderer per painted target, on the message thread. Geometry passed
// to render() is already in the caller's coordinates; scale affects styles only.
// The renderer respects the Graphics clip. Use getRenderBounds() when painting
// on a parent to allow an effect to extend beyond a child's hit area.
class LMEffectRenderer : private juce::ValueTree::Listener
{
public:
    LMEffectRenderer() = default;
    ~LMEffectRenderer() override;

    static std::vector<LMShadowStyle> readEffects (const juce::ValueTree&, const juce::String& target = {});
    void setEffects (const std::vector<LMShadowStyle>&);
    // Optional live XML binding; re-parses only when the source subtree changes.
    void setSource (const juce::ValueTree&, const juce::String& target = {});
    bool isEmpty() const { return layers.empty(); }

    void render (juce::Graphics&, const juce::Path&, float scale = 1.0f, float opacity = 1.0f);
    void render (juce::Graphics&, const juce::Path&, const juce::PathStrokeType&,
                 float scale = 1.0f, float opacity = 1.0f);
    void render (juce::Graphics&, const juce::GlyphArrangement&, float scale = 1.0f, float opacity = 1.0f);
    juce::Rectangle<float> getRenderBounds (juce::Rectangle<float> sourceBounds, float scale = 1.0f) const;

private:
    struct Layer
    {
        LMShadowStyle style;
        melatonin::DropShadow shadow;
    };
    std::vector<std::unique_ptr<Layer>> layers;
    juce::ValueTree source;
    juce::String sourceTarget;
    void updateEffects (const std::vector<LMShadowStyle>&);
    void refreshSource();
    static void configure (Layer&, float scale, float opacity, bool stroked);
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override { refreshSource(); }
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override { refreshSource(); }
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override { refreshSource(); }
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override { refreshSource(); }
    void valueTreeParentChanged (juce::ValueTree&) override {}
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LMEffectRenderer)
};
