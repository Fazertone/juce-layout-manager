#pragma once
#include "../layout_manager.h"

class Inspector
{
public:
    // 1. Serialize the UI Tree
    static juce::var getComponentTree(juce::Component* comp);

    // 2. Simulate Actions
    static void performAction(juce::Component* root, const juce::String& targetID, const juce::String& action, const juce::var& value);
};