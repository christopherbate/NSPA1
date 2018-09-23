#ifndef BASIC_TEST_H
#define BASIC_TEST_H

bool TestSocketCreation();
bool TestSocketData();
bool TestTimeout();

bool TestCTBDevice();
bool TestCTBDeviceData();
bool TestCTBDeviceHandshake();

bool TestSendBuffer();

bool TestSendFile();

bool TestUpdateSend();
bool TestUpdateRecv();

bool TestRecvBuffer();

bool TestHelpers();

#endif