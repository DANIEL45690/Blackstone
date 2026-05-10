#!/usr/bin/env python3

import os
import sys
import subprocess
import platform
import shutil
import time
import re
import json
import hashlib
from pathlib import Path
from typing import Dict, List, Tuple, Optional


def setup_environment() -> None:
    if platform.system() == "Windows":
        os.system("")
    else:
        os.system("clear")


def gradient_text(
    text: str, start_rgb: Tuple[int, int, int], end_rgb: Tuple[int, int, int]
) -> str:
    lines = text.split("\n")
    result = []
    for line in lines:
        if not line.strip():
            result.append(line)
            continue
        colored_line = []
        for i, char in enumerate(line):
            if char == " ":
                colored_line.append(char)
                continue
            progress = i / max(len(line) - 1, 1)
            r = int(start_rgb[0] + (end_rgb[0] - start_rgb[0]) * progress)
            g = int(start_rgb[1] + (end_rgb[1] - start_rgb[1]) * progress)
            b = int(start_rgb[2] + (end_rgb[2] - start_rgb[2]) * progress)
            colored_line.append(f"\033[38;2;{r};{g};{b}m{char}\033[0m")
        result.append("".join(colored_line))
    return "\n".join(result)


def print_ascii_header() -> None:
    header = r"""
  ███                      █████              ████  ████
 ░░░                      ░░███              ░░███ ░░███
 ████  ████████    █████  ███████    ██████   ░███  ░███   ██████  ████████
░░███ ░░███░░███  ███░░  ░░░███░    ░░░░░███  ░███  ░███  ███░░███░░███░░███
 ░███  ░███ ░███ ░░█████   ░███      ███████  ░███  ░███ ░███████  ░███ ░░░
 ░███  ░███ ░███  ░░░░███  ░███ ███ ███░░███  ░███  ░███ ░███░░░   ░███
 █████ ████ █████ ██████   ░░█████ ░░████████ █████ █████░░██████  █████
░░░░░ ░░░░ ░░░░░ ░░░░░░     ░░░░░   ░░░░░░░░ ░░░░░ ░░░░░  ░░░░░░  ░░░░░



"""
    print(gradient_text(header, (0, 255, 255), (255, 0, 255)))


