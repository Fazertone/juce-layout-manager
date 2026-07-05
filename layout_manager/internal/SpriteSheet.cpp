#include "SpriteSheet.h"

//==============================================================================
int Sprite::getStateFrame (const juce::String& state, int fallback) const
{
    const auto it = stateFrames.find (state);
    return it != stateFrames.end() ? it->second : fallback;
}

juce::Rectangle<int> Sprite::getFrameBounds (int frameIndex) const
{
    const int numFrames = getNumFrames();
    frameIndex = juce::jlimit (0, juce::jmax (0, numFrames - 1), frameIndex);

    const int col = (columns > 0) ? frameIndex % columns : 0;
    const int row = (columns > 0) ? frameIndex / columns : 0;

    const int imgW = image.getWidth();
    const int imgH = image.getHeight();

    // Compute each cell edge independently as roundToInt(k * dim / count) so the
    // rounding error never accumulates across cells. THIS is what the old
    // LMLookAndFeel sprite path got wrong for non-whole frame sizes.
    const int left   = juce::roundToInt ((double) col       * imgW / (double) juce::jmax (1, columns));
    const int right  = juce::roundToInt ((double) (col + 1) * imgW / (double) juce::jmax (1, columns));
    const int top    = juce::roundToInt ((double) row       * imgH / (double) juce::jmax (1, rows));
    const int bottom = juce::roundToInt ((double) (row + 1) * imgH / (double) juce::jmax (1, rows));

    return { left, top, right - left, bottom - top };
}

void Sprite::drawFrame (juce::Graphics& g, int frameIndex, juce::Rectangle<float> dest) const
{
    if (! image.isValid())
        return;

    const auto src = getFrameBounds (frameIndex);
    const int dx = juce::roundToInt (dest.getX());
    const int dy = juce::roundToInt (dest.getY());
    const int dw = juce::roundToInt (dest.getWidth());
    const int dh = juce::roundToInt (dest.getHeight());

    if (dw <= 0 || dh <= 0)
        return;

    if (edgeBlur <= 0)
    {
        g.drawImage (image, dx, dy, dw, dh,
                     src.getX(), src.getY(), src.getWidth(), src.getHeight());
        return;
    }

    // The frames are opaque JPGs, so without feathering their rectangular edge
    // lands as a hard seam over whatever is behind them (visible wherever the
    // sheet's own lighting/shadow differs from the panel). Render the frame
    // into a small ARGB buffer and scale each pixel's (premultiplied) alpha
    // with a gaussian falloff from 1 in the centre to 0 at the border, across
    // `edgeBlur` source pixels. multiplyAlpha() scales alpha AND the
    // premultiplied RGB channels together, so this is a true fade with no
    // colour shift.
    //
    // The temp image must be sized in *physical* pixels, not logical. On a
    // HiDPI/Retina display the non-blur path (g.drawImage) renders at 1:1
    // device resolution through the context transform; without the scale-up
    // we would create a lo-res buffer that gets blockily upscaled later.
    const float ctxScale = g.getInternalContext().getPhysicalPixelScaleFactor();
    const int pdw = juce::roundToInt ((float) dw * ctxScale);
    const int pdh = juce::roundToInt ((float) dh * ctxScale);

    const double sx = (double) pdw / (double) juce::jmax (1, src.getWidth());
    const double sy = (double) pdh / (double) juce::jmax (1, src.getHeight());
    const double fadePx = juce::jmax (1.0, edgeBlur * juce::jmin (sx, sy));

    // Gaussian falloff: alpha = 1 - exp(-0.5 * (d/σ)²), σ = fadePx / 3.
    // At d = 3σ (fadePx) alpha ≈ 0.99 — essentially opaque.
    const double sigma2 = (fadePx * fadePx) / 18.0;   // σ²/0.5 expanded

    juce::Image frame (juce::Image::ARGB, pdw, pdh, true);
    {
        juce::Graphics fg (frame);
        fg.drawImage (image, 0, 0, pdw, pdh,
                      src.getX(), src.getY(), src.getWidth(), src.getHeight());
    }

    juce::Image::BitmapData data (frame, juce::Image::BitmapData::readWrite);
    for (int y = 0; y < pdh; ++y)
    {
        const double dy = (double) juce::jmin (y, pdh - 1 - y);

        for (int x = 0; x < pdw; ++x)
        {
            const double dx = (double) juce::jmin (x, pdw - 1 - x);
            const double d  = juce::jmin (dx, dy);

            if (d >= fadePx)
                continue;   // gaussian(d) ≈ 1 here — interior, leave fully opaque

            const double f  = 1.0 - std::exp (-0.5 * d * d / sigma2);
            auto& px = *reinterpret_cast<juce::PixelARGB*> (data.getPixelPointer (x, y));
            px.multiplyAlpha ((float) f);
        }
    }

    g.drawImage (frame, dx, dy, dw, dh, 0, 0, pdw, pdh);
}

