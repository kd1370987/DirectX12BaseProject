// ルートシグネチャ作成時用に、共通部品の定義

// 共通サンプラー
#define RS_STATIC_SAMPLER \
"StaticSampler(s0, " \
"    filter = FILTER_MIN_MAG_MIP_LINEAR, " \
"    addressU = TEXTURE_ADDRESS_WRAP, " \
"    addressV = TEXTURE_ADDRESS_WRAP, " \
"    addressW = TEXTURE_ADDRESS_WRAP)"

// 共通定数バッファ
#define RS_CAMERA_CB "CBV(b0,visibility = SHADER_VISIBILITY_ALL)"

// デフォルト用
#define RS_FLAGS\
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
    "DENY_HULL_SHADER_ROOT_ACCESS | " \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)"

// デフォルトルートシグネチャ
#define DEFAULT_ROOT_SIG \
RS_FLAGS ","\
RS_CAMERA_CB ","\
"CBV(b1,visibility = SHADER_VISIBILITY_ALL),"\
"CBV(b2,visibility = SHADER_VISIBILITY_ALL),"\
"CBV(b3,visibility = SHADER_VISIBILITY_ALL),"\
"CBV(b4,visibility = SHADER_VISIBILITY_ALL),"\
"DescriptorTable(SRV(t0, numDescriptors=4),visibility = SHADER_VISIBILITY_PIXEL),"\
RS_STATIC_SAMPLER
