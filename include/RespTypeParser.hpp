#pragma once

#include "RespType.hpp"
#include <istream>

RespString parseRespString(std::istream &in_stream);
RespInt parseRespInt(std::istream &in_stream);
RespBulkString parseRespBulkString(std::istream &in_stream,
                                   bool parseLastCrlf = true);
RespError parseRespError(std::istream &in_stream);
RespArray parseRespArray(std::istream &in_stream);

RespValue parseRespValue(std::istream &in_stream);
