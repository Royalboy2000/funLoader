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

def get_vs_env(vsdevcmd_path):
    """Gets the environment variables set by VsDevCmd.bat."""
    command = f'cmd.exe /s /c "call \\"{vsdevcmd_path}\\" -arch=amd64 -host_arch=amd64 -no_logo >nul && set"'
    proc = subprocess.run(command, capture_output=True, text=True, shell=True)
    env = {}
    for line in proc.stdout.splitlines():
        if '=' in line:
            k, v = line.split('=', 1)
            env[k.upper()] = v
    return env

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

    env = get_vs_env(vsdevcmd_path)

    solution_path = "funLoader.sln"
    if not os.path.exists(solution_path):
        print("Solution file not found.")
        return

    command = ["msbuild", solution_path, "/p:Configuration=Release", "/p:Platform=x64"]

    print(f"Executing command: {' '.join(command)}")

    with open("output.txt", "w") as f:
        subprocess.run(command, env={**os.environ, **env}, check=True, stdout=f, stderr=f)

if __name__ == "__main__":
    compile_project()
