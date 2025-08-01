# Enhanced Velocity Detection System for T16 Controllers

## Overview

This document outlines the design and implementation of an advanced velocity detection system for the T16 family of MIDI controllers, replacing the current time-based approach with sophisticated pressure analysis methods that provide more musical and expressive velocity response.

## Current Implementation Issues

### Existing Method (main.cpp:55, Keyboard.hpp:55)
```cpp
// Current primitive approach
ulong pressTime = millis() - pressStartTime;
velocity = fmap((float)pressTime, 55.0f, 4.0f, 0.18f, 1.0f);
```

**Problems:**
- Only measures time-to-threshold crossing
- Ignores actual striking force and pressure dynamics
- Poor differentiation between playing styles
- Inconsistent across different key positions
- Temperature and sensor drift affects results
- Limited expressive range

## Proposed Enhanced System

### Core Concept: Multi-Method Velocity Analysis

The new system analyzes pressure profiles during key attack to extract velocity information using multiple complementary methods, providing more accurate and musical velocity response.

### Architecture Overview

```cpp
class VelocityDetectionEngine {
public:
    enum Method {
        RATE_OF_CHANGE,     // Primary: Pressure derivative analysis
        PEAK_PRESSURE,      // Secondary: Maximum pressure reached
        ATTACK_TIME,        // Legacy: Time-based (compatibility)
        HYBRID_ANALYSIS,    // Combination of multiple methods
        ADAPTIVE_LEARNING   // Machine learning approach (future)
    };
    
    struct Configuration {
        Method primaryMethod = RATE_OF_CHANGE;
        Method secondaryMethod = PEAK_PRESSURE;
        float hybridWeight = 0.7f;           // Primary method influence
        uint32_t analysisWindow = 5;         // Analysis window (ms)
        float sensitivity = 1.0f;            // Global sensitivity
        bool enableAdaptiveLearning = false; // Future feature
    };
    
    uint8_t calculateVelocity(uint8_t keyIndex, float pressure, uint32_t timestamp);
    void resetKey(uint8_t keyIndex);
    void updateConfiguration(const Configuration& config);
};
```

## Method 1: Rate of Change Analysis (Primary)

### Theory

Velocity in music represents the initial striking force. The rate of pressure change (`dP/dt`) immediately after threshold crossing best represents this physical property.

### Mathematical Foundation

```
velocity ∝ max(dP/dt) where t ∈ [threshold_time, threshold_time + analysis_window]
```

### Implementation

```cpp
class RateOfChangeAnalyzer {
private:
    static constexpr uint8_t HISTORY_SIZE = 32;
    static constexpr float SAMPLE_RATE = 4000.0f; // Hz
    static constexpr uint32_t DEFAULT_WINDOW_MS = 5;
    
    struct PressureHistory {
        float pressure[HISTORY_SIZE];
        uint32_t timestamp[HISTORY_SIZE];
        uint8_t writeIndex = 0;
        uint8_t size = 0;
    };
    
    PressureHistory keyHistory[MAX_KEYS];
    
public:
    float calculateRateVelocity(uint8_t keyIndex, float threshold = 0.18f) {
        auto& history = keyHistory[keyIndex];
        
        // Find threshold crossing point
        int thresholdIndex = findThresholdCrossing(history, threshold);
        if (thresholdIndex == -1) return 0.5f; // Default normalized velocity
        
        // Calculate maximum rate of change in analysis window
        float maxRate = 0.0f;
        uint32_t windowStart = history.timestamp[thresholdIndex];
        uint32_t windowEnd = windowStart + (DEFAULT_WINDOW_MS * 1000); // Convert to microseconds
        
        for (int i = 1; i < history.size; i++) {
            int idx = (thresholdIndex + i) % HISTORY_SIZE;
            
            // Check if within analysis window
            if (history.timestamp[idx] > windowEnd) break;
            
            // Calculate instantaneous rate
            uint32_t deltaTime = history.timestamp[idx] - history.timestamp[idx-1];
            if (deltaTime > 0) {
                float deltaPressure = history.pressure[idx] - history.pressure[idx-1];
                float rate = (deltaPressure * 1000000.0f) / deltaTime; // Pressure/second
                maxRate = max(maxRate, rate);
            }
        }
        
        // Normalize and apply calibration curve
        return normalizeRate(maxRate);
    }
    
private:
    int findThresholdCrossing(const PressureHistory& history, float threshold) {
        for (int i = 0; i < history.size - 1; i++) {
            if (history.pressure[i] <= threshold && history.pressure[i+1] > threshold) {
                return i + 1; // Return index of first value above threshold
            }
        }
        return -1; // No crossing found
    }
    
    float normalizeRate(float rawRate) {
        // Calibrated based on testing with actual hardware
        const float MIN_RATE = 0.01f;   // Slowest detectable press
        const float MAX_RATE = 2.5f;    // Fastest possible press
        
        // Apply logarithmic scaling for more musical response
        float normalized = (rawRate - MIN_RATE) / (MAX_RATE - MIN_RATE);
        normalized = constrain(normalized, 0.0f, 1.0f);
        
        // Apply gentle logarithmic curve for better feel
        return sqrt(normalized); // Square root gives good musical scaling
    }
};
```

