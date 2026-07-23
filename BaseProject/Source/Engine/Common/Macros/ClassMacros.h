#pragma once

// コピー禁止 - ムーブ許可
#define NON_COPYABLE_MOVABLE(Type) \
    Type(const Type&) = delete; \
    Type& operator=(const Type&) = delete; \
    Type(Type&&) noexcept = default; \
    Type& operator=(Type&&) noexcept = default;

// コピー禁止 - ムーブ禁止
#define NON_COPYABLE_NON_MOVABLE(Type) \
	Type(const Type&) = delete; \
	Type& operator=(const Type&) = delete; \
	Type(Type&&) noexcept = delete; \
	Type& operator=(Type&&) noexcept = delete;