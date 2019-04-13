#pragma once

#include "HydraAPI.h"
#include "HydraInternal.h"
#include "HydraRenderDriverAPI.h"
#include "HR_HDRImage.h"

using HydraRender::HDRImage4f;
using HydraRender::LDRImage1i;


#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pugi::xml_node get_global_trash_node();

static const size_t BAD_ID = -1;

template<typename IMPL_T>
struct HRObject
{
  HRObject() : name(L""), id(0), opened(false), openMode(HR_WRITE_DISCARD), inMemory(true), changeId(0), wasChanged(false) {}

 /* HRObject(const HRObject& other) : name(other.name), id(other.id), opened(other.opened), openMode(other.openMode),
                                    inMemory(other.inMemory), changeId(other.changeId), wasChanged(other.wasChanged)
  {
    m_xmlNode = other.m_xmlNode;
    m_xmlNodeNext = other.m_xmlNodeNext;
  }*/

  //////////////////////////////////////////////////

  std::wstring name;     ///< object name that user usually have to specify
  int32_t       id;       ///< object id, unique for each object type; no-overwrite policy is used; deleted objects never became valid again;


  HR_OPEN_MODE openMode;   ///< HR_WRITE_DISCARD or HR_OPEN_EXINTING or HR_OPEN_READ_ONLY
  bool         opened;     ///< if opened now
  bool         inMemory;   ///< if loaded in CPU memory, or swaped to disk
  bool         wasChanged; ///< was changed but not passed to render driver yet;
  int          changeId;   ///< the identifier of change;

  /////////////////////////////////////////////////////////////////////////////////// xml nodes

  virtual void update_next(pugi::xml_node a_newNode)
  {
    m_xmlNodeNext = a_newNode;
  }

  virtual void update_this(pugi::xml_node a_newNode)
  {
    m_xmlNode = a_newNode;
  }

  virtual pugi::xml_node xml_node_immediate()
  {
    if (m_xmlNodeNext != nullptr)
      return m_xmlNodeNext;
    else
      return m_xmlNode;
  }

  virtual pugi::xml_node xml_node_next(HR_OPEN_MODE a_openMode)
  { 
    if (m_xmlNodeNext == nullptr)
    {
      if (a_openMode == HR_OPEN_EXISTING)
        m_xmlNodeNext = copy_node(m_xmlNode, false);
      else if (a_openMode == HR_WRITE_DISCARD)
        m_xmlNodeNext = copy_node(m_xmlNode, true);
      else if (a_openMode == HR_OPEN_READ_ONLY)
        m_xmlNodeNext = copy_node_trash(m_xmlNode);
    }
    
    return m_xmlNodeNext;
  }

  virtual void commit()
  {
    if (m_xmlNodeNext == nullptr)
      return;

    m_xmlNode     = copy_node_back(m_xmlNodeNext);
    m_xmlNodeNext = pugi::xml_node();
  }

  /////////////////////////////////////////////////////////////////////////////////// xml nodes

  virtual pugi::xml_node copy_node(pugi::xml_node a_node, bool a_lite) = 0;
  virtual pugi::xml_node copy_node_back(pugi::xml_node a_node)         = 0;

  virtual pugi::xml_node copy_node_trash(pugi::xml_node a_node) ///< copy node to temporaray trash node that will be cleared further
  {
    pugi::xml_node nodeToCopy = get_global_trash_node();
    return nodeToCopy.append_copy(a_node);
  }

protected:

  pugi::xml_node m_xmlNode;
  pugi::xml_node m_xmlNodeNext;

};

void clear_node(pugi::xml_node a_xmlNode);
void clear_node_childs(pugi::xml_node a_xmlNode);

/**
\brief Mesh object type.

*/

struct HRMesh : public HRObject<IHRMesh>
{
  HRMesh() : pImpl(nullptr), m_allMeshMatId(-1), m_empty(true)  {}

  std::shared_ptr<IHRMesh> pImpl;

  pugi::xml_node copy_node(pugi::xml_node a_node, bool a_lite) override;
  pugi::xml_node copy_node_back(pugi::xml_node a_node) override;

//protected:

  struct InputTriMesh
  {
    InputTriMesh() : m_saveCompressed(false) { }

    void clear()
    {
      verticesPos.clear();
      verticesNorm.clear();
      verticesTexCoord.clear();
      verticesTangent.clear();
      triIndices.clear();
      matIndices.clear();
      customArrays.clear();
    }

