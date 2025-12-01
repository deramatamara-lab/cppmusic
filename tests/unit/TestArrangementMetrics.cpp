/**
 * @file TestArrangementMetrics.cpp
 * @brief Tests for arrangement analysis metrics
 */

#include <gtest/gtest.h>
#include <cmath>
#include <memory>
#include <vector>

// Minimal stub for testing arrangement metrics
namespace cppmusic::ai::arrangement {

struct PatternInfo {
    std::string id;
    double startTime{0.0};
    double duration{0.0};
    int track{0};
    std::vector<int> pitchClasses;  // 0-11
};

struct ArrangementMetrics {
    double variationScore{0.0};      // 0-1, how varied the arrangement is
    double repetitionScore{0.0};     // 0-1, how repetitive
    double energyCurve{0.0};         // -1 to 1, energy direction
    double harmonicDensity{0.0};     // 0-1, harmonic complexity
    double rhythmicDensity{0.0};     // 0-1, note density
};

class ArrangementAnalyzerStub {
public:
    ArrangementMetrics analyze(const std::vector<PatternInfo>& patterns) {
        ArrangementMetrics metrics;
        
        if (patterns.empty()) {
            return metrics;
        }
        
        // Calculate variation score based on unique patterns
        std::set<std::string> uniqueIds;
        for (const auto& p : patterns) {
            uniqueIds.insert(p.id);
        }
        metrics.variationScore = static_cast<double>(uniqueIds.size()) / patterns.size();
        
        // Repetition is inverse of variation
        metrics.repetitionScore = 1.0 - metrics.variationScore;
        
        // Harmonic density based on pitch class usage
        std::set<int> usedPitchClasses;
        for (const auto& p : patterns) {
            for (int pc : p.pitchClasses) {
                usedPitchClasses.insert(pc % 12);
            }
        }
        metrics.harmonicDensity = static_cast<double>(usedPitchClasses.size()) / 12.0;
        
        // Energy curve (simple: check if patterns get denser over time)
        if (patterns.size() >= 2) {
            // Count patterns in first and second half
            double midTime = patterns.back().startTime / 2.0;
            int firstHalf = 0, secondHalf = 0;
            for (const auto& p : patterns) {
                if (p.startTime < midTime) firstHalf++;
                else secondHalf++;
            }
            metrics.energyCurve = (secondHalf - firstHalf) / static_cast<double>(patterns.size());
        }
        
        // Rhythmic density
        double totalDuration = 0;
        double totalPatternTime = 0;
        for (const auto& p : patterns) {
            totalDuration = std::max(totalDuration, p.startTime + p.duration);
            totalPatternTime += p.duration;
        }
        if (totalDuration > 0) {
            metrics.rhythmicDensity = std::min(1.0, totalPatternTime / totalDuration);
        }
        
        return metrics;
    }
    
private:
    std::set<int> pitchClasses_;
};

}  // namespace cppmusic::ai::arrangement

using namespace cppmusic::ai::arrangement;

class ArrangementMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        analyzer = std::make_unique<ArrangementAnalyzerStub>();
    }
    
    std::unique_ptr<ArrangementAnalyzerStub> analyzer;
    
    PatternInfo createPattern(const std::string& id, double start, double dur,
                              int track, std::vector<int> pitches = {}) {
        PatternInfo p;
        p.id = id;
        p.startTime = start;
        p.duration = dur;
        p.track = track;
        p.pitchClasses = std::move(pitches);
        return p;
    }
};

TEST_F(ArrangementMetricsTest, EmptyArrangement) {
    std::vector<PatternInfo> patterns;
    auto metrics = analyzer->analyze(patterns);
    
    EXPECT_DOUBLE_EQ(metrics.variationScore, 0.0);
    EXPECT_DOUBLE_EQ(metrics.repetitionScore, 0.0);
    EXPECT_DOUBLE_EQ(metrics.energyCurve, 0.0);
}

TEST_F(ArrangementMetricsTest, SinglePattern) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 4, 0, {0, 4, 7})  // C major chord
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    EXPECT_DOUBLE_EQ(metrics.variationScore, 1.0);  // All unique
    EXPECT_DOUBLE_EQ(metrics.repetitionScore, 0.0);
}

