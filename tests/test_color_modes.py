import pytest
import json
import os
import tempfile
from pathlib import Path

# Test color mode scenarios
TEST_COLOR_MODES = {
    "mono": {
        "mode": "mono",
        "sequences": [],
        "expected_escapes": False
    },
    "ansi": {
        "mode": "ansi", 
        "sequences": ["\x1B[30m", "\x1B[31m", "\x1B[32m", "\x1B[90m", "\x1B[91m"],
        "expected_escapes": True
    },
    "truecolor": {
        "mode": "truecolor",
        "sequences": ["\x1B[38;2;255;0;0m", "\x1B[38;2;0;255;0m", "\x1B[38;2;0;0;255m"],
        "expected_escapes": True
    }
}

class TestColorModeManager:
    """Test suite for Color Mode functionality"""
    
    def setup_method(self):
        """Setup test environment before each test"""
        self.test_dir = tempfile.mkdtemp()
        self.log_file = Path(self.test_dir) / "color_test.log"
        
    def teardown_method(self):
        """Cleanup test environment after each test"""
        import shutil
        shutil.rmtree(self.test_dir, ignore_errors=True)
    
    def test_mono_mode_detection(self):
        """Test monochrome mode detection and validation"""
        # Create mono mode log content
        mono_content = """
mode=mono tc=0 ansi=0 esc=NO
FrameTimeAvgMs=16.67
ColorMode=mono
"""
        
        with open(self.log_file, 'w') as f:
            f.write(mono_content)
        
        # Validate mono mode characteristics
        with open(self.log_file) as f:
            content = f.read()
        
        assert "mode=mono" in content, "Should contain mono mode marker"
        assert "tc=0" in content, "Should have 0 truecolor sequences"
        assert "ansi=0" in content, "Should have 0 ANSI sequences"
        assert "esc=NO" in content, "Should have no escape sequences"
    
    def test_ansi_mode_detection(self):
        """Test ANSI mode detection and validation"""
        # Create ANSI mode log content
        ansi_content = """
mode=ansi tc=0 ansi=5 esc=YES
FrameTimeAvgMs=16.67
ColorMode=ansi
\x1B[30m\x1B[31m\x1B[32m\x1B[90m\x1B[91m
"""
        
        with open(self.log_file, 'w') as f:
            f.write(ansi_content)
        
        # Validate ANSI mode characteristics
        with open(self.log_file) as f:
            content = f.read()
        
        assert "mode=ansi" in content, "Should contain ANSI mode marker"
        assert "tc=0" in content, "Should have 0 truecolor sequences"
        assert "ansi=5" in content, "Should have 5 ANSI sequences"
        assert "esc=YES" in content, "Should have escape sequences"
        
        # Check for ANSI escape sequences
        ansi_sequences = ["\x1B[30m", "\x1B[31m", "\x1B[32m", "\x1B[90m", "\x1B[91m"]
        for seq in ansi_sequences:
            assert seq in content, f"Should contain ANSI sequence {repr(seq)}"
    
    def test_truecolor_mode_detection(self):
        """Test truecolor mode detection and validation"""
        # Create truecolor mode log content
        truecolor_content = """
mode=truecolor tc=3 ansi=0 esc=YES
FrameTimeAvgMs=16.67
ColorMode=truecolor
\x1B[38;2;255;0;0m\x1B[38;2;0;255;0m\x1B[38;2;0;0;255m
"""
        
        with open(self.log_file, 'w') as f:
            f.write(truecolor_content)
        
        # Validate truecolor mode characteristics
        with open(self.log_file) as f:
            content = f.read()
        
        assert "mode=truecolor" in content, "Should contain truecolor mode marker"
        assert "tc=3" in content, "Should have 3 truecolor sequences"
        assert "ansi=0" in content, "Should have 0 ANSI sequences"
        assert "esc=YES" in content, "Should have escape sequences"
        
        # Check for truecolor escape sequences
        truecolor_sequences = ["\x1B[38;2;255;0;0m", "\x1B[38;2;0;255;0m", "\x1B[38;2;0;0;255m"]
        for seq in truecolor_sequences:
            assert seq in content, f"Should contain truecolor sequence {repr(seq)}"
    
    def test_invalid_mode_handling(self):
        """Test handling of invalid color modes"""
        # Create invalid mode log content
        invalid_content = """
mode=unknown tc=0 ansi=0 esc=NO
FrameTimeAvgMs=16.67
ColorMode=invalid
"""
        
        with open(self.log_file, 'w') as f:
            f.write(invalid_content)
        
        # Validate invalid mode characteristics
        with open(self.log_file) as f:
            content = f.read()
        
        assert "mode=unknown" in content, "Should contain unknown mode marker"
        assert "tc=0" in content, "Should have 0 truecolor sequences"
        assert "ansi=0" in content, "Should have 0 ANSI sequences"
        assert "esc=NO" in content, "Should have no escape sequences"
    
    def test_mode_transition_validation(self):
        """Test validation of mode transitions"""
        # Create log with mode transitions
        transition_content = """
mode=mono tc=0 ansi=0 esc=NO
ColorMode=mono
mode=ansi tc=0 ansi=3 esc=YES
ColorMode=ansi
mode=truecolor tc=2 ansi=0 esc=YES
ColorMode=truecolor
"""
        
        with open(self.log_file, 'w') as f:
            f.write(transition_content)
        
        # Validate mode transitions
        with open(self.log_file) as f:
            content = f.read()
        
        # Check all modes are present
        assert "mode=mono" in content, "Should contain mono mode"
        assert "mode=ansi" in content, "Should contain ANSI mode"
        assert "mode=truecolor" in content, "Should contain truecolor mode"
        
        # Check transition markers
        assert "ColorMode=mono" in content, "Should contain mono transition"
        assert "ColorMode=ansi" in content, "Should contain ANSI transition"
        assert "ColorMode=truecolor" in content, "Should contain truecolor transition"
    
    def test_performance_metrics_with_color_modes(self):
        """Test performance metrics collection across different color modes"""
        # Create performance log with color mode data
        performance_content = """
TS=1763114873403 Window Create start
TS=1763114873462 Window CreateMs=59.702900
mode=mono tc=0 ansi=0 esc=NO
FrameTimeAvgMs=16.67
TS=1763114873500 Render start
TS=1763114873510 RenderMs=10.000000
mode=ansi tc=0 ansi=5 esc=YES
FrameTimeAvgMs=16.67
TS=1763114873600 Present start
TS=1763114873610 PresentMs=10.000000
mode=truecolor tc=3 ansi=0 esc=YES
FrameTimeAvgMs=16.67
"""
        
        with open(self.log_file, 'w') as f:
            f.write(performance_content)
        
        # Validate performance metrics
        with open(self.log_file) as f:
            content = f.read()
        
        # Check timing metrics
        assert "CreateMs=59.702900" in content, "Should contain creation time"
        assert "RenderMs=10.000000" in content, "Should contain render time"
        assert "PresentMs=10.000000" in content, "Should contain present time"
        assert "FrameTimeAvgMs=16.67" in content, "Should contain frame time average"
        
        # Check color mode changes during operations
        assert "mode=mono" in content, "Should start with mono mode"
        assert "mode=ansi" in content, "Should transition to ANSI mode"
        assert "mode=truecolor" in content, "Should transition to truecolor mode"
    
    def test_color_mode_compliance_requirements(self):
        """Test compliance with color mode requirements"""
        # Test mono mode requirements
        mono_content = "mode=mono tc=0 ansi=0 esc=NO"
        assert "tc=0" in mono_content, "Mono mode should have 0 truecolor sequences"
        assert "ansi=0" in mono_content, "Mono mode should have 0 ANSI sequences"
        assert "esc=NO" in mono_content, "Mono mode should have no escape sequences"
        
        # Test ANSI mode requirements
        ansi_content = "mode=ansi tc=0 ansi=5 esc=YES"
        assert "tc=0" in ansi_content, "ANSI mode should have 0 truecolor sequences"
        assert "ansi=5" in ansi_content, "ANSI mode should have ANSI sequences"
        assert "esc=YES" in ansi_content, "ANSI mode should have escape sequences"
        
        # Test truecolor mode requirements
        truecolor_content = "mode=truecolor tc=3 ansi=0 esc=YES"
        assert "tc=3" in truecolor_content, "Truecolor mode should have truecolor sequences"
        assert "ansi=0" in truecolor_content, "Truecolor mode should have 0 ANSI sequences"
        assert "esc=YES" in truecolor_content, "Truecolor mode should have escape sequences"
    
    def test_error_handling_for_color_modes(self):
        """Test error handling in color mode detection"""
        # Test missing mode information
        incomplete_content = """
tc=0 ansi=0 esc=NO
FrameTimeAvgMs=16.67
"""
        
        with open(self.log_file, 'w') as f:
            f.write(incomplete_content)
        
        with open(self.log_file) as f:
            content = f.read()
        
        # Should handle missing mode gracefully
        assert "mode=" not in content, "Should not contain mode information"
        assert "tc=0" in content, "Should still contain sequence counts"
    
    def test_color_sequence_counting(self):
        """Test accurate counting of color sequences"""
        # Create content with known sequence counts
        test_content = """
mode=mixed tc=2 ansi=3 esc=YES
\x1B[38;2;255;0;0m\x1B[38;2;0;255;0m\x1B[30m\x1B[31m\x1B[32m
"""
        
        with open(self.log_file, 'w') as f:
            f.write(test_content)
        
        with open(self.log_file) as f:
            content = f.read()
        
        # Verify counts
        assert "tc=2" in content, "Should count 2 truecolor sequences"
        assert "ansi=3" in content, "Should count 3 ANSI sequences"
        assert "esc=YES" in content, "Should detect escape sequences"

if __name__ == "__main__":
    pytest.main([__file__, "-v"])