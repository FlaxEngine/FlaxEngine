// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_OGL_SHADER_COMPILER

#include "ShaderCompilerOGL.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Core/Log.h"
#include "../RenderToolsOGL.h"
#include "../ShaderAPI.h"
#include <fstream>
#include <iostream>
#include <LZ4/lz4.h>

ShaderCompilerOGL::ShaderCompilerOGL(ShaderProfile profile)
	: _profile(profile)
	, _context(nullptr)
	, _sourceStream(0)
{
	// Check for supported profiles
	ASSERT(profile == ShaderProfile::GLSL_410 || profile == ShaderProfile::GLSL_440);
}

ShaderCompilerOGL::~ShaderCompilerOGL()
{
}

#define USE_DETAILED_LOG 1

class XscLog : public Xsc::Log
{
public:

	std::stringstream Log;

private:

#if USE_DETAILED_LOG

	static void PrintMultiLineString(std::stringstream& output, const std::string& str, const std::string& indent)
	{
		// Determine at which position the actual text begins (excluding the "error (X:Y) : " or the like) 
		auto textStartPos = str.find(" : ");
		if (textStartPos != std::string::npos)
			textStartPos += 3;
		else
			textStartPos = 0;

		std::string newLineIndent(textStartPos, ' ');

		size_t start = 0;
		bool useNewLineIndent = false;
		while (start < str.size())
		{
			output << indent;

			if (useNewLineIndent)
				output << newLineIndent;

			// Print next line
			auto end = str.find('\n', start);

			if (end != std::string::npos)
			{
				output << str.substr(start, end - start);
				start = end + 1;
			}
			else
			{
				output << str.substr(start);
				start = end;
			}

			output << std::endl;
			useNewLineIndent = true;
		}
	}

	void PrintReport(std::stringstream& output, const Xsc::Report& report, const std::string& indent)
	{
		// Print optional context description
		auto context = report.Context();
		if (!context.empty())
			PrintMultiLineString(output, context, indent);

		// Print report message
		auto msg = report.Message();
		PrintMultiLineString(output, msg, indent);

		// Print optional line and line-marker
		if (report.HasLine())
		{
			const auto& line = report.Line();
			const auto& marker = report.Marker();

			// Print line
			output << indent << line << std::endl;

			// Print line marker
			output << indent << marker << std::endl;
		}

		// Print optional hints
		auto hints = report.GetHints();
		if (hints.size() > 0)
		{
			for (auto hint : hints)
				output << indent << hint << std::endl;
		}
	}

#endif

public:

	// [Log]
	void SubmitReport(const Xsc::Report& report) override
	{
#if !USE_DETAILED_LOG
		std::string str = report.Message();
		switch (report.Type())
		{
		case Xsc::ReportTypes::Info:
			Log << "Info: " << str << std::endl;
			break;
		case Xsc::ReportTypes::Warning:
			Log << "Warning: " << str << std::endl;
			break;
		case Xsc::ReportTypes::Error:
			Log << "Error: " << str << std::endl;
			break;
		}
#else
		switch (report.Type())
		{
		case Xsc::ReportTypes::Info:
			Log << "Info: " << std::endl;
			break;
		case Xsc::ReportTypes::Warning:
			Log << "Warning: " << std::endl;
			break;
		case Xsc::ReportTypes::Error:
			Log << "Error: " << std::endl;
			break;
		}
		PrintReport(Log, report, FullIndent());
#endif
	}
};

