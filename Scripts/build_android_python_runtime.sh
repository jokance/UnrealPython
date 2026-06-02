#!/usr/bin/env bash
set -euo pipefail

PYTHON_VERSION="${PYTHON_VERSION:-3.14.2}"
ANDROID_API_LEVEL="${ANDROID_API_LEVEL:-24}"
HOSTS=()
CLEAN=0

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_ROOT="$(cd "${PLUGIN_DIR}/../.." && pwd)"
WORK_DIR="${WORK_DIR:-${PROJECT_ROOT}/Intermediate/UnrealPython/PythonAndroidBuild}"
RUNTIME_ROOT="${PLUGIN_DIR}/ThirdParty/python314/android"
CONTENT_PYTHON_DIR="${PLUGIN_DIR}/Content/Python"

usage() {
	cat <<'EOF'
Usage:
  build_android_python_runtime.sh [options]

Options:
  --host HOST        Build one Android host triplet. May be repeated.
                     Defaults to aarch64-linux-android and x86_64-linux-android.
  --python VERSION  CPython version. Default: 3.14.5.
  --api LEVEL       Android API level. Default: 24.
  --work-dir DIR    Build workspace. Default: <project>/Intermediate/UnrealPython/PythonAndroidBuild.
  --clean           Remove the script-managed source/build directory before building.
  -h, --help        Show this help.

Required environment:
  ANDROID_HOME      Android SDK root. CPython's Android script installs needed SDK packages.
  JAVA_HOME         Optional if java is already on PATH.

This script follows CPython's Android build flow, then patches android.py to build
a static libpython because UnrealPython links libpython into the UE Android module.
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--host)
			HOSTS+=("${2:?--host requires a value}")
			shift 2
			;;
		--python)
			PYTHON_VERSION="${2:?--python requires a value}"
			shift 2
			;;
		--api)
			ANDROID_API_LEVEL="${2:?--api requires a value}"
			shift 2
			;;
		--work-dir)
			WORK_DIR="${2:?--work-dir requires a value}"
			shift 2
			;;
		--clean)
			CLEAN=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown argument: $1" >&2
			usage >&2
			exit 2
			;;
	esac
done

