#pragma once

#include <cstdint>

/// \brief the shared "texalpha" shader most menu screen-space quad draws use. MenuDrawable binds
/// it once per page-render pass; draws that switch to another shader mid-pass (BitmapFont,
/// MenuPageListItem rows) must restore *this* one afterwards, not shader 0 (in GLES3 shader 0
/// means "no program bound"). Multiplies the "alpha" uniform with the texture's sampled alpha.
uint32_t getDefaultMenuShader();

//! "alpha" uniform location on getDefaultMenuShader() - loads the shader first if needed.
int getDefaultMenuShaderAlphaParam();

/// \brief the "texalphaignore" shader, only for MenuDrawable's page-composite framebuffer blit:
/// the FBO's own alpha is accumulated blend residue, so it is replaced by a fade-alpha uniform.
/// Do not use this for per-item PSD asset draws - see getDefaultMenuShader().
uint32_t getFramebufferBlitShader();

//! "alpha" uniform location on getFramebufferBlitShader() - loads the shader first if needed.
int getFramebufferBlitShaderAlphaParam();
