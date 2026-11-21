#include "Primitives.h"

namespace
{
float prop (const juce::ValueTree& node, const char* name, float fallback)
{
    const auto value = node.getProperty (name);
    return value.isVoid() ? fallback : (float) value;
}

juce::Colour colourProp (const juce::ValueTree& node, const char* name, juce::Colour fallback)
{
    return node.hasProperty (name) ? juce::Colour::fromString (node.getProperty (name).toString()) : fallback;
}

constexpr float degToRad = juce::MathConstants<float>::pi / 180.0f;
} // namespace

//==============================================================================
bool LayoutManager::stateMatches (const juce::ValueTree& node, const juce::StringArray& states)
{
    const auto spec = node.getProperty ("state").toString().trim();
    if (spec.isEmpty())
        return true;
    // "a|b": any of; "!a": not a.
    for (const auto& token : juce::StringArray::fromTokens (spec, "|", ""))
    {
        const auto t = token.trim();
        if (t.startsWithChar ('!') ? ! states.contains (t.substring (1)) : states.contains (t))
            return true;
    }
    return false;
}

bool LayoutManager::hasEffects (const juce::ValueTree& node)
{
    for (const auto& child : node)
        if (child.hasType ("Effects") && child.getProperty ("target").toString().isEmpty() && child.getNumChildren() > 0)
            return true;
    return false;
}

void LayoutManager::paintComponent (juce::Graphics& g, const juce::String& componentID, const LMPaintContext& context)
{
    const auto node = findComponentByName (componentID, layoutTree);
    if (node.isValid())
        paintChildren (g, node, context);
}

void LayoutManager::paintChildren (juce::Graphics& g, const juce::ValueTree& node, const LMPaintContext& context)
{
    for (const auto& child : node)
        paintElement (g, child, context);
}

void LayoutManager::paintElement (juce::Graphics& g, const juce::ValueTree& node, const LMPaintContext& context)
{
    if (! stateMatches (node, context.states) || (bool) node.getProperty ("hidden", false))
        return;
    const auto type = node.getType().toString();
    if (type == "Rectangle" || type == "Ellipse")
        paintShape (g, node);
    else if (type == "Line")
        paintLine (g, node);
    else if (type == "AutoLabel")
        paintAutoLabel (g, node);
    else if (type == "Group")
        paintGroup (g, node, context);
    else if (type == "Ticks")
        paintTicks (g, node);
    else if (type == "ConicStripes")
        paintConicStripes (g, node);
}

void LayoutManager::paintGroup (juce::Graphics& g, const juce::ValueTree& node, const LMPaintContext& context)
{
    float angle = prop (node, "rotation", 0.0f) * degToRad;
    if ((bool) node.getProperty ("rotateWithValue", false))
        angle += context.valueAngle;
    const float x = prop (node, "x", 0.0f) * scaling, y = prop (node, "y", 0.0f) * scaling;
    const float px = prop (node, "pivotX", 0.0f) * scaling, py = prop (node, "pivotY", 0.0f) * scaling;
    const float opacity = juce::jlimit (0.0f, 1.0f, prop (node, "opacity", 1.0f));
    if (opacity <= 0.0f)
        return;

    juce::Graphics::ScopedSaveState save (g);
    const float groupScale = prop (node, "scale", 1.0f);
    g.addTransform (juce::AffineTransform::rotation (angle, px, py).scaled (groupScale).translated (x, y));
    if (opacity < 1.0f)
    {
        g.beginTransparencyLayer (opacity);
        paintChildren (g, node, context);
        g.endTransparencyLayer();
    }
    else
        paintChildren (g, node, context);
}

void LayoutManager::paintTicks (juce::Graphics& g, const juce::ValueTree& node)
{
    const int count = juce::jmax (1, (int) prop (node, "count", 11.0f));
    const float start = prop (node, "startAngle", -135.0f) * degToRad;
    const float end = prop (node, "endAngle", 135.0f) * degToRad;
    const float cx = prop (node, "cx", 0.0f) * scaling, cy = prop (node, "cy", 0.0f) * scaling;
    const float outer = prop (node, "outerRadius", 50.0f) * scaling;
    const float length = prop (node, "length", 6.0f) * scaling;
    const int majorEvery = (int) prop (node, "majorEvery", 0.0f);
    const float majorLength = prop (node, "majorLength", prop (node, "length", 6.0f)) * scaling;
    const float width = prop (node, "strokeWeight", 1.0f) * scaling;
    const float opacity = prop (node, "opacity", 1.0f);
    g.setColour (colourProp (node, "strokeColour", juce::Colours::black).withMultipliedAlpha (opacity));
    for (int i = 0; i < count; ++i)
    {
        const float t = count > 1 ? (float) i / (float) (count - 1) : 0.0f;
        const float a = start + t * (end - start);
        const float len = majorEvery > 0 && i % majorEvery == 0 ? majorLength : length;
        juce::Path tick;
        tick.addRectangle (cx - width * 0.5f, cy - outer, width, len);
        g.fillPath (tick, juce::AffineTransform::rotation (a, cx, cy));
    }
}

