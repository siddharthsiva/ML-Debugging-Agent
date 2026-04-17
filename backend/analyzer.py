from __future__ import annotations

from dataclasses import asdict, dataclass
import math
import re
from typing import Any


@dataclass
class Symptom:
    name: str
    severity: str
    evidence: str


@dataclass
class Diagnosis:
    run_name: str
    likely_framework: str
    root_cause: str
    stage: str
    severity: str
    confidence: float
    summary: str
    symptoms: list[Symptom]
    suggested_fixes: list[str]
    telemetry_to_collect: list[str]

    def to_dict(self) -> dict[str, Any]:
        return {
            "run_name": self.run_name,
            "likely_framework": self.likely_framework,
            "root_cause": self.root_cause,
            "stage": self.stage,
            "severity": self.severity,
            "confidence": self.confidence,
            "summary": self.summary,
            "symptoms": [asdict(item) for item in self.symptoms],
            "suggested_fixes": self.suggested_fixes,
            "telemetry_to_collect": self.telemetry_to_collect,
        }


def _safe_float_list(values: Any) -> list[float]:
    parsed: list[float] = []
    if isinstance(values, list):
        for item in values:
            try:
                parsed.append(float(item))
            except (TypeError, ValueError):
                continue
    return parsed


def parse_loss_text(loss_text: str) -> list[float]:
    numbers = re.findall(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?", loss_text or "")
    return [value for value in _safe_float_list(numbers) if not math.isnan(value)]


def _detect_framework(lower_log: str, config: dict[str, Any]) -> str:
    framework = str(config.get("framework", "")).lower()
    if framework:
        return framework
    if "tensorflow" in lower_log or "keras" in lower_log:
        return "tensorflow"
    if "pytorch" in lower_log or "torch" in lower_log or "cuda" in lower_log:
        return "pytorch"
    return "unknown"


def _detect_stage(lower_log: str) -> str:
    if re.search(r"dataloader|dataset|file not found|filenotfound|keyerror|num_workers|worker exited", lower_log):
        return "data_loading"
    if re.search(r"mat1 and mat2|shape mismatch|forward pass|embedding", lower_log):
        return "forward_pass"
    if re.search(r"backward|optimizer|grad_norm|overflow|nan", lower_log):
        return "optimization"
    if re.search(r"validation|val_loss|eval", lower_log):
        return "validation"
    return "training_loop"


def _detect_loss_instability(losses: list[float]) -> tuple[bool, str]:
    if len(losses) < 4:
        return False, "Not enough loss points for trend analysis."

    if any(math.isnan(v) or math.isinf(v) for v in losses):
        return True, "Loss contains NaN or infinite values."

    increases = sum(1 for i in range(1, len(losses)) if losses[i] > losses[i - 1])
    if losses[-1] > losses[0] * 1.2 and increases >= len(losses) // 2:
        return True, "Loss trend is increasing instead of settling."

    if max(losses) > min(losses) * 3:
        return True, "Loss fluctuates sharply across the run."

    return False, "Loss curve does not show strong instability."


def _detect_plateau(losses: list[float]) -> tuple[bool, str]:
    if len(losses) < 5:
        return False, "Not enough loss points to detect plateau."

    first = losses[0]
    last = losses[-1]
    if abs(first) < 1e-9:
        return False, "Initial loss too small for plateau check."

    improvement = abs(first - last) / abs(first)
    if improvement < 0.05:
        return True, "Loss changed by less than 5 percent across the run."

    return False, "Loss appears to be moving."


def _detect_overfitting(train_losses: list[float], val_losses: list[float]) -> tuple[bool, str]:
    if len(train_losses) < 4 or len(val_losses) < 4:
        return False, "Not enough paired train and validation points for overfitting analysis."

    train_improves = train_losses[-1] < train_losses[0] * 0.85
    val_worsens = val_losses[-1] > min(val_losses) * 1.08
    if train_improves and val_worsens:
        return True, "Training loss is improving while validation loss is getting worse."

    return False, "Train and validation curves do not show strong overfitting."


def analyze_payload(payload: dict[str, Any]) -> dict[str, Any]:
    run_name = str(payload.get("run_name", "experiment-run") or "experiment-run")
    log_text = str(payload.get("log_text", ""))
    config = payload.get("config", {}) or {}

    train_losses = _safe_float_list(payload.get("train_loss_values", []))
    if not train_losses:
        train_losses = _safe_float_list(payload.get("loss_values", []))
    if not train_losses:
        train_losses = parse_loss_text(str(payload.get("loss_text", "")))

    val_losses = _safe_float_list(payload.get("val_loss_values", []))
    if not val_losses:
        val_losses = parse_loss_text(str(payload.get("val_loss_text", "")))

    lower_log = log_text.lower()
    likely_framework = _detect_framework(lower_log, config)
    stage = _detect_stage(lower_log)

    symptoms: list[Symptom] = []
    suggested_fixes: list[str] = []
    telemetry_to_collect: list[str] = []
    scores: dict[str, float] = {
        "oom": 0.0,
        "shape_mismatch": 0.0,
        "nan_instability": 0.0,
        "learning_rate": 0.0,
        "stalled_training": 0.0,
        "overfitting": 0.0,
        "data_pipeline": 0.0,
    }

    if re.search(r"cuda out of memory|out of memory|oom", lower_log):
        scores["oom"] += 0.95
        symptoms.append(Symptom("memory pressure", "high", "Detected an out-of-memory error in the log."))
        suggested_fixes.extend([
            "Reduce batch size or sequence length.",
            "Enable mixed precision or gradient checkpointing.",
            "Free cached tensors and avoid storing full activations unnecessarily.",
        ])
        telemetry_to_collect.extend(["GPU memory usage by step", "peak batch memory", "activation sizes"])

    if re.search(r"size mismatch|shape mismatch|mat1 and mat2 shapes cannot be multiplied|expected size", lower_log):
        scores["shape_mismatch"] += 0.95
        symptoms.append(Symptom("tensor dimension mismatch", "high", "The stack trace suggests incompatible tensor shapes."))
        suggested_fixes.extend([
            "Print input and output tensor shapes around the failing layer.",
            "Verify feature dimensions after tokenization or reshaping.",
            "Check that the model head matches the dataset label format.",
        ])
        telemetry_to_collect.extend(["tensor shapes per layer", "batch schema sample", "model summary"])

    if "nan" in lower_log or "inf" in lower_log:
        scores["nan_instability"] += 0.8
        symptoms.append(Symptom("numerical instability", "high", "The log contains NaN or Inf values."))
        suggested_fixes.extend([
            "Lower the learning rate and add gradient clipping.",
            "Check input normalization and loss scaling.",
            "Verify that the loss function matches the output activation.",
        ])
        telemetry_to_collect.extend(["gradient norm history", "input statistics", "loss by batch"])

    if re.search(r"filenotfound|file not found|permission denied|keyerror|indexerror|dataloader worker", lower_log):
        scores["data_pipeline"] += 0.9
        symptoms.append(Symptom("data pipeline issue", "high", "The log points to missing files, bad keys, or dataloader worker errors."))
        suggested_fixes.extend([
            "Validate dataset paths and file permissions.",
            "Log a sample batch before training begins.",
            "Run the data loader with a single worker to isolate the failing example.",
        ])
        telemetry_to_collect.extend(["dataset manifest", "first batch preview", "worker exception trace"])

    unstable, unstable_reason = _detect_loss_instability(train_losses)
    if unstable:
        scores["learning_rate"] += 0.65
        symptoms.append(Symptom("unstable loss curve", "medium", unstable_reason))
        suggested_fixes.extend([
            "Reduce the learning rate or warm up gradually.",
            "Inspect optimizer settings such as weight decay and betas.",
        ])

    stalled, stalled_reason = _detect_plateau(train_losses)
    if stalled:
        scores["stalled_training"] += 0.6
        symptoms.append(Symptom("training plateau", "medium", stalled_reason))
        suggested_fixes.extend([
            "Try a larger model capacity or better input features.",
            "Tune learning rate, scheduler, or batch size.",
            "Check whether labels are noisy or nearly constant.",
        ])
        telemetry_to_collect.extend(["learning rate schedule", "class balance", "feature drift"])

    overfit, overfit_reason = _detect_overfitting(train_losses, val_losses)
    if overfit:
        scores["overfitting"] += 0.85
        symptoms.append(Symptom("generalization gap", "medium", overfit_reason))
        suggested_fixes.extend([
            "Add stronger regularization or early stopping.",
            "Increase augmentation or collect more data.",
            "Reduce model size or train fewer epochs.",
        ])
        telemetry_to_collect.extend(["train vs validation loss", "validation accuracy by epoch", "regularization settings"])

    lr = config.get("learning_rate") or config.get("lr")
    try:
        if lr is not None and float(lr) > 0.01:
            scores["learning_rate"] += 0.35
            symptoms.append(Symptom("aggressive learning rate", "medium", f"Configured learning rate appears high: {lr}"))
    except (TypeError, ValueError):
        pass

    ranked = sorted(scores.items(), key=lambda item: item[1], reverse=True)
    top_cause, top_score = ranked[0]

    if top_score <= 0.0:
        top_cause = "unknown"
        top_score = 0.35
        symptoms.append(Symptom("limited evidence", "low", "No strong known failure signature was detected."))
        suggested_fixes.extend([
            "Collect a longer stack trace and a few epochs of metrics.",
            "Log tensor shapes, gradient norms, and optimizer settings.",
            "Compare the failing run with the last known-good experiment config.",
        ])
        telemetry_to_collect.extend(["full stack trace", "environment info", "last known-good run config"])

    summaries = {
        "oom": "The run likely failed because the model exceeded available memory.",
        "shape_mismatch": "The run likely failed due to an inconsistency in tensor dimensions.",
        "nan_instability": "The run appears numerically unstable and is producing invalid values.",
        "learning_rate": "The training setup looks unstable, most likely due to optimization settings.",
        "stalled_training": "The model is not learning effectively and may be stuck on a plateau.",
        "overfitting": "The model is fitting the training data but failing to generalize to validation data.",
        "data_pipeline": "The failure most likely comes from input data loading or preprocessing.",
        "unknown": "The current evidence is inconclusive, but the run can be debugged with deeper instrumentation.",
    }

    severity_map = {
        "oom": "high",
        "shape_mismatch": "high",
        "nan_instability": "high",
        "data_pipeline": "high",
        "learning_rate": "medium",
        "stalled_training": "medium",
        "overfitting": "medium",
        "unknown": "low",
    }

    deduped_fixes: list[str] = []
    seen_fixes: set[str] = set()
    for item in suggested_fixes:
        if item not in seen_fixes:
            seen_fixes.add(item)
            deduped_fixes.append(item)

    deduped_telemetry: list[str] = []
    seen_telemetry: set[str] = set()
    for item in telemetry_to_collect:
        if item not in seen_telemetry:
            seen_telemetry.add(item)
            deduped_telemetry.append(item)

    result = Diagnosis(
        run_name=run_name,
        likely_framework=likely_framework,
        root_cause=top_cause,
        stage=stage,
        severity=severity_map[top_cause],
        confidence=round(min(top_score, 0.99), 2),
        summary=summaries[top_cause],
        symptoms=symptoms,
        suggested_fixes=deduped_fixes[:6],
        telemetry_to_collect=deduped_telemetry[:6],
    )
    output = result.to_dict()
    output["metrics"] = {
        "train_loss_points": len(train_losses),
        "val_loss_points": len(val_losses),
    }
    return output


if __name__ == "__main__":
    demo_payload = {
        "run_name": "demo-run",
        "log_text": "RuntimeError: CUDA out of memory. Tried to allocate 2.00 GiB",
        "config": {"learning_rate": 0.001, "batch_size": 64, "framework": "pytorch"},
        "loss_values": [2.4, 2.1, 2.0, 2.0, 2.0],
    }
    from pprint import pprint

    pprint(analyze_payload(demo_payload))
