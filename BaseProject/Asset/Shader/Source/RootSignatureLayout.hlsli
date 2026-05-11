// ルートシグネチャ作成時用に、共通部品の定義

// 共通サンプラー
#define RS_STATIC_SAMPLER \
"StaticSampler(s0, " \
"    filter = FILTER_MIN_MAG_MIP_LINEAR, " \
"    addressU = TEXTURE_ADDRESS_WRAP, " \
"    addressV = TEXTURE_ADDRESS_WRAP, " \
"    addressW = TEXTURE_ADDRESS_WRAP)"

// 共通定数バッファ
#define RS_CAMERA_CB "CBV(b0)"