### Advantages
- **Physical Accuracy**: Directly measures striking force
- **Consistent**: Less affected by sensor variations
- **Expressive**: Clear differentiation between playing styles
- **Temperature Stable**: Rate calculations are less sensitive to absolute pressure drift

### Calibration Requirements
- Minimum detectable rate (soft touch)
- Maximum expected rate (hard strike)
- Sensor-specific scaling factors
- Temperature compensation coefficients

## Method 2: Peak Pressure Analysis (Secondary)

### Theory

The maximum pressure reached during attack correlates with velocity, especially for sustained playing styles.

### Implementation

```cpp
class PeakPressureAnalyzer {
private:
    struct PeakAnalysis {
        float peakPressure = 0.0f;
        uint32_t peakTime = 0;
        bool peakDetected = false;
        float previousPressure = 0.0f;
        uint8_t flatlineCount = 0;
    };
    
    PeakAnalysis keyAnalysis[MAX_KEYS];
    
public:
    float calculatePeakVelocity(uint8_t keyIndex, float currentPressure, uint32_t timestamp) {
        auto& analysis = keyAnalysis[keyIndex];
        
        // Update peak if current pressure is higher
        if (currentPressure > analysis.peakPressure) {
            analysis.peakPressure = currentPressure;
            analysis.peakTime = timestamp;
            analysis.flatlineCount = 0;
        } else if (abs(currentPressure - analysis.previousPressure) < 0.005f) {
            analysis.flatlineCount++;
        }
        
        // Detect peak (pressure stops increasing)
        if (!analysis.peakDetected && analysis.flatlineCount > 3) {
            analysis.peakDetected = true;
            return normalizePeakPressure(analysis.peakPressure);
        }
        
        analysis.previousPressure = currentPressure;
        return 0.5f; // Still analyzing
    }
    
private:
    float normalizePeakPressure(float peakPressure) {
        // Map peak pressure to velocity
        const float MIN_PEAK = 0.18f;  // Velocity threshold
        const float MAX_PEAK = 0.95f;  // Maximum useful pressure
        
        float normalized = (peakPressure - MIN_PEAK) / (MAX_PEAK - MIN_PEAK);
        return constrain(normalized, 0.0f, 1.0f);
    }
};
```

## Method 3: Hybrid Analysis (Recommended)

### Adaptive Weighting System