void LayoutManager::paintConicStripes (juce::Graphics& g, const juce::ValueTree& node)
{
    const juce::Rectangle<float> bounds (prop (node, "x", 0.0f) * scaling, prop (node, "y", 0.0f) * scaling,
                                         prop (node, "width", 0.0f) * scaling, prop (node, "height", 0.0f) * scaling);
    if (bounds.isEmpty())
        return;
    const int segments = juce::jmax (2, (int) prop (node, "segments", 240.0f));
    const float start = prop (node, "startAngle", 0.0f) * degToRad;
    const float opacity = prop (node, "opacity", 1.0f);
    const juce::Colour colours[2] = { colourProp (node, "colourA", juce::Colours::lightgrey).withMultipliedAlpha (opacity),
                                      colourProp (node, "colourB", juce::Colours::grey).withMultipliedAlpha (opacity) };
    // Fill with A, then add every B wedge to one path (one fill call).
    g.setColour (colours[0]);
    g.fillEllipse (bounds);
    juce::Path wedges;
    const float step = juce::MathConstants<float>::twoPi / (float) segments;
    for (int i = 1; i < segments; i += 2)
        wedges.addPieSegment (bounds, start + step * (float) i, start + step * (float) (i + 1), 0.0f);
    g.setColour (colours[1]);
    g.fillPath (wedges);
}

std::optional<juce::ColourGradient> LayoutManager::createFillGradient (const juce::ValueTree& node,
                                                                        juce::Rectangle<float> rect, float opacity) const
{
    const auto type = node.getProperty ("fill-gradient-type").toString();
    if ((type != "linear" && type != "radial") || ! node.hasProperty ("fill-gradient-stops"))
        return std::nullopt;

    struct Stop { juce::Colour colour; double position; };
    std::vector<Stop> stops;
    const auto tokens = juce::StringArray::fromTokens (node.getProperty ("fill-gradient-stops").toString(), ",", "");
    for (int i = 0; i < tokens.size(); ++i)
    {
        const auto parts = juce::StringArray::fromTokens (tokens[i], ":", "");
        const double fallback = tokens.size() > 1 ? (double) i / (double) (tokens.size() - 1) : 0.0;
        stops.push_back ({ juce::Colour::fromString (parts[0].trim()).withMultipliedAlpha (opacity),
                           parts.size() > 1 ? parts[1].getDoubleValue() : fallback });
    }
    if (stops.size() < 2)
        return std::nullopt;

    juce::Point<float> from, to;
    if (type == "linear" && node.hasProperty ("fill-gradient-angle"))
    {
        // CSS linear-gradient(<angle>): 0deg points up, clockwise; the line
        // spans the box's projection onto that direction.
        const float a = (float) node.getProperty ("fill-gradient-angle") * degToRad;
        const juce::Point<float> dir (std::sin (a), -std::cos (a));
        const float half = 0.5f * (std::abs (rect.getWidth() * dir.x) + std::abs (rect.getHeight() * dir.y));
        from = rect.getCentre() - dir * half;
        to = rect.getCentre() + dir * half;
    }
    else
    {
        // Figma convention: normalised, y = 1 at the top.
        const auto start = juce::StringArray::fromTokens (node.getProperty ("fill-gradient-start", "0.5,0").toString(), ",", "");
        const auto end = juce::StringArray::fromTokens (node.getProperty ("fill-gradient-end", "0.5,1").toString(), ",", "");
        from = { rect.getX() + rect.getWidth() * start[0].getFloatValue(), rect.getY() + rect.getHeight() * (1.0f - start[1].getFloatValue()) };
        to = { rect.getX() + rect.getWidth() * end[0].getFloatValue(), rect.getY() + rect.getHeight() * (1.0f - end[1].getFloatValue()) };
    }

    juce::ColourGradient gradient (stops.front().colour, from, stops.back().colour, to, type == "radial");
    for (const auto& stop : stops)
        if (stop.position > 0.0 && stop.position < 1.0)
            gradient.addColour (stop.position, stop.colour);
    if (stops.front().position > 0.0)
        gradient.addColour (stops.front().position, stops.front().colour);
    if (stops.back().position < 1.0)
        gradient.addColour (stops.back().position, stops.back().colour);
    return gradient;
}

