#pragma once
#include "Inspector.h"

juce::var Inspector::getComponentTree(juce::Component* comp)
{
    juce::DynamicObject* obj = new juce::DynamicObject();
    obj->setProperty("id", comp->getComponentID());
    obj->setProperty("class", typeid(*comp).name());
    obj->setProperty("visible", comp->isVisible());
    obj->setProperty("enabled", comp->isEnabled());
    //obj->setProperty("text", getComponentText(comp)); // Helper for Label/Button text
    
    // Bounds relative to parent
    auto bounds = comp->getBounds();
    obj->setProperty("x", bounds.getX());
    obj->setProperty("y", bounds.getY());
    obj->setProperty("w", bounds.getWidth());
    obj->setProperty("h", bounds.getHeight());

    // Recursively add children
    juce::Array<juce::var> children;
    for (auto* child : comp->getChildren())
        children.add(getComponentTree(child));
    
    obj->setProperty("children", children);
    return obj;
}

void Inspector::performAction(juce::Component* root, const juce::String& targetID, const juce::String& action, const juce::var& value)
{
    auto* target = root->findChildWithID(targetID);
    if (!target) return; // Throw error

    if (action == "click" && dynamic_cast<juce::Button*>(target))
    {
        dynamic_cast<juce::Button*>(target)->triggerClick();
    }
    else if (action == "setValue" && dynamic_cast<juce::Slider*>(target))
    {
        dynamic_cast<juce::Slider*>(target)->setValue(value);
    }
    // Handle TextEditor, ComboBox, etc.
}
