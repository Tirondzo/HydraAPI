//
// Created by Alex on 14.04.2019.
//

#include "HydraRenderDriverAPI.h"

#include <vector>
#include <string>
#include <sstream>
#include <fstream>

//#include "HydraInternal.h" // #TODO: this is only for hr_mkdir and hr_cleardir. Remove this further

//TODO: Cleanup this as much as possible

#include "HydraLegacyUtils.h"

#include "HydraXMLHelpers.h"

#include "HR_HDRImage.h"
#include "HydraInternal.h"

#include <mutex>
#include <future>
#include <thread>

#include "ssemath.h"
#include "vfloat4_x64.h"


#ifdef WIN32
#include "../clew/clew.h"
#else
#include <CL/cl.h>
#endif


#include "GRID_Joiner.h"
#include <thread>


#pragma warning(disable:4996) // for wcscpy to be ok

static constexpr bool MODERN_DRIVER_DEBUG = false;

using HydraRender::HDRImage4f;

/// <summary>das</summary>
struct RD_HydraGrid : public IHRRenderDriver
{
    RD_HydraGrid() : m_width(0), m_height(0) {};

    ~RD_HydraGrid() override
    {
        ClearAll();
        grid_joiner->active = false;
        if(grid_joiner_thread.joinable())
            grid_joiner_thread.join();
    }

    void              ClearAll() override;
    HRDriverAllocInfo AllocAll(HRDriverAllocInfo a_info) override;

    bool UpdateImage(int32_t a_texId, int32_t w, int32_t h, int32_t bpp, const void* a_data, pugi::xml_node a_texNode) override;
    bool UpdateMaterial(int32_t a_matId, pugi::xml_node a_materialNode) override;

    bool UpdateLight(int32_t a_lightIdId, pugi::xml_node a_lightNode) override;
    bool UpdateMesh(int32_t a_meshId, pugi::xml_node a_meshNode, const HRMeshDriverInput& a_input, const HRBatchInfo* a_batchList, int32_t listSize) override;

    bool UpdateImageFromFile(int32_t a_texId, const wchar_t* a_fileName, pugi::xml_node a_texNode) override;
    bool UpdateMeshFromFile(int32_t a_meshId, pugi::xml_node a_meshNode, const wchar_t* a_fileName) override;

    bool UpdateCamera(pugi::xml_node a_camNode) override;
    bool UpdateSettings(pugi::xml_node a_settingsNode) override;

    /////////////////////////////////////////////////////////////////////////////////////////////

    void BeginScene(pugi::xml_node a_sceneNode) override;
    void EndScene() override;
    void InstanceMeshes(int32_t a_mesh_id, const float* a_matrices, int32_t a_instNum, const int* a_lightInstId, const int* a_remapId, const int* a_realInstId) override;
    void InstanceLights(int32_t a_light_id, const float* a_matrix, pugi::xml_node* a_custAttrArray, int32_t a_instNum, int32_t a_lightGroupId) override;

    void Draw() override;

    HRRenderUpdateInfo HaveUpdateNow(int a_maxRaysPerPixel) override;

    void GetFrameBufferHDR(int32_t w, int32_t h, float*   a_out, const wchar_t* a_layerName) override;
    void GetFrameBufferLDR(int32_t w, int32_t h, int32_t* a_out)                             override;

    void GetFrameBufferLineHDR(int32_t a_xBegin, int32_t a_xEnd, int32_t y, float* a_out, const wchar_t* a_layerName) override;
    void GetFrameBufferLineLDR(int32_t a_xBegin, int32_t a_xEnd, int32_t y, int32_t* a_out)                           override;

    void GetGBufferLine(int32_t a_lineNumber, HRGBufferPixel* a_lineData, int32_t a_startX, int32_t a_endX, const std::unordered_set<int32_t>& a_shadowCatchers) override;

    void    LockFrameBufferUpdate()   override;
    void    UnlockFrameBufferUpdate() override;

    // info and devices
    //
    HRDriverInfo Info() override;
    const HRRenderDeviceInfoListElem* DeviceList() const override;
    bool EnableDevice(int32_t id, bool a_enable) override;