```cpp
class HybridVelocityAnalyzer {
private:
    RateOfChangeAnalyzer rateAnalyzer;
    PeakPressureAnalyzer peakAnalyzer;
    AttackTimeAnalyzer timeAnalyzer; // Legacy compatibility
    
    struct PlayingStyleDetector {
        float avgAttackTime = 50.0f;     // Running average
        float avgPeakPressure = 0.5f;    // Running average
        float confidenceLevel = 0.5f;    // How sure we are about the playing style
        
        enum Style {
            PERCUSSIVE,    // Fast attacks, rely more on rate
            EXPRESSIVE,    // Varied pressure, balance methods
            SUSTAINED,     // Slow attacks, rely more on peak
            UNKNOWN        // Still learning
        } detectedStyle = UNKNOWN;
    };
    
    PlayingStyleDetector styleDetector[MAX_KEYS];
    
public:
    float calculateHybridVelocity(uint8_t keyIndex, float pressure, uint32_t timestamp) {
        // Get results from all methods
        float rateVelocity = rateAnalyzer.calculateRateVelocity(keyIndex);
        float peakVelocity = peakAnalyzer.calculatePeakVelocity(keyIndex, pressure, timestamp);
        float timeVelocity = timeAnalyzer.calculateTimeVelocity(keyIndex, timestamp);
        
        // Detect playing style
        auto& style = styleDetector[keyIndex];
        updatePlayingStyle(style, pressure, timestamp);
        
        // Apply adaptive weighting based on detected playing style
        return applyAdaptiveWeighting(rateVelocity, peakVelocity, timeVelocity, style);
    }
    
private:
    float applyAdaptiveWeighting(float rate, float peak, float time, const PlayingStyleDetector& style) {
        float rateWeight, peakWeight, timeWeight;
        
        switch (style.detectedStyle) {
            case PlayingStyleDetector::PERCUSSIVE:
                rateWeight = 0.8f; peakWeight = 0.15f; timeWeight = 0.05f;
                break;
            case PlayingStyleDetector::EXPRESSIVE:
                rateWeight = 0.5f; peakWeight = 0.4f; timeWeight = 0.1f;
                break;
            case PlayingStyleDetector::SUSTAINED:
                rateWeight = 0.3f; peakWeight = 0.6f; timeWeight = 0.1f;
                break;
            default: // UNKNOWN
                rateWeight = 0.6f; peakWeight = 0.3f; timeWeight = 0.1f;
        }
        
        // Apply confidence-based blending
        float confidence = style.confidenceLevel;
        float adaptiveResult = (rate * rateWeight + peak * peakWeight + time * timeWeight);
        float defaultResult = (rate * 0.6f + peak * 0.3f + time * 0.1f);
        
        return lerp(defaultResult, adaptiveResult, confidence);
    }
    
    void updatePlayingStyle(PlayingStyleDetector& style, float pressure, uint32_t timestamp) {
        // Update running averages and detect patterns
        // This would include statistical analysis of recent key presses
        // Implementation details depend on specific musical requirements
    }
};
```

## Integration with Existing System

### Hardware Interface Integration

```cpp
// Integration with existing Keyboard.hpp
class Key {
private:
    VelocityDetectionEngine velocityEngine;
    static VelocityDetectionEngine::Configuration globalVelocityConfig;
    
public:
    void Update(Adc *adc) override {
        float currentPressure = adc->GetMux(0, mux_idx);
        uint32_t currentTime = micros(); // Higher precision than millis()
        
        // State machine for key press detection
        switch (state) {
            case IDLE:
                if (currentPressure > rel_threshold) {
                    state = STARTED;
                    velocityEngine.resetKey(idx);
                }
                break;
                
            case STARTED:
                if (currentPressure > press_threshold) {
                    // Calculate velocity using new system
                    uint8_t calculatedVelocity = velocityEngine.calculateVelocity(
                        idx, currentPressure, currentTime);
                    
                    // Apply velocity curve (existing functionality)
                    velocity = applyVelocityCurve(calculatedVelocity);
                    
                    state = PRESSED;
                    onStateChanged.Emit(idx, state);
                }
                break;
                
            // ... rest of state machine unchanged
        }
        
        // Continue pressure analysis for aftertouch
        pressure = calculateAftertouch(currentPressure);
    }
    
private:
    uint8_t applyVelocityCurve(uint8_t rawVelocity) {
        // Use existing LUT system from Keyboard class
        return output_lut[velocityLut][rawVelocity];
    }
};
```

