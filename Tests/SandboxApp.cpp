#include <JuceHeader.h>
#include "TestComponents.h"

// Minimal JUCE application wrapper
class SandboxApplication : public juce::JUCEApplication
{
public:
    SandboxApplication() {}

    const juce::String getApplicationName() override { return "LayoutSandbox"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                            juce::Desktop::getInstance().getDefaultLookAndFeel()
                                .findColour(juce::ResizableWindow::backgroundColourId),
                            DocumentWindow::allButtons)
        {
            mainComponent = std::make_unique<MainComponent>();

            setUsingNativeTitleBar(true);
            setContentOwned(mainComponent.get(), false);
            setResizable(true, true);
            centreWithSize(1200, 647);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        std::unique_ptr<MainComponent> mainComponent;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(SandboxApplication)