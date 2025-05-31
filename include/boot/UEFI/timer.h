#ifndef __TIMER_H__
#define __TIMER_H__

#include <efi/efi.h>
#include <efi/efilib.h>

static EFI_STATUS sleep(UINT64 time) {
    EFI_STATUS status;

    EFI_EVENT sleepEvt;
    status = uefi_call_wrapper(BS->CreateEvent, 5, EVT_TIMER, TPL_CALLBACK, NULL, NULL, &sleepEvt);
    status = uefi_call_wrapper(BS->SetTimer, 3, sleepEvt, TimerRelative, time);
    status = uefi_call_wrapper(BS->WaitForEvent, 3, 1, &sleepEvt, NULL);
    return status;
}

static EFI_STATUS sleep_s(UINT64 time) {
    EFI_STATUS status;

    EFI_EVENT sleepEvt;
    status = uefi_call_wrapper(BS->CreateEvent, 5, EVT_TIMER, TPL_CALLBACK, NULL, NULL, &sleepEvt);
    status = uefi_call_wrapper(BS->SetTimer, 3, sleepEvt, TimerRelative, time * 10 * 1000 * 1000);
    status = uefi_call_wrapper(BS->WaitForEvent, 3, 1, &sleepEvt, NULL);
    return status;
}

#endif