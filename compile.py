import os
import subprocess

def find_vsdevcmd():
    """Finds the VsDevCmd.bat script."""
    vswhere_path = os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.exists(vswhere_path):
        return None

    result = subprocess.run([vswhere_path, "-latest", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-find", "Common7\\Tools\\VsDevCmd.bat"], capture_output=True, text=True)
    if result.returncode != 0:
        return None

    return result.stdout.strip()

def compile_project():
    """Compiles the funLoader project."""
    if os.path.exists("output.txt"):
        while True:
            choice = input("The output.txt file already exists. Do you want to delete it? (y/n): ").lower()
            if choice in ["y", "n"]:
                break
        if choice == "n":
            print("Compilation cancelled.")
            return

    vsdevcmd_path = find_vsdevcmd()
    if not vsdevcmd_path:
        print("VsDevCmd.bat not found.")
        return

    solution_path = "funLoader.sln"
    if not os.path.exists(solution_path):
        print("Solution file not found.")
        return

    build_cmd = (
        f'call "{vsdevcmd_path}" -arch=amd64 -host_arch=amd64 -no_logo '
        f'&& msbuild "{solution_path}" /p:Configuration=Release /p:Platform=x64 /m'
    )

    print(f"Executing command: {build_cmd}")

    result = subprocess.run(
        ["cmd.exe", "/C", build_cmd],
        capture_output=True,
        text=True
    )

    with open("output.txt", "w", encoding="utf-8") as f:
        f.write(result.stdout)
        f.write(result.stderr)

    if result.returncode == 0:
        print("Build succeeded.")
    else:
        print(f"Build failed (exit code {result.returncode}). See output.txt for details.")
        for line in result.stderr.strip().splitlines()[-20:]:
            print(line)

if __name__ == "__main__":
    compile_project()
