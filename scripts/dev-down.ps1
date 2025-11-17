Write-Host "Stopping local stack..." -ForegroundColor Cyan
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
  Write-Error "Docker is not installed or not on PATH"
  exit 1
}

docker compose down
Write-Host "Stack stopped." -ForegroundColor Green

