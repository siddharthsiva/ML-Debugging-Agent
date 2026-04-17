from __future__ import annotations

from typing import Any

from .analyzer import analyze_payload
from .knowledge_base import retrieve_relevant_docs


def parse_node(payload: dict[str, Any]) -> dict[str, Any]:
    analysis = analyze_payload(payload)
    search_query = " ".join(
        [
            analysis.get("root_cause", ""),
            analysis.get("summary", ""),
            analysis.get("stage", ""),
            analysis.get("likely_framework", ""),
        ]
        + [item.get("evidence", "") for item in analysis.get("symptoms", [])]
    )
    return {
        "analysis": analysis,
        "search_query": search_query,
    }


def retrieve_node(parsed: dict[str, Any]) -> dict[str, Any]:
    docs = retrieve_relevant_docs(parsed.get("search_query", ""), limit=4)
    parsed["retrieved_docs"] = docs
    return parsed


def advise_node(state: dict[str, Any]) -> dict[str, Any]:
    analysis = state["analysis"]
    docs = state.get("retrieved_docs", [])

    extra_steps: list[str] = []
    for doc in docs:
        for advice in doc.get("advice", []):
            if advice not in extra_steps:
                extra_steps.append(advice)

    action_plan: list[str] = []
    for item in list(analysis.get("suggested_fixes", [])) + extra_steps:
        if item not in action_plan:
            action_plan.append(item)

    analysis["next_steps"] = extra_steps[:5]
    analysis["action_plan"] = action_plan[:6]
    analysis["matched_documents"] = [doc.get("title", "") for doc in docs]
    analysis["retrieval_matches"] = len(docs)
    return analysis


def run_debugging_pipeline(payload: dict[str, Any]) -> dict[str, Any]:
    parsed = parse_node(payload)
    retrieved = retrieve_node(parsed)
    return advise_node(retrieved)
