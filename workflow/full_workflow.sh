#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_DIR=$(dirname "$SCRIPT_DIR")

declare -A ALLOWED_WORKFLOW_ARGS=(
    [--fresh]=1
)
TOTAL_WORKFLOW_ARGS=()

for arg in "$@"; do
    if [[ -v ALLOWED_WORKFLOW_ARGS["$arg"] ]]; then
        echo "Executing script with $arg arg"
        TOTAL_WORKFLOW_ARGS+=("$arg")
    else 
        echo "Unknown arg: $arg" >&2
        exit 1
    fi
done

echo "Project directory: $PROJECT_DIR"
echo "Script directory: $SCRIPT_DIR"

if [ "$(basename "$SCRIPT_DIR")" != "workflow" ] || [ ! -f "$PROJECT_DIR"/CMakePresets.json ]; then
    echo "FAIL: project directory must contains CMakePresets.json" >&2
    exit 1
fi

if [ -d "$SCRIPT_DIR/wf_logs" ]; then
    echo "NOTICE: Log directory is already exist"
fi

CURRENT_LOGS_DIR="wf_logs_$(date +%Y-%m-%d_%H-%M-%S)"
mkdir -p "$SCRIPT_DIR"/wf_logs/"$CURRENT_LOGS_DIR"
echo "Created current wf logs dir: $CURRENT_LOGS_DIR"

mapfile -t WF_PRESETS < <(
cmake "$SCRIPT_DIR"/../ --list-presets=workflow | grep -oP '(?<=")[^"]+(?=")'
)

if [ "${#WF_PRESETS[@]}" -eq 0 ]; then
    echo "FAIL: CMake presets list is empty. Unable to continue" >&2
    exit 1
fi

for i in "${!WF_PRESETS[@]}"; do
    echo "Preset #$((i + 1)): ${WF_PRESETS[$i]}"
done

success=0
total=0
TOTAL_FAILED_PRESETS=()

TOTAL_START=$SECONDS

cd "$PROJECT_DIR" || exit 1

for preset in "${WF_PRESETS[@]}"; do
    echo "Proceeding preset: $preset"
    mkdir -p "$SCRIPT_DIR/wf_logs/$CURRENT_LOGS_DIR/$preset"
    log_file="$SCRIPT_DIR/wf_logs/$CURRENT_LOGS_DIR/$preset/log_$preset.log"

    start=$SECONDS

    if cmake --workflow "${TOTAL_WORKFLOW_ARGS[@]}" --preset "$preset" > "$log_file" 2>&1; then
        elapsed=$((SECONDS - start))
        echo "OK: $preset (${elapsed}s)"
        ((++success))
    else
        rc=$?
        elapsed=$((SECONDS - start))
        echo "FAIL: $preset (exit code $rc, ${elapsed}s), see $log_file" >&2
        TOTAL_FAILED_PRESETS+=("${preset} ${log_file}")
    fi
    ((++total))
done

echo
echo "================ SUMMARY ================"
echo "Total:   $total"
echo "Success: $success"
echo "Failed:  ${#TOTAL_FAILED_PRESETS[@]}"

for failed_preset in "${TOTAL_FAILED_PRESETS[@]}"; do
    echo "$failed_preset"
done

total_time=$((SECONDS - TOTAL_START))
echo "Total time: ${total_time}s"

if [ ${#TOTAL_FAILED_PRESETS[@]} -ne 0 ]; then
    exit 1
else
    exit 0
fi
