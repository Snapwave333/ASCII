#!/bin/bash
# NeonGlyph Production Deployment Validation and Rollout
# Final validation and staged rollout implementation

set -euo pipefail

# Configuration
DEPLOYMENT_ID="$(date +%Y%m%d_%H%M%S)_$(git rev-parse --short HEAD 2>/dev/null || echo 'unknown')"
ROLLOUT_STAGES=("staging" "canary" "production")
CURRENT_STAGE="${1:-staging}"
ROLLBACK_ENABLED=true
MAX_ERRORS=5
MAX_RESPONSE_TIME=2000 # milliseconds
MIN_SUCCESS_RATE=0.95

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
    echo -e "${timestamp} [${level}] ${message}"
}

# Validation metrics
METRICS_FILE="/tmp/deployment_metrics_${DEPLOYMENT_ID}.json"
ERROR_COUNT=0
WARNING_COUNT=0
SUCCESS_COUNT=0
TOTAL_REQUESTS=0

# Initialize metrics
init_metrics() {
    cat > "$METRICS_FILE" << EOF
{
    "deployment_id": "${DEPLOYMENT_ID}",
    "stage": "${CURRENT_STAGE}",
    "start_time": "$(date -Iseconds)",
    "validations": {},
    "summary": {
        "total_checks": 0,
        "passed": 0,
        "failed": 0,
        "warnings": 0,
        "success_rate": 0.0,
        "avg_response_time": 0.0,
        "max_response_time": 0.0,
        "min_response_time": 999999
    }
}
EOF
}

# Update metrics
update_metrics() {
    local check_name=$1
    local status=$2
    local response_time=$3
    local details=$4
    
    local temp_file=$(mktemp)
    jq --arg name "$check_name" \
       --arg status "$status" \
       --arg response_time "$response_time" \
       --arg details "$details" \
       --arg timestamp "$(date -Iseconds)" \
       '.validations[$name] = {
           "status": $status,
           "response_time": ($response_time | tonumber),
           "details": $details,
           "timestamp": $timestamp
       }' "$METRICS_FILE" > "$temp_file" && mv "$temp_file" "$METRICS_FILE"
    
    # Update counters
    case "$status" in
        "PASS")
            ((SUCCESS_COUNT++))
            ;;
        "FAIL")
            ((ERROR_COUNT++))
            ;;
        "WARN")
            ((WARNING_COUNT++))
            ;;
    esac
    
    ((TOTAL_REQUESTS++))
}

# Calculate final metrics
calculate_final_metrics() {
    local temp_file=$(mktemp)
    
    jq --arg total "$TOTAL_REQUESTS" \
       --arg passed "$SUCCESS_COUNT" \
       --arg failed "$ERROR_COUNT" \
       --arg warnings "$WARNING_COUNT" \
       '.summary.total_checks = ($total | tonumber) |
        .summary.passed = ($passed | tonumber) |
        .summary.failed = ($failed | tonumber) |
        .summary.warnings = ($warnings | tonumber) |
        .summary.success_rate = (if ($total | tonumber) > 0 then ($passed | tonumber) / ($total | tonumber) else 0 end)' \
       "$METRICS_FILE" > "$temp_file" && mv "$temp_file" "$METRICS_FILE"
    
    # Add end time
    jq --arg end_time "$(date -Iseconds)" \
       '.end_time = $end_time' "$METRICS_FILE" > "$temp_file" && mv "$temp_file" "$METRICS_FILE"
}

# HTTP request with metrics
http_request() {
    local url=$1
    local method=${2:-GET}
    local data=${3:-}
    local timeout=30
    
    local start_time=$(date +%s%3N)
    local response
    local http_code
    local end_time
    local response_time
    
    if [[ "$method" == "GET" ]]; then
        response=$(curl -s -w "\n%{http_code}" --max-time "$timeout" "$url" 2>/dev/null || echo -e "\n000")
    else
        response=$(curl -s -X "$method" -d "$data" -w "\n%{http_code}" --max-time "$timeout" "$url" 2>/dev/null || echo -e "\n000")
    fi
    
    http_code=$(echo "$response" | tail -n1)
    end_time=$(date +%s%3N)
    response_time=$((end_time - start_time))
    
    echo "$http_code:$response_time"
}

