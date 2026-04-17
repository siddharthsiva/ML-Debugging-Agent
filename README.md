# ML Experiment Debugging Agent

ML Experiment Debugging Agent is a debugging tool for failed PyTorch and deep learning training runs. It analyzes logs, configs, and loss curves to identify likely root causes such as memory pressure, tensor shape mismatches, unstable optimization, and stalled learning, then returns a structured report with concrete fixes.

## Why this is a strong portfolio project

It demonstrates:
- practical ML systems debugging
- API and backend engineering
- retrieval-backed recommendations
- user-facing diagnostic workflows
- clear project scope that is easy to explain in interviews

## What problem it solves

Training failures waste time during experimentation. This project reduces debugging friction by turning raw logs and metrics into an actionable diagnosis that helps an engineer move from failure to fix faster.

## Architecture

The current MVP uses a simple three-stage pipeline:
1. Parse symptoms from logs, configs, and metrics using Python rules and lightweight heuristics
2. Retrieve relevant debugging notes from a small failure-mode knowledge base
3. Advise the user with a ranked root cause, confidence score, and recommended next steps

## Current features

- Runs a Parse → Retrieve → Advise debugging pipeline
- Detects common failure modes:
  - CUDA out of memory
  - tensor shape mismatch
  - NaN or exploding loss
  - unstable learning rate
  - flat or stalled training
- Returns a root cause, confidence score, symptom list, matched references, and suggested fixes
- Includes a browser-based interface for interactive testing
- Includes a small evaluation harness over curated failure cases

## Engineering features

- structured FastAPI endpoints for analysis, upload-based artifact ingestion, evaluation, history lookup, and markdown report export
- curated failure-mode library, saved analysis history, and sample-case evaluation harness with per-category accuracy and confusion-style output
- browser-based diagnostic UI with sample scenarios, loss-curve visualization, JSON export, report download, artifact upload, and recent-run comparison
- keyword retrieval that is ready to switch to sentence-transformer embeddings when optional dependencies are installed
- containerization support through the root Dockerfile, deployment config in [render.yaml](render.yaml), and CI through the GitHub Actions workflow

## Recommended polish for the final portfolio version

- deploy the FastAPI service and host the demo UI publicly
- add 20 to 50 curated failure cases with screenshots of reports
- integrate experiment trackers such as Weights and Biases or MLflow
- swap the retrieval layer fully to embeddings and local vector search
- add authentication and multi-user saved workspaces if you want to turn it into a more product-like app

## Project structure

- backend/app.py: API entry point
- backend/analyzer.py: rule-based diagnosis engine
- backend/pipeline.py: Parse → Retrieve → Advise flow
- backend/evaluator.py: simple evaluation runner over curated failure cases
- frontend/index.html: lightweight interface for testing the service
- requirements.txt: optional dependencies for the API server

## Local run steps

1. Install Python 3.10+
2. Install dependencies:

   pip install -r requirements.txt

3. Start the backend:

   uvicorn backend.app:app --reload --port 8000

4. Open the frontend file in a browser and point it at the local API
5. Optionally run the evaluator over the curated cases:

   python -m backend.evaluator

## Resume-ready framing

- Built a debugging tool for failed ML training runs that analyzes logs, configs, and loss curves to identify common PyTorch failure modes and recommend fixes.
- Developed a structured diagnosis API and interactive frontend that surfaces likely root causes, supporting evidence, and next-step recommendations.
- Evaluated the system on curated failure cases spanning OOM errors, training instability, and tensor shape mismatches.

## Naming guidance

Yes, you can absolutely call this project ML Experiment Debugging Agent on your resume.

For the cleanest presentation, it is best if your GitHub repository name also matches or is close, such as:
- ml-experiment-debugging-agent
- ml-training-debugger
- experiment-debugging-agent

Your resume title does not have to be identical to the current local folder name, but matching the GitHub repo name makes the project feel more polished.
