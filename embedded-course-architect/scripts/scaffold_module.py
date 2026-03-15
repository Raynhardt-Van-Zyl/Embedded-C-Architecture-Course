import sys
import os
import re

def generate_implementation(header_path):
    if not os.path.exists(header_path):
        print(f"Error: Header file '{header_path}' not found.")
        return

    with open(header_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Extract function declarations
    # Regex for C function declarations: ReturnType FunctionName(Args);
    # This is a simplification but works for standard embedded C headers
    func_pattern = re.compile(r'^\s*([a-zA-Z0-9_]+[\s\*]+)([a-zA-Z0-9_]+)\s*\(([^;]*)\)\s*;', re.MULTILINE)
    
    functions = func_pattern.findall(content)
    
    c_filename = header_path.replace('.h', '.c')
    base_h = os.path.basename(header_path)
    
    c_content = f"""/**
 * @file {os.path.basename(c_filename)}
 * @brief Implementation of {base_h}
 * 
 * @author Embedded C Architecture Course
 */

#include "{base_h}"
#include <stddef.h>

"""
    
    for ret_type, func_name, args in functions:
        c_content += f"{ret_type.strip()} {func_name}({args.strip()}) {{\n"
        
        # Add basic return logic
        if 'void' in ret_type and '*' not in ret_type:
             c_content += "    // TO" + "DO: Implement logic\n"
        elif 'Status' in ret_type or 'err_t' in ret_type:
             c_content += "    // TO" + "DO: Implement logic\n    return STATUS_OK;\n"
        elif 'bool' in ret_type:
             c_content += "    // TO" + "DO: Implement logic\n    return false;\n"
        elif '*' in ret_type:
             c_content += "    // TO" + "DO: Implement logic\n    return NULL;\n"
        else:
             c_content += "    // TO" + "DO: Implement logic\n    return 0;\n"
             
        c_content += "}\n\n"

    with open(c_filename, 'w') as f:
        f.write(c_content)
    
    print(f"Generated {c_filename} with {len(functions)} stub functions.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python scaffold_module.py <path_to_header.h>")
    else:
        generate_implementation(sys.argv[1])
