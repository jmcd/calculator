// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ReswStrings.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>

#include <clocale>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // QApplication applies the user's C locale. The engines format and parse
    // numbers with the C library ("1.5"), so keep numeric conversions in "C";
    // the UI formats numbers itself with QLocale.
    std::setlocale(LC_NUMERIC, "C");

    QApplication::setOrganizationName(QStringLiteral("calculator-kde"));
    QApplication::setApplicationName(QStringLiteral("calculator-kde"));
    QApplication::setApplicationVersion(QStringLiteral(CALCULATOR_VERSION));
    QApplication::setDesktopFileName(QStringLiteral(CALCULATOR_APP_ID));
    QApplication::setApplicationDisplayName(ReswStrings::instance()->get(QStringLiteral("AppName"), QStringLiteral("Calculator")));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("accessories-calculator")));

    // Use the Plasma style when it's available, so controls follow the user's
    // KDE theme and colour scheme. Respect an explicit override.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
    {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
        QQuickStyle::setFallbackStyle(QStringLiteral("Fusion"));
    }
    if (QIcon::themeName().isEmpty())
    {
        QIcon::setThemeName(QStringLiteral("breeze"));
    }
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    // Kirigami Addons (used by the date calculator) calls i18n() functions, which
    // KDE apps get from KI18n's localized context. Install it when it's available;
    // this only needs KI18n at runtime, not at build time.
    QQmlComponent i18nContext(&engine);
    i18nContext.setData("import org.kde.ki18n\nKLocalizedQmlContext {}", QUrl());
    if (QObject* context = i18nContext.create())
    {
        context->setParent(&engine);
        engine.rootContext()->setContextObject(context);
    }

    engine.loadFromModule("Calculator", "Main");

    return QApplication::exec();
}
