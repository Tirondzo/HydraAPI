#ifndef HYDRAAPI_EX_GRID_JOINER_H
#define HYDRAAPI_EX_GRID_JOINER_H

#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <set>
#include <HR_HDRImageTool.h>
#include <pugixml.hpp>
#include <regex>
#include <HydraInternal.h>
#include <HydraLegacyUtils.h>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/types.h
#endif

#include <iostream>

// Grid Joiner can be a separated project with main function and IPC'ed with ImageAccum
// For now, it's so simple, so it's just a class that executed and managed with std::thread

// As an concept version it just averages images to result <=> res = (a+b+c)/3
// It can be advanced to join with batches technique, gaussian pyramid, whatever...
// Also some parts can be moved to GPU

class GRID_Joiner {
    static const int UPDATE_INTERVAL_MS = 500;

    std::set<std::string> processed_imgs;
    std::string processed_imgs_xml;
    std::string work_folder;
    std::string result_img;

    int w, h;
    void parseProcessedImgs();
    void serializeProcessedImgs();
    void mergeImage(const std::string& str);

    std::atomic_uint processed_count;
public:
    GRID_Joiner() : GRID_Joiner(0, 0, "","",""){};
    GRID_Joiner(int w, int h, std::string work_folder, std::string merged_xml, std::string result_img);

    std::atomic_bool active;
    size_t getProcessed() const;

    void start();
};

namespace GRID_Utils{
    std::string pathAppend(const std::string& p1, const std::string& p2);
    void mkdir_all(std::wstring path);
}

#endif //HYDRAAPI_EX_GRID_JOINER_H
