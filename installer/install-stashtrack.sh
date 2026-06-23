#!/usr/bin/env bash
set -euo pipefail

CONFIGURATION="Release"
BUILD_DIR=""
TOOLS_DIR=""
SYSTEM_INSTALL=0
DRY_RUN=0

usage() {
    cat <<'USAGE'
Usage: installer/install-stashtrack.sh [options]

Options:
  --configuration NAME   Build configuration to install (default: Release)
  --build-dir PATH      CMake build directory (default: ./build)
  --tools-dir PATH      Optional folder containing ffmpeg to copy beside the plug-in binary
  --system              Install to the system VST3 folder instead of the current user's folder
  --dry-run             Print planned actions without copying files
  -h, --help            Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --configuration)
            CONFIGURATION="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --tools-dir)
            TOOLS_DIR="$2"
            shift 2
            ;;
        --system)
            SYSTEM_INSTALL=1
            shift
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

if [[ -z "$BUILD_DIR" ]]; then
    BUILD_DIR="$ROOT_DIR/build"
fi

case "$(uname -s)" in
    Darwin)
        if [[ "$SYSTEM_INSTALL" -eq 1 ]]; then
            DEST_ROOT="/Library/Audio/Plug-Ins/VST3"
        else
            DEST_ROOT="$HOME/Library/Audio/Plug-Ins/VST3"
        fi
        BINARY_SUBDIR="Contents/MacOS"
        ;;
    Linux)
        if [[ "$SYSTEM_INSTALL" -eq 1 ]]; then
            DEST_ROOT="/usr/local/lib/vst3"
        else
            DEST_ROOT="$HOME/.vst3"
        fi
        BINARY_SUBDIR="Contents/x86_64-linux"
        ;;
    *)
        echo "Unsupported platform for this installer script." >&2
        exit 2
        ;;
esac

SOURCE="$BUILD_DIR/StashTrack_artefacts/$CONFIGURATION/VST3/StashTrack.vst3"
DESTINATION="$DEST_ROOT/StashTrack.vst3"
BINARY_FOLDER="$DESTINATION/$BINARY_SUBDIR"

echo "StashTrack VST3 installer"
echo "Source:      $SOURCE"
echo "Destination: $DESTINATION"

if [[ "$DRY_RUN" -eq 1 ]]; then
    echo "[dry-run] Would create $DEST_ROOT"
    echo "[dry-run] Would copy StashTrack.vst3 to $DESTINATION"
    echo "[dry-run] Would verify uv and uvx availability for uv-managed yt-dlp"
    echo "[dry-run] Would install uv into the plug-in bundle if uv is missing"

    if [[ -n "$TOOLS_DIR" ]]; then
        echo "[dry-run] Would copy ffmpeg from $TOOLS_DIR to $BINARY_FOLDER when present"
    fi

    exit 0
fi

if [[ ! -e "$SOURCE" ]]; then
    echo "Built plug-in was not found at $SOURCE. Build StashTrack_VST3 first." >&2
    exit 1
fi

mkdir -p "$DEST_ROOT"
rm -rf "$DESTINATION"
cp -R "$SOURCE" "$DESTINATION"

ensure_uv_in_bundle() {
    mkdir -p "$BINARY_FOLDER"

    local uv_path=""
    local uvx_path=""

    if command -v uv >/dev/null 2>&1; then
        uv_path="$(command -v uv)"
    fi

    if command -v uvx >/dev/null 2>&1; then
        uvx_path="$(command -v uvx)"
    fi

    if [[ -n "$uv_path" && -n "$uvx_path" ]]; then
        cp "$uv_path" "$BINARY_FOLDER/uv"
        cp "$uvx_path" "$BINARY_FOLDER/uvx"
        chmod +x "$BINARY_FOLDER/uv" "$BINARY_FOLDER/uvx"
        echo "Copied uv and uvx into the VST3 bundle."
        return
    fi

    echo "uv was not found. Downloading and installing uv into the StashTrack VST3 bundle..."

    if command -v curl >/dev/null 2>&1; then
        curl -LsSf https://astral.sh/uv/install.sh | UV_UNMANAGED_INSTALL="$BINARY_FOLDER" sh
    elif command -v wget >/dev/null 2>&1; then
        wget -qO- https://astral.sh/uv/install.sh | UV_UNMANAGED_INSTALL="$BINARY_FOLDER" sh
    else
        echo "Neither curl nor wget is available to install uv." >&2
        exit 1
    fi

    if [[ ! -x "$BINARY_FOLDER/uv" || ! -x "$BINARY_FOLDER/uvx" ]]; then
        echo "uv installer completed, but uv or uvx was not found in $BINARY_FOLDER." >&2
        exit 1
    fi
}

ensure_uv_in_bundle

if [[ -n "$TOOLS_DIR" ]]; then
    if [[ ! -d "$TOOLS_DIR" ]]; then
        echo "Tools directory was not found: $TOOLS_DIR" >&2
        exit 1
    fi

    mkdir -p "$BINARY_FOLDER"

    for tool in ffmpeg; do
        if [[ -f "$TOOLS_DIR/$tool" ]]; then
            cp "$TOOLS_DIR/$tool" "$BINARY_FOLDER/$tool"
            chmod +x "$BINARY_FOLDER/$tool"
            echo "Copied $tool"
        else
            echo "Skipped $tool because it was not found in $TOOLS_DIR"
        fi
    done
fi

echo "Installed StashTrack to $DESTINATION"
