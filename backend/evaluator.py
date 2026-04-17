from __future__ import annotations

import json
from pathlib import Path

from .pipeline import run_debugging_pipeline


def evaluate() -> dict:
    data_path = Path(__file__).resolve().parents[1] / "data" / "test_cases.json"
    test_cases = json.loads(data_path.read_text(encoding="utf-8"))

    total = 0
    correct = 0
    details = []
    category_totals: dict[str, int] = {}
    category_correct: dict[str, int] = {}
    confusion: dict[str, dict[str, int]] = {}

    for case in test_cases:
        total += 1
        expected = case["expected_root_cause"]
        result = run_debugging_pipeline(case["payload"])
        actual = result["root_cause"]
        ok = expected == actual
        if ok:
            correct += 1

        category_totals[expected] = category_totals.get(expected, 0) + 1
        if ok:
            category_correct[expected] = category_correct.get(expected, 0) + 1

        confusion.setdefault(expected, {})
        confusion[expected][actual] = confusion[expected].get(actual, 0) + 1

        details.append({
            "name": case["name"],
            "expected": expected,
            "actual": actual,
            "confidence": result.get("confidence", 0.0),
            "correct": ok,
        })

    category_accuracy = {
        category: {
            "correct": category_correct.get(category, 0),
            "total": count,
            "accuracy": round(category_correct.get(category, 0) / count, 2) if count else 0.0,
        }
        for category, count in category_totals.items()
    }

    accuracy = (correct / total) if total else 0.0
    return {
        "total_cases": total,
        "correct": correct,
        "accuracy": round(accuracy, 2),
        "pass_rate": f"{correct}/{total}",
        "category_accuracy": category_accuracy,
        "confusion_matrix": confusion,
        "details": details,
    }


if __name__ == "__main__":
    from pprint import pprint

    pprint(evaluate())
