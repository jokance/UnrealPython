#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
PLATFORM="${PLATFORM:-Mac}"
CONFIGURATION="${CONFIGURATION:-Development}"
TARGET="${TARGET:-}"
ENGINE_DIR="${ENGINE_DIR:-${UE_ENGINE_DIR:-${UNREAL_ENGINE_DIR:-${EngineDir:-}}}}"

PROJECT_FILE="$(find "$PROJECT_ROOT" -maxdepth 1 -name '*.uproject' -print | head -n 1)"
if [[ -z "$PROJECT_FILE" ]]; then
    echo "No .uproject file found under '$PROJECT_ROOT'." >&2
    exit 1
fi

if [[ -z "$TARGET" ]]; then
    TARGET_FILE="$(find "$PROJECT_ROOT/Source" -name '*Editor.Target.cs' -print | head -n 1)"
    if [[ -z "$TARGET_FILE" ]]; then
        echo "No *Editor.Target.cs file found under '$PROJECT_ROOT/Source'. Set TARGET explicitly." >&2
        exit 1
    fi

    TARGET="$(basename "$TARGET_FILE" .Target.cs)"
fi

ENGINE_ASSOCIATION="$(grep -m 1 '"EngineAssociation"' "$PROJECT_FILE" | sed -E 's/.*"EngineAssociation"[[:space:]]*:[[:space:]]*"([^"]*)".*/\1/')"

if [[ -z "$ENGINE_DIR" ]]; then
    LAUNCHER_INSTALLED="$HOME/Library/Application Support/Epic/UnrealEngineLauncher/LauncherInstalled.dat"
    if [[ -f "$LAUNCHER_INSTALLED" ]]; then
        ENGINE_DIR="$(APP_NAME="UE_${ENGINE_ASSOCIATION}" perl -0777 -ne '
            my $app = $ENV{"APP_NAME"};
            while (/{(.*?)}/sg) {
                my $entry = $1;
                if ($entry =~ /"AppName"\s*:\s*"\Q$app\E"/ && $entry =~ /"InstallLocation"\s*:\s*"([^"]+)"/) {
                    print $1;
                    last;
                }
            }
        ' "$LAUNCHER_INSTALLED")"
    fi
fi

if [[ -z "$ENGINE_DIR" ]]; then
    for candidate in \
        "/Users/Shared/Epic Games/UE_${ENGINE_ASSOCIATION}/Engine" \
        "/Users/Shared/Epic Games/${ENGINE_ASSOCIATION}/Engine" \
        "/Applications/Epic Games/UE_${ENGINE_ASSOCIATION}/Engine" \
        "/Applications/Epic Games/${ENGINE_ASSOCIATION}/Engine"; do
        if [[ -d "$candidate" ]]; then
            ENGINE_DIR="$candidate"
            break
        fi
    done
fi

if [[ -z "$ENGINE_DIR" ]]; then
    echo "EngineDir was not found. Set UE_ENGINE_DIR, UNREAL_ENGINE_DIR, EngineDir, or pass ENGINE_DIR in the environment." >&2
    exit 1
fi

BUILD_SCRIPT="$ENGINE_DIR/Build/BatchFiles/Mac/Build.sh"
if [[ ! -x "$BUILD_SCRIPT" && -x "$ENGINE_DIR/Engine/Build/BatchFiles/Mac/Build.sh" ]]; then
    ENGINE_DIR="$ENGINE_DIR/Engine"
    BUILD_SCRIPT="$ENGINE_DIR/Build/BatchFiles/Mac/Build.sh"
fi

if [[ ! -x "$BUILD_SCRIPT" ]]; then
    echo "Build.sh was not found or is not executable at '$BUILD_SCRIPT'. EngineDir must point to the Unreal Engine/Engine directory." >&2
    exit 1
fi

export GENERATE_UNREAL_PYTHON=1

echo "Project: $PROJECT_FILE"
echo "Target: $TARGET"
echo "Platform: $PLATFORM"
echo "Configuration: $CONFIGURATION"
echo "EngineDir: $ENGINE_DIR"
echo "GENERATE_UNREAL_PYTHON=1"

cd "$PROJECT_ROOT"
"$BUILD_SCRIPT" "$TARGET" "$PLATFORM" "$CONFIGURATION" "$PROJECT_FILE" -NoHotReloadFromIDE -ForceHeaderGeneration -SkipBuild
