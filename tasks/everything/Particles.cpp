#include "Particles.h"

void Particles::init(spg::App&, const std::vector<Emitter>& emitters,
    vk::Format colorFormat, vk::Format depthFormat)
{
  std::vector<uint32_t> freeSlots(maxParticles, 0);
  for (uint32_t i = 0; i < maxParticles; i++) freeSlots[i] = i;

  std::vector<Particle> particles(maxParticles);
  gpuParticles.name("particles")
    // .memGpuOnly()
    .initAndCopy(particles);

  sortedIndices.name("particleIndices")
    // .memGpuOnly()
    .initAndCopy(freeSlots);

  freeParticles.name("freeParticles")
    // .memGpuOnly()
    .initAndCopy(freeSlots);

  gpuEmitters.name("emitters")
    // .memGpuOnly()
    .initAndCopy(emitters);

  particleIterator.programName("particleIterator")
    .shaderPath(EVERYTHING_SHADERS_ROOT "particle_iter.comp.spv")
    .init();

  emitterIterator.programName("emitterIterator")
    .shaderPath(EVERYTHING_SHADERS_ROOT "emitter_iter.comp.spv")
    .init();

  depthSorter.programName("depthSorter")
    .shaderPath(EVERYTHING_SHADERS_ROOT "particle_depth_sorter.comp.spv")
    .init();

  renderShader.programName("particleRenderer")
    .fragmentPath(EVERYTHING_SHADERS_ROOT "particles.frag.spv")
    .vertexPath(EVERYTHING_SHADERS_ROOT "particles.vert.spv")
    .alphaBlend(true)
    .addColorAttachment(colorFormat)
    .depthOutputFormat(depthFormat)
    .depthWrite(false)
    .init();

  freeParticleCount.name("freeParticleCount")
    // .memGpuOnly()
    .initAndCopy(std::vector<uint32_t>(1, maxParticles));
}

void Particles::tick(spg::App& app, float deltaTime)
{
  particleIterator.dispatch(app.getCmdBuf())
    .push(deltaTime)
    .bind(0, gpuParticles)
    .bind(1, freeParticles)
    .bind(2, freeParticleCount);

  emitterIterator.dispatch(app.getCmdBuf())
    .push(deltaTime)
    .bind(0, gpuEmitters)
    .bind(1, freeParticles)
    .bind(2, freeParticleCount)
    .bind(3, gpuParticles);
}

void Particles::draw(spg::App& app, spg::Texture& color, spg::Texture& depth)
{
  struct Matrices
  {
    glm::mat4 globalToScreen;
    float aspect;
  };

  Matrices matrices;
  matrices.globalToScreen = app.getWorldViewProj();
  matrices.aspect = app.getAspect();

  depthSorter.dispatch(app.getCmdBuf())
    .push(matrices.globalToScreen)
    .bind(0, gpuParticles)
    .bind(1, sortedIndices);

  renderShader.dispatch(app.getCmdBuf())
    .implicitVertexCount(4)
    .primitiveTopology(vk::PrimitiveTopology::eTriangleStrip)
    .pushVertex(matrices)
    .bind(0, gpuParticles)
    .bind(1, sortedIndices)
    .attach(color, vk::AttachmentLoadOp::eLoad)
    .instanceCount(maxParticles)
    .attachAsDepth(depth, vk::AttachmentLoadOp::eLoad);
}