int GetUniformSize(Xsc::Reflection::DataType type)
{
	switch (type)
	{
		//case Xsc::Reflection::DataType::Undefined:

// String types:
//case Xsc::Reflection::DataType::String:

		// Scalar types
	case Xsc::Reflection::DataType::Bool: return 1;
	case Xsc::Reflection::DataType::Int: return 4;
	case Xsc::Reflection::DataType::UInt: return 4;
	case Xsc::Reflection::DataType::Half: return 2;
	case Xsc::Reflection::DataType::Float: return 4;
	case Xsc::Reflection::DataType::Double: return 8;

		// Vector types
	case Xsc::Reflection::DataType::Bool2: return 1 * 2;
	case Xsc::Reflection::DataType::Bool3: return 1 * 3;
	case Xsc::Reflection::DataType::Bool4: return 1 * 4;
	case Xsc::Reflection::DataType::Int2: return 4 * 2;
	case Xsc::Reflection::DataType::Int3: return 4 * 3;
	case Xsc::Reflection::DataType::Int4: return 4 * 4;
	case Xsc::Reflection::DataType::UInt2: return 4 * 2;
	case Xsc::Reflection::DataType::UInt3: return 4 * 3;
	case Xsc::Reflection::DataType::UInt4: return 4 * 4;
	case Xsc::Reflection::DataType::Half2: return 2 * 2;
	case Xsc::Reflection::DataType::Half3: return 2 * 3;
	case Xsc::Reflection::DataType::Half4: return 2 * 4;
	case Xsc::Reflection::DataType::Float2: return 4 * 2;
	case Xsc::Reflection::DataType::Float3: return 4 * 3;
	case Xsc::Reflection::DataType::Float4: return 4 * 4;
	case Xsc::Reflection::DataType::Double2: return 8 * 2;
	case Xsc::Reflection::DataType::Double3: return 8 * 3;
	case Xsc::Reflection::DataType::Double4: return 8 * 4;

		// Matrix types
	case Xsc::Reflection::DataType::Bool2x2: return 1 * 2 * 2;
	case Xsc::Reflection::DataType::Bool2x3: return 1 * 2 * 3;
	case Xsc::Reflection::DataType::Bool2x4: return 1 * 2 * 4;
	case Xsc::Reflection::DataType::Bool3x2: return 1 * 3 * 2;
	case Xsc::Reflection::DataType::Bool3x3: return 1 * 3 * 4;
	case Xsc::Reflection::DataType::Bool3x4: return 1 * 3 * 5;
	case Xsc::Reflection::DataType::Bool4x2: return 1 * 4 * 2;
	case Xsc::Reflection::DataType::Bool4x3: return 1 * 4 * 3;
	case Xsc::Reflection::DataType::Bool4x4: return 1 * 4 * 4;
	case Xsc::Reflection::DataType::Int2x2: return 4 * 2 * 2;
	case Xsc::Reflection::DataType::Int2x3: return 4 * 2 * 3;
	case Xsc::Reflection::DataType::Int2x4: return 4 * 2 * 4;
	case Xsc::Reflection::DataType::Int3x2: return 4 * 3 * 2;
	case Xsc::Reflection::DataType::Int3x3: return 4 * 3 * 3;
	case Xsc::Reflection::DataType::Int3x4: return 4 * 3 * 4;
	case Xsc::Reflection::DataType::Int4x2: return 4 * 4 * 2;
	case Xsc::Reflection::DataType::Int4x3: return 4 * 4 * 3;
	case Xsc::Reflection::DataType::Int4x4: return 4 * 4 * 4;
	case Xsc::Reflection::DataType::UInt2x2: return 4 * 2 * 2;
	case Xsc::Reflection::DataType::UInt2x3: return 4 * 2 * 3;
	case Xsc::Reflection::DataType::UInt2x4: return 4 * 2 * 4;
	case Xsc::Reflection::DataType::UInt3x2: return 4 * 3 * 2;
	case Xsc::Reflection::DataType::UInt3x3: return 4 * 3 * 3;
	case Xsc::Reflection::DataType::UInt3x4: return 4 * 3 * 4;
	case Xsc::Reflection::DataType::UInt4x2: return 4 * 4 * 2;
	case Xsc::Reflection::DataType::UInt4x3: return 4 * 4 * 3;
	case Xsc::Reflection::DataType::UInt4x4: return 4 * 4 * 4;
	case Xsc::Reflection::DataType::Half2x2: return 2 * 2 * 2;
	case Xsc::Reflection::DataType::Half2x3: return 2 * 2 * 3;
	case Xsc::Reflection::DataType::Half2x4: return 2 * 2 * 4;
	case Xsc::Reflection::DataType::Half3x2: return 2 * 3 * 2;
	case Xsc::Reflection::DataType::Half3x3: return 2 * 3 * 3;
	case Xsc::Reflection::DataType::Half3x4: return 2 * 3 * 4;
	case Xsc::Reflection::DataType::Half4x2: return 2 * 4 * 2;
	case Xsc::Reflection::DataType::Half4x3: return 2 * 4 * 3;
	case Xsc::Reflection::DataType::Half4x4: return 2 * 4 * 4;
	case Xsc::Reflection::DataType::Float2x2: return 4 * 2 * 2;
	case Xsc::Reflection::DataType::Float2x3: return 4 * 2 * 3;
	case Xsc::Reflection::DataType::Float2x4: return 4 * 2 * 4;
	case Xsc::Reflection::DataType::Float3x2: return 4 * 3 * 2;
	case Xsc::Reflection::DataType::Float3x3: return 4 * 3 * 3;
	case Xsc::Reflection::DataType::Float3x4: return 4 * 3 * 4;
	case Xsc::Reflection::DataType::Float4x2: return 4 * 4 * 2;
	case Xsc::Reflection::DataType::Float4x3: return 4 * 4 * 3;
	case Xsc::Reflection::DataType::Float4x4: return 4 * 4 * 4;
	case Xsc::Reflection::DataType::Double2x2: return 8 * 2 * 2;
	case Xsc::Reflection::DataType::Double2x3: return 8 * 2 * 3;
	case Xsc::Reflection::DataType::Double2x4: return 8 * 2 * 4;
	case Xsc::Reflection::DataType::Double3x2: return 8 * 3 * 2;
	case Xsc::Reflection::DataType::Double3x3: return 8 * 3 * 3;
	case Xsc::Reflection::DataType::Double3x4: return 8 * 3 * 4;
	case Xsc::Reflection::DataType::Double4x2: return 8 * 4 * 2;
	case Xsc::Reflection::DataType::Double4x3: return 8 * 4 * 3;
	case Xsc::Reflection::DataType::Double4x4: return 8 * 4 * 4;
	}

	return 0;
}

