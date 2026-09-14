#include "LMLookAndFeel.h"
#include <cmath>

namespace lmSliderStyle
{
float number (const juce::ValueTree& tree, const char* key, float fallback)
{
    const auto value = (float) tree.getProperty (key, fallback);
    return std::isfinite (value) ? value : fallback;
}

juce::Colour colour (const juce::ValueTree& tree, const char* key, const char* fallback)
{
    return juce::Colour::fromString (tree.getProperty (key, fallback).toString());
}
}

bool LMLookAndFeel::drawSymbolSlider (juce::Graphics& g, float position)
{
    using namespace lmSliderStyle;
    const auto style = sliderWidgetStyle.getProperty ("visualStyle", "arc").toString();
    if (style == "arc")
        return false;

    const auto area = sliderLayout.sliderBounds.toFloat();
    const auto centre = area.getCentre();
    const auto radius = juce::jmax (0.0f, juce::jmin (area.getWidth(), area.getHeight()) * 0.5f
                                       - number (sliderWidgetStyle, "padding", 2.0f) * effectScale);
    if (radius <= 0.0f)
        return true;
    position = juce::jlimit (0.0f, 1.0f, position);
    juce::Path circle;
    circle.addEllipse (centre.x - radius, centre.y - radius, radius * 2, radius * 2);
    circleEffects.render (g, circle, effectScale);
    g.setColour (colour (sliderWidgetStyle, "circleFillColour", "ff1c1c1c"));
    g.fillPath (circle);
    circleEffects.renderInner (g, circle, effectScale);
    const auto rimWidth = number (sliderWidgetStyle, "circleStrokeWidth", 0.0f) * effectScale;
    if (rimWidth > 0)
    {
        g.setColour (colour (sliderWidgetStyle, "circleStrokeColour", "ff292929"));
        g.strokePath (circle, juce::PathStrokeType (rimWidth));
    }

    juce::Path symbol;
    const auto r = radius * number (sliderWidgetStyle, "symbolRadius", 0.78f);
    const auto pi = juce::MathConstants<float>::pi;
    auto symbolWidth = number (sliderWidgetStyle, "symbolWidth", 1.5f) * effectScale;
    if (style == "bipolarSector")
    {
        const auto angle = (position * 2.0f - 1.0f)
            * juce::degreesToRadians (number (sliderWidgetStyle, "maximumSweep", 360.0f));
        symbol.startNewSubPath (centre);
        symbol.lineTo (centre.x, centre.y - r);
        if (std::abs (angle) > 0.00001f)
        {
            symbol.addCentredArc (centre.x, centre.y, r, r, 0, 0, angle, false);
            symbol.closeSubPath();
        }
    }
    else if (style == "sector")
    {
        // An outlined quadrant rotates around its vertex with bipolar pitch.
        symbol.startNewSubPath (0, 0);
        symbol.lineTo (-r, 0);
        symbol.addCentredArc (0, 0, r, r, 0, -pi * 0.5f, 0, false);
        symbol.closeSubPath();
        symbol.applyTransform (juce::AffineTransform::rotation ((position - 0.5f)
                                    * juce::degreesToRadians (number (sliderWidgetStyle, "rotationRange", 180.0f)))
                                    .translated (centre.x, centre.y));
    }
    else if (style == "pan")
    {
        // Anchor the radial edge on the panned side and close the other edge
        // towards it, collapsing the 270-degree outline to one diagonal.
        const auto start = position <= 0.5f ? -pi * 0.75f : juce::jmap (position, 0.5f, 1.0f, -pi * 0.75f, pi * 0.75f);
        const auto end = position >= 0.5f ? pi * 0.75f : juce::jmap (position, 0.0f, 0.5f, -pi * 0.75f, pi * 0.75f);
        symbol.startNewSubPath (centre);
        symbol.lineTo (centre.x + std::sin (start) * r, centre.y - std::cos (start) * r);
        if (end > start)
        {
            symbol.addCentredArc (centre.x, centre.y, r, r, 0, start, end, false);
            symbol.closeSubPath();
        }
    }
    else if (style == "dome")
    {
        const auto start = -pi * juce::jmap (position, number (sliderWidgetStyle, "minimumOpening", 0.60f),
                                                               number (sliderWidgetStyle, "maximumOpening", 0.75f));
        const auto end = -start;
        symbol.addCentredArc (0, 0, r, r, 0, start, end, true);
        symbol.lineTo (0, -r * position * number (sliderWidgetStyle, "maximumNotchDepth", 0.18f));
        symbol.closeSubPath();
        symbol.applyTransform (juce::AffineTransform::translation (centre.x, centre.y));
    }
    else if (style == "ring")
    {
        const auto minRadius = number (sliderWidgetStyle, "minimumRadius", 0.12f);
        const auto maxRadius = number (sliderWidgetStyle, "maximumRadius", 0.5f);
        symbolWidth = juce::jmap (position, number (sliderWidgetStyle, "minimumSymbolWidth", symbolWidth / effectScale)
                                              * effectScale, symbolWidth);
        const auto ringRadius = juce::jlimit (0.0f, juce::jmax (0.0f, radius - symbolWidth * 0.5f),
                                              radius * juce::jmap (position, minRadius, maxRadius));
        symbol.addEllipse (centre.x - ringRadius, centre.y - ringRadius, ringRadius * 2, ringRadius * 2);
    }
    else if (style == "svgMorph")
    {
        symbol = svgInterpolator.createPath (position);
        symbol.applyTransform (juce::AffineTransform::scale (r).translated (centre.x, centre.y));
    }
    else if (style == "jaggedRing")
    {
        const auto teeth = juce::jmax (3, (int) sliderWidgetStyle.getProperty ("teeth", 16));
        const auto depth = juce::jmap (position, 0.025f,
                                      number (sliderWidgetStyle, "maximumDepth", 0.3f));
        for (int i = 0; i < teeth * 2; ++i)
        {
            const auto angle = (float) i * pi / (float) teeth;
            const auto irregularity = number (sliderWidgetStyle, "irregularity", 0.12f) * position
                                       * std::sin ((float) i * 2.39996f);
            const auto length = r * (i % 2 == 0 ? 1.0f + irregularity : 1.0f - depth);
            const juce::Point<float> point { centre.x + std::sin (angle) * length,
                                             centre.y - std::cos (angle) * length };
            if (i == 0) symbol.startNewSubPath (point);
            else symbol.lineTo (point);
        }
        symbol.closeSubPath();
    }
    const juce::PathStrokeType stroke (symbolWidth,
                                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    symbolEffects.render (g, symbol, stroke, effectScale);
    g.setColour (colour (sliderWidgetStyle, "symbolColour", "ffc993ec"));
    g.strokePath (symbol, stroke);
    return true;
}

void LMLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                     float sliderPos, float minSliderPos, float maxSliderPos,
                                     juce::Slider::SliderStyle style, juce::Slider& slider)
{
    using namespace lmSliderStyle;
    if (! sliderWidgetStyle.isValid() || style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                               minSliderPos, maxSliderPos, style, slider);
        return;
    }
    const auto bounds = sliderLayout.sliderBounds.toFloat();
    const auto middle = bounds.getCentreX();
    const auto thumbY = juce::jlimit (bounds.getY(), bounds.getBottom(), sliderPos);
    juce::Path track;
    track.startNewSubPath (middle, bounds.getY());
    track.lineTo (middle, bounds.getBottom());
    const juce::PathStrokeType trackStroke (number (sliderWidgetStyle, "trackWidth", 0.7f) * effectScale);
    trackEffects.render (g, track, trackStroke, effectScale);
    g.setColour (colour (sliderWidgetStyle, "trackColour", "ffc993ec"));
    g.strokePath (track, trackStroke);

    juce::Path thumb;
    const bool bar = sliderWidgetStyle.getProperty ("visualStyle", "bar").toString() == "bar";
    if (bar)
    {
        const auto thickness = number (sliderWidgetStyle, "activeWidth", 4.0f) * effectScale;
        thumb.addRoundedRectangle (middle - thickness * 0.5f, thumbY - thickness * 0.5f,
                                   thickness, bounds.getBottom() - thumbY + thickness,
                                   thickness * 0.5f);
    }
    else
    {
        const auto radius = number (sliderWidgetStyle, "thumbRadius", 4.0f) * effectScale;
        thumb.addEllipse (middle - radius, thumbY - radius, radius * 2, radius * 2);
    }
    thumbEffects.render (g, thumb, effectScale);
    g.setColour (colour (sliderWidgetStyle, "thumbColour", "ffc993ec"));
    g.fillPath (thumb);
}
