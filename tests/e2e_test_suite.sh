#!/bin/bash
# NeonGlyph End-to-End Test Suite
# Comprehensive testing for production deployment validation

set -euo pipefail

# Configuration
TEST_SUITE_NAME="NeonGlyph Production E2E Tests"
TEST_RESULTS_DIR="/tmp/neonglyph_test_results"
TEST_LOG="${TEST_RESULTS_DIR}/test_$(date +%Y%m%d_%H%M%S).log"
MAX_TEST_TIME=300 # 5 minutes per test
PARALLEL_TESTS=4

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

# Logging function
log() {
    local level=$1
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${timestamp} [${level}] ${message}" | tee -a "${TEST_LOG}"
}

# Test result tracking
test_start() {
    local test_name=$1
    log "INFO" "Starting test: ${test_name}"
    ((TESTS_RUN++))
    echo "$(date +%s)" > /tmp/test_start_time
}

test_pass() {
    local test_name=$1
    log "PASS" "✓ ${test_name}"
    ((TESTS_PASSED++))
}

test_fail() {
    local test_name=$1
    local reason=$2
    log "FAIL" "✗ ${test_name}: ${reason}"
    ((TESTS_FAILED++))
}

test_skip() {
    local test_name=$1
    local reason=$2
    log "SKIP" "⊘ ${test_name}: ${reason}"
    ((TESTS_SKIPPED++))
}

# Timeout function
test_timeout() {
    local test_name=$1
    local timeout_seconds=$2
    
    if [[ -f /tmp/test_start_time ]]; then
        local start_time=$(cat /tmp/test_start_time)
        local current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        
        if [[ $elapsed -gt $timeout_seconds ]]; then
            test_fail "${test_name}" "Test timed out after ${timeout_seconds} seconds"
            return 1
        fi
    fi
    return 0
}

# Setup test environment
setup_test_environment() {
    log "INFO" "Setting up test environment..."
    
    mkdir -p "${TEST_RESULTS_DIR}"
    
    # Check if application is running
    if ! curl -s -f http://localhost:8080/health > /dev/null; then
        log "ERROR" "Application is not running. Please start it first."
        exit 1
    fi
    
    # Check if database is accessible
    if ! systemctl is-active --quiet postgresql; then
        log "ERROR" "PostgreSQL is not running. Please start it first."
        exit 1
    fi
    
    # Create test database
    sudo -u postgres createdb neonglyph_test 2>/dev/null || true
    
    log "INFO" "Test environment setup completed"
}

# Cleanup test environment
cleanup_test_environment() {
    log "INFO" "Cleaning up test environment..."
    
    # Drop test database
    sudo -u postgres dropdb neonglyph_test 2>/dev/null || true
    
    # Clean up temporary files
    rm -f /tmp/test_start_time
    rm -f /tmp/test_output
    
    log "INFO" "Test environment cleanup completed"
}

# Test 1: Health Check
test_health_check() {
    test_start "Health Check"
    
    local response=$(curl -s -w "\n%{http_code}" http://localhost:8080/health)
    local http_code=$(echo "$response" | tail -n1)
    local body=$(echo "$response" | head -n-1)
    
    if [[ "$http_code" == "200" ]]; then
        if echo "$body" | grep -q "healthy"; then
            test_pass "Health Check"
        else
            test_fail "Health Check" "Response body does not indicate healthy status"
        fi
    else
        test_fail "Health Check" "HTTP code: $http_code"
    fi
}

# Test 2: API Configuration
test_api_configuration() {
    test_start "API Configuration"
    
    # Test getting configuration
    local response=$(curl -s -w "\n%{http_code}" http://localhost:8080/api/config/vulkan.validation.enabled)
    local http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "200" ]]; then
        test_pass "API Configuration - GET"
    else
        test_fail "API Configuration - GET" "HTTP code: $http_code"
    fi
    
    # Test updating configuration
    response=$(curl -s -X PUT -d "value=false" -w "\n%{http_code}" http://localhost:8080/api/config/vulkan.validation.enabled)
    http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "200" ]]; then
        test_pass "API Configuration - PUT"
    else
        test_fail "API Configuration - PUT" "HTTP code: $http_code"
    fi
}

