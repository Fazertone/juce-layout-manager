#include "Effects.h"
#include <cmath>
#include <cstdlib>

namespace
{
float effectNumber (const juce::ValueTree& tree, const char* key, float fallback)
{
    const auto text = tree.getProperty (key, fallback).toString().trim();
    const auto utf8 = text.toRawUTF8();
    char* end = nullptr;
    const auto value = std::strtof (utf8, &end);
    return end != utf8 && *end == '\0' && std::isfinite (value) ? value : fallback;
}

float finiteEffectValue (float value, float fallback = 0.0f)
{
    return std::isfinite (value) ? value : fallback;
}

int scaledEffectValue (float value, float scale)
{
    return juce::roundToInt (value * scale);
}
}

LMEffectRenderer::~LMEffectRenderer()
{
    source.removeListener (this);
}

std::vector<LMShadowStyle> LMEffectRenderer::readEffects (const juce::ValueTree& node, const juce::String& target)
{
    std::vector<LMShadowStyle> result;
    for (const auto& group : node)
        if (group.hasType ("Effects") && group.getProperty ("target").toString() == target)
            for (const auto& effect : group)
                if (effect.hasType ("DropShadow"))
                {
                    LMShadowStyle style;
                    style.x = effectNumber (effect, "x", 0.0f);
                    style.y = effectNumber (effect, "y", 0.0f);
                    style.blur = juce::jmax (0.0f, effectNumber (effect, "blur", 0.0f));
                    style.spread = effectNumber (effect, "spread", 0.0f);
                    style.colour = juce::Colour::fromString (effect.getProperty ("colour", "ff000000").toString());
                    style.opacity = juce::jlimit (0.0f, 1.0f, effectNumber (effect, "opacity", 1.0f));
                    result.push_back (style);
                }
    return result;
}

void LMEffectRenderer::updateEffects (const std::vector<LMShadowStyle>& styles)
{
    layers.resize (styles.size());
    for (size_t i = 0; i < styles.size(); ++i)
    {
        if (layers[i] == nullptr)
            layers[i] = std::make_unique<Layer>();
        auto& style = layers[i]->style;
        style = styles[i];
        style.x = finiteEffectValue (style.x);
        style.y = finiteEffectValue (style.y);
        style.blur = juce::jmax (0.0f, finiteEffectValue (style.blur));
        style.spread = finiteEffectValue (style.spread);
        style.opacity = juce::jlimit (0.0f, 1.0f, finiteEffectValue (style.opacity, 1.0f));
    }
}

void LMEffectRenderer::setEffects (const std::vector<LMShadowStyle>& styles)
{
    source.removeListener (this);
    source = {};
    sourceTarget.clear();
    updateEffects (styles);
}

void LMEffectRenderer::setSource (const juce::ValueTree& node, const juce::String& target)
{
    if (source.isValid() && source == node && sourceTarget == target)
        return;
    source.removeListener (this);
    source = node;
    sourceTarget = target;
    source.addListener (this);
    refreshSource();
}

void LMEffectRenderer::refreshSource()
{
    updateEffects (readEffects (source, sourceTarget));
}

void LMEffectRenderer::configure (Layer& layer, float scale, float opacity, bool stroked)
{
    const auto& s = layer.style;
    layer.shadow.setRadius (scaledEffectValue (s.blur, scale));
    layer.shadow.setSpread (stroked ? 0 : scaledEffectValue (s.spread, scale));
    layer.shadow.setOffset (scaledEffectValue (s.x, scale), scaledEffectValue (s.y, scale));
    layer.shadow.setColor (s.colour.withMultipliedAlpha (s.opacity * juce::jlimit (0.0f, 1.0f, opacity)));
}

void LMEffectRenderer::render (juce::Graphics& g, const juce::Path& path, float scale, float opacity)
{
    if (path.isEmpty() || ! std::isfinite (scale) || scale <= 0.0f)
        return;
    juce::Graphics::ScopedSaveState saved (g);
    for (auto& layer : layers)
    {
        if (layer->style.opacity <= 0.0f || layer->style.colour.isTransparent() || opacity <= 0.0f)
            continue;
        if (path.getBounds().expanded ((float) scaledEffectValue (layer->style.spread, scale)).isEmpty())
            continue;
        configure (*layer, scale, opacity, false);
        layer->shadow.render (g, path);
    }
}

void LMEffectRenderer::render (juce::Graphics& g, const juce::Path& path,
                               const juce::PathStrokeType& stroke, float scale, float opacity)
{
    if (path.isEmpty() || stroke.getStrokeThickness() <= 0.0f || ! std::isfinite (scale) || scale <= 0.0f)
        return;
    juce::Graphics::ScopedSaveState saved (g);
    for (auto& layer : layers)
    {
        if (layer->style.opacity <= 0.0f || layer->style.colour.isTransparent() || opacity <= 0.0f)
            continue;
        const auto width = stroke.getStrokeThickness() + 2.0f * (float) scaledEffectValue (layer->style.spread, scale);
        if (width <= 0.0f)
            continue;
        juce::Path mask;
        juce::PathStrokeType (width, stroke.getJointStyle(), stroke.getEndStyle())
            .createStrokedPath (mask, path, {}, g.getInternalContext().getPhysicalPixelScaleFactor());
        configure (*layer, scale, opacity, true);
        layer->shadow.render (g, mask);
    }
}

void LMEffectRenderer::render (juce::Graphics& g, const juce::GlyphArrangement& glyphs, float scale, float opacity)
{
    if (isEmpty())
        return;
    juce::Path path;
    glyphs.createPath (path);
    render (g, path, scale, opacity);
}

juce::Rectangle<float> LMEffectRenderer::getRenderBounds (juce::Rectangle<float> bounds, float scale) const
{
    if (bounds.isEmpty() || ! std::isfinite (scale) || scale <= 0.0f)
        return bounds;
    auto result = bounds;
    for (const auto& layer : layers)
    {
        const auto& s = layer->style;
        if (s.opacity <= 0.0f || s.colour.isTransparent())
            continue;
        // Two logical pixels conservatively cover antialiasing and fractional-DPI padding.
        const auto padding = (float) (scaledEffectValue (s.blur, scale)
                           + juce::jmax (0, scaledEffectValue (s.spread, scale))) + 2.0f;
        result = result.getUnion (bounds.expanded (padding).translated (
            (float) scaledEffectValue (s.x, scale), (float) scaledEffectValue (s.y, scale)));
    }
    return result;
}
