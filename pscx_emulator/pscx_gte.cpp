#include <cassert>
#include <climits>

#include "pscx_gte.h"
#include "pscx_common.h"

Matrix fromMatrixCommand(uint32_t command)
{
	Matrix matrix;
	switch ((command >> 17) & 3)
	{
	case 0:
		matrix = Matrix::MATRIX_ROTATION;
		break;
	case 1:
		matrix = Matrix::MATRIX_LIGHT;
		break;
	case 2:
		matrix = Matrix::MATRIX_COLOR;
		break;
	case 3:
		matrix = Matrix::MATRIX_INVALID;
		break;
	}
	return matrix;
}

ControlVector fromControlVectorCommand(uint32_t command)
{
	ControlVector controlVector;
	switch ((command >> 13) & 3)
	{
	case 0:
		controlVector = ControlVector::CONTROL_VECTOR_TRANSLATION;
		break;
	case 1:
		controlVector = ControlVector::CONTROL_VECTOR_BACKGROUND_COLOR;
		break;
	case 2:
		controlVector = ControlVector::CONTROL_VECTOR_FAR_COLOR;
		break;
	case 3:
		controlVector = ControlVector::CONTROL_VECTOR_ZERO;
		break;
	}
	return controlVector;
}

// ********************** Gte implementation **********************
uint32_t Gte::getControl(uint32_t reg) const
{
	switch (reg)
	{
	case 0:
	{
		auto matrix = m_matrices[Matrix::MATRIX_ROTATION];

		uint32_t value0 = (uint16_t)matrix[0][0];
		uint32_t value1 = (uint16_t)matrix[0][1];

		return value0 | (value1 << 16);
	}
	case 1:
	{
		auto matrix = m_matrices[Matrix::MATRIX_ROTATION];

		uint32_t value0 = (uint16_t)matrix[0][2];
		uint32_t value1 = (uint16_t)matrix[1][0];

		return value0 | (value1 << 16);
	}
	case 2:
	{
		auto matrix = m_matrices[Matrix::MATRIX_ROTATION];

		uint32_t value0 = (uint16_t)matrix[1][1];
		uint32_t value1 = (uint16_t)matrix[1][2];

		return value0 | (value1 << 16);
	}
	case 3:
	{
		auto matrix = m_matrices[Matrix::MATRIX_ROTATION];

		uint32_t value0 = (uint16_t)matrix[2][0];
		uint32_t value1 = (uint16_t)matrix[2][1];

		return value0 | (value1 << 16);
	}
	case 4:
	{
		auto matrix = m_matrices[Matrix::MATRIX_ROTATION];
		return (uint16_t)matrix[2][2];
	}
	case 5:
	case 6:
	case 7:
	{
		auto controlVector = m_controlVectors[ControlVector::CONTROL_VECTOR_TRANSLATION];
		return controlVector[reg - 5];
	}
	case 8:
	{
		auto matrix = m_matrices[Matrix::MATRIX_LIGHT];

		uint32_t value0 = (uint16_t)matrix[0][0];
		uint32_t value1 = (uint16_t)matrix[0][1];

		return value0 | (value1 << 16);
	}
	case 9:
	{
		auto matrix = m_matrices[Matrix::MATRIX_LIGHT];

		uint32_t value0 = (uint16_t)matrix[0][2];
		uint32_t value1 = (uint16_t)matrix[1][0];

		return value0 | (value1 << 16);
	}
	case 10:
	{
		auto matrix = m_matrices[Matrix::MATRIX_LIGHT];

		uint32_t value0 = (uint16_t)matrix[1][1];
		uint32_t value1 = (uint16_t)matrix[1][2];

		return value0 | (value1 << 16);
	}
	case 11:
	{
		auto matrix = m_matrices[Matrix::MATRIX_LIGHT];

		uint32_t value0 = (uint16_t)matrix[2][0];
		uint32_t value1 = (uint16_t)matrix[2][1];

		return value0 | (value1 << 16);
	}
	case 12:
	{
		auto matrix = m_matrices[Matrix::MATRIX_LIGHT];
		return (uint16_t)matrix[2][2];
	}
	case 13:
	case 14:
	case 15:
	{
		auto controlVector = m_controlVectors[ControlVector::CONTROL_VECTOR_BACKGROUND_COLOR];
		return controlVector[reg - 13];
	}
	case 16:
	{
		auto matrix = m_matrices[Matrix::MATRIX_COLOR];

		uint32_t value0 = (uint16_t)matrix[0][0];
		uint32_t value1 = (uint16_t)matrix[0][1];

		return value0 | (value1 << 16);
	}
	case 17:
	{
		auto matrix = m_matrices[Matrix::MATRIX_COLOR];

		uint32_t value0 = (uint16_t)matrix[0][2];
		uint32_t value1 = (uint16_t)matrix[1][0];

		return value0 | (value1 << 16);
	}
	case 18:
	{
		auto matrix = m_matrices[Matrix::MATRIX_COLOR];

		uint32_t value0 = (uint16_t)matrix[1][1];
		uint32_t value1 = (uint16_t)matrix[1][2];

		return value0 | (value1 << 16);
	}
	case 19:
	{
		auto matrix = m_matrices[Matrix::MATRIX_COLOR];

		uint32_t value0 = (uint16_t)matrix[2][0];
		uint32_t value1 = (uint16_t)matrix[2][1];

		return value0 | (value1 << 16);
	}
	case 20:
	{
		auto matrix = m_matrices[Matrix::MATRIX_COLOR];
		return (uint16_t)matrix[2][2];
	}
	case 21:
	case 22:
	case 23:
	{
		auto controlVector = m_controlVectors[ControlVector::CONTROL_VECTOR_FAR_COLOR];
		return controlVector[reg - 21];
	}
	case 24:
		return m_offsetX;
	case 25:
		return m_offsetY;
	// m_projectionPlaneDistance is read back as a signed even though it is unsigned.
	case 26:
		return (int16_t)m_projectionPlaneDistance;
	case 27:
		return m_depthQueingCoefficient;
	case 28:
		return m_depthQueingOffset;
	case 29:
		return m_zScaleFactor3;
	case 30:
		return m_zScaleFactor4;
	case 31:
		return m_overflowFlags;
	default:
		assert(0, "Unhandled GTE control register");
	}
	// Shouldn't be executed.
	return 0x0;
}

