// Copyright 2022 Accenture.

#include "busid/BusId.h"

namespace common
{
namespace busid
{
// clang-format off
#define BUS_ID_NAME(BUS) \
    case ::busid::BUS: return #BUS

// clang-format on

char const* BusIdTraits::getName(uint8_t index)
{
    switch (index)
    {
        BUS_ID_NAME(SELFDIAG);
        BUS_ID_NAME(CAN_0);
        default: return "INVALID";
    }
}

#define NETWORK_INTERFACE_ID_VLANID(BUS, VLANID) \
    case ::busid::BUS::value: return ::busid::VlanIdType(VLANID)

} // namespace busid
} // namespace common
