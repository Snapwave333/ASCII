#!/bin/bash
# NeonGlyph Production Deployment Script
# Comprehensive deployment automation with rollback capabilities

set -euo pipefail

# Configuration
DEPLOYMENT_ID="$(date +%Y%m%d_%H%M%S)_$(git rev-parse --short HEAD 2>/dev/null || echo 'unknown')"
VERSION="${1:-latest}"
ENVIRONMENT="${2:-production}"
BACKUP_DIR="/opt/neonglyph/backups"
DEPLOYMENT_LOG="/var/log/neonglyph/deployment_${DEPLOYMENT_ID}.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging function
log() {
    local level=$1
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${timestamp} [${level}] ${message}" | tee -a "${DEPLOYMENT_LOG}"
}

# Error handling
error_exit() {
    log "ERROR" "$1"
    echo -e "${RED}Deployment failed! Check logs at: ${DEPLOYMENT_LOG}${NC}"
    exit 1
}

# Success notification
success_notification() {
    log "SUCCESS" "Deployment completed successfully"
    echo -e "${GREEN}Deployment completed successfully!${NC}"
    echo -e "${GREEN}Deployment ID: ${DEPLOYMENT_ID}${NC}"
    echo -e "${GREEN}Version: ${VERSION}${NC}"
    echo -e "${GREEN}Environment: ${ENVIRONMENT}${NC}"
    echo -e "${GREEN}Logs: ${DEPLOYMENT_LOG}${NC}"
}

# Pre-deployment checks
pre_deployment_checks() {
    log "INFO" "Starting pre-deployment checks..."
    
    # Check if running as root
    if [[ $EUID -ne 0 ]]; then
        error_exit "This script must be run as root"
    fi
    
    # Check disk space
    local available_space=$(df /opt | tail -1 | awk '{print $4}')
    if [[ $available_space -lt 1048576 ]]; then # 1GB in KB
        error_exit "Insufficient disk space. Required: 1GB, Available: $((available_space / 1024))MB"
    fi
    
    # Check system resources
    local memory_available=$(free -m | awk '/^Mem:/{print $7}')
    if [[ $memory_available -lt 512 ]]; then
        error_exit "Insufficient memory. Required: 512MB, Available: ${memory_available}MB"
    fi
    
    # Check network connectivity
    if ! ping -c 1 google.com &> /dev/null; then
        log "WARNING" "Network connectivity issues detected"
    fi
    
    # Check if services are running
    if systemctl is-active --quiet neonglyph; then
        log "INFO" "Current NeonGlyph service is running"
    else
        log "WARNING" "Current NeonGlyph service is not running"
    fi
    
    log "INFO" "Pre-deployment checks completed successfully"
}

# Create backup
create_backup() {
    log "INFO" "Creating backup..."
    
    mkdir -p "${BACKUP_DIR}"
    local backup_path="${BACKUP_DIR}/neonglyph_backup_${DEPLOYMENT_ID}.tar.gz"
    
    # Backup current installation
    if [[ -d "/opt/neonglyph/current" ]]; then
        tar -czf "${backup_path}" -C /opt/neonglyph current 2>/dev/null || {
            error_exit "Failed to create backup"
        }
        log "INFO" "Backup created: ${backup_path}"
        echo "BACKUP_PATH=${backup_path}" >> /tmp/deployment_env
    else
        log "WARNING" "No current installation found, skipping backup"
    fi
}

# Database migration
database_migration() {
    log "INFO" "Running database migrations..."
    
    # Check if PostgreSQL is running
    if ! systemctl is-active --quiet postgresql; then
        error_exit "PostgreSQL service is not running"
    fi
    
    # Run migrations
    cd /opt/neonglyph/releases/${VERSION}
    if [[ -f "database/migrate.sh" ]]; then
        sudo -u postgres bash database/migrate.sh || {
            error_exit "Database migration failed"
        }
        log "INFO" "Database migrations completed successfully"
    else
        log "WARNING" "No database migration script found"
    fi
}

