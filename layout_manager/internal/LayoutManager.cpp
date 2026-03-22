#include "LayoutManager.h"
#include "LMLookAndFeel.h"

RegisteredComponent::RegisteredComponent(juce::Component* component, juce::String name) : component(component), name(name){}
RegisteredComponent::~RegisteredComponent(){}



static juce::Font fontWithDecoration(juce::FontOptions options, const juce::ValueTree& elementData)
{
    juce::Font font(options);
    if (elementData.getProperty("textDecoration", "").toString().equalsIgnoreCase("underline"))
        font.setUnderline(true);
    return font;
}

// Helper method to find a child component by name in the layout tree
juce::ValueTree findChildByName(const juce::String& name, const juce::ValueTree& tree) 
{
    if (tree.hasProperty("name") && tree.getProperty("name").toString() == name)
        return tree;
        
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        juce::ValueTree found = findChildByName(name, tree.getChild(i));
        if (found.isValid())
            return found;
    }
    
    return juce::ValueTree();
}

// Constructor that takes XML file path
LayoutManager::LayoutManager(const juce::String& xmlFilePath)
    : scaling(1.0f), layoutTree("JUCELayout"), layoutLoaded(false)
{
    loadFromFile(xmlFilePath);
}

// Constructor that takes XML data directly
LayoutManager::LayoutManager(const juce::XmlElement& xmlData)
    : scaling(1.0f), layoutTree("JUCELayout"), layoutLoaded(false)
{
    loadFromXml(xmlData);
}

LayoutManager::LayoutManager(const char* xmlData, size_t xmlDataSize)
    : scaling(1.0f), layoutTree("JUCELayout"), layoutLoaded(false)
{
    std::unique_ptr<juce::XmlElement> xml = juce::parseXML (xmlData);
    loadFromXml(*xml);
}


// Destructor
LayoutManager::~LayoutManager()
{
    // Now it's safe for the OwnedArray to delete the look and feel objects
    buttonLookAndFeels.clear();
    registeredTypefaces.clear();
}

// Register a font from binary data to be used by name in XML
void LayoutManager::registerFont(const juce::String& fontName, const void* fontData, size_t fontDataSize)
{
    auto typeface = juce::Typeface::createSystemTypefaceFor(fontData, fontDataSize);
    if (typeface != nullptr)
    {
        registeredTypefaces[fontName] = typeface;
    }
    else
    {
        DBG("LayoutManager: Failed to register font: " + fontName);
    }
}

// Create a juce::FontOptions from XML element attributes using JUCE 8 FontOptions API
juce::FontOptions LayoutManager::createFontFromElement(const juce::ValueTree& elementData) const
{
    // Default values
    juce::String fontName = "Wix Madefor Text";
    float fontSize = 14.0f;
    juce::String fontType = "Regular";
    
    // Read attributes from XML element
    if (elementData.hasProperty("fontName"))
        fontName = elementData.getProperty("fontName").toString();
    
    if (elementData.hasProperty("fontSize"))
        fontSize = elementData.getProperty("fontSize").toString().getFloatValue();
    
    if (elementData.hasProperty("fontType"))
        fontType = elementData.getProperty("fontType").toString();
    
    // Build style string for font styles
    juce::String styleStr;
    if (fontType.containsIgnoreCase("Medium"))
    {
        styleStr = "Medium";
        if (fontType.containsIgnoreCase("Bold"))
            styleStr = "Bold " + styleStr;
        if (fontType.containsIgnoreCase("Italic"))
            styleStr += " Italic";
    }
    else
    {
        if (fontType.containsIgnoreCase("Bold"))
            styleStr = "Bold";
        if (fontType.containsIgnoreCase("Italic"))
        {
            if (styleStr.isNotEmpty())
                styleStr += " Italic";
            else
                styleStr = "Italic";
        }
    }
    
    // Build FontOptions
    juce::FontOptions options;
    
    // Try to find a registered typeface matching name + style (e.g., "Wix Madefor Text Bold")
    // This is for static fonts where each style is a separate file
    juce::String typefaceKey = styleStr.isNotEmpty() ? fontName + " " + styleStr : fontName;
    auto it = registeredTypefaces.find(typefaceKey);
    
    if (it != registeredTypefaces.end() && it->second != nullptr)
    {
        // Use the registered typeface with exact style match (static fonts)
        options = juce::FontOptions(it->second);
    }
    // else
    // {
    //     // Use name-based lookup - this works with both system fonts and 
    //     // variable fonts where styles are handled by the font system
    //     options = options.withName(fontName);
    //     if (styleStr.isNotEmpty())
    //         options = options.withStyle(styleStr);
    // }
    
    // Apply scaled font size (using point height)
    options = options.withPointHeight(fontSize * scaling);
    
    return options;
}

// Load layout from XML file
bool LayoutManager::loadFromFile(const juce::String& xmlFilePath)
{
    juce::File xmlFile(xmlFilePath);
    
    if (!xmlFile.existsAsFile())
    {
        DBG("LayoutManager: XML file does not exist: " + xmlFilePath);
        return false;
    }
    
    std::unique_ptr<juce::XmlElement> xmlElement = juce::XmlDocument::parse(xmlFile);
    
    if (xmlElement == nullptr)
    {
        DBG("LayoutManager: Failed to parse XML file: " + xmlFilePath);
        return false;
    }
    
    return loadFromXml(*xmlElement);
}



// Load layout from XML element
bool LayoutManager::loadFromXml(const juce::XmlElement& xmlData)
{
    if (xmlData.getTagName() != "JUCELayout")
    {
        DBG("LayoutManager: XML root element must be 'JUCELayout', found: " + xmlData.getTagName());
        return false;
    }
    
    // Clear existing layout data
    layoutTree.removeAllChildren(nullptr);
    layoutLoaded = false;
    
    try
    {
        layoutTree = juce::ValueTree::fromXml(xmlData);
        layoutLoaded = layoutTree.isValid();
        if (layoutLoaded)
        {
            //DBG("LayoutManager: Successfully loaded " + juce::String(layoutTree.getNumChildren()) + " components");
            // print the layoutTree output
           // DBG("LayoutManager: LayoutTree: " + layoutTree.toXmlString());
            return true;
        }
        else
        {
            DBG("LayoutManager: Failed to create ValueTree from XML");
            return false;
        }
    }
    catch (const std::exception& e)
    {
        DBG("LayoutManager: Exception while parsing XML: " + juce::String(e.what()));
        return false;
    }
}

// Helper method to find a component by name in the hierarchical tree
juce::ValueTree findComponentByName(const juce::String& componentName, const juce::ValueTree& tree) 
{
    // Check if current tree node matches
    if (tree.hasProperty("name") && tree.getProperty("name").toString() == componentName)
    {
        return tree;
    }
    
    // Recursively search children
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        juce::ValueTree found = findComponentByName(componentName, tree.getChild(i));
        if (found.isValid())
            return found;
    }
    
    return juce::ValueTree();
}

