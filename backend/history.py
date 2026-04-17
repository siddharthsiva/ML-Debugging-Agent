from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from uuid import uuid4


HISTORY_DIR = Path(__file__).resolve().parents[1] / "data" / "history"
HISTORY_FILE = HISTORY_DIR / "analysis_history.jsonl"


def _read_entries() -> list[dict[str, Any]]:
    if not HISTORY_FILE.exists():
        return []

    entries: list[dict[str, Any]] = []
    for line in HISTORY_FILE.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            entries.append(json.loads(line))
        except json.JSONDecodeError:
            continue
    return entries


def list_history(limit: int = 20) -> list[dict[str, Any]]:
    entries = _read_entries()
    entries.sort(key=lambda item: item.get("timestamp", ""), reverse=True)
    return entries[:limit]


def save_analysis(payload: dict[str, Any], result: dict[str, Any]) -> dict[str, Any]:
    HISTORY_DIR.mkdir(parents=True, exist_ok=True)
    entry = {
        "id": str(uuid4()),
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "run_name": payload.get("run_name", "experiment-run"),
        "framework": result.get("likely_framework", payload.get("framework", "unknown")),
        "root_cause": result.get("root_cause", "unknown"),
        "severity": result.get("severity", "low"),
        "confidence": result.get("confidence", 0.0),
        "summary": result.get("summary", ""),
        "result": result,
    }
    with HISTORY_FILE.open("a", encoding="utf-8") as handle:
        handle.write(json.dumps(entry) + "\n")
    return entry


def compare_latest_runs(run_name: str = "") -> dict[str, Any]:
    entries = list_history(limit=100)
    if run_name:
        entries = [item for item in entries if item.get("run_name") == run_name]

    if len(entries) < 2:
        return {
            "available": False,
            "message": "Not enough history to compare runs yet.",
        }

    current = entries[0]
    previous = entries[1]
    return {
        "available": True,
        "current_id": current.get("id"),
        "previous_id": previous.get("id"),
        "current_root_cause": current.get("root_cause"),
        "previous_root_cause": previous.get("root_cause"),
        "root_cause_changed": current.get("root_cause") != previous.get("root_cause"),
        "confidence_delta": round(float(current.get("confidence", 0.0)) - float(previous.get("confidence", 0.0)), 2),
        "summary": f"Latest run moved from {previous.get('root_cause', 'unknown')} to {current.get('root_cause', 'unknown')}."
    }
