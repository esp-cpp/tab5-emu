#include "globals.h"

#include "General/stdStrTable.h"
#include "Primitives/rdModel3.h"
#include "Primitives/rdVector.h"
#include "Primitives/rdMatrix.h"
#include "Raster/rdFace.h"
#include "Raster/rdCache.h"
#include "Engine/rdKeyframe.h"
#include "Engine/sithRender.h"
#include "World/sithWorld.h"
#include "World/sithSector.h"
#include "World/sithThing.h"

#include "jk.h"

#ifdef TARGET_TWL
#define MAYBE_UNUSED __attribute__((unused))
#else
#define MAYBE_UNUSED
#endif

#define NO_REINIT // HACK

#ifdef NO_JK_MMAP
// Vars
tHashTable* sithAIClass_g_pHashtable MAYBE_UNUSED = NULL;
char std_g_genBuffer[1024] MAYBE_UNUSED = {0};
struct HostServices* std_g_pHS MAYBE_UNUSED = NULL;
rdGeoset* rdModel3_pCurGeoset MAYBE_UNUSED = NULL;
rdVector3 localCamera MAYBE_UNUSED = {0};
rdVector3 aFaceVerts[32] NO_REINIT MAYBE_UNUSED = {0};
rdMeshinfo vertexDst MAYBE_UNUSED = {0};
int curGeometryMode MAYBE_UNUSED = 0;
rdLight* apGeoLights[64] MAYBE_UNUSED = {0};
rdVector3 rdModel3_aLocalLightPos[64] NO_REINIT MAYBE_UNUSED = {0};
rdVector3 rdModel3_aLocalLightDir[64] NO_REINIT MAYBE_UNUSED = {0};
int meshFrustumCull MAYBE_UNUSED = 0;
int curTextureMode MAYBE_UNUSED = 0;
rdVector3 aView[512] NO_REINIT MAYBE_UNUSED = {0};
rdMesh* pCurMesh MAYBE_UNUSED = NULL;
int thingFrustumCull MAYBE_UNUSED = 0;
rdMeshinfo vertexSrc MAYBE_UNUSED = {0};
rdModel3* pCurModel3 MAYBE_UNUSED = NULL;
int rdModel3_textureMode MAYBE_UNUSED = 0;
int curLightingMode MAYBE_UNUSED = 0;
rdLight* apMeshLights[64] MAYBE_UNUSED = {0};
rdThing* pCurThing MAYBE_UNUSED = NULL;
int rdModel3_lightingMode MAYBE_UNUSED = 0;
int rdModel3_geometryMode MAYBE_UNUSED = 0;
int rdModel3_numDrawnModels MAYBE_UNUSED = 0;
model3Loader_t pModel3Loader MAYBE_UNUSED = {0};
model3Unloader_t pModel3Unloader MAYBE_UNUSED = {0};
uint32_t rdModel3_numGeoLights MAYBE_UNUSED = {0};
int rdModel3_numMeshLights MAYBE_UNUSED = 0;
flex_t rdModel3_fRadius MAYBE_UNUSED = 0.0f;
tHashTable* sithPuppet_pClassHashtable MAYBE_UNUSED = NULL;
tHashTable* sithPuppet_pKeyHashtable MAYBE_UNUSED = NULL;
tHashTable* sithPuppet_pHashtblSubmodes MAYBE_UNUSED = NULL;
keyframeLoader_t pKeyframeLoader MAYBE_UNUSED = {0};
keyframeUnloader_t pKeyframeUnloader MAYBE_UNUSED = {0};
int32_t sithTime_g_frameTime MAYBE_UNUSED = {0};
flex_t sithTime_g_frameTimeFlex MAYBE_UNUSED = 0.0f;
flex_t  sithTime_g_fps MAYBE_UNUSED = {0};
uint32_t sithTime_g_msecGameTime MAYBE_UNUSED = {0};
flex32_t sithTime_g_secGameTime MAYBE_UNUSED = 0.0f;
uint32_t sithTime_g_clockTime MAYBE_UNUSED = {0};
int32_t sithTime_msecPauseStartTime MAYBE_UNUSED = {0};
int sithTime_g_bPaused MAYBE_UNUSED = 0;
int sithRender_texMode MAYBE_UNUSED = 0;
int sithRender_renderflags MAYBE_UNUSED = 0;
int sithRender_geoMode MAYBE_UNUSED = 0;
int sithRender_lightMode MAYBE_UNUSED = 0;
int sithRender_lightingIRMode MAYBE_UNUSED = 0;
flex_t sithRender_f_83198C MAYBE_UNUSED = 0.0f;
flex_t sithRender_f_831990 MAYBE_UNUSED = 0.0f;
int sithRender_bResetCameraAspect MAYBE_UNUSED = 0;
int32_t sithRender_g_numVisibleSectors MAYBE_UNUSED = {0};
int32_t sithRender_numSecorFrustrums MAYBE_UNUSED = {0};
int32_t sithRender_numThingLights MAYBE_UNUSED = {0};
int32_t sithRender_numThingSectors MAYBE_UNUSED = {0};
uint32_t sithRender_numSpritesToDraw MAYBE_UNUSED = {0};
int sithRender_numRenderedSectors MAYBE_UNUSED = 0;
int32_t sithRender_numAlphaAdjoins MAYBE_UNUSED = {0};
int sithRender_geoThingsDrawn MAYBE_UNUSED = 0;
int sithRender_nongeoThingsDrawn MAYBE_UNUSED = 0;
flex_t  sithRender_f_82F4B0 MAYBE_UNUSED =  0.0;
rdMeshinfo sithRender_faceView MAYBE_UNUSED = {0};
rdMeshinfo meshinfo_out MAYBE_UNUSED = {0};
sithRender_weapRendFunc_t sithRender_pExtraThingRenderFunc MAYBE_UNUSED = {0};
rdLight sithRender_aThingLights[32] MAYBE_UNUSED = {0};
SithSector* sithRender_aVisibleSectors[SITH_MAX_VISIBLE_SECTORS] MAYBE_UNUSED = {0};
rdClipFrustum sithRender_aSectorFrustrums[SITH_MAX_VISIBLE_SECTORS] MAYBE_UNUSED = {0};
SithSector* sithRender_aThingSectors[SITH_MAX_VISIBLE_SECTORS_2+2] MAYBE_UNUSED = {0};
rdVector3 sithRender_aClipVertices[32] NO_REINIT MAYBE_UNUSED = {0};
rdVector3 sithRender_aTransformedClipVertices[32] NO_REINIT MAYBE_UNUSED = {0};
SithSurface* sithRender_aAlphaAdjoins[SITH_MAX_VISIBLE_ALPHA_SURFACES] MAYBE_UNUSED = {0};
int sithRender_lastRenderTick MAYBE_UNUSED = 0;
SithSurface* aSithSurfaces[SITH_MAX_VISIBLE_ALPHA_SURFACES] MAYBE_UNUSED = {0};
SithCamera sithCamera_g_aCameras[SITHCAMERA_NUMCAMERAS] MAYBE_UNUSED = {0};
int sithCamera_g_bCurCameraSet MAYBE_UNUSED = 0;
int sithCamera_g_stateFlags MAYBE_UNUSED = 0;
int sithCamera_g_curCycleCamNum MAYBE_UNUSED = 0;
rdVector3 sithCamera_g_vecCameraPosOffset MAYBE_UNUSED = {0};
rdVector3 sithCamera_g_vecCameraAngleOffset MAYBE_UNUSED = {0};
flex_t sithCamera_g_cameraPosDelta MAYBE_UNUSED = 0.0f;
flex_t sithCamera_g_cameraAngleDelta MAYBE_UNUSED = 0.0f;
SithCamera* sithCamera_g_pCurCamera MAYBE_UNUSED = NULL;
int sithCamera_bStartup MAYBE_UNUSED = 0;
rdMatrix34 sithCamera_g_orbCamOrient MAYBE_UNUSED = {0};
rdMatrix34 sithCamera_idleCamOrient MAYBE_UNUSED = {0};
int sithCamera_bOpen MAYBE_UNUSED = 0;
rdVector4  rdroid_aMipDistances MAYBE_UNUSED =  {10.0, 20.0, 40.0, 80.0};
int rdroid_frameTrue MAYBE_UNUSED = 0;
int bRDroidStartup MAYBE_UNUSED = 0;
int bRDroidOpen MAYBE_UNUSED = 0;
int rdroid_g_curLightingMode MAYBE_UNUSED = 0;
struct HostServices* rdroid_g_pHS MAYBE_UNUSED = NULL;
int rdroid_g_curGeometryMode MAYBE_UNUSED = 0;
stdPalEffect rdroid_curColorEffects MAYBE_UNUSED = {0};
int rdroid_curOcclusionMethod MAYBE_UNUSED = 0;
rdZBufferMethod_t rdroid_curZBufferMethod MAYBE_UNUSED = {0};
int rdroid_curProcFaceUserData MAYBE_UNUSED = 0;
int rdroid_curSortingMethod MAYBE_UNUSED = 0;
int rdroid_curAcceleration MAYBE_UNUSED = 0;
int rdroid_curTextureMode MAYBE_UNUSED = 0;
int rdroid_g_curRenderOptions MAYBE_UNUSED = 0;
int rdroid_curCullFlags MAYBE_UNUSED = 0;
tHashTable* sithMaterial_pHashtable MAYBE_UNUSED = NULL;
rdMaterial** sithMaterial_aMaterials MAYBE_UNUSED = NULL;
int sithMaterial_numMaterials MAYBE_UNUSED = 0;
rdTri rdCache_aHWSolidTris[RDCACHE_MAX_TRIS] NO_REINIT MAYBE_UNUSED = {0};
int rdCache_totalNormalTris MAYBE_UNUSED = 0;
flex_t rdCache_aIntensities[RDCACHE_MAX_VERTICES] NO_REINIT MAYBE_UNUSED = {0};
rdVector3 rdCache_aVertices[RDCACHE_MAX_VERTICES] NO_REINIT MAYBE_UNUSED = {0};
int rdCache_totalVerts MAYBE_UNUSED = 0;
rdVector2 rdCache_aTexVertices[RDCACHE_MAX_VERTICES] NO_REINIT MAYBE_UNUSED = {0};
rdTri rdCache_aHWNormalTris[RDCACHE_MAX_TRIS] NO_REINIT MAYBE_UNUSED = {0};
int rdCache_totalSolidTris MAYBE_UNUSED = 0;
D3DVERTEX rdCache_aHWVertices[RDCACHE_MAX_VERTICES] NO_REINIT MAYBE_UNUSED = {0};
int rdCache_drawnFaces MAYBE_UNUSED = 0;
int rdCache_numUsedVertices MAYBE_UNUSED = 0;
int rdCache_numUsedTexVertices MAYBE_UNUSED = 0;
int rdCache_numUsedIntensities MAYBE_UNUSED = 0;
rdVector2i rdCache_ulcExtent MAYBE_UNUSED = {0};
rdVector2i rdCache_lrcExtent MAYBE_UNUSED = {0};
int rdCache_numProcFaces MAYBE_UNUSED = 0;
rdProcEntry rdCache_aProcFaces[RDCACHE_MAX_TRIS] NO_REINIT MAYBE_UNUSED = {0};
int rdCache_dword_865258 MAYBE_UNUSED = 0;
char16_t sithMulti_name[32] MAYBE_UNUSED = {0};
sithMultiHandler_t sithMulti_pfNewPlayerJoinedCallback MAYBE_UNUSED = {0};
int sithMulti_multiModeFlags MAYBE_UNUSED = 0;
uint32_t sithMulti_numRemovedStaticThings MAYBE_UNUSED = {0};
int sithMulti_aRemovedStaticThings[256] MAYBE_UNUSED = {0};
uint32_t sithMulti_msecQuitGameTime MAYBE_UNUSED = {0};
int sithMulti_msecPingStartTime MAYBE_UNUSED = 0;
uint32_t sithMulti_msecLastSyncScoreTime MAYBE_UNUSED = {0};
uint32_t sithMulti_curWelcomePlayerNum MAYBE_UNUSED = {0};
int sithMulti_msecWelcomeUpdateInterval MAYBE_UNUSED = 0;
rdColormap* rdColormap_pCurMap MAYBE_UNUSED = NULL;
rdColormap* rdColormap_pIdentityMap MAYBE_UNUSED = NULL;
SithEvent sithEvent_aEvents[256] MAYBE_UNUSED = {0};
SithEvent* sithEvent_g_pFirstQueuedEvent MAYBE_UNUSED = NULL;
int sithEvent_arrLut[256] MAYBE_UNUSED = {0};
SithEventTask sithEvent_aTasks[SITH_NUM_EVENTS] MAYBE_UNUSED = {0};
int sithEvent_numFreeEventBuffers MAYBE_UNUSED = 0;
int sithEvent_bInit MAYBE_UNUSED = 0;
int sithEvent_bOpen MAYBE_UNUSED = 0;
int rdClip_g_faceStatus MAYBE_UNUSED = 0;
rdVector3* rdClip_pSourceVert MAYBE_UNUSED = NULL;
flex_t rdClip_workIVerts[32] NO_REINIT MAYBE_UNUSED = {0};
rdVector3 rdClip_aWorkVerts[32] NO_REINIT MAYBE_UNUSED = {0};
rdVector3* rdClip_pDestVert MAYBE_UNUSED = NULL;
flex_t* rdClip_pDestIVert MAYBE_UNUSED = NULL;
rdVector2 rdClip_aWorkTVerts[32] NO_REINIT MAYBE_UNUSED = {0};
flex_t* rdClip_pSourceIVert MAYBE_UNUSED = NULL;
rdVector2* rdClip_pSourceTVert MAYBE_UNUSED = NULL;
rdVector2* rdClip_pDestTVert MAYBE_UNUSED = NULL;
int sithSoundMixer_dword_835FCC MAYBE_UNUSED = 0;
int sithSoundMixer_bPlayingMci MAYBE_UNUSED = 0;
int sithSoundMixer_bInitted MAYBE_UNUSED = 0;
flex_t sithSoundMixer_flt_835FD8 MAYBE_UNUSED = 0.0f;
int sithSoundMixer_bIsMuted MAYBE_UNUSED = 0;
flex_t  sithSoundMixer_musicVolume MAYBE_UNUSED =  1.0f;
flex_t  sithSoundMixer_globalVolume MAYBE_UNUSED =  1.0f;
int sithSoundMixer_numSoundsAvailable2 MAYBE_UNUSED = 0;
int sithSoundMixer_numSoundsAvailable MAYBE_UNUSED = 0;
sithPlayingSound sithSoundMixer_aPlayingSounds[SITH_MIXER_NUMPLAYINGSOUNDS] MAYBE_UNUSED = {0};
int sithSoundMixer_aIdk[SITH_MIXER_NUMPLAYINGSOUNDS] MAYBE_UNUSED = {0};
int sithSoundMixer_activeChannels MAYBE_UNUSED = 0;
int sithSoundMixer_bOpened MAYBE_UNUSED = 0;
SithSector* sithSoundMixer_pCurSector MAYBE_UNUSED = NULL;
int sithSoundMixer_dword_836BFC MAYBE_UNUSED = 0;
int sithSoundMixer_trackFrom MAYBE_UNUSED = 0;
int sithSoundMixer_trackTo MAYBE_UNUSED = 0;
sithPlayingSound* sithSoundMixer_hCurAmbientChannel MAYBE_UNUSED = NULL;
int sithSoundMixer_dword_836C00 MAYBE_UNUSED = 0;
int  sithSoundMixer_channelGUIDSeed MAYBE_UNUSED =  1;
int sithSoundMixer_dword_836C04 MAYBE_UNUSED = 0;
SithThing* sithSoundMixer_pFocusedThing MAYBE_UNUSED = NULL;
int  sithSound_maxDataLoaded MAYBE_UNUSED =  0x400000;
int  sithSound_var3 MAYBE_UNUSED =  1;
int sithSound_curDataLoaded MAYBE_UNUSED = 0;
int sithSound_bInit MAYBE_UNUSED = 0;
tHashTable* sithSound_hashtable MAYBE_UNUSED = NULL;
int sithSound_var4 MAYBE_UNUSED = 0;
int sithSound_var5 MAYBE_UNUSED = 0;
int sithControl_inputFuncToControlType[INPUT_FUNC_MAX] MAYBE_UNUSED = {0};
stdControlKeyInfo sithControl_aInputFuncToKeyinfo[INPUT_FUNC_MAX] MAYBE_UNUSED = {0};
uint32_t sithControl_msIdle MAYBE_UNUSED = {0};
int sithControl_bInitted MAYBE_UNUSED = 0;
int sithControl_bOpened MAYBE_UNUSED = 0;
sithControl_handler_t sithControl_aHandlers[SITHCONTROL_NUM_HANDLERS] MAYBE_UNUSED = {0};
int sithControl_numHandlers MAYBE_UNUSED = 0;
int sithControl_death_msgtimer MAYBE_UNUSED = 0;
rdVector3  sithControl_vec3_54A570 MAYBE_UNUSED =  {0.0, -1.0, 0.0};
flex_t  sithControl_flt_54A57C MAYBE_UNUSED =  0.2f;
sithSaveHandler_t sithGamesave_func1 MAYBE_UNUSED = {0};
sithSaveHandler_t sithGamesave_func2 MAYBE_UNUSED = {0};
sithSaveHandler_t sithGamesave_func3 MAYBE_UNUSED = {0};
sithSaveHandler_t sithGamesave_funcWrite MAYBE_UNUSED = {0};
sithSaveHandler_t sithGamesave_funcRead MAYBE_UNUSED = {0};
char sithGamesave_autosave_fname[128] MAYBE_UNUSED = {0};
sithGamesaveState_t sithGamesave_state MAYBE_UNUSED = {0};
int sithGamesave_dword_835914 MAYBE_UNUSED = 0;
char sithGamesave_aCurFilename[132] MAYBE_UNUSED = {0};
char16_t sithGamesave_wsaveName[256] MAYBE_UNUSED = {0};
char sithGamesave_saveName[128] MAYBE_UNUSED = {0};
sithGamesave_Header sithGamesave_headerTmp MAYBE_UNUSED = {0};
rdCamera* rdCamera_g_pCurCamera MAYBE_UNUSED = NULL;
rdMatrix34 rdCamera_g_camMatrix MAYBE_UNUSED = {0};
int32_t  sithNet_MultiModeFlags MAYBE_UNUSED = {0};
int32_t  sithNet_scorelimit MAYBE_UNUSED = {0};
int32_t  sithNet_teamScore[5] MAYBE_UNUSED = {0};
int32_t  sithNet_multiplayer_timelimit MAYBE_UNUSED = {0};
int32_t  sithMulti_multiplayerTimelimit MAYBE_UNUSED = {0};
int32_t  sithNet_isMulti MAYBE_UNUSED = {0};
int32_t  sithNet_isServer MAYBE_UNUSED = {0};
int32_t  sithMulti_bTimelimitMet MAYBE_UNUSED = {0};
int32_t  sithNet_serverNetId MAYBE_UNUSED = {0};
int32_t  sithNet_things[SITH_MAX_THINGS] MAYBE_UNUSED = {0};
int32_t  sithNet_thingsIdx MAYBE_UNUSED = {0};
int32_t  sithNet_syncIdx MAYBE_UNUSED = {0};
int32_t  sithNet_aSyncFlags[SITH_MAX_SYNC_THINGS] MAYBE_UNUSED = {0};
SithThing* sithNet_aSyncThings[SITH_MAX_SYNC_THINGS] MAYBE_UNUSED = {0};
int32_t   sithNet_tickrate MAYBE_UNUSED =  180;
int32_t  sithNet_dword_8C4BA8 MAYBE_UNUSED = {0};
int32_t  sithNet_dword_83262C MAYBE_UNUSED = {0};
int32_t  sithMulti_quitGameState MAYBE_UNUSED = {0};
int32_t  sithNet_checksum MAYBE_UNUSED = {0};
int32_t  sithNet_bNeedsFullThingSyncForLeaveJoin MAYBE_UNUSED = {0};
int32_t  sithMulti_newPlayerId MAYBE_UNUSED = {0};
int32_t  sithNet_bSyncScores MAYBE_UNUSED = {0};
int32_t  sithNet_dword_832620 MAYBE_UNUSED = {0};
flex_t  sithOverlayMap_curScale MAYBE_UNUSED =  100.0;
rdMatrix34 sithOverlayMap_mapOrient MAYBE_UNUSED = {0};
SithThing* sithOverlayMap_pLocalPlayer MAYBE_UNUSED = NULL;
rdCanvas* sithOverlayMap_pCanvas MAYBE_UNUSED = NULL;
int sithOverlayMap_x1 MAYBE_UNUSED = 0;
int sithOverlayMap_y1 MAYBE_UNUSED = 0;
sithMapView sithOverlayMap_inst MAYBE_UNUSED = {0};
int sithOverlayMap_bMapVisible MAYBE_UNUSED = 0;
int sithOverlayMap_bOpened MAYBE_UNUSED = 0;
tHashTable* sithSoundClass_pHashtblModes MAYBE_UNUSED = NULL;
tHashTable* sithSoundClass_pHashTable MAYBE_UNUSED = NULL;
void* sithTemplate_pMasterFile MAYBE_UNUSED = NULL;
tHashTable* sithTemplate_pHashtable MAYBE_UNUSED = NULL;
int sithTemplate_masterFileCount MAYBE_UNUSED = 0;
tHashTable* sithTemplate_pMasterHashtable MAYBE_UNUSED = NULL;
rdEdge NO_REINIT activeEdgeTail MAYBE_UNUSED = {0};
rdEdge NO_REINIT activeEdgeHead MAYBE_UNUSED = {0};
rdEdge* apNewActiveEdges[1024] NO_REINIT MAYBE_UNUSED = {0};
rdEdge* apRemoveActiveEdges[1024] NO_REINIT MAYBE_UNUSED = {0};
int yMinEdge MAYBE_UNUSED = 0;
int yMaxEdge MAYBE_UNUSED = 0;
int numActiveSpans MAYBE_UNUSED = 0;
int numActiveFaces MAYBE_UNUSED = 0;
int numActiveEdges MAYBE_UNUSED = 0;
rdEdge aActiveEdges[1024] NO_REINIT  MAYBE_UNUSED = {0};
int rdActive_drawnFaces MAYBE_UNUSED = 0;
int sithMain_bEndLevel MAYBE_UNUSED = 0;
int sith_bStartup MAYBE_UNUSED = 0;
int sith_bOpen MAYBE_UNUSED = 0;
int sithSurface_numAvail MAYBE_UNUSED = 0;
int sithSurface_aAvail[256+1] MAYBE_UNUSED = {0};
int sithSurface_numSurfaces MAYBE_UNUSED = 0;
rdSurface sithSurface_aSurfaces[256] MAYBE_UNUSED = {0};
int sithSurface_bOpened MAYBE_UNUSED = 0;
uint32_t  sithSurface_byte_8EE668 MAYBE_UNUSED =  0;
int sithSurface_numUnsyncedSurfaces MAYBE_UNUSED = 0;
rdMaterialLoader_t pMaterialsLoader MAYBE_UNUSED = {0};
rdMaterialUnloader_t pMaterialsUnloader MAYBE_UNUSED = {0};
tHashTable* sithSprite_pHashtable MAYBE_UNUSED = NULL;
WindowDrawHandler_t Window_drawAndFlip MAYBE_UNUSED = {0};
WindowDrawHandler_t Window_setCooperativeLevel MAYBE_UNUSED = {0};
wm_handler Window_ext_handlers[16] MAYBE_UNUSED = {0};
HWND Window_aDialogHwnds[16] MAYBE_UNUSED = {0};
int DebugGui_aIdk[32] MAYBE_UNUSED = {0};
int DebugGui_idk MAYBE_UNUSED = 0;
int DebugGui_some_line_amt MAYBE_UNUSED = 0;
int DebugGui_some_num_lines MAYBE_UNUSED = 0;
char DebugLog_buffer[4072] MAYBE_UNUSED = {0};
stdDebugConsoleCmd* sithConsole_aCmds MAYBE_UNUSED = NULL;
tHashTable* sithConsole_pCmdHashtable MAYBE_UNUSED = NULL;
int sithConsole_bOpened MAYBE_UNUSED = 0;
int sithConsole_bInitted MAYBE_UNUSED = 0;
int sithConsole_maxCmds MAYBE_UNUSED = 0;
int sithConsole_numRegisteredCmds MAYBE_UNUSED = 0;
uint32_t DebugGui_maxLines MAYBE_UNUSED = {0};
DebugConsolePrintFunc_t DebugGui_fnPrint MAYBE_UNUSED = {0};
DebugConsolePrintUniStrFunc_t DebugGui_fnPrintUniStr MAYBE_UNUSED = {0};
stdSound_buffer_t* sithConsole_alertSound MAYBE_UNUSED = NULL;
int16_t sithConsole_idk2 MAYBE_UNUSED = {0};
uint32_t d3d_maxVertices MAYBE_UNUSED = {0};
d3d_device* d3d_device_ptr MAYBE_UNUSED = NULL;
rdRect std3D_rectViewIdk MAYBE_UNUSED = {0};
flex_t std3D_aViewIdk[32] MAYBE_UNUSED = {0};
rdTri std3D_aViewTris[2] MAYBE_UNUSED = {0};
int32_t  std3D_gpuMaxTexSizeMaybe MAYBE_UNUSED =  1;
int32_t  std3D_dword_53D66C MAYBE_UNUSED =  1;
int32_t  std3D_dword_53D670 MAYBE_UNUSED =  0x100;
int32_t  std3D_dword_53D674 MAYBE_UNUSED =  0x100;
int32_t  std3D_frameCount MAYBE_UNUSED =  1;
intptr_t std3D_renderList MAYBE_UNUSED = {0};
int32_t std3D_numCachedTextures MAYBE_UNUSED = {0};
rdDDrawSurface* std3D_pFirstTexCache MAYBE_UNUSED = NULL;
rdDDrawSurface*  std3D_pLastTexCache MAYBE_UNUSED = NULL;
int stdComm_dword_8321F8 MAYBE_UNUSED = 0;
int stdComm_bInitted MAYBE_UNUSED = 0;
int stdComm_dword_8321F0 MAYBE_UNUSED = 0;
int stdComm_dword_8321F4 MAYBE_UNUSED = 0;
int stdComm_dplayIdSelf MAYBE_UNUSED = 0;
int stdComm_dword_832204 MAYBE_UNUSED = 0;
int stdComm_dword_832208 MAYBE_UNUSED = 0;
int stdComm_currentBigSyncStage MAYBE_UNUSED = 0;
int stdComm_dword_8321E0 MAYBE_UNUSED = 0;
int stdComm_bIsServer MAYBE_UNUSED = 0;
int stdComm_dword_8321E8 MAYBE_UNUSED = 0;
char16_t stdComm_waIdk[32] MAYBE_UNUSED = {0};
int stdComm_dword_8321DC MAYBE_UNUSED = 0;
int stdComm_dword_832200 MAYBE_UNUSED = 0;
int stdComm_dword_832210 MAYBE_UNUSED = 0;
SithMessage stdComm_cogMsgTmp MAYBE_UNUSED = {0};
MCIDEVICEID stdMci_mciId MAYBE_UNUSED = {0};
int stdMci_dwVolume MAYBE_UNUSED = 0;
int stdMci_bInitted MAYBE_UNUSED = 0;
int stdMci_uDeviceID MAYBE_UNUSED = 0;
int wuRegistry_bStarted MAYBE_UNUSED = 0;
uint8_t wuRegistry_lpClass[4] MAYBE_UNUSED = {0};
uint8_t wuRegistry_byte_855EB4[4] MAYBE_UNUSED = {0};
HKEY wuRegistry_hKey MAYBE_UNUSED = {0};
LPCSTR wuRegistry_lpSubKey MAYBE_UNUSED = {0};
uint32_t WinIdk_aDplayGuid[4] MAYBE_UNUSED = {0};
StdDisplayInfo stdDisplay_aDisplayDevices[16] MAYBE_UNUSED = {0};
int stdDisplay_gammaTableLen MAYBE_UNUSED = 0;
flex_d_t* stdDisplay_paGammaTable MAYBE_UNUSED = NULL;
rdColor24 stdDisplay_gammaPalette[256] MAYBE_UNUSED = {0};
StdDisplayInfo* stdDisplay_pCurDevice MAYBE_UNUSED = NULL;
StdVideoMode* stdDisplay_pCurVideoMode MAYBE_UNUSED = NULL;
int stdDisplay_bStartup MAYBE_UNUSED = 0;
int stdDisplay_bOpen MAYBE_UNUSED = 0;
int stdDisplay_bModeSet MAYBE_UNUSED = 0;
int stdDisplay_numVideoModes MAYBE_UNUSED = 0;
int stdDisplay_bPaged MAYBE_UNUSED = 0;
uint8_t stdDisplay_tmpGammaPal[0x300] MAYBE_UNUSED = {0};
int stdDisplay_gammaIdx MAYBE_UNUSED = 0;
uint16_t word_860800 MAYBE_UNUSED = {0};
uint16_t word_860802 MAYBE_UNUSED = {0};
uint16_t word_860804 MAYBE_UNUSED = {0};
uint16_t word_860806 MAYBE_UNUSED = {0};
int  stdControl_bReadMouse MAYBE_UNUSED =  1;
flex_t  stdControl_updateKHz MAYBE_UNUSED =  1.0f;
flex_t  stdControl_updateHz MAYBE_UNUSED =  1.0f;
flex_t  stdControl_mouseXSensitivity MAYBE_UNUSED =  1.0f;
flex_t  stdControl_mouseYSensitivity MAYBE_UNUSED =  1.0f;
flex_t  stdControl_mouseZSensitivity MAYBE_UNUSED =  1.0f;
int stdControl_aAxisEnabled[JK_NUM_JOYSTICKS] MAYBE_UNUSED = {0};
int stdControl_aAxisStates[JK_NUM_AXES] MAYBE_UNUSED = {0};
int stdControl_aKeyIdleTimes[JK_NUM_KEYS] MAYBE_UNUSED = {0};
int stdControl_aKeyInfo[JK_NUM_KEYS] MAYBE_UNUSED = {0};
int stdControl_aAxisConnected[JK_NUM_JOYSTICKS] MAYBE_UNUSED = {0};
int stdControl_aKeyPressed[JK_NUM_KEYS] MAYBE_UNUSED = {0};
int stdControl_bStartup MAYBE_UNUSED = 0;
int stdControl_bOpen MAYBE_UNUSED = 0;
int stdControl_bDisableKeyboard MAYBE_UNUSED = 0;
int stdControl_bControlsIdle MAYBE_UNUSED = 0;
int stdControl_bControlsActive MAYBE_UNUSED = 0;
void* stdControl_pDI MAYBE_UNUSED = NULL;
void* stdControl_mouseDirectInputDevice MAYBE_UNUSED = NULL;
void* stdControl_keyboardIDirectInputDevice MAYBE_UNUSED = NULL;
int stdControl_bReadJoysticks MAYBE_UNUSED = 0;
uint32_t stdControl_curReadTime MAYBE_UNUSED = {0};
uint32_t stdControl_lastReadTime MAYBE_UNUSED = {0};
uint32_t stdControl_readDeltaTime MAYBE_UNUSED = {0};
int32_t stdControl_dwLastMouseX MAYBE_UNUSED = {0};
int32_t stdControl_dwLastMouseY MAYBE_UNUSED = {0};
int stdControl_aJoystickEnabled[JK_NUM_JOYSTICKS] MAYBE_UNUSED = {0};
int stdControl_aJoystickExists[4] MAYBE_UNUSED = {0};
stdControlJoystickEntry stdControl_aAxes[JK_NUM_AXES] MAYBE_UNUSED = {0};
int stdControl_aJoystickMaxButtons[JK_NUM_JOYSTICKS] MAYBE_UNUSED = {0};
int Windows_installType MAYBE_UNUSED = 0;
uint8_t Video_aGammaTable[80]  MAYBE_UNUSED =  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0x17, 0x5D, 0x74, 0xD1, 0x45, 0x17, 0xED, 0x3F, 0xAB, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xEA, 0x3F, 0xB7, 0x6D, 0xDB, 0xB6, 0x6D, 0xDB, 0xE6, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE4, 0x3F, 0x72, 0x1C, 0xC7, 0x71, 0x1C, 0xC7, 0xE1, 0x3F, 0x79, 0x0D, 0xE5, 0x35, 0x94, 0xD7, 0xE0, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, 0x9E, 0xE7, 0x79, 0x9E, 0xE7, 0x79, 0xDE, 0x3F, 0xBE, 0xE9, 0x4D, 0x6F, 0x7A, 0xD3, 0xDB, 0x3F};
int  Video_fillColor MAYBE_UNUSED =  0;
videoModeStruct Video_modeStruct MAYBE_UNUSED = {0};
tVBuffer Video_otherBuf MAYBE_UNUSED = {0};
int Video_dword_866D78 MAYBE_UNUSED = 0;
int Video_curMode MAYBE_UNUSED = 0;
StdVideoMode Video_renderSurface[9]  MAYBE_UNUSED = {0};
tVBuffer Video_menuBuffer MAYBE_UNUSED = {0};
tVBuffer* Video_pOtherBuf MAYBE_UNUSED = NULL;
tVBuffer* Video_pMenuBuffer MAYBE_UNUSED = NULL;
int Video_bInitted MAYBE_UNUSED = 0;
int Video_bOpened MAYBE_UNUSED = 0;
flex_t Video_flt_55289C MAYBE_UNUSED = 0.0f;
int Video_dword_5528A0 MAYBE_UNUSED = 0;
int Video_dword_5528A4 MAYBE_UNUSED = 0;
int Video_dword_5528A8 MAYBE_UNUSED = 0;
int Video_lastTimeMsec MAYBE_UNUSED = 0;
int Video_dword_5528B0 MAYBE_UNUSED = 0;
tVBuffer* Video_pVbufIdk MAYBE_UNUSED = NULL;
rdCanvas* Video_pCanvas MAYBE_UNUSED = NULL;
rdColor24 Video_aPalette[0x100] MAYBE_UNUSED = {0};
tRasterInfo Video_format MAYBE_UNUSED = {0};
tRasterInfo Video_format2 MAYBE_UNUSED = {0};
videoModeStruct Video_modeStruct2 MAYBE_UNUSED = {0};
tVBuffer Video_bufIdk MAYBE_UNUSED = {0};
uint16_t stdConsole_textAttribute MAYBE_UNUSED = {0};
uint16_t stdConsole_wAttributes MAYBE_UNUSED = {0};
int stdConsole_cursorHidden MAYBE_UNUSED = 0;
CONSOLE_CURSOR_INFO stdConsole_ConsoleCursorInfo MAYBE_UNUSED = {0};
HANDLE stdConsole_hOutput MAYBE_UNUSED = {0};
HANDLE stdConsole_hInput MAYBE_UNUSED = {0};
int rdRaster_aOneOverNFixed[2048] MAYBE_UNUSED = {0};
flex_t rdRaster_aLerpAndScale[16] MAYBE_UNUSED = {0};
flex_t rdRaster_aOneOverNFlex[2048] MAYBE_UNUSED = {0};
flex_t  rdRaster_fixedScale MAYBE_UNUSED =  65536.0f;
SithThing* sithCogFunctionAI_aThingsInView[32] MAYBE_UNUSED = {0};
int sithCogFunctionAI_numThingsInView MAYBE_UNUSED = 0;
int sithCogFunctionAI_curThingInView MAYBE_UNUSED = 0;
#ifndef SITHCOMM_HEAP_MSGBUF // Added: heap-shadowed on some platforms
SithMessage sithComm_MsgTmpBuf[32] MAYBE_UNUSED = {0};
#endif
sithCogMsg_Pair sithComm_aMsgPairs[128] MAYBE_UNUSED = {0};
cogMsg_Handler sithMessage_aTypeFuncs[65] MAYBE_UNUSED = {0};
int sithMessage_bStopProcessMessages MAYBE_UNUSED = 0;
int sithMessage_g_outputstream MAYBE_UNUSED = 0;
int sithMessage_g_inputstream MAYBE_UNUSED = 0;
int sithComm_idk2 MAYBE_UNUSED = 0;
int sithMessage_bSturtup MAYBE_UNUSED = 0;
uint32_t sithComm_dword_847E84 MAYBE_UNUSED = {0};
int  sithComm_msgId MAYBE_UNUSED =  1;
SithMessage sithComm_MsgTmpBuf2 MAYBE_UNUSED = {0};
SithMessage sithComm_netMsgTmp MAYBE_UNUSED = {0};
char16_t jkCog_emptystring[2] MAYBE_UNUSED = {0};
char16_t jkCog_jkstring[130] MAYBE_UNUSED = {0};
int jkCog_bInitted MAYBE_UNUSED = 0;
stdStrTable jkCog_strings MAYBE_UNUSED = {0};
int sithCog_bOpened MAYBE_UNUSED = 0;
tHashTable* sithCog_g_pHashtable MAYBE_UNUSED = NULL;
SithCogSectorLink sithCog_aSectorLinks[SITHCOG_MAX_LINKS] MAYBE_UNUSED = {0};
int sithCog_numSectorLinks MAYBE_UNUSED = 0;
SithCogThingLink sithCog_aThingLinks[SITHCOG_MAX_LINKS] MAYBE_UNUSED = {0};
int sithCog_numThingLinks MAYBE_UNUSED = 0;
int sithCog_numSurfaceLinks MAYBE_UNUSED = 0;
SithCogSurfaceLink sithCog_aSurfaceLinks[SITHCOG_MAX_LINKS] MAYBE_UNUSED = {0};
sithCog* sithCog_g_pMasterCog MAYBE_UNUSED = NULL;
rdVector2i jkDev_aEntryPositions[10] MAYBE_UNUSED = {0};
jkDevLogEnt jkDev_aEntries[5] MAYBE_UNUSED = {0};
int jkDev_log_55A4A4 MAYBE_UNUSED = 0;
int jkDev_bScreenNeedsUpdate MAYBE_UNUSED = 0;
stdDebugConsoleCmd jkDev_aCheatCmds[JKDEV_NUM_CHEATS] MAYBE_UNUSED = {0};
uint32_t jkDev_numCheats MAYBE_UNUSED = {0};
int jkDev_bInitted MAYBE_UNUSED = 0;
int jkDev_bOpened MAYBE_UNUSED = 0;
tHashTable* jkDev_cheatHashtable MAYBE_UNUSED = NULL;
HWND jkDev_hDlg MAYBE_UNUSED = {0};
tVBuffer* jkDev_vbuf MAYBE_UNUSED = NULL;
int jkDev_BMFontHeight MAYBE_UNUSED = 0;
int jkDev_ColorKey MAYBE_UNUSED = 0;
int jkDev_dword_55A9D0 MAYBE_UNUSED = 0;
flex_t jkDev_amt MAYBE_UNUSED = 0.0f;
int jkSmack_gameMode MAYBE_UNUSED = 0;
int jkSmack_bInit MAYBE_UNUSED = 0;
int jkSmack_stopTick MAYBE_UNUSED = 0;
int jkSmack_currentGuiState MAYBE_UNUSED = 0;
int jkSmack_nextGuiState MAYBE_UNUSED = 0;
void* jkSmack_alloc MAYBE_UNUSED = NULL;
jkEpisode jkEpisode_aEpisodes[64] MAYBE_UNUSED = {0};
char jkEpisode_var4[128] MAYBE_UNUSED = {0};
char jkEpisode_var5[128] MAYBE_UNUSED = {0};
uint32_t jkEpisode_var2 MAYBE_UNUSED = {0};
jkEpisodeLoad jkEpisode_mLoad MAYBE_UNUSED = {0};
uint32_t  jkHud_targetRed MAYBE_UNUSED =  1;
uint32_t  jkHud_targetBlue MAYBE_UNUSED =  2;
uint32_t  jkHud_targetGreen MAYBE_UNUSED =  3;
flex_t jkHud_aFltIdk[6]  MAYBE_UNUSED =  {0.2, 0.4, 0.8, 1.0, 2.0, 0.0};
int32_t jkHud_aColors8bpp[6]  MAYBE_UNUSED =  {1,2,3,4,5,0};
int jkHud_aTeamColors8bpp[5]  MAYBE_UNUSED =  {0x0, 0x6, 0x69, 0x1F, 0x2};
char jkHud_chatStr[128] MAYBE_UNUSED = {0};
int jkHud_aTeamColors16bpp[5] MAYBE_UNUSED = {0};
uint32_t jkHud_rightBlitX MAYBE_UNUSED = {0};
uint32_t jkHud_leftBlitX MAYBE_UNUSED = {0};
SithOverlayMapConfig jkHud_mapRendConfig MAYBE_UNUSED = {0};
int jkHud_chatStrPos MAYBE_UNUSED = 0;
int jkHud_rightBlitY MAYBE_UNUSED = 0;
jkHudTeamScore jkHud_aTeamScores[5] MAYBE_UNUSED = {0};
int jkHud_dword_552D10 MAYBE_UNUSED = 0;
int32_t jkHud_aColors16bpp[6] MAYBE_UNUSED = {0};
jkHudPlayerScore jkHud_aPlayerScores[32] MAYBE_UNUSED = {0};
int jkHud_blittedAmmoAmt MAYBE_UNUSED = 0;
int jkHud_idk14 MAYBE_UNUSED = 0;
int jkHud_blittedHealthIdx MAYBE_UNUSED = 0;
int jkHud_blittedBatteryAmt MAYBE_UNUSED = 0;
int jkHud_blittedFieldlightAmt MAYBE_UNUSED = 0;
int jkHud_blittedShieldIdx MAYBE_UNUSED = 0;
int jkHud_isSuper MAYBE_UNUSED = 0;
int jkHud_idk15 MAYBE_UNUSED = 0;
int jkHud_blittedForceIdx MAYBE_UNUSED = 0;
int jkHud_idk16 MAYBE_UNUSED = 0;
int jkHud_leftBlitY MAYBE_UNUSED = 0;
rdRect jkHud_rectViewScores MAYBE_UNUSED = {0};
stdFont* jkHud_pMsgFontSft MAYBE_UNUSED = NULL;
stdBitmap* jkHud_pStatusLeftBm MAYBE_UNUSED = NULL;
stdBitmap* jkHud_pStatusRightBm MAYBE_UNUSED = NULL;
int jkHud_bHasTarget MAYBE_UNUSED = 0;
SithThing* jkHud_pTargetThing MAYBE_UNUSED = NULL;
uint32_t jkHud_targetRed16 MAYBE_UNUSED = {0};
uint32_t jkHud_targetGreen16 MAYBE_UNUSED = {0};
uint32_t jkHud_targetBlue16 MAYBE_UNUSED = {0};
int jkHud_bChatOpen MAYBE_UNUSED = 0;
stdFont* jkHud_pHelthNumSft MAYBE_UNUSED = NULL;
stdFont* jkHud_pAmoNumsSft MAYBE_UNUSED = NULL;
stdFont* jkHud_pAmoNumsSuperSft MAYBE_UNUSED = NULL;
stdFont* jkHud_pArmorNumSft MAYBE_UNUSED = NULL;
stdFont* jkHud_pArmorNumsSuperSft MAYBE_UNUSED = NULL;
int jkHud_bInitted MAYBE_UNUSED = 0;
int jkHud_bOpened MAYBE_UNUSED = 0;
stdBitmap* jkHud_pFieldlightBm MAYBE_UNUSED = NULL;
stdBitmap* jkHud_pStBatBm MAYBE_UNUSED = NULL;
stdBitmap* jkHud_pStHealthBm MAYBE_UNUSED = NULL;
stdBitmap* jkHud_pStShieldBm MAYBE_UNUSED = NULL;
stdBitmap* jkHud_pStFrcBm MAYBE_UNUSED = NULL;
stdBitmap* jkHud_pStFrcSuperBm MAYBE_UNUSED = NULL;
int jkHud_bViewScores MAYBE_UNUSED = 0;
int jkHud_dword_553ED0 MAYBE_UNUSED = 0;
int jkHud_tallyWhich MAYBE_UNUSED = 0;
uint32_t jkHud_numPlayers MAYBE_UNUSED = {0};
int jkHud_dword_553EDC MAYBE_UNUSED = 0;
int jkHud_dword_553EE0 MAYBE_UNUSED = 0;
jkHudInvInfo jkHudInv_info MAYBE_UNUSED = {0};
rdTexFormat jkHudInv_itemTexfmt MAYBE_UNUSED = {0};
int jkHudInv_flags MAYBE_UNUSED = 0;
int jkHudInv_dword_553F64 MAYBE_UNUSED = 0;
jkHudInvScroll jkHudInv_scroll MAYBE_UNUSED = {0};
stdBitmap* jkHudInv_aBitmaps[3] MAYBE_UNUSED = {0};
stdFont* jkHudInv_font MAYBE_UNUSED = NULL;
int jkHudInv_rend_isshowing_maybe MAYBE_UNUSED = 0;
int jkHudInv_dword_553F94 MAYBE_UNUSED = 0;
int jkHudInv_numItems MAYBE_UNUSED = 0;
int* jkHudInv_aItems MAYBE_UNUSED = NULL;
int Main_bDevMode MAYBE_UNUSED = 0;
int Main_bDisplayConfig MAYBE_UNUSED = 0;
int Main_bWindowGUI MAYBE_UNUSED = 0;
int Main_dword_86078C MAYBE_UNUSED = 0;
int Main_bFrameRate MAYBE_UNUSED = 0;
int Main_bDispStats MAYBE_UNUSED = 0;
int Main_bNoHUD MAYBE_UNUSED = 0;
int Main_logLevel MAYBE_UNUSED = 0;
int Main_verboseLevel MAYBE_UNUSED = 0;
char Main_path[128] MAYBE_UNUSED = {0};
stdFile_t  debug_log_fp MAYBE_UNUSED =  0;
HostServices* pHS MAYBE_UNUSED = NULL;
char jkCredits_aPalette[0x300] MAYBE_UNUSED = {0};
tVBuffer* jkCredits_pVbuffer2 MAYBE_UNUSED = NULL;
int jkCredits_dword_55AD64 MAYBE_UNUSED = 0;
int jkCredits_dword_55AD68 MAYBE_UNUSED = 0;
stdStrTable jkCredits_table MAYBE_UNUSED = {0};
int jkCredits_startMs MAYBE_UNUSED = 0;
int jkCredits_dword_55AD84 MAYBE_UNUSED = 0;
int jkCredits_strIdx MAYBE_UNUSED = 0;
char* jkCredits_aIdk MAYBE_UNUSED = NULL;
tVBuffer* jkCredits_pVbuffer MAYBE_UNUSED = NULL;
int jkCredits_dword_55AD94 MAYBE_UNUSED = 0;
stdFont* jkCredits_fontLarge MAYBE_UNUSED = NULL;
stdFont* jkCredits_fontSmall MAYBE_UNUSED = NULL;
int jkCredits_dword_55ADA0 MAYBE_UNUSED = 0;
int jkCredits_bInitted MAYBE_UNUSED = 0;
int jkCredits_dword_55ADA8 MAYBE_UNUSED = 0;
int g_sithMode MAYBE_UNUSED = 0;
int g_submodeFlags MAYBE_UNUSED = 0;
int g_debugmodeFlags MAYBE_UNUSED = 0;
int g_mapModeFlags MAYBE_UNUSED = 0;
int jkGame_gamma MAYBE_UNUSED = 0;
int jkGame_screenSize MAYBE_UNUSED = 0;
int jkGame_bInitted MAYBE_UNUSED = 0;
int jkGame_updateMsecsTotal MAYBE_UNUSED = 0;
int jkGame_dword_552B5C MAYBE_UNUSED = 0;
int jkGame_isDDraw MAYBE_UNUSED = 0;
HostServices* jkRes_pHS MAYBE_UNUSED = NULL;
char jkRes_episodeGobName[32]  MAYBE_UNUSED =  {0};
char jkRes_curDir[128]  MAYBE_UNUSED =  {0};
int jkRes_bHookedHS MAYBE_UNUSED = 0;
jkResFile jkRes_aFiles[32] MAYBE_UNUSED = {0};
jkRes jkRes_gCtx MAYBE_UNUSED = {0};
HostServices* pLowLevelHS MAYBE_UNUSED = NULL;
HostServices lowLevelHS MAYBE_UNUSED = {0};
char jkRes_idkGobPath[128] MAYBE_UNUSED = {0};
rdRect jkCutscene_rect1 MAYBE_UNUSED = {0};
rdRect jkCutscene_rect2 MAYBE_UNUSED = {0};
stdStrTable jkCutscene_strings MAYBE_UNUSED = {0};
stdFont* jkCutscene_subtitlefont MAYBE_UNUSED = NULL;
int jkCutscene_bInitted MAYBE_UNUSED = 0;
int jkCutscene_isRendering MAYBE_UNUSED = 0;
int jkCutscene_dword_55B750 MAYBE_UNUSED = 0;
int jkCutscene_dword_55AA50 MAYBE_UNUSED = 0;
int jkCutscene_55AA54 MAYBE_UNUSED = 0;
char jkMain_aLevelJklFname[128] MAYBE_UNUSED = {0};
int  thing_nine MAYBE_UNUSED =  1;
int jkMain_bInit MAYBE_UNUSED = 0;
int thing_six MAYBE_UNUSED = 0;
int thing_eight MAYBE_UNUSED = 0;
int jkMain_dword_552B98 MAYBE_UNUSED = 0;
int jkMain_lastTickMs MAYBE_UNUSED = 0;
int idx_13b4_related MAYBE_UNUSED = 0;
char gamemode_1_str[128] MAYBE_UNUSED = {0};
char jkMain_strIdk[128] MAYBE_UNUSED = {0};
char16_t jkMain_wstrIdk[128] MAYBE_UNUSED = {0};
SithWorldTextSectionParseHandler sithWorld_aSectionParsers[32] MAYBE_UNUSED = {0};
uint32_t sithWorld_some_integer_4 MAYBE_UNUSED = {0};
SithWorld* sithWorld_g_pCurrentWorld MAYBE_UNUSED = NULL;
SithWorld* sithWorld_g_pStaticWorld MAYBE_UNUSED = NULL;
SithWorld* sithWorld_g_pLastLoadedWorld MAYBE_UNUSED = NULL;
uint32_t sithWorld_numParsers MAYBE_UNUSED = {0};
uint32_t sithWorld_bInitted MAYBE_UNUSED = {0};
int sithWorld_bLoaded MAYBE_UNUSED = 0;
char sithWorld_episodeName[32] MAYBE_UNUSED = {0};
SithControlBinding sithInventory_powerKeybinds[SITHINVENTORY_NUM_POWERKEYBINDS] MAYBE_UNUSED = {0};
int  sithInventory_g_bInitInventory MAYBE_UNUSED =  1;
SithInventoryType sithInventory_g_aTypes[SITHBIN_NUMBINS] MAYBE_UNUSED = {0};
int sithInventory_g_bSendDeactivateMessage MAYBE_UNUSED = 0;
int sithInventory_bUnkPower MAYBE_UNUSED = 0;
int sithInventory_8339EC MAYBE_UNUSED = 0;
int sithInventory_bRendIsHidden MAYBE_UNUSED = 0;
int sithInventory_8339F4 MAYBE_UNUSED = 0;
uint32_t sithWeapon_controlOptions MAYBE_UNUSED = {0};
flex_t g_flt_8BD040 MAYBE_UNUSED = 0.0f;
flex_t g_flt_8BD044 MAYBE_UNUSED = 0.0f;
flex_t g_flt_8BD048 MAYBE_UNUSED = 0.0f;
flex_t g_flt_8BD04C MAYBE_UNUSED = 0.0f;
flex_t g_flt_8BD050 MAYBE_UNUSED = 0.0f;
flex_t g_flt_8BD054 MAYBE_UNUSED = 0.0f;
flex_t g_flt_8BD058 MAYBE_UNUSED = 0.0f;
int sithWeapon_CurWeaponMode MAYBE_UNUSED = 0;
int sithWeapon_bAutoPickup MAYBE_UNUSED = 0;
int sithWeapon_bAutoSwitch MAYBE_UNUSED = 0;
int sithWeapon_bAutoReload MAYBE_UNUSED = 0;
int sithWeapon_bMultiAutoPickup MAYBE_UNUSED = 0;
int sithWeapon_bMultiplayerAutoSwitch MAYBE_UNUSED = 0;
int sithWeapon_bMultiAutoReload MAYBE_UNUSED = 0;
int sithWeapon_bAutoAim MAYBE_UNUSED = 0;
flex32_t sithWeapon_secMountWait MAYBE_UNUSED = 0.0f;
flex_t sithWeapon_8BD0A0[2] MAYBE_UNUSED = {0};
flex32_t sithWeapon_fireWait MAYBE_UNUSED = 0.0f;
flex32_t sithWeapon_fireRate MAYBE_UNUSED = 0.0f;
flex32_t sithWeapon_LastFireTimeSecs MAYBE_UNUSED = 0.0f;
int sithWeapon_a8BD030[4] MAYBE_UNUSED = {0};
int sithWeapon_8BD05C MAYBE_UNUSED = 0;
flex32_t sithWeapon_8BD060 MAYBE_UNUSED = 0.0f;
int sithWeapon_8BD008[6] MAYBE_UNUSED = {0};
int sithWeapon_8BD024 MAYBE_UNUSED = 0;
int sithWeapon_senderIndex MAYBE_UNUSED = 0;
SithPlayer jkPlayer_playerInfos[JKPLAYER_NUM_INFOS] MAYBE_UNUSED = {0};
char16_t jkPlayer_playerShortName[32] MAYBE_UNUSED = {0};
int jkPlayer_numOtherThings MAYBE_UNUSED = 0;
int jkPlayer_numThings MAYBE_UNUSED = 0;
jkPlayerInfo jkPlayer_otherThings[NUM_JKPLAYER_THINGS] MAYBE_UNUSED = {0};
int  jkPlayer_bLoadingSomething MAYBE_UNUSED =  1;
int playerThingIdx MAYBE_UNUSED = 0;
uint32_t jkPlayer_maxPlayers MAYBE_UNUSED = {0};
SithThing* sithPlayer_g_pLocalPlayerThing MAYBE_UNUSED = NULL;
SithPlayer* sithPlayer_g_pLocalPlayer MAYBE_UNUSED = NULL;
jkPlayerInfo playerThings[JKPLAYER_NUM_INFOS] MAYBE_UNUSED = {0};
rdMatrix34 jkSaber_rotateMat MAYBE_UNUSED = {0};
int jkPlayer_setRotateOverlayMap MAYBE_UNUSED = 0;
int jkPlayer_setDrawStatus MAYBE_UNUSED = 0;
int jkPlayer_setCrosshair MAYBE_UNUSED = 0;
int jkPlayer_setSaberCam MAYBE_UNUSED = 0;
int jkPlayer_setFullSubtitles MAYBE_UNUSED = 0;
int jkPlayer_setDisableCutscenes MAYBE_UNUSED = 0;
int jkPlayer_aCutsceneVal[32] MAYBE_UNUSED = {0};
char jkPlayer_cutscenePath[1024] MAYBE_UNUSED = {0};
int jkPlayer_setNumCutscenes MAYBE_UNUSED = 0;
int jkPlayer_currentTickIdx MAYBE_UNUSED = 0;
int jkPlayer_setDiff MAYBE_UNUSED = 0;
rdVector3 jkPlayer_waggleVec MAYBE_UNUSED = {0};
flex_t jkPlayer_waggleMag MAYBE_UNUSED = 0.0f;
int jkPlayer_mpcInfoSet MAYBE_UNUSED = 0;
flex_t jkPlayer_waggleAngle MAYBE_UNUSED = 0.0f;
rdVector3 jkSaber_rotateVec MAYBE_UNUSED = {0};
char16_t jkPlayer_name[32] MAYBE_UNUSED = {0};
char jkPlayer_model[32] MAYBE_UNUSED = {0};
char jkPlayer_soundClass[32] MAYBE_UNUSED = {0};
char jkPlayer_sideMat[32] MAYBE_UNUSED = {0};
char jkPlayer_tipMat[32] MAYBE_UNUSED = {0};
int sithCollision_aNumSearchedSectors[4] MAYBE_UNUSED = {0};
SithCollideResult sithCollision_collisionHandlers[144] MAYBE_UNUSED = {0};
sithCollisionHitHandler_t sithCollision_aThingSurfaceCollideResults[12] MAYBE_UNUSED = {0};
sithCollisionSearchResult sithCollision_aCollisions[4] MAYBE_UNUSED = {0};
int sithCollision_aNumStackCollisions[4] MAYBE_UNUSED = {0};
int  sithCollision_searchStackIdx MAYBE_UNUSED =  -1;
sithCollisionSectorEntry sithCollision_apSearchedSectors[4] MAYBE_UNUSED = {0};
int sithCollision_dword_8B4BE4 MAYBE_UNUSED = 0;
rdVector3 sithCollision_collideHurtIdk MAYBE_UNUSED = {0};
rdVector3  sithSector_surfaceNormal MAYBE_UNUSED =  {0.0, 0.0, -1.0};
sithSectorEntry sithAIAwareness_aEntries[32] MAYBE_UNUSED = {0};
sithSectorAlloc* sithAIAwareness_g_aSectors MAYBE_UNUSED = NULL;
int sithAIAwareness_numEntries MAYBE_UNUSED = 0;
int sithAIAwareness_bInitted MAYBE_UNUSED = 0;
int sithAIAwareness_timerTicks MAYBE_UNUSED = 0;
flex_t sithSector_flt_8553B8 MAYBE_UNUSED = 0.0f;
flex_t sithSector_horizontalPixelsPerRev MAYBE_UNUSED = 0.0f;
flex_t sithSector_flt_8553C0 MAYBE_UNUSED = 0.0f;
flex_t sithSector_flt_8553C4 MAYBE_UNUSED = 0.0f;
flex_t sithSector_flt_8553C8 MAYBE_UNUSED = 0.0f;
rdVector3 sithSector_zMaxVec MAYBE_UNUSED = {0};
flex_t sithSector_ceilingSky MAYBE_UNUSED = 0.0f;
rdVector3 sithSector_zMinVec MAYBE_UNUSED = {0};
flex_t sithSector_horizontalPixelsPerRev_idk MAYBE_UNUSED = 0.0f;
flex_t sithSector_horizontalDist MAYBE_UNUSED = 0.0f;
flex_t sithSector_flt_8553F4 MAYBE_UNUSED = 0.0f;
SithSector* sithSector_aModifiedSectors[16] MAYBE_UNUSED = {0};
int sithSector_aSyncFlags[16] MAYBE_UNUSED = {0};
uint32_t sithSector_numModifiedSectors MAYBE_UNUSED = {0};
tHashTable* sithThing_pParseHashtbl MAYBE_UNUSED = NULL;
sithThing_handler_t sithThing_pfUnknownFunc MAYBE_UNUSED = {0};
uint16_t  sithThing_guidEntropy MAYBE_UNUSED =  1;
char16_t jkGuiSaveLoad_wtextEpisode[256] MAYBE_UNUSED = {0};
char16_t jkGuiSaveLoad_wtextHealth[64] MAYBE_UNUSED = {0};
int jkGuiSaveLoad_numEntries MAYBE_UNUSED = 0;
char16_t jkGuiSaveLoad_wtextShields[64] MAYBE_UNUSED = {0};
char16_t jkGuiSaveLoad_word_559830[256] MAYBE_UNUSED = {0};
int jkGuiSaveLoad_bIsSaveMenu MAYBE_UNUSED = 0;
char16_t jkGuiSaveLoad_wtextSaveName[256] MAYBE_UNUSED = {0};
Darray jkGuiSaveLoad_DarrayEntries MAYBE_UNUSED = {0};
char16_t jkGuiSaveLoad_word_559C54[10] MAYBE_UNUSED = {0};
char jkGuiSaveLoad_byte_559C50[4] MAYBE_UNUSED = {0};
int  jkGuiTitle_verMajor MAYBE_UNUSED =  1;
int  jkGuiTitle_verMinor MAYBE_UNUSED =  0;
int  jkGuiTitle_verRevision MAYBE_UNUSED =  0;
jkGuiStringEntry jkGuiTitle_aTexts[20] MAYBE_UNUSED = {0};
int jkGuiTitle_whichLoading MAYBE_UNUSED = 0;
flex_t  jkGuiSound_sfxVolume MAYBE_UNUSED =  0.8;
uint32_t  jkGuiSound_numChannels MAYBE_UNUSED =  16;
int jkGuiSound_bLowResSound MAYBE_UNUSED = 0;
int  jkGuiSound_b3DSound MAYBE_UNUSED =  1;
int  jkGuiSound_b3DSound_2 MAYBE_UNUSED =  1;
int  jkGuiSound_b3DSound_3 MAYBE_UNUSED =  1;
flex_t jkGuiSound_musicVolume MAYBE_UNUSED = 0.0f;
char jkGui_unkstr[32] MAYBE_UNUSED = {0};
int jkGui_GdiMode MAYBE_UNUSED = 0;
int jkGui_modesets MAYBE_UNUSED = 0;
stdBitmap* jkGui_stdBitmaps[0x27] MAYBE_UNUSED = {0};
stdFont* jkGui_stdFonts[16] MAYBE_UNUSED = {0};
jkEpisodeLoad jkGui_episodeLoad MAYBE_UNUSED = {0};
stdBitmap* jkGuiSingleTally_foStars MAYBE_UNUSED = NULL;
int jkGuiNetHost_maxRank MAYBE_UNUSED = 0;
int jkGuiNetHost_timeLimit MAYBE_UNUSED = 0;
int jkGuiNetHost_scoreLimit MAYBE_UNUSED = 0;
int jkGuiNetHost_maxPlayers MAYBE_UNUSED = 0;
int jkGuiNetHost_sessionFlags MAYBE_UNUSED = 0;
int jkGuiNetHost_gameFlags MAYBE_UNUSED = 0;
int jkGuiNetHost_tickRate MAYBE_UNUSED = 0;
char16_t jkGuiNetHost_gameName[32] MAYBE_UNUSED = {0};
int jkGuiMultiplayer_checksumSeed MAYBE_UNUSED = 0;
int jkGuiMultiplayer_dword_5564EC MAYBE_UNUSED = 0;
int jkGuiMultiplayer_dword_5564E8 MAYBE_UNUSED = 0;
jkMultiEntry jkGuiMultiplayer_multiEntry MAYBE_UNUSED = {0};
int jkGuiMultiplayer_dword_5564F0 MAYBE_UNUSED = 0;
HINSTANCE g_hInstance MAYBE_UNUSED = {0};
SithCogSymbolTable* sithCog_g_pSymbolTable MAYBE_UNUSED = NULL;
struct HostServices* pSithHS MAYBE_UNUSED = NULL;
HWND g_hWnd MAYBE_UNUSED = {0};
uint32_t g_nShowCmd MAYBE_UNUSED = {0};
uint32_t g_app_suspended MAYBE_UNUSED = {0};
uint32_t g_window_active MAYBE_UNUSED = {0};
uint32_t g_app_active MAYBE_UNUSED = {0};
uint32_t g_should_exit MAYBE_UNUSED = {0};
uint32_t g_thing_two_some_dialog_count MAYBE_UNUSED = {0};
uint32_t g_handler_count MAYBE_UNUSED = {0};
uint32_t g_855E8C MAYBE_UNUSED = {0};
uint32_t g_855E90 MAYBE_UNUSED = {0};
uint32_t g_window_not_destroyed MAYBE_UNUSED = {0};
stdPalEffectsState stdPalEffects_state MAYBE_UNUSED = {0};
rdColor24 stdPalEffects_palette[256] MAYBE_UNUSED = {0};
uint32_t stdPalEffects_numEffectRequests MAYBE_UNUSED = {0};
stdPalEffectRequest stdPalEffects_aEffects[32] MAYBE_UNUSED = {0};
stdPalEffectSetPaletteFunc_t stdPalEffects_setPalette MAYBE_UNUSED = {0};
uint16_t stdPalEffects_aPalette[256] MAYBE_UNUSED = {0};
char aFilenameStack[2560] MAYBE_UNUSED = {0};
char* apBufferStack[20] MAYBE_UNUSED = {0};
int linenumStack[20] MAYBE_UNUSED = {0};
char aEntryStack[0x14*(STDCONF_LINEBUFFER_LEN+4)] MAYBE_UNUSED = {0};
stdFile_t openFileStack[20] MAYBE_UNUSED = {0};
char printfBuffer[STDCONF_LINEBUFFER_LEN] MAYBE_UNUSED = {0};
int stdConffile_linenum MAYBE_UNUSED = 0;
int stdConffile_bOpen MAYBE_UNUSED = 0;
stdFile_t openFile MAYBE_UNUSED = {0};
stdFile_t writeFile MAYBE_UNUSED = {0};
uint32_t stackLevel MAYBE_UNUSED = {0};
char stdConffile_aWriteFilename[128] MAYBE_UNUSED = {0};
StdConffileEntry stdConffile_g_entry MAYBE_UNUSED = {0};
char stdConffile_pFilename[128] MAYBE_UNUSED = {0};
char* stdConffile_g_aLine MAYBE_UNUSED = NULL;
int stdMemory_bInitted MAYBE_UNUSED = 0;
int stdMemory_bOpened MAYBE_UNUSED = 0;
tMemoryState stdMemory_g_curState MAYBE_UNUSED = {0};
stdFile_t yyin MAYBE_UNUSED = {0};
stdFile_t yyout MAYBE_UNUSED = {0};
SithCogSymbolTable* sithCogParse_pSymbolTable MAYBE_UNUSED = NULL;
int yacc_linenum MAYBE_UNUSED = 0;
int  cog_yacc_loop_depth MAYBE_UNUSED =  1;
int cog_parser_node_stackpos[SITHCOG_NODE_STACKDEPTH] MAYBE_UNUSED = {0};
int cogvm_stackpos MAYBE_UNUSED = 0;
sith_cog_parser_node* cogparser_nodes_alloc MAYBE_UNUSED = NULL;
sith_cog_parser_node* cogparser_topnode MAYBE_UNUSED = NULL;
int32_t* cogvm_stack MAYBE_UNUSED = NULL;
int cogparser_num_nodes MAYBE_UNUSED = 0;
int cogparser_current_nodeidx MAYBE_UNUSED = 0;
SithCogScript* parsing_script MAYBE_UNUSED = NULL;
int  parsing_script_idk MAYBE_UNUSED =  1;
int dplay_dword_55D618 MAYBE_UNUSED = 0;
int dplay_dword_55D61C MAYBE_UNUSED = 0;
GUID_idk jkGui_guid_556040 MAYBE_UNUSED = {0};
int jkGuiMultiplayer_numConnections MAYBE_UNUSED = 0;
sith_dplay_connection jkGuiMultiplayer_aConnections[32] MAYBE_UNUSED = {0};
jkMultiEntry jkGuiMultiplayer_aEntries[32] MAYBE_UNUSED = {0};
jkMultiEntry2 jkGuiMultiplayer_stru_556168 MAYBE_UNUSED = {0};
jkPlayerMpcInfo jkGuiMultiplayer_mpcInfo MAYBE_UNUSED = {0};
Darray jkGuiMultiplayer_stru_5564A8 MAYBE_UNUSED = {0};
int jkGuiMouse_bOpen MAYBE_UNUSED = 0;
Darray jkGuiMouse_Darray_556698 MAYBE_UNUSED = {0};
int jkGuiMouse_dword_5566B0 MAYBE_UNUSED = 0;
Darray jkGuiMouse_Darray_5566B8 MAYBE_UNUSED = {0};
Darray jkGuiMouse_Darray_5566D0 MAYBE_UNUSED = {0};
char16_t* jkGuiMouse_pWStr_5566E8 MAYBE_UNUSED = NULL;
int jkGuiEsc_bInitialized MAYBE_UNUSED = 0;
int jkGuiKeyboard_dword_555DE0 MAYBE_UNUSED = 0;
int jkGuiKeyboard_bOnceIdk MAYBE_UNUSED = 0;
int jkGuiKeyboard_funcIdx MAYBE_UNUSED = 0;
int jkGuiKeyboard_flags MAYBE_UNUSED = 0;
Darray jkGuiKeyboard_darrEntries MAYBE_UNUSED = {0};
int jkGuiKeyboard_dword_555E10 MAYBE_UNUSED = 0;
char16_t jkGuiKeyboard_wstr_555E18[257] MAYBE_UNUSED = {0};
char16_t* jkGuiKeyboard_pWStr_55601C MAYBE_UNUSED = NULL;
rdVector3 jkGuiMap_vec3Idk2 MAYBE_UNUSED = {0};
rdCanvas* jkGuiMap_pCanvas MAYBE_UNUSED = NULL;
rdMatrix34 jkGuiMap_viewMat MAYBE_UNUSED = {0};
rdMatrix34 jkGuiMap_matTmp MAYBE_UNUSED = {0};
tVBuffer* jkGuiMap_pVbuffer MAYBE_UNUSED = NULL;
sithMap jkGuiMap_unk4 MAYBE_UNUSED = {0};
rdVector3 jkGuiMap_vec3Idk MAYBE_UNUSED = {0};
rdCamera* jkGuiMap_pCamera MAYBE_UNUSED = NULL;
int jkGuiMap_dword_556660 MAYBE_UNUSED = 0;
int jkGuiMap_bOrbitActive MAYBE_UNUSED = 0;
int jkGuiMap_dword_556668 MAYBE_UNUSED = 0;
int jkGuiMap_dword_55666C MAYBE_UNUSED = 0;
flex_t sithMap_unkArr[12]  MAYBE_UNUSED =  {0.5, 1.0, 1.5, 2.0, 2.5, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 0.0};
SithThing* sithMap_pPlayerThing MAYBE_UNUSED = NULL;
rdMatrix34 sithMap_invMatrix MAYBE_UNUSED = {0};
flex_t sithMap_flt_84DEA8 MAYBE_UNUSED = 0.0f;
flex_t sithMap_flt_84DEAC MAYBE_UNUSED = 0.0f;
sithMap sithMap_ctx MAYBE_UNUSED = {0};
SithWorld* sithMap_pCurWorld MAYBE_UNUSED = NULL;
rdMatrix34 sithMap_camera MAYBE_UNUSED = {0};
rdCamera* sithMap_pCurCamera MAYBE_UNUSED = NULL;
int sithMap_bInitted MAYBE_UNUSED = 0;
int sithMap_var MAYBE_UNUSED = 0;
uint32_t DirectPlay_numPlayers MAYBE_UNUSED = {0};
sithDplayPlayer DirectPlay_aPlayers[32] MAYBE_UNUSED = {0};
#endif

