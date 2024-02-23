#ifndef SIGNAL_HPP
#define SIGNAL_HPP
#include <vector>
#include <functional>
#include <map>

#define CONNECT_SLOT_1(signalName, memberFunc, objPtr) \
    signalName.Connect(std::bind(&std::remove_reference<decltype(*objPtr)>::type::memberFunc, objPtr, std::placeholders::_1))
#define CONNECT_SLOT_2(signalName, memberFunc, objPtr) \
    signalName.Connect(std::bind(&std::remove_reference<decltype(*objPtr)>::type::memberFunc, objPtr, std::placeholders::_1, std::placeholders::_2))

template <typename... Args>
class Signal
{
public:
    using Slot = std::function<void(Args...)>;
    using SlotID = uint8_t; // ID type for Slots

    SlotID Connect(Slot slot)
    {

        SlotID newId = nextSlotId++;
        m_slots.insert({newId, slot});
        return newId;
    }

    void Disconnect(SlotID id)
    {
        m_slots.erase(id);
    }

    void DisconnectAll()
    {
        m_slots.clear();
    }

    void Emit(Args... args)
    {
        for (const auto &idSlotPair : m_slots)
        {
            idSlotPair.second(args...);
        }
    }

private:
    std::map<SlotID, Slot> m_slots;
    uint8_t nextSlotId = 0; // This will auto-increment for each new Slot
};
#endif // SIGNAL_HPP