TEST_F(ArrangementMetricsTest, IdenticalPatternsRepetitive) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 4, 0),
        createPattern("A", 4, 4, 0),
        createPattern("A", 8, 4, 0),
        createPattern("A", 12, 4, 0)
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    EXPECT_DOUBLE_EQ(metrics.variationScore, 0.25);  // 1 unique / 4 total
    EXPECT_DOUBLE_EQ(metrics.repetitionScore, 0.75);
}

TEST_F(ArrangementMetricsTest, AllUniquePatternsVaried) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 4, 0),
        createPattern("B", 4, 4, 0),
        createPattern("C", 8, 4, 0),
        createPattern("D", 12, 4, 0)
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    EXPECT_DOUBLE_EQ(metrics.variationScore, 1.0);  // All unique
    EXPECT_DOUBLE_EQ(metrics.repetitionScore, 0.0);
}

TEST_F(ArrangementMetricsTest, HarmonicDensityMajorTriad) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 4, 0, {0, 4, 7})  // C major: 3 pitch classes
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    EXPECT_NEAR(metrics.harmonicDensity, 3.0/12.0, 0.001);  // 3 of 12
}

TEST_F(ArrangementMetricsTest, HarmonicDensityChromatic) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 4, 0, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11})
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    EXPECT_DOUBLE_EQ(metrics.harmonicDensity, 1.0);  // All 12 pitch classes
}

TEST_F(ArrangementMetricsTest, EnergyCurveIncreasing) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 1, 0),    // First half: 1 pattern
        createPattern("B", 4, 1, 0),    // Second half: 3 patterns
        createPattern("C", 5, 1, 0),
        createPattern("D", 6, 1, 0)
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    // Second half has more patterns, energy should be positive
    EXPECT_GT(metrics.energyCurve, 0.0);
}

TEST_F(ArrangementMetricsTest, EnergyCurveDecreasing) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 1, 0),    // First half: 3 patterns
        createPattern("B", 1, 1, 0),
        createPattern("C", 2, 1, 0),
        createPattern("D", 6, 1, 0)     // Second half: 1 pattern
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    // First half has more patterns, energy should be negative
    EXPECT_LT(metrics.energyCurve, 0.0);
}

TEST_F(ArrangementMetricsTest, RhythmicDensityNoOverlap) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 4, 0),
        createPattern("B", 4, 4, 0),
        createPattern("C", 8, 4, 0),
        createPattern("D", 12, 4, 0)
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    EXPECT_NEAR(metrics.rhythmicDensity, 1.0, 0.001);  // Fully packed
}

TEST_F(ArrangementMetricsTest, RhythmicDensitySparse) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 2, 0),
        createPattern("B", 8, 2, 0)
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    // 4 units of pattern time over 10 units total = 0.4
    EXPECT_NEAR(metrics.rhythmicDensity, 0.4, 0.001);
}

TEST_F(ArrangementMetricsTest, DeterministicAnalysis) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 4, 0, {0, 4, 7}),
        createPattern("B", 4, 4, 0, {2, 5, 9}),
        createPattern("A", 8, 4, 0, {0, 4, 7}),
        createPattern("C", 12, 4, 0, {4, 7, 11})
    };
    
    auto metrics1 = analyzer->analyze(patterns);
    auto metrics2 = analyzer->analyze(patterns);
    
    // Results should be identical
    EXPECT_DOUBLE_EQ(metrics1.variationScore, metrics2.variationScore);
    EXPECT_DOUBLE_EQ(metrics1.repetitionScore, metrics2.repetitionScore);
    EXPECT_DOUBLE_EQ(metrics1.energyCurve, metrics2.energyCurve);
    EXPECT_DOUBLE_EQ(metrics1.harmonicDensity, metrics2.harmonicDensity);
    EXPECT_DOUBLE_EQ(metrics1.rhythmicDensity, metrics2.rhythmicDensity);
}

TEST_F(ArrangementMetricsTest, MixedVariation) {
    std::vector<PatternInfo> patterns = {
        createPattern("A", 0, 4, 0),
        createPattern("A", 4, 4, 0),
        createPattern("B", 8, 4, 0),
        createPattern("B", 12, 4, 0)
    };
    
    auto metrics = analyzer->analyze(patterns);
    
    EXPECT_DOUBLE_EQ(metrics.variationScore, 0.5);  // 2 unique / 4 total
    EXPECT_DOUBLE_EQ(metrics.repetitionScore, 0.5);
}
