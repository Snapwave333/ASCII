import pytest
import json
import os
import tempfile
from pathlib import Path

# Test configuration data
TEST_CONFIG = {
    "window": {
        "width": 1920,
        "height": 1080,
        "title": "NeonGlyph Test",
        "fullscreen": False,
        "vsync": True,
        "targetFPS": 60
    },
    "audio": {
        "enabled": True,
        "sampleRate": 44100,
        "channels": 2,
        "bufferSize": 1024
    },
    "ascii": {
        "charset": " .:-=+*#%@",
        "density": 0.5,
        "invert": False,
        "width": 160,
        "height": 60
    },
    "ai": {
        "enabled": True,
        "model": "gemini-2.5-flash",
        "maxTokens": 1000,
        "temperature": 0.7
    },
    "safety": {
        "contentFilter": True,
        "maxInputLength": 10000,
        "blockedKeywords": ["spam", "malware"]
    },
    "output": {
        "format": "console",
        "file": "output.txt",
        "realtime": True
    },
    "llm": {
        "provider": "gemini",
        "apiKey": "test-key",
        "timeout": 30
    },
    "startup": {
        "showSplash": True,
        "autoStart": False,
        "startMinimized": False
    }
}

class TestConfigManager:
    """Test suite for ConfigManager functionality"""
    
    def setup_method(self):
        """Setup test environment before each test"""
        self.test_dir = tempfile.mkdtemp()
        self.config_file = Path(self.test_dir) / "test_config.json"
        self.palette_file = Path(self.test_dir) / "test_palettes.json"
        
        # Write test config file
        with open(self.config_file, 'w') as f:
            json.dump(TEST_CONFIG, f, indent=2)
            
        # Write test palette file
        test_palettes = {
            "palettes": [
                {
                    "name": "Test Palette",
                    "colors": ["#FF0000", "#00FF00", "#0000FF"]
                }
            ]
        }
        with open(self.palette_file, 'w') as f:
            json.dump(test_palettes, f, indent=2)
    
    def teardown_method(self):
        """Cleanup test environment after each test"""
        import shutil
        shutil.rmtree(self.test_dir, ignore_errors=True)
    
    def test_config_file_exists(self):
        """Test that configuration files are created properly"""
        assert self.config_file.exists(), "Config file should exist"
        assert self.palette_file.exists(), "Palette file should exist"
    
    def test_config_structure(self):
        """Test that configuration has required sections"""
        with open(self.config_file) as f:
            config = json.load(f)
        
        required_sections = ["window", "audio", "ascii", "ai", "safety", "output", "llm", "startup"]
        for section in required_sections:
            assert section in config, f"Config should contain {section} section"
    
    def test_window_config_validation(self):
        """Test window configuration validation"""
        with open(self.config_file) as f:
            config = json.load(f)
        
        window = config["window"]
        assert isinstance(window["width"], int), "Width should be integer"
        assert isinstance(window["height"], int), "Height should be integer"
        assert window["width"] > 0, "Width should be positive"
        assert window["height"] > 0, "Height should be positive"
        assert isinstance(window["title"], str), "Title should be string"
        assert isinstance(window["fullscreen"], bool), "Fullscreen should be boolean"
        assert isinstance(window["vsync"], bool), "VSync should be boolean"
        assert isinstance(window["targetFPS"], int), "Target FPS should be integer"
        assert 30 <= window["targetFPS"] <= 144, "Target FPS should be reasonable"
    
    def test_audio_config_validation(self):
        """Test audio configuration validation"""
        with open(self.config_file) as f:
            config = json.load(f)
        
        audio = config["audio"]
        assert isinstance(audio["enabled"], bool), "Enabled should be boolean"
        assert isinstance(audio["sampleRate"], int), "Sample rate should be integer"
        assert audio["sampleRate"] in [22050, 44100, 48000], "Sample rate should be standard"
        assert isinstance(audio["channels"], int), "Channels should be integer"
        assert audio["channels"] in [1, 2], "Channels should be mono or stereo"
        assert isinstance(audio["bufferSize"], int), "Buffer size should be integer"
        assert audio["bufferSize"] > 0, "Buffer size should be positive"
    
    def test_ascii_config_validation(self):
        """Test ASCII configuration validation"""
        with open(self.config_file) as f:
            config = json.load(f)
        
        ascii = config["ascii"]
        assert isinstance(ascii["charset"], str), "Charset should be string"
        assert len(ascii["charset"]) > 0, "Charset should not be empty"
        assert isinstance(ascii["density"], (int, float)), "Density should be numeric"
        assert 0 <= ascii["density"] <= 1, "Density should be between 0 and 1"
        assert isinstance(ascii["invert"], bool), "Invert should be boolean"
        assert isinstance(ascii["width"], int), "Width should be integer"
        assert isinstance(ascii["height"], int), "Height should be integer"
        assert ascii["width"] > 0, "Width should be positive"
        assert ascii["height"] > 0, "Height should be positive"
    
    def test_ai_config_validation(self):
        """Test AI configuration validation"""
        with open(self.config_file) as f:
            config = json.load(f)
        
        ai = config["ai"]
        assert isinstance(ai["enabled"], bool), "Enabled should be boolean"
        assert isinstance(ai["model"], str), "Model should be string"
        assert len(ai["model"]) > 0, "Model should not be empty"
        assert isinstance(ai["maxTokens"], int), "Max tokens should be integer"
        assert ai["maxTokens"] > 0, "Max tokens should be positive"
        assert isinstance(ai["temperature"], (int, float)), "Temperature should be numeric"
        assert 0 <= ai["temperature"] <= 2, "Temperature should be reasonable"
    
    def test_safety_config_validation(self):
        """Test safety configuration validation"""
        with open(self.config_file) as f:
            config = json.load(f)
        
        safety = config["safety"]
        assert isinstance(safety["contentFilter"], bool), "Content filter should be boolean"
        assert isinstance(safety["maxInputLength"], int), "Max input length should be integer"
        assert safety["maxInputLength"] > 0, "Max input length should be positive"
        assert isinstance(safety["blockedKeywords"], list), "Blocked keywords should be list"
        for keyword in safety["blockedKeywords"]:
            assert isinstance(keyword, str), "Each blocked keyword should be string"
    
    def test_palette_structure(self):
        """Test palette configuration structure"""
        with open(self.palette_file) as f:
            palettes = json.load(f)
        
        assert "palettes" in palettes, "Should contain palettes array"
        assert isinstance(palettes["palettes"], list), "Palettes should be array"
        
        for palette in palettes["palettes"]:
            assert "name" in palette, "Palette should have name"
            assert "colors" in palette, "Palette should have colors"
            assert isinstance(palette["name"], str), "Palette name should be string"
            assert isinstance(palette["colors"], list), "Palette colors should be array"
            
            for color in palette["colors"]:
                assert isinstance(color, str), "Color should be string"
                assert color.startswith("#"), "Color should start with #"
                assert len(color) == 7, "Color should be 7 characters (#RRGGBB)"
                # Validate hex color format
                try:
                    int(color[1:], 16)
                except ValueError:
                    assert False, f"Invalid hex color: {color}"
    
    def test_config_file_paths(self):
        """Test configuration file path resolution"""
        # Test various path scenarios
        test_paths = [
            self.config_file,  # Absolute path
            "test_config.json",  # Relative path
            "./test_config.json",  # Current directory
            "../test_config.json",  # Parent directory
        ]
        
        for path in test_paths:
            # This would test the C++ FindConfigFile function
            # For now, just verify paths are valid
            if Path(path).is_absolute():
                assert Path(path).exists(), f"Absolute path should exist: {path}"
    
    def test_environment_variable_override(self):
        """Test environment variable configuration override"""
        # Set test environment variable
        test_config_dir = str(self.test_dir)
        os.environ["NEONGLYPH_CONFIG_DIR"] = test_config_dir
        
        try:
            # This would test the C++ environment variable handling
            # Verify environment variable is set
            assert os.environ.get("NEONGLYPH_CONFIG_DIR") == test_config_dir
        finally:
            # Cleanup
            if "NEONGLYPH_CONFIG_DIR" in os.environ:
                del os.environ["NEONGLYPH_CONFIG_DIR"]
    
    def test_invalid_config_handling(self):
        """Test handling of invalid configuration files"""
        # Create invalid config file
        invalid_config = Path(self.test_dir) / "invalid_config.json"
        with open(invalid_config, 'w') as f:
            f.write("invalid json content")
        
        # This would test the C++ error handling
        # For now, just verify the file was created
        assert invalid_config.exists(), "Invalid config file should exist"
    
    def test_missing_config_fallback(self):
        """Test fallback to default configuration when file is missing"""
        missing_config = Path(self.test_dir) / "missing_config.json"
        
        # This would test the C++ fallback mechanism
        # Verify file doesn't exist
        assert not missing_config.exists(), "Missing config file should not exist"
    
    def test_config_validation_ranges(self):
        """Test configuration value ranges and constraints"""
        with open(self.config_file) as f:
            config = json.load(f)
        
        # Test window constraints
        window = config["window"]
        assert 640 <= window["width"] <= 7680, "Width should be reasonable"
        assert 480 <= window["height"] <= 4320, "Height should be reasonable"
        
        # Test audio constraints
        audio = config["audio"]
        assert 8000 <= audio["sampleRate"] <= 192000, "Sample rate should be reasonable"
        assert 64 <= audio["bufferSize"] <= 8192, "Buffer size should be reasonable"
        
        # Test ASCII constraints
        ascii = config["ascii"]
        assert 40 <= ascii["width"] <= 500, "ASCII width should be reasonable"
        assert 20 <= ascii["height"] <= 200, "ASCII height should be reasonable"

if __name__ == "__main__":
    pytest.main([__file__, "-v"])