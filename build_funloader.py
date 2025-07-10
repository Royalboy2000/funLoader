import os
import subprocess
import platform

# --- Configuration ---
SOLUTION_FILE = "funLoader.sln"
BUILD_CONFIGURATION = "Release"
BUILD_PLATFORM = "x64" # Or "Win32" for x86, but project is designed for x64

# --- "Requirements" ---
# This project is a C++/Assembly project designed to be built with Visual Studio.
# The primary requirements are:
# 1. Visual Studio IDE: Community, Professional, or Enterprise edition.
#    (e.g., Visual Studio 2019 or 2022 are recommended).
# 2. "Desktop development with C++" workload installed in Visual Studio.
#    This includes the C++ compiler, Windows SDK, and standard libraries.
# 3. MASM (Microsoft Macro Assembler): This is usually included with the
#    "Desktop development with C++" workload if individual components for C++
#    ATL/MFC development are selected, or it can be added. It's needed for `syscalls.asm`.
#
# There are no external libraries to download via pip or other package managers.

def find_msbuild():
    """
    Attempts to find MSBuild.exe.
    Checks common Visual Studio installation paths and the PATH environment variable.
    """
    # Check PATH first
    if subprocess.call("where MSBuild.exe", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0:
        return "MSBuild.exe" # Found in PATH

    # Common paths for different Visual Studio versions (prioritize newer versions)
    # These paths might need adjustment based on actual VS installations.
    vs_versions = {
        "VS2022": os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"), "Microsoft Visual Studio", "2022"),
        "VS2019": os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files (x86)"), "Microsoft Visual Studio", "2019"),
    }

    # MSBuild paths relative to VS installation root (can vary by edition: Community, Professional, Enterprise, BuildTools)
    # For VS 2019/2022, MSBuild is often under Current/Bin or a specific version subfolder.
    # Example: C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe
    # Example: C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe

    msbuild_paths_to_check = []
    for vs_name, vs_root_base in vs_versions.items():
        if os.path.exists(vs_root_base):
            for edition in ["Enterprise", "Professional", "Community", "BuildTools"]:
                # Path for MSBuild shipped with VS itself
                msbuild_path = os.path.join(vs_root_base, edition, "MSBuild", "Current", "Bin", "MSBuild.exe")
                if os.path.exists(msbuild_path):
                    msbuild_paths_to_check.append(msbuild_path)
                # AMD64 version, if needed explicitly (though the above usually works for x64 builds too)
                msbuild_path_amd64 = os.path.join(vs_root_base, edition, "MSBuild", "Current", "Bin", "amd64", "MSBuild.exe")
                if os.path.exists(msbuild_path_amd64):
                    msbuild_paths_to_check.append(msbuild_path_amd64)

    # Add a common path for Build Tools standalone installer (VS 2017+)
    bt_path_2019_plus = os.path.join(os.environ.get("ProgramFiles (x86)", "C:\\Program Files (x86)"),
                                     "Microsoft Visual Studio", "2019", "BuildTools", "MSBuild", "Current", "Bin", "MSBuild.exe")
    if os.path.exists(bt_path_2019_plus):
        msbuild_paths_to_check.append(bt_path_2019_plus)

    bt_path_2022 = os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"),
                                "Microsoft Visual Studio", "2022", "BuildTools", "MSBuild", "Current", "Bin", "MSBuild.exe")
    if os.path.exists(bt_path_2022):
        msbuild_paths_to_check.append(bt_path_2022)


    # Check older VS versions (e.g., VS2017, though less likely for this project's style)
    # Path for MSBuild v15.0 (VS2017)
    vs2017_msbuild = os.path.join(os.environ.get("ProgramFiles (x86)", "C:\\Program Files (x86)"),
                                 "Microsoft Visual Studio", "2017", "BuildTools", "MSBuild", "15.0", "Bin", "MSBuild.exe")
    if os.path.exists(vs2017_msbuild):
         msbuild_paths_to_check.append(vs2017_msbuild)


    for path in msbuild_paths_to_check:
        if os.path.exists(path):
            print(f"Found MSBuild at: {path}")
            return path

    # Fallback: try a very common MSBuild path if others fail (VS 2017+)
    # This is a guess if specific edition paths weren't found.
    common_msbuild_path = r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    if os.path.exists(common_msbuild_path):
        print(f"Found MSBuild at common fallback: {common_msbuild_path}")
        return common_msbuild_path

    print("MSBuild.exe not found in common locations or PATH.")
    print("Please ensure Visual Studio or Visual Studio Build Tools are installed and MSBuild is in your PATH,")
    print("or update this script with the correct path to MSBuild.exe.")
    return None

