import os
import subprocess
import tempfile

def find_vcvars_short_path():
    """Finds the 8.3 short path for the vcvars64.bat script."""
    program_files_x86 = os.environ.get("ProgramFiles(x86)")
    if not program_files_x86:
        return None

    result = subprocess.run(f'for %%I in ("{program_files_x86}") do @echo %%~sI', shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        return None

    pf86_short = result.stdout.strip()

    return os.path.join(pf86_short, "MICROS~1", "2022", "COMMUN~1", "VC", "AUXILI~1", "Build", "vcvars64.bat")


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

    vcvars_path = find_vcvars_short_path()
    if not vcvars_path:
        print("vcvars64.bat not found.")
        return

    solution_path = "funLoader.sln"
    if not os.path.exists(solution_path):
        print("Solution file not found.")
        return

    with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix=".bat") as f:
        f.write(f'call "{vcvars_path}" x64\n')
        f.write(f'msbuild {solution_path} /p:Configuration=Release /p:Platform=x64\n')
        temp_file_path = f.name

    print(f"Executing command: {temp_file_path}")

    with open("output.txt", "w") as f:
        subprocess.run([temp_file_path], stdout=f, stderr=f)

    os.remove(temp_file_path)

if __name__ == "__main__":
    compile_project()
