# Portable build of the calculation engine (used by the Linux / KDE frontend).
# UnitConverter.cpp is excluded because it depends on the PPL (concurrency::task).

add_library(CalcManager STATIC
    CalculatorHistory.cpp
    CalculatorManager.cpp
    ExpressionCommand.cpp
    NumberFormattingUtils.cpp
    CEngine/calc.cpp
    CEngine/CalcInput.cpp
    CEngine/CalcUtils.cpp
    CEngine/History.cpp
    CEngine/Number.cpp
    CEngine/Rational.cpp
    CEngine/RationalMath.cpp
    CEngine/scicomm.cpp
    CEngine/scidisp.cpp
    CEngine/scifunc.cpp
    CEngine/scioper.cpp
    CEngine/sciset.cpp
    Ratpack/basex.cpp
    Ratpack/conv.cpp
    Ratpack/exp.cpp
    Ratpack/fact.cpp
    Ratpack/itrans.cpp
    Ratpack/itransh.cpp
    Ratpack/logic.cpp
    Ratpack/num.cpp
    Ratpack/rat.cpp
    Ratpack/support.cpp
    Ratpack/trans.cpp
    Ratpack/transh.cpp
)

target_include_directories(CalcManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(CalcManager PROPERTIES POSITION_INDEPENDENT_CODE ON)
