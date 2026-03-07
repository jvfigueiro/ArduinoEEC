// Cart.cpp - Implementation of EEC-IV Communication Protocol
#include "Cart.h"

const uint8_t Cart::startMessage[11] = {
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x04, 0x00, 0x00, 0x00, 0x05
};

Cart::Cart(Stream* serial, uint8_t pin_re) {
  this->serial = serial;
  this->pin_re = pin_re;
  pinMode(pin_re, OUTPUT);
  reset();
}

void Cart::reset() {
  currentDiagnosticMode = 0;
  isSynced = false;
  frameDone = true;
  enableDiagnosticParameterSending = false;
  diagnosticParameterSendingDone = false;
  enablePidMapSending = false;
  pidMapSendingDone = false;
  hasData = false;

  mode = WAIT_SYNC;
  diagnosticParameterPointer = 0;
  pidMapPointer = 0;
  frameNumber = 0;
  wordBufferPointer = 0;
  dataWordCounter = 0;

  digitalWrite(pin_re, RE_READ);
}

void Cart::sendStartMessage() {
  digitalWrite(pin_re, RE_WRITE);
  delayMicroseconds(100);
  for (uint8_t i = 0; i < sizeof(startMessage); i++) {
    serial->write(startMessage[i]);
    serial->flush();
    delayMicroseconds(500);
  }
  delayMicroseconds(100);
  digitalWrite(pin_re, RE_READ);
}

void Cart::setBaudrate(long baudrate) {
  if (serial->available()) {
    while (serial->read() >= 0);
  }
  ((HardwareSerial*)serial)->begin(baudrate);

  switch (baudrate) {
    case 2400:
      delay.word = 1700;
      delay.byte = 60;
      break;
    case 9600:
      delay.word = 426;
      delay.byte = 15;
      break;
    case 19200:
      delay.word = 213;
      delay.byte = 7;
      break;
  }

  isSynced = false;
}

void Cart::loop() {
  if (mode == DIAG_PARAM_SLOT) {
    if (enableDiagnosticParameterSending && diagnosticParameterPointer == idSlot.frameNumber * 2) {
      delayMicroseconds(delay.word);
      digitalWrite(pin_re, RE_WRITE);
      serial->write(diagnosticParameter[diagnosticParameterPointer]);
      delayMicroseconds(delay.byte);
      serial->write(diagnosticParameter[diagnosticParameterPointer + 1]);
      digitalWrite(pin_re, RE_READ);
      diagnosticParameterPointer += 2;
      if (diagnosticParameterPointer > 7) {
        diagnosticParameterPointer = 0;
        diagnosticParameterSendingDone = true;
      }
    }
    mode = DATA_SLOT;
    return;
  }

  if (mode == DATA_SLOT) {
    if (enablePidMapSending && pidMapPointer == idSlot.frameNumber && pidMapPointer < 4) {
      Serial.print(F("[DEBUG] Enviando bloco de PIDs: "));
      Serial.println(pidMapPointer);

      digitalWrite(pin_re, RE_WRITE);
      for (uint8_t i = 0; i < 4; i++) {
        uint8_t pid = pidMap[pidMapPointer][i];
        delayMicroseconds(delay.word);
        serial->write(pid);
        delayMicroseconds(delay.byte);
        serial->write(PID_CHECKSUM(pid));
      }
      pidMapPointer++;
      if (pidMapPointer >= 4) {
        Serial.println(F("[DEBUG] Finalizado envio de PID MAP"));
        pidMapSendingDone = true;
        enablePidMapSending = false;
        pidMapPointer = 0;
      }
      digitalWrite(pin_re, RE_READ);
      mode = WAIT_SYNC;
    }
  }

  if (pushAvailableToBuffer()) {
    wordBufferPointer++;
    if (wordBufferPointer < 2) return;

    if (isBufferSync()) {
      mode = ID_SLOT;
      wordBufferPointer = 0;
      return;
    }

    switch (mode) {
      case WAIT_SYNC:
        return;
      case ID_SLOT:
        if (frameNumber > 15) {
          frameNumber = 0;
          isSynced = true;
          frameDone = true;
        }
        memcpy(&idSlot, wordBuffer, 2);
        if (((idSlot.rpm & 0xF) ^ ((idSlot.rpm >> 4) & 0xF) ^ idSlot.frameNumber ^ 0xA) != idSlot.parity) {
          frameNumber = 0;
          mode = WAIT_SYNC;
          return;
        }
        if (frameNumber != idSlot.frameNumber) {
          frameNumber = 0;
          mode = WAIT_SYNC;
          return;
        }
        frameNumber++;
        dataWordCounter = 0;
        mode = (idSlot.frameNumber < 4) ? DIAG_PARAM_SLOT : STATUS_SLOT;
        break;
      case STATUS_SLOT:
        handleStatusSlot();
        mode = DATA_SLOT;
        break;
      case DATA_SLOT:
        dataWordCounter++;
        if (dataWordCounter > 5) {
          mode = WAIT_SYNC;
          break;
        }
        memcpy(data, wordBuffer, 2);
        hasData = true;
        break;
      default:
        break;
    }
    wordBufferPointer = 0;
  }
}

void Cart::handleStatusSlot() {
  switch (idSlot.frameNumber) {
    case CURRENT_DIAGNOSTIC_MODE:
      currentDiagnosticMode = wordBuffer[0];
      break;
    case NEXT_DIAGNOSTIC_MODE:
      nextDiagnosticMode = wordBuffer[0];
      break;
    case DCL_ERROR_FLAG_LOW:
      memcpy(&dclErrorFlagLow, wordBuffer, 1);
      break;
    case DCL_ERROR_FLAG_HIGH:
      memcpy(&dclErrorFlagHigh, wordBuffer, 1);
      break;
  }
}

void Cart::getData(uint8_t* out) {
  memcpy(out, data, 2);
  hasData = false;
}

void Cart::setDiagnosticParameter(const uint8_t diagnosticParameter[]) {
  memcpy(this->diagnosticParameter, diagnosticParameter, 8);
  enableDiagnosticParameterSending = true;
  diagnosticParameterSendingDone = false;
}

void Cart::setPidMap(const uint8_t pidMap[12][4]) {
  memcpy(this->pidMap, pidMap, sizeof(this->pidMap));
  enablePidMapSending = true;
  pidMapSendingDone = false;
}

bool Cart::isBufferSync() {
  return !(wordBuffer[0] | wordBuffer[1]);
}

uint8_t Cart::pushAvailableToBuffer() {
  if (serial->available()) {
    pushBuffer(serial->read());
    return 1;
  }
  return 0;
}

void Cart::pushBuffer(uint8_t val) {
  wordBuffer[0] = wordBuffer[1];
  wordBuffer[1] = val;
}