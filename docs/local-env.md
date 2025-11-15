# NeonGlyph Local Development Environment

**Document Type**: Configuration Guide
**Version**: 2.0.1
**Last Updated**: 2025-11-14
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview

This document describes the local development environment setup using Docker containers for monitoring, metrics collection, and AI model serving integration with NeonGlyph.

## System Architecture

### Services Overview

| Service | Port | Purpose | Health Check |
|---------|------|---------|---------------|
| Prometheus | 9091 | Metrics storage and queries | `/-/healthy` |
| Grafana | 3000 | Visualization dashboards | `/api/health` |
| cAdvisor | 8080 | Container metrics exporter | `/healthz` |
| Ollama Gateway | 11434 (host) | AI model serving | `/api/tags` |

### Network Configuration

All services communicate through Docker's bridge network with the following configurations:
- **Internal DNS**: Automatic service discovery
- **Host Access**: Services exposed to localhost only
- **Security**: No external network exposure

## Prerequisites

### System Requirements
- **Docker Desktop**: Version 20.10 or later
- **Docker Compose**: Version 1.29 or later
- **PowerShell**: Version 5.1 or later (Windows)
- **Memory**: Minimum 4GB available RAM
- **Storage**: 2GB free disk space

### Host System Setup
1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop)
2. Enable WSL2 backend (Windows)
3. Configure resource limits in Docker Desktop settings

## Installation

### Step 1: Environment Preparation

1. **Clone Repository**
   ```powershell
   git clone https://github.com/your-repo/NeonGlyph.git
   cd NeonGlyph
   ```

2. **Verify Docker Installation**
   ```powershell
   docker --version
   docker-compose --version
   ```

3. **Check System Resources**
   ```powershell
   docker system info
   ```

### Step 2: Service Startup

1. **Start Development Environment**
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\scripts\dev-up.ps1
   ```

2. **Verify Service Status**
   ```powershell
   docker-compose ps
   ```

3. **Check Service Health**
   ```powershell
   # Test Prometheus
   Invoke-RestMethod -Uri "http://localhost:9091/-/healthy"
   
   # Test Grafana
   Invoke-RestMethod -Uri "http://localhost:3000/api/health"
   
   # Test cAdvisor
   Invoke-RestMethod -Uri "http://localhost:8080/healthz"
   ```

## Service Configuration

### Prometheus Configuration

**File**: `monitoring/prometheus.yml`

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9090']
  
  - job_name: 'cadvisor'
    static_configs:
      - targets: ['cadvisor:8080']
    scrape_interval: 10s
```

**Key Settings**:
- **Scrape Interval**: 15 seconds (configurable)
- **Retention**: 15 days default
- **Storage**: Local TSDB

### Grafana Configuration

**Default Credentials**:
- **Username**: admin
- **Password**: admin

**Initial Setup**:
1. Access Grafana at http://localhost:3000
2. Change default password
3. Add Prometheus data source:
   - URL: http://prometheus:9090
   - Access: Server (default)

### cAdvisor Configuration

**Capabilities**:
- Container CPU usage
- Memory consumption
- Network I/O statistics
- Filesystem usage
- Process information

**Security**: Read-only access to container metrics

### Ollama Integration

**Host System Requirements**:
- Ollama installed on host system
- Models downloaded and available
- API accessible at localhost:11434

**Container Access**:
```bash
# From within containers
curl http://host.docker.internal:11434/api/tags
```

## Usage

### Monitoring Dashboard

1. **Access Grafana**: http://localhost:3000
2. **Import Dashboards**:
   - Container metrics dashboard
   - System resource dashboard
   - Custom application metrics

3. **Create Alerts**:
   - High CPU usage
   - Memory threshold exceeded
   - Service unavailability

### Metrics Collection

**Available Metrics**:
- Container CPU usage
- Memory consumption
- Network I/O
- Disk I/O
- Process count

**Query Examples**:
```promql
# Container CPU usage
rate(container_cpu_usage_seconds_total{name="grafana"}[5m])

# Memory usage
container_memory_usage_bytes{name="prometheus"}

# Network I/O
rate(container_network_receive_bytes_total[5m])
```

### Log Management

**Log Configuration**:
```yaml
logging:
  driver: json-file
  options:
    max-size: "10m"
    max-file: "3"
```

**View Logs**:
```powershell
# View all service logs
docker-compose logs

# View specific service logs
docker-compose logs prometheus

# Follow log output
docker-compose logs -f grafana
```

## Resource Management

### Memory Usage Estimates

| Service | Baseline Memory | Peak Memory | Notes |
|---------|----------------|-------------|--------|
| Prometheus | 200MB | 500MB | Depends on retention and metrics |
| Grafana | 150MB | 300MB | Increases with dashboards |
| cAdvisor | 100MB | 200MB | Varies with container count |
| Total | 450MB | 1000MB | Monitor host resources |

