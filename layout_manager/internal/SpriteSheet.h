#pragma once

#include "../layout_manager.h"

//==============================================================================
/**
    A single named sprite: one frame strip/grid image plus its on-screen
    placement (in background-image coordinate space) and optional named
    state -> frame mappings (e.g. "off" -> 0, "pressed" -> 1, "on" -> 2).
*/
class Sprite
{
public:
    juce::String label;
    juce::Image image;                          // the frame strip/grid
    int columns = 1, rows = 1;
    juce::Rectangle<int> placement;             // x/y/w/h in background-image space
    std::map<juce::String, int> stateFrames;    // "off"->0, "pressed"->1, "on"->2
    int edgeBlur = 0;   // linear alpha feather width at the frame border, in
                        // background-image (source) pixels. 0 = no feather. Hides
                        // the hard rectangular seam of opaque JPG frames.

    int getNumFrames() const noexcept                        { return columns * rows; }
    int getStateFrame (const juce::String& state, int fallback = 0) const;

    // Cell bounds for a frame, computed so rounding error never accumulates
    // across cells (fixes the whole-pixel-only limitation of the old sprite LAF).
    juce::Rectangle<int> getFrameBounds (int frameIndex) const;

    void drawFrame (juce::Graphics& g, int frameIndex, juce::Rectangle<float> dest) const;

    bool isValid() const noexcept                            { return image.isValid(); }
};

//==============================================================================
/**
    Loads a set of sprites from an XML metadata file (as produced by the
    Pressure spritesheet tooling) and exposes them by label.

        SpriteSheetCollection knobs;
        knobs.loadFromXml (file);
        auto* s = knobs.get ("knob_input");   // nullptr if missing
        const Sprite& s2 = knobs["knob_input"]; // jassert + empty if missing
*/
class SpriteSheetCollection
{
public:
    // Images are resolved relative to the XML file's directory.
    bool loadFromXml (const juce::File& xmlFile);

    const Sprite* get (const juce::String& label) const;
    const Sprite& operator[] (const juce::String& label) const;

    juce::String type;          // "knob" / "switch"
    int numFramesDefault = 0;   // the sheet-wide num_frames attribute

private:
    std::map<juce::String, Sprite> sprites;
};

//==============================================================================
/**
    A rotary slider that renders itself from a Sprite frame strip. Works with a
    plain juce::AudioProcessorValueTreeState::SliderAttachment.
*/
class SpriteKnobComponent : public juce::Slider
{
public:
    SpriteKnobComponent();

    void setSprite (const Sprite* newSprite)    { sprite = newSprite; repaint(); }
    const Sprite* getSprite() const noexcept     { return sprite; }

    void paint (juce::Graphics& g) override;

private:
    const Sprite* sprite = nullptr;   // non-owning
};

//==============================================================================
/**
    A button that renders itself from a Sprite frame strip using named states
    ("pressed" while held, otherwise "on"/"off" from the toggle state). Works
    with a plain juce::AudioProcessorValueTreeState::ButtonAttachment.
*/
class SpriteSwitchComponent : public juce::Button
{
public:
    SpriteSwitchComponent();

    void setSprite (const Sprite* newSprite)    { sprite = newSprite; repaint(); }
    const Sprite* getSprite() const noexcept     { return sprite; }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

private:
    const Sprite* sprite = nullptr;   // non-owning
};