def print_art() -> None:
    art = r"""
⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡀
⢻⢷⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣾⡇
⢸⠀⠻⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣠⣤⢤⣶⠶⠶⢶⣶⡤⣤⣄⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⠋⢀⠇
⠈⣇⠀⠈⠻⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⡴⠞⠋⢉⡴⠞⠋⣿⠀⠀⠀⡯⠙⠳⣦⡉⠙⠲⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⠟⠁⠀⣼⠀
⠀⠹⣆⠀⠀⠈⠛⢦⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⠞⠋⠀⢀⣰⠏⠀⠀⠀⢻⡄⠀⢰⠇⠀⠀⠈⢻⣄⠀⠀⠙⢷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⡶⠋⠁⠀⠀⣰⠃⠀
⠀⠀⠹⣇⠀⠀⠀⠀⠉⠳⢦⣄⡀⠀⠀⠀⢀⡾⠃⠀⣠⠞⠋⠁⠀⠀⠀⠀⠀⠉⠉⠉⠀⠀⠀⠀⠀⠉⠙⢷⣄⠀⠙⢧⡀⠀⠀⠀⢀⣠⡶⠛⠁⠀⠀⠀⠀⣴⠃⠀⠀
⠀⠀⠀⠙⢧⡀⠀⠀⠀⠀⠀⠈⠙⠳⠶⢤⣿⣄⣀⣸⠋⢠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⠀⢹⣆⣀⣨⣷⡤⠶⠚⠋⠁⠀⠀⠀⠀⠀⢠⡾⠃⠀⠀⠀
⠀⠀⠀⠀⠈⠻⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠸⡇⠈⡇⠀⠀⠀⠀⠀⠀⠀⡀⠀⠀⠀⠀⠀⠀⠀⣼⠀⣼⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⠏⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠈⠻⢦⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⣧⠀⠀⠀⠀⠀⠀⠀⣷⠀⠀⠀⠀⠀⠀⠀⡟⠀⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⡴⠛⠁⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠳⣦⣀⠀⠀⠀⠀⠀⠀⢀⡟⠀⡏⠀⠀⠀⠀⠀⠀⢀⣿⠀⠀⠀⠀⠀⠀⠀⣿⠀⢿⡀⠀⠀⠀⠀⠀⠀⣠⡴⠞⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⡆⠉⢻⡶⢤⣀⡀⢀⡾⠁⣼⠃⠀⠀⠀⠀⠀⠀⣸⠹⣆⠀⠀⠀⠀⠀⠀⠹⣆⠘⢧⡀⢀⣠⡤⢶⡟⠉⢰⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⡏⣧⠀⢸⠃⠀⣨⠿⠋⣠⠞⠁⠀⠀⠀⠀⠀⠀⢸⡏⠀⣹⡆⠀⠀⠀⠀⠀⠀⠘⢦⣈⠛⢿⡅⠀⢸⡇⠀⡿⢻⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡕⣿⣧⣸⡀⠀⠛⡶⢶⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⣹⠰⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⠶⣾⠃⠀⢸⣇⡼⡿⢸⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠸⣇⠘⢯⡙⠷⣄⣸⠇⠀⠹⣆⠀⠀⠀⠀⠀⠀⠀⣴⠃⠀⠹⣄⠀⠀⠀⠀⠀⠀⢀⣼⠃⠀⢹⣆⣠⠞⣫⡿⠁⣼⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣄⢀⠙⢷⡘⣿⣷⡶⣄⠙⠷⣄⡀⠀⠀⠀⠘⠁⠀⠀⠀⠈⠃⠀⠀⠀⢀⣴⠞⢁⣤⢶⣾⡿⢡⡾⠋⡀⣰⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣿⠀⠸⣇⠈⣻⣷⣿⠳⣤⡈⠙⠓⠄⠀⠈⠳⡄⠀⣰⠛⠁⠀⠠⠞⠋⢀⣴⠟⣇⣿⡟⠀⣾⠀⠀⡿⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡄⠀⠻⣾⠏⠸⣿⣦⡈⠛⠶⢤⣤⣤⣤⠴⡷⠶⣿⠦⣤⣤⣤⡤⠾⠋⢁⣼⣿⠁⠹⣶⠏⠀⢰⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢷⣄⠀⣿⠀⠀⠘⠿⣿⣦⣤⢴⣿⡿⠃⠀⡷⠛⣦⠀⠘⢿⣷⠦⣤⣶⣿⠟⠁⠀⢀⡿⢀⣰⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣷⠘⣷⣄⠀⠀⠀⠉⠉⠉⠁⠀⠀⠀⡇⠀⡟⠀⠀⠀⠉⠉⠉⠉⠀⠀⠀⣠⣾⠁⡟⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⣴⡏⢹⢷⣦⣄⡀⠀⣀⣤⡤⢤⡀⡧⠀⠇⢀⡤⢤⣤⡀⠀⣀⣠⣴⣿⡏⢻⣼⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⠀⠀⠀⠈⣧⢸⡾⠁⣨⣿⡟⠙⢯⣀⠀⠀⠀⠀⠀⠀⢀⣀⡿⠉⢻⢿⡁⠘⣿⠃⡿⠀⠀⠀⢀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢿⡓⠶⠶⠿⡛⠥⠞⠉⢠⣿⣄⡀⠉⠉⠻⣦⣀⡴⠛⠉⠉⢀⣴⣿⡀⠙⠲⠬⣻⠷⠶⠶⢚⡿⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠦⣄⣀⣀⣀⣠⣴⡋⢻⣿⡛⢳⠒⣤⠼⣿⠧⣤⢲⡞⢻⣿⠋⢹⡦⣄⣀⣀⣀⣤⠔⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠹⣆⠈⠛⣾⣿⣧⣿⠙⠛⠓⠛⠚⠛⢋⣇⡾⣿⣷⠋⠀⣼⠋⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣷⡀⣿⣿⣆⠙⠃⠀⠀⠀⠀⠀⠘⠋⣼⡿⣿⢠⡾⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢳⡿⣮⡙⠛⣟⣻⣯⣯⣽⣟⣿⠛⢋⣴⣷⠟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣼⣏⠛⣋⣤⠶⠒⠶⣤⣙⠛⣹⢰⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⣧⡉⠉⠉⠀⣠⣤⡄⠀⠉⠉⢁⣼⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠛⠲⠤⠞⠋⠀⠙⠶⠤⠖⠋⠁⠀
"""
    print(gradient_text(art, (0, 255, 0), (0, 255, 255)))