// Set position of a component based on its name in the XML
void LayoutManager::setPosition(const RegisteredComponent& component)
{
    if (!layoutLoaded)
    {
        DBG("LayoutManager: Layout not loaded, cannot set position for component");
        return;
    }
    
    juce::String componentName = component.name;
    
    if (componentName.isEmpty())
    {
        DBG("LayoutManager: Component has no name set, cannot find layout data");
        return;
    }
    
    // Find the component in the hierarchical layout tree
    juce::ValueTree componentLayout = findComponentByName(componentName, layoutTree);
    
    if (componentLayout.isValid())
    {
        float x = static_cast<float>(static_cast<double>(componentLayout.getProperty("x", 0.0)));
        float y = static_cast<float>(static_cast<double>(componentLayout.getProperty("y", 0.0)));
        float width = static_cast<float>(static_cast<double>(componentLayout.getProperty("width", 100.0)));
        float height = static_cast<float>(static_cast<double>(componentLayout.getProperty("height", 20.0)));

        x *= scaling;
        y *= scaling;
        width *= scaling;
        height *= scaling;
        
        component.component->setBounds(static_cast<int>(x), static_cast<int>(y), 
                          static_cast<int>(width), static_cast<int>(height));
        
        //DBG("LayoutManager: Set bounds for '" + componentName + "' to  (" + 
        //    juce::String(x) + ", " + juce::String(y) + ", " + 
        //    juce::String(width) + ", " + juce::String(height) + ")");
    }
    else
    {
        DBG("LayoutManager: Component '" + componentName + "' not found in layout data");
    }
}


// Check if layout data is valid
bool LayoutManager::isValid() const
{
    return layoutLoaded && layoutTree.getNumChildren() > 0;
}

// Get component bounds by name (useful for debugging)
juce::Rectangle<float> LayoutManager::getComponentBounds(const juce::String& componentName) const
{
    if (!layoutLoaded)
        return juce::Rectangle<float>();
    
    // Find the component in the hierarchical layout tree
    juce::ValueTree componentLayout = findComponentByName(componentName, layoutTree);
    
    if (componentLayout.isValid())
    {
        float x = static_cast<float>(static_cast<double>(componentLayout.getProperty("x", 0.0)));
        float y = static_cast<float>(static_cast<double>(componentLayout.getProperty("y", 0.0)));
        float width = static_cast<float>(static_cast<double>(componentLayout.getProperty("width", 100.0)));
        float height = static_cast<float>(static_cast<double>(componentLayout.getProperty("height", 20.0)));
        
        return juce::Rectangle<float>(x, y, width, height);
    }
    
    return juce::Rectangle<float>();
}

juce::var LayoutManager::getAttribute(const juce::String& componentName, const juce::String& attributeName) const
{
    if (!layoutLoaded)
        return juce::var();

    juce::ValueTree componentLayout = findComponentByName(componentName, layoutTree);

    if (componentLayout.isValid())
    {
        return componentLayout.getProperty(attributeName);
    }

    DBG("LayoutManager: Component '" + componentName + "' not found in layout data, cannot get attribute '" + attributeName + "'");
    return juce::var();
}

// register a rotary slider
void LayoutManager::registerRotarySlider(juce::Component* component, juce::String name){
    juce::Slider* slider = dynamic_cast<juce::Slider*>(component);
    if (slider != nullptr) {
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    }
}

void LayoutManager::registerComponent(juce::Component* component, juce::String name)
{
    component->setComponentID(name);
    registeredComponents.push_back(RegisteredComponent(component, name));

    // if the component is an image button, set the image
    if (name.endsWith("_img_btn")) {
        if (name.endsWith("_toggle_img_btn")) {
            juce::File imageFileOff = assetsFolder.getChildFile("image_buttons").getChildFile(name + "_OFF.png");
            juce::File imageFileOn = assetsFolder.getChildFile("image_buttons").getChildFile(name + "_ON.png");
        
            if (imageFileOff.exists() && imageFileOn.exists()) {
                juce::Image imageOff = juce::ImageCache::getFromFile(imageFileOff);
                juce::Image imageOn = juce::ImageCache::getFromFile(imageFileOn);

                ToggleImageButton* toggleImageButton = dynamic_cast<ToggleImageButton*>(component);
                if (toggleImageButton != nullptr) {
                    toggleImageButton->setImages(imageOn, imageOff);
                    toggleImageButton->setClickingTogglesState(true);
                    DBG("LayoutManager: Registered toggle image button '" + name + "'");
                }
            }
        } else {
            DBG("LayoutManager: Registering image button '" + name + "'");
            // get the image from the assets folder in image_buttons/name.png
            juce::File imageFile = assetsFolder.getChildFile("image_buttons").getChildFile(name + ".png");
            if (imageFile.exists()) {
                juce::ImageButton* imageButton = dynamic_cast<juce::ImageButton*>(component);
                if (imageButton != nullptr) {
                    juce::Image image = juce::ImageCache::getFromFile(imageFile);
                    juce::File imageDownFile = assetsFolder.getChildFile("image_buttons").getChildFile(name + "_DOWN.png");
                    if (imageDownFile.exists()) {
                        juce::Image imageDown = juce::ImageCache::getFromFile(imageDownFile);
                        imageButton->setImages(false, true, true, image, 1.0f, {}, image, 1.0f, {}, imageDown, 1.0, {});
                    } else {
                        imageButton->setImages(false, true, true, image, 1.0f, {}, image, 0.33f, {}, image, 0.66f, {});
                    }
    
                } else {
                    DBG("LayoutManager: Component is not an image button");
                }
            } else {
                DBG("LayoutManager: Image file not found for component '" + name + "'");
            }
    
        }
    } else if (name.endsWith("_img")) {
        DBG("LayoutManager: Registering image '" + name + "'");
        // get the image from the assets folder in images/name.png
        juce::File imageFile = assetsFolder.getChildFile("images").getChildFile(name + ".png");
        if (imageFile.exists()) {
            ImageHolder* imageHolder = dynamic_cast<ImageHolder*>(component);
            if (imageHolder != nullptr) {
                juce::Image image = juce::ImageCache::getFromFile(imageFile);
                imageHolder->setImage(image);
            } else {
                DBG("LayoutManager: Component is not an image holder");
            }
        } else {
            DBG("LayoutManager: Image file not found for component '" + name + "'");
        }
    } else if (name.endsWith("_rotary_slider")) {
        // set slider style as Rotrary
        //juce::Slider* slider = dynamic_cast<juce::Slider*>(component);
        registerRotarySlider(component, name);
    } else if (name.endsWith("_text_editor")) {
        // Handle multiline property
        juce::TextEditor* textEditor = dynamic_cast<juce::TextEditor*>(component);
        auto elementData = findChildByName(name, layoutTree);
        if (elementData.hasProperty("multiline"))
        {
            bool isMultiline = elementData.getProperty("multiline").toString().equalsIgnoreCase("true") ||
                            static_cast<int>(elementData.getProperty("multiline")) == 1;
            if (isMultiline)
            {
                textEditor->setMultiLine(true);
                textEditor->setScrollbarsShown(true);
            }
        }

    }
}

void LayoutManager::applyResize(){
    // for all components
    for (int i = 0; i < registeredComponents.size(); i++){
        setPosition(registeredComponents[i]);
    }

    applyLayout();
}

