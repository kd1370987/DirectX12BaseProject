#pragma once

// コピームーブ禁止
#define NON_COPYABLE_MOVABLE(Type) \
	Type() = default; \
    Type(const Type&) = delete; \
    Type& operator=(const Type&) = delete; \
    Type(Type&&) noexcept = default; \
    Type& operator=(Type&&) noexcept = default;
