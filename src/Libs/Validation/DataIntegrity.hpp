#ifndef DATA_INTEGRITY_HPP
#define DATA_INTEGRITY_HPP

#include <Arduino.h>
#include <functional>
#include <vector>
#include <unordered_set>

namespace DataIntegrity {
    
    enum ChecksumType : uint8_t {
        CRC8_BASIC      = 0x01,
        CRC16_STANDARD  = 0x02,
        CRC32_STRONG    = 0x03,
        HASH_SHA256     = 0x04,
        REED_SOLOMON    = 0x05
    };
    
    enum class ValidationError : uint8_t {
        NONE = 0,
        INVALID_MAGIC,
        VERSION_MISMATCH,
        REPLAY_ATTACK,
        STALE_MESSAGE,
        OUT_OF_ORDER,
        SIZE_MISMATCH,
        CHECKSUM_FAILURE,
        INVALID_MIDI_CHANNEL,
        INVALID_SCALE,
        INVALID_OCTAVE,
        INVALID_CC_ID,
        INVALID_SCALE_NOTE,
        HARDWARE_INCOMPATIBLE,
        FIRMWARE_TOO_OLD,
        OUT_OF_RANGE,
        CUSTOM_VALIDATION_FAILED
    };
    
    enum class ValidationWarning : uint8_t {
        NONE = 0,
        CHANNEL_CONFLICT,
        DUPLICATE_CC_ID,
        UNKNOWN_PARAMETER,
        DEPRECATED_PARAMETER
    };
    
    enum class WarningLevel : uint8_t {
        WARN_LOW,
        MODERATE,
        WARN_HIGH
    };
    
    struct ValidationErrorDetail {
        ValidationError type;
        struct {
            uint8_t bank = 0;
            const char* parameter = nullptr;
            uint8_t index = 0;
        } location;
        uint32_t currentValue = 0;
        struct {
            uint32_t min;
            uint32_t max;
        } validRange = {0, 0};
        bool isAutoFixable = false;
        const char* suggestedFix = nullptr;
        const char* message = nullptr;
    };
    
    struct ValidationWarningDetail {
        ValidationWarning type;
        const char* message;
        struct {
            uint8_t bank = 0;
            const char* parameter = nullptr;
            uint8_t index = 0;
        } location;
        WarningLevel severity;
    };
    
    class ValidationResult {
    public:
        bool isValid = true;
        std::vector<ValidationErrorDetail> errors;
        std::vector<ValidationWarningDetail> warnings;
        std::vector<const char*> suggestedFixes;
        
        void addError(const ValidationErrorDetail& error) {
            errors.push_back(error);
            isValid = false;
            if (error.suggestedFix) {
                suggestedFixes.push_back(error.suggestedFix);
            }
        }
        
        void addWarning(const ValidationWarningDetail& warning) {
            warnings.push_back(warning);
        }
        
        bool canAutoFix() const {
            if (errors.empty()) return false;
            for (const auto& error : errors) {
                if (!error.isAutoFixable) return false;
            }
            return true;
        }
        
        void clear() {
            isValid = true;
            errors.clear();
            warnings.clear();
            suggestedFixes.clear();
        }
    };
    
    template<typename T>
    struct SecureMessage {
        uint32_t magic = 0x54313650;     // "T16P" - protocol identifier
        uint16_t version = 0x0001;        // Protocol version
        uint16_t messageId;               // Unique message ID
        uint32_t timestamp;               // Timestamp for replay attack prevention
        ChecksumType checksumType;        // Checksum algorithm used
        uint16_t payloadSize;             // Size of T data
        T payload;                        // Actual data
        uint32_t checksum;                // Computed checksum
        uint32_t sequenceNumber;          // Prevent message reordering
    } __attribute__((packed));
    
    class MessageValidator {
    private:
        static constexpr uint32_t MAX_CLOCK_SKEW = 60000; // 60 seconds
        static constexpr uint16_t MAX_MESSAGE_HISTORY = 100;
        
