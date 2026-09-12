#include <layout_manager/layout_manager.h>

namespace
{
juce::Image renderLayer (LayoutManager& layout, float scale = 1.0f, float density = 1.0f)
{
    layout.scaling = scale;
    juce::Image image (juce::Image::ARGB, juce::roundToInt (240 * scale * density),
                       juce::roundToInt (180 * scale * density), true);
    juce::Graphics g (image);
    g.addTransform (juce::AffineTransform::scale (density));
    layout.paintComponent (g, "scene");
    return image;
}

bool sameLayerPixels (const juce::Image& a, const juce::Image& b)
{
    if (a.getBounds() != b.getBounds())
        return false;
    for (int y = 0; y < a.getHeight(); ++y)
        for (int x = 0; x < a.getWidth(); ++x)
            if (a.getPixelAt (x, y) != b.getPixelAt (x, y))
                return false;
    return true;
}

class LayerBlurTests : public juce::UnitTest
{
public:
    LayerBlurTests() : UnitTest ("XML layer blur", "LayoutEffects") {}

    void runTest() override
    {
        const auto xml = juce::parseXML (R"(<JUCELayout><Object name="scene">
            <Rectangle name="blurred" x="50" y="45" width="90" height="45" cornerRadius="8" fillColour="#ffffffff">
                <Effects><LayerBlur blur="0"/></Effects>
            </Rectangle>
            <Rectangle name="neighbour" x="160" y="45" width="24" height="45" fillColour="#ffff0000"/>
        </Object></JUCELayout>)");
        LayoutManager layout (*xml);
        auto node = findComponentByName ("blurred", layout.layoutTree);
        auto effects = node.getChildWithName ("Effects");
        auto blur = effects.getChildWithName ("LayerBlur");
        beginTest ("Fractional layer blur softens edges and leaves neighbouring artwork sharp");
        const auto plain = renderLayer (layout);
        blur.setProperty ("blur", 0.58f, nullptr);
        const auto fractional = renderLayer (layout);
        expect (!sameLayerPixels (plain, fractional));
        expect (fractional.getPixelAt (49, 65).getAlpha() > 0);
        expect (fractional.getPixelAt (46, 65).isTransparent());
        expect (fractional.getPixelAt (100, 65) == juce::Colours::white);
        expect (fractional.getPixelAt (159, 65).isTransparent());
        expect (fractional.getPixelAt (160, 65) == juce::Colours::red);

        beginTest ("Repeated output is stable and artwork, geometry and effect edits invalidate the cache");
        expect (sameLayerPixels (fractional, renderLayer (layout)));
        node.setProperty ("fillColour", "ff0000ff", nullptr);
        expect (renderLayer (layout).getPixelAt (100, 65) == juce::Colours::blue);
        node.setProperty ("x", 65, nullptr);
        expect (renderLayer (layout).getPixelAt (52, 65).isTransparent());
        node.setProperty ("x", 50, nullptr);
        node.setProperty ("fillColour", "ffffffff", nullptr);
        expect (sameLayerPixels (fractional, renderLayer (layout)));
        blur.setProperty ("blur", 4, nullptr);
        expect (!sameLayerPixels (fractional, renderLayer (layout)));
        for (const auto invalid : { "0", "-1", "nan", "broken" })
        {
            blur.setProperty ("blur", invalid, nullptr);
            expect (sameLayerPixels (plain, renderLayer (layout)));
        }
        blur.setProperty ("blur", 0.58f, nullptr);
        effects.removeChild (blur, nullptr);
        expect (sameLayerPixels (plain, renderLayer (layout)));
        effects.addChild (blur, -1, nullptr);
        expect (sameLayerPixels (fractional, renderLayer (layout)));

        beginTest ("Complete appearance includes inner shadow, stroke and uncut outer shadows");
        const auto shadowXml = juce::parseXML (R"(<Effects>
            <DropShadow x="-3" y="3" blur="8" spread="2" colour="#ff00ff00" opacity="0.7"/>
            <InnerShadow x="2" y="2" blur="6" colour="#ff000000" opacity="0.8"/>
        </Effects>)");
        node.addChild (juce::ValueTree::fromXml (*shadowXml), -1, nullptr);
        node.setProperty ("strokeColour", "ff0000ff", nullptr);
        node.setProperty ("strokeWeight", 1, nullptr);
        const auto composite = renderLayer (layout);
        expect (composite.getPixelAt (44, 65).getAlpha() > 0);
        expect (composite.getPixelAt (55, 65).getBrightness() < 1.0f);
        expect (!sameLayerPixels (fractional, composite));
        for (int y = 0; y < composite.getHeight(); ++y)
            expect (composite.getPixelAt (0, y).isTransparent());

        beginTest ("Layout and device scaling produce stable unclipped output");
        const auto output = juce::File::getCurrentWorkingDirectory().getChildFile ("effect-renders");
        output.createDirectory();
        for (float scale : { 1.0f, 1.5f, 2.0f })
            for (float density : { 1.0f, 2.0f })
            {
                const auto image = renderLayer (layout, scale, density);
                expect (sameLayerPixels (image, renderLayer (layout, scale, density)));
                expect (image.getPixelAt (juce::roundToInt (44 * scale * density),
                                           juce::roundToInt (65 * scale * density)).getAlpha() > 0);
                juce::FileOutputStream stream (output.getChildFile (
                    "layer-blur-" + juce::String (scale) + "x-" + juce::String (density) + "dpi.png"));
                stream.setPosition (0);
                stream.truncate();
                expect (juce::PNGImageFormat().writeImageToStream (image, stream));
            }
        expect (sameLayerPixels (composite, renderLayer (layout)));

        beginTest ("Opt-in circular corners retain a capsule with an oversized Figma radius");
        node.setProperty ("cornerRadius", 58, nullptr);
        node.setProperty ("clampCornerRadius", true, nullptr);
        const auto capsule = renderLayer (layout);
        node.setProperty ("cornerRadius", 22.5f, nullptr);
        node.removeProperty ("clampCornerRadius", nullptr);
        expect (sameLayerPixels (capsule, renderLayer (layout)));
        node.setProperty ("cornerRadius", 58, nullptr);
        expect (!sameLayerPixels (capsule, renderLayer (layout)), "Legacy elliptical corners remain opt-in compatible");

        beginTest ("Ellipses support layer blur and reload discards old artwork");
        const auto ellipse = juce::parseXML (R"(<JUCELayout><Object name="scene">
            <Ellipse name="blurred" x="50" y="45" width="90" height="45" fillColour="#ffffffff">
                <Effects><LayerBlur blur="2"/></Effects>
            </Ellipse>
        </Object></JUCELayout>)");
        expect (layout.loadFromXml (*ellipse));
        const auto ellipseImage = renderLayer (layout);
        expect (ellipseImage.getPixelAt (49, 67).getAlpha() > 0);
        expect (ellipseImage.getPixelAt (160, 65).isTransparent());
        expect (ellipseImage.getPixelAt (100, 65) == juce::Colours::white);
    }
};
LayerBlurTests layerBlurTests;
}
