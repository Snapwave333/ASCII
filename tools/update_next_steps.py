import os
import re
import time

ROOT = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(ROOT, os.pardir))
DOC_PATH = os.path.join(ROOT, 'docs', 'NEXT_STEPS.md')

def exists(path):
    return os.path.exists(path)

def file_contains(path, needle):
    try:
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            return needle in f.read()
    except Exception:
        return False

def compute_progress():
    week1 = exists(os.path.join(ROOT, 'build64', 'Release', 'NeonGlyph.exe')) or exists(os.path.join(ROOT, 'build', 'Release', 'NeonGlyph.exe'))
    cp_path = os.path.join(ROOT, 'src', 'ComputePipelines.cpp')
    week2 = file_contains(cp_path, 'CreateComputePipeline("ascii_convert"') and file_contains(cp_path, 'vkCmdDispatch')
    week3 = exists(os.path.join(ROOT, 'build64', 'Release', 'ascii_font_atlas_test.exe')) or exists(os.path.join(ROOT, 'build', 'Release', 'ascii_font_atlas_test.exe'))
    week4 = exists(os.path.join(ROOT, 'build64', 'Release', 'runtime_logic_tests.exe')) or exists(os.path.join(ROOT, 'build', 'Release', 'runtime_logic_tests.exe'))
    week5 = exists(os.path.join(ROOT, 'assets', 'branding', 'logo.svg'))
    week6 = exists(os.path.join(ROOT, 'build64', 'Release', 'e2e_test.exe')) or exists(os.path.join(ROOT, 'build', 'Release', 'e2e_test.exe'))
    return [week1, week2, week3, week4, week5, week6]

def update_doc(progress):
    if not exists(DOC_PATH):
        return False
    with open(DOC_PATH, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    def mark(line, done):
        return re.sub(r'- \[( |x)\]', f'- [{'x' if done else ' '}]', line)
    updated = []
    week_index = 0
    for line in lines:
        if week_index < 6 and re.search(r'^- \[.\] Week \d+ —', line):
            updated.append(mark(line, progress[week_index]))
            week_index += 1
        else:
            updated.append(line)
    ts = time.strftime('%Y-%m-%d %H:%M:%S')
    banner = f'\nProgress Auto-Update: {ts}\n'
    if 'Progress Auto-Update:' in ''.join(updated):
        updated = [re.sub(r'Progress Auto-Update: .*', f'Progress Auto-Update: {ts}', l) for l in updated]
    else:
        updated.append(banner)
    ver_file = os.path.join(ROOT, 'build64', 'Release', 'verification_results.txt')
    if not exists(ver_file):
        ver_file = os.path.join(ROOT, 'build', 'Release', 'verification_results.txt')
    if exists(ver_file):
        try:
            with open(ver_file, 'r', encoding='utf-8', errors='ignore') as vf:
                lines_ver = vf.read().strip().splitlines()
            kv = {}
            for l in lines_ver:
                if '=' in l:
                    k, v = l.split('=', 1)
                    kv[k.strip()] = v.strip()
            summary = 'Verification Auto-Update: '
            parts = []
            for k in ['runtime_logic_tests', 'ascii_font_atlas_test', 'e2e_test', 'gpu_pipeline_test']:
                if k in kv:
                    status = kv[k]
                    label = 'PASS' if status == '0' else ('SKIP' if status == '2' else 'FAIL')
                    if k == 'gpu_pipeline_test':
                        gpu_log = os.path.join(ROOT, 'build64', 'Release', 'gpu_pipeline_test.log')
                        if exists(gpu_log):
                            try:
                                with open(gpu_log, 'r', encoding='utf-8', errors='ignore') as gl:
                                    gltxt = gl.read()
                                if 'PASS' in gltxt:
                                    label = 'PASS'
                                elif 'SKIP' in gltxt:
                                    label = 'SKIP'
                            except Exception:
                                pass
                        parts = [p for p in parts if not p.startswith('gpu_pipeline_test:')]
                        parts.append(f"gpu_pipeline_test:{label}")
                    else:
                        parts.append(f"{k}:{label}")
            summary_line = summary + ', '.join(parts) + '\n'
            has_summary = any('Verification Auto-Update:' in s for s in updated)
            if has_summary:
                updated = [re.sub(r'Verification Auto-Update: .*', summary_line.strip(), s) if 'Verification Auto-Update:' in s else s for s in updated]
            else:
                updated.append(summary_line)
        except Exception:
            pass
    with open(DOC_PATH, 'w', encoding='utf-8') as f:
        f.writelines(updated)
    return True

def main():
    prog = compute_progress()
    ok = update_doc(prog)
    if not ok:
        raise SystemExit(1)

if __name__ == '__main__':
    main()