#pragma once

#include <cstdint>
#include <cstring>
#include <utility>

#include "pscx_gte_divider.h"

// The three matrices used by the GTE operations.
enum Matrix
{
	MATRIX_ROTATION = 0,
	MATRIX_LIGHT = 1,
	MATRIX_COLOR = 2,
	// Bogus entry that can be configured when the command bits
	// [18; 17] are equal to 0b11 and which results in garbage operations.
	MATRIX_INVALID
};
Matrix fromMatrixCommand(uint32_t command);
// index ???

// The three control vectors used by the various GTE operations.
enum ControlVector
{
	CONTROL_VECTOR_TRANSLATION = 0,
	CONTROL_VECTOR_BACKGROUND_COLOR = 1,
	CONTROL_VECTOR_FAR_COLOR = 2,
	// Pseudo-vector always equal to 0.
	CONTROL_VECTOR_ZERO = 3
};
ControlVector fromControlVectorCommand(uint32_t command);
// index ???

// Decoded command fields in the GTE command instructions. Meaning varies
// depending on the command used.
struct CommandConfig
{
	CommandConfig(uint8_t shift, bool clampNegative, Matrix matrix, uint8_t vectorMul, ControlVector vectorAdd) :
		m_shift(shift),
		m_clampNegative(clampNegative),
		m_matrix(matrix),
		m_vectorMul(vectorMul),
		m_vectorAdd(vectorAdd)
	{}

	static CommandConfig fromCommand(uint32_t command)
	{
		uint8_t shift = (command & (1 << 19)) ? 0xc : 0x0;
		bool clampNegative = command & (1 << 10);
		uint8_t vectorIndex = (command >> 15) & 3;
		return CommandConfig(shift, clampNegative, fromMatrixCommand(command), vectorIndex, fromControlVectorCommand(command));
	}

	uint8_t getShift() const { return m_shift; }
	bool isClampNegative() const { return m_clampNegative; }
	Matrix getMatrix() const { return m_matrix; }
	uint8_t getVectorMul() const { return m_vectorMul; }
	ControlVector getVectorAdd() const { return m_vectorAdd; }

private:
	uint8_t m_shift;
	bool m_clampNegative;
	Matrix m_matrix;
	uint8_t m_vectorMul;
	ControlVector m_vectorAdd;
};

// Geometry transform engine.
struct Gte
{
	Gte() :
		m_offsetX(0x0),
		m_offsetY(0x0),
		m_projectionPlaneDistance(0x0),
		m_depthQueingCoefficient(0x0),
		m_depthQueingOffset(0x0),
		m_zScaleFactor3(0x0),
		m_zScaleFactor4(0x0),
		m_overflowFlags(0x0),
		m_zAverage(0x0),
		m_leadingZerosCountSign(0x0),
		// If "leading zeros count sign" equals to 0, then
		// "leading zeros count result" is 31. "Leading zeros count result" can
		// never be equal to 0.
		m_leadingZerosCountResult(32)
	{
		// Matrices and vectors initializations.
		memset(m_matrices, 0x0, sizeof(m_matrices));
		memset(m_controlVectors, 0x0, sizeof(m_controlVectors));
		memset(m_vectors, 0x0, sizeof(m_vectors));
		memset(m_accumulatorsSignedWord, 0x0, sizeof(m_accumulatorsSignedWord));
		memset(m_rgbColor, 0x0, sizeof(m_rgbColor));
		memset(m_accumulatorsSignedHalfwords, 0x0, sizeof(m_accumulatorsSignedHalfwords));

		for (size_t i = 0; i < 4; ++i)
			m_xyFifo[i] = std::make_pair(0x0, 0x0);

		memset(m_zFifo, 0x0, sizeof(m_zFifo));
		memset(m_rgbFifo, 0x0, sizeof(m_rgbFifo));
	}

	// Return the value of one of the control registers. Used by the CFC2 opcode.
	uint32_t getControl(uint32_t reg) const;
	// Store value in one of the "control" registers. Used by the CTC2 opcode.
	void setControl(uint32_t reg, uint32_t value);
	// Return the value of one of the data registers. Used by the MFC2 and SWC2 opcodes.
	uint32_t getData(uint32_t reg) const;
	// Store value in one of the "data" registers. Used by the MTC2 and LWC2 opcodes.
	void setData(uint32_t reg, uint32_t value);

	// Execute GTE command.
	void command(uint32_t command);