### CPU Usage

- **Baseline**: <5% on typical development machines
- **Peak**: 10-20% during dashboard queries
- **Optimization**: Adjust scrape intervals to reduce load

### Storage Management

**Prometheus Storage**:
- Default retention: 15 days
- Storage location: Docker volume
- Cleanup: Automatic based on retention policy

**Log Rotation**:
- Maximum size: 10MB per log file
- Maximum files: 3 per service
- Cleanup: Automatic when limits exceeded

## Security Configuration

### Network Security
- All services bound to localhost only
- No external network exposure
- Internal Docker networking for service communication

### Access Control
- **Grafana**: Change default admin password
- **Prometheus**: No authentication by default (localhost only)
- **cAdvisor**: Read-only metrics access

### Data Protection
- Metrics data stored locally
- No external data transmission
- Encrypted Docker volumes (optional)

## Troubleshooting

### Common Issues

1. **Docker Not Found**
   ```
   Error: Docker not found in PATH
   ```
   **Solution**: Install Docker Desktop and restart terminal

2. **Port Conflicts**
   ```
   Error: Bind for 0.0.0.0:3000 failed: port is already allocated
   ```
   **Solution**: Stop conflicting services or modify ports in docker-compose.yml

3. **Permission Denied**
   ```
   Error: Permission denied while trying to connect to Docker daemon
   ```
   **Solution**: Run PowerShell as Administrator or add user to docker-users group

4. **Ollama Connection Failed**
   ```
   Error: Connection refused to host.docker.internal:11434
   ```
   **Solution**: Ensure Ollama is running on host system

### Performance Issues

1. **High Memory Usage**
   - Reduce Prometheus retention period
   - Decrease scrape frequency
   - Limit number of metrics collected

2. **Slow Dashboard Loading**
   - Optimize Prometheus queries
   - Reduce time range for queries
   - Use recording rules for complex calculations

### Service Health Checks

**Prometheus Health**:
```powershell
Invoke-RestMethod -Uri "http://localhost:9091/-/healthy"
```

**Grafana Health**:
```powershell
Invoke-RestMethod -Uri "http://localhost:3000/api/health"
```

**cAdvisor Health**:
```powershell
Invoke-RestMethod -Uri "http://localhost:8080/healthz"
```

## Advanced Configuration

### Custom Metrics

**Application Integration**:
```cpp
// Example C++ metrics export
#include <prometheus/counter.h>

prometheus::Counter& request_counter = 
    prometheus::BuildCounter()
        .Name("neonglyph_requests_total")
        .Help("Total number of requests")
        .Register(registry);
```

### Alert Rules

**Prometheus Alert Configuration**:
```yaml
groups:
- name: neonglyph_alerts
  rules:
  - alert: HighErrorRate
    expr: rate(errors_total[5m]) > 0.1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "High error rate detected"
```

### Dashboard Customization

**Grafana Dashboard JSON**:
```json
{
  "dashboard": {
    "title": "NeonGlyph Performance",
    "panels": [
      {
        "title": "Frame Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "neonglyph_fps"
          }
        ]
      }
    ]
  }
}
```

## Maintenance

### Regular Tasks

**Daily**:
- Check service health status
- Monitor disk usage
- Verify log rotation

**Weekly**:
- Review Grafana dashboards
- Update alert configurations
- Clean up old Docker images

**Monthly**:
- Update Docker images
- Review retention policies
- Performance optimization review

### Backup Procedures

**Configuration Backup**:
```powershell
# Backup Grafana dashboards
docker exec grafana grafana-cli admin export-dashboard

# Backup Prometheus data
docker run --rm -v prometheus_data:/data -v $(pwd):/backup alpine tar czf /backup/prometheus-backup.tar.gz /data
```

## References

- [Prometheus Documentation](https://prometheus.io/docs/)
- [Grafana Documentation](https://grafana.com/docs/)
- [cAdvisor Documentation](https://github.com/google/cadvisor)
- [Docker Compose Reference](https://docs.docker.com/compose/)
- [NeonGlyph Build Guide](../.trae/documents/build-guide.md)
- [Documentation Index](../.trae/documents/DOCUMENTATION_INDEX.md)
- [Supabase Integration](Supabase.md)

## Change History

### Version 2.0.1 (2025-11-14)
- Updated references to Build Guide and Documentation Index
- Added cross-reference to Supabase integration
- Bumped metadata timestamps

### Version 2.0.0 (2024-11-13)
- Complete rewrite following documentation style guide
- Added comprehensive service configuration details
- Enhanced troubleshooting section
- Added security and maintenance guidelines

### Version 1.0.0 (2024-01-01)
- Initial Docker environment documentation
- Basic service startup instructions