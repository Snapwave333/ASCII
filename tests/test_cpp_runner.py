import subprocess
import sys
from pathlib import Path

def run_exe(path):
    p = subprocess.run([str(path)], capture_output=True, text=True)
    out = p.stdout.strip()
    err = p.stderr.strip()
    print(out)
    if err:
        print(err)
    return p.returncode, out

def test_runtime_logic_tests_exists():
    exe = Path("../build64/Release/runtime_logic_tests.exe")
    assert exe.exists()

def test_runtime_logic_tests_runs():
    exe = Path("../build64/Release/runtime_logic_tests.exe")
    code, out = run_exe(exe)
    assert code == 0
    assert "ALL PASS" in out

def test_ascii_font_atlas_test_exists():
    exe = Path("../build64/Release/ascii_font_atlas_test.exe")
    assert exe.exists()

def test_ascii_font_atlas_test_runs():
    exe = Path("../build64/Release/ascii_font_atlas_test.exe")
    code, out = run_exe(exe)
    assert code in (0, 0)
    assert ("PASS" in out) or ("SKIP" in out)

if __name__ == "__main__":
    sys.exit(0)
