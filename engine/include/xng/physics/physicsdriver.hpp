/**
 *  This file is part of xEngine, a C++ game engine library.
 *  Copyright (C) 2023  Julian Zampiccoli
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

#ifndef XENGINE_PHYSICSDRIVER_HPP
#define XENGINE_PHYSICSDRIVER_HPP

#include <memory>

#include "xng/driver/driver.hpp"

#include "world.hpp"

namespace xng {
    enum PhysicsDriverBackend {
        BOX2D,
        BULLET3
    };

    class XENGINE_EXPORT PhysicsDriver : public Driver {
    public:
        static std::unique_ptr<PhysicsDriver> load(PhysicsDriverBackend backend) {
            switch (backend) {
                case BOX2D:
                    return std::unique_ptr<PhysicsDriver>(
                            dynamic_cast<PhysicsDriver *>(Driver::load("box2d").release()));
                case BULLET3:
                    return std::unique_ptr<PhysicsDriver>(
                            dynamic_cast<PhysicsDriver *>(Driver::load("bullet3").release()));
            }
            throw std::runtime_error("Invalid backend");
        }

        virtual std::unique_ptr<World> createWorld() = 0;

    private:
        std::type_index getBaseType() override {
            return typeid(PhysicsDriver);
        }
    };
}

#endif //XENGINE_PHYSICSDRIVER_HPP
