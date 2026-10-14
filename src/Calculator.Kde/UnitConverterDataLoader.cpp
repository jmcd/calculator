// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "UnitConverterDataLoader.h"
#include "ReswStrings.h"

#include <algorithm>
#include <tuple>

using namespace UnitConversionManager;

namespace
{
    // Unit IDs. These match UnitConverterDataConstants in the Windows app, so
    // saved preferences use the same IDs on both platforms.
    constexpr int Area_Acre = 1;
    constexpr int Area_Hectare = 2;
    constexpr int Area_SquareCentimeter = 3;
    constexpr int Area_SquareFoot = 4;
    constexpr int Area_SquareInch = 5;
    constexpr int Area_SquareKilometer = 6;
    constexpr int Area_SquareMeter = 7;
    constexpr int Area_SquareMile = 8;
    constexpr int Area_SquareMillimeter = 9;
    constexpr int Area_SquareYard = 10;
    constexpr int Data_Bit = 11;
    constexpr int Data_Byte = 12;
    constexpr int Data_Gigabit = 13;
    constexpr int Data_Gigabyte = 14;
    constexpr int Data_Kilobit = 15;
    constexpr int Data_Kilobyte = 16;
    constexpr int Data_Megabit = 17;
    constexpr int Data_Megabyte = 18;
    constexpr int Data_Petabit = 19;
    constexpr int Data_Petabyte = 20;
    constexpr int Data_Terabit = 21;
    constexpr int Data_Terabyte = 22;
    constexpr int Energy_BritishThermalUnit = 23;
    constexpr int Energy_Calorie = 24;
    constexpr int Energy_ElectronVolt = 25;
    constexpr int Energy_FootPound = 26;
    constexpr int Energy_Joule = 27;
    constexpr int Energy_Kilocalorie = 28;
    constexpr int Energy_Kilojoule = 29;
    constexpr int Length_Centimeter = 30;
    constexpr int Length_Foot = 31;
    constexpr int Length_Inch = 32;
    constexpr int Length_Kilometer = 33;
    constexpr int Length_Meter = 34;
    constexpr int Length_Micron = 35;
    constexpr int Length_Mile = 36;
    constexpr int Length_Millimeter = 37;
    constexpr int Length_Nanometer = 38;
    constexpr int Length_NauticalMile = 39;
    constexpr int Length_Yard = 40;
    constexpr int Power_BritishThermalUnitPerMinute = 41;
    constexpr int Power_FootPoundPerMinute = 42;
    constexpr int Power_Horsepower = 43;
    constexpr int Power_Kilowatt = 44;
    constexpr int Power_Watt = 45;
    constexpr int Temperature_DegreesCelsius = 46;
    constexpr int Temperature_DegreesFahrenheit = 47;
    constexpr int Temperature_Kelvin = 48;
    constexpr int Time_Day = 49;
    constexpr int Time_Hour = 50;
    constexpr int Time_Microsecond = 51;
    constexpr int Time_Millisecond = 52;
    constexpr int Time_Minute = 53;
    constexpr int Time_Second = 54;
    constexpr int Time_Week = 55;
    constexpr int Time_Year = 56;
    constexpr int Speed_CentimetersPerSecond = 57;
    constexpr int Speed_FeetPerSecond = 58;
    constexpr int Speed_KilometersPerHour = 59;
    constexpr int Speed_Knot = 60;
    constexpr int Speed_Mach = 61;
    constexpr int Speed_MetersPerSecond = 62;
    constexpr int Speed_MilesPerHour = 63;
    constexpr int Volume_CubicCentimeter = 64;
    constexpr int Volume_CubicFoot = 65;
    constexpr int Volume_CubicInch = 66;
    constexpr int Volume_CubicMeter = 67;
    constexpr int Volume_CubicYard = 68;
    constexpr int Volume_CupUS = 69;
    constexpr int Volume_FluidOunceUK = 70;
    constexpr int Volume_FluidOunceUS = 71;
    constexpr int Volume_GallonUK = 72;
    constexpr int Volume_GallonUS = 73;
    constexpr int Volume_Liter = 74;
    constexpr int Volume_Milliliter = 75;
    constexpr int Volume_PintUK = 76;
    constexpr int Volume_PintUS = 77;
    constexpr int Volume_TablespoonUS = 78;
    constexpr int Volume_TeaspoonUS = 79;
    constexpr int Volume_QuartUK = 80;
    constexpr int Volume_QuartUS = 81;
    constexpr int Weight_Carat = 82;
    constexpr int Weight_Centigram = 83;
    constexpr int Weight_Decigram = 84;
    constexpr int Weight_Decagram = 85;
    constexpr int Weight_Gram = 86;
    constexpr int Weight_Hectogram = 87;
    constexpr int Weight_Kilogram = 88;
    constexpr int Weight_LongTon = 89;
    constexpr int Weight_Milligram = 90;
    constexpr int Weight_Ounce = 91;
    constexpr int Weight_Pound = 92;
    constexpr int Weight_ShortTon = 93;
    constexpr int Weight_Stone = 94;
    constexpr int Weight_Tonne = 95;
    constexpr int Area_SoccerField = 99;
    constexpr int Data_FloppyDisk = 100;
    constexpr int Data_CD = 101;
    constexpr int Data_DVD = 102;
    constexpr int Energy_Battery = 103;
    constexpr int Length_Paperclip = 105;
    constexpr int Length_JumboJet = 107;
    constexpr int Power_LightBulb = 108;
    constexpr int Power_Horse = 109;
    constexpr int Volume_Bathtub = 111;
    constexpr int Weight_Snowflake = 113;
    constexpr int Weight_Elephant = 114;
    constexpr int Volume_TeaspoonUK = 115;
    constexpr int Volume_TablespoonUK = 116;
    constexpr int Area_Hand = 118;
    constexpr int Speed_Turtle = 121;
    constexpr int Speed_Jet = 122;
    constexpr int Volume_CoffeeCup = 124;
    constexpr int Weight_Whale = 123;
    constexpr int Volume_SwimmingPool = 125;
    constexpr int Speed_Horse = 126;
    constexpr int Area_Paper = 127;
    constexpr int Area_Castle = 128;
    constexpr int Energy_Banana = 129;
    constexpr int Energy_SliceOfCake = 130;
    constexpr int Length_Hand = 131;
    constexpr int Power_TrainEngine = 132;
    constexpr int Weight_SoccerBall = 133;
    constexpr int Angle_Degree = 134;
    constexpr int Angle_Radian = 135;
    constexpr int Angle_Gradian = 136;
    constexpr int Pressure_Atmosphere = 137;
    constexpr int Pressure_Bar = 138;
    constexpr int Pressure_KiloPascal = 139;
    constexpr int Pressure_MillimeterOfMercury = 140;
    constexpr int Pressure_Pascal = 141;
    constexpr int Pressure_PSI = 142;
    constexpr int Data_Exabits = 143;
    constexpr int Data_Exabytes = 144;
    constexpr int Data_Exbibits = 145;
    constexpr int Data_Exbibytes = 146;
    constexpr int Data_Gibibits = 147;
    constexpr int Data_Gibibytes = 148;
    constexpr int Data_Kibibits = 149;
    constexpr int Data_Kibibytes = 150;
    constexpr int Data_Mebibits = 151;
    constexpr int Data_Mebibytes = 152;
    constexpr int Data_Pebibits = 153;
    constexpr int Data_Pebibytes = 154;
    constexpr int Data_Tebibits = 155;
    constexpr int Data_Tebibytes = 156;
    constexpr int Data_Yobibits = 157;
    constexpr int Data_Yobibytes = 158;
    constexpr int Data_Yottabit = 159;
    constexpr int Data_Yottabyte = 160;
    constexpr int Data_Zebibits = 161;
    constexpr int Data_Zebibytes = 162;
    constexpr int Data_Zetabits = 163;
    constexpr int Data_Zetabytes = 164;
    constexpr int Area_Pyeong = 165;
    constexpr int Energy_Kilowatthour = 166;
    constexpr int Data_Nibble = 167;
    constexpr int Length_Angstrom = 168;

