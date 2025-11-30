#!/usr/bin/env python3
"""
Performance Regression Checker
Analyzes current performance results against baseline and detects regressions
"""

import json
import argparse
import sys
import csv
from typing import Dict, List


def load_current_results(filepath: str) -> Dict:
    """Load current performance results from JSON"""
    try:
        with open(filepath, "r") as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading current results: {e}")
        return {}


def load_baseline_results(filepath: str) -> Dict[str, float]:
    """Load baseline results from CSV"""
    baseline = {}
    try:
        with open(filepath, "r") as f:
            reader = csv.reader(f)
            for row in reader:
                if len(row) >= 2:
                    try:
                        baseline[row[0]] = float(row[1])
                    except ValueError:
                        continue
    except Exception as e:
        print(f"Warning: Could not load baseline: {e}")
    return baseline


def analyze_regressions(
    current: Dict, baseline: Dict[str, float], threshold: float
) -> List[Dict]:
    """Analyze performance regressions"""
    regressions = []

    current_times = current.get("averageTimes", {})

    for metric_name, current_time in current_times.items():
        if metric_name not in baseline:
            continue

        baseline_time = baseline[metric_name]
        change_percent = ((current_time - baseline_time) / baseline_time) * 100

        regression_info = {
            "testName": metric_name,
            "currentValue": current_time,
            "baselineValue": baseline_time,
            "percentageChange": change_percent,
            "isRegression": change_percent > threshold,
            "isImprovement": change_percent < -threshold,
            "severity": categorize_severity(change_percent, threshold),
        }

        regressions.append(regression_info)

    # Sort by severity (regressions first, then by percentage change)
    regressions.sort(key=lambda x: (not x["isRegression"], -abs(x["percentageChange"])))

    return regressions


def categorize_severity(change_percent: float, threshold: float) -> str:
    """Categorize regression severity"""
    abs_change = abs(change_percent)

    if change_percent > 0:  # Performance degradation
        if abs_change > threshold * 3:
            return "critical"
        elif abs_change > threshold * 2:
            return "major"
        elif abs_change > threshold:
            return "minor"
        else:
            return "negligible"
    else:  # Performance improvement
        if abs_change > threshold * 2:
            return "major_improvement"
        elif abs_change > threshold:
            return "minor_improvement"
        else:
            return "negligible"


def generate_regression_report(regressions: List[Dict], output_path: str):
    """Generate regression report in JSON format"""
    report = {
        "timestamp": str(datetime.now()),
        "summary": {
            "total_metrics": len(regressions),
            "regressions": len([r for r in regressions if r["isRegression"]]),
            "improvements": len([r for r in regressions if r["isImprovement"]]),
            "critical_regressions": len(
                [r for r in regressions if r["severity"] == "critical"]
            ),
            "major_regressions": len(
                [r for r in regressions if r["severity"] == "major"]
            ),
        },
        "details": regressions,
    }

    with open(output_path, "w") as f:
        json.dump(report, f, indent=2)

    return report


