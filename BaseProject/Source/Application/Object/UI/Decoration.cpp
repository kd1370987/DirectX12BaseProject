#include "Decoration.h"

#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Texture/IO/TextureIO.h"
#include "Engine/Editor/Helper/EditorHelper.h"

//==========================================================================================
// デコレーションの評価と描画
//
// 描画はすべて「ローカル矩形を親の回転で回してから積む」1つの経路に寄せてある
// (SubmitLocalRect)。塗り・枠・文字のどれも、アンカーからのずれを持った矩形の集まりなので、
// ここを共通にしておかないと回転を掛けたときにバラバラにずれる。
//==========================================================================================
namespace App::Object::Decoration
{
	namespace
	{
		//----------------------------------------------------------------------------------
		// 小物
		//----------------------------------------------------------------------------------

		// 度で回す : UIBase::IsPointInside と同じ向き(時計回り)
		Math::Vector2 RotateDeg(const Math::Vector2& a_value, float a_degree)
		{
			if (a_degree == 0.0f) return a_value;

			const float _rad = DirectX::XMConvertToRadians(a_degree);
			const float _cos = std::cos(_rad);
			const float _sin = std::sin(_rad);

			return {
				a_value.x * _cos - a_value.y * _sin,
				a_value.x * _sin + a_value.y * _cos
			};
		}

		float Lerp(float a_start, float a_end, float a_rate)
		{
			return a_start + (a_end - a_start) * a_rate;
		}
		Math::Vector2 Lerp(const Math::Vector2& a_start, const Math::Vector2& a_end, float a_rate)
		{
			return { Lerp(a_start.x, a_end.x, a_rate), Lerp(a_start.y, a_end.y, a_rate) };
		}
		Math::Color Lerp(const Math::Color& a_start, const Math::Color& a_end, float a_rate)
		{
			return {
				Lerp(a_start.r, a_end.r, a_rate),
				Lerp(a_start.g, a_end.g, a_rate),
				Lerp(a_start.b, a_end.b, a_rate),
				Lerp(a_start.a, a_end.a, a_rate)
			};
		}

		// 板ポリ用の白テクスチャ
		//
		// 中身は 4x4 の白1色。ResourceManager 側がGUIDでキャッシュしているので、
		// 毎フレーム呼んでも作り直しにはならない
		Engine::Handle<Engine::Resource::Texture> GetWhiteTexture()
		{
			return Engine::Resource::TextureIO::LoadTexture(Engine::GUID(), Engine::TexColor::WHITE);
		}

		//----------------------------------------------------------------------------------
		// アニメーションを合成した結果
		//----------------------------------------------------------------------------------
		struct AnimResult
		{
			Math::Vector2 positionAdd = {};				// 位置へ足す(px, 親の倍率が掛かる前)
			Math::Vector2 scaleMul = { 1.0f, 1.0f };	// 大きさへ掛ける
			float rotationAdd = 0.0f;					// 回転へ足す(度)
			Math::Color colorMul = Engine::Color::WHITE;// 色へ掛ける
			Math::Vector2 uvAdd = {};					// UVオフセットへ足す
		};

		// トゥイーンの進み具合(0〜1)を出す
		float CalcTweenRate(const UIAnimation& a_anim)
		{
			if (a_anim.durationTime <= 1e-6f) return 1.0f;

			float _rate = std::clamp(a_anim.currentTime / a_anim.durationTime, 0.0f, 1.0f);

			// 往復 : 前半で end まで行き、後半で start へ戻る
			if (a_anim.isPingPong)
			{
				_rate = (_rate <= 0.5f) ? (_rate * 2.0f) : ((1.0f - _rate) * 2.0f);
			}

			return Ease(a_anim.ease, _rate);
		}

		void ApplyTween(const UIAnimation& a_anim, AnimResult& a_inoutResult)
		{
			using Engine::Utility::HasFlag;

			const float _rate = CalcTweenRate(a_anim);

			// channels で選ばれていないものは触らない。
			// 触ってしまうと、設定した覚えのない end(既定値0)へ寄っていく
			if (HasFlag(a_anim.channels, EAnimChannel::COLOR))
			{
				a_inoutResult.colorMul *= Lerp(a_anim.color.start, a_anim.color.end, _rate);
			}
			if (HasFlag(a_anim.channels, EAnimChannel::POSITION))
			{
				a_inoutResult.positionAdd += Lerp(a_anim.position.start, a_anim.position.end, _rate);
			}
			if (HasFlag(a_anim.channels, EAnimChannel::SCALE))
			{
				a_inoutResult.scaleMul *= Lerp(a_anim.scale.start, a_anim.scale.end, _rate);
			}
			if (HasFlag(a_anim.channels, EAnimChannel::ROTATION))
			{
				a_inoutResult.rotationAdd += Lerp(a_anim.rotation.start, a_anim.rotation.end, _rate);
			}
			if (HasFlag(a_anim.channels, EAnimChannel::UV))
			{
				a_inoutResult.uvAdd += Lerp(a_anim.uv.start, a_anim.uv.end, _rate);
			}
		}

		//----------------------------------------------------------------------------------
		// 周期運動の波
		//
		// sin(2π(f*t + phase))。位相は 0〜1 で1周。
		//----------------------------------------------------------------------------------
		float WaveSigned(float a_frequency, float a_phase, float a_time)
		{
			constexpr float _TWO_PI = 6.283185307f;
			return std::sin(_TWO_PI * (a_frequency * a_time + a_phase));
		}

		// 足すチャンネル(位置・回転)の値を出す
		template<typename T>
		T OscillationAdd(const Oscillation<T>& a_oscillation, float a_time)
		{
			const float _signed = WaveSigned(a_oscillation.frequency, a_oscillation.phase, a_time);

			// 上下限を使わないときは 0 を中心に ±amplitude
			if (!a_oscillation.isUseLimit) return a_oscillation.amplitude * _signed;

			// -1〜1 を 0〜1 へ均してから、下限と上限の間へ写す
			return Lerp(a_oscillation.minValue, a_oscillation.maxValue, (_signed + 1.0f) * 0.5f);
		}

		// 掛けるチャンネル(大きさ)の値を出す : 1 が等倍
		Math::Vector2 OscillationMul(const Oscillation<Math::Vector2>& a_oscillation, float a_time)
		{
			const float _signed = WaveSigned(a_oscillation.frequency, a_oscillation.phase, a_time);

			if (a_oscillation.isUseLimit)
			{
				return Lerp(a_oscillation.minValue, a_oscillation.maxValue, (_signed + 1.0f) * 0.5f);
			}

			// 振幅0で等倍のままになるよう、1を中心に揺らす
			return {
				1.0f + a_oscillation.amplitude.x * _signed,
				1.0f + a_oscillation.amplitude.y * _signed
			};
		}