void Gte::setControl(uint32_t reg, uint32_t value)
{
	LOG("Set GTE control reg = 0x" << std::hex << reg << " value = 0x" << value);

	switch (reg)
	{
	case 0:
	{
		auto rotationMatrix = m_matrices[Matrix::MATRIX_ROTATION];
		rotationMatrix[0][0] = (int16_t)value;
		rotationMatrix[0][1] = (int16_t)(value >> 16);
		break;
	}
	case 1:
	{
		auto rotationMatrix = m_matrices[Matrix::MATRIX_ROTATION];
		rotationMatrix[0][2] = (int16_t)value;
		rotationMatrix[1][0] = (int16_t)(value >> 16);
		break;
	}
	case 2:
	{
		auto rotationMatrix = m_matrices[Matrix::MATRIX_ROTATION];
		rotationMatrix[1][1] = (int16_t)value;
		rotationMatrix[1][2] = (int16_t)(value >> 16);
		break;
	}
	case 3:
	{
		auto rotationMatrix = m_matrices[Matrix::MATRIX_ROTATION];
		rotationMatrix[2][0] = (int16_t)value;
		rotationMatrix[2][1] = (int16_t)(value >> 16);
		break;
	}
	case 4:
	{
		auto rotationMatrix = m_matrices[Matrix::MATRIX_ROTATION];
		rotationMatrix[2][2] = (int16_t)value;
		break;
	}
	case 5:
	case 6:
	case 7:
	{
		auto translationVector = m_controlVectors[ControlVector::CONTROL_VECTOR_TRANSLATION];
		translationVector[reg - 5] = (int32_t)value;
		break;
	}
	case 8:
	{
		auto lightSourceMatrix = m_matrices[Matrix::MATRIX_LIGHT];
		lightSourceMatrix[0][0] = (int16_t)value;
		lightSourceMatrix[0][1] = (int16_t)(value >> 16);
		break;
	}
	case 9:
	{
		auto lightSourceMatrix = m_matrices[Matrix::MATRIX_LIGHT];
		lightSourceMatrix[0][2] = (int16_t)value;
		lightSourceMatrix[1][0] = (int16_t)(value >> 16);
		break;
	}
	case 10:
	{
		auto lightSourceMatrix = m_matrices[Matrix::MATRIX_LIGHT];
		lightSourceMatrix[1][1] = (int16_t)value;
		lightSourceMatrix[1][2] = (int16_t)(value >> 16);
		break;
	}
	case 11:
	{
		auto lightSourceMatrix = m_matrices[Matrix::MATRIX_LIGHT];
		lightSourceMatrix[2][0] = (int16_t)value;
		lightSourceMatrix[2][1] = (int16_t)(value >> 16);
		break;
	}
	case 12:
	{
		auto lightSourceMatrix = m_matrices[Matrix::MATRIX_LIGHT];
		lightSourceMatrix[2][2] = (int16_t)value;
		break;
	}
	case 13:
	case 14:
	case 15:
	{
		auto backgroundColor = m_controlVectors[ControlVector::CONTROL_VECTOR_BACKGROUND_COLOR];
		backgroundColor[reg - 13] = (int32_t)value;
		break;
	}
	case 16:
	{
		auto lightColorMatrix = m_matrices[Matrix::MATRIX_COLOR];
		lightColorMatrix[0][0] = (int16_t)value;
		lightColorMatrix[0][1] = (int16_t)(value >> 16);
		break;
	}
	case 17:
	{
		auto lightColorMatrix = m_matrices[Matrix::MATRIX_COLOR];
		lightColorMatrix[0][2] = (int16_t)value;
		lightColorMatrix[1][0] = (int16_t)(value >> 16);
		break;
	}
	case 18:
	{
		auto lightColorMatrix = m_matrices[Matrix::MATRIX_COLOR];
		lightColorMatrix[1][1] = (int16_t)value;
		lightColorMatrix[1][2] = (int16_t)(value >> 16);
		break;
	}
	case 19:
	{
		auto lightColorMatrix = m_matrices[Matrix::MATRIX_COLOR];
		lightColorMatrix[2][0] = (int16_t)value;
		lightColorMatrix[2][1] = (int16_t)(value >> 16);
		break;
	}
	case 20:
	{
		auto lightColorMatrix = m_matrices[Matrix::MATRIX_COLOR];
		lightColorMatrix[2][2] = (int16_t)value;
		break;
	}
	case 21:
	case 22:
	case 23:
	{
		auto farColor = m_controlVectors[ControlVector::CONTROL_VECTOR_FAR_COLOR];
		farColor[reg - 21] = (int32_t)value;
		break;
	}
	case 24:
	{
		m_offsetX = (int32_t)value;
		break;
	}
	case 25:
	{
		m_offsetY = (int32_t)value;
		break;
	}
	case 26:
	{
		m_projectionPlaneDistance = (uint16_t)value;
		break;
	}
	case 27:
	{
		m_depthQueingCoefficient = (int16_t)value;
		break;
	}
	case 28:
	{
		m_depthQueingOffset = (int32_t)value;
		break;
	}
	case 29:
	{
		m_zScaleFactor3 = (int16_t)value;
		break;
	}
	case 30:
	{
		m_zScaleFactor4 = (int16_t)value;
		break;
	}
	case 31:
	{
		m_overflowFlags = value & 0x7ffff00;
		m_overflowFlags |= ((value & 0x7f87e000) << 31);
		break;
	}
	default:
	{
		assert(0, "Unhandled GTE control register");
		break;
	}
	}
}

