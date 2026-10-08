#!/bin/bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: ./sign-macos.sh IDENTITY [BUILD_DIR [OUTPUT_DIR]]

Sign copies of the Release AU and VST3 bundles with a Developer ID Application
identity (certificate name or SHA-1 fingerprint) from your local Keychain.

Defaults: BUILD_DIR=<checkout>/build-macos, OUTPUT_DIR=<build-dir>/signed
OUTPUT_DIR must not exist. The original build products are left untouched.
List available identities with: security find-identity -v -p codesigning
Apple Development certificates cannot be used for this distribution workflow.
EOF
}

fail() { echo "Error: $*" >&2; exit 1; }

if [[ "${1:-}" == --help || "${1:-}" == -h ]]; then
    usage
    exit 0
fi
[[ $# -ge 1 && $# -le 3 ]] || { usage >&2; exit 1; }
[[ "$(uname -s)" == Darwin ]] || fail "This script requires macOS."

identity="$1"
identity_name="$identity"
if [[ "$identity" =~ ^[[:xdigit:]]{40}$ ]]; then
    # A fingerprint selects a certificate; it does not establish its type.
    identities="$(security find-identity -v -p codesigning)" ||
        fail "Could not list signing identities in Keychain."
    identity_name="$(awk -v fingerprint="$identity" '
        toupper($2) == toupper(fingerprint) {
            sub(/^[^"]*"/, ""); sub(/"[^"]*$/, ""); print; exit
        }
    ' <<< "$identities")"
    [[ -n "$identity_name" ]] || fail "No valid signing identity matches fingerprint $identity in Keychain."
fi
[[ "$identity_name" == 'Developer ID Application: '* ]] ||
    fail "Use a Developer ID Application certificate name or its SHA-1 fingerprint. Apple Development certificates cannot be used for distribution/notarization. See README.md for certificate setup."

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${2:-$project_dir/build-macos}"
[[ -d "$build_dir" ]] || fail "Build directory does not exist: $build_dir"
build_dir="$(cd -- "$build_dir" && pwd -P)"
output_dir="${3:-$build_dir/signed}"
[[ "$output_dir" == /* ]] || output_dir="$PWD/$output_dir"
[[ ! -e "$output_dir" && ! -L "$output_dir" ]] || fail "Output already exists: $output_dir"

bundles=(VST3/Blazar.vst3 AU/Blazar.component)
for relative_bundle in "${bundles[@]}"; do
    bundle="$build_dir/Blazar_artefacts/Release/$relative_bundle"
    [[ -f "$bundle/Contents/Info.plist" && -x "$bundle/Contents/MacOS/Blazar" ]] ||
        fail "Missing plugin bundle: $bundle. Run ./build-macos.sh first."
done

mkdir -p "$(dirname -- "$output_dir")"
output_dir="$(cd -- "$(dirname -- "$output_dir")" && pwd -P)/$(basename -- "$output_dir")"
for relative_bundle in "${bundles[@]}"; do
    source_bundle="$(cd -- "$build_dir/Blazar_artefacts/Release/$relative_bundle" && pwd -P)"
    case "$output_dir/" in
        "$source_bundle/"*) fail "Output must be outside the source plugin bundles." ;;
    esac
done
staging_dir="$(mktemp -d "${output_dir}.tmp.XXXXXX")"
trap 'rm -rf -- "$staging_dir"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

# Check the certificate type as well as signature integrity. This rejects
# ad-hoc, Apple Development and Developer ID Installer signatures.
developer_id_requirement='anchor apple generic and certificate 1[field.1.2.840.113635.100.6.2.6] exists and certificate leaf[field.1.2.840.113635.100.6.1.13] exists'
for relative_bundle in "${bundles[@]}"; do
    bundle="$staging_dir/${relative_bundle##*/}"
    ditto "$build_dir/Blazar_artefacts/Release/$relative_bundle" "$bundle"
    echo "Signing ${relative_bundle##*/}"
    # These bundles contain one executable each, with no nested code to sign.
    # Replacing the build's ad-hoc signature signs every architecture present.
    codesign --force --sign "$identity" --timestamp --options runtime "$bundle"
    # The '=' prefix makes this literal requirement text, not a filename.
    codesign --verify --deep --strict --all-architectures \
        --test-requirement "=$developer_id_requirement" --verbose=2 "$bundle"
done

# Carry the license texts and source/build information into the distribution.
for item in README.md LICENSE LICENSE.md NOTICE SOURCE.md third-party-notices; do
    ditto "$project_dir/$item" "$staging_dir/$item"
done

mv "$staging_dir" "$output_dir"
echo "Signed plugins: $output_dir"
echo "Next: run ./notarize-macos.sh with your notarytool Keychain profile."