void LayoutManager::applyLayout(){
    // for all components
    for (int i = 0; i < registeredComponents.size(); i++){
        // get the layout data
        juce::ValueTree elementData = findChildByName(registeredComponents[i].name, layoutTree);

        if (!elementData.isValid())
        {
            DBG("LayoutManager: Component '" + registeredComponents[i].name + "' not found in layout data");
            continue;
        }

        // Get the type from the XML element tag name
        juce::String elementType = elementData.getType().toString();
        
        if (elementType == "Label"){
            setLabelLayout(registeredComponents[i].component, elementData);
        } else if (elementType == "TextButton"){
            setTextButtonLayout(registeredComponents[i].component, elementData);
        } else if (elementType == "TextEditor"){
            setTextEditorLayout(registeredComponents[i].component, elementData);
        } else if (elementType == "ComboBox" || (elementType == "Object" && registeredComponents[i].name.endsWith("_dropdown_btn"))){
            setComboBoxLayout(registeredComponents[i].component, elementData);
        } else if (elementType == "HyperlinkButton"){
            setHyperlinkButtonLayout(registeredComponents[i], elementData);
        } else if (elementType == "RotarySlider"){
            setRotarySliderLayout(registeredComponents[i].component, elementData);
        } else if (elementType == "ImageButton"){
            // Just position it
            DBG("LayoutManager: Found ImageButton '" + registeredComponents[i].name + "'");
        } else if (elementType == "Object"){
            // Generic component, just position it
            //DBG("LayoutManager: Found generic Component '" + registeredComponents[i].name + "'");
        } else {
            DBG("LayoutManager: Unknown element type '" + elementType + "' for component '" + registeredComponents[i].name + "'");
        }
    }
}


void LayoutManager::setLabelLayout(juce::Component*component, const juce::ValueTree& elementData){
    // cast to label
    juce::Label* label = dynamic_cast<juce::Label*>(component);
    if (label == nullptr)
    {
        DBG("LayoutManager: Component is not a label");
        return;
    }

    label->setBorderSize(juce::BorderSize<int>(0));
    label->setMinimumHorizontalScale(1.0f);

    // Set font using generic font creation
    juce::Font font = fontWithDecoration(createFontFromElement(elementData), elementData);
    label->setFont(font);

    // Set label text if provided
    if (elementData.hasProperty("text"))
    {
        label->setText(elementData.getProperty("text").toString(), juce::dontSendNotification);
        // use Font::getStringWidth() to get the width of the text
        // = font.getStringWidth(elementData.getProperty("text").toString());
    }

    // Set justification (supporting both old textAlign and new textAlignHorizontal/textAlignVertical)
    int horizontalFlags = juce::Justification::left;
    int verticalFlags = juce::Justification::verticallyCentred;
    
    // Handle horizontal alignment
    if (elementData.hasProperty("textAlignHorizontal"))
    {
        juce::String textAlignH = elementData.getProperty("textAlignHorizontal").toString();
        if (textAlignH.equalsIgnoreCase("left"))
            horizontalFlags = juce::Justification::left;
        else if (textAlignH.equalsIgnoreCase("center"))
            horizontalFlags = juce::Justification::horizontallyCentred;
        else if (textAlignH.equalsIgnoreCase("right"))
            horizontalFlags = juce::Justification::right;
    }
    else if (elementData.hasProperty("textAlign"))
    {
        // Fallback to old textAlign property for backward compatibility
        juce::String textAlign = elementData.getProperty("textAlign").toString();
        if (textAlign.equalsIgnoreCase("left"))
            horizontalFlags = juce::Justification::left;
        else if (textAlign.equalsIgnoreCase("center"))
            horizontalFlags = juce::Justification::horizontallyCentred;
        else if (textAlign.equalsIgnoreCase("right"))
            horizontalFlags = juce::Justification::right;
    }
    
    // Handle vertical alignment
    if (elementData.hasProperty("textAlignVertical"))
    {
        juce::String textAlignV = elementData.getProperty("textAlignVertical").toString();
        if (textAlignV.equalsIgnoreCase("top"))
            verticalFlags = juce::Justification::top;
        else if (textAlignV.equalsIgnoreCase("center"))
            verticalFlags = juce::Justification::verticallyCentred;
        else if (textAlignV.equalsIgnoreCase("bottom"))
            verticalFlags = juce::Justification::bottom;
    }
    
    // Combine horizontal and vertical flags
    juce::Justification justification(horizontalFlags | verticalFlags);
    label->setJustificationType(justification);

    // Set text color
    if (elementData.hasProperty("fillColour"))
    {
        juce::String colourStr = elementData.getProperty("fillColour").toString();
        label->setColour(juce::Label::textColourId, juce::Colour::fromString(colourStr));
    }

    // // if debug, set background color to transparent grey
    // #ifdef JUCE_DEBUG
    // label->setColour(juce::Label::backgroundColourId, juce::Colour(juce::Colour::fromRGBA(128, 128, 128, 128)));
    // #endif
}