# Validate service availability
validate_availability() {
    log "INFO" "Validating service availability..."
    
    local result=$(http_request "http://localhost:8080/health")
    local http_code=$(echo "$result" | cut -d: -f1)
    local response_time=$(echo "$result" | cut -d: -f2)
    
    if [[ "$http_code" == "200" ]]; then
        if [[ $response_time -le $MAX_RESPONSE_TIME ]]; then
            update_metrics "availability" "PASS" "$response_time" "Service responding normally"
            log "INFO" "✓ Service availability: PASS (${response_time}ms)"
        else
            update_metrics "availability" "WARN" "$response_time" "Service slow but available"
            log "WARN" "⚠ Service availability: WARN (${response_time}ms) - Response time high"
        fi
    else
        update_metrics "availability" "FAIL" "$response_time" "Service unavailable (HTTP $http_code)"
        log "ERROR" "✗ Service availability: FAIL (${response_time}ms) - HTTP $http_code"
        return 1
    fi
}

# Validate API endpoints
validate_api_endpoints() {
    log "INFO" "Validating API endpoints..."
    
    local endpoints=(
        "GET:/health:"
        "GET:/api/config/vulkan.validation.enabled:"
        "GET:/api/errors/summary:"
        "POST:/api/metrics/record:metric_name=test&metric_value=1.0&category=test"
    )
    
    local failed_endpoints=()
    
    for endpoint in "${endpoints[@]}"; do
        IFS=':' read -r method path data <<< "$endpoint"
        
        local url="http://localhost:8080${path}"
        local result=$(http_request "$url" "$method" "$data")
        local http_code=$(echo "$result" | cut -d: -f1)
        local response_time=$(echo "$result" | cut -d: -f2)
        
        local expected_code="200"
        if [[ "$method" == "POST" ]]; then
            expected_code="201"
        fi
        
        if [[ "$http_code" == "$expected_code" ]]; then
            update_metrics "api_${method}_${path//\//_}" "PASS" "$response_time" "Endpoint working"
            log "INFO" "✓ API $method $path: PASS (${response_time}ms)"
        else
            update_metrics "api_${method}_${path//\//_}" "FAIL" "$response_time" "Unexpected response (HTTP $http_code)"
            log "ERROR" "✗ API $method $path: FAIL (${response_time}ms) - Expected $expected_code, got $http_code"
            failed_endpoints+=("$method $path")
        fi
    done
    
    if [[ ${#failed_endpoints[@]} -gt 0 ]]; then
        log "ERROR" "Failed endpoints: ${failed_endpoints[*]}"
        return 1
    fi
}

# Validate error handling
validate_error_handling() {
    log "INFO" "Validating error handling..."
    
    # Test invalid input
    local result=$(http_request "http://localhost:8080/api/config/invalid<>key")
    local http_code=$(echo "$result" | cut -d: -f1)
    local response_time=$(echo "$result" | cut -d: -f2)
    
    if [[ "$http_code" == "400" ]]; then
        update_metrics "error_handling_invalid_input" "PASS" "$response_time" "Properly rejected invalid input"
        log "INFO" "✓ Error handling (invalid input): PASS (${response_time}ms)"
    else
        update_metrics "error_handling_invalid_input" "FAIL" "$response_time" "Should reject invalid input (HTTP $http_code)"
        log "ERROR" "✗ Error handling (invalid input): FAIL (${response_time}ms) - Expected 400, got $http_code"
        return 1
    fi
    
    # Test rate limiting (simulate many requests)
    log "INFO" "Testing rate limiting..."
    local rate_limit_passed=false
    
    for i in {1..120}; do
        result=$(http_request "http://localhost:8080/api/config/test_rate_limit")
        http_code=$(echo "$result" | cut -d: -f1)
        
        if [[ "$http_code" == "429" ]]; then
            rate_limit_passed=true
            break
        fi
    done
    
    if $rate_limit_passed; then
        update_metrics "error_handling_rate_limit" "PASS" "100" "Rate limiting working"
        log "INFO" "✓ Error handling (rate limiting): PASS"
    else
        update_metrics "error_handling_rate_limit" "WARN" "100" "Rate limiting may not be enforced"
        log "WARN" "⚠ Error handling (rate limiting): WARN - Rate limit not triggered"
    fi
}

# Validate performance metrics
validate_performance() {
    log "INFO" "Validating performance metrics..."
    
    # Test response time under load
    local total_time=0
    local request_count=20
    local max_time=0
    local min_time=999999
    local success_count=0
    
    for i in $(seq 1 $request_count); do
        local result=$(http_request "http://localhost:8080/health")
        local http_code=$(echo "$result" | cut -d: -f1)
        local response_time=$(echo "$result" | cut -d: -f2)
        
        if [[ "$http_code" == "200" ]]; then
            ((success_count++))
            total_time=$((total_time + response_time))
            
            if [[ $response_time -gt $max_time ]]; then
                max_time=$response_time
            fi
            
            if [[ $response_time -lt $min_time ]]; then
                min_time=$response_time
            fi
        fi
    done
    
    local avg_time=$((total_time / success_count))
    local success_rate=$(echo "scale=2; $success_count / $request_count" | bc -l)
    
    update_metrics "performance_avg_response_time" "DATA" "$avg_time" "Average: ${avg_time}ms, Min: ${min_time}ms, Max: ${max_time}ms"
    update_metrics "performance_success_rate" "DATA" "$(echo "$success_rate * 100" | bc -l)" "Success rate: ${success_rate}"
    
    if [[ $(echo "$success_rate >= $MIN_SUCCESS_RATE" | bc -l) -eq 1 ]]; then
        log "INFO" "✓ Performance (success rate): PASS (${success_rate})"
    else
        log "ERROR" "✗ Performance (success rate): FAIL (${success_rate})"
        return 1
    fi
    
    if [[ $avg_time -le $MAX_RESPONSE_TIME ]]; then
        log "INFO" "✓ Performance (avg response time): PASS (${avg_time}ms)"
    else
        log "WARN" "⚠ Performance (avg response time): WARN (${avg_time}ms) - Above threshold"
    fi
}

# Validate security
validate_security() {
    log "INFO" "Validating security..."
    
    # Test SQL injection prevention
    local result=$(http_request "http://localhost:8080/api/config/SELECT%20*%20FROM%20users")
    local http_code=$(echo "$result" | cut -d: -f2)
    local response_time=$(echo "$result" | cut -d: -f2)
    
    if [[ "$http_code" == "400" ]]; then
        update_metrics "security_sql_injection" "PASS" "$response_time" "SQL injection prevented"
        log "INFO" "✓ Security (SQL injection): PASS (${response_time}ms)"
    else
        update_metrics "security_sql_injection" "FAIL" "$response_time" "SQL injection not prevented (HTTP $http_code)"
        log "ERROR" "✗ Security (SQL injection): FAIL (${response_time}ms) - HTTP $http_code"
        return 1
    fi
    
    # Test XSS prevention
    result=$(http_request "http://localhost:8080/api/config/test" "POST" "value=<script>alert('xss')</script>")
    http_code=$(echo "$result" | cut -d: -f1)
    response_time=$(echo "$result" | cut -d: -f2)
    
    if [[ "$http_code" == "400" ]]; then
        update_metrics "security_xss" "PASS" "$response_time" "XSS prevented"
        log "INFO" "✓ Security (XSS): PASS (${response_time}ms)"
    else
        update_metrics "security_xss" "FAIL" "$response_time" "XSS not prevented (HTTP $http_code)"
        log "ERROR" "✗ Security (XSS): FAIL (${response_time}ms) - HTTP $http_code"
        return 1
    fi
}

# Validate database connectivity
validate_database() {
    log "INFO" "Validating database connectivity..."
    
    # Test database connection through API
    local result=$(http_request "http://localhost:8080/api/database/health")
    local http_code=$(echo "$result" | cut -d: -f1)
    local response_time=$(echo "$result" | cut -d: -f2)
    
    if [[ "$http_code" == "200" ]]; then
        update_metrics "database_connectivity" "PASS" "$response_time" "Database accessible"
        log "INFO" "✓ Database connectivity: PASS (${response_time}ms)"
    else
        update_metrics "database_connectivity" "FAIL" "$response_time" "Database unavailable (HTTP $http_code)"
        log "ERROR" "✗ Database connectivity: FAIL (${response_time}ms) - HTTP $http_code"
        return 1
    fi
}

# Validate monitoring integration
validate_monitoring() {
    log "INFO" "Validating monitoring integration..."
    
    # Check Prometheus
    if systemctl is-active --quiet prometheus; then
        update_metrics "monitoring_prometheus" "PASS" "0" "Prometheus running"
        log "INFO" "✓ Monitoring (Prometheus): PASS"
    else
        update_metrics "monitoring_prometheus" "WARN" "0" "Prometheus not running"
        log "WARN" "⚠ Monitoring (Prometheus): WARN - Service not running"
    fi
    
    # Check Grafana
    if systemctl is-active --quiet grafana-server; then
        update_metrics "monitoring_grafana" "PASS" "0" "Grafana running"
        log "INFO" "✓ Monitoring (Grafana): PASS"
    else
        update_metrics "monitoring_grafana" "WARN" "0" "Grafana not running"
        log "WARN" "⚠ Monitoring (Grafana): WARN - Service not running"
    fi
}

# Generate deployment report
generate_deployment_report() {
    calculate_final_metrics
    
    local report_file="/tmp/deployment_report_${DEPLOYMENT_ID}.html"
    
    cat > "$report_file" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>NeonGlyph Deployment Validation Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .summary { margin: 20px 0; }
        .validation-results { margin: 20px 0; }
        .pass { color: green; }
        .fail { color: red; }
        .warn { color: orange; }
        .data { color: blue; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .metric-box { display: inline-block; padding: 10px; margin: 5px; border-radius: 5px; }
        .success-box { background-color: #d4edda; border: 1px solid #c3e6cb; }
        .warning-box { background-color: #fff3cd; border: 1px solid #ffeaa7; }
        .error-box { background-color: #f8d7da; border: 1px solid #f5c6cb; }
    </style>
</head>
<body>
    <div class="header">
        <h1>NeonGlyph Deployment Validation Report</h1>
        <p><strong>Deployment ID:</strong> ${DEPLOYMENT_ID}</p>
        <p><strong>Stage:</strong> ${CURRENT_STAGE}</p>
        <p><strong>Generated:</strong> $(date)</p>
    </div>
    
    <div class="summary">
        <h2>Validation Summary</h2>
        <div class="metric-box success-box">
            <strong>Passed:</strong> $(jq -r '.summary.passed' "$METRICS_FILE")
        </div>
        <div class="metric-box error-box">
            <strong>Failed:</strong> $(jq -r '.summary.failed' "$METRICS_FILE")
        </div>
        <div class="metric-box warning-box">
            <strong>Warnings:</strong> $(jq -r '.summary.warnings' "$METRICS_FILE")
        </div>
        <div class="metric-box">
            <strong>Success Rate:</strong> $(jq -r '.summary.success_rate' "$METRICS_FILE")%
        </div>
        <div class="metric-box">
            <strong>Total Checks:</strong> $(jq -r '.summary.total_checks' "$METRICS_FILE")
        </div>
    </div>
    
    <div class="validation-results">
        <h2>Validation Results</h2>
        <table>
            <tr>
                <th>Check</th>
                <th>Status</th>
                <th>Response Time</th>
                <th>Details</th>
            </tr>
EOF
    
    # Add validation results
    jq -r '.validations | to_entries[] | [.key, .value.status, .value.response_time, .value.details] | @tsv' "$METRICS_FILE" | while IFS=$'\t' read -r check status response_time details; do
        local status_class=""
        case "$status" in
            "PASS") status_class="pass" ;;
            "FAIL") status_class="fail" ;;
            "WARN") status_class="warn" ;;
            "DATA") status_class="data" ;;
        esac
        
        echo "            <tr><td>$check</td><td class=\"$status_class\">$status</td><td>${response_time}ms</td><td>$details</td></tr>" >> "$report_file"
    done
    
    cat >> "$report_file" << EOF
        </table>
    </div>
    
    <div class="footer">
        <p><strong>Start Time:</strong> $(jq -r '.start_time' "$METRICS_FILE")</p>
        <p><strong>End Time:</strong> $(jq -r '.end_time' "$METRICS_FILE")</p>
        <p><strong>Metrics File:</strong> $METRICS_FILE</p>
    </div>
</body>
</html>
EOF
    
    log "INFO" "Deployment report generated: $report_file"
}

# Check if deployment should proceed
check_deployment_status() {
    local success_rate=$(jq -r '.summary.success_rate' "$METRICS_FILE")
    local failed_checks=$(jq -r '.summary.failed' "$METRICS_FILE")
    
    # Check success rate threshold
    if [[ $(echo "$success_rate < 80" | bc -l) -eq 1 ]]; then
        log "ERROR" "Deployment validation failed: Success rate ${success_rate}% below 80% threshold"
        return 1
    fi
    
    # Check critical failures
    if [[ $failed_checks -gt 3 ]]; then
        log "ERROR" "Deployment validation failed: Too many failed checks ($failed_checks)"
        return 1
    fi
    
    # Check for critical service failures
    if jq -e '.validations.availability.status == "FAIL"' "$METRICS_FILE" > /dev/null; then
        log "ERROR" "Deployment validation failed: Service availability check failed"
        return 1
    fi
    
    log "INFO" "Deployment validation passed: Success rate ${success_rate}%, ${failed_checks} failed checks"
    return 0
}

# Execute next stage
proceed_to_next_stage() {
    local current_stage_index=-1
    
    # Find current stage index
    for i in "${!ROLLOUT_STAGES[@]}"; do
        if [[ "${ROLLOUT_STAGES[$i]}" == "$CURRENT_STAGE" ]]; then
            current_stage_index=$i
            break
        fi
    done
    
    # Check if there's a next stage
    local next_stage_index=$((current_stage_index + 1))
    if [[ $next_stage_index -lt ${#ROLLOUT_STAGES[@]} ]]; then
        local next_stage="${ROLLOUT_STAGES[$next_stage_index]}"
        log "INFO" "Proceeding to next stage: $next_stage"
        
        # Trigger next stage deployment
        log "INFO" "Next stage deployment command: bash $0 $next_stage"
    else
        log "INFO" "🎉 Deployment completed successfully through all stages!"
        log "INFO" "Final deployment validation passed for stage: $CURRENT_STAGE"
        log "INFO" "Deployment ID: $DEPLOYMENT_ID"
        
        # Generate final report
        generate_deployment_report
    fi
}

# Main validation function
run_validation() {
    log "INFO" "Starting deployment validation for stage: $CURRENT_STAGE"
    log "INFO" "Deployment ID: $DEPLOYMENT_ID"
    
    init_metrics
    
    # Run all validations
    validate_availability
    validate_api_endpoints
    validate_error_handling
    validate_performance
    validate_security
    validate_database
    validate_monitoring
    
    # Check overall status
    if check_deployment_status; then
        log "INFO" "✅ Deployment validation PASSED for stage: $CURRENT_STAGE"
        proceed_to_next_stage
    else
        log "ERROR" "❌ Deployment validation FAILED for stage: $CURRENT_STAGE"
        
        if $ROLLBACK_ENABLED; then
            log "ERROR" "Initiating rollback due to validation failures..."
            # Trigger rollback script
            log "ERROR" "Rollback command: bash /opt/neonglyph/deploy/rollback.sh $DEPLOYMENT_ID"
        fi
        
        exit 1
    fi
}

# Usage information
usage() {
    echo "Usage: $0 [stage]"
    echo "Stages: ${ROLLOUT_STAGES[*]}"
    echo "Example: $0 staging"
    exit 1
}

# Main execution
main() {
    if [[ $# -gt 1 ]]; then
        usage
    fi
    
    # Validate stage
    local valid_stage=false
    for stage in "${ROLLOUT_STAGES[@]}"; do
        if [[ "$stage" == "$CURRENT_STAGE" ]]; then
            valid_stage=true
            break
        fi
    done
    
    if ! $valid_stage; then
        log "ERROR" "Invalid stage: $CURRENT_STAGE"
        usage
    fi
    
    log "INFO" "Starting NeonGlyph deployment validation and rollout..."
    log "INFO" "Current stage: $CURRENT_STAGE"
    log "INFO" "Available stages: ${ROLLOUT_STAGES[*]}"
    
    run_validation
}

# Execute main function
main "$@"