        uint32_t expectedSequence = 0;
        uint32_t lastTimestamp = 0;
        std::unordered_set<uint16_t> processedMessages;
        
    public:
        template<typename T>
        ValidationResult validate(const SecureMessage<T>& msg) {
            ValidationResult result;
            
            // Check magic number
            if (msg.magic != 0x54313650) {
                result.addError({
                    .type = ValidationError::INVALID_MAGIC,
                    .currentValue = msg.magic,
                    .validRange = {0x54313650, 0x54313650},
                    .isAutoFixable = false,
                    .message = "Invalid protocol magic number"
                });
                return result;
            }
            
            // Check protocol version compatibility
            if (!isVersionCompatible(msg.version)) {
                result.addError({
                    .type = ValidationError::VERSION_MISMATCH,
                    .currentValue = msg.version,
                    .validRange = {0x0001, 0x0001},
                    .isAutoFixable = false,
                    .message = "Protocol version not supported"
                });
            }
            
            // Check for replay attacks
            if (processedMessages.find(msg.messageId) != processedMessages.end()) {
                result.addError({
                    .type = ValidationError::REPLAY_ATTACK,
                    .currentValue = msg.messageId,
                    .isAutoFixable = false,
                    .message = "Message already processed (replay attack)"
                });
                return result;
            }
            
            // Validate timestamp
            if (lastTimestamp > 0 && msg.timestamp < lastTimestamp - MAX_CLOCK_SKEW) {
                result.addError({
                    .type = ValidationError::STALE_MESSAGE,
                    .currentValue = msg.timestamp,
                    .isAutoFixable = false,
                    .message = "Message timestamp too old"
                });
            }
            
            // Check sequence number
            if (msg.sequenceNumber != expectedSequence) {
                result.addWarning({
                    .type = ValidationWarning::UNKNOWN_PARAMETER,
                    .message = "Message out of sequence",
                    .severity = WarningLevel::MODERATE
                });
            }
            
            // Verify payload size
            if (msg.payloadSize != sizeof(T)) {
                result.addError({
                    .type = ValidationError::SIZE_MISMATCH,
                    .currentValue = msg.payloadSize,
                    .validRange = {sizeof(T), sizeof(T)},
                    .isAutoFixable = false,
                    .message = "Payload size mismatch"
                });
                return result;
            }
            
            // Compute and verify checksum
            uint32_t computed = computeChecksum(&msg.payload, msg.payloadSize, msg.checksumType);
            if (computed != msg.checksum) {
                result.addError({
                    .type = ValidationError::CHECKSUM_FAILURE,
                    .currentValue = msg.checksum,
                    .validRange = {computed, computed},
                    .isAutoFixable = false,
                    .message = "Data corruption detected (checksum mismatch)"
                });
                return result;
            }
            
            // Update state for next message
            if (result.isValid) {
                processedMessages.insert(msg.messageId);
                if (processedMessages.size() > MAX_MESSAGE_HISTORY) {
                    // Remove oldest entries to prevent memory growth
                    processedMessages.clear();
                }
                lastTimestamp = msg.timestamp;
                expectedSequence = msg.sequenceNumber + 1;
            }
            
            return result;
        }
        
        void reset() {
            expectedSequence = 0;
            lastTimestamp = 0;
            processedMessages.clear();
        }
        
    private:
        bool isVersionCompatible(uint16_t version) {
            return version == 0x0001; // Currently only support version 1
        }
        
        uint32_t computeChecksum(const void* data, size_t size, ChecksumType type);
    };
    
    // CRC implementations
    uint8_t crc8(const void* data, size_t size);
    uint16_t crc16_ccitt(const void* data, size_t size);
    uint32_t crc32(const void* data, size_t size);
}

#endif // DATA_INTEGRITY_HPP