		// 掛けるチャンネル(色)の値を出す : 1 が元の色のまま
		//
		// 加算ではなく乗算にしてあるのは、上下限で「アルファは 0.8 までしか上げない」と
		// 書いたときに、元の色が何であっても意味が変わらないようにするため
		Math::Color OscillationMul(const Oscillation<Math::Color>& a_oscillation, float a_time)
		{
			const float _signed = WaveSigned(a_oscillation.frequency, a_oscillation.phase, a_time);

			if (a_oscillation.isUseLimit)
			{
				return Lerp(a_oscillation.minValue, a_oscillation.maxValue, (_signed + 1.0f) * 0.5f);
			}

			return {
				1.0f + a_oscillation.amplitude.r * _signed,
				1.0f + a_oscillation.amplitude.g * _signed,
				1.0f + a_oscillation.amplitude.b * _signed,
				1.0f + a_oscillation.amplitude.a * _signed
			};
		}

		void ApplyOscillation(const UIProceduralAnimation& a_anim, AnimResult& a_inoutResult)
		{
			using Engine::Utility::HasFlag;

			const float _time = a_anim.currentTime;

			if (HasFlag(a_anim.channels, EAnimChannel::POSITION))
			{
				a_inoutResult.positionAdd += OscillationAdd(a_anim.position, _time);
			}
			if (HasFlag(a_anim.channels, EAnimChannel::SCALE))
			{
				a_inoutResult.scaleMul *= OscillationMul(a_anim.scale, _time);
			}
			if (HasFlag(a_anim.channels, EAnimChannel::ROTATION))
			{
				a_inoutResult.rotationAdd += OscillationAdd(a_anim.rotation, _time);
			}
			if (HasFlag(a_anim.channels, EAnimChannel::COLOR))
			{
				a_inoutResult.colorMul *= OscillationMul(a_anim.color, _time);
			}
		}

		//----------------------------------------------------------------------------------
		// 親と合成したあとの状態
		//----------------------------------------------------------------------------------
		struct Resolved
		{
			Math::Vector2 anchorPos = {};	// ピボットが乗るスクリーン座標(px)
			Math::Vector2 size = {};		// 大きさ(px)
			float rotation = 0.0f;			// 回転(度)
			float layer = 0.0f;				// Z順
			Math::Color color = {};			// 最終色(飾り自身の色まで掛けたもの)

			/// <summary>飾り自身の色を除いた掛かり具合(親の色 × その場の色 × アニメ)</summary>
			/// <remarks>
			/// 枠は飾り本体とは別の色を持つので、最終色から逆算せずにこれを掛ける。
			/// ここを通さないと、反応で消したはずの枠だけ residual に残る
			/// </remarks>
			Math::Color modulate = {};

			Math::Vector2 uvOffset = {};	// UVオフセット
			Math::Vector2 uvScale = { 1.0f, 1.0f };	// UV倍率
			float uniformScale = 1.0f;		// 枠の太さ・字間など、1軸で効かせたいもの用

			// 湾曲(親の設定を、そのまま渡せる形へ畳んだもの)
			float curveK = 0.0f;			// 反りの強さ(1/px)。0で曲げない
			float curveOriginX = 0.0f;		// 弧の頂点の横位置(親のアンカーからのpx)
			float curveShiftY = 0.0f;		// 曲げたときに全体を上下へずらす量(px)

			// 親の回転を掛ける前の、この飾りのずれ(px)。
			// 弧の中心からの横ずれを測るのに使う(回した後の座標では測れない)
			Math::Vector2 localOffset = {};
		};

		//----------------------------------------------------------------------------------
		// アニメーションと反応を1つにまとめる
		//
		// 描画(Resolve)と当たり判定(CalcCurrentTransform)の両方がここを通る。
		// 別々に組み立てると、絵は大きくなっているのに判定は素のまま、というずれが起きる
		//----------------------------------------------------------------------------------
		AnimResult MakeAnimResult(const Decoration& a_decoration)
		{
			AnimResult _anim = {};
			if (a_decoration.opTweenAnim.has_value())      ApplyTween(*a_decoration.opTweenAnim, _anim);
			if (a_decoration.opOscillationAnim.has_value()) ApplyOscillation(*a_decoration.opOscillationAnim, _anim);

			// カーソルへの反応 : 目標へ寄せた結果(current)を掛ける。
			// 寄せる処理そのものは AdvanceAnimation が行う
			if (a_decoration.opReaction.has_value())
			{
				const UIReaction& _reaction = *a_decoration.opReaction;

				_anim.colorMul *= _reaction.current.color;
				_anim.scaleMul *= _reaction.current.scale;
				_anim.positionAdd += _reaction.current.offsetAdd;

				// 出さない状態は透明にして消す。
				// 描画側で弾かずアルファで消しているのは、途中の割合で薄く出せるようにするため
				_anim.colorMul.a *= _reaction.visibleRate;
			}

			return _anim;
		}

		Resolved Resolve(
			const Decoration& a_decoration,
			const ParentTransform& a_parentTr,
			const ParentOption& a_parentOp,
			const DrawOverride& a_override)
		{
			const AnimResult _anim = MakeAnimResult(a_decoration);

			Resolved _out = {};

			// 親の倍率 × 自分の倍率 × その場の倍率
			_out.uniformScale = a_parentTr.scale * a_decoration.scale * a_override.scale;

			// ずれは親の回転で回してから足す : 親を回すと飾りが親の周りを回る
			const Math::Vector2 _basePos = a_override.isUsePos ? a_override.pixelPos : a_parentTr.pixelPos;
			const Math::Vector2 _offset = (a_decoration.offsetPos + _anim.positionAdd) * _out.uniformScale;
			_out.anchorPos = _basePos + RotateDeg(_offset, a_parentTr.rotation);

			_out.size = a_decoration.pixelSize * _out.uniformScale * _anim.scaleMul * a_override.sizeScale;
			_out.rotation = a_parentTr.rotation + a_decoration.rotation + _anim.rotationAdd;
			_out.layer = a_parentTr.layer + a_decoration.layerOffset;

			_out.modulate = a_parentTr.color * a_override.tint * _anim.colorMul;
			_out.color = _out.modulate * a_decoration.color;
			_out.uvOffset = a_decoration.uvOffset + a_override.uvOffsetAdd + _anim.uvAdd;
			_out.uvScale = a_override.isUseUvScale ? a_override.uvScale : a_decoration.uvScale;

			//--------------------------------------------------------------
			// 湾曲
			//
			// 「開き角・深さ・弧の中心」を、シェーダーがそのまま使える
			//   反りの強さ k(1/px) と 弧の頂点の位置(px)
			// へここで畳む。畳んでおけば、枠・中身・文字がどんな大きさでも
			// 同じ k と同じ頂点を見るので、全部が1本の弧に乗る。
			//
			// k は「親の矩形の端で、円弧と同じだけ反る」ように決める。
			//   半幅 W を開き角 A で曲げたときの反り = W * tan(A/4)
			//   反りを k*W^2 で作るので k = tan(A/4) / W
			//
			// 幅ではなく高さを基準にすると、ゲージのような横長で背の低いUIが
			// ほとんど反らない(見た目上まったく曲がらない)ので必ず幅で測る
			//--------------------------------------------------------------
			const float _halfSpanX = a_parentOp.parentSize.x * 0.5f * a_parentTr.scale;
			const float _halfSpanY = a_parentOp.parentSize.y * 0.5f * a_parentTr.scale;

			// 深さの倍率。0(既定値)のままでも曲がるように1として扱う
			const float _curveDepth = (a_parentOp.curveRadius > 0.0f) ? a_parentOp.curveRadius : 1.0f;

			if (a_parentOp.curveAngle != 0.0f && _halfSpanX > 0.0f)
			{
				_out.curveK = std::tan(a_parentOp.curveAngle * 0.25f) * _curveDepth / _halfSpanX;
			}
			_out.curveOriginX = a_parentOp.curveCenter.x * _halfSpanX;
			_out.curveShiftY = a_parentOp.curveCenter.y * _halfSpanY;
			_out.localOffset = _offset;

			return _out;
		}

