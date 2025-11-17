#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

int main(int argc, char* argv[])
{
    juce::ignoreUnused(argc, argv);
    
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    return 0;
}

