-- NeonGlyph Production Database Schema
-- Comprehensive schema for application metrics, error tracking, and performance monitoring

-- Application configuration table
CREATE TABLE IF NOT EXISTS app_config (
    id SERIAL PRIMARY KEY,
    config_key VARCHAR(255) UNIQUE NOT NULL,
    config_value TEXT,
    config_type VARCHAR(50) DEFAULT 'string',
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT true
);

-- Runtime error tracking
CREATE TABLE IF NOT EXISTS runtime_errors (
    id SERIAL PRIMARY KEY,
    error_code VARCHAR(50) NOT NULL,
    error_message TEXT NOT NULL,
    error_type VARCHAR(100) NOT NULL,
    severity VARCHAR(20) NOT NULL CHECK (severity IN ('CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG')),
    stack_trace TEXT,
    file_path VARCHAR(500),
    line_number INTEGER,
    function_name VARCHAR(255),
    vulkan_context JSONB,
    system_info JSONB,
    session_id VARCHAR(100),
    user_agent TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    resolved_at TIMESTAMP,
    resolution_notes TEXT,
    is_resolved BOOLEAN DEFAULT false
);

-- Performance metrics
CREATE TABLE IF NOT EXISTS performance_metrics (
    id SERIAL PRIMARY KEY,
    metric_name VARCHAR(100) NOT NULL,
    metric_value DECIMAL(15,6) NOT NULL,
    metric_unit VARCHAR(20),
    category VARCHAR(50) NOT NULL,
    tags JSONB,
    session_id VARCHAR(100),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    percentile_95 DECIMAL(15,6),
    percentile_99 DECIMAL(15,6),
    min_value DECIMAL(15,6),
    max_value DECIMAL(15,6),
    sample_count INTEGER
);

