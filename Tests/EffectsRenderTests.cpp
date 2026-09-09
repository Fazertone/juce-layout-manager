#include <layout_manager/layout_manager.h>
#include <iostream>

namespace
{
juce::Image renderPath (LMEffectRenderer& effects, const juce::Path& path,
                         const juce::PathStrokeType* stroke = nullptr, float density = 1.0f)
{
    juce::Image image (juce::Image::ARGB, juce::roundToInt (128 * density), juce::roundToInt (128 * density), true);
    juce::Graphics g (image);
    g.addTransform (juce::AffineTransform::scale (density));
    if (stroke != nullptr)
        effects.render (g, path, *stroke);
    else
        effects.render (g, path);
    return image;
}

int alphaSum (const juce::Image& image)
{
    int sum = 0;
    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
            sum += image.getPixelAt (x, y).getAlpha();
    return sum;
}

bool equalImages (const juce::Image& a, const juce::Image& b)
{
    if (a.getBounds() != b.getBounds())
        return false;
    for (int y = 0; y < a.getHeight(); ++y)
        for (int x = 0; x < a.getWidth(); ++x)
            if (a.getPixelAt (x, y) != b.getPixelAt (x, y))
                return false;
    return true;
}

void writePNG (const juce::Image& image, const juce::String& name)
{
    const auto directory = juce::File::getCurrentWorkingDirectory().getChildFile ("effect-renders");
    directory.createDirectory();
    juce::FileOutputStream stream (directory.getChildFile (name));
    stream.setPosition (0);
    stream.truncate();
    juce::PNGImageFormat().writeImageToStream (image, stream);
}

class EffectsTests : public juce::UnitTest
{
public:
    EffectsTests() : UnitTest ("Declarative effects", "LayoutEffects") {}

