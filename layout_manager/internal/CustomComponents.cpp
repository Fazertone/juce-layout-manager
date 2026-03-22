#include "CustomComponents.h"

HyperlinkTextButton::HyperlinkTextButton() {}

void HyperlinkTextButton::setFillColour(const juce::Colour& colour)      { fillColour = colour; }
void HyperlinkTextButton::setCornerRadius(float radius)                   { cornerRadius = radius; }
void HyperlinkTextButton::setHasFill(bool fill)                           { hasFill = fill; }
void HyperlinkTextButton::setTextBounds(juce::Rectangle<float> bounds)    { textBounds = bounds; }
void HyperlinkTextButton::setTextFont(const juce::Font& font)             { textFont = font; }
void HyperlinkTextButton::setTextColour(const juce::Colour& colour)       { textColour = colour; }
void HyperlinkTextButton::setTextJustification(juce::Justification j)     { textJustification = j; }
void HyperlinkTextButton::setHasText(bool text)                           { hasText = text; }

void HyperlinkTextButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    if (hasFill)
    {
        auto colour = fillColour.withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);
        if (shouldDrawButtonAsDown)
            colour = colour.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted)
            colour = colour.darker(0.1f);
        g.setColour(colour);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerRadius);
    }

    if (hasText)
    {
        g.setFont(textFont);
        g.setColour(textColour.withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f));
        g.drawFittedText(getButtonText(), textBounds.toNearestInt(), textJustification, 32, 1.0f);
    }
}

HoverableTextButton::HoverableTextButton (const juce::String& buttonName)
    : juce::TextButton (buttonName)
{
}

HoverableTextButton::HoverableTextButton()
    : juce::TextButton ()
{
}

void HoverableTextButton::mouseEnter (const juce::MouseEvent& event)
{
    // if active
    if (isEnabled())
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
   
    juce::TextButton::mouseEnter (event);
}

void HoverableTextButton::mouseExit (const juce::MouseEvent& event)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
    juce::TextButton::mouseExit (event);
}

ImageHolder::ImageHolder()
    : juce::Component ()
{
}

void ImageHolder::setImage(const juce::Image& image){
    this->image = image;
}

void ImageHolder::paint(juce::Graphics& g){
    if (image.isValid()){
        g.drawImage(image, getLocalBounds().toFloat());
    }
}

ToggleImageButton::ToggleImageButton(){
}

void ToggleImageButton::setImages(const juce::Image& imageOn, const juce::Image& imageOff){
    this->imageOn = imageOn;
    this->imageOff = imageOff;
}

void ToggleImageButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){
     if (getToggleState()){
        g.drawImage(imageOn, getLocalBounds().toFloat());
    } else {
        g.drawImage(imageOff, getLocalBounds().toFloat());
     }
}

BlurredModalContainer::BlurredModalContainer(){
}

void BlurredModalContainer::paint(juce::Graphics& g){
    g.drawImageAt (blur.render(), 0, 0);
}

void BlurredModalContainer::captureAndBlurBackground(){
    if (auto* parent = getParentComponent())
    {
        // 1. Temporarily hide ourselves so we don't snapshot an empty overlay
        setVisible(false);

        // 2. Snapshot the parent (the background)
        // scaleFactor: 1.0 ensures 1:1 pixel mapping, set lower (e.g. 0.5) for higher performance
        auto snapshot = parent->createComponentSnapshot(parent->getLocalBounds(), false, 1.0f);

        // 3. Update the Melatonin blur cache
        blur.update(snapshot);
        blurWindow.update(snapshot);
    }
}

void BlurredModalContainer::openModal(){
    captureAndBlurBackground();
    setVisible(true);
    toFront(true);
}

void BlurredModalContainer::closeModal(){
    setVisible(false);
}


