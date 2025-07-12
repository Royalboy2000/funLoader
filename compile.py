import os
import subprocess
import tempfile

def find_vs_tools_dir():
    """Finds the Visual Studio Tools directory."""
    vswhere_path = os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.exists(vswhere_path):
        return None

    result = subprocess.run([vswhere_path, "-latest", "-property", "installationPath"], capture_output=True, text=True)
    if result.returncode != 0:
        return None

    return os.path.join(result.stdout.strip(), "Common7", "Tools")

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

    vs_tools_dir = find_vs_tools_dir()
    if not vs_tools_dir:
        print("Visual Studio Tools directory not found.")
        return

    solution_path = "funLoader.sln"
    if not os.path.exists(solution_path):
        print("Solution file not found.")
        return

    with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix=".bat") as f:
        f.write(f'subst Z: "{vs_tools_dir}"\n')
        f.write(f'call Z:\\VsDevCmd.bat -arch=amd64 -host_arch=amd64 -no_logo\n')
        f.write(f'msbuild {solution_path} /p:Configuration=Release /p:Platform=x64\n')
        f.write('subst Z: /d\n')
        temp_file_path = f.name

    print(f"Executing command: {temp_file_path}")

    with open("output.txt", "w") as f:
        subprocess.run([temp_file_path], stdout=f, stderr=f)

    os.remove(temp_file_path)

if __name__ == "__main__":
    compile_project()