    QString CategoryNameKey(ConverterCategory category)
    {
        switch (category)
        {
        case ConverterCategory::Currency: return QStringLiteral("CategoryName_CurrencyText");
        case ConverterCategory::Volume: return QStringLiteral("CategoryName_VolumeText");
        case ConverterCategory::Length: return QStringLiteral("CategoryName_LengthText");
        case ConverterCategory::Weight: return QStringLiteral("CategoryName_WeightText");
        case ConverterCategory::Temperature: return QStringLiteral("CategoryName_TemperatureText");
        case ConverterCategory::Energy: return QStringLiteral("CategoryName_EnergyText");
        case ConverterCategory::Area: return QStringLiteral("CategoryName_AreaText");
        case ConverterCategory::Speed: return QStringLiteral("CategoryName_SpeedText");
        case ConverterCategory::Time: return QStringLiteral("CategoryName_TimeText");
        case ConverterCategory::Power: return QStringLiteral("CategoryName_PowerText");
        case ConverterCategory::Data: return QStringLiteral("CategoryName_DataText");
        case ConverterCategory::Pressure: return QStringLiteral("CategoryName_PressureText");
        case ConverterCategory::Angle: return QStringLiteral("CategoryName_AngleText");
        }
        return {};
    }

    std::wstring Loc(const char* key)
    {
        return ReswStrings::instance()->get(QString::fromLatin1(key)).toStdWString();
    }
}

UnitConverterDataLoader::UnitConverterDataLoader(QString region)
    : m_region(std::move(region))
{
}

QString UnitConverterDataLoader::categoryName(ConverterCategory category)
{
    return ReswStrings::instance()->get(CategoryNameKey(category));
}