    void runTest() override
    {
        beginTest ("XML defaults, target filtering, alpha, finite values and clamping");
        auto xml = juce::parseXML (R"xml(<JUCELayout><Object name="shape">
            <Effects><DropShadow/><DropShadow name="editable" x="-2.6" y="3" blur="-1"
                spread="-2" colour="#80ff0000" opacity="0.5"/>
                <DropShadow blur="nan" x="broken" opacity="2"/></Effects>
            <Effects target="arcActive"><DropShadow blur="9"/></Effects>
        </Object></JUCELayout>)xml");
        LayoutManager layout (*xml);
        auto styles = layout.getEffects ("shape");
        expectEquals ((int) styles.size(), 3);
        expectEquals (styles[0].blur, 0.0f);
        expect (styles[0].colour == juce::Colours::black);
        expectEquals (styles[1].blur, 0.0f);
        expectEquals (styles[1].spread, -2.0f);
        expectEquals (styles[2].x, 0.0f);
        expectEquals (styles[2].blur, 0.0f);
        expectEquals (styles[2].opacity, 1.0f);
        expectEquals (layout.getEffects ("shape", "arcActive")[0].blur, 9.0f);
        expect (layout.getEffects ("missing").empty());

        juce::Path rect;
        rect.addRectangle (40, 40, 40, 40);
        LMEffectRenderer effect;
        effect.setEffects ({ { 0, 0, 0, 0, juce::Colour (0x80ff0000), 0.5f } });
        auto result = renderPath (effect, rect);
        expectWithinAbsoluteError ((int) result.getPixelAt (60, 60).getAlpha(), 64, 1);

        beginTest ("Soft circle falloff, cache stability, effect bounds and DPI");
        juce::Path circle;
        circle.addEllipse (40, 40, 40, 40);
        LMShadowStyle purple { 0, 0, 8, 0, juce::Colour (0xffc993ec), 0.6f };
        effect.setEffects ({ purple });
        result = renderPath (effect, circle);
        expect (result.getPixelAt (80, 60).getAlpha() > result.getPixelAt (83, 60).getAlpha());
        expect (result.getPixelAt (83, 60).getAlpha() > result.getPixelAt (87, 60).getAlpha());
        expectEquals ((int) result.getPixelAt (90, 60).getAlpha(), 0);
        effect.setEffects ({ purple });
        expect (equalImages (result, renderPath (effect, circle)), "Identical styles preserve output");
        for (const auto density : { 1.0f, 1.5f, 2.0f })
        {
            const auto image = renderPath (effect, circle, nullptr, density);
            const auto bounds = effect.getRenderBounds (circle.getBounds()) * density;
            expect (alphaSum (image) > 0);
            for (int y = 0; y < image.getHeight(); ++y)
                for (int x = 0; x < image.getWidth(); ++x)
                    if (image.getPixelAt (x, y).getAlpha() != 0)
                        expect (bounds.contains ((float) x, (float) y), "Effect bounds contain every visible pixel");
        }

        beginTest ("Offsets, signed spread, zero blur, and collapsed geometry");
        effect.setEffects ({ { 5, -3, 0, 2, juce::Colours::white, 1 } });
        result = renderPath (effect, rect);
        expectEquals ((int) result.getPixelAt (44, 36).getAlpha(), 255);
        expectEquals ((int) result.getPixelAt (40, 40).getAlpha(), 0);
        effect.setEffects ({ { 0, 0, 0, -5, juce::Colours::white, 1 } });
        result = renderPath (effect, rect);
        expectEquals ((int) result.getPixelAt (42, 60).getAlpha(), 0);
        expectEquals ((int) result.getPixelAt (46, 60).getAlpha(), 255);
        effect.setEffects ({ { 0, 0, 8, -30, juce::Colours::white, 1 } });
        expectEquals (alphaSum (renderPath (effect, rect)), 0);
        expectEquals (alphaSum (renderPath (effect, juce::Path())), 0);

        beginTest ("Stroke spread changes thickness and does not fill an arc's centre");
        juce::Path arc;
        arc.addCentredArc (64, 64, 30, 30, 0, -2.0f, 2.0f, true);
        const juce::PathStrokeType stroke (4, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        effect.setEffects ({ { 0, 0, 3, 0, juce::Colours::white, 1 } });
        const auto normal = renderPath (effect, arc, &stroke);
        effect.setEffects ({ { 0, 0, 3, 2, juce::Colours::white, 1 } });
        const auto spread = renderPath (effect, arc, &stroke);
        expect (alphaSum (spread) > alphaSum (normal));
        expectEquals ((int) spread.getPixelAt (64, 64).getAlpha(), 0);
        effect.setEffects ({ { 0, 0, 3, -2, juce::Colours::white, 1 } });
        expectEquals (alphaSum (renderPath (effect, arc, &stroke)), 0);

        beginTest ("Layer ordering and live XML edits, additions, removal and reload");
        effect.setEffects ({ { 0, 0, 0, 0, juce::Colours::red, 1 }, { 0, 0, 0, 0, juce::Colours::blue, 1 } });
        expect (renderPath (effect, rect).getPixelAt (60, 60) == juce::Colours::blue);
        auto node = findComponentByName ("shape", layout.layoutTree);
        effect.setSource (node, "arcActive");
        const auto before = renderPath (effect, rect);
        auto group = node.getChild (1);
        group.getChild (0).setProperty ("opacity", 0, nullptr);
        expectEquals (alphaSum (renderPath (effect, rect)), 0);
        group.getChild (0).setProperty ("opacity", 1, nullptr);
        expect (equalImages (before, renderPath (effect, rect)));
        group.removeAllChildren (nullptr);
        expect (effect.isEmpty());
        juce::ValueTree added ("DropShadow");
        group.addChild (added, -1, nullptr);
        expect (! effect.isEmpty());

        beginTest ("XML primitives, fitted text, reload, and look-and-feel lifetime");
        auto fixture = juce::parseXML (R"xml(<JUCELayout><Object name="scene">
            <Ellipse name="circle" x="30" y="25" width="42" height="42" fillColour="#ffc993ec">
                <Effects><DropShadow name="glow" blur="10" colour="#ffc993ec" opacity="0.6"/></Effects>
            </Ellipse>
            <Rectangle x="105" y="25" width="58" height="42" cornerRadius="8" fillColour="#ffdddddd">
                <Effects><DropShadow x="5" y="6" blur="7" spread="2" opacity="0.8"/></Effects>
            </Rectangle>
            <Rectangle x="210" y="25" width="58" height="42" cornerRadius="8" fillColour="#ffaaaaaa">
                <Effects><DropShadow blur="12" colour="#ff8855ff"/><DropShadow y="5" blur="4" opacity="0.7"/></Effects>
            </Rectangle>
            <AutoLabel name="text" x="18" y="102" width="145" height="50" fontSize="16"
                       text="Fitted text glow" fillColour="#ffffffff" textAlignHorizontal="center">
                <Effects><DropShadow blur="5" colour="#ffab77ff"/></Effects>
            </AutoLabel>
            <Line x="25" y="178" width="128" height="0" strokeWeight="3" strokeColour="#ffffffff">
                <Effects><DropShadow y="4" blur="5" spread="1" colour="#ff8866ff"/></Effects>
            </Line>
        </Object>
        <Label name="label" x="5" y="5" width="180" height="50" text="Label" fontSize="12">
            <Effects><DropShadow blur="4" colour="#ff9933ff"/></Effects>
        </Label>
        <RotarySlider name="dial" x="180" y="80" width="100" height="100">
            <SliderWidget name="slider_widget" x="12" y="12" width="76" height="76"
                circleFillColour="#ff444444" arcActiveColour="#ffc993ec" arcActiveWidth="4"
                arcBgColour="#ff777777" arcBgWidth="3">
                <Effects target="arcActive"><DropShadow blur="6" spread="1" colour="#ffc993ec"/></Effects>
                <Effects target="circle"><DropShadow x="2" y="3" blur="4" opacity="0.7"/></Effects>
            </SliderWidget>
        </RotarySlider></JUCELayout>)xml");
        juce::Label label;
        juce::Slider slider;
        auto manager = std::make_unique<LayoutManager> (*fixture);
        manager->registerComponent (&label, "label");
        manager->registerComponent (&slider, "dial");
        manager->applyResize();
        expect (slider.getSliderStyle() == juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRange (0, 1);
        slider.setValue (0.7);
        auto* labelLaf = &label.getLookAndFeel();
        auto* sliderLaf = &slider.getLookAndFeel();
        for (int i = 0; i < 5; ++i)
            manager->applyLayout();
        expect (&label.getLookAndFeel() == labelLaf);
        expect (&slider.getLookAndFeel() == sliderLaf);
        for (const auto density : { 1.0f, 1.5f, 2.0f })
        {
            juce::Image image (juce::Image::ARGB, juce::roundToInt (300 * density), juce::roundToInt (220 * density), true);
            juce::Graphics g (image);
            g.addTransform (juce::AffineTransform::scale (density));
            g.fillAll (juce::Colour (0xff202020));
            manager->paintComponent (g, "scene");
            {
                juce::Graphics::ScopedSaveState saved (g);
                g.setOrigin (180, 85);
                slider.paintEntireComponent (g, true);
            }
            writePNG (image, "effects-" + juce::String (density) + "x.png");
        }
        auto paintScene = [&manager]
        {
            juce::Image image (juce::Image::ARGB, 300, 220, true);
            juce::Graphics g (image);
            manager->paintComponent (g, "scene");
            return image;
        };
        const auto litScene = paintScene();
        manager->setAttribute ("glow", "opacity", 0.0f);
        const auto unlitScene = paintScene();
        expect (litScene.getPixelAt (27, 46).getAlpha() > unlitScene.getPixelAt (27, 46).getAlpha(),
                "Changing XML updates an already-cached painted effect");
        expectEquals (manager->getEffects ("circle")[0].opacity, 0.0f);
        manager->loadFromXml (*fixture);
        expectEquals (manager->getEffects ("circle")[0].opacity, 0.6f);
        expect (&label.getLookAndFeel() == labelLaf);
        manager.reset();
        expect (&label.getLookAndFeel() != labelLaf, "Layout-owned look-and-feel detached before destruction");

        beginTest ("Labels without effects preserve custom styling and support live opt-in");
        auto plain = juce::parseXML (R"xml(<JUCELayout><Label name="plain" x="0" y="0" width="160" height="50" text="Unchanged"/></JUCELayout>)xml");
        juce::LookAndFeel_V4 custom;
        juce::Label plainLabel;
        plainLabel.setLookAndFeel (&custom);
        LayoutManager plainManager (*plain);
        plainManager.registerComponent (&plainLabel, "plain");
        plainManager.applyResize();
        expect (&plainLabel.getLookAndFeel() == &custom);
        auto plainNode = findComponentByName ("plain", plainManager.layoutTree);
        juce::ValueTree effectsGroup ("Effects");
        juce::ValueTree shadowNode ("DropShadow");
        shadowNode.setProperty ("blur", 4, nullptr);
        effectsGroup.addChild (shadowNode, -1, nullptr);
        plainNode.addChild (effectsGroup, -1, nullptr);
        auto* optedIn = &plainLabel.getLookAndFeel();
        expect (dynamic_cast<LMLookAndFeel*> (optedIn) != nullptr);
        auto labelImage = plainLabel.createComponentSnapshot (plainLabel.getLocalBounds());
        writePNG (labelImage, "registered-label.png");
        plainNode.removeChild (effectsGroup, nullptr);
        expect (&plainLabel.getLookAndFeel() != optedIn);
        plainNode.addChild (effectsGroup, -1, nullptr);
        expect (&plainLabel.getLookAndFeel() == optedIn, "Re-opt-in reuses the same cache owner");
        plainLabel.setLookAndFeel (nullptr);

        beginTest ("Style rounding, invalid source clearing, cache invalidation and layout scale");
        effect.setEffects ({ { 2.6f, -2.6f, 0, 0, juce::Colours::white, 1 } });
        result = renderPath (effect, rect);
        expectEquals ((int) result.getPixelAt (43, 37).getAlpha(), 255);
        expectEquals ((int) result.getPixelAt (42, 37).getAlpha(), 0);
        effect.setSource ({});
        expect (effect.isEmpty());
        effect.setEffects ({ purple });
        const auto initialCircle = renderPath (effect, circle);
        purple.blur = 14;
        effect.setEffects ({ purple });
        expect (! equalImages (initialCircle, renderPath (effect, circle)));
        purple.colour = juce::Colours::green;
        effect.setEffects ({ purple });
        expect (renderPath (effect, circle).getPixelAt (60, 60).getGreen() > 0);
        plainManager.scaling = 1.5f;
        plainManager.applyResize();
        expectEquals (plainLabel.getWidth(), 240);
        juce::GlyphArrangement firstText, secondText;
        const juce::Font textFont (juce::FontOptions (18.0f));
        firstText.addFittedText (textFont, "A", 20, 20, 60, 40, juce::Justification::centred, 1);
        secondText.addFittedText (textFont, "WWW", 20, 20, 60, 40, juce::Justification::centred, 1);
        juce::Image firstTextImage (juce::Image::ARGB, 128, 128, true), secondTextImage (juce::Image::ARGB, 128, 128, true);
        { juce::Graphics tg (firstTextImage); effect.render (tg, firstText); }
        { juce::Graphics tg (secondTextImage); effect.render (tg, secondText); }
        expect (! equalImages (firstTextImage, secondTextImage));

        beginTest ("Cached versus recreated renderer timing (informational)");
        effect.setEffects ({ purple });
        juce::Image bench (juce::Image::ARGB, 128, 128, true);
        juce::Graphics g (bench);
        effect.render (g, circle);
        const auto start = juce::Time::getMillisecondCounterHiRes();
        for (int i = 0; i < 300; ++i)
            effect.render (g, circle);
        const auto cached = juce::Time::getMillisecondCounterHiRes() - start;
        const auto coldStart = juce::Time::getMillisecondCounterHiRes();
        for (int i = 0; i < 300; ++i)
        {
            LMEffectRenderer cold;
            cold.setEffects ({ purple });
            cold.render (g, circle);
        }
        logMessage ("300 renders: cached=" + juce::String (cached, 2) + "ms, cold="
                    + juce::String (juce::Time::getMillisecondCounterHiRes() - coldStart, 2) + "ms");
    }
};
EffectsTests tests;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("LayoutEffects", 12345);
    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;
    return failures == 0 ? 0 : 1;
}
