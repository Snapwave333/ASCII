#!/usr/bin/env python3
"""
Fix the duplicate PerformanceMeasurement struct definition
"""

import sys

def fix_performance_tracker():
    with open('src/production/PerformanceOptimizationTracker.cpp', 'r') as f:
        lines = f.readlines()
    
    # Find the duplicate struct (the second occurrence)
    first_struct_line = None
    duplicate_start = None
    duplicate_end = None
    
    for i, line in enumerate(lines):
        if 'struct PerformanceMeasurement {' in line:
            if first_struct_line is None:
                first_struct_line = i
            else:
                # Found the duplicate
                duplicate_start = i - 1  # Include the blank line before
                # Find the end of the duplicate struct
                brace_count = 0
                for j in range(i, len(lines)):
                    if '{' in lines[j]:
                        brace_count += lines[j].count('{')
                    if '}' in lines[j]:
                        brace_count -= lines[j].count('}')
                        if brace_count == 0:
                            duplicate_end = j + 1
                            break
                break
    
    if duplicate_start is not None and duplicate_end is not None:
        print(f"Removing duplicate struct at lines {duplicate_start+1}-{duplicate_end+1}")
        # Remove the duplicate
        new_lines = lines[:duplicate_start] + lines[duplicate_end:]
        
        with open('src/production/PerformanceOptimizationTracker.cpp', 'w') as f:
            f.writelines(new_lines)
        
        print("Fixed duplicate struct definition!")
    else:
        print("Could not find duplicate struct")

if __name__ == '__main__':
    fix_performance_tracker()