def print_loading_animation(message: str, duration: float = 1.0) -> None:
    chars = "⣾⣽⣻⢿⡿⣟⣯⣷"
    end_time = time.time() + duration
    i = 0
    while time.time() < end_time:
        sys.stdout.write(f"\r\033[36m{message} {chars[i % len(chars)]}\033[0m")
        sys.stdout.flush()
        time.sleep(0.1)
        i += 1
    sys.stdout.write("\r" + " " * (len(message) + 2) + "\r")
    sys.stdout.flush()


class Installer:
    def __init__(self):
        self.os_type = platform.system()
        self.compiler = None
        self.source_file = "main.cpp"
        self.output_name = "bank_crypto"
        self.source_path = None
        self.install_path = None
        self.dependencies = []

    def detect_system(self) -> bool:
        print_loading_animation("Detecting system", 0.8)
        print(f"\033[32m✓ System: {self.os_type}\033[0m")

        if self.os_type == "Windows":
            if shutil.which("g++"):
                self.compiler = "g++"
            elif shutil.which("cl"):
                self.compiler = "cl"
            else:
                print(
                    "\033[31m✗ No suitable compiler found. Please install MinGW or Visual Studio\033[0m"
                )
                return False
        else:
            if shutil.which("g++"):
                self.compiler = "g++"
            elif shutil.which("clang++"):
                self.compiler = "clang++"
            else:
                print(
                    "\033[31m✗ No suitable compiler found. Please install g++ or clang++\033[0m"
                )
                return False

        print(f"\033[32m✓ Compiler: {self.compiler}\033[0m")
        return True

    def locate_source(self) -> bool:
        print_loading_animation("Locating source file", 0.6)

        possible_locations = [
            Path.cwd() / self.source_file,
            Path.cwd() / "src" / self.source_file,
            Path.cwd() / "source" / self.source_file,
            Path(__file__).parent / self.source_file,
            Path(__file__).parent / "src" / self.source_file,
        ]

        for loc in possible_locations:
            if loc.exists() and loc.is_file():
                self.source_path = loc
                print(f"\033[32m✓ Source found: {loc}\033[0m")
                return True

        print(f"\033[31m✗ Source file '{self.source_file}' not found\033[0m")
        return False

    def check_dependencies(self) -> bool:
        print_loading_animation("Checking dependencies", 0.8)

        deps = {"Windows": [], "Linux": ["libcrypto", "libssl"], "Darwin": []}

        if self.os_type == "Linux":
            missing = []
            for dep in deps["Linux"]:
                if not shutil.which(dep):
                    missing.append(dep)
            if missing:
                print(
                    f"\033[33m⚠ Missing optional dependencies: {', '.join(missing)}\033[0m"
                )
                print(f"\033[33m  Install with: sudo apt-get install libssl-dev\033[0m")

        print("\033[32m✓ Dependencies check complete\033[0m")
        return True

    def compile_program(self) -> bool:
        print_loading_animation("Compiling source code", 1.5)

        compile_flags = {
            "g++": "-O3 -std=c++11 -Wall -pthread",
            "clang++": "-O3 -std=c++11 -Wall -pthread",
            "cl": "/O2 /std:c++11 /EHsc",
        }

        flags = compile_flags.get(self.compiler, "-O3 -std=c++11 -Wall")

        if self.os_type == "Windows" and self.compiler == "g++":
            flags += " -static"
        elif self.os_type == "Windows" and self.compiler == "cl":
            output_file = f"{self.output_name}.exe"
        else:
            output_file = self.output_name

        if self.os_type == "Windows":
            output_file = f"{self.output_name}.exe"
        else:
            output_file = self.output_name

        cmd = [self.compiler, flags, str(self.source_path), "-o", output_file]
        cmd_str = " ".join(cmd)

        try:
            result = subprocess.run(
                cmd_str, shell=True, capture_output=True, text=True, timeout=60
            )
            if result.returncode == 0:
                self.output_path = Path.cwd() / output_file
                print(f"\033[32m✓ Compilation successful: {self.output_path}\033[0m")
                return True
            else:
                print(f"\033[31m✗ Compilation failed\033[0m")
                print(f"\033[33mError: {result.stderr}\033[0m")
                return False
        except subprocess.TimeoutExpired:
            print(f"\033[31m✗ Compilation timeout (60s)\033[0m")
            return False
        except Exception as e:
            print(f"\033[31m✗ Compilation error: {e}\033[0m")
            return False

    def install_binary(self) -> bool:
        if self.os_type == "Windows":
            install_dir = (
                Path(os.environ.get("PROGRAMFILES", "C:\\Program Files")) / "BankCrypto"
            )
        else:
            install_dir = Path("/usr/local/bin")

        try:
            install_dir.mkdir(parents=True, exist_ok=True)
            if self.os_type != "Windows":
                dest = install_dir / self.output_name
            else:
                dest = install_dir / f"{self.output_name}.exe"

            shutil.copy2(self.output_path, dest)

            if self.os_type != "Windows":
                dest.chmod(0o755)

            self.install_path = dest
            print(f"\033[32m✓ Installed to: {dest}\033[0m")
            return True
        except PermissionError:
            print(f"\033[33m⚠ Permission denied for system install\033[0m")
            install_dir = Path.cwd() / "bin"
            install_dir.mkdir(exist_ok=True)
            dest = install_dir / self.output_path.name
            shutil.copy2(self.output_path, dest)
            self.install_path = dest
            print(f"\033[32m✓ Installed to local: {dest}\033[0m")
            return True
        except Exception as e:
            print(f"\033[31m✗ Installation failed: {e}\033[0m")
            return False

    def run_program(self) -> None:
        if not self.install_path or not self.install_path.exists():
            if self.output_path and self.output_path.exists():
                prog = self.output_path
            else:
                prog = Path.cwd() / self.output_name
                if self.os_type == "Windows":
                    prog = Path.cwd() / f"{self.output_name}.exe"
        else:
            prog = self.install_path

        if not prog.exists():
            print(f"\033[31m✗ Program not found: {prog}\033[0m")
            return

        print(f"\n\033[36m{'='*60}\033[0m")
        print(f"\033[32mStarting Bank Cryptographic Module...\033[0m")
        print(f"\033[36m{'='*60}\033[0m\n")
        time.sleep(0.5)

        try:
            subprocess.run(str(prog), shell=True)
        except KeyboardInterrupt:
            print("\n\033[33m\nProgram interrupted by user\033[0m")
        except Exception as e:
            print(f"\033[31mError running program: {e}\033[0m")


