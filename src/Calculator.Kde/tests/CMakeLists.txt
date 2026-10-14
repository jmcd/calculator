find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(calculator-kde-tests
    tst_calculator.cpp
    ${CALCULATOR_KDE_CORE_SOURCES}
)
qt_add_resources(calculator-kde-tests "converter_data"
    PREFIX "/converter"
    BASE "${PROJECT_SOURCE_DIR}/src/Calculator.ViewModels/DataLoaders"
    FILES "${PROJECT_SOURCE_DIR}/src/Calculator.ViewModels/DataLoaders/DefaultFromToCurrency.json"
)
target_include_directories(calculator-kde-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(calculator-kde-tests PRIVATE CalcManager Qt6::Gui Qt6::Network Qt6::Qml Qt6::Quick Qt6::Test)
target_compile_definitions(calculator-kde-tests PRIVATE
    CALCULATOR_SOURCE_RESOURCES_DIR="${CALCULATOR_RESOURCES_SRC}"
)

add_test(NAME calculator-kde-tests COMMAND calculator-kde-tests)
set_tests_properties(calculator-kde-tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LANG=en_US.UTF-8;LC_ALL=en_US.UTF-8"
)
