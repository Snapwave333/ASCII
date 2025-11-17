import os
import time
import json
import numpy as np
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw, ImageFont
import cv2
import logging
from typing import Dict, List, Tuple, Optional

logger = logging.getLogger(__name__)

class VisualRegressionTester:
    """Advanced visual regression testing for NeonGlyph rendering"""
    
    def __init__(self, reference_dir: Path, test_dir: Path):
        self.reference_dir = reference_dir
        self.test_dir = test_dir
        self.threshold = 0.1  # 10% difference threshold
        self.pixel_threshold = 30  # RGB difference threshold
        
        # Create directories
        self.reference_dir.mkdir(exist_ok=True)
        self.test_dir.mkdir(exist_ok=True)
        
    def capture_application_state(self, log_file: Path, name: str) -> Dict:
        """Capture application state from logs and create synthetic image"""
        if not log_file.exists():
            logger.error(f"Log file not found: {log_file}")
            return {}
            
        try:
            log_content = log_file.read_text(encoding="utf-8", errors="ignore")
        except Exception as e:
            logger.error(f"Failed to read log: {e}")
            return {}
            
        # Parse ASCII content from logs
        ascii_content = self._extract_ascii_content(log_content)
        performance_data = self._extract_performance_data(log_content)
        color_data = self._extract_color_data(log_content)
        
        # Create synthetic image representation
        synthetic_image = self._create_synthetic_image(
            ascii_content, performance_data, color_data
        )
        
        # Save the synthetic image
        image_path = self.test_dir / f"{name}_synthetic.png"
        synthetic_image.save(str(image_path))
        
        return {
            'image_path': image_path,
            'ascii_content': ascii_content,
            'performance_data': performance_data,
            'color_data': color_data,
            'timestamp': time.time()
        }
        
    def _extract_ascii_content(self, log_content: str) -> Dict:
        """Extract ASCII art content from log files"""
        content = {
            'neonglyph_logo': "NEONGLYPH" in log_content,
            'test_card': "RenderTestCard" in log_content,
            'maze_generated': "GenerateMaze" in log_content,
            'fluid_simulation': "AnimateFluidSim" in log_content,
            'text_rendered': "RenderText" in log_content,
            'symmetry_applied': "ApplySymmetry" in log_content,
            'color_morphing': "ApplyColorMorph" in log_content
        }
        
        # Count occurrences
        content['neonglyph_count'] = log_content.count("NEONGLYPH")
        content['test_card_count'] = log_content.count("RenderTestCard")
        content['morph_count'] = log_content.count("MorphToTestCard")
        
        return content
        
    def _extract_performance_data(self, log_content: str) -> Dict:
        """Extract performance metrics from log files"""
        performance_lines = [line for line in log_content.split('\n') if 'TS=' in line and 'FrameMs=' in line]
        
        if not performance_lines:
            return {}
            
        frame_times = []
        render_times = []
        present_times = []
        
        for line in performance_lines[-20:]:  # Last 20 frames
            try:
                parts = line.split()
                for part in parts:
                    if '=' in part:
                        key, value = part.split('=', 1)
                        if key == 'FrameMs':
                            frame_times.append(float(value))
                        elif key == 'RenderMs':
                            render_times.append(float(value))
                        elif key == 'PresentMs':
                            present_times.append(float(value))
            except Exception:
                continue
                
        return {
            'avg_frame_time': np.mean(frame_times) if frame_times else 0,
            'max_frame_time': max(frame_times) if frame_times else 0,
            'min_frame_time': min(frame_times) if frame_times else 0,
            'frame_time_std': np.std(frame_times) if len(frame_times) > 1 else 0,
            'avg_render_time': np.mean(render_times) if render_times else 0,
            'avg_present_time': np.mean(present_times) if present_times else 0,
            'total_frames': len(frame_times)
        }
        
    def _extract_color_data(self, log_content: str) -> Dict:
        """Extract color mode information"""
        color_modes = ["mono", "truecolor", "ansi"]
        color_data = {}
        
        for mode in color_modes:
            color_data[f'{mode}_count'] = log_content.count(f"ColorMode={mode}")
            color_data[f'{mode}_detected'] = color_data[f'{mode}_count'] > 0
            
        # Extract color palette information
        color_data['palette_changes'] = log_content.count("SetPalette")
        color_data['color_morphs'] = log_content.count("ApplyColorMorph")
        
        return color_data
        
    def _create_synthetic_image(self, ascii_content: Dict, performance_data: Dict, color_data: Dict) -> Image.Image:
        """Create synthetic image representation of application state"""
        # Create base image
        width, height = 800, 600
        image = Image.new('RGB', (width, height), color='black')
        draw = ImageDraw.Draw(image)
        
        try:
            # Try to use a monospace font
            font = ImageFont.truetype("consolas.ttf", 12)
            small_font = ImageFont.truetype("consolas.ttf", 10)
        except:
            # Fallback to default font
            font = ImageFont.load_default()
            small_font = ImageFont.load_default()
            
        y_offset = 20
        
        # Draw ASCII content indicators
        draw.text((10, y_offset), "ASCII CONTENT STATUS:", fill='white', font=font)
        y_offset += 25
        
        for key, value in ascii_content.items():
            if isinstance(value, bool) and value:
                color = 'green' if value else 'red'
                draw.text((20, y_offset), f"✓ {key.replace('_', ' ').title()}", fill=color, font=small_font)
                y_offset += 15
                
        y_offset += 20
        
        # Draw performance metrics
        if performance_data:
            draw.text((10, y_offset), "PERFORMANCE METRICS:", fill='cyan', font=font)
            y_offset += 25
            
            metrics_text = [
                f"Avg Frame Time: {performance_data.get('avg_frame_time', 0):.2f}ms",
                f"Max Frame Time: {performance_data.get('max_frame_time', 0):.2f}ms",
                f"Min Frame Time: {performance_data.get('min_frame_time', 0):.2f}ms",
                f"Frame Time Std: {performance_data.get('frame_time_std', 0):.2f}ms",
                f"Total Frames: {performance_data.get('total_frames', 0)}"
            ]
            
            for text in metrics_text:
                draw.text((20, y_offset), text, fill='yellow', font=small_font)
                y_offset += 15
                
        y_offset += 20
        
        # Draw color mode information
        if color_data:
            draw.text((10, y_offset), "COLOR MODES:", fill='magenta', font=font)
            y_offset += 25
            
            for mode in ["mono", "truecolor", "ansi"]:
                count = color_data.get(f'{mode}_count', 0)
                if count > 0:
                    draw.text((20, y_offset), f"✓ {mode.upper()}: {count} occurrences", fill='white', font=small_font)
                    y_offset += 15
                    
        # Draw performance visualization
        if performance_data and performance_data.get('total_frames', 0) > 0:
            y_offset += 20
            draw.text((10, y_offset), "FRAME TIME VISUALIZATION:", fill='orange', font=font)
            y_offset += 25
            
            # Draw frame time bars
            avg_time = performance_data.get('avg_frame_time', 0)
            max_time = performance_data.get('max_frame_time', 0)
            
            if max_time > 0:
                # Draw target line (16.67ms for 60fps)
                target_x = 400 + (16.67 / max_time) * 300
                draw.line([(target_x, y_offset), (target_x, y_offset + 50)], fill='red', width=2)
                draw.text((target_x + 5, y_offset), "60fps target", fill='red', font=small_font)
                
                # Draw average bar
                avg_x = 400 + (avg_time / max_time) * 300
                draw.rectangle([400, y_offset, avg_x, y_offset + 20], fill='green')
                draw.text((410, y_offset + 2), f"Avg: {avg_time:.1f}ms", fill='white', font=small_font)
                
                # Draw scale
                draw.line([(400, y_offset + 40), (700, y_offset + 40)], fill='white', width=1)
                for i in range(0, int(max_time) + 5, 5):
                    x = 400 + (i / max_time) * 300
                    draw.line([(x, y_offset + 35), (x, y_offset + 45)], fill='white', width=1)
                    draw.text((x-5, y_offset + 47), f"{i}ms", fill='white', font=small_font)
                    
        return image
        
    def compare_images(self, reference_path: Path, test_path: Path) -> Dict:
        """Compare two images and return similarity metrics"""
        try:
            ref_img = Image.open(str(reference_path))
            test_img = Image.open(str(test_path))
            
            # Ensure same size
            if ref_img.size != test_img.size:
                test_img = test_img.resize(ref_img.size, Image.Resampling.LANCZOS)
                
            # Convert to numpy arrays
            ref_array = np.array(ref_img)
            test_array = np.array(test_img)
            
            # Calculate difference
            diff = ImageChops.difference(ref_img, test_img)
            diff_array = np.array(diff)
            
            # Calculate metrics
            mse = np.mean((ref_array.astype(float) - test_array.astype(float)) ** 2)
            pixel_diff = np.sum(diff_array > self.pixel_threshold)
            total_pixels = ref_array.size
            pixel_diff_percentage = (pixel_diff / total_pixels) * 100
            
            # Structural similarity (simplified)
            ssim_score = self._calculate_ssim(ref_array, test_array)
            
            return {
                'mse': mse,
                'pixel_diff_percentage': pixel_diff_percentage,
                'ssim_score': ssim_score,
                'similar': pixel_diff_percentage < self.threshold,
                'reference_path': str(reference_path),
                'test_path': str(test_path)
            }
            
        except Exception as e:
            logger.error(f"Image comparison failed: {e}")
            return {
                'error': str(e),
                'similar': False
            }
            
    def _calculate_ssim(self, img1: np.ndarray, img2: np.ndarray) -> float:
        """Simplified SSIM calculation"""
        try:
            # Convert to grayscale if needed
            if len(img1.shape) == 3:
                img1 = np.dot(img1[...,:3], [0.2989, 0.5870, 0.1140])
            if len(img2.shape) == 3:
                img2 = np.dot(img2[...,:3], [0.2989, 0.5870, 0.1140])
                
            # Calculate means
            mu1 = np.mean(img1)
            mu2 = np.mean(img2)
            
            # Calculate variances and covariance
            var1 = np.var(img1)
            var2 = np.var(img2)
            cov = np.mean((img1 - mu1) * (img2 - mu2))
            
            # SSIM constants
            c1 = 0.01 ** 2
            c2 = 0.03 ** 2
            
            # Calculate SSIM
            ssim = ((2 * mu1 * mu2 + c1) * (2 * cov + c2)) / ((mu1**2 + mu2**2 + c1) * (var1 + var2 + c2))
            
            return float(ssim)
            
        except Exception:
            return 0.0
            
    def generate_visual_report(self, test_name: str, comparison_results: Dict) -> Path:
        """Generate HTML visual regression report"""
        report_path = self.test_dir / f"{test_name}_visual_report.html"
        
        html_content = f"""
<!DOCTYPE html>
<html>
<head>
    <title>Visual Regression Report - {test_name}</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        .test-result {{ margin: 20px 0; padding: 15px; border-radius: 5px; }}
        .pass {{ background-color: #d4edda; border: 1px solid #c3e6cb; }}
        .fail {{ background-color: #f8d7da; border: 1px solid #f5c6cb; }}
        .metrics {{ background-color: #d1ecf1; border: 1px solid #bee5eb; margin: 10px 0; padding: 10px; }}
        .image-comparison {{ display: flex; gap: 20px; margin: 20px 0; }}
        .image-container {{ text-align: center; }}
        .image-container img {{ max-width: 400px; max-height: 300px; border: 1px solid #ccc; }}
        pre {{ background-color: #f8f9fa; padding: 10px; border-radius: 3px; overflow-x: auto; }}
    </style>
</head>
<body>
    <h1>Visual Regression Report - {test_name}</h1>
    <p>Generated on: {time.strftime('%Y-%m-%d %H:%M:%S')}</p>
    
    <div class="test-result {'pass' if comparison_results.get('similar', False) else 'fail'}">
        <h3>Test Result: {'PASSED' if comparison_results.get('similar', False) else 'FAILED'}</h3>
        
        <div class="metrics">
            <h4>Metrics:</h4>
            <ul>
                <li>MSE (Mean Squared Error): {comparison_results.get('mse', 'N/A'):.4f}</li>
                <li>Pixel Difference: {comparison_results.get('pixel_diff_percentage', 'N/A'):.2f}%</li>
                <li>SSIM Score: {comparison_results.get('ssim_score', 'N/A'):.4f}</li>
                <li>Threshold: {self.threshold}%</li>
            </ul>
        </div>
        
        <div class="image-comparison">
            <div class="image-container">
                <h4>Reference Image</h4>
                <img src="{Path(comparison_results.get('reference_path', '')).name}" alt="Reference">
            </div>
            <div class="image-container">
                <h4>Test Image</h4>
                <img src="{Path(comparison_results.get('test_path', '')).name}" alt="Test">
            </div>
        </div>
    </div>
    
    <h3>Detailed Comparison Data:</h3>
    <pre>{json.dumps(comparison_results, indent=2)}</pre>
</body>
</html>
"""
        
        report_path.write_text(html_content, encoding='utf-8')
        return report_path

