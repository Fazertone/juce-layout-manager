# XML sliders and AHDSR

Register components with `LayoutManager::registerComponent`, set `scaling`, and
call `applyResize()`. Geometry is in layout units. Top-level bounds are relative
to the component's parent; `slider_widget` bounds are local to the slider.
The consumer owns components, parameter ranges, defaults and bindings. XML owns
visuals. Existing `RotarySlider` documents keep their original rendering unless
new attributes are supplied.

## Sliders

Use `juce::Slider` for the standard interaction model, or `LMSlider` for on-demand
value entry. Both use the same XML-driven `LMLookAndFeel`.

```xml
<RotarySlider name="attack" x="20" y="30" width="44" height="44"
              textBox="none" startAngle="225" endAngle="495"
              showValueOnHover="true">
  <SliderWidget name="slider_widget" x="5" y="5" width="34" height="34"
                visualStyle="arc" roundedEnds="true"
                arcActiveColour="#ffc993ec" arcActiveWidth="3"
                arcBgColour="#ffc993ec" arcBgWidth="0.6">
    <Effects target="arcActive">
      <DropShadow blur="2" colour="#ffc993ec" opacity="0.7"/>
    </Effects>
  </SliderWidget>
</RotarySlider>
```

`startAngle` and `endAngle` use JUCE's clockwise degrees from twelve o'clock;
225–495 gives a sweep with the opening at the bottom. `textBox="none"` hides the
text box; `below` enables it (use `slider_value_text` for its local bounds and
font). Omitting these attributes preserves the old slider configuration.
`roundedEnds` defaults to false for compatibility. Arc and circle attributes
and effect targets remain those in `README_Effects.md`.

Alternative `visualStyle` values on `slider_widget`:

| Style | Behaviour | Additional attributes (defaults) |
| --- | --- | --- |
| `sector` | Outlined quadrant rotates with the value | `rotationRange=180` |
| `pan` | Notched circular outline turns left/right around its midpoint | |
| `ring` | Inner ring expands | `minimumRadius=0.12`, `maximumRadius=0.5` |
| `jaggedRing` | Ring becomes increasingly irregular | `teeth=16`, `maximumDepth=0.3`, `irregularity=0.12` |
| `dome` | Arched outline opens and its notch deepens | `minimumOpening=0.60`, `maximumOpening=0.75`, `maximumNotchDepth=0.18` |

All symbol styles accept `padding` (layout units), `symbolRadius` (fraction of
backplate radius), `symbolWidth`, `symbolColour`, and the existing circle
fill/stroke attributes. Ring radii are fractions of the backplate radius;
dome openings are fractions of pi. Use `Effects target="symbol"` for outline
glow and `Effects target="circle"` for backplate effects.

```xml
<LinearSlider name="gain" x="20" y="100" width="24" height="110" textBox="none">
  <SliderWidget name="slider_widget" x="6" y="5" width="12" height="100"
                visualStyle="bar" trackWidth="0.7" trackColour="#ffbe8cd5"
                activeWidth="4" thumbColour="#ffc993ec">
    <Effects target="thumb"><DropShadow blur="2" colour="#ffc993ec"/></Effects>
  </SliderWidget>
</LinearSlider>
```

`LinearSlider` is vertical: minimum is at the bottom, maximum at the top.
`visualStyle="bar"` fills from minimum to the current value with rounded ends.
`visualStyle="dot"` uses a circular thumb (`thumbRadius`, default 4) and no fill.
Effects target `track` and `thumb`. Leave room around `slider_widget` for glows.
Resizing updates both drawing and JUCE's value-to-pointer mapping.

`LMSlider` shows a value bubble while hovering or dragging, controlled by
`showValueOnHover`. It also offers right-click → Enter value / Reset, Enter to
open numeric entry, arrow-key adjustment and JUCE's double-click reset. Configure
`textFromValueFunction` for bubble formatting and `setDoubleClickReturnValue`
from the parameter's actual default. Numeric entry uses **native parameter
units**, with the accepted range displayed in its title, and clamps/snaps to the
slider's range. `commitNumericText` rejects non-finite or malformed values.
Every numeric/keyboard edit emits a complete JUCE slider gesture, including to
`SliderAttachment` listeners. Call `cancelInteraction()` before rebinding or
hiding; pending dialogs/menus cannot then edit a different parameter.

## AHDSR

```xml
<AHDSR name="envelope" x="20" y="230" width="320" height="90"
       padding="10" peakInset="5" sustainWidth="0.2" minimumTimeSpan="100"
       lineColour="#ffc993ec" lineWidth="1.4"
       fillTopColour="#99c993ec" fillBottomColour="#00c993ec"
       guideColour="#ff303030" handleColour="#ffc993ec"
       handleRadius="3.6" hitRadius="10" handleSpacing="10"/>
```