		//----------------------------------------------------------------------------------
		// 描画の最小単位
		//
		// アンカーからのずれ(回転前)で矩形を指定する。
		// ずれを回してから積み、クアッド自体も同じ角度で回すので、
		// 塗り・枠・文字がバラけずに1枚として回る
		//----------------------------------------------------------------------------------
		void SubmitLocalRect(
			Engine::Graphics::GraphicsEngine* a_pGE,
			const Engine::Handle<Engine::Resource::Texture>& a_texHandle,
			const Resolved& a_resolved,
			const Math::Vector2& a_localTopLeft,
			const Math::Vector2& a_size,
			const Math::Color& a_color,
			const Math::Vector2& a_uvOffset,
			const Math::Vector2& a_uvScale
		)
		{
			if (a_size.x <= 0.0f || a_size.y <= 0.0f) return;
			if (a_color.a <= 0.0f) return;

			// 曲げたときの上下のずらしはローカルで足してから回す
			const Math::Vector2 _localTopLeft = {
				a_localTopLeft.x,
				a_localTopLeft.y + a_resolved.curveShiftY
			};
			const Math::Vector2 _pos = a_resolved.anchorPos + RotateDeg(_localTopLeft, a_resolved.rotation);

			// この矩形の中心が、弧の頂点からどれだけ横にずれているか(px)。
			//
			// 飾りのずれ(回転前) + 矩形のずれ + 矩形の半幅 で、親のローカルでの中心が出る。
			// ここを矩形ごとに正しく渡すから、幅の違う枠と中身(ゲージの残量)が
			// 同じ1本の弧に乗る。矩形の中で閉じて曲げると別々の曲がり方になってしまう
			const float _curveOffsetX =
				a_resolved.localOffset.x + a_localTopLeft.x + a_size.x * 0.5f - a_resolved.curveOriginX;

			a_pGE->SubmitUI(
				a_texHandle,
				_pos,
				a_size,
				a_color,
				a_resolved.rotation,
				a_resolved.layer,
				a_uvOffset,
				{ 0.0f, 0.0f },		// ずれで位置を決めているのでピボットは左上固定
				a_uvScale,
				a_resolved.curveK,
				_curveOffsetX
			);
		}

		//----------------------------------------------------------------------------------
		// 枠を描く : 矩形の内側に貼り付ける
		//----------------------------------------------------------------------------------
		void DrawEdge(
			Engine::Graphics::GraphicsEngine* a_pGE,
			const Decoration& a_decoration,
			const Resolved& a_resolved,
			const Math::Vector2& a_localTopLeft,
			const Math::Color& a_edgeColor)
		{
			using Engine::Utility::HasFlag;

			const float _thickness = a_decoration.edgePixel * a_resolved.uniformScale;
			if (_thickness <= 0.0f) return;
			if (a_decoration.edgeSide == EDirection::NONE) return;

			const Engine::Handle<Engine::Resource::Texture> _white = GetWhiteTexture();
			const Math::Vector2& _size = a_resolved.size;

			// 太さが矩形を超えたら塗りつぶしと同じになるので詰める
			const float _thickX = std::min(_thickness, _size.x);
			const float _thickY = std::min(_thickness, _size.y);

			auto _submit = [&](const Math::Vector2& a_offset, const Math::Vector2& a_edgeSize)
				{
					SubmitLocalRect(
						a_pGE, _white, a_resolved,
						a_localTopLeft + a_offset, a_edgeSize,
						a_edgeColor, {}, { 1.0f, 1.0f });
				};

			if (HasFlag(a_decoration.edgeSide, EDirection::UP))    _submit({ 0.0f, 0.0f }, { _size.x, _thickY });
			if (HasFlag(a_decoration.edgeSide, EDirection::DOWN))  _submit({ 0.0f, _size.y - _thickY }, { _size.x, _thickY });
			if (HasFlag(a_decoration.edgeSide, EDirection::LEFT))  _submit({ 0.0f, 0.0f }, { _thickX, _size.y });
			if (HasFlag(a_decoration.edgeSide, EDirection::RIGHT)) _submit({ _size.x - _thickX, 0.0f }, { _thickX, _size.y });
		}

		//----------------------------------------------------------------------------------
		// 文字列を行へ切る
		//----------------------------------------------------------------------------------
		std::vector<std::vector<uint32_t>> SplitLines(const std::string& a_utf8Text)
		{
			std::vector<std::vector<uint32_t>> _lines = { {} };

			for (const uint32_t _codePoint : Engine::String::ToCodePoints(a_utf8Text))
			{
				if (_codePoint == '\r') continue;		// 復帰は送りを持たない
				if (_codePoint == '\n')
				{
					_lines.emplace_back();
					continue;
				}
				_lines.back().push_back(_codePoint);
			}

			return _lines;
		}

		// 1行ぶんの幅(px, フォント基準サイズ)を測る
		float MeasureLine(
			Engine::Resource::Font* a_pFont,
			const std::vector<uint32_t>& a_line,
			float a_charSpacingInFontUnit)
		{
			float _width = 0.0f;
			uint32_t _prev = 0;

			for (const uint32_t _codePoint : a_line)
			{
				const Engine::Resource::Glyph* _pGlyph = a_pFont->RequestGlyph(_codePoint);
				if (_pGlyph == nullptr) continue;

				if (_prev != 0) _width += a_pFont->GetKerning(_prev, _codePoint);
				_width += _pGlyph->xAdvance + a_charSpacingInFontUnit;

				_prev = _codePoint;
			}

			return _width;
		}

