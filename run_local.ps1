Write-Host "Starting ML Experiment Debugging Agent..."

$python = Get-Command python -ErrorAction SilentlyContinue
$pyLauncher = Get-Command py -ErrorAction SilentlyContinue
if (-not $python -and -not $pyLauncher) {
    Write-Host "Python is not installed or not on PATH."
    Write-Host "Install it with: winget install -e --id Python.Python.3.11"
    exit 1
}

if ($python) {
    python -m venv .venv
} else {
    py -3.11 -m venv .venv
}

& .\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r requirements.txt
Start-Process "frontend\index.html"
python -m uvicorn backend.app:app --reload --host 127.0.0.1 --port 8000
