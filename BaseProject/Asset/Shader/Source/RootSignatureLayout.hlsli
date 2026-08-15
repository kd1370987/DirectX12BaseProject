// ルートシグネチャ作成時用に、共通部品の定義

// 共通サンプラー
#define RS_STATIC_SAMPLER \
"StaticSampler(s0, " \
"    filter = FILTER_MIN_MAG_MIP_LINEAR, " \
"    addressU = TEXTURE_ADDRESS_WRAP, " \
"    addressV = TEXTURE_ADDRESS_WRAP, " \
"    addressW = TEXTURE_ADDRESS_WRAP)"

// 端をクランプする共通サンプラー
// 縮小/拡大バッファをまたぐブラーのように、画面外を拾うと反対側の色が回り込んで
// しまう処理はこちらを使う（WRAP のままだと端に別の色が滲む）
#define RS_STATIC_SAMPLER_CLAMP \
"StaticSampler(s0, " \
"    filter = FILTER_MIN_MAG_MIP_LINEAR, " \
"    addressU = TEXTURE_ADDRESS_CLAMP, " \
"    addressV = TEXTURE_ADDRESS_CLAMP, " \
"    addressW = TEXTURE_ADDRESS_CLAMP)"


// デフォルト用
#define RS_FLAGS \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
    "DENY_HULL_SHADER_ROOT_ACCESS | " \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)"