//==============================================================================
bool SpriteSheetCollection::loadFromXml (const juce::File& xmlFile)
{
    sprites.clear();

    auto xml = juce::XmlDocument::parse (xmlFile);

    if (xml == nullptr)
    {
        DBG ("SpriteSheetCollection: could not parse XML at " << xmlFile.getFullPathName());
        jassertfalse;
        return false;
    }

    type            = xml->getStringAttribute ("type");
    numFramesDefault = xml->getIntAttribute ("num_frames", 0);

    const juce::File xmlDir = xmlFile.getParentDirectory();

    for (auto* child : xml->getChildIterator())
    {
        if (! child->hasTagName ("Sprite"))
            continue;

        Sprite sprite;
        sprite.label   = child->getStringAttribute ("label");
        sprite.columns = child->getIntAttribute ("columns", 1);
        sprite.rows    = child->getIntAttribute ("rows", 1);
        sprite.placement = { child->getIntAttribute ("x"), child->getIntAttribute ("y"),
                             child->getIntAttribute ("w"), child->getIntAttribute ("h") };
        sprite.edgeBlur  = child->getIntAttribute ("edgeblur", 0);

        // Optional named state frames (tooling *_src attributes are ignored).
        for (const auto* state : { "off", "pressed", "on" })
            if (child->hasAttribute (state))
                sprite.stateFrames[state] = child->getIntAttribute (state);

        const auto filename = child->getStringAttribute ("filename");
        const auto imageFile = xmlDir.getChildFile (filename);
        sprite.image = juce::ImageCache::getFromFile (imageFile);

        if (! sprite.image.isValid())
        {
            DBG ("SpriteSheetCollection: missing/invalid image '" << imageFile.getFullPathName()
                 << "' for sprite '" << sprite.label << "'");
            jassertfalse;
        }
        else
        {
            const int expectedW = sprite.placement.getWidth()  * sprite.columns;
            const int expectedH = sprite.placement.getHeight() * sprite.rows;

            if (std::abs (sprite.image.getWidth()  - expectedW) > 1
                || std::abs (sprite.image.getHeight() - expectedH) > 1)
            {
                DBG ("SpriteSheetCollection: dimension mismatch for '" << sprite.label
                     << "' image=" << sprite.image.getWidth() << "x" << sprite.image.getHeight()
                     << " expected=" << expectedW << "x" << expectedH);
                jassertfalse;
            }
        }

        if (sprite.label.isNotEmpty())
            sprites[sprite.label] = std::move (sprite);
    }

    return ! sprites.empty();
}

const Sprite* SpriteSheetCollection::get (const juce::String& label) const
{
    const auto it = sprites.find (label);
    return it != sprites.end() ? &it->second : nullptr;
}

const Sprite& SpriteSheetCollection::operator[] (const juce::String& label) const
{
    if (auto* s = get (label))
        return *s;

    // Missing label: fail loudly in Debug, return an empty sprite in Release
    // (draws nothing, never crashes).
    jassertfalse;
    static const Sprite empty;
    return empty;
}

//==============================================================================
SpriteKnobComponent::SpriteKnobComponent()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
}

void SpriteKnobComponent::paint (juce::Graphics& g)
{
    if (sprite == nullptr || ! sprite->isValid())
        return;

    const int numFrames = sprite->getNumFrames();
    const double proportion = valueToProportionOfLength (getValue());
    const int frame = juce::roundToInt (proportion * (double) (numFrames - 1));

    sprite->drawFrame (g, frame, getLocalBounds().toFloat());
}

//==============================================================================
SpriteSwitchComponent::SpriteSwitchComponent()
    : juce::Button ({})
{
}

void SpriteSwitchComponent::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted);

    if (sprite == nullptr || ! sprite->isValid())
        return;

    int frame;
    if (shouldDrawButtonAsDown)
        frame = sprite->getStateFrame ("pressed", 0);
    else
        frame = sprite->getStateFrame (getToggleState() ? "on" : "off", 0);

    sprite->drawFrame (g, frame, getLocalBounds().toFloat());
}