//==============================================================================
LMLayeredKnob::LMLayeredKnob()
{
    setSliderStyle (juce::Slider::Rotary);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
}

void LMLayeredKnob::setStyle (LayoutManager& manager, const juce::ValueTree& element)
{
    layout = &manager;
    node = element;
    const float start = prop (node, "startAngle", -135.0f) * degToRad;
    const float end = prop (node, "endAngle", 135.0f) * degToRad;
    // JUCE measures rotary angles clockwise from up and wants end > start
    // in [0, 2 pi] terms.
    setRotaryParameters (start + juce::MathConstants<float>::twoPi, end + juce::MathConstants<float>::twoPi, true);
    const auto mode = node.getProperty ("dragMode", "circular").toString();
    setSliderStyle (mode == "vertical" ? juce::Slider::RotaryVerticalDrag
                    : mode == "horizontalVertical" ? juce::Slider::RotaryHorizontalVerticalDrag
                                                   : juce::Slider::Rotary);
    if (node.hasProperty ("dragSensitivity"))
        setMouseDragSensitivity (juce::jmax (1, (int) ((float) node.getProperty ("dragSensitivity") * manager.scaling)));
    pivot = { prop (node, "pivotX", prop (node, "width", 0.0f) * 0.5f), prop (node, "pivotY", prop (node, "height", 0.0f) * 0.5f) };
    hitRadius = prop (node, "hitRadius", 0.0f);
    repaint();
}

float LMLayeredKnob::getValueAngle()
{
    const auto params = getRotaryParameters();
    const auto proportion = (float) valueToProportionOfLength (getValue());
    return params.startAngleRadians + proportion * (params.endAngleRadians - params.startAngleRadians);
}

void LMLayeredKnob::paint (juce::Graphics& g)
{
    if (layout == nullptr || ! node.isValid())
        return;
    LMPaintContext context;
    context.valueAngle = getValueAngle();
    if (isMouseOverOrDragging())
        context.states.add ("over");
    if (isMouseButtonDown())
        context.states.add ("down");
    if (! isEnabled())
        context.states.add ("disabled");
    layout->paintChildren (g, node, context);
}

bool LMLayeredKnob::hitTest (int x, int y)
{
    if (hitRadius <= 0.0f || layout == nullptr)
        return Slider::hitTest (x, y);
    const auto s = layout->scaling;
    return juce::Point<float> ((float) x, (float) y).getDistanceFrom (pivot * s) <= hitRadius * s;
}

//==============================================================================
LMStateButton::LMStateButton() : juce::Button ({})
{
    setClickingTogglesState (true);
}

void LMStateButton::setStyle (LayoutManager& manager, const juce::ValueTree& element)
{
    layout = &manager;
    node = element;
    setClickingTogglesState ((bool) node.getProperty ("toggles", true));
    const bool interactive = (bool) node.getProperty ("interactive", true);
    setInterceptsMouseClicks (interactive, interactive);
    setWantsKeyboardFocus (interactive);
    if (node.hasProperty ("title"))
        setTitle (node.getProperty ("title").toString());
    repaint();
}

void LMStateButton::paintButton (juce::Graphics& g, bool isOver, bool isDown)
{
    if (layout == nullptr || ! node.isValid())
        return;
    LMPaintContext context;
    context.states.add (getToggleState() ? "on" : "off");
    if (isOver)
        context.states.add ("over");
    if (isDown)
        context.states.add ("down");
    if (! isEnabled())
        context.states.add ("disabled");
    layout->paintChildren (g, node, context);
}

//==============================================================================
LMLayoutFileWatcher::LMLayoutFileWatcher (LayoutManager& manager, const juce::File& layoutFile,
                                          std::function<void()> callback, int intervalMs)
    : layout (manager), file (layoutFile), lastModified (layoutFile.getLastModificationTime()),
      onReload (std::move (callback))
{
    startTimer (intervalMs);
}

LMLayoutFileWatcher::~LMLayoutFileWatcher() { stopTimer(); }

void LMLayoutFileWatcher::timerCallback()
{
    const auto modified = file.getLastModificationTime();
    if (modified == lastModified)
        return;
    lastModified = modified;
    // A half-written file fails to parse; the next save retries.
    if (layout.loadFromFile (file.getFullPathName()) && onReload)
        onReload();
}
