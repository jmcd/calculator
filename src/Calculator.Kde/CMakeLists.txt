# Qt 6 / Kirigami frontend for KDE Plasma.

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Network Widgets Qml Quick QuickControls2)
include(GNUInstallDirs)

qt_standard_project_setup(REQUIRES 6.5)

set(CALCULATOR_APP_ID "io.github.jmcd.calculator")
set(CALCULATOR_RESOURCES_SRC "${PROJECT_SOURCE_DIR}/src/Calculator/Resources")

# Logic shared by the app and the tests.
set(CALCULATOR_KDE_CORE_SOURCES
    CalculatorController.cpp
    CalculatorController.h
    CurrencyDataLoader.cpp
    CurrencyDataLoader.h
    DateCalculator.cpp
    DateCalculator.h
    EngineResourceProvider.cpp
    EngineResourceProvider.h
    ReswStrings.cpp
    ReswStrings.h
    UnitConverterController.cpp
    UnitConverterController.h
    UnitConverterDataLoader.cpp
    UnitConverterDataLoader.h
)
list(TRANSFORM CALCULATOR_KDE_CORE_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

qt_add_executable(calculator-kde main.cpp ${CALCULATOR_KDE_CORE_SOURCES})

# The en-US engine strings are compiled in so the engine always has its operator
# symbols and error messages; translations are loaded from disk at runtime.
qt_add_resources(calculator-kde "engine_strings"
    PREFIX "/resources"
    BASE "${CALCULATOR_RESOURCES_SRC}"
    FILES "${CALCULATOR_RESOURCES_SRC}/en-US/CEngineStrings.resw"
)

# Default "from"/"to" currencies per locale, shared with the Windows app.
qt_add_resources(calculator-kde "converter_data"
    PREFIX "/converter"
    BASE "${PROJECT_SOURCE_DIR}/src/Calculator.ViewModels/DataLoaders"
    FILES "${PROJECT_SOURCE_DIR}/src/Calculator.ViewModels/DataLoaders/DefaultFromToCurrency.json"
)


qt_add_qml_module(calculator-kde
    URI Calculator
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/CalculatorPage.qml
        qml/CalcButton.qml
        qml/Display.qml
        qml/MemoryRow.qml
        qml/StandardKeypad.qml
        qml/ScientificKeypad.qml
        qml/ProgrammerKeypad.qml
        qml/ProgrammerDisplay.qml
        qml/BitFlipPanel.qml
        qml/HistoryMemoryPanel.qml
        qml/DateCalculatorPage.qml
        qml/ConverterPage.qml
        qml/AboutDialog.qml
)

target_link_libraries(calculator-kde PRIVATE
    CalcManager
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
)
target_compile_definitions(calculator-kde PRIVATE
    CALCULATOR_SOURCE_RESOURCES_DIR="${CALCULATOR_RESOURCES_SRC}"
    CALCULATOR_APP_ID="${CALCULATOR_APP_ID}"
    CALCULATOR_VERSION="${PROJECT_VERSION}"
)

install(TARGETS calculator-kde RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Localized strings shared with the Windows app.
install(DIRECTORY "${CALCULATOR_RESOURCES_SRC}/"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/calculator-kde/Resources
    FILES_MATCHING PATTERN "CEngineStrings.resw" PATTERN "Resources.resw"
)

install(FILES ${CALCULATOR_APP_ID}.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES ${CALCULATOR_APP_ID}.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

if(CALCULATOR_BUILD_TESTS)
    add_subdirectory(tests)
endif()
