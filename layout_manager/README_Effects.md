# Drop shadows and outer glows

Effects are opt-in children of layout elements. A zero-offset drop shadow is an
outer glow. Keep the fill/stroke/text drawing separate: effects draw behind it.

```xml
<Ellipse name="indicator" x="20" y="20" width="30" height="30" fillColour="#ffc993ec">
  <Effects>
    <DropShadow x="0" y="0" blur="6" spread="0" colour="#ffc993ec" opacity="0.6"/>
  </Effects>
</Ellipse>
```

`DropShadow` supports `x`, `y`, `blur`, `spread`, `colour`, and `opacity`.
Dimensions are floating-point **layout units**; x/y are offsets from the source,
not absolute positions. Defaults are zero for dimensions, black for colour, and
1 for opacity. Colour uses the existing JUCE `#AARRGGBB` convention. Colour alpha
multiplies opacity (0 transparent, 1 opaque). Negative blur clamps to zero;
opacity clamps to [0, 1]. Invalid/non-finite numbers use defaults. Signed spread
expands or contracts the shadow only; it never changes the source artwork.

Multiple DropShadow children paint in declaration order, later shadows over
earlier ones, all behind the artwork. Unknown effect tags are ignored.

## Built-in targets

- `Rectangle`, `Ellipse`, `Line`, `AutoLabel`, and registered `Label`: an
  untargeted `Effects` list on the element.
- `RotarySlider`: put `Effects target="circle"`, `Effects target="arcActive"`,
  or `Effects target="arcBg"` inside its `slider_widget` child. These affect only
  that piece of geometry, not the whole slider rectangle.
- Other/custom elements can use any target name through the C++ API; target
  names are selectors, not an automatic interaction/state system.

```xml
<SliderWidget name="slider_widget" x="12" y="12" width="76" height="76"
              arcActiveColour="#ffc993ec" arcActiveWidth="4">
  <Effects target="arcActive">
    <DropShadow blur="6" spread="1" colour="#ffc993ec" opacity="0.7"/>
  </Effects>
</SliderWidget>
<AutoLabel name="caption" x="20" y="110" width="140" height="40"
           text="Hello world" fontSize="16" fillColour="#ffffffff">
  <Effects><DropShadow x="2" y="3" blur="4" opacity="0.5"/></Effects>
</AutoLabel>
```

Text effects follow the fitted glyph arrangement, including wrapping and
justification. Registered labels retain normal editing and disabled-state
behavior. Existing layouts without effects keep their normal drawing path.
Ellipse supports solid fill, stroke, opacity and effects.

## Custom components and caching

Declare `LMEffectRenderer glow;` as a component member, one renderer per visual
target. Configure it when the style changes:

```cpp
glow.setEffects (layout.getEffects ("bank_selector", "selected"));
```

For a live binding, use the element's ValueTree instead:

```cpp
glow.setSource (findComponentByName ("bank_selector", layout.layoutTree), "selected");
```

This listens for changes to that subtree, including edits, additions, removal,
and reordering. Rebind after replacing the layout tree. `setEffects` disconnects
a previous live binding. LayoutManager rebinds its built-in controls on reload.
All configuration and rendering belong on the JUCE message thread.

In paint, supply geometry already scaled into the graphics context's local
coordinates. Pass the layout scale separately for the effect dimensions:

```cpp
glow.render (g, circlePath, layout.scaling);
g.setColour (accent);
g.fillPath (circlePath);
```

The other overloads accept `juce::PathStrokeType` or `juce::GlyphArrangement`.
Prepare text with the same font, bounds and fitting arguments as its foreground.
Stroke spread changes stroke thickness by twice the signed spread, preserving
its centreline and end/join styles; collapsed strokes produce no shadow.

Renderers retain Melatonin's blur caches. Reapplying identical styles keeps
them; colour/opacity/offset changes reuse the blur and update compositing.
Geometry, blur, spread, or device-scale changes regenerate the affected blur.
Layout dimensions round with `juce::roundToInt` before entering the pinned
integer backend. Device scaling is handled by Melatonin, separately from layout
scaling. Filled-path spread uses Melatonin's bounds-based path expansion;
complex shapes/glyphs and blur kernels can differ from exact Figma pixels.

## Clipping and repainting

Effects respect the caller's Graphics clip and JUCE ancestor clipping. Painted
primitives are not additionally clipped to their own XML rectangles. Registered
controls still have their normal component clip: leave drawing room inside the
control, or render the effect in a parent before painting child controls.

`getRenderBounds(sourceBounds, layoutScale)` returns the union of source and
conservative effect bounds. For strokes, pass the stroked source bounds. Use
this for parent repaint regions, including both old and new bounds when moving
or changing an effect. This preserves hit areas and layout positions. It does
not resize components or change their input behavior automatically.

## Offscreen tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DJUCE_DIR=/path/to/JUCE
cmake --build build --target LayoutEffectsTests
ctest --test-dir build --output-on-failure
```

The JUCE console test target needs no external test framework and writes small
PNG fixtures to `build/effect-renders/` at 1x, 1.5x, and 2x. It also reports an
informational cached-versus-cold rendering comparison. Disable the target with
`-DLAYOUT_MANAGER_BUILD_TESTS=OFF`. Consumers of the module do not build it.

## Inner shadows

`InnerShadow` accepts the same attributes as `DropShadow`. It is clipped to the
source shape and does not expand `getRenderBounds()`. Built-in filled rectangles,
ellipses and rotary backplates composite it after their fill and before their
stroke. Existing outer-shadow ordering is unchanged.

For custom components, call `render()` before the fill and `renderInner()` after
the fill. These are separate passes so an inner shadow cannot be covered by its
own source artwork. Each layer retains its Melatonin cache. See
`README_Controls.md` for XML slider and envelope examples.