uint32_t Gte::getData(uint32_t reg) const
{
	switch (reg)
	{
	case 0:
	{
		uint32_t value0 = (uint16_t)m_vectors[0][0];
		uint32_t value1 = (uint16_t)m_vectors[0][1];
		return value0 | (value1 << 16);
	}
	case 1:
	{
		return m_vectors[0][2];
	}
	case 2:
	{
		uint32_t value0 = (uint16_t)m_vectors[1][0];
		uint32_t value1 = (uint16_t)m_vectors[1][1];
		return value0 | (value1 << 16);
	}
	case 3:
	{
		return m_vectors[1][2];
	}
	case 4:
	{
		uint32_t value0 = (uint16_t)m_vectors[2][0];
		uint32_t value1 = (uint16_t)m_vectors[2][1];
		return value0 | (value1 << 16);
	}
	case 5:
	{
		return m_vectors[2][2];
	}
	case 6:
	{
		return (uint32_t)m_rgbColor[0] | ((uint32_t)m_rgbColor[1] << 8) | ((uint32_t)m_rgbColor[2] << 16) | ((uint32_t)m_rgbColor[3] << 24);
	}
	case 7:
	{
		return m_zAverage;
	}
	case 8:
	{
		return m_accumulatorsSignedHalfwords[0];
	}
	case 9:
	{
		return m_accumulatorsSignedHalfwords[1];
	}
	case 10:
	{
		return m_accumulatorsSignedHalfwords[2];
	}
	case 11:
	{
		return m_accumulatorsSignedHalfwords[3];
	}
	case 12:
	{
		auto xy = m_xyFifo[0];
		return (uint32_t)xy.first | ((uint32_t)xy.second << 16);
	}
	case 13:
	{
		auto xy = m_xyFifo[1];
		return (uint32_t)xy.first | ((uint32_t)xy.second << 16);
	}
	case 14:
	{
		auto xy = m_xyFifo[2];
		return (uint32_t)xy.first | ((uint32_t)xy.second << 16);
	}
	case 15:
	{
		auto xy = m_xyFifo[3];
		return (uint32_t)xy.first | ((uint32_t)xy.second << 16);
	}
	case 16:
	{
		return m_zFifo[0];
	}
	case 17:
	{
		return m_zFifo[1];
	}
	case 18:
	{
		return m_zFifo[2];
	}
	case 19:
	{
		return m_zFifo[3];
	}
	case 22:
	{
		return (uint32_t)m_rgbFifo[2][0] | ((uint32_t)m_rgbFifo[2][1] << 8) | ((uint32_t)m_rgbFifo[2][2] << 16) | ((uint32_t)m_rgbFifo[2][3] << 24);
	}
	case 24:
	{
		return m_accumulatorsSignedWord[0];
	}
	case 25:
	{
		return m_accumulatorsSignedWord[1];
	}
	case 26:
	{
		return m_accumulatorsSignedWord[2];
	}
	case 27:
	{
		return m_accumulatorsSignedWord[3];
	}
	case 28:
	case 29:
	{
		auto saturate = [](int32_t value) -> uint32_t { return value < 0 ? 0 : (value > 0x1f ? 0x1f : (uint32_t)value); };

		uint32_t value0 = saturate(m_accumulatorsSignedWord[1] >> 7);
		uint32_t value1 = saturate(m_accumulatorsSignedWord[2] >> 7);
		uint32_t value2 = saturate(m_accumulatorsSignedWord[3] >> 7);

		return value0 | (value1 << 5) | (value2 << 10);
	}
	case 31:
	{
		return m_leadingZerosCountResult;
	}
	default:
		assert(0, "Unhandled GTE data register");
	}
	// Shouldn't be executed.
}

