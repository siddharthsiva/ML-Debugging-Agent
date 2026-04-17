from __future__ import annotations

from dataclasses import dataclass, asdict
import json
from math import sqrt
from pathlib import Path
import re
from typing import Any

try:
    from sentence_transformers import SentenceTransformer
except ImportError:
    SentenceTransformer = None


@dataclass
class FailureDocument:
    title: str
    tags: list[str]
    symptoms: list[str]
    advice: list[str]

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


_EMBED_MODEL = None


def _default_docs() -> list[FailureDocument]:
    return [
        FailureDocument(
            title="CUDA out of memory",
            tags=["oom", "memory", "gpu"],
            symptoms=["cuda out of memory", "allocation failure", "large batch size"],
            advice=[
                "Reduce batch size or sequence length.",
                "Use mixed precision if supported.",
                "Enable gradient checkpointing for large models.",
            ],
        ),
        FailureDocument(
            title="Tensor shape mismatch",
            tags=["shape", "dimension", "matmul"],
            symptoms=["size mismatch", "shapes cannot be multiplied", "expected size"],
            advice=[
                "Print tensor shapes around the failing layer.",
                "Verify tokenization, flattening, and projection dimensions.",
                "Check classifier output size against label count.",
            ],
        ),
        FailureDocument(
            title="Exploding or NaN loss",
            tags=["nan", "instability", "gradients"],
            symptoms=["nan", "inf", "loss spikes"],
            advice=[
                "Lower the learning rate and clip gradients.",
                "Check input normalization and label ranges.",
                "Inspect the loss function and activation pairing.",
            ],
        ),
        FailureDocument(
            title="Training plateau",
            tags=["plateau", "underfitting", "stalled"],
            symptoms=["flat loss", "no improvement", "stagnation"],
            advice=[
                "Tune learning rate and scheduler settings.",
                "Increase model capacity or improve input features.",
                "Verify dataset balance and label quality.",
            ],
        ),
    ]


def _doc_to_text(doc: FailureDocument) -> str:
    return " ".join([doc.title] + doc.tags + doc.symptoms + doc.advice)


def _get_embedding_model():
    global _EMBED_MODEL
    if _EMBED_MODEL is None and SentenceTransformer is not None:
        try:
            _EMBED_MODEL = SentenceTransformer("all-MiniLM-L6-v2")
        except Exception:
            _EMBED_MODEL = False
    return _EMBED_MODEL if _EMBED_MODEL not in (None, False) else None


def get_retrieval_backend_name() -> str:
    return "sentence-transformers" if _get_embedding_model() is not None else "keyword-fallback"


def load_failure_docs() -> list[FailureDocument]:
    data_path = Path(__file__).resolve().parents[1] / "data" / "failure_modes.json"
    if not data_path.exists():
        return _default_docs()

    try:
        raw = json.loads(data_path.read_text(encoding="utf-8"))
        docs: list[FailureDocument] = []
        for item in raw:
            docs.append(
                FailureDocument(
                    title=item.get("title", "Untitled"),
                    tags=list(item.get("tags", [])),
                    symptoms=list(item.get("symptoms", [])),
                    advice=list(item.get("advice", [])),
                )
            )
        return docs or _default_docs()
    except Exception:
        return _default_docs()


def _score_document_keyword(query: str, doc: FailureDocument) -> int:
    query_lower = query.lower()
    query_tokens = set(re.findall(r"[a-z0-9_]+", query_lower))
    score = 0

    for token in doc.tags:
        token_lower = token.lower()
        if token_lower in query_lower or token_lower in query_tokens:
            score += 3

    for symptom in doc.symptoms:
        symptom_lower = symptom.lower()
        if symptom_lower in query_lower:
            score += 2
        else:
            symptom_tokens = set(re.findall(r"[a-z0-9_]+", symptom_lower))
            score += len(query_tokens.intersection(symptom_tokens))

    for advice in doc.advice:
        advice_tokens = set(re.findall(r"[a-z0-9_]+", advice.lower()))
        if query_tokens.intersection(advice_tokens):
            score += 1

    return score


def _cosine_similarity(a: list[float], b: list[float]) -> float:
    dot = sum(x * y for x, y in zip(a, b))
    norm_a = sqrt(sum(x * x for x in a))
    norm_b = sqrt(sum(y * y for y in b))
    if norm_a == 0 or norm_b == 0:
        return 0.0
    return dot / (norm_a * norm_b)


def _retrieve_with_embeddings(query: str, docs: list[FailureDocument], limit: int) -> list[dict[str, Any]]:
    model = _get_embedding_model()
    if model is None:
        return []

    try:
        texts = [_doc_to_text(doc) for doc in docs]
        query_vector = model.encode([query], normalize_embeddings=False)[0]
        doc_vectors = model.encode(texts, normalize_embeddings=False)
        scored: list[tuple[float, FailureDocument]] = []
        for vector, doc in zip(doc_vectors, docs):
            score = _cosine_similarity(list(query_vector), list(vector))
            scored.append((score, doc))
        scored.sort(key=lambda item: item[0], reverse=True)

        output: list[dict[str, Any]] = []
        for score, doc in scored[:limit]:
            item = doc.to_dict()
            item["score"] = round(float(score), 4)
            item["retrieval_mode"] = "embedding"
            output.append(item)
        return output
    except Exception:
        return []


def retrieve_relevant_docs(query: str, limit: int = 3) -> list[dict[str, Any]]:
    docs = load_failure_docs()
    embedded = _retrieve_with_embeddings(query, docs, limit)
    if embedded:
        return embedded

    results: list[tuple[int, FailureDocument]] = []
    for doc in docs:
        score = _score_document_keyword(query, doc)
        if score > 0:
            results.append((score, doc))

    results.sort(key=lambda item: item[0], reverse=True)
    output: list[dict[str, Any]] = []
    for score, doc in results[:limit]:
        item = doc.to_dict()
        item["score"] = score
        item["retrieval_mode"] = "keyword"
        output.append(item)
    return output