if [[ ${#HOSTS[@]} -eq 0 ]]; then
	HOSTS=("aarch64-linux-android" "x86_64-linux-android")
fi

require_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "Missing required command: $1" >&2
		exit 1
	fi
}

require_command curl
require_command tar
require_command python3
require_command make

if [[ -z "${ANDROID_HOME:-}" ]]; then
	echo "ANDROID_HOME must point at an Android SDK root." >&2
	exit 1
fi

mkdir -p "${WORK_DIR}"
WORK_DIR="$(cd "${WORK_DIR}" && pwd)"
PYTHON_SHORT_VERSION="${PYTHON_VERSION%.*}"
SOURCE_ARCHIVE="Python-${PYTHON_VERSION}.tgz"
SOURCE_URL="https://www.python.org/ftp/python/${PYTHON_VERSION}/${SOURCE_ARCHIVE}"
SOURCE_DIR="${WORK_DIR}/Python-${PYTHON_VERSION}"
DOWNLOAD_DIR="${WORK_DIR}/downloads"

prepare_source() {
	mkdir -p "${DOWNLOAD_DIR}"
	if [[ ${CLEAN} -eq 1 && -d "${SOURCE_DIR}" ]]; then
		case "$(cd "${SOURCE_DIR}/.." && pwd)/$(basename "${SOURCE_DIR}")" in
			"${WORK_DIR}"/*) rm -rf "${SOURCE_DIR}" ;;
			*) echo "Refusing to remove source outside work dir: ${SOURCE_DIR}" >&2; exit 1 ;;
		esac
	fi

	if [[ ! -f "${DOWNLOAD_DIR}/${SOURCE_ARCHIVE}" ]]; then
		curl -fL "${SOURCE_URL}" -o "${DOWNLOAD_DIR}/${SOURCE_ARCHIVE}"
	fi

	if [[ ! -d "${SOURCE_DIR}" ]]; then
		tar -xzf "${DOWNLOAD_DIR}/${SOURCE_ARCHIVE}" -C "${WORK_DIR}"
	fi
}

patch_static_libpython() {
	local android_py="${SOURCE_DIR}/Android/android.py"
	python3 - "$android_py" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text(encoding="utf-8")
old = '''        # Android always uses a shared libpython.
        "--enable-shared",
        "--without-static-libpython",
'''
new = '''        # UnrealPython embeds libpython into the UE Android module.
        "--disable-shared",
        "--with-static-libpython",
'''
if new in text:
    sys.exit(0)
if old not in text:
    raise SystemExit(f"Could not find CPython Android shared-libpython block in {path}")
path.write_text(text.replace(old, new), encoding="utf-8")
PY
}

copy_host_runtime() {
	local host="$1"
	local prefix="${SOURCE_DIR}/cross-build/${host}/prefix"
	local dest="${RUNTIME_ROOT}/${host}"

	if [[ ! -d "${prefix}/include/python${PYTHON_SHORT_VERSION}" ]]; then
		echo "Missing CPython Android include output: ${prefix}/include/python${PYTHON_SHORT_VERSION}" >&2
		exit 1
	fi

	local required_libs=(
		"libpython${PYTHON_SHORT_VERSION}.a"
		"libssl.a"
		"libcrypto.a"
		"libbz2.a"
		"libffi.a"
		"liblzma.a"
		"libzstd.a"
		"libmpdec.a"
	)
	for lib in "${required_libs[@]}"; do
		if [[ ! -f "${prefix}/lib/${lib}" ]]; then
			echo "Missing CPython Android library output: ${prefix}/lib/${lib}" >&2
			exit 1
		fi
	done

	mkdir -p "${dest}/include" "${dest}/lib"
	case "$(cd "${dest}/../.." && pwd)" in
		"$(cd "${PLUGIN_DIR}/ThirdParty/python314" && pwd)") ;;
		*) echo "Refusing to copy outside plugin runtime root: ${dest}" >&2; exit 1 ;;
	esac

	rm -rf "${dest}/include" "${dest}/lib"
	mkdir -p "${dest}/include" "${dest}/lib"
	cp -R "${prefix}/include/." "${dest}/include/"
	for lib in "${required_libs[@]}"; do
		cp "${prefix}/lib/${lib}" "${dest}/lib/${lib}"
	done
	# UE ships its own Android OpenSSL 1.1.1 in an earlier -L path. Link unique
	# archive names so CPython resolves against the bundled OpenSSL 3 build.
	cp "${dest}/lib/libssl.a" "${dest}/lib/libUnrealPythonSSL.a"
	cp "${dest}/lib/libcrypto.a" "${dest}/lib/libUnrealPythonCrypto.a"

	for optional_dir in engines-3 ossl-modules; do
		if [[ -d "${prefix}/lib/${optional_dir}" ]]; then
			cp -R "${prefix}/lib/${optional_dir}" "${dest}/lib/${optional_dir}"
		fi
	done
}

find_llvm_tool() {
	local tool="$1"
	local candidate

	if command -v "${tool}" >/dev/null 2>&1; then
		command -v "${tool}"
		return
	fi

	for candidate in "${ANDROID_HOME}"/ndk/*/toolchains/llvm/prebuilt/*/bin/"${tool}"; do
		if [[ -x "${candidate}" ]]; then
			echo "${candidate}"
			return
		fi
	done

	echo "Missing required LLVM tool from Android NDK: ${tool}" >&2
	exit 1
}

namespace_host_openssl_symbols() {
	local host="$1"
	local lib_dir="${RUNTIME_ROOT}/${host}/lib"
	local nm_tool="$2"
	local objcopy_tool="$3"
	local map_file="${WORK_DIR}/openssl-symbols-${host}.map"

	"${nm_tool}" --defined-only --format=posix \
		"${lib_dir}/libssl.a" \
		"${lib_dir}/libcrypto.a" |
		python3 -c 'import re, sys
symbols = set()
for line in sys.stdin:
    match = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s+[A-Za-z]\s+", line)
    if match:
        symbols.add(match.group(1))
for symbol in sorted(symbols):
    print(f"{symbol} UPY_OPENSSL3_{symbol}")' > "${map_file}"

	cp "${lib_dir}/libpython${PYTHON_SHORT_VERSION}.a" "${lib_dir}/libUnrealPython${PYTHON_SHORT_VERSION}.a"
	cp "${lib_dir}/libssl.a" "${lib_dir}/libUnrealPythonSSL.a"
	cp "${lib_dir}/libcrypto.a" "${lib_dir}/libUnrealPythonCrypto.a"

	"${objcopy_tool}" "--redefine-syms=${map_file}" "${lib_dir}/libUnrealPython${PYTHON_SHORT_VERSION}.a"
	"${objcopy_tool}" "--redefine-syms=${map_file}" "${lib_dir}/libUnrealPythonSSL.a"
	"${objcopy_tool}" "--redefine-syms=${map_file}" "${lib_dir}/libUnrealPythonCrypto.a"
}

copy_android_stdlib() {
	local host="$1"
	local prefix="${SOURCE_DIR}/cross-build/${host}/prefix"
	local stdlib_dir="${prefix}/lib/python${PYTHON_SHORT_VERSION}"
	local stdlib_dest="${RUNTIME_ROOT}/${host}/lib/python${PYTHON_SHORT_VERSION}"
	local support_module="${RUNTIME_ROOT}/_android_support.py"

	if [[ ! -d "${stdlib_dir}" ]]; then
		echo "Missing CPython Android stdlib output: ${stdlib_dir}" >&2
		exit 1
	fi
	if [[ ! -f "${stdlib_dir}/_android_support.py" ]]; then
		echo "Missing CPython Android support module: ${stdlib_dir}/_android_support.py" >&2
		exit 1
	fi

	mkdir -p "${RUNTIME_ROOT}"
	cp "${stdlib_dir}/_android_support.py" "${support_module}"
	python3 - "${support_module}" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text(encoding="utf-8")
old = "            fileno = original.fileno()\n"
new = """            try:
                fileno = original.fileno()
            except (AttributeError, OSError, io.UnsupportedOperation):
                fileno = None
"""
if new not in text:
    if old not in text:
        raise SystemExit(f"Could not find fileno block in {path}")
    path.write_text(text.replace(old, new), encoding="utf-8")
PY

	rm -rf "${stdlib_dest}"
	python3 - "${stdlib_dir}" "${stdlib_dest}" <<'PY'
from pathlib import Path
import shutil
import sys

src = Path(sys.argv[1])
dst = Path(sys.argv[2])
exclude_prefixes = ("lib-dynload/", "site-packages/")

for path in sorted(src.rglob("*")):
    if not path.is_file():
        continue
    rel = path.relative_to(src).as_posix()
    if rel == "_android_support.py":
        continue
    if rel.endswith(".pyc") or "/__pycache__/" in f"/{rel}":
        continue
    if rel.startswith(exclude_prefixes):
        continue
    target = dst / rel
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(path, target)
PY
}

prepare_source
patch_static_libpython

LLVM_NM="$(find_llvm_tool llvm-nm)"
LLVM_OBJCOPY="$(find_llvm_tool llvm-objcopy)"

export ANDROID_API_LEVEL

for host in "${HOSTS[@]}"; do
	echo "==> Building CPython ${PYTHON_VERSION} for ${host}"
	(
		cd "${SOURCE_DIR}"
		python3 Android/android.py build --cache-dir "${DOWNLOAD_DIR}" "${host}" -- -C
	)
	copy_host_runtime "${host}"
	namespace_host_openssl_symbols "${host}" "${LLVM_NM}" "${LLVM_OBJCOPY}"
	copy_android_stdlib "${host}"
done

echo "Android Python runtime copied to ${RUNTIME_ROOT}"
echo "Run: powershell -ExecutionPolicy Bypass -File \"${PLUGIN_DIR}/Scripts/verify_android_python_runtime.ps1\""
