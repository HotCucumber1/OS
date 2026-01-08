include(FetchContent)

FetchContent_Declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.1
)

set(SFML_BUILD_AUDIO FALSE)
set(SFML_BUILD_NETWORK FALSE)

FetchContent_MakeAvailable(sfml)

add_executable(PaintOnline main.cpp
        App/App.cpp
        Client/Client.cpp
        Server/Session.cpp
        Server/Room.cpp
        Server/Server.cpp)

target_link_libraries(
        PaintOnline PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
)
