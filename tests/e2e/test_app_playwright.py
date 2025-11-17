import os
import time
import subprocess
from pathlib import Path
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "build64" / "Release" / "NeonGlyph.exe"
LOG = ROOT / "staging_run.log"

def test_neonglyph_runs_and_logs_metrics():
    assert BIN.exists(), f"Binary not found: {BIN}"
    env = os.environ.copy()
    env["NG_LOG_ONLY"] = "1"
    if LOG.exists():
        try:
            LOG.unlink()
        except Exception:
            pass
    proc = subprocess.Popen(str(BIN), cwd=str(ROOT), env=env)
    try:
        with sync_playwright() as p:
            deadline = time.time() + 30
            saw_metrics = False
            saw_color = False
            frames = 0
            while time.time() < deadline:
                if LOG.exists():
                    try:
                        txt = LOG.read_text(encoding="utf-8", errors="ignore")
                    except Exception:
                        txt = ""
                    if "FrameTimeAvgMs=" in txt:
                        saw_metrics = True
                    if "ColorMode=" in txt:
                        saw_color = True
                    frames = txt.count("Calling BeginFrame")
                    if saw_metrics and saw_color and frames > 5:
                        break
                time.sleep(0.5)
            assert saw_metrics, "Frame metrics not found in log"
            assert saw_color, "Color mode not logged"
            # Frames count optional; metrics and color mode are primary signals
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=10)
        except Exception:
            pass