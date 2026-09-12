#include "Controls.h"
#include <cmath>
#include <cstdlib>

namespace
{
class LMNumericEditor final : public juce::Component
{
public:
    LMNumericEditor (const juce::String& name, double value, double minimum, double maximum,
                     std::function<bool (const juce::String&)> submit)
    {
        title.setText (name + " [" + juce::String (minimum) + " to " + juce::String (maximum) + "]",
                       juce::dontSendNotification);
        title.setColour (juce::Label::textColourId, juce::Colour (0xffc993ec));
        addAndMakeVisible (title);
        input.setText (juce::String (value, 3));
        input.setSelectAllWhenFocused (true);
        input.onReturnKey = [this, submit] { if (submit (input.getText())) close(); };
        input.onEscapeKey = [this] { close(); };
        addAndMakeVisible (input);
        apply.setButtonText ("Apply");
        apply.onClick = input.onReturnKey;
        addAndMakeVisible (apply);
        setSize (220, 94);
    }
    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        title.setBounds (area.removeFromTop (22));
        area.removeFromTop (4);
        input.setBounds (area.removeFromTop (24));
        apply.setBounds (area.removeFromBottom (22).removeFromRight (64));
    }
private:
    void close()
    {
        if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
            callout->dismiss();
    }
    juce::Label title;
    juce::TextEditor input;
    juce::TextButton apply;
};
}

LMSlider::LMSlider()
{
    setWantsKeyboardFocus (true);
    setPopupMenuEnabled (false);
}

LMSlider::~LMSlider() { cancelInteraction(); }

void LMSlider::setValueDisplayEnabled (bool enabled)
{
    setPopupDisplayEnabled (enabled, enabled, getTopLevelComponent(), 2000);
}

bool LMSlider::commitNumericText (const juce::String& text)
{
    const auto trimmed = text.trim();
    char* end = nullptr;
    const auto value = std::strtod (trimmed.toRawUTF8(), &end);
    if (trimmed.isEmpty() || end == trimmed.toRawUTF8() || *end != '\0' || ! std::isfinite (value))
        return false;
    const auto legal = getNormalisableRange().snapToLegalValue (juce::jlimit (getMinimum(), getMaximum(), value));
    const ScopedDragNotification gesture (*this);
    setValue (legal, juce::sendNotificationSync);
    return true;
}

void LMSlider::cancelInteraction()
{
    ++interactionRevision;
    if (dragEvent.has_value())
    {
        juce::Slider::mouseUp (*dragEvent);
        dragEvent.reset();
    }
    if (valueEditor != nullptr)
        valueEditor->dismiss();
}

void LMSlider::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
    {
        dragEvent.emplace (e);
        juce::Slider::mouseDown (e);
        return;
    }
    juce::PopupMenu menu;
    menu.addItem (1, "Enter value...");
    menu.addItem (2, "Reset to default", isDoubleClickReturnEnabled());
    const juce::Component::SafePointer<LMSlider> safe (this);
    const auto revision = interactionRevision;
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe, revision] (int result)
    {
        if (safe == nullptr || safe->interactionRevision != revision) return;
        if (result == 1) safe->showValueEditor();
        else if (result == 2) safe->commitNumericText (juce::String (safe->getDoubleClickReturnValue(), 8));
    });
}

void LMSlider::mouseDrag (const juce::MouseEvent& e)
{
    // JUCE retains its drag coordinates after mouseUp; ignore captured events
    // that arrive after a consumer has cancelled and rebound this slider.
    if (dragEvent.has_value()) juce::Slider::mouseDrag (e);
}

void LMSlider::mouseUp (const juce::MouseEvent& e)
{
    if (dragEvent.has_value()) juce::Slider::mouseUp (e);
    dragEvent.reset();
}

bool LMSlider::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::returnKey)
    {
        showValueEditor();
        return true;
    }
    const auto code = key.getKeyCode();
    if (code == juce::KeyPress::leftKey || code == juce::KeyPress::downKey
        || code == juce::KeyPress::rightKey || code == juce::KeyPress::upKey)
    {
        const auto direction = code == juce::KeyPress::leftKey || code == juce::KeyPress::downKey ? -1.0 : 1.0;
        const auto step = getInterval() > 0 ? getInterval() : (getMaximum() - getMinimum()) / 100.0;
        return commitNumericText (juce::String (getValue() + direction * step, 8));
    }
    return juce::Slider::keyPressed (key);
}

