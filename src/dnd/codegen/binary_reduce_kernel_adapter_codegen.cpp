//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dnd/codegen/binary_reduce_kernel_adapter_codegen.hpp>

using namespace std;
using namespace dnd;

static unsigned int get_arg_id_from_type_id(unsigned int type_id)
{
    switch (type_id) {
        case bool_type_id:
        case int8_type_id:
        case uint8_type_id:
            return 0;
        case int16_type_id:
        case uint16_type_id:
            return 1;
        case int32_type_id:
        case uint32_type_id:
            return 2;
        case int64_type_id:
        case uint64_type_id:
            return 3;
        case float32_type_id:
            return 4;
        case float64_type_id:
            return 5;
        default: {
            stringstream ss;
            ss << "The unary_kernel_adapter does not support " << dtype(type_id) << " for the return type";
            throw runtime_error(ss.str());
        }
    }
}

uint64_t dnd::get_binary_reduce_function_adapter_unique_id(const dtype& reduce_type, calling_convention_t callconv)
{
    uint64_t result = get_arg_id_from_type_id(reduce_type.type_id());

#if defined(_WIN32) && !defined(_M_X64)
    // For 32-bit Windows, support both cdecl and stdcall
    result += (uint64_t)callconv << 4;
#endif

    return result;
}

std::string dnd::get_binary_reduce_function_adapter_unique_id_string(uint64_t unique_id)
{
    stringstream ss;
    static const char *arg_types[8] = {"int8", "int16", "int32", "int64", "float32", "float64", "(invalid)", "(invalid)"};
    const char * reduce_type = arg_types[unique_id & 0x07];
    ss << reduce_type << " (";
    ss << reduce_type << ", ";
    ss << reduce_type << ")";
    return ss.str();
}

namespace {
    template<class T>
    struct binary_reduce_function_adapters {
        typedef T (*cdecl_func_ptr_t)(T, T);

        static void left_associative(char *dst, intptr_t dst_stride,
                        const char *src0, intptr_t src0_stride,
                        intptr_t count, const AuxDataBase *auxdata)
        {
            cdecl_func_ptr_t kfunc = get_auxiliary_data<cdecl_func_ptr_t>(auxdata);
            if (dst_stride == 0) {
                T dst_value = *reinterpret_cast<T *>(dst);
                for (intptr_t i = 0; i < count; ++i) {
                    dst_value = kfunc(dst_value, *reinterpret_cast<const T *>(src0));
                    src0 += src0_stride;
                }
                *reinterpret_cast<T *>(dst) = dst_value;
            } else {
                for (intptr_t i = 0; i < count; ++i) {
                    *reinterpret_cast<T *>(dst) = kfunc(*reinterpret_cast<T *>(dst),
                                                    *reinterpret_cast<const T *>(src0));
                    src0 += src0_stride;
                }
            }
        }

        static void right_associative(char *dst, intptr_t dst_stride,
                        const char *src0, intptr_t src0_stride,
                        intptr_t count, const AuxDataBase *auxdata)
        {
            cdecl_func_ptr_t kfunc = get_auxiliary_data<cdecl_func_ptr_t>(auxdata);
            if (dst_stride == 0) {
                T dst_value = *reinterpret_cast<T *>(dst);
                for (intptr_t i = 0; i < count; ++i) {
                    dst_value = kfunc(*reinterpret_cast<const T *>(src0), dst_value);
                    src0 += src0_stride;
                }
                *reinterpret_cast<T *>(dst) = dst_value;
            } else {
                for (intptr_t i = 0; i < count; ++i) {
                    *reinterpret_cast<T *>(dst) = kfunc(*reinterpret_cast<const T *>(src0),
                                                    *reinterpret_cast<T *>(dst));
                    src0 += src0_stride;
                }
            }
        }
    };
} // anonymous namespace

unary_operation_t dnd::codegen_left_associative_binary_reduce_function_adapter(
                    const dtype& reduce_type, calling_convention_t DND_UNUSED(callconv))
{
    // TODO: If there's a platform where there are differences in the calling convention
    //       between the equated types, this will have to change.
    switch(reduce_type.type_id()) {
        case bool_type_id:
        case int8_type_id:
        case uint8_type_id:
            return &binary_reduce_function_adapters<int8_t>::left_associative;
        case int16_type_id:
        case uint16_type_id:
            return &binary_reduce_function_adapters<int16_t>::left_associative;
        case int32_type_id:
        case uint32_type_id:
            return &binary_reduce_function_adapters<int32_t>::left_associative;
        case int64_type_id:
        case uint64_type_id:
            return &binary_reduce_function_adapters<int64_t>::left_associative;
        case float32_type_id:
            return &binary_reduce_function_adapters<float>::left_associative;
        case float64_type_id:
            return &binary_reduce_function_adapters<double>::left_associative;
        default: {
            stringstream ss;
            ss << "The binary reduce function adapter does not support " << reduce_type;
            throw runtime_error(ss.str());
        }
    }
}

unary_operation_t dnd::codegen_right_associative_binary_reduce_function_adapter(
                    const dtype& reduce_type, calling_convention_t DND_UNUSED(callconv))
{
    // TODO: If there's a platform where there are differences in the calling convention
    //       between the equated types, this will have to change.
    switch(reduce_type.type_id()) {
        case bool_type_id:
        case int8_type_id:
        case uint8_type_id:
            return &binary_reduce_function_adapters<int8_t>::right_associative;
        case int16_type_id:
        case uint16_type_id:
            return &binary_reduce_function_adapters<int16_t>::right_associative;
        case int32_type_id:
        case uint32_type_id:
            return &binary_reduce_function_adapters<int32_t>::right_associative;
        case int64_type_id:
        case uint64_type_id:
            return &binary_reduce_function_adapters<int64_t>::right_associative;
        case float32_type_id:
            return &binary_reduce_function_adapters<float>::right_associative;
        case float64_type_id:
            return &binary_reduce_function_adapters<double>::right_associative;
        default: {
            stringstream ss;
            ss << "The binary reduce function adapter does not support " << reduce_type;
            throw runtime_error(ss.str());
        }
    }
}