void LayoutManager::setAttribute(const juce::String& componentName, const juce::String& attributeName, const juce::var& value){
    if (!layoutLoaded)
    {
        DBG("LayoutManager: Layout not loaded, cannot set attribute");
        return;
    }

    // Find the component in the layout tree and update the property
    juce::ValueTree componentLayout = findComponentByName(componentName, layoutTree);
    if (componentLayout.isValid())
    {
        componentLayout.setProperty(juce::Identifier(attributeName), value, nullptr);
    }
    else
    {
        DBG("LayoutManager: Component '" + componentName + "' not found in layout data");
        return;
    }

    // Find the registered component to apply the change immediately
    juce::Component* component = nullptr;
    for (const auto& regComp : registeredComponents)
    {
        if (regComp.name == componentName)
        {
            component = regComp.component;
            break;
        }
    }

    if (component == nullptr)
    {
        DBG("LayoutManager: Component '" + componentName + "' not found in registered components");
        return;
    }

    // Apply the attribute change to the actual component based on component type
    
    // Handle Label
    if (auto* label = dynamic_cast<juce::Label*>(component))
    {
        if (attributeName == "text")
        {
            label->setText(value.toString(), juce::dontSendNotification);
        }
        else if (attributeName == "fillColour")
        {
            label->setColour(juce::Label::textColourId, juce::Colour::fromString(value.toString()));
        }
        else if (attributeName == "textAlignHorizontal" || attributeName == "textAlignVertical" || attributeName == "textAlign")
        {
            // Re-apply full justification since it combines both axes
            int horizontalFlags = juce::Justification::left;
            int verticalFlags = juce::Justification::verticallyCentred;
            
            if (componentLayout.hasProperty("textAlignHorizontal"))
            {
                juce::String textAlignH = componentLayout.getProperty("textAlignHorizontal").toString();
                if (textAlignH.equalsIgnoreCase("left"))
                    horizontalFlags = juce::Justification::left;
                else if (textAlignH.equalsIgnoreCase("center"))
                    horizontalFlags = juce::Justification::horizontallyCentred;
                else if (textAlignH.equalsIgnoreCase("right"))
                    horizontalFlags = juce::Justification::right;
            }
            else if (componentLayout.hasProperty("textAlign"))
            {
                juce::String textAlign = componentLayout.getProperty("textAlign").toString();
                if (textAlign.equalsIgnoreCase("left"))
                    horizontalFlags = juce::Justification::left;
                else if (textAlign.equalsIgnoreCase("center"))
                    horizontalFlags = juce::Justification::horizontallyCentred;
                else if (textAlign.equalsIgnoreCase("right"))
                    horizontalFlags = juce::Justification::right;
            }
            
            if (componentLayout.hasProperty("textAlignVertical"))
            {
                juce::String textAlignV = componentLayout.getProperty("textAlignVertical").toString();
                if (textAlignV.equalsIgnoreCase("top"))
                    verticalFlags = juce::Justification::top;
                else if (textAlignV.equalsIgnoreCase("center"))
                    verticalFlags = juce::Justification::verticallyCentred;
                else if (textAlignV.equalsIgnoreCase("bottom"))
                    verticalFlags = juce::Justification::bottom;
            }
            
            label->setJustificationType(juce::Justification(horizontalFlags | verticalFlags));
        }
        return;
    }

    // Handle TextButton
    if (auto* button = dynamic_cast<juce::TextButton*>(component))
    {
        if (attributeName == "text")
        {
            button->setButtonText(value.toString());
        }
        else if (attributeName == "fillColour")
        {
            button->setColour(juce::TextButton::buttonColourId, juce::Colour::fromString(value.toString()));
        }
        else if (attributeName == "textColour")
        {
            juce::Colour textColour = juce::Colour::fromString(value.toString());
            button->setColour(juce::TextButton::textColourOffId, textColour);
            button->setColour(juce::TextButton::textColourOnId, textColour);
        }
        return;
    }

    // Handle HyperlinkButton
    if (auto* hyperlinkButton = dynamic_cast<juce::HyperlinkButton*>(component))
    {
        if (attributeName == "text")
        {
            hyperlinkButton->setButtonText(value.toString());
        }
        else if (attributeName == "url")
        {
            hyperlinkButton->setURL(juce::URL(value.toString()));
        }
        else if (attributeName == "fillColour")
        {
            hyperlinkButton->setColour(juce::HyperlinkButton::textColourId, juce::Colour::fromString(value.toString()));
        }
        return;
    }

    // Handle TextEditor
    if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
    {
        if (attributeName == "text")
        {
            textEditor->setText(value.toString(), juce::dontSendNotification);
        }
        else if (attributeName == "fillColour" || attributeName == "textColour")
        {
            textEditor->setColour(juce::TextEditor::textColourId, juce::Colour::fromString(value.toString()));
        }
        else if (attributeName == "placeholderText")
        {
            textEditor->setTextToShowWhenEmpty(value.toString(), textEditor->findColour(juce::TextEditor::textColourId).withAlpha(0.5f));
        }
        return;
    }

    // Handle ComboBox
    if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
    {
        if (attributeName == "fillColour")
        {
            comboBox->setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromString(value.toString()));
        }
        else if (attributeName == "textColour")
        {
            juce::Colour textColour = juce::Colour::fromString(value.toString());
            comboBox->setColour(juce::ComboBox::textColourId, textColour);
            comboBox->setColour(juce::ComboBox::arrowColourId, textColour);
        }
        return;
    }

    // Handle Slider
    if (auto* slider = dynamic_cast<juce::Slider*>(component))
    {
        if (attributeName == "value")
        {
            slider->setValue(static_cast<double>(value), juce::dontSendNotification);
        }
        return;
    }

    // For position/size attributes, update the component bounds
    if (attributeName == "x" || attributeName == "y" || attributeName == "width" || attributeName == "height")
    {
        float x = static_cast<float>(static_cast<double>(componentLayout.getProperty("x", 0.0)));
        float y = static_cast<float>(static_cast<double>(componentLayout.getProperty("y", 0.0)));
        float width = static_cast<float>(static_cast<double>(componentLayout.getProperty("width", 100.0)));
        float height = static_cast<float>(static_cast<double>(componentLayout.getProperty("height", 20.0)));

        x *= scaling;
        y *= scaling;
        width *= scaling;
        height *= scaling;

        component->setBounds(static_cast<int>(x), static_cast<int>(y), 
                            static_cast<int>(width), static_cast<int>(height));
        return;
    }

    // Trigger repaint for visual attributes that may need it
    component->repaint();
}

void LayoutManager::setTextButtonLayout(juce::Component* component, const juce::ValueTree& elementData){
    // cast to text button
    juce::TextButton* button = dynamic_cast<juce::TextButton*>(component);
    if (button == nullptr)
    {
        DBG("LayoutManager: Component is not a text button");
        return;
    }

    // Search for btn_bg and btn_text/btn_txt within the button's element data
    juce::ValueTree btnBg = findChildByName("btn_bg", elementData);
    juce::ValueTree btnText = findChildByName("btn_text", elementData);
    
    // Try btn_txt if btn_text wasn't found
    if (!btnText.isValid())
    {
        btnText = findChildByName("btn_txt", elementData);
    }

    // Create and configure a FurnaceLookAndFeel for this button
    auto* laf = new LMLookAndFeel();
    buttonLookAndFeels.add(laf);

    // Set corner radius from the parent element or btn_bg
    if (elementData.hasProperty("cornerRadius"))
    {
        float cornerRadius = static_cast<float>(elementData.getProperty("cornerRadius"));
        laf->setButtonCornerRadius(cornerRadius);
    }
    else if (btnBg.isValid() && btnBg.hasProperty("cornerRadius"))
    {
        float cornerRadius = static_cast<float>(btnBg.getProperty("cornerRadius"));
        laf->setButtonCornerRadius(cornerRadius);
    }

    // Configure font from btn_text element
    if (btnText.isValid())
    {
        // Use generic font creation
        //juce::Font font = createFontFromElement(btnText);
        if (btnText.hasProperty("fontName")) {
            juce::String fontName = btnText.getProperty("fontName").toString();
            // use the font creator
            juce::FontOptions font = createFontFromElement(btnText);
            
            laf->setButtonFont(font);
        }
            
        // Get font size for height setting
        float fontSize = 16.0f;
        if (btnText.hasProperty("fontSize"))
            fontSize = static_cast<float>(btnText.getProperty("fontSize"));
        laf->setButtonFontHeight(fontSize * scaling);

        // Set button text
        if (btnText.hasProperty("text"))
        {
            button->setButtonText(btnText.getProperty("text").toString());
        }

        // Set text color
        if (btnText.hasProperty("fillColour"))
        {
            juce::String colourStr = btnText.getProperty("fillColour").toString();
            button->setColour(juce::TextButton::textColourOffId, juce::Colour::fromString(colourStr));
            button->setColour(juce::TextButton::textColourOnId, juce::Colour::fromString(colourStr));
        }
    }

    // Handle button background styling from btn_bg
    if (btnBg.isValid())
    {
        // Check for fill color
        if (btnBg.hasProperty("fillColour"))
        {
            juce::String colourStr = btnBg.getProperty("fillColour").toString();
            juce::Colour fillColour = juce::Colour::fromString(colourStr);
            button->setColour(juce::TextButton::buttonColourId, fillColour);
            laf->setButtonFillColour(fillColour);
            laf->setButtonHasFill(true);
        }
        else
        {
            // No fill color specified, make button transparent
            button->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            laf->setButtonHasFill(false);
        }

        // Check for stroke color
        if (btnBg.hasProperty("strokeColour"))
        {
            juce::String strokeColourStr = btnBg.getProperty("strokeColour").toString();
            laf->setButtonStrokeColour(juce::Colour::fromString(strokeColourStr));
            laf->setButtonHasStroke(true);
            
            // Set stroke width if specified, otherwise default to 1.0
            float strokeWidth = 1.0f;
            if (btnBg.hasProperty("strokeWeight"))
            {
                strokeWidth = static_cast<float>(btnBg.getProperty("strokeWeight"));
            }
            laf->setButtonStrokeWidth(strokeWidth * scaling);
        }
    }

    // Apply the custom LookAndFeel to the button
    button->setLookAndFeel(laf);
}

