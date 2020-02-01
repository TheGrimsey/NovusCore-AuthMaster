project(server-authmaster VERSION 1.0.0 DESCRIPTION "NovusCore Authmaster")

file(GLOB_RECURSE SERVER_AUTHMASTER_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SERVER_AUTHMASTER_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${ROOT_FOLDER})

find_assign_files(${SERVER_AUTHMASTER_FILES})
add_compile_definitions(NOMINMAX _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

target_link_libraries(${PROJECT_NAME} PRIVATE
	asio::asio
	common::common
	network::network
	Entt::Entt
	taskflow::taskflow
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)