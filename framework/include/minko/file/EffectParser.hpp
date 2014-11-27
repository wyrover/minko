/*
Copyright (c) 2013 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "minko/Common.hpp"

#include "minko/Signal.hpp"
#include "minko/file/AbstractParser.hpp"
#include "minko/file/FileProtocol.hpp"
#include "minko/render/Blending.hpp"
#include "minko/render/Shader.hpp"
#include "minko/scene/Layout.hpp"
#include "minko/data/MacroBinding.hpp"
#include "minko/data/BindingMap.hpp"
#include "minko/data/Store.hpp"

namespace Json {
    class Value;
}

namespace minko
{
	namespace file
	{
		class EffectParser :
			public AbstractParser
		{
		public:
			typedef std::shared_ptr<EffectParser>	Ptr;

        private:
            typedef std::shared_ptr<render::AbstractTexture>	            AbstractTexturePtr;
            typedef std::shared_ptr<Loader>								    LoaderPtr;
            typedef std::shared_ptr<render::Effect>						    EffectPtr;
            typedef std::shared_ptr<render::Pass>						    PassPtr;
            typedef std::shared_ptr<render::Shader>						    ShaderPtr;
            typedef std::shared_ptr<file::Options>						    OptionsPtr;
            typedef std::unordered_map<std::string, AbstractTexturePtr>     TexturePtrMap;
            typedef Signal<LoaderPtr, const ParserError&>::Slot             LoaderErrorSlot;
            typedef std::vector<PassPtr>                                    Technique;
            typedef std::unordered_map<std::string, Technique>              Techniques;
            typedef std::vector<PassPtr>                                    Passes;
            typedef std::unordered_map<LoaderPtr, Signal<LoaderPtr>::Slot>  LoaderCompleteSlotMap;
            typedef std::unordered_map<LoaderPtr, LoaderErrorSlot>          LoaderErrorSlotMap;

            enum class GLSLBlockType
            {
                TEXT,
                FILE
            };

            typedef std::pair<GLSLBlockType, std::string>           GLSLBlock;
            typedef std::forward_list<GLSLBlock> 			        GLSLBlockList;
            typedef std::shared_ptr<GLSLBlockList>			        GLSLBlockListPtr;
            typedef std::unordered_map<ShaderPtr, GLSLBlockListPtr> ShaderToGLSLBlocks;

            template <typename T>
            struct Block
            {
                T bindings;
            };

            typedef Block<data::BindingMap> AttributeBlock;
            typedef Block<data::BindingMap> StateBlock;
            typedef Block<data::MacroBindingMap> MacroBlock;

            struct UniformBlock : public Block<data::BindingMap>
            {
                std::unordered_map<std::string, render::SamplerState> samplerStates;
            };

            struct Scope
            {
                const Scope* parent;
                std::vector<const Scope*> children;
                AttributeBlock attributes;
                UniformBlock uniforms;
                StateBlock states;
                MacroBlock macros;
                std::string defaultTechnique;
                Passes passes;
                Techniques techniques;

                Scope() :
                    parent(nullptr)
                {}

                Scope(const Scope& scope) :
                    parent(scope.parent),
                    children(),
                    attributes(scope.attributes),
                    uniforms(scope.uniforms),
                    states(scope.states),
                    macros(scope.macros),
                    defaultTechnique(scope.defaultTechnique),
                    passes(scope.passes),
                    techniques(scope.techniques)
                {}

                Scope(const Scope& scope, Scope& parent) :
                    parent(&parent),
                    children(),
                    attributes(scope.attributes),
                    uniforms(scope.uniforms),
                    states(scope.states),
                    macros(scope.macros),
                    defaultTechnique(scope.defaultTechnique),
                    passes(scope.passes),
                    techniques(scope.techniques)
                {
                    parent.children.push_back(this);
                }
            };

		private:
			static std::unordered_map<std::string, unsigned int>				_blendFactorMap;
			static std::unordered_map<std::string, render::CompareMode>			_compareFuncMap;
			static std::unordered_map<std::string, render::StencilOperation>	_stencilOpMap;
			static std::unordered_map<std::string, float>						_priorityMap;
            static std::array<std::string, 14>                                  _stateNames;

		private:
            std::string						_filename;
			std::string						_resolvedFilename;
			std::shared_ptr<file::Options>	_options;
			std::shared_ptr<render::Effect> _effect;
			std::string						_effectName;
			std::shared_ptr<AssetLibrary>	_assetLibrary;

            Scope                           _globalScope;
            ShaderToGLSLBlocks              _shaderToGLSL;
			unsigned int					_numDependencies;
			unsigned int					_numLoadedDependencies;

            LoaderCompleteSlotMap           _loaderCompleteSlots;
            LoaderErrorSlotMap              _loaderErrorSlots;

		public:
			inline static
			Ptr
			create()
			{
				return std::shared_ptr<EffectParser>(new EffectParser());
			}

			inline
			std::shared_ptr<render::Effect>
			effect()
			{
				return _effect;
			}

			inline
			const std::string&
			effectName()
			{
				return _effectName;
			}

			void
			parse(const std::string&				filename,
				  const std::string&                resolvedFilename,
                  std::shared_ptr<Options>          options,
				  const std::vector<unsigned char>&	data,
				  std::shared_ptr<AssetLibrary>		assetLibrary);

		private:
			EffectParser();

            static
            std::unordered_map<std::string, unsigned int>
            initializeBlendFactorMap();

            static
            std::unordered_map<std::string, render::CompareMode>
            initializeCompareFuncMap();

            static
            std::unordered_map<std::string, render::StencilOperation>
            initializeStencilOperationMap();

            static
            std::unordered_map<std::string, float>
            initializePriorityMap();

            static
            std::array<std::string, 14>
            initializeStateNames();

            float
            getPriorityValue(const std::string& name);

            void
            parseGlobalScope(const Json::Value& node, Scope& scope);

            bool
            parseConfiguration(const Json::Value& node);

            void
            parseTechniques(const Json::Value& node, Scope& scope, Techniques& techniques);

            void
            parsePass(const Json::Value& node, Scope& scope, std::vector<PassPtr>& passes);

            void
            parsePasses(const Json::Value& node, Scope& scope, std::vector<PassPtr>& passes);

            void
            parseDefaultValue(const Json::Value& node, const Scope& scope);


            /*void
            parseDefaultValue(const Json::Value&    node,
                              const Scope&          scope,
                              const std::string&    valueName,
                              Json::ValueType       expectedType,
                              data::Provider::Ptr   defaultValues);*/

            void
            parseDefaultValue(const Json::Value&    node,
                              const Scope&          scope,
                              const std::string&    valueName,
                              data::Provider::Ptr   defaultValues);

            void
            parseDefaultValueVector(const Json::Value&    defaultValueNode,
                                    const Scope&          scope,
                                    const std::string&    valueName,
                                    data::Provider::Ptr   defaultValues);

            void
            parseDefaultValueObject(const Json::Value&    node,
                                    const Scope&          scope,
                                    const std::string&    valueName,
                                    data::Provider::Ptr   defaultValues);

            void
            parseBinding(const Json::Value& node, const Scope& scope, data::Binding& binding);

            void
            parseMacroBinding(const Json::Value& node, const Scope& scope, data::MacroBinding& binding);

            void
            parseMacroBindings(const Json::Value& node, const Scope& scope, data::MacroBindingMap& bindings);

            void
            parseAttributes(const Json::Value&node, const Scope& scope, AttributeBlock& attributes);

            void
            parseUniforms(const Json::Value& node, const Scope& scope, UniformBlock& uniforms);

            void
            parseMacros(const Json::Value& node, const Scope& scope, MacroBlock& macros);

            void
            parseStates(const Json::Value& node, const Scope& scope, StateBlock& states);

            void
            parseBlendMode(const Json::Value&				node,
                           const Scope&                     scope,
                           render::Blending::Source&		srcFactor,
                           render::Blending::Destination&	dstFactor);

            void
            parseZSort(const Json::Value&   node,
                       const Scope&         scope,
                       bool&                zSorted) const;

            void
            parseColorMask(const Json::Value&   node,
                           const Scope&         scope,
                           bool&                colorMask) const;

            void
            parseDepthTest(const Json::Value&	node,
                           const Scope&         scope,
                           bool&                depthMask,
                           render::CompareMode& depthFunc);

            void
            parseTriangleCulling(const Json::Value&         node,
                                 const Scope&               scope,
                                 render::TriangleCulling&   triangleCulling);

            float
            parsePriority(const Json::Value&    node,
                          const Scope&          scope,
                          float                 defaultPriority);

            void
            parseStencilState(const Json::Value&        node,
                              const Scope&              scope,
                              render::CompareMode&      stencilFunc,
                              int&                      stencilRef,
                              uint&                     stencilMask,
                              render::StencilOperation& stencilFailOp,
                              render::StencilOperation& stencilZFailOp,
                              render::StencilOperation& stencilZPassOp);

            void
            parseStencilOperations(const Json::Value&           node,
                                   const Scope&                 scope,
                                   render::StencilOperation&    stencilFailOp,
                                   render::StencilOperation&    stencilZFailOp,
                                   render::StencilOperation&    stencilZPassOp);

            void
            parseScissorTest(const Json::Value&    node,
                             const Scope&          scope,
                             bool&                 scissorTest,
                             math::ivec4&          scissorBox);

            ShaderPtr
            parseShader(const Json::Value& node, const Scope& scope, render::Shader::Type type);

            void
            parseGLSL(const std::string&        glsl,
                      OptionsPtr         		options,
                      GLSLBlockListPtr		    blocks,
                      GLSLBlockList::iterator	insertIt);

            void
            loadGLSLDependencies(GLSLBlockListPtr blocks, OptionsPtr options);

            void
            dependencyErrorHandler(std::shared_ptr<Loader>  loader,
                                   const ParserError&       error,
                                   const std::string&       filename);

            void
            glslIncludeCompleteHandler(LoaderPtr 			    loader,
                                       GLSLBlockListPtr 		blocks,
                                       GLSLBlockList::iterator 	blockIt,
                                       const std::string&       filename);

            std::string
            concatenateGLSLBlocks(GLSLBlockListPtr blocks);

            void
            loadTexture(const std::string&  textureFilename,
                        const std::string&  uniformName,
						data::Provider::Ptr     defaultValues);

            std::shared_ptr<render::States>
            createStates(const StateBlock& block);

            void
            finalize();
		};
	}
}
