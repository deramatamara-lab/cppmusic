#pragma once
/**
 * @file DemoProjects.hpp
 * @brief Built-in demo projects for quick-start experience.
 *
 * Provides factory functions to create demo projects with pre-configured
 * patterns, tracks, and clips for showcasing DAW functionality.
 */

#include "../project/ProjectModel.h"
#include <memory>

namespace daw::model
{

/**
 * @brief Factory for creating built-in demo projects.
 *
 * Demo projects provide a quick-start experience for users, demonstrating
 * the DAW's capabilities without requiring any initial setup.
 */
class DemoProjects
{
public:
    /**
     * @brief Create a simple demo project with basic drum pattern.
     *
     * Contents:
     * - 4 channels (Kick, Snare, Hi-Hat, Synth Bass)
     * - A 4-bar pattern with basic drum sequence and simple bass line
     * - Playlist arrangement with repeated pattern clips
     *
     * @return A fully configured ProjectModel ready for playback.
     */
    static std::shared_ptr<daw::project::ProjectModel> createSimpleDemoProject();

    /**
     * @brief Create a more complex demo project with multiple patterns.
     *
     * Contents:
     * - 8 channels with various instruments
     * - Multiple patterns (intro, verse, chorus)
     * - Complex playlist arrangement
     *
     * @return A fully configured ProjectModel ready for playback.
     */
    static std::shared_ptr<daw::project::ProjectModel> createAdvancedDemoProject();

    /**
     * @brief Create a minimal demo with just one pattern for testing.
     *
     * Contents:
     * - 1 channel (Synth)
     * - Simple 1-bar pattern with a few notes
     *
     * @return A minimal ProjectModel for testing.
     */
    static std::shared_ptr<daw::project::ProjectModel> createMinimalDemoProject();

private:
    // Helper to create a basic 4-on-the-floor kick pattern
    static void addKickPattern(daw::project::Pattern& pattern);
    
    // Helper to create a basic snare backbeat
    static void addSnarePattern(daw::project::Pattern& pattern);
    
    // Helper to create hi-hat pattern
    static void addHiHatPattern(daw::project::Pattern& pattern);
    
    // Helper to create a simple bass line
    static void addBassPattern(daw::project::Pattern& pattern);
};

} // namespace daw::model
