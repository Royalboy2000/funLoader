import os
import subprocess

def find_msbuild():
    """Finds the MSBuild executable."""
    vswhere_path = os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.exists(vswhere_path):
        return None

    result = subprocess.run([vswhere_path, "-latest", "-requires", "Microsoft.Component.MSBuild", "-find", "MSBuild\\**\\Bin\\MSBuild.exe"], capture_output=True, text=True)
    if result.returncode != 0:
        return None

    return result.stdout.strip()

def compile_project():
    """Compiles the funLoader project."""
    msbuild_path = find_msbuild()
    if not msbuild_path:
        print("MSBuild not found.")
        return

    solution_path = "funLoader.sln"
    if not os.path.exists(solution_path):
        print("Solution file not found.")
        return

    command = [msbuild_path, solution_path, "/p:Configuration=Release", "/p:Platform=x64"]

    print(f"Executing command: {' '.join(command)}")

    with open("output.txt", "w") as f:
        subprocess.run(command, stdout=f, stderr=f)

if __name__ == "__main__":
    compile_project()