void LayoutManager::setTextEditorLayout(juce::Component* component, const juce::ValueTree& elementData){
    // Cast to TextEditor
    juce::TextEditor* textEditor = dynamic_cast<juce::TextEditor*>(component);
    if (textEditor == nullptr)
    {
        DBG("LayoutManager: Component is not a text editor");
        return;
    }

    // Ensure our LookAndFeel draws the background by disabling JUCE defaults
    textEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    textEditor->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    textEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);


    // Search for TextEditorBox, TextEditorEntry, and TextEditorSuggestion children
    juce::ValueTree txtBoxBg = findChildByName("text_editor_box", elementData);
    juce::ValueTree txtBoxEntry = findChildByName("text_editor_entry", elementData);
    juce::ValueTree txtBoxSuggestion = findChildByName("text_editor_suggestion", elementData);

    // Create and configure a FurnaceLookAndFeel for this text editor
    auto* laf = new LMLookAndFeel();
    buttonLookAndFeels.add(laf);

    // Set corner radius from the parent element or txt_box_bg
    if (elementData.hasProperty("cornerRadius"))
    {
        float cornerRadius = static_cast<float>(elementData.getProperty("cornerRadius"));
        laf->setTextEditorCornerRadius(cornerRadius);
    }
    else if (txtBoxBg.isValid() && txtBoxBg.hasProperty("cornerRadius"))
    {
        float cornerRadius = static_cast<float>(txtBoxBg.getProperty("cornerRadius"));
        laf->setTextEditorCornerRadius(cornerRadius);
    }

    // Configure stroke and fill colors from txt_box_bg
    if (txtBoxBg.isValid())
    {
        // Only enable stroke if strokeColour is specified
        if (txtBoxBg.hasProperty("strokeColour"))
        {
            float strokeWidth = 1.0f;
            if (txtBoxBg.hasProperty("strokeWeight"))
            {
                strokeWidth = static_cast<float>(txtBoxBg.getProperty("strokeWeight"));
            }

            juce::String colourStr = txtBoxBg.getProperty("strokeColour").toString();
            laf->setTextEditorStrokeColour(juce::Colour::fromString(colourStr));

            float scaledStrokeWidth = strokeWidth * scaling;
            if (scaledStrokeWidth <= 0.0f)
                scaledStrokeWidth = 1.0f;

            laf->setTextEditorStrokeWidth(scaledStrokeWidth);
            laf->setTextEditorHasStroke(true);
        }
        else
        {
            laf->setTextEditorHasStroke(false);
        }

        if (txtBoxBg.hasProperty("fillColour"))
        {
            juce::String colourStr = txtBoxBg.getProperty("fillColour").toString();
            laf->setTextEditorFillColour(juce::Colour::fromString(colourStr));
            laf->setTextEditorHasFill(true);
        }
        else
        {
            laf->setTextEditorHasFill(false);
        }
    }
    else
    {
        // No txt_box_bg element, so no stroke by default
        laf->setTextEditorHasStroke(false);
    }

    juce::Colour textColour;

    // Configure font and text color from txt_box_entry element
    if (txtBoxEntry.isValid())
    {
        // Use generic font creation
        juce::Font font = createFontFromElement(txtBoxEntry);
        textEditor->setFont(font);

        // Set text color
        if (txtBoxEntry.hasProperty("fillColour"))
        {
            juce::String colourStr = txtBoxEntry.getProperty("fillColour").toString();
            textColour = juce::Colour::fromString(colourStr);
            textEditor->setColour(juce::TextEditor::textColourId, textColour);
        }

        // Set text alignment (supporting both old textAlign and new textAlignHorizontal/textAlignVertical)
        int horizontalFlags = juce::Justification::left;
        int verticalFlags = juce::Justification::verticallyCentred;
        
        // Handle horizontal alignment
        if (txtBoxEntry.hasProperty("textAlignHorizontal"))
        {
            juce::String textAlignH = txtBoxEntry.getProperty("textAlignHorizontal").toString();
            if (textAlignH.equalsIgnoreCase("left"))
                horizontalFlags = juce::Justification::left;
            else if (textAlignH.equalsIgnoreCase("center"))
                horizontalFlags = juce::Justification::horizontallyCentred;
            else if (textAlignH.equalsIgnoreCase("right"))
                horizontalFlags = juce::Justification::right;
        }
        else if (txtBoxEntry.hasProperty("textAlign"))
        {
            // Fallback to old textAlign property for backward compatibility
            juce::String textAlign = txtBoxEntry.getProperty("textAlign").toString();
            if (textAlign.equalsIgnoreCase("left"))
                horizontalFlags = juce::Justification::left;
            else if (textAlign.equalsIgnoreCase("center"))
                horizontalFlags = juce::Justification::horizontallyCentred;
            else if (textAlign.equalsIgnoreCase("right"))
                horizontalFlags = juce::Justification::right;
        }
        
        // Handle vertical alignment
        if (txtBoxEntry.hasProperty("textAlignVertical"))
        {
            juce::String textAlignV = txtBoxEntry.getProperty("textAlignVertical").toString();
            if (textAlignV.equalsIgnoreCase("top"))
                verticalFlags = juce::Justification::top;
            else if (textAlignV.equalsIgnoreCase("center"))
                verticalFlags = juce::Justification::verticallyCentred;
            else if (textAlignV.equalsIgnoreCase("bottom"))
                verticalFlags = juce::Justification::bottom;
        }
        
        // Combine horizontal and vertical flags
        juce::Justification justification(horizontalFlags | verticalFlags);
        textEditor->setJustification(justification);

        // use the x for the left indent (don't use the y for now, not useful)
        if (txtBoxEntry.hasProperty("x"))
        {
            textEditor->setIndents(txtBoxEntry.getProperty("x").toString().getIntValue(), 0);
        }
    }

    // Remove default border
    textEditor->setBorder(juce::BorderSize<int>(0));

    // Placeholder / suggestion styling
    juce::Colour placeholderColour = textColour;
    juce::String placeholderText;

    if (txtBoxSuggestion.isValid())
    {
        if (txtBoxSuggestion.hasProperty("fillColour"))
        {
            placeholderColour = juce::Colour::fromString(txtBoxSuggestion.getProperty("fillColour").toString());
        }

        if (txtBoxSuggestion.hasProperty("text"))
        {
            placeholderText = txtBoxSuggestion.getProperty("text").toString();
        }
    }

    // Apply placeholder text or at least ensure the colour matches the entry style
    textEditor->setTextToShowWhenEmpty(placeholderText, placeholderColour);
    textEditor->setColour(juce::CaretComponent::caretColourId, textColour);

    // Apply the custom LookAndFeel to the text editor
    textEditor->setLookAndFeel(laf);
}

