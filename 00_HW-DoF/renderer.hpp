#pragma once

#include <vector>

#include "camera.hpp"
#include "spotlight.hpp"
#include "framebuffer.hpp"
#include "shadowmap_framebuffer.hpp"
#include "ogl_material_factory.hpp"
#include "ogl_geometry_factory.hpp"

class QuadRenderer {
public:
	QuadRenderer()
		: mQuad(generateQuadTex())
	{}

	void render(const OGLShaderProgram &aShaderProgram, MaterialParameterValues &aParameters) const {
		aShaderProgram.use();
		aShaderProgram.setMaterialParameters(aParameters, MaterialParameterValues());
		GL_CHECK(glBindVertexArray(mQuad.vao.get()));
  		GL_CHECK(glDrawElements(mQuad.mode, mQuad.indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(0)));
	}
protected:

	IndexedBuffer mQuad;
};

inline std::vector<CADescription> getColorNormalPositionAttachments() {
	return {
		{ GL_RGBA, GL_FLOAT, GL_RGBA },
		// To store values outside the range [0,1] we need different internal format then normal GL_RGBA
		{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
		{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
		{ GL_RGBA, GL_FLOAT, GL_RGBA32F } // added new attachment
	};
}

inline std::vector<CADescription> getSingleColorAttachment() {
	return {
		{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
	};
}

std::unique_ptr<Framebuffer>           mSceneFBO;
std::unique_ptr<Framebuffer>           mBlurFBO;
std::shared_ptr<OGLShaderProgram>      mBlurShader;
std::shared_ptr<OGLShaderProgram>      mDofShader;

class Renderer {
public:
	Renderer(OGLMaterialFactory &aMaterialFactory)
		: mMaterialFactory(aMaterialFactory)
	{
		mCompositingShader = std::static_pointer_cast<OGLShaderProgram>(
				mMaterialFactory.getShaderProgram("compositing"));
		// mShadowMapShader = std::static_pointer_cast<OGLShaderProgram>(
		// 	mMaterialFactory.getShaderProgram("solid_color"));
		mShadowMapShader = std::static_pointer_cast<OGLShaderProgram>(
			mMaterialFactory.getShaderProgram("shadowmap"));

		mBlurShader = std::static_pointer_cast<OGLShaderProgram>(
			mMaterialFactory.getShaderProgram("gaussian_blur"));
		mDofShader = std::static_pointer_cast<OGLShaderProgram>(
			mMaterialFactory.getShaderProgram("dof"));
	}

	void initialize(int aWidth, int aHeight) {
		mWidth = aWidth;
		mHeight = aHeight;
		GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));

		mFramebuffer = std::make_unique<Framebuffer>(aWidth, aHeight, getColorNormalPositionAttachments());
		mShadowmapFramebuffer = std::make_unique<Framebuffer>(600, 600, getSingleColorAttachment());
		// mShadowmapFramebuffer = std::make_unique<ShadowmapFramebuffer>(600, 600);
		mCompositingParameters = {
			{ "u_diffuse", TextureInfo("diffuse", mFramebuffer->getColorAttachment(0)) },
			{ "u_normal", TextureInfo("diffuse", mFramebuffer->getColorAttachment(1)) },
			{ "u_position", TextureInfo("diffuse", mFramebuffer->getColorAttachment(2)) },
			{ "u_depthMap", TextureInfo("diffuse", mFramebuffer->getColorAttachment(3)) }, //added depth map
			// { "u_shadowMap", TextureInfo("shadowMap", mShadowmapFramebuffer->getDepthMap()) },
			{ "u_shadowMap", TextureInfo("shadowMap", mShadowmapFramebuffer->getColorAttachment(0)) },
		};

		mSceneFBO = std::make_unique<Framebuffer>(mWidth, mHeight, getSingleColorAttachment());
		mBlurFBO = std::make_unique<Framebuffer>(mWidth, mHeight, getSingleColorAttachment());

		mDoFParameters = {
			{ "u_scene",    TextureInfo("u_scene",    mSceneFBO->getColorAttachment(0)) },
			{ "u_blur",     TextureInfo("u_blur",     mBlurFBO->getColorAttachment(0)) },
			{ "u_depthMap", TextureInfo("u_depthMap", mFramebuffer->getColorAttachment(3)) },
			{ "u_focusUV",    glm::vec2(0.5f, 0.5f) },   // start centered
			{ "u_focusRange", 0.1f }                     // default range
		};
	}

	void clear() {
		mFramebuffer->bind();
		GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	}

	template<typename TScene, typename TCamera>
	void geometryPass(const TScene &aScene, const TCamera &aCamera, RenderOptions aRenderOptions) {
		GL_CHECK(glEnable(GL_DEPTH_TEST));
		GL_CHECK(glViewport(0, 0, mWidth, mHeight));
		mFramebuffer->bind();
		mFramebuffer->setDrawBuffers();
		auto projection = aCamera.getProjectionMatrix();
		auto view = aCamera.getViewMatrix();

		std::vector<RenderData> renderData;
		for (const auto &object : aScene.getObjects()) {
			auto data = object.getRenderData(aRenderOptions);
			if (data) {
				renderData.push_back(data.value());
			}
		}

		MaterialParameterValues fallbackParameters;
		fallbackParameters["u_projMat"] = projection;
		fallbackParameters["u_viewMat"] = view;
		fallbackParameters["u_solidColor"] = glm::vec4(0,0,0,1);
		fallbackParameters["u_viewPos"] = aCamera.getPosition();
		fallbackParameters["u_near"] = aCamera.near();
		fallbackParameters["u_far"] = aCamera.far();
		for (const auto &data: renderData) {
			const glm::mat4 modelMat = data.modelMat;
			const MaterialParameters &params = data.mMaterialParams;
			const OGLShaderProgram &shaderProgram = static_cast<const OGLShaderProgram &>(data.mShaderProgram);
			const OGLGeometry &geometry = static_cast<const OGLGeometry&>(data.mGeometry);

			fallbackParameters["u_modelMat"] = modelMat;
			fallbackParameters["u_normalMat"] = glm::mat3(modelMat);

			shaderProgram.use();
			shaderProgram.setMaterialParameters(params.mParameterValues, fallbackParameters);

			geometry.bind();
			geometry.draw();
		}
		mFramebuffer->unbind();
	}

	template<typename TLight>
	void compositingPass(const TLight& light) {
		GL_CHECK(glDisable(GL_DEPTH_TEST));
		GL_CHECK(glViewport(0, 0, mWidth, mHeight));

		// 1) render lit‐scene into mSceneFBO
		mSceneFBO->bind();
		mSceneFBO->setDrawBuffers();
		GL_CHECK(glClearColor(0, 0, 0, 0));
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));

		// 2) build a fresh param block from your existing map + light uniforms
		MaterialParameterValues params = mCompositingParameters;
		params["u_lightPos"] = light.getPosition();
		params["u_lightMat"] = light.getViewMatrix();
		params["u_lightProjMat"] = light.getProjectionMatrix();

		// 3) draw full‐screen quad into mSceneFBO
		mQuadRenderer.render(*mCompositingShader, params);
		mSceneFBO->unbind();
	}

	template<typename TScene, typename TLight>
	void shadowMapPass(const TScene &aScene, const TLight &aLight) {
		GL_CHECK(glEnable(GL_DEPTH_TEST));
		mShadowmapFramebuffer->bind();
		GL_CHECK(glViewport(0, 0, 600, 600));
		GL_CHECK(glClearColor(1.0f, 1.0f, 1.0f, 1.0f));
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		mShadowmapFramebuffer->setDrawBuffers();
		auto projection = aLight.getProjectionMatrix();
		auto view = aLight.getViewMatrix();

		MaterialParameterValues fallbackParameters;
		fallbackParameters["u_projMat"] = projection;
		fallbackParameters["u_viewMat"] = view;
		fallbackParameters["u_viewPos"] = aLight.getPosition();

		std::vector<RenderData> renderData;
		RenderOptions renderOptions = {"solid"};
		for (const auto &object : aScene.getObjects()) {
			auto data = object.getRenderData(renderOptions);
			if (data) {
				renderData.push_back(data.value());
			}
		}
		mShadowMapShader->use();
		for (const auto &data: renderData) {
			const glm::mat4 modelMat = data.modelMat;
			const MaterialParameters &params = data.mMaterialParams;
			const OGLShaderProgram &shaderProgram = static_cast<const OGLShaderProgram &>(data.mShaderProgram);
			const OGLGeometry &geometry = static_cast<const OGLGeometry&>(data.mGeometry);

			fallbackParameters["u_modelMat"] = modelMat;
			fallbackParameters["u_normalMat"] = glm::mat3(modelMat);

			mShadowMapShader->setMaterialParameters(fallbackParameters, {});

			geometry.bind();
			geometry.draw();
		}



		mShadowmapFramebuffer->unbind();
	}

	void blurPass() {
		bool horizontal = true, first = true;
		for (int i = 0; i < 2; ++i) {
			// each pass writes into mBlurFBO
			mBlurFBO->bind();
			mBlurFBO->setDrawBuffers();

			// set up only the two uniforms our gaussian_blur.frag needs
			MaterialParameterValues params;
			// u_image comes from mSceneFBO on first pass, then from mBlurFBO
			params["u_image"] = TextureInfo("u_image",
				first
				? mSceneFBO->getColorAttachment(0)
				: mBlurFBO->getColorAttachment(0));
			params["u_horizontal"] = horizontal;

			mQuadRenderer.render(*mBlurShader, params);
			mBlurFBO->unbind();

			horizontal = !horizontal;
			first = false;
		}
	}

	void dofPass(const glm::vec2& focusUV, float focusRange) {
		// 1) back to the default framebuffer (the screen)
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		GL_CHECK(glViewport(0, 0, mWidth, mHeight));

		// 2) copy your template DoF parameters and override the two dynamic ones
		MaterialParameterValues params = mDoFParameters;
		params["u_focusUV"] = focusUV;     // <<< mouse‐driven focus
		params["u_focusRange"] = focusRange;  // <<< how wide the sharp band is

		// 3) draw
		mQuadRenderer.render(*mDofShader, params);
	}


protected:
	int mWidth = 100;
	int mHeight = 100;
	std::unique_ptr<Framebuffer> mFramebuffer;
	std::unique_ptr<Framebuffer> mShadowmapFramebuffer;
	// std::unique_ptr<ShadowmapFramebuffer> mShadowmapFramebuffer;

	MaterialParameterValues mCompositingParameters;
	QuadRenderer mQuadRenderer;
	std::shared_ptr<OGLShaderProgram> mCompositingShader;
	std::shared_ptr<OGLShaderProgram> mShadowMapShader;
	OGLMaterialFactory &mMaterialFactory;

	std::unique_ptr<Framebuffer>   mSceneFBO;    // to capture the lit scene
	std::unique_ptr<Framebuffer>   mBlurFBO;     // ping‐pong target for gaussian blur
	std::shared_ptr<OGLShaderProgram> mBlurShader;
	std::shared_ptr<OGLShaderProgram> mDofShader;
	MaterialParameterValues    mDoFParameters;    
};