bool ShaderCompilerOGL::ProcessShader(Xsc::Reflection::ReflectionData& data, uint32& cbMask)
{
	// Reset masks
	cbMask = 0;
	srMask = 0;
	uaMask = 0;

	// Extract constant buffers usage information
	for (const auto& cb : data.constantBuffers)
	{
		if (cb.location < 0)
		{
			_context->OnError("Missing bound resource.");
			return true;
		}

		// Set flag
		cbMask |= 1 << cb.location;

		// Try to add CB to the list
		for (int32 b = 0; b < _constantBuffers.Count(); b++)
		{
			auto& cc = _constantBuffers[b];
			if (cc.Slot == cb.location)
			{
				// Calculate the size (based on uniforms used in this buffer)
				cc.Size = 0;
				for (const auto& uniform : data.uniforms)
				{
					if (uniform.type == Xsc::Reflection::UniformType::Variable
						&& uniform.uniformBlock == cb.location)
					{
						cc.Size += GetUniformSize((Xsc::Reflection::DataType)uniform.baseType);
					}
				}

				cc.IsUsed = true;
				break;
			}
		}
	}

	// Extract resources usage
	for (const auto& sr : data.textures)
	{
		if (sr.location < 0)
		{
			_context->OnError("Missing bound resource.");
			return true;
		}

		// Set flag
		srMask |= 1 << sr.location;
	}

	return false;
}