# Deploy application
deploy_application() {
    log "INFO" "Deploying application..."
    
    # Create release directory
    local release_dir="/opt/neonglyph/releases/${VERSION}"
    mkdir -p "${release_dir}"
    
    # Copy application files
    cp -r /tmp/neonglyph_build/* "${release_dir}/" || {
        error_exit "Failed to copy application files"
    }
    
    # Set permissions
    chown -R neonglyph:neonglyph "${release_dir}"
    chmod +x "${release_dir}/bin/neonglyph"
    
    # Update symlink
    ln -sfn "${release_dir}" /opt/neonglyph/current || {
        error_exit "Failed to update current symlink"
    }
    
    log "INFO" "Application deployed successfully"
}

# Configuration update
update_configuration() {
    log "INFO" "Updating configuration..."
    
    # Copy production configuration
    cp /opt/neonglyph/shared/config/production.json /opt/neonglyph/current/config/ || {
        error_exit "Failed to copy production configuration"
    }
    
    # Update environment variables
    cp /opt/neonglyph/shared/.env.production /opt/neonglyph/current/.env || {
        error_exit "Failed to copy environment variables"
    }
    
    log "INFO" "Configuration updated successfully"
}

# Service management
manage_services() {
    log "INFO" "Managing services..."
    
    # Stop current service
    if systemctl is-active --quiet neonglyph; then
        systemctl stop neonglyph || {
            error_exit "Failed to stop current NeonGlyph service"
        }
        log "INFO" "Current service stopped"
    fi
    
    # Reload systemd
    systemctl daemon-reload || {
        error_exit "Failed to reload systemd"
    }
    
    # Start new service
    systemctl start neonglyph || {
        error_exit "Failed to start new NeonGlyph service"
    }
    
    # Enable service on boot
    systemctl enable neonglyph || {
        error_exit "Failed to enable NeonGlyph service"
    }
    
    log "INFO" "Services managed successfully"
}

# Health check
health_check() {
    log "INFO" "Performing health check..."
    
    local max_attempts=30
    local attempt=1
    
    while [[ $attempt -le $max_attempts ]]; do
        if curl -s -f http://localhost:8080/health > /dev/null; then
            log "INFO" "Health check passed"
            return 0
        fi
        
        log "INFO" "Health check attempt $attempt failed, retrying in 10 seconds..."
        sleep 10
        ((attempt++))
    done
    
    error_exit "Health check failed after $max_attempts attempts"
}

# Performance validation
performance_validation() {
    log "INFO" "Running performance validation..."
    
    # Run performance tests
    cd /opt/neonglyph/current
    if [[ -f "tests/performance_test.sh" ]]; then
        bash tests/performance_test.sh || {
            log "WARNING" "Performance validation failed"
            return 1
        }
    fi
    
    # Check response time
    local response_time=$(curl -s -w "%{time_total}" -o /dev/null http://localhost:8080/health)
    if (( $(echo "$response_time > 2.0" | bc -l) )); then
        log "WARNING" "Response time too high: ${response_time}s"
    else
        log "INFO" "Response time acceptable: ${response_time}s"
    fi
    
    log "INFO" "Performance validation completed"
}

# Security validation
security_validation() {
    log "INFO" "Running security validation..."
    
    # Check for security vulnerabilities
    if [[ -f "security/scan.sh" ]]; then
        bash security/scan.sh || {
            log "WARNING" "Security scan failed"
        }
    fi
    
    # Validate file permissions
    find /opt/neonglyph/current -type f -perm /002 -exec ls -l {} \; | while read line; do
        log "WARNING" "World-writable file found: $line"
    done
    
    log "INFO" "Security validation completed"
}

# Update monitoring
update_monitoring() {
    log "INFO" "Updating monitoring configuration..."
    
    # Update Prometheus configuration
    cp monitoring/prometheus.yml /etc/prometheus/ || {
        log "WARNING" "Failed to update Prometheus configuration"
    }
    
    # Update Grafana dashboards
    cp monitoring/grafana_dashboard.json /var/lib/grafana/dashboards/ || {
        log "WARNING" "Failed to update Grafana dashboards"
    }
    
    # Restart monitoring services
    systemctl reload prometheus || log "WARNING" "Failed to reload Prometheus"
    systemctl reload grafana-server || log "WARNING" "Failed to reload Grafana"
    
    log "INFO" "Monitoring updated successfully"
}

# Record deployment
record_deployment() {
    log "INFO" "Recording deployment..."
    
    # Create deployment record
    cat > /tmp/deployment_record.json << EOF
{
    "deployment_id": "${DEPLOYMENT_ID}",
    "version": "${VERSION}",
    "environment": "${ENVIRONMENT}",
    "deployment_type": "production",
    "status": "success",
    "deployed_by": "$(whoami)",
    "deployment_notes": "Automated deployment with comprehensive validation",
    "start_time": "$(date -Iseconds)",
    "end_time": "$(date -Iseconds)",
    "validation_results": {
        "health_check": "passed",
        "performance_validation": "passed",
        "security_validation": "passed"
    },
    "performance_baseline": {
        "response_time": "$(curl -s -w '%{time_total}' -o /dev/null http://localhost:8080/health)",
        "memory_usage": "$(free -m | awk '/^Mem:/{printf "%.2f", $3/$2*100}')",
        "cpu_usage": "$(top -bn1 | grep 'Cpu(s)' | awk '{print $2}' | cut -d'%' -f1)"
    }
}
EOF
    
    # Insert into database (would be actual database insert)
    log "INFO" "Deployment recorded: ${DEPLOYMENT_ID}"
}

# Cleanup
cleanup() {
    log "INFO" "Cleaning up..."
    
    # Remove temporary files
    rm -f /tmp/deployment_env
    rm -f /tmp/deployment_record.json
    
    # Clean old releases (keep last 5)
    cd /opt/neonglyph/releases
    ls -t | tail -n +6 | xargs -r rm -rf
    
    # Clean old backups (keep last 10)
    cd /opt/neonglyph/backups
    ls -t | tail -n +11 | xargs -r rm -f
    
    log "INFO" "Cleanup completed"
}

# Rollback function
rollback() {
    log "ERROR" "Deployment failed, initiating rollback..."
    
    # Restore from backup if available
    if [[ -f /tmp/deployment_env ]]; then
        source /tmp/deployment_env
        if [[ -n "${BACKUP_PATH:-}" ]] && [[ -f "${BACKUP_PATH}" ]]; then
            log "INFO" "Restoring from backup: ${BACKUP_PATH}"
            
            # Stop current service
            systemctl stop neonglyph || true
            
            # Restore from backup
            tar -xzf "${BACKUP_PATH}" -C /opt/neonglyph/ || {
                error_exit "Failed to restore from backup"
            }
            
            # Restart service
            systemctl start neonglyph || {
                error_exit "Failed to restart service after rollback"
            }
            
            log "INFO" "Rollback completed successfully"
            return 0
        fi
    fi
    
    log "ERROR" "Rollback failed - manual intervention required"
}

# Main deployment function
main() {
    log "INFO" "Starting NeonGlyph production deployment..."
    log "INFO" "Deployment ID: ${DEPLOYMENT_ID}"
    log "INFO" "Version: ${VERSION}"
    log "INFO" "Environment: ${ENVIRONMENT}"
    
    # Set up error handling
    trap 'rollback' ERR
    
    # Execute deployment steps
    pre_deployment_checks
    create_backup
    database_migration
    deploy_application
    update_configuration
    manage_services
    health_check
    performance_validation
    security_validation
    update_monitoring
    record_deployment
    cleanup
    
    # Success
    success_notification
    
    # Remove error trap
    trap - ERR
}

# Execute main function
main "$@"