void LMSlider::showValueEditor()
{
    const juce::Component::SafePointer<LMSlider> safe (this);
    const auto revision = interactionRevision;
    auto editor = std::make_unique<LMNumericEditor> (getName(), getValue(), getMinimum(), getMaximum(), [safe, revision] (const juce::String& text)
    {
        return safe != nullptr && safe->interactionRevision == revision && safe->commitNumericText (text);
    });
    valueEditor = &juce::CallOutBox::launchAsynchronously (std::move (editor), getScreenBounds(), nullptr);
}

LMAhdsrComponent::LMAhdsrComponent()
{
    for (auto& range : ranges) range = { 0, 5000, 1, 0.3f };
    ranges[sustain] = { 0, 1, 0.01f };
    setWantsKeyboardFocus (true);
    setTitle ("AHDSR envelope");
    setDescription ("Drag envelope points. Left and right select a stage; up and down adjust it.");
}

LMAhdsrComponent::~LMAhdsrComponent() { cancelGesture(); }

void LMAhdsrComponent::setStyle (const juce::ValueTree& tree, float newScale)
{
    style = tree;
    scale = newScale;
    lineEffects.setSource (style, "line");
    repaint();
}

void LMAhdsrComponent::setRanges (const Ranges& newRanges)
{
    cancelGesture();
    ranges = newRanges;
    setValues (values);
}

void LMAhdsrComponent::setValues (const Values& newValues)
{
    for (int i = 0; i < stageCount; ++i)
    {
        const auto& range = ranges[(size_t) i];
        const auto value = std::isfinite (newValues[(size_t) i]) ? newValues[(size_t) i] : range.start;
        values[(size_t) i] = range.snapToLegalValue (juce::jlimit (range.start, range.end, value));
    }
    repaint();
}

void LMAhdsrComponent::setValue (Stage stage, float value)
{
    auto next = values;
    next[(size_t) stage] = value;
    setValues (next);
}

float LMAhdsrComponent::number (const char* key, float fallback) const
{
    const auto value = (float) style.getProperty (key, fallback);
    return std::isfinite (value) ? value : fallback;
}

juce::Colour LMAhdsrComponent::colour (const char* key, const char* fallback) const
{
    return juce::Colour::fromString (style.getProperty (key, fallback).toString());
}

juce::Rectangle<float> LMAhdsrComponent::plotBounds() const
{
    return getLocalBounds().toFloat().reduced (number ("padding", 10) * scale);
}

float LMAhdsrComponent::timeScale() const
{
    if (activeHandle >= 0) return frozenTimeScale;
    const auto total = values[attack] + values[hold] + values[decay] + values[release];
    const auto fraction = juce::jlimit (0.05f, 0.8f, number ("sustainWidth", 0.2f));
    return plotBounds().getWidth() * (1.0f - fraction) / juce::jmax (number ("minimumTimeSpan", 100), total);
}

std::array<juce::Point<float>, LMAhdsrComponent::stageCount> LMAhdsrComponent::endpoints() const
{
    const auto plot = plotBounds();
    const auto unit = timeScale();
    const auto peak = plot.getY() + number ("peakInset", 0) * scale;
    const auto level = plot.getBottom() - values[sustain] * (plot.getBottom() - peak);
    const auto a = plot.getX() + values[attack] * unit;
    const auto h = a + values[hold] * unit;
    const auto d = h + values[decay] * unit;
    const auto s = d + plot.getWidth() * juce::jlimit (0.05f, 0.8f, number ("sustainWidth", 0.2f));
    return {{ { a, peak }, { h, peak }, { d, level }, { s, level }, { s + values[release] * unit, plot.getBottom() } }};
}

std::array<juce::Point<float>, LMAhdsrComponent::stageCount> LMAhdsrComponent::getHandlePositions() const
{
    auto points = endpoints();
    const auto plot = plotBounds();
    if (plot.isEmpty()) return points;
    const auto spacing = number ("handleSpacing", 10) * scale;
    const auto handleArea = getLocalBounds().toFloat().reduced (number ("handleRadius", 3.7f) * scale);
    for (size_t i = 0; i < points.size(); ++i)
    {
        points[i].x = juce::jlimit (plot.getX(), plot.getRight(), points[i].x);
        const auto original = points[i];
        auto available = [&] (juce::Point<float> candidate)
        {
            if (! handleArea.contains (candidate)) return false;
            for (size_t j = 0; j < i; ++j)
                if (candidate.getDistanceFrom (points[j]) < spacing * 0.95f) return false;
            return true;
        };
        if (available (original)) continue;
        bool found = false;
        for (int distance = 1; distance <= stageCount && ! found; ++distance)
            for (const auto offset : { juce::Point<float> (1, 0), { -1, 0 }, { 0, 1 }, { 0, -1 } })
            {
                const auto candidate = original + offset * ((float) distance * spacing);
                if (available (candidate))
                {
                    points[i] = candidate;
                    found = true;
                    break;
                }
            }
    }
    return points;
}

