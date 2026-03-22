#pragma once

#include "../layout_manager.h" 

class HyperlinkTextButton : public juce::HyperlinkButton
{
public:
    HyperlinkTextButton();

    // Background
    void setFillColour(const juce::Colour& colour);
    void setCornerRadius(float radius);
    void setHasFill(bool fill);

    // Text
    void setTextBounds(juce::Rectangle<float> bounds);
    void setTextFont(const juce::Font& font);
    void setTextColour(const juce::Colour& colour);
    void setTextJustification(juce::Justification justification);
    void setHasText(bool hasText);

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    // Background
    juce::Colour fillColour = juce::Colours::transparentBlack;
    float cornerRadius = 4.0f;
    bool hasFill = false;

    // Text
    juce::Rectangle<float> textBounds;
    juce::Font textFont;
    juce::Colour textColour = juce::Colours::white;
    juce::Justification textJustification { juce::Justification::centredLeft };
    bool hasText = false;
};

class HoverableTextButton : public juce::TextButton
{
public:
    HoverableTextButton (const juce::String& buttonName);
    HoverableTextButton();

    void mouseEnter (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;
};

class ImageHolder : public juce::Component
{
public:
    ImageHolder();

    void paint(juce::Graphics& g) override;
    void setImage(const juce::Image& image);

private:
    juce::Image image;
};

class ToggleImageButton : public juce::ToggleButton
{
public:
    ToggleImageButton();
    void setImages(const juce::Image& imageOn, const juce::Image& imageOff);

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    juce::Image imageOn;
    juce::Image imageOff;
    juce::Image imageHover;
};

class BlurredModalContainer : public juce::Component
{
public:
    BlurredModalContainer();

    void paint(juce::Graphics& g) override;

    void openModal();
    void closeModal();

private:
    melatonin::CachedBlur blur { 3 }; // Initial radius
    melatonin::CachedBlur blurWindow{ 8 }; // Initial radius

    void captureAndBlurBackground();
};