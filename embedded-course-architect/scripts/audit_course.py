import os
import re

def scan_directory(root_dir):
    issues = []
    
    for dirpath, dirnames, filenames in os.walk(root_dir):
        # 1. Check for empty directories
        if not filenames and not dirnames:
             issues.append(f"[EMPTY_DIR] {dirpath}")
        
        # Filter relevant files
        c_files = [f for f in filenames if f.endswith('.c')]
        h_files = [f for f in filenames if f.endswith('.h')]
        
        # 2. Check for missing implementation files
        for h_file in h_files:
            base_name = h_file[:-2]
            c_file = base_name + '.c'
            if c_file not in c_files:
                issues.append(f"[MISSING_IMPL] Header '{h_file}' in '{dirpath}' has no corresponding '{c_file}'")

        # 3. Scan file content
        for filename in filenames:
            if filename.endswith(('.c', '.h')):
                filepath = os.path.join(dirpath, filename)
                try:
                    with open(filepath, 'r', encoding='utf-8') as f:
                        content = f.read()
                        
                        # Check for TODOs
                        if 'TODO' in content:
                             issues.append(f"[TODO_FOUND] {filepath}")

                        # Check Naming Convention (only in code/, not hardware/)
                        if 'code' in root_dir:
                            if re.search(r'\bHAL_[A-Z][a-zA-Z0-9_]*', content):
                                issues.append(f"[NAMING_VIOLATION] Found 'HAL_' prefix in '{filepath}' (Use PascalCase 'Hal' for course code)")
                except Exception as e:
                    issues.append(f"[READ_ERROR] Could not read {filepath}: {e}")

    return issues

if __name__ == "__main__":
    print("Auditing 'code/' directory...")
    code_issues = scan_directory("code")
    
    print("Auditing 'hardware/' directory...")
    hw_issues = scan_directory("hardware")
    
    all_issues = code_issues + hw_issues
    
    if all_issues:
        print(f"\nFound {len(all_issues)} issues:")
        for issue in all_issues:
            print(issue)
    else:
        print("\nNo issues found! (Unlikely...)")
