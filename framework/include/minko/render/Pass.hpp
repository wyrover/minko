/*
Copyright (c) 2014 Aerys

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

#include "minko/render/Blending.hpp"
#include "minko/render/CompareMode.hpp"
#include "minko/render/TriangleCulling.hpp"
#include "minko/render/ProgramSignature.hpp"
#include "minko/render/Program.hpp"
#include "minko/render/States.hpp"
#include "minko/render/VertexAttribute.hpp"

namespace minko
{
	namespace render
	{
		class Pass :
			public std::enable_shared_from_this<Pass>
		{
		public:
			typedef std::shared_ptr<Pass> Ptr;

		private:
 			typedef std::shared_ptr<Program>							ProgramPtr;
			typedef std::shared_ptr<VertexBuffer>						VertexBufferPtr;
            typedef std::unordered_map<std::string, SamplerState>		SamplerStatesMap;
            typedef std::unordered_map<ProgramSignature*, ProgramPtr>	SignatureToProgramMap;
			typedef std::list<std::function<void(ProgramPtr)>>			OnProgramFunctionList;	
			typedef std::unordered_map<std::string, data::MacroBinding> MacroBindingsMap;

		private:
			const std::string		_name;
			ProgramPtr				_programTemplate;
			data::BindingMap		_attributeBindings;
			data::BindingMap		_uniformBindings;
			data::BindingMap		_stateBindings;
			data::MacroBindingMap	_macroBindings;
            States				    _states;
            SignatureToProgramMap	_signatureToProgram;

			OnProgramFunctionList	_uniformFunctions;
			OnProgramFunctionList	_attributeFunctions;
            OnProgramFunctionList   _macroFunctions;

		public:
            ~Pass()
            {
                for (auto& signatureAndProgram : _signatureToProgram)
                    delete signatureAndProgram.first;
            }
            
			inline static
			Ptr
			create(const std::string&				name,
				   std::shared_ptr<render::Program>	program,
				   const data::BindingMap&			attributeBindings,
				   const data::BindingMap&			uniformBindings,
				   const data::BindingMap&			stateBindings,
				   const data::MacroBindingMap&		macroBindings,
                   const States&       				states)
			{
				return std::shared_ptr<Pass>(new Pass(
					name,
					program,
					attributeBindings,
					uniformBindings,
					stateBindings,
					macroBindings,
                    states
				));
			}

			inline static
			Ptr
			create(Ptr pass, float, bool deepCopy = false)
			{
				auto p = create(
					pass->_name,
					deepCopy ? Program::create(pass->_programTemplate, deepCopy) : pass->_programTemplate,
					pass->_attributeBindings,
					pass->_uniformBindings,
					pass->_stateBindings,
					pass->_macroBindings,
					deepCopy ? States(pass->_states) : pass->_states
				);

                for (auto& signatureProgram : pass->_signatureToProgram)
                    p->_signatureToProgram[new ProgramSignature(*signatureProgram.first)] = signatureProgram.second;

				p->_uniformFunctions = pass->_uniformFunctions;
				p->_attributeFunctions = pass->_attributeFunctions;
	
				if (pass->_programTemplate->isReady())
				{
					for (auto& f : p->_uniformFunctions)
						f(pass->_programTemplate);
					for (auto& f : p->_attributeFunctions)
						f(pass->_programTemplate);
				}

				return p;
			}

			inline
			const std::string&
			name()
			{
				return _name;
			}

			inline
			std::shared_ptr<Program>
			program()
			{
				return _programTemplate;
			}

			inline
			const data::BindingMap&
			attributeBindings() const
			{
				return _attributeBindings;
			}

			inline
			const data::BindingMap&
			uniformBindings() const
			{
				return _uniformBindings;
			}

			inline
			const data::BindingMap&
			stateBindings() const
			{
				return _stateBindings;
			}

			inline
			const data::MacroBindingMap&
			macroBindings() const
			{
				return _macroBindings;
			}

			inline
			States&
			states()
			{
				return _states;
			}

            std::pair<std::shared_ptr<Program>, const ProgramSignature*>
            selectProgram(const std::unordered_map<std::string, std::string>&   translatedPropertyNames,
						  const data::Store&	                            targetData,
						  const data::Store&	                            rendererData,
                          const data::Store&	                            rootData);
			
			template <typename... T>
			void
			setUniform(const std::string& name, const T&... values)
			{
				_uniformFunctions.push_back(std::bind(
					&Pass::setUniformOnProgram<T...>, std::placeholders::_1, name, values...
				));

				if (_programTemplate->isReady())
					_programTemplate->setUniform(name, values...);
				for (auto signatureAndProgram : _signatureToProgram)
					signatureAndProgram.second->setUniform(name, values...);
			}

			inline
			void
            setAttribute(const std::string& name, const VertexAttribute& attribute)
			{
				_attributeFunctions.push_back(std::bind(
					&Pass::setVertexAttributeOnProgram, std::placeholders::_1, name, attribute
				));

				if (_programTemplate->isReady())
					_programTemplate->setAttribute(name, attribute);
				for (auto signatureAndProgram : _signatureToProgram)
					signatureAndProgram.second->setAttribute(name, attribute);
			}

			inline
			void
			define(const std::string& macroName)
			{
                _macroFunctions.push_back(std::bind(&Pass::defineOnProgram, std::placeholders::_1, macroName));
                _programTemplate->define(macroName);
			}

            template <typename T>
			inline
			void
			define(const std::string& macroName, T macroValue)
			{
                _macroFunctions.push_back(std::bind(
                    &Pass::defineOnProgramWithValue<T>, std::placeholders::_1, macroName, macroValue
                ));
                _programTemplate->define(macroName, macroValue);
			}

		private:
			Pass(const std::string&					name,
				 std::shared_ptr<render::Program>	program,
				 const data::BindingMap&			attributeBindings,
				 const data::BindingMap&			uniformBindings,
				 const data::BindingMap&			stateBindings,
				 const data::MacroBindingMap&		macroBindings,
                 const States&                      states);

			template <typename... T>
			static
			void
			setUniformOnProgram(std::shared_ptr<Program> program, const std::string& name, const T&... values)
			{
				program->setUniform(name, values...);
			}

			static
			void
            setVertexAttributeOnProgram(std::shared_ptr<Program> program, const std::string& name, const VertexAttribute& attribute)
			{
				program->setAttribute(name, attribute);
			}

            static
            void
            defineOnProgram(std::shared_ptr<Program> program, const std::string& macroName)
            {
                program->define(macroName);
            }

            template <typename T>
            static
            void
            defineOnProgramWithValue(std::shared_ptr<Program> program, const std::string& macroName, T value)
            {
                program->define(macroName, value);
            }

			ProgramPtr
			finalizeProgram(ProgramPtr program);
		};
	}
}
