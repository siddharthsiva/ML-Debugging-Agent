from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .evaluator import evaluate
from .pipeline import run_debugging_pipeline
from .reporting import render_markdown_report

try:
    from fastapi import FastAPI
    from fastapi.middleware.cors import CORSMiddleware
    from fastapi.responses import PlainTextResponse
    from pydantic import BaseModel
except ImportError:  # Optional dependency for environments without FastAPI installed
    FastAPI = None
    CORSMiddleware = None
    PlainTextResponse = None
    BaseModel = object


if FastAPI:
    app = FastAPI(
        title="ML Experiment Debugging Agent",
        version="0.2.0",
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

    @app.get("/api/health")
    def health() -> dict[str, str]:
        return {"status": "ok", "service": "ml-experiment-debugging-agent"}

    @app.get("/api/cases")
    def cases() -> list[dict[str, Any]]:
        return _load_cases()

    @app.get("/api/evaluate")
    def evaluate_cases() -> dict[str, Any]:
        return evaluate()

    @app.post("/api/analyze")
    def analyze(request: AnalyzeRequest) -> dict[str, Any]:
        payload = request.model_dump()
        payload["config"] = {**payload.get("config", {}), "framework": request.framework, "model_name": request.model_name}
        return run_debugging_pipeline(payload)

    @app.post("/api/report-markdown")
    def export_markdown(request: AnalyzeRequest):
        payload = request.model_dump()
        payload["config"] = {**payload.get("config", {}), "framework": request.framework, "model_name": request.model_name}
        result = run_debugging_pipeline(payload)
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
