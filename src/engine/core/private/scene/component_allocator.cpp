#include "scene/component_allocator.hpp"

#include "class.hpp"
#include "logger.hpp"
#include "object_ptr.hpp"

void* Contiguous::allocate()
{
    reserve(component_count + 1);
    void* new_ptr = memory + (component_count * object_class->stride());
    std::memset(new_ptr, 0, object_class->stride());
    component_count += 1;
    return new_ptr;
}

void Contiguous::free(void* ptr)
{
    if (auto obj_ptr = allocation_map.find(ptr); obj_ptr != allocation_map.end())
    {
        obj_ptr->second->ptr = nullptr;
        allocation_map.erase(obj_ptr);
        component_count--;
        reserve(component_count);
    }
    else
        LOG_FATAL("Allocation {:x} is not allocated in this pool ({})", reinterpret_cast<size_t>(ptr), object_class->name())
}

void Contiguous::reserve(size_t desired_count)
{
    if (desired_count == 0)
        resize(0);

    if (desired_count < static_cast<size_t>(static_cast<double>(allocated_count) / 1.5) || desired_count > static_cast<size_t>(static_cast<double>(allocated_count) * 1.5))
        resize(desired_count);
}

void Contiguous::resize(size_t new_count)
{
    if (new_count < component_count)
        LOG_FATAL("Cannot resize Object pool bellow it's component count");

    if (new_count == 0)
    {
        move_old_to_new_block(memory, nullptr);
        free(memory);
        memory          = nullptr;
        allocated_count = 0;
        component_count = 0;
    }
    else
    {
        if (new_count != allocated_count)
        {
            void* new_memory = realloc(memory, new_count * object_class->stride());
            if (new_memory != memory)
                LOG_FATAL("Failed to allocate memory for object {}", object_class->name())

            if (memory != new_memory)
                move_old_to_new_block(memory, new_memory);
            memory          = new_memory;
            allocated_count = new_count;
        }
    }
}

void Contiguous::move_old_to_new_block(void* old, void* new_block)
{
    if (!old)
        return;

    size_t stride = object_class->stride();
    for (size_t i = 0; i < component_count; ++i)
    {
        void* old_ptr = old + (i * stride);
        void* new_ptr = new_block + (i * stride);

        if (auto found         = allocation_map.find(old_ptr); found != allocation_map.end())
            found->second->ptr = new_ptr;
    }
}