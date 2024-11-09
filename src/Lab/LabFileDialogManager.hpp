//
//  LabFileDialogManager.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 8/4/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace lab {

class FileDialogManager {
    int _requestId = 1;
    std::mutex _mutex;

public:
    FileDialogManager() = default;
    ~FileDialogManager() = default;

    auto requestOpenFile(const std::vector<std::string>& extensions, const std::string& dir) -> int;
    struct FileReq {
        enum Result { notReady, canceled, ready, expired };
        std::string path;
        Result status;
    };
    auto popOpenedFile(int id) -> FileReq;

private:
    std::map<int, std::string> _fileRequests;
    std::map<int, FileReq::Result> _fileRequestStatus;
    void runOnMainThread(std::function<void()> func);
};

} // lab
