/**
 * @file DemoProjects.cpp
 * @brief Implementation of DemoProjects factory.
 */

#include "DemoProjects.hpp"
#include <juce_graphics/juce_graphics.h>

namespace daw::model
{

std::shared_ptr<daw::project::ProjectModel> DemoProjects::createSimpleDemoProject()
{
    auto project = std::make_shared<daw::project::ProjectModel>();
    
    // Create tracks with distinct colors
    auto* kickTrack = project->addTrack("Kick", juce::Colour(0xFFE53935));     // Red
    auto* snareTrack = project->addTrack("Snare", juce::Colour(0xFFFB8C00));   // Orange
    auto* hihatTrack = project->addTrack("Hi-Hat", juce::Colour(0xFFFFEB3B));  // Yellow
    auto* bassTrack = project->addTrack("Bass", juce::Colour(0xFF43A047));     // Green
    
    // Create patterns for each instrument
    auto* kickPattern = project->addPattern("Kick Pattern", 16);
    auto* snarePattern = project->addPattern("Snare Pattern", 16);
    auto* hihatPattern = project->addPattern("Hi-Hat Pattern", 16);
    auto* bassPattern = project->addPattern("Bass Pattern", 16);
    
    // Add notes to patterns
    addKickPattern(*kickPattern);
    addSnarePattern(*snarePattern);
    addHiHatPattern(*hihatPattern);
    addBassPattern(*bassPattern);
    
    // Create clips on the playlist (4 bars = 16 beats at 4/4)
    // Repeat the pattern twice for an 8-bar arrangement
    
    // Kick clips
    auto* kickClip1 = project->addClip(kickTrack->getId(), 0.0, 16.0, "Kick 1");
    project->linkClipToPattern(kickClip1->getId(), kickPattern->getId());
    auto* kickClip2 = project->addClip(kickTrack->getId(), 16.0, 16.0, "Kick 2");
    project->linkClipToPattern(kickClip2->getId(), kickPattern->getId());
    
    // Snare clips
    auto* snareClip1 = project->addClip(snareTrack->getId(), 0.0, 16.0, "Snare 1");
    project->linkClipToPattern(snareClip1->getId(), snarePattern->getId());
    auto* snareClip2 = project->addClip(snareTrack->getId(), 16.0, 16.0, "Snare 2");
    project->linkClipToPattern(snareClip2->getId(), snarePattern->getId());
    
    // Hi-hat clips
    auto* hihatClip1 = project->addClip(hihatTrack->getId(), 0.0, 16.0, "Hi-Hat 1");
    project->linkClipToPattern(hihatClip1->getId(), hihatPattern->getId());
    auto* hihatClip2 = project->addClip(hihatTrack->getId(), 16.0, 16.0, "Hi-Hat 2");
    project->linkClipToPattern(hihatClip2->getId(), hihatPattern->getId());
    
    // Bass clips (enter later for a build-up effect)
    auto* bassClip1 = project->addClip(bassTrack->getId(), 16.0, 16.0, "Bass 1");
    project->linkClipToPattern(bassClip1->getId(), bassPattern->getId());
    
    return project;
}

std::shared_ptr<daw::project::ProjectModel> DemoProjects::createAdvancedDemoProject()
{
    auto project = std::make_shared<daw::project::ProjectModel>();
    
    // Create 8 tracks
    project->addTrack("Kick", juce::Colour(0xFFE53935));
    project->addTrack("Snare", juce::Colour(0xFFFB8C00));
    project->addTrack("Hi-Hat", juce::Colour(0xFFFFEB3B));
    project->addTrack("Clap", juce::Colour(0xFFAB47BC));
    project->addTrack("Bass", juce::Colour(0xFF43A047));
    project->addTrack("Lead", juce::Colour(0xFF1E88E5));
    project->addTrack("Pad", juce::Colour(0xFF5C6BC0));
    project->addTrack("FX", juce::Colour(0xFF00ACC1));
    
    // Create multiple patterns
    auto* introPattern = project->addPattern("Intro", 16);
    auto* versePattern = project->addPattern("Verse", 16);
    auto* chorusPattern = project->addPattern("Chorus", 16);
    
    // Add basic content to intro pattern
    addKickPattern(*introPattern);
    addHiHatPattern(*introPattern);
    
    // Verse has more elements
    addKickPattern(*versePattern);
    addSnarePattern(*versePattern);
    addHiHatPattern(*versePattern);
    addBassPattern(*versePattern);
    
    // Chorus is fullest
    addKickPattern(*chorusPattern);
    addSnarePattern(*chorusPattern);
    addHiHatPattern(*chorusPattern);
    addBassPattern(*chorusPattern);
    
    return project;
}

std::shared_ptr<daw::project::ProjectModel> DemoProjects::createMinimalDemoProject()
{
    auto project = std::make_shared<daw::project::ProjectModel>();
    
    // Single track
    auto* synthTrack = project->addTrack("Synth", juce::Colour(0xFF1E88E5));
    
    // Single pattern with a simple melody
    auto* pattern = project->addPattern("Minimal", 4);  // 1 bar
    
    // C major arpeggio
    pattern->addNote({60, 100, 0.0, 0.25, 0});   // C4
    pattern->addNote({64, 100, 0.5, 0.25, 0});   // E4
    pattern->addNote({67, 100, 1.0, 0.25, 0});   // G4
    pattern->addNote({72, 100, 1.5, 0.25, 0});   // C5
    pattern->addNote({67, 100, 2.0, 0.25, 0});   // G4
    pattern->addNote({64, 100, 2.5, 0.25, 0});   // E4
    pattern->addNote({60, 100, 3.0, 0.25, 0});   // C4
    
    // Create a clip
    auto* clip = project->addClip(synthTrack->getId(), 0.0, 4.0, "Synth Clip");
    project->linkClipToPattern(clip->getId(), pattern->getId());
    
    return project;
}

void DemoProjects::addKickPattern(daw::project::Pattern& pattern)
{
    // 4-on-the-floor kick pattern
    // Kicks on beats 1, 2, 3, 4 (positions 0, 4, 8, 12 in 16th notes)
    pattern.addNote({36, 127, 0.0, 0.25, 0});   // Beat 1
    pattern.addNote({36, 120, 4.0, 0.25, 0});   // Beat 2
    pattern.addNote({36, 127, 8.0, 0.25, 0});   // Beat 3
    pattern.addNote({36, 120, 12.0, 0.25, 0});  // Beat 4
}

void DemoProjects::addSnarePattern(daw::project::Pattern& pattern)
{
    // Snare on beats 2 and 4
    pattern.addNote({38, 110, 4.0, 0.25, 0});   // Beat 2
    pattern.addNote({38, 115, 12.0, 0.25, 0});  // Beat 4
}

void DemoProjects::addHiHatPattern(daw::project::Pattern& pattern)
{
    // 8th note hi-hats
    for (int i = 0; i < 16; i += 2)
    {
        uint8_t velocity = (i % 4 == 0) ? 100 : 80;  // Accent on beats
        pattern.addNote({42, velocity, static_cast<double>(i), 0.25, 0});
    }
}

void DemoProjects::addBassPattern(daw::project::Pattern& pattern)
{
    // Simple bass line following a I-IV-V-I progression
    pattern.addNote({36, 100, 0.0, 1.0, 0});    // C2 (root)
    pattern.addNote({41, 100, 4.0, 1.0, 0});    // F2 (fourth)
    pattern.addNote({43, 100, 8.0, 1.0, 0});    // G2 (fifth)
    pattern.addNote({36, 100, 12.0, 1.0, 0});   // C2 (root)
}

} // namespace daw::model
