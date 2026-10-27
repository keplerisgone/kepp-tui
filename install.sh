#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
prefix=${KEPP_PREFIX:-"${HOME:-}/.local"}
build_dir=${KEPP_BUILD_DIR:-"$script_dir/build-release"}

usage() {
  cat <<'EOF'
Usage: ./install.sh [--prefix PATH] [--build-dir PATH]

Build KEPP TUI in Release mode and install the `kepp` executable.

Options:
  --prefix PATH       Installation prefix (default: $KEPP_PREFIX or ~/.local)
  --build-dir PATH    CMake build directory (default: $KEPP_BUILD_DIR or ./build-release)
  -h, --help          Show this help

The script does not use sudo, install system packages, or modify shell startup files.
EOF
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      [ "$#" -ge 2 ] || { echo "error: --prefix requires a path" >&2; exit 2; }
      prefix=$2
      shift 2
      ;;
    --prefix=*)
      prefix=${1#*=}
      shift
      ;;
    --build-dir)
      [ "$#" -ge 2 ] || { echo "error: --build-dir requires a path" >&2; exit 2; }
      build_dir=$2
      shift 2
      ;;
    --build-dir=*)
      build_dir=${1#*=}
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "error: unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [ -z "$prefix" ] || [ "$prefix" = "/.local" ]; then
  echo "error: HOME is not set; use --prefix PATH" >&2
  exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "error: CMake 3.20 or newer is required" >&2
  echo "  macOS:         brew install cmake" >&2
  echo "  Debian/Ubuntu: sudo apt install cmake build-essential" >&2
  exit 1
fi

echo "==> Configuring KEPP TUI (Release)"
cmake -S "$script_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release

echo "==> Building KEPP TUI"
cmake --build "$build_dir" --parallel

echo "==> Installing to $prefix"
cmake --install "$build_dir" --prefix "$prefix"

installed="$prefix/bin/kepp"
if [ ! -x "$installed" ]; then
  echo "error: installation did not create $installed" >&2
  exit 1
fi

echo "==> Installed $($installed --version) at $installed"
case ":${PATH:-}:" in
  *:"$prefix/bin":*)
    echo "Run: kepp"
    ;;
  *)
    echo
    echo "Add KEPP to PATH, then open a new shell:"
    case "${SHELL:-}" in
      */zsh)  echo "  echo 'export PATH=\"$prefix/bin:\$PATH\"' >> ~/.zshrc" ;;
      */bash) echo "  echo 'export PATH=\"$prefix/bin:\$PATH\"' >> ~/.bashrc" ;;
      *)      echo "  export PATH=\"$prefix/bin:\$PATH\"" ;;
    esac
    echo
    echo "Or run it now with: $installed"
    ;;
esac
