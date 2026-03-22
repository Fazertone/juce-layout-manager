#include "LMLookAndFeel.h"

LMLookAndFeel::LMLookAndFeel()
{
    // Set the default font for the application
    //setDefaultSansSerifTypeface(juce::Typeface::createSystemTypefaceFor(BinaryData::WixMadeforTextVariableFontwght_ttf, BinaryData::WixMadeforTextVariableFontwght_ttfSize));
    
    // Initialize button font with default
  //  buttonFont = juce::Font(buttonFontHeight);
}

juce::Font LMLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    juce::ignoreUnused(buttonHeight);
    // Use the configured font height, or scale based on button height if preferred
    return buttonFont.withPointHeight(buttonFontHeight);
}

void LMLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();

    // Draw fill if enabled
    if (buttonHasFill)
    {
        auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                          .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, buttonCornerRadius);
    }

    // Draw stroke if enabled
    if (buttonHasStroke)
    {
        g.setColour(buttonStrokeColour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
        const float strokeWidth = juce::jmax(0.0f, buttonStrokeWidth);
        if (strokeWidth > 0.0f)
        {
            const float inset = strokeWidth * 0.5f;
            const float w = bounds.getWidth() - strokeWidth;
            const float h = bounds.getHeight() - strokeWidth;

            if (w > 0.0f && h > 0.0f)
            {
                g.drawRoundedRectangle(bounds.reduced(inset), buttonCornerRadius, strokeWidth);
            }
        }
    }
}

void LMLookAndFeel::setButtonFont(const juce::Font& font)
{
    buttonFont = font;
}

void LMLookAndFeel::setButtonFontHeight(float height)
{
    buttonFontHeight = height;
}

void LMLookAndFeel::setButtonCornerRadius(float radius)
{
    buttonCornerRadius = radius;
}

void LMLookAndFeel::setButtonStrokeColour(const juce::Colour& colour)
{
    buttonStrokeColour = colour;
}

void LMLookAndFeel::setButtonStrokeWidth(float width)
{
    buttonStrokeWidth = juce::jmax(0.0f, width);
}

void LMLookAndFeel::setButtonFillColour(const juce::Colour& colour)
{
    buttonFillColour = colour;
}

void LMLookAndFeel::setButtonHasStroke(bool hasStroke)
{
    buttonHasStroke = hasStroke;
}

void LMLookAndFeel::setButtonHasFill(bool hasFill)
{
    buttonHasFill = hasFill;
}

void LMLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor)
{
    juce::ignoreUnused(textEditor);
    
    if (textEditorHasFill)
    {
        g.setColour(textEditorFillColour);
        g.fillRoundedRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), textEditorCornerRadius);
    }
}

void LMLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor)
{
    juce::ignoreUnused(textEditor);
    
    if (textEditorHasStroke)
    {
        g.setColour(textEditorStrokeColour);
        const float strokeWidth = juce::jmax(0.0f, textEditorStrokeWidth);
        if (strokeWidth > 0.0f)
        {
            const float inset = strokeWidth * 0.5f;
            const float w = static_cast<float>(width) - strokeWidth;
            const float h = static_cast<float>(height) - strokeWidth;

            if (w > 0.0f && h > 0.0f)
            {
                g.drawRoundedRectangle(inset, inset, w, h, textEditorCornerRadius, strokeWidth);
            }
        }
    }
}

void LMLookAndFeel::setTextEditorCornerRadius(float radius)
{
    textEditorCornerRadius = radius;
}

void LMLookAndFeel::setTextEditorStrokeColour(const juce::Colour& colour)
{
    textEditorStrokeColour = colour;
}

void LMLookAndFeel::setTextEditorStrokeWidth(float width)
{
    textEditorStrokeWidth = juce::jmax(0.0f, width);
}

void LMLookAndFeel::setTextEditorFillColour(const juce::Colour& colour)
{
    textEditorFillColour = colour;
}

void LMLookAndFeel::setTextEditorHasStroke(bool hasStroke)
{
    textEditorHasStroke = hasStroke;
}

void LMLookAndFeel::setTextEditorHasFill(bool hasFill)
{
    textEditorHasFill = hasFill;
}

