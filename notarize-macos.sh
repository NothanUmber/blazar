#!/bin/bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: ./notarize-macos.sh PROFILE [SIGNED_DIR [OUTPUT_DIR]]
       ./notarize-macos.sh --resume PROFILE OUTPUT_DIR

Upload signed plugins to Apple using a notarytool Keychain profile, wait for
acceptance, staple and validate both bundles, then create Blazar-macOS.zip.

Defaults: SIGNED_DIR=<checkout>/build-macos/signed
          OUTPUT_DIR=<checkout>/build-macos/notarized
New submissions require an OUTPUT_DIR that does not exist. Logs and the uploaded
archive are retained there. If waiting times out (30 minutes), or stapling fails,
use --resume with the same output directory; it does not upload again.

One-time credentials setup (interactive):
  xcrun notarytool store-credentials blazar-notary
EOF
}

fail() { echo "Error: $*" >&2; exit 1; }

if [[ "${1:-}" == --help || "${1:-}" == -h ]]; then
    usage
    exit 0
fi
resume=false
if [[ "${1:-}" == --resume ]]; then
    resume=true
    shift
    [[ $# -eq 2 ]] || { usage >&2; exit 1; }
else
    [[ $# -ge 1 && $# -le 3 ]] || { usage >&2; exit 1; }
fi
[[ "$(uname -s)" == Darwin ]] || fail "This script requires macOS."
xcrun --find notarytool >/dev/null || fail "Install Xcode with notarytool support."
xcrun --find stapler >/dev/null || fail "Install Xcode with stapler support."

profile="$1"
[[ -n "$profile" ]] || fail "A notarytool Keychain profile is required."
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if "$resume"; then
    output_dir="$2"
else
    signed_dir="${2:-$project_dir/build-macos/signed}"
    [[ -d "$signed_dir" ]] || fail "Signed directory does not exist: $signed_dir"
    signed_dir="$(cd -- "$signed_dir" && pwd -P)"
    output_dir="${3:-$project_dir/build-macos/notarized}"
fi
[[ "$output_dir" == /* ]] || output_dir="$PWD/$output_dir"

developer_id_requirement='anchor apple generic and certificate 1[field.1.2.840.113635.100.6.2.6] exists and certificate leaf[field.1.2.840.113635.100.6.1.13] exists'
verify_plugins() {
    local name
    for name in Blazar.vst3 Blazar.component; do
        # The '=' prefix makes this literal requirement text, not a filename.
        codesign --verify --deep --strict --all-architectures \
            --test-requirement "=$developer_id_requirement" --verbose=2 "$1/$name"
    done
}

if "$resume"; then
    [[ -f "$output_dir/submission.plist" && -f "$output_dir/submission.zip" ]] ||
        fail "No saved submission in $output_dir"
else
    [[ ! -e "$output_dir" && ! -L "$output_dir" ]] || fail "Output already exists: $output_dir"
    verify_plugins "$signed_dir"
    mkdir -p "$(dirname -- "$output_dir")"
    output_dir="$(cd -- "$(dirname -- "$output_dir")" && pwd -P)/$(basename -- "$output_dir")"
    case "$output_dir/" in
        "$signed_dir/"*) fail "Output must be outside the signed directory." ;;
    esac
    mkdir "$output_dir"
    # Keep the exact uploaded archive for resuming; never read a newer build.
    ditto -c -k --sequesterRsrc "$signed_dir" "$output_dir/submission.zip"
    echo "Uploading plugins to Apple's notary service..."
    xcrun notarytool submit "$output_dir/submission.zip" \
        --keychain-profile "$profile" --output-format plist > "$output_dir/submission.plist"
fi

submission_id="$(/usr/libexec/PlistBuddy -c 'Print :id' "$output_dir/submission.plist")"
[[ -n "$submission_id" ]] || fail "The submission response contains no ID."
[[ ! -e "$output_dir/Blazar-macOS.zip" ]] || fail "The final ZIP already exists: $output_dir/Blazar-macOS.zip"
echo "Submission: $submission_id"
printf 'To resume: %q --resume %q %q\n' "$project_dir/notarize-macos.sh" "$profile" "$output_dir"
echo "Waiting up to 30 minutes for Apple (processing continues after a timeout)..."

# notarytool may return a nonzero exit status for a rejected submission. Read
# its structured response anyway so that we can retrieve Apple's diagnostic log.
wait_exit=0
xcrun notarytool wait "$submission_id" --keychain-profile "$profile" \
    --timeout 30m --output-format plist > "$output_dir/result.plist" || wait_exit=$?
status="$(/usr/libexec/PlistBuddy -c 'Print :status' "$output_dir/result.plist" 2>/dev/null || true)"
case "$status" in
    Accepted|Invalid|Rejected)
        xcrun notarytool log "$submission_id" --keychain-profile "$profile" \
            "$output_dir/notarization-log.json"
        echo "Apple's diagnostic log: $output_dir/notarization-log.json"
        ;;
    *)
        fail "Notarization has not completed (status: ${status:-unavailable}, exit: $wait_exit). Use the resume command above."
        ;;
esac
[[ "$status" == Accepted ]] || fail "Apple returned $status. See the diagnostic log; no release ZIP was created."

staging_dir="$(mktemp -d "$output_dir/stapling.XXXXXX")"
trap 'rm -rf -- "$staging_dir"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
ditto -x -k "$output_dir/submission.zip" "$staging_dir"
verify_plugins "$staging_dir"
for name in Blazar.vst3 Blazar.component; do
    xcrun stapler staple "$staging_dir/$name"
    xcrun stapler validate "$staging_dir/$name"
done
verify_plugins "$staging_dir"

# ZIP files cannot hold a stapled ticket themselves. Repackage the bundles
# after stapling, and expose the release filename only once packaging succeeds.
ditto -c -k --sequesterRsrc "$staging_dir" "$output_dir/Blazar-macOS.zip.tmp"
mv "$output_dir/Blazar-macOS.zip.tmp" "$output_dir/Blazar-macOS.zip"
echo "Notarized distribution: $output_dir/Blazar-macOS.zip"