    void ExecuteCommand(const wchar_t* a_cmd, wchar_t* a_out) override;
    void EndFlush() override;

    void SetLogDir(const wchar_t* a_logDir, bool a_hideCmd) override;

protected:
    std::wstring m_libPath;
    std::wstring m_libFileState;
    HRDriverAllocInfo m_currAllocInfo;

    std::wstring m_logFolder;
    std::string  m_logFolderS;
    bool m_hideCmd   = false;

    int m_width, m_height;

    std::unique_ptr<GRID_Joiner> grid_joiner;
    std::thread grid_joiner_thread;

    size_t last_merged = 0;
};

static inline float contribFunc(float colorX, float colorY, float colorZ)
{
    return fmax(0.33334f*(colorX + colorY + colorZ), 0.0f);
}

using namespace cvex;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RD_HydraGrid::ClearAll()
{
    //TODO: Why it calls 2 times??
    //std::cout << "Called ClearAll()" << std::endl << std::flush;
}

HRDriverAllocInfo RD_HydraGrid::AllocAll(HRDriverAllocInfo a_info)
{
    m_libPath = std::wstring(a_info.libraryPath);
    if(a_info.stateFileName == nullptr)
        m_libFileState = L"";
    else
        m_libFileState = std::wstring(a_info.stateFileName);

    m_currAllocInfo = a_info;
    return m_currAllocInfo;
}

const HRRenderDeviceInfoListElem* RD_HydraGrid::DeviceList() const
{
    return nullptr;
}

bool RD_HydraGrid::EnableDevice(int32_t id, bool a_enable)
{
    return true;
}

HRDriverInfo RD_HydraGrid::Info()
{
    HRDriverInfo info;

    info.supportHDRFrameBuffer              = true;
    info.supportHDRTextures                 = true;
    info.supportMultiMaterialInstance       = true;

    info.supportImageLoadFromInternalFormat = true;
    info.supportMeshLoadFromInternalFormat  = true;

    info.supportImageLoadFromExternalFormat = true;
    info.supportLighting                    = true;
    info.createsLightGeometryItself         = false;
    info.supportGetFrameBufferLine          = true;
    info.supportUtilityPrepass              = true;
    info.supportDisplacement                = true;

    info.memTotal                           = int64_t(8) * int64_t(1024 * 1024 * 1024); // #TODO: wth i have to do with that ???

    return info;
}