# Test 3: Error Reporting
test_error_reporting() {
    test_start "Error Reporting"
    
    local response=$(curl -s -X POST -d "error_code=TEST001&error_message=Test error&error_type=test&severity=INFO" \
                    -w "\n%{http_code}" http://localhost:8080/api/errors/report)
    local http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "201" ]]; then
        test_pass "Error Reporting"
    else
        test_fail "Error Reporting" "HTTP code: $http_code"
    fi
}

# Test 4: Performance Metrics
test_performance_metrics() {
    test_start "Performance Metrics"
    
    local response=$(curl -s -X POST -d "metric_name=test_metric&metric_value=42.0&metric_unit=ms&category=performance" \
                    -w "\n%{http_code}" http://localhost:8080/api/metrics/record)
    local http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "201" ]]; then
        test_pass "Performance Metrics"
    else
        test_fail "Performance Metrics" "HTTP code: $http_code"
    fi
}

# Test 5: Vulkan Initialization
test_vulkan_initialization() {
    test_start "Vulkan Initialization"
    
    # This would test actual Vulkan initialization
    # For now, we'll check if the application reports Vulkan status
    local response=$(curl -s http://localhost:8080/api/status)
    
    if echo "$response" | grep -q "vulkan_initialized"; then
        test_pass "Vulkan Initialization"
    else
        test_fail "Vulkan Initialization" "Vulkan status not found in response"
    fi
}

# Test 6: ASCII Conversion
test_ascii_conversion() {
    test_start "ASCII Conversion"
    
    # Create test image
    local test_image="/tmp/test_image.png"
    convert -size 100x100 xc:red "$test_image" 2>/dev/null || {
        test_skip "ASCII Conversion" "ImageMagick not available"
        return
    }
    
    # Test conversion endpoint
    local response=$(curl -s -F "image=@${test_image}" -w "\n%{http_code}" http://localhost:8080/api/ascii/convert)
    local http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "200" ]]; then
        test_pass "ASCII Conversion"
    else
        test_fail "ASCII Conversion" "HTTP code: $http_code"
    fi
    
    # Clean up
    rm -f "$test_image"
}

# Test 7: Error Handling
test_error_handling() {
    test_start "Error Handling"
    
    # Test invalid input
    local response=$(curl -s -w "\n%{http_code}" http://localhost:8080/api/config/invalid<>key)
    local http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "400" ]]; then
        test_pass "Error Handling - Invalid Input"
    else
        test_fail "Error Handling - Invalid Input" "Expected 400, got: $http_code"
    fi
    
    # Test rate limiting
    for i in {1..150}; do
        curl -s http://localhost:8080/api/config/test > /dev/null &
    done
    wait
    
    response=$(curl -s -w "\n%{http_code}" http://localhost:8080/api/config/test)
    http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "429" ]]; then
        test_pass "Error Handling - Rate Limiting"
    else
        test_fail "Error Handling - Rate Limiting" "Expected 429, got: $http_code"
    fi
}

# Test 8: Database Connectivity
test_database_connectivity() {
    test_start "Database Connectivity"
    
    # Test database connection
    if sudo -u postgres psql -d neonglyph_test -c "SELECT 1;" > /dev/null 2>&1; then
        test_pass "Database Connectivity"
    else
        test_fail "Database Connectivity" "Cannot connect to test database"
    fi
}

# Test 9: Security Validation
test_security_validation() {
    test_start "Security Validation"
    
    # Test SQL injection prevention
    local response=$(curl -s -w "\n%{http_code}" "http://localhost:8080/api/config/SELECT%20*%20FROM%20users")
    local http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "400" ]]; then
        test_pass "Security Validation - SQL Injection"
    else
        test_fail "Security Validation - SQL Injection" "Expected 400, got: $http_code"
    fi
    
    # Test XSS prevention
    response=$(curl -s -w "\n%{http_code}" -d "value=<script>alert('xss')</script>" \
              http://localhost:8080/api/config/test)
    http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "400" ]]; then
        test_pass "Security Validation - XSS"
    else
        test_fail "Security Validation - XSS" "Expected 400, got: $http_code"
    fi
}

