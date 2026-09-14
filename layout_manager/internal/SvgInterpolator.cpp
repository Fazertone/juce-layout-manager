#include "SvgInterpolator.h"
#include <algorithm>
#include <cmath>

bool LMSvgInterpolator::setSource (const juce::ValueTree& tree)
{
    if (source.isValid() && source.isEquivalentTo (tree))
        return ! frames.empty();
    source = tree.createCopy();
    frames.clear();
    std::vector<Frame> parsed;
    for (const auto& child : tree)
    {
        Frame frame;
        frame.position = (float) child.getProperty ("at", -1.0f);
        if (! std::isfinite (frame.position) || frame.position < 0 || frame.position > 1)
            return false;
        frame.circle = child.hasType ("Circle");
        if (frame.circle)
        {
            frame.circleRadius = (float) child.getProperty ("radius", 0.1f);
            if (! std::isfinite (frame.circleRadius) || frame.circleRadius <= 0)
                return false;
        }
        else
        {
            if (! child.hasType ("Path")) return false;
            auto viewBox = juce::StringArray::fromTokens (child.getProperty ("viewBox").toString(), " ,\t\r\n", "");
            viewBox.removeEmptyStrings();
            if (viewBox.size() != 4) return false;
            const auto x = viewBox[0].getFloatValue(), y = viewBox[1].getFloatValue();
            const auto width = viewBox[2].getFloatValue(), height = viewBox[3].getFloatValue();
            const auto scale = (float) child.getProperty ("scale", 1.0f);
            if (! std::isfinite (x + y + width + height + scale) || width <= 0 || height <= 0 || scale <= 0)
                return false;
            auto path = juce::Drawable::parseSVGPath (child.getProperty ("d").toString());
            // Multiple contours and open paths have no unambiguous point correspondence.
            int starts = 0, closes = 0;
            for (juce::Path::Iterator it (path); it.next();)
            {
                starts += it.elementType == juce::Path::Iterator::startNewSubPath;
                closes += it.elementType == juce::Path::Iterator::closePath;
            }
            if (starts != 1 || closes != 1) return false;
            path.applyTransform (juce::AffineTransform::translation (-x - width * 0.5f, -y - height * 0.5f)
                                     .scaled (2.0f * scale / juce::jmax (width, height)));
            juce::PathFlatteningIterator it (path, {}, 0.002f);
            while (it.next())
            {
                const juce::Point<float> point { it.x1, it.y1 };
                if (! std::isfinite (point.x) || ! std::isfinite (point.y)) return false;
                if (frame.points.empty() || point != frame.points.back()) frame.points.push_back (point);
            }
            if (frame.points.size() > 1 && frame.points.front() == frame.points.back()) frame.points.pop_back();
            if (frame.points.size() < 3 || path.getLength() <= 0) return false;
        }
        parsed.push_back (std::move (frame));
    }
    if (parsed.size() < 2) return false;
    std::sort (parsed.begin(), parsed.end(), [] (const auto& a, const auto& b) { return a.position < b.position; });
    for (size_t i = 1; i < parsed.size(); ++i)
        if (parsed[i].position == parsed[i - 1].position) return false;

    size_t count = 0;
    bool matching = true;
    for (const auto& frame : parsed)
        if (! frame.circle)
        {
            if (count != 0 && count != frame.points.size()) matching = false;
            count = frame.points.size();
        }

    // Matching SVG vertices interpolate directly. Otherwise merge all perimeter
    // landmarks, so resampling retains every corner at every authored keyframe.
    std::vector<float> fractions;
    if (! matching)
    {
        std::vector<std::vector<float>> distances (parsed.size());
        for (size_t i = 0; i < parsed.size(); ++i)
        {
            const auto& points = parsed[i].points;
            auto& cumulative = distances[i];
            cumulative.push_back (0);
            for (size_t p = 0; p < points.size(); ++p)
                cumulative.push_back (cumulative.back() + points[p].getDistanceFrom (points[(p + 1) % points.size()]));
            if (points.empty()) continue;
            const auto total = cumulative.back();
            if (total <= 0) return false;
            for (auto& distance : cumulative) distance /= total;
            fractions.insert (fractions.end(), cumulative.begin(), cumulative.end() - 1);
        }
        std::sort (fractions.begin(), fractions.end());
        fractions.erase (std::unique (fractions.begin(), fractions.end()), fractions.end());
        for (size_t i = 0; i < parsed.size(); ++i)
        {
            auto& frame = parsed[i];
            if (frame.circle) continue;
            std::vector<juce::Point<float>> points;
            size_t edge = 0;
            for (const auto fraction : fractions)
            {
                const auto& cumulative = distances[i];
                while (edge + 1 < frame.points.size() && cumulative[edge + 1] <= fraction) ++edge;
                const auto mix = (fraction - cumulative[edge]) / (cumulative[edge + 1] - cumulative[edge]);
                points.push_back (frame.points[edge] + (frame.points[(edge + 1) % frame.points.size()] - frame.points[edge]) * mix);
            }
            frame.points = std::move (points);
        }
        count = fractions.size();
    }
    if (count == 0) count = 128;
    // Keep even a triangle-to-circle morph round at its circle keyframe.
    if (std::any_of (parsed.begin(), parsed.end(), [] (const auto& frame) { return frame.circle; }))
    {
        const auto subdivisions = (size_t) std::ceil (128.0f / (float) count);
        for (auto& frame : parsed)
        {
            if (frame.circle) continue;
            std::vector<juce::Point<float>> points;
            for (size_t i = 0; i < count; ++i)
                for (size_t part = 0; part < subdivisions; ++part)
                    points.push_back (frame.points[i] + (frame.points[(i + 1) % count] - frame.points[i])
                                                        * ((float) part / (float) subdivisions));
            frame.points = std::move (points);
        }
        if (! fractions.empty())
        {
            std::vector<float> refined;
            for (size_t i = 0; i < count; ++i)
                for (size_t part = 0; part < subdivisions; ++part)
                    refined.push_back (juce::jmap ((float) part / (float) subdivisions, fractions[i],
                                                    i + 1 < count ? fractions[i + 1] : 1.0f));
            fractions = std::move (refined);
        }
        count *= subdivisions;
    }
    float phase = 0, direction = 1;
    for (const auto& frame : parsed)
        if (! frame.circle)
        {
            phase = std::atan2 (frame.points.front().x, -frame.points.front().y);
            float area = 0;
            for (size_t i = 0; i < frame.points.size(); ++i)
            {
                const auto a = frame.points[i], b = frame.points[(i + 1) % frame.points.size()];
                area += a.x * b.y - b.x * a.y;
            }
            direction = area < 0 ? -1.0f : 1.0f;
            break;
        }
    for (auto& frame : parsed)
        if (frame.circle)
            for (size_t i = 0; i < count; ++i)
            {
                const auto fraction = fractions.empty() ? (float) i / (float) count : fractions[i];
                const auto angle = phase + direction * juce::MathConstants<float>::twoPi * fraction;
                frame.points.emplace_back (std::sin (angle) * frame.circleRadius, -std::cos (angle) * frame.circleRadius);
            }
    frames = std::move (parsed);
    return true;
}

juce::Path LMSvgInterpolator::createPath (float position) const
{
    juce::Path path;
    if (frames.empty() || ! std::isfinite (position)) return path;
    position = juce::jlimit (frames.front().position, frames.back().position, position);
    size_t upper = 1;
    while (upper + 1 < frames.size() && frames[upper].position < position) ++upper;
    const auto& from = frames[upper - 1];
    const auto& to = frames[upper];
    const auto mix = (position - from.position) / (to.position - from.position);
    for (size_t i = 0; i < from.points.size(); ++i)
    {
        const auto point = from.points[i] + (to.points[i] - from.points[i]) * mix;
        if (i == 0) path.startNewSubPath (point);
        else path.lineTo (point);
    }
    path.closeSubPath();
    return path;
}
