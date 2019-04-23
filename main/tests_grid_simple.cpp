#include "tests.h"
#include <math.h>
#include <iomanip>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <math.h>
#include "linmath.h"

#include <chrono>
#include <ctime>

#if defined(WIN32)
#include <FreeImage.h>
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "FreeImage.lib")

#else
#include <FreeImage.h>
#include <GLFW/glfw3.h>
#endif

#include "mesh_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <fstream>

#include "../hydra_api/HR_HDRImageTool.h"

#pragma warning(disable:4996)
using namespace TEST_UTILS;
using HydraRender::SaveImageToFile;

extern GLFWwindow* g_window;

bool test_grid_simple_01() {
    //initGLIfNeeded();

    hrErrorCallerPlace(L"gtest");

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hrSceneLibraryOpen(L"gtest", HR_WRITE_DISCARD);

    SimpleMesh cube = CreateCube(0.75f);
    SimpleMesh plane = CreatePlane(10.0f);
    SimpleMesh sphere = CreateSphere(1.0f, 32);
    SimpleMesh torus = CreateTorus(0.35f, 1.5f, 32, 32);
    SimpleMesh cubeOpen = CreateCubeOpen(4.0f);

    for (size_t i = 0; i < plane.vTexCoord.size(); i++)
        plane.vTexCoord[i] *= 2.0f;


    HRTextureNodeRef testTex2 = hrTexture2DCreateFromFileDL(L"data/textures/chess_white.bmp");

    HRMaterialRef mat0 = hrMaterialCreate(L"mysimplemat");
    HRMaterialRef mat1 = hrMaterialCreate(L"mysimplemat2");
    HRMaterialRef mat2 = hrMaterialCreate(L"mysimplemat3");
    HRMaterialRef mat3 = hrMaterialCreate(L"mysimplemat4");
    HRMaterialRef mat4 = hrMaterialCreate(L"myblue");
    HRMaterialRef mat5 = hrMaterialCreate(L"mymatplane");

    HRMaterialRef mat6 = hrMaterialCreate(L"red");
    HRMaterialRef mat7 = hrMaterialCreate(L"green");
    HRMaterialRef mat8 = hrMaterialCreate(L"white");

    hrMaterialOpen(mat0, HR_WRITE_DISCARD);
    {
        xml_node matNode = hrMaterialParamNode(mat0);

        xml_node diff = matNode.append_child(L"diffuse");

        diff.append_attribute(L"brdf_type").set_value(L"lambert");
        diff.append_child(L"color").text().set(L"0.5 0.75 0.5");

        HRTextureNodeRef testTex = hrTexture2DCreateFromFile(
                L"data/textures/texture1.bmp"); // hrTexture2DCreateFromFileDL
        hrTextureBind(testTex, diff);
    }
    hrMaterialClose(mat0);

    hrMaterialOpen(mat1, HR_WRITE_DISCARD);
    {
        xml_node matNode = hrMaterialParamNode(mat1);

        xml_node diff = matNode.append_child(L"diffuse");

        diff.append_attribute(L"brdf_type").set_value(L"lambert");
        diff.append_child(L"color").text().set(L"0.207843 0.188235 0");

        xml_node refl = matNode.append_child(L"reflectivity");

        refl.append_attribute(L"brdf_type").set_value(L"phong");
        refl.append_child(L"color").text().set(L"0.367059 0.345882 0");
        refl.append_child(L"glossiness").text().set(L"0.5");
        //refl.append_child(L"fresnel_IOR").text().set(L"1.5");
        //refl.append_child(L"fresnel").text().set(L"1");

    }
    hrMaterialClose(mat1);

    hrMaterialOpen(mat2, HR_WRITE_DISCARD);
    {
        xml_node matNode = hrMaterialParamNode(mat2);

        xml_node diff = matNode.append_child(L"diffuse");

        diff.append_attribute(L"brdf_type").set_value(L"lambert");
        diff.append_child(L"color").text().set(L"0.75 0.75 0.75");

        HRTextureNodeRef testTex = hrTexture2DCreateFromFile(L"data/textures/relief_wood.jpg");
        hrTextureBind(testTex, diff);
    }
    hrMaterialClose(mat2);

    hrMaterialOpen(mat3, HR_WRITE_DISCARD);
    {
        xml_node matNode = hrMaterialParamNode(mat3);

        xml_node diff = matNode.append_child(L"diffuse");

        diff.append_attribute(L"brdf_type").set_value(L"lambert");
        diff.append_child(L"color").text().set(L"0.75 0.75 0.75");

        HRTextureNodeRef testTex = hrTexture2DCreateFromFileDL(L"data/textures/163.jpg");
        hrTextureBind(testTex, diff);
    }
    hrMaterialClose(mat3);

    hrMaterialOpen(mat4, HR_WRITE_DISCARD);
    {
        xml_node matNode = hrMaterialParamNode(mat4);

        xml_node diff = matNode.append_child(L"diffuse");

        diff.append_attribute(L"brdf_type").set_value(L"lambert");
        diff.append_child(L"color").text().set(L"0.1 0.1 0.75");
    }
    hrMaterialClose(mat4);

    hrMaterialOpen(mat5, HR_WRITE_DISCARD);
    {
        xml_node matNode = hrMaterialParamNode(mat5);

        xml_node diff = matNode.append_child(L"diffuse");

        diff.append_attribute(L"brdf_type").set_value(L"lambert");
        diff.append_child(L"color").text().set(L"0.75 0.75 0.25");

        HRTextureNodeRef testTex = hrTexture2DCreateFromFileDL(L"data/textures/texture1.bmp");
        hrTextureBind(testTex, diff);
    }
    hrMaterialClose(mat5);


    hrMaterialOpen(mat6, HR_WRITE_DISCARD);
    {
        xml_node matNode = hrMaterialParamNode(mat6);
        xml_node diff = matNode.append_child(L"diffuse");

        diff.append_attribute(L"brdf_type").set_value(L"lambert");
        diff.append_child(L"color").text().set(L"0.5 0.0 0.0");
    }
    hrMaterialClose(mat6);

    hrMaterialOpen(mat7, HR_WRITE_DISCARD);
    {
        xml_node matNode = hrMaterialParamNode(mat7);
        xml_node diff = matNode.append_child(L"diffuse");

        diff.append_attribute(L"brdf_type").set_value(L"lambert");
        diff.append_child(L"color").text().set(L"0.0 0.5 0.0");
    }
    hrMaterialClose(mat7);

    hrMaterialOpen(mat8, HR_WRITE_DISCARD);
    {
        xml_node matNode = hrMaterialParamNode(mat8);
        xml_node diff = matNode.append_child(L"diffuse");

        diff.append_attribute(L"brdf_type").set_value(L"lambert");
        diff.append_child(L"color").text().set(L"0.5 0.5 0.5");
    }
    hrMaterialClose(mat8);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    HRMeshRef teapotRef = hrMeshCreateFromFileDL(
            L"data/meshes/teapot.vsgf"); // chunk_00009.vsgf // teapot.vsgf // chunk_00591.vsgf

    HRMeshRef cubeOpenRef = hrMeshCreate(L"my_box");
    HRMeshRef planeRef = hrMeshCreate(L"my_plane");
    HRMeshRef sphereRef = hrMeshCreate(L"my_sphere");
    HRMeshRef torusRef = hrMeshCreate(L"my_torus");

    hrMeshOpen(cubeOpenRef, HR_TRIANGLE_IND3, HR_WRITE_DISCARD);
    {
        hrMeshVertexAttribPointer4f(cubeOpenRef, L"pos", &cubeOpen.vPos[0]);
        hrMeshVertexAttribPointer4f(cubeOpenRef, L"norm", &cubeOpen.vNorm[0]);
        hrMeshVertexAttribPointer2f(cubeOpenRef, L"texcoord", &cubeOpen.vTexCoord[0]);

        int cubeMatIndices[10] = {mat8.id, mat8.id, mat8.id, mat8.id, mat8.id, mat8.id, mat7.id, mat7.id, mat6.id,
                                  mat6.id};

        //hrMeshMaterialId(cubeRef, 0);
        hrMeshPrimitiveAttribPointer1i(cubeOpenRef, L"mind", cubeMatIndices);
        hrMeshAppendTriangles3(cubeOpenRef, int(cubeOpen.triIndices.size()), &cubeOpen.triIndices[0]);
    }
    hrMeshClose(cubeOpenRef);


    hrMeshOpen(planeRef, HR_TRIANGLE_IND3, HR_WRITE_DISCARD);
    {
        hrMeshVertexAttribPointer4f(planeRef, L"pos", &plane.vPos[0]);
        hrMeshVertexAttribPointer4f(planeRef, L"norm", &plane.vNorm[0]);
        hrMeshVertexAttribPointer2f(planeRef, L"texcoord", &plane.vTexCoord[0]);

        hrMeshMaterialId(planeRef, mat5.id);
        hrMeshAppendTriangles3(planeRef, int32_t(plane.triIndices.size()), &plane.triIndices[0]);
    }
    hrMeshClose(planeRef);

    hrMeshOpen(sphereRef, HR_TRIANGLE_IND3, HR_WRITE_DISCARD);
    {
        hrMeshVertexAttribPointer4f(sphereRef, L"pos", &sphere.vPos[0]);
        hrMeshVertexAttribPointer4f(sphereRef, L"norm", &sphere.vNorm[0]);
        hrMeshVertexAttribPointer2f(sphereRef, L"texcoord", &sphere.vTexCoord[0]);

        for (size_t i = 0; i < sphere.matIndices.size() / 2; i++)
            sphere.matIndices[i] = mat0.id;

        for (size_t i = sphere.matIndices.size() / 2; i < sphere.matIndices.size(); i++)
            sphere.matIndices[i] = mat2.id;

        hrMeshPrimitiveAttribPointer1i(sphereRef, L"mind", &sphere.matIndices[0]);
        hrMeshAppendTriangles3(sphereRef, int32_t(sphere.triIndices.size()), &sphere.triIndices[0]);
    }
    hrMeshClose(sphereRef);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    HRLightRef rectLight = hrLightCreate(L"my_area_light");

    hrLightOpen(rectLight, HR_WRITE_DISCARD);
    {
        pugi::xml_node lightNode = hrLightParamNode(rectLight);

        lightNode.attribute(L"type").set_value(L"area");
        lightNode.attribute(L"shape").set_value(L"rect");
        lightNode.attribute(L"distribution").set_value(L"diffuse");

        pugi::xml_node sizeNode = lightNode.append_child(L"size");

        sizeNode.append_attribute(L"half_length") = 1.0f;
        sizeNode.append_attribute(L"half_width") = 1.0f;

        pugi::xml_node intensityNode = lightNode.append_child(L"intensity");

        intensityNode.append_child(L"color").text().set(L"1 1 1");
        intensityNode.append_child(L"multiplier").text().set(L"10.0");
    }
    hrLightClose(rectLight);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // camera
    //
    HRCameraRef camRef = hrCameraCreate(L"my camera");

    hrCameraOpen(camRef, HR_WRITE_DISCARD);
    {
        xml_node camNode = hrCameraParamNode(camRef);

        camNode.append_child(L"fov").text().set(L"45");
        camNode.append_child(L"nearClipPlane").text().set(L"0.01");
        camNode.append_child(L"farClipPlane").text().set(L"100.0");

        camNode.append_child(L"up").text().set(L"0 1 0");
        camNode.append_child(L"position").text().set(L"0 0 15");
        camNode.append_child(L"look_at").text().set(L"0 0 0");
    }
    hrCameraClose(camRef);

    // set up render settings
    //
    HRRenderRef renderRef = hrRenderCreate(L"HydraGRID");

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /*
    auto pList = hrRenderGetDeviceList(renderRef);

    while (pList != nullptr) {
        std::wcout << L"device id = " << pList->id << L", name = " << pList->name << L", driver = " << pList->driver
                   << std::endl;
        pList = pList->next;
    }

    //hrRenderEnableDevice(renderRef, 0, true);
    hrRenderEnableDevice(renderRef, CURR_RENDER_DEVICE, true);
     */

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hrRenderOpen(renderRef, HR_WRITE_DISCARD);
    {
        pugi::xml_node grid_node = hrRenderParamNode(renderRef);

        std::chrono::time_point<std::chrono::system_clock> end_time = std::chrono::system_clock::now() + std::chrono::hours(1);
        grid_node.append_child(L"actual_before").text().set(
                std::chrono::time_point_cast<std::chrono::minutes>(end_time).time_since_epoch().count()
                );
    }
    hrRenderClose(renderRef);


    HRRenderRef renderHydra = hrRenderCreate(L"HydraModern");
    hrRenderOpen(renderHydra, HR_WRITE_DISCARD);
    {
        pugi::xml_node node = hrRenderParamNode(renderHydra);

        node.append_child(L"width").text() = 512;
        node.append_child(L"height").text() = 512;

        node.append_child(L"method_primary").text() = L"pathtracing";
        node.append_child(L"method_secondary").text() = L"pathtracing";
        node.append_child(L"method_tertiary").text() = L"pathtracing";
        node.append_child(L"method_caustic").text() = L"pathtracing";
        node.append_child(L"shadows").text() = L"1";

        node.append_child(L"trace_depth").text() = 5;
        node.append_child(L"diff_trace_depth").text() = 3;
        node.append_child(L"maxRaysPerPixel").text() = 1024;

        // #{CUSTOM EXE PATH}:
        //node.append_child(L"render_exe_dir").text() = L"E:/PROG/hydratest"; // copy all files from "C:/[Hydra]/bin2" to "E:/PROG/hydratest". Don't create "E:/PROG/hydratest/bin2".
    }
    hrRenderClose(renderHydra);



    // #{CUSTOM EXE PATH}:
    //hrRenderLogDir(renderRef, L"E:/PROG/hydratest/logs", false);          // we also fix logs folder path. it if does not end with "/", we add "/" inside hrRenderLogDir

    // create scene
    //
    HRSceneInstRef scnRef = hrSceneCreate(L"my scene");

    static GLfloat rtri = 25.0f; // Angle For The Triangle ( NEW )
    static GLfloat rquad = 40.0f;
    static float g_FPS = 60.0f;
    static int frameCounter = 0;

    const float DEG_TO_RAD = float(3.14159265358979323846f) / 180.0f;

    float matrixT[4][4];
    float mRot1[4][4], mTranslate[4][4], mRes[4][4];

    hrSceneOpen(scnRef, HR_WRITE_DISCARD);

    int mmIndex = 0;
    mat4x4_identity(mRot1);
    mat4x4_identity(mTranslate);
    mat4x4_identity(mRes);

    mat4x4_translate(mTranslate, 0.0f, -0.70f * 3.65f, -5.0f + 5.0f);
    mat4x4_scale(mRot1, mRot1, 3.65f);
    mat4x4_mul(mRes, mTranslate, mRot1);
    mat4x4_transpose(matrixT, mRes); // this fucking math library swap rows and columns
    matrixT[3][3] = 1.0f;

    hrMeshInstance(scnRef, teapotRef, &matrixT[0][0]);

    mat4x4_identity(mRot1);
    mat4x4_rotate_Y(mRot1, mRot1, 180.0f * DEG_TO_RAD);
    //mat4x4_rotate_Y(mRot1, mRot1, rquad*DEG_TO_RAD);
    mat4x4_transpose(matrixT, mRot1);
    hrMeshInstance(scnRef, cubeOpenRef, &matrixT[0][0]);

    /////////////////////////////////////////////////////////////////////// instance light (!!!)

    mat4x4_identity(mTranslate);
    mat4x4_translate(mTranslate, 0, 3.85f, 0);
    mat4x4_transpose(matrixT, mTranslate);
    hrLightInstance(scnRef, rectLight, &matrixT[0][0]);

    hrSceneClose(scnRef);

    hrFlush(scnRef, renderRef);

    return true;
}


bool test_grid_simple_02() {
    initGLIfNeeded();
    hrErrorCallerPlace(L"gtest");

    hrSceneLibraryOpen(L"gtest", HR_OPEN_EXISTING);
    HRRenderRef renderRef;
    HRSceneInstRef scnRef;
    {
        renderRef.id = 1;  // get second render (Hydra Modern)
        scnRef.id    = 0;  // and first scene
    }

    hrRenderEnableDevice(renderRef, CURR_RENDER_DEVICE, true);

    hrRenderOpen(renderRef, HR_OPEN_EXISTING);
    hrFlush(scnRef, renderRef);
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        HRRenderUpdateInfo info = hrRenderHaveUpdate(renderRef);
        if (info.haveUpdateFB)
        {
            auto pres = std::cout.precision(2);
            std::cout << "rendering progress = " << info.progress << "% \r"; std::cout.flush();
            std::cout.precision(pres);
        }
        if (info.finalUpdate)
            break;
    }

    hrRenderSaveFrameBufferLDR(renderRef, L"test.png");
    hrRenderSaveFrameBufferHDR(renderRef, L"test.image4f");

    return true;
}