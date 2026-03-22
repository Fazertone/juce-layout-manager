#pragma once

#include "../layout_manager.h" 

class LMLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LMLookAndFeel();

    // Text button customization methods
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    void drawButtonBackground(juce::Graphics& g,
                            juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

    // Setters for customization
    void setButtonFont(const juce::Font& font);
    void setButtonFontHeight(float height);
    void setButtonCornerRadius(float radius);
    void setButtonStrokeColour(const juce::Colour& colour);
    void setButtonStrokeWidth(float width);
    void setButtonFillColour(const juce::Colour& colour);
    void setButtonHasStroke(bool hasStroke);
    void setButtonHasFill(bool hasFill);

    // TextEditor customization methods
    void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;
    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;

    // TextEditor configuration setters
    void setTextEditorCornerRadius(float radius);
    void setTextEditorStrokeColour(const juce::Colour& colour);
    void setTextEditorStrokeWidth(float width);
    void setTextEditorFillColour(const juce::Colour& colour);
    void setTextEditorHasStroke(bool hasStroke);
    void setTextEditorHasFill(bool hasFill);

    // ComboBox customization methods
    void drawComboBox (juce::Graphics& g, int width, int height, bool isMouseButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
    juce::Font getComboBoxFont (juce::ComboBox& box) override;
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;

    // ComboBox configuration setters
    void setComboBoxFont(const juce::Font& font);
    void setComboBoxFontHeight(float height);
    void setComboBoxCornerRadius(float radius);
    void setComboBoxStrokeColour(const juce::Colour& colour);
    void setComboBoxStrokeWidth(float width);
    void setComboBoxFillColour(const juce::Colour& colour);
    void setComboBoxHasStroke(bool hasStroke);
    void setComboBoxHasFill(bool hasFill);

    // PopupMenu customization methods
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColourToUse) override;
    juce::Font getPopupMenuFont() override;

    // TabbedButtonBar customization methods
    void createTabTextLayout (const juce::TabBarButton& button, float length, float depth,
                              juce::Colour colour, juce::TextLayout& textLayout);
    void drawTabButton (juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override;
    void drawTabAreaBehindFrontButton (juce::TabbedButtonBar& bar, juce::Graphics& g, const int w, const int h) override;
    int getTabButtonBestWidth 	(juce::TabBarButton &, int tabDepth) override;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override;

    // RotarySlider configuration setters
    void setRotarySliderCircleFillColour(const juce::Colour& colour);
    void setRotarySliderCircleStrokeColour(const juce::Colour& colour);
    void setRotarySliderCircleStrokeWidth(float width);
    void setRotarySliderArcActiveColour(const juce::Colour& colour);
    void setRotarySliderArcActiveWidth(float width);
    void setRotarySliderArcBgColour(const juce::Colour& colour);
    void setRotarySliderArcBgWidth(float width);

    class SliderLabelComp;
    juce::Label* createSliderTextBox (juce::Slider& slider) override;

    // other styling
    juce::Font mainFont;
    float mainFontHeight = 16.0f;

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override;
    juce::Slider::SliderLayout sliderLayout;
    juce::Label sliderLabelBase;
private:
    juce::Font buttonFont;
    float buttonFontHeight = 16.0f;
    float buttonCornerRadius = 4.0f;
    juce::Colour buttonStrokeColour = juce::Colours::white;
    float buttonStrokeWidth = 1.0f;
    juce::Colour buttonFillColour = juce::Colours::transparentBlack;
    bool buttonHasStroke = false;
    bool buttonHasFill = true;

    // ComboBox styling
    juce::Font comboBoxFont;
    float comboBoxFontHeight = 16.0f;
    float comboBoxCornerRadius = 4.0f;
    juce::Colour comboBoxStrokeColour = juce::Colours::white;
    float comboBoxStrokeWidth = 1.0f;
    juce::Colour comboBoxFillColour = juce::Colours::black;
    bool comboBoxHasStroke = false;
    bool comboBoxHasFill = true;

    // TextEditor styling
    float textEditorCornerRadius = 4.0f;
    juce::Colour textEditorStrokeColour = juce::Colours::grey;
    float textEditorStrokeWidth = 1.0f;
    juce::Colour textEditorFillColour = juce::Colours::transparentBlack;
    bool textEditorHasStroke = false;
    bool textEditorHasFill = false;

    // RotarySlider styling - circle with stroke and fill, arc with color and fill up to slider position
    // if stroke width is 0, then no stroke
    juce::Colour rotarySliderCircleFillColour = juce::Colours::transparentBlack;
    juce::Colour rotarySliderCircleStrokeColour = juce::Colours::transparentBlack;
    float rotarySliderCircleStrokeWidth = 0.0f;

    juce::Colour rotarySliderArcActiveColour = juce::Colours::transparentBlack;
    float rotarySliderArcActiveWidth = 0.0f;
    juce::Colour rotarySliderArcBgColour = juce::Colours::transparentBlack;
    float rotarySliderArcBgWidth = 0.0f;

};

class SpriteKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SpriteKnobLookAndFeel();
    juce::Image spriteSheet;
    int yframes;
    int xframes;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override;
};