from __future__ import annotations

from typing import Any


def render_markdown_report(result: dict[str, Any]) -> str:
    run_name = result.get("run_name", "experiment-run")
    symptoms = result.get("symptoms", [])
    fixes = result.get("suggested_fixes", [])
    telemetry = result.get("telemetry_to_collect", [])
    matches = result.get("matched_documents", [])

    lines = [
        f"# Diagnostic Report: {run_name}",
        "",
        f"- Root cause: {result.get('root_cause', 'unknown')}",
        f"- Confidence: {result.get('confidence', 0.0)}",
        f"- Severity: {result.get('severity', 'low')}",
        f"- Stage: {result.get('stage', 'training_loop')}",
        f"- Framework: {result.get('likely_framework', 'unknown')}",
        "",
        "## Summary",
        result.get("summary", "No summary available."),
        "",
        "## Symptoms",
    ]

    if symptoms:
        for item in symptoms:
            lines.append(f"- {item.get('name', 'signal')}: {item.get('evidence', '')}")
    else:
        lines.append("- No strong symptoms were detected.")

    lines.extend(["", "## Suggested fixes"])
    for fix in fixes or ["Collect more telemetry and rerun the analyzer."]:
        lines.append(f"- {fix}")

    lines.extend(["", "## Telemetry to collect"])
    for item in telemetry or ["Full stack trace", "Loss and validation metrics"]:
        lines.append(f"- {item}")

    lines.extend(["", "## Matched references"])
    for item in matches or ["No reference documents matched."]:
        lines.append(f"- {item}")

    return "\n".join(lines)