    void freeMem()
    {
      verticesPos      = std::vector<float>();
      verticesNorm     = std::vector<float>();
      verticesTexCoord = std::vector<float>();
      verticesTangent  = std::vector<float>();
      triIndices       = std::vector<uint32_t>();
      matIndices       = std::vector<uint32_t>();
      customArrays.clear();                       /// its a vector of vectors
    }

    void reserve(size_t vNum, size_t indNum)
    {
      verticesPos.reserve     (vNum * 4 + 10);
      verticesNorm.reserve    (vNum * 4 + 10);
      verticesTangent.reserve (vNum * 4 + 10);
      verticesTexCoord.reserve(vNum * 2 + 10);
      
      if(triIndices.capacity() < indNum + 10)
        triIndices.reserve(indNum + 10);
      
      if(matIndices.capacity() < indNum / 3 + 10)
        matIndices.reserve(indNum / 3 + 10);
      
      for (auto& arr : customArrays)
      {
        arr.idata.reserve(1*vNum);
        arr.fdata.reserve(4*vNum);
      }
    }
  
    void resize(size_t vNum, size_t indNum)
    {
      verticesPos.resize     (vNum * 4);
      verticesNorm.resize    (vNum * 4);
      verticesTangent.resize (vNum * 4);
      verticesTexCoord.resize(vNum * 2);
      
      triIndices.resize      (indNum);
      matIndices.resize      (indNum / 3);
    
      for (auto& arr : customArrays)
      {
        arr.idata.resize(1*vNum);
        arr.fdata.resize(4*vNum);
      }
    }

    std::vector<float>    verticesPos;       ///< float4
    std::vector<float>    verticesNorm;      ///< float4
    std::vector<float>    verticesTexCoord;  ///< float2
    std::vector<float>    verticesTangent;   ///< float4
    std::vector<uint32_t> triIndices;        ///< size of 3*triNum
    std::vector<uint32_t> matIndices;        ///< size of 1*triNum

    struct CustArray
    {
      std::vector<int>   idata;
      std::vector<float> fdata;
      std::wstring       name;
      int depth;
      int apply;
    };

    std::vector<CustArray> customArrays;
    bool m_saveCompressed;
  };

  struct InputTriMeshPointers
  {
    InputTriMeshPointers() : pos(nullptr), normals(nullptr), texCoords(nullptr), tangents(nullptr), mindices(nullptr), 
                             posStride(4), normStride(4), tangStride(4) {}

    void clear()
    {
      pos       = nullptr;
      normals   = nullptr;
      texCoords = nullptr;
      tangents  = nullptr;
      mindices  = nullptr;

      customVertPointers.clear();
      customPrimPointers.clear();
    }

    const float* pos;
    const float* normals;
    const float* texCoords;
    const float* tangents;
    int posStride;
    int normStride;
    int tangStride;

    enum POINTER_TYPES {CUST_POINTER_FLOAT = 1, CUST_POINTER_INT = 2};

    struct CustPointer
    {
      union 
      {
        const int*   idata;
        const float* fdata;
      };
      int           stride;
      POINTER_TYPES ptype;
      std::wstring  name;
    };

    std::vector<CustPointer> customVertPointers;
    std::vector<CustPointer> customPrimPointers;

    const int* mindices;
  };

  InputTriMesh         m_input;
  InputTriMeshPointers m_inputPointers;
  int                  m_allMeshMatId;
  bool                 m_empty;
};

struct HRLight : public HRObject<IHRLight>
{
  HRLight() = default;

  pugi::xml_node copy_node(pugi::xml_node a_node, bool a_lite) override;
  pugi::xml_node copy_node_back(pugi::xml_node a_node) override;
};

struct HRMaterial : public HRObject<IHRMat>
{
  HRMaterial() = default;

  std::shared_ptr<IHRMat> pImpl;

  pugi::xml_node copy_node(pugi::xml_node a_node, bool a_lite) override;
  pugi::xml_node copy_node_back(pugi::xml_node a_node) override;
};

struct HRCamera : public HRObject<IHRCam>
{
  HRCamera() = default;

  std::shared_ptr<IHRCam> pImpl;

  pugi::xml_node copy_node(pugi::xml_node a_node, bool a_lite) override;
  pugi::xml_node copy_node_back(pugi::xml_node a_node) override;
};

