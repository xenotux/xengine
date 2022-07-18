/**
 *  This file is part of xEngine, a C++ game engine library.
 *  Copyright (C) 2021  Julian Zampiccoli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef XENGINE_GPUDRIVER_HPP
#define XENGINE_GPUDRIVER_HPP

#include "driver/driver.hpp"

#include "gpu/renderdevice.hpp"
#include "gpu/renderdeviceinfo.hpp"

namespace xng {
    class XENGINE_EXPORT GpuDriver : public Driver {
    public:
        ~GpuDriver() override = default;

        /**
         * @return Return the information objects of the available render devices.
         */
        virtual const std::vector<RenderDeviceInfo> &getAvailableRenderDevices() = 0;

        virtual std::unique_ptr<RenderDevice> createRenderDevice() = 0;

        virtual std::unique_ptr<RenderDevice> createRenderDevice(const std::string &deviceName) = 0;

    private:
        std::type_index getBaseType() override {
            return typeid(GpuDriver);
        }
    };
}
#endif //XENGINE_GPUDRIVER_HPP