void OpenJKDF2_Globals_Reset()
{
// Vars
sithAIClass_g_pHashtable = NULL;
_memset_inline(&std_g_genBuffer, 0, sizeof(std_g_genBuffer));
std_g_pHS = NULL;
rdModel3_pCurGeoset = NULL;
_memset_inline(&localCamera, 0, sizeof(localCamera));
// NO_REINIT for aFaceVerts
_memset_inline(&vertexDst, 0, sizeof(vertexDst));
curGeometryMode = 0;
_memset_inline(&apGeoLights, 0, sizeof(apGeoLights));
// NO_REINIT for rdModel3_aLocalLightPos
// NO_REINIT for rdModel3_aLocalLightDir
meshFrustumCull = 0;
curTextureMode = 0;
// NO_REINIT for aView
pCurMesh = NULL;
thingFrustumCull = 0;
_memset_inline(&vertexSrc, 0, sizeof(vertexSrc));
pCurModel3 = NULL;
rdModel3_textureMode = 0;
curLightingMode = 0;
_memset_inline(&apMeshLights, 0, sizeof(apMeshLights));
pCurThing = NULL;
rdModel3_lightingMode = 0;
rdModel3_geometryMode = 0;
rdModel3_numDrawnModels = 0;
_memset_inline(&pModel3Loader, 0, sizeof(pModel3Loader));
_memset_inline(&pModel3Unloader, 0, sizeof(pModel3Unloader));
rdModel3_numGeoLights = 0;
rdModel3_numMeshLights = 0;
rdModel3_fRadius = 0.0f;
sithPuppet_pClassHashtable = NULL;
sithPuppet_pKeyHashtable = NULL;
sithPuppet_pHashtblSubmodes = NULL;
_memset_inline(&pKeyframeLoader, 0, sizeof(pKeyframeLoader));
_memset_inline(&pKeyframeUnloader, 0, sizeof(pKeyframeUnloader));
sithTime_g_frameTime = 0;
sithTime_g_frameTimeFlex = 0.0f;
_memset_inline(&sithTime_g_fps, 0, sizeof(sithTime_g_fps));
sithTime_g_msecGameTime = 0;
sithTime_g_secGameTime = 0.0f;
sithTime_g_clockTime = 0;
sithTime_msecPauseStartTime = 0;
sithTime_g_bPaused = 0;
sithRender_texMode = 0;
sithRender_renderflags = 0;
sithRender_geoMode = 0;
sithRender_lightMode = 0;
sithRender_lightingIRMode = 0;
sithRender_f_83198C = 0.0f;
sithRender_f_831990 = 0.0f;
sithRender_bResetCameraAspect = 0;
sithRender_g_numVisibleSectors = 0;
sithRender_numSecorFrustrums = 0;
sithRender_numThingLights = 0;
sithRender_numThingSectors = 0;
sithRender_numSpritesToDraw = 0;
sithRender_numRenderedSectors = 0;
sithRender_numAlphaAdjoins = 0;
sithRender_geoThingsDrawn = 0;
sithRender_nongeoThingsDrawn = 0;
flex_t  __sithRender_f_82F4B0_origValue =  0.0;
_memcpy(&sithRender_f_82F4B0, &__sithRender_f_82F4B0_origValue, sizeof(sithRender_f_82F4B0));
_memset_inline(&sithRender_faceView, 0, sizeof(sithRender_faceView));
_memset_inline(&meshinfo_out, 0, sizeof(meshinfo_out));
_memset_inline(&sithRender_pExtraThingRenderFunc, 0, sizeof(sithRender_pExtraThingRenderFunc));
_memset_inline(&sithRender_aThingLights, 0, sizeof(sithRender_aThingLights));
_memset_inline(&sithRender_aVisibleSectors, 0, sizeof(sithRender_aVisibleSectors));
_memset_inline(&sithRender_aSectorFrustrums, 0, sizeof(sithRender_aSectorFrustrums));
_memset_inline(&sithRender_aThingSectors, 0, sizeof(sithRender_aThingSectors));
// NO_REINIT for sithRender_aClipVertices
// NO_REINIT for sithRender_aTransformedClipVertices
_memset_inline(&sithRender_aAlphaAdjoins, 0, sizeof(sithRender_aAlphaAdjoins));
sithRender_lastRenderTick = 0;
_memset_inline(&aSithSurfaces, 0, sizeof(aSithSurfaces));
_memset_inline(&sithCamera_g_aCameras, 0, sizeof(sithCamera_g_aCameras));
sithCamera_g_bCurCameraSet = 0;
sithCamera_g_stateFlags = 0;
sithCamera_g_curCycleCamNum = 0;
_memset_inline(&sithCamera_g_vecCameraPosOffset, 0, sizeof(sithCamera_g_vecCameraPosOffset));
_memset_inline(&sithCamera_g_vecCameraAngleOffset, 0, sizeof(sithCamera_g_vecCameraAngleOffset));
sithCamera_g_cameraPosDelta = 0.0f;
sithCamera_g_cameraAngleDelta = 0.0f;
sithCamera_g_pCurCamera = NULL;
sithCamera_bStartup = 0;
_memset_inline(&sithCamera_g_orbCamOrient, 0, sizeof(sithCamera_g_orbCamOrient));
_memset_inline(&sithCamera_idleCamOrient, 0, sizeof(sithCamera_idleCamOrient));
sithCamera_bOpen = 0;
rdVector4  __rdroid_aMipDistances_origValue =  {10.0, 20.0, 40.0, 80.0};
_memcpy(&rdroid_aMipDistances, &__rdroid_aMipDistances_origValue, sizeof(rdroid_aMipDistances));
rdroid_frameTrue = 0;
bRDroidStartup = 0;
bRDroidOpen = 0;
rdroid_g_curLightingMode = 0;
rdroid_g_pHS = NULL;
rdroid_g_curGeometryMode = 0;
_memset_inline(&rdroid_curColorEffects, 0, sizeof(rdroid_curColorEffects));
rdroid_curOcclusionMethod = 0;
_memset_inline(&rdroid_curZBufferMethod, 0, sizeof(rdroid_curZBufferMethod));
rdroid_curProcFaceUserData = 0;
rdroid_curSortingMethod = 0;
rdroid_curAcceleration = 0;
rdroid_curTextureMode = 0;
rdroid_g_curRenderOptions = 0;
rdroid_curCullFlags = 0;
sithMaterial_pHashtable = NULL;
sithMaterial_aMaterials = NULL;
sithMaterial_numMaterials = 0;
// NO_REINIT for rdCache_aHWSolidTris
rdCache_totalNormalTris = 0;
// NO_REINIT for rdCache_aIntensities
// NO_REINIT for rdCache_aVertices
rdCache_totalVerts = 0;
// NO_REINIT for rdCache_aTexVertices
// NO_REINIT for rdCache_aHWNormalTris
rdCache_totalSolidTris = 0;
// NO_REINIT for rdCache_aHWVertices
rdCache_drawnFaces = 0;
rdCache_numUsedVertices = 0;
rdCache_numUsedTexVertices = 0;
rdCache_numUsedIntensities = 0;
_memset_inline(&rdCache_ulcExtent, 0, sizeof(rdCache_ulcExtent));
_memset_inline(&rdCache_lrcExtent, 0, sizeof(rdCache_lrcExtent));
rdCache_numProcFaces = 0;
// NO_REINIT for rdCache_aProcFaces
rdCache_dword_865258 = 0;
_memset_inline(&sithMulti_name, 0, sizeof(sithMulti_name));
_memset_inline(&sithMulti_pfNewPlayerJoinedCallback, 0, sizeof(sithMulti_pfNewPlayerJoinedCallback));
sithMulti_multiModeFlags = 0;
sithMulti_numRemovedStaticThings = 0;
_memset_inline(&sithMulti_aRemovedStaticThings, 0, sizeof(sithMulti_aRemovedStaticThings));
sithMulti_msecQuitGameTime = 0;
sithMulti_msecPingStartTime = 0;
sithMulti_msecLastSyncScoreTime = 0;
sithMulti_curWelcomePlayerNum = 0;
sithMulti_msecWelcomeUpdateInterval = 0;
rdColormap_pCurMap = NULL;
rdColormap_pIdentityMap = NULL;
_memset_inline(&sithEvent_aEvents, 0, sizeof(sithEvent_aEvents));
sithEvent_g_pFirstQueuedEvent = NULL;
_memset_inline(&sithEvent_arrLut, 0, sizeof(sithEvent_arrLut));
_memset_inline(&sithEvent_aTasks, 0, sizeof(sithEvent_aTasks));
sithEvent_numFreeEventBuffers = 0;
sithEvent_bInit = 0;
sithEvent_bOpen = 0;
rdClip_g_faceStatus = 0;
rdClip_pSourceVert = NULL;
// NO_REINIT for rdClip_workIVerts
// NO_REINIT for rdClip_aWorkVerts
rdClip_pDestVert = NULL;
rdClip_pDestIVert = NULL;
// NO_REINIT for rdClip_aWorkTVerts
rdClip_pSourceIVert = NULL;
rdClip_pSourceTVert = NULL;
rdClip_pDestTVert = NULL;
sithSoundMixer_dword_835FCC = 0;
sithSoundMixer_bPlayingMci = 0;
sithSoundMixer_bInitted = 0;
sithSoundMixer_flt_835FD8 = 0.0f;
sithSoundMixer_bIsMuted = 0;
flex_t  __sithSoundMixer_musicVolume_origValue =  1.0f;
_memcpy(&sithSoundMixer_musicVolume, &__sithSoundMixer_musicVolume_origValue, sizeof(sithSoundMixer_musicVolume));
flex_t  __sithSoundMixer_globalVolume_origValue =  1.0f;
_memcpy(&sithSoundMixer_globalVolume, &__sithSoundMixer_globalVolume_origValue, sizeof(sithSoundMixer_globalVolume));
sithSoundMixer_numSoundsAvailable2 = 0;
sithSoundMixer_numSoundsAvailable = 0;
_memset_inline(&sithSoundMixer_aPlayingSounds, 0, sizeof(sithSoundMixer_aPlayingSounds));
_memset_inline(&sithSoundMixer_aIdk, 0, sizeof(sithSoundMixer_aIdk));
sithSoundMixer_activeChannels = 0;
sithSoundMixer_bOpened = 0;
sithSoundMixer_pCurSector = NULL;
sithSoundMixer_dword_836BFC = 0;
sithSoundMixer_trackFrom = 0;
sithSoundMixer_trackTo = 0;
sithSoundMixer_hCurAmbientChannel = NULL;
sithSoundMixer_dword_836C00 = 0;
int  __sithSoundMixer_channelGUIDSeed_origValue =  1;
_memcpy(&sithSoundMixer_channelGUIDSeed, &__sithSoundMixer_channelGUIDSeed_origValue, sizeof(sithSoundMixer_channelGUIDSeed));
sithSoundMixer_dword_836C04 = 0;
sithSoundMixer_pFocusedThing = NULL;
int  __sithSound_maxDataLoaded_origValue =  0x400000;
_memcpy(&sithSound_maxDataLoaded, &__sithSound_maxDataLoaded_origValue, sizeof(sithSound_maxDataLoaded));
int  __sithSound_var3_origValue =  1;
_memcpy(&sithSound_var3, &__sithSound_var3_origValue, sizeof(sithSound_var3));
sithSound_curDataLoaded = 0;
sithSound_bInit = 0;
sithSound_hashtable = NULL;
sithSound_var4 = 0;
sithSound_var5 = 0;
_memset_inline(&sithControl_inputFuncToControlType, 0, sizeof(sithControl_inputFuncToControlType));
_memset_inline(&sithControl_aInputFuncToKeyinfo, 0, sizeof(sithControl_aInputFuncToKeyinfo));
sithControl_msIdle = 0;
sithControl_bInitted = 0;
sithControl_bOpened = 0;
_memset_inline(&sithControl_aHandlers, 0, sizeof(sithControl_aHandlers));
sithControl_numHandlers = 0;
sithControl_death_msgtimer = 0;
rdVector3  __sithControl_vec3_54A570_origValue =  {0.0, -1.0, 0.0};
_memcpy(&sithControl_vec3_54A570, &__sithControl_vec3_54A570_origValue, sizeof(sithControl_vec3_54A570));
flex_t  __sithControl_flt_54A57C_origValue =  0.2f;
_memcpy(&sithControl_flt_54A57C, &__sithControl_flt_54A57C_origValue, sizeof(sithControl_flt_54A57C));
_memset_inline(&sithGamesave_func1, 0, sizeof(sithGamesave_func1));
_memset_inline(&sithGamesave_func2, 0, sizeof(sithGamesave_func2));
_memset_inline(&sithGamesave_func3, 0, sizeof(sithGamesave_func3));
_memset_inline(&sithGamesave_funcWrite, 0, sizeof(sithGamesave_funcWrite));
_memset_inline(&sithGamesave_funcRead, 0, sizeof(sithGamesave_funcRead));
_memset_inline(&sithGamesave_autosave_fname, 0, sizeof(sithGamesave_autosave_fname));
_memset_inline(&sithGamesave_state, 0, sizeof(sithGamesave_state));
sithGamesave_dword_835914 = 0;
_memset_inline(&sithGamesave_aCurFilename, 0, sizeof(sithGamesave_aCurFilename));
_memset_inline(&sithGamesave_wsaveName, 0, sizeof(sithGamesave_wsaveName));
_memset_inline(&sithGamesave_saveName, 0, sizeof(sithGamesave_saveName));
_memset_inline(&sithGamesave_headerTmp, 0, sizeof(sithGamesave_headerTmp));
rdCamera_g_pCurCamera = NULL;
_memset_inline(&rdCamera_g_camMatrix, 0, sizeof(rdCamera_g_camMatrix));
_memset_inline(&sithNet_MultiModeFlags, 0, sizeof(sithNet_MultiModeFlags));
_memset_inline(&sithNet_scorelimit, 0, sizeof(sithNet_scorelimit));
_memset_inline(&sithNet_teamScore, 0, sizeof(sithNet_teamScore));
_memset_inline(&sithNet_multiplayer_timelimit, 0, sizeof(sithNet_multiplayer_timelimit));
_memset_inline(&sithMulti_multiplayerTimelimit, 0, sizeof(sithMulti_multiplayerTimelimit));
_memset_inline(&sithNet_isMulti, 0, sizeof(sithNet_isMulti));
_memset_inline(&sithNet_isServer, 0, sizeof(sithNet_isServer));
_memset_inline(&sithMulti_bTimelimitMet, 0, sizeof(sithMulti_bTimelimitMet));
_memset_inline(&sithNet_serverNetId, 0, sizeof(sithNet_serverNetId));
_memset_inline(&sithNet_things, 0, sizeof(sithNet_things));
_memset_inline(&sithNet_thingsIdx, 0, sizeof(sithNet_thingsIdx));
_memset_inline(&sithNet_syncIdx, 0, sizeof(sithNet_syncIdx));
_memset_inline(&sithNet_aSyncFlags, 0, sizeof(sithNet_aSyncFlags));
_memset_inline(&sithNet_aSyncThings, 0, sizeof(sithNet_aSyncThings));
int32_t   __sithNet_tickrate_origValue =  180;
_memcpy(&sithNet_tickrate, &__sithNet_tickrate_origValue, sizeof(sithNet_tickrate));
_memset_inline(&sithNet_dword_8C4BA8, 0, sizeof(sithNet_dword_8C4BA8));
_memset_inline(&sithNet_dword_83262C, 0, sizeof(sithNet_dword_83262C));
_memset_inline(&sithMulti_quitGameState, 0, sizeof(sithMulti_quitGameState));
_memset_inline(&sithNet_checksum, 0, sizeof(sithNet_checksum));
_memset_inline(&sithNet_bNeedsFullThingSyncForLeaveJoin, 0, sizeof(sithNet_bNeedsFullThingSyncForLeaveJoin));
_memset_inline(&sithMulti_newPlayerId, 0, sizeof(sithMulti_newPlayerId));
_memset_inline(&sithNet_bSyncScores, 0, sizeof(sithNet_bSyncScores));
_memset_inline(&sithNet_dword_832620, 0, sizeof(sithNet_dword_832620));
flex_t  __sithOverlayMap_curScale_origValue =  100.0;
_memcpy(&sithOverlayMap_curScale, &__sithOverlayMap_curScale_origValue, sizeof(sithOverlayMap_curScale));
_memset_inline(&sithOverlayMap_mapOrient, 0, sizeof(sithOverlayMap_mapOrient));
sithOverlayMap_pLocalPlayer = NULL;
sithOverlayMap_pCanvas = NULL;
sithOverlayMap_x1 = 0;
sithOverlayMap_y1 = 0;
_memset_inline(&sithOverlayMap_inst, 0, sizeof(sithOverlayMap_inst));
sithOverlayMap_bMapVisible = 0;
sithOverlayMap_bOpened = 0;
sithSoundClass_pHashtblModes = NULL;
sithSoundClass_pHashTable = NULL;
sithTemplate_pMasterFile = NULL;
sithTemplate_pHashtable = NULL;
sithTemplate_masterFileCount = 0;
sithTemplate_pMasterHashtable = NULL;
// NO_REINIT for activeEdgeTail
// NO_REINIT for activeEdgeHead
// NO_REINIT for apNewActiveEdges
// NO_REINIT for apRemoveActiveEdges
yMinEdge = 0;
yMaxEdge = 0;
numActiveSpans = 0;
numActiveFaces = 0;
numActiveEdges = 0;
// NO_REINIT for aActiveEdges
rdActive_drawnFaces = 0;
sithMain_bEndLevel = 0;
sith_bStartup = 0;
sith_bOpen = 0;
sithSurface_numAvail = 0;
_memset_inline(&sithSurface_aAvail, 0, sizeof(sithSurface_aAvail));
sithSurface_numSurfaces = 0;
_memset_inline(&sithSurface_aSurfaces, 0, sizeof(sithSurface_aSurfaces));
sithSurface_bOpened = 0;
uint32_t  __sithSurface_byte_8EE668_origValue =  0;
_memcpy(&sithSurface_byte_8EE668, &__sithSurface_byte_8EE668_origValue, sizeof(sithSurface_byte_8EE668));
sithSurface_numUnsyncedSurfaces = 0;
_memset_inline(&pMaterialsLoader, 0, sizeof(pMaterialsLoader));
_memset_inline(&pMaterialsUnloader, 0, sizeof(pMaterialsUnloader));
sithSprite_pHashtable = NULL;
_memset_inline(&Window_drawAndFlip, 0, sizeof(Window_drawAndFlip));
_memset_inline(&Window_setCooperativeLevel, 0, sizeof(Window_setCooperativeLevel));
_memset_inline(&Window_ext_handlers, 0, sizeof(Window_ext_handlers));
_memset_inline(&Window_aDialogHwnds, 0, sizeof(Window_aDialogHwnds));
_memset_inline(&DebugGui_aIdk, 0, sizeof(DebugGui_aIdk));
DebugGui_idk = 0;
DebugGui_some_line_amt = 0;
DebugGui_some_num_lines = 0;
_memset_inline(&DebugLog_buffer, 0, sizeof(DebugLog_buffer));
sithConsole_aCmds = NULL;
sithConsole_pCmdHashtable = NULL;
sithConsole_bOpened = 0;
sithConsole_bInitted = 0;
sithConsole_maxCmds = 0;
sithConsole_numRegisteredCmds = 0;
DebugGui_maxLines = 0;
_memset_inline(&DebugGui_fnPrint, 0, sizeof(DebugGui_fnPrint));
_memset_inline(&DebugGui_fnPrintUniStr, 0, sizeof(DebugGui_fnPrintUniStr));
sithConsole_alertSound = NULL;
sithConsole_idk2 = 0;
d3d_maxVertices = 0;
d3d_device_ptr = NULL;
_memset_inline(&std3D_rectViewIdk, 0, sizeof(std3D_rectViewIdk));
_memset_inline(&std3D_aViewIdk, 0, sizeof(std3D_aViewIdk));
_memset_inline(&std3D_aViewTris, 0, sizeof(std3D_aViewTris));
int32_t  __std3D_gpuMaxTexSizeMaybe_origValue =  1;
_memcpy(&std3D_gpuMaxTexSizeMaybe, &__std3D_gpuMaxTexSizeMaybe_origValue, sizeof(std3D_gpuMaxTexSizeMaybe));
int32_t  __std3D_dword_53D66C_origValue =  1;
_memcpy(&std3D_dword_53D66C, &__std3D_dword_53D66C_origValue, sizeof(std3D_dword_53D66C));
int32_t  __std3D_dword_53D670_origValue =  0x100;
_memcpy(&std3D_dword_53D670, &__std3D_dword_53D670_origValue, sizeof(std3D_dword_53D670));
int32_t  __std3D_dword_53D674_origValue =  0x100;
_memcpy(&std3D_dword_53D674, &__std3D_dword_53D674_origValue, sizeof(std3D_dword_53D674));
int32_t  __std3D_frameCount_origValue =  1;
_memcpy(&std3D_frameCount, &__std3D_frameCount_origValue, sizeof(std3D_frameCount));
_memset_inline(&std3D_renderList, 0, sizeof(std3D_renderList));
std3D_numCachedTextures = 0;
std3D_pFirstTexCache = NULL;
std3D_pLastTexCache = NULL;
stdComm_dword_8321F8 = 0;
stdComm_bInitted = 0;
stdComm_dword_8321F0 = 0;
stdComm_dword_8321F4 = 0;
stdComm_dplayIdSelf = 0;
stdComm_dword_832204 = 0;
stdComm_dword_832208 = 0;
stdComm_currentBigSyncStage = 0;
stdComm_dword_8321E0 = 0;
stdComm_bIsServer = 0;
stdComm_dword_8321E8 = 0;
_memset_inline(&stdComm_waIdk, 0, sizeof(stdComm_waIdk));
stdComm_dword_8321DC = 0;
stdComm_dword_832200 = 0;
stdComm_dword_832210 = 0;
_memset_inline(&stdComm_cogMsgTmp, 0, sizeof(stdComm_cogMsgTmp));
_memset_inline(&stdMci_mciId, 0, sizeof(stdMci_mciId));
stdMci_dwVolume = 0;
stdMci_bInitted = 0;
stdMci_uDeviceID = 0;
wuRegistry_bStarted = 0;
_memset_inline(&wuRegistry_lpClass, 0, sizeof(wuRegistry_lpClass));
_memset_inline(&wuRegistry_byte_855EB4, 0, sizeof(wuRegistry_byte_855EB4));
_memset_inline(&wuRegistry_hKey, 0, sizeof(wuRegistry_hKey));
_memset_inline(&wuRegistry_lpSubKey, 0, sizeof(wuRegistry_lpSubKey));
_memset_inline(&WinIdk_aDplayGuid, 0, sizeof(WinIdk_aDplayGuid));
_memset_inline(&stdDisplay_aDisplayDevices, 0, sizeof(stdDisplay_aDisplayDevices));
stdDisplay_gammaTableLen = 0;
stdDisplay_paGammaTable = NULL;
_memset_inline(&stdDisplay_gammaPalette, 0, sizeof(stdDisplay_gammaPalette));
stdDisplay_pCurDevice = NULL;
stdDisplay_pCurVideoMode = NULL;
stdDisplay_bStartup = 0;
stdDisplay_bOpen = 0;
stdDisplay_bModeSet = 0;
stdDisplay_numVideoModes = 0;
stdDisplay_bPaged = 0;
_memset_inline(&stdDisplay_tmpGammaPal, 0, sizeof(stdDisplay_tmpGammaPal));
stdDisplay_gammaIdx = 0;
word_860800 = 0;
word_860802 = 0;
word_860804 = 0;
word_860806 = 0;
int  __stdControl_bReadMouse_origValue =  1;
_memcpy(&stdControl_bReadMouse, &__stdControl_bReadMouse_origValue, sizeof(stdControl_bReadMouse));
flex_t  __stdControl_updateKHz_origValue =  1.0f;
_memcpy(&stdControl_updateKHz, &__stdControl_updateKHz_origValue, sizeof(stdControl_updateKHz));
flex_t  __stdControl_updateHz_origValue =  1.0f;
_memcpy(&stdControl_updateHz, &__stdControl_updateHz_origValue, sizeof(stdControl_updateHz));
flex_t  __stdControl_mouseXSensitivity_origValue =  1.0f;
_memcpy(&stdControl_mouseXSensitivity, &__stdControl_mouseXSensitivity_origValue, sizeof(stdControl_mouseXSensitivity));
flex_t  __stdControl_mouseYSensitivity_origValue =  1.0f;
_memcpy(&stdControl_mouseYSensitivity, &__stdControl_mouseYSensitivity_origValue, sizeof(stdControl_mouseYSensitivity));
flex_t  __stdControl_mouseZSensitivity_origValue =  1.0f;
_memcpy(&stdControl_mouseZSensitivity, &__stdControl_mouseZSensitivity_origValue, sizeof(stdControl_mouseZSensitivity));
_memset_inline(&stdControl_aAxisEnabled, 0, sizeof(stdControl_aAxisEnabled));
_memset_inline(&stdControl_aAxisStates, 0, sizeof(stdControl_aAxisStates));
_memset_inline(&stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
_memset_inline(&stdControl_aKeyInfo, 0, sizeof(stdControl_aKeyInfo));
_memset_inline(&stdControl_aAxisConnected, 0, sizeof(stdControl_aAxisConnected));
_memset_inline(&stdControl_aKeyPressed, 0, sizeof(stdControl_aKeyPressed));
stdControl_bStartup = 0;
stdControl_bOpen = 0;
stdControl_bDisableKeyboard = 0;
stdControl_bControlsIdle = 0;
stdControl_bControlsActive = 0;
stdControl_pDI = NULL;
stdControl_mouseDirectInputDevice = NULL;
stdControl_keyboardIDirectInputDevice = NULL;
stdControl_bReadJoysticks = 0;
stdControl_curReadTime = 0;
stdControl_lastReadTime = 0;
stdControl_readDeltaTime = 0;
stdControl_dwLastMouseX = 0;
stdControl_dwLastMouseY = 0;
_memset_inline(&stdControl_aJoystickEnabled, 0, sizeof(stdControl_aJoystickEnabled));
_memset_inline(&stdControl_aJoystickExists, 0, sizeof(stdControl_aJoystickExists));
_memset_inline(&stdControl_aAxes, 0, sizeof(stdControl_aAxes));
_memset_inline(&stdControl_aJoystickMaxButtons, 0, sizeof(stdControl_aJoystickMaxButtons));
Windows_installType = 0;
uint8_t __Video_aGammaTable_origValue[80]  =  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0x17, 0x5D, 0x74, 0xD1, 0x45, 0x17, 0xED, 0x3F, 0xAB, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xEA, 0x3F, 0xB7, 0x6D, 0xDB, 0xB6, 0x6D, 0xDB, 0xE6, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE4, 0x3F, 0x72, 0x1C, 0xC7, 0x71, 0x1C, 0xC7, 0xE1, 0x3F, 0x79, 0x0D, 0xE5, 0x35, 0x94, 0xD7, 0xE0, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, 0x9E, 0xE7, 0x79, 0x9E, 0xE7, 0x79, 0xDE, 0x3F, 0xBE, 0xE9, 0x4D, 0x6F, 0x7A, 0xD3, 0xDB, 0x3F};
_memcpy(&Video_aGammaTable, &__Video_aGammaTable_origValue, sizeof(Video_aGammaTable));
int  __Video_fillColor_origValue =  0;
_memcpy(&Video_fillColor, &__Video_fillColor_origValue, sizeof(Video_fillColor));
_memset_inline(&Video_modeStruct, 0, sizeof(Video_modeStruct));
_memset_inline(&Video_otherBuf, 0, sizeof(Video_otherBuf));
Video_dword_866D78 = 0;
Video_curMode = 0;
_memset_inline(&Video_renderSurface, 0, sizeof(Video_renderSurface));
_memset_inline(&Video_menuBuffer, 0, sizeof(Video_menuBuffer));
Video_pOtherBuf = NULL;
Video_pMenuBuffer = NULL;
Video_bInitted = 0;
Video_bOpened = 0;
Video_flt_55289C = 0.0f;
Video_dword_5528A0 = 0;
Video_dword_5528A4 = 0;
Video_dword_5528A8 = 0;
Video_lastTimeMsec = 0;
Video_dword_5528B0 = 0;
Video_pVbufIdk = NULL;
Video_pCanvas = NULL;
_memset_inline(&Video_aPalette, 0, sizeof(Video_aPalette));
_memset_inline(&Video_format, 0, sizeof(Video_format));
_memset_inline(&Video_format2, 0, sizeof(Video_format2));
_memset_inline(&Video_modeStruct2, 0, sizeof(Video_modeStruct2));
_memset_inline(&Video_bufIdk, 0, sizeof(Video_bufIdk));
stdConsole_textAttribute = 0;
stdConsole_wAttributes = 0;
stdConsole_cursorHidden = 0;
_memset_inline(&stdConsole_ConsoleCursorInfo, 0, sizeof(stdConsole_ConsoleCursorInfo));
_memset_inline(&stdConsole_hOutput, 0, sizeof(stdConsole_hOutput));
_memset_inline(&stdConsole_hInput, 0, sizeof(stdConsole_hInput));
_memset_inline(&rdRaster_aOneOverNFixed, 0, sizeof(rdRaster_aOneOverNFixed));
_memset_inline(&rdRaster_aLerpAndScale, 0, sizeof(rdRaster_aLerpAndScale));
_memset_inline(&rdRaster_aOneOverNFlex, 0, sizeof(rdRaster_aOneOverNFlex));
flex_t  __rdRaster_fixedScale_origValue =  65536.0f;
_memcpy(&rdRaster_fixedScale, &__rdRaster_fixedScale_origValue, sizeof(rdRaster_fixedScale));
_memset_inline(&sithCogFunctionAI_aThingsInView, 0, sizeof(sithCogFunctionAI_aThingsInView));
sithCogFunctionAI_numThingsInView = 0;
sithCogFunctionAI_curThingInView = 0;
#ifndef SITHCOMM_HEAP_MSGBUF
_memset_inline(&sithComm_MsgTmpBuf, 0, sizeof(sithComm_MsgTmpBuf));
#endif
_memset_inline(&sithComm_aMsgPairs, 0, sizeof(sithComm_aMsgPairs));
_memset_inline(&sithMessage_aTypeFuncs, 0, sizeof(sithMessage_aTypeFuncs));
sithMessage_bStopProcessMessages = 0;
sithMessage_g_outputstream = 0;
sithMessage_g_inputstream = 0;
sithComm_idk2 = 0;
sithMessage_bSturtup = 0;
sithComm_dword_847E84 = 0;
int  __sithComm_msgId_origValue =  1;
_memcpy(&sithComm_msgId, &__sithComm_msgId_origValue, sizeof(sithComm_msgId));
_memset_inline(&sithComm_MsgTmpBuf2, 0, sizeof(sithComm_MsgTmpBuf2));
_memset_inline(&sithComm_netMsgTmp, 0, sizeof(sithComm_netMsgTmp));
_memset_inline(&jkCog_emptystring, 0, sizeof(jkCog_emptystring));
_memset_inline(&jkCog_jkstring, 0, sizeof(jkCog_jkstring));
jkCog_bInitted = 0;
_memset_inline(&jkCog_strings, 0, sizeof(jkCog_strings));
sithCog_bOpened = 0;
sithCog_g_pHashtable = NULL;
_memset_inline(&sithCog_aSectorLinks, 0, sizeof(sithCog_aSectorLinks));
sithCog_numSectorLinks = 0;
_memset_inline(&sithCog_aThingLinks, 0, sizeof(sithCog_aThingLinks));
sithCog_numThingLinks = 0;
sithCog_numSurfaceLinks = 0;
_memset_inline(&sithCog_aSurfaceLinks, 0, sizeof(sithCog_aSurfaceLinks));
sithCog_g_pMasterCog = NULL;
_memset_inline(&jkDev_aEntryPositions, 0, sizeof(jkDev_aEntryPositions));
_memset_inline(&jkDev_aEntries, 0, sizeof(jkDev_aEntries));
jkDev_log_55A4A4 = 0;
jkDev_bScreenNeedsUpdate = 0;
_memset_inline(&jkDev_aCheatCmds, 0, sizeof(jkDev_aCheatCmds));
jkDev_numCheats = 0;
jkDev_bInitted = 0;
jkDev_bOpened = 0;
jkDev_cheatHashtable = NULL;
_memset_inline(&jkDev_hDlg, 0, sizeof(jkDev_hDlg));
jkDev_vbuf = NULL;
jkDev_BMFontHeight = 0;
jkDev_ColorKey = 0;
jkDev_dword_55A9D0 = 0;
jkDev_amt = 0.0f;
jkSmack_gameMode = 0;
jkSmack_bInit = 0;
jkSmack_stopTick = 0;
jkSmack_currentGuiState = 0;
jkSmack_nextGuiState = 0;
jkSmack_alloc = NULL;
_memset_inline(&jkEpisode_aEpisodes, 0, sizeof(jkEpisode_aEpisodes));
_memset_inline(&jkEpisode_var4, 0, sizeof(jkEpisode_var4));
_memset_inline(&jkEpisode_var5, 0, sizeof(jkEpisode_var5));
jkEpisode_var2 = 0;
_memset_inline(&jkEpisode_mLoad, 0, sizeof(jkEpisode_mLoad));
uint32_t  __jkHud_targetRed_origValue =  1;
_memcpy(&jkHud_targetRed, &__jkHud_targetRed_origValue, sizeof(jkHud_targetRed));
uint32_t  __jkHud_targetBlue_origValue =  2;
_memcpy(&jkHud_targetBlue, &__jkHud_targetBlue_origValue, sizeof(jkHud_targetBlue));
uint32_t  __jkHud_targetGreen_origValue =  3;
_memcpy(&jkHud_targetGreen, &__jkHud_targetGreen_origValue, sizeof(jkHud_targetGreen));
flex_t __jkHud_aFltIdk_origValue[6]  =  {0.2, 0.4, 0.8, 1.0, 2.0, 0.0};
_memcpy(&jkHud_aFltIdk, &__jkHud_aFltIdk_origValue, sizeof(jkHud_aFltIdk));
int32_t __jkHud_aColors8bpp_origValue[6]  =  {1,2,3,4,5,0};
_memcpy(&jkHud_aColors8bpp, &__jkHud_aColors8bpp_origValue, sizeof(jkHud_aColors8bpp));
int __jkHud_aTeamColors8bpp_origValue[5]  =  {0x0, 0x6, 0x69, 0x1F, 0x2};
_memcpy(&jkHud_aTeamColors8bpp, &__jkHud_aTeamColors8bpp_origValue, sizeof(jkHud_aTeamColors8bpp));
_memset_inline(&jkHud_chatStr, 0, sizeof(jkHud_chatStr));
_memset_inline(&jkHud_aTeamColors16bpp, 0, sizeof(jkHud_aTeamColors16bpp));
jkHud_rightBlitX = 0;
jkHud_leftBlitX = 0;
_memset_inline(&jkHud_mapRendConfig, 0, sizeof(jkHud_mapRendConfig));
jkHud_chatStrPos = 0;
jkHud_rightBlitY = 0;
_memset_inline(&jkHud_aTeamScores, 0, sizeof(jkHud_aTeamScores));
jkHud_dword_552D10 = 0;
_memset_inline(&jkHud_aColors16bpp, 0, sizeof(jkHud_aColors16bpp));
_memset_inline(&jkHud_aPlayerScores, 0, sizeof(jkHud_aPlayerScores));
jkHud_blittedAmmoAmt = 0;
jkHud_idk14 = 0;
jkHud_blittedHealthIdx = 0;
jkHud_blittedBatteryAmt = 0;
jkHud_blittedFieldlightAmt = 0;
jkHud_blittedShieldIdx = 0;
jkHud_isSuper = 0;
jkHud_idk15 = 0;
jkHud_blittedForceIdx = 0;
jkHud_idk16 = 0;
jkHud_leftBlitY = 0;
_memset_inline(&jkHud_rectViewScores, 0, sizeof(jkHud_rectViewScores));
jkHud_pMsgFontSft = NULL;
jkHud_pStatusLeftBm = NULL;
jkHud_pStatusRightBm = NULL;
jkHud_bHasTarget = 0;
jkHud_pTargetThing = NULL;
jkHud_targetRed16 = 0;
jkHud_targetGreen16 = 0;
jkHud_targetBlue16 = 0;
jkHud_bChatOpen = 0;
jkHud_pHelthNumSft = NULL;
jkHud_pAmoNumsSft = NULL;
jkHud_pAmoNumsSuperSft = NULL;
jkHud_pArmorNumSft = NULL;
jkHud_pArmorNumsSuperSft = NULL;
jkHud_bInitted = 0;
jkHud_bOpened = 0;
jkHud_pFieldlightBm = NULL;
jkHud_pStBatBm = NULL;
jkHud_pStHealthBm = NULL;
jkHud_pStShieldBm = NULL;
jkHud_pStFrcBm = NULL;
jkHud_pStFrcSuperBm = NULL;
jkHud_bViewScores = 0;
jkHud_dword_553ED0 = 0;
jkHud_tallyWhich = 0;
jkHud_numPlayers = 0;
jkHud_dword_553EDC = 0;
jkHud_dword_553EE0 = 0;
_memset_inline(&jkHudInv_info, 0, sizeof(jkHudInv_info));
_memset_inline(&jkHudInv_itemTexfmt, 0, sizeof(jkHudInv_itemTexfmt));
jkHudInv_flags = 0;
jkHudInv_dword_553F64 = 0;
_memset_inline(&jkHudInv_scroll, 0, sizeof(jkHudInv_scroll));
_memset_inline(&jkHudInv_aBitmaps, 0, sizeof(jkHudInv_aBitmaps));
jkHudInv_font = NULL;
jkHudInv_rend_isshowing_maybe = 0;
jkHudInv_dword_553F94 = 0;
jkHudInv_numItems = 0;
jkHudInv_aItems = NULL;
Main_bDevMode = 0;
Main_bDisplayConfig = 0;
Main_bWindowGUI = 0;
Main_dword_86078C = 0;
Main_bFrameRate = 0;
Main_bDispStats = 0;
Main_bNoHUD = 0;
Main_logLevel = 0;
Main_verboseLevel = 0;
_memset_inline(&Main_path, 0, sizeof(Main_path));
stdFile_t  __debug_log_fp_origValue =  0;
_memcpy(&debug_log_fp, &__debug_log_fp_origValue, sizeof(debug_log_fp));
pHS = NULL;
_memset_inline(&jkCredits_aPalette, 0, sizeof(jkCredits_aPalette));
jkCredits_pVbuffer2 = NULL;
jkCredits_dword_55AD64 = 0;
jkCredits_dword_55AD68 = 0;
_memset_inline(&jkCredits_table, 0, sizeof(jkCredits_table));
jkCredits_startMs = 0;
jkCredits_dword_55AD84 = 0;
jkCredits_strIdx = 0;
jkCredits_aIdk = NULL;
jkCredits_pVbuffer = NULL;
jkCredits_dword_55AD94 = 0;
jkCredits_fontLarge = NULL;
jkCredits_fontSmall = NULL;
jkCredits_dword_55ADA0 = 0;
jkCredits_bInitted = 0;
jkCredits_dword_55ADA8 = 0;
g_sithMode = 0;
g_submodeFlags = 0;
g_debugmodeFlags = 0;
g_mapModeFlags = 0;
jkGame_gamma = 0;
jkGame_screenSize = 0;
jkGame_bInitted = 0;
jkGame_updateMsecsTotal = 0;
jkGame_dword_552B5C = 0;
jkGame_isDDraw = 0;
jkRes_pHS = NULL;
char __jkRes_episodeGobName_origValue[32]  =  {0};
_memcpy(&jkRes_episodeGobName, &__jkRes_episodeGobName_origValue, sizeof(jkRes_episodeGobName));
char __jkRes_curDir_origValue[128]  =  {0};
_memcpy(&jkRes_curDir, &__jkRes_curDir_origValue, sizeof(jkRes_curDir));
jkRes_bHookedHS = 0;
_memset_inline(&jkRes_aFiles, 0, sizeof(jkRes_aFiles));
_memset_inline(&jkRes_gCtx, 0, sizeof(jkRes_gCtx));
pLowLevelHS = NULL;
_memset_inline(&lowLevelHS, 0, sizeof(lowLevelHS));
_memset_inline(&jkRes_idkGobPath, 0, sizeof(jkRes_idkGobPath));
_memset_inline(&jkCutscene_rect1, 0, sizeof(jkCutscene_rect1));
_memset_inline(&jkCutscene_rect2, 0, sizeof(jkCutscene_rect2));
_memset_inline(&jkCutscene_strings, 0, sizeof(jkCutscene_strings));
jkCutscene_subtitlefont = NULL;
jkCutscene_bInitted = 0;
jkCutscene_isRendering = 0;
jkCutscene_dword_55B750 = 0;
jkCutscene_dword_55AA50 = 0;
jkCutscene_55AA54 = 0;
_memset_inline(&jkMain_aLevelJklFname, 0, sizeof(jkMain_aLevelJklFname));
int  __thing_nine_origValue =  1;
_memcpy(&thing_nine, &__thing_nine_origValue, sizeof(thing_nine));
jkMain_bInit = 0;
thing_six = 0;
thing_eight = 0;
jkMain_dword_552B98 = 0;
jkMain_lastTickMs = 0;
idx_13b4_related = 0;
_memset_inline(&gamemode_1_str, 0, sizeof(gamemode_1_str));
_memset_inline(&jkMain_strIdk, 0, sizeof(jkMain_strIdk));
_memset_inline(&jkMain_wstrIdk, 0, sizeof(jkMain_wstrIdk));
_memset_inline(&sithWorld_aSectionParsers, 0, sizeof(sithWorld_aSectionParsers));
sithWorld_some_integer_4 = 0;
sithWorld_g_pCurrentWorld = NULL;
sithWorld_g_pStaticWorld = NULL;
sithWorld_g_pLastLoadedWorld = NULL;
sithWorld_numParsers = 0;
sithWorld_bInitted = 0;
sithWorld_bLoaded = 0;
_memset_inline(&sithWorld_episodeName, 0, sizeof(sithWorld_episodeName));
_memset_inline(&sithInventory_powerKeybinds, 0, sizeof(sithInventory_powerKeybinds));
int  __sithInventory_g_bInitInventory_origValue =  1;
_memcpy(&sithInventory_g_bInitInventory, &__sithInventory_g_bInitInventory_origValue, sizeof(sithInventory_g_bInitInventory));
_memset_inline(&sithInventory_g_aTypes, 0, sizeof(sithInventory_g_aTypes));
sithInventory_g_bSendDeactivateMessage = 0;
sithInventory_bUnkPower = 0;
sithInventory_8339EC = 0;
sithInventory_bRendIsHidden = 0;
sithInventory_8339F4 = 0;
sithWeapon_controlOptions = 0;
g_flt_8BD040 = 0.0f;
g_flt_8BD044 = 0.0f;
g_flt_8BD048 = 0.0f;
g_flt_8BD04C = 0.0f;
g_flt_8BD050 = 0.0f;
g_flt_8BD054 = 0.0f;
g_flt_8BD058 = 0.0f;
sithWeapon_CurWeaponMode = 0;
sithWeapon_bAutoPickup = 0;
sithWeapon_bAutoSwitch = 0;
sithWeapon_bAutoReload = 0;
sithWeapon_bMultiAutoPickup = 0;
sithWeapon_bMultiplayerAutoSwitch = 0;
sithWeapon_bMultiAutoReload = 0;
sithWeapon_bAutoAim = 0;
sithWeapon_secMountWait = 0.0f;
_memset_inline(&sithWeapon_8BD0A0, 0, sizeof(sithWeapon_8BD0A0));
sithWeapon_fireWait = 0.0f;
sithWeapon_fireRate = 0.0f;
sithWeapon_LastFireTimeSecs = 0.0f;
_memset_inline(&sithWeapon_a8BD030, 0, sizeof(sithWeapon_a8BD030));
sithWeapon_8BD05C = 0;
sithWeapon_8BD060 = 0.0f;
_memset_inline(&sithWeapon_8BD008, 0, sizeof(sithWeapon_8BD008));
sithWeapon_8BD024 = 0;
sithWeapon_senderIndex = 0;
_memset_inline(&jkPlayer_playerInfos, 0, sizeof(jkPlayer_playerInfos));
_memset_inline(&jkPlayer_playerShortName, 0, sizeof(jkPlayer_playerShortName));
jkPlayer_numOtherThings = 0;
jkPlayer_numThings = 0;
_memset_inline(&jkPlayer_otherThings, 0, sizeof(jkPlayer_otherThings));
int  __jkPlayer_bLoadingSomething_origValue =  1;
_memcpy(&jkPlayer_bLoadingSomething, &__jkPlayer_bLoadingSomething_origValue, sizeof(jkPlayer_bLoadingSomething));
playerThingIdx = 0;
jkPlayer_maxPlayers = 0;
sithPlayer_g_pLocalPlayerThing = NULL;
sithPlayer_g_pLocalPlayer = NULL;
_memset_inline(&playerThings, 0, sizeof(playerThings));
_memset_inline(&jkSaber_rotateMat, 0, sizeof(jkSaber_rotateMat));
jkPlayer_setRotateOverlayMap = 0;
jkPlayer_setDrawStatus = 0;
jkPlayer_setCrosshair = 0;
jkPlayer_setSaberCam = 0;
jkPlayer_setFullSubtitles = 0;
jkPlayer_setDisableCutscenes = 0;
_memset_inline(&jkPlayer_aCutsceneVal, 0, sizeof(jkPlayer_aCutsceneVal));
_memset_inline(&jkPlayer_cutscenePath, 0, sizeof(jkPlayer_cutscenePath));
jkPlayer_setNumCutscenes = 0;
jkPlayer_currentTickIdx = 0;
jkPlayer_setDiff = 0;
_memset_inline(&jkPlayer_waggleVec, 0, sizeof(jkPlayer_waggleVec));
jkPlayer_waggleMag = 0.0f;
jkPlayer_mpcInfoSet = 0;
jkPlayer_waggleAngle = 0.0f;
_memset_inline(&jkSaber_rotateVec, 0, sizeof(jkSaber_rotateVec));
_memset_inline(&jkPlayer_name, 0, sizeof(jkPlayer_name));
_memset_inline(&jkPlayer_model, 0, sizeof(jkPlayer_model));
_memset_inline(&jkPlayer_soundClass, 0, sizeof(jkPlayer_soundClass));
_memset_inline(&jkPlayer_sideMat, 0, sizeof(jkPlayer_sideMat));
_memset_inline(&jkPlayer_tipMat, 0, sizeof(jkPlayer_tipMat));
_memset_inline(&sithCollision_aNumSearchedSectors, 0, sizeof(sithCollision_aNumSearchedSectors));
_memset_inline(&sithCollision_collisionHandlers, 0, sizeof(sithCollision_collisionHandlers));
_memset_inline(&sithCollision_aThingSurfaceCollideResults, 0, sizeof(sithCollision_aThingSurfaceCollideResults));
_memset_inline(&sithCollision_aCollisions, 0, sizeof(sithCollision_aCollisions));
_memset_inline(&sithCollision_aNumStackCollisions, 0, sizeof(sithCollision_aNumStackCollisions));
int  __sithCollision_searchStackIdx_origValue =  -1;
_memcpy(&sithCollision_searchStackIdx, &__sithCollision_searchStackIdx_origValue, sizeof(sithCollision_searchStackIdx));
_memset_inline(&sithCollision_apSearchedSectors, 0, sizeof(sithCollision_apSearchedSectors));
sithCollision_dword_8B4BE4 = 0;
_memset_inline(&sithCollision_collideHurtIdk, 0, sizeof(sithCollision_collideHurtIdk));
rdVector3  __sithSector_surfaceNormal_origValue =  {0.0, 0.0, -1.0};
_memcpy(&sithSector_surfaceNormal, &__sithSector_surfaceNormal_origValue, sizeof(sithSector_surfaceNormal));
_memset_inline(&sithAIAwareness_aEntries, 0, sizeof(sithAIAwareness_aEntries));
sithAIAwareness_g_aSectors = NULL;
sithAIAwareness_numEntries = 0;
sithAIAwareness_bInitted = 0;
sithAIAwareness_timerTicks = 0;
sithSector_flt_8553B8 = 0.0f;
sithSector_horizontalPixelsPerRev = 0.0f;
sithSector_flt_8553C0 = 0.0f;
sithSector_flt_8553C4 = 0.0f;
sithSector_flt_8553C8 = 0.0f;
_memset_inline(&sithSector_zMaxVec, 0, sizeof(sithSector_zMaxVec));
sithSector_ceilingSky = 0.0f;
_memset_inline(&sithSector_zMinVec, 0, sizeof(sithSector_zMinVec));
sithSector_horizontalPixelsPerRev_idk = 0.0f;
sithSector_horizontalDist = 0.0f;
sithSector_flt_8553F4 = 0.0f;
_memset_inline(&sithSector_aModifiedSectors, 0, sizeof(sithSector_aModifiedSectors));
_memset_inline(&sithSector_aSyncFlags, 0, sizeof(sithSector_aSyncFlags));
sithSector_numModifiedSectors = 0;
sithThing_pParseHashtbl = NULL;
_memset_inline(&sithThing_pfUnknownFunc, 0, sizeof(sithThing_pfUnknownFunc));
uint16_t  __sithThing_guidEntropy_origValue =  1;
_memcpy(&sithThing_guidEntropy, &__sithThing_guidEntropy_origValue, sizeof(sithThing_guidEntropy));
_memset_inline(&jkGuiSaveLoad_wtextEpisode, 0, sizeof(jkGuiSaveLoad_wtextEpisode));
_memset_inline(&jkGuiSaveLoad_wtextHealth, 0, sizeof(jkGuiSaveLoad_wtextHealth));
jkGuiSaveLoad_numEntries = 0;
_memset_inline(&jkGuiSaveLoad_wtextShields, 0, sizeof(jkGuiSaveLoad_wtextShields));
_memset_inline(&jkGuiSaveLoad_word_559830, 0, sizeof(jkGuiSaveLoad_word_559830));
jkGuiSaveLoad_bIsSaveMenu = 0;
_memset_inline(&jkGuiSaveLoad_wtextSaveName, 0, sizeof(jkGuiSaveLoad_wtextSaveName));
_memset_inline(&jkGuiSaveLoad_DarrayEntries, 0, sizeof(jkGuiSaveLoad_DarrayEntries));
_memset_inline(&jkGuiSaveLoad_word_559C54, 0, sizeof(jkGuiSaveLoad_word_559C54));
_memset_inline(&jkGuiSaveLoad_byte_559C50, 0, sizeof(jkGuiSaveLoad_byte_559C50));
int  __jkGuiTitle_verMajor_origValue =  1;
_memcpy(&jkGuiTitle_verMajor, &__jkGuiTitle_verMajor_origValue, sizeof(jkGuiTitle_verMajor));
int  __jkGuiTitle_verMinor_origValue =  0;
_memcpy(&jkGuiTitle_verMinor, &__jkGuiTitle_verMinor_origValue, sizeof(jkGuiTitle_verMinor));
int  __jkGuiTitle_verRevision_origValue =  0;
_memcpy(&jkGuiTitle_verRevision, &__jkGuiTitle_verRevision_origValue, sizeof(jkGuiTitle_verRevision));
_memset_inline(&jkGuiTitle_aTexts, 0, sizeof(jkGuiTitle_aTexts));
jkGuiTitle_whichLoading = 0;
flex_t  __jkGuiSound_sfxVolume_origValue =  0.8;
_memcpy(&jkGuiSound_sfxVolume, &__jkGuiSound_sfxVolume_origValue, sizeof(jkGuiSound_sfxVolume));
uint32_t  __jkGuiSound_numChannels_origValue =  16;
_memcpy(&jkGuiSound_numChannels, &__jkGuiSound_numChannels_origValue, sizeof(jkGuiSound_numChannels));
jkGuiSound_bLowResSound = 0;
int  __jkGuiSound_b3DSound_origValue =  1;
_memcpy(&jkGuiSound_b3DSound, &__jkGuiSound_b3DSound_origValue, sizeof(jkGuiSound_b3DSound));
int  __jkGuiSound_b3DSound_2_origValue =  1;
_memcpy(&jkGuiSound_b3DSound_2, &__jkGuiSound_b3DSound_2_origValue, sizeof(jkGuiSound_b3DSound_2));
int  __jkGuiSound_b3DSound_3_origValue =  1;
_memcpy(&jkGuiSound_b3DSound_3, &__jkGuiSound_b3DSound_3_origValue, sizeof(jkGuiSound_b3DSound_3));
jkGuiSound_musicVolume = 0.0f;
_memset_inline(&jkGui_unkstr, 0, sizeof(jkGui_unkstr));
jkGui_GdiMode = 0;
jkGui_modesets = 0;
_memset_inline(&jkGui_stdBitmaps, 0, sizeof(jkGui_stdBitmaps));
_memset_inline(&jkGui_stdFonts, 0, sizeof(jkGui_stdFonts));
_memset_inline(&jkGui_episodeLoad, 0, sizeof(jkGui_episodeLoad));
jkGuiSingleTally_foStars = NULL;
jkGuiNetHost_maxRank = 0;
jkGuiNetHost_timeLimit = 0;
jkGuiNetHost_scoreLimit = 0;
jkGuiNetHost_maxPlayers = 0;
jkGuiNetHost_sessionFlags = 0;
jkGuiNetHost_gameFlags = 0;
jkGuiNetHost_tickRate = 0;
_memset_inline(&jkGuiNetHost_gameName, 0, sizeof(jkGuiNetHost_gameName));
jkGuiMultiplayer_checksumSeed = 0;
jkGuiMultiplayer_dword_5564EC = 0;
jkGuiMultiplayer_dword_5564E8 = 0;
_memset_inline(&jkGuiMultiplayer_multiEntry, 0, sizeof(jkGuiMultiplayer_multiEntry));
jkGuiMultiplayer_dword_5564F0 = 0;
_memset_inline(&g_hInstance, 0, sizeof(g_hInstance));
sithCog_g_pSymbolTable = NULL;
pSithHS = NULL;
_memset_inline(&g_hWnd, 0, sizeof(g_hWnd));
g_nShowCmd = 0;
g_app_suspended = 0;
g_window_active = 0;
g_app_active = 0;
g_should_exit = 0;
g_thing_two_some_dialog_count = 0;
g_handler_count = 0;
g_855E8C = 0;
g_855E90 = 0;
g_window_not_destroyed = 0;
_memset_inline(&stdPalEffects_state, 0, sizeof(stdPalEffects_state));
_memset_inline(&stdPalEffects_palette, 0, sizeof(stdPalEffects_palette));
stdPalEffects_numEffectRequests = 0;
_memset_inline(&stdPalEffects_aEffects, 0, sizeof(stdPalEffects_aEffects));
_memset_inline(&stdPalEffects_setPalette, 0, sizeof(stdPalEffects_setPalette));
_memset_inline(&stdPalEffects_aPalette, 0, sizeof(stdPalEffects_aPalette));
_memset_inline(&aFilenameStack, 0, sizeof(aFilenameStack));
_memset_inline(&apBufferStack, 0, sizeof(apBufferStack));
_memset_inline(&linenumStack, 0, sizeof(linenumStack));
_memset_inline(&aEntryStack, 0, sizeof(aEntryStack));
_memset_inline(&openFileStack, 0, sizeof(openFileStack));
_memset_inline(&printfBuffer, 0, sizeof(printfBuffer));
stdConffile_linenum = 0;
stdConffile_bOpen = 0;
_memset_inline(&openFile, 0, sizeof(openFile));
_memset_inline(&writeFile, 0, sizeof(writeFile));
stackLevel = 0;
_memset_inline(&stdConffile_aWriteFilename, 0, sizeof(stdConffile_aWriteFilename));
_memset_inline(&stdConffile_g_entry, 0, sizeof(stdConffile_g_entry));
_memset_inline(&stdConffile_pFilename, 0, sizeof(stdConffile_pFilename));
stdConffile_g_aLine = NULL;
stdMemory_bInitted = 0;
stdMemory_bOpened = 0;
_memset_inline(&stdMemory_g_curState, 0, sizeof(stdMemory_g_curState));
_memset_inline(&yyin, 0, sizeof(yyin));
_memset_inline(&yyout, 0, sizeof(yyout));
sithCogParse_pSymbolTable = NULL;
yacc_linenum = 0;
int  __cog_yacc_loop_depth_origValue =  1;
_memcpy(&cog_yacc_loop_depth, &__cog_yacc_loop_depth_origValue, sizeof(cog_yacc_loop_depth));
_memset_inline(&cog_parser_node_stackpos, 0, sizeof(cog_parser_node_stackpos));
cogvm_stackpos = 0;
cogparser_nodes_alloc = NULL;
cogparser_topnode = NULL;
cogvm_stack = NULL;
cogparser_num_nodes = 0;
cogparser_current_nodeidx = 0;
parsing_script = NULL;
int  __parsing_script_idk_origValue =  1;
_memcpy(&parsing_script_idk, &__parsing_script_idk_origValue, sizeof(parsing_script_idk));
dplay_dword_55D618 = 0;
dplay_dword_55D61C = 0;
_memset_inline(&jkGui_guid_556040, 0, sizeof(jkGui_guid_556040));
jkGuiMultiplayer_numConnections = 0;
_memset_inline(&jkGuiMultiplayer_aConnections, 0, sizeof(jkGuiMultiplayer_aConnections));
_memset_inline(&jkGuiMultiplayer_aEntries, 0, sizeof(jkGuiMultiplayer_aEntries));
_memset_inline(&jkGuiMultiplayer_stru_556168, 0, sizeof(jkGuiMultiplayer_stru_556168));
_memset_inline(&jkGuiMultiplayer_mpcInfo, 0, sizeof(jkGuiMultiplayer_mpcInfo));
_memset_inline(&jkGuiMultiplayer_stru_5564A8, 0, sizeof(jkGuiMultiplayer_stru_5564A8));
jkGuiMouse_bOpen = 0;
_memset_inline(&jkGuiMouse_Darray_556698, 0, sizeof(jkGuiMouse_Darray_556698));
jkGuiMouse_dword_5566B0 = 0;
_memset_inline(&jkGuiMouse_Darray_5566B8, 0, sizeof(jkGuiMouse_Darray_5566B8));
_memset_inline(&jkGuiMouse_Darray_5566D0, 0, sizeof(jkGuiMouse_Darray_5566D0));
jkGuiMouse_pWStr_5566E8 = NULL;
jkGuiEsc_bInitialized = 0;
jkGuiKeyboard_dword_555DE0 = 0;
jkGuiKeyboard_bOnceIdk = 0;
jkGuiKeyboard_funcIdx = 0;
jkGuiKeyboard_flags = 0;
_memset_inline(&jkGuiKeyboard_darrEntries, 0, sizeof(jkGuiKeyboard_darrEntries));
jkGuiKeyboard_dword_555E10 = 0;
_memset_inline(&jkGuiKeyboard_wstr_555E18, 0, sizeof(jkGuiKeyboard_wstr_555E18));
jkGuiKeyboard_pWStr_55601C = NULL;
_memset_inline(&jkGuiMap_vec3Idk2, 0, sizeof(jkGuiMap_vec3Idk2));
jkGuiMap_pCanvas = NULL;
_memset_inline(&jkGuiMap_viewMat, 0, sizeof(jkGuiMap_viewMat));
_memset_inline(&jkGuiMap_matTmp, 0, sizeof(jkGuiMap_matTmp));
jkGuiMap_pVbuffer = NULL;
_memset_inline(&jkGuiMap_unk4, 0, sizeof(jkGuiMap_unk4));
_memset_inline(&jkGuiMap_vec3Idk, 0, sizeof(jkGuiMap_vec3Idk));
jkGuiMap_pCamera = NULL;
jkGuiMap_dword_556660 = 0;
jkGuiMap_bOrbitActive = 0;
jkGuiMap_dword_556668 = 0;
jkGuiMap_dword_55666C = 0;
flex_t __sithMap_unkArr_origValue[12]  =  {0.5, 1.0, 1.5, 2.0, 2.5, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 0.0};
_memcpy(&sithMap_unkArr, &__sithMap_unkArr_origValue, sizeof(sithMap_unkArr));
sithMap_pPlayerThing = NULL;
_memset_inline(&sithMap_invMatrix, 0, sizeof(sithMap_invMatrix));
sithMap_flt_84DEA8 = 0.0f;
sithMap_flt_84DEAC = 0.0f;
_memset_inline(&sithMap_ctx, 0, sizeof(sithMap_ctx));
sithMap_pCurWorld = NULL;
_memset_inline(&sithMap_camera, 0, sizeof(sithMap_camera));
sithMap_pCurCamera = NULL;
sithMap_bInitted = 0;
sithMap_var = 0;
DirectPlay_numPlayers = 0;
_memset_inline(&DirectPlay_aPlayers, 0, sizeof(DirectPlay_aPlayers));
}

