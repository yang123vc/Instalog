#define _VARIADIC_MAX 10
#define _SCL_SECURE_NO_WARNINGS
#include <cstdlib>
#include <cstring>
#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <iterator>
#include <functional>

#define NOMINMAX
#define NTDDI_VERSION 0x05010200
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/algorithm/string.hpp>