# Test 10: Performance Load Test
test_performance_load() {
    test_start "Performance Load Test"
    
    # Simple load test with Apache Bench
    if command -v ab > /dev/null; then
        local result=$(ab -n 100 -c 10 http://localhost:8080/health 2>/dev/null | grep "Failed requests")
        if echo "$result" | grep -q "Failed requests:.*0"; then
            test_pass "Performance Load Test"
        else
            test_fail "Performance Load Test" "Failed requests detected: $result"
        fi
    else
        test_skip "Performance Load Test" "Apache Bench not available"
    fi
}

# Test 11: Monitoring Integration
test_monitoring_integration() {
    test_start "Monitoring Integration"
    
    # Check if Prometheus is running
    if systemctl is-active --quiet prometheus; then
        test_pass "Monitoring Integration - Prometheus"
    else
        test_fail "Monitoring Integration - Prometheus" "Prometheus service not running"
    fi
    
    # Check if Grafana is running
    if systemctl is-active --quiet grafana-server; then
        test_pass "Monitoring Integration - Grafana"
    else
        test_fail "Monitoring Integration - Grafana" "Grafana service not running"
    fi
}

# Test 12: Deployment Validation
test_deployment_validation() {
    test_start "Deployment Validation"
    
    # Check if all services are properly configured
    local services=("neonglyph" "prometheus" "grafana-server" "postgresql")
    local all_services_ok=true
    
    for service in "${services[@]}"; do
        if ! systemctl is-active --quiet "$service"; then
            log "ERROR" "Service $service is not running"
            all_services_ok=false
        fi
    done
    
    if $all_services_ok; then
        test_pass "Deployment Validation"
    else
        test_fail "Deployment Validation" "Some services are not running"
    fi
}

# Test 13: Business Logic Validation
test_business_logic_validation() {
    test_start "Business Logic Validation"
    
    # Test configuration validation
    local response=$(curl -s -X PUT -d "value=invalid_boolean" -w "\n%{http_code}" \
                    http://localhost:8080/api/config/vulkan.validation.enabled)
    local http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "400" ]]; then
        test_pass "Business Logic Validation - Configuration"
    else
        test_fail "Business Logic Validation - Configuration" "Expected 400, got: $http_code"
    fi
    
    # Test type validation
    response=$(curl -s -X PUT -d "value=999999" -w "\n%{http_code}" \
              http://localhost:8080/api/config/ascii.font.size)
    http_code=$(echo "$response" | tail -n1)
    
    if [[ "$http_code" == "400" ]]; then
        test_pass "Business Logic Validation - Type Validation"
    else
        test_fail "Business Logic Validation - Type Validation" "Expected 400, got: $http_code"
    fi
}

# Test 14: Error Recovery
test_error_recovery() {
    test_start "Error Recovery"
    
    # Test application recovery from errors
    local pid_file="/var/run/neonglyph.pid"
    
    if [[ -f "$pid_file" ]]; then
        local pid=$(cat "$pid_file")
        
        # Send SIGTERM to simulate graceful shutdown
        kill -TERM "$pid" 2>/dev/null || true
        sleep 5
        
        # Check if service restarted
        if systemctl is-active --quiet neonglyph; then
            test_pass "Error Recovery - Service Restart"
        else
            test_fail "Error Recovery - Service Restart" "Service did not restart after SIGTERM"
        fi
    else
        test_skip "Error Recovery" "PID file not found"
    fi
}

# Test 15: Data Integrity
test_data_integrity() {
    test_start "Data Integrity"
    
    # Test database constraints
    local test_result=$(sudo -u postgres psql -d neonglyph_test -c "
        INSERT INTO app_config (config_key, config_value) VALUES ('test_key', 'test_value');
        INSERT INTO app_config (config_key, config_value) VALUES ('test_key', 'duplicate_value');
    " 2>&1 || true)
    
    if echo "$test_result" | grep -q "duplicate key"; then
        test_pass "Data Integrity - Unique Constraints"
    else
        test_fail "Data Integrity - Unique Constraints" "Unique constraint not enforced"
    fi
}

# Run all tests
run_all_tests() {
    log "INFO" "Starting comprehensive test suite..."
    
    setup_test_environment
    
    # Run tests
    test_health_check
    test_api_configuration
    test_error_reporting
    test_performance_metrics
    test_vulkan_initialization
    test_ascii_conversion
    test_error_handling
    test_database_connectivity
    test_security_validation
    test_performance_load
    test_monitoring_integration
    test_deployment_validation
    test_business_logic_validation
    test_error_recovery
    test_data_integrity
    
    cleanup_test_environment
}

# Generate test report
generate_test_report() {
    log "INFO" "Generating test report..."
    
    local report_file="${TEST_RESULTS_DIR}/test_report_$(date +%Y%m%d_%H%M%S).html"
    
    cat > "$report_file" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>NeonGlyph Production Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .summary { margin: 20px 0; }
        .test-results { margin: 20px 0; }
        .pass { color: green; }
        .fail { color: red; }
        .skip { color: orange; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <div class="header">
        <h1>NeonGlyph Production Test Report</h1>
        <p>Generated on: $(date)</p>
        <p>Test Suite: ${TEST_SUITE_NAME}</p>
    </div>
    
    <div class="summary">
        <h2>Test Summary</h2>
        <p>Total Tests: ${TESTS_RUN}</p>
        <p class="pass">Passed: ${TESTS_PASSED}</p>
        <p class="fail">Failed: ${TESTS_FAILED}</p>
        <p class="skip">Skipped: ${TESTS_SKIPPED}</p>
        <p>Success Rate: $(( TESTS_PASSED * 100 / TESTS_RUN ))%</p>
    </div>
    
    <div class="test-results">
        <h2>Test Results</h2>
        <table>
            <tr>
                <th>Test Name</th>
                <th>Status</th>
                <th>Details</th>
            </tr>
EOF
    
    # Parse test results from log
    grep -E "(PASS|FAIL|SKIP)" "${TEST_LOG}" | while read -r line; do
        if echo "$line" | grep -q "PASS"; then
            echo "            <tr><td>$(echo "$line" | cut -d' ' -f3-)</td><td class=\"pass\">PASS</td><td></td></tr>" >> "$report_file"
        elif echo "$line" | grep -q "FAIL"; then
            echo "            <tr><td>$(echo "$line" | cut -d' ' -f3- | cut -d':' -f1)</td><td class=\"fail\">FAIL</td><td>$(echo "$line" | cut -d':' -f2-)</td></tr>" >> "$report_file"
        elif echo "$line" | grep -q "SKIP"; then
            echo "            <tr><td>$(echo "$line" | cut -d' ' -f3- | cut -d':' -f1)</td><td class=\"skip\">SKIP</td><td>$(echo "$line" | cut -d':' -f2-)</td></tr>" >> "$report_file"
        fi
    done
    
    cat >> "$report_file" << EOF
        </table>
    </div>
    
    <div class="footer">
        <p>Log file: ${TEST_LOG}</p>
        <p>Results directory: ${TEST_RESULTS_DIR}</p>
    </div>
</body>
</html>
EOF
    
    log "INFO" "Test report generated: $report_file"
}

# Main execution
main() {
    log "INFO" "Starting NeonGlyph Production E2E Test Suite..."
    
    run_all_tests
    
    log "INFO" "Test suite completed!"
    log "INFO" "Results: ${TESTS_PASSED}/${TESTS_RUN} passed, ${TESTS_FAILED} failed, ${TESTS_SKIPPED} skipped"
    
    generate_test_report
    
    # Exit with appropriate code
    if [[ $TESTS_FAILED -eq 0 ]]; then
        log "INFO" "All tests passed!"
        exit 0
    else
        log "ERROR" "Some tests failed!"
        exit 1
    fi
}

# Execute main function
main "$@"