
#pragma once

#include "CoreMinimal.h"

#include <Python.h>
#include <structmember.h>

UNREALPYTHON_API DECLARE_LOG_CATEGORY_EXTERN(LogUnrealPython, Log, All);

#ifdef WITH_UPY_DOC_STRINGS
#define UPyDoc_STR(str) str
#else
#define UPyDoc_STR(str) nullptr
#endif