		//----------------------------------------------------------------------------------
		// 文字を描く
		//----------------------------------------------------------------------------------
		void DrawText(
			Engine::Graphics::GraphicsEngine* a_pGE,
			Engine::Resource::ResourceManager* a_pResourceManager,
			const Decoration& a_decoration,
			const Resolved& a_resolved)
		{
			if (a_decoration.text.empty()) return;
			if (!a_decoration.fontRef.IsValid()) return;
			if (!a_pResourceManager->IsReady(a_decoration.fontRef)) return;

			// グリフは要求された時点で焼くので、参照は書き込み可能で引く
			Engine::Resource::Font* _pFont = a_pResourceManager->Ref(a_decoration.fontRef.GetRaw());
			if (_pFont == nullptr || !_pFont->IsValid()) return;

			const float _atlasSize = static_cast<float>(_pFont->GetAtlasSize());
			if (_atlasSize <= 0.0f) return;

			// 基準サイズ(64px)で焼いたものを、出したい大きさへ縮小する
			const float _fontScale =
				_pFont->GetScaleForSize(a_decoration.fontPixelSize) * a_resolved.uniformScale;
			if (_fontScale <= 0.0f) return;

			// 字間はピクセル指定なので、測るときはフォント基準サイズへ戻して足す
			const float _charSpacingInFontUnit =
				(_fontScale > 1e-6f) ? (a_decoration.charSpacing * a_resolved.uniformScale / _fontScale) : 0.0f;

			const auto _lines = SplitLines(a_decoration.text);

			const float _lineHeight = _pFont->GetLineHeight() * a_decoration.lineSpacing;
			const float _ascent = _pFont->GetAscent();

			// ブロック全体の大きさ : ピボットを当てる基準になる
			float _blockWidth = 0.0f;
			std::vector<float> _lineWidthVec;
			_lineWidthVec.reserve(_lines.size());
			for (const auto& _line : _lines)
			{
				const float _lineWidth = MeasureLine(_pFont, _line, _charSpacingInFontUnit);
				_lineWidthVec.push_back(_lineWidth);
				_blockWidth = std::max(_blockWidth, _lineWidth);
			}
			const float _blockHeight = _lineHeight * static_cast<float>(_lines.size());

			// ピボットはブロック全体に対して効かせる
			const Math::Vector2 _blockTopLeft = {
				-a_decoration.pivot.x * _blockWidth * _fontScale,
				-a_decoration.pivot.y * _blockHeight * _fontScale
			};

			const Engine::Handle<Engine::Resource::Texture> _atlasHandle = _pFont->GetAtlasTextureHandle();

			for (size_t _lineIndex = 0; _lineIndex < _lines.size(); ++_lineIndex)
			{
				const auto& _line = _lines[_lineIndex];

				// 行揃え : ブロック幅に対して行を寄せる
				float _lineStart = 0.0f;
				switch (a_decoration.textAlign)
				{
				case ETextAlign::Center: _lineStart = (_blockWidth - _lineWidthVec[_lineIndex]) * 0.5f; break;
				case ETextAlign::Right:  _lineStart = (_blockWidth - _lineWidthVec[_lineIndex]);        break;
				case ETextAlign::Left:
				default: break;
				}

				// ベースラインは行の上端から ascent ぶん下
				const float _baselineY = _lineHeight * static_cast<float>(_lineIndex) + _ascent;

				float _penX = _lineStart;
				uint32_t _prev = 0;

				for (const uint32_t _codePoint : _line)
				{
					const Engine::Resource::Glyph* _pGlyph = _pFont->RequestGlyph(_codePoint);
					if (_pGlyph == nullptr) continue;

					if (_prev != 0) _penX += _pFont->GetKerning(_prev, _codePoint);

					if (!_pGlyph->IsEmpty())
					{
						// フォント基準サイズでの位置を出してから、まとめて縮小する
						const Math::Vector2 _localTopLeft = {
							_blockTopLeft.x + (_penX + _pGlyph->xOffset) * _fontScale,
							_blockTopLeft.y + (_baselineY + _pGlyph->yOffset) * _fontScale
						};
						const Math::Vector2 _glyphSize = {
							static_cast<float>(_pGlyph->width) * _fontScale,
							static_cast<float>(_pGlyph->height) * _fontScale
						};

						const Math::Vector2 _uvOffset = {
							static_cast<float>(_pGlyph->x) / _atlasSize,
							static_cast<float>(_pGlyph->y) / _atlasSize
						};
						const Math::Vector2 _uvScale = {
							static_cast<float>(_pGlyph->width) / _atlasSize,
							static_cast<float>(_pGlyph->height) / _atlasSize
						};

						SubmitLocalRect(
							a_pGE, _atlasHandle, a_resolved,
							_localTopLeft, _glyphSize,
							a_resolved.color, _uvOffset, _uvScale);
					}

					_penX += _pGlyph->xAdvance + _charSpacingInFontUnit;
					_prev = _codePoint;
				}
			}
		}
	}

	//======================================================================================
	// イージング
	//======================================================================================
	float Ease(EEase a_ease, float a_rate)
	{
		const float _t = std::clamp(a_rate, 0.0f, 1.0f);

		switch (a_ease)
		{
		case EEase::InQuad:		return _t * _t;
		case EEase::OutQuad:	return 1.0f - (1.0f - _t) * (1.0f - _t);
		case EEase::InOutQuad:
			return (_t < 0.5f)
				? (2.0f * _t * _t)
				: (1.0f - 2.0f * (1.0f - _t) * (1.0f - _t));

		case EEase::InCubic:	return _t * _t * _t;
		case EEase::OutCubic:
		{
			const float _inv = 1.0f - _t;
			return 1.0f - _inv * _inv * _inv;
		}
		case EEase::InOutCubic:
		{
			if (_t < 0.5f) return 4.0f * _t * _t * _t;
			const float _inv = -2.0f * _t + 2.0f;
			return 1.0f - (_inv * _inv * _inv) * 0.5f;
		}

		case EEase::OutBack:
		{
			// 行き過ぎてから戻る。定数は一般的なイージング表の値
			constexpr float _C1 = 1.70158f;
			constexpr float _C3 = _C1 + 1.0f;
			const float _inv = _t - 1.0f;
			return 1.0f + _C3 * _inv * _inv * _inv + _C1 * _inv * _inv;
		}
		case EEase::OutElastic:
		{
			if (_t <= 0.0f) return 0.0f;
			if (_t >= 1.0f) return 1.0f;

			constexpr float _C4 = 6.283185307f / 3.0f;
			return std::pow(2.0f, -10.0f * _t) * std::sin((_t * 10.0f - 0.75f) * _C4) + 1.0f;
		}

		case EEase::Linear:
		default:
			return _t;
		}
	}

	//======================================================================================
	// 時間を進める
	//======================================================================================
	namespace
	{
		// 親の状態に対応する見た目を選ぶ
		UIStateStyle PickStateStyle(const UIReaction& a_reaction, EUIState a_state)
		{
			switch (a_state)
			{
			case EUIState::Hovered:  return a_reaction.hovered;
			case EUIState::Pressed:  return a_reaction.pressed;
			case EUIState::Disabled: return a_reaction.disabled;

			case EUIState::Normal:
			default:
				// 素のまま(掛けても足しても変わらない値)
				return UIStateStyle();
			}
		}

		// 反応を目標へ寄せる
		void AdvanceReaction(UIReaction& a_reaction, EUIState a_parentState, float a_deltaTime)
		{
			const UIStateStyle _target = PickStateStyle(a_reaction, a_parentState);

			const bool _isVisibleState =
				Engine::Utility::HasFlag(a_reaction.visibleState, ToStateFlag(a_parentState));
			const float _targetVisible = _isVisibleState ? 1.0f : 0.0f;

			//----------------------------------------------------------------------
			// 寄せる割合
			//
			// 1 - exp(-speed * dt) にしてあるのは、フレームレートが変わっても
			// 同じ速さで寄るようにするため(dt をそのまま掛けると重いときほど速く寄る)。
			// 初回だけ補間しないのは、出た瞬間に Normal から寄り始めてちらつくのを避けるため
			//----------------------------------------------------------------------
			float _rate = 1.0f;
			if (a_reaction.blendSpeed > 0.0f && a_reaction.isInitialized)
			{
				_rate = 1.0f - std::exp(-a_reaction.blendSpeed * a_deltaTime);
			}

			a_reaction.current.color     = Lerp(a_reaction.current.color, _target.color, _rate);
			a_reaction.current.scale     = Lerp(a_reaction.current.scale, _target.scale, _rate);
			a_reaction.current.offsetAdd = Lerp(a_reaction.current.offsetAdd, _target.offsetAdd, _rate);
			a_reaction.visibleRate       = Lerp(a_reaction.visibleRate, _targetVisible, _rate);

			a_reaction.isInitialized = true;
		}
	}