// -------------------------------------------------------------
// ComboBox
// -------------------------------------------------------------
void LMLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isMouseButtonDown,
                                   int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    juce::ignoreUnused (isMouseButtonDown, buttonX, buttonY, buttonW, buttonH);

    auto bounds = box.getLocalBounds().toFloat();

    // Draw fill if enabled
    if (comboBoxHasFill)
    {
        g.setColour (comboBoxFillColour.withMultipliedAlpha (box.isEnabled() ? 1.0f : 0.5f));
        g.fillRoundedRectangle (bounds, comboBoxCornerRadius);
    }

    // Draw stroke if enabled
    if (comboBoxHasStroke)
    {
        g.setColour (comboBoxStrokeColour.withMultipliedAlpha (box.isEnabled() ? 1.0f : 0.5f));
        const float strokeWidth = juce::jmax (0.0f, comboBoxStrokeWidth);
        if (strokeWidth > 0.0f)
        {
            g.drawRoundedRectangle (bounds.reduced (strokeWidth * 0.5f), comboBoxCornerRadius, strokeWidth);
        }
    }

    // Draw the caret (arrow)
    if (box.isEnabled())
    {
        // Caret should be square with the same height as the text
        // We'll use the font height for the caret size
        float caretSize = comboBoxFontHeight;
        
        // Position it on the right, centered vertically
        float caretX = (float)width - caretSize - 2.0f; // 2px padding from right
        float caretY = ((float)height - caretSize) * 0.5f;

        juce::Rectangle<float> caretBounds (caretX, caretY, caretSize, caretSize);
        
        // Draw a simple downward triangle for the caret
        juce::Path p;
        float arrowW = caretSize * 0.6f;
        float arrowH = caretSize * 0.3f;
        float centerX = caretBounds.getCentreX();
        float centerY = caretBounds.getCentreY();

        p.addTriangle (centerX - arrowW * 0.5f, centerY - arrowH * 0.5f,
                       centerX + arrowW * 0.5f, centerY - arrowH * 0.5f,
                       centerX,                 centerY + arrowH * 0.5f);

        g.setColour (box.findColour (juce::ComboBox::arrowColourId).withMultipliedAlpha (box.isEnabled() ? 1.0f : 0.5f));
        g.fillPath (p);
    }
}

juce::Font LMLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    juce::ignoreUnused (box);
    return comboBoxFont.withPointHeight (comboBoxFontHeight);
}

void LMLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    // Caret is square with height = font height
    float caretSize = comboBoxFontHeight;
    
    // Label should take up the remaining space, minus the caret area
    label.setBounds (1, 1,
                     juce::jmax (1, box.getWidth() - (int)caretSize - 4),
                     box.getHeight() - 2);

    label.setFont (getComboBoxFont (box));
}

void LMLookAndFeel::setComboBoxFont (const juce::Font& font)
{
    comboBoxFont = font;
}

void LMLookAndFeel::setComboBoxFontHeight (float height)
{
    comboBoxFontHeight = height;
}

void LMLookAndFeel::setComboBoxCornerRadius (float radius)
{
    comboBoxCornerRadius = radius;
}

void LMLookAndFeel::setComboBoxStrokeColour (const juce::Colour& colour)
{
    comboBoxStrokeColour = colour;
}

void LMLookAndFeel::setComboBoxStrokeWidth (float width)
{
    comboBoxStrokeWidth = juce::jmax (0.0f, width);
}

void LMLookAndFeel::setComboBoxFillColour (const juce::Colour& colour)
{
    comboBoxFillColour = colour;
}

void LMLookAndFeel::setComboBoxHasStroke (bool hasStroke)
{
    comboBoxHasStroke = hasStroke;
}

void LMLookAndFeel::setComboBoxHasFill (bool hasFill)
{
    comboBoxHasFill = hasFill;
}