void Gte::setData(uint32_t reg, uint32_t value)
{
	LOG("Set GTE data reg = 0x" << std::hex << reg << " value = 0x" << value);

	switch (reg)
	{
	case 0:
	{
		m_vectors[0][0] = (int16_t)value;
		m_vectors[0][1] = (int16_t)(value >> 16);
		break;
	}
	case 1:
	{
		m_vectors[0][2] = (int16_t)value;
		break;
	}
	case 2:
	{
		m_vectors[1][0] = (int16_t)value;
		m_vectors[1][1] = (int16_t)(value >> 16);
		break;
	}
	case 3:
	{
		m_vectors[1][2] = (int16_t)value;
		break;
	}
	case 4:
	{
		m_vectors[2][0] = (int16_t)value;
		m_vectors[2][1] = (int16_t)(value >> 16);
		break;
	}
	case 5:
	{
		m_vectors[2][2] = (int16_t)value;
		break;
	}
	case 6:
	{
		m_rgbColor[0] = (uint8_t)value;
		m_rgbColor[1] = (uint8_t)(value >> 8);
		m_rgbColor[2] = (uint8_t)(value >> 16);
		m_rgbColor[3] = (uint8_t)(value >> 24);
		break;
	}
	case 7:
	{
		m_zAverage = (uint16_t)value;
		break;
	}
	case 8:
	{
		m_accumulatorsSignedHalfwords[0] = (int16_t)value;
		break;
	}
	case 9:
	{
		m_accumulatorsSignedHalfwords[1] = (int16_t)value;
		break;
	}
	case 10:
	{
		m_accumulatorsSignedHalfwords[2] = (int16_t)value;
		break;
	}
	case 11:
	{
		m_accumulatorsSignedHalfwords[3] = (int16_t)value;
		break;
	}
	case 12:
	{
		int16_t value0 = (int16_t)value;
		int16_t value1 = (int16_t)(value >> 16);
		m_xyFifo[0] = std::make_pair(value0, value1);
		break;
	}
	case 13:
	{
		int16_t value0 = (int16_t)value;
		int16_t value1 = (int16_t)(value >> 16);
		m_xyFifo[1] = std::make_pair(value0, value1);
		break;
	}
	case 14:
	{
		int16_t value0 = (int16_t)value;
		int16_t value1 = (int16_t)(value >> 16);
		m_xyFifo[2] = std::make_pair(value0, value1);
		m_xyFifo[3] = std::make_pair(value0, value1);
		break;
	}
	case 16:
	{
		m_zFifo[0] = (uint16_t)value;
		break;
	}
	case 17:
	{
		m_zFifo[1] = (uint16_t)value;
		break;
	}
	case 18:
	{
		m_zFifo[2] = (uint16_t)value;
		break;
	}
	case 19:
	{
		m_zFifo[3] = (uint16_t)value;
		break;
	}
	case 22:
	{
		m_rgbFifo[2][0] = (uint8_t)value;
		m_rgbFifo[2][1] = (uint8_t)(value >> 8);
		m_rgbFifo[2][2] = (uint8_t)(value >> 16);
		m_rgbFifo[2][3] = (uint8_t)(value >> 24);
		break;
	}
	case 24:
	{
		m_accumulatorsSignedWord[0] = (int32_t)value;
		break;
	}
	case 25:
	{
		m_accumulatorsSignedWord[1] = (int32_t)value;
		break;
	}
	case 26:
	{
		m_accumulatorsSignedWord[2] = (int32_t)value;
		break;
	}
	case 27:
	{
		m_accumulatorsSignedWord[3] = (int32_t)value;
		break;
	}
	case 30:
	{
		m_leadingZerosCountSign = value;

		// If "value" is negative, we count the leading ones,
		// otherwise we count the leading zeroes.
		uint16_t leadingZerosCountValue = (value >> 31) & 1 ? ~value : value;
		m_leadingZerosCountResult = calculateLeadingZeros(leadingZerosCountValue);

		break;
	}
	case 31:
	{
		LOG("Write to read-only GTE data register 31");
		break;
	}
	default:
	{
		assert(0, "Unhandled GTE data register");
		break;
	}
	}
}

