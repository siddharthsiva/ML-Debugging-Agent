from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .evaluator import evaluate
from .file_parsing import parse_config_text, parse_loss_file_text
from .history import compare_latest_runs, list_history, save_analysis
from .knowledge_base import get_retrieval_backend_name
from .pipeline import run_debugging_pipeline
from .reporting import render_markdown_report

try:
    from fastapi import FastAPI, File, Form, UploadFile
    from fastapi.middleware.cors import CORSMiddleware
    from fastapi.responses import PlainTextResponse
    from pydantic import BaseModel
except ImportError:  # Optional dependency for environments without FastAPI installed
    FastAPI = None
    File = None
    Form = None
    UploadFile = None
    CORSMiddleware = None
    PlainTextResponse = None
    BaseModel = object


if FastAPI:
    app = FastAPI(
        title="ML Experiment Debugging Agent",
        version="0.3.0",
        description="Analyze failed ML training runs from logs, configs, and curves to identify likely root causes and recommend fixes.",
    )
    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"],
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

    class AnalyzeRequest(BaseModel):
        run_name: str = "experiment-run"
        framework: str = "pytorch"
        model_name: str = ""
        log_text: str = ""
        config: dict[str, Any] = {}
        loss_values: list[float] = []
        train_loss_values: list[float] = []
        val_loss_values: list[float] = []
        loss_text: str = ""
        val_loss_text: str = ""

    def _load_cases() -> list[dict[str, Any]]:
        data_path = Path(__file__).resolve().parents[1] / "data" / "test_cases.json"
        return json.loads(data_path.read_text(encoding="utf-8"))

    def _prepare_payload(payload: dict[str, Any]) -> dict[str, Any]:
        config = payload.get("config", {}) or {}
        payload["config"] = {
            **config,
            "framework": payload.get("framework", config.get("framework", "pytorch")),
            "model_name": payload.get("model_name", config.get("model_name", "")),
        }
        return payload

    def _run_and_persist(payload: dict[str, Any]) -> dict[str, Any]:
        prepared = _prepare_payload(payload)
        result = run_debugging_pipeline(prepared)
        saved = save_analysis(prepared, result)
        result["history_id"] = saved["id"]
        result["saved_at"] = saved["timestamp"]
        result["retrieval_backend"] = get_retrieval_backend_name()
        return result

    @app.get("/api/health")
    def health() -> dict[str, str]:
        return {
            "status": "ok",
            "service": "ml-experiment-debugging-agent",
            "retrieval_backend": get_retrieval_backend_name(),
        }

    @app.get("/api/cases")
    def cases() -> list[dict[str, Any]]:
        return _load_cases()

    @app.get("/api/evaluate")
    def evaluate_cases() -> dict[str, Any]:
        return evaluate()

    @app.get("/api/history")
    def history(limit: int = 20) -> list[dict[str, Any]]:
        return list_history(limit=limit)

    @app.get("/api/history/compare-latest")
    def compare_history(run_name: str = "") -> dict[str, Any]:
        return compare_latest_runs(run_name=run_name)

    @app.post("/api/analyze")
    def analyze(request: AnalyzeRequest) -> dict[str, Any]:
        return _run_and_persist(request.model_dump())

    @app.post("/api/analyze-upload")
    async def analyze_upload(
        run_name: str = Form("uploaded-run"),
        framework: str = Form("pytorch"),
        model_name: str = Form(""),
        log_file: UploadFile | None = File(default=None),
        config_file: UploadFile | None = File(default=None),
        train_loss_file: UploadFile | None = File(default=None),
        val_loss_file: UploadFile | None = File(default=None),
    ) -> dict[str, Any]:
        log_text = (await log_file.read()).decode("utf-8", errors="ignore") if log_file else ""
        config_text = (await config_file.read()).decode("utf-8", errors="ignore") if config_file else ""
        train_loss_text = (await train_loss_file.read()).decode("utf-8", errors="ignore") if train_loss_file else ""
        val_loss_text = (await val_loss_file.read()).decode("utf-8", errors="ignore") if val_loss_file else ""

        payload = {
            "run_name": run_name,
            "framework": framework,
            "model_name": model_name,
            "log_text": log_text,
            "config": parse_config_text(config_text),
            "train_loss_values": parse_loss_file_text(train_loss_text),
            "val_loss_values": parse_loss_file_text(val_loss_text),
            "loss_text": train_loss_text,
            "val_loss_text": val_loss_text,
        }
        return _run_and_persist(payload)

    @app.post("/api/report-markdown")
    def export_markdown(request: AnalyzeRequest):
        result = _run_and_persist(request.model_dump())
        report = render_markdown_report(result)
        return PlainTextResponse(report)

else:
    app = None

    if __name__ == "__main__":
        sample = {
            "run_name": "demo-run",
            "framework": "pytorch",
            "model_name": "resnet18",
            "log_text": "RuntimeError: mat1 and mat2 shapes cannot be multiplied (32x512 and 768x10)",
            "config": {"lr": 0.0005, "batch_size": 32},
            "loss_values": [1.8, 1.7, 1.65, 1.62],
        }
        print("FastAPI is not installed in this environment. Showing demo analysis instead.\n")
        print(json.dumps(run_debugging_pipeline(sample), indent=2))
