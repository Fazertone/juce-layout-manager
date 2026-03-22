#pragma once

#include "../layout_manager.h" 

class LMLookAndFeel;

juce::ValueTree findComponentByName(const juce::String& componentName, const juce::ValueTree& tree);

class RegisteredComponent {
public:
    RegisteredComponent(juce::Component* component, juce::String name);
    ~RegisteredComponent();

    juce::Component* component;
    juce::String name;
};

class LayoutManager {
public:
    // Constructor that takes XML file path
    LayoutManager(const juce::String& xmlFilePath);
    
    // Constructor that takes XML data directly
    LayoutManager(const juce::XmlElement& xmlData);
    
    LayoutManager(const char* xmlData, size_t xmlDataSize);

    // path to assets folder
    juce::File assetsFolder;

    // Destructor
    ~LayoutManager();

    std::vector<RegisteredComponent> registeredComponents;
    void registerComponent(juce::Component* component, juce::String name);

    void registerRotarySlider(juce::Component* component, juce::String name);

    void applyResize();
    void applyLayout();

    // Font registration - register fonts from binary data to be used by name in XML
    // fontName: the name to use in XML (e.g., "Wix Madefor Text")
    // fontData: pointer to font binary data
    // fontDataSize: size of font binary data
    void registerFont(const juce::String& fontName, const void* fontData, size_t fontDataSize);
    
    // Create a juce::FontOptions from XML element attributes (fontName, fontSize, fontType)
    // Uses registered fonts if available, otherwise falls back to system fonts
    juce::FontOptions createFontFromElement(const juce::ValueTree& elementData) const;

    // Set position of a component based on its name in the XML
    void setPosition(const RegisteredComponent& component);
    void setLabelLayout(juce::Component* component, const juce::ValueTree& elementData);
    void setTextButtonLayout(juce::Component* component, const juce::ValueTree& elementData);
    void setTextEditorLayout(juce::Component* component, const juce::ValueTree& elementData);
    void setHyperlinkButtonLayout(const RegisteredComponent& component, const juce::ValueTree& elementData);
    void setComboBoxLayout(juce::Component* component, const juce::ValueTree& elementData);
    void setRotarySliderLayout(juce::Component* component, const juce::ValueTree& elementData);
    void paintComponent(juce::Graphics& g, juce::String componentID);
    // Load layout from XML file
    bool loadFromFile(const juce::String& xmlFilePath);
    
    // Load layout from XML element
    bool loadFromXml(const juce::XmlElement& xmlData);
    
    // Check if layout data is valid
    bool isValid() const;
    
    // Get component bounds by name (useful for debugging)
    juce::Rectangle<float> getComponentBounds(const juce::String& componentName) const;
    
    // Get a specific attribute for a component by name
    juce::var getAttribute(const juce::String& componentName, const juce::String& attributeName) const;
    void setAttribute(const juce::String& componentName, const juce::String& attributeName, const juce::var& value);
    
    float scaling = 1.0f;
    
    juce::ValueTree layoutTree;
    bool layoutLoaded;

    std::map<juce::String, juce::Typeface::Ptr> registeredTypefaces;
private:
    // Store LookAndFeel instances for text buttons and text editors
    juce::OwnedArray<LMLookAndFeel> buttonLookAndFeels;
    
    // Registered typefaces by font name

    // Helper methods for painting
    void paintRectangle(juce::Graphics& g, const juce::ValueTree& rectData);
    void paintLine(juce::Graphics& g, const juce::ValueTree& lineData);
    void paintAutoLabel(juce::Graphics& g, const juce::ValueTree& labelData);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayoutManager)
};
