import os
import sys
import argparse

def generate_cmake_file(directory_path, output_file):
    """
    指定されたディレクトリ内の静的・動的ライブラリを探し、
    CMake変数 ABSL_RELEASE_LIBS に格納する .cmake ファイルを出力します。
    """
    # 対象とするライブラリの拡張子 (.a や .so など)
    valid_extensions = ('.a', '.so', '.dylib', '.lib', '.dll')
    
    try:
        # ディレクトリ内のファイル一覧を取得してソート
        files = sorted(os.listdir(directory_path))
    except FileNotFoundError:
        print(f"エラー: ディレクトリ '{directory_path}' が見つかりません。", file=sys.stderr)
        return False
    except PermissionError:
        print(f"エラー: ディレクトリ '{directory_path}' の読み込み権限がありません。", file=sys.stderr)
        return False

    # ライブラリファイルのみを抽出
    lib_files = [f for f in files if f.endswith(valid_extensions)]

    # .cmake ファイルの出力処理
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write("set(ABSL_RELEASE_LIBS\n")
            for lib in lib_files:
                f.write(f'    "{lib}"\n')
            f.write(")\n")
        print(f"成功: '{output_file}' を生成しました。 (検出ライブラリ数: {len(lib_files)})")
        return True
    except IOError as e:
        print(f"エラー: ファイルの書き込みに失敗しました。 {e}", file=sys.stderr)
        return False

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ディレクトリ内のライブラリ一覧からCMake変数を生成するスクリプト")
    parser.add_argument("directory", help="スキャン対象のライブラリディレクトリパス")
    parser.add_argument("-o", "--output", default="absl_libs.cmake", help="出力する .cmake ファイル名 (デフォルト: absl_libs.cmake)")
    
    args = parser.parse_args()
    generate_cmake_file(args.directory, args.output)