void Gte::command(uint32_t command)
{
	uint32_t opcode = command & 0x3f;
	CommandConfig config = CommandConfig::fromCommand(command);

	// Clear flags prior to command execution.
	m_overflowFlags = 0x0;

	switch (opcode)
	{
	case 0x06:
		cmdNormalClip();
		break;
	case 0x12:
		cmdMultiplyVectorByMatrixAndAddVector(config);
		break;
	case 0x13:
		cmdNormalColorDepthSingleVector(config);
		break;
	case 0x2d:
		cmdAverageSingleZ3();
		break;
	case 0x30:
		cmdRotateTranslatePerspectiveTransform(config);
		break;
	default:
		assert(0, "Unhandled GTE opcode");
	}

	// Update the flags MSB: OR together bits [30:23] + [18:13].
	bool msb = m_overflowFlags & 0x7f87e000;
	m_overflowFlags |= (((uint32_t)msb) << 31);
}

void Gte::cmdNormalClip()
{
	std::pair<int16_t, int16_t> xy0 = m_xyFifo[0];
	std::pair<int16_t, int16_t> xy1 = m_xyFifo[1];
	std::pair<int16_t, int16_t> xy2 = m_xyFifo[2];
	
	// Convert to 32 bits.
	int32_t x0 = xy0.first, y0 = xy0.second;
	int32_t x1 = xy1.first, y1 = xy1.second;
	int32_t x2 = xy2.first, y2 = xy2.second;

	int32_t a = x0 * (y1 - y2);
	int32_t b = x1 * (y2 - y0);
	int32_t c = x2 * (y0 - y1);

	// Compute the sum in 64 bits to detect overflows.
	int64_t sum = (int64_t)a + (int64_t)b + (int64_t)c;
	m_accumulatorsSignedWord[0] = converti64Toi32Result(sum);
}

void Gte::cmdMultiplyVectorByMatrixAndAddVector(const CommandConfig& config)
{
	multiplyMatrixByVector(config, config.getMatrix(), config.getVectorMul(), config.getVectorAdd());
}

void Gte::cmdNormalColorDepthSingleVector(const CommandConfig& config)
{
	doNormalColorDepthTransformation(config, 0x0);
}

void Gte::cmdAverageSingleZ3()
{
	uint32_t z1 = m_zFifo[1];
	uint32_t z2 = m_zFifo[2];
	uint32_t z3 = m_zFifo[3];

	uint32_t sum = z1 + z2 + z3;

	// The average factor should generally be set to 1/3 of the
	// ordering table size. So, for instance, for a table of 1024
	// entries it should be set at 341 to use the full table granularity.
	int64_t zScaleFactor3 = m_zScaleFactor3;
	int64_t average = zScaleFactor3 * sum;

	m_accumulatorsSignedWord[0] = converti64Toi32Result(average);
	m_zAverage = converti64TozAverage(average);
}