def print_regression_summary(regressions: List[Dict]):
    """Print a human-readable summary to console"""
    print("\n" + "=" * 60)
    print("           PERFORMANCE REGRESSION ANALYSIS")
    print("=" * 60)

    # Summary counts
    total = len(regressions)
    regressions_count = len([r for r in regressions if r["isRegression"]])
    improvements_count = len([r for r in regressions if r["isImprovement"]])
    critical_count = len([r for r in regressions if r["severity"] == "critical"])
    major_count = len([r for r in regressions if r["severity"] == "major"])

    print("\nSUMMARY:")
    print(f"  Total metrics analyzed: {total}")
    print(f"  🔴 Regressions found: {regressions_count}")
    print(f"  🟢 Improvements found: {improvements_count}")
    print(f"  ⚠️  Critical regressions: {critical_count}")
    print(f"  🟡 Major regressions: {major_count}")

    if critical_count > 0:
        print("\n❌ CRITICAL REGRESSIONS DETECTED!")
        for regression in regressions:
            if regression["severity"] == "critical":
                print(
                    f"  • {regression['testName']}: {regression['percentageChange']:+.1f}% "
                    f"({regression['currentValue']:.2f}ms vs {regression['baselineValue']:.2f}ms)"
                )

    if major_count > 0:
        print("\n⚠️  MAJOR REGRESSIONS:")
        for regression in regressions:
            if regression["severity"] == "major":
                print(
                    f"  • {regression['testName']}: {regression['percentageChange']:+.1f}% "
                    f"({regression['currentValue']:.2f}ms vs {regression['baselineValue']:.2f}ms)"
                )

    # Show top improvements
    improvements = [r for r in regressions if r["isImprovement"]]
    if improvements:
        print("\n🚀 TOP IMPROVEMENTS:")
        for improvement in improvements[:5]:  # Top 5 improvements
            print(
                f"  • {improvement['testName']}: {improvement['percentageChange']:+.1f}% "
                f"({improvement['currentValue']:.2f}ms vs {improvement['baselineValue']:.2f}ms)"
            )

    # Show performance distribution
    print("\nPERFORMANCE DISTRIBUTION:")
    severity_counts = {}
    for regression in regressions:
        severity = regression["severity"]
        severity_counts[severity] = severity_counts.get(severity, 0) + 1

    for severity, count in sorted(severity_counts.items()):
        emoji = {
            "critical": "🔴",
            "major": "🟡",
            "minor": "🟠",
            "negligible": "⚪",
            "minor_improvement": "🟢",
            "major_improvement": "💚",
        }.get(severity, "❓")
        print(f"  {emoji} {severity.replace('_', ' ').title()}: {count}")

    print("\n" + "=" * 60)


def check_exit_conditions(regressions: List[Dict]) -> int:
    """Check if we should exit with error code based on regressions"""
    critical_count = len([r for r in regressions if r["severity"] == "critical"])
    major_count = len([r for r in regressions if r["severity"] == "major"])

    if critical_count > 0:
        print(
            f"\n❌ FAILURE: {critical_count} critical performance regressions detected!"
        )
        return 1
    elif major_count > 3:  # Allow up to 3 major regressions
        print(f"\n❌ FAILURE: Too many major regressions ({major_count} > 3)!")
        return 1
    elif major_count > 0:
        print(f"\n⚠️  WARNING: {major_count} major performance regressions detected.")
        return 0  # Warning but don't fail
    else:
        print("\n✅ SUCCESS: No significant performance regressions detected.")
        return 0


def main():
    parser = argparse.ArgumentParser(description="Check for performance regressions")
    parser.add_argument(
        "--current", required=True, help="Current performance results (JSON)"
    )
    parser.add_argument(
        "--baseline", required=True, help="Baseline performance results (CSV)"
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=5.0,
        help="Regression threshold percentage (default: 5.0)",
    )
    parser.add_argument(
        "--output", required=True, help="Output regression report (JSON)"
    )
    parser.add_argument(
        "--fail-on-regression",
        action="store_true",
        help="Exit with error code on critical regressions",
    )
    parser.add_argument("--verbose", action="store_true", help="Verbose output")

    args = parser.parse_args()

    # Load data
    current_results = load_current_results(args.current)
    if not current_results:
        print("Error: Could not load current results")
        sys.exit(1)

    baseline_results = load_baseline_results(args.baseline)
    if not baseline_results:
        print(
            "Warning: No baseline data available - cannot perform regression analysis"
        )
        # Still generate empty report
        empty_report = {
            "timestamp": str(datetime.now()),
            "summary": {
                "total_metrics": 0,
                "regressions": 0,
                "improvements": 0,
                "critical_regressions": 0,
                "major_regressions": 0,
            },
            "details": [],
        }
        with open(args.output, "w") as f:
            json.dump(empty_report, f, indent=2)
        return

    # Analyze regressions
    regressions = analyze_regressions(current_results, baseline_results, args.threshold)

    # Generate report
    report = generate_regression_report(regressions, args.output)

    # Print summary
    if args.verbose or len([r for r in regressions if r["isRegression"]]) > 0:
        print_regression_summary(regressions)
    else:
        print(f"Regression analysis complete. {len(regressions)} metrics analyzed.")
        print(
            f"Regressions: {report['summary']['regressions']}, "
            f"Improvements: {report['summary']['improvements']}"
        )

    # Check exit conditions
    if args.fail_on_regression:
        exit_code = check_exit_conditions(regressions)
        sys.exit(exit_code)


if __name__ == "__main__":
    from datetime import datetime

    main()
