import os
import sys
import json
import time

def now_ts():
    return time.strftime('%Y-%m-%d %H:%M:%S')

def main():
    action = sys.argv[1] if len(sys.argv) > 1 else 'unspecified'
    details = {
        'timestamp': now_ts(),
        'action': action,
        'responsible': 'automation/AI assistant',
        'system_response': sys.argv[2] if len(sys.argv) > 2 else '',
        'verification_result': sys.argv[3] if len(sys.argv) > 3 else '',
    }
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
    log_dir = os.path.join(root, 'logs')
    os.makedirs(log_dir, exist_ok=True)
    log_path = os.path.join(log_dir, 'workflow_audit.log')
    with open(log_path, 'a', encoding='utf-8') as f:
        f.write(json.dumps(details) + '\n')

if __name__ == '__main__':
    main()