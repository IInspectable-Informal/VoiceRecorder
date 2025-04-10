#pragma once
//WinRT
#include <winrt\Windows.Storage.h>
#include <winrt\Windows.Storage.AccessCache.h>

namespace winrt
{
    const Windows::Storage::ApplicationDataContainer AppDC = Windows::Storage::ApplicationData::Current().LocalSettings();
    const Windows::Storage::AccessCache::StorageItemAccessList FA = Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList();
}