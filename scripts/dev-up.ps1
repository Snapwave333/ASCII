param(
  [int]$GrafanaPort = 3000,
  [int]$PrometheusPort = 9090,
  [int]$CAdvisorPort = 8080
)

Write-Host "Starting local stack..." -ForegroundColor Cyan

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
  Write-Error "Docker is not installed or not on PATH"
  exit 1
}

docker compose pull
docker compose up -d

Write-Host "Health checks:" -ForegroundColor Green
try { Invoke-WebRequest -UseBasicParsing "http://localhost:$GrafanaPort/api/health" | Out-Null; Write-Host "Grafana OK" } catch { Write-Warning "Grafana not ready" }
try { Invoke-WebRequest -UseBasicParsing "http://localhost:$PrometheusPort/-/healthy" | Out-Null; Write-Host "Prometheus OK" } catch { Write-Warning "Prometheus not ready" }
try { Invoke-WebRequest -UseBasicParsing "http://localhost:$CAdvisorPort/healthz" | Out-Null; Write-Host "cAdvisor OK" } catch { Write-Warning "cAdvisor not ready" }

Write-Host "Ollama at host.docker.internal:11434" -ForegroundColor Yellow
Write-Host "Open Grafana: http://localhost:$GrafanaPort (admin/admin)" -ForegroundColor Yellow
Write-Host "Open Prometheus: http://localhost:$PrometheusPort" -ForegroundColor Yellow
Write-Host "Open cAdvisor: http://localhost:$CAdvisorPort" -ForegroundColor Yellow

