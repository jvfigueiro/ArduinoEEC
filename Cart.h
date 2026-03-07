// Cart.h - Header for EEC-IV Communication Protocol
#pragma once

#include <Arduino.h>

#define RE_READ 0x0
#define RE_WRITE 0x1

#define PID_CHECKSUM(pid) ((((pid & 0xF) ^ ((pid >> 4) & 0xF) ^ 0x8 ^ 0xA) << 4 ) | 0x8)

class Cart {
  public:
    struct IdSlot {
      unsigned int rpm : 8;
      unsigned int frameNumber : 4;
      unsigned int parity : 4;
    } idSlot;

    struct DclErrorFlagLow {
      unsigned int loadAddrPartiy : 1;
      unsigned int loadAddrBadValue : 1;
      unsigned int dataChecksumPartiy : 1;
      unsigned int incorrectChecksum : 1;
      unsigned int adValuesParityError : 1;
      unsigned int pidMapParityError : 1;
      unsigned int dmrMapParityError : 1;
      unsigned int unused : 1;
    } dclErrorFlagLow;

    struct DclErrorFlagHigh {
      unsigned int unused : 2;
      unsigned int executeVectorParityError : 1;
      unsigned int executeVectorIncorrectChecksum : 1;
      unsigned int badDiagParameterSlot : 1;
      unsigned int eecInReset : 1;
      unsigned int selfTestComplete : 1;
      unsigned int background : 1;
    } dclErrorFlagHigh;

    Cart(Stream* serial, uint8_t pin_re);

    void reset();
    void setBaudrate(long baudrate);
    void sendStartMessage();
    void setDiagnosticParameter(const uint8_t diagnosticParameter[]);
    void setPidMap(const uint8_t pidMap[12][4]);
    void loop();
    void getData(uint8_t* data);

    bool hasData = false;
    bool enableDiagnosticParameterSending = false;
    bool diagnosticParameterSendingDone = false;
    bool enablePidMapSending = false;
    bool pidMapSendingDone = false;
    bool isSynced = false;
    bool frameDone = true;

    uint8_t currentDiagnosticMode = 0;
    uint8_t nextDiagnosticMode = 0;
    uint8_t dataWordCounter = 0;

  private:
    enum Mode {
      WAIT_SYNC, ID_SLOT, DIAG_PARAM_SLOT, STATUS_SLOT, DATA_SLOT,
    } mode = WAIT_SYNC;

    struct delay_s {
      uint16_t word;
      uint16_t byte;
    } delay;

    enum StatusSlotType {
      CURRENT_DIAGNOSTIC_MODE = 0x4,
      NEXT_DIAGNOSTIC_MODE = 0x5,
      DCL_ERROR_FLAG_LOW = 0x6,
      DCL_ERROR_FLAG_HIGH = 0x7,
    };

    static const uint8_t startMessage[11];

    Stream* serial;
    uint8_t pin_re;

    uint8_t diagnosticParameter[8];
    uint8_t diagnosticParameterPointer = 0;

    uint8_t pidMap[12][4];
    uint8_t pidMapPointer = 0;

    uint8_t frameNumber = 0;
    uint8_t wordBufferPointer = 0;
    uint8_t wordBuffer[2];
    uint8_t data[2];

    bool isBufferSync();
    uint8_t pushAvailableToBuffer();
    void pushBuffer(uint8_t val);
    void handleStatusSlot();
};