	// Rotate, translate and perspective transform single. Operates on v0.
	void cmdRotateTranslatePerspectiveTransformSingle(const CommandConfig& commandConfig);
	// Normal clipping.
	void cmdNormalClip();
	// Depth queue single.
	void cmdDepthQueueSingle(const CommandConfig& config);
	// Multiply vector by matrix and add vector.
	void cmdMultiplyVectorByMatrixAndAddVector(const CommandConfig& config);
	// Normal color depth cue single vector.
	void cmdNormalColorDepthSingleVector(const CommandConfig& config);
	// Average three z values.
	void cmdAverageSingleZ3();
	// Rotate, translate and perspective transform triple.
	// Operates on v0, v1 and v2.
	void cmdRotateTranslatePerspectiveTransform(const CommandConfig& commandConfig);
	// Normal color color triple.
	// Operates on v0, v1 and v2.
	void cmdNormalColorColorTriple(const CommandConfig& commandConfig);

	void doNormalColorColor(const CommandConfig& commandConfig, uint8_t vectorIndex);

	void doNormalColorDepthTransformation(const CommandConfig& config, uint8_t vectorIndex);
	void multiplyMatrixByVector(const CommandConfig& config, Matrix matrix, uint8_t vectorIndex, ControlVector controlVector);

	// Convert the contents of m_accumulatorsSignedWord[1...3] to put them in m_accumulatorsSignedHalfwords[1...3].
	void convertAccumulatorsSignedWordsToHalfwords(const CommandConfig& commandConfig);
	void convertAccumulatorsSignedWordsToRGBFifo();

	// Rotate, translate and perspective transform a single vector.
	// Returns the projection factor that's also used for depth queuing.
	uint32_t doRotateTranslatePerspectiveTransform(const CommandConfig& commandConfig, size_t vectorIndex);
	// Perform the depth queuing calculations using the projection factor
	// computed by the doRotateTranslatePerspectiveTransform method.
	void depthQueuing(uint32_t projectionFactor);
	
	// Set a bit in the flag register.
	void setFlag(uint8_t bit);

	// Truncate i64 value for keeping only the low 43 bits + sign and
	// update the flags if an overflow occurs.
	int64_t truncatei64Toi44(uint8_t flag, int64_t value);
	// Truncate i32 value to an i16, saturating in case of an overflow
	// and updating the flags if an overflow occurs. If m_clampNegative is true,
	// negative values will be clamped to 0.
	int16_t truncatei32Toi16Saturate(const CommandConfig& config, uint8_t flag, int32_t value);
	int16_t truncatei32Toi11Saturate(uint8_t flag, int32_t value);

	// Convert i64 to i32, in the case of a truncation set the result overflow flags
	// ( but don't saturate the result ).
	int32_t converti64Toi32Result(int64_t value);
	// Convert a 64 bit signed average value to an unsigned halfword while updating the overflow flags.
	uint16_t converti64TozAverage(int64_t average);

private:
	// Control registers.
	// Screen offset X: signed 16.16.
	int32_t m_offsetX;
	// Screen offset Y: signed 16.16.
	int32_t m_offsetY;

	// Projection plane distance.
	uint16_t m_projectionPlaneDistance;
	
	// Depth queing coefficient: signed 8.8.
	int16_t m_depthQueingCoefficient;
	// Depth queing offset: signed 8.24.
	int32_t m_depthQueingOffset;

	// Scale factor when computing the average of 3 Z values (triangle): signed 4.12.
	int16_t m_zScaleFactor3;
	// Scale factor when computing the average of 4 Z values (quad): signed 4.12.
	int16_t m_zScaleFactor4;

	// Three 3x3 signed 4.12 matrices: rotation, light and color.
	int16_t m_matrices[3][3][3];
	// Four 3x signed words control vectors: translation, BackgroundColor, FarColor and Zero
	// ( which is always equal to [0, 0, 0] ).
	int32_t m_controlVectors[4][3];
	// Overflow flags that are generated by the GTE commands.
	uint32_t m_overflowFlags;

	// Data registers.
	// Vectors (v0, v1, v2): 3x3 signed 4.12. The fourth vector is to simplify
	// the emulator's implementation in doNcd.
	int16_t m_vectors[4][3];
	// Accumulators for intermediate results, 4 x signed word.
	int32_t m_accumulatorsSignedWord[4];
	// Z average value.
	uint16_t m_zAverage;
	// RGB color. High byte is passed around but not used in computations,
	// it often contains a GPU GP0 command byte.
	uint8_t m_rgbColor[4];
	// Accumulators for intermediate results, 4 x signed halfwords.
	int16_t m_accumulatorsSignedHalfwords[4];
	// XY fifo: 4 x 2 x signed halfwords.
	std::pair<int16_t, int16_t> m_xyFifo[4];
	// Z fifo: 4 x unsigned halfwords.
	uint16_t m_zFifo[4];
	// RGB color FIFO.
	uint8_t m_rgbFifo[3][4];
	// Input value that is used to compute "leading zeros count result" value below.
	uint32_t m_leadingZerosCountSign;
	// Contains the numbers of leading zeros in "leading zeros count result".
	// If it is positive, the "leading zeros count sign" equals to 0.
	// If it is negative, the "leading zeros count sign" equals to 1.
	uint8_t m_leadingZerosCountResult;
};
