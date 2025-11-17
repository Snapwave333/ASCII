#!/usr/bin/env python3
"""
Advanced Playwright E2E Test Runner for NeonGlyph Visual Rendering

This script provides comprehensive testing of the full visual rendering pipeline
including ASCII art generation, animations, color modes, and performance metrics.
"""

import os
import sys
import time
import json
import logging
import subprocess
from pathlib import Path
from datetime import datetime
import argparse

# Add the current directory to Python path for imports
sys.path.insert(0, str(Path(__file__).parent))

from advanced_visual_testing import NeonGlyphVisualTester, test_visual_rendering_comprehensive, test_performance_metrics, test_animation_detection
from visual_regression_testing import VisualRegressionTester, test_visual_regression_baseline, test_visual_content_validation

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('visual_test_results.log')
    ]
)
logger = logging.getLogger(__name__)

class ComprehensiveVisualTestRunner:
    """Orchestrates comprehensive visual testing of NeonGlyph"""
    
    def __init__(self, test_duration=30, output_dir="test_results"):
        self.test_duration = test_duration
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        
        self.results = {
            'test_start_time': datetime.now().isoformat(),
            'test_duration': test_duration,
            'tests_run': [],
            'overall_result': 'UNKNOWN',
            'performance_summary': {},
            'visual_content_summary': {},
            'animation_summary': {}
        }
        
    def run_full_visual_test_suite(self):
        """Run the complete visual testing suite"""
        logger.info("🚀 Starting comprehensive visual test suite")
        logger.info(f"Test duration: {self.test_duration} seconds")
        logger.info(f"Output directory: {self.output_dir}")
        
        try:
            # Test 1: Visual Rendering Pipeline
            logger.info("\n" + "="*60)
            logger.info("TEST 1: Visual Rendering Pipeline")
            logger.info("="*60)
            
            result1 = self._test_visual_rendering_pipeline()
            self.results['tests_run'].append(result1)
            
            # Test 2: Performance Metrics
            logger.info("\n" + "="*60)
            logger.info("TEST 2: Performance Metrics Validation")
            logger.info("="*60)
            
            result2 = self._test_performance_metrics()
            self.results['tests_run'].append(result2)
            
            # Test 3: Animation Detection
            logger.info("\n" + "="*60)
            logger.info("TEST 3: Animation Detection")
            logger.info("="*60)
            
            result3 = self._test_animation_detection()
            self.results['tests_run'].append(result3)
            
            # Test 4: Visual Regression
            logger.info("\n" + "="*60)
            logger.info("TEST 4: Visual Regression Testing")
            logger.info("="*60)
            
            result4 = self._test_visual_regression()
            self.results['tests_run'].append(result4)
            
            # Generate comprehensive report
            self._generate_comprehensive_report()
            
            # Determine overall result
            self._calculate_overall_result()
            
            logger.info("\n" + "🎉"*20)
            logger.info(f"VISUAL TEST SUITE COMPLETED: {self.results['overall_result']}")
            logger.info(f"Results saved to: {self.output_dir}")
            logger.info("🎉"*20)
            
            return self.results['overall_result'] == 'PASSED'
            
        except Exception as e:
            logger.error(f"Visual test suite failed with error: {e}")
            self.results['overall_result'] = 'ERROR'
            self.results['error'] = str(e)
            return False
            
    def _test_visual_rendering_pipeline(self) -> dict:
        """Test the complete visual rendering pipeline"""
        logger.info("Testing visual rendering pipeline...")
        
        tester = NeonGlyphVisualTester()
        test_result = {
            'test_name': 'Visual Rendering Pipeline',
            'start_time': datetime.now().isoformat(),
            'result': 'FAILED',
            'details': {}
        }
        
        try:
            result = tester.test_full_rendering_pipeline()
            
            test_result['result'] = 'PASSED' if result['test_passed'] else 'FAILED'
            test_result['details'] = {
                'analysis': result['analysis'],
                'animation_events': len(result['animation_events']),
                'performance_consistent': True
            }
            
            # Store performance summary
            if result['analysis'].get('performance_metrics'):
                self.results['performance_summary'] = result['analysis']['performance_metrics']
                
            # Store visual content summary
            self.results['visual_content_summary'] = result['analysis']
            
            logger.info(f"Visual rendering test: {test_result['result']}")
            
        except Exception as e:
            logger.error(f"Visual rendering test failed: {e}")
            test_result['error'] = str(e)
            
        finally:
            tester.cleanup()
            
        test_result['end_time'] = datetime.now().isoformat()
        return test_result
        
    def _test_performance_metrics(self) -> dict:
        """Test performance metrics accuracy"""
        logger.info("Testing performance metrics...")
        
        test_result = {
            'test_name': 'Performance Metrics',
            'start_time': datetime.now().isoformat(),
            'result': 'FAILED',
            'details': {}
        }
        
        try:
            test_performance_metrics()
            test_result['result'] = 'PASSED'
            logger.info("Performance metrics test: PASSED")
            
        except Exception as e:
            logger.error(f"Performance metrics test failed: {e}")
            test_result['error'] = str(e)
            
        test_result['end_time'] = datetime.now().isoformat()
        return test_result
        
    def _test_animation_detection(self) -> dict:
        """Test animation detection capabilities"""
        logger.info("Testing animation detection...")
        
        test_result = {
            'test_name': 'Animation Detection',
            'start_time': datetime.now().isoformat(),
            'result': 'FAILED',
            'details': {}
        }
        
        try:
            test_animation_detection()
            test_result['result'] = 'PASSED'
            logger.info("Animation detection test: PASSED")
            
        except Exception as e:
            logger.error(f"Animation detection test failed: {e}")
            test_result['error'] = str(e)
            
        test_result['end_time'] = datetime.now().isoformat()
        return test_result
        
    def _test_visual_regression(self) -> dict:
        """Test visual regression detection"""
        logger.info("Testing visual regression...")
        
        test_result = {
            'test_name': 'Visual Regression',
            'start_time': datetime.now().isoformat(),
            'result': 'FAILED',
            'details': {}
        }
        
        try:
            # Test visual content validation
            validation_result = test_visual_content_validation()
            
            # Test baseline comparison
            baseline_result = test_visual_regression_baseline()
            
            test_result['result'] = 'PASSED' if validation_result and baseline_result else 'FAILED'
            test_result['details'] = {
                'content_validation': validation_result,
                'baseline_comparison': baseline_result
            }
            
            logger.info(f"Visual regression test: {test_result['result']}")
            
        except Exception as e:
            logger.error(f"Visual regression test failed: {e}")
            test_result['error'] = str(e)
            
        test_result['end_time'] = datetime.now().isoformat()
        return test_result
        
    def _generate_comprehensive_report(self):
        """Generate comprehensive HTML test report"""
        report_path = self.output_dir / "comprehensive_visual_report.html"
        
        # Calculate summary statistics
        total_tests = len(self.results['tests_run'])
        passed_tests = sum(1 for test in self.results['tests_run'] if test['result'] == 'PASSED')
        failed_tests = sum(1 for test in self.results['tests_run'] if test['result'] == 'FAILED')
        
        html_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NeonGlyph Visual Testing Report</title>
    <style>
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #1a1a1a;
            color: #ffffff;
        }}
        .container {{
            max-width: 1200px;
            margin: 0 auto;
            background-color: #2d2d2d;
            border-radius: 10px;
            padding: 30px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.3);
        }}
        .header {{
            text-align: center;
            margin-bottom: 30px;
            border-bottom: 2px solid #444;
            padding-bottom: 20px;
        }}
        .header h1 {{
            color: #00ff88;
            font-size: 2.5em;
            margin: 0;
            text-shadow: 0 0 10px #00ff88;
        }}
        .summary {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }}
        .summary-card {{
            background-color: #333;
            border-radius: 8px;
            padding: 20px;
            text-align: center;
            border: 1px solid #444;
        }}
        .summary-card h3 {{
            margin: 0 0 10px 0;
            color: #00ff88;
        }}
        .summary-card .value {{
            font-size: 2em;
            font-weight: bold;
            color: #ffffff;
        }}
        .passed {{ color: #00ff88; }}
        .failed {{ color: #ff4444; }}
        .test-results {{
            background-color: #333;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }}
        .test-result {{
            border-left: 4px solid #444;
            padding: 15px;
            margin: 10px 0;
            background-color: #3a3a3a;
            border-radius: 4px;
        }}
        .test-result.passed {{ border-left-color: #00ff88; }}
        .test-result.failed {{ border-left-color: #ff4444; }}
        .test-result h4 {{
            margin: 0 0 10px 0;
            color: #ffffff;
        }}
        .details {{
            background-color: #2a2a2a;
            padding: 10px;
            border-radius: 4px;
            margin-top: 10px;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
        }}
        .performance-chart {{
            background-color: #333;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }}
        .ascii-art {{
            font-family: 'Courier New', monospace;
            background-color: #000;
            color: #00ff88;
            padding: 20px;
            border-radius: 8px;
            text-align: center;
            margin: 20px 0;
            border: 1px solid #00ff88;
        }}
        .footer {{
            text-align: center;
            margin-top: 30px;
            padding-top: 20px;
            border-top: 2px solid #444;
            color: #888;
        }}
        .timestamp {{ color: #00ff88; }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🎮 NEONGLYPH VISUAL TESTING REPORT</h1>
            <p>Advanced Playwright E2E Testing Suite</p>
            <p class="timestamp">Generated: {self.results['test_start_time']}</p>
        </div>
        
        <div class="summary">
            <div class="summary-card">
                <h3>Overall Result</h3>
                <div class="value {'passed' if self.results['overall_result'] == 'PASSED' else 'failed'}">
                    {self.results['overall_result']}
                </div>
            </div>
            <div class="summary-card">
                <h3>Total Tests</h3>
                <div class="value">{total_tests}</div>
            </div>
            <div class="summary-card">
                <h3>Passed</h3>
                <div class="value passed">{passed_tests}</div>
            </div>
            <div class="summary-card">
                <h3>Failed</h3>
                <div class="value failed">{failed_tests}</div>
            </div>
        </div>
        
        <div class="performance-chart">
            <h3>📊 Performance Summary</h3>
            {self._generate_performance_html()}
        </div>
        
        <div class="test-results">
            <h3>🔍 Detailed Test Results</h3>
            {self._generate_test_results_html()}
        </div>
        
        <div class="ascii-art">
            <h3>🎨 Visual Content Summary</h3>
            {self._generate_ascii_summary()}
        </div>
        
        <div class="footer">
            <p>NeonGlyph Advanced Visual Testing Suite</p>
            <p>Test Duration: {self.results['test_duration']} seconds</p>
        </div>
    </div>
</body>
</html>
"""
        
        report_path.write_text(html_content, encoding='utf-8')
        logger.info(f"Comprehensive report generated: {report_path}")
        
    def _generate_performance_html(self) -> str:
        """Generate performance summary HTML"""
        perf = self.results.get('performance_summary', {})
        if not perf:
            return "<p>No performance data available</p>"
            
        return f"""
        <div class="details">
            <strong>Frame Performance:</strong><br>
            Average Frame Time: {perf.get('avg_frame_time', 0):.2f}ms<br>
            Maximum Frame Time: {perf.get('max_frame_time', 0):.2f}ms<br>
            Minimum Frame Time: {perf.get('min_frame_time', 0):.2f}ms<br>
            Standard Deviation: {perf.get('frame_time_std', 0):.2f}ms<br>
            Total Frames Rendered: {perf.get('total_frames', 0)}<br>
            <br>
            <strong>Performance Status:</strong> {'✅ EXCELLENT' if perf.get('avg_frame_time', 999) < 16.67 else '❌ NEEDS IMPROVEMENT'}
        </div>
        """
        
    def _generate_test_results_html(self) -> str:
        """Generate test results HTML"""
        html = ""
        for test in self.results['tests_run']:
            status_class = 'passed' if test['result'] == 'PASSED' else 'failed'
            html += f"""
            <div class="test-result {status_class}">
                <h4>{test['test_name']} - {test['result']}</h4>
                <p><strong>Duration:</strong> {test.get('start_time', '')} to {test.get('end_time', '')}</p>
                {f'<div class="details">Error: {test.get("error", "")}</div>' if test.get('error') else ''}
                {f'<div class="details">Details: {json.dumps(test.get("details", {}), indent=2)}</div>' if test.get('details') else ''}
            </div>
            """
        return html
        
    def _generate_ascii_summary(self) -> str:
        """Generate ASCII art summary"""
        content = self.results.get('visual_content_summary', {})
        
        ascii_art = """
    ╔══════════════════════════════════════════════════════════════╗
    ║                    NEONGLYPH VISUAL TEST                   ║
    ╠══════════════════════════════════════════════════════════════╣
"""
        
        if content.get('ascii_content_detected'):
            ascii_art += "    ║  ✅ ASCII Content: DETECTED                                 ║\n"
        else:
            ascii_art += "    ║  ❌ ASCII Content: NOT DETECTED                            ║\n"
            
        color_modes = content.get('color_modes_detected', [])
        ascii_art += f"    ║  🎨 Color Modes: {', '.join(color_modes) if color_modes else 'None'}" + " "*(35-len(', '.join(color_modes))) + "║\n"
        
        if content.get('animations_detected'):
            ascii_art += "    ║  🎬 Animations: DETECTED                                  ║\n"
        else:
            ascii_art += "    ║  📺 Animations: NOT DETECTED                              ║\n"
            
        ascii_art += """
    ╚══════════════════════════════════════════════════════════════╝
"""
        
        return f"<pre>{ascii_art}</pre>"
        
    def _calculate_overall_result(self):
        """Calculate overall test result"""
        passed_tests = sum(1 for test in self.results['tests_run'] if test['result'] == 'PASSED')
        total_tests = len(self.results['tests_run'])
        
        if passed_tests == total_tests and total_tests > 0:
            self.results['overall_result'] = 'PASSED'
        elif passed_tests > 0:
            self.results['overall_result'] = 'PARTIAL'
        else:
            self.results['overall_result'] = 'FAILED'
            
        # Save JSON results
        results_path = self.output_dir / "test_results.json"
        results_path.write_text(json.dumps(self.results, indent=2), encoding='utf-8')
        
        logger.info(f"Overall result calculated: {self.results['overall_result']} ({passed_tests}/{total_tests} tests passed)")

def main():
    """Main function to run the comprehensive visual test suite"""
    parser = argparse.ArgumentParser(description="NeonGlyph Advanced Visual Testing Suite")
    parser.add_argument("--duration", type=int, default=30, help="Test duration in seconds (default: 30)")
    parser.add_argument("--output", type=str, default="test_results", help="Output directory for results")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
        
    logger.info("🚀 NeonGlyph Advanced Visual Testing Suite")
    logger.info("="*60)
    
    # Create test runner
    runner = ComprehensiveVisualTestRunner(
        test_duration=args.duration,
        output_dir=args.output
    )
    
    # Run the test suite
    success = runner.run_full_visual_test_suite()
    
    # Exit with appropriate code
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()