// /Users/bob/esp-cpp/tab5-emu/components/jk/OpenJKDF2/resource/ui/openjkdf2_i8n.uni+/Users/bob/esp-cpp/tab5-emu/components/jk/OpenJKDF2/resource/ui/openjkdf2.uni
// /Users/bob/esp-cpp/tab5-emu/components/jk/OpenJKDF2
const size_t embeddedResource_aFiles_num = 2;
const embeddedResource_t embeddedResource_aFiles[2] = {
// /Users/bob/esp-cpp/tab5-emu/components/jk/OpenJKDF2/resource/ui/openjkdf2_i8n.uni
{"ui/openjkdf2_i8n.uni", 
"MSGS 2          # <--- DON'T FORGET TO UPDATE THIS COUNT\n"
"\n"
"#  \"<key>\"     <unused number>   \"<string>\"\n"
"\n"
"#******************************\n"
"#  This file can override the following:\n"
"#   - ui/openjkdf2.uni\n"
"#   - ui/jkstrings.uni\n"
"#   - misc/cogStrings.uni (in some circumstances, WIP)\n"
"#   - misc/sithStrings.uni\n"
"#******************************\n"
"   \"OPENJKDF2_EXAMPLE\"    0  \"Example override!\"\n"
"\n"
"END\n"
"\n"
, 0x186},
// /Users/bob/esp-cpp/tab5-emu/components/jk/OpenJKDF2/resource/ui/openjkdf2.uni
{"ui/openjkdf2.uni", 
"MSGS 102          # <--- DON'T FORGET TO UPDATE THIS COUNT\n"
"\n"
"#  \"<key>\"     <unused number>   \"<string>\"\n"
"\n"
"#******************************\n"
"#  OpenJKDF2 added UI strings\n"
"#******************************\n"
"   \"OPENJKDF2_EXAMPLE\"    0  \"Example!\"\n"
"\n"
"#******************************\n"
"#  Gameplay\n"
"#******************************\n"
"   \"GUIEXT_SHOW_SABER_CROSSHAIR\"          0 \"Show Crosshair with lightsaber\"\n"
"   \"GUIEXT_SHOW_SABER_CROSSHAIR_HINT\"     0 \"Display the crosshair when the lightsaber is equipped?\"\n"
"   \"GUIEXT_SHOW_FIST_CROSSHAIR\"           0 \"Show Crosshair with fist\"\n"
"   \"GUIEXT_SHOW_FIST_CROSSHAIR_HINT\"      0 \"Display the crosshair when the fists are equipped?\"\n"
"   \"GUIEXT_DISABLE_WAGGLE\"                0 \"Disable Weapon Waggle\"\n"
"   \"GUIEXT_DISABLE_WAGGLE_HINT\"           0 \"Disable weapon waggle when moving?\"\n"
"   \"GUIEXT_CROSSHAIR_SCALE\"               0 \"Crosshair Scale\"\n"
"   \"GUIEXT_CROSSHAIR_SCALE_HINT\"          0 \"Adjust the size of the crosshair (from 0% to 200%)\"\n"
"\n"
"#******************************\n"
"#  Display\n"
"#******************************\n"
"   \"GUIEXT_FOV\"                           0 \"FOV\"\n"
"   \"GUIEXT_FOV_HINT\"                      0 \"Set FOV\"\n"
"   \"GUIEXT_FOV_VERTICAL\"                  0 \"FOV is vertical (Hor+)\"\n"
"   \"GUIEXT_EN_FULLSCREEN\"                 0 \"Enable Fullscreen\"\n"
"   \"GUIEXT_EN_HIDPI\"                      0 \"Enable HiDPI\"\n"
"   \"GUIEXT_EN_TEXTURE_FILTERING\"          0 \"Enable Texture Filtering\"\n"
"   \"GUIEXT_EN_SQUARE_ASPECT\"              0 \"Use 1:1 aspect\"\n"
"   \"GUIEXT_FPS_LIMIT\"                     0 \"FPS Limit\"\n"
"   \"GUIEXT_FPS_LIMIT_HINT\"                0 \"Set FPS limit\"\n"
"   \"GUIEXT_EN_VSYNC\"                      0 \"Enable VSync\"\n"
"   \"GUIEXT_EN_BLOOM\"                      0 \"Enable Bloom\"\n"
"   \"GUIEXT_EN_SSAO\"                       0 \"Enable SSAO\"\n"
"   \"GUIEXT_SSAA_MULT\"                     0 \"SSAA Multiplier:\"\n"
"   \"GUIEXT_GAMMA_VAL\"                     0 \"Gamma Value:\"\n"
"   \"GUIEXT_HUD_SCALE\"                     0 \"HUD Scale:\"\n"
"\n"
"   \"GUIEXT_EN_JKGFXMOD\"                   0 \"Enable jkgfxmod\"\n"
"   \"GUIEXT_EN_JKGFXMOD_HINT\"              0 \"Override in-game textures with textures from jkgm folder?\"\n"
"   \"GUIEXT_EN_TEXTURE_PRECACHE\"           0 \"Enable texture precaching\"\n"
"   \"GUIEXT_EN_TEXTURE_PRECACHE_HINT\"      0 \"Load all material textures ahead-of-time instead of just-in-time?\"\n"
"\n"
"# TWL-only for now\n"
"   \"GUIEXT_EN_EMISSIVE_TEXTURES\"          0 \"Enable emissive textures\"\n"
"   \"GUIEXT_EN_EMISSIVE_TEXTURES_HINT\"     0 \"Display textures which glow in the dark.\"\n"
"   \"GUIEXT_EN_CLASSIC_LIGHTING\"           0 \"Enable classic lighting\"\n"
"   \"GUIEXT_EN_CLASSIC_LIGHTING_HINT\"      0 \"Use the original 256-color light tables.\"\n"
"\n"
"\n"
"#******************************\n"
"#  Sound\n"
"#******************************\n"
"  \"GUIEXT_CUTSCENE_VOLUME\"                0 \"Cutscene Volume\"\n"
"  \"GUIEXT_CUTSCENE_VOLUME_HINT\"           0 \"Set the volume of audio during cutscenes\"\n"
"\n"
"#******************************\n"
"#  Quake Console and Updates\n"
"#******************************\n"
"   \"GUIEXT_UPDATE_IS_AVAIL\"                   0 \"An update is available: %S => %S\"\n"
"   \"GUIEXT_UPDATE_CLICK_TO_DL\"                0 \"Click here to download.\"\n"
"   \"GUIEXT_UPDATE_DOWNLOADING\"                0 \"Downloading...\"\n"
"   \"GUIEXT_UPDATE_COMPLETE_RESTART\"           0 \"Update complete, restart to apply.\"\n"
"   \"GUIEXT_UPDATE_COMPLETE_SEMIAUTO\"          0 \"Update downloaded, complete installation and restart.\"\n"
"\n"
"\n"
"#******************************\n"
"#  Multiplayer Setup\n"
"#******************************\n"
"   \"GUIEXT_DEDICATED_SERVER\"        0 \"Dedicated Server\"\n"
"   \"GUIEXT_DEDICATED_SERVER_HINT\"   0 \"Launch server without participating as a player.\"\n"
"   \"GUIEXT_COOP\"                    0 \"Experimental Co-op\"\n"
"   \"GUIEXT_COOP_HINT\"               0 \"Launch server with actors enabled.\"\n"
"   \"GUIEXT_SERVER_PORT\"             0 \"Server Port:\"\n"
"\n"
"   #\"GUIEXT_\"                0 \"\"\n"
"\n"
"#******************************\n"
"#  General\n"
"#******************************\n"
"   \"GUIEXT_DISABLE_MISSION_CONFIRMATION\"        0 \"Disable mission start confirmation\"\n"
"   \"GUIEXT_DISABLE_MISSION_CONFIRMATION_HINT\"   0 \"Enter the mission as soon as the level is loaded?\"\n"
"\n"
"   \"GUIEXT_INCONSISTENT_PHYS\"                   0 \"Restore inconsistent physics\"\n"
"   \"GUIEXT_INCONSISTENT_PHYS_HINT\"              0 \"Sets physics delta to the render framerate instead of 150Hz?\"\n"
"\n"
"   \"GUIEXT_CORPSE_DESPAWN\"                      0 \"Disable corpse despawning\"\n"
"   \"GUIEXT_CORPSE_DESPAWN_HINT\"                 0 \"Prevent despawn for corpse bodies?\"\n"
"\n"
"   \"GUIEXT_50HZ_MIDAIR_PHYS\"                    0 \"Restore 50Hz midair player physics\"\n"
"   \"GUIEXT_50HZ_MIDAIR_PHYS_HINT\"               0 \"Step player physics and POV at 50Hz while midair?\"\n"
"\n"
"   \"GUIEXT_LEDGE_SQUEEZE\"                       0 \"Ease climbing onto ledges\"\n"
"   \"GUIEXT_LEDGE_SQUEEZE_HINT\"                  0 \"Shrink the player collider while midair so it catches less on gap corners?\"\n"
"\n"
"#******************************\n"
"#  Controls\n"
"#******************************\n"
"   \"KEY_JOY1_B9\"                                0 \"Joy 1 Button 9\"\n"
"   \"KEY_JOY1_B10\"                               0 \"Joy 1 Button 10\"\n"
"   \"KEY_JOY1_B11\"                               0 \"Joy 1 Button 11\"\n"
"   \"KEY_JOY1_B12\"                               0 \"Joy 1 Button 12\"\n"
"   \"KEY_JOY1_B13\"                               0 \"Joy 1 Button 13\"\n"
"   \"KEY_JOY1_B14\"                               0 \"Joy 1 Button 14\"\n"
"   \"KEY_JOY1_B15\"                               0 \"Joy 1 Button 15\"\n"
"   \"KEY_JOY1_B16\"                               0 \"Joy 1 Button 16\"\n"
"   \"KEY_JOY1_B17\"                               0 \"Joy 1 Button 17\"\n"
"   \"KEY_JOY1_B18\"                               0 \"Joy 1 Button 18\"\n"
"   \"KEY_JOY1_B19\"                               0 \"Joy 1 Button 19\"\n"
"   \"KEY_JOY1_B20\"                               0 \"Joy 1 Button 20\"\n"
"   \"KEY_JOY1_B21\"                               0 \"Joy 1 Button 21\"\n"
"   \"KEY_JOY1_B22\"                               0 \"Joy 1 Button 22\"\n"
"   \"KEY_JOY1_B23\"                               0 \"Joy 1 Button 23\"\n"
"   \"KEY_JOY1_B24\"                               0 \"Joy 1 Button 24\"\n"
"   \"KEY_JOY1_B25\"                               0 \"Joy 1 Button 25\"\n"
"   \"KEY_JOY1_B26\"                               0 \"Joy 1 Button 26\"\n"
"   \"KEY_JOY1_B27\"                               0 \"Joy 1 Button 27\"\n"
"   \"KEY_JOY1_B28\"                               0 \"Joy 1 Button 28\"\n"
"   \"KEY_JOY1_B29\"                               0 \"Joy 1 Button 29\"\n"
"   \"KEY_JOY1_B30\"                               0 \"Joy 1 Button 30\"\n"
"   \"KEY_JOY1_B31\"                               0 \"Joy 1 Button 31\"\n"
"   \"KEY_JOY1_B32\"                               0 \"Joy 1 Button 32\"\n"
"\n"
"   \"KEY_JOY2_B9\"                                0 \"Joy 2 Button 9\"\n"
"   \"KEY_JOY2_B10\"                               0 \"Joy 2 Button 10\"\n"
"   \"KEY_JOY2_B11\"                               0 \"Joy 2 Button 11\"\n"
"   \"KEY_JOY2_B12\"                               0 \"Joy 2 Button 12\"\n"
"   \"KEY_JOY2_B13\"                               0 \"Joy 2 Button 13\"\n"
"   \"KEY_JOY2_B14\"                               0 \"Joy 2 Button 14\"\n"
"   \"KEY_JOY2_B15\"                               0 \"Joy 2 Button 15\"\n"
"   \"KEY_JOY2_B16\"                               0 \"Joy 2 Button 16\"\n"
"   \"KEY_JOY2_B17\"                               0 \"Joy 2 Button 17\"\n"
"   \"KEY_JOY2_B18\"                               0 \"Joy 2 Button 18\"\n"
"   \"KEY_JOY2_B19\"                               0 \"Joy 2 Button 19\"\n"
"   \"KEY_JOY2_B20\"                               0 \"Joy 2 Button 20\"\n"
"   \"KEY_JOY2_B21\"                               0 \"Joy 2 Button 21\"\n"
"   \"KEY_JOY2_B22\"                               0 \"Joy 2 Button 22\"\n"
"   \"KEY_JOY2_B23\"                               0 \"Joy 2 Button 23\"\n"
"   \"KEY_JOY2_B24\"                               0 \"Joy 2 Button 24\"\n"
"   \"KEY_JOY2_B25\"                               0 \"Joy 2 Button 25\"\n"
"   \"KEY_JOY2_B26\"                               0 \"Joy 2 Button 26\"\n"
"   \"KEY_JOY2_B27\"                               0 \"Joy 2 Button 27\"\n"
"   \"KEY_JOY2_B28\"                               0 \"Joy 2 Button 28\"\n"
"   \"KEY_JOY2_B29\"                               0 \"Joy 2 Button 29\"\n"
"   \"KEY_JOY2_B30\"                               0 \"Joy 2 Button 30\"\n"
"   \"KEY_JOY2_B31\"                               0 \"Joy 2 Button 31\"\n"
"   \"KEY_JOY2_B32\"                               0 \"Joy 2 Button 32\"\n"
"\n"
"   \"KEY_MOUSE_B5\"                               0 \"Mouse Button 5\"\n"
"   \"KEY_MOUSE_B6\"                               0 \"Mouse Button 6\"\n"
"   \"KEY_MOUSE_B7\"                               0 \"Mouse Button 7\"\n"
"   \"KEY_MOUSE_B8\"                               0 \"Mouse Button 8\"\n"
"\n"
"#******************************\n"
"#  Control Setup strings\n"
"#******************************/\n"
"\n"
"   ### Names for the control functions (actions).\n"
"   ### They match the names returned in sithControl_EnumBindings() and\n"
"   ### CONTROL_FUNCTION_NAMES in sithControl.h\n"
"   \"GAMERESTORE\"     0 \"Game Restore\"\n"
"   \"GAMERECORD\"      0 \"Game Record\"\n"
"   \"DEBUG\"           0 \"Debug\"\n"
"   \"USELASTSELECTED\" 0 \"Use Selected Skill/Item\"\n"
"\n"
"\n"
"END\n"
"\n"
, 0x22a3},
};