	//======================================================================================
	// いま効いているアニメーション・反応の量
	//======================================================================================
	DecorationTransform CalcCurrentTransform(const Decoration& a_decoration)
	{
		const AnimResult _anim = MakeAnimResult(a_decoration);

		DecorationTransform _out = {};
		_out.offsetAdd   = _anim.positionAdd;
		_out.scaleMul    = _anim.scaleMul;
		_out.rotationAdd = _anim.rotationAdd;

		return _out;
	}

	void AdvanceAnimation(Decoration& a_decoration, EUIState a_parentState, float a_deltaTime)
	{
		// カーソルへの反応
		if (a_decoration.opReaction.has_value())
		{
			AdvanceReaction(*a_decoration.opReaction, a_parentState, a_deltaTime);
		}

		if (a_decoration.opTweenAnim.has_value())
		{
			UIAnimation& _anim = *a_decoration.opTweenAnim;
			_anim.currentTime += a_deltaTime;

			if (_anim.durationTime > 1e-6f && _anim.currentTime >= _anim.durationTime)
			{
				// ループは余りを持ち越す。切り捨てるとフレームレートで速さが変わる
				_anim.currentTime = _anim.isLoop
					? std::fmod(_anim.currentTime, _anim.durationTime)
					: _anim.durationTime;
			}
		}

		if (a_decoration.opOscillationAnim.has_value())
		{
			UIProceduralAnimation& _anim = *a_decoration.opOscillationAnim;
			_anim.currentTime += a_deltaTime;

			// 揺れは終わらないので、精度が落ちないところで巻き戻す
			if (_anim.currentTime > 3600.0f) _anim.currentTime -= 3600.0f;
		}
	}

	//======================================================================================
	// 描画
	//======================================================================================
	void DrawDecoration(
		Engine::Graphics::GraphicsEngine* a_pGraphicsEngine,
		Engine::Resource::ResourceManager* a_pResourceManager,
		const Decoration& a_decoration,
		const ParentTransform& a_parentTr,
		const ParentOption& a_parentOp,
		const DrawOverride& a_override)
	{
		if (a_pGraphicsEngine == nullptr || a_pResourceManager == nullptr) return;
		if (!a_decoration.isVisible) return;

		// 群で絞られているなら、対象外は描かない
		if (a_override.isUseGroup && a_decoration.group != a_override.group) return;

		const Resolved _resolved = Resolve(a_decoration, a_parentTr, a_parentOp, a_override);

		// 文字はグリフごとに矩形を組むので別経路
		if (a_decoration.type == EDecorationType::Text)
		{
			DrawText(a_pGraphicsEngine, a_pResourceManager, a_decoration, _resolved);
			return;
		}

		// 矩形の左上(アンカーからのずれ)
		const Math::Vector2 _localTopLeft = {
			-a_decoration.pivot.x * _resolved.size.x,
			-a_decoration.pivot.y * _resolved.size.y
		};

		// ---- 塗り ----
		if (a_decoration.isFill)
		{
			// 板ポリは組み込みの白テクスチャを色で染める。
			// 画像は届くまで描かない(空ハンドルで積むとテクスチャ取得で落ちる)
			Engine::Handle<Engine::Resource::Texture> _texHandle = {};

			if (a_decoration.type == EDecorationType::Image)
			{
				if (a_pResourceManager->IsReady(a_decoration.texRef))
				{
					_texHandle = a_decoration.texRef.GetRaw();
				}
			}
			else
			{
				_texHandle = GetWhiteTexture();
			}

			if (_texHandle.IsValid())
			{
				SubmitLocalRect(
					a_pGraphicsEngine, _texHandle, _resolved,
					_localTopLeft, _resolved.size,
					_resolved.color, _resolved.uvOffset, _resolved.uvScale
				);
			}
		}

		// ---- 枠 ----
		// 塗りと同じ掛かり具合(親の色・その場の色・アニメ・反応)を通す。
		// ここを飛ばすと、反応で消したはずの枠だけ残ってしまう
		const Math::Color _edgeColor = _resolved.modulate * a_decoration.edgeColor;
		DrawEdge(a_pGraphicsEngine, a_decoration, _resolved, _localTopLeft, _edgeColor);
	}

	//======================================================================================
	// GUIDから参照を引き直す
	//======================================================================================
	void RequestResources(Decoration& a_decoration, Engine::Resource::ResourceManager* a_pResourceManager)
	{
		if (a_pResourceManager == nullptr) return;

		// 実体の到着は待たない。描画側が IsReady を見てスキップする
		a_decoration.texRef = a_decoration.texGUID.IsValid()
			? a_pResourceManager->RequestLoad<Engine::Resource::Texture>(a_decoration.texGUID)
			: Engine::ResourceRef<Engine::Resource::Texture>();

		a_decoration.fontRef = a_decoration.fontGUID.IsValid()
			? a_pResourceManager->RequestLoad<Engine::Resource::Font>(a_decoration.fontGUID)
			: Engine::ResourceRef<Engine::Resource::Font>();
	}

	//======================================================================================
	// アーカイブ
	//======================================================================================
	namespace
	{
		template<typename T>
		void ArchiveAnimElement(Engine::Persistence::Archive& a_ar, const std::string& a_name, AnimElement<T>& a_element)
		{
			a_ar.Field(a_name + "Start", a_element.start);
			a_ar.Field(a_name + "End", a_element.end);
		}

		void ArchiveStateStyle(Engine::Persistence::Archive& a_ar, const std::string& a_name, UIStateStyle& a_style)
		{
			a_ar.Field(a_name + "Color", a_style.color);
			a_ar.Field(a_name + "Scale", a_style.scale);
			a_ar.Field(a_name + "Offset", a_style.offsetAdd);
		}

		template<typename T>
		void ArchiveOscillation(Engine::Persistence::Archive& a_ar, const std::string& a_name, Oscillation<T>& a_oscillation)
		{
			a_ar.Field(a_name + "Amplitude", a_oscillation.amplitude);
			a_ar.Field(a_name + "Frequency", a_oscillation.frequency);
			a_ar.Field(a_name + "Phase", a_oscillation.phase);

			a_ar.Field(a_name + "UseLimit", a_oscillation.isUseLimit);
			a_ar.Field(a_name + "Min", a_oscillation.minValue);
			a_ar.Field(a_name + "Max", a_oscillation.maxValue);
		}
	}