void LMAhdsrComponent::paint (juce::Graphics& g)
{
    const auto plot = plotBounds();
    if (plot.isEmpty()) return;
    const auto points = endpoints();
    g.setColour (colour ("guideColour", "ff2d2d2d"));
    for (int i = 0; i <= stageCount; ++i)
        g.drawVerticalLine (juce::roundToInt (plot.getX() + plot.getWidth() * (float) i / stageCount),
                            plot.getY() - 4 * scale, plot.getBottom() + 4 * scale);
    juce::Path line;
    line.startNewSubPath (plot.getBottomLeft());
    for (const auto& point : points) line.lineTo (point);
    auto fill = line;
    fill.lineTo (points.back().x, plot.getBottom());
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (colour ("fillTopColour", "90c993ec"), plot.getTopLeft(),
                                            colour ("fillBottomColour", "00c993ec"), plot.getBottomLeft(), false));
    g.fillPath (fill);
    const juce::PathStrokeType stroke (number ("lineWidth", 1.5f) * scale);
    lineEffects.render (g, line, stroke, scale);
    g.setColour (colour ("lineColour", "ffc993ec"));
    g.strokePath (line, stroke);
    const auto handles = getHandlePositions();
    const auto radius = number ("handleRadius", 3.7f) * scale;
    for (size_t i = 0; i < handles.size(); ++i)
    {
        // The two peak handles are always visible, as in the reference.
        if (i > 1 && ! isMouseOverOrDragging() && ! hasKeyboardFocus (true)) continue;
        g.setColour (colour ("handleColour", "ffc993ec"));
        if (handles[i] != points[i]) g.drawLine ({ handles[i], points[i] }, 0.5f * scale);
        g.fillEllipse (handles[i].x - radius, handles[i].y - radius, radius * 2, radius * 2);
        if (hasKeyboardFocus (true) && focusedHandle == (int) i)
            g.drawEllipse (handles[i].x - radius - 2 * scale, handles[i].y - radius - 2 * scale,
                           radius * 2 + 4 * scale, radius * 2 + 4 * scale, scale);
    }
}

void LMAhdsrComponent::mouseDown (const juce::MouseEvent& event)
{
    cancelGesture();
    const auto handles = getHandlePositions();
    float distance = number ("hitRadius", 10) * scale;
    int closest = -1;
    for (int i = 0; i < stageCount; ++i)
        if (const auto d = event.position.getDistanceFrom (handles[(size_t) i]); d < distance)
        {
            closest = i;
            distance = d;
        }
    if (closest < 0 || event.mods.isPopupMenu()) return;
    frozenTimeScale = timeScale();
    activeHandle = focusedHandle = closest;
    dragValues = values;
    dragOrigin = event.position;
    if (onGestureStart)
    {
        onGestureStart ((Stage) activeHandle);
        if (activeHandle == decay) onGestureStart (sustain);
    }
    repaint();
}

void LMAhdsrComponent::editValue (Stage stage, float value)
{
    setValue (stage, value);
    if (onValueChange) onValueChange (stage, values[(size_t) stage]);
}

void LMAhdsrComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (activeHandle < 0) return;
    const auto delta = event.position - dragOrigin;
    if (activeHandle != sustain)
        editValue ((Stage) activeHandle, dragValues[(size_t) activeHandle] + delta.x / juce::jmax (0.001f, frozenTimeScale));
    if (activeHandle == sustain || activeHandle == decay)
        editValue (sustain, dragValues[sustain] - delta.y / juce::jmax (1.0f, plotBounds().getHeight() - number ("peakInset", 0) * scale));
}

void LMAhdsrComponent::mouseUp (const juce::MouseEvent&) { cancelGesture(); }

void LMAhdsrComponent::cancelGesture()
{
    const auto previous = activeHandle;
    activeHandle = -1;
    if (previous >= 0 && onGestureEnd)
    {
        onGestureEnd ((Stage) previous);
        if (previous == decay) onGestureEnd (sustain);
    }
    repaint();
}