/**
\brief Nodes are render textures, pretty much like any other nodes used in 3D modeling software.

A common 3D modeling software, like 3ds Max, Blender and especially Houdini uses concept called Nodes.

Nodes are fixed function or programmable units, like a functions - take some from input and put some other to output.
Nodes usually can be bind to some material slots (or light slots). Users will build a trees from nodes to do some complex stuff.

The simplest node is a Bitmap, created with hrTexture2D. Bitmaps are simple 2D images. Just a 2D array of values.
Their input is a texture coordinates, output - texture color. Texture can be bound to material slot.

The more complex example is 'Falloff' node - a blend for 2 textures based of angle between view vector and surface normal.
Falloff is a fixed function node with predefined parameters.

Any parameters (for fixed function or programmable nodes) can be set via sequence of HRTextureNodeOpen(node), hrParameter(node,...), HRTextureNodeClose(node);

*/
struct HRTextureNode : public HRObject<IHRTextureNode>
{
  HRTextureNode() : pImpl(nullptr), m_loadedFromFile(false), customData(nullptr), customDataSize(0),
                    ldrCallback(nullptr), hdrCallback(nullptr), displaceCallback(nullptr) {}

  ~HRTextureNode() {free(customData);}

  HRTextureNode(const HRTextureNode& other) : HRObject(other), m_loadedFromFile(other.m_loadedFromFile),
                                              ldrCallback(other.ldrCallback), hdrCallback(other.hdrCallback),
                                              displaceCallback(other.displaceCallback), customDataSize(other.customDataSize),
                                              pImpl(other.pImpl)
  {
    if(customDataSize == 0)
      customData = nullptr;
    else
    {
      customData = malloc(customDataSize);
      if(other.customData != nullptr)
        memcpy(customData, other.customData, customDataSize);
    }
  }

  std::shared_ptr<IHRTextureNode> pImpl;

  pugi::xml_node copy_node(pugi::xml_node a_node, bool a_lite) override;
  pugi::xml_node copy_node_back(pugi::xml_node a_node) override;

  bool m_loadedFromFile;

  void *customData;
  uint32_t customDataSize;

