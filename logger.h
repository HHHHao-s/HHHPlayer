#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <QDebug>

#define LOG_INFO(...) qInfo("[INFO]%s:%d - ", __FILE__, __LINE__); qInfo(__VA_ARGS__);qInfo("\n")
#define LOG_ERROR(...) qFatal("[ERROR]%s:%d - ", __FILE__, __LINE__); qFatal(__VA_ARGS__);qFatal("\n")