// -------------------------------------------------------------
// PopupMenu
// -------------------------------------------------------------
void LMLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                        bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                                        const juce::String& text, const juce::String& shortcutKeyText,
                                        const juce::Drawable* icon, const juce::Colour* textColourToUse)
{
    if (isSeparator)
    {
        auto r = area.reduced (4, 0);
        g.setColour (findColour (juce::PopupMenu::textColourId).withAlpha (0.2f));
        g.fillRect (r.removeFromTop (juce::roundToInt (((float) r.getHeight() * 0.5f) - 0.5f)).withHeight (1));
    }
    else
    {
        auto textColour = (textColourToUse == nullptr ? findColour (juce::PopupMenu::textColourId)
                                                      : *textColourToUse);

        auto r = area.reduced (1);

        if (isHighlighted && isActive)
        {
            g.setColour (findColour (juce::PopupMenu::highlightedBackgroundColourId));
            g.fillRect (r);

            g.setColour (findColour (juce::PopupMenu::highlightedTextColourId));
        }
        else
        {
            g.setColour (textColour.withMultipliedAlpha (isActive ? 1.0f : 0.5f));
        }

        r.reduce (juce::roundToInt ((float) r.getHeight() * 0.125f), 0);

        auto font = getPopupMenuFont();
        g.setFont (font);

        auto iconArea = r.removeFromLeft (juce::roundToInt (font.getHeight() * 1.1f)).toFloat();

        if (icon != nullptr)
        {
            icon->drawWithin (g, iconArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
            r.removeFromLeft (juce::roundToInt (font.getHeight() * 0.5f));
        }
        else if (isTicked)
        {
            auto tick = getTickShape (1.0f);
            g.fillPath (tick, juce::RectanglePlacement (juce::RectanglePlacement::centred).getTransformToFit (tick.getBounds(), iconArea.reduced (iconArea.getWidth() * 0.05f, 0)));
        }

        if (hasSubMenu)
        {
            auto arrowH = 0.6f * getPopupMenuFont().getAscent();
            auto x = (float) r.removeFromRight (juce::roundToInt (arrowH)).getX();
            auto y = (float) r.getY() + (float) r.getHeight() * 0.5f;

            juce::Path p;
            p.addTriangle (x, y - arrowH * 0.5f,
                           x, y + arrowH * 0.5f,
                           x + arrowH * 0.6f, y);

            g.fillPath (p);
        }

        r.removeFromRight (3);
        g.drawFittedText (text, r, juce::Justification::centredLeft, 1);

        if (shortcutKeyText.isNotEmpty())
        {
            auto f2 = font;
            f2.setHeight (f2.getHeight() * 0.75f);
            f2.setHorizontalScale (0.95f);
            g.setFont (f2);

            g.drawText (shortcutKeyText, r, juce::Justification::centredRight, true);
        }
    }
}

juce::Font LMLookAndFeel::getPopupMenuFont()
{
    return comboBoxFont.withPointHeight (comboBoxFontHeight);
}

// -------------------------------------------------------------
// TabbedButtonBar
// -------------------------------------------------------------
void LMLookAndFeel::createTabTextLayout (const juce::TabBarButton& button, float length, float depth,
                                          juce::Colour colour, juce::TextLayout& textLayout)
{
    //Font font (button.withDefaultMetrics (FontOptions { depth*0.75f  }));

    juce::String fontName = "Wix Madefor Text"; // Default
    juce::String fontType = "Regular"; // Default
    // style plain, unless selected, then bold
    juce::Font::FontStyleFlags styleFlags = juce::Font::plain;
    if (button.getToggleState())
    {
        styleFlags = juce::Font::bold;
    }

    juce::Font font = juce::Font(fontName, depth, styleFlags); // Medium weight if available in typeface
    font.setUnderline (button.hasKeyboardFocus (false));
    font.setPointHeight(depth);


    juce::AttributedString s;
    s.setJustification (juce::Justification::left);
    s.append (button.getButtonText().trim(), font, colour);

    textLayout.createLayout (s, length);
}

void LMLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown)
{
    const juce::Rectangle<int> activeArea (button.getActiveArea());

    juce::Colour col = juce::Colours::white;

    if (!button.getToggleState())
    {
        col = juce::Colour(0xff8b8b8b);
    }

    float length = activeArea.getWidth();
    float depth  = activeArea.getHeight();

    juce::TextLayout textLayout;
    createTabTextLayout (button, length, depth, col, textLayout);

    textLayout.draw (g, juce::Rectangle<float> (length, depth));
}

void LMLookAndFeel::drawTabAreaBehindFrontButton (juce::TabbedButtonBar& bar, juce::Graphics& g, const int w, const int h)
{
    juce::ignoreUnused(bar, g, w, h);
}

int LMLookAndFeel::getTabButtonBestWidth (juce::TabBarButton & button, int tabDepth)
{
    juce::ignoreUnused(button, tabDepth);
    return tabDepth*5;
}


juce::Slider::SliderLayout LMLookAndFeel::getSliderLayout(juce::Slider& slider)
{
	return sliderLayout;
}