class Menu:
    def __init__(self, installer: Installer):
        self.installer = installer
        self.commands = {
            "/install": self.install,
            "/i": self.install,
            "/run": self.run,
            "/r": self.run,
            "/compile": self.compile,
            "/c": self.compile,
            "/status": self.status,
            "/s": self.status,
            "/help": self.help,
            "/h": self.help,
            "/setup": self.full_setup,
            "/exit": self.exit,
            "/quit": self.exit,
            "/q": self.exit,
        }

    def clear_screen(self):
        if platform.system() == "Windows":
            os.system("cls")
        else:
            os.system("clear")

    def print_menu(self):
        self.clear_screen()
        print_ascii_header()
        print_art()

        print("\033[36m" + "=" * 60 + "\033[0m")
        print("\033[33mBANK CRYPTOGRAPHIC MODULE INSTALLER v3.0\033[0m")
        print("\033[36m" + "=" * 60 + "\033[0m")
        print()
        print("\033[32mAvailable Commands:\033[0m")
        print(
            "  \033[36m/install\033[0m   or \033[36m/i\033[0m   - Install the program"
        )
        print("  \033[36m/compile\033[0m   or \033[36m/c\033[0m   - Compile only")
        print("  \033[36m/run\033[0m      or \033[36m/r\033[0m   - Run the program")
        print(
            "  \033[36m/setup\033[0m           - Complete setup (detect + install + run)"
        )
        print(
            "  \033[36m/status\033[0m   or \033[36m/s\033[0m   - Check installation status"
        )
        print("  \033[36m/help\033[0m     or \033[36m/h\033[0m   - Show this menu")
        print("  \033[36m/exit\033[0m     or \033[36m/q\033[0m   - Exit installer")
        print()
        print("\033[33m" + "─" * 60 + "\033[0m")
        print()

    def install(self):
        print("\n\033[36mStarting installation...\033[0m\n")

        if not self.installer.detect_system():
            return False

        if not self.installer.locate_source():
            return False

        if not self.installer.check_dependencies():
            return False

        if not self.installer.compile_program():
            return False

        if not self.installer.install_binary():
            return False

        print("\n\033[32m✓ Installation completed successfully!\033[0m")
        return True

    def compile(self):
        print("\n\033[36mCompiling only...\033[0m\n")

        if not self.installer.locate_source():
            return False

        if not self.installer.compile_program():
            return False

        print("\n\033[32m✓ Compilation completed!\033[0m")
        return True

    def run(self):
        print("\n\033[36mRunning program...\033[0m\n")

        if not self.installer.output_path or not self.installer.output_path.exists():
            if not self.installer.locate_source():
                return False
            if not self.installer.compile_program():
                return False

        self.installer.run_program()
        return True

    def status(self):
        print("\n\033[36mInstallation Status:\033[0m\n")

        status_items = [
            ("System Type", self.installer.os_type),
            ("Compiler", self.installer.compiler or "Not detected"),
            (
                "Source File",
                (
                    self.installer.source_path.name
                    if self.installer.source_path
                    else "Not found"
                ),
            ),
            (
                "Binary",
                (
                    self.installer.output_path.name
                    if self.installer.output_path
                    and self.installer.output_path.exists()
                    else "Not compiled"
                ),
            ),
            (
                "Installed",
                (
                    self.installer.install_path.name
                    if self.installer.install_path
                    and self.installer.install_path.exists()
                    else "Not installed"
                ),
            ),
        ]

        for name, value in status_items:
            if "Not" in str(value):
                print(f"  \033[33m{name}: {value}\033[0m")
            else:
                print(f"  \033[32m{name}: {value}\033[0m")

        print()
        return True

    def full_setup(self):
        print("\n\033[36mStarting complete setup...\033[0m\n")
        print_loading_animation("Preparing environment", 0.5)

        if self.install():
            print_loading_animation("Launching program", 0.5)
            self.installer.run_program()
        else:
            print("\033[31mSetup failed. Please check errors above.\033[0m")

    def help(self):
        self.print_menu()
        return True

    def exit(self):
        print("\n\033[33mExiting installer...\033[0m")
        return False

    def run(self):
        running = True
        while running:
            try:
                self.print_menu()
                cmd = (
                    input("\033[36m┌─[\033[32mbank@installer\033[36m]\n└──╼ \033[0m")
                    .strip()
                    .lower()
                )

                if not cmd:
                    continue

                if cmd in self.commands:
                    result = self.commands[cmd]()
                    if result is False:
                        running = False
                    else:
                        input("\n\033[33mPress Enter to continue...\033[0m")
                else:
                    print(f"\033[31mUnknown command: {cmd}\033[0m")
                    input("\nPress Enter to continue...")

            except KeyboardInterrupt:
                print("\n\033[33m\nExiting...\033[0m")
                running = False
            except Exception as e:
                print(f"\033[31mError: {e}\033[0m")
                input("\nPress Enter to continue...")


def main():
    try:
        installer = Installer()
        menu = Menu(installer)

        if len(sys.argv) > 1:
            cmd = sys.argv[1].lower()
            commands = {
                "install": menu.install,
                "compile": menu.compile,
                "run": menu.run,
                "setup": menu.full_setup,
                "status": menu.status,
            }
            if cmd in commands:
                commands[cmd]()
            else:
                print(f"Unknown command: {cmd}")
                print("Usage: python installer.py [install|compile|run|setup|status]")
        else:
            menu.run()

    except Exception as e:
        print(f"\033[31mFatal error: {e}\033[0m")
        sys.exit(1)


if __name__ == "__main__":
    main()