def compile_project(msbuild_path):
    if not os.path.exists(SOLUTION_FILE):
        print(f"Error: Solution file '{SOLUTION_FILE}' not found in the current directory.")
        return False

    # Construct the MSBuild command
    # /t:Build or /t:Rebuild
    # /p:Configuration=Release
    # /p:Platform=x64
    # /m for parallel builds
    # /verbosity:minimal (can be normal, detailed, diagnostic)
    command = [
        msbuild_path,
        SOLUTION_FILE,
        "/t:Rebuild", # Clean and then build
        f"/p:Configuration={BUILD_CONFIGURATION}",
        f"/p:Platform={BUILD_PLATFORM}",
        "/m", # Use multiple cores if available
        "/verbosity:minimal"
    ]

    print(f"\nAttempting to compile: {' '.join(command)}")
    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        stdout, stderr = process.communicate()

        if process.returncode == 0:
            print(f"\nBuild successful for {SOLUTION_FILE} ({BUILD_CONFIGURATION}|{BUILD_PLATFORM})!")
            # Output executable is typically under: funLoader/x64/Release/funLoader.exe
            # or bin/Release/x64/funLoader.exe depending on project settings
            output_dir1 = os.path.join("funLoader", BUILD_PLATFORM, BUILD_CONFIGURATION)
            output_dir2 = os.path.join("bin", BUILD_CONFIGURATION, BUILD_PLATFORM) # Common alternative
            output_exe_name = "funLoader.exe"

            expected_exe_path1 = os.path.join(output_dir1, output_exe_name)
            expected_exe_path2 = os.path.join(output_dir2, output_exe_name)

            if os.path.exists(expected_exe_path1):
                print(f"Output: {os.path.abspath(expected_exe_path1)}")
            elif os.path.exists(expected_exe_path2):
                 print(f"Output: {os.path.abspath(expected_exe_path2)}")
            else:
                print(f"Build reported success, but expected output '{output_exe_name}' not found in typical locations:")
                print(f" - {os.path.abspath(output_dir1)}")
                print(f" - {os.path.abspath(output_dir2)}")
            return True
        else:
            print(f"\nBuild failed for {SOLUTION_FILE}. Return code: {process.returncode}")
            print("Stdout:")
            print(stdout)
            print("Stderr:")
            print(stderr)
            return False
    except FileNotFoundError:
        print(f"Error: MSBuild command '{msbuild_path}' not found. Is it installed and in PATH?")
        return False
    except Exception as e:
        print(f"An error occurred during compilation: {e}")
        return False

if __name__ == "__main__":
    print("funLoader Build Script")
    print("----------------------")
    print("Platform: Windows (requires Visual Studio / MSBuild)")

    if platform.system() != "Windows":
        print("Error: This build script is designed for Windows and relies on MSBuild.")
        exit(1)

    print("\nStep 1: Checking for MSBuild.exe...")
    msbuild = find_msbuild()

    if msbuild:
        print("\nStep 2: Attempting to compile the project...")
        if compile_project(msbuild):
            print("\nCompilation process finished.")
        else:
            print("\nCompilation process failed.")
    else:
        print("\nCannot proceed with compilation as MSBuild was not found.")
        print("Please install Visual Studio with C++ Desktop Development workload,")
        print("or Visual Studio Build Tools, and ensure MSBuild.exe is accessible.")

    # "Downloading Requirements" - As explained, this project's requirements are primarily
    # the build tools and SDKs, not downloadable packages via this script.
    print("\n--- Requirements Reminder ---")
    print("This project requires Visual Studio (e.g., 2019 or 2022) with:")
    print("  - 'Desktop development with C++' workload.")
    print("  - MASM support (usually part of the C++ workload or an individual component).")
    print("Ensure these are installed to build the project successfully.")
    print("There are no external libraries to download via this script.")