bool LMAhdsrComponent::keyPressed (const juce::KeyPress& key)
{
    const auto code = key.getKeyCode();
    if (code == juce::KeyPress::leftKey || code == juce::KeyPress::rightKey)
    {
        focusedHandle = (focusedHandle + (code == juce::KeyPress::leftKey ? stageCount - 1 : 1)) % stageCount;
        repaint();
        return true;
    }
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey)
    {
        const auto stage = (Stage) focusedHandle;
        const auto& range = ranges[(size_t) stage];
        const auto normal = range.convertTo0to1 (values[(size_t) stage]);
        const auto delta = code == juce::KeyPress::upKey ? 0.01f : -0.01f;
        if (onGestureStart) onGestureStart (stage);
        editValue (stage, range.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, normal + delta)));
        if (onGestureEnd) onGestureEnd (stage);
        return true;
    }
    return false;
}

namespace
{
void tintSvg (juce::Component& component, juce::Colour colour)
{
    if (auto* shape = dynamic_cast<juce::DrawableShape*> (&component))
    {
        const auto fill = shape->getFill();
        const auto stroke = shape->getStrokeFill();
        if (! fill.isInvisible()) shape->setFill (juce::FillType (colour.withMultipliedAlpha (fill.getOpacity())));
        if (! stroke.isInvisible()) shape->setStrokeFill (juce::FillType (colour.withMultipliedAlpha (stroke.getOpacity())));
    }
    for (auto* child : component.getChildren()) tintSvg (*child, colour);
}
}

LMSvgButton::LMSvgButton() : juce::Button ("SVG button")
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void LMSvgButton::setStyle (const juce::ValueTree& tree, const juce::File& assetsFolder, float newScale)
{
    style = tree;
    scale = newScale;
    circleEffects.setSource (style, "circle");
    selectedEffects.setSource (style, "selected");
    const auto source = assetsFolder.getChildFile (style.getProperty ("svg").toString()).loadFileAsString();
    const auto base = style.getProperty ("colorOverride").toString();
    const auto selected = style.getProperty ("colorOverrideSelected", base).toString();
    const std::array<juce::String, 4> colours { base,
        style.getProperty ("colorOverrideHover", base).toString(),
        style.getProperty ("colorOverrideClick", selected).toString(), selected };
    juce::String key;
    for (const auto& colour : colours) key += colour + ";";
    if (source != cachedArtwork || key != cachedColours)
    {
        cachedArtwork = source;
        cachedColours = key;
        for (size_t i = 0; i < artwork.size(); ++i)
        {
            auto xml = juce::parseXML (source);
            artwork[i] = xml != nullptr ? juce::Drawable::createFromSVG (*xml) : nullptr;
            if (artwork[i] != nullptr && colours[i].isNotEmpty())
                tintSvg (*artwork[i], juce::Colour::fromString (colours[i].trimCharactersAtStart ("#")));
        }
    }
    repaint();
}

void LMSvgButton::paintButton (juce::Graphics& g, bool hover, bool down)
{
    const bool selected = getToggleState();
    const auto number = [this] (const char* key, float fallback)
    { return (float) style.getProperty (key, fallback) * scale; };
    const auto colour = [this] (const char* key, const juce::String& fallback)
    { return juce::Colour::fromString (style.getProperty (key, fallback).toString().trimCharactersAtStart ("#")); };
    const auto area = getLocalBounds().toFloat();
    if (style.getProperty ("backgroundCircle", false))
    {
        const auto diameter = number ("circleDiameter", juce::jmin (getWidth(), getHeight()) / scale);
        juce::Path circle;
        circle.addEllipse (juce::Rectangle<float> (diameter, diameter).withCentre (area.getCentre()));
        const auto base = style.getProperty ("circleColour", "00000000").toString();
        const auto fill = selected ? colour ("circleColourSelected", base)
                         : down ? colour ("circleColourClick", base)
                         : hover ? colour ("circleColourHover", base) : colour ("circleColour", base);
        if (! fill.isTransparent())
        {
            auto& effects = selected ? selectedEffects : circleEffects;
            effects.render (g, circle, scale);
            g.setColour (fill);
            g.fillPath (circle);
            effects.renderInner (g, circle, scale);
        }
    }
    const auto index = down ? 2 : selected ? 3 : hover ? 1 : 0;
    if (auto* icon = artwork[(size_t) index].get())
    {
        const auto size = number ("iconSize", 24);
        icon->drawWithin (g, juce::Rectangle<float> (size, size).withCentre (area.getCentre()),
                          juce::RectanglePlacement::centred, isEnabled() ? 1.0f : 0.4f);
    }
}