bool ShaderCompilerOGL::CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite)
{
	// Prepare
	auto output = _context->Output;
	auto options = _context->Options;
	auto type = meta.GetStage();
	auto permutationsCount = meta.GetPermutationsCount();

	// Construct shader target
	switch (type)
	{
	case ShaderStage::Vertex: _inputDesc.shaderTarget = Xsc::ShaderTarget::VertexShader; break;
	case ShaderStage::Hull: _inputDesc.shaderTarget = Xsc::ShaderTarget::TessellationControlShader; break;
	case ShaderStage::Domain: _inputDesc.shaderTarget = Xsc::ShaderTarget::TessellationEvaluationShader; break;
	case ShaderStage::Geometry: _inputDesc.shaderTarget = Xsc::ShaderTarget::GeometryShader; break;
	case ShaderStage::Pixel: _inputDesc.shaderTarget = Xsc::ShaderTarget::FragmentShader; break;
	case ShaderStage::Compute: _inputDesc.shaderTarget = Xsc::ShaderTarget::ComputeShader; break;
	default: return true;
	}

	// [Output] Type
	output->WriteByte(static_cast<byte>(type));

	// [Output] Permutations count
	output->WriteByte(permutationsCount);

	// [Output] Shader function name
	output->WriteStringAnsi(meta.Name, 11);

	// Compile all shader function permutations
	for (int32 permutationIndex = 0; permutationIndex < permutationsCount; permutationIndex++)
	{
		_macros.Clear();

		// Get function permutation macros
		meta.GetDefinitionsForPermutation(permutationIndex, _macros);

		// Add additional define for compiled function name
		GetDefineForFunction(meta, _macros);

		// Add custom and global macros (global last because contain null define to indicate ending)
		_macros.Add(_context->Options->Macros);
		_macros.Add(_globalMacros);

		// Setup the source code
		// TODO: use shared _sourceStream and reduce dynamic memory allocations
		std::shared_ptr<std::stringstream> input = std::shared_ptr<std::stringstream>(new std::stringstream());
		for (int32 i = 0; i < _macros.Count(); i++)
		{
			ASSERT(_macros[i].Name && _macros[i].Definition);
			*input << "#define " << _macros[i].Name << " " << _macros[i].Definition << "\n";
		}
		*input << options->Source;

		// Setup options for this permutation
		std::stringstream outputStream;// TODO: reuse it to reduce dynamic memory allocations
		_inputDesc.sourceCode = input;
		_inputDesc.entryPoint = meta.Name;
		_outputDesc.sourceCode = &outputStream;

		// Compile
		Xsc::Reflection::ReflectionData reflection;
		try
		{
			// Captures and prints to log the full AST tree
#define DEBUG_AST 0
#if DEBUG_AST
			_outputDesc.options.showAST = true;
			std::stringstream buffer;
			std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
#endif

			// Cross-compile HLSL to GLSL
			XscLog log;
			bool result = Xsc::CompileShader(_inputDesc, _outputDesc, &log, &reflection);

#if DEBUG_AST
			String text = String(buffer.str());
			LOG_STR(Info, text);
#endif

			// Check compilation result
			if (!result)
			{
				auto str = log.Log.str();
				_context->OnError(str.c_str());
				return true;
			}
		}
		catch (const std::exception& e)
		{
			_context->OnError(e.what());
			return true;
		}

		// Process reflection data
		uint32 cbMask, uaMsrMaskask, uaMask;
		if (ProcessShader(reflection, cbMask, srMaskk, uaMask))
			return true;

		// Get the source code (and append the null-terminator)
		auto str = outputStream.str();
		str += "\0";

		// Prepare the shader buffer (for raw mode)
		int32 shaderBufferType = SHADER_DATA_FORMAT_RAW;
		uint32 shaderBufferSize = (uint32)str.length() + 1;
		uint32 shaderBufferOriginalSize = shaderBufferSize;
		byte* shaderBuffer = (byte*)str.c_str();

		// Try to compress the source code (don't compress if memory gain is too small)
		{
			float maxCompressionRatio = 0.75f;

			// Try use LZ4
			int32 srcSize = (int32)shaderBufferSize;
			int32 estSize = LZ4_compressBound(srcSize);
			_dataCompressedCache.Clear();
			_dataCompressedCache.EnsureCapacity(estSize);
			int32 dstSize = LZ4_compress_default(str.c_str(), (char*)_dataCompressedCache.Get(), srcSize, estSize);
			float ratio = (float)dstSize / srcSize;
			if (dstSize == 0)
			{
				LOG(Warning, "Shader source data LZ4 compression failed.");
			}
			else if (ratio <= maxCompressionRatio)
			{
				// Use compressed format
				shaderBufferType = SHADER_DATA_FORMAT_LZ4;
				shaderBufferSize = dstSize;
				shaderBuffer = _dataCompressedCache.Get();
			}
		}

		// [Output] Write cross-compiled shader function
		output->WriteInt32(shaderBufferType);
		output->WriteUint32(shaderBufferOriginalSize);
		output->WriteUint32(shaderBufferSize);
		output->WriteBytes(shaderBuffer, shaderBufferSize);

		// [Output] Shader meta
		uint32 instructionCount = 0;// TODO: use AST reflection to count the shader instructions
		output->WriteUint32(instructionCount);
		output->WriteUint32(cbMask);
		output->WriteUint32(srMask);
		output->WriteUint32(uaMask);

		// Custom data
		if (customDataWrite && customDataWrite(output, meta, permutationIndex, _macros))
			return true;
	}

	return false;
}

class FlaxIncludeHandler : public Xsc::IncludeHandler
{
public:

	FlaxIncludeHandler()
	{
	}

