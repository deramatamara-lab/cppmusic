#include <juce_core/juce_core.h>
#include <project/PatternJsonParser.h>

class PatternJsonParserTest : public juce::UnitTest
{
public:
    PatternJsonParserTest() : juce::UnitTest("PatternJsonParser Test") {}

    void runTest() override
    {
        using daw::project::ParsedPatternFromJson;
        using daw::project::parsePatternFromJson;

        beginTest("Parses object with steps and notes array");
        {
            const juce::String json = R"JSON(
                {
                  "steps": 16,
                  "notes": [
                    { "step": 0, "note": 36, "velocity": 110, "length": 0.25, "channel": 0 },
                    { "step": 4, "note": 38, "velocity": 100, "length": 0.25, "channel": 0 },
                    { "step": 8, "note": 42, "velocity": 90,  "length": 0.25, "channel": 0 }
                  ]
                }
            )JSON";

            ParsedPatternFromJson parsed;
            const bool ok = parsePatternFromJson(json, parsed);
            expect(ok, "Parser should succeed for valid JSON object");
            expectEquals(parsed.numSteps, 16, "numSteps should match declared steps");
            expectEquals(parsed.notes.size(), size_t(3), "Should parse three notes");
            if (parsed.notes.size() == 3)
            {
                expectEquals(static_cast<int>(parsed.notes[0].note), 36, "First note pitch matches");
                expectWithinAbsoluteError(parsed.notes[0].startBeat, 0.0, 1e-6, "First note at step 0");
                expectWithinAbsoluteError(parsed.notes[1].startBeat, 4.0, 1e-6, "Second note at step 4");
            }
        }

        beginTest("Parses top-level array and infers steps");
        {
            const juce::String json = R"JSON(
                [
                  { "step": 0,  "note": 60 },
                  { "step": 7,  "note": 62 },
                  { "step": 15, "note": 64 }
                ]
            )JSON";

            ParsedPatternFromJson parsed;
            const bool ok = parsePatternFromJson(json, parsed);
            expect(ok, "Parser should succeed for top-level array");
            expect(parsed.numSteps >= 16, "numSteps should be at least max step + 1");
            expectEquals(parsed.notes.size(), size_t(3), "Should parse three notes");
        }

        beginTest("Rejects invalid JSON");
        {
            ParsedPatternFromJson parsed;
            const bool ok = parsePatternFromJson("not json", parsed);
            expect(!ok, "Parser should fail for invalid JSON");
            expectEquals(parsed.numSteps, 0, "numSteps should be reset on failure");
            expectEquals(parsed.notes.size(), size_t(0), "No notes on failure");
        }
    }
};

static PatternJsonParserTest patternJsonParserTest;
