#ifndef _OPENJKDF2_GLOBALS_H
#define _OPENJKDF2_GLOBALS_H

#ifdef __cplusplus
//extern "C" {
#endif

#include "engine_config.h"
#include "types.h"

#include "Engine/rdLight.h"
#include "Engine/rdCamera.h"
#include "Engine/sithCamera.h"
#include "General/stdPalEffects.h"
#include "Raster/rdCache.h"

// HACK: Fix macOS x86_64
#define SDL_cpuinfo_h_

typedef struct embeddedResource_t
{
    const char* fpath;
    const char* data;
    size_t data_len;
} embeddedResource_t;

#ifdef TARGET_TWL
#define MAYBE_UNUSED __attribute__((unused))
#else
#define MAYBE_UNUSED
#endif

#define NO_REINIT // HACK

// Addrs
#define sithAIClass_g_pHashtable_ADDR (0x00852F88)
#define std_g_genBuffer_ADDR (0x866880)
#define std_g_pHS_ADDR (0x866C80)
#define rdModel3_pCurGeoset_ADDR (0x0073A3D8)
#define localCamera_ADDR (0x0073A3E0)
#define aFaceVerts_ADDR (0x0073A3F0)
#define vertexDst_ADDR (0x0073A570)
#define curGeometryMode_ADDR (0x0073A590)
#define apGeoLights_ADDR (0x0073A598)
#define rdModel3_aLocalLightPos_ADDR (0x0073A698)
#define rdModel3_aLocalLightDir_ADDR (0x0)
#define meshFrustumCull_ADDR (0x0073A998)
#define curTextureMode_ADDR (0x0073A99C)
#define aView_ADDR (0x0073A9A0)
#define pCurMesh_ADDR (0x0073C1A0)
#define thingFrustumCull_ADDR (0x0073C1A4)
#define vertexSrc_ADDR (0x0073C1A8)
#define pCurModel3_ADDR (0x0073C1C8)
#define rdModel3_textureMode_ADDR (0x0073C1CC)
#define curLightingMode_ADDR (0x0073C1D0)
#define apMeshLights_ADDR (0x0073C1D8)
#define pCurThing_ADDR (0x0073C2D8)
#define rdModel3_lightingMode_ADDR (0x0073C2DC)
#define rdModel3_geometryMode_ADDR (0x0073C2E0)
#define rdModel3_numDrawnModels_ADDR (0x0073C2E4)
#define pModel3Loader_ADDR (0x0073C2E8)
#define pModel3Unloader_ADDR (0x0073C2EC)
#define rdModel3_numGeoLights_ADDR (0x0073C2F0)
#define rdModel3_numMeshLights_ADDR (0x0073C2F4)
#define rdModel3_fRadius_ADDR (0x0073C2F8)
#define sithPuppet_pClassHashtable_ADDR (0x00847E88)
#define sithPuppet_pKeyHashtable_ADDR (0x00847E8C)
#define sithPuppet_pHashtblSubmodes_ADDR (0x00847E90)
#define pKeyframeLoader_ADDR (0x73D608)
#define pKeyframeUnloader_ADDR (0x73D60C)
#define sithTime_g_frameTime_ADDR (0x00836C08)
#define sithTime_g_frameTimeFlex_ADDR (0x00836C0C)
#define sithTime_g_fps_ADDR (0x00836C10)
#define sithTime_g_msecGameTime_ADDR (0x00836C14)
#define sithTime_g_secGameTime_ADDR (0x00836C18)
#define sithTime_g_clockTime_ADDR (0x00836C1C)
#define sithTime_msecPauseStartTime_ADDR (0x00836C20)
#define sithTime_g_bPaused_ADDR (0x00836C24)
#define sithRender_texMode_ADDR (0x008EC360)
#define sithRender_renderflags_ADDR (0x008EC364)
#define sithRender_geoMode_ADDR (0x008EC368)
#define sithRender_lightMode_ADDR (0x008EC36C)
#define sithRender_lightingIRMode_ADDR (0x00831988)
#define sithRender_f_83198C_ADDR (0x0083198C)
#define sithRender_f_831990_ADDR (0x00831990)
#define sithRender_bResetCameraAspect_ADDR (0x00831994)
#define sithRender_g_numVisibleSectors_ADDR (0x008318F0)
#define sithRender_numSecorFrustrums_ADDR (0x0082FEE8)
#define sithRender_numThingLights_ADDR (0x00831978)
#define sithRender_numThingSectors_ADDR (0x0082F640)
#define sithRender_numSpritesToDraw_ADDR (0x0082F4B4)
#define sithRender_numRenderedSectors_ADDR (0x0083197C)
#define sithRender_numAlphaAdjoins_ADDR (0x0082F4B8)
#define sithRender_geoThingsDrawn_ADDR (0x00831980)
#define sithRender_nongeoThingsDrawn_ADDR (0x00831984)
#define sithRender_f_82F4B0_ADDR (0x0082F4B0)
#define sithRender_faceView_ADDR (0x0082F648)
#define meshinfo_out_ADDR (0x0082F490)
#define sithRender_pExtraThingRenderFunc_ADDR (0x00831998)
#define sithRender_aThingLights_ADDR (0x0082F668)
#define sithRender_aVisibleSectors_ADDR (0x0082F110)
#define sithRender_aSectorFrustrums_ADDR (0x0082FEF0)
#define sithRender_aThingSectors_ADDR (0x0082FC68)
#define sithRender_aClipVertices_ADDR (0x0082F310)
#define sithRender_aTransformedClipVertices_ADDR (0x0082F4C0)
#define sithRender_aAlphaAdjoins_ADDR (0x008318F8)
#define sithRender_lastRenderTick_ADDR (0x008EE678)
#define aSithSurfaces_ADDR (0x00847E98)
#define sithCamera_g_aCameras_ADDR (0x008EC380)
#define sithCamera_g_bCurCameraSet_ADDR (0x008EE5A0)
#define sithCamera_g_stateFlags_ADDR (0x008EE5A4)
#define sithCamera_g_curCycleCamNum_ADDR (0x008EE5A8)
#define sithCamera_g_vecCameraPosOffset_ADDR (0x008EE5AC)
#define sithCamera_g_vecCameraAngleOffset_ADDR (0x008EE5B8)
#define sithCamera_g_cameraPosDelta_ADDR (0x008EE5C4)
#define sithCamera_g_cameraAngleDelta_ADDR (0x008EE5C8)
#define sithCamera_g_pCurCamera_ADDR (0x0082F104)
#define sithCamera_bStartup_ADDR (0x0082F108)
#define sithCamera_g_orbCamOrient_ADDR (0x008EE5E0)
#define sithCamera_idleCamOrient_ADDR (0x0082F0B8)
#define sithCamera_bOpen_ADDR (0x0082F10C)
#define rdroid_aMipDistances_ADDR (0x00548250)
#define rdroid_frameTrue_ADDR (0x00570390)
#define bRDroidStartup_ADDR (0x00570394)
#define bRDroidOpen_ADDR (0x00570398)
#define rdroid_g_curLightingMode_ADDR (0x00889EA0)
#define rdroid_g_pHS_ADDR (0x00889EA4)
#define rdroid_g_curGeometryMode_ADDR (0x00889EA8)
#define rdroid_curColorEffects_ADDR (0x00889EC0)
#define rdroid_curOcclusionMethod_ADDR (0x00889EE8)
#define rdroid_curZBufferMethod_ADDR (0x00889EEC)
#define rdroid_curProcFaceUserData_ADDR (0x00889EF0)
#define rdroid_curSortingMethod_ADDR (0x00889EF4)
#define rdroid_curAcceleration_ADDR (0x00889EF8)
#define rdroid_curTextureMode_ADDR (0x00889EFC)
#define rdroid_g_curRenderOptions_ADDR (0x00889F00)
#define rdroid_curCullFlags_ADDR (0x00889F04)
#define sithMaterial_pHashtable_ADDR (0x00852F84)
#define sithMaterial_aMaterials_ADDR (0x0088AFD0)
#define sithMaterial_numMaterials_ADDR (0x00852F80)
#define rdCache_aHWSolidTris_ADDR (0x005703A0)
#define rdCache_totalNormalTris_ADDR (0x005753A0)
#define rdCache_aIntensities_ADDR (0x005753A8)
#define rdCache_aVertices_ADDR (0x005953A8)
#define rdCache_totalVerts_ADDR (0x005F53A8)
#define rdCache_aTexVertices_ADDR (0x005F53B0)
#define rdCache_aHWNormalTris_ADDR (0x006353B0)
#define rdCache_totalSolidTris_ADDR (0x0063A3B0)
#define rdCache_aHWVertices_ADDR (0x0063A3B8)
#define rdCache_drawnFaces_ADDR (0x0073A3B8)
#define rdCache_numUsedVertices_ADDR (0x0073A3BC)
#define rdCache_numUsedTexVertices_ADDR (0x0073A3C0)
#define rdCache_numUsedIntensities_ADDR (0x0073A3C4)
#define rdCache_ulcExtent_ADDR (0x0086EE70)
#define rdCache_lrcExtent_ADDR (0x0086EE78)
#define rdCache_numProcFaces_ADDR (0x0086EE80)
#define rdCache_aProcFaces_ADDR (0x0086EEA0)
#define rdCache_dword_865258_ADDR (0x00865258)
#define sithMulti_name_ADDR (0x008C4BE0)
#define sithMulti_pfNewPlayerJoinedCallback_ADDR (0x0083264C)
#define sithMulti_multiModeFlags_ADDR (0x00832650)
#define sithMulti_numRemovedStaticThings_ADDR (0x0083265C)
#define sithMulti_aRemovedStaticThings_ADDR (0x00832218)
#define sithMulti_msecQuitGameTime_ADDR (0x00832658)
#define sithMulti_msecPingStartTime_ADDR (0x00832654)
#define sithMulti_msecLastSyncScoreTime_ADDR (0x00832660)
#define sithMulti_curWelcomePlayerNum_ADDR (0x00832618)
#define sithMulti_msecWelcomeUpdateInterval_ADDR (0x00832664)
#define rdColormap_pCurMap_ADDR (0x0073A3C8)
#define rdColormap_pIdentityMap_ADDR (0x0073A3CC)
#define sithEvent_aEvents_ADDR (0x00852F98)
#define sithEvent_g_pFirstQueuedEvent_ADDR (0x00855000)
#define sithEvent_arrLut_ADDR (0x00854B98)
#define sithEvent_aTasks_ADDR (0x00854F98)
#define sithEvent_numFreeEventBuffers_ADDR (0x00854FFC)
#define sithEvent_bInit_ADDR (0x855004)
#define sithEvent_bOpen_ADDR (0x855008)
#define rdClip_g_faceStatus_ADDR (0x0082EE5C)
#define rdClip_pSourceVert_ADDR (0x0082EC50)
#define rdClip_workIVerts_ADDR (0x0082EC58)
#define rdClip_aWorkVerts_ADDR (0x0082ECD8)
#define rdClip_pDestVert_ADDR (0x0082EB44)
#define rdClip_pDestIVert_ADDR (0x0082EB48)
#define rdClip_aWorkTVerts_ADDR (0x0082EB50)
#define rdClip_pSourceIVert_ADDR (0x0082EB38)
#define rdClip_pSourceTVert_ADDR (0x0082EB3C)
#define rdClip_pDestTVert_ADDR (0x0082EB40)
#define sithSoundMixer_dword_835FCC_ADDR (0x00835FCC)
#define sithSoundMixer_bPlayingMci_ADDR (0x00835FD0)
#define sithSoundMixer_bInitted_ADDR (0x00835FD4)
#define sithSoundMixer_flt_835FD8_ADDR (0x00835FD8)
#define sithSoundMixer_bIsMuted_ADDR (0x00835FDC)
#define sithSoundMixer_musicVolume_ADDR (0x0054A678)
#define sithSoundMixer_globalVolume_ADDR (0x0054A67C)
#define sithSoundMixer_numSoundsAvailable2_ADDR (0x00835FE0)
#define sithSoundMixer_numSoundsAvailable_ADDR (0x00835FE4)
#define sithSoundMixer_aPlayingSounds_ADDR (0x00835FE8)
#define sithSoundMixer_aIdk_ADDR (0x00836B68)
#define sithSoundMixer_activeChannels_ADDR (0x00836BE8)
#define sithSoundMixer_bOpened_ADDR (0x00836BEC)
#define sithSoundMixer_pCurSector_ADDR (0x00836BF4)
#define sithSoundMixer_dword_836BFC_ADDR (0x00836BFC)
#define sithSoundMixer_trackFrom_ADDR (0x008BBC74)
#define sithSoundMixer_trackTo_ADDR (0x008BBC70)
#define sithSoundMixer_hCurAmbientChannel_ADDR (0x00836BF8)
#define sithSoundMixer_dword_836C00_ADDR (0x00836C00)
#define sithSoundMixer_channelGUIDSeed_ADDR (0x0054A684)
#define sithSoundMixer_dword_836C04_ADDR (0x00836C04)
#define sithSoundMixer_pFocusedThing_ADDR (0x00836BF0)
#define sithSound_maxDataLoaded_ADDR (0x0054C390)
#define sithSound_var3_ADDR (0x0054C394)
#define sithSound_curDataLoaded_ADDR (0x0084DF30)
#define sithSound_bInit_ADDR (0x0084DF34)
#define sithSound_hashtable_ADDR (0x0084DF38)
#define sithSound_var4_ADDR (0x0084DF40)
#define sithSound_var5_ADDR (0x0084DF44)
#define sithControl_inputFuncToControlType_ADDR (0x00833A20)
#define sithControl_aInputFuncToKeyinfo_ADDR (0x00833B48)
#define sithControl_msIdle_ADDR (0x00835830)
#define sithControl_bInitted_ADDR (0x00835834)
#define sithControl_bOpened_ADDR (0x00835838)
#define sithControl_aHandlers_ADDR (0x00833A00)
#define sithControl_numHandlers_ADDR (0x008339F8)
#define sithControl_death_msgtimer_ADDR (0x0083583C)
#define sithControl_vec3_54A570_ADDR (0x0054A570)
#define sithControl_flt_54A57C_ADDR (0x0054A57C)
#define sithGamesave_func1_ADDR (0x00835910)
#define sithGamesave_func2_ADDR (0x00835908)
#define sithGamesave_func3_ADDR (0x00835904)
#define sithGamesave_funcWrite_ADDR (0x00835F44)
#define sithGamesave_funcRead_ADDR (0x0083590C)
#define sithGamesave_autosave_fname_ADDR (0x008BBC80)
#define sithGamesave_state_ADDR (0x00835900)
#define sithGamesave_dword_835914_ADDR (0x00835914)
#define sithGamesave_aCurFilename_ADDR (0x00835F48)
#define sithGamesave_wsaveName_ADDR (0x008BBD00)
#define sithGamesave_saveName_ADDR (0x008BBF00)
#define sithGamesave_headerTmp_ADDR (0x00835918)
#define rdCamera_g_pCurCamera_ADDR (0x0073A3D0)
#define rdCamera_g_camMatrix_ADDR (0x0086EE40)
#define sithNet_MultiModeFlags_ADDR (0x008C4BA0)
#define sithNet_scorelimit_ADDR (0x008C4BAC)
#define sithNet_teamScore_ADDR (0x008C4BC0)
#define sithNet_multiplayer_timelimit_ADDR (0x008C4BD4)
#define sithMulti_multiplayerTimelimit_ADDR (0x0083261C)
#define sithNet_isMulti_ADDR (0x00832624)
#define sithNet_isServer_ADDR (0x00832628)
#define sithMulti_bTimelimitMet_ADDR (0x00832638)
#define sithNet_serverNetId_ADDR (0x008C4BA4)
#define sithNet_things_ADDR (0x008326AC)
#define sithNet_thingsIdx_ADDR (0x008330F0)
#define sithNet_syncIdx_ADDR (0x008330F4)
#define sithNet_aSyncFlags_ADDR (0x008330B0)
#define sithNet_aSyncThings_ADDR (0x00832668)
#define sithNet_tickrate_ADDR (0x005490D4)
#define sithNet_dword_8C4BA8_ADDR (0x008C4BA8)
#define sithNet_dword_83262C_ADDR (0x0083262C)
#define sithMulti_quitGameState_ADDR (0x0083263C)
#define sithNet_checksum_ADDR (0x00832630)
#define sithNet_bNeedsFullThingSyncForLeaveJoin_ADDR (0x00832640)
#define sithMulti_newPlayerId_ADDR (0x00832644)
#define sithNet_bSyncScores_ADDR (0x00832648)
#define sithNet_dword_832620_ADDR (0x00832620)
#define sithOverlayMap_curScale_ADDR (0x0054A5E0)
#define sithOverlayMap_mapOrient_ADDR (0x00835840)
#define sithOverlayMap_pLocalPlayer_ADDR (0x00835870)
#define sithOverlayMap_pCanvas_ADDR (0x00835874)
#define sithOverlayMap_x1_ADDR (0x00835878)
#define sithOverlayMap_y1_ADDR (0x0083587C)
#define sithOverlayMap_inst_ADDR (0x00835880)
#define sithOverlayMap_bMapVisible_ADDR (0x008358C4)
#define sithOverlayMap_bOpened_ADDR (0x008358C8)
#define sithSoundClass_pHashtblModes_ADDR (0x00847F1C)
#define sithSoundClass_pHashTable_ADDR (0x00847F20)
#define sithTemplate_pMasterFile_ADDR (0x8BBC60)
#define sithTemplate_pHashtable_ADDR (0x008BBC64)
#define sithTemplate_masterFileCount_ADDR (0x8BBC68)
#define sithTemplate_pMasterHashtable_ADDR (0x8BBC6C)
#define activeEdgeTail_ADDR (0x0082D688)
#define activeEdgeHead_ADDR (0x0073E618)
#define apNewActiveEdges_ADDR (0x0073D610)
#define apRemoveActiveEdges_ADDR (0x00754680)
#define yMinEdge_ADDR (0x0073E610)
#define yMaxEdge_ADDR (0x0073E670)
#define numActiveSpans_ADDR (0x0073E674)
#define numActiveFaces_ADDR (0x0082D684)
#define numActiveEdges_ADDR (0x0082D680)
#define aActiveEdges_ADDR (0x00755680)
#define rdActive_drawnFaces_ADDR (0x0082D6E0)
#define sithMain_bEndLevel_ADDR (0x0082F0A8)
#define sith_bStartup_ADDR (0x0082F0AC)
#define sith_bOpen_ADDR (0x0082F0B0)
#define sithSurface_numAvail_ADDR (0x0084DF48)
#define sithSurface_aAvail_ADDR (0x0084DF4C)
#define sithSurface_numSurfaces_ADDR (0x0084E350)
#define sithSurface_aSurfaces_ADDR (0x0084E358)
#define sithSurface_bOpened_ADDR (0x00852F58)
#define sithSurface_byte_8EE668_ADDR (0x008EE668)
#define sithSurface_numUnsyncedSurfaces_ADDR (0x00847F18)
#define pMaterialsLoader_ADDR (0x73D600)
#define pMaterialsUnloader_ADDR (0x73D604)
#define sithSprite_pHashtable_ADDR (0x00852F90)
#define Window_drawAndFlip_ADDR (0x00855E9C)
#define Window_setCooperativeLevel_ADDR (0x00855EA0)
#define Window_ext_handlers_ADDR (0x00855DF0)
#define Window_aDialogHwnds_ADDR (0x00855DA0)
#define DebugGui_aIdk_ADDR (0x008BBF80)
#define DebugGui_idk_ADDR (0x008BC000)
#define DebugGui_some_line_amt_ADDR (0x008BC004)
#define DebugGui_some_num_lines_ADDR (0x008BC008)
#define DebugLog_buffer_ADDR (0x008BC020)
#define sithConsole_aCmds_ADDR (0x008358D0)
#define sithConsole_pCmdHashtable_ADDR (0x008358D4)
#define sithConsole_bOpened_ADDR (0x008358D8)
#define sithConsole_bInitted_ADDR (0x008358DC)
#define sithConsole_maxCmds_ADDR (0x008358E4)
#define sithConsole_numRegisteredCmds_ADDR (0x008358E8)
#define DebugGui_maxLines_ADDR (0x008358EC)
#define DebugGui_fnPrint_ADDR (0x008358F0)
#define DebugGui_fnPrintUniStr_ADDR (0x008358F4)
#define sithConsole_alertSound_ADDR (0x008358F8)
#define sithConsole_idk2_ADDR (0x008358FC)
#define d3d_maxVertices_ADDR (0x0055C7F0)
#define d3d_device_ptr_ADDR (0x0055C7DC)
#define std3D_rectViewIdk_ADDR (0x00865320)
#define std3D_aViewIdk_ADDR (0x008652A0)
#define std3D_aViewTris_ADDR (0x00865260)
#define std3D_gpuMaxTexSizeMaybe_ADDR (0x0053D668)
#define std3D_dword_53D66C_ADDR (0x0053D66C)
#define std3D_dword_53D670_ADDR (0x0053D670)
#define std3D_dword_53D674_ADDR (0x0053D674)
#define std3D_frameCount_ADDR (0x0053d678)
#define std3D_renderList_ADDR (0x0055C7D0)
#define std3D_numCachedTextures_ADDR (0x0055C7F4)
#define std3D_pFirstTexCache_ADDR (0x0055C7F8)
#define std3D_pLastTexCache_ADDR (0x0055C7Fc)
#define stdComm_dword_8321F8_ADDR (0x008321F8)
#define stdComm_bInitted_ADDR (0x008321D8)
#define stdComm_dword_8321F0_ADDR (0x008321F0)
#define stdComm_dword_8321F4_ADDR (0x008321F4)
#define stdComm_dplayIdSelf_ADDR (0x008321EC)
#define stdComm_dword_832204_ADDR (0x00832204)
#define stdComm_dword_832208_ADDR (0x00832208)
#define stdComm_currentBigSyncStage_ADDR (0x0083220C)
#define stdComm_dword_8321E0_ADDR (0x008321E0)
#define stdComm_bIsServer_ADDR (0x008321E4)
#define stdComm_dword_8321E8_ADDR (0x008321E8)
#define stdComm_waIdk_ADDR (0x008C4C60)
#define stdComm_dword_8321DC_ADDR (0x008321DC)
#define stdComm_dword_832200_ADDR (0x00832200)
#define stdComm_dword_832210_ADDR (0x00832210)
#define stdComm_cogMsgTmp_ADDR (0x00008319B0)
#define stdMci_mciId_ADDR (0x00563720)
#define stdMci_dwVolume_ADDR (0x00563724)
#define stdMci_bInitted_ADDR (0x00563728)
#define stdMci_uDeviceID_ADDR (0x00548118)
#define wuRegistry_bStarted_ADDR (0x00855EA4)
#define wuRegistry_lpClass_ADDR (0x00855EB0)
#define wuRegistry_byte_855EB4_ADDR (0x00855EB4)
#define wuRegistry_hKey_ADDR (0x00855EAC)
#define wuRegistry_lpSubKey_ADDR (0x00855EA8)
#define WinIdk_aDplayGuid_ADDR (0x008607F0)
#define stdDisplay_aDisplayDevices_ADDR (0x00868280)
#define stdDisplay_gammaTableLen_ADDR (0x055B410)
#define stdDisplay_paGammaTable_ADDR (0x055B414)
#define stdDisplay_gammaPalette_ADDR (0x0055ADB8)
#define stdDisplay_pCurDevice_ADDR (0x0055B3E8)
#define stdDisplay_pCurVideoMode_ADDR (0x0055B3F0)
#define stdDisplay_bStartup_ADDR (0x0055B3D8)
#define stdDisplay_bOpen_ADDR (0x0055B3DC)
#define stdDisplay_bModeSet_ADDR (0x0055B3E0)
#define stdDisplay_numVideoModes_ADDR (0x0055B3EC)
#define stdDisplay_bPaged_ADDR (0x0055B3F4)
#define stdDisplay_tmpGammaPal_ADDR (0x0055B0C0)
#define stdDisplay_gammaIdx_ADDR (0x0055B40C)
#define word_860800_ADDR (0x860800)
#define word_860802_ADDR (0x860802)
#define word_860804_ADDR (0x860804)
#define word_860806_ADDR (0x860806)
#define stdControl_bReadMouse_ADDR (0x0053EEE0)
#define stdControl_updateKHz_ADDR (0x0053EEE4)
#define stdControl_updateHz_ADDR (0x0053EEE8)
#define stdControl_mouseXSensitivity_ADDR (0x0053EEEC)
#define stdControl_mouseYSensitivity_ADDR (0x0053EEF0)
#define stdControl_mouseZSensitivity_ADDR (0x0053EEF4)
#define stdControl_aAxisEnabled_ADDR (0x0055C828)
#define stdControl_aAxisStates_ADDR (0x0055C830)
#define stdControl_aKeyIdleTimes_ADDR (0x0055C870)
#define stdControl_aKeyInfo_ADDR (0x0055CCE0)
#define stdControl_aAxisConnected_ADDR (0x0055D150)
#define stdControl_aKeyPressed_ADDR (0x0055D158)
#define stdControl_bStartup_ADDR (0x0055D5C8)
#define stdControl_bOpen_ADDR (0x0055D5CC)
#define stdControl_bDisableKeyboard_ADDR (0x0055D5D0)
#define stdControl_bControlsIdle_ADDR (0x0055D5D4)
#define stdControl_bControlsActive_ADDR (0x0055D5D8)
#define stdControl_pDI_ADDR (0x0055D5DC)
#define stdControl_mouseDirectInputDevice_ADDR (0x0055D5E0)
#define stdControl_keyboardIDirectInputDevice_ADDR (0x0055D5E4)
#define stdControl_bReadJoysticks_ADDR (0x0055D5E8)
#define stdControl_curReadTime_ADDR (0x0055D5EC)
#define stdControl_lastReadTime_ADDR (0x0055D5F0)
#define stdControl_readDeltaTime_ADDR (0x0055D5F8)
#define stdControl_dwLastMouseX_ADDR (0x0055D5FC)
#define stdControl_dwLastMouseY_ADDR (0x0055D600)
#define stdControl_aJoystickEnabled_ADDR (0x00865028)
#define stdControl_aJoystickExists_ADDR (0x00865030)
#define stdControl_aAxes_ADDR (0x00865040)
#define stdControl_aJoystickMaxButtons_ADDR (0x008651A8)
#define Windows_installType_ADDR (0x008606E0)
#define Video_aGammaTable_ADDR (0x005252f8)
#define Video_fillColor_ADDR (0x00552898)
#define Video_modeStruct_ADDR (0x008605C0)
#define Video_otherBuf_ADDR (0x00866CA0)
#define Video_dword_866D78_ADDR (0x00866D78)
#define Video_curMode_ADDR (0x00866D7C)
#define Video_renderSurface_ADDR (0x00866D80)
#define Video_menuBuffer_ADDR (0x0086AC00)
#define Video_pOtherBuf_ADDR (0x00552888)
#define Video_pMenuBuffer_ADDR (0x0055288C)
#define Video_bInitted_ADDR (0x005528B4)
#define Video_bOpened_ADDR (0x005528B8)
#define Video_flt_55289C_ADDR (0x0055289C)
#define Video_dword_5528A0_ADDR (0x005528A0)
#define Video_dword_5528A4_ADDR (0x005528A4)
#define Video_dword_5528A8_ADDR (0x005528A8)
#define Video_lastTimeMsec_ADDR (0x005528AC)
#define Video_dword_5528B0_ADDR (0x005528B0)
#define Video_pVbufIdk_ADDR (0x00552890)
#define Video_pCanvas_ADDR (0x00552894)
#define Video_aPalette_ADDR (0x00860140)
#define Video_format_ADDR (0x008600E0)
#define Video_format2_ADDR (0x0085FF60)
#define Video_modeStruct2_ADDR (0x0085FFC0)
#define Video_bufIdk_ADDR (0x00866120)
#define stdConsole_textAttribute_ADDR (0x0055BB68)
#define stdConsole_wAttributes_ADDR (0x0055BB6C)
#define stdConsole_cursorHidden_ADDR (0x0055BB70)
#define stdConsole_ConsoleCursorInfo_ADDR (0x0055BB78)
#define stdConsole_hOutput_ADDR (0x00866860)
#define stdConsole_hInput_ADDR (0x00866864)
#define rdRaster_aOneOverNFixed_ADDR (0x0086ADE0)
#define rdRaster_aLerpAndScale_ADDR (0x86CDDC)
#define rdRaster_aOneOverNFlex_ADDR (0x0086CE20)
#define rdRaster_fixedScale_ADDR (0x00548E28)
#define sithCogFunctionAI_aThingsInView_ADDR (0x00855CE8)
#define sithCogFunctionAI_numThingsInView_ADDR (0x00855D68)
#define sithCogFunctionAI_curThingInView_ADDR (0x00855D6C)
#define sithComm_MsgTmpBuf_ADDR (0x00837468)
#define sithComm_aMsgPairs_ADDR (0x00847968)
#define sithMessage_aTypeFuncs_ADDR (0x00847D68)
#define sithMessage_bStopProcessMessages_ADDR (0x00847E6C)
#define sithMessage_g_outputstream_ADDR (0x00847E70)
#define sithMessage_g_inputstream_ADDR (0x00847E74)
#define sithComm_idk2_ADDR (0x00847E7C)
#define sithMessage_bSturtup_ADDR (0x00847E80)
#define sithComm_dword_847E84_ADDR (0x00847E84)
#define sithComm_msgId_ADDR (0x0054B004)
#define sithComm_MsgTmpBuf2_ADDR (0x00836C40)
#define sithComm_netMsgTmp_ADDR (0x008B4C00)
#define jkCog_emptystring_ADDR (0x005540BC)
#define jkCog_jkstring_ADDR (0x00553FB8)
#define jkCog_bInitted_ADDR (0x00553FB0)
#define jkCog_strings_ADDR (0x0553FA0)
#define sithCog_bOpened_ADDR (0x00836C2C)
#define sithCog_g_pHashtable_ADDR (0x00836C3C)
#define sithCog_aSectorLinks_ADDR (0x008B5440)
#define sithCog_numSectorLinks_ADDR (0x00836C38)
#define sithCog_aThingLinks_ADDR (0x008B7460)
#define sithCog_numThingLinks_ADDR (0x00836C30)
#define sithCog_numSurfaceLinks_ADDR (0x00836C34)
#define sithCog_aSurfaceLinks_ADDR (0x008B9C60)
#define sithCog_g_pMasterCog_ADDR (0x008B542C)
#define jkDev_aEntryPositions_ADDR (0x00559EF0)
#define jkDev_aEntries_ADDR (0x00559F40)
#define jkDev_log_55A4A4_ADDR (0x0055A4A4)
#define jkDev_bScreenNeedsUpdate_ADDR (0x0055A4A8)
#define jkDev_aCheatCmds_ADDR (0x0055A4B0)
#define jkDev_numCheats_ADDR (0x0055A9B0)
#define jkDev_bInitted_ADDR (0x0055A9B4)
#define jkDev_bOpened_ADDR (0x0055A9B8)
#define jkDev_cheatHashtable_ADDR (0x0055A9BC)
#define jkDev_hDlg_ADDR (0x0055A9C0)
#define jkDev_vbuf_ADDR (0x0055A9C4)
#define jkDev_BMFontHeight_ADDR (0x0055A9C8)
#define jkDev_ColorKey_ADDR (0x0055A9CC)
#define jkDev_dword_55A9D0_ADDR (0x0055A9D0)
#define jkDev_amt_ADDR (0x0055A9D4)
#define jkSmack_gameMode_ADDR (0x00552B78)
#define jkSmack_bInit_ADDR (0x00552B7C)
#define jkSmack_stopTick_ADDR (0x00552B84)
#define jkSmack_currentGuiState_ADDR (0x00552B88)
#define jkSmack_nextGuiState_ADDR (0x00552B8C)
#define jkSmack_alloc_ADDR (0x00552B74)
#define jkEpisode_aEpisodes_ADDR (0x8568E0)
#define jkEpisode_var4_ADDR (0x005540D8)
#define jkEpisode_var5_ADDR (0x00554158)
#define jkEpisode_var2_ADDR (0x005541D8)
#define jkEpisode_mLoad_ADDR (0x00552B68)
#define jkHud_targetRed_ADDR (0x00525960)
#define jkHud_targetBlue_ADDR (0x00525964)
#define jkHud_targetGreen_ADDR (0x00525968)
#define jkHud_aFltIdk_ADDR (0x00525970)
#define jkHud_aColors8bpp_ADDR (0x00525988)
#define jkHud_aTeamColors8bpp_ADDR (0x00525A48)
#define jkHud_chatStr_ADDR (0x00552BD8)
#define jkHud_aTeamColors16bpp_ADDR (0x00552C58)
#define jkHud_rightBlitX_ADDR (0x00552C6C)
#define jkHud_leftBlitX_ADDR (0x00552C70)
#define jkHud_mapRendConfig_ADDR (0x00552C78)
#define jkHud_chatStrPos_ADDR (0x00552CB8)
#define jkHud_rightBlitY_ADDR (0x00552CBC)
#define jkHud_aTeamScores_ADDR (0x00552CC0)
#define jkHud_dword_552D10_ADDR (0x00552D10)
#define jkHud_aColors16bpp_ADDR (0x00552D18)
#define jkHud_aPlayerScores_ADDR (0x00552D30)
#define jkHud_blittedAmmoAmt_ADDR (0x00553E30)
#define jkHud_idk14_ADDR (0x00553E34)
#define jkHud_blittedHealthIdx_ADDR (0x00553E38)
#define jkHud_blittedBatteryAmt_ADDR (0x00553E3C)
#define jkHud_blittedFieldlightAmt_ADDR (0x00553E40)
#define jkHud_blittedShieldIdx_ADDR (0x00553E44)
#define jkHud_isSuper_ADDR (0x00553E48)
#define jkHud_idk15_ADDR (0x00553E4C)
#define jkHud_blittedForceIdx_ADDR (0x00553E50)
#define jkHud_idk16_ADDR (0x00553E54)
#define jkHud_leftBlitY_ADDR (0x00553E58)
#define jkHud_rectViewScores_ADDR (0x00553E60)
#define jkHud_pMsgFontSft_ADDR (0x00553E70)
#define jkHud_pStatusLeftBm_ADDR (0x00553E74)
#define jkHud_pStatusRightBm_ADDR (0x00553E78)
#define jkHud_bHasTarget_ADDR (0x00553E7C)
#define jkHud_pTargetThing_ADDR (0x00553E80)
#define jkHud_targetRed16_ADDR (0x00553E88)
#define jkHud_targetGreen16_ADDR (0x00553E8C)
#define jkHud_targetBlue16_ADDR (0x00553E90)
#define jkHud_bChatOpen_ADDR (0x00553E94)
#define jkHud_pHelthNumSft_ADDR (0x00553E98)
#define jkHud_pAmoNumsSft_ADDR (0x00553E9C)
#define jkHud_pAmoNumsSuperSft_ADDR (0x00553EA0)
#define jkHud_pArmorNumSft_ADDR (0x00553EA4)
#define jkHud_pArmorNumsSuperSft_ADDR (0x00553EA8)
#define jkHud_bInitted_ADDR (0x00553EAC)
#define jkHud_bOpened_ADDR (0x00553EB0)
#define jkHud_pFieldlightBm_ADDR (0x00553EB4)
#define jkHud_pStBatBm_ADDR (0x00553EB8)
#define jkHud_pStHealthBm_ADDR (0x00553EBC)
#define jkHud_pStShieldBm_ADDR (0x00553EC0)
#define jkHud_pStFrcBm_ADDR (0x00553EC4)
#define jkHud_pStFrcSuperBm_ADDR (0x00553EC8)
#define jkHud_bViewScores_ADDR (0x00553ECC)
#define jkHud_dword_553ED0_ADDR (0x00553ED0)
#define jkHud_tallyWhich_ADDR (0x00553ED4)
#define jkHud_numPlayers_ADDR (0x00553ED8)
#define jkHud_dword_553EDC_ADDR (0x00553EDC)
#define jkHud_dword_553EE0_ADDR (0x00553EE0)
#define jkHudInv_info_ADDR (0x00553EE8)
#define jkHudInv_itemTexfmt_ADDR (0x00553F28)
#define jkHudInv_flags_ADDR (0x00553F60)
#define jkHudInv_dword_553F64_ADDR (0x00553F64)
#define jkHudInv_scroll_ADDR (0x00553F68)
#define jkHudInv_aBitmaps_ADDR (0x00553F80)
#define jkHudInv_font_ADDR (0x00553F8C)
#define jkHudInv_rend_isshowing_maybe_ADDR (0x00553F90)
#define jkHudInv_dword_553F94_ADDR (0x00553F94)
#define jkHudInv_numItems_ADDR (0x00553F98)
#define jkHudInv_aItems_ADDR (0x00553F9C)
#define Main_bDevMode_ADDR (0x00860780)
#define Main_bDisplayConfig_ADDR (0x00860784)
#define Main_bWindowGUI_ADDR (0x00860788)
#define Main_dword_86078C_ADDR (0x86078C)
#define Main_bFrameRate_ADDR (0x00860790)
#define Main_bDispStats_ADDR (0x00860794)
#define Main_bNoHUD_ADDR (0x00860798)
#define Main_logLevel_ADDR (0x0086079C)
#define Main_verboseLevel_ADDR (0x008607A0)
#define Main_path_ADDR (0x008606E4)
#define debug_log_fp_ADDR (0x00552880)
#define pHS_ADDR (0x860440)
#define jkCredits_aPalette_ADDR (0x0055AA60)
#define jkCredits_pVbuffer2_ADDR (0x0055AD60)
#define jkCredits_dword_55AD64_ADDR (0x0055AD64)
#define jkCredits_dword_55AD68_ADDR (0x0055AD68)
#define jkCredits_table_ADDR (0x0055AD70)
#define jkCredits_startMs_ADDR (0x0055AD80)
#define jkCredits_dword_55AD84_ADDR (0x0055AD84)
#define jkCredits_strIdx_ADDR (0x0055AD88)
#define jkCredits_aIdk_ADDR (0x0055AD8C)
#define jkCredits_pVbuffer_ADDR (0x0055AD90)
#define jkCredits_dword_55AD94_ADDR (0x0055AD94)
#define jkCredits_fontLarge_ADDR (0x0055AD98)
#define jkCredits_fontSmall_ADDR (0x0055AD9C)
#define jkCredits_dword_55ADA0_ADDR (0x0055ADA0)
#define jkCredits_bInitted_ADDR (0x0055ADA4)
#define jkCredits_dword_55ADA8_ADDR (0x0055ADA8)
#define g_sithMode_ADDR (0x8EE660)
#define g_submodeFlags_ADDR (0x8EE664)
#define g_debugmodeFlags_ADDR (0x8EE66C)
#define g_mapModeFlags_ADDR (0x8EE674)
#define jkGame_gamma_ADDR (0x008606A4)
#define jkGame_screenSize_ADDR (0x008605F0)
#define jkGame_bInitted_ADDR (0x005528BC)
#define jkGame_updateMsecsTotal_ADDR (0x00552B58)
#define jkGame_dword_552B5C_ADDR (0x00552B5C)
#define jkGame_isDDraw_ADDR (0x00552B60)
#define jkRes_pHS_ADDR (0x00555C68)
#define jkRes_episodeGobName_ADDR (0x00555C70)
#define jkRes_curDir_ADDR (0x00555C90)
#define jkRes_bHookedHS_ADDR (0x00555D18)
#define jkRes_aFiles_ADDR (0x00554A68)
#define jkRes_gCtx_ADDR (0x005541E0)
#define pLowLevelHS_ADDR (0x00554974)
#define lowLevelHS_ADDR (0x00554978)
#define jkRes_idkGobPath_ADDR (0x005549E8)
#define jkCutscene_rect1_ADDR (0x0055AA18)
#define jkCutscene_rect2_ADDR (0x0055AA38)
#define jkCutscene_strings_ADDR (0x0055AA28)
#define jkCutscene_subtitlefont_ADDR (0x0055AA4C)
#define jkCutscene_bInitted_ADDR (0x0055AA58)
#define jkCutscene_isRendering_ADDR (0x0055AA48)
#define jkCutscene_dword_55B750_ADDR (0x0055B750)
#define jkCutscene_dword_55AA50_ADDR (0x0055AA50)
#define jkCutscene_55AA54_ADDR (0x0055AA54)
#define jkMain_aLevelJklFname_ADDR (0x005528D0)
#define thing_nine_ADDR (0x0052552C)
#define jkMain_bInit_ADDR (0x00552B80)
#define thing_six_ADDR (0x00552B90)
#define thing_eight_ADDR (0x00552B94)
#define jkMain_dword_552B98_ADDR (0x00552B98)
#define jkMain_lastTickMs_ADDR (0x00552B9C)
#define idx_13b4_related_ADDR (0x008C4BD8)
#define gamemode_1_str_ADDR (0x00552958)
#define jkMain_strIdk_ADDR (0x005529D8)
#define jkMain_wstrIdk_ADDR (0x00552A58)
#define sithWorld_aSectionParsers_ADDR (0x00833548)
#define sithWorld_some_integer_4_ADDR (0x008339E0)
#define sithWorld_g_pCurrentWorld_ADDR (0x8339C8)
#define sithWorld_g_pStaticWorld_ADDR (0x8339CC)
#define sithWorld_g_pLastLoadedWorld_ADDR (0x8339D0)
#define sithWorld_numParsers_ADDR (0x8339D4)
#define sithWorld_bInitted_ADDR (0x8339D8)
#define sithWorld_bLoaded_ADDR (0x008339DC)
#define sithWorld_episodeName_ADDR (0x008EE620)
#define sithInventory_powerKeybinds_ADDR (0x008BD0C0)
#define sithInventory_g_bInitInventory_ADDR (0x00549FA0)
#define sithInventory_g_aTypes_ADDR (0x008BD1C0)
#define sithInventory_g_bSendDeactivateMessage_ADDR (0x008339E4)
#define sithInventory_bUnkPower_ADDR (0x008339E8)
#define sithInventory_8339EC_ADDR (0x008339EC)
#define sithInventory_bRendIsHidden_ADDR (0x008339F0)
#define sithInventory_8339F4_ADDR (0x008339F4)
#define sithWeapon_controlOptions_ADDR (0x008BD020)
#define g_flt_8BD040_ADDR (0x008BD040)
#define g_flt_8BD044_ADDR (0x008BD044)
#define g_flt_8BD048_ADDR (0x008BD048)
#define g_flt_8BD04C_ADDR (0x008BD04C)
#define g_flt_8BD050_ADDR (0x008BD050)
#define g_flt_8BD054_ADDR (0x008BD054)
#define g_flt_8BD058_ADDR (0x008BD058)
#define sithWeapon_CurWeaponMode_ADDR (0x008BD064)
#define sithWeapon_bAutoPickup_ADDR (0x008BD080)
#define sithWeapon_bAutoSwitch_ADDR (0x008BD084)
#define sithWeapon_bAutoReload_ADDR (0x008BD088)
#define sithWeapon_bMultiAutoPickup_ADDR (0x008BD08C)
#define sithWeapon_bMultiplayerAutoSwitch_ADDR (0x008BD090)
#define sithWeapon_bMultiAutoReload_ADDR (0x008BD094)
#define sithWeapon_bAutoAim_ADDR (0x008BD098)
#define sithWeapon_secMountWait_ADDR (0x008BD09C)
#define sithWeapon_8BD0A0_ADDR (0x008BD0A0)
#define sithWeapon_fireWait_ADDR (0x008BD0A8)
#define sithWeapon_fireRate_ADDR (0x008BD0AC)
#define sithWeapon_LastFireTimeSecs_ADDR (0x008BD0B0)
#define sithWeapon_a8BD030_ADDR (0x008BD030)
#define sithWeapon_8BD05C_ADDR (0x008BD05C)
#define sithWeapon_8BD060_ADDR (0x008BD060)
#define sithWeapon_8BD008_ADDR (0x008BD008)
#define sithWeapon_8BD024_ADDR (0x008BD024)
#define sithWeapon_senderIndex_ADDR (0x008BD028)
#define jkPlayer_playerInfos_ADDR (0x008C4CA0)
#define jkPlayer_playerShortName_ADDR (0x08EC320)
#define jkPlayer_numOtherThings_ADDR (0x0085FF44)
#define jkPlayer_numThings_ADDR (0x0085B560)
#define jkPlayer_otherThings_ADDR (0x008592E0)
#define jkPlayer_bLoadingSomething_ADDR (0x00525470)
#define playerThingIdx_ADDR (0x83199C)
#define jkPlayer_maxPlayers_ADDR (0x8319A0)
#define sithPlayer_g_pLocalPlayerThing_ADDR (0x8319A4)
#define sithPlayer_g_pLocalPlayer_ADDR (0x8319A8)
#define playerThings_ADDR (0x85B580)
#define jkSaber_rotateMat_ADDR (0x85FA80)
#define jkPlayer_setRotateOverlayMap_ADDR (0x00860764)
#define jkPlayer_setDrawStatus_ADDR (0x00860768)
#define jkPlayer_setCrosshair_ADDR (0x0086076C)
#define jkPlayer_setSaberCam_ADDR (0x00860770)
#define jkPlayer_setFullSubtitles_ADDR (0x00860774)
#define jkPlayer_setDisableCutscenes_ADDR (0x00860778)
#define jkPlayer_aCutsceneVal_ADDR (0x0085FAC0)
#define jkPlayer_cutscenePath_ADDR (0x0085FB40)
#define jkPlayer_setNumCutscenes_ADDR (0x0085FF40)
#define jkPlayer_currentTickIdx_ADDR (0x8EE640)
#define jkPlayer_setDiff_ADDR (0x008EE670)
#define jkPlayer_waggleVec_ADDR (0x00552BA0)
#define jkPlayer_waggleMag_ADDR (0x00552BAC)
#define jkPlayer_mpcInfoSet_ADDR (0x00552BB0)
#define jkPlayer_waggleAngle_ADDR (0x00552BB4)
#define jkSaber_rotateVec_ADDR (0x00552BB8)
#define jkPlayer_name_ADDR (0x008604C0)
#define jkPlayer_model_ADDR (0x00860500)
#define jkPlayer_soundClass_ADDR (0x00860520)
#define jkPlayer_sideMat_ADDR (0x00860560)
#define jkPlayer_tipMat_ADDR (0x00860580)
#define sithCollision_aNumSearchedSectors_ADDR (0x847F28)
#define sithCollision_collisionHandlers_ADDR (0x00847F38)
#define sithCollision_aThingSurfaceCollideResults_ADDR (0x8485F8)
#define sithCollision_aCollisions_ADDR (0x00848628)
#define sithCollision_aNumStackCollisions_ADDR (0x84DA28)
#define sithCollision_searchStackIdx_ADDR (0x0054BA90)
#define sithCollision_apSearchedSectors_ADDR (0x0084D628)
#define sithCollision_dword_8B4BE4_ADDR (0x008B4BE4)
#define sithCollision_collideHurtIdk_ADDR (0x008B4BF0)
#define sithSector_surfaceNormal_ADDR (0x0054C6E8)
#define sithAIAwareness_aEntries_ADDR (0x00855028)
#define sithAIAwareness_g_aSectors_ADDR (0x008553A8)
#define sithAIAwareness_numEntries_ADDR (0x008553AC)
#define sithAIAwareness_bInitted_ADDR (0x008553B0)
#define sithAIAwareness_timerTicks_ADDR (0x008553B4)
#define sithSector_flt_8553B8_ADDR (0x008553B8)
#define sithSector_horizontalPixelsPerRev_ADDR (0x008553BC)
#define sithSector_flt_8553C0_ADDR (0x008553C0)
#define sithSector_flt_8553C4_ADDR (0x008553C4)
#define sithSector_flt_8553C8_ADDR (0x008553C8)
#define sithSector_zMaxVec_ADDR (0x008553D0)
#define sithSector_ceilingSky_ADDR (0x008553DC)
#define sithSector_zMinVec_ADDR (0x008553E0)
#define sithSector_horizontalPixelsPerRev_idk_ADDR (0x008553EC)
#define sithSector_horizontalDist_ADDR (0x008553F0)
#define sithSector_flt_8553F4_ADDR (0x008553F4)
#define sithSector_aModifiedSectors_ADDR (0x00855438)
#define sithSector_aSyncFlags_ADDR (0x00855478)
#define sithSector_numModifiedSectors_ADDR (0x008554B8)
#define sithThing_pParseHashtbl_ADDR (0x008326A8)
#define sithThing_pfUnknownFunc_ADDR (0x008330FC)
#define sithThing_guidEntropy_ADDR (0x00549358)
#define jkGuiSaveLoad_wtextEpisode_ADDR (0x00559528)
#define jkGuiSaveLoad_wtextHealth_ADDR (0x00559728)
#define jkGuiSaveLoad_numEntries_ADDR (0x005597A8)
#define jkGuiSaveLoad_wtextShields_ADDR (0x005597B0)
#define jkGuiSaveLoad_word_559830_ADDR (0x00559830)
#define jkGuiSaveLoad_bIsSaveMenu_ADDR (0x00559A30)
#define jkGuiSaveLoad_wtextSaveName_ADDR (0x00559A38)
#define jkGuiSaveLoad_DarrayEntries_ADDR (0x00559C38)
#define jkGuiSaveLoad_word_559C54_ADDR (0x00559C54)
#define jkGuiSaveLoad_byte_559C50_ADDR (0x00559C50)
#define jkGuiTitle_verMajor_ADDR (0x00525030)
#define jkGuiTitle_verMinor_ADDR (0x00552884)
#define jkGuiTitle_verRevision_ADDR (0x00525034)
#define jkGuiTitle_aTexts_ADDR (0x005568E0)
#define jkGuiTitle_whichLoading_ADDR (0x00556980)
#define jkGuiSound_sfxVolume_ADDR (0x00547DD0)
#define jkGuiSound_numChannels_ADDR (0x0054A680)
#define jkGuiSound_bLowResSound_ADDR (0x00563700)
#define jkGuiSound_b3DSound_ADDR (0x00563704)
#define jkGuiSound_b3DSound_2_ADDR (0x00563708)
#define jkGuiSound_b3DSound_3_ADDR (0x0056370C)
#define jkGuiSound_musicVolume_ADDR (0x0086077C)
#define jkGui_unkstr_ADDR (0x856880)
#define jkGui_GdiMode_ADDR (0x00556020)
#define jkGui_modesets_ADDR (0x0055603c)
#define jkGui_stdBitmaps_ADDR (0x008567E0)
#define jkGui_stdFonts_ADDR (0x008568A0)
#define jkGui_episodeLoad_ADDR (0x00556028)
#define jkGuiSingleTally_foStars_ADDR (0x00556880)
#define jkGuiNetHost_maxRank_ADDR (0x00860460)
#define jkGuiNetHost_timeLimit_ADDR (0x00860464)
#define jkGuiNetHost_scoreLimit_ADDR (0x00860468)
#define jkGuiNetHost_maxPlayers_ADDR (0x0086046C)
#define jkGuiNetHost_sessionFlags_ADDR (0x00860470)
#define jkGuiNetHost_gameFlags_ADDR (0x00860474)
#define jkGuiNetHost_tickRate_ADDR (0x00860478)
#define jkGuiNetHost_gameName_ADDR (0x0086047C)
#define jkGuiMultiplayer_checksumSeed_ADDR (0x00832634)
#define jkGuiMultiplayer_dword_5564EC_ADDR (0x005564EC)
#define jkGuiMultiplayer_dword_5564E8_ADDR (0x005564E8)
#define jkGuiMultiplayer_multiEntry_ADDR (0x00556050)
#define jkGuiMultiplayer_dword_5564F0_ADDR (0x005564F0)
#define g_hInstance_ADDR (0x00855DEC)
#define sithCog_g_pSymbolTable_ADDR (0x008B5428)
#define pSithHS_ADDR (0x82F0A4)
#define g_hWnd_ADDR (0x855DE0)
#define g_nShowCmd_ADDR (0x855DE8)
#define g_app_suspended_ADDR (0x855E70)
#define g_window_active_ADDR (0x855E74)
#define g_app_active_ADDR (0x855E78)
#define g_should_exit_ADDR (0x855E7C)
#define g_thing_two_some_dialog_count_ADDR (0x855E80)
#define g_handler_count_ADDR (0x855E84)
#define g_855E8C_ADDR (0x855E8C)
#define g_855E90_ADDR (0x855E90)
#define g_window_not_destroyed_ADDR (0x855E94)
#define stdPalEffects_state_ADDR (0x00866200)
#define stdPalEffects_palette_ADDR (0x0055BB80)
#define stdPalEffects_numEffectRequests_ADDR (0x0055BE80)
#define stdPalEffects_aEffects_ADDR (0x00866260)
#define stdPalEffects_setPalette_ADDR (0x0055BE84)
#define stdPalEffects_aPalette_ADDR (0x0055BE88)
#define aFilenameStack_ADDR (0x0055D620)
#define apBufferStack_ADDR (0x0055E020)
#define linenumStack_ADDR (0x0055E070)
#define aEntryStack_ADDR (0x0055E0C0)
#define openFileStack_ADDR (0x00563110)
#define printfBuffer_ADDR (0x00563160)
#define stdConffile_linenum_ADDR (0x00563560)
#define stdConffile_bOpen_ADDR (0x00563564)
#define openFile_ADDR (0x00563568)
#define writeFile_ADDR (0x0056356C)
#define stackLevel_ADDR (0x00563570)
#define stdConffile_aWriteFilename_ADDR (0x860820)
#define stdConffile_g_entry_ADDR (0x8608A0)
#define stdConffile_pFilename_ADDR (0x860CC0)
#define stdConffile_g_aLine_ADDR (0x860D40)
#define stdMemory_bInitted_ADDR (0x00570384)
#define stdMemory_bOpened_ADDR (0x00570388)
#define stdMemory_g_curState_ADDR (0x008607C0)
#define yyin_ADDR (0x00855D90)
#define yyout_ADDR (0x00855D94)
#define sithCogParse_pSymbolTable_ADDR (0x008554C0)
#define yacc_linenum_ADDR (0x00889F0C)
#define cog_yacc_loop_depth_ADDR (0x0054C850)
#define cog_parser_node_stackpos_ADDR (0x008554C8)
#define cogvm_stackpos_ADDR (0x00855CD8)
#define cogparser_nodes_alloc_ADDR (0x00855CD0)
#define cogparser_topnode_ADDR (0x00855CCC)
#define cogvm_stack_ADDR (0x00855CC8)
#define cogparser_num_nodes_ADDR (0x00855CE0)
#define cogparser_current_nodeidx_ADDR (0x00855CDC)
#define parsing_script_ADDR (0x00855CD4)
#define parsing_script_idk_ADDR (0x0054C854)
#define dplay_dword_55D618_ADDR (0x0055D618)
#define dplay_dword_55D61C_ADDR (0x0055D61C)
#define jkGui_guid_556040_ADDR (0x00556040)
#define jkGuiMultiplayer_numConnections_ADDR (0x00865020)
#define jkGuiMultiplayer_aConnections_ADDR (0x008620A0)
#define jkGuiMultiplayer_aEntries_ADDR (0x00863220)
#define jkGuiMultiplayer_stru_556168_ADDR (0x00556168)
#define jkGuiMultiplayer_mpcInfo_ADDR (0x008591E0)
#define jkGuiMultiplayer_stru_5564A8_ADDR (0x005564A8)
#define jkGuiMouse_bOpen_ADDR (0x00556690)
#define jkGuiMouse_Darray_556698_ADDR (0x00556698)
#define jkGuiMouse_dword_5566B0_ADDR (0x005566B0)
#define jkGuiMouse_Darray_5566B8_ADDR (0x005566B8)
#define jkGuiMouse_Darray_5566D0_ADDR (0x005566D0)
#define jkGuiMouse_pWStr_5566E8_ADDR (0x005566E8)
#define jkGuiEsc_bInitialized_ADDR (0x00555DD8)
#define jkGuiKeyboard_dword_555DE0_ADDR (0x00555DE0)
#define jkGuiKeyboard_bOnceIdk_ADDR (0x00555DE8)
#define jkGuiKeyboard_funcIdx_ADDR (0x00555DEC)
#define jkGuiKeyboard_flags_ADDR (0x00555DF0)
#define jkGuiKeyboard_darrEntries_ADDR (0x00555DF8)
#define jkGuiKeyboard_dword_555E10_ADDR (0x00555E10)
#define jkGuiKeyboard_wstr_555E18_ADDR (0x00555E18)
#define jkGuiKeyboard_pWStr_55601C_ADDR (0x0055601C)
#define jkGuiMap_vec3Idk2_ADDR (0x005565A0)
#define jkGuiMap_pCanvas_ADDR (0x005565AC)
#define jkGuiMap_viewMat_ADDR (0x005565B0)
#define jkGuiMap_matTmp_ADDR (0x005565E0)
#define jkGuiMap_pVbuffer_ADDR (0x00556610)
#define jkGuiMap_unk4_ADDR (0x00556618)
#define jkGuiMap_vec3Idk_ADDR (0x00556650)
#define jkGuiMap_pCamera_ADDR (0x0055665C)
#define jkGuiMap_dword_556660_ADDR (0x00556660)
#define jkGuiMap_bOrbitActive_ADDR (0x00556664)
#define jkGuiMap_dword_556668_ADDR (0x00556668)
#define jkGuiMap_dword_55666C_ADDR (0x0055666C)
#define sithMap_unkArr_ADDR (0x0052E9E0)
#define sithMap_pPlayerThing_ADDR (0x0084DE78)
#define sithMap_invMatrix_ADDR (0x0084DE80)
#define sithMap_flt_84DEA8_ADDR (0x0084DEA8)
#define sithMap_flt_84DEAC_ADDR (0x0084DEAC)
#define sithMap_ctx_ADDR (0x0084DEB0)
#define sithMap_pCurWorld_ADDR (0x0084DEE4)
#define sithMap_camera_ADDR (0x0084DEE8)
#define sithMap_pCurCamera_ADDR (0x0084DF18)
#define sithMap_bInitted_ADDR (0x0084DF1C)
#define sithMap_var_ADDR (0x0084DF20)
#define DirectPlay_numPlayers_ADDR (0x00861FE0)
#define DirectPlay_aPlayers_ADDR (0x00860D60)

