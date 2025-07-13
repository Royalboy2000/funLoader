import os
import subprocess
import sys
import argparse

def find_vsdevcmd():
    """Finds the VsDevCmd.bat script using vswhere."""
    if sys.platform != "win32":
        print("This script is intended to be run on Windows.")
        return None

    vswhere = os.path.join(
        os.environ.get("ProgramFiles(x86)", ""),
        "Microsoft Visual Studio",
        "Installer",
        "vswhere.exe"
    )
    if not os.path.isfile(vswhere):
        return None
    cmd = [
        vswhere,
        "-latest",
        "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "-find", "Common7\\Tools\\VsDevCmd.bat"
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else None

def get_vs_env(vsdevcmd_path):
    """
    Captures the environment variables set by VsDevCmd.bat.
    Returns a dict of environment variables.
    """
    # Call VsDevCmd.bat and then dump the environment with 'set'
    proc = subprocess.run(
        ['cmd.exe', '/s', '/c',
         f'call "{vsdevcmd_path}" -arch=amd64 -host_arch=amd64 -no_logo >nul && set'],
        capture_output=True, text=True
    )
    env = {}
    for line in proc.stdout.splitlines():
        if '=' in line:
            key, val = line.split('=', 1)
            env[key] = val
    return env

def compile_project(vsdevcmd_path):
    """Compiles the funLoader project using MSBuild in the VS environment."""
    output_file = "output.txt"
    # Handle existing log file
    if os.path.exists(output_file):
        resp = input(f"'{output_file}' exists. Delete and continue? (y/n): ").strip().lower()
        if resp != 'y':
            print("Aborting.")
            sys.exit(1)
        os.remove(output_file)

    # Locate VsDevCmd.bat
    if vsdevcmd_path:
        vsdevcmd = vsdevcmd_path
    else:
        vsdevcmd = find_vsdevcmd()

    if not vsdevcmd or not os.path.isfile(vsdevcmd):
        print("Error: VsDevCmd.bat not found.")
        print("Please ensure Visual Studio 2022 with C++ tools is installed.")
        print("Alternatively, set the VSDCMD_PATH environment variable or use the --vsdevcmd_path argument.")
        sys.exit(1)

    # Capture the VS environment
    env_vars = get_vs_env(vsdevcmd)
    if "PATH" not in env_vars:
        print("Error: Failed to capture VS environment.")
        sys.exit(1)

    # Project solution
    solution = "funLoader.sln"
    if not os.path.isfile(solution):
        print(f"Error: Solution '{solution}' not found.")
        sys.exit(1)

    # Invoke MSBuild directly in the captured environment
    result = subprocess.run(
        ["msbuild", solution, "/p:Configuration=Release", "/p:Platform=x64", "/m"],
        env={**os.environ, **env_vars},
        capture_output=True,
        text=True
    )

    # Write logs
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(result.stdout)
        f.write(result.stderr)

    # Report status and show last errors if any
    if result.returncode == 0:
        print("Build succeeded.")
    else:
        print(f"Build failed with exit code {result.returncode}. See '{output_file}' for details.")
        print("Last error lines:")
        for line in result.stderr.strip().splitlines()[-10:]:
            print(line)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compile the funLoader project.")
    parser.add_argument("--vsdevcmd_path", help="Path to VsDevCmd.bat")
    args = parser.parse_args()

    vsdevcmd_path = args.vsdevcmd_path or os.environ.get("VSDCMD_PATH")

    compile_project(vsdevcmd_path)