	void ArchiveDecoration(Engine::Persistence::Archive& a_ar, Decoration& a_decoration)
	{
		a_ar.Field("Type", a_decoration.type);
		a_ar.StringField("Name", a_decoration.name);
		a_ar.Field("IsVisible", a_decoration.isVisible);
		a_ar.Field("Group", a_decoration.group);

		a_ar.Field("OffsetPos", a_decoration.offsetPos);
		a_ar.Field("PixelSize", a_decoration.pixelSize);
		a_ar.Field("Rotation", a_decoration.rotation);
		a_ar.Field("Scale", a_decoration.scale);
		a_ar.Field("Pivot", a_decoration.pivot);
		a_ar.Field("LayerOffset", a_decoration.layerOffset);
		a_ar.Field("Color", a_decoration.color);

		a_ar.Field("UVOffset", a_decoration.uvOffset);
		a_ar.Field("UVScale", a_decoration.uvScale);

		a_ar.GUIDField("TexGUID", a_decoration.texGUID);

		a_ar.Field("IsFill", a_decoration.isFill);
		a_ar.Field("EdgeColor", a_decoration.edgeColor);
		a_ar.Field("EdgePixel", a_decoration.edgePixel);
		a_ar.Field("EdgeSide", a_decoration.edgeSide);

		a_ar.StringField("Text", a_decoration.text);
		a_ar.GUIDField("FontGUID", a_decoration.fontGUID);
		a_ar.Field("FontPixelSize", a_decoration.fontPixelSize);
		a_ar.Field("LineSpacing", a_decoration.lineSpacing);
		a_ar.Field("CharSpacing", a_decoration.charSpacing);
		a_ar.Field("TextAlign", a_decoration.textAlign);

		//----------------------------------------------------------------------------------
		// アニメーション
		//
		// optional は「持っているか」を先に書いてから中身を書く。
		// 持っていない場合は中身を一切読み書きしないので、保存側と読込側で必ず対になる
		//----------------------------------------------------------------------------------
		bool _hasTween = a_decoration.opTweenAnim.has_value();
		a_ar.Field("HasTween", _hasTween);
		if (_hasTween)
		{
			if (!a_decoration.opTweenAnim.has_value()) a_decoration.opTweenAnim = UIAnimation();

			UIAnimation& _anim = *a_decoration.opTweenAnim;
			a_ar.Field("TweenDuration", _anim.durationTime);
			a_ar.Field("TweenIsLoop", _anim.isLoop);
			a_ar.Field("TweenIsPingPong", _anim.isPingPong);
			a_ar.Field("TweenEase", _anim.ease);
			a_ar.Field("TweenChannels", _anim.channels);

			ArchiveAnimElement(a_ar, "TweenColor", _anim.color);
			ArchiveAnimElement(a_ar, "TweenPosition", _anim.position);
			ArchiveAnimElement(a_ar, "TweenScale", _anim.scale);
			ArchiveAnimElement(a_ar, "TweenRotation", _anim.rotation);
			ArchiveAnimElement(a_ar, "TweenUV", _anim.uv);
		}
		else
		{
			a_decoration.opTweenAnim.reset();
		}

		//----------------------------------------------------------------------------------
		// カーソルへの反応
		//----------------------------------------------------------------------------------
		bool _hasReaction = a_decoration.opReaction.has_value();
		a_ar.Field("HasReaction", _hasReaction);
		if (_hasReaction)
		{
			if (!a_decoration.opReaction.has_value()) a_decoration.opReaction = UIReaction();

			UIReaction& _reaction = *a_decoration.opReaction;
			a_ar.Field("ReactionVisibleState", _reaction.visibleState);
			a_ar.Field("ReactionBlendSpeed", _reaction.blendSpeed);

			ArchiveStateStyle(a_ar, "ReactionHovered", _reaction.hovered);
			ArchiveStateStyle(a_ar, "ReactionPressed", _reaction.pressed);
			ArchiveStateStyle(a_ar, "ReactionDisabled", _reaction.disabled);
		}
		else
		{
			a_decoration.opReaction.reset();
		}

		bool _hasOscillation = a_decoration.opOscillationAnim.has_value();
		a_ar.Field("HasOscillation", _hasOscillation);
		if (_hasOscillation)
		{
			if (!a_decoration.opOscillationAnim.has_value()) a_decoration.opOscillationAnim = UIProceduralAnimation();

			UIProceduralAnimation& _anim = *a_decoration.opOscillationAnim;
			a_ar.Field("OscChannels", _anim.channels);

			ArchiveOscillation(a_ar, "OscPosition", _anim.position);
			ArchiveOscillation(a_ar, "OscScale", _anim.scale);
			ArchiveOscillation(a_ar, "OscRotation", _anim.rotation);
			ArchiveOscillation(a_ar, "OscColor", _anim.color);
		}
		else
		{
			a_decoration.opOscillationAnim.reset();
		}
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	namespace
	{
		// 値の種類ごとの編集
		bool DrawFloatValue(const char* a_label, float& a_value)
		{
			return ImGui::DragFloat(a_label, &a_value, 0.1f);
		}
		bool DrawVectorValue(const char* a_label, Math::Vector2& a_value)
		{
			return ImGui::DragFloat2(a_label, &a_value.x, 0.1f);
		}
		bool DrawColorValue(const char* a_label, Math::Color& a_value)
		{
			return Engine::Editor::EditorHelper::DrawColorEdit(a_label, a_value);
		}

		// 状態1つぶんの見た目
		bool DrawStateStyleUI(const char* a_label, UIStateStyle& a_style)
		{
			if (!ImGui::TreeNode(a_label)) return false;

			bool _isChanged = false;

			if (Engine::Editor::EditorHelper::DrawColorEdit("Color", a_style.color)) _isChanged = true;
			ImGui::SetItemTooltip("元の色へ乗算(白で変化なし)");

			if (ImGui::DragFloat2("Scale", &a_style.scale.x, 0.01f, 0.0f, 16.0f)) _isChanged = true;
			ImGui::SetItemTooltip("大きさへ乗算(1で等倍)");

			if (ImGui::DragFloat2("Offset", &a_style.offsetAdd.x, 0.5f)) _isChanged = true;
			ImGui::SetItemTooltip("位置へ加算(px)");

			ImGui::TreePop();

			return _isChanged;
		}

		// トゥイーンの1チャンネルぶん
		template<typename T, typename DrawFunc>
		bool DrawAnimElementUI(
			const char* a_label,
			EAnimChannel a_channel,
			EAnimChannel& a_inoutChannels,
			AnimElement<T>& a_element,
			DrawFunc a_drawFunc)
		{
			bool _isChanged = false;

			// 立てたチャンネルだけ中身を出す。
			// 動かないものの値を触らせても混乱するだけなので畳んでおく
			bool _isOn = Engine::Utility::HasFlag(a_inoutChannels, a_channel);
			if (ImGui::Checkbox(a_label, &_isOn))
			{
				a_inoutChannels = _isOn
					? (a_inoutChannels | a_channel)
					: (a_inoutChannels & ~a_channel);
				_isChanged = true;
			}
			if (!_isOn) return _isChanged;

			ImGui::Indent();
			ImGui::PushID(a_label);
			if (a_drawFunc("Start", a_element.start)) _isChanged = true;
			if (a_drawFunc("End", a_element.end))     _isChanged = true;
			ImGui::PopID();
			ImGui::Unindent();

			return _isChanged;
		}

		// 揺れの1チャンネルぶん
		//
		// a_defaultMin / a_defaultMax は、上下限を初めて立てたときに入れておく値。
		// 既定の 0 のままだと、掛けるチャンネル(大きさ・色)が真っ黒に潰れて
		// 「立てた瞬間に消えた」ように見えるため
		template<typename T, typename DrawFunc>
		bool DrawOscillationUI(
			const char* a_label,
			EAnimChannel a_channel,
			EAnimChannel& a_inoutChannels,
			Oscillation<T>& a_oscillation,
			DrawFunc a_drawFunc,
			const T& a_defaultMin,
			const T& a_defaultMax)
		{
			bool _isChanged = false;

			bool _isOn = Engine::Utility::HasFlag(a_inoutChannels, a_channel);
			if (ImGui::Checkbox(a_label, &_isOn))
			{
				a_inoutChannels = _isOn
					? (a_inoutChannels | a_channel)
					: (a_inoutChannels & ~a_channel);
				_isChanged = true;
			}
			if (!_isOn) return _isChanged;

			ImGui::Indent();
			ImGui::PushID(a_label);

			if (ImGui::Checkbox("UseLimit", &a_oscillation.isUseLimit))
			{
				// まだ一度も触っていないときだけ入れる。
				// 切って入れ直すたびに上書きすると、調整した値が消えてしまう
				if (a_oscillation.isUseLimit &&
					a_oscillation.minValue == T{} && a_oscillation.maxValue == T{})
				{
					a_oscillation.minValue = a_defaultMin;
					a_oscillation.maxValue = a_defaultMax;
				}
				_isChanged = true;
			}
			ImGui::SetItemTooltip("振れ幅ではなく、届く範囲(下限〜上限)で指定する");

			if (a_oscillation.isUseLimit)
			{
				if (a_drawFunc("Min", a_oscillation.minValue)) _isChanged = true;
				if (a_drawFunc("Max", a_oscillation.maxValue)) _isChanged = true;
			}
			else
			{
				if (a_drawFunc("Amplitude", a_oscillation.amplitude)) _isChanged = true;
			}

			if (ImGui::DragFloat("Frequency", &a_oscillation.frequency, 0.01f, 0.0f, 60.0f)) _isChanged = true;
			if (ImGui::DragFloat("Phase", &a_oscillation.phase, 0.01f, 0.0f, 1.0f))          _isChanged = true;

			ImGui::PopID();
			ImGui::Unindent();

			return _isChanged;
		}
	}

	bool DrawDecorationInspector(Decoration& a_decoration, Engine::Resource::ResourceManager* a_pResourceManager)
	{
		bool _isChanged = false;

		//----------------------------------------------------------------------------------
		// 共通
		//----------------------------------------------------------------------------------
		if (ImGui::Checkbox("Visible", &a_decoration.isVisible)) _isChanged = true;
		ImGui::SameLine();
		if (ImGui::InputText("Name", &a_decoration.name)) _isChanged = true;

		if (Engine::Editor::EditorHelper::DrawEnumCombo("Type", a_decoration.type)) _isChanged = true;

		int _group = static_cast<int>(a_decoration.group);
		if (ImGui::DragInt("Group", &_group, 1, 0, 15))
		{
			a_decoration.group = static_cast<uint32_t>(std::max(_group, 0));
			_isChanged = true;
		}
		ImGui::SetItemTooltip("HUDが飾りを出し分けるための札 (TargetBoxHUD : 0=通常枠 / 1=ロック枠)");

		ImGui::Spacing();
		ImGui::SeparatorText("Transform (親からの相対)");

		if (ImGui::DragFloat2("OffsetPos", &a_decoration.offsetPos.x, 1.0f)) _isChanged = true;
		ImGui::SetItemTooltip("親のピボット位置からのずれ(px)");

		// 文字の大きさは FontPixelSize が決めるので、矩形の大きさは出さない
		if (a_decoration.type != EDecorationType::Text)
		{
			if (ImGui::DragFloat2("PixelSize", &a_decoration.pixelSize.x, 1.0f, 0.0f, 8192.0f)) _isChanged = true;
		}

		if (ImGui::DragFloat("Rotation", &a_decoration.rotation, 0.1f, -360.0f, 360.0f)) _isChanged = true;
		if (ImGui::DragFloat("Scale", &a_decoration.scale, 0.01f, 0.0f, 64.0f)) _isChanged = true;
		if (ImGui::DragFloat2("Pivot (0-1)", &a_decoration.pivot.x, 0.01f, 0.0f, 1.0f)) _isChanged = true;
		if (ImGui::DragFloat("LayerOffset", &a_decoration.layerOffset, 0.1f)) _isChanged = true;
		ImGui::SetItemTooltip("親のレイヤーへ足す。大きいほど手前");
		if (Engine::Editor::EditorHelper::DrawColorEdit("Color", a_decoration.color)) _isChanged = true;

		ImGui::Spacing();

		//----------------------------------------------------------------------------------
		// 種類ごと
		//----------------------------------------------------------------------------------
		switch (a_decoration.type)
		{
		case EDecorationType::Image:
		{
			ImGui::SeparatorText("Image");

			if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Texture", "Texture", a_decoration.texGUID))
			{
				RequestResources(a_decoration, a_pResourceManager);
				_isChanged = true;
			}
			Engine::Editor::EditorHelper::DrawTexture(a_decoration.texRef, 128, 128);

			if (ImGui::DragFloat2("UVOffset", &a_decoration.uvOffset.x, 0.01f)) _isChanged = true;
			if (ImGui::DragFloat2("UVScale", &a_decoration.uvScale.x, 0.01f)) _isChanged = true;
			ImGui::SetItemTooltip("1枚に並べた絵から1コマ切り出すときの倍率 (uv * UVScale + UVOffset)");
			break;
		}

		case EDecorationType::Text:
		{
			ImGui::SeparatorText("Text");

			if (ImGui::InputTextMultiline("Text", &a_decoration.text)) _isChanged = true;

			if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Font", "Font", a_decoration.fontGUID))
			{
				RequestResources(a_decoration, a_pResourceManager);
				_isChanged = true;
			}

			if (ImGui::DragFloat("FontPixelSize", &a_decoration.fontPixelSize, 0.5f, 1.0f, 512.0f)) _isChanged = true;
			ImGui::SetItemTooltip("フォントは64pxで焼いてあるので、それより大きくするとぼやける");

			if (ImGui::DragFloat("LineSpacing", &a_decoration.lineSpacing, 0.01f, 0.1f, 4.0f)) _isChanged = true;
			if (ImGui::DragFloat("CharSpacing", &a_decoration.charSpacing, 0.1f)) _isChanged = true;
			if (Engine::Editor::EditorHelper::DrawEnumCombo("TextAlign", a_decoration.textAlign)) _isChanged = true;
			ImGui::TextDisabled("ブロック全体の位置は Pivot、行同士の揃えが TextAlign");
			break;
		}

		case EDecorationType::Polygon:
		default:
			ImGui::SeparatorText("Polygon");
			ImGui::TextDisabled("組み込みの白テクスチャを Color で染めて出します");
			break;
		}

		//----------------------------------------------------------------------------------
		// 枠(文字以外)
		//----------------------------------------------------------------------------------
		if (a_decoration.type != EDecorationType::Text)
		{
			ImGui::Spacing();
			ImGui::SeparatorText("Edge");

			if (ImGui::Checkbox("Fill", &a_decoration.isFill)) _isChanged = true;
			ImGui::SetItemTooltip("切ると枠だけになる");

			if (ImGui::DragFloat("EdgePixel", &a_decoration.edgePixel, 0.5f, 0.0f, 256.0f)) _isChanged = true;
			if (a_decoration.edgePixel > 0.0f)
			{
				if (Engine::Editor::EditorHelper::DrawColorEdit("EdgeColor", a_decoration.edgeColor)) _isChanged = true;
				Engine::Editor::EditorHelper::DrawEnumFlagsCombo("EdgeSide", a_decoration.edgeSide);
			}
		}

		//----------------------------------------------------------------------------------
		// カーソルへの反応
		//----------------------------------------------------------------------------------
		ImGui::Spacing();
		ImGui::SeparatorText("Reaction");

		bool _hasReaction = a_decoration.opReaction.has_value();
		if (ImGui::Checkbox("Reaction", &_hasReaction))
		{
			if (_hasReaction) a_decoration.opReaction = UIReaction();
			else              a_decoration.opReaction.reset();
			_isChanged = true;
		}
		ImGui::SetItemTooltip("親のUIにカーソルが乗った / 押されたときに反応する");

		if (a_decoration.opReaction.has_value())
		{
			UIReaction& _reaction = *a_decoration.opReaction;

			ImGui::Indent();
			ImGui::PushID("Reaction");

			Engine::Editor::EditorHelper::DrawEnumFlagsCombo("VisibleState", _reaction.visibleState);
			ImGui::TextDisabled("この状態のときだけ出す(カーソル時だけ枠を出す等)");

			if (ImGui::DragFloat("BlendSpeed", &_reaction.blendSpeed, 0.5f, 0.0f, 120.0f)) _isChanged = true;
			ImGui::SetItemTooltip("切り替わりの速さ。0 で即時");

			if (DrawStateStyleUI("Hovered", _reaction.hovered))  _isChanged = true;
			if (DrawStateStyleUI("Pressed", _reaction.pressed))  _isChanged = true;
			if (DrawStateStyleUI("Disabled", _reaction.disabled)) _isChanged = true;

			ImGui::PopID();
			ImGui::Unindent();
		}

		//----------------------------------------------------------------------------------
		// アニメーション
		//----------------------------------------------------------------------------------
		ImGui::Spacing();
		ImGui::SeparatorText("Animation");

		// ---- トゥイーン ----
		bool _hasTween = a_decoration.opTweenAnim.has_value();
		if (ImGui::Checkbox("Tween", &_hasTween))
		{
			if (_hasTween) a_decoration.opTweenAnim = UIAnimation();
			else           a_decoration.opTweenAnim.reset();
			_isChanged = true;
		}

		if (a_decoration.opTweenAnim.has_value())
		{
			UIAnimation& _anim = *a_decoration.opTweenAnim;

			ImGui::Indent();
			ImGui::PushID("Tween");

			if (ImGui::DragFloat("Duration", &_anim.durationTime, 0.01f, 0.0f, 60.0f)) _isChanged = true;
			if (ImGui::Checkbox("Loop", &_anim.isLoop)) _isChanged = true;
			ImGui::SameLine();
			if (ImGui::Checkbox("PingPong", &_anim.isPingPong)) _isChanged = true;
			if (Engine::Editor::EditorHelper::DrawEnumCombo("Ease", _anim.ease)) _isChanged = true;

			ImGui::Spacing();
			ImGui::TextDisabled("チェックを入れたチャンネルだけが動きます");

			if (DrawAnimElementUI("Color##ch", EAnimChannel::COLOR, _anim.channels, _anim.color, DrawColorValue)) _isChanged = true;
			if (DrawAnimElementUI("Position##ch", EAnimChannel::POSITION, _anim.channels, _anim.position, DrawVectorValue)) _isChanged = true;
			if (DrawAnimElementUI("Scale##ch", EAnimChannel::SCALE, _anim.channels, _anim.scale, DrawVectorValue)) _isChanged = true;
			if (DrawAnimElementUI("Rotation##ch", EAnimChannel::ROTATION, _anim.channels, _anim.rotation, DrawFloatValue)) _isChanged = true;
			if (DrawAnimElementUI("UV##ch", EAnimChannel::UV, _anim.channels, _anim.uv, DrawVectorValue)) _isChanged = true;

			ImGui::Spacing();
			if (ImGui::Button("Replay")) _anim.currentTime = 0.0f;
			ImGui::SameLine();
			ImGui::Text("%.2f / %.2f", _anim.currentTime, _anim.durationTime);

			ImGui::PopID();
			ImGui::Unindent();
		}

		// ---- 揺れ ----
		bool _hasOscillation = a_decoration.opOscillationAnim.has_value();
		if (ImGui::Checkbox("Oscillation", &_hasOscillation))
		{
			if (_hasOscillation) a_decoration.opOscillationAnim = UIProceduralAnimation();
			else                 a_decoration.opOscillationAnim.reset();
			_isChanged = true;
		}

		if (a_decoration.opOscillationAnim.has_value())
		{
			UIProceduralAnimation& _anim = *a_decoration.opOscillationAnim;

			ImGui::Indent();
			ImGui::PushID("Oscillation");

			ImGui::TextDisabled("位置と回転は足す量、大きさと色は掛ける量(1が元のまま)");

			// 上下限を立てたときの初期値 : そのチャンネルらしい範囲を入れておく
			if (DrawOscillationUI("Position##osc", EAnimChannel::POSITION, _anim.channels, _anim.position, DrawVectorValue,
				Math::Vector2(-10.0f, -10.0f), Math::Vector2(10.0f, 10.0f))) _isChanged = true;

			if (DrawOscillationUI("Scale##osc", EAnimChannel::SCALE, _anim.channels, _anim.scale, DrawVectorValue,
				Math::Vector2(0.9f, 0.9f), Math::Vector2(1.1f, 1.1f))) _isChanged = true;

			if (DrawOscillationUI("Rotation##osc", EAnimChannel::ROTATION, _anim.channels, _anim.rotation, DrawFloatValue,
				-10.0f, 10.0f)) _isChanged = true;

			if (DrawOscillationUI("Color##osc", EAnimChannel::COLOR, _anim.channels, _anim.color, DrawColorValue,
				Math::Color(1.0f, 1.0f, 1.0f, 0.3f), Math::Color(1.0f, 1.0f, 1.0f, 1.0f))) _isChanged = true;

			ImGui::PopID();
			ImGui::Unindent();
		}

		return _isChanged;
	}
}
