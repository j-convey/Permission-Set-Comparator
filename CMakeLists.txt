cmake_minimum_required(VERSION 3.21)
project(SalesforcePermCalc LANGUAGES CXX)

# User can set -DCMAKE_PREFIX_PATH to their Qt6 install, e.g. C:/Qt/6.7.0/msvc2019_64

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(SalesforcePermCalc
    perm_set_calculator.cpp
    resources.rc
)

# If Qt's AUTOMOC is enabled implicitly; ensure it is for signals/slots.
set_target_properties(SalesforcePermCalc PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE ON
)

# Include current dir for generated MOC includes
target_include_directories(SalesforcePermCalc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(SalesforcePermCalc PRIVATE Qt6::Widgets)

# Copy icon files (if present) next to exe after build for runtime loading
add_custom_command(TARGET SalesforcePermCalc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Salesforce_perm_Calc_icon.png
        $<TARGET_FILE_DIR:SalesforcePermCalc>/Salesforce_perm_Calc_icon.png
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Salesforce_perm_Calc_icon.ico
        $<TARGET_FILE_DIR:SalesforcePermCalc>/Salesforce_perm_Calc_icon.ico
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Permission Sets.csv"
        "$<TARGET_FILE_DIR:SalesforcePermCalc>/Permission Sets.csv"
    VERBATIM
)
