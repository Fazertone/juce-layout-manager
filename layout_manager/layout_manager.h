/*
  ==============================================================================

   BEGIN_JUCE_MODULE_DECLARATION

    ID:                 my_shared_code
    vendor:             MyCompany
    version:            1.0.0
    name:               My Shared Utilities
    description:        Shared code for my audio plugins.
    website:            http://www.mycompany.com
    license:            GPL/Commercial

    dependencies:       juce_core juce_audio_basics melatonin_blur

   END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

// 1. Include necessary JUCE modules
#include <juce_graphics/juce_graphics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
// include Melatonin Blur
#include <melatonin_blur/melatonin_blur.h>

// 2. Include your internal headers
#include "internal/LayoutManager.h"
#include "internal/LMLookAndFeel.h"
#include "internal/CustomComponents.h"
#include "internal/Inspector.h"