void Gte::cmdRotateTranslatePerspectiveTransform(const CommandConfig& commandConfig)
{
	// Transform three vectors at once.
	doRotateTranslatePerspectiveTransform(commandConfig, 0x0);
	doRotateTranslatePerspectiveTransform(commandConfig, 0x1);
	// We do the depth queuing on the last vector.
	uint32_t projectionFactor = doRotateTranslatePerspectiveTransform(commandConfig, 0x2);
	depthQueuing(projectionFactor);
}

void Gte::doNormalColorDepthTransformation(const CommandConfig& config, uint8_t vectorIndex)
{
	multiplyMatrixByVector(config, Matrix::MATRIX_LIGHT, vectorIndex, ControlVector::CONTROL_VECTOR_ZERO);

	// Use the custom 4th vector to store the intermediate values.
	// This vector doesn't exist in the real hardware ( at least not in the registers ),
	// it's just a hack to make the code simpler.
	m_vectors[3][0] = m_accumulatorsSignedHalfwords[1];
	m_vectors[3][1] = m_accumulatorsSignedHalfwords[2];
	m_vectors[3][2] = m_accumulatorsSignedHalfwords[3];

	multiplyMatrixByVector(config, Matrix::MATRIX_COLOR, 3, ControlVector::CONTROL_VECTOR_BACKGROUND_COLOR);

	for (size_t i = 0; i < 3; ++i)
	{
		int64_t farColor = ((int64_t)m_controlVectors[ControlVector::CONTROL_VECTOR_FAR_COLOR][i]) << 12;
		int32_t accumulatorsSignedHalfword = m_accumulatorsSignedHalfwords[i + 1];
		int32_t color = ((int32_t)m_rgbColor[i]) << 4;

		int64_t shading = color * (int64_t)accumulatorsSignedHalfword;
		int64_t product = farColor - shading;

		int32_t temporary = converti64Toi32Result(product) >> config.getShift();
		int64_t accumulatorsSignedHalfword0 = m_accumulatorsSignedHalfwords[0];
		int64_t saturateResult = truncatei32Toi16Saturate(CommandConfig::fromCommand(0x0), i, temporary);

		int32_t result = converti64Toi32Result(shading + accumulatorsSignedHalfword0 * saturateResult);
		m_accumulatorsSignedWord[i + 1] = result >> config.getShift();
	}

	convertAccumulatorsSignedWordsToHalfwords(config);
	convertAccumulatorsSignedWordsToRGBFifo();
}

void Gte::multiplyMatrixByVector(const CommandConfig& config, Matrix matrix, uint8_t vectorIndex, ControlVector controlVector)
{
	assert(matrix != Matrix::MATRIX_INVALID, "GTE multiplication with invalid matrix");
	assert(controlVector != ControlVector::CONTROL_VECTOR_FAR_COLOR, "GTE multiplication with far color vector");

	// Iterate over the matrix rows.
	for (size_t row = 0; row < 3; row++)
	{
		// Firstly, the controlVector is added to the result.
		int64_t result = ((int64_t)m_controlVectors[controlVector][row]) << 12;

		// Iterate over the matrix columns.
		for (size_t column = 0; column < 3; column++)
		{
			int32_t vectorComponent = m_vectors[vectorIndex][column];
			int32_t matrixComponent = m_matrices[matrix][row][column];

			int32_t product = vectorComponent * matrixComponent;

			// The operation is done using 44 bit signed arithmetics.
			result = truncatei64Toi44(column, result + (int64_t)product);
		}

		// Store the result in the accumulator.
		m_accumulatorsSignedWord[row + 1] = result >> config.getShift();
	}

	convertAccumulatorsSignedWordsToHalfwords(config);
}

void Gte::convertAccumulatorsSignedWordsToHalfwords(const CommandConfig& commandConfig)
{
	m_accumulatorsSignedHalfwords[1] = truncatei32Toi16Saturate(commandConfig, 0x0, m_accumulatorsSignedWord[1]);
	m_accumulatorsSignedHalfwords[2] = truncatei32Toi16Saturate(commandConfig, 0x1, m_accumulatorsSignedWord[2]);
	m_accumulatorsSignedHalfwords[3] = truncatei32Toi16Saturate(commandConfig, 0x2, m_accumulatorsSignedWord[3]);
}