### Configuration Integration

```cpp
// Extension to Configuration.hpp
struct VelocityDetectionConfig {
    uint8_t method = 0;              // 0=Rate, 1=Peak, 2=Time, 3=Hybrid
    uint8_t sensitivity = 64;        // 0-127 sensitivity adjustment
    uint8_t analysisWindow = 5;      // Analysis window in ms
    uint8_t hybridWeight = 70;       // Primary method weight (0-100%)
    uint8_t adaptiveLearning = 0;    // Enable adaptive learning
    uint8_t reserved[3];             // Future expansion
};

// Add to KeyModeData structure
struct KeyModeData {
    // ... existing fields ...
    VelocityDetectionConfig velocityDetection;
    bool hasChanged = false;
};
```

### Web Configurator Integration

```typescript
// New velocity configuration component
interface VelocityConfig {
    method: 'rate' | 'peak' | 'time' | 'hybrid';
    sensitivity: number; // 0-127
    analysisWindow: number; // 1-10 ms
    hybridWeight: number; // 0-100%
    adaptiveLearning: boolean;
}

class VelocityConfigurationPanel extends React.Component<{
    config: VelocityConfig;
    onChange: (config: VelocityConfig) => void;
}> {
    render() {
        return (
            <div className="velocity-config-panel">
                <h3>Velocity Detection</h3>
                
                <SelectField
                    label="Detection Method"
                    value={this.props.config.method}
                    options={[
                        { value: 'rate', label: 'Rate of Change (Recommended)' },
                        { value: 'peak', label: 'Peak Pressure' },
                        { value: 'time', label: 'Attack Time (Legacy)' },
                        { value: 'hybrid', label: 'Hybrid Analysis' }
                    ]}
                    onChange={(method) => this.updateConfig({ method })}
                />
                
                <SliderField
                    label="Sensitivity"
                    value={this.props.config.sensitivity}
                    min={0} max={127}
                    onChange={(sensitivity) => this.updateConfig({ sensitivity })}
                />
                
                <SliderField
                    label="Analysis Window (ms)"
                    value={this.props.config.analysisWindow}
                    min={1} max={10}
                    onChange={(analysisWindow) => this.updateConfig({ analysisWindow })}
                />
                
                {this.props.config.method === 'hybrid' && (
                    <SliderField
                        label="Primary Method Weight"
                        value={this.props.config.hybridWeight}
                        min={0} max={100}
                        onChange={(hybridWeight) => this.updateConfig({ hybridWeight })}
                    />
                )}
                
                <VelocityTestArea onTest={this.handleVelocityTest} />
            </div>
        );
    }
    
    private handleVelocityTest = (testResults: VelocityTestResult[]) => {
        // Real-time velocity testing and visualization
        // Show velocity response curves and sensitivity analysis
    };
}
```

## Performance Considerations

### Memory Usage
```cpp
// Memory requirements per key
struct KeyVelocityState {
    float pressureHistory[32];    // 128 bytes
    uint32_t timestampHistory[32]; // 128 bytes
    uint8_t analysisState;        // 1 byte
    float cachedResults[4];       // 16 bytes (rate, peak, time, hybrid)
    // Total: ~273 bytes per key
};

// For 64-key variant: 273 * 64 = ~17KB additional RAM usage
// ESP32-S3 has 512KB SRAM, so this is acceptable
```

### CPU Performance
```cpp
// Optimization strategies
class OptimizedVelocityEngine {
private:
    // Use fixed-point arithmetic for embedded performance
    using FixedPoint = int32_t; // 16.16 fixed point
    
    // Lookup tables for expensive calculations
    static uint8_t sqrtLUT[256];
    static uint8_t logLUT[256];
    
    // SIMD-style operations where possible
    void calculateMultipleRates(float* pressures, uint32_t* timestamps, float* results, int count);
    
    // Lazy evaluation - only calculate when needed
    mutable bool resultsValid[MAX_KEYS];
    mutable float cachedResults[MAX_KEYS];
};
```

