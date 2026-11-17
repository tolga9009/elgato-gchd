#!/usr/bin/env bash
set -euo pipefail
if (( $# < 1 || $# > 2 )); then
    echo "Usage: $0 <official.dmg|official.msi|official-url> [install-dir]" >&2; exit 2
fi
script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
manifest="$script_dir/../docs/firmware-hashes.json"
source_file=$1
destination=${2:-/usr/local/lib/firmware/gchd}
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
mac_url=https://edge.elgato.com/egc/macos/egcm/2.0.3/final/gchdm_203_970.dmg
win_url=https://edge.elgato.com/egc/windows/egcw/3.20/final/GameCaptureSetup_3.20.33.1533_x64.msi
case "$source_file" in
    "$mac_url"|"$win_url")
        curl --fail --location --proto '=https' --proto-redir '=https' --output "$work/${source_file##*/}" "$source_file"
        source_file="$work/${source_file##*/}" ;;
    *://*) echo 'Only the two documented official Elgato URLs are allowed.' >&2; exit 2 ;;
esac
installer_hash=$(sha256sum -- "$source_file"); installer_hash=${installer_hash%% *}
case "$installer_hash" in
    9eeb0bb7c00f29aa17a0bf4ff958f43e7ddae035b1390a003b0a43b6585c3542)
        if ! 7z x -y "-o$work/extracted" "$source_file" '*mb86*.bin' -r; then
            command -v dmg2img >/dev/null
            dmg2img "$source_file" "$work/firmware.img"
            7z x -y "-o$work/extracted" "$work/firmware.img" '*mb86*.bin' -r
        fi ;;
    c8f05d8b6203ace81ca76a62c9202663fd05f314eabbe4e44969e8b48a654c24)
        7z e -y "-o$work" "$source_file" x86_yPushFile3.dll
        7z e -y -r "-o$work/extracted" "$work/x86_yPushFile3.dll" MB86H57_H58_IDLE MB86H57_H58_ENC_H MB86M01_ASSP_NSEC_IDLE MB86M01_ASSP_NSEC_ENC_H ;;
    *) echo 'Installer SHA-256 does not match either documented official version.' >&2; exit 1 ;;
esac
python3 - "$manifest" "$work/extracted" "$work/verified" <<'PYVERIFY'
import sys, pathlib, json, hashlib
manifest, extracted, verified = map(pathlib.Path, sys.argv[1:])
records=json.loads(manifest.read_text())
names=sorted({r['name'] for r in records})
verified.mkdir()
for name in names:
    matches=[p for p in extracted.rglob('*') if p.is_file() and p.name.upper().removesuffix('.BIN')==name]
    if len(matches)!=1: raise SystemExit('Missing or ambiguous firmware: '+name)
    data=matches[0].read_bytes()
    if not any(r['name']==name and r['size']==len(data) and r['sha256']==hashlib.sha256(data).hexdigest() for r in records):
        raise SystemExit('Firmware verification failed: '+name)
    (verified/name).write_bytes(data)
PYVERIFY
install -d -- "$destination"
for name in MB86H57_H58_IDLE MB86H57_H58_ENC_H MB86M01_ASSP_NSEC_IDLE MB86M01_ASSP_NSEC_ENC_H; do
    install -m 0644 -- "$work/verified/$name" "$destination/$name"
done
echo "Verified four official firmware files and installed to $destination" >&2
