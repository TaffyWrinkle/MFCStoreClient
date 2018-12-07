// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "WindowsStore.h"
#include "WindowsStoreImpl.h"
#include <appmodel.h>
#include <roapi.h>

namespace WinRT
{
    bool isRunningInsideAppPackage()
    {
        UINT32 length = 0;

        auto result = GetCurrentPackageFamilyName(&length, nullptr);
        if (result != ERROR_INSUFFICIENT_BUFFER)
        {
            return false;
        }

        return true;
    }

    WindowsStoreErrorType store_initialize(WindowsStorePtr* storePtr, HWND window, WindowsStoreCallback licenseChangedCallback, void* userData)
    {
        *storePtr = nullptr;

        // Initialize the Windows Runtime.
        HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);

        if (!SUCCEEDED(hr))
        {
            return WINRT_WINDOWS_RUNTIME_ERROR;
        }

        // Check if Windows 10 Windows.Services.Store api is supported
        if (!Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.Services.Store.StoreContext"))
        {
            return WINRT_WINDOWS_VERSION_ERROR;
        }

        // Check if app is running inside of an App Package
        if (!isRunningInsideAppPackage())
        {
            return WINRT_APP_PACKAGE_ERROR;
        }

        // attempt to initialize the Windows Store WinRT Implementation
        WindowsStoreImpl* store = new WindowsStoreImpl();
        WindowsStoreErrorType result = store->Initialize(window, licenseChangedCallback, userData);
        if (result != WINRT_NO_ERROR)
        {
            delete store;
            *storePtr = nullptr;
        }
        else
        {
            *storePtr = (WindowsStorePtr)store;
        }

        return result;
    }

    void store_purchase(WindowsStorePtr storePtr, WindowsStoreCallback callback, void* userData)
    {
        if (storePtr)
        {
            WindowsStoreImpl* pImpl = (WindowsStoreImpl*)storePtr;
            pImpl->Purchase(callback, userData);
        }
    }

    void store_license_state(WindowsStorePtr storePtr, WindowsStoreCallback callback, void* userData)
    {
        if (storePtr)
        {
            WindowsStoreImpl* pImpl = (WindowsStoreImpl*)storePtr;
            pImpl->GetLicenseState(callback, userData);
        }
    }

    void store_get_price(WindowsStorePtr storePtr, WindowsStoreCallback callback, void* userData)
    {
        if (storePtr)
        {
            WindowsStoreImpl* pImpl = (WindowsStoreImpl*)storePtr;
            pImpl->GetPrice(callback, userData);
        }
    }

    void store_free(WindowsStorePtr storePtr)
    {
        RoUninitialize();
        if (storePtr)
        {
            delete storePtr;
        }
    }
}

