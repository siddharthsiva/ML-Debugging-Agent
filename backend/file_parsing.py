from __future__ import annotations

import csv
import io
import json
import re
from typing import Any


def parse_config_text(raw_text: str) -> dict[str, Any]:
    raw_text = (raw_text or "").strip()
    if not raw_text:
        return {}

    try:
        return json.loads(raw_text)
    except json.JSONDecodeError:
        return {"raw_config": raw_text}


def parse_numeric_series(raw_text: str) -> list[float]:
    numbers = re.findall(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?", raw_text or "")
    parsed: list[float] = []
    for item in numbers:
        try:
            parsed.append(float(item))
        except ValueError:
            continue
    return parsed


def parse_loss_file_text(raw_text: str) -> list[float]:
    if not raw_text.strip():
        return []

    parsed = parse_numeric_series(raw_text)
    if parsed:
        return parsed

    reader = csv.reader(io.StringIO(raw_text))
    values: list[float] = []
    for row in reader:
        for cell in row:
            try:
                values.append(float(cell))
            except ValueError:
                continue
    return values
