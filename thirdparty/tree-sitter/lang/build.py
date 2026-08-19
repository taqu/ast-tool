import os
import subprocess
import sys
from pathlib import Path

def build_cmake_directories(root_dir, build_type="Release"):
    root_path = Path(root_dir).resolve()
    print(f"[*] 走査を開始するルートディレクトリ: {root_path}")

    # ルート直下の子ディレクトリを走査
    for sub_dir in root_path.iterdir():
        if not sub_dir.is_dir():
            continue

        # サブディレクトリ内に CMakeLists.txt があるかチェック
        cmake_file = sub_dir / "CMakeLists.txt"
        if not cmake_file.exists():
            continue

        print(f"\n==============================================")
        print(f"[+] ターゲット検出: {sub_dir.name}")
        print(f"==============================================")

        # 各サブディレクトリ配下に個別のビルド用フォルダを作成
        build_dir = sub_dir / "build"
        build_dir.mkdir(parents=True, exist_ok=True)
        install_dir = root_path / "install"

        try:
            # 1. CMakeの構成 (Configure)
            print(f"[*] 構成中 (Configure)...")
            configure_cmd = [
                "cmake",
                f"-DCMAKE_BUILD_TYPE={build_type}",
                "-DBUILD_SHARED_LIBS=off",
                f"-DCMAKE_INSTALL_PREFIX={install_dir}",
                "-S",
                str(sub_dir),
                "-B",
                str(build_dir),
            ]
            subprocess.run(configure_cmd, check=True)

            # 2. CMakeのビルド (Build)
            print(f"[*] ビルド中 (Build)...")
            build_cmd = [
                "cmake",
                "--build",
                str(build_dir),
                "--config",
                build_type,
                "-j",
                str(os.cpu_count() or 1),  # 利用可能なCPUコアをフル活用
            ]
            subprocess.run(build_cmd, check=True)

            install_cmd = [
                "cmake",
                "--install",
                str(build_dir),
                "--config",
                build_type,
            ]
            subprocess.run(install_cmd, check=True)

            print(f"[✓] 成功: {sub_dir.name} のビルドが完了しました。")

        except subprocess.CalledProcessError as e:
            print(
                f"[✗] エラー: {sub_dir.name} のビルドプロセスで失敗しました。",
                file=sys.stderr,
            )
            continue


if __name__ == "__main__":
    build_cmake_directories(".", build_type="Release")