void UnitConverterDataLoader::LoadData()
{
    if (m_loaded)
    {
        return;
    }
    m_loaded = true;

    const QString& region = m_region;

    // Region-dependent defaults.
    const bool useUSCustomaryAndFahrenheit = region == u"US" || region == u"FM" || region == u"MH" || region == u"PW";
    const bool useUSCustomary = useUSCustomaryAndFahrenheit || region == u"LR";
    const bool useSI = !useUSCustomary;
    const bool useFahrenheit = useUSCustomaryAndFahrenheit || region == u"BS" || region == u"KY" || region == u"LR";
    const bool useWattInsteadOfKilowatt = region == u"GB";
    const bool usePyeong = region == u"JP" || region == u"TW" || region == u"KP" || region == u"KR";

    for (int c = static_cast<int>(ConverterCategory::Currency); c <= static_cast<int>(ConverterCategory::Angle); ++c)
    {
        const auto category = static_cast<ConverterCategory>(c);
        const bool supportsNegative =
            category == ConverterCategory::Temperature || category == ConverterCategory::Power || category == ConverterCategory::Angle;
        m_categories.emplace_back(c, categoryName(category).toStdWString(), supportsNegative);
    }

    std::map<int, std::vector<std::pair<Unit, int>>> unitsWithOrder;
    auto addUnit = [&](ConverterCategory category,
                       int id,
                       const char* nameKey,
                       const char* abbreviationKey,
                       int order,
                       bool isSource = false,
                       bool isTarget = false,
                       bool isWhimsical = false) {
        unitsWithOrder[static_cast<int>(category)].emplace_back(
            Unit(id, Loc(nameKey), Loc(abbreviationKey), isSource, isTarget, isWhimsical), order);
    };

    // ---- Area ----
    addUnit(ConverterCategory::Area, Area_Acre, "UnitName_Acre", "UnitAbbreviation_Acre", 9);
    addUnit(ConverterCategory::Area, Area_Hectare, "UnitName_Hectare", "UnitAbbreviation_Hectare", 4);
    addUnit(ConverterCategory::Area, Area_SquareCentimeter, "UnitName_SquareCentimeter", "UnitAbbreviation_SquareCentimeter", 2);
    addUnit(ConverterCategory::Area, Area_SquareFoot, "UnitName_SquareFoot", "UnitAbbreviation_SquareFoot", 7, useSI, useUSCustomary);
    addUnit(ConverterCategory::Area, Area_SquareInch, "UnitName_SquareInch", "UnitAbbreviation_SquareInch", 6);
    addUnit(ConverterCategory::Area, Area_SquareKilometer, "UnitName_SquareKilometer", "UnitAbbreviation_SquareKilometer", 5);
    addUnit(ConverterCategory::Area, Area_SquareMeter, "UnitName_SquareMeter", "UnitAbbreviation_SquareMeter", 3, useUSCustomary, useSI);
    addUnit(ConverterCategory::Area, Area_SquareMile, "UnitName_SquareMile", "UnitAbbreviation_SquareMile", 10);
    addUnit(ConverterCategory::Area, Area_SquareMillimeter, "UnitName_SquareMillimeter", "UnitAbbreviation_SquareMillimeter", 1);
    addUnit(ConverterCategory::Area, Area_SquareYard, "UnitName_SquareYard", "UnitAbbreviation_SquareYard", 8);
    addUnit(ConverterCategory::Area, Area_Hand, "UnitName_Hand", "UnitAbbreviation_Hand", 11, false, false, true);
    addUnit(ConverterCategory::Area, Area_Paper, "UnitName_Paper", "UnitAbbreviation_Paper", 12, false, false, true);
    addUnit(ConverterCategory::Area, Area_SoccerField, "UnitName_SoccerField", "UnitAbbreviation_SoccerField", 13, false, false, true);
    addUnit(ConverterCategory::Area, Area_Castle, "UnitName_Castle", "UnitAbbreviation_Castle", 14, false, false, true);
    if (usePyeong)
    {
        addUnit(ConverterCategory::Area, Area_Pyeong, "UnitName_Pyeong", "UnitAbbreviation_Pyeong", 15);
    }

    // ---- Data ----
    addUnit(ConverterCategory::Data, Data_Bit, "UnitName_Bit", "UnitAbbreviation_Bit", 1);
    addUnit(ConverterCategory::Data, Data_Byte, "UnitName_Byte", "UnitAbbreviation_Byte", 3);
    addUnit(ConverterCategory::Data, Data_Exabits, "UnitName_Exabits", "UnitAbbreviation_Exabits", 24);
    addUnit(ConverterCategory::Data, Data_Exabytes, "UnitName_Exabytes", "UnitAbbreviation_Exabytes", 26);
    addUnit(ConverterCategory::Data, Data_Exbibits, "UnitName_Exbibits", "UnitAbbreviation_Exbibits", 25);
    addUnit(ConverterCategory::Data, Data_Exbibytes, "UnitName_Exbibytes", "UnitAbbreviation_Exbibytes", 27);
    addUnit(ConverterCategory::Data, Data_Gibibits, "UnitName_Gibibits", "UnitAbbreviation_Gibibits", 13);
    addUnit(ConverterCategory::Data, Data_Gibibytes, "UnitName_Gibibytes", "UnitAbbreviation_Gibibytes", 15);
    addUnit(ConverterCategory::Data, Data_Gigabit, "UnitName_Gigabit", "UnitAbbreviation_Gigabit", 12);
    addUnit(ConverterCategory::Data, Data_Gigabyte, "UnitName_Gigabyte", "UnitAbbreviation_Gigabyte", 14, true, false);
    addUnit(ConverterCategory::Data, Data_Kibibits, "UnitName_Kibibits", "UnitAbbreviation_Kibibits", 5);
    addUnit(ConverterCategory::Data, Data_Kibibytes, "UnitName_Kibibytes", "UnitAbbreviation_Kibibytes", 7);
    addUnit(ConverterCategory::Data, Data_Kilobit, "UnitName_Kilobit", "UnitAbbreviation_Kilobit", 4);
    addUnit(ConverterCategory::Data, Data_Kilobyte, "UnitName_Kilobyte", "UnitAbbreviation_Kilobyte", 6);
    addUnit(ConverterCategory::Data, Data_Mebibits, "UnitName_Mebibits", "UnitAbbreviation_Mebibits", 9);
    addUnit(ConverterCategory::Data, Data_Mebibytes, "UnitName_Mebibytes", "UnitAbbreviation_Mebibytes", 11);
    addUnit(ConverterCategory::Data, Data_Megabit, "UnitName_Megabit", "UnitAbbreviation_Megabit", 8);
    addUnit(ConverterCategory::Data, Data_Megabyte, "UnitName_Megabyte", "UnitAbbreviation_Megabyte", 10, false, true);
    addUnit(ConverterCategory::Data, Data_Nibble, "UnitName_Nibble", "UnitAbbreviation_Nibble", 2);
    addUnit(ConverterCategory::Data, Data_Pebibits, "UnitName_Pebibits", "UnitAbbreviation_Pebibits", 21);
    addUnit(ConverterCategory::Data, Data_Pebibytes, "UnitName_Pebibytes", "UnitAbbreviation_Pebibytes", 23);
    addUnit(ConverterCategory::Data, Data_Petabit, "UnitName_Petabit", "UnitAbbreviation_Petabit", 20);
    addUnit(ConverterCategory::Data, Data_Petabyte, "UnitName_Petabyte", "UnitAbbreviation_Petabyte", 22);
    addUnit(ConverterCategory::Data, Data_Tebibits, "UnitName_Tebibits", "UnitAbbreviation_Tebibits", 17);
    addUnit(ConverterCategory::Data, Data_Tebibytes, "UnitName_Tebibytes", "UnitAbbreviation_Tebibytes", 19);
    addUnit(ConverterCategory::Data, Data_Terabit, "UnitName_Terabit", "UnitAbbreviation_Terabit", 16);
    addUnit(ConverterCategory::Data, Data_Terabyte, "UnitName_Terabyte", "UnitAbbreviation_Terabyte", 18);
    addUnit(ConverterCategory::Data, Data_Yobibits, "UnitName_Yobibits", "UnitAbbreviation_Yobibits", 33);
    addUnit(ConverterCategory::Data, Data_Yobibytes, "UnitName_Yobibytes", "UnitAbbreviation_Yobibytes", 35);
    addUnit(ConverterCategory::Data, Data_Yottabit, "UnitName_Yottabit", "UnitAbbreviation_Yottabit", 32);
    addUnit(ConverterCategory::Data, Data_Yottabyte, "UnitName_Yottabyte", "UnitAbbreviation_Yottabyte", 34);
    addUnit(ConverterCategory::Data, Data_Zebibits, "UnitName_Zebibits", "UnitAbbreviation_Zebibits", 29);
    addUnit(ConverterCategory::Data, Data_Zebibytes, "UnitName_Zebibytes", "UnitAbbreviation_Zebibytes", 31);
    addUnit(ConverterCategory::Data, Data_Zetabits, "UnitName_Zetabits", "UnitAbbreviation_Zetabits", 28);
    addUnit(ConverterCategory::Data, Data_Zetabytes, "UnitName_Zetabytes", "UnitAbbreviation_Zetabytes", 30);
    addUnit(ConverterCategory::Data, Data_FloppyDisk, "UnitName_FloppyDisk", "UnitAbbreviation_FloppyDisk", 13, false, false, true);
    addUnit(ConverterCategory::Data, Data_CD, "UnitName_CD", "UnitAbbreviation_CD", 14, false, false, true);
    addUnit(ConverterCategory::Data, Data_DVD, "UnitName_DVD", "UnitAbbreviation_DVD", 15, false, false, true);

    // ---- Energy ----
    addUnit(ConverterCategory::Energy, Energy_BritishThermalUnit, "UnitName_BritishThermalUnit", "UnitAbbreviation_BritishThermalUnit", 7);
    addUnit(ConverterCategory::Energy, Energy_Calorie, "UnitName_Calorie", "UnitAbbreviation_Calorie", 4);
    addUnit(ConverterCategory::Energy, Energy_ElectronVolt, "UnitName_Electron-Volt", "UnitAbbreviation_Electron-Volt", 1);
    addUnit(ConverterCategory::Energy, Energy_FootPound, "UnitName_Foot-Pound", "UnitAbbreviation_Foot-Pound", 6);
    addUnit(ConverterCategory::Energy, Energy_Joule, "UnitName_Joule", "UnitAbbreviation_Joule", 2, true, false);
    addUnit(ConverterCategory::Energy, Energy_Kilowatthour, "UnitName_Kilowatthour", "UnitAbbreviation_Kilowatthour", 166, true, false);
    addUnit(ConverterCategory::Energy, Energy_Kilocalorie, "UnitName_Kilocalorie", "UnitAbbreviation_Kilocalorie", 5, false, true);
    addUnit(ConverterCategory::Energy, Energy_Kilojoule, "UnitName_Kilojoule", "UnitAbbreviation_Kilojoule", 3);
    addUnit(ConverterCategory::Energy, Energy_Battery, "UnitName_Battery", "UnitAbbreviation_Battery", 8, false, false, true);
    addUnit(ConverterCategory::Energy, Energy_Banana, "UnitName_Banana", "UnitAbbreviation_Banana", 9, false, false, true);
    addUnit(ConverterCategory::Energy, Energy_SliceOfCake, "UnitName_SliceOfCake", "UnitAbbreviation_SliceOfCake", 10, false, false, true);

    // ---- Length ----
    addUnit(ConverterCategory::Length, Length_Angstrom, "UnitName_Angstrom", "UnitAbbreviation_Angstrom", 1);
    addUnit(ConverterCategory::Length, Length_Centimeter, "UnitName_Centimeter", "UnitAbbreviation_Centimeter", 5, useUSCustomary, useSI);
    addUnit(ConverterCategory::Length, Length_Foot, "UnitName_Foot", "UnitAbbreviation_Foot", 9);
    addUnit(ConverterCategory::Length, Length_Inch, "UnitName_Inch", "UnitAbbreviation_Inch", 8, useSI, useUSCustomary);
    addUnit(ConverterCategory::Length, Length_Kilometer, "UnitName_Kilometer", "UnitAbbreviation_Kilometer", 7);
    addUnit(ConverterCategory::Length, Length_Meter, "UnitName_Meter", "UnitAbbreviation_Meter", 6);
    addUnit(ConverterCategory::Length, Length_Micron, "UnitName_Micron", "UnitAbbreviation_Micron", 3);
    addUnit(ConverterCategory::Length, Length_Mile, "UnitName_Mile", "UnitAbbreviation_Mile", 11);
    addUnit(ConverterCategory::Length, Length_Millimeter, "UnitName_Millimeter", "UnitAbbreviation_Millimeter", 4);
    addUnit(ConverterCategory::Length, Length_Nanometer, "UnitName_Nanometer", "UnitAbbreviation_Nanometer", 2);
    addUnit(ConverterCategory::Length, Length_NauticalMile, "UnitName_NauticalMile", "UnitAbbreviation_NauticalMile", 12);
    addUnit(ConverterCategory::Length, Length_Yard, "UnitName_Yard", "UnitAbbreviation_Yard", 10);
    addUnit(ConverterCategory::Length, Length_Paperclip, "UnitName_Paperclip", "UnitAbbreviation_Paperclip", 13, false, false, true);
    addUnit(ConverterCategory::Length, Length_Hand, "UnitName_Hand", "UnitAbbreviation_Hand", 14, false, false, true);
    addUnit(ConverterCategory::Length, Length_JumboJet, "UnitName_JumboJet", "UnitAbbreviation_JumboJet", 15, false, false, true);

    // ---- Power ----
    addUnit(ConverterCategory::Power, Power_BritishThermalUnitPerMinute, "UnitName_BTUPerMinute", "UnitAbbreviation_BTUPerMinute", 5);
    addUnit(ConverterCategory::Power, Power_FootPoundPerMinute, "UnitName_Foot-PoundPerMinute", "UnitAbbreviation_Foot-PoundPerMinute", 4);
    addUnit(ConverterCategory::Power, Power_Horsepower, "UnitName_Horsepower", "UnitAbbreviation_Horsepower", 3, false, true);
    addUnit(ConverterCategory::Power, Power_Kilowatt, "UnitName_Kilowatt", "UnitAbbreviation_Kilowatt", 2, !useWattInsteadOfKilowatt);
    addUnit(ConverterCategory::Power, Power_Watt, "UnitName_Watt", "UnitAbbreviation_Watt", 1, useWattInsteadOfKilowatt);
    addUnit(ConverterCategory::Power, Power_LightBulb, "UnitName_LightBulb", "UnitAbbreviation_LightBulb", 6, false, false, true);
    addUnit(ConverterCategory::Power, Power_Horse, "UnitName_Horse", "UnitAbbreviation_Horse", 7, false, false, true);
    addUnit(ConverterCategory::Power, Power_TrainEngine, "UnitName_TrainEngine", "UnitAbbreviation_TrainEngine", 8, false, false, true);

    // ---- Temperature ----
    addUnit(ConverterCategory::Temperature, Temperature_DegreesCelsius, "UnitName_DegreesCelsius", "UnitAbbreviation_DegreesCelsius", 1, useFahrenheit, !useFahrenheit);
    addUnit(ConverterCategory::Temperature, Temperature_DegreesFahrenheit, "UnitName_DegreesFahrenheit", "UnitAbbreviation_DegreesFahrenheit", 2, !useFahrenheit, useFahrenheit);
    addUnit(ConverterCategory::Temperature, Temperature_Kelvin, "UnitName_Kelvin", "UnitAbbreviation_Kelvin", 3);

    // ---- Time ----
    addUnit(ConverterCategory::Time, Time_Day, "UnitName_Day", "UnitAbbreviation_Day", 6);
    addUnit(ConverterCategory::Time, Time_Hour, "UnitName_Hour", "UnitAbbreviation_Hour", 5, true, false);
    addUnit(ConverterCategory::Time, Time_Microsecond, "UnitName_Microsecond", "UnitAbbreviation_Microsecond", 1);
    addUnit(ConverterCategory::Time, Time_Millisecond, "UnitName_Millisecond", "UnitAbbreviation_Millisecond", 2);
    addUnit(ConverterCategory::Time, Time_Minute, "UnitName_Minute", "UnitAbbreviation_Minute", 4, false, true);
    addUnit(ConverterCategory::Time, Time_Second, "UnitName_Second", "UnitAbbreviation_Second", 3);
    addUnit(ConverterCategory::Time, Time_Week, "UnitName_Week", "UnitAbbreviation_Week", 7);
    addUnit(ConverterCategory::Time, Time_Year, "UnitName_Year", "UnitAbbreviation_Year", 8);

    // ---- Speed ----
    addUnit(ConverterCategory::Speed, Speed_CentimetersPerSecond, "UnitName_CentimetersPerSecond", "UnitAbbreviation_CentimetersPerSecond", 1);
    addUnit(ConverterCategory::Speed, Speed_FeetPerSecond, "UnitName_FeetPerSecond", "UnitAbbreviation_FeetPerSecond", 4);
    addUnit(ConverterCategory::Speed, Speed_KilometersPerHour, "UnitName_KilometersPerHour", "UnitAbbreviation_KilometersPerHour", 3, useUSCustomary, useSI);
    addUnit(ConverterCategory::Speed, Speed_Knot, "UnitName_Knot", "UnitAbbreviation_Knot", 6);
    addUnit(ConverterCategory::Speed, Speed_Mach, "UnitName_Mach", "UnitAbbreviation_Mach", 7);
    addUnit(ConverterCategory::Speed, Speed_MetersPerSecond, "UnitName_MetersPerSecond", "UnitAbbreviation_MetersPerSecond", 2);
    addUnit(ConverterCategory::Speed, Speed_MilesPerHour, "UnitName_MilesPerHour", "UnitAbbreviation_MilesPerHour", 5, useSI, useUSCustomary);
    addUnit(ConverterCategory::Speed, Speed_Turtle, "UnitName_Turtle", "UnitAbbreviation_Turtle", 8, false, false, true);
    addUnit(ConverterCategory::Speed, Speed_Horse, "UnitName_Horse", "UnitAbbreviation_Horse", 9, false, false, true);
    addUnit(ConverterCategory::Speed, Speed_Jet, "UnitName_Jet", "UnitAbbreviation_Jet", 10, false, false, true);

    // ---- Volume ----
    addUnit(ConverterCategory::Volume, Volume_CubicCentimeter, "UnitName_CubicCentimeter", "UnitAbbreviation_CubicCentimeter", 2);
    addUnit(ConverterCategory::Volume, Volume_CubicFoot, "UnitName_CubicFoot", "UnitAbbreviation_CubicFoot", 13);
    addUnit(ConverterCategory::Volume, Volume_CubicInch, "UnitName_CubicInch", "UnitAbbreviation_CubicInch", 12);
    addUnit(ConverterCategory::Volume, Volume_CubicMeter, "UnitName_CubicMeter", "UnitAbbreviation_CubicMeter", 4);
    addUnit(ConverterCategory::Volume, Volume_CubicYard, "UnitName_CubicYard", "UnitAbbreviation_CubicYard", 14);
    addUnit(ConverterCategory::Volume, Volume_CupUS, "UnitName_CupUS", "UnitAbbreviation_CupUS", 8);
    addUnit(ConverterCategory::Volume, Volume_FluidOunceUK, "UnitName_FluidOunceUK", "UnitAbbreviation_FluidOunceUK", 17);
    addUnit(ConverterCategory::Volume, Volume_FluidOunceUS, "UnitName_FluidOunceUS", "UnitAbbreviation_FluidOunceUS", 7);
    addUnit(ConverterCategory::Volume, Volume_GallonUK, "UnitName_GallonUK", "UnitAbbreviation_GallonUK", 20);
    addUnit(ConverterCategory::Volume, Volume_GallonUS, "UnitName_GallonUS", "UnitAbbreviation_GallonUS", 11);
    addUnit(ConverterCategory::Volume, Volume_Liter, "UnitName_Liter", "UnitAbbreviation_Liter", 3);
    addUnit(ConverterCategory::Volume, Volume_Milliliter, "UnitName_Milliliter", "UnitAbbreviation_Milliliter", 1, useUSCustomary, useSI);
    addUnit(ConverterCategory::Volume, Volume_PintUK, "UnitName_PintUK", "UnitAbbreviation_PintUK", 18);
    addUnit(ConverterCategory::Volume, Volume_PintUS, "UnitName_PintUS", "UnitAbbreviation_PintUS", 9);
    addUnit(ConverterCategory::Volume, Volume_TablespoonUS, "UnitName_TablespoonUS", "UnitAbbreviation_TablespoonUS", 6);
    addUnit(ConverterCategory::Volume, Volume_TeaspoonUS, "UnitName_TeaspoonUS", "UnitAbbreviation_TeaspoonUS", 5, useSI, useUSCustomary && region != u"GB");
    addUnit(ConverterCategory::Volume, Volume_QuartUK, "UnitName_QuartUK", "UnitAbbreviation_QuartUK", 19);
    addUnit(ConverterCategory::Volume, Volume_QuartUS, "UnitName_QuartUS", "UnitAbbreviation_QuartUS", 10);
    addUnit(ConverterCategory::Volume, Volume_TeaspoonUK, "UnitName_TeaspoonUK", "UnitAbbreviation_TeaspoonUK", 15, false, useUSCustomary && region == u"GB");
    addUnit(ConverterCategory::Volume, Volume_TablespoonUK, "UnitName_TablespoonUK", "UnitAbbreviation_TablespoonUK", 16);
    addUnit(ConverterCategory::Volume, Volume_CoffeeCup, "UnitName_CoffeeCup", "UnitAbbreviation_CoffeeCup", 22, false, false, true);
    addUnit(ConverterCategory::Volume, Volume_Bathtub, "UnitName_Bathtub", "UnitAbbreviation_Bathtub", 23, false, false, true);
    addUnit(ConverterCategory::Volume, Volume_SwimmingPool, "UnitName_SwimmingPool", "UnitAbbreviation_SwimmingPool", 24, false, false, true);

    // ---- Weight ----
    addUnit(ConverterCategory::Weight, Weight_Carat, "UnitName_Carat", "UnitAbbreviation_Carat", 1);
    addUnit(ConverterCategory::Weight, Weight_Centigram, "UnitName_Centigram", "UnitAbbreviation_Centigram", 3);
    addUnit(ConverterCategory::Weight, Weight_Decigram, "UnitName_Decigram", "UnitAbbreviation_Decigram", 4);
    addUnit(ConverterCategory::Weight, Weight_Decagram, "UnitName_Decagram", "UnitAbbreviation_Decagram", 6);
    addUnit(ConverterCategory::Weight, Weight_Gram, "UnitName_Gram", "UnitAbbreviation_Gram", 5);
    addUnit(ConverterCategory::Weight, Weight_Hectogram, "UnitName_Hectogram", "UnitAbbreviation_Hectogram", 7);
    addUnit(ConverterCategory::Weight, Weight_Kilogram, "UnitName_Kilogram", "UnitAbbreviation_Kilogram", 8, useUSCustomary, useSI);
    addUnit(ConverterCategory::Weight, Weight_LongTon, "UnitName_LongTon", "UnitAbbreviation_LongTon", 14);
    addUnit(ConverterCategory::Weight, Weight_Milligram, "UnitName_Milligram", "UnitAbbreviation_Milligram", 2);
    addUnit(ConverterCategory::Weight, Weight_Ounce, "UnitName_Ounce", "UnitAbbreviation_Ounce", 10);
    addUnit(ConverterCategory::Weight, Weight_Pound, "UnitName_Pound", "UnitAbbreviation_Pound", 11, useSI, useUSCustomary);
    addUnit(ConverterCategory::Weight, Weight_ShortTon, "UnitName_ShortTon", "UnitAbbreviation_ShortTon", 13);
    addUnit(ConverterCategory::Weight, Weight_Stone, "UnitName_Stone", "UnitAbbreviation_Stone", 12);
    addUnit(ConverterCategory::Weight, Weight_Tonne, "UnitName_Tonne", "UnitAbbreviation_Tonne", 9);
    addUnit(ConverterCategory::Weight, Weight_Snowflake, "UnitName_Snowflake", "UnitAbbreviation_Snowflake", 15, false, false, true);
    addUnit(ConverterCategory::Weight, Weight_SoccerBall, "UnitName_SoccerBall", "UnitAbbreviation_SoccerBall", 16, false, false, true);
    addUnit(ConverterCategory::Weight, Weight_Elephant, "UnitName_Elephant", "UnitAbbreviation_Elephant", 17, false, false, true);
    addUnit(ConverterCategory::Weight, Weight_Whale, "UnitName_Whale", "UnitAbbreviation_Whale", 18, false, false, true);

    // ---- Pressure ----
    addUnit(ConverterCategory::Pressure, Pressure_Atmosphere, "UnitName_Atmosphere", "UnitAbbreviation_Atmosphere", 1, true, false);
    addUnit(ConverterCategory::Pressure, Pressure_Bar, "UnitName_Bar", "UnitAbbreviation_Bar", 2, false, true);
    addUnit(ConverterCategory::Pressure, Pressure_KiloPascal, "UnitName_KiloPascal", "UnitAbbreviation_KiloPascal", 3);
    addUnit(ConverterCategory::Pressure, Pressure_MillimeterOfMercury, "UnitName_MillimeterOfMercury ", "UnitAbbreviation_MillimeterOfMercury ", 4);
    addUnit(ConverterCategory::Pressure, Pressure_Pascal, "UnitName_Pascal", "UnitAbbreviation_Pascal", 5);
    addUnit(ConverterCategory::Pressure, Pressure_PSI, "UnitName_PSI", "UnitAbbreviation_PSI", 6);

    // ---- Angle ----
    addUnit(ConverterCategory::Angle, Angle_Degree, "UnitName_Degree", "UnitAbbreviation_Degree", 1, true, false);
    addUnit(ConverterCategory::Angle, Angle_Radian, "UnitName_Radian", "UnitAbbreviation_Radian", 2, false, true);
    addUnit(ConverterCategory::Angle, Angle_Gradian, "UnitName_Gradian", "UnitAbbreviation_Gradian", 3);

    for (auto& [categoryId, entries] : unitsWithOrder)
    {
        std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
        auto& units = m_unitsByCategory[categoryId];
        for (const auto& entry : entries)
        {
            units.push_back(entry.first);
        }
    }

    // Conversion factors to a per-category base unit; ratio = source / target.
    const std::map<ConverterCategory, std::map<int, double>> conversionFactors = {
        { ConverterCategory::Area,
            {
            { Area_Acre, 4046.8564224 },
            { Area_SquareMeter, 1.0 },
            { Area_SquareFoot, 0.09290304 },
            { Area_SquareYard, 0.83612736 },
            { Area_SquareMillimeter, 0.000001 },
            { Area_SquareCentimeter, 0.0001 },
            { Area_SquareInch, 0.00064516 },
            { Area_SquareMile, 2589988.110336 },
            { Area_SquareKilometer, 1000000.0 },
            { Area_Hectare, 10000.0 },
            { Area_Hand, 0.012516104 },
            { Area_Paper, 0.06032246 },
            { Area_SoccerField, 10869.66 },
            { Area_Castle, 100000.0 },
            { Area_Pyeong, 400.0 / 121.0 },
            } },
        { ConverterCategory::Data,
            {
            { Data_Bit, 0.000000125 },
            { Data_Nibble, 0.0000005 },
            { Data_Byte, 0.000001 },
            { Data_Kilobyte, 0.001 },
            { Data_Megabyte, 1.0 },
            { Data_Gigabyte, 1000.0 },
            { Data_Terabyte, 1000000.0 },
            { Data_Petabyte, 1000000000.0 },
            { Data_Exabytes, 1000000000000.0 },
            { Data_Zetabytes, 1000000000000000.0 },
            { Data_Yottabyte, 1000000000000000000.0 },
            { Data_Kilobit, 0.000125 },
            { Data_Megabit, 0.125 },
            { Data_Gigabit, 125.0 },
            { Data_Terabit, 125000.0 },
            { Data_Petabit, 125000000.0 },
            { Data_Exabits, 125000000000.0 },
            { Data_Zetabits, 125000000000000.0 },
            { Data_Yottabit, 125000000000000000.0 },
            { Data_Gibibits, 134.217728 },
            { Data_Gibibytes, 1073.741824 },
            { Data_Kibibits, 0.000128 },
            { Data_Kibibytes, 0.001024 },
            { Data_Mebibits, 0.131072 },
            { Data_Mebibytes, 1.048576 },
            { Data_Pebibits, 140737488.355328 },
            { Data_Pebibytes, 1125899906.842624 },
            { Data_Tebibits, 137438.953472 },
            { Data_Tebibytes, 1099511.627776 },
            { Data_Exbibits, 144115188075.855872 },
            { Data_Exbibytes, 1152921504606.846976 },
            { Data_Zebibits, 147573952589676.412928 },
            { Data_Zebibytes, 1180591620717411.303424 },
            { Data_Yobibits, 151115727451828646.838272 },
            { Data_Yobibytes, 1208925819614629174.706176 },
            { Data_FloppyDisk, 1.474560 },
            { Data_CD, 700.0 },
            { Data_DVD, 4700.0 },
            } },
        { ConverterCategory::Energy,
            {
            { Energy_Calorie, 4.184 },
            { Energy_Kilocalorie, 4184.0 },
            { Energy_BritishThermalUnit, 1055.056 },
            { Energy_Kilojoule, 1000.0 },
            { Energy_Kilowatthour, 3600000.0 },
            { Energy_ElectronVolt, 0.0000000000000000001602176565 },
            { Energy_Joule, 1.0 },
            { Energy_FootPound, 1.3558179483314 },
            { Energy_Battery, 9000.0 },
            { Energy_Banana, 439614.0 },
            { Energy_SliceOfCake, 1046700.0 },
            } },
        { ConverterCategory::Length,
            {
            { Length_Inch, 0.0254 },
            { Length_Foot, 0.3048 },
            { Length_Yard, 0.9144 },
            { Length_Mile, 1609.344 },
            { Length_Micron, 0.000001 },
            { Length_Millimeter, 0.001 },
            { Length_Nanometer, 0.000000001 },
            { Length_Angstrom, 0.0000000001 },
            { Length_Centimeter, 0.01 },
            { Length_Meter, 1.0 },
            { Length_Kilometer, 1000.0 },
            { Length_NauticalMile, 1852.0 },
            { Length_Paperclip, 0.035052 },
            { Length_Hand, 0.18669 },
            { Length_JumboJet, 76.0 },
            } },
        { ConverterCategory::Power,
            {
            { Power_BritishThermalUnitPerMinute, 17.58426666666667 },
            { Power_FootPoundPerMinute, 0.0225969658055233 },
            { Power_Watt, 1.0 },
            { Power_Kilowatt, 1000.0 },
            { Power_Horsepower, 745.69987158227022 },
            { Power_LightBulb, 60.0 },
            { Power_Horse, 745.7 },
            { Power_TrainEngine, 2982799.486329081 },
            } },
        { ConverterCategory::Time,
            {
            { Time_Day, 86400.0 },
            { Time_Second, 1.0 },
            { Time_Week, 604800.0 },
            { Time_Year, 31557600.0 },
            { Time_Millisecond, 0.001 },
            { Time_Microsecond, 0.000001 },
            { Time_Minute, 60.0 },
            { Time_Hour, 3600.0 },
            } },
        { ConverterCategory::Volume,
            {
            { Volume_CupUS, 236.588237 },
            { Volume_PintUS, 473.176473 },
            { Volume_PintUK, 568.26125 },
            { Volume_QuartUS, 946.352946 },
            { Volume_QuartUK, 1136.5225 },
            { Volume_GallonUS, 3785.411784 },
            { Volume_GallonUK, 4546.09 },
            { Volume_Liter, 1000.0 },
            { Volume_TeaspoonUS, 4.92892159375 },
            { Volume_TablespoonUS, 14.78676478125 },
            { Volume_CubicCentimeter, 1.0 },
            { Volume_CubicYard, 764554.857984 },
            { Volume_CubicMeter, 1000000.0 },
            { Volume_Milliliter, 1.0 },
            { Volume_CubicInch, 16.387064 },
            { Volume_CubicFoot, 28316.846592 },
            { Volume_FluidOunceUS, 29.5735295625 },
            { Volume_FluidOunceUK, 28.4130625 },
            { Volume_TeaspoonUK, 5.91938802083333333333 },
            { Volume_TablespoonUK, 17.7581640625 },
            { Volume_CoffeeCup, 236.5882 },
            { Volume_Bathtub, 378541.2 },
            { Volume_SwimmingPool, 3750000000.0 },
            } },
        { ConverterCategory::Weight,
            {
            { Weight_Kilogram, 1.0 },
            { Weight_Hectogram, 0.1 },
            { Weight_Decagram, 0.01 },
            { Weight_Gram, 0.001 },
            { Weight_Pound, 0.45359237 },
            { Weight_Ounce, 0.028349523125 },
            { Weight_Milligram, 0.000001 },
            { Weight_Centigram, 0.00001 },
            { Weight_Decigram, 0.0001 },
            { Weight_LongTon, 1016.0469088 },
            { Weight_Tonne, 1000.0 },
            { Weight_Stone, 6.35029318 },
            { Weight_Carat, 0.0002 },
            { Weight_ShortTon, 907.18474 },
            { Weight_Snowflake, 0.000002 },
            { Weight_SoccerBall, 0.4325 },
            { Weight_Elephant, 4000.0 },
            { Weight_Whale, 90000.0 },
            } },
        { ConverterCategory::Speed,
            {
            { Speed_CentimetersPerSecond, 1.0 },
            { Speed_FeetPerSecond, 30.48 },
            { Speed_KilometersPerHour, 27.777777777777777777778 },
            { Speed_Knot, 51.44 },
            { Speed_Mach, 34030.0 },
            { Speed_MetersPerSecond, 100.0 },
            { Speed_MilesPerHour, 44.7 },
            { Speed_Turtle, 8.94 },
            { Speed_Horse, 2011.5 },
            { Speed_Jet, 24585.0 },
            } },
        { ConverterCategory::Angle,
            {
            { Angle_Degree, 1.0 },
            { Angle_Radian, 57.29577951308233 },
            { Angle_Gradian, 0.9 },
            } },
        { ConverterCategory::Pressure,
            {
            { Pressure_Atmosphere, 1.0 },
            { Pressure_Bar, 0.9869232667160128 },
            { Pressure_KiloPascal, 0.0098692326671601 },
            { Pressure_MillimeterOfMercury, 0.0013155687145324 },
            { Pressure_Pascal, 9.869232667160128e-6 },
            { Pressure_PSI, 0.068045961016531 },
            } },
    };

    // Temperature needs offsets, so its conversions are listed explicitly.
    const std::map<int, std::map<int, ConversionData>> explicitConversions = {
        { Temperature_DegreesCelsius,
            {
                { Temperature_DegreesCelsius, { 1, 0, false } },
                { Temperature_DegreesFahrenheit, { 1.8, 32, false } },
                { Temperature_Kelvin, { 1, 273.15, false } },
            } },
        { Temperature_DegreesFahrenheit,
            {
                { Temperature_DegreesCelsius, { 0.55555555555555555555555555555556, -32, true } },
                { Temperature_DegreesFahrenheit, { 1, 0, false } },
                { Temperature_Kelvin, { 0.55555555555555555555555555555556, 459.67, true } },
            } },
        { Temperature_Kelvin,
            {
                { Temperature_DegreesCelsius, { 1, -273.15, true } },
                { Temperature_DegreesFahrenheit, { 1.8, -459.67, false } },
                { Temperature_Kelvin, { 1, 0, false } },
            } },
    };

    for (const auto& [categoryId, units] : m_unitsByCategory)
    {
        const auto factorsIt = conversionFactors.find(static_cast<ConverterCategory>(categoryId));
        for (const Unit& source : units)
        {
            auto& ratios = m_ratiosByUnit[source.id];
            if (auto explicitIt = explicitConversions.find(source.id); explicitIt != explicitConversions.end())
            {
                for (const Unit& target : units)
                {
                    if (auto it = explicitIt->second.find(target.id); it != explicitIt->second.end())
                    {
                        ratios[target] = it->second;
                    }
                }
                continue;
            }

            if (factorsIt == conversionFactors.end())
            {
                continue;
            }
            const auto& factors = factorsIt->second;
            const auto sourceFactor = factors.find(source.id);
            if (sourceFactor == factors.end())
            {
                continue;
            }
            for (const Unit& target : units)
            {
                if (auto targetFactor = factors.find(target.id); targetFactor != factors.end() && targetFactor->second > 0)
                {
                    ratios[target] = ConversionData(sourceFactor->second / targetFactor->second, 0, false);
                }
            }
        }
    }
}

std::vector<Category> UnitConverterDataLoader::GetOrderedCategories()
{
    return m_categories;
}

std::vector<Unit> UnitConverterDataLoader::GetOrderedUnits(const Category& category)
{
    if (auto it = m_unitsByCategory.find(category.id); it != m_unitsByCategory.end())
    {
        return it->second;
    }
    return {};
}

std::unordered_map<Unit, ConversionData, UnitHash> UnitConverterDataLoader::LoadOrderedRatios(const Unit& unit)
{
    if (auto it = m_ratiosByUnit.find(unit.id); it != m_ratiosByUnit.end())
    {
        return it->second;
    }
    return {};
}

bool UnitConverterDataLoader::SupportsCategory(const Category& target)
{
    return target.id != static_cast<int>(ConverterCategory::Currency);
}
