#pragma once

// Cached interpolation of single closed SVG contours in a centred unit square.
// See README_Controls.md for the SvgInterpolator / Circle / Path XML format.
class LMSvgInterpolator
{
public:
    bool setSource (const juce::ValueTree&);
    juce::Path createPath (float position) const;

private:
    struct Frame
    {
        float position = 0, circleRadius = 0;
        bool circle = false;
        std::vector<juce::Point<float>> points;
    };
    juce::ValueTree source;
    std::vector<Frame> frames;
};