#ifdef NO_JK_MMAP

// Vars
extern tHashTable* sithAIClass_g_pHashtable MAYBE_UNUSED;
extern char std_g_genBuffer[1024] MAYBE_UNUSED;
extern struct HostServices* std_g_pHS MAYBE_UNUSED;
extern rdGeoset* rdModel3_pCurGeoset MAYBE_UNUSED;
extern rdVector3 localCamera MAYBE_UNUSED;
extern rdVector3 aFaceVerts[32] NO_REINIT MAYBE_UNUSED;
extern rdMeshinfo vertexDst MAYBE_UNUSED;
extern int curGeometryMode MAYBE_UNUSED;
extern rdLight* apGeoLights[64] MAYBE_UNUSED;
extern rdVector3 rdModel3_aLocalLightPos[64] NO_REINIT MAYBE_UNUSED;
extern rdVector3 rdModel3_aLocalLightDir[64] NO_REINIT MAYBE_UNUSED;
extern int meshFrustumCull MAYBE_UNUSED;
extern int curTextureMode MAYBE_UNUSED;
extern rdVector3 aView[512] NO_REINIT MAYBE_UNUSED;
extern rdMesh* pCurMesh MAYBE_UNUSED;
extern int thingFrustumCull MAYBE_UNUSED;
extern rdMeshinfo vertexSrc MAYBE_UNUSED;
extern rdModel3* pCurModel3 MAYBE_UNUSED;
extern int rdModel3_textureMode MAYBE_UNUSED;
extern int curLightingMode MAYBE_UNUSED;
extern rdLight* apMeshLights[64] MAYBE_UNUSED;
extern rdThing* pCurThing MAYBE_UNUSED;
extern int rdModel3_lightingMode MAYBE_UNUSED;
extern int rdModel3_geometryMode MAYBE_UNUSED;
extern int rdModel3_numDrawnModels MAYBE_UNUSED;
extern model3Loader_t pModel3Loader MAYBE_UNUSED;
extern model3Unloader_t pModel3Unloader MAYBE_UNUSED;
extern uint32_t rdModel3_numGeoLights MAYBE_UNUSED;
extern int rdModel3_numMeshLights MAYBE_UNUSED;
extern flex_t rdModel3_fRadius MAYBE_UNUSED;
extern tHashTable* sithPuppet_pClassHashtable MAYBE_UNUSED;
extern tHashTable* sithPuppet_pKeyHashtable MAYBE_UNUSED;
extern tHashTable* sithPuppet_pHashtblSubmodes MAYBE_UNUSED;
extern keyframeLoader_t pKeyframeLoader MAYBE_UNUSED;
extern keyframeUnloader_t pKeyframeUnloader MAYBE_UNUSED;
extern int32_t sithTime_g_frameTime MAYBE_UNUSED;
extern flex_t sithTime_g_frameTimeFlex MAYBE_UNUSED;
extern flex_t  sithTime_g_fps MAYBE_UNUSED;
extern uint32_t sithTime_g_msecGameTime MAYBE_UNUSED;
extern flex32_t sithTime_g_secGameTime MAYBE_UNUSED;
extern uint32_t sithTime_g_clockTime MAYBE_UNUSED;
extern int32_t sithTime_msecPauseStartTime MAYBE_UNUSED;
extern int sithTime_g_bPaused MAYBE_UNUSED;
extern int sithRender_texMode MAYBE_UNUSED;
extern int sithRender_renderflags MAYBE_UNUSED;
extern int sithRender_geoMode MAYBE_UNUSED;
extern int sithRender_lightMode MAYBE_UNUSED;
extern int sithRender_lightingIRMode MAYBE_UNUSED;
extern flex_t sithRender_f_83198C MAYBE_UNUSED;
extern flex_t sithRender_f_831990 MAYBE_UNUSED;
extern int sithRender_bResetCameraAspect MAYBE_UNUSED;
extern int32_t sithRender_g_numVisibleSectors MAYBE_UNUSED;
extern int32_t sithRender_numSecorFrustrums MAYBE_UNUSED;
extern int32_t sithRender_numThingLights MAYBE_UNUSED;
extern int32_t sithRender_numThingSectors MAYBE_UNUSED;
extern uint32_t sithRender_numSpritesToDraw MAYBE_UNUSED;
extern int sithRender_numRenderedSectors MAYBE_UNUSED;
extern int32_t sithRender_numAlphaAdjoins MAYBE_UNUSED;
extern int sithRender_geoThingsDrawn MAYBE_UNUSED;
extern int sithRender_nongeoThingsDrawn MAYBE_UNUSED;
extern flex_t  sithRender_f_82F4B0 MAYBE_UNUSED;
extern rdMeshinfo sithRender_faceView MAYBE_UNUSED;
extern rdMeshinfo meshinfo_out MAYBE_UNUSED;
extern sithRender_weapRendFunc_t sithRender_pExtraThingRenderFunc MAYBE_UNUSED;
extern rdLight sithRender_aThingLights[32] MAYBE_UNUSED;
extern SithSector* sithRender_aVisibleSectors[SITH_MAX_VISIBLE_SECTORS] MAYBE_UNUSED;
extern rdClipFrustum sithRender_aSectorFrustrums[SITH_MAX_VISIBLE_SECTORS] MAYBE_UNUSED;
extern SithSector* sithRender_aThingSectors[SITH_MAX_VISIBLE_SECTORS_2+2] MAYBE_UNUSED;
extern rdVector3 sithRender_aClipVertices[32] NO_REINIT MAYBE_UNUSED;
extern rdVector3 sithRender_aTransformedClipVertices[32] NO_REINIT MAYBE_UNUSED;
extern SithSurface* sithRender_aAlphaAdjoins[SITH_MAX_VISIBLE_ALPHA_SURFACES] MAYBE_UNUSED;
extern int sithRender_lastRenderTick MAYBE_UNUSED;
extern SithSurface* aSithSurfaces[SITH_MAX_VISIBLE_ALPHA_SURFACES] MAYBE_UNUSED;
extern SithCamera sithCamera_g_aCameras[SITHCAMERA_NUMCAMERAS] MAYBE_UNUSED;
extern int sithCamera_g_bCurCameraSet MAYBE_UNUSED;
extern int sithCamera_g_stateFlags MAYBE_UNUSED;
extern int sithCamera_g_curCycleCamNum MAYBE_UNUSED;
extern rdVector3 sithCamera_g_vecCameraPosOffset MAYBE_UNUSED;
extern rdVector3 sithCamera_g_vecCameraAngleOffset MAYBE_UNUSED;
extern flex_t sithCamera_g_cameraPosDelta MAYBE_UNUSED;
extern flex_t sithCamera_g_cameraAngleDelta MAYBE_UNUSED;
extern SithCamera* sithCamera_g_pCurCamera MAYBE_UNUSED;
extern int sithCamera_bStartup MAYBE_UNUSED;
extern rdMatrix34 sithCamera_g_orbCamOrient MAYBE_UNUSED;
extern rdMatrix34 sithCamera_idleCamOrient MAYBE_UNUSED;
extern int sithCamera_bOpen MAYBE_UNUSED;
extern rdVector4  rdroid_aMipDistances MAYBE_UNUSED;
extern int rdroid_frameTrue MAYBE_UNUSED;
extern int bRDroidStartup MAYBE_UNUSED;
extern int bRDroidOpen MAYBE_UNUSED;
extern int rdroid_g_curLightingMode MAYBE_UNUSED;
extern struct HostServices* rdroid_g_pHS MAYBE_UNUSED;
extern int rdroid_g_curGeometryMode MAYBE_UNUSED;
extern stdPalEffect rdroid_curColorEffects MAYBE_UNUSED;
extern int rdroid_curOcclusionMethod MAYBE_UNUSED;
extern rdZBufferMethod_t rdroid_curZBufferMethod MAYBE_UNUSED;
extern int rdroid_curProcFaceUserData MAYBE_UNUSED;
extern int rdroid_curSortingMethod MAYBE_UNUSED;
extern int rdroid_curAcceleration MAYBE_UNUSED;
extern int rdroid_curTextureMode MAYBE_UNUSED;
extern int rdroid_g_curRenderOptions MAYBE_UNUSED;
extern int rdroid_curCullFlags MAYBE_UNUSED;
extern tHashTable* sithMaterial_pHashtable MAYBE_UNUSED;
extern rdMaterial** sithMaterial_aMaterials MAYBE_UNUSED;
extern int sithMaterial_numMaterials MAYBE_UNUSED;
extern rdTri rdCache_aHWSolidTris[RDCACHE_MAX_TRIS] NO_REINIT MAYBE_UNUSED;
extern int rdCache_totalNormalTris MAYBE_UNUSED;
extern flex_t rdCache_aIntensities[RDCACHE_MAX_VERTICES] NO_REINIT MAYBE_UNUSED;
extern rdVector3 rdCache_aVertices[RDCACHE_MAX_VERTICES] NO_REINIT MAYBE_UNUSED;
extern int rdCache_totalVerts MAYBE_UNUSED;
extern rdVector2 rdCache_aTexVertices[RDCACHE_MAX_VERTICES] NO_REINIT MAYBE_UNUSED;
extern rdTri rdCache_aHWNormalTris[RDCACHE_MAX_TRIS] NO_REINIT MAYBE_UNUSED;
extern int rdCache_totalSolidTris MAYBE_UNUSED;
extern D3DVERTEX rdCache_aHWVertices[RDCACHE_MAX_VERTICES] NO_REINIT MAYBE_UNUSED;
extern int rdCache_drawnFaces MAYBE_UNUSED;
extern int rdCache_numUsedVertices MAYBE_UNUSED;
extern int rdCache_numUsedTexVertices MAYBE_UNUSED;
extern int rdCache_numUsedIntensities MAYBE_UNUSED;
extern rdVector2i rdCache_ulcExtent MAYBE_UNUSED;
extern rdVector2i rdCache_lrcExtent MAYBE_UNUSED;
extern int rdCache_numProcFaces MAYBE_UNUSED;
extern rdProcEntry rdCache_aProcFaces[RDCACHE_MAX_TRIS] NO_REINIT MAYBE_UNUSED;
extern int rdCache_dword_865258 MAYBE_UNUSED;
extern char16_t sithMulti_name[32] MAYBE_UNUSED;
extern sithMultiHandler_t sithMulti_pfNewPlayerJoinedCallback MAYBE_UNUSED;
extern int sithMulti_multiModeFlags MAYBE_UNUSED;
extern uint32_t sithMulti_numRemovedStaticThings MAYBE_UNUSED;
extern int sithMulti_aRemovedStaticThings[256] MAYBE_UNUSED;
extern uint32_t sithMulti_msecQuitGameTime MAYBE_UNUSED;
extern int sithMulti_msecPingStartTime MAYBE_UNUSED;
extern uint32_t sithMulti_msecLastSyncScoreTime MAYBE_UNUSED;
extern uint32_t sithMulti_curWelcomePlayerNum MAYBE_UNUSED;
extern int sithMulti_msecWelcomeUpdateInterval MAYBE_UNUSED;
extern rdColormap* rdColormap_pCurMap MAYBE_UNUSED;
extern rdColormap* rdColormap_pIdentityMap MAYBE_UNUSED;
extern SithEvent sithEvent_aEvents[256] MAYBE_UNUSED;
extern SithEvent* sithEvent_g_pFirstQueuedEvent MAYBE_UNUSED;
extern int sithEvent_arrLut[256] MAYBE_UNUSED;
extern SithEventTask sithEvent_aTasks[SITH_NUM_EVENTS] MAYBE_UNUSED;
extern int sithEvent_numFreeEventBuffers MAYBE_UNUSED;
extern int sithEvent_bInit MAYBE_UNUSED;
extern int sithEvent_bOpen MAYBE_UNUSED;
extern int rdClip_g_faceStatus MAYBE_UNUSED;
extern rdVector3* rdClip_pSourceVert MAYBE_UNUSED;
extern flex_t rdClip_workIVerts[32] NO_REINIT MAYBE_UNUSED;
extern rdVector3 rdClip_aWorkVerts[32] NO_REINIT MAYBE_UNUSED;
extern rdVector3* rdClip_pDestVert MAYBE_UNUSED;
extern flex_t* rdClip_pDestIVert MAYBE_UNUSED;
extern rdVector2 rdClip_aWorkTVerts[32] NO_REINIT MAYBE_UNUSED;
extern flex_t* rdClip_pSourceIVert MAYBE_UNUSED;
extern rdVector2* rdClip_pSourceTVert MAYBE_UNUSED;
extern rdVector2* rdClip_pDestTVert MAYBE_UNUSED;
extern int sithSoundMixer_dword_835FCC MAYBE_UNUSED;
extern int sithSoundMixer_bPlayingMci MAYBE_UNUSED;
extern int sithSoundMixer_bInitted MAYBE_UNUSED;
extern flex_t sithSoundMixer_flt_835FD8 MAYBE_UNUSED;
extern int sithSoundMixer_bIsMuted MAYBE_UNUSED;
extern flex_t  sithSoundMixer_musicVolume MAYBE_UNUSED;
extern flex_t  sithSoundMixer_globalVolume MAYBE_UNUSED;
extern int sithSoundMixer_numSoundsAvailable2 MAYBE_UNUSED;
extern int sithSoundMixer_numSoundsAvailable MAYBE_UNUSED;
extern sithPlayingSound sithSoundMixer_aPlayingSounds[SITH_MIXER_NUMPLAYINGSOUNDS] MAYBE_UNUSED;
extern int sithSoundMixer_aIdk[SITH_MIXER_NUMPLAYINGSOUNDS] MAYBE_UNUSED;
extern int sithSoundMixer_activeChannels MAYBE_UNUSED;
extern int sithSoundMixer_bOpened MAYBE_UNUSED;
extern SithSector* sithSoundMixer_pCurSector MAYBE_UNUSED;
extern int sithSoundMixer_dword_836BFC MAYBE_UNUSED;
extern int sithSoundMixer_trackFrom MAYBE_UNUSED;
extern int sithSoundMixer_trackTo MAYBE_UNUSED;
extern sithPlayingSound* sithSoundMixer_hCurAmbientChannel MAYBE_UNUSED;
extern int sithSoundMixer_dword_836C00 MAYBE_UNUSED;
extern int  sithSoundMixer_channelGUIDSeed MAYBE_UNUSED;
extern int sithSoundMixer_dword_836C04 MAYBE_UNUSED;
extern SithThing* sithSoundMixer_pFocusedThing MAYBE_UNUSED;
extern int  sithSound_maxDataLoaded MAYBE_UNUSED;
extern int  sithSound_var3 MAYBE_UNUSED;
extern int sithSound_curDataLoaded MAYBE_UNUSED;
extern int sithSound_bInit MAYBE_UNUSED;
extern tHashTable* sithSound_hashtable MAYBE_UNUSED;
extern int sithSound_var4 MAYBE_UNUSED;
extern int sithSound_var5 MAYBE_UNUSED;
extern int sithControl_inputFuncToControlType[INPUT_FUNC_MAX] MAYBE_UNUSED;
extern stdControlKeyInfo sithControl_aInputFuncToKeyinfo[INPUT_FUNC_MAX] MAYBE_UNUSED;
extern uint32_t sithControl_msIdle MAYBE_UNUSED;
extern int sithControl_bInitted MAYBE_UNUSED;
extern int sithControl_bOpened MAYBE_UNUSED;
extern sithControl_handler_t sithControl_aHandlers[SITHCONTROL_NUM_HANDLERS] MAYBE_UNUSED;
extern int sithControl_numHandlers MAYBE_UNUSED;
extern int sithControl_death_msgtimer MAYBE_UNUSED;
extern rdVector3  sithControl_vec3_54A570 MAYBE_UNUSED;
extern flex_t  sithControl_flt_54A57C MAYBE_UNUSED;
extern sithSaveHandler_t sithGamesave_func1 MAYBE_UNUSED;
extern sithSaveHandler_t sithGamesave_func2 MAYBE_UNUSED;
extern sithSaveHandler_t sithGamesave_func3 MAYBE_UNUSED;
extern sithSaveHandler_t sithGamesave_funcWrite MAYBE_UNUSED;
extern sithSaveHandler_t sithGamesave_funcRead MAYBE_UNUSED;
extern char sithGamesave_autosave_fname[128] MAYBE_UNUSED;
extern sithGamesaveState_t sithGamesave_state MAYBE_UNUSED;
extern int sithGamesave_dword_835914 MAYBE_UNUSED;
extern char sithGamesave_aCurFilename[132] MAYBE_UNUSED;
extern char16_t sithGamesave_wsaveName[256] MAYBE_UNUSED;
extern char sithGamesave_saveName[128] MAYBE_UNUSED;
extern sithGamesave_Header sithGamesave_headerTmp MAYBE_UNUSED;
extern rdCamera* rdCamera_g_pCurCamera MAYBE_UNUSED;
extern rdMatrix34 rdCamera_g_camMatrix MAYBE_UNUSED;
extern int32_t  sithNet_MultiModeFlags MAYBE_UNUSED;
extern int32_t  sithNet_scorelimit MAYBE_UNUSED;
extern int32_t  sithNet_teamScore[5] MAYBE_UNUSED;
extern int32_t  sithNet_multiplayer_timelimit MAYBE_UNUSED;
extern int32_t  sithMulti_multiplayerTimelimit MAYBE_UNUSED;
extern int32_t  sithNet_isMulti MAYBE_UNUSED;
extern int32_t  sithNet_isServer MAYBE_UNUSED;
extern int32_t  sithMulti_bTimelimitMet MAYBE_UNUSED;
extern int32_t  sithNet_serverNetId MAYBE_UNUSED;
extern int32_t  sithNet_things[SITH_MAX_THINGS] MAYBE_UNUSED;
extern int32_t  sithNet_thingsIdx MAYBE_UNUSED;
extern int32_t  sithNet_syncIdx MAYBE_UNUSED;
extern int32_t  sithNet_aSyncFlags[SITH_MAX_SYNC_THINGS] MAYBE_UNUSED;
extern SithThing* sithNet_aSyncThings[SITH_MAX_SYNC_THINGS] MAYBE_UNUSED;
extern int32_t   sithNet_tickrate MAYBE_UNUSED;
extern int32_t  sithNet_dword_8C4BA8 MAYBE_UNUSED;
extern int32_t  sithNet_dword_83262C MAYBE_UNUSED;
extern int32_t  sithMulti_quitGameState MAYBE_UNUSED;
extern int32_t  sithNet_checksum MAYBE_UNUSED;
extern int32_t  sithNet_bNeedsFullThingSyncForLeaveJoin MAYBE_UNUSED;
extern int32_t  sithMulti_newPlayerId MAYBE_UNUSED;
extern int32_t  sithNet_bSyncScores MAYBE_UNUSED;
extern int32_t  sithNet_dword_832620 MAYBE_UNUSED;
extern flex_t  sithOverlayMap_curScale MAYBE_UNUSED;
extern rdMatrix34 sithOverlayMap_mapOrient MAYBE_UNUSED;
extern SithThing* sithOverlayMap_pLocalPlayer MAYBE_UNUSED;
extern rdCanvas* sithOverlayMap_pCanvas MAYBE_UNUSED;
extern int sithOverlayMap_x1 MAYBE_UNUSED;
extern int sithOverlayMap_y1 MAYBE_UNUSED;
extern sithMapView sithOverlayMap_inst MAYBE_UNUSED;
extern int sithOverlayMap_bMapVisible MAYBE_UNUSED;
extern int sithOverlayMap_bOpened MAYBE_UNUSED;
extern tHashTable* sithSoundClass_pHashtblModes MAYBE_UNUSED;
extern tHashTable* sithSoundClass_pHashTable MAYBE_UNUSED;
extern void* sithTemplate_pMasterFile MAYBE_UNUSED;
extern tHashTable* sithTemplate_pHashtable MAYBE_UNUSED;
extern int sithTemplate_masterFileCount MAYBE_UNUSED;
extern tHashTable* sithTemplate_pMasterHashtable MAYBE_UNUSED;
extern rdEdge NO_REINIT activeEdgeTail MAYBE_UNUSED;
extern rdEdge NO_REINIT activeEdgeHead MAYBE_UNUSED;
extern rdEdge* apNewActiveEdges[1024] NO_REINIT MAYBE_UNUSED;
extern rdEdge* apRemoveActiveEdges[1024] NO_REINIT MAYBE_UNUSED;
extern int yMinEdge MAYBE_UNUSED;
extern int yMaxEdge MAYBE_UNUSED;
extern int numActiveSpans MAYBE_UNUSED;
extern int numActiveFaces MAYBE_UNUSED;
extern int numActiveEdges MAYBE_UNUSED;
extern rdEdge aActiveEdges[1024] NO_REINIT  MAYBE_UNUSED;
extern int rdActive_drawnFaces MAYBE_UNUSED;
extern int sithMain_bEndLevel MAYBE_UNUSED;
extern int sith_bStartup MAYBE_UNUSED;
extern int sith_bOpen MAYBE_UNUSED;
extern int sithSurface_numAvail MAYBE_UNUSED;
extern int sithSurface_aAvail[256+1] MAYBE_UNUSED;
extern int sithSurface_numSurfaces MAYBE_UNUSED;
extern rdSurface sithSurface_aSurfaces[256] MAYBE_UNUSED;
extern int sithSurface_bOpened MAYBE_UNUSED;
extern uint32_t  sithSurface_byte_8EE668 MAYBE_UNUSED;
extern int sithSurface_numUnsyncedSurfaces MAYBE_UNUSED;
extern rdMaterialLoader_t pMaterialsLoader MAYBE_UNUSED;
extern rdMaterialUnloader_t pMaterialsUnloader MAYBE_UNUSED;
extern tHashTable* sithSprite_pHashtable MAYBE_UNUSED;
extern WindowDrawHandler_t Window_drawAndFlip MAYBE_UNUSED;
extern WindowDrawHandler_t Window_setCooperativeLevel MAYBE_UNUSED;
extern wm_handler Window_ext_handlers[16] MAYBE_UNUSED;
extern HWND Window_aDialogHwnds[16] MAYBE_UNUSED;
extern int DebugGui_aIdk[32] MAYBE_UNUSED;
extern int DebugGui_idk MAYBE_UNUSED;
extern int DebugGui_some_line_amt MAYBE_UNUSED;
extern int DebugGui_some_num_lines MAYBE_UNUSED;
extern char DebugLog_buffer[4072] MAYBE_UNUSED;
extern stdDebugConsoleCmd* sithConsole_aCmds MAYBE_UNUSED;
extern tHashTable* sithConsole_pCmdHashtable MAYBE_UNUSED;
extern int sithConsole_bOpened MAYBE_UNUSED;
extern int sithConsole_bInitted MAYBE_UNUSED;
extern int sithConsole_maxCmds MAYBE_UNUSED;
extern int sithConsole_numRegisteredCmds MAYBE_UNUSED;
extern uint32_t DebugGui_maxLines MAYBE_UNUSED;
extern DebugConsolePrintFunc_t DebugGui_fnPrint MAYBE_UNUSED;
extern DebugConsolePrintUniStrFunc_t DebugGui_fnPrintUniStr MAYBE_UNUSED;
extern stdSound_buffer_t* sithConsole_alertSound MAYBE_UNUSED;
extern int16_t sithConsole_idk2 MAYBE_UNUSED;
extern uint32_t d3d_maxVertices MAYBE_UNUSED;
extern d3d_device* d3d_device_ptr MAYBE_UNUSED;
extern rdRect std3D_rectViewIdk MAYBE_UNUSED;
extern flex_t std3D_aViewIdk[32] MAYBE_UNUSED;
extern rdTri std3D_aViewTris[2] MAYBE_UNUSED;
extern int32_t  std3D_gpuMaxTexSizeMaybe MAYBE_UNUSED;
extern int32_t  std3D_dword_53D66C MAYBE_UNUSED;
extern int32_t  std3D_dword_53D670 MAYBE_UNUSED;
extern int32_t  std3D_dword_53D674 MAYBE_UNUSED;
extern int32_t  std3D_frameCount MAYBE_UNUSED;
extern intptr_t std3D_renderList MAYBE_UNUSED;
extern int32_t std3D_numCachedTextures MAYBE_UNUSED;
extern rdDDrawSurface* std3D_pFirstTexCache MAYBE_UNUSED;
extern rdDDrawSurface*  std3D_pLastTexCache MAYBE_UNUSED;
extern int stdComm_dword_8321F8 MAYBE_UNUSED;
extern int stdComm_bInitted MAYBE_UNUSED;
extern int stdComm_dword_8321F0 MAYBE_UNUSED;
extern int stdComm_dword_8321F4 MAYBE_UNUSED;
extern int stdComm_dplayIdSelf MAYBE_UNUSED;
extern int stdComm_dword_832204 MAYBE_UNUSED;
extern int stdComm_dword_832208 MAYBE_UNUSED;
extern int stdComm_currentBigSyncStage MAYBE_UNUSED;
extern int stdComm_dword_8321E0 MAYBE_UNUSED;
extern int stdComm_bIsServer MAYBE_UNUSED;
extern int stdComm_dword_8321E8 MAYBE_UNUSED;
extern char16_t stdComm_waIdk[32] MAYBE_UNUSED;
extern int stdComm_dword_8321DC MAYBE_UNUSED;
extern int stdComm_dword_832200 MAYBE_UNUSED;
extern int stdComm_dword_832210 MAYBE_UNUSED;
extern SithMessage stdComm_cogMsgTmp MAYBE_UNUSED;
extern MCIDEVICEID stdMci_mciId MAYBE_UNUSED;
extern int stdMci_dwVolume MAYBE_UNUSED;
extern int stdMci_bInitted MAYBE_UNUSED;
extern int stdMci_uDeviceID MAYBE_UNUSED;
extern int wuRegistry_bStarted MAYBE_UNUSED;
extern uint8_t wuRegistry_lpClass[4] MAYBE_UNUSED;
extern uint8_t wuRegistry_byte_855EB4[4] MAYBE_UNUSED;
extern HKEY wuRegistry_hKey MAYBE_UNUSED;
extern LPCSTR wuRegistry_lpSubKey MAYBE_UNUSED;
extern uint32_t WinIdk_aDplayGuid[4] MAYBE_UNUSED;
extern StdDisplayInfo stdDisplay_aDisplayDevices[16] MAYBE_UNUSED;
extern int stdDisplay_gammaTableLen MAYBE_UNUSED;
extern flex_d_t* stdDisplay_paGammaTable MAYBE_UNUSED;
extern rdColor24 stdDisplay_gammaPalette[256] MAYBE_UNUSED;
extern StdDisplayInfo* stdDisplay_pCurDevice MAYBE_UNUSED;
extern StdVideoMode* stdDisplay_pCurVideoMode MAYBE_UNUSED;
extern int stdDisplay_bStartup MAYBE_UNUSED;
extern int stdDisplay_bOpen MAYBE_UNUSED;
extern int stdDisplay_bModeSet MAYBE_UNUSED;
extern int stdDisplay_numVideoModes MAYBE_UNUSED;
extern int stdDisplay_bPaged MAYBE_UNUSED;
extern uint8_t stdDisplay_tmpGammaPal[0x300] MAYBE_UNUSED;
extern int stdDisplay_gammaIdx MAYBE_UNUSED;
extern uint16_t word_860800 MAYBE_UNUSED;
extern uint16_t word_860802 MAYBE_UNUSED;
extern uint16_t word_860804 MAYBE_UNUSED;
extern uint16_t word_860806 MAYBE_UNUSED;
extern int  stdControl_bReadMouse MAYBE_UNUSED;
extern flex_t  stdControl_updateKHz MAYBE_UNUSED;
extern flex_t  stdControl_updateHz MAYBE_UNUSED;
extern flex_t  stdControl_mouseXSensitivity MAYBE_UNUSED;
extern flex_t  stdControl_mouseYSensitivity MAYBE_UNUSED;
extern flex_t  stdControl_mouseZSensitivity MAYBE_UNUSED;
extern int stdControl_aAxisEnabled[JK_NUM_JOYSTICKS] MAYBE_UNUSED;
extern int stdControl_aAxisStates[JK_NUM_AXES] MAYBE_UNUSED;
extern int stdControl_aKeyIdleTimes[JK_NUM_KEYS] MAYBE_UNUSED;
extern int stdControl_aKeyInfo[JK_NUM_KEYS] MAYBE_UNUSED;
extern int stdControl_aAxisConnected[JK_NUM_JOYSTICKS] MAYBE_UNUSED;
extern int stdControl_aKeyPressed[JK_NUM_KEYS] MAYBE_UNUSED;
extern int stdControl_bStartup MAYBE_UNUSED;
extern int stdControl_bOpen MAYBE_UNUSED;
extern int stdControl_bDisableKeyboard MAYBE_UNUSED;
extern int stdControl_bControlsIdle MAYBE_UNUSED;
extern int stdControl_bControlsActive MAYBE_UNUSED;
extern void* stdControl_pDI MAYBE_UNUSED;
extern void* stdControl_mouseDirectInputDevice MAYBE_UNUSED;
extern void* stdControl_keyboardIDirectInputDevice MAYBE_UNUSED;
extern int stdControl_bReadJoysticks MAYBE_UNUSED;
extern uint32_t stdControl_curReadTime MAYBE_UNUSED;
extern uint32_t stdControl_lastReadTime MAYBE_UNUSED;
extern uint32_t stdControl_readDeltaTime MAYBE_UNUSED;
extern int32_t stdControl_dwLastMouseX MAYBE_UNUSED;
extern int32_t stdControl_dwLastMouseY MAYBE_UNUSED;
extern int stdControl_aJoystickEnabled[JK_NUM_JOYSTICKS] MAYBE_UNUSED;
extern int stdControl_aJoystickExists[4] MAYBE_UNUSED;
extern stdControlJoystickEntry stdControl_aAxes[JK_NUM_AXES] MAYBE_UNUSED;
extern int stdControl_aJoystickMaxButtons[JK_NUM_JOYSTICKS] MAYBE_UNUSED;
extern int Windows_installType MAYBE_UNUSED;
extern uint8_t Video_aGammaTable[80]  MAYBE_UNUSED;
extern int  Video_fillColor MAYBE_UNUSED;
extern videoModeStruct Video_modeStruct MAYBE_UNUSED;
extern tVBuffer Video_otherBuf MAYBE_UNUSED;
extern int Video_dword_866D78 MAYBE_UNUSED;
extern int Video_curMode MAYBE_UNUSED;
extern StdVideoMode Video_renderSurface[9]  MAYBE_UNUSED;
extern tVBuffer Video_menuBuffer MAYBE_UNUSED;
extern tVBuffer* Video_pOtherBuf MAYBE_UNUSED;
extern tVBuffer* Video_pMenuBuffer MAYBE_UNUSED;
extern int Video_bInitted MAYBE_UNUSED;
extern int Video_bOpened MAYBE_UNUSED;
extern flex_t Video_flt_55289C MAYBE_UNUSED;
extern int Video_dword_5528A0 MAYBE_UNUSED;
extern int Video_dword_5528A4 MAYBE_UNUSED;
extern int Video_dword_5528A8 MAYBE_UNUSED;
extern int Video_lastTimeMsec MAYBE_UNUSED;
extern int Video_dword_5528B0 MAYBE_UNUSED;
extern tVBuffer* Video_pVbufIdk MAYBE_UNUSED;
extern rdCanvas* Video_pCanvas MAYBE_UNUSED;
extern rdColor24 Video_aPalette[0x100] MAYBE_UNUSED;
extern tRasterInfo Video_format MAYBE_UNUSED;
extern tRasterInfo Video_format2 MAYBE_UNUSED;
extern videoModeStruct Video_modeStruct2 MAYBE_UNUSED;
extern tVBuffer Video_bufIdk MAYBE_UNUSED;
extern uint16_t stdConsole_textAttribute MAYBE_UNUSED;
extern uint16_t stdConsole_wAttributes MAYBE_UNUSED;
extern int stdConsole_cursorHidden MAYBE_UNUSED;
extern CONSOLE_CURSOR_INFO stdConsole_ConsoleCursorInfo MAYBE_UNUSED;
extern HANDLE stdConsole_hOutput MAYBE_UNUSED;
extern HANDLE stdConsole_hInput MAYBE_UNUSED;
extern int rdRaster_aOneOverNFixed[2048] MAYBE_UNUSED;
extern flex_t rdRaster_aLerpAndScale[16] MAYBE_UNUSED;
extern flex_t rdRaster_aOneOverNFlex[2048] MAYBE_UNUSED;
extern flex_t  rdRaster_fixedScale MAYBE_UNUSED;
extern SithThing* sithCogFunctionAI_aThingsInView[32] MAYBE_UNUSED;
extern int sithCogFunctionAI_numThingsInView MAYBE_UNUSED;
extern int sithCogFunctionAI_curThingInView MAYBE_UNUSED;
extern SithMessage sithComm_MsgTmpBuf[32] MAYBE_UNUSED;
extern sithCogMsg_Pair sithComm_aMsgPairs[128] MAYBE_UNUSED;
extern cogMsg_Handler sithMessage_aTypeFuncs[65] MAYBE_UNUSED;
extern int sithMessage_bStopProcessMessages MAYBE_UNUSED;
extern int sithMessage_g_outputstream MAYBE_UNUSED;
extern int sithMessage_g_inputstream MAYBE_UNUSED;
extern int sithComm_idk2 MAYBE_UNUSED;
extern int sithMessage_bSturtup MAYBE_UNUSED;
extern uint32_t sithComm_dword_847E84 MAYBE_UNUSED;
extern int  sithComm_msgId MAYBE_UNUSED;
extern SithMessage sithComm_MsgTmpBuf2 MAYBE_UNUSED;
extern SithMessage sithComm_netMsgTmp MAYBE_UNUSED;
extern char16_t jkCog_emptystring[2] MAYBE_UNUSED;
extern char16_t jkCog_jkstring[130] MAYBE_UNUSED;
extern int jkCog_bInitted MAYBE_UNUSED;
extern stdStrTable jkCog_strings MAYBE_UNUSED;
extern int sithCog_bOpened MAYBE_UNUSED;
extern tHashTable* sithCog_g_pHashtable MAYBE_UNUSED;
extern SithCogSectorLink sithCog_aSectorLinks[SITHCOG_MAX_LINKS] MAYBE_UNUSED;
extern int sithCog_numSectorLinks MAYBE_UNUSED;
extern SithCogThingLink sithCog_aThingLinks[SITHCOG_MAX_LINKS] MAYBE_UNUSED;
extern int sithCog_numThingLinks MAYBE_UNUSED;
extern int sithCog_numSurfaceLinks MAYBE_UNUSED;
extern SithCogSurfaceLink sithCog_aSurfaceLinks[SITHCOG_MAX_LINKS] MAYBE_UNUSED;
extern sithCog* sithCog_g_pMasterCog MAYBE_UNUSED;
extern rdVector2i jkDev_aEntryPositions[10] MAYBE_UNUSED;
extern jkDevLogEnt jkDev_aEntries[5] MAYBE_UNUSED;
extern int jkDev_log_55A4A4 MAYBE_UNUSED;
extern int jkDev_bScreenNeedsUpdate MAYBE_UNUSED;
extern stdDebugConsoleCmd jkDev_aCheatCmds[JKDEV_NUM_CHEATS] MAYBE_UNUSED;
extern uint32_t jkDev_numCheats MAYBE_UNUSED;
extern int jkDev_bInitted MAYBE_UNUSED;
extern int jkDev_bOpened MAYBE_UNUSED;
extern tHashTable* jkDev_cheatHashtable MAYBE_UNUSED;
extern HWND jkDev_hDlg MAYBE_UNUSED;
extern tVBuffer* jkDev_vbuf MAYBE_UNUSED;
extern int jkDev_BMFontHeight MAYBE_UNUSED;
extern int jkDev_ColorKey MAYBE_UNUSED;
extern int jkDev_dword_55A9D0 MAYBE_UNUSED;
extern flex_t jkDev_amt MAYBE_UNUSED;
extern int jkSmack_gameMode MAYBE_UNUSED;
extern int jkSmack_bInit MAYBE_UNUSED;
extern int jkSmack_stopTick MAYBE_UNUSED;
extern int jkSmack_currentGuiState MAYBE_UNUSED;
extern int jkSmack_nextGuiState MAYBE_UNUSED;
extern void* jkSmack_alloc MAYBE_UNUSED;
extern jkEpisode jkEpisode_aEpisodes[64] MAYBE_UNUSED;
extern char jkEpisode_var4[128] MAYBE_UNUSED;
extern char jkEpisode_var5[128] MAYBE_UNUSED;
extern uint32_t jkEpisode_var2 MAYBE_UNUSED;
extern jkEpisodeLoad jkEpisode_mLoad MAYBE_UNUSED;
extern uint32_t  jkHud_targetRed MAYBE_UNUSED;
extern uint32_t  jkHud_targetBlue MAYBE_UNUSED;
extern uint32_t  jkHud_targetGreen MAYBE_UNUSED;
extern flex_t jkHud_aFltIdk[6]  MAYBE_UNUSED;
extern int32_t jkHud_aColors8bpp[6]  MAYBE_UNUSED;
extern int jkHud_aTeamColors8bpp[5]  MAYBE_UNUSED;
extern char jkHud_chatStr[128] MAYBE_UNUSED;
extern int jkHud_aTeamColors16bpp[5] MAYBE_UNUSED;
extern uint32_t jkHud_rightBlitX MAYBE_UNUSED;
extern uint32_t jkHud_leftBlitX MAYBE_UNUSED;
extern SithOverlayMapConfig jkHud_mapRendConfig MAYBE_UNUSED;
extern int jkHud_chatStrPos MAYBE_UNUSED;
extern int jkHud_rightBlitY MAYBE_UNUSED;
extern jkHudTeamScore jkHud_aTeamScores[5] MAYBE_UNUSED;
extern int jkHud_dword_552D10 MAYBE_UNUSED;
extern int32_t jkHud_aColors16bpp[6] MAYBE_UNUSED;
extern jkHudPlayerScore jkHud_aPlayerScores[32] MAYBE_UNUSED;
extern int jkHud_blittedAmmoAmt MAYBE_UNUSED;
extern int jkHud_idk14 MAYBE_UNUSED;
extern int jkHud_blittedHealthIdx MAYBE_UNUSED;
extern int jkHud_blittedBatteryAmt MAYBE_UNUSED;
extern int jkHud_blittedFieldlightAmt MAYBE_UNUSED;
extern int jkHud_blittedShieldIdx MAYBE_UNUSED;
extern int jkHud_isSuper MAYBE_UNUSED;
extern int jkHud_idk15 MAYBE_UNUSED;
extern int jkHud_blittedForceIdx MAYBE_UNUSED;
extern int jkHud_idk16 MAYBE_UNUSED;
extern int jkHud_leftBlitY MAYBE_UNUSED;
extern rdRect jkHud_rectViewScores MAYBE_UNUSED;
extern stdFont* jkHud_pMsgFontSft MAYBE_UNUSED;
extern stdBitmap* jkHud_pStatusLeftBm MAYBE_UNUSED;
extern stdBitmap* jkHud_pStatusRightBm MAYBE_UNUSED;
extern int jkHud_bHasTarget MAYBE_UNUSED;
extern SithThing* jkHud_pTargetThing MAYBE_UNUSED;
extern uint32_t jkHud_targetRed16 MAYBE_UNUSED;
extern uint32_t jkHud_targetGreen16 MAYBE_UNUSED;
extern uint32_t jkHud_targetBlue16 MAYBE_UNUSED;
extern int jkHud_bChatOpen MAYBE_UNUSED;
extern stdFont* jkHud_pHelthNumSft MAYBE_UNUSED;
extern stdFont* jkHud_pAmoNumsSft MAYBE_UNUSED;
extern stdFont* jkHud_pAmoNumsSuperSft MAYBE_UNUSED;
extern stdFont* jkHud_pArmorNumSft MAYBE_UNUSED;
extern stdFont* jkHud_pArmorNumsSuperSft MAYBE_UNUSED;
extern int jkHud_bInitted MAYBE_UNUSED;
extern int jkHud_bOpened MAYBE_UNUSED;
extern stdBitmap* jkHud_pFieldlightBm MAYBE_UNUSED;
extern stdBitmap* jkHud_pStBatBm MAYBE_UNUSED;
extern stdBitmap* jkHud_pStHealthBm MAYBE_UNUSED;
extern stdBitmap* jkHud_pStShieldBm MAYBE_UNUSED;
extern stdBitmap* jkHud_pStFrcBm MAYBE_UNUSED;
extern stdBitmap* jkHud_pStFrcSuperBm MAYBE_UNUSED;
extern int jkHud_bViewScores MAYBE_UNUSED;
extern int jkHud_dword_553ED0 MAYBE_UNUSED;
extern int jkHud_tallyWhich MAYBE_UNUSED;
extern uint32_t jkHud_numPlayers MAYBE_UNUSED;
extern int jkHud_dword_553EDC MAYBE_UNUSED;
extern int jkHud_dword_553EE0 MAYBE_UNUSED;
extern jkHudInvInfo jkHudInv_info MAYBE_UNUSED;
extern rdTexFormat jkHudInv_itemTexfmt MAYBE_UNUSED;
extern int jkHudInv_flags MAYBE_UNUSED;
extern int jkHudInv_dword_553F64 MAYBE_UNUSED;
extern jkHudInvScroll jkHudInv_scroll MAYBE_UNUSED;
extern stdBitmap* jkHudInv_aBitmaps[3] MAYBE_UNUSED;
extern stdFont* jkHudInv_font MAYBE_UNUSED;
extern int jkHudInv_rend_isshowing_maybe MAYBE_UNUSED;
extern int jkHudInv_dword_553F94 MAYBE_UNUSED;
extern int jkHudInv_numItems MAYBE_UNUSED;
extern int* jkHudInv_aItems MAYBE_UNUSED;
extern int Main_bDevMode MAYBE_UNUSED;
extern int Main_bDisplayConfig MAYBE_UNUSED;
extern int Main_bWindowGUI MAYBE_UNUSED;
extern int Main_dword_86078C MAYBE_UNUSED;
extern int Main_bFrameRate MAYBE_UNUSED;
extern int Main_bDispStats MAYBE_UNUSED;
extern int Main_bNoHUD MAYBE_UNUSED;
extern int Main_logLevel MAYBE_UNUSED;
extern int Main_verboseLevel MAYBE_UNUSED;
extern char Main_path[128] MAYBE_UNUSED;
extern stdFile_t  debug_log_fp MAYBE_UNUSED;
extern HostServices* pHS MAYBE_UNUSED;
extern char jkCredits_aPalette[0x300] MAYBE_UNUSED;
extern tVBuffer* jkCredits_pVbuffer2 MAYBE_UNUSED;
extern int jkCredits_dword_55AD64 MAYBE_UNUSED;
extern int jkCredits_dword_55AD68 MAYBE_UNUSED;
extern stdStrTable jkCredits_table MAYBE_UNUSED;
extern int jkCredits_startMs MAYBE_UNUSED;
extern int jkCredits_dword_55AD84 MAYBE_UNUSED;
extern int jkCredits_strIdx MAYBE_UNUSED;
extern char* jkCredits_aIdk MAYBE_UNUSED;
extern tVBuffer* jkCredits_pVbuffer MAYBE_UNUSED;
extern int jkCredits_dword_55AD94 MAYBE_UNUSED;
extern stdFont* jkCredits_fontLarge MAYBE_UNUSED;
extern stdFont* jkCredits_fontSmall MAYBE_UNUSED;
extern int jkCredits_dword_55ADA0 MAYBE_UNUSED;
extern int jkCredits_bInitted MAYBE_UNUSED;
extern int jkCredits_dword_55ADA8 MAYBE_UNUSED;
extern int g_sithMode MAYBE_UNUSED;
extern int g_submodeFlags MAYBE_UNUSED;
extern int g_debugmodeFlags MAYBE_UNUSED;
extern int g_mapModeFlags MAYBE_UNUSED;
extern int jkGame_gamma MAYBE_UNUSED;
extern int jkGame_screenSize MAYBE_UNUSED;
extern int jkGame_bInitted MAYBE_UNUSED;
extern int jkGame_updateMsecsTotal MAYBE_UNUSED;
extern int jkGame_dword_552B5C MAYBE_UNUSED;
extern int jkGame_isDDraw MAYBE_UNUSED;
extern HostServices* jkRes_pHS MAYBE_UNUSED;
extern char jkRes_episodeGobName[32]  MAYBE_UNUSED;
extern char jkRes_curDir[128]  MAYBE_UNUSED;
extern int jkRes_bHookedHS MAYBE_UNUSED;
extern jkResFile jkRes_aFiles[32] MAYBE_UNUSED;
extern jkRes jkRes_gCtx MAYBE_UNUSED;
extern HostServices* pLowLevelHS MAYBE_UNUSED;
extern HostServices lowLevelHS MAYBE_UNUSED;
extern char jkRes_idkGobPath[128] MAYBE_UNUSED;
extern rdRect jkCutscene_rect1 MAYBE_UNUSED;
extern rdRect jkCutscene_rect2 MAYBE_UNUSED;
extern stdStrTable jkCutscene_strings MAYBE_UNUSED;
extern stdFont* jkCutscene_subtitlefont MAYBE_UNUSED;
extern int jkCutscene_bInitted MAYBE_UNUSED;
extern int jkCutscene_isRendering MAYBE_UNUSED;
extern int jkCutscene_dword_55B750 MAYBE_UNUSED;
extern int jkCutscene_dword_55AA50 MAYBE_UNUSED;
extern int jkCutscene_55AA54 MAYBE_UNUSED;
extern char jkMain_aLevelJklFname[128] MAYBE_UNUSED;
extern int  thing_nine MAYBE_UNUSED;
extern int jkMain_bInit MAYBE_UNUSED;
extern int thing_six MAYBE_UNUSED;
extern int thing_eight MAYBE_UNUSED;
extern int jkMain_dword_552B98 MAYBE_UNUSED;
extern int jkMain_lastTickMs MAYBE_UNUSED;
extern int idx_13b4_related MAYBE_UNUSED;
extern char gamemode_1_str[128] MAYBE_UNUSED;
extern char jkMain_strIdk[128] MAYBE_UNUSED;
extern char16_t jkMain_wstrIdk[128] MAYBE_UNUSED;
extern SithWorldTextSectionParseHandler sithWorld_aSectionParsers[32] MAYBE_UNUSED;
extern uint32_t sithWorld_some_integer_4 MAYBE_UNUSED;
extern SithWorld* sithWorld_g_pCurrentWorld MAYBE_UNUSED;
extern SithWorld* sithWorld_g_pStaticWorld MAYBE_UNUSED;
extern SithWorld* sithWorld_g_pLastLoadedWorld MAYBE_UNUSED;
extern uint32_t sithWorld_numParsers MAYBE_UNUSED;
extern uint32_t sithWorld_bInitted MAYBE_UNUSED;
extern int sithWorld_bLoaded MAYBE_UNUSED;
extern char sithWorld_episodeName[32] MAYBE_UNUSED;
extern SithControlBinding sithInventory_powerKeybinds[SITHINVENTORY_NUM_POWERKEYBINDS] MAYBE_UNUSED;
extern int  sithInventory_g_bInitInventory MAYBE_UNUSED;
extern SithInventoryType sithInventory_g_aTypes[SITHBIN_NUMBINS] MAYBE_UNUSED;
extern int sithInventory_g_bSendDeactivateMessage MAYBE_UNUSED;
extern int sithInventory_bUnkPower MAYBE_UNUSED;
extern int sithInventory_8339EC MAYBE_UNUSED;
extern int sithInventory_bRendIsHidden MAYBE_UNUSED;
extern int sithInventory_8339F4 MAYBE_UNUSED;
extern uint32_t sithWeapon_controlOptions MAYBE_UNUSED;
extern flex_t g_flt_8BD040 MAYBE_UNUSED;
extern flex_t g_flt_8BD044 MAYBE_UNUSED;
extern flex_t g_flt_8BD048 MAYBE_UNUSED;
extern flex_t g_flt_8BD04C MAYBE_UNUSED;
extern flex_t g_flt_8BD050 MAYBE_UNUSED;
extern flex_t g_flt_8BD054 MAYBE_UNUSED;
extern flex_t g_flt_8BD058 MAYBE_UNUSED;
extern int sithWeapon_CurWeaponMode MAYBE_UNUSED;
extern int sithWeapon_bAutoPickup MAYBE_UNUSED;
extern int sithWeapon_bAutoSwitch MAYBE_UNUSED;
extern int sithWeapon_bAutoReload MAYBE_UNUSED;
extern int sithWeapon_bMultiAutoPickup MAYBE_UNUSED;
extern int sithWeapon_bMultiplayerAutoSwitch MAYBE_UNUSED;
extern int sithWeapon_bMultiAutoReload MAYBE_UNUSED;
extern int sithWeapon_bAutoAim MAYBE_UNUSED;
extern flex32_t sithWeapon_secMountWait MAYBE_UNUSED;
extern flex_t sithWeapon_8BD0A0[2] MAYBE_UNUSED;
extern flex32_t sithWeapon_fireWait MAYBE_UNUSED;
extern flex32_t sithWeapon_fireRate MAYBE_UNUSED;
extern flex32_t sithWeapon_LastFireTimeSecs MAYBE_UNUSED;
extern int sithWeapon_a8BD030[4] MAYBE_UNUSED;
extern int sithWeapon_8BD05C MAYBE_UNUSED;
extern flex32_t sithWeapon_8BD060 MAYBE_UNUSED;
extern int sithWeapon_8BD008[6] MAYBE_UNUSED;
extern int sithWeapon_8BD024 MAYBE_UNUSED;
extern int sithWeapon_senderIndex MAYBE_UNUSED;
extern SithPlayer jkPlayer_playerInfos[JKPLAYER_NUM_INFOS] MAYBE_UNUSED;
extern char16_t jkPlayer_playerShortName[32] MAYBE_UNUSED;
extern int jkPlayer_numOtherThings MAYBE_UNUSED;
extern int jkPlayer_numThings MAYBE_UNUSED;
extern jkPlayerInfo jkPlayer_otherThings[NUM_JKPLAYER_THINGS] MAYBE_UNUSED;
extern int  jkPlayer_bLoadingSomething MAYBE_UNUSED;
extern int playerThingIdx MAYBE_UNUSED;
extern uint32_t jkPlayer_maxPlayers MAYBE_UNUSED;
extern SithThing* sithPlayer_g_pLocalPlayerThing MAYBE_UNUSED;
extern SithPlayer* sithPlayer_g_pLocalPlayer MAYBE_UNUSED;
extern jkPlayerInfo playerThings[JKPLAYER_NUM_INFOS] MAYBE_UNUSED;
extern rdMatrix34 jkSaber_rotateMat MAYBE_UNUSED;
extern int jkPlayer_setRotateOverlayMap MAYBE_UNUSED;
extern int jkPlayer_setDrawStatus MAYBE_UNUSED;
extern int jkPlayer_setCrosshair MAYBE_UNUSED;
extern int jkPlayer_setSaberCam MAYBE_UNUSED;
extern int jkPlayer_setFullSubtitles MAYBE_UNUSED;
extern int jkPlayer_setDisableCutscenes MAYBE_UNUSED;
extern int jkPlayer_aCutsceneVal[32] MAYBE_UNUSED;
extern char jkPlayer_cutscenePath[1024] MAYBE_UNUSED;
extern int jkPlayer_setNumCutscenes MAYBE_UNUSED;
extern int jkPlayer_currentTickIdx MAYBE_UNUSED;
extern int jkPlayer_setDiff MAYBE_UNUSED;
extern rdVector3 jkPlayer_waggleVec MAYBE_UNUSED;
extern flex_t jkPlayer_waggleMag MAYBE_UNUSED;
extern int jkPlayer_mpcInfoSet MAYBE_UNUSED;
extern flex_t jkPlayer_waggleAngle MAYBE_UNUSED;
extern rdVector3 jkSaber_rotateVec MAYBE_UNUSED;
extern char16_t jkPlayer_name[32] MAYBE_UNUSED;
extern char jkPlayer_model[32] MAYBE_UNUSED;
extern char jkPlayer_soundClass[32] MAYBE_UNUSED;
extern char jkPlayer_sideMat[32] MAYBE_UNUSED;
extern char jkPlayer_tipMat[32] MAYBE_UNUSED;
extern int sithCollision_aNumSearchedSectors[4] MAYBE_UNUSED;
extern SithCollideResult sithCollision_collisionHandlers[144] MAYBE_UNUSED;
extern sithCollisionHitHandler_t sithCollision_aThingSurfaceCollideResults[12] MAYBE_UNUSED;
extern sithCollisionSearchResult sithCollision_aCollisions[4] MAYBE_UNUSED;
extern int sithCollision_aNumStackCollisions[4] MAYBE_UNUSED;
extern int  sithCollision_searchStackIdx MAYBE_UNUSED;
extern sithCollisionSectorEntry sithCollision_apSearchedSectors[4] MAYBE_UNUSED;
extern int sithCollision_dword_8B4BE4 MAYBE_UNUSED;
extern rdVector3 sithCollision_collideHurtIdk MAYBE_UNUSED;
extern rdVector3  sithSector_surfaceNormal MAYBE_UNUSED;
extern sithSectorEntry sithAIAwareness_aEntries[32] MAYBE_UNUSED;
extern sithSectorAlloc* sithAIAwareness_g_aSectors MAYBE_UNUSED;
extern int sithAIAwareness_numEntries MAYBE_UNUSED;
extern int sithAIAwareness_bInitted MAYBE_UNUSED;
extern int sithAIAwareness_timerTicks MAYBE_UNUSED;
extern flex_t sithSector_flt_8553B8 MAYBE_UNUSED;
extern flex_t sithSector_horizontalPixelsPerRev MAYBE_UNUSED;
extern flex_t sithSector_flt_8553C0 MAYBE_UNUSED;
extern flex_t sithSector_flt_8553C4 MAYBE_UNUSED;
extern flex_t sithSector_flt_8553C8 MAYBE_UNUSED;
extern rdVector3 sithSector_zMaxVec MAYBE_UNUSED;
extern flex_t sithSector_ceilingSky MAYBE_UNUSED;
extern rdVector3 sithSector_zMinVec MAYBE_UNUSED;
extern flex_t sithSector_horizontalPixelsPerRev_idk MAYBE_UNUSED;
extern flex_t sithSector_horizontalDist MAYBE_UNUSED;
extern flex_t sithSector_flt_8553F4 MAYBE_UNUSED;
extern SithSector* sithSector_aModifiedSectors[16] MAYBE_UNUSED;
extern int sithSector_aSyncFlags[16] MAYBE_UNUSED;
extern uint32_t sithSector_numModifiedSectors MAYBE_UNUSED;
extern tHashTable* sithThing_pParseHashtbl MAYBE_UNUSED;
extern sithThing_handler_t sithThing_pfUnknownFunc MAYBE_UNUSED;
extern uint16_t  sithThing_guidEntropy MAYBE_UNUSED;
extern char16_t jkGuiSaveLoad_wtextEpisode[256] MAYBE_UNUSED;
extern char16_t jkGuiSaveLoad_wtextHealth[64] MAYBE_UNUSED;
extern int jkGuiSaveLoad_numEntries MAYBE_UNUSED;
extern char16_t jkGuiSaveLoad_wtextShields[64] MAYBE_UNUSED;
extern char16_t jkGuiSaveLoad_word_559830[256] MAYBE_UNUSED;
extern int jkGuiSaveLoad_bIsSaveMenu MAYBE_UNUSED;
extern char16_t jkGuiSaveLoad_wtextSaveName[256] MAYBE_UNUSED;
extern Darray jkGuiSaveLoad_DarrayEntries MAYBE_UNUSED;
extern char16_t jkGuiSaveLoad_word_559C54[10] MAYBE_UNUSED;
extern char jkGuiSaveLoad_byte_559C50[4] MAYBE_UNUSED;
extern int  jkGuiTitle_verMajor MAYBE_UNUSED;
extern int  jkGuiTitle_verMinor MAYBE_UNUSED;
extern int  jkGuiTitle_verRevision MAYBE_UNUSED;
extern jkGuiStringEntry jkGuiTitle_aTexts[20] MAYBE_UNUSED;
extern int jkGuiTitle_whichLoading MAYBE_UNUSED;
extern flex_t  jkGuiSound_sfxVolume MAYBE_UNUSED;
extern uint32_t  jkGuiSound_numChannels MAYBE_UNUSED;
extern int jkGuiSound_bLowResSound MAYBE_UNUSED;
extern int  jkGuiSound_b3DSound MAYBE_UNUSED;
extern int  jkGuiSound_b3DSound_2 MAYBE_UNUSED;
extern int  jkGuiSound_b3DSound_3 MAYBE_UNUSED;
extern flex_t jkGuiSound_musicVolume MAYBE_UNUSED;
extern char jkGui_unkstr[32] MAYBE_UNUSED;
extern int jkGui_GdiMode MAYBE_UNUSED;
extern int jkGui_modesets MAYBE_UNUSED;
extern stdBitmap* jkGui_stdBitmaps[0x27] MAYBE_UNUSED;
extern stdFont* jkGui_stdFonts[16] MAYBE_UNUSED;
extern jkEpisodeLoad jkGui_episodeLoad MAYBE_UNUSED;
extern stdBitmap* jkGuiSingleTally_foStars MAYBE_UNUSED;
extern int jkGuiNetHost_maxRank MAYBE_UNUSED;
extern int jkGuiNetHost_timeLimit MAYBE_UNUSED;
extern int jkGuiNetHost_scoreLimit MAYBE_UNUSED;
extern int jkGuiNetHost_maxPlayers MAYBE_UNUSED;
extern int jkGuiNetHost_sessionFlags MAYBE_UNUSED;
extern int jkGuiNetHost_gameFlags MAYBE_UNUSED;
extern int jkGuiNetHost_tickRate MAYBE_UNUSED;
extern char16_t jkGuiNetHost_gameName[32] MAYBE_UNUSED;
extern int jkGuiMultiplayer_checksumSeed MAYBE_UNUSED;
extern int jkGuiMultiplayer_dword_5564EC MAYBE_UNUSED;
extern int jkGuiMultiplayer_dword_5564E8 MAYBE_UNUSED;
extern jkMultiEntry jkGuiMultiplayer_multiEntry MAYBE_UNUSED;
extern int jkGuiMultiplayer_dword_5564F0 MAYBE_UNUSED;
extern HINSTANCE g_hInstance MAYBE_UNUSED;
extern SithCogSymbolTable* sithCog_g_pSymbolTable MAYBE_UNUSED;
extern struct HostServices* pSithHS MAYBE_UNUSED;
extern HWND g_hWnd MAYBE_UNUSED;
extern uint32_t g_nShowCmd MAYBE_UNUSED;
extern uint32_t g_app_suspended MAYBE_UNUSED;
extern uint32_t g_window_active MAYBE_UNUSED;
extern uint32_t g_app_active MAYBE_UNUSED;
extern uint32_t g_should_exit MAYBE_UNUSED;
extern uint32_t g_thing_two_some_dialog_count MAYBE_UNUSED;
extern uint32_t g_handler_count MAYBE_UNUSED;
extern uint32_t g_855E8C MAYBE_UNUSED;
extern uint32_t g_855E90 MAYBE_UNUSED;
extern uint32_t g_window_not_destroyed MAYBE_UNUSED;
extern stdPalEffectsState stdPalEffects_state MAYBE_UNUSED;
extern rdColor24 stdPalEffects_palette[256] MAYBE_UNUSED;
extern uint32_t stdPalEffects_numEffectRequests MAYBE_UNUSED;
extern stdPalEffectRequest stdPalEffects_aEffects[32] MAYBE_UNUSED;
extern stdPalEffectSetPaletteFunc_t stdPalEffects_setPalette MAYBE_UNUSED;
extern uint16_t stdPalEffects_aPalette[256] MAYBE_UNUSED;
extern char aFilenameStack[2560] MAYBE_UNUSED;
extern char* apBufferStack[20] MAYBE_UNUSED;
extern int linenumStack[20] MAYBE_UNUSED;
extern char aEntryStack[0x14*(STDCONF_LINEBUFFER_LEN+4)] MAYBE_UNUSED;
extern stdFile_t openFileStack[20] MAYBE_UNUSED;
extern char printfBuffer[STDCONF_LINEBUFFER_LEN] MAYBE_UNUSED;
extern int stdConffile_linenum MAYBE_UNUSED;
extern int stdConffile_bOpen MAYBE_UNUSED;
extern stdFile_t openFile MAYBE_UNUSED;
extern stdFile_t writeFile MAYBE_UNUSED;
extern uint32_t stackLevel MAYBE_UNUSED;
extern char stdConffile_aWriteFilename[128] MAYBE_UNUSED;
extern StdConffileEntry stdConffile_g_entry MAYBE_UNUSED;
extern char stdConffile_pFilename[128] MAYBE_UNUSED;
extern char* stdConffile_g_aLine MAYBE_UNUSED;
extern int stdMemory_bInitted MAYBE_UNUSED;
extern int stdMemory_bOpened MAYBE_UNUSED;
extern tMemoryState stdMemory_g_curState MAYBE_UNUSED;
extern stdFile_t yyin MAYBE_UNUSED;
extern stdFile_t yyout MAYBE_UNUSED;
extern SithCogSymbolTable* sithCogParse_pSymbolTable MAYBE_UNUSED;
extern int yacc_linenum MAYBE_UNUSED;
extern int  cog_yacc_loop_depth MAYBE_UNUSED;
extern int cog_parser_node_stackpos[SITHCOG_NODE_STACKDEPTH] MAYBE_UNUSED;
extern int cogvm_stackpos MAYBE_UNUSED;
extern sith_cog_parser_node* cogparser_nodes_alloc MAYBE_UNUSED;
extern sith_cog_parser_node* cogparser_topnode MAYBE_UNUSED;
extern int32_t* cogvm_stack MAYBE_UNUSED;
extern int cogparser_num_nodes MAYBE_UNUSED;
extern int cogparser_current_nodeidx MAYBE_UNUSED;
extern SithCogScript* parsing_script MAYBE_UNUSED;
extern int  parsing_script_idk MAYBE_UNUSED;
extern int dplay_dword_55D618 MAYBE_UNUSED;
extern int dplay_dword_55D61C MAYBE_UNUSED;
extern GUID_idk jkGui_guid_556040 MAYBE_UNUSED;
extern int jkGuiMultiplayer_numConnections MAYBE_UNUSED;
extern sith_dplay_connection jkGuiMultiplayer_aConnections[32] MAYBE_UNUSED;
extern jkMultiEntry jkGuiMultiplayer_aEntries[32] MAYBE_UNUSED;
extern jkMultiEntry2 jkGuiMultiplayer_stru_556168 MAYBE_UNUSED;
extern jkPlayerMpcInfo jkGuiMultiplayer_mpcInfo MAYBE_UNUSED;
extern Darray jkGuiMultiplayer_stru_5564A8 MAYBE_UNUSED;
extern int jkGuiMouse_bOpen MAYBE_UNUSED;
extern Darray jkGuiMouse_Darray_556698 MAYBE_UNUSED;
extern int jkGuiMouse_dword_5566B0 MAYBE_UNUSED;
extern Darray jkGuiMouse_Darray_5566B8 MAYBE_UNUSED;
extern Darray jkGuiMouse_Darray_5566D0 MAYBE_UNUSED;
extern char16_t* jkGuiMouse_pWStr_5566E8 MAYBE_UNUSED;
extern int jkGuiEsc_bInitialized MAYBE_UNUSED;
extern int jkGuiKeyboard_dword_555DE0 MAYBE_UNUSED;
extern int jkGuiKeyboard_bOnceIdk MAYBE_UNUSED;
extern int jkGuiKeyboard_funcIdx MAYBE_UNUSED;
extern int jkGuiKeyboard_flags MAYBE_UNUSED;
extern Darray jkGuiKeyboard_darrEntries MAYBE_UNUSED;
extern int jkGuiKeyboard_dword_555E10 MAYBE_UNUSED;
extern char16_t jkGuiKeyboard_wstr_555E18[257] MAYBE_UNUSED;
extern char16_t* jkGuiKeyboard_pWStr_55601C MAYBE_UNUSED;
extern rdVector3 jkGuiMap_vec3Idk2 MAYBE_UNUSED;
extern rdCanvas* jkGuiMap_pCanvas MAYBE_UNUSED;
extern rdMatrix34 jkGuiMap_viewMat MAYBE_UNUSED;
extern rdMatrix34 jkGuiMap_matTmp MAYBE_UNUSED;
extern tVBuffer* jkGuiMap_pVbuffer MAYBE_UNUSED;
extern sithMap jkGuiMap_unk4 MAYBE_UNUSED;
extern rdVector3 jkGuiMap_vec3Idk MAYBE_UNUSED;
extern rdCamera* jkGuiMap_pCamera MAYBE_UNUSED;
extern int jkGuiMap_dword_556660 MAYBE_UNUSED;
extern int jkGuiMap_bOrbitActive MAYBE_UNUSED;
extern int jkGuiMap_dword_556668 MAYBE_UNUSED;
extern int jkGuiMap_dword_55666C MAYBE_UNUSED;
extern flex_t sithMap_unkArr[12]  MAYBE_UNUSED;
extern SithThing* sithMap_pPlayerThing MAYBE_UNUSED;
extern rdMatrix34 sithMap_invMatrix MAYBE_UNUSED;
extern flex_t sithMap_flt_84DEA8 MAYBE_UNUSED;
extern flex_t sithMap_flt_84DEAC MAYBE_UNUSED;
extern sithMap sithMap_ctx MAYBE_UNUSED;
extern SithWorld* sithMap_pCurWorld MAYBE_UNUSED;
extern rdMatrix34 sithMap_camera MAYBE_UNUSED;
extern rdCamera* sithMap_pCurCamera MAYBE_UNUSED;
extern int sithMap_bInitted MAYBE_UNUSED;
extern int sithMap_var MAYBE_UNUSED;
extern uint32_t DirectPlay_numPlayers MAYBE_UNUSED;
extern sithDplayPlayer DirectPlay_aPlayers[32] MAYBE_UNUSED;