void Gte::convertAccumulatorsSignedWordsToRGBFifo()
{
	auto accumulatorsSignedWordsToColor = [](Gte* gte, int32_t accumulatorsSignedWord, uint8_t which) -> uint8_t
	{
		int32_t signedWord = accumulatorsSignedWord >> 4;
		if (signedWord < 0x0)
		{
			gte->setFlag(21 - which);
			return 0x0;
		}
		else if (signedWord > 0xff)
		{
			gte->setFlag(21 - which);
			return 0xff;
		}
		return signedWord;
	};

	uint8_t r = accumulatorsSignedWordsToColor(this, m_accumulatorsSignedWord[1], 0x0);
	uint8_t g = accumulatorsSignedWordsToColor(this, m_accumulatorsSignedWord[2], 0x1);
	uint8_t b = accumulatorsSignedWordsToColor(this, m_accumulatorsSignedWord[3], 0x2);

	memcpy(m_rgbFifo[0], m_rgbFifo[1], sizeof(m_rgbFifo[0]));
	memcpy(m_rgbFifo[1], m_rgbFifo[2], sizeof(m_rgbFifo[0]));
	m_rgbFifo[2][0] = r, m_rgbFifo[2][1] = g, m_rgbFifo[2][2] = b, m_rgbFifo[2][3] = m_rgbColor[3];
}

uint32_t Gte::doRotateTranslatePerspectiveTransform(const CommandConfig& commandConfig, size_t vectorIndex)
{
	// The computed Z coordinate with unconditional 12 bit shift applied.
	int32_t zShifted(0x0);

	// Step 1: to compute "translationVector + vector * rotationMatrix" and store the 32 bit
	// result in accumulators words 0, 1 and 2.
	// Iterate over the matrix rows.
	for (size_t row = 0; row < 3; ++row)
	{
		// Start with the translation. Convert translation vector component
		// from i32 to i64 with 12 fractional bits.
		int64_t vectorComponent = ((int64_t)m_controlVectors[ControlVector::CONTROL_VECTOR_TRANSLATION][row]) << 12;

		// Iterate over the rotation matrix columns.
		for (size_t column = 0; column < 3; ++column)
		{
			int32_t vector = m_vectors[vectorIndex][column];
			int32_t matrix = m_matrices[Matrix::MATRIX_ROTATION][row][column];

			int32_t rotation = vector * matrix;

			// The operation is done using 44 bit signed arithmetics.
			vectorComponent = truncatei64Toi44(column, (int64_t)(vectorComponent + rotation));
		}

		// Store the result in the accumulator.
		m_accumulatorsSignedWord[row + 1] = vectorComponent >> commandConfig.getShift();

		// The last result will be Z, overwrite it each time and the last one will be the good one.
		zShifted = vectorComponent >> 12;
	}

	// Step 2: we take the 32 bit camera coordinates in accumulator signed words and convert
	// them to 16 bit values in the accumulator signed halfwords vector, saturating them in case of an overflow.

	// 16 bit clamped "X" coordinate.
	m_accumulatorsSignedHalfwords[1] = truncatei32Toi16Saturate(commandConfig, 0x0, m_accumulatorsSignedWord[0x1]);
	// 16 bit clamped "Y" coordinate.
	m_accumulatorsSignedHalfwords[2] = truncatei32Toi16Saturate(commandConfig, 0x1, m_accumulatorsSignedWord[0x2]);
	// 16 bit clamped "Z" coordinate.
	if (zShifted > SHRT_MAX || zShifted < SHRT_MIN)
		setFlag(22);

	// Clamp the value normally.
	int32_t minValue(0x0);
	if (!commandConfig.isClampNegative())
		minValue = SHRT_MIN;

	int32_t accumulatorsSignedWord = m_accumulatorsSignedWord[0x3];
	m_accumulatorsSignedHalfwords[0x3] = accumulatorsSignedWord < minValue ? minValue : (accumulatorsSignedWord > SHRT_MAX ? SHRT_MAX : accumulatorsSignedWord);

	// Step 3: convert the shifted Z value to a 16 bit unsigned integer ( with saturation ) and puh it onto the Z FIFO.
	uint16_t zSaturated(zShifted);
	if (zShifted < 0)
	{
		setFlag(18);
		zSaturated = 0x0;
	}
	else if (zShifted > USHRT_MAX)
	{
		setFlag(18);
		zSaturated = USHRT_MAX;
	}

	// Push Z Saturated onto the Z FIFO.
	m_zFifo[0] = m_zFifo[1];
	m_zFifo[1] = m_zFifo[2];
	m_zFifo[2] = m_zFifo[3];
	m_zFifo[3] = zSaturated;

	// Step 4: perspective projection against the screen plane.
	// Compute the projection factor by dividing projection plane
	// distance by the Z coordinate.

	// Projection factor: 1.16 unsigned.
	uint32_t projectionFactor(0x0);
	if (zSaturated > m_projectionPlaneDistance / 2)
	{
		// GTE-specific division algorithm for distance / z.
		// Returns a saturated 17 bit value.
		projectionFactor = divide(m_projectionPlaneDistance, zSaturated);
	}
	else
	{
		// If the Z coordinate is smaller than or equal to half
		// the projection plane distance, then we clip it.
		setFlag(17);
		projectionFactor = 0x1ffff;
	}

	// Work in 64 bits to detect overflows.
	int64_t factor = projectionFactor;
	int64_t x = m_accumulatorsSignedHalfwords[0x1];
	int64_t y = m_accumulatorsSignedHalfwords[0x2];
	int64_t offsetX = m_offsetX;
	int64_t offsetY = m_offsetY;

	// Project X and Y onto the plane.
	int32_t screenPosX = converti64Toi32Result(x * factor + offsetX) >> 16;
	int32_t screenPosY = converti64Toi32Result(y * factor + offsetY) >> 16;

	// Push onto the XY FIFO.
	m_xyFifo[0x3] = std::make_pair(truncatei32Toi11Saturate(0x0, screenPosX),
		truncatei32Toi11Saturate(0x1, screenPosY));
	m_xyFifo[0x0] = m_xyFifo[0x1];
	m_xyFifo[0x1] = m_xyFifo[0x2];
	m_xyFifo[0x2] = m_xyFifo[0x3];

	return projectionFactor;
}

