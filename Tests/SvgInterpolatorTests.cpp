#include <layout_manager/layout_manager.h>
#include <cmath>

namespace
{
class SvgInterpolatorTests final : public juce::UnitTest
{
public:
    SvgInterpolatorTests() : UnitTest ("SVG interpolation", "LayoutEffects") {}
    void runTest() override
    {
        beginTest ("Circle and SVG keyframes preserve size, corners and intermediate continuity");
        const auto xml = juce::parseXML (R"xml(<SvgInterpolator>
          <Circle at="0" radius="0.1"/>
          <Path at="0.5" viewBox="10 20 20 20" scale="0.5" d="M10 20 L30 20 L30 40 L10 40 Z"/>
          <Path at="1" viewBox="0 0 2 2" d="M0 0 L2 0 L2 2 L0 2 Z"/>
        </SvgInterpolator>)xml");
        auto tree = juce::ValueTree::fromXml (*xml);
        LMSvgInterpolator morph;
        expect (morph.setSource (tree));
        expectWithinAbsoluteError (morph.createPath (0).getLength(), juce::MathConstants<float>::twoPi * 0.1f, 0.002f);
        expectWithinAbsoluteError (morph.createPath (0.5f).getLength(), 4.0f, 0.002f);
        expectWithinAbsoluteError (morph.createPath (1).getLength(), 8.0f, 0.002f);
        expectWithinAbsoluteError (morph.createPath (0.75f).getBounds().getWidth(), 1.5f, 0.002f);
        expect (morph.createPath (-1) == morph.createPath (0));
        expect (morph.createPath (2) == morph.createPath (1));
        expect (morph.createPath (0.75f) == morph.createPath (0.75f));
        expectWithinAbsoluteError (morph.createPath (0.4999f).getLength(), morph.createPath (0.5001f).getLength(), 0.005f);
        expect (morph.setSource (tree));
        tree.getChild (2).setProperty ("scale", 0.75f, nullptr);
        expect (morph.setSource (tree));
        expectWithinAbsoluteError (morph.createPath (1).getBounds().getWidth(), 1.5f, 0.002f);

        beginTest ("Different contour vertex counts retain authored corners");
        const auto different = juce::parseXML (R"xml(<SvgInterpolator>
          <Path at="0" viewBox="0, 0, 2, 2" d="M0 0 L2 0 L1 2 L0 0 Z"/>
          <Path at="1" viewBox="0 0 2 2" d="M0 0 L2 0 L2 2 L0 2 Z"/>
        </SvgInterpolator>)xml");
        expect (morph.setSource (juce::ValueTree::fromXml (*different)));
        expectWithinAbsoluteError (morph.createPath (0).getLength(), 2.0f + 2.0f * std::sqrt (5.0f), 0.002f);
        expectWithinAbsoluteError (morph.createPath (1).getLength(), 8.0f, 0.002f);
        expect (morph.createPath (0.5f).contains (0, 0));

        beginTest ("Invalid keyframes clear cached artwork");
        for (const auto* invalid : {
            "<SvgInterpolator/>",
            "<SvgInterpolator><Circle at='0'/><Circle at='0'/></SvgInterpolator>",
            "<SvgInterpolator><Circle at='0'/><Path at='1' viewBox='0 0 0 1' d='M0 0 L1 1 Z'/></SvgInterpolator>",
            "<SvgInterpolator><Circle at='0'/><Path at='1' viewBox='0 0 1 1' d='M0 0 L1 0 L1 1'/></SvgInterpolator>",
            "<SvgInterpolator><Circle at='0'/><Path at='1' viewBox='0 0 1 1' d='M0 0 L1 0 L1 1 Z M0 0 L1 0 L1 1 Z'/></SvgInterpolator>" })
        {
            expect (! morph.setSource (juce::ValueTree::fromXml (*juce::parseXML (invalid))));
            expect (morph.createPath (0.5f).isEmpty());
        }

        beginTest ("Bipolar knobs collapse to their anchors and level fills its backplate");
        for (const auto scale : { 1.0f, 1.5f, 2.0f })
        {
            LMLookAndFeel style;
            style.effectScale = scale;
            style.sliderLayout.sliderBounds = juce::Rectangle<int> (0, 0, juce::roundToInt (80 * scale), juce::roundToInt (80 * scale));
            style.sliderWidgetStyle = juce::ValueTree ("SliderWidget");
            style.sliderWidgetStyle.setProperty ("symbolColour", "ffffffff", nullptr);
            auto boundsAt = [&] (const char* name, float value)
            {
                style.sliderWidgetStyle.setProperty ("visualStyle", name, nullptr);
                juce::Image image (juce::Image::ARGB, style.sliderLayout.sliderBounds.getWidth(), style.sliderLayout.sliderBounds.getHeight(), true);
                juce::Graphics g (image);
                style.drawSymbolSlider (g, value);
                juce::Rectangle<int> bounds;
                for (int y = 0; y < image.getHeight(); ++y)
                    for (int x = 0; x < image.getWidth(); ++x)
                        if (image.getPixelAt (x, y).getRed() > 160)
                            bounds = bounds.getUnion ({ x, y, 1, 1 });
                return bounds.toFloat() / scale;
            };
            const auto neutral = boundsAt ("bipolarSector", 0.5f);
            expect (neutral.getWidth() <= 2 && neutral.getHeight() > 28, "Neutral pitch is a vertical radius: " + neutral.toString());
            expect (boundsAt ("bipolarSector", 0.375f).getRight() <= 41);
            expect (boundsAt ("bipolarSector", 0.625f).getX() >= 39);
            const auto centre = boundsAt ("pan", 0.5f);
            const auto left = boundsAt ("pan", 0), right = boundsAt ("pan", 1);
            expect (centre.getWidth() > 58 && centre.getHeight() > 49);
            expect (left.getRight() <= 41 && left.getY() >= 39);
            expect (right.getX() >= 39 && right.getY() >= 39);
            expectWithinAbsoluteError (left.getWidth(), right.getWidth(), 1.0f);
            style.sliderWidgetStyle.setProperty ("maximumRadius", 1.0f, nullptr);
            style.sliderWidgetStyle.setProperty ("symbolWidth", 4.0f, nullptr);
            const auto full = boundsAt ("ring", 1);
            expect (full.getWidth() >= 75 && full.getWidth() <= 76);
        }
    }
};
SvgInterpolatorTests svgInterpolatorTests;
}
