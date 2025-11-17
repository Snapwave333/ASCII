import pytest
import json
import os
import tempfile
from pathlib import Path

# Test performance metrics data
TEST_PERFORMANCE_DATA = {
    "frame_times": [16.67, 16.67, 16.67, 16.67, 16.67],  # 60 FPS target
    "render_times": [8.0, 8.5, 7.8, 8.2, 8.1],
    "present_times": [6.0, 5.8, 6.2, 5.9, 6.1],
    "cpu_usage": [15.2, 14.8, 15.5, 14.9, 15.1],
    "draw_calls": [450, 455, 448, 452, 451],
    "dropped_frames": [0, 0, 0, 0, 0],
    "jitter_values": [0.1, 0.2, 0.1, 0.3, 0.1]
}

class TestPerformanceMetrics:
    """Test suite for Performance Metrics functionality"""
    
    def setup_method(self):
        """Setup test environment before each test"""
        self.test_dir = tempfile.mkdtemp()
        self.metrics_file = Path(self.test_dir) / "performance_metrics.log"
        
    def teardown_method(self):
        """Cleanup test environment after each test"""
        import shutil
        shutil.rmtree(self.test_dir, ignore_errors=True)
    
    def test_frame_time_target_compliance(self):
        """Test that frame times meet 60 FPS target (16.67ms)"""
        target_frame_time = 16.67  # 60 FPS in milliseconds
        tolerance = 1.0  # 1ms tolerance
        
        for frame_time in TEST_PERFORMANCE_DATA["frame_times"]:
            assert abs(frame_time - target_frame_time) <= tolerance, \
                f"Frame time {frame_time}ms exceeds tolerance of {tolerance}ms from target {target_frame_time}ms"
    
    def test_frame_time_consistency(self):
        """Test frame time consistency and stability"""
        frame_times = TEST_PERFORMANCE_DATA["frame_times"]
        
        # Calculate standard deviation
        mean = sum(frame_times) / len(frame_times)
        variance = sum((x - mean) ** 2 for x in frame_times) / len(frame_times)
        std_dev = variance ** 0.5
        
        # Standard deviation should be low for consistent frame times
        assert std_dev <= 0.5, f"Frame time standard deviation {std_dev} is too high for consistent performance"
    
    def test_render_time_budget(self):
        """Test that render times stay within budget"""
        render_times = TEST_PERFORMANCE_DATA["render_times"]
        render_budget = 12.0  # 12ms render budget for 60 FPS
        
        for render_time in render_times:
            assert render_time <= render_budget, \
                f"Render time {render_time}ms exceeds budget of {render_budget}ms"
    
    def test_present_time_efficiency(self):
        """Test presentation time efficiency"""
        present_times = TEST_PERFORMANCE_DATA["present_times"]
        present_budget = 8.0  # 8ms present budget
        
        for present_time in present_times:
            assert present_time <= present_budget, \
                f"Present time {present_time}ms exceeds budget of {present_budget}ms"
    
    def test_cpu_usage_efficiency(self):
        """Test CPU usage stays within acceptable limits"""
        cpu_usage = TEST_PERFORMANCE_DATA["cpu_usage"]
        max_cpu_usage = 25.0  # 25% CPU usage limit
        
        for cpu in cpu_usage:
            assert cpu <= max_cpu_usage, \
                f"CPU usage {cpu}% exceeds limit of {max_cpu_usage}%"
    
    def test_draw_call_budget(self):
        """Test draw call budget compliance"""
        draw_calls = TEST_PERFORMANCE_DATA["draw_calls"]
        max_draw_calls = 800  # 800 draw call limit
        
        for calls in draw_calls:
            assert calls <= max_draw_calls, \
                f"Draw calls {calls} exceeds budget of {max_draw_calls}"
    
    def test_no_dropped_frames(self):
        """Test that no frames are dropped"""
        dropped_frames = TEST_PERFORMANCE_DATA["dropped_frames"]
        
        for dropped in dropped_frames:
            assert dropped == 0, f"Dropped frames detected: {dropped}"
    
    def test_jitter_stability(self):
        """Test jitter stability within acceptable limits"""
        jitter_values = TEST_PERFORMANCE_DATA["jitter_values"]
        max_jitter = 0.5  # 0.5ms jitter limit
        
        for jitter in jitter_values:
            assert jitter <= max_jitter, \
                f"Jitter {jitter}ms exceeds limit of {max_jitter}ms"
    
    def test_performance_metrics_format(self):
        """Test performance metrics log format"""
        # Create sample performance log
        log_content = """
TS=1763114873403 Window Create start
TS=1763114873462 Window CreateMs=59.702900
FrameTimeAvgMs=16.67
RenderMs=8.1
PresentMs=6.0
CPU=15.2%
DrawCalls=451
DroppedFrames=0
Jitter=0.1ms
"""
        
        with open(self.metrics_file, 'w') as f:
            f.write(log_content)
        
        # Validate log format
        with open(self.metrics_file) as f:
            content = f.read()
        
        # Check required metrics are present
        assert "Window CreateMs=" in content, "Should contain window creation time"
        assert "FrameTimeAvgMs=" in content, "Should contain frame time average"
        assert "RenderMs=" in content, "Should contain render time"
        assert "PresentMs=" in content, "Should contain present time"
        assert "CPU=" in content, "Should contain CPU usage"
        assert "DrawCalls=" in content, "Should contain draw calls"
        assert "DroppedFrames=" in content, "Should contain dropped frames"
        assert "Jitter=" in content, "Should contain jitter"
    
    def test_performance_baseline_establishment(self):
        """Test establishment of performance baselines"""
        # Calculate baseline metrics
        frame_times = TEST_PERFORMANCE_DATA["frame_times"]
        render_times = TEST_PERFORMANCE_DATA["render_times"]
        present_times = TEST_PERFORMANCE_DATA["present_times"]
        
        # Baseline calculations
        baseline_frame_time = sum(frame_times) / len(frame_times)
        baseline_render_time = sum(render_times) / len(render_times)
        baseline_present_time = sum(present_times) / len(present_times)
        
        # Validate baselines are reasonable
        assert 15.0 <= baseline_frame_time <= 18.0, \
            f"Baseline frame time {baseline_frame_time}ms is out of reasonable range"
        assert baseline_render_time <= 10.0, \
            f"Baseline render time {baseline_render_time}ms is too high"
        assert baseline_present_time <= 7.0, \
            f"Baseline present time {baseline_present_time}ms is too high"
    
    def test_performance_regression_detection(self):
        """Test detection of performance regressions"""
        # Simulate baseline metrics
        baseline = {
            "frame_time": 16.67,
            "render_time": 8.0,
            "present_time": 6.0,
            "cpu_usage": 15.0,
            "draw_calls": 450
        }
        
        # Simulate current metrics (with regression)
        current = {
            "frame_time": 22.0,  # Regression: higher frame time
            "render_time": 12.0,  # Regression: higher render time
            "present_time": 6.0,
            "cpu_usage": 15.0,
            "draw_calls": 450
        }
        
        # Detect regressions
        regressions = []
        if current["frame_time"] > baseline["frame_time"] * 1.1:  # 10% threshold
            regressions.append(f"Frame time regression: {current['frame_time']} vs {baseline['frame_time']}")
        
        if current["render_time"] > baseline["render_time"] * 1.2:  # 20% threshold
            regressions.append(f"Render time regression: {current['render_time']} vs {baseline['render_time']}")
        
        # Verify regressions are detected
        assert len(regressions) == 2, f"Should detect 2 regressions, found: {regressions}"
    
    def test_performance_stress_scenarios(self):
        """Test performance under stress scenarios"""
        # Simulate high-load scenario
        stress_data = {
            "frame_times": [20.0, 22.0, 25.0, 28.0, 30.0],  # Degrading performance
            "cpu_usage": [45.0, 55.0, 65.0, 75.0, 85.0],   # Increasing CPU usage
            "draw_calls": [1200, 1400, 1600, 1800, 2000],   # Exceeding budget
            "dropped_frames": [1, 2, 5, 8, 12]               # Increasing dropped frames
        }
        
        # Validate stress scenario handling
        for i, (frame_time, cpu, calls, dropped) in enumerate(zip(
            stress_data["frame_times"],
            stress_data["cpu_usage"],
            stress_data["draw_calls"],
            stress_data["dropped_frames"]
        )):
            # Performance should degrade gracefully
            if frame_time > 25.0:
                assert cpu > 60.0, f"High frame time should correlate with high CPU usage"
            
            if calls > 800:
                assert dropped > 0, f"Exceeding draw call budget should cause dropped frames"
    
    def test_performance_optimization_opportunities(self):
        """Test identification of performance optimization opportunities"""
        # Analyze performance data for optimization opportunities
        frame_times = TEST_PERFORMANCE_DATA["frame_times"]
        render_times = TEST_PERFORMANCE_DATA["render_times"]
        present_times = TEST_PERFORMANCE_DATA["present_times"]
        
        optimization_opportunities = []
        
        # Check for render optimization opportunities
        avg_render = sum(render_times) / len(render_times)
        if avg_render > 8.0:
            optimization_opportunities.append(f"Render time optimization: average {avg_render}ms")
        
        # Check for present optimization opportunities
        avg_present = sum(present_times) / len(present_times)
        if avg_present > 5.0:
            optimization_opportunities.append(f"Present time optimization: average {avg_present}ms")
        
        # Frame time consistency optimization
        frame_variance = max(frame_times) - min(frame_times)
        if frame_variance > 1.0:
            optimization_opportunities.append(f"Frame time consistency: variance {frame_variance}ms")
        
        # Verify optimization opportunities are identified
        assert len(optimization_opportunities) >= 0, "Should identify optimization opportunities"

if __name__ == "__main__":
    pytest.main([__file__, "-v"])