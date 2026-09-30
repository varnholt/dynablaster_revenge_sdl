cmake_minimum_required(VERSION 3.20)
project(dynablaster_revenge LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release builds emit no PDB by default under the Visual Studio generator, which makes crash
# dumps from real playtests (WER's %LOCALAPPDATA%\CrashDumps) unsymbolized/unusable - see project
# memory on the countdown/camera crash investigation. Adding /Zi + /DEBUG here doesn't change
# runtime codegen (still full /O2), just emits a separate .pdb alongside the .exe/.dll.
# Deliberately NOT /OPT:ICF: it folds byte-identical function bodies into one, so a crash dump's
# innermost frame can be misattributed to whichever function the linker picked to represent the
# fold - actively harmful for trustworthy crash symbolication, which is the whole point here.
if (MSVC)
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /Zi")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " /DEBUG /OPT:REF")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " /DEBUG /OPT:REF")
endif()

include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.16
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

# header-only MP3 decoder, used for music playback in SoundManager.
FetchContent_Declare(
    minimp3
    GIT_REPOSITORY https://github.com/lieff/minimp3.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(minimp3)

# networking for the client<->server protocol. Not yet linked into any target; the
# socket-handling rewrite lands separately.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3_net
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_net.git
    GIT_TAG release-3.2.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3_net)

file(GLOB_RECURSE DYNABLASTER_ENGINE_SOURCES
    CONFIGURE_DEPENDS
    src/tools/*.cpp src/tools/*.h
    src/math/*.cpp src/math/*.h
    src/engine/render/*.cpp src/engine/render/*.h
    src/engine/nodes/*.cpp src/engine/nodes/*.h
    src/engine/animation/*.cpp src/engine/animation/*.h
    src/engine/materials/*.cpp src/engine/materials/*.h
    src/framework/gldevice.cpp src/framework/gldevice.h
    src/framework/renderdevice.cpp src/framework/renderdevice.h
    src/framework/globaltime.cpp src/framework/globaltime.h
    src/framework/framebuffer.cpp src/framework/framebuffer.h
    src/framework/frametimer.cpp src/framework/frametimer.h
    src/framework/timerhandler.cpp src/framework/timerhandler.h
    src/framework/drawable.cpp src/framework/drawable.h
    src/framework/keyevent.cpp src/framework/keyevent.h
    src/image/*.cpp src/image/*.h
)

# shared/ (sibling directory, the real client<->server wire protocol). The shared/map.h vs
# tools/map.h `Map` collision that blocked this before is gone - tools/map.h's unused generic
# container was renamed to RbTreeMap (had zero real callers in client-sdl).
file(GLOB DYNABLASTER_SHARED_SOURCES CONFIGURE_DEPENDS ../shared/*.cpp ../shared/*.h)

# stray untracked Qt4-era files left in shared/ (not part of this port, never added to git) -
# exclude explicitly rather than deleting them out from under the working tree.
list(REMOVE_ITEM DYNABLASTER_SHARED_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/systemtools.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/systemtools.h
)

# server/src (sibling directory) - the real embedded server, copied in as-is, same set client.pro
# already embeds into the old client for local/hosted games (dummyplayer.cpp/servergui.cpp are
# standalone-server-only test tooling, excluded here same as client.pro excludes them). Zero
# GL/rendering code - pure Qt network/game-logic, so no SDL porting needed, just wiring in.
set(DYNABLASTER_SERVER_SOURCES
    ../server/src/collisiondetection.cpp
    ../server/src/collisiondetection.h
    ../server/src/extrashakepackethandler.cpp
    ../server/src/extrashakepackethandler.h
    ../server/src/extraspawn.cpp
    ../server/src/extraspawn.h
    ../server/src/game.cpp
    ../server/src/game.h
    ../server/src/server.cpp
    ../server/src/server.h
    ../server/src/serverversion.h
)

# ../ai - the bot AI (A* pathfinding + bot behaviour), headless BotClients embedded in the client.
set(DYNABLASTER_AI_SOURCES
    ../ai/astarmap.cpp
    ../ai/astarmap.h
    ../ai/astarnode.cpp
    ../ai/astarnode.h
    ../ai/astarpathfinding.cpp
    ../ai/astarpathfinding.h
    ../ai/bombchainreaction.cpp
    ../ai/bombchainreaction.h
    ../ai/bot.cpp
    ../ai/bot.h
    ../ai/botaction.cpp
    ../ai/botaction.h
    ../ai/botbombaction.cpp
    ../ai/botbombaction.h
    ../ai/botbombmapitem.cpp
    ../ai/botbombmapitem.h
    ../ai/botcharacter.cpp
    ../ai/botcharacter.h
    ../ai/botclient.cpp
    ../ai/botclient.h
    ../ai/botconstants.cpp
    ../ai/botconstants.h
    ../ai/botfactory.cpp
    ../ai/botfactory.h
    ../ai/botidleaction.cpp
    ../ai/botidleaction.h
    ../ai/botmap.cpp
    ../ai/botmap.h
    ../ai/botoption.cpp
    ../ai/botoption.h
    ../ai/botplayerinfo.cpp
    ../ai/botplayerinfo.h
    ../ai/botwalkaction.cpp
    ../ai/botwalkaction.h
    ../ai/path.cpp
    ../ai/path.h
    ../ai/pathfinding.cpp
    ../ai/pathfinding.h
    ../ai/protobot.cpp
    ../ai/protobot.h
    ../ai/protobotinsults.cpp
    ../ai/protobotinsults.h
    ../ai/protobotmemory.cpp
    ../ai/protobotmemory.h
)

# Everything the game and the test/diagnostic harness both need - i.e. everything except the two
# main()s themselves (src/main.cpp - the real game, src/main_harness.cpp - --selftest/--click/
# --logo3d/etc tooling). Kept as one shared static library so the two executables can never drift
# out of sync with each other and so the ~280 engine/menu/effects files only compile once.
add_library(dynablaster_core STATIC
    src/gles3.cpp
    src/gles3.h
    src/glescontext.cpp
    src/glescontext.h
    src/shader.cpp
    src/shader.h
    src/sdlglobaltime.cpp
    src/sdlglobaltime.h

    # Menu system port (Phase 1 of the menu-system plan): listed explicitly rather than
    # glob-included like the engine sources above, because client-sdl/src/menus/ only has the
    # subset of the real menus/ directory needed for the main menu page so far - globbing would
    # pull in not-yet-ported files.
    src/menus/vertex.cpp
    src/menus/vertex.h
    src/menus/clipper.cpp
    src/menus/clipper.h
    src/menus/defaultshader.cpp
    src/menus/defaultshader.h
    src/menus/psdlayer.cpp
    src/menus/psdlayer.h
    src/menus/bitmapfont.cpp
    src/menus/bitmapfont.h
    src/menus/fontmap.cpp
    src/menus/fontmap.h
    src/menus/fontpool.h
    src/menus/gamefonts.cpp
    src/menus/gamefonts.h
    src/menus/menupageanimation.cpp
    src/menus/menupageanimation.h
    src/menus/menupagefadeanimation.cpp
    src/menus/menupagefadeanimation.h
    src/menus/menupageitem.cpp
    src/menus/menupageitem.h
    src/menus/menumousecursor.cpp
    src/menus/menumousecursor.h
    src/menus/menu.cpp
    src/menus/menu.h
    src/menus/menupage.cpp
    src/menus/menupage.h
    src/menus/menudrawable.cpp
    src/menus/menudrawable.h
    src/menus/menupagebuttonitem.cpp
    src/menus/menupagebuttonitem.h
    src/menus/menupagebackgrounditem.cpp
    src/menus/menupagebackgrounditem.h
    src/menus/menupagelabelitem.cpp
    src/menus/menupagelabelitem.h
    src/menus/menupagetextedit.cpp
    src/menus/menupagetextedit.h
    src/menus/menupagelistitemelement.cpp
    src/menus/menupagelistitemelement.h
    src/menus/menupagelistitem.cpp
    src/menus/menupagelistitem.h
    src/menus/menupagecomboboxitem.cpp
    src/menus/menupagecomboboxitem.h
    src/menus/menupageeditablecomboboxitem.cpp
    src/menus/menupageeditablecomboboxitem.h
    src/menus/menupagecheckboxitem.cpp
    src/menus/menupagecheckboxitem.h
    src/menus/menupagepixmapitem.cpp
    src/menus/menupagepixmapitem.h
    src/menus/menupagescrollbar.cpp
    src/menus/menupagescrollbar.h
    src/menus/menupageslideritem.cpp
    src/menus/menupageslideritem.h
    src/menus/menupagescrollimageitem.cpp
    src/menus/menupagescrollimageitem.h
    src/menus/menupagenavigator.cpp
    src/menus/menupagenavigator.h

    # Sphere-fragments effect (the 3D "exploding earth/bomb" logo subsystem GameLogoDrawable is
    # built on) - listed explicitly like the menu system above, since only this subset of the real
    # client/src/effects/ tree has been ported so far.
    src/postproduction/filter.cpp
    src/postproduction/filter.h
    src/postproduction/fullscreenquad.cpp
    src/postproduction/fullscreenquad.h
    src/postproduction/blurfilter.cpp
    src/postproduction/blurfilter.h
    src/postproduction/motionblurfilter.cpp
    src/postproduction/motionblurfilter.h
    src/postproduction/shroomfilter.cpp
    src/postproduction/shroomfilter.h
    src/effects/ribbons/ribbonanimationfactory.cpp
    src/effects/ribbons/ribbonanimationfactory.h
    src/effects/lensflare/lensflare.cpp
    src/effects/lensflare/lensflare.h
    src/effects/lensflare/lensflarefactory.cpp
    src/effects/lensflare/lensflarefactory.h
    src/effects/spherefragments/vertex3d.h
    src/effects/spherefragments/geometryvbo.cpp
    src/effects/spherefragments/geometryvbo.h
    src/effects/spherefragments/spheregeometryvbo.cpp
    src/effects/spherefragments/spheregeometryvbo.h
    src/effects/spherefragments/bombfuzegeometryvbo.cpp
    src/effects/spherefragments/bombfuzegeometryvbo.h
    src/effects/spherefragments/bombsocketgeometryvbo.cpp
    src/effects/spherefragments/bombsocketgeometryvbo.h
    src/effects/spherefragments/blendquad.cpp
    src/effects/spherefragments/blendquad.h
    src/effects/spherefragments/duplicatealpha.cpp
    src/effects/spherefragments/duplicatealpha.h
    src/effects/spherefragments/spherefragment.cpp
    src/effects/spherefragments/spherefragment.h
    src/effects/spherefragments/spherefragmentcontainer.cpp
    src/effects/spherefragments/spherefragmentcontainer.h
    src/effects/spherefragments/spherefragmentsdrawable.cpp
    src/effects/spherefragments/spherefragmentsdrawable.h

    # GameLogoDrawable - the animated main-menu logo built on top of the sphere-fragments effect
    # above (rotating-cube pass and earth/highlight ortho pass excluded - see gamelogodrawable.h,
    # both are dead code in the real shipped game).
    src/game/gamelogopointsprite.cpp
    src/game/gamelogopointsprite.h
    src/game/gamelogodrawable.cpp
    src/game/gamelogodrawable.h

    # Phase 4 (real client<->server networking) prep - small, self-contained utility classes
    # BombermanClient itself depends on, ported ahead of it.
    src/game/gamehelptexts.h
    src/game/gamesettings.cpp
    src/game/gamesettings.h
    src/game/gamestatemachine.cpp
    src/game/gamestatemachine.h
    src/game/helpmanager.cpp
    src/game/helpmanager.h
    src/game/mapitemanimation.cpp
    src/game/mapitemanimation.h
    src/game/mushroomanimation.cpp
    src/game/mushroomanimation.h
    src/levels/camerainterpolation.cpp
    src/levels/camerainterpolation.h
    src/levels/level.cpp
    src/levels/level.h

    # Phase 4 continued: real client<->server networking. soundmanager.{h,cpp}/gameplayback.{h,cpp}
    # are no-op stand-ins for still-unported subsystems (real audio backend, replay/record) - see
    # the "Deferred subsystem" comment in each for what's stubbed and why. Bot AI is NOT a stub -
    # see DYNABLASTER_AI_SOURCES above, the real ../ai/ tree is embedded like server/src is.
    src/game/gameplayback.cpp
    src/game/gameplayback.h
    src/game/soundmanager.cpp
    src/game/soundmanager.h
    src/game/positioninterpolation.cpp
    src/game/positioninterpolation.h
    src/game/bombermanclient.cpp
    src/game/bombermanclient.h
    src/game/wordwrap.cpp
    src/game/wordwrap.h
    src/menus/hosthistory.cpp
    src/menus/hosthistory.h

    # Phase 5 (real in-game rendering - map/players/bombs/extras/HUD) - see project memory for the
    # full dependency-graph scoping this was based on and what's deliberately deferred (particle
    # effects, joystick, chat, music player UI, etc).
    src/game/skull.cpp
    src/game/skull.h
    src/game/playerboundingrect.cpp
    src/game/playerboundingrect.h
    src/game/playeritem.cpp
    src/game/playeritem.h
    src/game/extra.cpp
    src/game/extra.h
    src/game/extraanimations.cpp
    src/game/extraanimations.h
    src/levels/levelfactory.cpp
    src/levels/levelfactory.h
    src/levels/castle/levelcastle.cpp
    src/levels/castle/levelcastle.h
    src/levels/castle/snowanimation.cpp
    src/levels/castle/snowanimation.h
    src/levels/mansion/levelmansion.cpp
    src/levels/mansion/levelmansion.h
    src/levels/space/levelspace.cpp
    src/levels/space/levelspace.h
    src/levels/space/spacebackground.cpp
    src/levels/space/spacebackground.h
    src/game/detonationmanager.cpp
    src/game/detonationmanager.h
    src/game/detonation.cpp
    src/game/detonation.h
    src/game/deathflowfieldanimation.cpp
    src/game/deathflowfieldanimation.h
    src/game/playerdeatheffect.cpp
    src/game/playerdeatheffect.h
    src/game/infectedflowfieldanimation.cpp
    src/game/infectedflowfieldanimation.h
    src/game/invisibleplayereffect.cpp
    src/game/invisibleplayereffect.h
    src/game/playerinfectedeffect.cpp
    src/game/playerinfectedeffect.h
    src/game/fuseparticlesystem.cpp
    src/game/fuseparticlesystem.h
    src/game/playerinvincibleinstance.cpp
    src/game/playerinvincibleinstance.h
    src/game/playerinvincibleeffect.cpp
    src/game/playerinvincibleeffect.h
    src/game/gameplayernamedisplay.cpp
    src/game/gameplayernamedisplay.h
    src/game/gamewindrawable.cpp
    src/game/gamewindrawable.h
    src/game/gamemessage.cpp
    src/game/gamemessage.h
    src/game/animatedgamemessage.cpp
    src/game/animatedgamemessage.h
    src/game/gamemessagingdrawable.cpp
    src/game/gamemessagingdrawable.h
    src/game/gamedrawable.cpp
    src/game/gamedrawable.h
    src/game/countdowndrawable.cpp
    src/game/countdowndrawable.h
    src/game/roundsdrawable.cpp
    src/game/roundsdrawable.h
    src/game/startalersfactory.cpp
    src/game/startalersfactory.h
    src/game/musicplayerdrawable.cpp
    src/game/musicplayerdrawable.h
    src/game/gameversion.h

    ${DYNABLASTER_ENGINE_SOURCES}
    ${DYNABLASTER_SHARED_SOURCES}
    ${DYNABLASTER_SERVER_SOURCES}
    ${DYNABLASTER_AI_SOURCES}
)

# NOTE: do not add src/tools (or similar) directly as an include dir - tools/string.h vs the
# real <string.h> is a real collision, and it silently breaks Qt/STL headers that #include
# <string.h> internally (they'd resolve to our String class instead). Bare includes like
# "stream.h" from inside engine/materials were fixed to proper relative paths instead.
target_include_directories(dynablaster_core PUBLIC
    third_party
    src
    src/framework
    src/engine
    ../shared
    ../server/src
    ../ai
    ${minimp3_SOURCE_DIR}
)
target_link_libraries(dynablaster_core PUBLIC SDL3::SDL3 SDL3_net::SDL3_net)

# real C++ exception catching (SettingsValue::toFloat()'s try/catch, shared/settings.cpp) needs
# this at compile time too, not just at dynablaster_revenge's link step - DISABLE_EXCEPTION_CATCHING
# affects codegen (invoke_* wrappers), not just the final link.
if (EMSCRIPTEN)
    target_compile_options(dynablaster_core PUBLIC -sDISABLE_EXCEPTION_CATCHING=0)
endif()

# dynablaster_revenge - the real game, no CLI flags, launches straight into the main menu.
add_executable(dynablaster_revenge src/main.cpp)
target_link_libraries(dynablaster_revenge PRIVATE dynablaster_core)

# wasm32 target (browser build). No raw sockets in a browser, so real client<->server networking
# doesn't work here (see main.cpp's __EMSCRIPTEN__ NET_Init handling) - this is client-only.
if (EMSCRIPTEN)
    target_link_options(dynablaster_revenge PRIVATE
        -sMAX_WEBGL_VERSION=2
        -sMIN_WEBGL_VERSION=2
        -sFULL_ES3=1
        -sALLOW_MEMORY_GROWTH=1
        # emscripten aborts on any thrown C++ exception by default (DISABLE_EXCEPTION_CATCHING=1) -
        # SettingsValue::toFloat()'s try/catch (shared/settings.cpp) needs real catching to work.
        -sDISABLE_EXCEPTION_CATCHING=0
        --emrun
        # only the castle level ships in the browser, LevelFactory falls back to it; SHELL: keeps
        # CMake from de-duplicating the repeated --exclude-file flag away from its argument
        "SHELL:--exclude-file '*/data/level-mansion*'"
        "SHELL:--exclude-file '*/data/level-space*'"
        --preload-file ${CMAKE_SOURCE_DIR}/data@data
    )
    set_target_properties(dynablaster_revenge PROPERTIES SUFFIX ".html")
endif()

# dynablaster_revenge_harness - test/diagnostic tooling (--selftest, --menu, --click, --realclick,
# --dumplayer, --page, --logo3d, --screenshot, and the standalone castle-level demo) - see
# src/main_harness.cpp's own header comment. Not the shipped game; shares dynablaster_core so it
# can never drift out of sync with the real thing it's testing.
add_executable(dynablaster_revenge_harness
    src/main_harness.cpp
    src/effectlab.cpp
    src/effectlab.h
    src/screenshot.cpp
    src/screenshot.h
    src/inputinjector.cpp
    src/inputinjector.h
    src/demofactory.cpp
    src/demofactory.h
)
target_link_libraries(dynablaster_revenge_harness PRIVATE dynablaster_core)

foreach(dynablaster_target dynablaster_revenge dynablaster_revenge_harness)
    # Shaders/textures load through FileStream at runtime (not compiled in), so they need to sit
    # next to the exe - mirrors the DLL copy step above.
    add_custom_command(TARGET ${dynablaster_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data
            $<TARGET_FILE_DIR:${dynablaster_target}>/data
    )
endforeach()

# both executables copy into the same directory; serialize them so the copies never overlap
add_dependencies(dynablaster_revenge_harness dynablaster_revenge)

enable_testing()
add_executable(dynablaster_gles_compatibility_test tests/test_gles_compatibility.cpp)
target_link_libraries(dynablaster_gles_compatibility_test PRIVATE dynablaster_core)
add_test(NAME gles_compatibility COMMAND dynablaster_gles_compatibility_test)
add_test(NAME selftest_smoke
    COMMAND dynablaster_revenge_harness --selftest --screenshot=${CMAKE_BINARY_DIR}/selftest.png
)