	std::unique_ptr<std::istream> Include(const std::string& includeName, bool useSearchPathsFirst) override
	{
		const String filename(includeName);
		auto shaderApi = ShaderAPI::Instance();

		ScopeLock lock(shaderApi->Locker);

		const auto shader = shaderApi->GetShaderSource(filename);
		if (shader == nullptr)
			return nullptr;

		std::unique_ptr<std::stringstream> stream = std::unique_ptr<std::stringstream>(new std::stringstream());
		*stream << shader->Get();
		return stream;
	}
};

bool ShaderCompilerOGL::Compile(ShaderCompilationContext* context)
{
	// Clear cache
	_globalMacros.Clear();
	_macros.Clear();
	_constantBuffers.Clear();
	_context = context;

	// Prepare
	auto options = context->Options;
	auto output = context->Output;
	auto meta = context->Meta;
	bool use440 = _profile == ShaderProfile::GLSL_440;
	int32 shadersCount = meta->GetShadersCount();
	FlaxIncludeHandler includeHandler;

	// Setup global shader macros
	_globalMacros.EnsureCapacity(32);
	_macros.EnsureCapacity(32);
	_globalMacros.Add({ "OPENGL", "1" });
	GetGlobalDefines(_globalMacros);
	
	// Allocate shader source stream data (it will include compilation macros)
    //uint32 macrosCountApprox = _globalMacros.Count() + options->Macros.Count() + 10;
    //_sourceStream.Reset(Math::Max<uint32>(_sourceStream.GetCapacity(), sizeof(Char) * Math::RoundUpToPowerOf2<uint32>(options->SourceLength + 32 * macrosCountApprox)));
    // TODO: preallocate _sourceStream to reduce dynamic allocations

	// Setup cross-compiler options
	{
		switch (_profile)
		{
			case ShaderProfile::GLSL_440: _inputDesc.shaderVersion = Xsc::InputShaderVersion::HLSL5; break;
			case ShaderProfile::GLSL_410: _inputDesc.shaderVersion = Xsc::InputShaderVersion::HLSL4; break;
		}
		_inputDesc.filename = context->TargetNameAnsi;
		_inputDesc.extensions = Xsc::Extensions::LayoutAttribute;
		_inputDesc.includeHandler = &includeHandler;
	}
	{
		switch (_profile)
		{
			case ShaderProfile::GLSL_440: _outputDesc.shaderVersion = Xsc::OutputShaderVersion::GLSL440; break;
			case ShaderProfile::GLSL_410: _outputDesc.shaderVersion = Xsc::OutputShaderVersion::GLSL410; break;
		}
		_outputDesc.options.optimize = !options->NoOptimize;
		_outputDesc.options.separateShaders = true;
		_outputDesc.options.separateSamplers = true;
		_outputDesc.options.preserveComments = false;
		_outputDesc.options.explicitBinding = true;
		_outputDesc.formatting.writeGeneratorHeader = false;
		_outputDesc.formatting.blanks = false;
		_outputDesc.nameMangling.inputPrefix = "f_";
		_outputDesc.nameMangling.outputPrefix = "f_";
		_outputDesc.nameMangling.useAlwaysSemantics = true;
		_outputDesc.nameMangling.renameBufferFields = true;
	}

	// Setup constant buffers cache
	_constantBuffers.EnsureCapacity(meta->CB.Count(), false);
	for (int32 i = 0; i < meta->CB.Count(); i++)
		_constantBuffers.Add({ meta->CB[i].Slot, false, 0 });

	// [Output] Write version number
	output->WriteInt32(1);

	// [Output] Write amount of shaders
	output->WriteInt32(shadersCount);

	// Compile all shaders
	if (CompileShaders(context))
		return true;

	// [Output] Constant Buffers
	{
		int32 cbsCount = _constantBuffers.Count();
		ASSERT(cbsCount == meta->CB.Count());

		// Find maximum used slot index
		byte maxCbSlot = 0;
		for (int32 i = 0; i < cbsCount; i++)
		{
			maxCbSlot = Math::Max(maxCbSlot, _constantBuffers[i].Slot);
		}

		output->WriteByte(static_cast<byte>(cbsCount));
		output->WriteByte(maxCbSlot);
		// TODO: do we still need to serialize max cb slot?

		for (int32 i = 0; i < cbsCount; i++)
		{
			output->WriteByte(_constantBuffers[i].Slot);
			output->WriteUint32(_constantBuffers[i].Size);
		}
	}

	return false;
}

#endif
