#!/usr/bin/env python3
import sys, re, os

BAD = re.compile(r"\b(TODO|FIXME|placeholder|fake|stub|not implemented)\b", re.IGNORECASE)
ALLOW_DIRS = {"docs", ".github"}
ALLOW_EXTS = {".md", ".txt", ".license", ".png", ".jpg", ".jpeg", ".gif", ".svg"}

def is_allowed(path):
    parts = path.replace("\\","/").split("/")
    if parts[0] in ALLOW_DIRS:
        return True
    _, ext = os.path.splitext(path)
    return ext in ALLOW_EXTS

def scan(root):
    offenders = []
    for base, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if not d.startswith('.')]
        for f in files:
            p = os.path.join(base, f)
            if is_allowed(p):
                continue
            try:
                with open(p, 'r', encoding='utf-8', errors='ignore') as fh:
                    for i, line in enumerate(fh, 1):
                        if BAD.search(line):
                            offenders.append((p, i, line.strip()))
            except Exception:
                pass
    return offenders

def main():
    root = sys.argv[1] if len(sys.argv) > 1 else '.'
    offenders = scan(root)
    if offenders:
        print('Golden Rule violations:')
        for p, i, l in offenders:
            print(f'{p}:{i}: {l}')
        sys.exit(1)
    print('No Golden Rule violations.')
    sys.exit(0)

if __name__ == '__main__':
    main()