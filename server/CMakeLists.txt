cmake_minimum_required(VERSION 3.20)
project(dynablaster_server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3_net
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_net.git
    GIT_TAG release-3.2.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3_net)

# story mode enemy behaviour scripts
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/lua.cmake)

# shared/ (sibling directory, the real client<->server wire protocol) - the exact set server.pro
# lists, plus playerrotation.{h,cpp}: not in server.pro's own SOURCES/HEADERS lists, but
# player.h #includes it directly (PlayerRotation is a real Player member), so it's compiled in
# regardless. playerinfo.{h,cpp} is client-only (nothing in server/src references it) and stays
# excluded.
set(DYNABLASTER_SHARED_SOURCES
    ../shared/blockmapitem.cpp
    ../shared/blockmapitem.h
    ../shared/binaryreader.cpp
    ../shared/binaryreader.h
    ../shared/binarywriter.cpp
    ../shared/binarywriter.h
    ../shared/bombkickanimation.cpp
    ../shared/bombkickanimation.h
    ../shared/bombmapitem.cpp
    ../shared/bombmapitem.h
    ../shared/bombpacket.cpp
    ../shared/bombpacket.h
    ../shared/constants.cpp
    ../shared/constants.h
    ../shared/countdownpacket.cpp
    ../shared/countdownpacket.h
    ../shared/creategamedata.cpp
    ../shared/creategamedata.h
    ../shared/creategamerequestpacket.cpp
    ../shared/creategamerequestpacket.h
    ../shared/creategameresponsepacket.cpp
    ../shared/creategameresponsepacket.h
    ../shared/detonationpacket.cpp
    ../shared/detonationpacket.h
    ../shared/enemycreatedpacket.cpp
    ../shared/enemycreatedpacket.h
    ../shared/enemyhitpacket.cpp
    ../shared/enemyhitpacket.h
    ../shared/enemykilledpacket.cpp
    ../shared/enemykilledpacket.h
    ../shared/enemypositionpacket.cpp
    ../shared/enemypositionpacket.h
    ../shared/enemytype.cpp
    ../shared/enemytype.h
    ../shared/storystage.h
    ../shared/storystages.cpp
    ../shared/storystatepacket.cpp
    ../shared/storystatepacket.h
    ../shared/elapsedtimer.cpp
    ../shared/elapsedtimer.h
    ../shared/errorpacket.cpp
    ../shared/errorpacket.h
    ../shared/extramapitem.cpp
    ../shared/extramapitem.h
    ../shared/extramapitemcreatedpacket.cpp
    ../shared/extramapitemcreatedpacket.h
    ../shared/extrashakepacket.cpp
    ../shared/extrashakepacket.h
    ../shared/gameeventpacket.cpp
    ../shared/gameeventpacket.h
    ../shared/gameinformation.cpp
    ../shared/gameinformation.h
    ../shared/gameround.cpp
    ../shared/gameround.h
    ../shared/gamestatspacket.cpp
    ../shared/gamestatspacket.h
    ../shared/joingamerequestpacket.cpp
    ../shared/joingamerequestpacket.h
    ../shared/joingameresponsepacket.cpp
    ../shared/joingameresponsepacket.h
    ../shared/keypacket.cpp
    ../shared/keypacket.h
    ../shared/leavegamerequestpacket.cpp
    ../shared/leavegamerequestpacket.h
    ../shared/leavegameresponsepacket.cpp
    ../shared/leavegameresponsepacket.h
    ../shared/listgamesrequestpacket.cpp
    ../shared/listgamesrequestpacket.h
    ../shared/listgamesresponsepacket.cpp
    ../shared/listgamesresponsepacket.h
    ../shared/loginrequestpacket.cpp
    ../shared/loginrequestpacket.h
    ../shared/loginresponsepacket.cpp
    ../shared/loginresponsepacket.h
    ../shared/map.cpp
    ../shared/map.h
    ../shared/mapcreaterequestpacket.cpp
    ../shared/mapcreaterequestpacket.h
    ../shared/mapitem.cpp
    ../shared/mapitem.h
    ../shared/nethandles.h
    ../shared/mapitemcreatedpacket.cpp
    ../shared/mapitemcreatedpacket.h
    ../shared/mapitemdestroyedpacket.cpp
    ../shared/mapitemdestroyedpacket.h
    ../shared/mapitemmovepacket.cpp
    ../shared/mapitemmovepacket.h
    ../shared/mapitempacket.cpp
    ../shared/mapitempacket.h
    ../shared/mapitemremovedpacket.cpp
    ../shared/mapitemremovedpacket.h
    ../shared/messagepacket.cpp
    ../shared/messagepacket.h
    ../shared/packet.cpp
    ../shared/packet.h
    ../shared/packetstreambuffer.cpp
    ../shared/packetstreambuffer.h
    ../shared/player.cpp
    ../shared/player.h
    ../shared/playerdisease.cpp
    ../shared/playerdisease.h
    ../shared/playerrotation.cpp
    ../shared/playerrotation.h
    ../shared/playerinfectedpacket.cpp
    ../shared/playerinfectedpacket.h
    ../shared/playerkilledpacket.cpp
    ../shared/playerkilledpacket.h
    ../shared/playermodifiedpacket.cpp
    ../shared/playermodifiedpacket.h
    ../shared/playerstats.cpp
    ../shared/playerstats.h
    ../shared/playersynchronizepacket.cpp
    ../shared/playersynchronizepacket.h
    ../shared/point.cpp
    ../shared/point.h
    ../shared/positionpacket.cpp
    ../shared/positionpacket.h
    ../shared/random.cpp
    ../shared/random.h
    ../shared/serverconfiguration.cpp
    ../shared/serverconfiguration.h
    ../shared/settings.cpp
    ../shared/settings.h
    ../shared/startgamerequestpacket.cpp
    ../shared/startgamerequestpacket.h
    ../shared/startgameresponsepacket.cpp
    ../shared/startgameresponsepacket.h
    ../shared/stonedroppacket.cpp
    ../shared/stonedroppacket.h
    ../shared/stonemapitem.cpp
    ../shared/stonemapitem.h
    ../shared/stopgamerequestpacket.cpp
    ../shared/stopgamerequestpacket.h
    ../shared/stopgameresponsepacket.cpp
    ../shared/stopgameresponsepacket.h
    ../shared/stringutils.cpp
    ../shared/stringutils.h
    ../shared/timepacket.cpp
    ../shared/timepacket.h
    ../shared/timer.cpp
    ../shared/timer.h
    ../shared/vec2.cpp
    ../shared/vec2.h
)

