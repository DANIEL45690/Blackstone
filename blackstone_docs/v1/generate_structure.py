import os


def create_empty_structure():
    base_path = "blackstone_docs"

    folders = [f"{base_path}/assets/icons", f"{base_path}/assets/fonts"]

    files = [
        f"{base_path}/index.html",
        f"{base_path}/styles.css",
        f"{base_path}/script.js",
        f"{base_path}/generate_structure.py",
        f"{base_path}/README.md",
    ]

    for folder in folders:
        os.makedirs(folder, exist_ok=True)
        print(f"📁 Создана папка: {folder}")

    for file in files:
        with open(file, "w") as f:
            pass
        print(f"📄 Создан файл: {file}")

    print("\n✅ Готово! Структура создана в папке 'blackstone_docs'")
    print("\n📁 Структура:")
    print("blackstone_docs/")
    print("├── index.html")
    print("├── styles.css")
    print("├── script.js")
    print("├── generate_structure.py")
    print("├── README.md")
    print("└── assets/")
    print("    ├── icons/")
    print("    └── fonts/")


if __name__ == "__main__":
    create_empty_structure()