void Gte::depthQueuing(uint32_t projectionFactor)
{
	// Work in 64 bits to detect overflows.
	int64_t factor = projectionFactor;
	int64_t depthQueingCoefficient = m_depthQueingCoefficient;
	int64_t depthQueingOffset = m_depthQueingOffset;

	int64_t depth = depthQueingOffset + depthQueingCoefficient * factor;
	m_accumulatorsSignedWord[0x0] = converti64Toi32Result(depth);

	// Compute 16 bit accumulate signed halfword value.
	depth >>= 12;
	m_accumulatorsSignedHalfwords[0x0] = depth;
	if (depth < 0)
	{
		setFlag(12);
		m_accumulatorsSignedHalfwords[0x0] = 0x0;
	}
	else if (depth > 4096)
	{
		setFlag(12);
		m_accumulatorsSignedHalfwords[0x0] = 4096;
	}
}

void Gte::setFlag(uint8_t bit)
{
	m_overflowFlags |= (1 << bit);
}

int64_t Gte::truncatei64Toi44(uint8_t flag, int64_t value)
{
	if (value > 0x7ffffffffff)
		setFlag(30 - flag);
	else if (value < -0x80000000000)
		setFlag(27 - flag);

	return (value << 20) >> 20;
}

int16_t Gte::truncatei32Toi16Saturate(const CommandConfig& config, uint8_t flag, int32_t value)
{
	int32_t minValue(0x0);
	if (!config.isClampNegative())
		minValue = SHRT_MIN;

	if (value > SHRT_MAX)
	{
		setFlag(24 - flag);
		// Clamp to max.
		return SHRT_MAX;
	}
	else if (value < minValue)
	{
		setFlag(24 - flag);
		// Clamp to min.
		return minValue;
	}
	return value;
}

int16_t Gte::truncatei32Toi11Saturate(uint8_t flag, int32_t value)
{
	if (value < -0x400)
	{
		setFlag(14 - flag);
		return -0x400;
	}
	else if (value > 0x3ff)
	{
		setFlag(14 - flag);
		return 0x3ff;
	}
	return value;
}

int32_t Gte::converti64Toi32Result(int64_t value)
{
	if (value < -0x80000000000)
		setFlag(15);
	else if (value > 0x7ffffffffff)
		setFlag(16);

	return value;
}

uint16_t Gte::converti64TozAverage(int64_t average)
{
	int64_t value = average >> 12;
	if (value < 0)
	{
		setFlag(18);
		return 0x0;
	}
	else if (value > 0xffff)
	{
		setFlag(18);
		return 0xffff;
	}
	return value;
}
