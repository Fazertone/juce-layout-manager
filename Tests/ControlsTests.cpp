#include <layout_manager/layout_manager.h>
#include <cmath>

namespace
{
juce::MouseEvent controlEvent (juce::Component& component, juce::Point<float> point)
{
    return { juce::Desktop::getInstance().getMainMouseSource(), point,
             juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, &component, &component,
             juce::Time::getCurrentTime(), point, juce::Time::getCurrentTime(), 1, false };
}

class ControlsTests final : public juce::UnitTest
{
public:
    ControlsTests() : UnitTest ("XML controls", "LayoutEffects") {}
    void runTest() override
    {
        beginTest ("Text-only button states and pill fields scale from XML");
        auto buttonXml = juce::parseXML (R"xml(<JUCELayout>
          <TextButton name="yes" x="0" y="0" width="100" height="30">
            <Rectangle name="btn_bg"/>
            <Object name="btn_text" text="YES" fontSize="12" fillColour="#ff554360"
              fillColourHover="#ffc993ec" fillColourDown="#ffe3b5ff"/>
          </TextButton>
          <TextEditor name="entry" x="0" y="0" width="100" height="20" cornerRadius="10">
            <Rectangle name="text_editor_box" fillColour="#ff171517"/>
          </TextEditor>
        </JUCELayout>)xml");
        juce::TextButton yes;
        juce::TextEditor entry;
        LayoutManager buttons (*buttonXml);
        buttons.registerComponent (&yes, "yes");
        buttons.registerComponent (&entry, "entry");
        for (const auto scale : { 1.0f, 1.5f, 2.0f })
        {
            buttons.scaling = scale;
            buttons.applyResize();
            int previousRed = 0;
            for (const auto state : { juce::Button::buttonNormal, juce::Button::buttonOver, juce::Button::buttonDown })
            {
                yes.setState (state);
                const auto rendered = yes.createComponentSnapshot (yes.getLocalBounds());
                int brightest = 0;
                for (int y = 0; y < rendered.getHeight(); ++y)
                    for (int x = 0; x < rendered.getWidth(); ++x)
                        if (rendered.getPixelAt (x, y).getAlpha() > 200)
                            brightest = juce::jmax (brightest, (int) rendered.getPixelAt (x, y).getRed());
                expect (brightest > previousRed);
                previousRed = brightest;
                expectEquals ((int) rendered.getPixelAt (0, 0).getAlpha(), 0);
            }
            yes.setEnabled (false);
            yes.setState (juce::Button::buttonOver);
            const auto disabled = yes.createComponentSnapshot (yes.getLocalBounds());
            int maxAlpha = 0;
            for (int y = 0; y < disabled.getHeight(); ++y)
                for (int x = 0; x < disabled.getWidth(); ++x)
                    maxAlpha = juce::jmax (maxAlpha, (int) disabled.getPixelAt (x, y).getAlpha());
            expect (maxAlpha > 0 && maxAlpha <= 128);
            yes.setEnabled (true);
            const auto rounded = entry.createComponentSnapshot (entry.getLocalBounds());
            expectEquals ((int) rounded.getPixelAt (juce::roundToInt (3 * scale), juce::roundToInt (scale)).getAlpha(), 0);
            expectEquals ((int) rounded.getPixelAt (juce::roundToInt (10 * scale), juce::roundToInt (10 * scale)).getAlpha(), 255);
        }

        beginTest ("Linear slider uses XML bounds, full range and distinct bar/dot geometry");
        auto xml = juce::parseXML (R"xml(<JUCELayout><Object name="panel">
          <LinearSlider name="gain" x="0" y="0" width="32" height="120" textBox="none">
            <SliderWidget name="slider_widget" x="8" y="10" width="16" height="100"
              visualStyle="bar" activeWidth="4" trackWidth="1" trackColour="ff555555" thumbColour="ffc993ec"/>
          </LinearSlider></Object></JUCELayout>)xml");
        LMSlider slider;
        LayoutManager layout (*xml);
        layout.registerComponent (&slider, "gain");
        slider.setRange (-12, 12, 0.1);
        for (auto scale : { 1.0f, 1.5f, 2.0f })
        {
            layout.scaling = scale;
            layout.applyResize();
            expectEquals (slider.getWidth(), juce::roundToInt (32 * scale));
            expectWithinAbsoluteError (slider.getPositionOfValue (12), 10.0f * scale, 0.01f);
            expectWithinAbsoluteError (slider.getPositionOfValue (-12), 110.0f * scale, 0.01f);
            slider.setValue (0);
            const auto bar = slider.createComponentSnapshot (slider.getLocalBounds());
            auto widget = findComponentByName ("gain", layout.layoutTree).getChild (0);
            widget.setProperty ("visualStyle", "dot", nullptr);
            layout.applyLayout();
            const auto dot = slider.createComponentSnapshot (slider.getLocalBounds());
            const int x = juce::roundToInt (16 * scale), y = juce::roundToInt (90 * scale);
            expect (bar.getPixelAt (x, y).getRed() > dot.getPixelAt (x, y).getRed());
            widget.setProperty ("visualStyle", "bar", nullptr);
        }

        beginTest ("Each XML rotary symbol changes with the normalised value");
        for (const auto* name : { "sector", "bipolarSector", "pan", "ring", "jaggedRing", "svgMorph", "dome", "arc" })
        {
            auto document = juce::parseXML (juce::String (R"xml(<JUCELayout><RotarySlider name="knob" x="0" y="0" width="80" height="80" textBox="none" startAngle="225" endAngle="495">
              <SliderWidget name="slider_widget" x="8" y="8" width="64" height="64" visualStyle=")xml") + name + R"xml(" symbolWidth="2" roundedEnds="true" arcActiveColour="ffc993ec" arcActiveWidth="3" arcBgColour="ff775588" arcBgWidth="1">
                <SvgInterpolator><Circle at="0" radius="0.1"/>
                  <Path at="1" viewBox="0 0 2 2" d="M1 0 L2 1 L1 2 L0 1 Z"/>
                </SvgInterpolator>
              </SliderWidget></RotarySlider></JUCELayout>)xml");
            LMSlider knob;
            LayoutManager manager (*document);
            manager.registerComponent (&knob, "knob");
            manager.applyResize();
            knob.setRange (0, 1);
            knob.setValue (0.2);
            const auto first = knob.createComponentSnapshot (knob.getLocalBounds());
            knob.setValue (0.8);
            const auto second = knob.createComponentSnapshot (knob.getLocalBounds());
            int changed = 0;
            for (int y = 0; y < 80; ++y)
                for (int x = 0; x < 80; ++x)
                    changed += first.getPixelAt (x, y) != second.getPixelAt (x, y);
            expect (changed > 20, name);
        }

        beginTest ("Inner shadows composite over fills without expanding their bounds");
        juce::Path box;
        box.addRoundedRectangle (20, 20, 80, 80, 5);
        auto document = juce::parseXML (R"xml(<Object><Effects><InnerShadow x="0" y="4" blur="6" colour="ff000000" opacity="1"/></Effects></Object>)xml");
        LMEffectRenderer effect;
        effect.setSource (juce::ValueTree::fromXml (*document));
        juce::Image image (juce::Image::ARGB, 120, 120, true);
        juce::Graphics graphics (image);
        effect.render (graphics, box);
        expectEquals ((int) image.getPixelAt (60, 22).getAlpha(), 0);
        graphics.setColour (juce::Colours::white);
        graphics.fillPath (box);
        effect.renderInner (graphics, box);
        expect (image.getPixelAt (60, 22).getRed() < image.getPixelAt (60, 60).getRed());
        expectEquals ((int) image.getPixelAt (60, 18).getAlpha(), 0);
        expect (effect.getRenderBounds (box.getBounds()) == box.getBounds());

        beginTest ("XML radial washes fade from their origin and respect rounded clipping");
        auto radialXml = juce::parseXML (R"xml(<JUCELayout><Object name="panel">
          <Rectangle name="wash" x="0" y="0" width="100" height="100" cornerRadius="10"
            fill-gradient-type="radial" fill-gradient-start="0.5,0.5" fill-gradient-end="1,0.5"
            fill-gradient-stops="#ff9933ff:0,#009933ff:1"/>
        </Object></JUCELayout>)xml");
        LayoutManager radial (*radialXml);
        juce::Image wash (juce::Image::ARGB, 100, 100, true);
        juce::Graphics washGraphics (wash);
        radial.paintComponent (washGraphics, "panel");
        expect (wash.getPixelAt (50, 50).getAlpha() > wash.getPixelAt (75, 50).getAlpha());
        expect (wash.getPixelAt (75, 50).getAlpha() > wash.getPixelAt (95, 50).getAlpha());
        expectEquals ((int) wash.getPixelAt (0, 0).getAlpha(), 0);

        beginTest ("AHDSR setters are silent and zero-time handles remain independently draggable");
        LMAhdsrComponent graph;
        graph.setSize (320, 100);
        int starts = 0, ends = 0, changes = 0;
        graph.onGestureStart = [&] (auto) { ++starts; };
        graph.onGestureEnd = [&] (auto) { ++ends; };
        graph.onValueChange = [&] (auto, float) { ++changes; };
        graph.setValues ({ 0, 0, 0, 1, 0 });
        expectEquals (changes, 0);
        auto handles = graph.getHandlePositions();
        expect (handles[0] != handles[1] && handles[1] != handles[2]);
        graph.mouseDown (controlEvent (graph, handles[LMAhdsrComponent::hold]));
        graph.mouseDrag (controlEvent (graph, handles[LMAhdsrComponent::hold].translated (30, 0)));
        expect (graph.getValues()[LMAhdsrComponent::hold] > 0);
        expectEquals (graph.getValues()[LMAhdsrComponent::attack], 0.0f);
        graph.cancelGesture();
        graph.cancelGesture();
        expectEquals (starts, 1);
        expectEquals (ends, 1);
        graph.setValues ({ 100, 100, 100, 0.5f, 100 });
        handles = graph.getHandlePositions();
        graph.mouseDown (controlEvent (graph, handles[LMAhdsrComponent::decay]));
        graph.mouseDrag (controlEvent (graph, handles[LMAhdsrComponent::decay].translated (20, -10)));
        expect (graph.getValues()[LMAhdsrComponent::decay] > 100);
        expect (graph.getValues()[LMAhdsrComponent::sustain] > 0.5f);
        graph.cancelGesture();
        expectEquals (starts, 3);
        expectEquals (ends, 3);
        graph.setValues ({ 5000, 0, 0, 0, 0 });
        handles = graph.getHandlePositions();
        for (size_t i = 0; i < handles.size(); ++i)
            for (size_t j = 0; j < i; ++j)
                expect (handles[i].getDistanceFrom (handles[j]) >= 9.5f,
                        "Zero release and sustain remain distinct at the right edge");
        graph.setValues ({ -1, 1.0e9f, 0, 2, 0 });
        expectEquals (graph.getValues()[LMAhdsrComponent::attack], 0.0f);
        expectEquals (graph.getValues()[LMAhdsrComponent::hold], 5000.0f);
        expectEquals (graph.getValues()[LMAhdsrComponent::sustain], 1.0f);
    }
};
ControlsTests controlsTests;
}
