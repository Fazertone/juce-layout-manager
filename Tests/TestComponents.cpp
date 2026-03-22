#include <JuceHeader.h>
#include "TestComponents.h"
#include "BinaryData.h"
#include "juce_core/juce_core.h"

// MockProcessor implementation
MockProcessor::MockProcessor() 
    : AudioProcessor (BusesProperties()),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MockProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Add parameters you plan to reference in your XML
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "gain", "Gain", 0.0f, 1.0f, 0.5f));
        
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "bypass", "Bypass", false));
        
    return { params.begin(), params.end() };
}

void MockProcessor::prepareToPlay (double, int)
{
}

void MockProcessor::releaseResources()
{
}

void MockProcessor::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
}

bool MockProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MockProcessor::createEditor()
{
    return nullptr;
}

const juce::String MockProcessor::getName() const
{
    return "MockProcessor";
}

double MockProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

bool MockProcessor::acceptsMidi() const
{
    return false;
}

bool MockProcessor::producesMidi() const
{
    return false;
}

int MockProcessor::getNumPrograms()
{
    return 1;
}

int MockProcessor::getCurrentProgram()
{
    return 0;
}

void MockProcessor::setCurrentProgram (int)
{
}

const juce::String MockProcessor::getProgramName (int)
{
    return {};
}

void MockProcessor::changeProgramName (int, const juce::String&)
{
}

void MockProcessor::getStateInformation (juce::MemoryBlock&)
{
}

void MockProcessor::setStateInformation (const void*, int)
{
}

// TopMenuComponent implementation
TopMenuComponent::TopMenuComponent(LayoutManager* layoutManager) 
    : layoutManager(layoutManager)
{
    this->layoutManager->registerComponent(this, "TopMenu");
    this->layoutManager->registerComponent(&on_btn, "on_text_btn");
    this->layoutManager->registerComponent(&savePresetImageBtn, "save_preset_img_btn");
    this->layoutManager->registerComponent(&safari_logo_img, "safari_logo_img");
    this->layoutManager->registerComponent(&input_level_rotary_slider, "input_level_rotary_slider");

    // // set image
    // Image savePresetImage = ImageCache::getFromMemory(BinaryData::save_preset_img_btn_png, BinaryData::save_preset_img_btn_pngSize);
    // savePresetImageBtn.setImages(false, true, true, savePresetImage, 1.0f, {}, savePresetImage, 1.0f, {}, savePresetImage, 1.0f, {});

    addAndMakeVisible(on_btn);
    addAndMakeVisible(savePresetImageBtn);
    addAndMakeVisible(safari_logo_img);
    addAndMakeVisible(input_level_rotary_slider);

}

void TopMenuComponent::paint(juce::Graphics& g)
{
    this->layoutManager->paintComponent(g, this->getComponentID());
}

// MainComponent implementation
MainComponent::MainComponent()
{
    loadLayout();
    layoutManager->registerComponent(this, "main_component");

    topMenuComponent = std::make_unique<TopMenuComponent>(layoutManager.get());
    
    addAndMakeVisible(topMenuComponent.get());

    setSize(1200, 647);

    auto ui_state = Inspector::getComponentTree(this);
    juce::File outputFile("~/Code/plugins/JuceGuiMaker/Tests/output/FullUIState.json");
    outputFile.create();
    auto outputStream = juce::FileOutputStream(outputFile);
    juce::JSON::writeToStream(outputStream, ui_state);

}

void MainComponent::loadLayout()
{
    // Load file from disk so you don't have to recompile when changing XML
    // In dev, hardcoding the absolute path is often the easiest way 
    // to ensure it finds the file while you edit it in VS Code.
    juce::File xmlFile("~/Code/plugins/JuceGuiMaker/Tests/data/xml/MeawChainerFrame.xml");
    
    if (xmlFile.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(xmlFile);
        if (xml != nullptr)
        {
            // Use the XmlElement constructor
            layoutManager = std::make_unique<LayoutManager>(*xml);
            layoutManager->assetsFolder = juce::File("~/Code/plugins/JuceGuiMaker/tests/data");
            resized(); // Force update
        }
    } else {
        // throw error and jassert
        jassertfalse;
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    layoutManager->paintComponent(g, this->getComponentID());
}

void MainComponent::resized()
{
    layoutManager->applyResize();

    // Keep our reload button on top
    reloadBtn.toFront(false);
}
