#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Effects.h"
#include <array>
#include <optional>

// Standard JUCE slider semantics, plus value entry when XML hides the text box.
class LMSlider : public juce::Slider
{
public:
    LMSlider();
    ~LMSlider() override;
    void setValueDisplayEnabled (bool enabled);
    bool commitNumericText (const juce::String& text);
    void cancelInteraction();
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void showValueEditor();
    unsigned interactionRevision = 0;
    std::optional<juce::MouseEvent> dragEvent;
    juce::Component::SafePointer<juce::CallOutBox> valueEditor;
};

// UI-only envelope; times use the consumer's units, sustain is an amplitude.
// Setters never emit callbacks. All methods and callbacks run on the message thread.
class LMAhdsrComponent : public juce::Component
{
public:
    enum Stage { attack, hold, decay, sustain, release, stageCount };
    using Values = std::array<float, stageCount>;
    using Ranges = std::array<juce::NormalisableRange<float>, stageCount>;

    LMAhdsrComponent();
    ~LMAhdsrComponent() override;
    void setStyle (const juce::ValueTree&, float scale);
    void setRanges (const Ranges&);
    void setValues (const Values&);
    void setValue (Stage, float);
    const Values& getValues() const { return values; }
    std::array<juce::Point<float>, stageCount> getHandlePositions() const;
    void cancelGesture();

    std::function<void (Stage)> onGestureStart, onGestureEnd;
    std::function<void (Stage, float)> onValueChange;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusLost (FocusChangeType) override { repaint(); }
    void focusGained (FocusChangeType) override { repaint(); }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    juce::Rectangle<float> plotBounds() const;
    float timeScale() const;
    std::array<juce::Point<float>, stageCount> endpoints() const;
    void editValue (Stage, float);
    float number (const char*, float) const;
    juce::Colour colour (const char*, const char*) const;
    Values values { 0, 0, 0, 1, 50 }, dragValues {};
    Ranges ranges;
    juce::ValueTree style;
    float scale = 1.0f, frozenTimeScale = 1.0f;
    int activeHandle = -1, focusedHandle = 0;
    juce::Point<float> dragOrigin;
    LMEffectRenderer lineEffects;
};

// SVG artwork stays vector based. Overrides tint visible fills and strokes only.
class LMSvgButton : public juce::Button
{
public:
    LMSvgButton();
    void setStyle (const juce::ValueTree&, const juce::File& assetsFolder, float scale);
    void paintButton (juce::Graphics&, bool hover, bool down) override;

private:
    juce::ValueTree style;
    juce::String cachedArtwork, cachedColours;
    std::array<std::unique_ptr<juce::Drawable>, 4> artwork;
    LMEffectRenderer circleEffects, selectedEffects;
    float scale = 1.0f;
};
