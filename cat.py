import os
import re

# Extensions pertinentes à inclure
INCLUDED_EXTENSIONS = (
    ".cpp", ".cc", ".c",
    ".hpp", ".hh", ".h",
    ".ipp", ".inl",
    ".md", ".txt"
)

# Dossiers et prefixes à ignorer
IGNORED_PREFIXES = (
    ".git", ".vscode", "__pycache__",
    "build", "cmake-build-debug", "cmake-build-release",
    "out", "dist", "bin", ".cache",
    ".VSCodeCounter", "idea", "idea/out"
)

# Extensions à ignorer (liste enrichie avec toutes celles que tu as fournies)
IGNORED_EXTENSIONS = (
    # binaires / build
    ".o", ".exe", ".bin",
    ".a1", ".s1", ".a",
    # documents / captures
    ".png", ".jpg", ".jpeg",
    ".pdf", ".md~",
    ".xml",
    # textes non pertinents
    ".txt",
    # archives
    ".zip", ".rar", ".7z",
    ".gz", ".bz2", ".xz",
    # JSON et data
    ".json",
    # divers
    ".ipynb",
    ".blend", ".blend1",
)

# Patterns fichiers de config
CONFIG_FILE_PATTERNS = [
    re.compile(r"CMakeLists\.txt"),
    re.compile(r".*\.cmake$"),
    re.compile(r"compile_commands\.json$"),
    re.compile(r"Makefile$"),
    re.compile(r".*\.clang.*$"),
]

def is_config_file(filename: str) -> bool:
    return any(pattern.match(filename) for pattern in CONFIG_FILE_PATTERNS)

def should_skip(path: str, filename: str) -> bool:
    # Exclusion par dossier
    parts = path.split(os.sep)
    if any(part.startswith(IGNORED_PREFIXES) for part in parts):
        return True
    if filename.startswith(IGNORED_PREFIXES):
        return True

    # Extensions à ignorer
    if filename.endswith(IGNORED_EXTENSIONS):
        return True

    # Fichiers de configuration
    if is_config_file(filename):
        return True

    # N'inclure que les fichiers utiles (C++ + doc)
    if not filename.endswith(INCLUDED_EXTENSIONS):
        return True

    return False

def concat_files(root_dir: str, output_file: str):
    with open(output_file, 'w', encoding='utf-8') as outfile:
        for dirpath, _, filenames in os.walk(root_dir):
            for filename in filenames:
                if should_skip(dirpath, filename):
                    continue

                file_path = os.path.join(dirpath, filename)
                try:
                    with open(file_path, 'r', encoding='utf-8') as infile:
                        relative_path = os.path.relpath(file_path, root_dir)
                        outfile.write(f"\n--- FILE: {relative_path} ---\n")
                        outfile.write(infile.read())
                        outfile.write("\n")
                except Exception as e:
                    print(f"Erreur lecture {file_path} : {e}")

    print(f"\nFichier généré : {output_file}")

# Exemple
if __name__ == "__main__":
    source_dir = "."
    output = "./full_cpp_dump.txt"
    concat_files(source_dir, output)
