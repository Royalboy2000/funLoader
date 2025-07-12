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

def find_vcvars():
    """Finds the vcvars64.bat script."""
    vswhere_path = os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.exists(vswhere_path):
        return None

    result = subprocess.run([vswhere_path, "-latest", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-find", "VC\\Auxiliary\\Build\\vcvars64.bat"], capture_output=True, text=True)
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

    msbuild_path = find_msbuild()
    if not msbuild_path:
        print("MSBuild not found.")
        return

    vcvars_path = find_vcvars()
    if not vcvars_path:
        print("vcvars64.bat not found.")
        return

    solution_path = "funLoader.sln"
    if not os.path.exists(solution_path):
        print("Solution file not found.")
        return

    command = f'call "{vcvars_path}" && "{msbuild_path}" {solution_path} /p:Configuration=Release /p:Platform=x64'

    print(f"Executing command: {command}")

    with open("output.txt", "w") as f:
        subprocess.run(command, stdout=f, stderr=f, shell=True)

if __name__ == "__main__":
    compile_project()