#else

// Vars
#define sithAIClass_g_pHashtable (*(tHashTable**)sithAIClass_g_pHashtable_ADDR)
#define std_g_genBuffer ((char*)std_g_genBuffer_ADDR)
#define std_g_pHS (*(struct HostServices**)std_g_pHS_ADDR)
#define rdModel3_pCurGeoset (*(rdGeoset**)rdModel3_pCurGeoset_ADDR)
#define localCamera (*(rdVector3*)localCamera_ADDR)
#define aFaceVerts ((rdVector3*)aFaceVerts_ADDR)
#define vertexDst (*(rdMeshinfo*)vertexDst_ADDR)
#define curGeometryMode (*(int*)curGeometryMode_ADDR)
#define apGeoLights ((rdLight**)apGeoLights_ADDR)
#define rdModel3_aLocalLightPos ((rdVector3*)rdModel3_aLocalLightPos_ADDR)
#define rdModel3_aLocalLightDir ((rdVector3*)rdModel3_aLocalLightDir_ADDR)
#define meshFrustumCull (*(int*)meshFrustumCull_ADDR)
#define curTextureMode (*(int*)curTextureMode_ADDR)
#define aView ((rdVector3*)aView_ADDR)
#define pCurMesh (*(rdMesh**)pCurMesh_ADDR)
#define thingFrustumCull (*(int*)thingFrustumCull_ADDR)
#define vertexSrc (*(rdMeshinfo*)vertexSrc_ADDR)
#define pCurModel3 (*(rdModel3**)pCurModel3_ADDR)
#define rdModel3_textureMode (*(int*)rdModel3_textureMode_ADDR)
#define curLightingMode (*(int*)curLightingMode_ADDR)
#define apMeshLights ((rdLight**)apMeshLights_ADDR)
#define pCurThing (*(rdThing**)pCurThing_ADDR)
#define rdModel3_lightingMode (*(int*)rdModel3_lightingMode_ADDR)
#define rdModel3_geometryMode (*(int*)rdModel3_geometryMode_ADDR)
#define rdModel3_numDrawnModels (*(int*)rdModel3_numDrawnModels_ADDR)
#define pModel3Loader (*(model3Loader_t*)pModel3Loader_ADDR)
#define pModel3Unloader (*(model3Unloader_t*)pModel3Unloader_ADDR)
#define rdModel3_numGeoLights (*(uint32_t*)rdModel3_numGeoLights_ADDR)
#define rdModel3_numMeshLights (*(int*)rdModel3_numMeshLights_ADDR)
#define rdModel3_fRadius (*(flex_t*)rdModel3_fRadius_ADDR)
#define sithPuppet_pClassHashtable (*(tHashTable**)sithPuppet_pClassHashtable_ADDR)
#define sithPuppet_pKeyHashtable (*(tHashTable**)sithPuppet_pKeyHashtable_ADDR)
#define sithPuppet_pHashtblSubmodes (*(tHashTable**)sithPuppet_pHashtblSubmodes_ADDR)
#define pKeyframeLoader (*(keyframeLoader_t*)pKeyframeLoader_ADDR)
#define pKeyframeUnloader (*(keyframeUnloader_t*)pKeyframeUnloader_ADDR)
#define sithTime_g_frameTime (*(int32_t*)sithTime_g_frameTime_ADDR)
#define sithTime_g_frameTimeFlex (*(flex_t*)sithTime_g_frameTimeFlex_ADDR)
#define sithTime_g_fps (*(flex_t *)sithTime_g_fps_ADDR)
#define sithTime_g_msecGameTime (*(uint32_t*)sithTime_g_msecGameTime_ADDR)
#define sithTime_g_secGameTime (*(flex32_t*)sithTime_g_secGameTime_ADDR)
#define sithTime_g_clockTime (*(uint32_t*)sithTime_g_clockTime_ADDR)
#define sithTime_msecPauseStartTime (*(int32_t*)sithTime_msecPauseStartTime_ADDR)
#define sithTime_g_bPaused (*(int*)sithTime_g_bPaused_ADDR)
#define sithRender_texMode (*(int*)sithRender_texMode_ADDR)
#define sithRender_renderflags (*(int*)sithRender_renderflags_ADDR)
#define sithRender_geoMode (*(int*)sithRender_geoMode_ADDR)
#define sithRender_lightMode (*(int*)sithRender_lightMode_ADDR)
#define sithRender_lightingIRMode (*(int*)sithRender_lightingIRMode_ADDR)
#define sithRender_f_83198C (*(flex_t*)sithRender_f_83198C_ADDR)
#define sithRender_f_831990 (*(flex_t*)sithRender_f_831990_ADDR)
#define sithRender_bResetCameraAspect (*(int*)sithRender_bResetCameraAspect_ADDR)
#define sithRender_g_numVisibleSectors (*(int32_t*)sithRender_g_numVisibleSectors_ADDR)
#define sithRender_numSecorFrustrums (*(int32_t*)sithRender_numSecorFrustrums_ADDR)
#define sithRender_numThingLights (*(int32_t*)sithRender_numThingLights_ADDR)
#define sithRender_numThingSectors (*(int32_t*)sithRender_numThingSectors_ADDR)
#define sithRender_numSpritesToDraw (*(uint32_t*)sithRender_numSpritesToDraw_ADDR)
#define sithRender_numRenderedSectors (*(int*)sithRender_numRenderedSectors_ADDR)
#define sithRender_numAlphaAdjoins (*(int32_t*)sithRender_numAlphaAdjoins_ADDR)
#define sithRender_geoThingsDrawn (*(int*)sithRender_geoThingsDrawn_ADDR)
#define sithRender_nongeoThingsDrawn (*(int*)sithRender_nongeoThingsDrawn_ADDR)
#define sithRender_f_82F4B0 (*(flex_t *)sithRender_f_82F4B0_ADDR)
#define sithRender_faceView (*(rdMeshinfo*)sithRender_faceView_ADDR)
#define meshinfo_out (*(rdMeshinfo*)meshinfo_out_ADDR)
#define sithRender_pExtraThingRenderFunc (*(sithRender_weapRendFunc_t*)sithRender_pExtraThingRenderFunc_ADDR)
#define sithRender_aThingLights ((rdLight*)sithRender_aThingLights_ADDR)
#define sithRender_aVisibleSectors ((SithSector**)sithRender_aVisibleSectors_ADDR)
#define sithRender_aSectorFrustrums ((rdClipFrustum*)sithRender_aSectorFrustrums_ADDR)
#define sithRender_aThingSectors ((SithSector**)sithRender_aThingSectors_ADDR)
#define sithRender_aClipVertices ((rdVector3*)sithRender_aClipVertices_ADDR)
#define sithRender_aTransformedClipVertices ((rdVector3*)sithRender_aTransformedClipVertices_ADDR)
#define sithRender_aAlphaAdjoins ((SithSurface**)sithRender_aAlphaAdjoins_ADDR)
#define sithRender_lastRenderTick (*(int*)sithRender_lastRenderTick_ADDR)
#define aSithSurfaces ((SithSurface**)aSithSurfaces_ADDR)
#define sithCamera_g_aCameras ((SithCamera*)sithCamera_g_aCameras_ADDR)
#define sithCamera_g_bCurCameraSet (*(int*)sithCamera_g_bCurCameraSet_ADDR)
#define sithCamera_g_stateFlags (*(int*)sithCamera_g_stateFlags_ADDR)
#define sithCamera_g_curCycleCamNum (*(int*)sithCamera_g_curCycleCamNum_ADDR)
#define sithCamera_g_vecCameraPosOffset (*(rdVector3*)sithCamera_g_vecCameraPosOffset_ADDR)
#define sithCamera_g_vecCameraAngleOffset (*(rdVector3*)sithCamera_g_vecCameraAngleOffset_ADDR)
#define sithCamera_g_cameraPosDelta (*(flex_t*)sithCamera_g_cameraPosDelta_ADDR)
#define sithCamera_g_cameraAngleDelta (*(flex_t*)sithCamera_g_cameraAngleDelta_ADDR)
#define sithCamera_g_pCurCamera (*(SithCamera**)sithCamera_g_pCurCamera_ADDR)
#define sithCamera_bStartup (*(int*)sithCamera_bStartup_ADDR)
#define sithCamera_g_orbCamOrient (*(rdMatrix34*)sithCamera_g_orbCamOrient_ADDR)
#define sithCamera_idleCamOrient (*(rdMatrix34*)sithCamera_idleCamOrient_ADDR)
#define sithCamera_bOpen (*(int*)sithCamera_bOpen_ADDR)
#define rdroid_aMipDistances (*(rdVector4 *)rdroid_aMipDistances_ADDR)
#define rdroid_frameTrue (*(int*)rdroid_frameTrue_ADDR)
#define bRDroidStartup (*(int*)bRDroidStartup_ADDR)
#define bRDroidOpen (*(int*)bRDroidOpen_ADDR)
#define rdroid_g_curLightingMode (*(int*)rdroid_g_curLightingMode_ADDR)
#define rdroid_g_pHS (*(struct HostServices**)rdroid_g_pHS_ADDR)
#define rdroid_g_curGeometryMode (*(int*)rdroid_g_curGeometryMode_ADDR)
#define rdroid_curColorEffects (*(stdPalEffect*)rdroid_curColorEffects_ADDR)
#define rdroid_curOcclusionMethod (*(int*)rdroid_curOcclusionMethod_ADDR)
#define rdroid_curZBufferMethod (*(rdZBufferMethod_t*)rdroid_curZBufferMethod_ADDR)
#define rdroid_curProcFaceUserData (*(int*)rdroid_curProcFaceUserData_ADDR)
#define rdroid_curSortingMethod (*(int*)rdroid_curSortingMethod_ADDR)
#define rdroid_curAcceleration (*(int*)rdroid_curAcceleration_ADDR)
#define rdroid_curTextureMode (*(int*)rdroid_curTextureMode_ADDR)
#define rdroid_g_curRenderOptions (*(int*)rdroid_g_curRenderOptions_ADDR)
#define rdroid_curCullFlags (*(int*)rdroid_curCullFlags_ADDR)
#define sithMaterial_pHashtable (*(tHashTable**)sithMaterial_pHashtable_ADDR)
#define sithMaterial_aMaterials (*(rdMaterial***)sithMaterial_aMaterials_ADDR)
#define sithMaterial_numMaterials (*(int*)sithMaterial_numMaterials_ADDR)
#define rdCache_aHWSolidTris ((rdTri*)rdCache_aHWSolidTris_ADDR)
#define rdCache_totalNormalTris (*(int*)rdCache_totalNormalTris_ADDR)
#define rdCache_aIntensities ((flex_t*)rdCache_aIntensities_ADDR)
#define rdCache_aVertices ((rdVector3*)rdCache_aVertices_ADDR)
#define rdCache_totalVerts (*(int*)rdCache_totalVerts_ADDR)
#define rdCache_aTexVertices ((rdVector2*)rdCache_aTexVertices_ADDR)
#define rdCache_aHWNormalTris ((rdTri*)rdCache_aHWNormalTris_ADDR)
#define rdCache_totalSolidTris (*(int*)rdCache_totalSolidTris_ADDR)
#define rdCache_aHWVertices ((D3DVERTEX*)rdCache_aHWVertices_ADDR)
#define rdCache_drawnFaces (*(int*)rdCache_drawnFaces_ADDR)
#define rdCache_numUsedVertices (*(int*)rdCache_numUsedVertices_ADDR)
#define rdCache_numUsedTexVertices (*(int*)rdCache_numUsedTexVertices_ADDR)
#define rdCache_numUsedIntensities (*(int*)rdCache_numUsedIntensities_ADDR)
#define rdCache_ulcExtent (*(rdVector2i*)rdCache_ulcExtent_ADDR)
#define rdCache_lrcExtent (*(rdVector2i*)rdCache_lrcExtent_ADDR)
#define rdCache_numProcFaces (*(int*)rdCache_numProcFaces_ADDR)
#define rdCache_aProcFaces ((rdProcEntry*)rdCache_aProcFaces_ADDR)
#define rdCache_dword_865258 (*(int*)rdCache_dword_865258_ADDR)
#define sithMulti_name ((char16_t*)sithMulti_name_ADDR)
#define sithMulti_pfNewPlayerJoinedCallback (*(sithMultiHandler_t*)sithMulti_pfNewPlayerJoinedCallback_ADDR)
#define sithMulti_multiModeFlags (*(int*)sithMulti_multiModeFlags_ADDR)
#define sithMulti_numRemovedStaticThings (*(uint32_t*)sithMulti_numRemovedStaticThings_ADDR)
#define sithMulti_aRemovedStaticThings ((int*)sithMulti_aRemovedStaticThings_ADDR)
#define sithMulti_msecQuitGameTime (*(uint32_t*)sithMulti_msecQuitGameTime_ADDR)
#define sithMulti_msecPingStartTime (*(int*)sithMulti_msecPingStartTime_ADDR)
#define sithMulti_msecLastSyncScoreTime (*(uint32_t*)sithMulti_msecLastSyncScoreTime_ADDR)
#define sithMulti_curWelcomePlayerNum (*(uint32_t*)sithMulti_curWelcomePlayerNum_ADDR)
#define sithMulti_msecWelcomeUpdateInterval (*(int*)sithMulti_msecWelcomeUpdateInterval_ADDR)
#define rdColormap_pCurMap (*(rdColormap**)rdColormap_pCurMap_ADDR)
#define rdColormap_pIdentityMap (*(rdColormap**)rdColormap_pIdentityMap_ADDR)
#define sithEvent_aEvents ((SithEvent*)sithEvent_aEvents_ADDR)
#define sithEvent_g_pFirstQueuedEvent (*(SithEvent**)sithEvent_g_pFirstQueuedEvent_ADDR)
#define sithEvent_arrLut ((int*)sithEvent_arrLut_ADDR)
#define sithEvent_aTasks ((SithEventTask*)sithEvent_aTasks_ADDR)
#define sithEvent_numFreeEventBuffers (*(int*)sithEvent_numFreeEventBuffers_ADDR)
#define sithEvent_bInit (*(int*)sithEvent_bInit_ADDR)
#define sithEvent_bOpen (*(int*)sithEvent_bOpen_ADDR)
#define rdClip_g_faceStatus (*(int*)rdClip_g_faceStatus_ADDR)
#define rdClip_pSourceVert (*(rdVector3**)rdClip_pSourceVert_ADDR)
#define rdClip_workIVerts ((flex_t*)rdClip_workIVerts_ADDR)
#define rdClip_aWorkVerts ((rdVector3*)rdClip_aWorkVerts_ADDR)
#define rdClip_pDestVert (*(rdVector3**)rdClip_pDestVert_ADDR)
#define rdClip_pDestIVert (*(flex_t**)rdClip_pDestIVert_ADDR)
#define rdClip_aWorkTVerts ((rdVector2*)rdClip_aWorkTVerts_ADDR)
#define rdClip_pSourceIVert (*(flex_t**)rdClip_pSourceIVert_ADDR)
#define rdClip_pSourceTVert (*(rdVector2**)rdClip_pSourceTVert_ADDR)
#define rdClip_pDestTVert (*(rdVector2**)rdClip_pDestTVert_ADDR)
#define sithSoundMixer_dword_835FCC (*(int*)sithSoundMixer_dword_835FCC_ADDR)
#define sithSoundMixer_bPlayingMci (*(int*)sithSoundMixer_bPlayingMci_ADDR)
#define sithSoundMixer_bInitted (*(int*)sithSoundMixer_bInitted_ADDR)
#define sithSoundMixer_flt_835FD8 (*(flex_t*)sithSoundMixer_flt_835FD8_ADDR)
#define sithSoundMixer_bIsMuted (*(int*)sithSoundMixer_bIsMuted_ADDR)
#define sithSoundMixer_musicVolume (*(flex_t *)sithSoundMixer_musicVolume_ADDR)
#define sithSoundMixer_globalVolume (*(flex_t *)sithSoundMixer_globalVolume_ADDR)
#define sithSoundMixer_numSoundsAvailable2 (*(int*)sithSoundMixer_numSoundsAvailable2_ADDR)
#define sithSoundMixer_numSoundsAvailable (*(int*)sithSoundMixer_numSoundsAvailable_ADDR)
#define sithSoundMixer_aPlayingSounds ((sithPlayingSound*)sithSoundMixer_aPlayingSounds_ADDR)
#define sithSoundMixer_aIdk ((int*)sithSoundMixer_aIdk_ADDR)
#define sithSoundMixer_activeChannels (*(int*)sithSoundMixer_activeChannels_ADDR)
#define sithSoundMixer_bOpened (*(int*)sithSoundMixer_bOpened_ADDR)
#define sithSoundMixer_pCurSector (*(SithSector**)sithSoundMixer_pCurSector_ADDR)
#define sithSoundMixer_dword_836BFC (*(int*)sithSoundMixer_dword_836BFC_ADDR)
#define sithSoundMixer_trackFrom (*(int*)sithSoundMixer_trackFrom_ADDR)
#define sithSoundMixer_trackTo (*(int*)sithSoundMixer_trackTo_ADDR)
#define sithSoundMixer_hCurAmbientChannel (*(sithPlayingSound**)sithSoundMixer_hCurAmbientChannel_ADDR)
#define sithSoundMixer_dword_836C00 (*(int*)sithSoundMixer_dword_836C00_ADDR)
#define sithSoundMixer_channelGUIDSeed (*(int *)sithSoundMixer_channelGUIDSeed_ADDR)
#define sithSoundMixer_dword_836C04 (*(int*)sithSoundMixer_dword_836C04_ADDR)
#define sithSoundMixer_pFocusedThing (*(SithThing**)sithSoundMixer_pFocusedThing_ADDR)
#define sithSound_maxDataLoaded (*(int *)sithSound_maxDataLoaded_ADDR)
#define sithSound_var3 (*(int *)sithSound_var3_ADDR)
#define sithSound_curDataLoaded (*(int*)sithSound_curDataLoaded_ADDR)
#define sithSound_bInit (*(int*)sithSound_bInit_ADDR)
#define sithSound_hashtable (*(tHashTable**)sithSound_hashtable_ADDR)
#define sithSound_var4 (*(int*)sithSound_var4_ADDR)
#define sithSound_var5 (*(int*)sithSound_var5_ADDR)
#define sithControl_inputFuncToControlType ((int*)sithControl_inputFuncToControlType_ADDR)
#define sithControl_aInputFuncToKeyinfo ((stdControlKeyInfo*)sithControl_aInputFuncToKeyinfo_ADDR)
#define sithControl_msIdle (*(uint32_t*)sithControl_msIdle_ADDR)
#define sithControl_bInitted (*(int*)sithControl_bInitted_ADDR)
#define sithControl_bOpened (*(int*)sithControl_bOpened_ADDR)
#define sithControl_aHandlers ((sithControl_handler_t*)sithControl_aHandlers_ADDR)
#define sithControl_numHandlers (*(int*)sithControl_numHandlers_ADDR)
#define sithControl_death_msgtimer (*(int*)sithControl_death_msgtimer_ADDR)
#define sithControl_vec3_54A570 (*(rdVector3 *)sithControl_vec3_54A570_ADDR)
#define sithControl_flt_54A57C (*(flex_t *)sithControl_flt_54A57C_ADDR)
#define sithGamesave_func1 (*(sithSaveHandler_t*)sithGamesave_func1_ADDR)
#define sithGamesave_func2 (*(sithSaveHandler_t*)sithGamesave_func2_ADDR)
#define sithGamesave_func3 (*(sithSaveHandler_t*)sithGamesave_func3_ADDR)
#define sithGamesave_funcWrite (*(sithSaveHandler_t*)sithGamesave_funcWrite_ADDR)
#define sithGamesave_funcRead (*(sithSaveHandler_t*)sithGamesave_funcRead_ADDR)
#define sithGamesave_autosave_fname ((char*)sithGamesave_autosave_fname_ADDR)
#define sithGamesave_state (*(sithGamesaveState_t*)sithGamesave_state_ADDR)
#define sithGamesave_dword_835914 (*(int*)sithGamesave_dword_835914_ADDR)
#define sithGamesave_aCurFilename ((char*)sithGamesave_aCurFilename_ADDR)
#define sithGamesave_wsaveName ((char16_t*)sithGamesave_wsaveName_ADDR)
#define sithGamesave_saveName ((char*)sithGamesave_saveName_ADDR)
#define sithGamesave_headerTmp (*(sithGamesave_Header*)sithGamesave_headerTmp_ADDR)
#define rdCamera_g_pCurCamera (*(rdCamera**)rdCamera_g_pCurCamera_ADDR)
#define rdCamera_g_camMatrix (*(rdMatrix34*)rdCamera_g_camMatrix_ADDR)
#define sithNet_MultiModeFlags (*(int32_t *)sithNet_MultiModeFlags_ADDR)
#define sithNet_scorelimit (*(int32_t *)sithNet_scorelimit_ADDR)
#define sithNet_teamScore ((int32_t *)sithNet_teamScore_ADDR)
#define sithNet_multiplayer_timelimit (*(int32_t *)sithNet_multiplayer_timelimit_ADDR)
#define sithMulti_multiplayerTimelimit (*(int32_t *)sithMulti_multiplayerTimelimit_ADDR)
#define sithNet_isMulti (*(int32_t *)sithNet_isMulti_ADDR)
#define sithNet_isServer (*(int32_t *)sithNet_isServer_ADDR)
#define sithMulti_bTimelimitMet (*(int32_t *)sithMulti_bTimelimitMet_ADDR)
#define sithNet_serverNetId (*(int32_t *)sithNet_serverNetId_ADDR)
#define sithNet_things ((int32_t *)sithNet_things_ADDR)
#define sithNet_thingsIdx (*(int32_t *)sithNet_thingsIdx_ADDR)
#define sithNet_syncIdx (*(int32_t *)sithNet_syncIdx_ADDR)
#define sithNet_aSyncFlags ((int32_t *)sithNet_aSyncFlags_ADDR)
#define sithNet_aSyncThings ((SithThing**)sithNet_aSyncThings_ADDR)
#define sithNet_tickrate (*(int32_t  *)sithNet_tickrate_ADDR)
#define sithNet_dword_8C4BA8 (*(int32_t *)sithNet_dword_8C4BA8_ADDR)
#define sithNet_dword_83262C (*(int32_t *)sithNet_dword_83262C_ADDR)
#define sithMulti_quitGameState (*(int32_t *)sithMulti_quitGameState_ADDR)
#define sithNet_checksum (*(int32_t *)sithNet_checksum_ADDR)
#define sithNet_bNeedsFullThingSyncForLeaveJoin (*(int32_t *)sithNet_bNeedsFullThingSyncForLeaveJoin_ADDR)
#define sithMulti_newPlayerId (*(int32_t *)sithMulti_newPlayerId_ADDR)
#define sithNet_bSyncScores (*(int32_t *)sithNet_bSyncScores_ADDR)
#define sithNet_dword_832620 (*(int32_t *)sithNet_dword_832620_ADDR)
#define sithOverlayMap_curScale (*(flex_t *)sithOverlayMap_curScale_ADDR)
#define sithOverlayMap_mapOrient (*(rdMatrix34*)sithOverlayMap_mapOrient_ADDR)
#define sithOverlayMap_pLocalPlayer (*(SithThing**)sithOverlayMap_pLocalPlayer_ADDR)
#define sithOverlayMap_pCanvas (*(rdCanvas**)sithOverlayMap_pCanvas_ADDR)
#define sithOverlayMap_x1 (*(int*)sithOverlayMap_x1_ADDR)
#define sithOverlayMap_y1 (*(int*)sithOverlayMap_y1_ADDR)
#define sithOverlayMap_inst (*(sithMapView*)sithOverlayMap_inst_ADDR)
#define sithOverlayMap_bMapVisible (*(int*)sithOverlayMap_bMapVisible_ADDR)
#define sithOverlayMap_bOpened (*(int*)sithOverlayMap_bOpened_ADDR)
#define sithSoundClass_pHashtblModes (*(tHashTable**)sithSoundClass_pHashtblModes_ADDR)
#define sithSoundClass_pHashTable (*(tHashTable**)sithSoundClass_pHashTable_ADDR)
#define sithTemplate_pMasterFile (*(void**)sithTemplate_pMasterFile_ADDR)
#define sithTemplate_pHashtable (*(tHashTable**)sithTemplate_pHashtable_ADDR)
#define sithTemplate_masterFileCount (*(int*)sithTemplate_masterFileCount_ADDR)
#define sithTemplate_pMasterHashtable (*(tHashTable**)sithTemplate_pMasterHashtable_ADDR)
#define activeEdgeTail (*(rdEdge NO_REINIT*)activeEdgeTail_ADDR)
#define activeEdgeHead (*(rdEdge NO_REINIT*)activeEdgeHead_ADDR)
#define apNewActiveEdges ((rdEdge**)apNewActiveEdges_ADDR)
#define apRemoveActiveEdges ((rdEdge**)apRemoveActiveEdges_ADDR)
#define yMinEdge (*(int*)yMinEdge_ADDR)
#define yMaxEdge (*(int*)yMaxEdge_ADDR)
#define numActiveSpans (*(int*)numActiveSpans_ADDR)
#define numActiveFaces (*(int*)numActiveFaces_ADDR)
#define numActiveEdges (*(int*)numActiveEdges_ADDR)
#define aActiveEdges ((rdEdge*)aActiveEdges_ADDR)
#define rdActive_drawnFaces (*(int*)rdActive_drawnFaces_ADDR)
#define sithMain_bEndLevel (*(int*)sithMain_bEndLevel_ADDR)
#define sith_bStartup (*(int*)sith_bStartup_ADDR)
#define sith_bOpen (*(int*)sith_bOpen_ADDR)
#define sithSurface_numAvail (*(int*)sithSurface_numAvail_ADDR)
#define sithSurface_aAvail ((int*)sithSurface_aAvail_ADDR)
#define sithSurface_numSurfaces (*(int*)sithSurface_numSurfaces_ADDR)
#define sithSurface_aSurfaces ((rdSurface*)sithSurface_aSurfaces_ADDR)
#define sithSurface_bOpened (*(int*)sithSurface_bOpened_ADDR)
#define sithSurface_byte_8EE668 (*(uint32_t *)sithSurface_byte_8EE668_ADDR)
#define sithSurface_numUnsyncedSurfaces (*(int*)sithSurface_numUnsyncedSurfaces_ADDR)
#define pMaterialsLoader (*(rdMaterialLoader_t*)pMaterialsLoader_ADDR)
#define pMaterialsUnloader (*(rdMaterialUnloader_t*)pMaterialsUnloader_ADDR)
#define sithSprite_pHashtable (*(tHashTable**)sithSprite_pHashtable_ADDR)
#define Window_drawAndFlip (*(WindowDrawHandler_t*)Window_drawAndFlip_ADDR)
#define Window_setCooperativeLevel (*(WindowDrawHandler_t*)Window_setCooperativeLevel_ADDR)
#define Window_ext_handlers ((wm_handler*)Window_ext_handlers_ADDR)
#define Window_aDialogHwnds ((HWND*)Window_aDialogHwnds_ADDR)
#define DebugGui_aIdk ((int*)DebugGui_aIdk_ADDR)
#define DebugGui_idk (*(int*)DebugGui_idk_ADDR)
#define DebugGui_some_line_amt (*(int*)DebugGui_some_line_amt_ADDR)
#define DebugGui_some_num_lines (*(int*)DebugGui_some_num_lines_ADDR)
#define DebugLog_buffer ((char*)DebugLog_buffer_ADDR)
#define sithConsole_aCmds (*(stdDebugConsoleCmd**)sithConsole_aCmds_ADDR)
#define sithConsole_pCmdHashtable (*(tHashTable**)sithConsole_pCmdHashtable_ADDR)
#define sithConsole_bOpened (*(int*)sithConsole_bOpened_ADDR)
#define sithConsole_bInitted (*(int*)sithConsole_bInitted_ADDR)
#define sithConsole_maxCmds (*(int*)sithConsole_maxCmds_ADDR)
#define sithConsole_numRegisteredCmds (*(int*)sithConsole_numRegisteredCmds_ADDR)
#define DebugGui_maxLines (*(uint32_t*)DebugGui_maxLines_ADDR)
#define DebugGui_fnPrint (*(DebugConsolePrintFunc_t*)DebugGui_fnPrint_ADDR)
#define DebugGui_fnPrintUniStr (*(DebugConsolePrintUniStrFunc_t*)DebugGui_fnPrintUniStr_ADDR)
#define sithConsole_alertSound (*(stdSound_buffer_t**)sithConsole_alertSound_ADDR)
#define sithConsole_idk2 (*(int16_t*)sithConsole_idk2_ADDR)
#define d3d_maxVertices (*(uint32_t*)d3d_maxVertices_ADDR)
#define d3d_device_ptr (*(d3d_device**)d3d_device_ptr_ADDR)
#define std3D_rectViewIdk (*(rdRect*)std3D_rectViewIdk_ADDR)
#define std3D_aViewIdk ((flex_t*)std3D_aViewIdk_ADDR)
#define std3D_aViewTris ((rdTri*)std3D_aViewTris_ADDR)
#define std3D_gpuMaxTexSizeMaybe (*(int32_t *)std3D_gpuMaxTexSizeMaybe_ADDR)
#define std3D_dword_53D66C (*(int32_t *)std3D_dword_53D66C_ADDR)
#define std3D_dword_53D670 (*(int32_t *)std3D_dword_53D670_ADDR)
#define std3D_dword_53D674 (*(int32_t *)std3D_dword_53D674_ADDR)
#define std3D_frameCount (*(int32_t *)std3D_frameCount_ADDR)
#define std3D_renderList (*(intptr_t*)std3D_renderList_ADDR)
#define std3D_numCachedTextures (*(int32_t*)std3D_numCachedTextures_ADDR)
#define std3D_pFirstTexCache (*(rdDDrawSurface**)std3D_pFirstTexCache_ADDR)
#define std3D_pLastTexCache (*(rdDDrawSurface* *)std3D_pLastTexCache_ADDR)
#define stdComm_dword_8321F8 (*(int*)stdComm_dword_8321F8_ADDR)
#define stdComm_bInitted (*(int*)stdComm_bInitted_ADDR)
#define stdComm_dword_8321F0 (*(int*)stdComm_dword_8321F0_ADDR)
#define stdComm_dword_8321F4 (*(int*)stdComm_dword_8321F4_ADDR)
#define stdComm_dplayIdSelf (*(int*)stdComm_dplayIdSelf_ADDR)
#define stdComm_dword_832204 (*(int*)stdComm_dword_832204_ADDR)
#define stdComm_dword_832208 (*(int*)stdComm_dword_832208_ADDR)
#define stdComm_currentBigSyncStage (*(int*)stdComm_currentBigSyncStage_ADDR)
#define stdComm_dword_8321E0 (*(int*)stdComm_dword_8321E0_ADDR)
#define stdComm_bIsServer (*(int*)stdComm_bIsServer_ADDR)
#define stdComm_dword_8321E8 (*(int*)stdComm_dword_8321E8_ADDR)
#define stdComm_waIdk ((char16_t*)stdComm_waIdk_ADDR)
#define stdComm_dword_8321DC (*(int*)stdComm_dword_8321DC_ADDR)
#define stdComm_dword_832200 (*(int*)stdComm_dword_832200_ADDR)
#define stdComm_dword_832210 (*(int*)stdComm_dword_832210_ADDR)
#define stdComm_cogMsgTmp (*(SithMessage*)stdComm_cogMsgTmp_ADDR)
#define stdMci_mciId (*(MCIDEVICEID*)stdMci_mciId_ADDR)
#define stdMci_dwVolume (*(int*)stdMci_dwVolume_ADDR)
#define stdMci_bInitted (*(int*)stdMci_bInitted_ADDR)
#define stdMci_uDeviceID (*(int*)stdMci_uDeviceID_ADDR)
#define wuRegistry_bStarted (*(int*)wuRegistry_bStarted_ADDR)
#define wuRegistry_lpClass ((uint8_t*)wuRegistry_lpClass_ADDR)
#define wuRegistry_byte_855EB4 ((uint8_t*)wuRegistry_byte_855EB4_ADDR)
#define wuRegistry_hKey (*(HKEY*)wuRegistry_hKey_ADDR)
#define wuRegistry_lpSubKey (*(LPCSTR*)wuRegistry_lpSubKey_ADDR)
#define WinIdk_aDplayGuid ((uint32_t*)WinIdk_aDplayGuid_ADDR)
#define stdDisplay_aDisplayDevices ((StdDisplayInfo*)stdDisplay_aDisplayDevices_ADDR)
#define stdDisplay_gammaTableLen (*(int*)stdDisplay_gammaTableLen_ADDR)
#define stdDisplay_paGammaTable (*(flex_d_t**)stdDisplay_paGammaTable_ADDR)
#define stdDisplay_gammaPalette ((rdColor24*)stdDisplay_gammaPalette_ADDR)
#define stdDisplay_pCurDevice (*(StdDisplayInfo**)stdDisplay_pCurDevice_ADDR)
#define stdDisplay_pCurVideoMode (*(StdVideoMode**)stdDisplay_pCurVideoMode_ADDR)
#define stdDisplay_bStartup (*(int*)stdDisplay_bStartup_ADDR)
#define stdDisplay_bOpen (*(int*)stdDisplay_bOpen_ADDR)
#define stdDisplay_bModeSet (*(int*)stdDisplay_bModeSet_ADDR)
#define stdDisplay_numVideoModes (*(int*)stdDisplay_numVideoModes_ADDR)
#define stdDisplay_bPaged (*(int*)stdDisplay_bPaged_ADDR)
#define stdDisplay_tmpGammaPal ((uint8_t*)stdDisplay_tmpGammaPal_ADDR)
#define stdDisplay_gammaIdx (*(int*)stdDisplay_gammaIdx_ADDR)
#define word_860800 (*(uint16_t*)word_860800_ADDR)
#define word_860802 (*(uint16_t*)word_860802_ADDR)
#define word_860804 (*(uint16_t*)word_860804_ADDR)
#define word_860806 (*(uint16_t*)word_860806_ADDR)
#define stdControl_bReadMouse (*(int *)stdControl_bReadMouse_ADDR)
#define stdControl_updateKHz (*(flex_t *)stdControl_updateKHz_ADDR)
#define stdControl_updateHz (*(flex_t *)stdControl_updateHz_ADDR)
#define stdControl_mouseXSensitivity (*(flex_t *)stdControl_mouseXSensitivity_ADDR)
#define stdControl_mouseYSensitivity (*(flex_t *)stdControl_mouseYSensitivity_ADDR)
#define stdControl_mouseZSensitivity (*(flex_t *)stdControl_mouseZSensitivity_ADDR)
#define stdControl_aAxisEnabled ((int*)stdControl_aAxisEnabled_ADDR)
#define stdControl_aAxisStates ((int*)stdControl_aAxisStates_ADDR)
#define stdControl_aKeyIdleTimes ((int*)stdControl_aKeyIdleTimes_ADDR)
#define stdControl_aKeyInfo ((int*)stdControl_aKeyInfo_ADDR)
#define stdControl_aAxisConnected ((int*)stdControl_aAxisConnected_ADDR)
#define stdControl_aKeyPressed ((int*)stdControl_aKeyPressed_ADDR)
#define stdControl_bStartup (*(int*)stdControl_bStartup_ADDR)
#define stdControl_bOpen (*(int*)stdControl_bOpen_ADDR)
#define stdControl_bDisableKeyboard (*(int*)stdControl_bDisableKeyboard_ADDR)
#define stdControl_bControlsIdle (*(int*)stdControl_bControlsIdle_ADDR)
#define stdControl_bControlsActive (*(int*)stdControl_bControlsActive_ADDR)
#define stdControl_pDI (*(void**)stdControl_pDI_ADDR)
#define stdControl_mouseDirectInputDevice (*(void**)stdControl_mouseDirectInputDevice_ADDR)
#define stdControl_keyboardIDirectInputDevice (*(void**)stdControl_keyboardIDirectInputDevice_ADDR)
#define stdControl_bReadJoysticks (*(int*)stdControl_bReadJoysticks_ADDR)
#define stdControl_curReadTime (*(uint32_t*)stdControl_curReadTime_ADDR)
#define stdControl_lastReadTime (*(uint32_t*)stdControl_lastReadTime_ADDR)
#define stdControl_readDeltaTime (*(uint32_t*)stdControl_readDeltaTime_ADDR)
#define stdControl_dwLastMouseX (*(int32_t*)stdControl_dwLastMouseX_ADDR)
#define stdControl_dwLastMouseY (*(int32_t*)stdControl_dwLastMouseY_ADDR)
#define stdControl_aJoystickEnabled ((int*)stdControl_aJoystickEnabled_ADDR)
#define stdControl_aJoystickExists ((int*)stdControl_aJoystickExists_ADDR)
#define stdControl_aAxes ((stdControlJoystickEntry*)stdControl_aAxes_ADDR)
#define stdControl_aJoystickMaxButtons ((int*)stdControl_aJoystickMaxButtons_ADDR)
#define Windows_installType (*(int*)Windows_installType_ADDR)
#define Video_aGammaTable ((uint8_t*)Video_aGammaTable_ADDR)
#define Video_fillColor (*(int *)Video_fillColor_ADDR)
#define Video_modeStruct (*(videoModeStruct*)Video_modeStruct_ADDR)
#define Video_otherBuf (*(tVBuffer*)Video_otherBuf_ADDR)
#define Video_dword_866D78 (*(int*)Video_dword_866D78_ADDR)
#define Video_curMode (*(int*)Video_curMode_ADDR)
#define Video_renderSurface ((StdVideoMode*)Video_renderSurface_ADDR)
#define Video_menuBuffer (*(tVBuffer*)Video_menuBuffer_ADDR)
#define Video_pOtherBuf (*(tVBuffer**)Video_pOtherBuf_ADDR)
#define Video_pMenuBuffer (*(tVBuffer**)Video_pMenuBuffer_ADDR)
#define Video_bInitted (*(int*)Video_bInitted_ADDR)
#define Video_bOpened (*(int*)Video_bOpened_ADDR)
#define Video_flt_55289C (*(flex_t*)Video_flt_55289C_ADDR)
#define Video_dword_5528A0 (*(int*)Video_dword_5528A0_ADDR)
#define Video_dword_5528A4 (*(int*)Video_dword_5528A4_ADDR)
#define Video_dword_5528A8 (*(int*)Video_dword_5528A8_ADDR)
#define Video_lastTimeMsec (*(int*)Video_lastTimeMsec_ADDR)
#define Video_dword_5528B0 (*(int*)Video_dword_5528B0_ADDR)
#define Video_pVbufIdk (*(tVBuffer**)Video_pVbufIdk_ADDR)
#define Video_pCanvas (*(rdCanvas**)Video_pCanvas_ADDR)
#define Video_aPalette ((rdColor24*)Video_aPalette_ADDR)
#define Video_format (*(tRasterInfo*)Video_format_ADDR)
#define Video_format2 (*(tRasterInfo*)Video_format2_ADDR)
#define Video_modeStruct2 (*(videoModeStruct*)Video_modeStruct2_ADDR)
#define Video_bufIdk (*(tVBuffer*)Video_bufIdk_ADDR)
#define stdConsole_textAttribute (*(uint16_t*)stdConsole_textAttribute_ADDR)
#define stdConsole_wAttributes (*(uint16_t*)stdConsole_wAttributes_ADDR)
#define stdConsole_cursorHidden (*(int*)stdConsole_cursorHidden_ADDR)
#define stdConsole_ConsoleCursorInfo (*(CONSOLE_CURSOR_INFO*)stdConsole_ConsoleCursorInfo_ADDR)
#define stdConsole_hOutput (*(HANDLE*)stdConsole_hOutput_ADDR)
#define stdConsole_hInput (*(HANDLE*)stdConsole_hInput_ADDR)
#define rdRaster_aOneOverNFixed ((int*)rdRaster_aOneOverNFixed_ADDR)
#define rdRaster_aLerpAndScale ((flex_t*)rdRaster_aLerpAndScale_ADDR)
#define rdRaster_aOneOverNFlex ((flex_t*)rdRaster_aOneOverNFlex_ADDR)
#define rdRaster_fixedScale (*(flex_t *)rdRaster_fixedScale_ADDR)
#define sithCogFunctionAI_aThingsInView ((SithThing**)sithCogFunctionAI_aThingsInView_ADDR)
#define sithCogFunctionAI_numThingsInView (*(int*)sithCogFunctionAI_numThingsInView_ADDR)
#define sithCogFunctionAI_curThingInView (*(int*)sithCogFunctionAI_curThingInView_ADDR)
#define sithComm_MsgTmpBuf ((SithMessage*)sithComm_MsgTmpBuf_ADDR)
#define sithComm_aMsgPairs ((sithCogMsg_Pair*)sithComm_aMsgPairs_ADDR)
#define sithMessage_aTypeFuncs ((cogMsg_Handler*)sithMessage_aTypeFuncs_ADDR)
#define sithMessage_bStopProcessMessages (*(int*)sithMessage_bStopProcessMessages_ADDR)
#define sithMessage_g_outputstream (*(int*)sithMessage_g_outputstream_ADDR)
#define sithMessage_g_inputstream (*(int*)sithMessage_g_inputstream_ADDR)
#define sithComm_idk2 (*(int*)sithComm_idk2_ADDR)
#define sithMessage_bSturtup (*(int*)sithMessage_bSturtup_ADDR)
#define sithComm_dword_847E84 (*(uint32_t*)sithComm_dword_847E84_ADDR)
#define sithComm_msgId (*(int *)sithComm_msgId_ADDR)
#define sithComm_MsgTmpBuf2 (*(SithMessage*)sithComm_MsgTmpBuf2_ADDR)
#define sithComm_netMsgTmp (*(SithMessage*)sithComm_netMsgTmp_ADDR)
#define jkCog_emptystring ((char16_t*)jkCog_emptystring_ADDR)
#define jkCog_jkstring ((char16_t*)jkCog_jkstring_ADDR)
#define jkCog_bInitted (*(int*)jkCog_bInitted_ADDR)
#define jkCog_strings (*(stdStrTable*)jkCog_strings_ADDR)
#define sithCog_bOpened (*(int*)sithCog_bOpened_ADDR)
#define sithCog_g_pHashtable (*(tHashTable**)sithCog_g_pHashtable_ADDR)
#define sithCog_aSectorLinks ((SithCogSectorLink*)sithCog_aSectorLinks_ADDR)
#define sithCog_numSectorLinks (*(int*)sithCog_numSectorLinks_ADDR)
#define sithCog_aThingLinks ((SithCogThingLink*)sithCog_aThingLinks_ADDR)
#define sithCog_numThingLinks (*(int*)sithCog_numThingLinks_ADDR)
#define sithCog_numSurfaceLinks (*(int*)sithCog_numSurfaceLinks_ADDR)
#define sithCog_aSurfaceLinks ((SithCogSurfaceLink*)sithCog_aSurfaceLinks_ADDR)
#define sithCog_g_pMasterCog (*(sithCog**)sithCog_g_pMasterCog_ADDR)
#define jkDev_aEntryPositions ((rdVector2i*)jkDev_aEntryPositions_ADDR)
#define jkDev_aEntries ((jkDevLogEnt*)jkDev_aEntries_ADDR)
#define jkDev_log_55A4A4 (*(int*)jkDev_log_55A4A4_ADDR)
#define jkDev_bScreenNeedsUpdate (*(int*)jkDev_bScreenNeedsUpdate_ADDR)
#define jkDev_aCheatCmds ((stdDebugConsoleCmd*)jkDev_aCheatCmds_ADDR)
#define jkDev_numCheats (*(uint32_t*)jkDev_numCheats_ADDR)
#define jkDev_bInitted (*(int*)jkDev_bInitted_ADDR)
#define jkDev_bOpened (*(int*)jkDev_bOpened_ADDR)
#define jkDev_cheatHashtable (*(tHashTable**)jkDev_cheatHashtable_ADDR)
#define jkDev_hDlg (*(HWND*)jkDev_hDlg_ADDR)
#define jkDev_vbuf (*(tVBuffer**)jkDev_vbuf_ADDR)
#define jkDev_BMFontHeight (*(int*)jkDev_BMFontHeight_ADDR)
#define jkDev_ColorKey (*(int*)jkDev_ColorKey_ADDR)
#define jkDev_dword_55A9D0 (*(int*)jkDev_dword_55A9D0_ADDR)
#define jkDev_amt (*(flex_t*)jkDev_amt_ADDR)
#define jkSmack_gameMode (*(int*)jkSmack_gameMode_ADDR)
#define jkSmack_bInit (*(int*)jkSmack_bInit_ADDR)
#define jkSmack_stopTick (*(int*)jkSmack_stopTick_ADDR)
#define jkSmack_currentGuiState (*(int*)jkSmack_currentGuiState_ADDR)
#define jkSmack_nextGuiState (*(int*)jkSmack_nextGuiState_ADDR)
#define jkSmack_alloc (*(void**)jkSmack_alloc_ADDR)
#define jkEpisode_aEpisodes ((jkEpisode*)jkEpisode_aEpisodes_ADDR)
#define jkEpisode_var4 ((char*)jkEpisode_var4_ADDR)
#define jkEpisode_var5 ((char*)jkEpisode_var5_ADDR)
#define jkEpisode_var2 (*(uint32_t*)jkEpisode_var2_ADDR)
#define jkEpisode_mLoad (*(jkEpisodeLoad*)jkEpisode_mLoad_ADDR)
#define jkHud_targetRed (*(uint32_t *)jkHud_targetRed_ADDR)
#define jkHud_targetBlue (*(uint32_t *)jkHud_targetBlue_ADDR)
#define jkHud_targetGreen (*(uint32_t *)jkHud_targetGreen_ADDR)
#define jkHud_aFltIdk ((flex_t*)jkHud_aFltIdk_ADDR)
#define jkHud_aColors8bpp ((int32_t*)jkHud_aColors8bpp_ADDR)
#define jkHud_aTeamColors8bpp ((int*)jkHud_aTeamColors8bpp_ADDR)
#define jkHud_chatStr ((char*)jkHud_chatStr_ADDR)
#define jkHud_aTeamColors16bpp ((int*)jkHud_aTeamColors16bpp_ADDR)
#define jkHud_rightBlitX (*(uint32_t*)jkHud_rightBlitX_ADDR)
#define jkHud_leftBlitX (*(uint32_t*)jkHud_leftBlitX_ADDR)
#define jkHud_mapRendConfig (*(SithOverlayMapConfig*)jkHud_mapRendConfig_ADDR)
#define jkHud_chatStrPos (*(int*)jkHud_chatStrPos_ADDR)
#define jkHud_rightBlitY (*(int*)jkHud_rightBlitY_ADDR)
#define jkHud_aTeamScores ((jkHudTeamScore*)jkHud_aTeamScores_ADDR)
#define jkHud_dword_552D10 (*(int*)jkHud_dword_552D10_ADDR)
#define jkHud_aColors16bpp ((int32_t*)jkHud_aColors16bpp_ADDR)
#define jkHud_aPlayerScores ((jkHudPlayerScore*)jkHud_aPlayerScores_ADDR)
#define jkHud_blittedAmmoAmt (*(int*)jkHud_blittedAmmoAmt_ADDR)
#define jkHud_idk14 (*(int*)jkHud_idk14_ADDR)
#define jkHud_blittedHealthIdx (*(int*)jkHud_blittedHealthIdx_ADDR)
#define jkHud_blittedBatteryAmt (*(int*)jkHud_blittedBatteryAmt_ADDR)
#define jkHud_blittedFieldlightAmt (*(int*)jkHud_blittedFieldlightAmt_ADDR)
#define jkHud_blittedShieldIdx (*(int*)jkHud_blittedShieldIdx_ADDR)
#define jkHud_isSuper (*(int*)jkHud_isSuper_ADDR)
#define jkHud_idk15 (*(int*)jkHud_idk15_ADDR)
#define jkHud_blittedForceIdx (*(int*)jkHud_blittedForceIdx_ADDR)
#define jkHud_idk16 (*(int*)jkHud_idk16_ADDR)
#define jkHud_leftBlitY (*(int*)jkHud_leftBlitY_ADDR)
#define jkHud_rectViewScores (*(rdRect*)jkHud_rectViewScores_ADDR)
#define jkHud_pMsgFontSft (*(stdFont**)jkHud_pMsgFontSft_ADDR)
#define jkHud_pStatusLeftBm (*(stdBitmap**)jkHud_pStatusLeftBm_ADDR)
#define jkHud_pStatusRightBm (*(stdBitmap**)jkHud_pStatusRightBm_ADDR)
#define jkHud_bHasTarget (*(int*)jkHud_bHasTarget_ADDR)
#define jkHud_pTargetThing (*(SithThing**)jkHud_pTargetThing_ADDR)
#define jkHud_targetRed16 (*(uint32_t*)jkHud_targetRed16_ADDR)
#define jkHud_targetGreen16 (*(uint32_t*)jkHud_targetGreen16_ADDR)
#define jkHud_targetBlue16 (*(uint32_t*)jkHud_targetBlue16_ADDR)
#define jkHud_bChatOpen (*(int*)jkHud_bChatOpen_ADDR)
#define jkHud_pHelthNumSft (*(stdFont**)jkHud_pHelthNumSft_ADDR)
#define jkHud_pAmoNumsSft (*(stdFont**)jkHud_pAmoNumsSft_ADDR)
#define jkHud_pAmoNumsSuperSft (*(stdFont**)jkHud_pAmoNumsSuperSft_ADDR)
#define jkHud_pArmorNumSft (*(stdFont**)jkHud_pArmorNumSft_ADDR)
#define jkHud_pArmorNumsSuperSft (*(stdFont**)jkHud_pArmorNumsSuperSft_ADDR)
#define jkHud_bInitted (*(int*)jkHud_bInitted_ADDR)
#define jkHud_bOpened (*(int*)jkHud_bOpened_ADDR)
#define jkHud_pFieldlightBm (*(stdBitmap**)jkHud_pFieldlightBm_ADDR)
#define jkHud_pStBatBm (*(stdBitmap**)jkHud_pStBatBm_ADDR)
#define jkHud_pStHealthBm (*(stdBitmap**)jkHud_pStHealthBm_ADDR)
#define jkHud_pStShieldBm (*(stdBitmap**)jkHud_pStShieldBm_ADDR)
#define jkHud_pStFrcBm (*(stdBitmap**)jkHud_pStFrcBm_ADDR)
#define jkHud_pStFrcSuperBm (*(stdBitmap**)jkHud_pStFrcSuperBm_ADDR)
#define jkHud_bViewScores (*(int*)jkHud_bViewScores_ADDR)
#define jkHud_dword_553ED0 (*(int*)jkHud_dword_553ED0_ADDR)
#define jkHud_tallyWhich (*(int*)jkHud_tallyWhich_ADDR)
#define jkHud_numPlayers (*(uint32_t*)jkHud_numPlayers_ADDR)
#define jkHud_dword_553EDC (*(int*)jkHud_dword_553EDC_ADDR)
#define jkHud_dword_553EE0 (*(int*)jkHud_dword_553EE0_ADDR)
#define jkHudInv_info (*(jkHudInvInfo*)jkHudInv_info_ADDR)
#define jkHudInv_itemTexfmt (*(rdTexFormat*)jkHudInv_itemTexfmt_ADDR)
#define jkHudInv_flags (*(int*)jkHudInv_flags_ADDR)
#define jkHudInv_dword_553F64 (*(int*)jkHudInv_dword_553F64_ADDR)
#define jkHudInv_scroll (*(jkHudInvScroll*)jkHudInv_scroll_ADDR)
#define jkHudInv_aBitmaps ((stdBitmap**)jkHudInv_aBitmaps_ADDR)
#define jkHudInv_font (*(stdFont**)jkHudInv_font_ADDR)
#define jkHudInv_rend_isshowing_maybe (*(int*)jkHudInv_rend_isshowing_maybe_ADDR)
#define jkHudInv_dword_553F94 (*(int*)jkHudInv_dword_553F94_ADDR)
#define jkHudInv_numItems (*(int*)jkHudInv_numItems_ADDR)
#define jkHudInv_aItems (*(int**)jkHudInv_aItems_ADDR)
#define Main_bDevMode (*(int*)Main_bDevMode_ADDR)
#define Main_bDisplayConfig (*(int*)Main_bDisplayConfig_ADDR)
#define Main_bWindowGUI (*(int*)Main_bWindowGUI_ADDR)
#define Main_dword_86078C (*(int*)Main_dword_86078C_ADDR)
#define Main_bFrameRate (*(int*)Main_bFrameRate_ADDR)
#define Main_bDispStats (*(int*)Main_bDispStats_ADDR)
#define Main_bNoHUD (*(int*)Main_bNoHUD_ADDR)
#define Main_logLevel (*(int*)Main_logLevel_ADDR)
#define Main_verboseLevel (*(int*)Main_verboseLevel_ADDR)
#define Main_path ((char*)Main_path_ADDR)
#define debug_log_fp (*(stdFile_t *)debug_log_fp_ADDR)
#define pHS (*(HostServices**)pHS_ADDR)
#define jkCredits_aPalette ((char*)jkCredits_aPalette_ADDR)
#define jkCredits_pVbuffer2 (*(tVBuffer**)jkCredits_pVbuffer2_ADDR)
#define jkCredits_dword_55AD64 (*(int*)jkCredits_dword_55AD64_ADDR)
#define jkCredits_dword_55AD68 (*(int*)jkCredits_dword_55AD68_ADDR)
#define jkCredits_table (*(stdStrTable*)jkCredits_table_ADDR)
#define jkCredits_startMs (*(int*)jkCredits_startMs_ADDR)
#define jkCredits_dword_55AD84 (*(int*)jkCredits_dword_55AD84_ADDR)
#define jkCredits_strIdx (*(int*)jkCredits_strIdx_ADDR)
#define jkCredits_aIdk (*(char**)jkCredits_aIdk_ADDR)
#define jkCredits_pVbuffer (*(tVBuffer**)jkCredits_pVbuffer_ADDR)
#define jkCredits_dword_55AD94 (*(int*)jkCredits_dword_55AD94_ADDR)
#define jkCredits_fontLarge (*(stdFont**)jkCredits_fontLarge_ADDR)
#define jkCredits_fontSmall (*(stdFont**)jkCredits_fontSmall_ADDR)
#define jkCredits_dword_55ADA0 (*(int*)jkCredits_dword_55ADA0_ADDR)
#define jkCredits_bInitted (*(int*)jkCredits_bInitted_ADDR)
#define jkCredits_dword_55ADA8 (*(int*)jkCredits_dword_55ADA8_ADDR)
#define g_sithMode (*(int*)g_sithMode_ADDR)
#define g_submodeFlags (*(int*)g_submodeFlags_ADDR)
#define g_debugmodeFlags (*(int*)g_debugmodeFlags_ADDR)
#define g_mapModeFlags (*(int*)g_mapModeFlags_ADDR)
#define jkGame_gamma (*(int*)jkGame_gamma_ADDR)
#define jkGame_screenSize (*(int*)jkGame_screenSize_ADDR)
#define jkGame_bInitted (*(int*)jkGame_bInitted_ADDR)
#define jkGame_updateMsecsTotal (*(int*)jkGame_updateMsecsTotal_ADDR)
#define jkGame_dword_552B5C (*(int*)jkGame_dword_552B5C_ADDR)
#define jkGame_isDDraw (*(int*)jkGame_isDDraw_ADDR)
#define jkRes_pHS (*(HostServices**)jkRes_pHS_ADDR)
#define jkRes_episodeGobName ((char*)jkRes_episodeGobName_ADDR)
#define jkRes_curDir ((char*)jkRes_curDir_ADDR)
#define jkRes_bHookedHS (*(int*)jkRes_bHookedHS_ADDR)
#define jkRes_aFiles ((jkResFile*)jkRes_aFiles_ADDR)
#define jkRes_gCtx (*(jkRes*)jkRes_gCtx_ADDR)
#define pLowLevelHS (*(HostServices**)pLowLevelHS_ADDR)
#define lowLevelHS (*(HostServices*)lowLevelHS_ADDR)
#define jkRes_idkGobPath ((char*)jkRes_idkGobPath_ADDR)
#define jkCutscene_rect1 (*(rdRect*)jkCutscene_rect1_ADDR)
#define jkCutscene_rect2 (*(rdRect*)jkCutscene_rect2_ADDR)
#define jkCutscene_strings (*(stdStrTable*)jkCutscene_strings_ADDR)
#define jkCutscene_subtitlefont (*(stdFont**)jkCutscene_subtitlefont_ADDR)
#define jkCutscene_bInitted (*(int*)jkCutscene_bInitted_ADDR)
#define jkCutscene_isRendering (*(int*)jkCutscene_isRendering_ADDR)
#define jkCutscene_dword_55B750 (*(int*)jkCutscene_dword_55B750_ADDR)
#define jkCutscene_dword_55AA50 (*(int*)jkCutscene_dword_55AA50_ADDR)
#define jkCutscene_55AA54 (*(int*)jkCutscene_55AA54_ADDR)
#define jkMain_aLevelJklFname ((char*)jkMain_aLevelJklFname_ADDR)
#define thing_nine (*(int *)thing_nine_ADDR)
#define jkMain_bInit (*(int*)jkMain_bInit_ADDR)
#define thing_six (*(int*)thing_six_ADDR)
#define thing_eight (*(int*)thing_eight_ADDR)
#define jkMain_dword_552B98 (*(int*)jkMain_dword_552B98_ADDR)
#define jkMain_lastTickMs (*(int*)jkMain_lastTickMs_ADDR)
#define idx_13b4_related (*(int*)idx_13b4_related_ADDR)
#define gamemode_1_str ((char*)gamemode_1_str_ADDR)
#define jkMain_strIdk ((char*)jkMain_strIdk_ADDR)
#define jkMain_wstrIdk ((char16_t*)jkMain_wstrIdk_ADDR)
#define sithWorld_aSectionParsers ((SithWorldTextSectionParseHandler*)sithWorld_aSectionParsers_ADDR)
#define sithWorld_some_integer_4 (*(uint32_t*)sithWorld_some_integer_4_ADDR)
#define sithWorld_g_pCurrentWorld (*(SithWorld**)sithWorld_g_pCurrentWorld_ADDR)
#define sithWorld_g_pStaticWorld (*(SithWorld**)sithWorld_g_pStaticWorld_ADDR)
#define sithWorld_g_pLastLoadedWorld (*(SithWorld**)sithWorld_g_pLastLoadedWorld_ADDR)
#define sithWorld_numParsers (*(uint32_t*)sithWorld_numParsers_ADDR)
#define sithWorld_bInitted (*(uint32_t*)sithWorld_bInitted_ADDR)
#define sithWorld_bLoaded (*(int*)sithWorld_bLoaded_ADDR)
#define sithWorld_episodeName ((char*)sithWorld_episodeName_ADDR)
#define sithInventory_powerKeybinds ((SithControlBinding*)sithInventory_powerKeybinds_ADDR)
#define sithInventory_g_bInitInventory (*(int *)sithInventory_g_bInitInventory_ADDR)
#define sithInventory_g_aTypes ((SithInventoryType*)sithInventory_g_aTypes_ADDR)
#define sithInventory_g_bSendDeactivateMessage (*(int*)sithInventory_g_bSendDeactivateMessage_ADDR)
#define sithInventory_bUnkPower (*(int*)sithInventory_bUnkPower_ADDR)
#define sithInventory_8339EC (*(int*)sithInventory_8339EC_ADDR)
#define sithInventory_bRendIsHidden (*(int*)sithInventory_bRendIsHidden_ADDR)
#define sithInventory_8339F4 (*(int*)sithInventory_8339F4_ADDR)
#define sithWeapon_controlOptions (*(uint32_t*)sithWeapon_controlOptions_ADDR)
#define g_flt_8BD040 (*(flex_t*)g_flt_8BD040_ADDR)
#define g_flt_8BD044 (*(flex_t*)g_flt_8BD044_ADDR)
#define g_flt_8BD048 (*(flex_t*)g_flt_8BD048_ADDR)
#define g_flt_8BD04C (*(flex_t*)g_flt_8BD04C_ADDR)
#define g_flt_8BD050 (*(flex_t*)g_flt_8BD050_ADDR)
#define g_flt_8BD054 (*(flex_t*)g_flt_8BD054_ADDR)
#define g_flt_8BD058 (*(flex_t*)g_flt_8BD058_ADDR)
#define sithWeapon_CurWeaponMode (*(int*)sithWeapon_CurWeaponMode_ADDR)
#define sithWeapon_bAutoPickup (*(int*)sithWeapon_bAutoPickup_ADDR)
#define sithWeapon_bAutoSwitch (*(int*)sithWeapon_bAutoSwitch_ADDR)
#define sithWeapon_bAutoReload (*(int*)sithWeapon_bAutoReload_ADDR)
#define sithWeapon_bMultiAutoPickup (*(int*)sithWeapon_bMultiAutoPickup_ADDR)
#define sithWeapon_bMultiplayerAutoSwitch (*(int*)sithWeapon_bMultiplayerAutoSwitch_ADDR)
#define sithWeapon_bMultiAutoReload (*(int*)sithWeapon_bMultiAutoReload_ADDR)
#define sithWeapon_bAutoAim (*(int*)sithWeapon_bAutoAim_ADDR)
#define sithWeapon_secMountWait (*(flex32_t*)sithWeapon_secMountWait_ADDR)
#define sithWeapon_8BD0A0 ((flex_t*)sithWeapon_8BD0A0_ADDR)
#define sithWeapon_fireWait (*(flex32_t*)sithWeapon_fireWait_ADDR)
#define sithWeapon_fireRate (*(flex32_t*)sithWeapon_fireRate_ADDR)
#define sithWeapon_LastFireTimeSecs (*(flex32_t*)sithWeapon_LastFireTimeSecs_ADDR)
#define sithWeapon_a8BD030 ((int*)sithWeapon_a8BD030_ADDR)
#define sithWeapon_8BD05C (*(int*)sithWeapon_8BD05C_ADDR)
#define sithWeapon_8BD060 (*(flex32_t*)sithWeapon_8BD060_ADDR)
#define sithWeapon_8BD008 ((int*)sithWeapon_8BD008_ADDR)
#define sithWeapon_8BD024 (*(int*)sithWeapon_8BD024_ADDR)
#define sithWeapon_senderIndex (*(int*)sithWeapon_senderIndex_ADDR)
#define jkPlayer_playerInfos ((SithPlayer*)jkPlayer_playerInfos_ADDR)
#define jkPlayer_playerShortName ((char16_t*)jkPlayer_playerShortName_ADDR)
#define jkPlayer_numOtherThings (*(int*)jkPlayer_numOtherThings_ADDR)
#define jkPlayer_numThings (*(int*)jkPlayer_numThings_ADDR)
#define jkPlayer_otherThings ((jkPlayerInfo*)jkPlayer_otherThings_ADDR)
#define jkPlayer_bLoadingSomething (*(int *)jkPlayer_bLoadingSomething_ADDR)
#define playerThingIdx (*(int*)playerThingIdx_ADDR)
#define jkPlayer_maxPlayers (*(uint32_t*)jkPlayer_maxPlayers_ADDR)
#define sithPlayer_g_pLocalPlayerThing (*(SithThing**)sithPlayer_g_pLocalPlayerThing_ADDR)
#define sithPlayer_g_pLocalPlayer (*(SithPlayer**)sithPlayer_g_pLocalPlayer_ADDR)
#define playerThings ((jkPlayerInfo*)playerThings_ADDR)
#define jkSaber_rotateMat (*(rdMatrix34*)jkSaber_rotateMat_ADDR)
#define jkPlayer_setRotateOverlayMap (*(int*)jkPlayer_setRotateOverlayMap_ADDR)
#define jkPlayer_setDrawStatus (*(int*)jkPlayer_setDrawStatus_ADDR)
#define jkPlayer_setCrosshair (*(int*)jkPlayer_setCrosshair_ADDR)
#define jkPlayer_setSaberCam (*(int*)jkPlayer_setSaberCam_ADDR)
#define jkPlayer_setFullSubtitles (*(int*)jkPlayer_setFullSubtitles_ADDR)
#define jkPlayer_setDisableCutscenes (*(int*)jkPlayer_setDisableCutscenes_ADDR)
#define jkPlayer_aCutsceneVal ((int*)jkPlayer_aCutsceneVal_ADDR)
#define jkPlayer_cutscenePath ((char*)jkPlayer_cutscenePath_ADDR)
#define jkPlayer_setNumCutscenes (*(int*)jkPlayer_setNumCutscenes_ADDR)
#define jkPlayer_currentTickIdx (*(int*)jkPlayer_currentTickIdx_ADDR)
#define jkPlayer_setDiff (*(int*)jkPlayer_setDiff_ADDR)
#define jkPlayer_waggleVec (*(rdVector3*)jkPlayer_waggleVec_ADDR)
#define jkPlayer_waggleMag (*(flex_t*)jkPlayer_waggleMag_ADDR)
#define jkPlayer_mpcInfoSet (*(int*)jkPlayer_mpcInfoSet_ADDR)
#define jkPlayer_waggleAngle (*(flex_t*)jkPlayer_waggleAngle_ADDR)
#define jkSaber_rotateVec (*(rdVector3*)jkSaber_rotateVec_ADDR)
#define jkPlayer_name ((char16_t*)jkPlayer_name_ADDR)
#define jkPlayer_model ((char*)jkPlayer_model_ADDR)
#define jkPlayer_soundClass ((char*)jkPlayer_soundClass_ADDR)
#define jkPlayer_sideMat ((char*)jkPlayer_sideMat_ADDR)
#define jkPlayer_tipMat ((char*)jkPlayer_tipMat_ADDR)
#define sithCollision_aNumSearchedSectors ((int*)sithCollision_aNumSearchedSectors_ADDR)
#define sithCollision_collisionHandlers ((SithCollideResult*)sithCollision_collisionHandlers_ADDR)
#define sithCollision_aThingSurfaceCollideResults ((sithCollisionHitHandler_t*)sithCollision_aThingSurfaceCollideResults_ADDR)
#define sithCollision_aCollisions ((sithCollisionSearchResult*)sithCollision_aCollisions_ADDR)
#define sithCollision_aNumStackCollisions ((int*)sithCollision_aNumStackCollisions_ADDR)
#define sithCollision_searchStackIdx (*(int *)sithCollision_searchStackIdx_ADDR)
#define sithCollision_apSearchedSectors ((sithCollisionSectorEntry*)sithCollision_apSearchedSectors_ADDR)
#define sithCollision_dword_8B4BE4 (*(int*)sithCollision_dword_8B4BE4_ADDR)
#define sithCollision_collideHurtIdk (*(rdVector3*)sithCollision_collideHurtIdk_ADDR)
#define sithSector_surfaceNormal (*(rdVector3 *)sithSector_surfaceNormal_ADDR)
#define sithAIAwareness_aEntries ((sithSectorEntry*)sithAIAwareness_aEntries_ADDR)
#define sithAIAwareness_g_aSectors (*(sithSectorAlloc**)sithAIAwareness_g_aSectors_ADDR)
#define sithAIAwareness_numEntries (*(int*)sithAIAwareness_numEntries_ADDR)
#define sithAIAwareness_bInitted (*(int*)sithAIAwareness_bInitted_ADDR)
#define sithAIAwareness_timerTicks (*(int*)sithAIAwareness_timerTicks_ADDR)
#define sithSector_flt_8553B8 (*(flex_t*)sithSector_flt_8553B8_ADDR)
#define sithSector_horizontalPixelsPerRev (*(flex_t*)sithSector_horizontalPixelsPerRev_ADDR)
#define sithSector_flt_8553C0 (*(flex_t*)sithSector_flt_8553C0_ADDR)
#define sithSector_flt_8553C4 (*(flex_t*)sithSector_flt_8553C4_ADDR)
#define sithSector_flt_8553C8 (*(flex_t*)sithSector_flt_8553C8_ADDR)
#define sithSector_zMaxVec (*(rdVector3*)sithSector_zMaxVec_ADDR)
#define sithSector_ceilingSky (*(flex_t*)sithSector_ceilingSky_ADDR)
#define sithSector_zMinVec (*(rdVector3*)sithSector_zMinVec_ADDR)
#define sithSector_horizontalPixelsPerRev_idk (*(flex_t*)sithSector_horizontalPixelsPerRev_idk_ADDR)
#define sithSector_horizontalDist (*(flex_t*)sithSector_horizontalDist_ADDR)
#define sithSector_flt_8553F4 (*(flex_t*)sithSector_flt_8553F4_ADDR)
#define sithSector_aModifiedSectors ((SithSector**)sithSector_aModifiedSectors_ADDR)
#define sithSector_aSyncFlags ((int*)sithSector_aSyncFlags_ADDR)
#define sithSector_numModifiedSectors (*(uint32_t*)sithSector_numModifiedSectors_ADDR)
#define sithThing_pParseHashtbl (*(tHashTable**)sithThing_pParseHashtbl_ADDR)
#define sithThing_pfUnknownFunc (*(sithThing_handler_t*)sithThing_pfUnknownFunc_ADDR)
#define sithThing_guidEntropy (*(uint16_t *)sithThing_guidEntropy_ADDR)
#define jkGuiSaveLoad_wtextEpisode ((char16_t*)jkGuiSaveLoad_wtextEpisode_ADDR)
#define jkGuiSaveLoad_wtextHealth ((char16_t*)jkGuiSaveLoad_wtextHealth_ADDR)
#define jkGuiSaveLoad_numEntries (*(int*)jkGuiSaveLoad_numEntries_ADDR)
#define jkGuiSaveLoad_wtextShields ((char16_t*)jkGuiSaveLoad_wtextShields_ADDR)
#define jkGuiSaveLoad_word_559830 ((char16_t*)jkGuiSaveLoad_word_559830_ADDR)
#define jkGuiSaveLoad_bIsSaveMenu (*(int*)jkGuiSaveLoad_bIsSaveMenu_ADDR)
#define jkGuiSaveLoad_wtextSaveName ((char16_t*)jkGuiSaveLoad_wtextSaveName_ADDR)
#define jkGuiSaveLoad_DarrayEntries (*(Darray*)jkGuiSaveLoad_DarrayEntries_ADDR)
#define jkGuiSaveLoad_word_559C54 ((char16_t*)jkGuiSaveLoad_word_559C54_ADDR)
#define jkGuiSaveLoad_byte_559C50 ((char*)jkGuiSaveLoad_byte_559C50_ADDR)
#define jkGuiTitle_verMajor (*(int *)jkGuiTitle_verMajor_ADDR)
#define jkGuiTitle_verMinor (*(int *)jkGuiTitle_verMinor_ADDR)
#define jkGuiTitle_verRevision (*(int *)jkGuiTitle_verRevision_ADDR)
#define jkGuiTitle_aTexts ((jkGuiStringEntry*)jkGuiTitle_aTexts_ADDR)
#define jkGuiTitle_whichLoading (*(int*)jkGuiTitle_whichLoading_ADDR)
#define jkGuiSound_sfxVolume (*(flex_t *)jkGuiSound_sfxVolume_ADDR)
#define jkGuiSound_numChannels (*(uint32_t *)jkGuiSound_numChannels_ADDR)
#define jkGuiSound_bLowResSound (*(int*)jkGuiSound_bLowResSound_ADDR)
#define jkGuiSound_b3DSound (*(int *)jkGuiSound_b3DSound_ADDR)
#define jkGuiSound_b3DSound_2 (*(int *)jkGuiSound_b3DSound_2_ADDR)
#define jkGuiSound_b3DSound_3 (*(int *)jkGuiSound_b3DSound_3_ADDR)
#define jkGuiSound_musicVolume (*(flex_t*)jkGuiSound_musicVolume_ADDR)
#define jkGui_unkstr ((char*)jkGui_unkstr_ADDR)
#define jkGui_GdiMode (*(int*)jkGui_GdiMode_ADDR)
#define jkGui_modesets (*(int*)jkGui_modesets_ADDR)
#define jkGui_stdBitmaps ((stdBitmap**)jkGui_stdBitmaps_ADDR)
#define jkGui_stdFonts ((stdFont**)jkGui_stdFonts_ADDR)
#define jkGui_episodeLoad (*(jkEpisodeLoad*)jkGui_episodeLoad_ADDR)
#define jkGuiSingleTally_foStars (*(stdBitmap**)jkGuiSingleTally_foStars_ADDR)
#define jkGuiNetHost_maxRank (*(int*)jkGuiNetHost_maxRank_ADDR)
#define jkGuiNetHost_timeLimit (*(int*)jkGuiNetHost_timeLimit_ADDR)
#define jkGuiNetHost_scoreLimit (*(int*)jkGuiNetHost_scoreLimit_ADDR)
#define jkGuiNetHost_maxPlayers (*(int*)jkGuiNetHost_maxPlayers_ADDR)
#define jkGuiNetHost_sessionFlags (*(int*)jkGuiNetHost_sessionFlags_ADDR)
#define jkGuiNetHost_gameFlags (*(int*)jkGuiNetHost_gameFlags_ADDR)
#define jkGuiNetHost_tickRate (*(int*)jkGuiNetHost_tickRate_ADDR)
#define jkGuiNetHost_gameName ((char16_t*)jkGuiNetHost_gameName_ADDR)
#define jkGuiMultiplayer_checksumSeed (*(int*)jkGuiMultiplayer_checksumSeed_ADDR)
#define jkGuiMultiplayer_dword_5564EC (*(int*)jkGuiMultiplayer_dword_5564EC_ADDR)
#define jkGuiMultiplayer_dword_5564E8 (*(int*)jkGuiMultiplayer_dword_5564E8_ADDR)
#define jkGuiMultiplayer_multiEntry (*(jkMultiEntry*)jkGuiMultiplayer_multiEntry_ADDR)
#define jkGuiMultiplayer_dword_5564F0 (*(int*)jkGuiMultiplayer_dword_5564F0_ADDR)
#define g_hInstance (*(HINSTANCE*)g_hInstance_ADDR)
#define sithCog_g_pSymbolTable (*(SithCogSymbolTable**)sithCog_g_pSymbolTable_ADDR)
#define pSithHS (*(struct HostServices**)pSithHS_ADDR)
#define g_hWnd (*(HWND*)g_hWnd_ADDR)
#define g_nShowCmd (*(uint32_t*)g_nShowCmd_ADDR)
#define g_app_suspended (*(uint32_t*)g_app_suspended_ADDR)
#define g_window_active (*(uint32_t*)g_window_active_ADDR)
#define g_app_active (*(uint32_t*)g_app_active_ADDR)
#define g_should_exit (*(uint32_t*)g_should_exit_ADDR)
#define g_thing_two_some_dialog_count (*(uint32_t*)g_thing_two_some_dialog_count_ADDR)
#define g_handler_count (*(uint32_t*)g_handler_count_ADDR)
#define g_855E8C (*(uint32_t*)g_855E8C_ADDR)
#define g_855E90 (*(uint32_t*)g_855E90_ADDR)
#define g_window_not_destroyed (*(uint32_t*)g_window_not_destroyed_ADDR)
#define stdPalEffects_state (*(stdPalEffectsState*)stdPalEffects_state_ADDR)
#define stdPalEffects_palette ((rdColor24*)stdPalEffects_palette_ADDR)
#define stdPalEffects_numEffectRequests (*(uint32_t*)stdPalEffects_numEffectRequests_ADDR)
#define stdPalEffects_aEffects ((stdPalEffectRequest*)stdPalEffects_aEffects_ADDR)
#define stdPalEffects_setPalette (*(stdPalEffectSetPaletteFunc_t*)stdPalEffects_setPalette_ADDR)
#define stdPalEffects_aPalette ((uint16_t*)stdPalEffects_aPalette_ADDR)
#define aFilenameStack ((char*)aFilenameStack_ADDR)
#define apBufferStack ((char**)apBufferStack_ADDR)
#define linenumStack ((int*)linenumStack_ADDR)
#define aEntryStack ((char*)aEntryStack_ADDR)
#define openFileStack ((stdFile_t*)openFileStack_ADDR)
#define printfBuffer ((char*)printfBuffer_ADDR)
#define stdConffile_linenum (*(int*)stdConffile_linenum_ADDR)
#define stdConffile_bOpen (*(int*)stdConffile_bOpen_ADDR)
#define openFile (*(stdFile_t*)openFile_ADDR)
#define writeFile (*(stdFile_t*)writeFile_ADDR)
#define stackLevel (*(uint32_t*)stackLevel_ADDR)
#define stdConffile_aWriteFilename ((char*)stdConffile_aWriteFilename_ADDR)
#define stdConffile_g_entry (*(StdConffileEntry*)stdConffile_g_entry_ADDR)
#define stdConffile_pFilename ((char*)stdConffile_pFilename_ADDR)
#define stdConffile_g_aLine (*(char**)stdConffile_g_aLine_ADDR)
#define stdMemory_bInitted (*(int*)stdMemory_bInitted_ADDR)
#define stdMemory_bOpened (*(int*)stdMemory_bOpened_ADDR)
#define stdMemory_g_curState (*(tMemoryState*)stdMemory_g_curState_ADDR)
#define yyin (*(stdFile_t*)yyin_ADDR)
#define yyout (*(stdFile_t*)yyout_ADDR)
#define sithCogParse_pSymbolTable (*(SithCogSymbolTable**)sithCogParse_pSymbolTable_ADDR)
#define yacc_linenum (*(int*)yacc_linenum_ADDR)
#define cog_yacc_loop_depth (*(int *)cog_yacc_loop_depth_ADDR)
#define cog_parser_node_stackpos ((int*)cog_parser_node_stackpos_ADDR)
#define cogvm_stackpos (*(int*)cogvm_stackpos_ADDR)
#define cogparser_nodes_alloc (*(sith_cog_parser_node**)cogparser_nodes_alloc_ADDR)
#define cogparser_topnode (*(sith_cog_parser_node**)cogparser_topnode_ADDR)
#define cogvm_stack (*(int32_t**)cogvm_stack_ADDR)
#define cogparser_num_nodes (*(int*)cogparser_num_nodes_ADDR)
#define cogparser_current_nodeidx (*(int*)cogparser_current_nodeidx_ADDR)
#define parsing_script (*(SithCogScript**)parsing_script_ADDR)
#define parsing_script_idk (*(int *)parsing_script_idk_ADDR)
#define dplay_dword_55D618 (*(int*)dplay_dword_55D618_ADDR)
#define dplay_dword_55D61C (*(int*)dplay_dword_55D61C_ADDR)
#define jkGui_guid_556040 (*(GUID_idk*)jkGui_guid_556040_ADDR)
#define jkGuiMultiplayer_numConnections (*(int*)jkGuiMultiplayer_numConnections_ADDR)
#define jkGuiMultiplayer_aConnections ((sith_dplay_connection*)jkGuiMultiplayer_aConnections_ADDR)
#define jkGuiMultiplayer_aEntries ((jkMultiEntry*)jkGuiMultiplayer_aEntries_ADDR)
#define jkGuiMultiplayer_stru_556168 (*(jkMultiEntry2*)jkGuiMultiplayer_stru_556168_ADDR)
#define jkGuiMultiplayer_mpcInfo (*(jkPlayerMpcInfo*)jkGuiMultiplayer_mpcInfo_ADDR)
#define jkGuiMultiplayer_stru_5564A8 (*(Darray*)jkGuiMultiplayer_stru_5564A8_ADDR)
#define jkGuiMouse_bOpen (*(int*)jkGuiMouse_bOpen_ADDR)
#define jkGuiMouse_Darray_556698 (*(Darray*)jkGuiMouse_Darray_556698_ADDR)
#define jkGuiMouse_dword_5566B0 (*(int*)jkGuiMouse_dword_5566B0_ADDR)
#define jkGuiMouse_Darray_5566B8 (*(Darray*)jkGuiMouse_Darray_5566B8_ADDR)
#define jkGuiMouse_Darray_5566D0 (*(Darray*)jkGuiMouse_Darray_5566D0_ADDR)
#define jkGuiMouse_pWStr_5566E8 (*(char16_t**)jkGuiMouse_pWStr_5566E8_ADDR)
#define jkGuiEsc_bInitialized (*(int*)jkGuiEsc_bInitialized_ADDR)
#define jkGuiKeyboard_dword_555DE0 (*(int*)jkGuiKeyboard_dword_555DE0_ADDR)
#define jkGuiKeyboard_bOnceIdk (*(int*)jkGuiKeyboard_bOnceIdk_ADDR)
#define jkGuiKeyboard_funcIdx (*(int*)jkGuiKeyboard_funcIdx_ADDR)
#define jkGuiKeyboard_flags (*(int*)jkGuiKeyboard_flags_ADDR)
#define jkGuiKeyboard_darrEntries (*(Darray*)jkGuiKeyboard_darrEntries_ADDR)
#define jkGuiKeyboard_dword_555E10 (*(int*)jkGuiKeyboard_dword_555E10_ADDR)
#define jkGuiKeyboard_wstr_555E18 ((char16_t*)jkGuiKeyboard_wstr_555E18_ADDR)
#define jkGuiKeyboard_pWStr_55601C (*(char16_t**)jkGuiKeyboard_pWStr_55601C_ADDR)
#define jkGuiMap_vec3Idk2 (*(rdVector3*)jkGuiMap_vec3Idk2_ADDR)
#define jkGuiMap_pCanvas (*(rdCanvas**)jkGuiMap_pCanvas_ADDR)
#define jkGuiMap_viewMat (*(rdMatrix34*)jkGuiMap_viewMat_ADDR)
#define jkGuiMap_matTmp (*(rdMatrix34*)jkGuiMap_matTmp_ADDR)
#define jkGuiMap_pVbuffer (*(tVBuffer**)jkGuiMap_pVbuffer_ADDR)
#define jkGuiMap_unk4 (*(sithMap*)jkGuiMap_unk4_ADDR)
#define jkGuiMap_vec3Idk (*(rdVector3*)jkGuiMap_vec3Idk_ADDR)
#define jkGuiMap_pCamera (*(rdCamera**)jkGuiMap_pCamera_ADDR)
#define jkGuiMap_dword_556660 (*(int*)jkGuiMap_dword_556660_ADDR)
#define jkGuiMap_bOrbitActive (*(int*)jkGuiMap_bOrbitActive_ADDR)
#define jkGuiMap_dword_556668 (*(int*)jkGuiMap_dword_556668_ADDR)
#define jkGuiMap_dword_55666C (*(int*)jkGuiMap_dword_55666C_ADDR)
#define sithMap_unkArr ((flex_t*)sithMap_unkArr_ADDR)
#define sithMap_pPlayerThing (*(SithThing**)sithMap_pPlayerThing_ADDR)
#define sithMap_invMatrix (*(rdMatrix34*)sithMap_invMatrix_ADDR)
#define sithMap_flt_84DEA8 (*(flex_t*)sithMap_flt_84DEA8_ADDR)
#define sithMap_flt_84DEAC (*(flex_t*)sithMap_flt_84DEAC_ADDR)
#define sithMap_ctx (*(sithMap*)sithMap_ctx_ADDR)
#define sithMap_pCurWorld (*(SithWorld**)sithMap_pCurWorld_ADDR)
#define sithMap_camera (*(rdMatrix34*)sithMap_camera_ADDR)
#define sithMap_pCurCamera (*(rdCamera**)sithMap_pCurCamera_ADDR)
#define sithMap_bInitted (*(int*)sithMap_bInitted_ADDR)
#define sithMap_var (*(int*)sithMap_var_ADDR)
#define DirectPlay_numPlayers (*(uint32_t*)DirectPlay_numPlayers_ADDR)
#define DirectPlay_aPlayers ((sithDplayPlayer*)DirectPlay_aPlayers_ADDR)

#endif

extern void OpenJKDF2_Globals_Reset();

// Embed
// /Users/bob/esp-cpp/tab5-emu/components/jk/OpenJKDF2/resource/ui/openjkdf2_i8n.uni+/Users/bob/esp-cpp/tab5-emu/components/jk/OpenJKDF2/resource/ui/openjkdf2.uni
extern const size_t embeddedResource_aFiles_num;
extern const embeddedResource_t embeddedResource_aFiles[2];

#ifdef __cplusplus
//}
#endif

#endif // _OPENJKDF2_GLOBALS_H
