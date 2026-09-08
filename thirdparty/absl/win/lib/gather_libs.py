import os
import argparse

def main():
    parser = argparse.ArgumentParser(description="Gather *.lib and *d.lib files into a CMake set statement file.")
    parser.add_argument("-d", "--dir", default=".", help="Directory to scan (default: current directory)")
    parser.add_argument("-o", "--output", default="absl_libs.cmake", help="Output CMake file (default: absl_libs.cmake)")
    parser.add_argument("-p", "--path-type", choices=["filename", "relative", "absolute"], default="filename",
                        help="Type of paths to output: filename, relative, or absolute (default: filename)")
    
    args = parser.parse_args()
    
    target_dir = os.path.abspath(args.dir)
    if not os.path.isdir(target_dir):
        print(f"Error: Directory '{target_dir}' does not exist.")
        return

    # List all files ending with .lib
    files = [f for f in os.listdir(target_dir) if f.lower().endswith('.lib') and os.path.isfile(os.path.join(target_dir, f))]
    
    # We want to separate them into Release and Debug.
    # The heuristic:
    # 1. Collect all filenames.
    # 2. For any file Xd.lib, if X.lib also exists in the directory, then Xd.lib is Debug and X.lib is Release.
    # 3. What if a file ends in d.lib (e.g. absl_cord.lib) but its "release" counterpart (absl_cor.lib) does not exist,
    #    and there are other matched pairs in the directory? Then it is Release.
    # 4. If there are no pairs at all (e.g. only one config was built), we default to checking if it ends with 'd.lib'.
    
    file_set = set(files)
    debug_libs = []
    release_libs = []
    
    # First, find all matched pairs
    pairs = set()
    for f in files:
        if f.lower().endswith('d.lib'):
            # Check if counterpart without 'd' exists
            # e.g., absl_based.lib -> absl_base.lib
            release_counterpart = f[:-5] + '.lib'
            if release_counterpart in file_set:
                pairs.add(f)
                pairs.add(release_counterpart)
                
    has_pairs = len(pairs) > 0
    
    for f in sorted(files, key=lambda s: s.lower()):
        # Determine path format
        if args.path_type == "filename":
            path_str = f
        elif args.path_type == "relative":
            # Use forward slashes for CMake compatibility
            path_str = os.path.relpath(os.path.join(target_dir, f), os.getcwd()).replace('\\', '/')
            if not path_str.startswith('.'):
                path_str = "./" + path_str
        else: # absolute
            path_str = os.path.join(target_dir, f).replace('\\', '/')
            
        # Classify
        if has_pairs:
            if f in pairs:
                if f.lower().endswith('d.lib') and (f[:-5] + '.lib') in file_set:
                    debug_libs.append(path_str)
                else:
                    release_libs.append(path_str)
            else:
                # If it's not in the pairs:
                # If it ends with d.lib, but the counterpart doesn't exist, it's likely release (like absl_cord.lib).
                release_libs.append(path_str)
        else:
            # Fallback: if no pairs at all, simple suffix check
            if f.lower().endswith('d.lib'):
                debug_libs.append(path_str)
            else:
                release_libs.append(path_str)
                
    # Write to CMake file
    output_path = os.path.join(target_dir, args.output) if not os.path.isabs(args.output) else args.output
    
    try:
        with open(output_path, 'w', encoding='utf-8') as out_file:
            out_file.write("# Generated CMake library list\n\n")
            
            out_file.write("set(ABSL_RELEASE_LIBS\n")
            for lib in release_libs:
                out_file.write(f"    \"{lib}\"\n")
            out_file.write(")\n\n")
            
            out_file.write("set(ABSL_DEBUG_LIBS\n")
            for lib in debug_libs:
                out_file.write(f"    \"{lib}\"\n")
            out_file.write(")\n")
            
        print(f"Successfully gathered libraries in '{target_dir}':")
        print(f"  Release libraries: {len(release_libs)}")
        print(f"  Debug libraries:   {len(debug_libs)}")
        print(f"Written to '{output_path}'")
    except Exception as e:
        print(f"Error writing to output file: {e}")

if __name__ == "__main__":
    main()