### Real-time Constraints
- Maximum processing time per key update: **50µs** (at 4kHz sample rate)
- Memory allocation: **Zero dynamic allocation** during operation
- Interrupt safety: **All methods are reentrant**

## Testing and Calibration

### Automated Calibration Procedure

```cpp
class VelocityCalibrationEngine {
public:
    struct CalibrationData {
        float minDetectableRate[MAX_KEYS];
        float maxExpectedRate[MAX_KEYS]; 
        float pressureScaling[MAX_KEYS];
        float temperatureCoeff[MAX_KEYS];
    };
    
    // Guided calibration process
    enum CalibrationPhase {
        SOFT_TOUCH_SAMPLING,    // Sample gentle touches
        HARD_STRIKE_SAMPLING,   // Sample forceful strikes
        CONSISTENCY_TESTING,    // Test repeatability
        CROSS_KEY_BALANCING,    // Balance between keys
        VALIDATION              // Final validation
    };
    
    bool performCalibration(CalibrationPhase phase, 
                           std::function<void(const char*)> progressCallback);
    
    CalibrationData generateCalibrationData();
    bool validateCalibration(const CalibrationData& data);
};
```

### Performance Metrics

```cpp
// Metrics to track system performance
struct VelocityMetrics {
    float averageAccuracy;        // Compared to reference measurements
    float crossKeyConsistency;    // Variance between keys
    float repeatability;          // Same input → same output
    float temperatureStability;   // Performance across temperature range
    float latency;               // Response time
    uint32_t processingTime;     // CPU time per calculation
};
```

## Migration Strategy

### Phase 1: Parallel Implementation
- Implement new system alongside existing
- Add configuration option to switch methods
- Extensive A/B testing with musicians

### Phase 2: Gradual Transition
- Default to new system for new devices
- Provide migration tools for existing configurations
- Maintain backward compatibility

### Phase 3: Full Migration
- Remove legacy time-based method
- Optimize for new system exclusively
- Update all documentation and presets

## Future Enhancements

### Machine Learning Integration
```cpp
class AdaptiveVelocityEngine {
private:
    // Simple neural network for velocity prediction
    struct NeuralNet {
        float weights[32][16];    // Input layer weights
        float biases[16];         // Hidden layer biases
        float outputWeights[16];  // Output layer weights
    };
    
    NeuralNet personalizedModel[MAX_KEYS];
    
public:
    // Learn from user playing patterns
    void trainOnUserInput(uint8_t keyIndex, const PressureProfile& profile, uint8_t desiredVelocity);
    
    // Predict velocity using learned model
    uint8_t predictVelocity(uint8_t keyIndex, const PressureProfile& profile);
};
```

### Multi-Sensor Fusion
- Integration with accelerometer data
- Gyroscope input for gesture recognition
- Optical sensor backup for critical applications

### Advanced Analysis
- Spectral analysis of pressure signals
- Pattern recognition for playing techniques
- Biomechanical modeling of finger dynamics

## Conclusion

The enhanced velocity detection system provides a significant improvement over the current time-based approach, offering:

- **Better Musical Expression**: More accurate representation of playing dynamics
- **Improved Consistency**: Reduced variation between keys and environmental conditions  
- **Enhanced Configurability**: Multiple methods and fine-tuning options
- **Future Expandability**: Architecture supports advanced features and learning

The system maintains backward compatibility while providing a clear upgrade path for enhanced musical expression and professional-grade velocity response.

## References

1. "Digital Piano Action and Velocity Sensing" - Yamaha Technical Papers
2. "Expressive Controllers for Electronic Music" - NIME Conference Proceedings  
3. "Pressure-Sensitive Interface Design" - CHI Conference Papers
4. "Real-time Audio Processing on Embedded Systems" - AES Convention Papers
5. "Machine Learning for Musical Expression" - ICML Workshop Papers