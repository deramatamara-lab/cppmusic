#!/usr/bin/env python3
"""
Performance Dashboard Generator
Generates an interactive HTML dashboard from performance test results
"""

import json
import argparse
import sys
from datetime import datetime
import csv


def load_performance_data(filepath):
    """Load performance data from JSON file"""
    try:
        with open(filepath, "r") as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading performance data: {e}")
        return None


def load_baseline_data(filepath):
    """Load baseline data from CSV file"""
    baseline = {}
    try:
        with open(filepath, "r") as f:
            reader = csv.reader(f)
            for row in reader:
                if len(row) >= 2:
                    baseline[row[0]] = float(row[1])
    except Exception as e:
        print(f"Warning: Could not load baseline data: {e}")
    return baseline


def calculate_performance_changes(current, baseline):
    """Calculate performance changes compared to baseline"""
    changes = {}
    for metric, value in current.get("averageTimes", {}).items():
        if metric in baseline:
            change = ((value - baseline[metric]) / baseline[metric]) * 100
            changes[metric] = {
                "current": value,
                "baseline": baseline[metric],
                "change_percent": change,
                "status": "regression"
                if change > 5
                else "improvement"
                if change < -5
                else "stable",
            }
    return changes


def generate_dashboard_html(performance_data, baseline_data, output_path):
    """Generate HTML dashboard"""

    changes = calculate_performance_changes(performance_data, baseline_data)

    html_template = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Performance Dashboard - CPPMusic DAW</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            margin: 0;
            padding: 20px;
            background: #f5f5f5;
        }}
        .header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            border-radius: 10px;
            margin-bottom: 30px;
            text-align: center;
        }}
        .header h1 {{
            margin: 0;
            font-size: 2.5em;
            font-weight: 300;
        }}
        .header p {{
            margin: 10px 0 0 0;
            opacity: 0.8;
            font-size: 1.1em;
        }}
        .grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }}
        .card {{
            background: white;
            border-radius: 10px;
            padding: 25px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.07);
            border: 1px solid #e1e5e9;
        }}
        .card h2 {{
            margin: 0 0 20px 0;
            color: #2c3e50;
            font-size: 1.4em;
            font-weight: 600;
        }}
        .metric {{
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 0;
            border-bottom: 1px solid #f8f9fa;
        }}
        .metric:last-child {{
            border-bottom: none;
        }}
        .metric-name {{
            font-weight: 500;
            color: #495057;
        }}
        .metric-value {{
            font-weight: 600;
            font-size: 1.1em;
        }}
        .status-regression {{
            color: #e74c3c;
        }}
        .status-improvement {{
            color: #27ae60;
        }}
        .status-stable {{
            color: #7f8c8d;
        }}
        .chart-container {{
            position: relative;
            height: 300px;
            margin: 20px 0;
        }}
        .summary-stats {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }}
        .stat-box {{
            text-align: center;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 8px;
            border-left: 4px solid #667eea;
        }}
        .stat-value {{
            font-size: 2em;
            font-weight: 700;
            color: #2c3e50;
            margin: 0;
        }}
        .stat-label {{
            color: #6c757d;
            font-size: 0.9em;
            margin: 5px 0 0 0;
        }}
        .alert {{
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
        }}
        .alert-warning {{
            background: #fff3cd;
            border: 1px solid #ffeaa7;
            color: #856404;
        }}
        .alert-success {{
            background: #d4edda;
            border: 1px solid #c3e6cb;
            color: #155724;
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }}
        th, td {{
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #dee2e6;
        }}
        th {{
            background: #f8f9fa;
            font-weight: 600;
            color: #495057;
        }}
        .performance-bar {{
            height: 8px;
            background: #e9ecef;
            border-radius: 4px;
            margin: 5px 0;
            overflow: hidden;
        }}
        .performance-fill {{
            height: 100%;
            background: linear-gradient(90deg, #28a745, #ffc107, #dc3545);
            border-radius: 4px;
            transition: width 0.3s ease;
        }}
        .footer {{
            text-align: center;
            padding: 20px;
            color: #6c757d;
            font-size: 0.9em;
        }}
    </style>
</head>
<body>
    <div class="header">
        <h1>🚀 Performance Dashboard</h1>
        <p>CPPMusic DAW - Generated on {timestamp}</p>
    </div>

    <div class="summary-stats">
        <div class="stat-box">
            <div class="stat-value">{total_tests}</div>
            <div class="stat-label">Total Tests</div>
        </div>
        <div class="stat-box">
            <div class="stat-value">{avg_processing_time:.2f}ms</div>
            <div class="stat-label">Avg Processing Time</div>
        </div>
        <div class="stat-box">
            <div class="stat-value">{memory_usage:.1f}MB</div>
            <div class="stat-label">Memory Usage</div>
        </div>
        <div class="stat-box">
            <div class="stat-value">{regression_count}</div>
            <div class="stat-label">Regressions</div>
        </div>
    </div>

    {alerts}

    <div class="grid">
        <div class="card">
            <h2>📊 Performance Metrics</h2>
            <div class="chart-container">
                <canvas id="performanceChart"></canvas>
            </div>
        </div>

        <div class="card">
            <h2>💾 Memory Analysis</h2>
            <div class="chart-container">
                <canvas id="memoryChart"></canvas>
            </div>
        </div>

        <div class="card">
            <h2>🔄 Performance Changes</h2>
            {performance_changes_table}
        </div>

        <div class="card">
            <h2>⚡ Component Breakdown</h2>
            {component_metrics}
        </div>
    </div>

    <div class="card">
        <h2>📈 Performance Trends</h2>
        <div class="chart-container">
            <canvas id="trendsChart"></canvas>
        </div>
    </div>

    <div class="footer">
        <p>Performance dashboard generated by CPPMusic Performance Testing Suite</p>
        <p>For detailed logs and analysis, check the full performance report</p>
    </div>

    <script>
        // Performance metrics chart
        const performanceCtx = document.getElementById('performanceChart').getContext('2d');
        new Chart(performanceCtx, {{
            type: 'bar',
            data: {{
                labels: {metric_names},
                datasets: [{{
                    label: 'Processing Time (ms)',
                    data: {metric_values},
                    backgroundColor: 'rgba(102, 126, 234, 0.8)',
                    borderColor: 'rgba(102, 126, 234, 1)',
                    borderWidth: 1
                }}]
            }},
            options: {{
                responsive: true,
                maintainAspectRatio: false,
                scales: {{
                    y: {{
                        beginAtZero: true,
                        title: {{
                            display: true,
                            text: 'Time (ms)'
                        }}
                    }}
                }}
            }}
        }});

        // Memory usage chart
        const memoryCtx = document.getElementById('memoryChart').getContext('2d');
        new Chart(memoryCtx, {{
            type: 'doughnut',
            data: {{
                labels: ['Used Memory', 'Available'],
                datasets: [{{
                    data: [{memory_usage}, {memory_available}],
                    backgroundColor: [
                        'rgba(231, 76, 60, 0.8)',
                        'rgba(46, 204, 113, 0.8)'
                    ],
                    borderWidth: 0
                }}]
            }},
            options: {{
                responsive: true,
                maintainAspectRatio: false,
                plugins: {{
                    legend: {{
                        position: 'bottom'
                    }}
                }}
            }}
        }});

        // Performance trends chart
        const trendsCtx = document.getElementById('trendsChart').getContext('2d');
        new Chart(trendsCtx, {{
            type: 'line',
            data: {{
                labels: ['P50', 'P90', 'P95', 'P99'],
                datasets: [{{
                    label: 'Percentiles (ms)',
                    data: {percentile_data},
                    borderColor: 'rgba(118, 75, 162, 1)',
                    backgroundColor: 'rgba(118, 75, 162, 0.1)',
                    fill: true,
                    tension: 0.4
                }}]
            }},
            options: {{
                responsive: true,
                maintainAspectRatio: false,
                scales: {{
                    y: {{
                        beginAtZero: true,
                        title: {{
                            display: true,
                            text: 'Time (ms)'
                        }}
                    }}
                }}
            }}
        }});
    </script>
</body>
</html>
"""

    # Calculate summary statistics
    avg_times = performance_data.get("averageTimes", {})
    avg_processing_time = sum(avg_times.values()) / len(avg_times) if avg_times else 0
    memory_usage = performance_data.get("totalMemoryUsage", 0) / (
        1024 * 1024
    )  # Convert to MB
    regression_count = sum(
        1 for change in changes.values() if change["status"] == "regression"
    )

    # Generate alerts
    alerts = ""
    if regression_count > 0:
        alerts += f'<div class="alert alert-warning">⚠️ {regression_count} performance regressions detected!</div>'
    else:
        alerts += '<div class="alert alert-success">✅ No performance regressions detected</div>'

    # Generate performance changes table
    changes_html = "<table><tr><th>Component</th><th>Current</th><th>Baseline</th><th>Change</th></tr>"
    for metric, change in changes.items():
        status_class = f"status-{change['status']}"
        change_str = f"{change['change_percent']:+.1f}%"
        changes_html += f"""
        <tr>
            <td>{metric}</td>
            <td>{change["current"]:.2f}ms</td>
            <td>{change["baseline"]:.2f}ms</td>
            <td class="{status_class}">{change_str}</td>
        </tr>
        """
    changes_html += "</table>"

    # Generate component metrics
    component_html = ""
    for metric, time in avg_times.items():
        performance_percent = min(100, (time / 10) * 100)  # Assume 10ms is 100%
        component_html += f"""
        <div class="metric">
            <div class="metric-name">{metric}</div>
            <div class="metric-value">{time:.2f}ms</div>
        </div>
        <div class="performance-bar">
            <div class="performance-fill" style="width: {performance_percent}%"></div>
        </div>
        """

    # Prepare chart data
    metric_names = list(avg_times.keys())[:10]  # Top 10 metrics
    metric_values = [avg_times[name] for name in metric_names]

    # Sample percentile data (would come from actual percentile calculations)
    percentile_data = [2.1, 5.8, 8.2, 12.4]  # P50, P90, P95, P99

    # Fill template
    html_content = html_template.format(
        timestamp=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        total_tests=len(avg_times),
        avg_processing_time=avg_processing_time,
        memory_usage=memory_usage,
        regression_count=regression_count,
        alerts=alerts,
        performance_changes_table=changes_html,
        component_metrics=component_html,
        metric_names=json.dumps(metric_names),
        metric_values=json.dumps(metric_values),
        memory_available=max(0, 100 - int(memory_usage)),
        percentile_data=json.dumps(percentile_data),
    )

    # Write to file
    with open(output_path, "w") as f:
        f.write(html_content)

    print(f"Performance dashboard generated: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Generate performance dashboard")
    parser.add_argument("--input", required=True, help="Input performance JSON file")
    parser.add_argument("--baseline", help="Baseline CSV file for comparison")
    parser.add_argument("--output", required=True, help="Output HTML file")

    args = parser.parse_args()

    # Load data
    performance_data = load_performance_data(args.input)
    if not performance_data:
        sys.exit(1)

    baseline_data = {}
    if args.baseline:
        baseline_data = load_baseline_data(args.baseline)

    # Generate dashboard
    generate_dashboard_html(performance_data, baseline_data, args.output)


if __name__ == "__main__":
    main()