void LayoutManager::setComboBoxLayout(juce::Component* component, const juce::ValueTree& elementData)
{
    // Cast to ComboBox
    juce::ComboBox* comboBox = dynamic_cast<juce::ComboBox*>(component);
    if (comboBox == nullptr)
    {
        DBG("LayoutManager: Component is not a combo box");
        return;
    }

    // Search for btn_bg and btn_text/btn_txt within the combo box's element data
    juce::ValueTree btnBg = findChildByName("btn_bg", elementData);
    juce::ValueTree btnText = findChildByName("btn_text", elementData);
    
    // Try btn_txt if btn_text wasn't found
    if (!btnText.isValid())
    {
        btnText = findChildByName("btn_txt", elementData);
    }

    // Create and configure a LookAndFeel for this combo box
    auto* laf = new LMLookAndFeel();
    buttonLookAndFeels.add(laf);

    // Set corner radius from the parent element or btn_bg
    if (elementData.hasProperty("cornerRadius"))
    {
        float cornerRadius = static_cast<float>(elementData.getProperty("cornerRadius"));
        laf->setComboBoxCornerRadius(cornerRadius);
    }
    else if (btnBg.isValid() && btnBg.hasProperty("cornerRadius"))
    {
        float cornerRadius = static_cast<float>(btnBg.getProperty("cornerRadius"));
        laf->setComboBoxCornerRadius(cornerRadius);
    }

    // Configure font from btn_text element
    if (btnText.isValid())
    {
        if (btnText.hasProperty("fontName")) {
            juce::FontOptions font = createFontFromElement(btnText);
            laf->setComboBoxFont(font);
        }
            
        // Get font size for height setting
        float fontSize = 16.0f;
        if (btnText.hasProperty("fontSize"))
            fontSize = static_cast<float>(btnText.getProperty("fontSize"));
        laf->setComboBoxFontHeight(fontSize * scaling);

        // Set text color
        if (btnText.hasProperty("fillColour"))
        {
            juce::String colourStr = btnText.getProperty("fillColour").toString();
            juce::Colour textColour = juce::Colour::fromString(colourStr);
            comboBox->setColour(juce::ComboBox::textColourId, textColour);
            comboBox->setColour(juce::ComboBox::arrowColourId, textColour);
            
            // Set popup menu text colors
            comboBox->setColour(juce::PopupMenu::textColourId, textColour);
            // Use a slightly different color for highlight text if needed, or same as text
            comboBox->setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        }
    }

    // Handle background styling from btn_bg
    if (btnBg.isValid())
    {
        // Check for fill color
        if (btnBg.hasProperty("fillColour"))
        {
            juce::String colourStr = btnBg.getProperty("fillColour").toString();
            juce::Colour fillColour = juce::Colour::fromString(colourStr);
            comboBox->setColour(juce::ComboBox::backgroundColourId, fillColour);
            laf->setComboBoxFillColour(fillColour);
            laf->setComboBoxHasFill(true);

            // Set popup menu background colors
            comboBox->setColour(juce::PopupMenu::backgroundColourId, fillColour);
            // Set highlight background to something contrasting or a standard highlight
            comboBox->setColour(juce::PopupMenu::highlightedBackgroundColourId, fillColour.brighter(0.2f));
        }
        else
        {
            // No fill color specified, make transparent for the combo box itself
            comboBox->setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
            laf->setComboBoxHasFill(false);

            // Default popup background to black if not specified in XML
            comboBox->setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);
            comboBox->setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colours::darkgrey);
        }

        // Check for stroke color
        if (btnBg.hasProperty("strokeColour"))
        {
            juce::String strokeColourStr = btnBg.getProperty("strokeColour").toString();
            laf->setComboBoxStrokeColour(juce::Colour::fromString(strokeColourStr));
            laf->setComboBoxHasStroke(true);
            
            // Set stroke width if specified, otherwise default to 1.0
            float strokeWidth = 1.0f;
            if (btnBg.hasProperty("strokeWeight"))
            {
                strokeWidth = static_cast<float>(btnBg.getProperty("strokeWeight"));
            }
            laf->setComboBoxStrokeWidth(strokeWidth * scaling);
        }
    }

    // Apply the custom LookAndFeel to the combo box
    comboBox->setLookAndFeel(laf);
}

void LayoutManager::setHyperlinkButtonLayout(const RegisteredComponent& component, const juce::ValueTree& elementData)
{
    HyperlinkTextButton* hyperlinkButton = dynamic_cast<HyperlinkTextButton*>(component.component);
    if (hyperlinkButton == nullptr)
    {
        DBG("LayoutManager: Component '" + component.name + "' is not a HyperlinkTextButton");
        return;
    }

    // Background
    juce::ValueTree hyperlinkBackground = findChildByName("hyperlink_bg", elementData);

    if (hyperlinkBackground.isValid())
    {
        hyperlinkButton->setHasFill(true);

        if (hyperlinkBackground.hasProperty("fillColour"))
            hyperlinkButton->setFillColour(juce::Colour::fromString(hyperlinkBackground.getProperty("fillColour").toString()));

        if (hyperlinkBackground.hasProperty("cornerRadius"))
            hyperlinkButton->setCornerRadius(static_cast<float>(hyperlinkBackground.getProperty("cornerRadius")) * scaling);
    }

    // Text
    juce::ValueTree hyperlinkText = findChildByName("hyperlink_text", elementData);

    if (hyperlinkText.isValid())
    {
        hyperlinkButton->setHasText(true);

        if (hyperlinkText.hasProperty("text"))
            hyperlinkButton->setButtonText(hyperlinkText.getProperty("text").toString());

        hyperlinkButton->setTextFont(fontWithDecoration(createFontFromElement(hyperlinkText), hyperlinkText));

        if (hyperlinkText.hasProperty("fillColour"))
            hyperlinkButton->setTextColour(juce::Colour::fromString(hyperlinkText.getProperty("fillColour").toString()));

        float tx     = static_cast<float>(static_cast<double>(hyperlinkText.getProperty("x",      0.0))) * scaling;
        float ty     = static_cast<float>(static_cast<double>(hyperlinkText.getProperty("y",      0.0))) * scaling;
        float twidth  = static_cast<float>(static_cast<double>(hyperlinkText.getProperty("width",  0.0))) * scaling;
        float theight = static_cast<float>(static_cast<double>(hyperlinkText.getProperty("height", 0.0))) * scaling;
        hyperlinkButton->setTextBounds({ tx, ty, twidth, theight });

        int horizontalFlags = juce::Justification::left;
        int verticalFlags   = juce::Justification::verticallyCentred;

        if (hyperlinkText.hasProperty("textAlignHorizontal"))
        {
            juce::String h = hyperlinkText.getProperty("textAlignHorizontal").toString();
            if (h.equalsIgnoreCase("center"))
                horizontalFlags = juce::Justification::horizontallyCentred;
            else if (h.equalsIgnoreCase("right"))
                horizontalFlags = juce::Justification::right;
        }

        if (hyperlinkText.hasProperty("textAlignVertical"))
        {
            juce::String v = hyperlinkText.getProperty("textAlignVertical").toString();
            if (v.equalsIgnoreCase("top"))
                verticalFlags = juce::Justification::top;
            else if (v.equalsIgnoreCase("bottom"))
                verticalFlags = juce::Justification::bottom;
            else if (v.equalsIgnoreCase("center"))
                verticalFlags = juce::Justification::verticallyCentred;
        }

        hyperlinkButton->setTextJustification(juce::Justification(horizontalFlags | verticalFlags));
    }
}

