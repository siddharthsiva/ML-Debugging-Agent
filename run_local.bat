@echo off
setlocal

echo Starting ML Experiment Debugging Agent...

set "PY_CMD=python"
where python >nul 2>nul
if errorlevel 1 (
  where py >nul 2>nul
  if errorlevel 1 (
    echo Python is not installed or not on PATH.
    echo Install it with:
    echo winget install -e --id Python.Python.3.11
    exit /b 1
  )
  set "PY_CMD=py -3.11"
)

%PY_CMD% -m venv .venv
call .venv\Scripts\activate
python -m pip install --upgrade pip
pip install -r requirements.txt
start "ML Debugging Agent UI" frontend\index.html
python -m uvicorn backend.app:app --reload --host 127.0.0.1 --port 8000
