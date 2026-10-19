/*

  Copyright (c) 2016 Hubert Denkmair <hubert@denkmair.de>
  Copyright (c) 2026 Schildkroet

  This file is part of cangaroo.

  cangaroo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  cangaroo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cangaroo.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "api/candle_defs.h"
#include "CandleApiDriver.h"
#include "api/candle.h"

#include "core/Log.h"
#include "CandleApiInterface.h"
#include "driver/GenericCanSetupPage.h"

#include <algorithm>
#include <cwctype>
#include <QString>

// Composite USB devices expose each CAN interface as a separate Windows device
// path containing "&mi_XX" (e.g. "&mi_00", "&mi_02"). Strip that segment so
// we can recognise multiple interface paths as the same physical device.
// Windows returns device paths in lowercase, so compare case-insensitively.
static std::wstring baseDevicePath(const std::wstring &path)
{
    // Normalise to lowercase so the map key is consistent regardless of capitalisation.
    std::wstring result = path;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);

    // Pass 1: strip every &mi_NNNN segment.
    // On serial-number-based composite USB devices &mi_XX appears in BOTH the
    // hardware-ID part AND the instance-ID part of the path, so a single erase
    // is not enough.  Loop until all occurrences are gone.
    const std::wstring mi_tag = L"&mi_";
    auto pos = result.find(mi_tag);
    while (pos != std::wstring::npos) {
        auto end = pos + mi_tag.size();
        while (end < result.size() && iswxdigit(result[end]))
            ++end;
        result.erase(pos, end - pos);
        pos = result.find(mi_tag, pos);
    }

    // Pass 2: for location-based instance IDs (no USB serial number) the
    // interface number is the last segment before #{GUID}, encoded as exactly
    // 4 hex digits (e.g. &0000 for MI_00, &0002 for MI_02).  Strip it so all
    // interfaces of the same physical device share the same key.
    const auto guid_pos = result.rfind(L"#{");
    if (guid_pos >= 5 && result[guid_pos - 5] == L'&') {
        bool all_hex = true;
        for (std::size_t k = guid_pos - 4; k < guid_pos; ++k) {
            if (!iswxdigit(result[k])) { all_hex = false; break; }
        }
        if (all_hex)
            result.erase(guid_pos - 5, 5);
    }

    return result;
}


// Map known VID/PID combinations to human-readable product names.
// The Windows device path is lower-case and contains "vid_XXXX&pid_XXXX".
static QString productNameFromPath(const std::wstring &path)
{
    std::wstring lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    auto extract = [&](const wchar_t *tag) -> uint16_t {
        auto pos = lower.find(tag);
        if (pos == std::wstring::npos)
            return 0;
        return static_cast<uint16_t>(wcstoul(lower.c_str() + pos + wcslen(tag), nullptr, 16));
    };

    const uint16_t vid = extract(L"vid_");
    const uint16_t pid = extract(L"pid_");

    if (vid == 0x1209 && pid == 0xCA01) return QStringLiteral("CANnectivity");

    return QStringLiteral("candle");
}

CandleApiDriver::CandleApiDriver(Backend &backend)
  : CanDriver(backend),
    setupPage(new GenericCanSetupPage(0))
{
    candle_log_fn = nullptr;
    candle_log_verbose = false;
    QObject::connect(&backend, &Backend::onSetupDialogCreated, setupPage, &GenericCanSetupPage::onSetupDialogCreated);
}

QString CandleApiDriver::getName()
{
    return "CandleAPI";
}

bool CandleApiDriver::update()
{
    // Destroy existing interfaces first, then release shared devices.
    // Interfaces hold shared_ptr refs to the devices, so deleting them first
    // ensures the devices are freed in the correct order.
    deleteAllInterfaces();
    _devices.clear();

    candle_list_handle clist;
    if (!candle_list_scan(&clist)) {
        return true;
    }

    uint8_t num_devices = 0;
    if (!candle_list_length(clist, &num_devices)) {
        candle_list_free(clist);
        return true;
    }

    int deviceCounter = 0;
    for (uint8_t i = 0; i < num_devices; i++) {
        candle_handle dev;
        if (!candle_dev_get(clist, i, &dev)) {
            log_error(QStringLiteral("CandleAPI: candle_dev_get failed for index %1").arg(static_cast<int>(i)));
            continue;
        }

        // Open temporarily to read channel count and per-channel capabilities.
        if (!candle_dev_open(dev)) {
            log_error(QStringLiteral("CandleAPI: discovery open failed for index %1").arg(static_cast<int>(i)));
            candle_dev_free(dev);
            continue;
        }

        uint8_t num_channels = 0;
        if (!candle_channel_count(dev, &num_channels) || num_channels == 0) {
            candle_dev_close(dev);
            candle_dev_free(dev);
            continue;
        }

        const std::wstring devPath(candle_dev_get_path(dev));
        const std::wstring baseKey = baseDevicePath(devPath);

        // Skip additional USB interfaces of a device we already processed.
        // On composite devices each CAN channel has its own interface path
        // (MI_00, MI_02, …) but they all share the same USB endpoint and
        // the firmware reports the total icount from any interface.
        if (_devices.count(baseKey)) {
            candle_dev_close(dev);
            candle_dev_free(dev);
            continue;
        }

        // Create a single shared handle for all channels on this device.
        // candle_dev_get() allocates a fresh candle_device_t; we pre-populate
        // its capability fields so getAvailableBitrates() works before open().
        candle_handle shared_handle;
        candle_dev_get(clist, i, &shared_handle);
        {
            candle_device_t *src = static_cast<candle_device_t*>(dev);
            candle_device_t *dst = static_cast<candle_device_t*>(shared_handle);
            wcscpy(dst->path, src->path);
            dst->state   = src->state;
            dst->dconf   = src->dconf;
            dst->bt_const = src->bt_const;
            memcpy(dst->ch_caps, src->ch_caps, sizeof(src->ch_caps));
        }

        candle_dev_close(dev);
        candle_dev_free(dev);

        auto sharedDev = std::make_shared<CandleSharedDevice>();
        sharedDev->handle = shared_handle;
        sharedDev->productName = productNameFromPath(devPath);
        sharedDev->deviceIndex = deviceCounter++;
        _devices[baseKey] = sharedDev;

        // One CanInterface per channel, all sharing the same physical device.
        for (uint8_t ch = 0; ch < num_channels; ch++) {
            addInterface(new CandleApiInterface(this, sharedDev, ch));
        }
    }

    candle_list_free(clist);
    return true;
}
