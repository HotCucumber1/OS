find_package(Boost REQUIRED COMPONENTS
        json
        url
)

add_executable(WebSearch
        main.cpp
        backend/MtSearch/MtSearch.cpp
        backend/Server/Session.cpp
        backend/Server/Listener.cpp
        backend/Handlers/ProcessOptionsHandler.h
        backend/Handlers/BaseHandler.h
        backend/Handlers/BadRequestHandler.h)

target_link_libraries(WebSearch PRIVATE
        Boost::json
        Boost::url
)