// RotarySlider drawing - adapted from previous project
void LMLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)
{
    juce::Slider::SliderLayout layout = getSliderLayout(slider);

    juce::Rectangle<int> sliderBounds = layout.sliderBounds;
    DBG("DrawRotarySlider: SliderBounds: " + sliderBounds.toString());

    // Calculate radii - outer for arc, inner for circle
    width = sliderBounds.getWidth();
    height = sliderBounds.getHeight();
    x = sliderBounds.getX();
    y = sliderBounds.getY();
    auto outer_radius = (float)juce::jmin(width / 2, height / 2) - rotarySliderArcActiveWidth;
    auto inner_radius = outer_radius * 0.66f;

    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Draw background arc (from current position to end) if specified
    if (rotarySliderArcBgWidth > 0.0f && rotarySliderArcBgColour.getAlpha() > 0)
    {
        g.setColour(rotarySliderArcBgColour);
        juce::Path bgArc;
        bgArc.addArc(centreX - outer_radius, centreY - outer_radius, outer_radius * 2.0f, outer_radius * 2.0f, 
                     angle, rotaryEndAngle, true);
        g.strokePath(bgArc, juce::PathStrokeType(rotarySliderArcBgWidth));
    }

    // Draw active arc (from start to current position) if specified
    if (rotarySliderArcActiveWidth > 0.0f && rotarySliderArcActiveColour.getAlpha() > 0)
    {
        g.setColour(rotarySliderArcActiveColour);
        juce::Path activeArc;
        activeArc.addArc(centreX - outer_radius, centreY - outer_radius, outer_radius * 2.0f, outer_radius * 2.0f, 
                        rotaryStartAngle, angle, true);
        g.strokePath(activeArc, juce::PathStrokeType(rotarySliderArcActiveWidth));
    }

    // Draw circle fill if specified
    if (rotarySliderCircleFillColour.getAlpha() > 0)
    {
        g.setColour(rotarySliderCircleFillColour);
        g.fillEllipse(centreX - inner_radius, centreY - inner_radius, inner_radius * 2.0f, inner_radius * 2.0f);
    }

    // Draw circle stroke if specified
    if (rotarySliderCircleStrokeWidth > 0.0f && rotarySliderCircleStrokeColour.getAlpha() > 0)
    {
        g.setColour(rotarySliderCircleStrokeColour);
        g.drawEllipse(centreX - inner_radius, centreY - inner_radius, inner_radius * 2.0f, inner_radius * 2.0f, 
                     rotarySliderCircleStrokeWidth);
    }
}

// RotarySlider setters
void LMLookAndFeel::setRotarySliderCircleFillColour(const juce::Colour& colour)
{
    rotarySliderCircleFillColour = colour;
}

void LMLookAndFeel::setRotarySliderCircleStrokeColour(const juce::Colour& colour)
{
    rotarySliderCircleStrokeColour = colour;
}

void LMLookAndFeel::setRotarySliderCircleStrokeWidth(float width)
{
    rotarySliderCircleStrokeWidth = juce::jmax(0.0f, width);
}

void LMLookAndFeel::setRotarySliderArcActiveColour(const juce::Colour& colour)
{
    rotarySliderArcActiveColour = colour;
}

void LMLookAndFeel::setRotarySliderArcActiveWidth(float width)
{
    rotarySliderArcActiveWidth = juce::jmax(0.0f, width);
}

void LMLookAndFeel::setRotarySliderArcBgColour(const juce::Colour& colour)
{
    rotarySliderArcBgColour = colour;
}

void LMLookAndFeel::setRotarySliderArcBgWidth(float width)
{
    rotarySliderArcBgWidth = juce::jmax(0.0f, width);
}

class LMLookAndFeel::SliderLabelComp final : public juce::Label
{
public:
    SliderLabelComp() : juce::Label ({}, {}) {}

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return createIgnoredAccessibilityHandler (*this);
    }
};

juce::Label* LMLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    DBG("Creating slider text box");
    auto l = new SliderLabelComp();

    l->setBorderSize(sliderLabelBase.getBorderSize());
    l->setMinimumHorizontalScale(1.0f);
    l->setFont(sliderLabelBase.getFont());
    l->setJustificationType(sliderLabelBase.getJustificationType());
    l->setColour(juce::Label::textColourId, sliderLabelBase.findColour(juce::Label::textColourId));

    return l;
}

SpriteKnobLookAndFeel::SpriteKnobLookAndFeel()
{
    ;
    //setColour(Slider::thumbColourId, Colours::red);
}


void SpriteKnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)
{
    {
        const float rotation = (slider.getValue()
            - slider.getMinimum())
            / (slider.getMaximum()
                - slider.getMinimum());

        const int totalframes = xframes * yframes;
        int frameId = (int)round(rotation * ((double)totalframes - 1.0));

        if (frameId < 0) frameId = 0;
        else if (frameId > totalframes) frameId = totalframes;

        const int frameIdy = floor((float)frameId / (float)xframes);
        const int frameIdx = frameId - (frameIdy * xframes);

        const float radius = juce::jmin(width / 1.0f, height / 1.0f);
        const float centerX = x + width * 0.5f;
        const float centerY = y + height * 0.5f;
        const float rx = centerX - radius - 1.0f;
        const float ry = centerY - radius;

        int imgWidth = spriteSheet.getWidth() / (float)xframes;
        int imgHeight = spriteSheet.getHeight() / (float)yframes;

        // THIS DOESN'T WORK IF THE PIXEL VALUES ARE NOT WHOLE NUMBERS
        g.drawImage(spriteSheet, 0, 0, width, height, frameIdx * imgWidth, frameIdy * imgHeight, imgWidth, imgHeight);
    }
}

