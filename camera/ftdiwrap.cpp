#include "ftdiwrap.h"

#include "ftd2xx.h"
#include "QDebug"


// 线阵探测器 接口
FtdiWrap::FtdiWrap()
{

}

int FtdiWrap::Open()
{
    DWORD numDevs;
    FT_STATUS ftStatus;
    FT_HANDLE ftHandle;
    DWORD RxBytes;
    DWORD TxBytes;
    DWORD EventDWord;
    char RxBuffer[4096];
    DWORD BytesReceived;

    ftStatus = FT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
    qDebug("FT_ListDevices(): ftStatus: %i, number: %i", ftStatus, numDevs);

//    Parameters
//    iDevice Index of the device to open. Indices are 0 based.
//    ftHandle Pointer to a variable of type FT_HANDLE where the handle will be
//    stored. This handle must be used to access the device.
//    Return Value
//    FT_OK if successful, otherwise the return value is an FT error code.
    ftStatus = FT_Open(0, &ftHandle);
    qDebug("FT_Open(): ftStatus: %i", ftStatus);
    if (ftStatus != FT_OK) {
        return 1;
    }

    ftStatus = FT_ResetDevice(ftHandle);
    qDebug("FT_ResetDevice(): ftStatus: %i", ftStatus);

    // Purge both Rx and Tx buffers
    ftStatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
    qDebug("FT_Purge(): ftStatus: %i", ftStatus);

    ftStatus = FT_ResetDevice(ftHandle);
    qDebug("FT_ResetDevice(): ftStatus: %i", ftStatus);

    ftStatus = FT_SetTimeouts(ftHandle, 3000, 3000);
    qDebug("FT_SetTimeouts(): ftStatus: %i", ftStatus);

    Sleep(500);


    //    Summary
    //    Write data to the device.
    //    Definition
    //    FT_STATUS FT_Write (FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD dwBytesToWrite,
    //     LPDWORD lpdwBytesWritten)
    //    Parameters
    //    ftHandle Handle of the device.
    //    lpBuffer Pointer to the buffer that contains the data to be written to the
    //    device.
    //    dwBytesToWrite Number of bytes to write to the device.
    //    lpdwBytesWritten Pointer to a variable of type DWORD which receives the number of
    //    bytes written to the device.
//    char txBuffer[] = "#CSDTP:1%";
    char txBuffer[] = "#?data%";
//    char txBuffer[] = "#Text:0%";
//    char txBuffer[] = "#Text:1%";
    DWORD bytesWritten;
    int length = sizeof(txBuffer);
    qDebug("buffer size: %i", length);
    ftStatus = FT_Write(ftHandle, &txBuffer, length - 1, &bytesWritten);
    qDebug("FT_Write(): ftStatus: %i, written length: %i", ftStatus, bytesWritten);

//    Summary
//    Gets the device status including number of characters in the receive queue, number of characters in the
//    transmit queue, and the current event status.
//    Definition
//    FT_STATUS FT_GetStatus (FT_HANDLE ftHandle, LPDWORD lpdwAmountInRxQueue,
//    LPDWORD lpdwAmountInTxQueue, LPDWORD lpdwEventStatus)
//    Parameters
//    ftHandle Handle of the device.
//    lpdwAmountInRxQueue Pointer to a variable of type DWORD which receives the number of characters in
//    the receive queue.
//    lpdwAmountInTxQueue Pointer to a variable of type DWORD which receives the number of characters in
//    the transmit queue.
//    lpdwEventStatus Pointer to a variable of type DWORD which receives the current state of
//    the event status.
    FT_GetStatus(ftHandle, &RxBytes, &TxBytes, &EventDWord);
    qDebug("FT_GetStatus(): ftStatus: %i, read length: %i, send length: %i", ftStatus, RxBytes, TxBytes);

//    if (RxBytes > 0) {

//        Summary
//        Read data from the device.
//        Definition
//        FT_STATUS FT_Read (FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD dwBytesToRead, LPDWORD lpdwBytesReturned)
//        Parameters
//        ftHandle Handle of the device.
//        lpBuffer Pointer to the buffer that receives the data from the device.
//        dwBytesToRead Number of bytes to be read from the device.
//        lpdwBytesReturned Pointer to a variable of type DWORD which receives the number of
//        bytes read from the device.
        ftStatus = FT_Read(ftHandle, RxBuffer, RxBytes, &BytesReceived);
        qDebug("FT_Read(): ftStatus: %i, length: %i, %i", ftStatus, RxBytes, BytesReceived);
//    }

    FT_Close(ftHandle);
    qDebug("FT_Close(): ftStatus: %i", ftStatus);

    return 0;
}