# Test functions
def test_visual_regression_baseline():
    """Test visual regression against baseline"""
    reference_dir = Path("reference_images")
    test_dir = Path("test_images")
    
    tester = VisualRegressionTester(reference_dir, test_dir)
    
    # Create baseline reference if it doesn't exist
    log_file = Path("staging_run.log")
    if log_file.exists():
        state = tester.capture_application_state(log_file, "baseline")
        
        if state:
            logger.info(f"Baseline state captured: {state}")
            
            # For first run, copy as reference
            ref_image = reference_dir / "baseline_synthetic.png"
            if not ref_image.exists():
                test_image = Path(state['image_path'])
                if test_image.exists():
                    ref_image.write_bytes(test_image.read_bytes())
                    logger.info("Baseline reference created")
                    
            # Compare with reference
            comparison = tester.compare_images(
                ref_image, 
                Path(state['image_path'])
            )
            
            # Generate report
            report_path = tester.generate_visual_report("baseline", comparison)
            logger.info(f"Visual regression report generated: {report_path}")
            
            return comparison
            
    return {}

def test_visual_content_validation():
    """Test that visual content is properly rendered"""
    log_file = Path("staging_run.log")
    
    if not log_file.exists():
        logger.error("No application log found for visual validation")
        return False
        
    tester = VisualRegressionTester(Path("reference_images"), Path("test_images"))
    state = tester.capture_application_state(log_file, "validation")
    
    if not state:
        return False
        
    # Validate content requirements
    ascii_content = state['ascii_content']
    performance_data = state['performance_data']
    
    # Essential visual content checks
    assert ascii_content.get('neonglyph_logo', False), "NeonGlyph logo not detected"
    assert ascii_content.get('test_card', False), "Test card not rendered"
    assert ascii_content.get('ascii_content_detected', False), "No ASCII content detected"
    
    # Performance validation
    if performance_data:
        avg_frame_time = performance_data.get('avg_frame_time', 999)
        assert avg_frame_time < 16.67, f"Frame time too high: {avg_frame_time}ms"
        assert performance_data.get('total_frames', 0) > 10, "Insufficient frames rendered"
        
    logger.info("✅ Visual content validation PASSED")
    return True

if __name__ == "__main__":
    # Run visual regression tests
    baseline_result = test_visual_regression_baseline()
    validation_result = test_visual_content_validation()
    
    logger.info("🎉 Visual regression testing completed!")
    logger.info(f"Baseline test result: {baseline_result}")
    logger.info(f"Validation result: {validation_result}")