void LayoutManager::setRotarySliderLayout(juce::Component* component, const juce::ValueTree& elementData){
    // Cast to Slider
    juce::Slider* slider = dynamic_cast<juce::Slider*>(component);
    if (slider == nullptr)
    {
        DBG("LayoutManager: Component is not a slider");
        return;
    }

    // Create a new LookAndFeel instance for this rotary slider
    auto* laf = new LMLookAndFeel();
    buttonLookAndFeels.add(laf);

    // get slider widget child
    juce::ValueTree sliderWidget = findChildByName("slider_widget", elementData);
    juce::ValueTree sliderValueText = findChildByName("slider_value_text", elementData);

    if (sliderWidget.isValid()){
        // set the sliderLayout.sliderBounds
        if (sliderWidget.hasProperty("x") && sliderWidget.hasProperty("y") && sliderWidget.hasProperty("width") && sliderWidget.hasProperty("height")){
            juce::Rectangle<int> sliderBounds(sliderWidget.getProperty("x").toString().getFloatValue(), sliderWidget.getProperty("y").toString().getFloatValue(), sliderWidget.getProperty("width").toString().getFloatValue(), sliderWidget.getProperty("height").toString().getFloatValue());
            DBG("SliderBounds: " + sliderBounds.toString());
            laf->sliderLayout.sliderBounds = sliderBounds;
        }
        // Set circle fill colour if specified
        if (sliderWidget.hasProperty("circleFillColour"))
        {
            juce::String colourStr = sliderWidget.getProperty("circleFillColour").toString();
            laf->setRotarySliderCircleFillColour(juce::Colour::fromString(colourStr));
        }

        // Set circle stroke colour if specified
        if (sliderWidget.hasProperty("circleStrokeColour"))
        {
            juce::String colourStr = sliderWidget.getProperty("circleStrokeColour").toString();
            laf->setRotarySliderCircleStrokeColour(juce::Colour::fromString(colourStr));
        }

        // Set circle stroke width if specified
        if (sliderWidget.hasProperty("circleStrokeWidth"))
        {
            float strokeWidth = static_cast<float>(sliderWidget.getProperty("circleStrokeWidth"));
            laf->setRotarySliderCircleStrokeWidth(strokeWidth * scaling);
        }

        // Set arc active colour if specified
        if (sliderWidget.hasProperty("arcActiveColour"))
        {
            juce::String colourStr = sliderWidget.getProperty("arcActiveColour").toString();
            laf->setRotarySliderArcActiveColour(juce::Colour::fromString(colourStr));
        }

        // Set arc active width if specified
        if (sliderWidget.hasProperty("arcActiveWidth"))
        {
            float width = static_cast<float>(sliderWidget.getProperty("arcActiveWidth"));
            laf->setRotarySliderArcActiveWidth(width * scaling);
        }

        // Set arc background colour if specified
        if (sliderWidget.hasProperty("arcBgColour"))
        {
            juce::String colourStr = sliderWidget.getProperty("arcBgColour").toString();
            laf->setRotarySliderArcBgColour(juce::Colour::fromString(colourStr));
        }

        // Set arc background width if specified
        if (sliderWidget.hasProperty("arcBgWidth"))
        {
            float width = static_cast<float>(sliderWidget.getProperty("arcBgWidth"));
            laf->setRotarySliderArcBgWidth(width * scaling);
        }
    }

    //     // <SliderValueText name="slider_value_text" x="15.098" y="2.444" width="20.0" height="6.0" fillColour="#ffcfcfcf" text="1000 Hz" fontName="Wix Madefor Text" fontSize="5" textAlignVertical="center"/>

    if (sliderValueText.isValid()){
        if (sliderValueText.hasProperty("x") && sliderValueText.hasProperty("y") && sliderValueText.hasProperty("width") && sliderValueText.hasProperty("height")){
            juce::Rectangle<int> textBoxBounds(sliderValueText.getProperty("x").toString().getFloatValue(), sliderValueText.getProperty("y").toString().getFloatValue(), sliderValueText.getProperty("width").toString().getFloatValue(), sliderValueText.getProperty("height").toString().getFloatValue());
            DBG("TextBoxBounds: " + textBoxBounds.toString());
            laf->sliderLayout.textBoxBounds = textBoxBounds;
        }
        // <SliderValueText name="slider_value_text" x="15.098" y="2.444" width="20.0" height="6.0" fillColour="#ffcfcfcf" text="1000 Hz" fontName="Wix Madefor Text" fontSize="5" textAlignVertical="center"/>

        // set the textBoxTextColourId, textBoxBackgroundColourId and textBoxHighlightColourId and textBoxOutlineColourId from 
        
        setLabelLayout(&laf->sliderLabelBase, sliderValueText);
    }

    // Apply the custom LookAndFeel to the slider
    slider->setLookAndFeel(laf);
}



void LayoutManager::paintComponent(juce::Graphics& g, juce::String componentID){
    juce::ValueTree elementData = findChildByName(componentID, layoutTree);
    
    if (!elementData.isValid())
        return;
    
    // Iterate through immediate children of elementData
    for (int i = 0; i < elementData.getNumChildren(); ++i)
    {
        juce::ValueTree child = elementData.getChild(i);
        juce::String childType = child.getType().toString();
        
        if (childType == "Rectangle")
        {
            paintRectangle(g, child);
        }
        else if (childType == "Line")
        {
            paintLine(g, child);
        }
        else if (childType == "AutoLabel")
        {
            paintAutoLabel(g, child);
        }
    }
}

