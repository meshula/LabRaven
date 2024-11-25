//
//  LabFileDialogManager.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 8/4/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "LabFileDialogManager.hpp"
#include "nfd.h"
#include <stdlib.h>

#ifdef __APPLE__
#include <Foundation/Foundation.h>
#include <dispatch/dispatch.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

namespace lab {

void FileDialogManager::runOnMainThread(std::function<void()> func) {
#ifdef __APPLE__
    if ([NSThread isMainThread]) {
        func();
    }
    else {
        dispatch_async(dispatch_get_main_queue(), ^{
            func();
        });
    }
#elif _WIN32
    if (GetCurrentThreadId() == GetWindowThreadProcessId(GetForegroundWindow(), nullptr)) {
        func();
    } 
    else {
        // Post the function to the main GUI thread
        // Use a custom mechanism or library like Boost.Asio, Qt, or implement a message queue
        // For simplicity, this is left as a placeholder
        // PostToMainThread(func);
    }
#elif __linux__
    // Linux-specific main thread check
    // If using a framework like GTK or Qt, use their specific functions
    // For example, with GTK: g_idle_add(func);
    // For Qt: QCoreApplication::postEvent or QMetaObject::invokeMethod

    // Placeholder for a custom mechanism or framework-specific solution
    // If using a custom message loop, post the function to it
    // CustomPostToMainThread(func);
    func(); // Placeholder: Directly call the function (unsafe if not on the main thread)
#else
    // Default action for unsupported platforms or assume single-threaded environment
    func();
#endif
}

auto FileDialogManager::RequestOpenFile(const std::vector<std::string>& extensions, const std::string& dir) -> int {
    std::string ext = "";
    std::string path = ".";
    int id = _requestId++;
    
    if (!extensions.empty()) {
        ext = extensions[0];
        for (size_t i = 1; i < extensions.size(); ++i) {
            ext += ", " + extensions[i];
        }
    }
    if (!dir.empty()) {
        path = dir;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _fileRequestStatus[id] = FileReq::notReady;
    }

    auto openFileDialog = [this, id, ext, path]() -> void {
        nfdchar_t* outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog(ext.c_str(), path.c_str(), &outPath);
        std::lock_guard<std::mutex> lock(_mutex);
        if (result == NFD_OKAY) {
            _fileRequests[id] = outPath;
            _fileRequestStatus[id] = FileReq::ready;
            free(outPath);
        } else {
            _fileRequests[id] = "";
            _fileRequestStatus[id] = (result == NFD_CANCEL) ? FileReq::canceled : FileReq::expired;
        }
    };
    
    runOnMainThread(openFileDialog);
    return id;
}


auto FileDialogManager::PopOpenedFile(int id) -> FileReq {
    std::lock_guard<std::mutex> lock(_mutex);
    auto statusIt = _fileRequestStatus.find(id);
    if (statusIt == _fileRequestStatus.end()) {
        return { "", FileReq::expired };
    }
    
    auto status = statusIt->second;
    if (status == FileReq::notReady) {
        return { "", FileReq::notReady };
    }
    
    auto pathIt = _fileRequests.find(id);
    if (pathIt != _fileRequests.end()) {
        std::string path = pathIt->second;
        _fileRequests.erase(pathIt);
        _fileRequestStatus.erase(statusIt);
        return { path, status };
    }
    
    return { "", FileReq::expired };
}


} // lab