  HR_TEXTURE2D_PROC_LDR_CALLBACK ldrCallback;
  HR_TEXTURE2D_PROC_HDR_CALLBACK hdrCallback;
  HRExtensions::HR_TEXTURE_DISPLACEMENT_CALLBACK displaceCallback;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct HRSceneData : public HRObject<IHRSceneData>
{

  static const int TEXTURES_RESERVE = 1000;
  static const int MATERIAL_RESERVE = 1000;
  static const int LIGHTS_RESERVE   = 1000;
  static const int CAMERAS_RESERVE  = 100;
  static const int MESHES_RESERVE   = 10000;

  HRSceneData() : pImpl(nullptr), m_commitId(0) {}

  std::shared_ptr<IHRSceneData> pImpl;

  pugi::xml_node copy_node(pugi::xml_node a_node, bool a_lite) override;
  pugi::xml_node copy_node_back(pugi::xml_node a_node) override;

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
  std::vector<HRMesh>        meshes;
  std::vector<HRLight>       lights;
  std::vector<HRMaterial>    materials;
  std::vector<HRCamera>      cameras;
  std::vector<HRTextureNode> textures;
  VirtualBuffer              m_vbCache;

  pugi::xml_document         m_xmlDoc;
  pugi::xml_node             m_texturesLib;
  pugi::xml_node             m_materialsLib;
  pugi::xml_node             m_lightsLib;
  pugi::xml_node             m_cameraLib;
  pugi::xml_node             m_geometryLib;
  pugi::xml_node             m_sceneNode;
  pugi::xml_node             m_settingsNode;
  pugi::xml_node             m_trashNode;

  pugi::xml_document         m_xmlDocChanges;
  pugi::xml_node             m_texturesLibChanges;
  pugi::xml_node             m_materialsLibChanges;
  pugi::xml_node             m_lightsLibChanges;
  pugi::xml_node             m_cameraLibChanges;
  pugi::xml_node             m_geometryLibChanges;
  pugi::xml_node             m_sceneNodeChanges;
  pugi::xml_node             m_settingsNodeChanges;

  std::unordered_map<std::wstring, int32_t>      m_textureCache;
  std::unordered_map<std::wstring, std::wstring> m_iesCache;

  // dependency data
  //
  std::unordered_multimap<int32_t, int32_t> m_materialToMeshDependency;
  std::unordered_set<int32_t>               m_shadowCatchers;

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
  
  void init(bool a_emptyvb, HRSystemMutex* a_pVBSysMutexLock);
  void init_existing(bool a_attachMode, HRSystemMutex* a_pVBSysMutexLock);
  void clear();
  void clear_changes();

  int32_t m_commitId;
  std::wstring m_path;
  std::wstring m_pathState;
  std::wstring m_fileState;
protected:
  void init_virtual_buffer(bool a_attachMode, HRSystemMutex* a_pVBSysMutexLock);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct HRSceneInst : public HRObject<IHRSceneInst>
{
  HRSceneInst() : pImpl(nullptr), drawBegin(0), drawBeginLight(0), driverDirtyFlag(true), lightGroupCounter(0), instancedScenesCounter(0) {}

  void update(pugi::xml_node a_newNode)
  {
    m_xmlNodeNext = a_newNode;
  }

  void commit() override
  {
    if (m_xmlNodeNext == nullptr)
      return;

    const wchar_t* discard = m_xmlNodeNext.attribute(L"discard").value();
    if (std::wstring(discard) == L"1")
      m_xmlNode = copy_node_back(m_xmlNodeNext);
    else
      m_xmlNode = append_instances_back(m_xmlNodeNext);

    m_xmlNodeNext = pugi::xml_node();
  }

  pugi::xml_node xml_node_next(HR_OPEN_MODE a_openMode) override
  {
     if (m_xmlNodeNext == nullptr)
       m_xmlNodeNext = copy_node(m_xmlNode, true); // don't copy all scene instances ! :) 
    return m_xmlNodeNext;
  }

  void clear()
  {
    meshUsedByDrv.clear();
    matUsedByDrv.clear();
    lightUsedByDrv.clear();
    texturesUsedByDrv.clear();

    drawList.clear();
    drawListLights.clear();
    pImpl = nullptr;

    drawBeginLight = 0;
    drawBegin      = 0;
    driverDirtyFlag = true;
    lightGroupCounter = 0;
    instancedScenesCounter = 0;
    m_bbox = BBox();
  }

  std::shared_ptr<IHRSceneInst> pImpl;

  pugi::xml_node copy_node(pugi::xml_node a_node, bool a_lite) override;
  pugi::xml_node copy_node_back(pugi::xml_node a_node) override;
  pugi::xml_node append_instances_back(pugi::xml_node a_node);

  struct Instance
  {
    Instance() : lightInstId(-1), lightId(-1), meshId(-1), remapListId(-1), lightGroupInstId(-1), scene_id(-1),
                 scene_sid(0)
    {
      m[0] = 1; m[1] = 0; m[2] = 0; m[3] = 0;
      m[4] = 0; m[5] = 1; m[6] = 0; m[7] = 0;
      m[8] = 0; m[9] = 0; m[10]= 1; m[11]= 0;
      m[12]= 0; m[13]= 0; m[14]= 0; m[15]= 1;
    }

    float    m[16];
    int32_t  lightInstId;
    int32_t  lightId;
    int32_t  meshId;
    int32_t  remapListId;
    int32_t  lightGroupInstId;
    int32_t  scene_id;
    int32_t  scene_sid;
    pugi::xml_node node;
  };

  std::vector<Instance> drawList;
  size_t                drawBegin;

  std::vector<Instance>     drawListLights;
  std::vector<std::wstring> drawLightsCustom;
  size_t                    drawBeginLight;

  std::unordered_set<int32_t> meshUsedByDrv;
  std::unordered_set<int32_t> matUsedByDrv;
  std::unordered_set<int32_t> lightUsedByDrv;
  std::unordered_set<int32_t> texturesUsedByDrv;

  std::vector< std::vector<int32_t> >   m_remapList;
  std::unordered_map<uint64_t, int32_t> m_remapCache;

  BBox m_bbox;

  bool driverDirtyFlag;  // if true, driver need to Update this scene.
  int32_t lightGroupCounter;
  int32_t instancedScenesCounter;
};

struct HRRender : public HRObject<IHRRender>
{
  HRRender() : m_pDriver(nullptr), maxRaysPerPixel(0) {}

  std::shared_ptr<IHRRenderDriver> m_pDriver;

  pugi::xml_node copy_node(pugi::xml_node a_node, bool a_lite) override;
  pugi::xml_node copy_node_back(pugi::xml_node a_node) override;

  int maxRaysPerPixel;

  void clear()
  {
    if (m_pDriver != nullptr)
      m_pDriver->ClearAll();
    m_pDriver = nullptr;
    maxRaysPerPixel = 0;
  }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TEMP_BUFFER_MAX_SIZE_DONT_FREE 104857600

struct HRObjectManager
{
  HRObjectManager() : m_pFactory(nullptr), m_pDriver(nullptr), m_pImgTool(nullptr), m_currSceneId(0), m_currRenderId(0), m_currCamId(0), m_pVBSysMutex(nullptr),
                      m_copyTexFilesToLocalStorage(false), m_useLocalPath(true), m_attachMode(false), m_sortTriIndices(false), m_computeBBoxes(false) {}
 
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

  HRSceneData              scnData; /// In our first impl sceneData can be only one and it is directly related to HRObjectManager 
  std::vector<HRSceneInst> scnInst;
  std::vector<HRRender>    renderSettings;
  int32_t m_currSceneId;
  int32_t m_currRenderId;
  int32_t m_currCamId;
  
  HRSystemMutex*           m_pVBSysMutex;
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

  void init(const wchar_t* a_className);
  void destroy();

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

  HRMesh*        PtrById(HRMeshRef a_ref);
  HRLight*       PtrById(HRLightRef a_ref);
  HRMaterial*    PtrById(HRMaterialRef a_ref);
  HRCamera*      PtrById(HRCameraRef a_ref);
  HRTextureNode* PtrById(HRTextureNodeRef a_ref);
  HRSceneInst*   PtrById(HRSceneInstRef a_ref);
  HRRender*      PtrById(HRRenderRef a_ref);

  const std::wstring GetLoc(pugi::xml_node a_node) const;
  void SetLoc(pugi::xml_node a_node, const std::wstring& a_loc);

  std::vector<int> m_tempBuffer;
  std::wstring     m_tempPathToChangeFile;
  std::vector<int> EmptyBuffer() { return std::vector<int>(); }

  void CommitChanges(pugi::xml_document& a_from, pugi::xml_document& a_to);
  IHydraFactory* m_pFactory; // actual Factory

  std::shared_ptr<IHRRenderDriver>     m_pDriver;
  std::unordered_set<IHRRenderDriver*> driverAllocated;
  std::shared_ptr<IHRImageTool>        m_pImgTool;

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

  inline pugi::xml_node textures_lib_append_child()  { return scnData.m_texturesLibChanges.append_child(L"texture"); }
  inline pugi::xml_node materials_lib_append_child() { return scnData.m_materialsLibChanges.append_child(L"material"); }
  inline pugi::xml_node lights_lib_append_child()    { return scnData.m_lightsLibChanges.append_child(L"light"); }
  inline pugi::xml_node camera_lib_append_child()    { return scnData.m_cameraLibChanges.append_child(L"camera"); }

  inline pugi::xml_node geom_lib_append_child()      { return scnData.m_geometryLibChanges.append_child(L"mesh"); }
  inline pugi::xml_node scenes_node_append_child()   { return scnData.m_sceneNodeChanges.append_child(L"scene"); }
  inline pugi::xml_node settings_lib_append_child()  { return scnData.m_settingsNodeChanges.append_child(L"render_settings"); }

  inline pugi::xml_node trash_node()                 { return scnData.m_trashNode; }

  void BadMaterialId(int32_t a_id) { if (m_badMaterialId.size() < 10) m_badMaterialId.push_back(a_id); }
  std::vector<int32_t> m_badMaterialId;

  bool m_copyTexFilesToLocalStorage;
  bool m_useLocalPath;
  bool m_sortTriIndices;
  bool m_attachMode;
  bool m_computeBBoxes;
};

void HrError(std::wstring a_str);
void _HrPrint(HR_SEVERITY_LEVEL a_level, const wchar_t* a_str);

template <typename HEAD>
void _HrPrint(std::wstringstream& out, HEAD head)
{
  out << head << std::endl;
}

template <typename HEAD, typename... TAIL>
void _HrPrint(std::wstringstream& out, HEAD head, TAIL... tail)
{
  out << head;
  _HrPrint(out, tail...);
}

template <typename ... Args>
static void HrPrint(HR_SEVERITY_LEVEL a_level, Args ... a_args)
{
  std::wstringstream out;
  _HrPrint(out, a_args...);

  std::wstring strOut = out.str();
  _HrPrint(a_level, strOut.c_str());
}

HR_ERROR_CALLBACK getErrorCallback();
HR_INFO_CALLBACK  getPrintCallback();
std::wstring&     getErrWstrObject();
std::wstring&     getErrCallerWstrObject();

template<typename X>
static void HrError(std::wstring a_str, X value)
{
  HrPrint(HR_SEVERITY_ERROR, a_str, value);
}

std::wstring ToWString(uint64_t i);
std::wstring ToWString(int64_t i);
std::wstring ToWString(int i);
std::wstring ToWString(float i);
std::wstring ToWString(unsigned int i);

void ComputeVertexTangents(HRMesh::InputTriMesh& mesh, int indexNum);
void ComputeVertexNormals(HRMesh::InputTriMesh& mesh, const int indexNum, bool useFaceNormals);