void LayoutManager::paintRectangle(juce::Graphics& g, const juce::ValueTree& rectData)
{
    // Get basic position and size
    float x = static_cast<float>(static_cast<double>(rectData.getProperty("x", 0.0))) * scaling;
    float y = static_cast<float>(static_cast<double>(rectData.getProperty("y", 0.0))) * scaling;
    float width = static_cast<float>(static_cast<double>(rectData.getProperty("width", 0.0))) * scaling;
    float height = static_cast<float>(static_cast<double>(rectData.getProperty("height", 0.0))) * scaling;
    
    float cornerRadius = static_cast<float>(static_cast<double>(rectData.getProperty("cornerRadius", 0.0))) * scaling;
    float opacity = static_cast<float>(static_cast<double>(rectData.getProperty("opacity", 1.0)));
    
    juce::Rectangle<float> rect(x, y, width, height);
    
    // Check for gradient fill
    if (rectData.hasProperty("fill-gradient-type"))
    {
        juce::String gradientType = rectData.getProperty("fill-gradient-type").toString();
        
        if (gradientType == "linear" && rectData.hasProperty("fill-gradient-stops"))
        {
            // Parse gradient start and end points (relative coordinates 0-1)
            juce::String startStr = rectData.getProperty("fill-gradient-start", "0.5,0").toString();
            juce::String endStr = rectData.getProperty("fill-gradient-end", "0.5,1").toString();
            
            juce::StringArray startParts = juce::StringArray::fromTokens(startStr, ",", "");
            juce::StringArray endParts = juce::StringArray::fromTokens(endStr, ",", "");
            
            float startX = startParts[0].getFloatValue();
            float startY = startParts[1].getFloatValue();
            float endX = endParts[0].getFloatValue();
            float endY = endParts[1].getFloatValue();
            
            // Flip Y coordinates (1.0 - y) to fix upside-down gradients
            startY = 1.0f - startY;
            endY = 1.0f - endY;
            
            // Convert relative coordinates to absolute
            juce::Point<float> gradStart(x + width * startX, y + height * startY);
            juce::Point<float> gradEnd(x + width * endX, y + height * endY);
            
            // Parse gradient stops
            juce::String stopsStr = rectData.getProperty("fill-gradient-stops").toString();
            juce::StringArray stops = juce::StringArray::fromTokens(stopsStr, ",", "");
            
            if (stops.size() >= 2)
            {
                // Parse first and last stops
                juce::StringArray firstStop = juce::StringArray::fromTokens(stops[0], ":", "");
                juce::StringArray lastStop = juce::StringArray::fromTokens(stops[stops.size()-1], ":", "");
                
                juce::Colour colour1 = juce::Colour::fromString(firstStop[0]);
                juce::Colour colour2 = juce::Colour::fromString(lastStop[0]);
                
                // Apply opacity to gradient colors
                colour1 = colour1.withAlpha(colour1.getFloatAlpha() * opacity);
                colour2 = colour2.withAlpha(colour2.getFloatAlpha() * opacity);
                
                juce::ColourGradient gradient(colour1, gradStart, colour2, gradEnd, false);
                
                // Add intermediate stops if present
                for (int i = 1; i < stops.size() - 1; ++i)
                {
                    juce::StringArray stopParts = juce::StringArray::fromTokens(stops[i], ":", "");
                    if (stopParts.size() >= 2)
                    {
                        juce::Colour stopColour = juce::Colour::fromString(stopParts[0]);
                        stopColour = stopColour.withAlpha(stopColour.getFloatAlpha() * opacity);
                        double position = stopParts[1].getDoubleValue();
                        gradient.addColour(position, stopColour);
                    }
                }
                
                g.setGradientFill(gradient);
                
                if (cornerRadius > 0.0f)
                    g.fillRoundedRectangle(rect, cornerRadius);
                else
                    g.fillRect(rect);
            }
        }
    }
    // Check for solid fill color
    else if (rectData.hasProperty("fillColour"))
    {
        juce::String colourStr = rectData.getProperty("fillColour").toString();
        juce::Colour fillColour = juce::Colour::fromString(colourStr);
        fillColour = fillColour.withAlpha(fillColour.getFloatAlpha() * opacity);
        
        g.setColour(fillColour);
        
        if (cornerRadius > 0.0f)
            g.fillRoundedRectangle(rect, cornerRadius);
        else
            g.fillRect(rect);
    }
    
    // Draw stroke if present
    if (rectData.hasProperty("strokeColour"))
    {
        juce::String strokeColourStr = rectData.getProperty("strokeColour").toString();
        juce::Colour strokeColour = juce::Colour::fromString(strokeColourStr);
        strokeColour = strokeColour.withAlpha(strokeColour.getFloatAlpha() * opacity);
        
        float strokeWidth = static_cast<float>(static_cast<double>(rectData.getProperty("strokeWeight", 1.0))) * scaling;
        
        g.setColour(strokeColour);
        
        if (cornerRadius > 0.0f)
            g.drawRoundedRectangle(rect, cornerRadius, strokeWidth);
        else
            g.drawRect(rect, strokeWidth);
    }
}

void LayoutManager::paintLine(juce::Graphics& g, const juce::ValueTree& lineData)
{
    // Get position and size
    float x = static_cast<float>(static_cast<double>(lineData.getProperty("x", 0.0))) * scaling;
    float y = static_cast<float>(static_cast<double>(lineData.getProperty("y", 0.0))) * scaling;
    float width = static_cast<float>(static_cast<double>(lineData.getProperty("width", 0.0))) * scaling;
    float height = static_cast<float>(static_cast<double>(lineData.getProperty("height", 0.0))) * scaling;
    
    float opacity = static_cast<float>(static_cast<double>(lineData.getProperty("opacity", 1.0)));
    float strokeWidth = static_cast<float>(static_cast<double>(lineData.getProperty("strokeWeight", 1.0))) * scaling;
    
    // Get stroke color
    juce::Colour strokeColour = juce::Colours::black;   
    if (lineData.hasProperty("strokeColour"))
    {
        juce::String colourStr = lineData.getProperty("strokeColour").toString();
        strokeColour = juce::Colour::fromString(colourStr);
    }
    
    strokeColour = strokeColour.withAlpha(strokeColour.getFloatAlpha() * opacity);
    g.setColour(strokeColour);
    
    // Draw line from (x,y) to (x+width, y+height)
    juce::Line<float> line(x, y, x + width, y + height);
    g.drawLine(line, strokeWidth);
}

void LayoutManager::paintAutoLabel(juce::Graphics& g, const juce::ValueTree& labelData)
{
    // Get position and size
    float x = static_cast<float>(static_cast<double>(labelData.getProperty("x", 0.0))) * scaling;
    float y = static_cast<float>(static_cast<double>(labelData.getProperty("y", 0.0))) * scaling;
    float width = static_cast<float>(static_cast<double>(labelData.getProperty("width", 0.0))) * scaling;
    float height = static_cast<float>(static_cast<double>(labelData.getProperty("height", 0.0))) * scaling;
    
    float opacity = static_cast<float>(static_cast<double>(labelData.getProperty("opacity", 1.0)));
    
    // Get text content
    juce::String text;
    if (labelData.hasProperty("text"))
    {
        text = labelData.getProperty("text").toString();
    }
    
    if (text.isEmpty())
        return;
    
    // Create font from element using existing method
    juce::Font font = fontWithDecoration(createFontFromElement(labelData), labelData);
    g.setFont(font);
    
    // Get text color from fillColour
    juce::Colour textColour = juce::Colours::black;
    if (labelData.hasProperty("fillColour"))
    {
        juce::String colourStr = labelData.getProperty("fillColour").toString();
        textColour = juce::Colour::fromString(colourStr);
    }
    
    textColour = textColour.withAlpha(textColour.getFloatAlpha() * opacity);
    g.setColour(textColour);
    
    // Determine justification
    int horizontalFlags = juce::Justification::left;
    int verticalFlags = juce::Justification::verticallyCentred;
    
    // Handle horizontal alignment
    if (labelData.hasProperty("textAlignHorizontal"))
    {
        juce::String textAlignH = labelData.getProperty("textAlignHorizontal").toString();
        if (textAlignH.equalsIgnoreCase("left"))
            horizontalFlags = juce::Justification::left;
        else if (textAlignH.equalsIgnoreCase("center"))
            horizontalFlags = juce::Justification::horizontallyCentred;
        else if (textAlignH.equalsIgnoreCase("right"))
            horizontalFlags = juce::Justification::right;
    }
    
    // Handle vertical alignment
    if (labelData.hasProperty("textAlignVertical"))
    {
        juce::String textAlignV = labelData.getProperty("textAlignVertical").toString();
        if (textAlignV.equalsIgnoreCase("top"))
            verticalFlags = juce::Justification::top;
        else if (textAlignV.equalsIgnoreCase("center"))
            verticalFlags = juce::Justification::verticallyCentred;
        else if (textAlignV.equalsIgnoreCase("bottom"))
            verticalFlags = juce::Justification::bottom;
    }
    
    juce::Justification justification(horizontalFlags | verticalFlags);
    


    // Draw the text
    juce::Rectangle<int> textBounds(x, y, width, height);
    // max 32 lines, arbitrary value
    g.drawFittedText(text, textBounds, justification, 32, 1.0);
}