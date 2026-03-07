// EecIv.cpp - Implementation for EEC-IV Engine Control Interface
#include "EecIv.h"

// Definição da variável estática pidMap
const uint8_t EecIv::pidMap[12][4] = {
  { 0x10, 0x11, 0x12, 0x13 },
  { 0x14, 0x15, 0x16, 0x17 },
  { 0x18, 0x19, 0x1A, 0x1B },
  { 0x1C, 0x1D, 0x1E, 0x1F },
  { 0x20, 0x21, 0x22, 0x23 },
  { 0x24, 0x25, 0x26, 0x27 },
  { 0x28, 0x29, 0x2A, 0x2B },
  { 0x2C, 0x2D, 0x2E, 0x2F },
  { 0x30, 0x31, 0x32, 0x33 },
  { 0x34, 0x35, 0x36, 0x37 },
  { 0x38, 0x39, 0x3A, 0x3B },
  { 0x3C, 0x3D, 0x3E, 0x3F },
};

EecIv::EecIv(Cart* cart) {
  this->cart = cart;
  currentState = IDLE;
  syncPointer = 0;
  errorCodePointer = 0;
  loopCounter = 0;
  koeoCounter = 0;
  timeoutTimer = 0;
  startMessageCounter = 0;
  mode = READ_FAULTS;
}

void EecIv::setMode(EecIv::OperationMode mode) {
  this->mode = mode;
}

void EecIv::restartReading() {
  cart->reset();
  currentState = CHECK_IF_IN_DIAG_MODE;
  startMessageCounter = 0;
  loopCounter = 0;
  koeoCounter = 0;
  syncPointer = 0;
  errorCodePointer = 0;
  liveDataOffset = 0;
  liveDataLastFrame = 0;
  initTimeoutTimer();
}

int EecIv::exceededTimeout() {
  return (millis() - timeoutTimer) > timeoutMax;
}

void EecIv::initTimeoutTimer() {
  timeoutTimer = millis();
}

void EecIv::mainLoop() {
  cart->loop();

  switch (currentState) {
    case IDLE:
      break;

    case CHECK_IF_IN_DIAG_MODE:
      if (cart->isSynced) {
        if (debugPrint) debugPrint("Already in diag mode");
        currentState = REQUEST_CLEAR_DCL_ERRORS;
      } else if (exceededTimeout()) {
        if (debugPrint) debugPrint("Exceeded waiting for sync 2400");
        currentState = SEND_START_MESSAGE;
      }
      break;

    case SEND_START_MESSAGE:
      if (debugPrint) debugPrint("Sending start message");
      cart->sendStartMessage();
      currentState = CHANGE_BAUD_RATE_9600;
      break;

    case CHANGE_BAUD_RATE_9600:
      cart->setBaudrate(9600);
      initTimeoutTimer();
      currentState = WAIT_FOR_SYNC_9600;
      break;

    case WAIT_FOR_SYNC_9600:
      if (cart->isSynced) {
        if (debugPrint) debugPrint("Synced with default baud");
        
        // Based on mode, determine next state
        if (mode == LIVE_DATA) {
          currentState = REQUEST_LIVE_DATA;
        } else if (mode == READ_FAULTS) {
          currentState = REQUEST_CONT_SELF_TEST_CODES;
        } else if (mode == KOEO) {
          currentState = REQUEST_KOEO;
        } else {
          currentState = IDLE;
        }
      } else if (exceededTimeout()) {
        if (debugPrint) debugPrint("Exceeded waiting for sync in default baud");
        currentState = SEND_START_MESSAGE;
      }
      break;

    case REQUEST_CONT_SELF_TEST_CODES:
      if (debugPrint) debugPrint("Requesting self test codes");
      // This would include code to request self test codes
      // For now, just moving to IDLE as this is not fully implemented
      currentState = IDLE;
      break;

    case REQUEST_KOEO:
      if (debugPrint) debugPrint("Requesting KOEO test");
      // This would include code to request KOEO test
      // For now, just moving to IDLE as this is not fully implemented
      currentState = IDLE;
      break;

    case REQUEST_LIVE_DATA:
      if (debugPrint) debugPrint("Requesting LIVE DATA");
      cart->setPidMap(pidMap);
      currentState = WAIT_REQUEST_LIVE_DATA_DONE;
      break;

    case WAIT_REQUEST_LIVE_DATA_DONE:
      if (cart->pidMapSendingDone) {
        if (debugPrint) debugPrint("LIVE DATA ready");
        currentState = RECEIVE_LIVE_DATA;
      }
      break;

    case RECEIVE_LIVE_DATA:
      if (cart->hasData) {
        uint8_t data[2];
        cart->getData(data);
        if (onLiveData) onLiveData(data);
      }
      break;

    case REQUEST_CLEAR_DCL_ERRORS:
      if (debugPrint) debugPrint("Clearing DCL errors");
      // Add code here to clear DCL errors
      // For now just proceed to next appropriate state
      
      if (mode == LIVE_DATA) {
        currentState = REQUEST_LIVE_DATA;
      } else if (mode == READ_FAULTS) {
        currentState = REQUEST_CONT_SELF_TEST_CODES;
      } else if (mode == KOEO) {
        currentState = REQUEST_KOEO;
      } else {
        currentState = IDLE;
      }
      break;

    default:
      break;
  }
}