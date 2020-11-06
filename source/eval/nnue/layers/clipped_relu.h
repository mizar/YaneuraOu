﻿// NNUE評価関数の層ClippedReLUの定義

#ifndef _NNUE_LAYERS_CLIPPED_RELU_H_
#define _NNUE_LAYERS_CLIPPED_RELU_H_

#include "../../../config.h"

#if defined(EVAL_NNUE)

#include "../nnue_common.h"

namespace Eval {

namespace NNUE {

namespace Layers {

// Clipped ReLU
template <typename PreviousLayer>
class ClippedReLU {
 public:
  // 入出力の型
  using InputType = typename PreviousLayer::OutputType;
  using OutputType = std::uint8_t;
  static_assert(std::is_same<InputType, std::int32_t>::value, "");

  // 入出力の次元数
  static constexpr IndexType kInputDimensions =
      PreviousLayer::kOutputDimensions;
  static constexpr IndexType kOutputDimensions = kInputDimensions;

  // この層で使用する順伝播用バッファのサイズ
  static constexpr std::size_t kSelfBufferSize =
      CeilToMultiple(kOutputDimensions * sizeof(OutputType), kCacheLineSize);

  // 入力層からこの層までで使用する順伝播用バッファのサイズ
  static constexpr std::size_t kBufferSize =
      PreviousLayer::kBufferSize + kSelfBufferSize;

  // 評価関数ファイルに埋め込むハッシュ値
  static constexpr std::uint32_t GetHashValue() {
    std::uint32_t hash_value = 0x538D24C7u;
    hash_value += PreviousLayer::GetHashValue();
    return hash_value;
  }

  // 入力層からこの層までの構造を表す文字列
  static std::string GetStructureString() {
    return "ClippedReLU[" +
        std::to_string(kOutputDimensions) + "](" +
        PreviousLayer::GetStructureString() + ")";
  }

  // パラメータを読み込む
  bool ReadParameters(std::istream& stream) {
    return previous_layer_.ReadParameters(stream);
  }

  // パラメータを書き込む
  bool WriteParameters(std::ostream& stream) const {
    return previous_layer_.WriteParameters(stream);
  }

  // 順伝播
  const OutputType* Propagate(
      const TransformedFeatureType* transformed_features, char* buffer) const {
    const auto input = previous_layer_.Propagate(
        transformed_features, buffer + kSelfBufferSize);
    const auto output = reinterpret_cast<OutputType*>(buffer);

#if defined(USE_AVX2)
    constexpr IndexType kNumChunks = kInputDimensions / kSimdWidth;
    const __m256i kZero = _mm256_setzero_si256();
    const __m256i kOffsets = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
    const auto in = reinterpret_cast<const __m256i*>(input);
    const auto out = reinterpret_cast<__m256i*>(output);
    for (IndexType i = 0; i < kNumChunks; ++i) {
      const __m256i words0 = _mm256_srai_epi16(_mm256_packs_epi32(
          _mm256_load_si256(&in[i * 4 + 0]),
          _mm256_load_si256(&in[i * 4 + 1])), kWeightScaleBits);
      const __m256i words1 = _mm256_srai_epi16(_mm256_packs_epi32(
          _mm256_load_si256(&in[i * 4 + 2]),
          _mm256_load_si256(&in[i * 4 + 3])), kWeightScaleBits);
      _mm256_store_si256(&out[i], _mm256_permutevar8x32_epi32(_mm256_max_epi8(
          _mm256_packs_epi16(words0, words1), kZero), kOffsets));
    }
    constexpr IndexType kStart = kNumChunks * kSimdWidth;
	// 端数分の処理が必要なので、↓以下でそれを行う。

#elif defined(USE_SSSE3)
    constexpr IndexType kNumChunks = kInputDimensions / kSimdWidth;
    const __m128i kZero = _mm_setzero_si128();
#if !defined(USE_SSE41) // SSE4非対応だがSSE3は使える環境
    const __m128i k0x80s = _mm_set1_epi8(-128);
#endif
    const auto in = reinterpret_cast<const __m128i*>(input);
    const auto out = reinterpret_cast<__m128i*>(output);
    for (IndexType i = 0; i < kNumChunks; ++i) {
      const __m128i words0 = _mm_srai_epi16(_mm_packs_epi32(
          _mm_load_si128(&in[i * 4 + 0]),
          _mm_load_si128(&in[i * 4 + 1])), kWeightScaleBits);
      const __m128i words1 = _mm_srai_epi16(_mm_packs_epi32(
          _mm_load_si128(&in[i * 4 + 2]),
          _mm_load_si128(&in[i * 4 + 3])), kWeightScaleBits);
        const __m128i packedbytes = _mm_packs_epi16(words0, words1);
        _mm_store_si128(&out[i],
#if defined(USE_SSE41)
            _mm_max_epi8(packedbytes, kZero)
#else // SSE4非対応だがSSE3は使える環境
            _mm_subs_epi8(_mm_adds_epi8(packedbytes, k0x80s), k0x80s)
#endif
        );
    }
    constexpr IndexType kStart = kNumChunks * kSimdWidth;

#elif defined(USE_NEON)
    constexpr IndexType kNumChunks = kInputDimensions / (kSimdWidth / 2);
    const int8x8_t kZero = {0};
    const auto in = reinterpret_cast<const int32x4_t*>(input);
    const auto out = reinterpret_cast<int8x8_t*>(output);
    for (IndexType i = 0; i < kNumChunks; ++i) {
      int16x8_t shifted;
      const auto pack = reinterpret_cast<int16x4_t*>(&shifted);
      pack[0] = vqshrn_n_s32(in[i * 2 + 0], kWeightScaleBits);
      pack[1] = vqshrn_n_s32(in[i * 2 + 1], kWeightScaleBits);
      out[i] = vmax_s8(vqmovn_s16(shifted), kZero);
    }
    constexpr IndexType kStart = kNumChunks * (kSimdWidth / 2);

#else
    constexpr IndexType kStart = 0;
#endif
    for (IndexType i = kStart; i < kInputDimensions; ++i) {
      output[i] = static_cast<OutputType>(
          std::max(0, std::min(127, input[i] >> kWeightScaleBits)));
    }
    return output;
  }

 private:
  // 学習用クラスをfriendにする
  friend class Trainer<ClippedReLU>;

  // この層の直前の層
  PreviousLayer previous_layer_;
};

}  // namespace Layers

}  // namespace NNUE

}  // namespace Eval

#endif  // defined(EVAL_NNUE)

#endif