void RD_HydraGrid::SetLogDir(const wchar_t* a_logDir, bool a_hideCmd)
{
    m_logFolder  = a_logDir;
    m_logFolderS = ws2s(m_logFolder);

    const std::wstring check = s2ws(m_logFolderS);
    if (m_logFolder != check)
    {
        if (m_pInfoCallBack != nullptr)
            m_pInfoCallBack(L"bad log dir", L"RD_HydraGrid::SetLogDir", HR_SEVERITY_WARNING);
    }
    m_hideCmd = a_hideCmd;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool RD_HydraGrid::UpdateImage(int32_t a_texId, int32_t w, int32_t h, int32_t bpp, const void* a_data, pugi::xml_node a_texNode)
{
    return true;
}

bool RD_HydraGrid::UpdateImageFromFile(int32_t a_texId, const wchar_t* a_fileName, pugi::xml_node a_texNode)
{
    return true;
}

bool RD_HydraGrid::UpdateMaterial(int32_t a_matId, pugi::xml_node a_materialNode)
{
    return true;
}

bool RD_HydraGrid::UpdateLight(int32_t a_lightIdId, pugi::xml_node a_lightNode)
{
    return true;
}

bool RD_HydraGrid::UpdateMesh(int32_t a_meshId, pugi::xml_node a_meshNode, const HRMeshDriverInput& a_input, const HRBatchInfo* a_batchList, int32_t listSize)
{
    return true;
}

bool RD_HydraGrid::UpdateMeshFromFile(int32_t a_meshId, pugi::xml_node a_meshNode, const wchar_t* a_fileName)
{
    return false;
}

bool RD_HydraGrid::UpdateCamera(pugi::xml_node a_camNode)
{
    return true;
}


bool RD_HydraGrid::UpdateSettings(pugi::xml_node a_settingsNode)
{
    m_width     = a_settingsNode.child(L"width").text().as_int();
    m_height    = a_settingsNode.child(L"height").text().as_int();

    //TODO: Parse settings here
    std::string work_folder = "gtest";
    std::string res_folder = GRID_Utils::pathAppend(work_folder, "merged");

    std::string merged_xml = GRID_Utils::pathAppend(res_folder, "composition.xml");
    std::string res_img = GRID_Utils::pathAppend(res_folder, "res.image4f");

    grid_joiner = std::move(std::make_unique<GRID_Joiner>(m_width, m_height, work_folder, merged_xml, res_img));
    //grid_joiner = new GRID_Joiner("gtest", "merged.xml", "res.image4f");

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::wstring GetAbsolutePath(const std::wstring& a_path);

void RD_HydraGrid::BeginScene(pugi::xml_node a_sceneNode)
{
}


void RD_HydraGrid::EndScene()
{
}

void RD_HydraGrid::EndFlush()
{
    grid_joiner_thread = std::move(std::thread(&GRID_Joiner::start, &(*grid_joiner)));
    //std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    //grid_joiner_thread.detach();
}

void RD_HydraGrid::Draw()
{
    // like glFinish();
}

void RD_HydraGrid::InstanceMeshes(int32_t a_meshId, const float* a_matrices, int32_t a_instNum, const int* a_lightInstId, const int* a_remapId, const int* a_realInstId)
{
}

void RD_HydraGrid::InstanceLights(int32_t a_light_id, const float* a_matrix, pugi::xml_node* a_custAttrArray, int32_t a_instNum, int32_t a_lightGroupId)
{
}

HRRenderUpdateInfo RD_HydraGrid::HaveUpdateNow(int a_maxRaysPerPixel)
{
    HRRenderUpdateInfo result;
    result.finalUpdate   = false;
    result.progress      = 0.0f;
    result.haveUpdateFB  = false;
    result.haveUpdateMSG = false;

    size_t curr = grid_joiner->getProcessed();
    result.haveUpdateFB = curr != last_merged;
    result.progress = float(curr) / a_maxRaysPerPixel;
    result.finalUpdate = curr >= a_maxRaysPerPixel;
    last_merged = curr;

    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int   __float_as_int(float x) { return *((int*)&x); }
static inline float __int_as_float(int x) { return *((float*)&x); }

static inline int   as_int(float x) { return __float_as_int(x); }
static inline float as_float(int x) { return __int_as_float(x); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RD_HydraGrid::GetFrameBufferLineHDR(int32_t a_xBegin, int32_t a_xEnd, int32_t y, float* a_out, const wchar_t* a_layerName)
{

}

static inline float clamp(float u, float a, float b) { float r = fmax(a, u); return fmin(r, b); }
static inline int   clamp(int u, int a, int b) { int r = (a > u) ? a : u; return (r < b) ? r : b; }


void RD_HydraGrid::GetFrameBufferLineLDR(int32_t a_xBegin, int32_t a_xEnd, int32_t y, int32_t* a_out)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RD_HydraGrid::LockFrameBufferUpdate()
{
}

void RD_HydraGrid::UnlockFrameBufferUpdate()
{
}

void RD_HydraGrid::GetFrameBufferHDR(int32_t w, int32_t h, float* a_out, const wchar_t* a_layerName)
{
}

void RD_HydraGrid::GetFrameBufferLDR(int32_t w, int32_t h, int32_t* a_out)
{
}


void RD_HydraGrid::GetGBufferLine(int32_t a_lineNumber, HRGBufferPixel* a_lineData, int32_t a_startX, int32_t a_endX, const std::unordered_set<int32_t>& a_shadowCatchers)
{
}

void RD_HydraGrid::ExecuteCommand(const wchar_t* a_cmd, wchar_t* a_out)
{
   //TODO: Commands processing
   std::string inputA = ws2s(a_cmd);
   if(inputA == "pause" || inputA == "stop") grid_joiner->active = false;
   else if(inputA == "resume") grid_joiner->active = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IHRRenderDriver* CreateHydraGridConnection_RenderDriver()
{
    return new RD_HydraGrid;
}



