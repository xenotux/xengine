/**
 *  xEngine - C++ Game Engine Library
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

#include "xng/render/graph/passes/compositepass.hpp"

#include "xng/render/graph/framegraphbuilder.hpp"
#include "xng/render/graph/framegraphsettings.hpp"

#include "xng/render/geometry/vertexstream.hpp"

#include "graph/compositepass_vs.hpp" // Generated by cmake
#include "graph/compositepass_fs.hpp" // Generated by cmake

namespace xng {
    CompositePass::CompositePass() = default;

    void CompositePass::setup(FrameGraphBuilder &builder) {
        RenderTargetDesc rdesc;
        rdesc.size = builder.getBackBufferDescription().size
                     * builder.getSettings().get<float>(FrameGraphSettings::SETTING_RENDER_SCALE);
        rdesc.hasDepthStencilAttachment = true;
        rdesc.numberOfColorAttachments = 1;
        target = builder.createRenderTarget(rdesc);
        blitTarget = builder.createRenderTarget(rdesc);

        builder.read(target);
        builder.read(blitTarget);

        if (!vertexBuffer.assigned) {
            VertexBufferDesc desc;
            desc.size = mesh.vertices.size() * mesh.vertexLayout.getSize();
            vertexBuffer = builder.createVertexBuffer(desc);

            VertexArrayObjectDesc oDesc;
            oDesc.vertexLayout = mesh.vertexLayout;
            vertexArrayObject = builder.createVertexArrayObject(oDesc);

            builder.write(vertexBuffer);
        }
        builder.persist(vertexBuffer);
        builder.persist(vertexArrayObject);

        builder.read(vertexBuffer);
        builder.read(vertexArrayObject);

        if (!blendPipeline.assigned) {
            RenderPipelineDesc pdesc{};
            pdesc.shaders = {
                    {VERTEX,   compositepass_vs},
                    {FRAGMENT, compositepass_fs}
            };
            pdesc.bindings = {
                    BIND_TEXTURE_BUFFER,
                    BIND_TEXTURE_BUFFER
            };
            pdesc.primitive = TRIANGLES;
            pdesc.vertexLayout = mesh.vertexLayout;
            pdesc.enableBlending = true;
            pdesc.enableDepthTest = true;
            pdesc.depthTestWrite = true;
            //https://stackoverflow.com/a/16938711
            pdesc.colorBlendSourceMode = SRC_ALPHA;
            pdesc.colorBlendDestinationMode = ONE_MINUS_SRC_ALPHA;
            pdesc.alphaBlendSourceMode = ONE;
            pdesc.alphaBlendDestinationMode = ONE_MINUS_SRC_ALPHA;
            blendPipeline = builder.createRenderPipeline(pdesc);
        }

        builder.persist(blendPipeline);
        builder.read(blendPipeline);

        RenderPassDesc passDesc;
        passDesc.numberOfColorAttachments = 1;
        passDesc.hasDepthStencilAttachment = true;
        pass = builder.createRenderPass(passDesc);

        builder.read(pass);

        screenColor = builder.getSlot(SLOT_SCREEN_COLOR);
        screenDepth = builder.getSlot(SLOT_SCREEN_DEPTH);

        deferredColor = builder.getSlot(SLOT_DEFERRED_COLOR);
        deferredDepth = builder.getSlot(SLOT_DEFERRED_DEPTH);

        forwardColor = builder.getSlot(SLOT_FORWARD_COLOR);
        forwardDepth = builder.getSlot(SLOT_FORWARD_DEPTH);

        backgroundColor = builder.getSlot(SLOT_BACKGROUND_COLOR);

        builder.write(screenColor);
        builder.write(screenDepth);
        builder.read(deferredColor);
        builder.read(deferredDepth);
        builder.read(forwardColor);
        builder.read(forwardDepth);
        builder.read(backgroundColor);

        commandBuffer = builder.createCommandBuffer();
        builder.write(commandBuffer);
    }

    void CompositePass::execute(FrameGraphPassResources &resources,
                                const std::vector<std::reference_wrapper<CommandQueue>> &renderQueues,
                                const std::vector<std::reference_wrapper<CommandQueue>> &computeQueues,
                                const std::vector<std::reference_wrapper<CommandQueue>> &transferQueues) {
        auto &t = resources.get<RenderTarget>(target);
        auto &bt = resources.get<RenderTarget>(blitTarget);

        auto &pip = resources.get<RenderPipeline>(blendPipeline);
        auto &p = resources.get<RenderPass>(pass);

        auto &vb = resources.get<VertexBuffer>(vertexBuffer);
        auto &vao = resources.get<VertexArrayObject>(vertexArrayObject);

        auto &sColor = resources.get<TextureBuffer>(screenColor);
        auto &sDepth = resources.get<TextureBuffer>(screenDepth);

        auto &dColor = resources.get<TextureBuffer>(deferredColor);
        auto &dDepth = resources.get<TextureBuffer>(deferredDepth);

        auto &fColor = resources.get<TextureBuffer>(forwardColor);
        auto &fDepth = resources.get<TextureBuffer>(forwardDepth);

        auto &bColor = resources.get<TextureBuffer>(backgroundColor);

        auto &cBuffer = resources.get<CommandBuffer>(commandBuffer);

        if (!quadAllocated) {
            quadAllocated = true;
            auto verts = VertexStream().addVertices(mesh.vertices).getVertexBuffer();
            vb.upload(0,
                      verts.data(),
                      verts.size());
            vao.setBuffers(vb);
        }

        t.setAttachments({RenderTargetAttachment::texture(sColor)}, RenderTargetAttachment::texture(sDepth));
        bt.setAttachments({RenderTargetAttachment::texture(bColor)}, RenderTargetAttachment::texture(dDepth));

        assert(t.isComplete());
        assert(bt.isComplete());

        std::vector<Command> commands;

        commands.emplace_back(t.blitColor(bt,
                                          {},
                                          {},
                                          bt.getDescription().size,
                                          t.getDescription().size,
                                          NEAREST,
                                          0,
                                          0));

        commands.emplace_back(p.begin(t));

        commands.emplace_back(pip.bind());
        commands.emplace_back(vao.bind());

        commands.emplace_back(RenderPipeline::bindShaderResources({
                                                                          {dColor, {
                                                                                           {FRAGMENT, ShaderResource::READ}
                                                                                   }},
                                                                          {dDepth, {
                                                                                           {FRAGMENT, ShaderResource::READ}
                                                                                   }},
                                                                  }));
        commands.emplace_back(p.drawArray(DrawCall(0, mesh.vertices.size())));

        commands.emplace_back(RenderPipeline::bindShaderResources({
                                                                          {fColor, {
                                                                                           {FRAGMENT, ShaderResource::READ}
                                                                                   }},
                                                                          {fDepth, {
                                                                                           {FRAGMENT, ShaderResource::READ}
                                                                                   }},
                                                                  }));
        commands.emplace_back(p.drawArray(DrawCall(0, mesh.vertices.size())));
        commands.emplace_back(p.end());

        cBuffer.begin();
        cBuffer.add(commands);
        cBuffer.end();

        renderQueues.at(0).get().submit({cBuffer}, {}, {});

        bt.clearAttachments();
        t.clearAttachments();
    }

    std::type_index CompositePass::getTypeIndex() const {
        return typeid(CompositePass);
    }
}