-- Vulkan device information
CREATE TABLE IF NOT EXISTS vulkan_devices (
    id SERIAL PRIMARY KEY,
    device_name VARCHAR(255) NOT NULL,
    device_type VARCHAR(50) NOT NULL,
    vendor_id INTEGER,
    device_id INTEGER,
    driver_version VARCHAR(100),
    api_version VARCHAR(100),
    memory_mb INTEGER,
    queue_families JSONB,
    extensions JSONB,
    features JSONB,
    is_primary BOOLEAN DEFAULT false,
    is_available BOOLEAN DEFAULT true,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Application sessions
CREATE TABLE IF NOT EXISTS app_sessions (
    id SERIAL PRIMARY KEY,
    session_id VARCHAR(100) UNIQUE NOT NULL,
    start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    end_time TIMESTAMP,
    duration_ms INTEGER,
    status VARCHAR(50) DEFAULT 'active',
    vulkan_initialized BOOLEAN DEFAULT false,
    window_created BOOLEAN DEFAULT false,
    ascii_initialized BOOLEAN DEFAULT false,
    error_count INTEGER DEFAULT 0,
    warning_count INTEGER DEFAULT 0,
    system_info JSONB,
    config_used JSONB,
    performance_summary JSONB
);

-- ASCII conversion metrics
CREATE TABLE IF NOT EXISTS ascii_conversion_metrics (
    id SERIAL PRIMARY KEY,
    session_id VARCHAR(100) NOT NULL,
    input_format VARCHAR(50) NOT NULL,
    output_format VARCHAR(50) NOT NULL,
    input_size_bytes INTEGER NOT NULL,
    output_size_bytes INTEGER NOT NULL,
    processing_time_ms INTEGER NOT NULL,
    characters_processed INTEGER NOT NULL,
    font_used VARCHAR(100),
    palette_used VARCHAR(100),
    quality_score DECIMAL(5,2),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Security audit log
CREATE TABLE IF NOT EXISTS security_audit_log (
    id SERIAL PRIMARY KEY,
    event_type VARCHAR(100) NOT NULL,
    event_description TEXT NOT NULL,
    user_identifier VARCHAR(255),
    ip_address INET,
    user_agent TEXT,
    resource_accessed VARCHAR(500),
    action_taken VARCHAR(100),
    success BOOLEAN DEFAULT true,
    additional_data JSONB,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Deployment history
CREATE TABLE IF NOT EXISTS deployment_history (
    id SERIAL PRIMARY KEY,
    deployment_id VARCHAR(100) UNIQUE NOT NULL,
    version VARCHAR(50) NOT NULL,
    environment VARCHAR(50) NOT NULL,
    deployment_type VARCHAR(50) NOT NULL,
    status VARCHAR(50) DEFAULT 'pending',
    deployed_by VARCHAR(255),
    deployment_notes TEXT,
    rollback_info JSONB,
    start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    end_time TIMESTAMP,
    validation_results JSONB,
    performance_baseline JSONB
);

-- Create indexes for performance
CREATE INDEX IF NOT EXISTS idx_runtime_errors_severity ON runtime_errors(severity);
CREATE INDEX IF NOT EXISTS idx_runtime_errors_created_at ON runtime_errors(created_at);
CREATE INDEX IF NOT EXISTS idx_runtime_errors_session_id ON runtime_errors(session_id);
CREATE INDEX IF NOT EXISTS idx_performance_metrics_name ON performance_metrics(metric_name);
CREATE INDEX IF NOT EXISTS idx_performance_metrics_timestamp ON performance_metrics(timestamp);
CREATE INDEX IF NOT EXISTS idx_performance_metrics_category ON performance_metrics(category);
CREATE INDEX IF NOT EXISTS idx_app_sessions_status ON app_sessions(status);
CREATE INDEX IF NOT EXISTS idx_app_sessions_start_time ON app_sessions(start_time);
CREATE INDEX IF NOT EXISTS idx_ascii_conversion_session ON ascii_conversion_metrics(session_id);
CREATE INDEX IF NOT EXISTS idx_security_audit_type ON security_audit_log(event_type);
CREATE INDEX IF NOT EXISTS idx_security_audit_created ON security_audit_log(created_at);

-- Insert default configuration values
INSERT INTO app_config (config_key, config_value, config_type, description) VALUES
('vulkan.validation.enabled', 'true', 'boolean', 'Enable Vulkan validation layers in debug builds'),
('vulkan.multi_gpu.safety_mode', 'true', 'boolean', 'Enable multi-GPU safety mode for compatibility'),
('logging.level', 'INFO', 'string', 'Application logging level (TRACE, DEBUG, INFO, WARNING, ERROR)'),
('performance.monitoring.enabled', 'true', 'boolean', 'Enable performance metrics collection'),
('error.tracking.enabled', 'true', 'boolean', 'Enable runtime error tracking and reporting'),
('ascii.font.size', '12', 'integer', 'Default font size for ASCII conversion'),
('ascii.quality.threshold', '0.8', 'float', 'Minimum quality threshold for ASCII output'),
('security.audit.enabled', 'true', 'boolean', 'Enable security audit logging'),
('deployment.auto_rollback.enabled', 'true', 'boolean', 'Enable automatic rollback on deployment failure'),
('monitoring.alert.threshold.error_rate', '0.05', 'float', 'Error rate threshold for alerts (5%)'),
('monitoring.alert.threshold.response_time', '1000', 'integer', 'Response time threshold for alerts (ms)');

-- Create views for common queries
CREATE OR REPLACE VIEW active_sessions AS
SELECT * FROM app_sessions WHERE status = 'active';

CREATE OR REPLACE VIEW recent_errors AS
SELECT * FROM runtime_errors 
WHERE created_at > CURRENT_TIMESTAMP - INTERVAL '24 hours'
ORDER BY created_at DESC;

CREATE OR REPLACE VIEW performance_summary AS
SELECT 
    metric_name,
    AVG(metric_value) as avg_value,
    MIN(metric_value) as min_value,
    MAX(metric_value) as max_value,
    COUNT(*) as sample_count,
    DATE_TRUNC('hour', timestamp) as hour_bucket
FROM performance_metrics 
WHERE timestamp > CURRENT_TIMESTAMP - INTERVAL '7 days'
GROUP BY metric_name, DATE_TRUNC('hour', timestamp)
ORDER BY hour_bucket DESC;

CREATE OR REPLACE VIEW deployment_status AS
SELECT 
    deployment_id,
    version,
    environment,
    status,
    start_time,
    end_time,
    EXTRACT(EPOCH FROM (end_time - start_time)) as duration_seconds,
    validation_results->>'overall_status' as validation_status
FROM deployment_history
ORDER BY start_time DESC;