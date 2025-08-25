# Practical Examples and Use Cases

Real-world examples and practical use cases for the `pg_hello` extension, demonstrating how to leverage its features in various scenarios.

## 📋 Table of Contents

- [Basic Usage Examples](#basic-usage-examples)
- [User Management and Onboarding](#user-management-and-onboarding)
- [Logging and Auditing](#logging-and-auditing)
- [Performance Testing](#performance-testing)
- [Configuration Management](#configuration-management)
- [Integration Patterns](#integration-patterns)
- [Advanced Use Cases](#advanced-use-cases)
- [Real-World Applications](#real-world-applications)

## 🚀 Basic Usage Examples

### Simple Greeting System

```sql
-- Create a users table with automatic greetings
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) NOT NULL,
    full_name VARCHAR(100),
    greeting_message TEXT,
    created_at BIGINT DEFAULT now_ms(),
    last_login BIGINT
);

-- Function to generate personalized greetings
CREATE OR REPLACE FUNCTION generate_user_greeting(user_name TEXT, is_first_login BOOLEAN DEFAULT FALSE)
RETURNS TEXT AS $$
DECLARE
    greeting_text TEXT;
    prefix TEXT;
BEGIN
    -- Set greeting style based on login status
    IF is_first_login THEN
        SET pg_hello.repeat = 1;
        prefix := 'Welcome';
    ELSE
        SET pg_hello.repeat = 2;
        prefix := 'Welcome back';
    END IF;
    
    -- Generate greeting
    SELECT pg_hello(user_name) INTO greeting_text;
    
    -- Customize the greeting
    greeting_text := REPLACE(greeting_text, 'Hello', prefix);
    
    RETURN greeting_text;
END;
$$ LANGUAGE plpgsql;

-- Usage examples
INSERT INTO users (username, email, full_name) VALUES 
    ('alice', 'alice@example.com', 'Alice Johnson'),
    ('bob', 'bob@example.com', 'Bob Smith'),
    ('carol', 'carol@example.com', 'Carol Davis');

-- Update greetings for new users
UPDATE users 
SET greeting_message = generate_user_greeting(username, TRUE)
WHERE greeting_message IS NULL;

-- View results
SELECT username, greeting_message FROM users;
```

### Dynamic Timestamp Logging

```sql
-- Create activity log with millisecond precision
CREATE TABLE activity_log (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    action VARCHAR(50) NOT NULL,
    description TEXT,
    timestamp_ms BIGINT DEFAULT now_ms(),
    readable_timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Function to log user activities
CREATE OR REPLACE FUNCTION log_user_activity(
    p_user_id INTEGER,
    p_action VARCHAR(50),
    p_description TEXT DEFAULT NULL
)
RETURNS BIGINT AS $$
DECLARE
    log_timestamp BIGINT;
BEGIN
    -- Get precise timestamp
    SELECT now_ms() INTO log_timestamp;
    
    -- Insert log entry
    INSERT INTO activity_log (user_id, action, description, timestamp_ms)
    VALUES (p_user_id, p_action, p_description, log_timestamp);
    
    RETURN log_timestamp;
END;
$$ LANGUAGE plpgsql;

-- Usage examples
SELECT log_user_activity(1, 'LOGIN', 'User logged in from web interface');
SELECT log_user_activity(1, 'VIEW_PROFILE', 'User viewed their profile page');
SELECT log_user_activity(2, 'UPDATE_SETTINGS', 'User changed email preferences');

-- Query recent activity
SELECT 
    u.username,
    al.action,
    al.description,
    al.timestamp_ms,
    al.readable_timestamp
FROM activity_log al
JOIN users u ON u.id = al.user_id
ORDER BY al.timestamp_ms DESC
LIMIT 10;
```

## 👥 User Management and Onboarding

### Welcome Email System

```sql
-- Email queue table
CREATE TABLE email_queue (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    email_type VARCHAR(50) NOT NULL,
    recipient_email VARCHAR(100) NOT NULL,
    subject TEXT NOT NULL,
    body TEXT NOT NULL,
    personalized_greeting TEXT,
    status VARCHAR(20) DEFAULT 'PENDING',
    created_at BIGINT DEFAULT now_ms(),
    sent_at BIGINT NULL
);

-- Function to queue welcome emails
CREATE OR REPLACE FUNCTION queue_welcome_email(user_id INTEGER)
RETURNS BOOLEAN AS $$
DECLARE
    user_record users%ROWTYPE;
    welcome_greeting TEXT;
    email_subject TEXT;
    email_body TEXT;
BEGIN
    -- Get user information
    SELECT * INTO user_record FROM users WHERE id = user_id;
    
    IF NOT FOUND THEN
        RAISE EXCEPTION 'User not found: %', user_id;
    END IF;
    
    -- Generate personalized greeting
    SET pg_hello.repeat = 1;
    SELECT pg_hello(user_record.full_name) INTO welcome_greeting;
    
    -- Create email content
    email_subject := 'Welcome to Our Platform!';
    email_body := format(
        '%s

We''re excited to have you join our community!

Your account details:
- Username: %s
- Email: %s
- Account created: %s

Get started by exploring our features and connecting with other users.

Best regards,
The Team',
        welcome_greeting,
        user_record.username,
        user_record.email,
        to_timestamp(user_record.created_at / 1000)
    );
    
    -- Queue email
    INSERT INTO email_queue (
        user_id, 
        email_type, 
        recipient_email, 
        subject, 
        body, 
        personalized_greeting
    ) VALUES (
        user_id,
        'WELCOME',
        user_record.email,
        email_subject,
        email_body,
        welcome_greeting
    );
    
    RETURN TRUE;
END;
$$ LANGUAGE plpgsql;

-- Trigger to automatically queue welcome emails
CREATE OR REPLACE FUNCTION trigger_welcome_email()
RETURNS TRIGGER AS $$
BEGIN
    -- Queue welcome email for new users
    PERFORM queue_welcome_email(NEW.id);
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER user_welcome_email_trigger
    AFTER INSERT ON users
    FOR EACH ROW EXECUTE FUNCTION trigger_welcome_email();

-- Test the system
INSERT INTO users (username, email, full_name) VALUES 
    ('dave', 'dave@example.com', 'Dave Wilson');

-- Check queued emails
SELECT 
    u.username,
    eq.email_type,
    eq.subject,
    eq.personalized_greeting,
    eq.status
FROM email_queue eq
JOIN users u ON u.id = eq.user_id
ORDER BY eq.created_at DESC;
```

### User Onboarding Progress

```sql
-- Onboarding steps tracking
CREATE TABLE onboarding_steps (
    id SERIAL PRIMARY KEY,
    step_name VARCHAR(50) NOT NULL,
    step_description TEXT NOT NULL,
    step_order INTEGER NOT NULL,
    is_required BOOLEAN DEFAULT TRUE
);

CREATE TABLE user_onboarding_progress (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    step_id INTEGER REFERENCES onboarding_steps(id),
    completed_at BIGINT NULL,
    completion_message TEXT,
    UNIQUE(user_id, step_id)
);

-- Insert default onboarding steps
INSERT INTO onboarding_steps (step_name, step_description, step_order, is_required) VALUES
    ('PROFILE_SETUP', 'Complete your profile information', 1, TRUE),
    ('EMAIL_VERIFICATION', 'Verify your email address', 2, TRUE),
    ('FIRST_LOGIN', 'Log in to your account', 3, TRUE),
    ('TUTORIAL_COMPLETE', 'Complete the getting started tutorial', 4, FALSE),
    ('FIRST_ACTION', 'Perform your first meaningful action', 5, FALSE);

-- Function to mark onboarding step as complete
CREATE OR REPLACE FUNCTION complete_onboarding_step(
    p_user_id INTEGER,
    p_step_name VARCHAR(50)
)
RETURNS TEXT AS $$
DECLARE
    step_record onboarding_steps%ROWTYPE;
    user_record users%ROWTYPE;
    completion_timestamp BIGINT;
    congratulations_message TEXT;
BEGIN
    -- Get step information
    SELECT * INTO step_record 
    FROM onboarding_steps 
    WHERE step_name = p_step_name;
    
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Onboarding step not found: %', p_step_name;
    END IF;
    
    -- Get user information
    SELECT * INTO user_record FROM users WHERE id = p_user_id;
    
    -- Get completion timestamp
    SELECT now_ms() INTO completion_timestamp;
    
    -- Generate congratulations message
    SET pg_hello.repeat = 1;
    SELECT pg_hello(user_record.username || ', congratulations') INTO congratulations_message;
    congratulations_message := congratulations_message || ' You completed: ' || step_record.step_description;
    
    -- Mark step as complete
    INSERT INTO user_onboarding_progress (user_id, step_id, completed_at, completion_message)
    VALUES (p_user_id, step_record.id, completion_timestamp, congratulations_message)
    ON CONFLICT (user_id, step_id) DO UPDATE SET
        completed_at = completion_timestamp,
        completion_message = congratulations_message;
    
    RETURN congratulations_message;
END;
$$ LANGUAGE plpgsql;

-- Function to get onboarding progress
CREATE OR REPLACE FUNCTION get_onboarding_progress(p_user_id INTEGER)
RETURNS TABLE (
    step_name VARCHAR(50),
    step_description TEXT,
    is_required BOOLEAN,
    is_completed BOOLEAN,
    completed_at TIMESTAMP,
    completion_message TEXT
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        os.step_name,
        os.step_description,
        os.is_required,
        (uop.completed_at IS NOT NULL) as is_completed,
        to_timestamp(uop.completed_at / 1000) as completed_at,
        uop.completion_message
    FROM onboarding_steps os
    LEFT JOIN user_onboarding_progress uop ON os.id = uop.step_id AND uop.user_id = p_user_id
    ORDER BY os.step_order;
END;
$$ LANGUAGE plpgsql;

-- Test the onboarding system
SELECT complete_onboarding_step(1, 'PROFILE_SETUP');
SELECT complete_onboarding_step(1, 'EMAIL_VERIFICATION');

-- Check progress
SELECT * FROM get_onboarding_progress(1);
```

## 📊 Logging and Auditing

### Comprehensive Audit System

```sql
-- Audit log table
CREATE TABLE audit_log (
    id SERIAL PRIMARY KEY,
    timestamp_ms BIGINT DEFAULT now_ms(),
    readable_timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    user_id INTEGER,
    username VARCHAR(50),
    action VARCHAR(100) NOT NULL,
    table_name VARCHAR(50),
    record_id INTEGER,
    old_values JSONB,
    new_values JSONB,
    description TEXT,
    session_id TEXT,
    ip_address INET,
    user_agent TEXT
);

-- Function to create audit entries
CREATE OR REPLACE FUNCTION create_audit_entry(
    p_action VARCHAR(100),
    p_table_name VARCHAR(50) DEFAULT NULL,
    p_record_id INTEGER DEFAULT NULL,
    p_old_values JSONB DEFAULT NULL,
    p_new_values JSONB DEFAULT NULL,
    p_description TEXT DEFAULT NULL
)
RETURNS BIGINT AS $$
DECLARE
    audit_timestamp BIGINT;
    current_user_id INTEGER;
    current_username VARCHAR(50);
    audit_description TEXT;
BEGIN
    -- Get current timestamp
    SELECT now_ms() INTO audit_timestamp;
    
    -- Get current user info (simplified - in real app would come from session)
    current_user_id := COALESCE(current_setting('app.current_user_id', TRUE)::INTEGER, 0);
    current_username := COALESCE(current_setting('app.current_username', TRUE), session_user);
    
    -- Create descriptive audit message
    IF p_description IS NULL THEN
        audit_description := format('User %s performed %s', current_username, p_action);
        IF p_table_name IS NOT NULL THEN
            audit_description := audit_description || format(' on %s', p_table_name);
        END IF;
        IF p_record_id IS NOT NULL THEN
            audit_description := audit_description || format(' (ID: %s)', p_record_id);
        END IF;
    ELSE
        audit_description := p_description;
    END IF;
    
    -- Insert audit record
    INSERT INTO audit_log (
        timestamp_ms,
        user_id,
        username,
        action,
        table_name,
        record_id,
        old_values,
        new_values,
        description,
        session_id,
        ip_address
    ) VALUES (
        audit_timestamp,
        current_user_id,
        current_username,
        p_action,
        p_table_name,
        p_record_id,
        p_old_values,
        p_new_values,
        audit_description,
        COALESCE(current_setting('app.session_id', TRUE), 'unknown'),
        COALESCE(inet_client_addr(), '127.0.0.1'::inet)
    );
    
    RETURN audit_timestamp;
END;
$$ LANGUAGE plpgsql;

-- Audit trigger for users table
CREATE OR REPLACE FUNCTION audit_users_changes()
RETURNS TRIGGER AS $$
DECLARE
    old_json JSONB;
    new_json JSONB;
BEGIN
    IF TG_OP = 'DELETE' THEN
        old_json := row_to_json(OLD)::jsonb;
        PERFORM create_audit_entry('DELETE', 'users', OLD.id, old_json, NULL);
        RETURN OLD;
    ELSIF TG_OP = 'UPDATE' THEN
        old_json := row_to_json(OLD)::jsonb;
        new_json := row_to_json(NEW)::jsonb;
        PERFORM create_audit_entry('UPDATE', 'users', NEW.id, old_json, new_json);
        RETURN NEW;
    ELSIF TG_OP = 'INSERT' THEN
        new_json := row_to_json(NEW)::jsonb;
        PERFORM create_audit_entry('INSERT', 'users', NEW.id, NULL, new_json);
        RETURN NEW;
    END IF;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER users_audit_trigger
    AFTER INSERT OR UPDATE OR DELETE ON users
    FOR EACH ROW EXECUTE FUNCTION audit_users_changes();

-- Function to generate audit reports
CREATE OR REPLACE FUNCTION generate_audit_report(
    start_date DATE DEFAULT CURRENT_DATE - INTERVAL '7 days',
    end_date DATE DEFAULT CURRENT_DATE
)
RETURNS TABLE (
    action_summary TEXT,
    action_count BIGINT,
    unique_users BIGINT,
    first_occurrence TIMESTAMP,
    last_occurrence TIMESTAMP
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        al.action as action_summary,
        COUNT(*) as action_count,
        COUNT(DISTINCT al.user_id) as unique_users,
        MIN(to_timestamp(al.timestamp_ms / 1000)) as first_occurrence,
        MAX(to_timestamp(al.timestamp_ms / 1000)) as last_occurrence
    FROM audit_log al
    WHERE to_timestamp(al.timestamp_ms / 1000)::date BETWEEN start_date AND end_date
    GROUP BY al.action
    ORDER BY action_count DESC;
END;
$$ LANGUAGE plpgsql;

-- Test the audit system
-- Set user context (in real app, this would be set by application)
SELECT set_config('app.current_user_id', '1', FALSE);
SELECT set_config('app.current_username', 'alice', FALSE);
SELECT set_config('app.session_id', 'sess_' || extract(epoch from now())::text, FALSE);

-- Perform some actions that will be audited
UPDATE users SET email = 'alice.new@example.com' WHERE username = 'alice';
INSERT INTO users (username, email, full_name) VALUES ('eve', 'eve@example.com', 'Eve Brown');

-- View audit trail
SELECT 
    readable_timestamp,
    username,
    action,
    description
FROM audit_log
ORDER BY timestamp_ms DESC
LIMIT 10;

-- Generate audit report
SELECT * FROM generate_audit_report();
```

## 🔧 Performance Testing

### Load Testing Framework

```sql
-- Performance test configuration
CREATE TABLE performance_test_config (
    test_name VARCHAR(50) PRIMARY KEY,
    function_name VARCHAR(50) NOT NULL,
    test_data_generator TEXT NOT NULL,
    expected_max_time_ms NUMERIC DEFAULT 10.0,
    test_iterations INTEGER DEFAULT 1000,
    warmup_iterations INTEGER DEFAULT 100
);

-- Performance test results
CREATE TABLE performance_test_results (
    id SERIAL PRIMARY KEY,
    test_name VARCHAR(50) REFERENCES performance_test_config(test_name),
    test_timestamp BIGINT DEFAULT now_ms(),
    total_iterations INTEGER,
    total_time_ms NUMERIC,
    avg_time_ms NUMERIC,
    min_time_ms NUMERIC,
    max_time_ms NUMERIC,
    percentile_95_ms NUMERIC,
    percentile_99_ms NUMERIC,
    test_passed BOOLEAN
);

-- Insert test configurations
INSERT INTO performance_test_config (test_name, function_name, test_data_generator, expected_max_time_ms, test_iterations) VALUES
    ('pg_hello_basic', 'pg_hello', '''Test User'' || i::text', 5.0, 1000),
    ('pg_hello_repeat', 'pg_hello', '''Repeat Test'' || i::text', 15.0, 500),
    ('now_ms_basic', 'now_ms', '''''', 2.0, 2000),
    ('spi_version_basic', 'spi_version', '''''', 50.0, 100);

-- Performance testing function
CREATE OR REPLACE FUNCTION run_performance_test(p_test_name VARCHAR(50))
RETURNS performance_test_results AS $$
DECLARE
    test_config performance_test_config%ROWTYPE;
    result_record performance_test_results%ROWTYPE;
    start_time BIGINT;
    end_time BIGINT;
    iteration_times NUMERIC[];
    i INTEGER;
    test_query TEXT;
    test_data TEXT;
BEGIN
    -- Get test configuration
    SELECT * INTO test_config FROM performance_test_config WHERE test_name = p_test_name;
    
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Performance test not found: %', p_test_name;
    END IF;
    
    -- Initialize results
    result_record.test_name := p_test_name;
    result_record.test_timestamp := now_ms();
    result_record.total_iterations := test_config.test_iterations;
    
    -- Warmup phase
    FOR i IN 1..test_config.warmup_iterations LOOP
        EXECUTE format('SELECT %s(%s)', 
            test_config.function_name,
            CASE 
                WHEN test_config.test_data_generator = '''''' THEN ''
                ELSE replace(test_config.test_data_generator, 'i', i::text)
            END
        );
    END LOOP;
    
    -- Main test phase
    iteration_times := ARRAY[]::NUMERIC[];
    start_time := now_ms();
    
    FOR i IN 1..test_config.test_iterations LOOP
        DECLARE
            iter_start BIGINT;
            iter_end BIGINT;
        BEGIN
            iter_start := now_ms();
            
            EXECUTE format('SELECT %s(%s)', 
                test_config.function_name,
                CASE 
                    WHEN test_config.test_data_generator = '''''' THEN ''
                    ELSE replace(test_config.test_data_generator, 'i', i::text)
                END
            );
            
            iter_end := now_ms();
            iteration_times := array_append(iteration_times, iter_end - iter_start);
        END;
    END LOOP;
    
    end_time := now_ms();
    
    -- Calculate statistics
    result_record.total_time_ms := end_time - start_time;
    result_record.avg_time_ms := (SELECT AVG(t) FROM unnest(iteration_times) AS t);
    result_record.min_time_ms := (SELECT MIN(t) FROM unnest(iteration_times) AS t);
    result_record.max_time_ms := (SELECT MAX(t) FROM unnest(iteration_times) AS t);
    
    -- Calculate percentiles
    WITH sorted_times AS (
        SELECT t, ROW_NUMBER() OVER (ORDER BY t) as rn, COUNT(*) OVER () as total
        FROM unnest(iteration_times) AS t
    )
    SELECT 
        MAX(CASE WHEN rn <= total * 0.95 THEN t END),
        MAX(CASE WHEN rn <= total * 0.99 THEN t END)
    INTO result_record.percentile_95_ms, result_record.percentile_99_ms
    FROM sorted_times;
    
    -- Determine if test passed
    result_record.test_passed := result_record.avg_time_ms <= test_config.expected_max_time_ms;
    
    -- Save results
    INSERT INTO performance_test_results SELECT result_record.*;
    
    RETURN result_record;
END;
$$ LANGUAGE plpgsql;

-- Function to run all performance tests
CREATE OR REPLACE FUNCTION run_all_performance_tests()
RETURNS TABLE (
    test_name VARCHAR(50),
    avg_time_ms NUMERIC,
    percentile_95_ms NUMERIC,
    test_passed BOOLEAN,
    performance_grade TEXT
) AS $$
DECLARE
    test_config RECORD;
    test_result performance_test_results%ROWTYPE;
BEGIN
    FOR test_config IN SELECT * FROM performance_test_config ORDER BY test_name LOOP
        test_result := run_performance_test(test_config.test_name);
        
        RETURN QUERY SELECT 
            test_result.test_name,
            test_result.avg_time_ms,
            test_result.percentile_95_ms,
            test_result.test_passed,
            CASE 
                WHEN test_result.avg_time_ms <= test_config.expected_max_time_ms * 0.5 THEN 'Excellent'
                WHEN test_result.avg_time_ms <= test_config.expected_max_time_ms * 0.8 THEN 'Good'
                WHEN test_result.avg_time_ms <= test_config.expected_max_time_ms THEN 'Acceptable'
                ELSE 'Poor'
            END as performance_grade;
    END LOOP;
END;
$$ LANGUAGE plpgsql;

-- Run performance tests
SELECT * FROM run_all_performance_tests();

-- View performance trends
SELECT 
    test_name,
    to_timestamp(test_timestamp / 1000) as test_date,
    avg_time_ms,
    percentile_95_ms,
    test_passed
FROM performance_test_results
ORDER BY test_name, test_timestamp DESC;
```

## ⚙️ Configuration Management

### Dynamic Configuration System

```sql
-- Configuration management table
CREATE TABLE extension_config (
    id SERIAL PRIMARY KEY,
    config_key VARCHAR(100) NOT NULL,
    config_value TEXT NOT NULL,
    config_type VARCHAR(20) DEFAULT 'string',
    description TEXT,
    min_value NUMERIC NULL,
    max_value NUMERIC NULL,
    allowed_values TEXT[] NULL,
    created_at BIGINT DEFAULT now_ms(),
    updated_at BIGINT DEFAULT now_ms(),
    updated_by VARCHAR(50),
    is_active BOOLEAN DEFAULT TRUE,
    UNIQUE(config_key)
);

-- Configuration history table
CREATE TABLE extension_config_history (
    id SERIAL PRIMARY KEY,
    config_key VARCHAR(100) NOT NULL,
    old_value TEXT,
    new_value TEXT,
    changed_at BIGINT DEFAULT now_ms(),
    changed_by VARCHAR(50),
    change_reason TEXT
);

-- Insert default configurations
INSERT INTO extension_config (config_key, config_value, config_type, description, min_value, max_value) VALUES
    ('pg_hello.default_repeat', '1', 'integer', 'Default repeat count for greetings', 1, 10),
    ('pg_hello.max_name_length', '50', 'integer', 'Maximum length for names in greetings', 1, 255),
    ('pg_hello.greeting_prefix', 'Hello', 'string', 'Default greeting prefix', NULL, NULL),
    ('pg_hello.enable_logging', 'true', 'boolean', 'Enable greeting function logging', NULL, NULL),
    ('pg_hello.cache_ttl_seconds', '300', 'integer', 'Cache TTL for greetings in seconds', 60, 3600);

-- Function to get configuration value
CREATE OR REPLACE FUNCTION get_config_value(p_config_key VARCHAR(100))
RETURNS TEXT AS $$
DECLARE
    config_value TEXT;
BEGIN
    SELECT ec.config_value INTO config_value
    FROM extension_config ec
    WHERE ec.config_key = p_config_key AND ec.is_active = TRUE;
    
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Configuration key not found: %', p_config_key;
    END IF;
    
    RETURN config_value;
END;
$$ LANGUAGE plpgsql;

-- Function to update configuration
CREATE OR REPLACE FUNCTION update_config_value(
    p_config_key VARCHAR(100),
    p_new_value TEXT,
    p_changed_by VARCHAR(50) DEFAULT 'system',
    p_change_reason TEXT DEFAULT NULL
)
RETURNS BOOLEAN AS $$
DECLARE
    old_value TEXT;
    config_rec extension_config%ROWTYPE;
    validation_error TEXT;
BEGIN
    -- Get current configuration
    SELECT * INTO config_rec
    FROM extension_config
    WHERE config_key = p_config_key AND is_active = TRUE;
    
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Configuration key not found: %', p_config_key;
    END IF;
    
    old_value := config_rec.config_value;
    
    -- Validate new value based on type
    CASE config_rec.config_type
        WHEN 'integer' THEN
            BEGIN
                -- Check if it's a valid integer
                PERFORM p_new_value::INTEGER;
                
                -- Check range if specified
                IF config_rec.min_value IS NOT NULL AND p_new_value::INTEGER < config_rec.min_value THEN
                    validation_error := format('Value %s is below minimum %s', p_new_value, config_rec.min_value);
                END IF;
                
                IF config_rec.max_value IS NOT NULL AND p_new_value::INTEGER > config_rec.max_value THEN
                    validation_error := format('Value %s is above maximum %s', p_new_value, config_rec.max_value);
                END IF;
            EXCEPTION
                WHEN invalid_text_representation THEN
                    validation_error := format('Invalid integer value: %s', p_new_value);
            END;
            
        WHEN 'boolean' THEN
            IF p_new_value NOT IN ('true', 'false', '1', '0', 'yes', 'no', 'on', 'off') THEN
                validation_error := format('Invalid boolean value: %s', p_new_value);
            END IF;
            
        WHEN 'string' THEN
            -- Check allowed values if specified
            IF config_rec.allowed_values IS NOT NULL AND NOT (p_new_value = ANY(config_rec.allowed_values)) THEN
                validation_error := format('Value must be one of: %s', array_to_string(config_rec.allowed_values, ', '));
            END IF;
    END CASE;
    
    -- Raise error if validation failed
    IF validation_error IS NOT NULL THEN
        RAISE EXCEPTION 'Configuration validation failed: %', validation_error;
    END IF;
    
    -- Update configuration
    UPDATE extension_config
    SET 
        config_value = p_new_value,
        updated_at = now_ms(),
        updated_by = p_changed_by
    WHERE config_key = p_config_key;
    
    -- Record history
    INSERT INTO extension_config_history (config_key, old_value, new_value, changed_by, change_reason)
    VALUES (p_config_key, old_value, p_new_value, p_changed_by, p_change_reason);
    
    -- Apply configuration change
    PERFORM apply_config_change(p_config_key, p_new_value);
    
    RETURN TRUE;
END;
$$ LANGUAGE plpgsql;

-- Function to apply configuration changes
CREATE OR REPLACE FUNCTION apply_config_change(p_config_key VARCHAR(100), p_new_value TEXT)
RETURNS VOID AS $$
BEGIN
    -- Apply specific configuration changes
    CASE p_config_key
        WHEN 'pg_hello.default_repeat' THEN
            EXECUTE format('SET pg_hello.repeat = %s', p_new_value);
            
        WHEN 'pg_hello.enable_logging' THEN
            -- Could enable/disable logging triggers
            NULL; -- Placeholder
            
        ELSE
            -- Generic configuration change
            NULL;
    END CASE;
END;
$$ LANGUAGE plpgsql;

-- Advanced greeting function with configuration
CREATE OR REPLACE FUNCTION pg_hello_advanced(
    p_name TEXT,
    p_override_repeat INTEGER DEFAULT NULL,
    p_override_prefix TEXT DEFAULT NULL
)
RETURNS TEXT AS $$
DECLARE
    repeat_count INTEGER;
    greeting_prefix TEXT;
    max_name_length INTEGER;
    final_greeting TEXT;
    safe_name TEXT;
BEGIN
    -- Get configuration values
    repeat_count := COALESCE(p_override_repeat, get_config_value('pg_hello.default_repeat')::INTEGER);
    greeting_prefix := COALESCE(p_override_prefix, get_config_value('pg_hello.greeting_prefix'));
    max_name_length := get_config_value('pg_hello.max_name_length')::INTEGER;
    
    -- Validate and truncate name if necessary
    safe_name := SUBSTRING(TRIM(p_name) FROM 1 FOR max_name_length);
    
    IF LENGTH(safe_name) = 0 THEN
        safe_name := 'Guest';
    END IF;
    
    -- Set repeat configuration
    EXECUTE format('SET pg_hello.repeat = %s', repeat_count);
    
    -- Generate greeting with custom prefix
    SELECT pg_hello(safe_name) INTO final_greeting;
    final_greeting := REPLACE(final_greeting, 'Hello', greeting_prefix);
    
    -- Log if enabled
    IF get_config_value('pg_hello.enable_logging')::BOOLEAN THEN
        INSERT INTO activity_log (user_id, action, description)
        VALUES (0, 'GREETING_GENERATED', format('Generated greeting for: %s', safe_name));
    END IF;
    
    RETURN final_greeting;
END;
$$ LANGUAGE plpgsql;

-- Test configuration management
SELECT update_config_value('pg_hello.default_repeat', '3', 'admin', 'Testing new default');
SELECT update_config_value('pg_hello.greeting_prefix', 'Hi', 'admin', 'More casual greeting');

-- Test advanced greeting function
SELECT pg_hello_advanced('Alice');
SELECT pg_hello_advanced('Bob', 5, 'Welcome');

-- View configuration history
SELECT 
    config_key,
    old_value,
    new_value,
    to_timestamp(changed_at / 1000) as changed_at,
    changed_by,
    change_reason
FROM extension_config_history
ORDER BY changed_at DESC;
```

## 🔗 Integration Patterns

### Event-Driven Architecture

```sql
-- Event table for extension activities
CREATE TABLE extension_events (
    id SERIAL PRIMARY KEY,
    event_type VARCHAR(50) NOT NULL,
    event_source VARCHAR(50) NOT NULL,
    event_data JSONB NOT NULL,
    event_timestamp BIGINT DEFAULT now_ms(),
    processed_at BIGINT NULL,
    processed_by VARCHAR(50) NULL,
    status VARCHAR(20) DEFAULT 'PENDING'
);

-- Event handlers table
CREATE TABLE event_handlers (
    id SERIAL PRIMARY KEY,
    event_type VARCHAR(50) NOT NULL,
    handler_name VARCHAR(100) NOT NULL,
    handler_function VARCHAR(100) NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    priority INTEGER DEFAULT 0
);

-- Register event handlers
INSERT INTO event_handlers (event_type, handler_name, handler_function, priority) VALUES
    ('USER_GREETED', 'Log Greeting', 'handle_user_greeted', 1),
    ('USER_GREETED', 'Update Stats', 'handle_greeting_stats', 2),
    ('CONFIG_CHANGED', 'Notify Admins', 'handle_config_change', 1),
    ('PERFORMANCE_ISSUE', 'Send Alert', 'handle_performance_alert', 1);

-- Function to publish events
CREATE OR REPLACE FUNCTION publish_event(
    p_event_type VARCHAR(50),
    p_event_source VARCHAR(50),
    p_event_data JSONB
)
RETURNS BIGINT AS $$
DECLARE
    event_id BIGINT;
BEGIN
    INSERT INTO extension_events (event_type, event_source, event_data)
    VALUES (p_event_type, p_event_source, p_event_data)
    RETURNING id INTO event_id;
    
    -- Trigger event processing
    PERFORM process_event(event_id);
    
    RETURN event_id;
END;
$$ LANGUAGE plpgsql;

-- Event processing function
CREATE OR REPLACE FUNCTION process_event(p_event_id BIGINT)
RETURNS VOID AS $$
DECLARE
    event_record extension_events%ROWTYPE;
    handler_record event_handlers%ROWTYPE;
    handler_result TEXT;
BEGIN
    -- Get event details
    SELECT * INTO event_record FROM extension_events WHERE id = p_event_id;
    
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Event not found: %', p_event_id;
    END IF;
    
    -- Process all active handlers for this event type
    FOR handler_record IN 
        SELECT * FROM event_handlers 
        WHERE event_type = event_record.event_type AND is_active = TRUE
        ORDER BY priority
    LOOP
        BEGIN
            -- Execute handler function
            EXECUTE format('SELECT %s($1, $2)', handler_record.handler_function)
            USING event_record.event_data, p_event_id
            INTO handler_result;
            
        EXCEPTION
            WHEN OTHERS THEN
                -- Log handler errors but continue processing
                RAISE WARNING 'Event handler % failed for event %: %', 
                    handler_record.handler_name, p_event_id, SQLERRM;
        END;
    END LOOP;
    
    -- Mark event as processed
    UPDATE extension_events
    SET 
        processed_at = now_ms(),
        processed_by = 'system',
        status = 'PROCESSED'
    WHERE id = p_event_id;
END;
$$ LANGUAGE plpgsql;

-- Event handler implementations
CREATE OR REPLACE FUNCTION handle_user_greeted(p_event_data JSONB, p_event_id BIGINT)
RETURNS TEXT AS $$
DECLARE
    user_name TEXT;
    greeting_text TEXT;
BEGIN
    -- Extract data from event
    user_name := p_event_data->>'user_name';
    greeting_text := p_event_data->>'greeting';
    
    -- Log the greeting
    INSERT INTO activity_log (user_id, action, description)
    VALUES (
        0,
        'USER_GREETED',
        format('Greeting generated for %s: %s', user_name, greeting_text)
    );
    
    RETURN 'Greeting logged successfully';
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION handle_greeting_stats(p_event_data JSONB, p_event_id BIGINT)
RETURNS TEXT AS $$
DECLARE
    stats_table TEXT := 'greeting_statistics';
    user_name TEXT;
BEGIN
    user_name := p_event_data->>'user_name';
    
    -- Create stats table if it doesn't exist
    EXECUTE format('
        CREATE TABLE IF NOT EXISTS %I (
            user_name TEXT PRIMARY KEY,
            greeting_count INTEGER DEFAULT 0,
            last_greeted_at BIGINT,
            first_greeted_at BIGINT
        )', stats_table);
    
    -- Update statistics
    EXECUTE format('
        INSERT INTO %I (user_name, greeting_count, last_greeted_at, first_greeted_at)
        VALUES ($1, 1, $2, $2)
        ON CONFLICT (user_name) DO UPDATE SET
            greeting_count = %I.greeting_count + 1,
            last_greeted_at = $2',
        stats_table, stats_table, stats_table)
    USING user_name, now_ms();
    
    RETURN 'Statistics updated successfully';
END;
$$ LANGUAGE plpgsql;

-- Enhanced greeting function with events
CREATE OR REPLACE FUNCTION pg_hello_with_events(p_name TEXT)
RETURNS TEXT AS $$
DECLARE
    greeting_result TEXT;
    event_data JSONB;
BEGIN
    -- Generate greeting
    SELECT pg_hello(p_name) INTO greeting_result;
    
    -- Prepare event data
    event_data := jsonb_build_object(
        'user_name', p_name,
        'greeting', greeting_result,
        'timestamp_ms', now_ms(),
        'function_called', 'pg_hello_with_events'
    );
    
    -- Publish event
    PERFORM publish_event('USER_GREETED', 'pg_hello_extension', event_data);
    
    RETURN greeting_result;
END;
$$ LANGUAGE plpgsql;

-- Test event-driven system
SELECT pg_hello_with_events('Alice');
SELECT pg_hello_with_events('Bob');
SELECT pg_hello_with_events('Alice');  -- Second greeting for Alice

-- Check events
SELECT 
    event_type,
    event_source,
    event_data,
    to_timestamp(event_timestamp / 1000) as event_time,
    status
FROM extension_events
ORDER BY event_timestamp DESC;

-- Check statistics
SELECT * FROM greeting_statistics;
```

## 🚀 Advanced Use Cases

### Multi-Language Greeting System

```sql
-- Language configurations
CREATE TABLE greeting_languages (
    language_code VARCHAR(5) PRIMARY KEY,
    language_name VARCHAR(50) NOT NULL,
    greeting_word VARCHAR(20) NOT NULL,
    is_active BOOLEAN DEFAULT TRUE
);

-- Insert language data
INSERT INTO greeting_languages (language_code, language_name, greeting_word) VALUES
    ('en', 'English', 'Hello'),
    ('es', 'Spanish', 'Hola'),
    ('fr', 'French', 'Bonjour'),
    ('de', 'German', 'Hallo'),
    ('it', 'Italian', 'Ciao'),
    ('pt', 'Portuguese', 'Olá'),
    ('ja', 'Japanese', 'こんにちは'),
    ('ko', 'Korean', '안녕하세요'),
    ('zh', 'Chinese', '你好'),
    ('ar', 'Arabic', 'مرحبا');

-- User language preferences
CREATE TABLE user_language_preferences (
    user_id INTEGER REFERENCES users(id),
    language_code VARCHAR(5) REFERENCES greeting_languages(language_code),
    is_primary BOOLEAN DEFAULT FALSE,
    created_at BIGINT DEFAULT now_ms(),
    PRIMARY KEY (user_id, language_code)
);

-- Multi-language greeting function
CREATE OR REPLACE FUNCTION pg_hello_multilang(
    p_name TEXT,
    p_language_code VARCHAR(5) DEFAULT 'en'
)
RETURNS TEXT AS $$
DECLARE
    greeting_word TEXT;
    final_greeting TEXT;
BEGIN
    -- Get greeting word for the language
    SELECT gl.greeting_word INTO greeting_word
    FROM greeting_languages gl
    WHERE gl.language_code = p_language_code AND gl.is_active = TRUE;
    
    -- Fallback to English if language not found
    IF NOT FOUND THEN
        SELECT gl.greeting_word INTO greeting_word
        FROM greeting_languages gl
        WHERE gl.language_code = 'en';
    END IF;
    
    -- Generate greeting using the extension but replace the greeting word
    SELECT pg_hello(p_name) INTO final_greeting;
    final_greeting := REPLACE(final_greeting, 'Hello', greeting_word);
    
    RETURN final_greeting;
END;
$$ LANGUAGE plpgsql;

-- Function to get user's preferred greeting
CREATE OR REPLACE FUNCTION get_user_greeting(p_user_id INTEGER, p_fallback_lang VARCHAR(5) DEFAULT 'en')
RETURNS TEXT AS $$
DECLARE
    user_record users%ROWTYPE;
    preferred_lang VARCHAR(5);
    greeting_result TEXT;
BEGIN
    -- Get user information
    SELECT * INTO user_record FROM users WHERE id = p_user_id;
    
    IF NOT FOUND THEN
        RAISE EXCEPTION 'User not found: %', p_user_id;
    END IF;
    
    -- Get user's primary language preference
    SELECT ulp.language_code INTO preferred_lang
    FROM user_language_preferences ulp
    WHERE ulp.user_id = p_user_id AND ulp.is_primary = TRUE;
    
    -- Use fallback if no preference set
    preferred_lang := COALESCE(preferred_lang, p_fallback_lang);
    
    -- Generate multilingual greeting
    SELECT pg_hello_multilang(user_record.username, preferred_lang) INTO greeting_result;
    
    RETURN greeting_result;
END;
$$ LANGUAGE plpgsql;

-- Set user language preferences
INSERT INTO user_language_preferences (user_id, language_code, is_primary) VALUES
    (1, 'es', TRUE),  -- Alice prefers Spanish
    (2, 'fr', TRUE),  -- Bob prefers French
    (3, 'ja', TRUE);  -- Carol prefers Japanese

-- Test multilingual greetings
SELECT pg_hello_multilang('Alice', 'es') as spanish_greeting;
SELECT pg_hello_multilang('Bob', 'fr') as french_greeting;
SELECT pg_hello_multilang('Carol', 'ja') as japanese_greeting;

-- Test user-specific greetings
SELECT get_user_greeting(1) as alice_greeting;  -- Should be in Spanish
SELECT get_user_greeting(2) as bob_greeting;    -- Should be in French
SELECT get_user_greeting(3) as carol_greeting;  -- Should be in Japanese
```

### Notification and Alert System

```sql
-- Notification templates
CREATE TABLE notification_templates (
    id SERIAL PRIMARY KEY,
    template_name VARCHAR(100) UNIQUE NOT NULL,
    subject_template TEXT NOT NULL,
    body_template TEXT NOT NULL,
    notification_type VARCHAR(50) NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    created_at BIGINT DEFAULT now_ms()
);

-- Notification queue
CREATE TABLE notification_queue (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    template_name VARCHAR(100) REFERENCES notification_templates(template_name),
    subject TEXT NOT NULL,
    body TEXT NOT NULL,
    notification_type VARCHAR(50) NOT NULL,
    status VARCHAR(20) DEFAULT 'PENDING',
    variables JSONB,
    created_at BIGINT DEFAULT now_ms(),
    sent_at BIGINT NULL,
    error_message TEXT NULL
);

-- Insert notification templates
INSERT INTO notification_templates (template_name, subject_template, body_template, notification_type) VALUES
    ('welcome_notification', 'Welcome {{user_name}}!', '{{greeting}}\n\nWelcome to our platform! We''re excited to have you join us.\n\nYour account is now active and ready to use.', 'email'),
    ('daily_summary', 'Daily Summary for {{user_name}}', '{{greeting}}\n\nHere''s your daily summary:\n- Total activities: {{activity_count}}\n- Last login: {{last_login}}\n\nHave a great day!', 'email'),
    ('security_alert', 'Security Alert - {{alert_type}}', 'Hello {{user_name}},\n\nWe detected a security event on your account:\n{{alert_details}}\n\nIf this wasn''t you, please contact support immediately.', 'email');

-- Function to generate notifications with greetings
CREATE OR REPLACE FUNCTION generate_notification(
    p_user_id INTEGER,
    p_template_name VARCHAR(100),
    p_variables JSONB DEFAULT '{}'::jsonb
)
RETURNS BIGINT AS $$
DECLARE
    user_record users%ROWTYPE;
    template_record notification_templates%ROWTYPE;
    personalized_greeting TEXT;
    final_subject TEXT;
    final_body TEXT;
    notification_id BIGINT;
    template_variables JSONB;
BEGIN
    -- Get user and template information
    SELECT * INTO user_record FROM users WHERE id = p_user_id;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'User not found: %', p_user_id;
    END IF;
    
    SELECT * INTO template_record FROM notification_templates WHERE template_name = p_template_name AND is_active = TRUE;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Notification template not found: %', p_template_name;
    END IF;
    
    -- Generate personalized greeting
    personalized_greeting := get_user_greeting(p_user_id);
    
    -- Prepare template variables
    template_variables := p_variables || jsonb_build_object(
        'user_name', user_record.username,
        'user_email', user_record.email,
        'full_name', user_record.full_name,
        'greeting', personalized_greeting,
        'current_timestamp', to_timestamp(now_ms() / 1000)
    );
    
    -- Process subject template
    final_subject := template_record.subject_template;
    FOR key_value IN SELECT key, value FROM jsonb_each_text(template_variables) LOOP
        final_subject := REPLACE(final_subject, '{{' || key_value.key || '}}', key_value.value);
    END LOOP;
    
    -- Process body template
    final_body := template_record.body_template;
    FOR key_value IN SELECT key, value FROM jsonb_each_text(template_variables) LOOP
        final_body := REPLACE(final_body, '{{' || key_value.key || '}}', key_value.value);
    END LOOP;
    
    -- Queue notification
    INSERT INTO notification_queue (
        user_id,
        template_name,
        subject,
        body,
        notification_type,
        variables
    ) VALUES (
        p_user_id,
        p_template_name,
        final_subject,
        final_body,
        template_record.notification_type,
        template_variables
    ) RETURNING id INTO notification_id;
    
    RETURN notification_id;
END;
$$ LANGUAGE plpgsql;

-- Function to send daily summaries
CREATE OR REPLACE FUNCTION send_daily_summaries()
RETURNS INTEGER AS $$
DECLARE
    user_record RECORD;
    activity_count INTEGER;
    last_login_time TIMESTAMP;
    notification_count INTEGER := 0;
    summary_variables JSONB;
BEGIN
    FOR user_record IN SELECT * FROM users WHERE id <= 3 LOOP  -- Limit for demo
        -- Get user activity count for today
        SELECT COUNT(*) INTO activity_count
        FROM activity_log al
        WHERE al.user_id = user_record.id
        AND to_timestamp(al.timestamp_ms / 1000)::date = CURRENT_DATE;
        
        -- Get last login time
        SELECT to_timestamp(al.timestamp_ms / 1000) INTO last_login_time
        FROM activity_log al
        WHERE al.user_id = user_record.id
        AND al.action = 'LOGIN'
        ORDER BY al.timestamp_ms DESC
        LIMIT 1;
        
        -- Prepare variables
        summary_variables := jsonb_build_object(
            'activity_count', activity_count,
            'last_login', COALESCE(last_login_time::text, 'No recent login')
        );
        
        -- Generate notification
        PERFORM generate_notification(user_record.id, 'daily_summary', summary_variables);
        notification_count := notification_count + 1;
    END LOOP;
    
    RETURN notification_count;
END;
$$ LANGUAGE plpgsql;

-- Test notification system
-- Generate welcome notifications for all users
SELECT generate_notification(id, 'welcome_notification') as notification_id
FROM users;

-- Generate daily summaries
SELECT send_daily_summaries() as summaries_generated;

-- View generated notifications
SELECT 
    u.username,
    nq.template_name,
    nq.subject,
    LEFT(nq.body, 100) || '...' as body_preview,
    nq.status,
    to_timestamp(nq.created_at / 1000) as created_at
FROM notification_queue nq
JOIN users u ON u.id = nq.user_id
ORDER BY nq.created_at DESC;
```

This comprehensive examples guide demonstrates the versatility and power of the `pg_hello` extension across various real-world scenarios! 🎯