Register an `LMAhdsrComponent` under that name. `Values` and `Ranges` are arrays
in `attack, hold, decay, sustain, release` order; the named `Stage` enum indexes
them. Time stages share the consumer's time unit; sustain is linear amplitude
0–1. Supply ranges with `setRanges`, and drive `setValues` or `setValue` from
message-thread parameter notifications. These setters never emit edit callbacks.

Connect `onGestureStart(Stage)`, `onValueChange(Stage, float)` and
`onGestureEnd(Stage)` to your parameter adapter. JUCE `ParameterAttachment`
provides message-thread notifications and host gesture handling; the envelope
itself does not depend on APVTS or a processor.

- Horizontal dragging adjusts each time stage. Sustain drags vertically; the
  decay endpoint edits decay horizontally and sustain vertically, beginning and
  ending both gestures.
- Stage widths show relative linear duration. The sustain preview occupies
  `sustainWidth` of the plot, not a timed sustain parameter. `minimumTimeSpan`
  prevents an unusable scale when all times are near zero.
- The time scale is frozen during dragging and fitted again at gesture end.
  `peakInset` reserves headroom within the plot. Overlapping handles are offset
  with connectors, preserving access to zero-duration stages.
- The two peak handles stay visible; other handles appear on hover or focus.
  Left/right arrows select a stage; up/down edit it with complete gestures.
  The accompanying sliders provide accessible numeric editing of every stage.
- Call `cancelGesture()` before detaching bindings. Cancellation ends each
  active gesture once; subsequent drag events are ignored.

Colours, line width, padding, guide colour and handle sizes are styled in XML.
An optional `Effects target="line"` supplies a line glow. `AutoLabel` and
registered labels also accept `letterSpacing`, expressed as a fraction of font
height (0 preserves previous text spacing).

## Tests

`LayoutEffectsTests` includes `Tests/ControlsTests.cpp`: multi-scale hit mapping,
bar/dot geometry, all symbol styles, inner-shadow compositing, collapsed AHDSR
handles, two-axis gestures and cancellation. Run `ctest --test-dir build
--output-on-failure` after building the target.

## Panel colour washes

`Rectangle` also accepts `fill-gradient-type="radial"`, using the same normalized
start/end coordinates and colour-stop syntax as existing linear fills. The start
is the centre and its distance to the end sets the radius. Coordinates retain the
existing bottom-origin Y convention. Use a transparent final stop and layer
rounded rectangles to create soft colour washes without bitmap assets:

```xml
<Rectangle name="wash" x="0" y="0" width="405" height="407" cornerRadius="8"
           fill-gradient-type="radial" fill-gradient-start="0,0.9"
           fill-gradient-end="0.6,0.5"
           fill-gradient-stops="#253b85be:0,#00000000:1"/>
```

The fill is clipped to the rounded shape; gradients without the new `radial`
value retain their existing rendering. `ControlsTests` checks falloff and clipping.

### SVG buttons

Register an `LMSvgButton` against a `SvgButton` XML element. `svg` is relative
to `LayoutManager::assetsFolder`; artwork is retained as vector drawables.
`iconSize` sets the centred square artwork area in layout units.

```xml
<SvgButton name="grid" svg="beats_icons/Grid.svg" x="0" y="0" width="32" height="32"
           iconSize="23" colorOverride="#ff9874ad" colorOverrideHover="#ffc993ec"
           colorOverrideClick="#ff35263c" colorOverrideSelected="#ff35263c"
           backgroundCircle="true" circleDiameter="21" circleColour="#00000000"
           circleColourHover="#227e568f" circleColourSelected="#ffc993ec">
  <Effects target="selected"><DropShadow blur="4" colour="#ffc993ec" opacity="0.65"/></Effects>
</SvgButton>
```

Colour overrides are optional and tint visible SVG fills and strokes while
preserving their opacity. Omit them to retain multicolour artwork. Hover and
selected fall back to the base override; click falls back to selected. Pressed
artwork takes precedence over selected, then hover, then base.
`backgroundCircle` defaults to false. Its colours are `circleColour`,
`circleColourHover`, `circleColourClick`, and `circleColourSelected`; state colours
fall back to `circleColour`. `Effects target="circle"` applies to the unselected
background and `Effects target="selected"` to the selected background. Leave room
inside the component bounds for glow tails. Toggle/radio semantics and callbacks
remain the consumer's responsibility, using the standard JUCE Button API.
