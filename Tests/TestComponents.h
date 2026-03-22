#pragma once

#include <JuceHeader.h>

// Forward declaration
class LayoutManager;

class MockProcessor : public juce::AudioProcessor
{
public:
    // Create a few dummy parameters that match your XML test cases
    juce::AudioProcessorValueTreeState apvts;

    MockProcessor();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Boilerplate overrides required by pure virtual functions
    void prepareToPlay (double, int) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    // Crucial: The editor logic
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;
    
    // Required pure virtual function implementations
    const juce::String getName() const override;
    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
};

class TopMenuComponent : public juce::Component
{
public:
    TopMenuComponent(LayoutManager* layoutManager);

    void paint(juce::Graphics& g) override;

    juce::TextButton on_btn;
    juce::ImageButton savePresetImageBtn;
    ImageHolder safari_logo_img;

    juce::Slider input_level_rotary_slider;

private:
    LayoutManager* layoutManager;
};

class MainComponent : public juce::Component
{
public:
    MainComponent();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void loadLayout();

    MockProcessor mockProcessor;
    std::unique_ptr<LayoutManager> layoutManager;
    std::unique_ptr<TopMenuComponent> topMenuComponent;
    juce::TextButton reloadBtn;
};