# src/ - game/session logic plus main.cpp. dummyplayer.{h,cpp} is left out: its only caller
# was servergui.cpp (dropped, unused Qt Widgets console), nothing in server.cpp/game.cpp
# references DummyPlayer at all - it's dead code without the GUI.
set(DYNABLASTER_SERVER_SOURCES
    src/collisiondetection.cpp
    src/collisiondetection.h
    src/connection.cpp
    src/connection.h
    src/extrashakepackethandler.cpp
    src/extrashakepackethandler.h
    src/extraspawn.cpp
    src/extraspawn.h
    src/game.cpp
    src/game.h
    src/server.cpp
    src/server.h
    src/serverversion.h
    src/enemies/enemy.cpp
    src/enemies/enemy.h
    src/enemies/enemyfactory.cpp
    src/enemies/enemyfactory.h
    src/enemies/enemyworld.h
    src/enemies/scriptedenemy.cpp
    src/enemies/scriptedenemy.h
    src/story/storymode.cpp
    src/story/storymode.h
)

add_executable(dynablaster_server
    src/main.cpp
    ${DYNABLASTER_SHARED_SOURCES}
    ${DYNABLASTER_SERVER_SOURCES}
)

target_include_directories(dynablaster_server PRIVATE
    ../shared
    src
)

target_link_libraries(dynablaster_server PRIVATE SDL3::SDL3 SDL3_net::SDL3_net lua)

add_custom_command(TARGET dynablaster_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:dynablaster_server>/data
    # the enemy scripts ship with the client's data, the server runs the same ones
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../client/data/scripts
        $<TARGET_FILE_DIR:dynablaster_server>/data/scripts
)
